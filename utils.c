

#include "messaging.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "md5.h"
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


/* Message types */
extern const char * const KADEM_QUERY;   
extern const char * const KADEM_ANSWER; 

/*Message query types */
extern const char * const KADEM_PING;
extern const char * const KADEM_STORE;
extern const char * const KADEM_FIND_NODE;
extern const char * const KADEM_FIND_VALUE;
extern const char * const KADEM_PRINT_TABLE;



void kdm_debug(const char *msg, ...){
    va_list ap;
    va_start(ap, msg);
    if(_kdm_debug) {
        vprintf(msg,ap);
    }
    va_end(ap);
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
        if(message->payloadLength> 4000){
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




/************************/



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

