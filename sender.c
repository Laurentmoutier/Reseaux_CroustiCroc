#include "packet_interface.h"

#include <stddef.h> /* size_t */
#include <stdint.h> /* uintx_t */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>  /* atoi */
/*
GROSSE ERREUR,
prise en compte du cas ou on ne sait pas ou on est (premier_argument) seulement on le sait, puisque on est dans
le main du sender ici ! 

*/

/* struct de argv : 
			- si -f : sender -f fichier host numero_du_port
			- sinon : receiver/sender host numero_du_port symbole fichier 
			  avec rajout possible à la fin : 2> fichier message_d_erreur
*/
void connect(struct sockaddr_in6* si_other, char* hostname, int port){
	//fill the socket struxture and connect to other socket
	return;
}

int main(int argc, char* argv[]){
	int i;
    int interpreter=1;
    void *fileToRead=NULL, *hostname=NULL, *port=NULL;
    for (i=1; i<argc; i++){ // manager exceptions : si arc =1 ou argc >5 ou pas -f er argc>3 on exit : nbr d'aruments trop faible
        if(!strcmp(argv[i], "-f")){ //il faudra prendre en compte le "<"
            i=i+1;
            fileToRead=argv[i];
            interpreter=0;
        }else if(hostname==NULL){
            hostname=argv[i];
        }else if(port==NULL){     
            port=argv[i];
        }
    }

    printf("%s\n", hostname);
    printf("%u\n", interpreter);
    printf("%s\n", fileToRead);


	// char* fichier;
	// char* host;
	// int port_number = 0;
	// int premier_argument = 0; //permet de savoir s'il s'agit du receiver ou du sender
	// int symbole = 0;
	// char* erreur;
	// if(strcmp(argv[0], "./sender") == 0){
	// 	premier_argument = 1;
	// }else if(strcmp(argv[0], "./receiver") == 0){
	// 	premier_argument = 2;
	// }else{
	// 	fprintf(stderr, "le premier argument n'est ni un sender, ni un receiver \n");
	// 	return -1;
	// }

	// if(strcmp(argv[1], "-f")==0){// on sait donc qu'il s'agit du sender, et que celui ci specifie l'arg d'apres le nom du fichier
	// 	fichier = argv[2];
	// 	host = argv[3];
	// 	port_number = atoi(argv[4]); 
	// 	if(port_number == 0){
	// 		fprintf(stderr, "option -f : probleme au numero du port \n");
	// 		return -1;
	// 	}
	// }else{
	// 	host = argv[1];
	// 	port_number = atoi(argv[2]);
	// 	if(port_number == 0){
	// 		fprintf(stderr, "option sans -f : probleme au numero du port \n");
	// 		return -1;
	// 	}

	// 	if(argv[3] == "<"){
	// 		symbole = 1;
	// 	}else if(argv[3] == ">"){
	// 		symbole = 2;
	// 	}else{
	// 		fprintf(stderr, "option sans -f : probleme au symbole \n");
	// 		return -1;
	// 	}
	// 	fichier  = argv[4];
	// 	if(argc>5){
	// 		erreur = argv[6];
	// 	}
	// }


	
}