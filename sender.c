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
		if(waitingInBuffer[i] != NULL){
			return 1;
		}
	}
	return 0;
}

int checkTimer(double msTime, pkt_t ** waitingInBuffer, double * timeBuffer, int sockFd, struct sockaddr_in6 * si_other){
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
void fillPacket(pkt_t * pkt, char * fileBuffer, int len, int seq, int senderWin){
	char * payload;
	pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_length(pkt, len);
    pkt_set_tr(pkt, 0);
    pkt_set_seqnum(pkt, seq);
    pkt_set_window(pkt, senderWin);

	payload = fileBuffer;

	// pkt
	// fileBuffer += len;
}
void sendNextPaquet(char * fileBuffer, int * currentOffset, int fileMaxOffset, int sockFd,struct sockaddr_in6 * si_other, int seq, int senderWin){
	int len = MAXPAYSIZE;
	if(fileMaxOffset - *currentOffset < MAXPAYSIZE){
		len = fileMaxOffset - *currentOffset;
	}
	if(fileMaxOffset - *currentOffset == 0){ //last paquet
		len = 0;
	}
	pkt_t * pkt = pkt_new();
	fillPacket(pkt, fileBuffer, len);
	//PUT THIS PAQUET IN BUFFER + THE TIMESTAMP IN OTHER BUFFER
	buf = fileBuffer
}

int main(int argc, char* argv[]){
	int i;
    int interpreter=1;
    void *fileToRead=NULL, *hostname=NULL, *portPt=NULL;
    uint16_t port;
    for (i=1; i<argc; i++){ // manager exceptions : si arc =1 ou argc >5 ou pas -f er argc>3 on exit : nbr d'aruments trop faible
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

    //connection
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
    int currentOffset;
    uint8_t done = 0;
    struct pollfd pfds[1];
    int pollRet;
    unsigned int rcvlen;
    char decbuf[12];
    pkt_t * waitingInBuffer[MAXWIN] = {NULL};
    double timeBuffer[MAXWIN];
    struct timeval tv;
    double msTime;
    int receiverWindow = 1;
    int senderWin = 31;
    int sent = 0;
    int seqNum = 0;
    while(!done){
    	sent = 0;
    	//retransmissions
	    gettimeofday(&tv, NULL);
	    msTime = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
	    if(receiverWindow > 0){
	    	sent = checkTimer(msTime, waitingInBuffer, timeBuffer, sockFd, &si_other);
	    }
	    //sending a new paquet
	    if(receiverWindow-sent > 0){
	    	if(bufferIsFree(waitingInBuffer) == 1){// if buffer full: cannot send (all previous sends are awaiting ack)
	    		sendNextPaquet(fileBuffer, &currentOffset, fileMaxOffset, sockFd, &si_other, seqNum, senderWin);
	    	}
	    }
    	//poll: checking for ack (end of the loop)
    	pfds[0].fd = sockFd;
	    pfds[0].events = POLLIN;
	    pollRet = poll(pfds, 1, 1);
	    if(pollRet >0){ //received an ack
	    	printf("received an ack\n");
	    	// char decbuf[12];
            rcvlen = recvfrom(sockFd, decbuf, 12, 0, (struct sockaddr *) &si_other, &slen);
	    }
    }
	char *msg = "A string declared as a pointer.\n";
    int  sizeToEncode = 20;
    sendto(sockFd, msg, sizeToEncode,0,(struct sockaddr *) &si_other, sizeof(si_other));

	
}