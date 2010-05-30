#include "kademlia.h"
#include "json-c-0.9/json.h"
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <ctype.h>
#include <unistd.h>
#include "node.h"
#include "XORmetrics.h"

#include <string.h>

//extern _kdm_debug;

int main(int argc, char *argv[]){
    struct kademMachine machine;
    json_object *header, *argument;
    struct kademMessage message;
    char udpPacket[400], hash[HASH_STRING_LENGTH+1];
    char test[] = "be328b78dbf5117a5c5d77efad4e81c9";
    char test2[] = "be328b78dbf5117a5c5d77efad4e81c4";
    char * transactionId = "04";
    char * addr = "127.0.0.1";
    int port = 4000;
    char peer_addr[50];
    int i;
    strcpy(peer_addr,"caca/8000");

    _kdm_debug = 1;
    initMachine(&machine,6000,7000,"");
 
    for(i=0;i<NUMBER_OF_BUCKETS;i++){
        if(machine.routes.table[i] != NULL){
            fprintf(stderr,"Not inited properly\n");
            exit(-1);
        } 
    }
    kdm_debug("====================Machine inited, id: %s=====================\n\n\n", machine.id);

    insert_into_contact_table(&machine.routes,machine.id,test,"127.0.0.1",12000);
    print_routing_table(machine.routes);

    //###############################
    // Test of udpToMessage         #
    //###############################
    
    kdm_debug("==================== BEGIN Test message creation=====================\n\n\n");
    generateTransactionId(hash,machine.id);
    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));
    json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
    json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_PING));

    argument = json_object_new_object();
    json_object_object_add(argument,"id",json_object_new_string(machine.id));
    json_object_object_add(argument,"value",json_object_new_string(test));
    json_object_object_add(argument,"token",json_object_new_string(hash));
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
    
    kdm_debug("==================== END Test message creation=====================\n\n\n");

    //###############################
    // End  Test of udpToMessage    #
    //###############################

    /*kdm_debug(">>>>TEST Ping\n");
    kademPing(&machine,"127.0.0.1",4000);
    kdm_debug("<<<<TEST Ping\n\n");
   
    kdm_debug(">>>>TEST Pong\n");
    kademPong(&machine,&message,addr,port);
    kdm_debug("<<<<TEST Pong\n\n");
    */
    kdm_debug(">>>>TEST Find Node\n");
    kademFindNode(&machine,"test_node",addr,port);
    kdm_debug("<<<<TEST Find Node\n\n\n");
    
    kdm_debug(">>>>>>>>>>>>>>>>>>>>>>> BEGIN TEST RPC Handle Find Node<<<<<<<<<<<<<<<<<<<<<<\n\n");
    kdm_debug(">>>>FIRST ROUND node found\n");
    RPCHandleFindNode(&machine,&message,addr,port);
    kdm_debug("<<<<FIRST ROUND node found\n\n");
    
    kdm_debug(">>>>SECOND ROUND node not found\n");

    json_object_object_add(argument,"value",json_object_new_string(test2));
    json_object_object_add(header,"a",json_object_get(argument));
    message.header = header;

    RPCHandleFindNode(&machine,&message,addr,port);
    kdm_debug("<<<<SECOUND ROUND node not found\n\n");
    
    kdm_debug(">>>>>>>>>>>>>>>>>>>>>>> END TEST RPC Handle Find Node<<<<<<<<<<<<<<<<<<<<<<\n\n\n");
   
    
    kdm_debug(">>>>>>>>>>>>>>>>>>>>>>> BEGIN TEST KADEM Handle Find Node<<<<<<<<<<<<<<<<<<<<<<\n\n");
    
    kdm_debug(">>>>FIRST ROUND\n");
    kademHandleFindNode(&machine,&message,addr,port);
    kdm_debug("<<<<FIRST ROUND\n\n");
  

    
    
    kdm_debug(">>>>>>>>>>>>>>>>>>>>>>> END TEST KADEM Handle Find Node<<<<<<<<<<<<<<<<<<<<<<\n\n");
    
    /* 
        kdm_debug(">>>>TEST Handle Find Node\n");
    print_routing_table(machine.routes);
    kademHandleAnswerFindNode(&machine,&message,addr,port);
    kdm_debug("<<<<TEST Handle Find Node\n\n");
  
 */


   /* 
    kdm_debug(">>>>TEST Find value\n");
    kademFindValue(&machine,"value1","token1", addr,port);
    kdm_debug("<<<<TEST Find value\n\n");
   
    kdm_debug(">>>>TEST Handle Find value\n");
   
    printf("First test: value not found\n");
    printf("%s\n",json_object_to_json_string(message.header));
    json_object_object_add(argument,"value",json_object_new_string("@john"));
    json_object_object_add(argument,"token",json_object_new_string("123456789"));
    json_object_object_add(header,"a",json_object_get(argument));
    printf("%s\n",json_object_to_json_string(message.header));
    message.header = header;
    printf("%s\n",json_object_to_json_string(message.header));
    
    kademHandleFindValue(&machine,&message,addr,port);
   

    printf("\nSecond test: value found\n");
    machine.stored_values = insert_to_tail_file(machine.stored_values,create_store_file("@john","127.0.0.1/4000",15));    
    kademHandleFindValue(&machine,&message,addr,port);

    kdm_debug("<<<<TEST Handle Find value\n\n");
    
    kdm_debug(">>>>TEST Store value\n");
    kademStoreValue(&machine,"token1","value1","127.0.0.1/5000",14,addr,port);
    kdm_debug("<<<<TEST Store value\n\n");

    kdm_debug(">>>>TEST Handle Store value\n");
    kademHandleStoreValue(&machine,&message,addr,port);
    kdm_debug("<<<<TEST Handle Store value\n\n");
   
    
    kdm_debug(">>>>Send error\n");
    kademSendError(&machine,"trans1",KADEM_ERROR_GENERIC,KADEM_ERROR_GENERIC_VALUE,addr,port);
    kdm_debug("<<<<Send error\n\n");
  
*/
	//###############################
    // Test of RPCHandlePing        #
    //###############################

	    return 0;
}
