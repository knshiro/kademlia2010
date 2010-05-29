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
#include "store_file.h"
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
    char id[HASH_STRING_LENGTH+1]; 

	// Init latest_query_rpc
	strcpy(machine->latest_query_rpc.query, "");
	strcpy(machine->latest_query_rpc.value, "");
	strcpy(machine->latest_query_rpc.ip, "");
	machine->latest_query_rpc.port = 0;


    machine->stored_values = NULL;

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
    md5_sig_to_string(signature, id, HASH_STRING_LENGTH+1);
    kdm_debug("signature: %s\n", id);

    strcpy(machine->id,id);

    return 0;
}


int kademMaintenance(struct kademMachine * machine){
    
    //TODO refresh the k-buckets
    time_t _timestamp;
    int i;
    _timestamp = time (NULL);
    for (i=0; i<160; i++)
    {
        node_details * bucket;
        bucket = machine->routes.table[i];
        while ((bucket != NULL) && (_timestamp - bucket->timestamp > KADEM_TIMEOUT_REFRESH_DATA))
        {
            if (bucket->timeout == 2)
            {
                delete_node(machine->routes.table[i], bucket->nodeID);
            } 
            else 
            {
                kademPing(machine, bucket->ip, bucket->port);
                bucket->timeout = bucket->timeout + 1; 
                bucket->timestamp = bucket->timestamp-2;
            }
            bucket = bucket->next;
        }
    }
    //TODO dans la boucle principale: réinitialiser le timeout à chaque réception de réponse de ping et mettre à jour le timestamp à chaque utilisation du node(réponse à une query ou réception de query)
    
    //TODO: refresh the queries
    store_file * refreshed_queries;
    refreshed_queries = machine->sent_queries;
    while (refreshed_queries != NULL)
    {
    //if timestamp < qqch => supprimer la query
    }
    
    //TODO refresh the files stored
    machine->stored_values = clean(machine->stored_values,KADEM_TIMEOUT_REFRESH_DATA);
        
    return 0;
}

/*
struct kademMessage* findMessageByTransactionId(struct kademMachine * machine, char * transactionId){
    struct kademMessage* result = NULL;
    const char * buffer;
    int found =0,i=0;
    while(!found && i<KADEM_MAX_PAYLOAD_SIZE){
       buffer = json_object_get_string(json_object_object_get(machine->messageBuffer[i].header,"t")); 
        if(strcmp(buffer,transactionId)==0){
            result = &machine->messageBuffer[i];
            found = 1;
        }
        else{
            i++;
        }
    }    
    return result;
}
*/

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
    char string_time[32],buffer[HASH_STRING_LENGTH+1], buffer2[2*HASH_STRING_LENGTH+1],signature[HASH_SIGNATURE_LENGTH];
   
    //Retrieve time
    now = time(NULL);
    sprintf(string_time,"%ld",now);
    len = strlen(string_time); 
    kdm_debug("Time : %s (%d chars)\n", string_time, len);

    //Hash time
    md5_buffer(string_time,len,signature);
    md5_sig_to_string(signature, buffer, HASH_STRING_LENGTH+1);
    kdm_debug("Time hashed: %s\n", buffer);
  
    //Hash time + id
    strcpy(buffer2,buffer);
    strcat(buffer2,id);
    len = strlen(buffer2);
    md5_buffer(buffer2,len,signature);
    md5_sig_to_string(signature, transactionId, HASH_STRING_LENGTH+1);
    
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
    char transactionId[HASH_SIGNATURE_LENGTH+1]; 
	store_file* query;
    char * head;

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

	// Store Query in sent_queries
	head = json_object_to_json_string(message.header);
	query = create_store_file( transactionId, head, strlen(head));
	insert_to_tail_file(machine->sent_queries, query);
	
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

