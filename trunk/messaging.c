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
#include "md5.h"

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
const char * const KADEM_KILL_NODE = "kill_node";

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

    //argv[1] = port of the client P2P
    //argv[2] = port of the client RPC
    //argv[3] = port of the DHT
    //argv[4] = identifiant/login

    printf("client's port_P2P: %i\n", atoi(argv[1]));

    char* login = argv[4];
    printf("your login: %s\n", login);

    //initialize DHT machine
    struct dhtMachine dhtmachine;
    strcpy(dhtmachine.address_ip,"127.0.0.1");
    dhtmachine.port = (atoi(argv[3]));

    //initialize client machine
    //socket and port P2P
    struct rtlp_client_pcb cpcb;
    int sockfd =  create_socket(atoi(argv[1]));
    printf("socket_P2P: %i\n",sockfd);
    cpcb.sockfd = sockfd;



    //socket and port RPC
    struct rtlp_client_pcb cpcb_rpc;
    int sockfd2 =  create_socket(atoi(argv[2]));
    printf("socket_RPC: %i\n",sockfd);
    cpcb_rpc.sockfd = sockfd2;
    cpcb_rpc.port_P2P = atoi(argv[1]);
    strcpy(cpcb_rpc.ip_P2P,"127.0.0.1");

    //IP address
    char hostname[100]; 
    char buffer[20];
    char port_str[6];
    int hostname_len = sizeof(hostname);
    struct hostent *client;
    gethostname(hostname, hostname_len);     
    client = gethostbyname(hostname);
    printf("Host address : %s\n",inet_ntoa(*((struct in_addr *)client->h_addr)));
    char* host_address;
    host_address = inet_ntoa(*((struct in_addr *)client->h_addr));


    //current one other node's address:
    struct sockaddr_in their_addr;
    their_addr.sin_family = AF_INET;   
    their_addr.sin_port = htons(2200); 
    inet_aton("127.0.0.1", &their_addr.sin_addr);


    int flags = 0;
    unsigned int fromlen, numread;
    fd_set readfds;
    struct sockaddr_in from;
    fromlen = sizeof(from);
    char buf_rec[400];
    char buf_send[400];
    char buf_send2[40];
    char buf2[400];
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    struct timeval tv2;
    tv2.tv_sec = 10;
    tv2.tv_usec = 0;
    struct kademMessage message_from_dht;
    int srv, srv2, leng_name, k;
    char ID[20], delims[20], message[400], message2[400], port_from_get_char[50];
    char * to_ID, *payload_from_get, *ip_address_from_get;
    int port_from_get_int;
    int get_send;
    int l=0;
    //Initialize / enter the network -> send a PUT.	
    if(put(&cpcb_rpc,&dhtmachine, login, host_address, argv[2])<0){
        printf("Cannot enter the network.");
        exit(-1);
    }

    //Wait for the answer of the PUT.
    printf("after put\n");
    FD_ZERO(&readfds);
    FD_SET(cpcb_rpc.sockfd, &readfds);
    while(l==0){
    	srv = select(cpcb_rpc.sockfd+1, &readfds, NULL, NULL, &tv);
        printf("srv: %i\n",srv);
    	if (srv == -1) {
            	perror("select"); // error occurred in select()
   	} else {
 		if(FD_ISSET(cpcb_rpc.sockfd, &readfds)) {
			        printf("after put2\n");
				l=1;
                                bzero(buf_rec, sizeof(buf_rec));
                                if((numread =recvfrom(cpcb_rpc.sockfd,buf_rec,sizeof(buf_rec),0,(struct sockaddr*) &from, &fromlen))<0){
                                    error("Couldnt' receive from socket");	
                                } else {
                                    printf("Message received from %s on port %i\n",inet_ntoa(from.sin_addr),ntohs(from.sin_port));
                                    printf("Buffer : %s\n",buf_rec);
				}
		}

    	}
   }

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
                int leg = strlen(buf_send);
                strncpy(buf_send2,buf_send,leg-1);
                bzero(message, sizeof(message));
                bzero(message2, sizeof(message2));
		printf("buf to send: %s\n", buf_send);


                if(strcmp(buf_send2,"//print_table")==0){
                    if(print_routing_table(&cpcb_rpc, &dhtmachine)>0){
                        printf("Request print table sent!\n");
                    }
                    bzero(buf_send2, sizeof(buf_send2));
                }
                else if(strcmp(buf_send2,"//print_object")==0){
                    if(print_object_ids(&cpcb_rpc, &dhtmachine)>0){
                        printf("Request print object ids sent!\n");
                    }
                    bzero(buf_send2, sizeof(buf_send2));
                }
                else if(strcmp(strtok(buf_send2," "),"//ping")==0){
                    char ping[40];
                    strcpy(ping,buf_send2+7);
                    if(ping_node(&cpcb_rpc,&dhtmachine,ping)>0){
                        printf("Ping sent!\n");
                    }
                    bzero(buf_send2, sizeof(buf_send2));
                    bzero(ping, sizeof(ping));
                }
                else if(strcmp(buf_send2,"//kill_node")==0){
                    if(kill_node(&cpcb_rpc, &dhtmachine)>0){
                        printf("kill_node sent!\n");
                    }
                    bzero(buf_send2, sizeof(buf_send2));
                }
                else if(strcmp(buf_send2,"//find_node")==0){
                        char find[40];
                        strcpy(find,buf_send2+12);
                        if(find_node(&cpcb_rpc,&dhtmachine,find)>0){
                            printf("Find node sent!\n");
                        }
                        bzero(buf_send2, sizeof(buf_send2));
                        bzero(find, sizeof(find));
                    }
                    //Send a GET to the DHT. Receive the address and port of the node. Send a message to a node. 
                    else{
                        bzero(ID, sizeof(ID));
                        strcpy(delims," ");
                        //ID: id of the node. message: message to send to the node.
                        strcpy(buf2,buf_send);					
                        to_ID = strtok(buf_send,delims);
                        printf("to_ID: %s\n",to_ID);
                        leng_name = strlen(to_ID)-3;
                        if(leng_name<1){
                            exit(-1);
                        }
                        strncpy(ID,to_ID+3,leng_name);

                        strcpy(message,buf2+leng_name+4);
                        strcat(message2,"from:");
			printf("messsage 2from:  %s\n", message2);
                        strcat(message2,login);
			printf("messsage login:  %s\n", message2);
                        strcat(message2," ");
                        strcat(message2,message);
			printf("messsage 2message:  %s\n", message2);
                        printf("ID: %s\n",ID);
                        //send a GET to the DHT with the ID of the node.
                        get_send = get(&cpcb_rpc,&dhtmachine,ID);
                        if(get_send>0){
                            printf("Get sent!\n");
                        }
                        //Wait for the GET resp.
                        FD_ZERO(&readfds);
                        FD_SET(cpcb_rpc.sockfd, &readfds);

                        srv2 = select(cpcb_rpc.sockfd+1, &readfds, NULL, NULL, &tv2);
                        //printf("srv2: %i\n", srv2);
                        if (srv2 == -1) {
                            perror("select"); // error occurred in select()
                        } 
                        else if(srv2 == 0) {
                            printf("No answer from the DHT\n");
                        }
                        else{
                            if(FD_ISSET(cpcb_rpc.sockfd, &readfds)) {
                                //bzero(buf_rec, sizeof(buf_rec));
                                if((numread =recvfrom(cpcb_rpc.sockfd,buf_rec,sizeof(buf_rec),0,(struct sockaddr*) &from, &fromlen))<0){
                                    error("Couldnt' receive from socket");	
                                } else {
				   
                                    printf("Message received from %s on port %i\n",inet_ntoa(from.sin_addr),ntohs(from.sin_port));
                                    printf("Buffer : %s\n",buf_rec);
                                    //extract IP address and Port.
                                    message_from_dht = kademUdpToMessage(buf_rec, numread);
                                    printf("ip_address/port from new node: %s\n",message_from_dht.payload);
                                    printf("numread %d, sizeof(bufrec) %d, payload%d\n",numread,sizeof(buf_rec),message_from_dht.payloadLength);
                                    //Charge the IP address and the port
                                    strcpy(delims,"/");
                                    payload_from_get = message_from_dht.payload;
                                    ip_address_from_get = strtok(payload_from_get,delims);
                                    
                                    printf("%s\n",ip_address_from_get);
                                    leng_name = strlen(ip_address_from_get);
                                    
                                    printf("%s\n",message_from_dht.payload+leng_name+1);
                                    strcpy(port_from_get_char, message_from_dht.payload+leng_name+1);
				    
                                    port_from_get_int = atoi(port_from_get_char);
                                    printf("ip_address to send: %s, port: %i\n", ip_address_from_get, port_from_get_int);

					printf("payload sent:%s\n",message2);
                                    their_addr.sin_port = htons(port_from_get_int); 
                                    inet_aton(ip_address_from_get, &their_addr.sin_addr);					

                                    if(sendto(sockfd, message2, strlen(message2), flags, (struct sockaddr *)&their_addr, sizeof their_addr)<0){
                                        error("Couldnt' send");
                                    }
                                }
                            }
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
        return -1;
    }

    return 0;
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
        return -1;
    }

    return 0;
}



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

    message.header = header;
    message.payloadLength = 0;

    header2 = json_object_to_json_string(message.header);

    printf("message: %s\n",header2);	

    if(kademSendMessage(cpcb->sockfd, &message, dhtmachine->address_ip, dhtmachine->port)<0){
        return -1;
    }

    return 0;
}


