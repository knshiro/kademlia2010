

#define RTLP_MAX_PAYLOAD_SIZE	1024
#define RTLP_MAX_SEND_BUF_SIZE	1024

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include "json-c-0.9/json.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

int _kdm_debug;

struct kademMessage {
    json_object *header;
    int payloadLength;    // Length of the payload and header
    char payload[4000];    // Payload of packet
};


struct dhtMachine {
   int port;
   char* address_ip;

};


//Packet to send
struct pkbuf {
	//struct rtlp_hdr hdr;		/* Header of the packet */
	int len;					/* Length of the payload*/
	char payload[RTLP_MAX_PAYLOAD_SIZE]; 	/* Payload of packet */
};


/* Client PCB */
struct rtlp_client_pcb {
	int sockfd;			/* Corresponding UDP socket */

	/* Current active pkbuf on the network */
	struct pkbuf send_buf[RTLP_MAX_SEND_BUF_SIZE]; 		

  	struct pkbuf recv_buf[RTLP_MAX_SEND_BUF_SIZE]; 		
  
	//struct sockaddr_in serv_addr;  /* The address of the server connected */

};


int kademSendMessage(int sockfd, struct kademMessage *message, char * dst_addr, int dst_port);
struct kademMessage kademUdpToMessage(char * udpPacket, int length);
void kdm_debug(const char *msg, ...);


int print_routing_table(struct rtlp_client_pcb* cpcb, struct dhtMachine* dhtmachine);
int print_object_ids(struct rtlp_client_pcb* cpcb, struct dhtMachine* dhtmachine);
int ping_node(struct rtlp_client_pcb* cpcb, struct dhtMachine* dhtmachine, char* node);
int put(struct rtlp_client_pcb* cpcb, struct dhtMachine* dhtmachine, char * value, char* ip_address, int port);
int find_node(struct rtlp_client_pcb* cpcb, struct dhtMachine* dhtmachine, char* node);
int kill_node(struct rtlp_client_pcb* cpcb, struct dhtMachine* dhtmachine);


/* Message types */
extern const char * const KADEM_QUERY;   
extern const char * const KADEM_ANSWER; 

/*Message query types */
extern const char * const KADEM_PING;
extern const char * const KADEM_STORE;
extern const char * const KADEM_FIND_NODE;
extern const char * const KADEM_FIND_VALUE;
extern const char * const KADEM_PRINT_OBJECT_IDS;
extern const char * const KADEM_KILL_NODE;