int kademHandlePong(struct kademMachine *machine, struct kademMessage *message, char* ip, int port){
    
    kdm_debug(">>>> Handle pong\n");
    
    const char *transactionId, *id, *rpc_query_type;
    json_object *header = message->header, *response, *rpc_header;
    struct kademMessage rpc_message;

    transactionId = json_object_get_string(json_object_object_get(header,"t"));
    id = json_object_get_string(json_object_object_get(response,"id"));
    insert_into_contact_table(&machine->routes,machine->id,id,ip,port); 
    delete_key(machine->sent_queries, transactionId); 

    response = json_object_object_get(header,"r");
    if(strcmp(machine->latest_query_rpc.query,KADEM_PING)){
        if(strcmp(machine->latest_query_rpc.value,id)==0){
            strcpy(machine->latest_query_rpc.query,"");
            rpc_header = json_object_new_object();
            json_object_object_add(rpc_header,"resp",json_object_new_string("OK"));
            rpc_message.header = rpc_header;
            kademSendMessage(machine->sock_local_rpc, &rpc_message, machine->latest_query_rpc.ip, machine->latest_query_rpc.port);
            json_object_put(rpc_header);
        }
    }

    kdm_debug("<<<< Handle pong\n");
    return 0; 
}

int kademFindNode(struct kademMachine * machine, char * target_id, char * addr, int port){

    struct kademMessage message;
    int ret;
    json_object *header, *argument;
    char transactionId[HASH_SIGNATURE_LENGTH+1]; 
	store_file* query;
    char * head;

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

	// Store Query in sent_queries
	head = json_object_to_json_string(message.header);
	query = create_store_file( transactionId, head, strlen(head));
	insert_to_tail_file(machine->sent_queries, query);	
	
    return ret;

}

int kademHandleFindNode(struct kademMachine * machine, struct kademMessage * message, char * addr, int port){

    struct kademMessage answer_message;
    int ret;
    json_object *header, *response, *node_array, *query;
    const char * transactionId, *key;
    node_details* nodes, *current_node;
    char * node_string;

    transactionId = json_object_get_string(json_object_object_get(message->header,"t"));

    query = json_object_object_get(message->header,"a");
    key = json_object_get_string(json_object_object_get(query,"value"));
    
    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));
    json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

    response = json_object_new_object();
    json_object_object_add(response,"id",json_object_new_string(machine->id));

    node_array = json_object_new_array();
    //find nodes
    nodes = k_nearest_nodes(nodes,&machine->routes,machine->id,key); 
    current_node = nodes;
    while(current_node != NULL){
        strcpy(node_string,nodes->ip);
        strcat(node_string,"/");
        sprintf(node_string+strlen(nodes->ip),"%d",nodes->port);
        strcat(node_string,"/");
        strcat(node_string,nodes->nodeID);
        json_object_array_add(node_array,json_object_new_string(node_string));
        current_node = current_node->next;
    }
    json_object_object_add(response,"nodes",json_object_get(node_array));

    json_object_object_add(header, KADEM_ANSWER,json_object_get(response));

    answer_message.header = header;
    answer_message.payloadLength = 0;

    ret = kademSendMessage(machine->sock_p2p, &answer_message, addr, port);
    json_object_put(header);
    json_object_put(response);
    json_object_put(node_array);

    return ret;

}

int kademHandleAnswerFindNode(struct kademMachine * machine, struct kademMessage * message){

 

    return 0; 
}

int kademFindValue(struct kademMachine * machine, char * value, char* token, char *addr, int port){

    struct kademMessage message;
    int ret;
    json_object *header, *argument;
    char transactionId[HASH_SIGNATURE_LENGTH+1]; 
	store_file* query;
    const char * head;

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

	// Store Query in sent_queries
	head = json_object_to_json_string(message.header);
	query = create_store_file( transactionId, head, strlen(head));
	insert_to_tail_file(machine->sent_queries, query);

    return ret;

}

