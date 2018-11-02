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
#define MAXPKTSIZE 528
#define MAXWIN 32
#define TIMER 4000

void resend(double msTime, pkt_t ** waitingInBuffer, double * timeBuffer, int i, int sockFd, struct sockaddr_in6 si_other){
	pkt_t * pkt = waitingInBuffer[i];
	char * buf;
	size_t len;
	uint16_t pkt_len = pkt_get_length(pkt);
	if(pkt_len == 0){
		len = 12;
	}else{
		len = pkt_len + 16;
	}
	buf = malloc(len);
	pkt_encode(pkt, buf, &len);
	sendto(sockFd, buf, len,0,(struct sockaddr *) &si_other, sizeof(si_other));
	free(buf);
	timeBuffer[i] = msTime;
}

void sendPaquet(pkt_t * pkt, int sockFd, struct sockaddr_in6 si_other){
	printf("window: %u\n", pkt_get_window(pkt));
	printf("TR: %u\n", pkt_get_tr(pkt));
	printf("type(1=data): %u\n", pkt_get_type(pkt));
	printf("length: %u\n", pkt_get_length(pkt));
	printf("seqnum: %u\n", pkt_get_seqnum(pkt));
	printf("crc1: %u\n", pkt_get_crc1(pkt));
	int payLength = pkt_get_length(pkt);
	size_t totalSize = sizeof(pkt_t) + payLength - sizeof(char*);
	printf("paylength : %u     total size : %u\n", payLength, totalSize);

	char * buf;
	size_t len;
	uint16_t pkt_len = pkt_get_length(pkt);
	if(pkt_len == 0){
		len = 12;
	}else{
		len = pkt_len + 16;
	}
	buf = malloc(len);
	pkt_status_code encodeRet = pkt_encode(pkt, buf, &len);
	printf("encode ret when encoding : %u    (len=%u)\n", encodeRet,len);
	sendto(sockFd, buf, len,0,(struct sockaddr *) &si_other, sizeof(si_other));
	free(buf);
}

