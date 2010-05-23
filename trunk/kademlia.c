#include "kademlia.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "md5.h"
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "node.h"
#include <time.h>

/* Message types */
const char * const KADEM_QUERY      =    "q";
const char * const KADEM_ANSWER     =    "r";
const char * const KADEM_ERROR      =    "e";


/* Message query types */
const char * const KADEM_PING       =    "ping";
const char * const KADEM_STORE      =    "put";
const char * const KADEM_FIND_NODE  =    "find_node";
const char * const KADEM_FIND_VALUE =    "get";

/* Message error codes */
const char * const KADEM_ERROR_GENERIC          =    "201";
const char * const KADEM_ERROR_INVALID_TOKEN    =    "202";
const char * const KADEM_ERROR_PROTOCOL         =    "203";
const char * const KADEM_ERROR_METHOD_UNKNOWN   =    "204";
const char * const KADEM_ERROR_STORE            =    "205";

/* Message error values */
const char * const KADEM_ERROR_GENERIC_VALUE          =    "Generic Error";
const char * const KADEM_ERROR_INVALID_TOKEN_VALUE    =    "Invalid Token";
const char * const KADEM_ERROR_PROTOCOL_VALUE         =    "Protocol Error";
const char * const KADEM_ERROR_METHOD_UNKNOWN_VALUE   =    "Method Unknown Error";
const char * const KADEM_ERROR_STORE_VALUE            =    "Unable to store data";




void kdm_debug(const char *msg, ...){
    va_list ap;
    va_start(ap, msg);
    if(_kdm_debug) {
        vprintf(msg,ap);
    }
    va_end(ap);
}

int initMachine(struct kademMachine * machine, int port_local_rpc, int port_p2p){

    int sockfd;
    struct sockaddr_in local_rpc_addr, p2p_addr;
    int length = sizeof(struct sockaddr_in);

    char hostname[100]; 
    char buffer[20];
    char port_str[6];
    int hostname_len = sizeof(hostname);
    struct hostent *server;
    unsigned int buf_len;
    char signature[HASH_SIGNATURE_LENGTH];
    char id[HASH_STRING_LENGTH]; 
    


    //####################################
    // Create first socket for local RPC #
    //####################################

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0){ 
        perror("ERROR opening socket");
        return -1;
    }
    kdm_debug("Local RPC socket created\n");

    machine->sock_local_rpc = sockfd;
    kdm_debug("Socket %d stored in kadmelia machine\n",sockfd);

    // Specify type 
    local_rpc_addr.sin_family = AF_INET;

    // Give any address and listen to the specified port
    local_rpc_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    local_rpc_addr.sin_port = htons(port_local_rpc);

    if(bind(sockfd,(struct sockaddr *)&local_rpc_addr,length) == -1) {
        perror("Impossible to bind socket to local address\n");
        close(sockfd);
        return -1;
    }


    //#######################################
    // Create second socket for peer 2 peer #
    //#######################################

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0){ 
        perror("ERROR opening socket");
        return -1;
    }
    kdm_debug("P2p socket created\n");

    machine->sock_p2p = sockfd;
    kdm_debug("Socket %d stored in kadmelia machine\n",sockfd);

    // Specify type 
    p2p_addr.sin_family = AF_INET;

    // Give any address and listen to the specified port
    p2p_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    p2p_addr.sin_port = htons(port_p2p);

    if(bind(sockfd,(struct sockaddr *)&p2p_addr,length) == -1) {
        perror("Impossible to bind socket to local address\n");
        close(sockfd);
        return -1;
    }

    //#######################################
    // Creation of the id                   #
    //#######################################

    //Get the host ip
    gethostname(hostname, hostname_len);     
    kdm_debug("Host name : %s\n",hostname);
    server = gethostbyname(hostname);
    kdm_debug("Host addr : %s\n",inet_ntoa(*((struct in_addr *)server->h_addr)));

    //Create the string id 
    strcpy(buffer,inet_ntoa(*((struct in_addr *)server->h_addr)));
    strcat(buffer,"/");
    sprintf(port_str,"%d",port_p2p);
    strcat(buffer,port_str);
    buf_len = strlen(buffer);
    kdm_debug("Id not hashed: %s\n", buffer);

    //Hash
    md5_buffer(buffer,buf_len,signature);
    md5_sig_to_string(signature, id, HASH_STRING_LENGTH);
    kdm_debug("signature: %s\n", id);

    strcpy(machine->id,id);

    return 0;
}