int kademHandleFindValue(struct kademMachine * machine, struct kademMessage * message,char *addr, int port){

    struct kademMessage answer_message;
    int ret;
    json_object *header, *response, *node_array, *query;
    const char * transactionId, *key, *token;
    store_file * value; 
    char node_string[HASH_STRING_LENGTH+15+6+3+1];
    node_details* nodes, *current_node;
    
    transactionId = json_object_get_string(json_object_object_get(message->header,"t"));
    query = json_object_object_get(message->header,"a");
    key = json_object_get_string(json_object_object_get(query,"value"));
    token = json_object_get_string(json_object_object_get(query,"token"));

    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));
    json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

    response = json_object_new_object();
    json_object_object_add(response,"id",json_object_new_string(machine->id));
    json_object_object_add(response,"token",json_object_new_string(token));

    answer_message.payloadLength = 0;

    node_array = json_object_new_array();
    kdm_debug("Looking for value : %s\n",key);
    
    //look for value
    if((value = find_key(machine->stored_values,key))!=NULL){
        kdm_debug("Value found !\n");
        json_object_object_add(response,"value",json_object_new_string(key));
        json_object_object_add(response,"numbytes",json_object_new_int(value->value_len));
        answer_message.payloadLength = value->value_len;
        memcpy(answer_message.payload,value->value,answer_message.payloadLength);
    }

    else {
        kdm_debug("Value not found\n");
        nodes = k_nearest_nodes(nodes,&machine->routes,machine->id,key); 
        current_node = nodes;
        while(current_node != NULL){
            strcpy(node_string,nodes->ip);
            strcat(node_string,"/");
            sprintf(node_string+strlen(nodes->ip),"%d",nodes->port);
            strcat(node_string,"/");
            strcat(node_string,nodes->nodeID);
            json_object_array_add(node_array,json_object_new_string(node_string));
            current_node = current_node->next;
        }
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

	// Search in machine's store_find_queries the corresponding list of nodes (by hash)
	  //Find the sent request corresponding to the answer in machine's sent_queries (by transactionID)
	const char *temp, *temp2, *token;
	store_file *result, *result2, *find_queries;
	store_file* _store_file;
	json_object *argument2;
	node_details *found_nodes;
	json_object * sent_query_msg;

	temp = json_object_get_string(json_object_object_get(message->header,"t"));
	result = find_key(machine->sent_queries, temp); //result->value is a string (header of sent query)
	
	  //Find the query with the hash (value of sent query)
	sent_query_msg = json_tokener_parse(result->value);
	argument2 = json_object_object_get(sent_query_msg,"a");
	temp2 = json_object_get_string(json_object_object_get(argument2,"value"));
	token = json_object_get_string(json_object_object_get(argument2,"token"));
    result2 = find_key(machine->store_find_queries, temp2); //temp2 is the hash of the searched value
	
	if (message->payloadLength > 0) // If value found:
	{
		// store value
		_store_file = create_store_file( temp2, message->payload, message->payloadLength);
		insert_to_tail_file(machine->stored_values, _store_file);
		
		//Look if the query is the latest query from the RPC
		if (strcpy(machine->latest_query_rpc.query, "") == 0)
		{
			if((strcpy(machine->latest_query_rpc.query, "get") == 0 ) && (strcpy(machine->latest_query_rpc.value, temp2) == 0))
			{
				// Answer to the get of the RPC
			}
			if((strcpy(machine->latest_query_rpc.query, "put") == 0 ) && (strcpy(machine->latest_query_rpc.value, temp2) == 0))
			{
				// Answer to the put of the RPC
			}
		}
	}

	else // If no value found: new set of nodes received
	{
	  // compare nodes with answered nodes: Knodes1 = Knodes2 - Knodes1
		found_nodes = (node_details*)result2->value;
		

// Search the nearest nodes and store it in store_find_queries machine's field
		node_details* nearest_nodes;
		store_file* store_file_temp;

		nearest_nodes = k_nearest_nodes(nearest_nodes, &machine->routes, machine->id, temp);
		store_file_temp = create_store_file(temp, nearest_nodes, sizeof(nearest_nodes));
		insert_to_tail_file(machine->store_find_queries, store_file_temp);

		// Send find values request to nearest nodes and increment count
		machine->store_find_queries->count = 0;
		while (nearest_nodes != NULL)
		{
			kademFindValue(machine, temp,token, nearest_nodes->ip, nearest_nodes->port);
			machine->store_find_queries->count = machine->store_find_queries->count + 1;
			nearest_nodes = nearest_nodes -> next;
		}
	}
	
	  // if Knodes1 != NULL : send get to nodes and count = count + 1

	  // count = count - 1

	  // if count = 0 => stop the query and send "not found"
			// if put in RPC

    return 0; 
}