int kill_node(struct rtlp_client_pcb* cpcb, struct dhtMachine* dhtmachine){

    char* header2;

    struct kademMessage message;
    json_object *header;

    header = json_object_new_object(); 
    json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
    json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_KILL_NODE));

    message.header = header;
    message.payloadLength = 0;

    header2 = json_object_to_json_string(message.header);

    printf("message: %s\n",header2);	

    if(kademSendMessage(cpcb->sockfd, &message, dhtmachine->address_ip, dhtmachine->port)<0){
        return -1;
    }

    return 0;
}


int put(struct rtlp_client_pcb* cpcb, struct dhtMachine* dhtmachine, char * value, char* ip_address, int port){
    char port_str[10];
    char str[33], signature[16];

    //create the payload: "ip_address/port"
    char payload[70];
    char leng_payload[5];
    strcpy(payload, cpcb->ip_P2P);
    printf("ip_address of mess: %s\n",cpcb->ip_P2P);
    printf("payload avant: %s\n",payload);
    strcat(payload,"/");
    sprintf(port_str,"%d",cpcb->port_P2P);
    strcat(payload,port_str);
    strcat(payload,"/");
    char aro[40]="@";
    strcat(aro,value);
    strcat(payload,aro);

    //md5 convert @login to md5
    const char buf = "initialization of the buffer";
    //strcpy(buf,aro);
    hash(strlen(aro),aro, str, signature);
    printf("str: %s\n",str);

    int leng = strlen(payload)+1;
    sprintf(leng_payload,"%d",leng); 
    printf("payload: %s, size=%i\n",payload,leng);

    char* header2;

    struct kademMessage message;
    json_object *header, *argument;

    header = json_object_new_object(); 

    json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
    json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_STORE));

    argument = json_object_new_object();
    json_object_object_add(argument,"value",json_object_new_string(str));
    json_object_object_add(argument,"numbytes",json_object_new_string(leng_payload));
    json_object_object_add(header,"a",json_object_get(argument));

    message.header = header;
    strcpy(message.payload,payload);
    printf("message.payload: %s\n",message.payload);
    message.payloadLength = leng;

    header2 = json_object_to_json_string(message.header);

    printf("message: %s\n",header2);
    
    if(kademSendMessage(cpcb->sockfd, &message, dhtmachine->address_ip, dhtmachine->port)<0){
        return -1;
    }

    return 0;
}

