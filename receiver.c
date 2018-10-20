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
	for(int i =0; i < MAXWIN; i++){
		if(waitingInBuffer[i] == NULL){
			waitingInBuffer[i] = rcvdPkt;
			return;
		}
	}
}
void checkBuffer(pkt_t ** waitingInBuffer, uint8_t nextSeqNum, FILE * fp){
	for(int i =0; i < MAXWIN; i++){
		if(waitingInBuffer[i] != NULL && pkt_get_seqnum(waitingInBuffer[i]) == nextSeqNum){
			fwrite(pkt_get_payload(waitingInBuffer[i]),1,pkt_get_length(waitingInBuffer[i]),fp);
			pkt_del(waitingInBuffer[i]);
			waitingInBuffer[i] = NULL;
			checkBuffer(waitingInBuffer, nextSeqNum+1, fp);
			return;
		}
	}
}


void listenLoop(int sockFd, struct sockaddr_in6 * si_other, void * fileToWrite){
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
        fp = fopen(fileToWrite, "wa"); //was "wb"
    }
	while(!receivedLastPaquet){
		bzero(buf, MAXPKTSIZE);
		rcvlen = recvfrom(sockFd, buf, MAXPKTSIZE, 0, (struct sockaddr *) &si_other, &slen);
		if (rcvlen == -1){
				return;
		}
		pkt_del(rcvdPkt);
		rcvdPkt = pkt_new();
		if(pkt_decode(buf, rcvlen, rcvdPkt) != PKT_OK){
			cleanBuffer(waitingInBuffer, (uint8_t)pkt_get_seqnum(rcvdPkt));
			if((uint8_t)pkt_get_seqnum(rcvdPkt) == nextSeqnum){ //le paquet qu on attendait
				fwrite(pkt_get_payload(rcvdPkt),1,pkt_get_length(rcvdPkt),fp);
			}
			else{
				if((uint8_t)pkt_get_seqnum(rcvdPkt) > nextSeqnum){
					addTobuffer(waitingInBuffer, rcvdPkt);
				}
				//faut check le buffer et puis ack/nack

			}
			//ecrire dans le fichier + envoyer ack/nack avec la bonne window
		}
		printf("received : %s\n", buf);
		receivedLastPaquet = 1;
	}
	return;
}

int main(int argc, char* argv[]){
	int i;
    int interpreter=1;
    void *fileToWrite=NULL, *hostname=NULL, *portPt=NULL;
    uint16_t port;
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
	// struct pollfd fds[1];
	int sockFd = socket(AF_INET6, SOCK_DGRAM, 0);
	if( sockFd == -1){
		return 1;
	}
	si_me.sin6_family = AF_INET6;
	si_me.sin6_port = htons(port);
	si_me.sin6_addr = in6addr_any;
	if(bind(sockFd, (struct sockaddr*)&si_me, sizeof(si_me)) == -1){
		return 2;
	}
	fflush(stdout);
	listenLoop(sockFd, &si_other, fileToWrite);
	// fds[0].fd =sockFd;
 //    fds[0].events = POLLIN | POLLPRI;
 //   	int ret = 0;
 //   	while(ret <=0){
 //   		ret = poll(fds, 1, 0);
 //   	}
    
    // if (ret > 0){int rcvlen = recvfrom(sockFd, buf, 1000, 0, (struct sockaddr *) &si_other, &slen);}
    
	shutdown(sockFd, 2);
	return -1;
}