int kademStoreValue(struct kademMachine * machine, char * token, char * value, char * data, int data_len, char *dst_addr, int dst_port){

    struct kademMessage message;
    int ret;
    json_object *header, *argument;
    char transactionId[HASH_SIGNATURE_LENGTH+1]; 
	store_file* query;
    char * head;

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

	// Store Query in sent_queries
	head = json_object_to_json_string(message.header);
	query = create_store_file( transactionId, head, strlen(head));
	insert_to_tail_file(machine->sent_queries, query);

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
        //fix payload + create_store_file
        key = json_object_get_string(json_object_object_get(query_argument,"value"));
        value = message->payload;
        int length = strlen(value);
        machine->stored_values = insert_to_tail_file(machine->stored_values, create_store_file(key,value,length));

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
        //fix ret value
        ret = kademSendError(machine, transactionId,KADEM_ERROR_STORE,KADEM_ERROR_STORE_VALUE,addr,port); 
    }
    return ret;

}


int kademHandleAnswerStoreValue(struct kademMachine * machine, struct kademMessage * message){
    
    return 0;
}


int kademSendStoreValue(struct kademMachine * machine, node_details* node_to_send, char* value, char* token){
	
	int i=0;
	//look for the token into token_sent.
	store_file* one_token_sent;
	one_token_sent = find_key(machine->token_sent, token);

	//look how many nodes we send the value to.
	int number_sent;
	number_sent = count_nodes_details(node_to_send); 

	node_details* temp=NULL;

	for(i;i<number_sent;i++){
		kademStoreValue(machine, token, value, one_token_sent->value, strlen(one_token_sent->value), temp->ip, temp->port);
		temp = temp->next;
		if(temp = NULL){
			break;
		}
	}

	//delete the one_token_sent in the token_store list.
	delete_key(machine->token_sent, token);

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
            
	    		// Retrieve ip and port from sending node
                from_port = ntohs(from.sin_port);
                from_addr = inet_ntoa(from.sin_addr);                
                kdm_debug("Receive message from: %s (%d)\n",from_addr,from_port);

                // Determine the type of the message 
                buffer = json_object_get_string(json_object_object_get(message.header,"y"));
                // Message is a query
                if (strcmp(buffer,KADEM_QUERY) == 0){

                    // Determine the type of the query
                    buffer = json_object_get_string(json_object_object_get(message.header,KADEM_QUERY));
	    			if (strcmp(buffer,"print_routing_table") == 0){
                        RPCHandlePrintRoutingTable(machine, &message, from_addr, from_port);  
                    	}  
                    if (strcmp(buffer,"print_object_ids") == 0){
	    				RPCHandlePrintObjects(machine, &message, from_addr, from_port); 
                    	}                
                    if (strcmp(buffer,"ping") == 0){
                        RPCHandlePing(machine, &message, from_addr, from_port);  
                    	}                
                    if (strcmp(buffer,"kill_node") == 0){
                        RPCHandleKillNode(machine, &message, from_addr, from_port);  
                    	}                
                    if (strcmp(buffer,"put") == 0){
                        RPCHandleStoreValue(machine, &message, from_addr, from_port);  
                   	 	}
	   				if (strcmp(buffer,"get") == 0){
                        RPCHandleFindValue(machine, &message, from_addr, from_port);  
                    	}
	   				if (strcmp(buffer,"find_node") == 0){
                        RPCHandleFindNode(machine, &message, from_addr, from_port);  
                    	}
                    else{
                        const char *transID;
                        transID = json_object_get_string(json_object_object_get(message.header,"t"));
                        kademSendError(machine, transID, KADEM_ERROR_METHOD_UNKNOWN, KADEM_ERROR_METHOD_UNKNOWN_VALUE, from_addr, from_port);
                         }					       
                     }

                 // Message type is unknown
                 else{
                     const char *transID; 
                     transID = json_object_get_string(json_object_object_get(message.header,"t"));
                     kademSendError(machine, transID, KADEM_ERROR_PROTOCOL, KADEM_ERROR_PROTOCOL_VALUE, from_addr, from_port);
                     }
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
                    else{
                        const char *transID; 
                        transID = json_object_get_string(json_object_object_get(message.header,"t"));
                        kademSendError(machine, transID, KADEM_ERROR_METHOD_UNKNOWN, KADEM_ERROR_METHOD_UNKNOWN_VALUE, from_addr, from_port);
                      }   
                  }
                  

                    // Message is an answer
                else if (strcmp(buffer,KADEM_ANSWER) == 0){
                    //Find the sent request corresponding to the answer in machine's sent_queries (by transactionID)
                    const char *temp, *query_type;
                	store_file *result;
                	json_object * sent_query_msg;
                    const char *transID; 
                           
                    transID = json_object_get_string(json_object_object_get(message.header,"t"));
    	            temp = json_object_get_string(json_object_object_get(message.header,"t"));//get transactionID of received msg
  	                result = find_key(machine->sent_queries, temp); //result->value is a string (header of the corresponding sent query
	                if (result != NULL)
	                {
	                // Extract the type of query from the string
	                sent_query_msg = json_tokener_parse(result->value);
	                query_type = json_object_get_string(json_object_object_get(sent_query_msg,"q"));
	                
	                   if (strcmp(query_type,KADEM_PING) == 0)
	                   {
                           kademHandlePong(machine, &message, from_addr, from_port);  
                       }                
                       if (strcmp(query_type,KADEM_STORE) == 0)
                       {
                           kademHandleAnswerStoreValue(machine, &message);  
                       }                
                       if (strcmp(query_type,KADEM_FIND_NODE) == 0)
                       {
                           kademHandleAnswerFindNode(machine, &message);  
                       }                
                       if (strcmp(query_type,KADEM_FIND_VALUE) == 0)
                       {
                           kademHandleAnswerFindValue(machine, &message);  
                       } 
                       else
                       {
                           kademSendError(machine, transID, KADEM_ERROR_METHOD_UNKNOWN, KADEM_ERROR_METHOD_UNKNOWN_VALUE, from_addr, from_port);
                       }   
                    delete_key(machine->sent_queries, transID); //Delete the sent query from sent_queries    
                    }               
                }
    
    
                    // Message is an error
                else if (strcmp(buffer,KADEM_ERROR) == 0){
                    json_object * error_msg;
                    char *_code, *_value;
                    const char *transID; 
                        
                    transID = json_object_get_string(json_object_object_get(message.header,"t"));
                    error_msg = json_object_object_get(message.header,KADEM_QUERY);
                    _code = json_object_get_string(json_object_object_get(error_msg,"code"));
                    _value = json_object_get_string(json_object_object_get(error_msg,"value"));
                    
                    if (strcmp(_code,KADEM_ERROR_GENERIC) == 0){
                        fprintf(stderr, "%s from address: %s and port: %i", _value, from_addr, from_port);  
                    }
                    if (strcmp(_code,KADEM_ERROR_INVALID_TOKEN) == 0){
                        fprintf(stderr, "%s from address: %s and port: %i", _value, from_addr, from_port);  
                    }
                    if (strcmp(_code,KADEM_ERROR_PROTOCOL) == 0){
                        fprintf(stderr, "%s from address: %s and port: %i", _value, from_addr, from_port);  
                    }
                    if (strcmp(_code,KADEM_ERROR_METHOD_UNKNOWN) == 0){
                        fprintf(stderr, "%s from address: %s and port: %i", _value, from_addr, from_port);  
                    }
                    if (strcmp(_code,KADEM_ERROR_STORE) == 0){
                        fprintf(stderr, "%s from address: %s and port: %i", _value, from_addr, from_port);  
                    }   
                    else
                    {
                    kademSendError(machine, transID, KADEM_ERROR_METHOD_UNKNOWN, KADEM_ERROR_METHOD_UNKNOWN_VALUE, from_addr, from_port);
                    }
                    delete_key(machine->sent_queries, transID); //Delete the sent query from sent_queries                                     
                  }
                  
                      // Message type is unknown
                else
                {
                const char *transID; 
                transID = json_object_get_string(json_object_object_get(message.header,"t"));
                kademSendError(machine, transID, KADEM_ERROR_PROTOCOL, KADEM_ERROR_PROTOCOL_VALUE, from_addr, from_port);
                delete_key(machine->sent_queries, transID); //Delete the sent query from sent_queries
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

	//TODO: store the value (into the store_file?).
	//look for value into the header of message.
	int length;
   	const char * temp;
	json_object *argument2, *argument3;
	store_file* new_value;
	char *data, *temp1, *temp2; 
	struct kademMessage answer_message;
	json_object *header, *argument;
	char ok[] = "OK";
   
    	data = message->payload;
    	length = strlen(data);
    	argument2 = json_object_object_get(message->header,"a");
	//TEMP: value to store
	temp = json_object_get_string(json_object_object_get(argument2,"value"));
	new_value = create_store_file(temp, data, length);
	insert_to_tail_file(machine->stored_values, new_value); //Value stored.

    	//Answer to the RPC
   	header = json_object_new_object(); 

	json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

   	argument = json_object_new_object();
   	json_object_object_add(argument,"resp",json_object_new_string(ok));
   	json_object_object_add(header,KADEM_ANSWER,json_object_get(argument));

	answer_message.header = header;
	answer_message.payloadLength = 0;


	if(kademSendMessage(machine->sock_local_rpc, &answer_message, addr, port)<0){
		return -1;
	}   //Answered to the RPC.
    
	json_object_put(header);
    	json_object_put(argument);

	//Store the query in the machine last_query field. Store the data? Useless?
	argument3 = json_object_object_get(message->header,"a");
	temp1 = json_object_get_string(json_object_object_get(argument3,"value"));
	temp2 = json_object_get_string(json_object_object_get(message->header,"q"));
	strcpy(machine->latest_query_rpc.query, temp2);
	strcpy(machine->latest_query_rpc.value, temp1);		
	strcpy(machine->latest_query_rpc.ip, addr);   //useless because we already answered.
	machine->latest_query_rpc.port = port;

	//find the nearest nodes.
	node_details* nearest_nodes;
	store_file* store_file_temp;
	//store the query GET into "store_find_queries".
	nearest_nodes = k_nearest_nodes(nearest_nodes, &machine->routes, machine->id, temp);
	store_file_temp = create_store_file(temp, nearest_nodes, sizeof(nearest_nodes));
	insert_to_tail_file(machine->store_find_queries, store_file_temp);

	//Create a token for this query.
	char token[HASH_SIGNATURE_LENGTH+1];    
	generateTransactionId(token, machine->id);
	store_file* store_token;
	store_token = create_store_file(store_token, message->payload, 0);
	insert_to_tail_file(machine->token_sent, store_token);

	// Send find values request to nearest nodes and increment count
	store_token->count = 0;
	while (nearest_nodes != NULL){
		kademFindValue(machine, temp, token, nearest_nodes->ip, nearest_nodes->port);
		store_file_temp->count++;
	}

	return 0;
}