int kademMaintenance(struct kademMachine * machine){
    
    //TODO refresh the k-buckets
    //TODO refresh the files stored
    machine->stored_values = clean(machine->stored_values,KADEM_TIMEOUT_REFRESH_DATA);
        
    return 0;
}


/*============================================
    Communication tools
  ============================================*/

int kademSendMessage(int sockfd, struct kademMessage *message, char * dst_addr, int dst_port){

    int messageSize = 0, sentBytes = 0;
    char *udpPacket;
    const char *header; 
    struct sockaddr_in serv_addr; 
    struct hostent *server;


    //###############################
    // Find the address of the peer #
    //###############################

    // Get Server address from supplied hostname
    server = gethostbyname(dst_addr);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    kdm_debug("Name resolved\n");

    // Zero out server address - IMPORTANT!
    bzero((char *) &serv_addr, sizeof(serv_addr));
    // Set address type to IPv4 */
    serv_addr.sin_family = AF_INET;
    // Copy discovered address from hostname to struct
    bcopy((char *)server->h_addr,
            (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);
    // Assign port number, using network byte order
    serv_addr.sin_port = htons(dst_port);
    kdm_debug("Address set\n");


    //###############################
    // Create the data to be sent   #
    //###############################

    //First the header
    header = json_object_to_json_string(message->header);
    messageSize += strlen(header);
    kdm_debug("Header sent:\n%s\n",header);

    //If needed include a payload
    if(message->payloadLength>0){
        if(message->payloadLength> KADEM_MAX_PAYLOAD_SIZE){
            fprintf(stderr,"Payload too big to be sent (%d bytes)\n", message->payloadLength);
            return 1;
        }
        udpPacket = (char *) malloc(messageSize + message->payloadLength+1);
        memcpy(udpPacket,header,messageSize);
        memcpy(udpPacket+messageSize,message->payload,message->payloadLength);
        udpPacket[messageSize + message->payloadLength] = '\0';
        messageSize += message->payloadLength;
    }
    else{
        udpPacket = (char *) malloc(messageSize + 1);
        memcpy(udpPacket,header,messageSize);
        udpPacket[messageSize] = '\0';
    }


    //###############################
    // Write message to socket      #
    //###############################

    if((sentBytes = sendto(sockfd, udpPacket, messageSize , 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0)
    {
        perror("Could not send packet!");
        return -1;
    }
    kdm_debug("Packet sent %d bytes (%d header, %d payload)\n", sentBytes, strlen(header),message->payloadLength);
    kdm_debug("%s\n", udpPacket);
    free(udpPacket);

    return 0; 
}

struct kademMessage kademUdpToMessage(char * udpPacket, int length){
    int payloadLength;
    struct kademMessage message;
    json_object *new_obj;
    new_obj = json_tokener_parse(udpPacket);
    kdm_debug("Message header: %s\n",json_object_to_json_string(new_obj));
    if((payloadLength = length - strlen(json_object_to_json_string(new_obj)) )>0){
        kdm_debug("Found payload of %d bytes\n",payloadLength); 
        memcpy(&message.payload,udpPacket+(length-payloadLength),payloadLength);
    }
    message.header = new_obj;
    message.payloadLength = payloadLength;
    return message;
}

int kademSendError(struct kademMachine * machine, const char *transactionId, const char * const code, const char * const message, char *addr, int port){

    struct kademMessage error_message;
    int ret;
    json_object *header, *error;

    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));

    json_object_object_add(header, "y",json_object_new_string(KADEM_ERROR));

    error = json_object_new_object();
    json_object_object_add(error,"code",json_object_new_string(code));
    json_object_object_add(error,"value",json_object_new_string(message));

    json_object_object_add(header, KADEM_ERROR,json_object_get(error));

    error_message.header = header;
    error_message.payloadLength = 0;

    ret = kademSendMessage(machine->sock_p2p, &error_message, addr, port);
    json_object_put(header);
    json_object_put(error);

    return ret;
}




int generateTransactionId(char * transactionId, char * id){

    time_t now;
    int len;
    char string_time[32],buffer[HASH_STRING_LENGTH], buffer2[2*HASH_STRING_LENGTH],signature[HASH_SIGNATURE_LENGTH];
   
    //Retrieve time
    now = time(NULL);
    sprintf(string_time,"%ld",now);
    len = strlen(string_time); 
    kdm_debug("Time : %s (%d chars)\n", string_time, len);

    //Hash time
    md5_buffer(string_time,len,signature);
    md5_sig_to_string(signature, buffer, HASH_STRING_LENGTH);
    kdm_debug("Time hashed: %s\n", buffer);
  
    //Hash time + id
    strcpy(buffer2,buffer);
    strcat(buffer2,id);
    len = strlen(buffer2);
    md5_buffer(buffer2,len,signature);
    md5_sig_to_string(signature, transactionId, HASH_STRING_LENGTH);
    
    kdm_debug("Transaction id generated: %s\n", transactionId);

    return 0;
}


/*============================================
    P2P communication
  ============================================*/

int kademPing(struct kademMachine * machine, char * addr, int port){

    struct kademMessage message;
    int ret;
    json_object *header, *argument;
    char transactionId[HASH_SIGNATURE_LENGTH]; 

    generateTransactionId(transactionId,machine->id);


    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));
    json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
    json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_PING));

    argument = json_object_new_object();
    json_object_object_add(argument,"id",json_object_new_string(machine->id));
    json_object_object_add(header,"a",json_object_get(argument));

    message.header = header;
    message.payloadLength = 0;

    ret = kademSendMessage(machine->sock_p2p, &message, addr, port);
    json_object_put(header);
    json_object_put(argument);


    return ret;
}