//Find value.
int get(struct rtlp_client_pcb* cpcb, struct dhtMachine* dhtmachine, char* key){


    char* str[33], signature[16], aro[40]="@";
    strcat(aro,key);
    printf("aro: %s\n",aro);

    //strcpy(buf,aro);
    printf("test2\n");
    const unsigned int buf_len = strlen(aro);
    printf("buf_len: %i\n",buf_len);
    hash(buf_len,aro, str, signature);
    printf("str: %s\n",str);

    const char* header2;

    struct kademMessage message;
    json_object *header, *argument;

    header = json_object_new_object(); 

    json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
    json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_FIND_VALUE));

    argument = json_object_new_object();
    json_object_object_add(argument,"value",json_object_new_string(str));
    json_object_object_add(header,"a",json_object_get(argument));

    message.header = header;
    message.payloadLength = 0;

    header2 = json_object_to_json_string(message.header);
    printf("dhtmachine->address_ip: %s   dhtmachine->port: %i\n ",dhtmachine->address_ip,dhtmachine->port);
    printf("header GET: %s\n",header2);
    if(kademSendMessage(cpcb->sockfd, &message, dhtmachine->address_ip, dhtmachine->port)<0){
        return 0;
    }

    return 1;
}

int find_node(struct rtlp_client_pcb* cpcb, struct dhtMachine* dhtmachine, char* node){

    char* header2;

    struct kademMessage message;
    json_object *header, *argument;

    header = json_object_new_object(); 

    json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
    json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_FIND_NODE));

    argument = json_object_new_object();
    json_object_object_add(argument,"value",json_object_new_string(node));
    json_object_object_add(header,"a",json_object_get(argument));

    message.header = header;
    message.payloadLength = 0;

    header2 = json_object_to_json_string(message.header);

    printf("message: %s\n",header2);	

    if(kademSendMessage(cpcb->sockfd, &message, dhtmachine->address_ip, dhtmachine->port)<0){
        return -1;
    }

    return 0;
}



