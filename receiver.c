#include "packet_interface.h"

#include <stddef.h> /* size_t */
#include <stdint.h> /* uintx_t */
#include <stdio.h>  /* atoi */
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h> 
#include <poll.h> 


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
    struct sockaddr_in6 si_me, si_other;
    int slen = sizeof(si_other);
	struct pollfd fds[1];
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
	char buf[1000];
	bzero(buf, 1000);
	fds[0].fd =sockFd;
    fds[0].events = POLLIN | POLLPRI;
    int rcvlen = recvfrom(sockFd, buf, 1000, 0, (struct sockaddr *) &si_other, &slen);
	if (rcvlen == -1){
		return 3;
	}
	printf("received\n");
	return -1;
}