int bufferIsFree(uint8_t * presentInBuffer){
	int i = 0;
	for(i = 0; i < MAXWIN; i++){
		if(presentInBuffer[i] == 0){
			return 1;
		}
	}
	return 0;
}
int checkTimer(uint8_t * presentInBuffer ,double msTime, pkt_t ** waitingInBuffer, double * timeBuffer, int* fileProgressionBuffer, int sockFd, struct sockaddr_in6 si_other, unsigned int receiverWindow){
	int sent = 0;
	int i = 0;
	for(i = 0; i < MAXWIN; i++){
		if(presentInBuffer[i] != 0){
			sent ++;
			if(timeBuffer[i] + TIMER < msTime ){
				resend(msTime, waitingInBuffer, timeBuffer, i, sockFd, si_other);
				printf("resending from buffer!!! \n");
			}
		}
	}
	return sent;
}
void fillPacket(pkt_t * pkt, void * buf, uint16_t len, uint8_t seq, int senderWin){
	pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_length(pkt, len);
    pkt_set_tr(pkt, 0);
    pkt_set_seqnum(pkt, seq);
    pkt_set_window(pkt, senderWin);
    pkt_set_timestamp(pkt, 0);
	uint32_t crc = 0;
    crc = crc32(crc, (Bytef *)(pkt), sizeof(uint64_t));
    pkt_set_crc1(pkt, crc);
    if(len!=0){
		char * pay = malloc(len);
		memcpy(pay, buf, len);
		pkt_set_payload(pkt, buf, len);
		crc = 0;
	    crc = crc32(crc, (Bytef *)(pkt->payload), len);
	    pkt_set_crc2(pkt, crc);
	    free(pay);
	}
}
void addToBuffer(uint8_t * presentInBuffer, pkt_t ** waitingInBuffer, double * timeBuffer, int* fileProgressionBuffer, int* currentOffset, pkt_t * pkt, double* msTime){
	int i =0;
	for(i = 0; i < MAXWIN; i++){
		if(presentInBuffer[i] == 0){
			presentInBuffer[i] = 1;
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
	if(fileMaxOffset - *currentOffset <= 0){ //last paquet
		len = 0;
	}
	printf("%u\n", len);
	return len;
}

void manageThisAck(char * decbuf, uint8_t * lastAck, unsigned int * receiverWindow, int len){
	pkt_t * pkt = pkt_new();
	pkt_status_code pktDec = pkt_decode(decbuf, len, pkt);
	*receiverWindow = pkt_get_window(pkt); //VRIFIER TYPE NACK OU ACK
	printf("%u <<<= window  (status code = %u)\n",  pkt_get_window(pkt), pktDec);
	if (*lastAck != pkt_get_seqnum(pkt)){
		*lastAck = pkt_get_seqnum(pkt);
	}
}

void tryEmptyingBuffer(uint8_t * presentInBuffer, pkt_t ** waitingInBuffer, double * timeBuffer, int* fileProgressionBuffer, uint8_t lastAck){
	int fileProgressionAcked = -1;
	int i = 0;
	int u = 0;
	uint8_t prev = lastAck - 1; // prev = the latest seqnum that is acknowledged (lastAck = next EXPECTED seqnum)
	if(lastAck == 0){
		prev = 255;
	}
	for(i=0; i < MAXWIN; i++){ //get the progression in the file that is acked
		if(presentInBuffer[i] == 1 && pkt_get_seqnum(waitingInBuffer[i])==prev){
			fileProgressionAcked = fileProgressionBuffer[i];
		}
	}
	for(u=0; u<MAXWIN; u++){ // remove any paquet that has less file progression than what is acked
		if(fileProgressionBuffer[u] != -1 && fileProgressionBuffer[u] <= fileProgressionAcked){//then delete these paquets from buffer
			fileProgressionBuffer[u] = -1;
			timeBuffer[u] = -1;
			pkt_del(waitingInBuffer[u]);
			waitingInBuffer[u] = NULL;
			presentInBuffer[u] = 0;
		}
	}
}
int bufferEmpty(uint8_t * presentInBuffer){
	int i = 0;
	for(i =0; i< MAXWIN; i++){
		if(presentInBuffer[i] == 1){
			return 0;
		}
	}
	return 1;
}

int sendTest(int sockFd, struct sockaddr_in6 si_other, int slen, int pollRet, char * fileBuffer){
    struct pollfd pfds[1];
	int len = 30;
	char * buff = malloc(len);
	strcpy(buff, "hello from sender");
	if(pollRet>0){
		printf("sending:\n");
		sendto(sockFd, buff, len,0,(struct sockaddr *) &si_other, sizeof(si_other));
		printf("SENT\n");
	}
	pfds[0].fd = sockFd;
	pfds[0].events = POLLIN;
	pollRet = poll(pfds, 1, 1);
	if(pollRet >0){ //received an ack
		printf("received\n");
		char decbuf[12];
		int rcvlen = recvfrom(sockFd, decbuf, 12, 0, (struct sockaddr *) &si_other, &slen);
		// manageThisAck(decbuf, &lastAck, &receiverWindow, rcvlen);
	}
	else{
		pollRet = 0;
	}
	return pollRet;
}

int main(int argc, char* argv[]){
	int i = 1;
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
    FILE * ffp;
    ffp = stdin;
    char * fileBuffer;
    unsigned int fileMaxOffset;
    if(interpreter==0){
    	printf("reading from file\n");
    	ffp = fopen(fileToRead, "rb");
    	fseek(ffp, 0, SEEK_END);
    	fileMaxOffset= ftell(ffp); // gets the end of the file
    	rewind(ffp);
    	fileBuffer = (char *)malloc((fileMaxOffset+1)*sizeof(char));
    	fread(fileBuffer, fileMaxOffset, 1, ffp);
    	printf("%s", fileBuffer);
    } 
    else{
    	printf("reading from stdin\n");
    	char * tmpBuff;
    	int iterSize = 4*sizeof(char);
    	int bufferSize = iterSize;
    	int readBytes = 0;
    	int minus = 0;
    	char tmp[iterSize];
    	fileBuffer = malloc(iterSize*sizeof(char));
    	while(fgets(tmp, iterSize, ffp)){
    		if(readBytes >= bufferSize){
    			bufferSize = bufferSize*2*sizeof(char);
    			tmpBuff = malloc(bufferSize);
    			memcpy(tmpBuff, fileBuffer, readBytes);
    			free(fileBuffer);
    			fileBuffer = tmpBuff;
    			printf("new size : %u\n", bufferSize);
    		}
    		memcpy(fileBuffer+readBytes, tmp, strlen(tmp));
    		readBytes += strlen(tmp);
    	}
    	fileMaxOffset = readBytes;
    	printf("\nbuff len : %u    %u\n", readBytes, strlen(fileBuffer));
    	printf("%s\n", fileBuffer);
    }




    struct sockaddr_in6 si_other;
    int slen = sizeof(si_other);
	bzero((char *)&si_other,slen);
	si_other.sin6_port = htons(port);
	si_other.sin6_family = AF_INET6;
    int sockFd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockFd == -1){
        return 2; // error
    }
    // fflush(stdin);
    // FILE * fp;
    // if(fileToRead!=NULL){
    //     fp = fopen(fileToRead, "rb"); //was "wb"
    // }
    // fseek(fp, 0, SEEK_END);
    // unsigned int fileMaxOffset= ftell(fp); // gets the end of the file
    // rewind(fp);
    // char * fileBuffer;
    // fileBuffer = (char *)malloc((fileMaxOffset+1)*sizeof(char)); // si probleme: peut etre que fichier trop gros pr la memoire du pc
    // fread(fileBuffer, fileMaxOffset, 1, fp);

    int currentOffset = 0;
    uint8_t done = 0;
    struct pollfd pfds[1];
    int pollRet = 1;
    unsigned int rcvlen;
    char decbuf[12];
    uint8_t presentInBuffer[MAXWIN];
    pkt_t * waitingInBuffer[MAXWIN];
    double timeBuffer[MAXWIN];
    int fileProgressionBuffer[MAXWIN];
    i = 0;
    for(i = 0; i<MAXWIN; i++){ //init arrays
    	waitingInBuffer[i] = NULL;
    	timeBuffer[i] = -1;
    	fileProgressionBuffer[i] = -1;
    	presentInBuffer[i] = 0;
    }	
    struct timeval tv;
    double msTime;
   	unsigned int receiverWindow = 1;
    unsigned int senderWin = 31;
    int sent = 0;
    uint8_t seqNum = 0;
    uint8_t lastAck = 0;
    char * buf;
    size_t sendLen = 0;
    int allSent = 0;
    while(!done){
    	printf("rwin:  %d\n", receiverWindow);
    	usleep(500000);
    	// pollRet = sendTest(sockFd, si_other, slen, pollRet);
    	// check retransmission timers and retransmit
    	sent = 0;
	    gettimeofday(&tv, NULL);
	    msTime = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
	    if(receiverWindow > 0){
	    	//checks if the stored paque<ts have their retransmission timers expired, and resend them if it's the case
	    	sent = checkTimer(presentInBuffer ,msTime, waitingInBuffer,  timeBuffer,fileProgressionBuffer, sockFd, si_other, receiverWindow);
	    	//sent = nb in the buffer (ie: paquets that are still considered as traveling in the network)
	    }
	    // printf("sent = %d\n", sent);
	    if(receiverWindow - sent > 0){
	    	//sending a new paquet
	    	if(bufferIsFree(presentInBuffer) == 1){// if buffer full: cannot send (all previous sends are awaiting ack)
	    		//make paquet
				pkt_t * pkt = pkt_new();
				
	    		sendLen = nextPktLen(&currentOffset, fileMaxOffset); //payload length
	    		if(sendLen == 0){
	    			allSent = 1;
	    			seqNum = lastAck; //for the last paquet
	    			fillPacket(pkt, buf, sendLen, seqNum, senderWin);

	    			// printf("window: %u\n", pkt_get_window(pkt));
	    			// printf("TR: %u\n", pkt_get_tr(pkt));
	    			// printf("type(1=data): %u\n", pkt_get_type(pkt));
	    			// printf("length: %u\n", pkt_get_length(pkt));
	    			// printf("seqnum: %u\n", pkt_get_seqnum(pkt));

	    			// size_t leen = 12;
	    			// char bufa[leen];
	    			// pkt_status_code aa = pkt_encode(pkt, bufa, &leen);
	    			// pkt_t * decodPkt = pkt_new();
	    			// printf("decoded check : %u   (encode:%u)\n", pkt_decode(bufa, 12, decodPkt), aa);
	    		}
	    		else{
					buf = calloc(1, sendLen);
					memcpy(buf, fileBuffer + currentOffset, sendLen);
					fillPacket(pkt, buf, sendLen, seqNum, senderWin);

					// printf("window: %u\n", pkt_get_window(pkt));
					// printf("TR: %u\n", pkt_get_tr(pkt));
					// printf("type(1=data): %u\n", pkt_get_type(pkt));
					// printf("length: %u\n", pkt_get_length(pkt));
					// printf("seqnum: %u\n", pkt_get_seqnum(pkt));
					// size_t leen = 16+sendLen;
					// char bufa[leen];
					// pkt_status_code aa = pkt_encode(pkt, bufa, &leen);
					// pkt_t * decodPkt = pkt_new();
					// printf("decoded check : %u (encode:%u)\n", pkt_decode(bufa, 16+sendLen, decodPkt), aa);


					free(buf);
				}
				sendPaquet(pkt, sockFd, si_other);
				
				

				sent ++;
				addToBuffer(presentInBuffer, waitingInBuffer, timeBuffer, fileProgressionBuffer, &currentOffset, pkt, &msTime);
				currentOffset = currentOffset + sendLen;

	    		seqNum = seqNum + 1;
	    		if(seqNum>255){
	    			seqNum = 0;
	    		}
	    	}
	    }
    	// poll: checking for ack (end of the loop)
    	pfds[0].fd = sockFd;
	    pfds[0].events = POLLIN;
	    pollRet = poll(pfds, 1, 1);
	    if(pollRet >0){ //received an ack
	    	char decbuf[12];
            rcvlen = recvfrom(sockFd, decbuf, 12, 0, (struct sockaddr *) &si_other, &slen);
            printf("ACK RECEIVED\n" );
            manageThisAck(decbuf, &lastAck, &receiverWindow, rcvlen);
	    }
	    tryEmptyingBuffer(presentInBuffer, waitingInBuffer, timeBuffer, fileProgressionBuffer, lastAck);
	    if(bufferEmpty(presentInBuffer) == 1 && allSent==1){
	    	printf("buffer is empty: all sent\n");
	    	done = 1;
	    }
	    // done = 1; // erase this
    }


}