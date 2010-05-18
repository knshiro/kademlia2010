#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <arpa/inet.h>
#include "messaging.h"
#include <dirent.h>
#include <sys/wait.h>



/* Message types */
const char * const KADEM_QUERY      =    "q";                                        
const char * const KADEM_ANSWER     =    "r";                                        
            
/*Message query types */
const char * const KADEM_PING       =    "ping";                                     
const char * const KADEM_STORE      =    "put";                                      
const char * const KADEM_FIND_NODE  =    "find_node";                                
const char * const KADEM_FIND_VALUE =    "get";  
const char * const KADEM_PRINT_TABLE = 	 "print_routing_table";
const char * const KADEM_PRINT_OBJECT_IDS = "print_object_ids";


/* create a socket and bind it to the local address and a port  */
int create_socket(int port){
  int sockfd;
  struct sockaddr_in client_addr;
  int length = sizeof(struct sockaddr_in);

  /*create socket*/
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
	  perror("ERROR opening socket");
  }
  printf("Socket created!\n");

  /* prepare attachment addrese = local addresse */
  client_addr.sin_family = AF_INET;

  /*  Give the local IP address and assign a port*/
  client_addr.sin_addr.s_addr=htonl(INADDR_ANY);
  

  if(port>-1)
  {
	client_addr.sin_port = htons(port);
  	if(bind(sockfd,(struct sockaddr *)&client_addr,length) == -1) {
 		perror("Impossible to bind socket to local address\n");
  		close(sockfd);
  		return -1;
 	}
  }

  return sockfd;
}


//////////////////////////////////////////////////////////



int main(int argc, char* argv[]){

//argv[1] = port of the client 
//argv[2] = port of the DHT
printf("client's port: %i\n", atoi(argv[1]));


//initialize DHT machine
struct dhtMachine dhtmachine;
dhtmachine.address_ip = "127.0.0.1";
dhtmachine.port = (atoi(argv[2]));

//initialize client machine
struct rtlp_client_pcb cpcb;
int sockfd =  create_socket(atoi(argv[1]));
printf("socket: %i\n",sockfd);
cpcb.sockfd = sockfd;

//current one other node's address:
struct sockaddr_in their_addr;
their_addr.sin_family = AF_INET;   
their_addr.sin_port = htons(atoi(argv[2])); 
inet_aton("127.0.0.1", &their_addr.sin_addr);



int flags = 0;
unsigned int fromlen, numread;
fd_set readfds;
struct sockaddr_in from;
fromlen = sizeof(from);
char buf_rec[400];
char buf_send[400];
char buf2[400];
struct timeval tv;
tv.tv_sec = 0;
int srv, leng_name, k;
char* to_ID=(char*)malloc(10*sizeof(char));
char* ID=(char*)malloc(10*sizeof(char));
char* delims = (char*)malloc(10*sizeof(char));
char* message = (char*)malloc(400*sizeof(char));

	

//test print_routing_table
//int k = print_routing_table(&cpcb, &dhtmachine);

		//Wait for data in stdin or in socket
		while(1){

			FD_ZERO(&readfds);
			FD_SET(cpcb.sockfd, &readfds);
			FD_SET(fileno(stdin), &readfds);

			//stdin or socket!
			srv = select(cpcb.sockfd+1, &readfds, NULL, NULL, &tv);
			//printf("srv: %i\n", srv);
			if (srv == -1) {
				perror("select"); // error occurred in select()
			} else {
				//Socket->printf
				if(FD_ISSET(cpcb.sockfd, &readfds)) {

					bzero(buf_rec, sizeof(buf_rec));
					if((numread = recvfrom(cpcb.sockfd,buf_rec,sizeof(buf_rec), 0, (struct sockaddr*)&from, &fromlen)) < 0){
						error("Couldnt' receive from socket");	
					}
					printf("Message received from %s on port %i\n",inet_ntoa(from.sin_addr),from.sin_port);
					printf("%s\n",buf_rec);
				} 

				//stdin->send
				else if(FD_ISSET(fileno(stdin), &readfds)){
					fgets(buf_send, sizeof(buf_send), stdin);
					//printf("buf_send: %s\n",buf_send);
					//int leg = strlen(buf_send);
					//printf("len: %i\n",leg);
					if(strcmp(buf_send,"//print_table")==0){
						if(print_routing_table(&cpcb, &dhtmachine)>0){
							printf("Request print table sent!\n");
						}
					}
					else if(strcmp(buf_send,"//ping")==0){
						if(print_object_ids(&cpcb, &dhtmachine)>0){
							printf("Request print object ids sent!\n");
						}
					}
					else if(strcmp(buf_send,"//print_object")==0){
						if(ping_node(&cpcb,&dhtmachine,"joe")>0){
							printf("Ping sent!\n");
						}
					}
					else{
						strcpy(buf2,buf_send);					
						delims = " ";
						to_ID = strtok(buf_send,delims);
						//printf("to_ID: %s\n",to_ID);
						leng_name = strlen(to_ID)-3;
						//printf("leng_name: %i\n",leng_name);
						if(leng_name<1){
							exit(-1);
						}
						strncpy(ID,to_ID+3,leng_name);
 					
						//strncpy(message,buf2+leng_name+4,strlen(buf2)-leng_name-4);
						strcpy(message,buf2+leng_name+4);
						//printf("message: %s\n\n",message);
						//changer sockaddr pour envoyer.
						//envoyer le resultat.*/
						if(sendto(sockfd, message, strlen(message), flags, (struct sockaddr *)&their_addr, sizeof their_addr)<0){
							error("Couldnt' send");
						}
						
					}
				}
			}
		}
return 0;
}



int print_routing_table(struct rtlp_client_pcb* cpcb, struct dhtMachine* dhtmachine){

	char* header2;

	struct kademMessage message;
	json_object *header;

	header = json_object_new_object(); 
	json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
	json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_PRINT_TABLE));
	
	message.header = header;
	message.payloadLength = 0;

	header2 = json_object_to_json_string(message.header);
    	
    	printf("message: %s\n",header2);	

	if(kademSendMessage(cpcb->sockfd, &message, dhtmachine->address_ip, dhtmachine->port)<0){
		return 0;
	}

return 1;
}



int print_object_ids(struct rtlp_client_pcb* cpcb, struct dhtMachine* dhtmachine){

	char* header2;

	struct kademMessage message;
	json_object *header;

	header = json_object_new_object(); 
	json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
	json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_PRINT_OBJECT_IDS));
	
	message.header = header;
	message.payloadLength = 0;

	header2 = json_object_to_json_string(message.header);
    	
    	printf("message: %s\n",header2);	

	if(kademSendMessage(cpcb->sockfd, &message, dhtmachine->address_ip, dhtmachine->port)<0){
		return 0;
	}

return 1;
}



//marche pas encore.
int ping_node(struct rtlp_client_pcb* cpcb, struct dhtMachine* dhtmachine, char* node){

	char* header2;

	struct kademMessage message;
	json_object *header, *argument;

	header = json_object_new_object(); 

	json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
	json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_PING));

 	argument = json_object_new_object();
   	json_object_object_add(argument,"id",json_object_new_string(node));
    	json_object_object_add(header,"a",json_object_get(argument));
	json_object_object_add(header, "a",json_object_new_string(argument));

	message.header = header;
	message.payloadLength = 0;

	header2 = json_object_to_json_string(message.header);
    	
    	printf("message: %s\n",header2);	

	if(kademSendMessage(cpcb->sockfd, &message, dhtmachine->address_ip, dhtmachine->port)<0){
		return 0;
	}

return 1;
}






