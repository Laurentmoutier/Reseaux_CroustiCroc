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

/* struct de argv : 
			- si -f : sender -f fichier host numero_du_port
			- sinon : receiver/sender host numero_du_port symbole fichier 
			  avec rajout possible à la fin : 2> fichier message_d_erreur
*/
void mainLoop(void* si_other, int sockFd, FILE * fd){
	//while not all aknowledged: check if window is free, if yes send new paquet
	//an unaknowledged paquet is a thread
	return;
}

int connectToReceiver(struct sockaddr_in6* si_other, char* hostname, int port){
	int sockFd;
	bzero((char *)&si_other,sizeof(si_other));
	si_other->sin6_port = htons((uint16_t)port);
	si_other->sin6_family = AF_INET6;
	printf("%d\n", port);
	//fill the socket struxture and connect to other socket
	return sockFd;
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
	bzero((char *)&si_other,sizeof(si_other));
	si_other.sin6_port = htons(port);
	si_other.sin6_family = AF_INET6;
	if (inet_pton(AF_INET6,hostname , &si_other.sin6_addr) == 0){
    	return 1; // error
    }
    int sockFd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockFd == -1){
        return 2; // error
    }
    char *msg = "A string declared as a pointer.\n";
    // bzero(msg, 10);
    int  sizeToEncode = 20;
    int sent = sendto(sockFd, msg, sizeToEncode,0,(struct sockaddr *) &si_other, sizeof(si_other));

    fflush(stdin);

	printf("%u\n", ntohs(si_other.sin6_port));



	

	
}