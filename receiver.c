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

void cleanBuffer(pkt_t ** waitingInBuffer, uint8_t toBeRemoved){
	// if this paquet is in the buffer, it will be removed
	for(int i =0; i < MAXWIN; i++){
		if(waitingInBuffer[i] != NULL){
			if(pkt_get_seqnum(waitingInBuffer[i]) == toBeRemoved){
				pkt_del(waitingInBuffer[i]);
				waitingInBuffer[i] = NULL;
				return;
			}
		}
	}
}
void addTobuffer(pkt_t ** waitingInBuffer, pkt_t * rcvdPkt){
	//adds a packet in the buffer, if there is enough free space
	for(int i =0; i < MAXWIN; i++){
		if(waitingInBuffer[i] == NULL){
			waitingInBuffer[i] = rcvdPkt;
			return;
		}
	}
}
int checkBuffer(pkt_t ** waitingInBuffer, uint8_t nextSeqnum, FILE * fp){
	//checks the buffer for paquet that can be written in the file (if its seqNum is the next expected seqNum)
	for(int i =0; i < MAXWIN; i++){
		if(waitingInBuffer[i] != NULL && pkt_get_seqnum(waitingInBuffer[i]) == nextSeqnum){
			fwrite(pkt_get_payload(waitingInBuffer[i]),1,pkt_get_length(waitingInBuffer[i]),fp);
			pkt_del(waitingInBuffer[i]);
			waitingInBuffer[i] = NULL;
			nextSeqnum++;
			if(nextSeqnum > 255){
				nextSeqnum = 0;
			}
			nextSeqnum = checkBuffer(waitingInBuffer, nextSeqnum, fp);
			return nextSeqnum;
		}
	}
	return nextSeqnum;
}
int getFreeSpace(pkt_t ** waitingInBuffer){
	uint8_t freeSpace = 0;
	for(int i =0; i < MAXWIN; i++){
		if(waitingInBuffer[i] == NULL){
			freeSpace++;
		}
	}
	return freeSpace;
}
void sendAck(int sockFd, struct sockaddr_in6 * si_other, uint8_t nextSeqnum, uint8_t isAck, uint8_t window){
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
	size_t len = 12;
	char* buff = malloc(len*sizeof(char));
	pkt_encode(pkt, buff, &len);
    int sent = sendto(sockFd, buff, len,0,(struct sockaddr *) &si_other, sizeof(si_other));
    free(buff);
	return;
}

void listenLoop(int sockFd, struct sockaddr_in6 * si_other, void * fileToWrite){
	//il faudrait verifier que le paquet recu est bien dans notre window avant de mettre en buffer (pas seqnum = expected+200)
	int receivedLastPaquet = 0;
	char buf[MAXPKTSIZE];
	int slen = sizeof(si_other);
	int rcvlen = 0;
    uint8_t nextSeqnum = 0;
	pkt_t * rcvdPkt = pkt_new();
	FILE * fp;
	pkt_t * waitingInBuffer[MAXWIN] = {NULL};
	uint8_t waiting = 0;
    if(fileToWrite!=NULL){
        fp = fopen(fileToWrite, "wa"); //was "wb" pour binary essayer avec b
    }
    unsigned int lastAckSent = 0;
	while(!receivedLastPaquet){
		bzero(buf, MAXPKTSIZE);
		rcvlen = recv(sockFd, buf, MAXPKTSIZE, 0);
		if (rcvlen == -1){
				return;
		}
		pkt_del(rcvdPkt);
		rcvdPkt = pkt_new();
		if(pkt_decode(buf, rcvlen, rcvdPkt) != PKT_OK){
			if(pkt_get_type(rcvdPkt) == PTYPE_DATA && pkt_get_tr(rcvdPkt) == 1){
				//send NACK nextSeqnum est le seqnum du recu qui est tronque
				sendAck(sockFd, si_other, pkt_get_seqnum(rcvdPkt), 0, getFreeSpace(waitingInBuffer));
				printf("NACK SENT\n");
				lastAckSent = nextSeqnum;
			}
			else{
				if(pkt_get_type(rcvdPkt) == PTYPE_DATA && pkt_get_length(rcvdPkt) == 0 && pkt_get_seqnum(rcvdPkt) == lastAckSent){
					//send ACK + close connection
					sendAck(sockFd, si_other, ++nextSeqnum%255, 1, getFreeSpace(waitingInBuffer));
					printf("ACK SENT\n");

					receivedLastPaquet = 1;
				}
				cleanBuffer(waitingInBuffer, (uint8_t)pkt_get_seqnum(rcvdPkt));
				if((uint8_t)pkt_get_seqnum(rcvdPkt) == nextSeqnum){ //le paquet qu on attendait
					fwrite(pkt_get_payload(rcvdPkt),1,pkt_get_length(rcvdPkt),fp);
				}
				else{ // pas le paquet attendu
					if((uint8_t)pkt_get_seqnum(rcvdPkt) > nextSeqnum){ //il est pas encore traité: on le met en buffer
						cleanBuffer(waitingInBuffer, (uint8_t)pkt_get_seqnum(rcvdPkt));
						addTobuffer(waitingInBuffer, rcvdPkt);
					}				
				}
				nextSeqnum = checkBuffer(waitingInBuffer, nextSeqnum, fp);
				//send ACK (if window = 0 : don t send)
				sendAck(sockFd, si_other, nextSeqnum, 1, getFreeSpace(waitingInBuffer));
				printf("ACK SENT\n");

				lastAckSent = nextSeqnum;
			}
		}
		// receivedLastPaquet = 1; //remove this
	}
	return;
}

