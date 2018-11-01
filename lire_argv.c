#include <stdio.h>  /* atoi */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* struct de argv : 
			- si -f : sender -f fichier host numero_du_port
			- sinon : receiver/sender host numero_du_port symbole fichier 
			  avec rajout possible à la fin : 2> fichier message_d_erreur
*/
int main(int argc, char* argv[]){
	if(argc <4){
		printf("nombre d'argument trop petit \n");
		return -1;
	}
	int i = 1;
        int interpreter=1;
        void *fileToRead=NULL, *hostname=NULL, *portPt=NULL, *errorlog=NULL;
        uint16_t port;
        int symbole = 0;
        if(!strcmp(argv[i], "-f")){ //il faudra prendre en compte le "<"
		printf("on est dans le if du f\n");
	    	if(argc >5){
	    		printf("nombre d'argument trop grand \n");
	    		return -1;
	    	}
		i=i+1;
		fileToRead=argv[i];
		i=i+1;
		interpreter=0;
		for (; i<argc; i++){ // manager exceptions : si argc =1 ou argc >5 ou pas -f et argc>3 on exit : nbr d'aruments trop faible
			if(!strcmp(argv[i],"<")){
				printf("passe le <\n");
			}else if(hostname==NULL){
				hostname=argv[i];
			}else if(portPt==NULL){ 
				portPt=argv[i];
				port = atoi((const char*)portPt);
			}
		}
	}else{

		printf("on est dans le cas normal du f\n");
	    	if(argc >8){
	    		printf("nombre d'argument trop grand \n");
	    		return -1;
	    	}
		for (; i<argc; i++){ // manager exceptions : si argc =1 ou argc >5 ou pas -f et argc>3 on exit : nbr d'aruments trop faible
	        	if(hostname==NULL){
				hostname=argv[i];
			}else if(portPt==NULL){  
				portPt=argv[i];
				port = atoi((const char*)portPt);
			}else if(!strcmp(argv[i],"haha")){
				printf("on est ici0 \n");
			}else if(fileToRead==NULL){
				fileToRead=argv[i];		
				printf("on est ici1 \n");
			}else if(!strcmp(argv[i],"hoho")){
				printf("on est ici2 \n");
				i = i+1;
				errorlog = argv[i];
			}
    		}
    }
    printf("le hostname vaut %s \n", (const char*)hostname);
    printf("le fileToread vaut %s \n", (const char*)fileToRead);
    printf("le port vaut %d \n", port);
    printf("error et log sont dirige vers %s \n", (const char*)errorlog);
    return 0;
}
