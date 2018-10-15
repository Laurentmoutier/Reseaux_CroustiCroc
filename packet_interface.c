#include "packet_interface.h"

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
}
	
void pkt_del(pkt_t *pkt){
	free(pkt->payload);
    free(pkt);
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt){
	/* Your code will be inserted here */
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len){
	/* Your code will be inserted here */
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