int kademPong(struct kademMachine *machine, struct kademMessage *message, char * addr, int port){

    struct kademMessage answer_message;
    int ret;
    json_object *header, *response;
    const char * transactionId; 

    transactionId = json_object_get_string(json_object_object_get(message->header,"t"));


    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));
    json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

    response = json_object_new_object();
    json_object_object_add(response,"id",json_object_new_string(machine->id));
    json_object_object_add(header, KADEM_ANSWER,json_object_get(response));

    answer_message.header = header;
    answer_message.payloadLength = 0;

    ret = kademSendMessage(machine->sock_p2p, &answer_message, addr, port);
    json_object_put(header);
    json_object_put(response);


    return ret;

}

int kademHandlePong(char * addr, int port){


    return 0; 
}

int kademFindNode(struct kademMachine * machine, char * target_id, char * addr, int port){

    struct kademMessage message;
    int ret;
    json_object *header, *argument;
    char transactionId[HASH_SIGNATURE_LENGTH]; 

    generateTransactionId(transactionId,machine->id);

    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));
    json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
    json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_FIND_NODE));

    argument = json_object_new_object();
    json_object_object_add(argument,"id",json_object_new_string(machine->id));
    json_object_object_add(argument,"target",json_object_new_string(target_id));

    json_object_object_add(header,"a",json_object_get(argument));

    message.header = header;
    message.payloadLength = 0;

    ret = kademSendMessage(machine->sock_p2p, &message, addr, port);
    json_object_put(header);
    json_object_put(argument);

    return ret;

}

int kademHandleFindNode(struct kademMachine * machine, struct kademMessage * message, char * addr, int port){

    struct kademMessage answer_message;
    int ret;
    json_object *header, *response, *response_array;
    const char * transactionId;

    transactionId = json_object_get_string(json_object_object_get(message->header,"t"));


    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));
    json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

    response = json_object_new_object();
    json_object_object_add(response,"id",json_object_new_string(machine->id));

    response_array = json_object_new_array();
    //TODO implement find nodes

    json_object_object_add(response,"nodes",json_object_get(response_array)); 

    json_object_object_add(header, KADEM_ANSWER,json_object_get(response));

    answer_message.header = header;
    answer_message.payloadLength = 0;

    ret = kademSendMessage(machine->sock_p2p, &answer_message, addr, port);
    json_object_put(header);
    json_object_put(response);
    json_object_put(response_array);

    return ret;

}

int kademHandleAnswerFindNode(struct kademMachine * machine, struct kademMessage * message){


    return 0; 
}

