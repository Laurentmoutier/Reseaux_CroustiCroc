#include "packet_interface.h"
#include <stddef.h> /* size_t */ 
#include <stdint.h> /* uintx_t */
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> /* memcpy */
#include <arpa/inet.h> // NEW INCLUDE: mettre sur inginious
#include <zlib.h> // NEW INCLUDE: mettre sur inginious



/* Extra #includes */
/* Your code will be inserted here */

struct __attribute__((__packed__)) pkt {
	unsigned int type:2;
	unsigned int trFlag:1;
	unsigned int window:5; // network byte order sur certains fields ==> changer les getters et setters!!
	uint8_t seqNum;
	uint16_t length;
	uint32_t timestamp;
	uint32_t crc1;
	char* payload; // const char* instead of just char* ??
	uint32_t crc2;
};

/* Extra code */
/* Your code will be inserted here */

unsigned int crc32b(unsigned char *message) {
	// fonction prise de stackoverflow. Il faudra la re-ecrire.
	// https://stackoverflow.com/questions/21001659/crc32-algorithm-implementation-in-c-without-a-look-up-table-and-with-a-public-li
   int i, j;
   unsigned int byte, crc, mask;

   i = 0;
   crc = 0xFFFFFFFF;
   while (message[i] != 0) {
      byte = message[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
      i = i + 1;
   }
   return ~crc;
}

pkt_t* pkt_new(){
	pkt_t* pkt_ptr = malloc(sizeof(struct pkt));
	return pkt_ptr;
}
	
void pkt_del(pkt_t *pkt){
	free(pkt->payload);
    free(pkt);
}

void setTypeTrWinFromData(const char* data, unsigned int* type, unsigned int* trFlag, unsigned int* window){

	int typeMask = 0b11;
	int trMask = 0b100;
	int windowMask = 0b11111000;
	unsigned int *dat = (unsigned int*)data;
	*type = *dat & typeMask;
	*trFlag = (*dat & trMask) >> 2;
	*window = (*dat & windowMask) >> 3;

}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt){
	printf("decode:\n");
	unsigned int type;
	unsigned int trFlag;
	unsigned int window; 
	setTypeTrWinFromData(data, &type, &trFlag, &window);
	uint8_t seqNum = *(data + 1);
	printf("seq num : %u\n", seqNum);
	uint16_t length;
	memcpy(&length, data+2, 2);
	length = htons(length);
	printf("len : %u\n", length);
	uint32_t timestamp;
	memcpy(&timestamp, data+4, 4);
	timestamp = htonl(timestamp);
	uint32_t crc1;
	memcpy(&crc1, data+8, 4);
	crc1 = htonl(crc1);
	printf("time : %u\n", timestamp);
	printf("crc1 : %u\n", crc1);
// tr = 0 pour crc
	char* payload; 
	uint32_t crc2;
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len){
	int payLength = pkt_get_length(pkt);
	int totalSize = sizeof(pkt_t) + payLength - sizeof(char*);
	if(totalSize > *len){ //buffer trop petit
		return E_NOMEM;
	}
	int copyLength = 1 + 1 + 2 + 4 + 4; ///8bits + seqNum(8bit) + length(16bit) + timeStamp(32bit) + crc32(32bit)
	memcpy(buf, pkt, copyLength);
	if(payLength == 0){ //no payload
		return PKT_OK;
	}
	// has a payload
	memcpy(buf + copyLength, pkt->payload, payLength);
	memcpy(buf + copyLength + payLength, &pkt->crc2, 4);
	return PKT_OK;
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
	if(type > 0b11 || type== 0b0){
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
	pkt_status_code trRet = pkt_set_tr(paquet, 0b0);
	pkt_status_code winRet = pkt_set_window(paquet, 0b11111);
	pkt_status_code seqRet = pkt_set_seqnum(paquet, 0b01000000);
	pkt_status_code lenRet = pkt_set_length(paquet, 0b0000100);
	// uLong crc1 = crc32(crc1, (const Bytef *)paquet, 8);
	pkt_status_code crc1Ret = pkt_set_crc1(paquet, 0b10001111111111111111111111110001);
	pkt_status_code crc2Ret = pkt_set_crc2(paquet, 0b10001111111111111111111111110001);
	pkt_status_code timeRet = pkt_set_timestamp(paquet, 0b01111111111111111111111111111110);


	char* pay = (char*)malloc(8*sizeof(char));
	strcpy(pay, "aaaaa");
	pkt_set_payload(paquet, pay, 5);

	ptypes_t type = pkt_get_type(paquet);
	uint8_t tr = pkt_get_tr(paquet);
	uint8_t window = pkt_get_window(paquet);
	uint8_t seqNum = pkt_get_seqnum(paquet);
	uint16_t length = pkt_get_length(paquet);
	uint32_t timestamp = pkt_get_timestamp(paquet);
	// uint32_t crc1 = pkt_get_crc1(paquet);
	const char* payload = pkt_get_payload(paquet);

	printf("get type: %u\n", type);
	printf("get tr: %u\n", tr);
	printf("get window: %u\n", window);
	printf("get seqNum: %u\n", seqNum);
	printf("get length: %u\n", length);
	printf("get timestamp: %u\n", timestamp);
	// printf("get crc1: %u\n", crc1);
	printf("payload : %s\n", payload);

	printf("\n");
	size_t len = 30;
	char* buff = malloc(len*sizeof(char));
	pkt_encode(paquet, buff, &len);
	pkt_t* decPakt = pkt_new();
	pkt_decode(buff, 5+16,decPakt);
	pkt_del(paquet);
   	return 0;
}