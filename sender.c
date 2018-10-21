#include "packet_interface.h"
#include "packet_interface.c"


#include <stddef.h> /* size_t */
#include <stdint.h> /* uintx_t */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>  /* atoi */
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h> 
#include <poll.h>
#include <sys/time.h>
#include <math.h>
#define MAXPAYSIZE 512
#define MAXWIN 32
#define TIMER 2100

void resend(double msTime, pkt_t ** waitingInBuffer, double * timeBuffer, int i, int sockFd, struct sockaddr_in6 * si_other){
	pkt_t * pkt = waitingInBuffer[i];
	char * buf;
	size_t len;
	if(pkt_get_length(pkt) == 0){
		len = 12;
	}else{
		len = pkt_get_length(pkt) + 16;
	}
	buf = malloc(len);
	pkt_encode(pkt, buf, &len);
	sendto(sockFd, buf, len,0,(struct sockaddr *) &si_other, sizeof(si_other));
	free(buf);
	timeBuffer[i] = msTime;

}
int bufferIsFree(pkt_t ** waitingInBuffer){
	for(int i = 0; i < MAXWIN; i++){
		if(waitingInBuffer[i] == NULL){
			return 1;
		}
	}
	return 0;
}
int checkTimer(double msTime, pkt_t ** waitingInBuffer, double * timeBuffer, int* fileProgressionBuffer, int sockFd, struct sockaddr_in6 * si_other){
	int sent = 0;
	for(int i = 0; i < MAXWIN; i++){
		if(waitingInBuffer[i] != NULL){
			if(timeBuffer[i] > msTime + TIMER){
				resend(msTime, waitingInBuffer, timeBuffer, i, sockFd, si_other);
				sent ++;
			}
		}
	}
	return sent;
}
void fillPacket(pkt_t * pkt, char * fileBuffer, uint16_t len, uint8_t seq, int senderWin){
	char * payload;
	pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_length(pkt, len);
    pkt_set_tr(pkt, 0);
    pkt_set_seqnum(pkt, seq);
    pkt_set_window(pkt, senderWin);
    pkt_set_timestamp(pkt, 0);
	uint32_t crc = 0;
    crc = crc32(crc, (Bytef *)(pkt), sizeof(uint64_t));
    pkt_set_crc1(pkt, crc);
	payload = fileBuffer;
	pkt_set_payload(pkt, payload, len);
	crc = 0;
    crc = crc32(crc, (Bytef *)(pkt->payload), len);
    pkt_set_crc2(pkt, crc);
    fileBuffer += len; //points to next place in the fileBuffer where the next paquet will carry on
}
void addToBuffer(pkt_t ** waitingInBuffer, double * timeBuffer, int* fileProgressionBuffer, int* currentOffset, pkt_t * pkt, double* msTime){
	for(int i = 0; i < MAXWIN; i++){
		if(waitingInBuffer[i] == NULL){
			waitingInBuffer[i] = pkt;
			timeBuffer[i] = *msTime;
			fileProgressionBuffer[i] = *currentOffset;
			return;
		}
	}
}
int nextPktLen( int * currentOffset, int fileMaxOffset){
	size_t len = MAXPAYSIZE;
	if(fileMaxOffset - *currentOffset < MAXPAYSIZE){
		len = fileMaxOffset - *currentOffset;
	}
	if(fileMaxOffset - *currentOffset == 0){ //last paquet
		len = 0;
	}
	return len;
}
void manageThisAck(char * decbuf, uint8_t * lastAck, unsigned int * receiverWindow, int len){
	pkt_t * pkt = pkt_new();
	pkt_decode(decbuf, len, pkt);
	*receiverWindow = pkt_get_window(pkt);
	if (*lastAck != pkt_get_seqnum(pkt)){
		*lastAck = pkt_get_seqnum(pkt);
	}
}
void tryEmptyingBuffer(pkt_t ** waitingInBuffer, double * timeBuffer, int* fileProgressionBuffer, uint8_t lastAck){
	int fileProgressionAcked = 0;
	for(int i=0; i < MAXWIN; i++){
		if(waitingInBuffer[i] != NULL && pkt_get_seqnum(waitingInBuffer[i])==lastAck){
			fileProgressionAcked = fileProgressionBuffer[i];
		}
	}
	for(int u=0; u<MAXWIN; u++){
		if(fileProgressionBuffer[u] != -1 && fileProgressionBuffer[u] <= fileProgressionAcked && waitingInBuffer[u]!=NULL){//then delete these paquets from buffer
			fileProgressionBuffer[u] = -1;
			timeBuffer[u] = -1;
			pkt_del(waitingInBuffer[u]);
			waitingInBuffer[u] = NULL;
		}
	}
}
int bufferEmpty(pkt_t ** waitingInBuffer){
	for(int i =0; i< MAXWIN; i++){
		if(waitingInBuffer != NULL){
			return 0;
		}
	}
	return 1;
}
int main(int argc, char* argv[]){
	int i;
    int interpreter=1;
    void *fileToRead=NULL, *hostname=NULL, *portPt=NULL;
    uint16_t port;
    for (i=1; i<argc; i++){ // manager exceptions : si argc =1 ou argc >5 ou pas -f et argc>3 on exit : nbr d'aruments trop faible
        if(!strcmp(argv[i], "-f")){ //il faudra prendre en compte le "<"
            i=i+1;
            fileToRead=argv[i];
            interpreter=0;
        }else if(hostname==NULL){
            hostname=argv[i];
        }else if(portPt==NULL){     
            portPt=argv[i];
            port = atoi((const char*)portPt);
        }
    }
    struct sockaddr_in6 si_other;
    int slen = sizeof(si_other);
	bzero((char *)&si_other,slen);
	si_other.sin6_port = htons(port);
	si_other.sin6_family = AF_INET6;
	// if (inet_pton(AF_INET6,hostname , &si_other.sin6_addr) == 0){
 //    	return 1; // error
 //    }
    int sockFd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockFd == -1){
        return 2; // error
    }
    fflush(stdin);
    FILE * fp;
    if(fileToRead!=NULL){
        fp = fopen(fileToRead, "rb"); //was "wb"
    }
    fseek(fp, 0, SEEK_END);
    unsigned int fileMaxOffset= ftell(fp); // gets the end of the file
    rewind(fp);
    char * fileBuffer;
    fileBuffer = (char *)malloc((fileMaxOffset+1)*sizeof(char)); // si probleme: peut etre que fichier trop gros pr la memoire du pc
    int currentOffset = 0;
    uint8_t done = 0;
    struct pollfd pfds[1];
    int pollRet;
    unsigned int rcvlen;
    char decbuf[12];
    pkt_t * waitingInBuffer[MAXWIN];
    double timeBuffer[MAXWIN];
    int fileProgressionBuffer[MAXWIN];
    for(int i = 0; i<MAXWIN; i++){ //init arrays
    	waitingInBuffer[i] = NULL;
    	timeBuffer[i] = -1;
    	fileProgressionBuffer[i] = -1;
    }	
    struct timeval tv;
    double msTime;
   	unsigned int receiverWindow = 1;
    unsigned int senderWin = 31;
    int sent = 0;
    uint8_t seqNum = 0;
    uint8_t lastAck = 31;
    char * buf;
    size_t sendLen = 0;
    while(!done){
    	//check retransmission timers and retransmit
    	sent = 0;
	    gettimeofday(&tv, NULL);
	    msTime = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
	    if(receiverWindow > 0){
	    	// sent = checkTimer(msTime, waitingInBuffer,  timeBuffer,fileProgressionBuffer, sockFd, &si_other);
	    	for(int i = 0; i < MAXWIN; i++){
	    		if(waitingInBuffer[i] != NULL){
	    			if(timeBuffer[i] > msTime + TIMER){
	    				//resend:
	    				pkt_t * paquet = waitingInBuffer[i];
	    				char * buff;
	    				size_t len;
	    				if(pkt_get_length(paquet) == 0){
	    					len = 12;
	    				}else{
	    					len = pkt_get_length(paquet) + 16;
	    				}
	    				buff = malloc(len);
	    				pkt_encode(paquet, buff, &len);
	    				sendto(sockFd, buff, len,0,(struct sockaddr *) &si_other, sizeof(si_other));
	    				free(buff);
	    				timeBuffer[i] = msTime;
	    				// resend(msTime, waitingInBuffer, timeBuffer, i, sockFd, si_other);
	    				sent ++;
	    			}
	    		}
	    	}
	    }
	    if(receiverWindow-sent > 0){
	    	//sending a new paquet
	    	if(bufferIsFree(waitingInBuffer) == 1){// if buffer full: cannot send (all previous sends are awaiting ack)
	    		sendLen = nextPktLen(&currentOffset, fileMaxOffset);
				buf = malloc(sendLen);
				pkt_t * pkt = pkt_new();
				fillPacket(pkt, fileBuffer, sendLen,seqNum,  senderWin);
				pkt_encode(pkt, buf, &sendLen);
				sendto(sockFd, buf, sendLen,0,(struct sockaddr *) &si_other, sizeof(si_other));
				addToBuffer(waitingInBuffer, timeBuffer, fileProgressionBuffer, &currentOffset, pkt, &msTime);
				currentOffset = currentOffset + sendLen;
	    		free(buf);
	    	}
	    }
    	// poll: checking for ack (end of the loop)
    	pfds[0].fd = sockFd;
	    pfds[0].events = POLLIN;
	    pollRet = poll(pfds, 1, 1);
	    if(pollRet >0){ //received an ack
	    	printf("received an ack\n");
	    	char decbuf[12];
            rcvlen = recvfrom(sockFd, decbuf, 12, 0, (struct sockaddr *) &si_other, &slen);
            manageThisAck(decbuf, &lastAck, &receiverWindow, rcvlen);
            printf("%d\n", receiverWindow);
	    }
	    tryEmptyingBuffer(waitingInBuffer, timeBuffer, fileProgressionBuffer, lastAck);
	    if(bufferEmpty(waitingInBuffer) == 1){
	    	done = 1;
	    }
	    // done = 1;
    }


}