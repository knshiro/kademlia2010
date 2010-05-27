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
    char udpPacket[400], hash[HASH_STRING_LENGTH];
    char test[] = "test5asf3as4f5asd4f";
    char * transactionId = "04";
    char * addr = "127.0.0.1";
    int port = 4000;

    _kdm_debug = 1;
    initMachine(&machine,6000,7000);
    kdm_debug("Machine inited, id: %s\n", machine.id);
 
    printf("%d\n",sizeof(test)); 
   
    /*
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
    machine.stored_values = insert_to_tail_file(machine.stored_values,create_store_file("@john","127.0.0.1/4000"));    
    kademHandleFindValue(&machine,&message,addr,port);

    kdm_debug("<<<<Handle Find value\n\n");
    
    kdm_debug(">>>>Store value\n");
    kademStoreValue(&machine,"token1","value1","127.0.0.1/5000",14,addr,port);
    kdm_debug("<<<<Store value\n\n");

   /* kdm_debug(">>>>Store value\n");
    kademHandleStoreValue(&machine,"token1","value1","127.0.0.1/5000",14,addr,port);
    kdm_debug("<<<<Store value\n\n");*/
   
   /* 
    kdm_debug(">>>>Send error\n");
    kademSendError(&machine,"trans1",KADEM_ERROR_GENERIC,KADEM_ERROR_GENERIC_VALUE,addr,port);
    kdm_debug("<<<<Send error\n\n");
  */  

	//###############################
    // Test of RPCHandlePing        #
    //###############################

	// Create buckets
	machine.stored_values = malloc(sizeof(stored_values));
	//Create the contact information table and 160 node_details *:
	int i=0;
	routing_table table;
	table = machine.routes;
	for(i;i<159;i++)
		{
			table.table[i]=NULL;
		}
	
	printf("\n***************** first series of test  *************\n");
	//TEST1: remplissage de k-buckets
	int bucket_no;
	// Remplissage k-bucket 1-2
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000001", "127.0.0.1", 1);
	printf("bucket_no: %i\n",bucket_no);
	print_nodes(table.table[bucket_no],1);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000002", "127.0.0.1", 2);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000003", "127.0.0.1", 3);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000004", "127.0.0.1", 4);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000005", "127.0.0.1", 5);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000006", "127.0.0.1", 6);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000007", "127.0.0.1", 7);
	printf("bucket_no: %i\n",bucket_no);

	// Remplissage k-bucket 3-5-6
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000008", "127.0.0.1", 8);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000009", "127.0.0.1", 9);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000000A", "127.0.0.1", 10);
	printf("bucket_no: %i\n",bucket_no);	
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000000D", "127.0.0.1", 13);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000000E", "127.0.0.1", 14);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000000F", "127.0.0.1", 15);
	printf("bucket_no: %i\n",bucket_no);
	
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000020", "127.0.0.1", 32);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000002A", "127.0.0.1", 42);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000002E", "127.0.0.1", 46);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000034", "127.0.0.1", 52);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000039", "127.0.0.1", 57);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000003F", "127.0.0.1", 63);
	printf("bucket_no: %i\n",bucket_no);

	bucket_no = insert_into_contact_table(&table, "00000000000","00000000040", "127.0.0.1", 64);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000050", "127.0.0.1", 80);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000060", "127.0.0.1", 96);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000069", "127.0.0.1", 105);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000070", "127.0.0.1", 112);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000007F", "127.0.0.1", 127);
	printf("bucket_no: %i\n",bucket_no);

	// Semi-remplissage de 4-7 bucket
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000010", "127.0.0.1", 16);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000001B", "127.0.0.1", 27);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000014", "127.0.0.1", 20);
	printf("bucket_no: %i\n",bucket_no);

	bucket_no = insert_into_contact_table(&table, "00000000000","00000000080", "127.0.0.1", 128);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000093", "127.0.0.1", 147);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","000000000D2", "127.0.0.1", 210);
	printf("bucket_no: %i\n",bucket_no);
	

	int u = print_routing_table(table);

    return 0;
}