int kademFindValue(struct kademMachine * machine, char * value, char *addr, int port){

    struct kademMessage message;
    int ret;
    json_object *header, *argument;
    char * token = "tokenabc1";     //TODO create a real token
    char transactionId[HASH_SIGNATURE_LENGTH]; 

    generateTransactionId(transactionId,machine->id);

    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));
    json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
    json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_FIND_VALUE));

    argument = json_object_new_object();
    json_object_object_add(argument,"id",json_object_new_string(machine->id));
    json_object_object_add(argument,"token",json_object_new_string(token));
    json_object_object_add(argument,"value",json_object_new_string(value));

    json_object_object_add(header,"a",json_object_get(argument));

    message.header = header;
    message.payloadLength = 0;

    ret = kademSendMessage(machine->sock_p2p, &message, addr, port);
    json_object_put(header);
    json_object_put(argument);

    return ret;

}

int kademHandleFindValue(struct kademMachine * machine, struct kademMessage * message,char *addr, int port){

    struct kademMessage answer_message;
    int ret;
    json_object *header, *response, *node_array;
    const char * transactionId; 
    char * token = "tokenabc1";     //TODO create a real token

    transactionId = json_object_get_string(json_object_object_get(message->header,"t"));

    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));
    json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

    response = json_object_new_object();
    json_object_object_add(response,"id",json_object_new_string(machine->id));
    json_object_object_add(response,"token",json_object_new_string(token));

    answer_message.payloadLength = 0;

    node_array = json_object_new_array();
    //TODO look for value
    if(1){

        json_object_object_add(response,"value",json_object_new_string("value1"));
        json_object_object_add(response,"numbytes",json_object_new_string("0"));
        answer_message.payloadLength = 0;

    }
    else {
        //TODO Fill with value
        json_object_object_add(response,"nodes",json_object_get(node_array));

    }

    json_object_object_add(header, KADEM_ANSWER,json_object_get(response));
    answer_message.header = header;

    ret = kademSendMessage(machine->sock_p2p, &answer_message, addr, port);
    json_object_put(header);
    json_object_put(response);
    json_object_put(node_array);

    return ret;

}

int kademHandleAnswerFindValue(struct kademMachine * machine, struct kademMessage * message){

    return 0; 
}

int kademStoreValue(struct kademMachine * machine, char * token, char * value, char * data, int data_len, char *dst_addr, int dst_port){

    struct kademMessage message;
    int ret;
    json_object *header, *argument;
    char transactionId[HASH_SIGNATURE_LENGTH]; 

    generateTransactionId(transactionId,machine->id);

    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));
    json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
    json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_FIND_VALUE));

    argument = json_object_new_object();
    json_object_object_add(argument,"id",json_object_new_string(machine->id));
    json_object_object_add(argument,"token",json_object_new_string(token));
    json_object_object_add(argument,"value",json_object_new_string(value));
    json_object_object_add(argument,"numbytes",json_object_new_int(data_len));

    json_object_object_add(header,"a",json_object_get(argument));

    message.header = header;
    message.payloadLength = data_len;
    memcpy(message.payload,data,data_len); 

    ret = kademSendMessage(machine->sock_p2p, &message, dst_addr, dst_port);
    json_object_put(header);
    json_object_put(argument);

    return ret;
}

int kademHandleStoreValue(struct kademMachine * machine, struct kademMessage * message,char * addr, int port){
    struct kademMessage answer_message;
    int ret,found = 0,i=0;
    json_object *header, *response, *query_argument;
    const char *transactionId, *token;
    char *value, *key;
    
    transactionId = json_object_get_string(json_object_object_get(message->header,"t"));
    query_argument = json_object_object_get(message->header,"a");
    
    token = json_object_get_string(json_object_object_get(query_argument,"token"));

    // Verify if token exists
    while(!found && i<KADEM_MAX_NB_TOKEN){
        if(strcmp(machine->tokens[i],token) == 0){
            found = 1;
            strcpy(machine->tokens[i],"");
        }
        else{
            i++;
        }
    }
    
    if(found){
    //TODO fix payload + create_store_file
    key = json_object_get_string(json_object_object_get(query_argument,"value"));
    value = message->payload;
    machine->stored_values = insert_to_tail_file(machine->stored_values, create_store_file(key,value));


    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));


    json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

    response = json_object_new_object();
    json_object_object_add(response,"id",json_object_new_string(machine->id));
    json_object_object_add(header, KADEM_ANSWER,json_object_get(response));
    
    
    answer_message.header = header;
    answer_message.payloadLength = 0;

    ret = kademSendMessage(machine->sock_p2p, &answer_message, addr, port);
    json_object_put(header);
    json_object_put(response);
    }
    else {
       //TODO fix ret value
       ret = kademSendError(machine, transactionId,KADEM_ERROR_STORE,KADEM_ERROR_STORE_VALUE,addr,port); 
    }
    return ret;

}

