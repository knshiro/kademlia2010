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
    char udpPacket[400];
    char * transactionId = "01";

    _kdm_debug = 1;
    initMachine(&machine,6000,7000);
  
    kdm_debug("Machine inited, id: %s\n", machine.id);
    kademPing(&machine,"127.0.0.1",4000);
   

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


    //###############################
    // End  Test of udpToMessage    #
    //###############################

    return 0;
}
