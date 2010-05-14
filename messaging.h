

#define RTLP_MAX_PAYLOAD_SIZE	1024
#define RTLP_MAX_SEND_BUF_SIZE	1024

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