int kademHandleAnswerStoreValue(struct kademMachine * machine, struct kademMessage * message){
    return 0;
}

int startKademlia(struct kademMachine * machine){
    int ret_select, num_read,from_port;
    socklen_t from_len;
    fd_set readfds;
    struct timeval tv;
    struct kademMessage message;
    struct sockaddr_in from;
    char udpPacket[KADEM_MAX_PAYLOAD_SIZE+100];
    const char * buffer;
    char * from_addr;


    from_len = sizeof(struct sockaddr_in);

    while(1){

        // clear the set ahead of time
        FD_ZERO(&readfds);
        // add our descriptors to the set
        FD_SET(machine->sock_local_rpc,&readfds);
        FD_SET(machine->sock_p2p,&readfds);

        // Set timeout
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret_select = select(max(machine->sock_local_rpc, machine->sock_p2p)+1, &readfds, NULL, NULL, &tv);
        //If there is an error just quit
        if(ret_select <0){
            perror("Select :");
            return 1;
        }
        //If timed out do refreshing functions
        else if (ret_select == 0){
        }
        //If received a message
        else{

            //#####################################
            // Messages from local rpc            #
            //#####################################
            if(FD_ISSET(machine->sock_local_rpc, &readfds)) {
                if((num_read = recvfrom(machine->sock_local_rpc,udpPacket,sizeof(udpPacket),0,(struct sockaddr*) &from, &from_len)) < 0)
                {
                    perror("Couldnt' receive from socket");
                }
                message = kademUdpToMessage(udpPacket,num_read);
            }


            //#####################################
            // Messages from kademlia p2p network #
            //#####################################
            if(FD_ISSET(machine->sock_p2p, &readfds)) {
                if((num_read = recvfrom(machine->sock_p2p,udpPacket,sizeof(udpPacket),0,(struct sockaddr*) &from, &from_len)) < 0)
                {
                    perror("Couldnt' receive from socket");
                }
                message = kademUdpToMessage(udpPacket,num_read);

                // Retrieve ip and port from sending node
                from_port = ntohs(from.sin_port);
                from_addr = inet_ntoa(from.sin_addr);                
                kdm_debug("Receve message from: %s (%d)\n",from_addr,from_port);

                // Determine the type of the message 
                buffer = json_object_get_string(json_object_object_get(message.header,"y"));
                // Message is a query
                if (strcmp(buffer,KADEM_QUERY) == 0){

                    // Determine the type of the query
                    buffer = json_object_get_string(json_object_object_get(message.header,KADEM_QUERY));

                    if (strcmp(buffer,KADEM_PING) == 0){
                        kademPong(machine, &message, from_addr, from_port);  
                    }                
                    if (strcmp(buffer,KADEM_STORE) == 0){
                        kademHandleStoreValue(machine, &message, from_addr, from_port);  
                    }                
                    if (strcmp(buffer,KADEM_FIND_NODE) == 0){
                        kademHandleFindNode(machine, &message, from_addr, from_port);  
                    }                
                    if (strcmp(buffer,KADEM_FIND_VALUE) == 0){
                        kademHandleFindValue(machine, &message, from_addr, from_port);  
                    }                

                }


                // Message is an answer
                else if (strcmp(buffer,KADEM_ANSWER) == 0){
                }


                // Message is an error
                else if (strcmp(buffer,KADEM_ERROR) == 0){
                }

                // Message type is unknown
                else{
                }
            }
        }
    }

    return 0;

}




/*============================================
    RPC communication
  ============================================*/