int RPCHandlePing(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){

	// Extract value from KademMessage
	char* hash_value;	
	hash_value = json_object_get_string(json_object_object_get(json_object_object_get(message,"a"),"value"));

	// Use hash value and MachineID to find the right node_details using find_node_details and look_for_id functions
	int bucket_val;
	node_details* bucket;
	node_details* Kbucket;
	char* _ip;
	int _port;

	bucket_val = find_node_details(machine->id, hash_value)	;
	Kbucket = machine->routes.table[bucket_val];
	bucket = look_for_IP(Kbucket, hash_value);		

	// Extract IP/Port from node_details
	_ip = bucket->ip;
	_port = bucket->port;

	// Send Kadem_ping to the right node
	kademPing(machine, _ip, _port);	

	return 0;
}

int RPCHandlePingRspn(struct kademMachine * machine, int answ, char *addr, int port){

	// Wait for answer from the node and reply to the application OK or NOK

	struct kademMessage answer_message;
	json_object *header, *argument;	
	char* rspn;
	header = json_object_new_object(); 

	if (answ == 0)
	{	
		rspn = "OK";
	}
	else
	{	
		rspn = "NOK";
	}
		
	json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

   	argument = json_object_new_object();
   	json_object_object_add(argument,"resp",json_object_new_string(rspn));
    json_object_object_add(header,KADEM_ANSWER,json_object_get(argument));

	answer_message.header = header;
	answer_message.payloadLength = 0;


	if(kademSendMessage(machine->sock_local_rpc, &answer_message, addr, port)<0)
	{
		return -1;
	}
	
	return 0;
}


