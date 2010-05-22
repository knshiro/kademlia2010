#include "kademlia.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "md5.h"
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


/* Message types */
const char * const KADEM_QUERY      =    "q";                                        
const char * const KADEM_ANSWER     =    "r";                                        
            
/*Message query types */
const char * const KADEM_PING       =    "ping";                                     
const char * const KADEM_STORE      =    "put";                                      
const char * const KADEM_FIND_NODE  =    "find_node";                                
const char * const KADEM_FIND_VALUE =    "get";  


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
    char signature[128];
    char id[36]; 



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
    md5_sig_to_string(signature, id, 33);
    kdm_debug("signature: %s\n", id);

    strcpy(machine->id,id);

    return 0;
}

int kademSendMessage(int sockfd, struct kademMessage *message, char * dst_addr, int dst_port){

    int messageSize = 0, sentBytes = 0;
    char *udpPacket;
    char *header; 
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
    kdm_debug("Header sent:\n%s",header);
    
    //If needed include a payload
    if(message->payloadLength>0){
        if(message->payloadLength> KADEM_MAX_PAYLOAD_SIZE){
            fprintf(stderr,"Payload too big to be sent (%d bytes)\n", message->payloadLength);
            return 1;
        }
        udpPacket = (char *) malloc(messageSize + message->payloadLength);
        memcpy(udpPacket,header,messageSize);
        memcpy(udpPacket+messageSize,message->payload,message->payloadLength);
        messageSize += message->payloadLength;
    }
    else{
        udpPacket = header;
    }


    //###############################
    // Write message to socket      #
    //###############################
    
    if((sentBytes = sendto(sockfd, udpPacket, messageSize , 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0)
    {
        perror("Could not send packet!");
        return -1;
    }
    kdm_debug("Packet sent (%d bytes)\n", sentBytes);
    
    return 0; 
}

/**
 * Parses a udp packet to a kadmelia message 
 */

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

/**
 *  sends a kadmelia ping request to addr, port
 */

int kademPing(struct kademMachine * machine, char * addr, int port){
    
    struct kademMessage message;
    int ret;
    json_object *header, *argument;
    char * transactionId = "01";    //TODO create a real transactionId
   
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

/**
 *  Sends a kadmelia ping answer to addr, port
 */

int kademPong(struct kademMachine machine, struct kademMessage *message, char * addr, int port){
    
    return 0; 
}

int kademHandlePong(char * addr, int port){


    return 0; 
}

int kademFindNode(struct kademMachine * machine, char * target_id, char * addr, int port){
    
    struct kademMessage message;
    int ret;
    json_object *header, *argument;
    char * transactionId = "01";    //TODO create a real transactionId
   
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

int kademHandleFindNode(struct kademMachine * machine, struct kademMessage * message){


    return 0; 
}

int kademHandleAnswerFindNode(struct kademMachine * machine, struct kademMessage * message){


    return 0; 
}

int kademFindValue(struct kademMachine * machine, char * value, char *addr, int port){

    struct kademMessage message;
    int ret;
    json_object *header, *argument;
    char * transactionId = "01";    //TODO create a real transactionId
    char * token = "tokenabc1";     //TODO create a real token

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

int kademHandleFindValue(struct kademMachine * machine, struct kademMessage * message){

    return 0; 
}

int kademHandleAnswerFindValue(struct kademMachine * machine, struct kademMessage * message){

    return 0; 
}

int kademStoreValue(struct kademMachine * machine, char * token, char * value, char * data, int data_len, char *dst_addr, int dst_port){
    
    struct kademMessage message;
    int ret;
    json_object *header, *argument;
    char * transactionId = "01";    //TODO create a real transactionId

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

int kademHandleStoreValue(struct kademMachine * machine, struct kademMessage * message){
    return 0;
}

int kademHandleAnswerStoreValue(struct kademMachine * machine, struct kademMessage * message){
    return 0;
}