int RPCHandleStoreValue(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){

	//TODO: store the value.

	struct kademMessage answer_message;
	json_object *header, *argument;
	char* ok = "OK";
	header = json_object_new_object(); 

	json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));


   	argument = json_object_new_object();
   	json_object_object_add(argument,"resp",json_object_new_string(ok));
    	json_object_object_add(header,KADEM_ANSWER,json_object_get(argument));

	answer_message.header = header;
	answer_message.payloadLength = 0;


	if(kademSendMessage(machine->sock_local_rpc, &answer_message, addr, port)<0){
		return -1;
	}
	
	return 0;
}

int RPCHandlePing(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){

	//TODO: do the ping.

	struct kademMessage answer_message;
	json_object *header, *argument;
	char* ok = "OK"; //or Not OK to do.
	header = json_object_new_object(); 

	json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

   	argument = json_object_new_object();
   	json_object_object_add(argument,"resp",json_object_new_string(ok));
    	json_object_object_add(header,KADEM_ANSWER,json_object_get(argument));

	answer_message.header = header;
	answer_message.payloadLength = 0;


	if(kademSendMessage(machine->sock_local_rpc, &answer_message, addr, port)<0){
		return -1;
	}
	
	return 0;
}


int RPCHandlePrintRoutingTable(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){

	//TODO: print routing table.
	int check = -1;
	check = print_routing_table(machine->routes);
		
	struct kademMessage answer_message;
	json_object *header, *argument;
	char* ok = "OK";
	header = json_object_new_object(); 

	json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

   	argument = json_object_new_object();
   	json_object_object_add(argument,"resp",json_object_new_string(ok));
    	json_object_object_add(header,KADEM_ANSWER,json_object_get(argument));

	answer_message.header = header;
	answer_message.payloadLength = 0;


	if(kademSendMessage(machine->sock_local_rpc, &answer_message, addr, port)<0){
		return -1;
	}
	
	return 0;
}


int RPCHandlePrintObjects(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){

	//TODO: print objects	

	struct kademMessage answer_message;
	json_object *header, *argument;
	char* ok = "OK";
	header = json_object_new_object(); 

	json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

   	argument = json_object_new_object();
   	json_object_object_add(argument,"resp",json_object_new_string(ok));
    	json_object_object_add(header,KADEM_ANSWER,json_object_get(argument));

	answer_message.header = header;
	answer_message.payloadLength = 0;


	if(kademSendMessage(machine->sock_local_rpc, &answer_message, addr, port)<0){
		return -1;
	}
	return 0;
}


int RPCHandleFindValue(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){
	
	//TODO: find the appropriate IP address and port.
	char* ip_address_to_send;
	int port_to_send;


	//create the payload: "ip_address/port":
	char* ok = "OK";
	char payload[4000];
	char leng_payload[5];
	strcat(payload, ip_address_to_send);
	strcat(payload,"/");
	strcat(payload,port_to_send);
	int leng = strlen(payload);
	sprintf(leng_payload,"%d",leng); 
	printf("payload: %s, size=%i\n",payload,leng);
	
	char* header2;

	struct kademMessage answer_message;
	json_object *header, *argument;

	header = json_object_new_object(); 

	json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

   	argument = json_object_new_object();
   	json_object_object_add(argument,"resp",json_object_new_string(ok));
	json_object_object_add(argument,"numbytes",json_object_new_string(leng_payload));
    	json_object_object_add(header,"r",json_object_get(argument));

	answer_message.header = header;
	strcpy(answer_message.payload,payload);
	kdm_debug("message.payload: %s\n",answer_message.payload);
	answer_message.payloadLength = leng;

	header2 = json_object_to_json_string(answer_message.header);
    	
    	kdm_debug("message: %s\n",header2);

	if(kademSendMessage(machine->sock_local_rpc, &answer_message, addr, port)<0){
		return -1;
	}

	return 0;
}


int RPCHandleKillNode(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){

	//TODO: kill_node
		
	struct kademMessage answer_message;
	json_object *header, *argument;
	char* ok = "OK";
	header = json_object_new_object(); 

	json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

   	argument = json_object_new_object();
   	json_object_object_add(argument,"resp",json_object_new_string(ok));
    	json_object_object_add(header,KADEM_ANSWER,json_object_get(argument));

	answer_message.header = header;
	answer_message.payloadLength = 0;

	if(kademSendMessage(machine->sock_local_rpc, &answer_message, addr, port)<0){
		return -1;
	}
	
	return 0;
}


int RPCHandleFindNode(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){


	return 0;	
}