int RPCHandlePrintRoutingTable(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){

	//TOTEST: print routing table.
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

	//TOTEST: print objects	
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


int RPCHandleFindValue(struct kademMachine * machine, struct kademMessage * message, char * addr, int port)
{
	
	//TOTEST: look for the appropriate IP address and port into the store_files.
	char* value;
	const char* temp, *temp1, *temp2, *header2;
    char ok[] = "OK", leng_payload[5], token[HASH_STRING_LENGTH+1];
    int leng;
    struct kademMessage answer_message;
    json_object *header, *argument, *argument2, *argument3;
	store_file * result;
	

    generateTransactionId(token,machine->id);
	//look for value into the header of message.
	argument2 = json_object_object_get(message->header,"a");
	temp = json_object_get_string(json_object_object_get(argument2,"value"));
	
	result = find_key(machine->stored_values, temp);
	
	if (result == NULL) // Send request to the k-nodes the nearest from the key
	{
		// Store the query in the machine last_query field
		argument3 = json_object_object_get(message->header,"a");
		temp1 = json_object_get_string(json_object_object_get(argument3,"value"));
		temp2 = json_object_get_string(json_object_object_get(message->header,"q"));

		strcpy(machine->latest_query_rpc.query, temp2);
		strcpy(machine->latest_query_rpc.value, temp1);		
		strcpy(machine->latest_query_rpc.ip, addr);
		machine->latest_query_rpc.port = port;

		// Search the nearest nodes and store it in store_find_queries machine's field
		node_details* nearest_nodes;
		store_file* store_file_temp;

		nearest_nodes = k_nearest_nodes(nearest_nodes, &machine->routes, machine->id, temp);
		store_file_temp = create_store_file(temp, nearest_nodes, sizeof(nearest_nodes));
		insert_to_tail_file(machine->store_find_queries, store_file_temp);

		// Send find values request to nearest nodes and increment count
		store_file_temp->count = 0;
		while (nearest_nodes != NULL)
		{
			kademFindValue(machine, temp, token, nearest_nodes->ip, nearest_nodes->port);
			store_file_temp->count++;
			nearest_nodes = nearest_nodes -> next;
		}

		return -2;
	}

	else
	{
		value = result->value;
	
		//create the payload: "ip_address/port":
		leng = strlen(value);
		sprintf(leng_payload,"%d",leng); 
	
        header = json_object_new_object(); 
	
		json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));
	
   		argument = json_object_new_object();
   		json_object_object_add(argument,"resp",json_object_new_string(ok));
		json_object_object_add(argument,"numbytes",json_object_new_string(leng_payload));
    	json_object_object_add(header,"r",json_object_get(argument));
	
		answer_message.header = header;
		strcpy(answer_message.payload,value);
		kdm_debug("message.payload: %s\n",answer_message.payload);
		answer_message.payloadLength = leng;
	
		header2 = json_object_to_json_string(answer_message.header);
    		
   		kdm_debug("message: %s\n",header2);

		if(kademSendMessage(machine->sock_local_rpc, &answer_message, addr, port)<0)
		{
			return -1;
		}
        json_object_put(header);
        json_object_put(argument);
		return 0;
	}
}

