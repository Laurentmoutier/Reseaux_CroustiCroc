#include <stdio.h>
#include <stddef.h> /* size_t */
#include <stdint.h> /* uintx_t */
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>


typedef struct pkt pkt_t;

typedef enum {
	PTYPE_DATA = 1,
	PTYPE_ACK = 2,
	PTYPE_NACK = 3,
} ptypes_t;

typedef enum {
	PKT_OK = 0,     /* Le paquet a ete traite avec succes */
	E_TYPE,         /* Erreur liee au champs Type */
	E_TR,           /* Erreur liee au champ TR */
	E_LENGTH,       /* Erreur liee au champs Length  */
	E_CRC,          /* CRC invalide */
	E_WINDOW,       /* Erreur liee au champs Window */
	E_SEQNUM,       /* Numero de sequence invalide */
	E_NOMEM,        /* Pas assez de memoire */
	E_NOHEADER,     /* Le paquet n'a pas de header (trop court) */
	E_UNCONSISTENT, /* Le paquet est incoherent */
} pkt_status_code;

struct __attribute__((__packed__)) pkt {
	unsigned int type:2;
	unsigned int trFlag:1;
	unsigned int window:5;
	uint8_t seqNum;
	uint16_t length;
	uint32_t timestamp;
	uint32_t crc1;
	char* payload; // const char*
	uint32_t crc2;
};

pkt_t* pkt_new(){
	pkt_t* pkt_ptr = malloc (sizeof (struct pkt));
}


void pkt_del(pkt_t *pkt){
	free(pkt->payload);
    free(pkt);
}

ptypes_t pkt_get_type(const pkt_t* pkt){
	unsigned int type = pkt->type;
	// int mask = 0b11;
	// int result = type & mask;
	return type;
}

uint8_t  pkt_get_tr(const pkt_t* pkt){
	uint8_t tr = pkt->trFlag;
	return tr;
}

uint8_t  pkt_get_window(const pkt_t* pkt){
	uint8_t window = pkt->window;
	return window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt){
	uint8_t seqNum = pkt->seqNum;
	return seqNum;
}

uint16_t pkt_get_length(const pkt_t* pkt){
	uint16_t length = pkt->length;
	return ntohs(length);
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt){
	uint32_t timestamp = pkt->timestamp;
	return ntohl(timestamp);
}

uint32_t pkt_get_crc1   (const pkt_t* pkt){
	uint32_t crc1 = pkt->crc1;
	return ntohl(crc1);
}

uint32_t pkt_get_crc2   (const pkt_t* pkt){
	uint32_t crc2 = pkt->crc2;
	return ntohl(crc2);
}

const char* pkt_get_payload(const pkt_t* pkt){
	const char* payload = pkt->payload;
	return payload;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type){
	if(type > 0b11){
		return E_TYPE;
	}
	if(pkt != NULL){
		pkt->type = type;
		return PKT_OK;
	}
	return E_UNCONSISTENT; // ou bien autre erreur?
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr){
	if(tr > 0b1){
		return E_TR;
	}
	if(pkt != NULL){
		pkt->trFlag = tr;
		return PKT_OK;
	}
	return E_UNCONSISTENT;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window){
	if(window > 0b11111){
		return E_WINDOW;
	}
	if(pkt != NULL){
		pkt->window = window;
		return PKT_OK;
	}
	return E_UNCONSISTENT;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum){
	if(pkt != NULL){
		pkt->seqNum = seqnum;
		return PKT_OK;
	}
	return E_UNCONSISTENT;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length){
	if(pkt != NULL){
		pkt->length = htons(length);
		return PKT_OK;
	}
	return E_UNCONSISTENT;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp){
	if(pkt != NULL){
		pkt->timestamp = htonl(timestamp);
		return PKT_OK;
	}
	return E_UNCONSISTENT;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1){
	if(pkt != NULL){
		pkt->crc1 = htonl(crc1);
		return PKT_OK;
	}
	return E_UNCONSISTENT;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2){
	if(pkt != NULL){
		pkt->crc2 = htonl(crc2);
		return PKT_OK;
	}
	return E_UNCONSISTENT;
}

pkt_status_code pkt_set_payload(pkt_t *pkt,
							    const char *data,
								const uint16_t length){
	if(pkt != NULL){
		if(pkt->payload != NULL){
			free(pkt->payload);
		}
		pkt->payload = malloc(length); //length + 1 ?
		memcpy(pkt->payload, data, length);
		pkt_set_length(pkt, length);
		return PKT_OK;
	}
	return E_UNCONSISTENT;
}

int main(){
	pkt_t* paquet;
	paquet = pkt_new();

	printf("setters...\n");
	pkt_status_code typeRet = pkt_set_type(paquet, 0b11);
	pkt_status_code trRet = pkt_set_tr(paquet, 0b1);
	pkt_status_code winRet = pkt_set_window(paquet, 0b11011);
	pkt_status_code seqRet = pkt_set_seqnum(paquet, 0b11101111);
	pkt_status_code crc1Ret = pkt_set_crc1(paquet, 0b11111111111111111101111111111111);
	pkt_status_code crc2Ret = pkt_set_crc2(paquet, 0b11111111111110111111111111111111);
	pkt_status_code timeRet = pkt_set_timestamp(paquet, 0b11111111101111111111111111111111);


	char* pay = (char*)malloc(8*sizeof(char));
	strcpy(pay, "hello");
	pkt_set_payload(paquet, pay, 5);

	ptypes_t type = pkt_get_type(paquet);
	uint8_t tr = pkt_get_tr(paquet);
	uint8_t window = pkt_get_window(paquet);
	uint8_t seqNum = pkt_get_seqnum(paquet);
	uint16_t length = pkt_get_length(paquet);
	uint32_t timestamp = pkt_get_timestamp(paquet);
	uint32_t crc1 = pkt_get_crc1(paquet);
	const char* payload = pkt_get_payload(paquet);

	printf("get type: %u\n", type);
	printf("get tr: %u\n", tr);
	printf("get window: %u\n", window);
	printf("get seqNum: %u\n", seqNum);
	printf("get length: %u\n", length);
	printf("get timestamp: %u\n", timestamp);
	printf("get crc1: %u\n", crc1);
	printf("payload : %s\n", payload);


	pkt_del(paquet);
   	return 0;
}