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
    char test[] = "00000000000000000000000000000001";
    char * transactionId = "04";
    char * addr = "127.0.0.1";
    int port = 4000;
    char peer_addr[50];
    int i;
    strcpy(peer_addr,"caca/8000");

    _kdm_debug = 1;
    initMachine(&machine,6000,7000,"" );
 
    for(i=0;i<NUMBER_OF_BUCKETS;i++){
        if(machine.routes.table[i] != NULL){
            fprintf(stderr,"Not inited properly\n");
            exit(-1);
        } 
    }
    kdm_debug("====================Machine inited, id: %s=====================\n\n\n", machine.id);

    //###############################
    // Test of udpToMessage         #
    //###############################
    /*
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
    

    //###############################
    // End  Test of udpToMessage    #
    //###############################

    kdm_debug(">>>>TEST Ping\n");
    kademPing(&machine,"127.0.0.1",4000);
    kdm_debug("<<<<TEST Ping\n\n");
   
    kdm_debug(">>>>TEST Pong\n");
    kademPong(&machine,&message,addr,port);
    kdm_debug("<<<<TEST Pong\n\n");
    
    kdm_debug(">>>>TEST Find Node\n");
    kademFindNode(&machine,"test_node",addr,port);
    kdm_debug("<<<<TEST Find Node\n\n");
    
    kdm_debug(">>>>TEST Handle Find Node\n");
    print_routing_table(machine.routes);
    kademHandleFindNode(&machine,&message,addr,port);
    kdm_debug("<<<<TEST Handle Find Node\n\n");
    
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
    // Generate a message
    generateTransactionId(hash,machine.id);
    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));
    json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
    json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_FIND_NODE));

    argument = json_object_new_object();
    json_object_object_add(argument,"value",json_object_new_string("00000000000000000000000011000001"));
    json_object_object_add(header,"a",json_object_get(argument));

    strcpy(udpPacket,json_object_to_json_string(header));
    strcat(udpPacket,"bonjour");
    message = kademUdpToMessage(udpPacket,strlen(json_object_to_json_string(header))+7);
    /*printf("Header size: %i\n", strlen(json_object_to_json_string(message.header)));
    printf("There is a payload : %i\n", message.payloadLength >0);
    printf("Payload has the good size : %i\n", message.payloadLength == 7);
    printf("Payload is %s\n", message.payload);
    
    // Insert a node in the 127 bucket
    int k = insert_into_contact_table(&machine.routes, machine.id, "00000000000000000000000000000001", "127.0.0.2", 9999);
    kdm_debug("resultat de insert into contact table: %i\n", k);
    RPCHandlePing(&machine,&message,"prout",8888);
    
    //RPCHandlePing marche
    */
    /////////////////// kademPing ne marche pas
    
    //###############################
    // Test of Maintenance          #
    //###############################
    
    // Remplir la table de routage
     // Remplissage k-bucket 1-2
        int bucket_no;
        routing_table *table;
        node_details * bucket = NULL;
        time_t _timestamp;
        _timestamp = time (NULL);
        
        table = &machine.routes;
        strcpy(machine.id, "00000000000000000000000000000000");
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000001", "127.0.0.1", 1);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000002", "127.0.0.2", 2);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000003", "127.0.0.3", 3);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000004", "127.0.0.4", 4);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000005", "127.0.0.5", 5);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000006", "127.0.0.6", 6);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000007", "127.0.0.7", 7);
        printf("bucket_no: %i\n",bucket_no);

        // Remplissage k-bucket 3-5-6
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000008", "127.0.0.8", 8);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000009", "127.0.0.9", 9);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","0000000000000000000000000000000A", "127.0.0.10", 10);
        printf("bucket_no: %i\n",bucket_no);    
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","0000000000000000000000000000000D", "127.0.0.13", 13);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","0000000000000000000000000000000E", "127.0.0.14", 14);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","0000000000000000000000000000000F", "127.0.0.15", 15);
        printf("bucket_no: %i\n",bucket_no);
        
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000020", "127.0.0.32", 32);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","0000000000000000000000000000002A", "127.0.0.42", 42);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","0000000000000000000000000000002E", "127.0.0.46", 46);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000034", "127.0.0.52", 52);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000039", "127.0.0.57", 57);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","0000000000000000000000000000003F", "127.0.0.63", 63);
        printf("bucket_no: %i\n",bucket_no);

        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000040", "127.0.0.64", 64);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000050", "127.0.0.80", 80);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000060", "127.0.0.96", 96);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000069", "127.0.0.105", 105);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000070", "127.0.0.112", 112);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","0000000000000000000000000000007F", "127.0.0.127", 127);
        printf("bucket_no: %i\n",bucket_no);

        // Semi-remplissage de 4-7 bucket
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000010", "127.0.0.16", 16);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","0000000000000000000000000000001B", "127.0.0.27", 27);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000014", "127.0.0.20", 20);
        printf("bucket_no: %i\n",bucket_no);

        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000080", "127.0.0.128", 128);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000000000093", "127.0.0.147", 147);
        printf("bucket_no: %i\n",bucket_no);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","000000000000000000000000000000D2", "127.0.0.210", 210);
        printf("bucket_no: %i\n",bucket_no);
        
        // Remplissage de 127-bucket
        printf(">>>>>>>>>>>>>> 127-bucket\n\n");
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","90000000000000000000000000000000", "127.0.1.1", 1271);
        printf("bucket_no: %i\n",bucket_no);
        bucket = table->table[bucket_no];
        while (bucket->next != NULL)
        {
            bucket = bucket->next;
        }
        bucket->timestamp = _timestamp-303;
        printf("timestamp: %i\n", bucket->timestamp);
        bucket->count = 2;
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","B1000000000000000000000000000000", "127.0.1.2", 1272);
        printf("bucket_no: %i\n",bucket_no);
        bucket = table->table[bucket_no];
        while (bucket->next != NULL)
        {
            bucket = bucket->next;
        }
        bucket->timestamp = _timestamp-300;
        bucket->count = 2;
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","F1100000000000000000000000000000", "127.0.1.3", 1273);
        printf("bucket_no: %i\n",bucket_no);
        bucket = table->table[bucket_no];
        while (bucket->next != NULL)
        {
            bucket = bucket->next;
        }
        bucket->timestamp = _timestamp-300;
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","FFF00000000000000000000000000000", "127.0.1.4", 1274);
        printf("bucket_no: %i\n",bucket_no);
        bucket = table->table[bucket_no];
        while (bucket->next != NULL)
        {
            bucket = bucket->next;
        }
        bucket->timestamp = _timestamp-299;
        bucket->count = 2;
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","FFFFF000000000000000000000000000", "127.0.1.5", 1275);
        printf("bucket_no: %i\n",bucket_no);
        bucket = table->table[bucket_no];
        while (bucket->next != NULL)
        {
            bucket = bucket->next;
        }
        bucket->timestamp = _timestamp-298;
        bucket->count = 2;
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","FFFFFFFF000000000000000000000000", "127.0.1.6", 1276);

        print_nodes(table->table[127],127);
        
        // Remplissage de bucket 28
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000011100001", "127.0.0.254", 500);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000011110001", "127.0.0.255", 501);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000011111001", "127.0.0.255", 501);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000011111101", "127.0.0.255", 501);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000011111111", "127.0.0.255", 501);
        bucket_no = insert_into_contact_table(table, "00000000000000000000000000000000","00000000000000000000000011111112", "127.0.0.255", 501);
        
        //insert message into sent_queries
        char * head;
        store_file * query;
        head = json_object_to_json_string(header);
        query = create_store_file( transactionId, head, strlen(head)+1);
        machine.sent_queries = insert_to_tail_file(machine.sent_queries, query);
        
        // Create waiting_node liste
        store_file * tampon;
        query = create_store_file("90000000000000000000000000000000", "127.0.0.1/5000/10000000000000000000000000000001", strlen("127.9.0.0/5000/10000000000000000000000000000001"));
        print_values(query);
        machine.waiting_nodes = insert_to_tail_file(machine.waiting_nodes, query);
        tampon = machine.waiting_nodes;
        while (tampon->next != NULL)
        {
            tampon = tampon->next;
        }
        tampon->timestamp = _timestamp-3;
        
        query = create_store_file("B1000000000000000000000000000000", "node 2", 6);
        machine.waiting_nodes = insert_to_tail_file(machine.waiting_nodes, query);  
        tampon = machine.waiting_nodes;
        while (tampon->next != NULL)
        {
            tampon = tampon->next;
        }
        tampon->timestamp = _timestamp-2;
        
        query = create_store_file("30000000000000000000000000000003", "node 3", 6);
        machine.waiting_nodes = insert_to_tail_file(machine.waiting_nodes, query);  
        tampon = machine.waiting_nodes;
        while (tampon->next != NULL)
        {
            tampon = tampon->next;
        }
        tampon->timestamp = _timestamp-2;
        
        query = create_store_file("40000000000000000000000000000004", "node 4", 6);
        machine.waiting_nodes = insert_to_tail_file(machine.waiting_nodes, query);  
        tampon = machine.waiting_nodes;
        while (tampon->next != NULL)
        {
            tampon = tampon->next;
        }
        tampon->timestamp = _timestamp-1;
        query = create_store_file("50000000000000000000000000000005", "node 5", 6);
        machine.waiting_nodes = insert_to_tail_file(machine.waiting_nodes, query);
        
        kdm_debug("##############  waiting nodes: ##############\n");
        print_values(machine.waiting_nodes);
        
        //Create sent_queries list
        query = create_store_file("B1000000000000000000000000000000", "node 2", 6);
        machine.sent_queries = insert_to_tail_file(machine.sent_queries, query);  
        tampon = machine.sent_queries;
        while (tampon->next != NULL)
        {
            tampon = tampon->next;
        }
        tampon->timestamp = _timestamp-3;
        
        query = create_store_file("30000000000000000000000000000003", "node 3", 6);
        machine.sent_queries = insert_to_tail_file(machine.sent_queries, query);  
        tampon = machine.sent_queries;
        while (tampon->next != NULL)
        {
            tampon = tampon->next;
        }
        tampon->timestamp = _timestamp-3;
        
        query = create_store_file("40000000000000000000000000000004", "node 4", 6);
        machine.sent_queries = insert_to_tail_file(machine.sent_queries, query);  
        tampon = machine.sent_queries;
        while (tampon->next != NULL)
        {
            tampon = tampon->next;
        }
        tampon->timestamp = _timestamp-1;
        query = create_store_file("50000000000000000000000000000005", "node 5", 6);
        machine.sent_queries = insert_to_tail_file(machine.sent_queries, query);
        kdm_debug("##############  waiting nodes: ##############\n");
        printFiles(machine.sent_queries);
        
        // Creation of store_find_queries
        node_details *query1, *query2, *node, *tampon2;
        node = create_node_from_string("127.0.0.1/3100/00000000000000000000000000000001");
        query1 = insert_to_tail(query1, node);
        tampon2 = query1;
        while (tampon2->next != NULL)
        {
            tampon2 = tampon2->next;
        }
        tampon2->count = 1;
        tampon2->timestamp = _timestamp-5;
        node = create_node_from_string("127.0.0.2/3200/00000000000000000000000000000002");
        query1 = insert_to_tail(query1, node);
        tampon2 = query1;
        while (tampon2->next != NULL)
        {
            tampon2 = tampon2->next;
        }
        tampon2->count = 1;
        tampon2->timestamp = _timestamp-4;
        node = create_node_from_string("127.0.0.3/3300/00000000000000000000000000000003");
        query1 = insert_to_tail(query1, node);
        tampon2 = query1;
        while (tampon2->next != NULL)
        {
            tampon2 = tampon2->next;
        }
        tampon2->count = 0;
        tampon2->timestamp = _timestamp-5;
        node = create_node_from_string("127.0.0.4/3400/00000000000000000000000000000004");
        query1 = insert_to_tail(query1, node);
        tampon2 = query1;
        while (tampon2->next != NULL)
        {
            tampon2 = tampon2->next;
        }
        tampon2->count = 1;
        tampon2->timestamp = _timestamp-1;
        
        //Query2
        node = create_node_from_string("127.1.0.1/3100/00000000000000000000000000000001");
        query2 = insert_to_tail(query2, node);
        tampon2 = query2;
        while (tampon2->next != NULL)
        {
            tampon2 = tampon2->next;
        }
        tampon2->count = 1;
        tampon2->timestamp = _timestamp-5;
        node = create_node_from_string("127.1.0.2/3200/00000000000000000000000000000002");
        query2 = insert_to_tail(query2, node);
        tampon2 = query2;
        while (tampon2->next != NULL)
        {
            tampon2 = tampon2->next;
        }
        tampon2->count = 1;
        tampon2->timestamp = _timestamp-4;
        node = create_node_from_string("127.1.0.3/3300/00000000000000000000000000000003");
        query2 = insert_to_tail(query2, node);
        tampon2 = query2;
        while (tampon2->next != NULL)
        {
            tampon2 = tampon2->next;
        }
        tampon2->count = 0;
        tampon2->timestamp = _timestamp-5;
        node = create_node_from_string("127.1.0.4/3400/00000000000000000000000000000004");
        query2 = insert_to_tail(query2, node);
        tampon2 = query2;
        while (tampon2->next != NULL)
        {
            tampon2 = tampon2->next;
        }
        tampon2->count = 1;
        tampon2->timestamp = _timestamp-1;
        
        
        store_file * find_queries;
        find_queries = create_store_file("100", query1, sizeof(node_details) );
        machine.store_find_queries = find_queries;
        find_queries = create_store_file("200", query2, sizeof(node_details) );
        insert_to_tail_file(machine.store_find_queries, find_queries);
         
         tampon2 = query1;
        while (tampon2 != NULL)
        {
        printf("count:%i\n", tampon2->count);
        tampon2 = tampon2->next;
        }
         
         print_nodes(query1, 0);
         print_nodes(query2, 0);
         printFiles(machine.store_find_queries);
         
        
        // Lance maintenance
        kademMaintenance(&machine, NULL, addr, port);
        
        print_values(machine.sent_queries);
        //print_routing_table(machine.routes);
    
    
    
    
	return 0;
}