//TODO
// Handle find value answer for RPC and DHT
int HandleFindValue(struct kademMachine * machine, struct kademMessage * message, char addr[16], int port)
{
}	


int RPCHandleKillNode(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){
		
	//TOTEST
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
	
	exit(0);
}


int RPCHandleFindNode(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){

       	//look for nodeID into the header of message
        char * temp=(char*)malloc(17);
        json_object *argument2;
        argument2 = json_object_object_get(message->header,"a");
        temp = json_object_get_string(json_object_object_get(argument2,"value"));

        //Look where the node might be in store_file.
        int bucket_no;
        bucket_no = find_node_details(machine->id, temp);
        node_details* find;
        find = look_for_IP(machine->routes.table[bucket_no], temp);

        char* ok = "OK";

        struct kademMessage answer_message;
        char* ip_port=(char*)malloc(30*sizeof(char));
        json_object *header, *argument;

        if (find != NULL){
                char* ip_port = (char*)malloc((15+6+1+1)*sizeof(char));
                ip_port = concatenate2(find);

                header = json_object_new_object(); 

                json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

                argument = json_object_new_object();
                json_object_object_add(argument,"resp",json_object_new_string(ok));
                json_object_object_add(argument,"nodes",json_object_new_string(ip_port));
                json_object_object_add(header,"r",json_object_get(argument));

                answer_message.header = header;
                answer_message.payloadLength = 0;

                if(kademSendMessage(machine->sock_local_rpc, &answer_message, addr, port)<0){
                        return -1;
                }
                return 0;
        }

        if (find == NULL){
		
		// Store the query in the machine last_query field
		json_object *argument3;
		char* temp1;
		char* temp2;
		argument3 = json_object_object_get(message->header,"a");
		temp1 = json_object_get_string(json_object_object_get(argument3,"value"));
		temp2 = json_object_get_string(json_object_object_get(message->header,"q"));

		strcpy(machine->latest_query_rpc.query, temp2);
		strcpy(machine->latest_query_rpc.value, temp1);		
		strcpy(machine->latest_query_rpc.ip, addr);
		machine->latest_query_rpc.port = port;
                

		// Search the nearest nodes and store it in store_find_queries machine's field
		node_details* nearest_nodes;
		store_file* store_file_temp;

		nearest_nodes = k_nearest_nodes(nearest_nodes, &machine->routes, machine->id, temp);
		store_file_temp = create_store_file(temp, nearest_nodes, sizeof(nearest_nodes));
		insert_to_tail_file(machine->store_find_queries, store_file_temp);

		// Send find values request to nearest nodes and increment count
		store_file_temp->count = 0;
		while (nearest_nodes != NULL)
		{
			kademFindNode(machine, temp, nearest_nodes->ip, nearest_nodes->port);
			store_file_temp->count++;
			nearest_nodes = nearest_nodes -> next;
		}

		return -2;
		
        }

        return 0; 
}

