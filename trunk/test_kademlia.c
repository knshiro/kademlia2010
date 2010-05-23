#include "kademlia.h"
#include "json-c-0.9/json.h"
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <ctype.h>
#include <unistd.h>

#include <string.h>
//extern _kdm_debug;

int main(int argc, char *argv[]){
    struct kademMachine machine;
    json_object *header, *argument;
    struct kademMessage message;
    char udpPacket[400], hash[HASH_STRING_LENGTH];
    char * transactionId = "04";
    char * addr = "127.0.0.1";
    int port = 4000;

    _kdm_debug = 1;
    initMachine(&machine,6000,7000);
    kdm_debug("Machine inited, id: %s\n", machine.id);
  
    //###############################
    // Test of udpToMessage         #
    //###############################
    
    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));
    json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
    json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_PING));

    argument = json_object_new_object();
    json_object_object_add(argument,"id",json_object_new_string(machine.id));
    json_object_object_add(header,"a",json_object_get(argument));

    strcpy(udpPacket,json_object_to_json_string(header));
    strcat(udpPacket,"bonjour");
    message = kademUdpToMessage(udpPacket,strlen(json_object_to_json_string(header))+7);
    printf("Header size: %i\n", strlen(json_object_to_json_string(message.header)));
    printf("There is a payload : %i\n", message.payloadLength >0);
    printf("Payload has the good size : %i\n", message.payloadLength == 7);
    printf("Payload is %s\n", message.payload);

    generateTransactionId(hash,machine.id);
    printf("Transaction id is %s\n", hash);
    

    //###############################
    // End  Test of udpToMessage    #
    //###############################

    kdm_debug(">>>>Ping\n");
    kademPing(&machine,"127.0.0.1",4000);
    kdm_debug("<<<<Ping\n\n");
   
    kdm_debug(">>>>Pong\n");
    kademPong(&machine,&message,addr,port);
    kdm_debug("<<<<Pong\n\n");
    
    kdm_debug(">>>>Find Node\n");
    kademFindNode(&machine,"test_node",addr,port);
    kdm_debug("<<<<Find Node\n\n");
    
    kdm_debug(">>>>Handle Find Node\n");
    kademHandleFindNode(&machine,&message,addr,port);
    kdm_debug("<<<<Handle Find Node\n\n");
    
    kdm_debug(">>>>Find value\n");
    kademFindValue(&machine,"value1",addr,port);
    kdm_debug("<<<<Find value\n\n");
    
    kdm_debug(">>>>Handle Find value\n");
    kademHandleFindValue(&machine,&message,addr,port);
    kdm_debug("<<<<Handle Find value\n\n");
    
    kdm_debug(">>>>Store value\n");
    kademStoreValue(&machine,"token1","value1","127.0.0.1/5000",14,addr,port);
    kdm_debug("<<<<Store value\n\n");

   /* kdm_debug(">>>>Store value\n");
    kademHandleStoreValue(&machine,"token1","value1","127.0.0.1/5000",14,addr,port);
    kdm_debug("<<<<Store value\n\n");*/
    
    kdm_debug(">>>>Send error\n");
    kademSendError(&machine,"trans1",KADEM_ERROR_GENERIC,KADEM_ERROR_GENERIC_VALUE,addr,port);
    kdm_debug("<<<<Send error\n\n");
    
    return 0;
}
