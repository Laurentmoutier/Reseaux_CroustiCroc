#include "packet_interface.h"
#include "packet_interface.c"

#include <stddef.h> /* size_t */
#include <stdint.h> /* uintx_t */
#include <stdio.h>  /* atoi */
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h> 
#include <poll.h>
#define MAXPKTSIZE 528
#define MAXWIN 32

void cleanBuffer(uint8_t * presentInBuffer, pkt_t ** waitingInBuffer, uint8_t toBeRemoved){
	// if this paquet is in the buffer, it will be removed
	int i = 0;
	for(i =0; i < MAXWIN; i++){
		if(presentInBuffer[i] != 0){
			if(pkt_get_seqnum(waitingInBuffer[i]) == toBeRemoved){
				pkt_del(waitingInBuffer[i]);
				waitingInBuffer[i] = NULL;
				presentInBuffer[i] = 0;
				return;
			}
		}
	}
}
void addTobuffer(uint8_t * presentInBuffer, pkt_t ** waitingInBuffer, pkt_t * rcvdPkt){
	//adds a packet in the buffer, if there is enough free space
	int i =0;
	for(i =0; i < MAXWIN; i++){
		if(presentInBuffer[i] == 0){
			presentInBuffer[i] = 1;
			waitingInBuffer[i] = rcvdPkt;
			return;
		}
	}
}
int checkBuffer(uint8_t * presentInBuffer, pkt_t ** waitingInBuffer, uint8_t nextSeqnum, FILE * fp, int interpreter){
	//checks the buffer for paquet that can be written in the file (if its seqNum is the next expected seqNum)
	int i =0;
	for(i =0; i < MAXWIN; i++){
		if(presentInBuffer[i] != 0 && pkt_get_seqnum(waitingInBuffer[i]) == nextSeqnum){
			if(interpreter == 0){
				fwrite(pkt_get_payload(waitingInBuffer[i]),1,pkt_get_length(waitingInBuffer[i]),fp);
			}else{
				fprintf(stdout, "%s", pkt_get_payload(waitingInBuffer[i]));
			}
			pkt_del(waitingInBuffer[i]);
			waitingInBuffer[i] = NULL;
			presentInBuffer[i] = 0;
			nextSeqnum++;
			if(nextSeqnum > 255){
				nextSeqnum = 0;
			}
			nextSeqnum = checkBuffer(presentInBuffer, waitingInBuffer, nextSeqnum, fp, interpreter);
			return nextSeqnum;
		}
	}
	return nextSeqnum;
}

int getFreeSpace(uint8_t * presentInBuffer, pkt_t ** waitingInBuffer){
	int freeSpace = -1;
	int i =0;
	for(i =0; i < MAXWIN; i++){
		if(presentInBuffer[i] == 0){
			freeSpace++;
		}
	}
	if(freeSpace == -1){
		return 0;
	}
	return (uint8_t)freeSpace;
}
void sendAck(int sockFd, struct sockaddr_in6 si_other, uint8_t nextSeqnum, uint8_t isAck, uint8_t window){
	//building pkt
	pkt_t * pkt = pkt_new();
	pkt_status_code typeRet = pkt_set_type(pkt, PTYPE_ACK);
	if (isAck == 0){
		typeRet = pkt_set_type(pkt, PTYPE_NACK);
	}
	pkt_status_code trRet = pkt_set_tr(pkt, 0b0);
	pkt_status_code winRet = pkt_set_window(pkt, window);
	pkt_status_code seqRet = pkt_set_seqnum(pkt, nextSeqnum);
	pkt_status_code lenRet = pkt_set_length(pkt, 0b0000000);
	pkt_status_code timeRet = pkt_set_timestamp(pkt, 0b01111111111111111111111111111110); //todo timestamp
	uint32_t crc1 = 0;
	crc1 = crc32(crc1, (Bytef *)(pkt), sizeof(uint64_t));
	pkt_status_code crc1Ret = pkt_set_crc1(pkt, crc1);
	//encoding paquet and sending it
	size_t len = MAXPKTSIZE;
	char* buff = malloc(len*sizeof(char));
	pkt_status_code pktEnc = pkt_encode(pkt, buff, &len);
    int sent = sendto(sockFd, buff, len,0,(struct sockaddr *) &si_other, sizeof(si_other));
    free(buff);
	return;
}


void testRecv(int sockFd, struct sockaddr_in6 si_me, struct sockaddr_in6 si_other, int slen){
	char buf[MAXPKTSIZE];
	bzero(buf, MAXPKTSIZE);
	char decbuf[12];
	int rcvlen = recvfrom(sockFd, decbuf, 12, 0, (struct sockaddr *) &si_other, &slen);
	printf("received\n");
	int len = 30;
	char * buff = malloc(len);
	strcpy(buff, "hello from receiver");
	sendto(sockFd, buff, len,0,(struct sockaddr *) &si_other, sizeof(si_other));
}