int main(int argc, char* argv[]){
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
		printf("bind failed\n");
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
	uint8_t waiting = 0;
    if(fileToWrite!=NULL){
        fp = fopen(fileToWrite, "wa"); //was "wb" pour binary essayer avec b
    }
    unsigned int lastAckSent = 0;
	while(!receivedLastPaquet){
		bzero(buf, MAXPKTSIZE);
		rcvlen = recv(sockFd, buf, MAXPKTSIZE, 0);
		if (rcvlen == -1){
				return 1;
		}
		pkt_del(rcvdPkt);
		rcvdPkt = pkt_new();
		if(pkt_decode(buf, rcvlen, rcvdPkt) != PKT_OK){
			if(pkt_get_type(rcvdPkt) == PTYPE_DATA && pkt_get_tr(rcvdPkt) == 1){
				//send NACK nextSeqnum est le seqnum du recu qui est tronque
				sendAck(sockFd, &si_other, pkt_get_seqnum(rcvdPkt), 0, getFreeSpace(waitingInBuffer));
				printf("NACK SENT\n");
				lastAckSent = nextSeqnum;
			}
			else{
				if(pkt_get_type(rcvdPkt) == PTYPE_DATA && pkt_get_length(rcvdPkt) == 0 && pkt_get_seqnum(rcvdPkt) == lastAckSent){
					//send ACK + close connection
					sendAck(sockFd, &si_other, ++nextSeqnum%255, 1, getFreeSpace(waitingInBuffer));
					printf("ACK SENT\n");

					receivedLastPaquet = 1;
				}
				cleanBuffer(waitingInBuffer, (uint8_t)pkt_get_seqnum(rcvdPkt));
				if((uint8_t)pkt_get_seqnum(rcvdPkt) == nextSeqnum){ //le paquet qu on attendait
					fwrite(pkt_get_payload(rcvdPkt),1,pkt_get_length(rcvdPkt),fp);
				}
				else{ // pas le paquet attendu
					if((uint8_t)pkt_get_seqnum(rcvdPkt) > nextSeqnum){ //il est pas encore traité: on le met en buffer
						cleanBuffer(waitingInBuffer, (uint8_t)pkt_get_seqnum(rcvdPkt));
						addTobuffer(waitingInBuffer, rcvdPkt);
					}				
				}
				nextSeqnum = checkBuffer(waitingInBuffer, nextSeqnum, fp);
				//send ACK (if window = 0 : don t send)
				sendAck(sockFd, &si_other, nextSeqnum, 1, getFreeSpace(waitingInBuffer));
				printf("ACK SENT\n");

				lastAckSent = nextSeqnum;
			}
		}
		// receivedLastPaquet = 1; //remove this
	}

	// listenLoop(sockFd, &si_other, fileToWrite);

	shutdown(sockFd, 2);
	return -1;
}