int main(int argc, char* argv[]){
	printf("IL FAUT PRINT SUR L ENTREE STANDARD (QUI PEUT ETRE REDIRIGÉE SUR UN FICHIER)\n");
	int i;
    int interpreter=1;
    void *fileToWrite=NULL, *hostname=NULL, *portPt=NULL;
    uint16_t port;
    int slen;
    for (i=1; i<argc; i++){ // manager exceptions : si arc =1 ou argc >5 ou pas -f er argc>3 on exit : nbr d'aruments trop faible
        if(!strcmp(argv[i], "-f")){ //il faudra prendre en compte le "<"
            i=i+1;
            fileToWrite=argv[i];
            interpreter=0;
        }else if(hostname==NULL){
            hostname=argv[i];
        }else if(portPt==NULL){     
            portPt=argv[i];
            port = atoi((const char*)portPt);
        }
    }
    //connection
    struct sockaddr_in6 si_me, si_other;
	int sockFd = socket(AF_INET6, SOCK_DGRAM, 0);
	if( sockFd == -1){
		return 1;
	}
	memset((char *) &si_me, 0, sizeof(si_me));

	slen = sizeof(si_other);
	si_me.sin6_family = AF_INET6;
	si_me.sin6_port = htons(port);
	si_me.sin6_addr = in6addr_any;

	// si_other.sin6_port = htons(port);
	// si_other.sin6_family = AF_INET6;

	if(bind(sockFd, (struct sockaddr*)&si_me, sizeof(si_me)) == -1){
		// printf("bind failed\n");
		return 2;
	}

	fflush(stdout);

	int receivedLastPaquet = 0;
	char buf[MAXPKTSIZE];
	int rcvlen = 0;
    uint8_t nextSeqnum = 0;
	pkt_t * rcvdPkt = pkt_new();
	FILE * fp;
	pkt_t * waitingInBuffer[MAXWIN] = {NULL};
	uint8_t presentInBuffer[MAXWIN];
	for(i = 0; i< MAXWIN; i++){
		presentInBuffer[i] = 0;
	}
	uint8_t waiting = 0;
    if(interpreter == 0){	
        fp = fopen(fileToWrite, "wa"); //was "wb" pour binary essayer avec b
    }
    unsigned int lastAckSent = 0;
	while(!receivedLastPaquet){
		// testRecv(sockFd, si_me, si_other, slen);
		rcvlen = recvfrom(sockFd, buf, MAXPKTSIZE, 0, (struct sockaddr *) &si_other, &slen);
		if (rcvlen == -1){
				return 1;
		}
		pkt_del(rcvdPkt);
		rcvdPkt = pkt_new();
		int decRet = pkt_decode(buf, rcvlen, rcvdPkt);
		printf("%d is the type received\n",pkt_get_type(rcvdPkt) );
		if(decRet == PKT_OK){
			if(pkt_get_type(rcvdPkt) == PTYPE_DATA && pkt_get_tr(rcvdPkt) == 1){
				//send NACK nextSeqnum est le seqnum du recu qui est tronque
				sendAck(sockFd, si_other, pkt_get_seqnum(rcvdPkt), 0, getFreeSpace(presentInBuffer, waitingInBuffer));
				lastAckSent = nextSeqnum;
			}
			else{
				printf("received seqnum : %u\n", (uint8_t)pkt_get_seqnum(rcvdPkt));
				printf("expected seqnum : %u\n", nextSeqnum);
				if(pkt_get_type(rcvdPkt) == PTYPE_DATA && pkt_get_length(rcvdPkt) == 0 && pkt_get_seqnum(rcvdPkt) == lastAckSent){
					//send ACK + close connection
					sendAck(sockFd, si_other, ++nextSeqnum%255, 1, getFreeSpace(presentInBuffer, waitingInBuffer));
					printf("ENDING CONNECTION (last paquet received)\n");
					//finir la connection que si on a des unacked
					receivedLastPaquet = 1;
				}
				cleanBuffer(presentInBuffer, waitingInBuffer, (uint8_t)pkt_get_seqnum(rcvdPkt));
				if((uint8_t)pkt_get_seqnum(rcvdPkt) == nextSeqnum){ //le paquet qu on attendait
					if(interpreter == 0){
						fwrite(pkt_get_payload(rcvdPkt),1,pkt_get_length(rcvdPkt),fp);
					}else{
						fprintf(stdout, "%s", pkt_get_payload(rcvdPkt));
					}
					nextSeqnum ++;
					if (nextSeqnum > 255){
						nextSeqnum = 0;
					}
				}
				else{ // pas le paquet attendu
					if((uint8_t)pkt_get_seqnum(rcvdPkt) > nextSeqnum){ //il est pas encore traité: on le met en buffer
						cleanBuffer(presentInBuffer, waitingInBuffer, (uint8_t)pkt_get_seqnum(rcvdPkt));
						addTobuffer(presentInBuffer, waitingInBuffer, rcvdPkt);
					}				
				}
				nextSeqnum = checkBuffer(presentInBuffer, waitingInBuffer, nextSeqnum, fp, interpreter);
				//send ACK (if window = 0 : don t send)
				int freeSpace = getFreeSpace(presentInBuffer, waitingInBuffer);
				printf("free space : %d\n", freeSpace);
				sendAck(sockFd, si_other, nextSeqnum, 1, freeSpace);
				printf("ACK SENT\n");
				lastAckSent = nextSeqnum;
			}
		}
		else{
			printf(" decode error : %u\n", decRet);
		}
		// receivedLastPaquet = 1; //remove this
	}
	// listenLoop(sockFd, &si_other, fileToWrite);
	shutdown(sockFd, 2);
	return -1;
}