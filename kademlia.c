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

void kdm_trace(const char *msg, ...){
	va_list ap;
	va_start(ap, msg);
	if(_kdm_trace) {
		vprintf(msg,ap);
	}
	va_end(ap);
}

//OK
int initMachine(struct kademMachine * machine, int port_local_rpc, int port_p2p, char *peer_addr){

	kdm_trace("port_P2P: %i\n",port_p2p);
	kdm_trace("<<<< initMachine\n");
	int sockfd;
	struct sockaddr_in local_rpc_addr, p2p_addr;
	int length = sizeof(struct sockaddr_in);

	int i, peer_port;
	char *pointer;
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
	machine->sent_queries = NULL;
	machine->waiting_nodes = NULL;
	machine->token_sent = NULL;
	machine->token_rec = NULL;
	machine->store_find_queries = NULL;

	for(i=0;i<NUMBER_OF_BUCKETS;i++){
		machine->routes.table[i] = NULL;
	}

	srand (time (NULL));

	//####################################
	// Create first socket for local RPC #
	//####################################

	// Create socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0){ 
		perror("ERROR opening socket");
		return -1;
	}
	kdm_trace("Local RPC socket created\n");

	machine->sock_local_rpc = sockfd;
	kdm_trace("Socket %d stored in kadmelia machine\n",sockfd);

	// Specify type 
	local_rpc_addr.sin_family = AF_INET;

	// Give any address and listen to the specified port
	local_rpc_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	local_rpc_addr.sin_port = htons(port_local_rpc);

	if(bind(machine->sock_local_rpc,(struct sockaddr *)&local_rpc_addr,length) == -1) {
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
	kdm_trace("P2P socket created\n");

	machine->sock_p2p = sockfd;
	kdm_trace("Socket %d stored in kadmelia machine\n",sockfd);

	// Specify type 
	p2p_addr.sin_family = AF_INET;
	kdm_trace("port_P2P: %i\n",htons(port_p2p));
	// Give any address and listen to the specified port
	p2p_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	p2p_addr.sin_port = htons(port_p2p);
	
	if(bind(machine->sock_p2p,(struct sockaddr *)&p2p_addr,length) == -1) {
		perror("Impossible to bind socket to local address\n");
		close(sockfd);
		return -1;
	}

	//#######################################
	// Creation of the id                   #
	//#######################################

	//Get the host ip
	gethostname(hostname, hostname_len);     
	kdm_trace("Host name : %s\n",hostname);
	server = gethostbyname(hostname);
	kdm_trace("Host addr : %s\n",inet_ntoa(*((struct in_addr *)server->h_addr)));

	//Create the string id 
	strcpy(buffer,inet_ntoa(*((struct in_addr *)server->h_addr)));
	strcat(buffer,"/");
	sprintf(port_str,"%d",port_p2p);
	strcat(buffer,port_str);
	buf_len = strlen(buffer);
	kdm_trace("Id not hashed: %s\n", buffer);

	//Hash
	md5_buffer(buffer,buf_len,signature);
	md5_sig_to_string(signature, id, HASH_STRING_LENGTH+1);
	kdm_trace("signature: %s\n", id);

	strcpy(machine->id,id);
	kdm_trace("id_init: %s\n", machine->id);


	//#######################################
	// Try to connect to somebody           #
	//#######################################

	if(strcmp(peer_addr,"")!=0){
		kdm_trace("peer_addr: %s\n", peer_addr);
		//Get the host ip
		pointer = strtok(peer_addr, "/");
		kdm_trace("server : %s\n", pointer);
		server = gethostbyname(pointer);
		if (server == NULL) {
			fprintf(stderr,"ERROR, no such host\n");
		}
		else {
			kdm_trace("Host addr : %s\n",inet_ntoa(*((struct in_addr *)server->h_addr)));
			pointer = strtok(NULL, "/");
			peer_port = atoi(pointer);

			kdm_trace("Port %d\n", peer_port);
			kademPing(machine, inet_ntoa(*((struct in_addr *)server->h_addr)), peer_port );
		}
	}
	kdm_trace(">>>> initMachine\n");
	return 0;
}


int kademMaintenance(struct kademMachine * machine, struct kademMessage* message, char* addr, int port){

	kdm_trace("\n>>>>>>>> kademMaintenance\n");
	kdm_trace("Refresh k_buckets\n");
	
    //Refresh the k-buckets of the routing_Table
	time_t _timestamp;
	int i;
	_timestamp = time (NULL);
	node_details * bucket;
	for (i=0; i<NUMBER_OF_BUCKETS; i++)
	{
		bucket = machine->routes.table[i];
		while ((bucket != NULL) && (_timestamp - bucket->timestamp > KADEM_TIMEOUT_REFRESH_DATA))
		{
			if (bucket->count == 2)
			{
				machine->routes.table[i] = delete_node(machine->routes.table[i], bucket->nodeID);
			} 
			else 
			{
				kademPing(machine, bucket->ip, bucket->port);
				bucket->count++;
				bucket->timestamp = bucket->timestamp + 2;
			}
			bucket = bucket->next;
		}
	}
	
	if(message != NULL && strcmp(json_object_get_string(json_object_object_get(message->header,"y")),"e")!=0)
	{    
        kdm_trace("Insert the node from the message\n");
		
        char* message2;
		message2 = json_object_to_json_string(message->header);
		//kdm_trace("message received header: %s\n",message2);
		//Try to insert the node from which the DHT receives a message except if the message is a ping.
		//Look at the query. 
		char * transactionID;
		char* query;
		transactionID = json_object_get_string(json_object_object_get(message->header,"t"));
		store_file* file;
		printFiles(machine->sent_queries);
		file = find_key(machine->sent_queries, transactionID);
		//kdm_trace("file: %s\n", file);
		

		if (file!=NULL)
		{
            kdm_trace("The message is an answer\n");
			json_object * obj;
			obj = json_tokener_parse(file->value);


			query = json_object_get_string(json_object_object_get(message->header,"r"));

			//kdm_trace("query: %s\n", query);

			if(strcmp( json_object_get_string(json_object_object_get(obj,"q")), KADEM_PING) == 0)
			{
				kdm_trace("query: ping => nothing to do\n");
				//Do nothing
			}
			else
			{
				
				// Extract value from KademMessage
				char* node_id;
				
				node_id = json_object_get_string(json_object_object_get(json_object_object_get(message->header,"r"),"id"));
				//kdm_trace("hash_value: %s\n",hash_value);
				//look for this value into the contact_table.
				int bucket_val, j;
				node_details* bucket2;
				node_details* Kbucket;

				bucket_val = find_node_details(machine->id, node_id)	;
				Kbucket = machine->routes.table[bucket_val];
				bucket2 = look_for_IP(Kbucket, node_id);

				char ip_to_ping[16];
				char portt[7];
				int port_to_ping;
				char nodeID_to_ping[33];
				char delim[] = "/";
				char waiting_node[16+1+6+1+HASH_STRING_LENGTH+1];
				store_file * waiting;

				if(bucket2 != NULL)
				{
					//kdm_trace("Node existing in the routing table => MAJ\n");
					//refresh the timestamp
					j = insert_into_contact_table(&(machine->routes), machine->id, node_id, addr, port);
				}
				else
				{
					//kdm_trace("Node non existing in the routing table => insert it\n");
					//try to insert into the bucket.
					j = insert_into_contact_table(&(machine->routes), machine->id, node_id, addr, port);
					if(j<0)
					{
						//kdm_trace("ip: %s, port: %i\n", Kbucket->ip, Kbucket->port);
						strcpy(ip_to_ping, Kbucket->ip);
						port_to_ping = Kbucket->port;
						strcpy(nodeID_to_ping, Kbucket->nodeID);
						//store the waiting node into the store_file during the ping.  key=nodeID of the node which is pinged / value =  IP/port/nodeID
						strcpy(waiting_node,addr);
						strcat(waiting_node,delim);
						sprintf(portt,"%d",port); 
						strcat(waiting_node,portt);
						strcat(waiting_node,delim);
						strcat(waiting_node,node_id);
						waiting = create_store_file(nodeID_to_ping, waiting_node, strlen(waiting_node));
						machine->waiting_nodes = insert_to_tail_file(machine->waiting_nodes, waiting);
						//kdm_trace("ip: %s, port: %i\n", ip_to_ping, port_to_ping);
						kademPing(machine, ip_to_ping, port_to_ping);
					}

				}
			}
		}
		else
		{

            kdm_trace("message is a query\n");
			query = json_object_get_string(json_object_object_get(message->header,"a"));
			// Extract value from KademMessage
			char* node_id;
			node_id = json_object_get_string(json_object_object_get(json_object_object_get(message->header,"a"),"id"));
			//look for this value into the contact_table.
			int bucket_val, j;
			node_details* bucket2;
			node_details* Kbucket;

			bucket_val = find_node_details(machine->id, node_id)	;
			Kbucket = machine->routes.table[bucket_val];
			bucket2 = look_for_IP(Kbucket, node_id);

			char ip_to_ping[16];
			char portt[7];
			int port_to_ping;
			char nodeID_to_ping[33];
			char delim[] = "/";
            char waiting_node[16+1+6+1+HASH_STRING_LENGTH+1];
			store_file * waiting;

			if(bucket2 != NULL)
			{
				//kdm_trace("Node existing in the routing table => MAJ\n");
				//refresh the timestamp
				j = insert_into_contact_table(&(machine->routes), machine->id, node_id, addr, port);
			}
			else
			{
				//kdm_trace("Node non existing in the routing table => insert it\n");
				//try to insert into the bucket.
				j = insert_into_contact_table(&(machine->routes), machine->id, node_id, addr, port);
				if(j<0)
				{
					//kdm_trace("ip: %s, port: %i\n", Kbucket->ip, Kbucket->port);
					strcpy(ip_to_ping, Kbucket->ip);
					port_to_ping = Kbucket->port;
					strcpy(nodeID_to_ping, Kbucket->nodeID);
					//store the waiting node into the store_file during the ping.  key=nodeID of the node which is pinged / value =  IP/port/nodeID
					strcpy(waiting_node,addr);
					strcat(waiting_node,delim);
					sprintf(portt,"%d",port); 
					strcat(waiting_node,portt);
					strcat(waiting_node,delim);
					strcat(waiting_node,node_id);
					waiting = create_store_file(nodeID_to_ping, waiting_node, strlen(waiting_node));
					machine->waiting_nodes = insert_to_tail_file(machine->waiting_nodes, waiting);
					//kdm_trace("ip: %s, port: %i\n", ip_to_ping, port_to_ping);
					kademPing(machine, ip_to_ping, port_to_ping);
				}
			}
		}
	}
	kdm_trace("Maintenance: waiting_nodes\n");

	//Check into the waiting_node list which nodes can be inserted according to their timestamp.
	store_file * temp;
	temp = machine->waiting_nodes;
	int bucket_number;
	node_details* node_to_insert;
	while(temp !=NULL){
		if(_timestamp - temp->timestamp > KADEM_TIMEOUT_PING){
			bucket_number = find_node_details(machine->id, temp->key);
			node_to_insert = create_node_from_string(temp->value);
			machine->routes.table[bucket_number] = delete_head_insert_tail(machine->routes.table[bucket_number], node_to_insert);
		}
		temp = temp->next;
	}

	kdm_trace("Maintenance: sent_queries\n");
	//TOTEST: Refresh the queries
	store_file * refreshed_queries;
	machine->sent_queries = clean(machine->sent_queries, KADEM_TIMEOUT_REFRESH_QUERY);
	//printFiles(refreshed_queries);

    //kdm_trace("	   >>>> Maintenance: sent_queries\n");

	//TOTEST refresh the files stored
	//kdm_trace("	   <<<< Maintenance: stored_values & token_rec\n");
	//machine->stored_values = clean(machine->stored_values, KADEM_TIMEOUT_REFRESH_DATA);
	machine->token_rec = clean(machine->token_rec,5*KADEM_TIMEOUT_PING);
	//kdm_trace("	   >>>> Maintenance: stored_values & token_rec\n");


	//Delete the out of date node that have count=1 into store_find_queries
	//kdm_trace("	   <<<< Maintenance: store_find_queries\n");
	store_file * temp2;
	node_details *list;
	temp2 = machine->store_find_queries;
	while(temp2 != NULL){
		list = temp2->value;
		//kdm_trace("ok1\n");
		while(list != NULL)
		{
			//print_nodes(list, 0);
			//kdm_trace("ok2\ncount: %i, timestamp: %i\n", list->count, list->timestamp);
			if((list->count == 1) && (_timestamp - list->timestamp > 2*KADEM_TIMEOUT_PING))
			{//kdm_trace("NodeID: %s\n", list->nodeID);
				temp2->value = delete_node(temp2->value, list->nodeID);
			}
			list = list->next;
		}
		temp2 = temp2->next;
	}
	//kdm_trace("	   >>>>> Maintenance: store_find_queries\n");

	kdm_trace("<<<<<<<<<<< kademMaintenance\n\n");

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
	//kdm_trace("Name resolved\n");

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
	//kdm_trace("Address set\n");


	//###############################
	// Create the data to be sent   #
	//###############################

	//First the header
	header = json_object_to_json_string(message->header);
	messageSize += strlen(header);

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
		//kdm_trace("Socket %d, addr %s, port %d, message size %d, size addr %d\n",sockfd,dst_addr, dst_port, messageSize,sizeof(serv_addr));
		perror("Could not send packet!");
		return -1;
	}
	kdm_trace("Packet sent %d bytes to addr %s and port %i: ( %d payload)\n%s\n", sentBytes, dst_addr,dst_port,message->payloadLength, header);
	free(udpPacket);

	return 0; 
}

struct kademMessage kademUdpToMessage(char * udpPacket, int length){
	int payloadLength;
	struct kademMessage message;
	json_object *new_obj;
	new_obj = json_tokener_parse(udpPacket);
	kdm_trace("Message header: %s\n",json_object_to_json_string(new_obj));
	if((payloadLength = length - strlen(json_object_to_json_string(new_obj)) )>0){
		kdm_trace("Found payload of %d bytes\n",payloadLength); 
		memcpy(&message.payload,udpPacket+(length-payloadLength),payloadLength);
	}
	message.header = new_obj;
	message.payloadLength = payloadLength;
	return message;
}

int kademSendError(struct kademMachine * machine, const char *transactionId, const char * const code, const char * const message, char *addr, int port){

	kdm_trace(">>>> kademSendError\n");
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
	kdm_trace("<<<< kademSendError\n");
	return ret;
}




int generateTransactionId(char * transactionId, char * id){

	//kdm_trace(">>>> generateTransactionId\n");
	int random_nb;
	int len;
	char string_rand[32],buffer[HASH_STRING_LENGTH+1], buffer2[2*HASH_STRING_LENGTH+1],signature[HASH_SIGNATURE_LENGTH];

	//Retrieve time
	random_nb = rand();
	sprintf(string_rand,"%d",random_nb);
	len = strlen(string_rand); 
	//kdm_trace("Rand : %s (%d chars)\n", string_rand, len);

	//Hash time
	md5_buffer(string_rand,len,signature);
	md5_sig_to_string(signature, buffer, HASH_STRING_LENGTH+1);
	//kdm_trace("Rand hashed: %s\n", buffer);
	//kdm_trace("id: %s\n", id);
	//Hash time + id
	strcpy(buffer2,buffer);
	strcat(buffer2,id);
	len = strlen(buffer2);
	md5_buffer(buffer2,len,signature);
	md5_sig_to_string(signature, transactionId, HASH_STRING_LENGTH+1);

	//kdm_trace("Transaction id generated: %s\n", transactionId);
	//kdm_trace("<<<< generateTransactionId\n");
	return 0;

}


/*============================================
  P2P communication
  ============================================*/

int kademPing(struct kademMachine * machine, char * addr, int port){

	kdm_trace("<<<< kademPing\n");
	struct kademMessage message;
	int ret;
	json_object *header, *argument;
	char transactionId[HASH_STRING_LENGTH+1]; 
	store_file* query;
	char * head;
	kdm_trace("id_ping: %s\n", machine->id);
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

	// Store Query in sent_queries
	head = json_object_to_json_string(header);
	query = create_store_file( transactionId, head, strlen(head)+1);
	machine->sent_queries = insert_to_tail_file(machine->sent_queries, query);

	json_object_put(header);
	json_object_put(argument);
	kdm_trace(">>>> kademPing\n");
	return ret;
}

int kademPong(struct kademMachine *machine, struct kademMessage *message, char * addr, int port){

	kdm_trace(">>>>>>>>>>> kademPong\n");
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


	kdm_trace("<<<<<<<<<< kademPong\n\n");
	return ret;
}

//NO Segmentation Fault.
int kademHandlePong(struct kademMachine *machine, struct kademMessage *message, char* ip, int port){

	kdm_trace(">>>> Handle pong\n");

	const char *transactionId, *id;
	json_object *header, *response, *rpc_header, *rpc_argument;
	struct kademMessage rpc_message;

	header = message->header;
	transactionId = json_object_get_string(json_object_object_get(header,"t"));

	response = json_object_object_get(header,"r");

	id = json_object_get_string(json_object_object_get(response,"id"));

	insert_into_contact_table(&machine->routes,machine->id,id,ip,port); 
	machine->sent_queries = delete_key(machine->sent_queries, transactionId); 

	if(strcmp(machine->latest_query_rpc.query,KADEM_PING)==0){
		if(strcmp(machine->latest_query_rpc.value,id)==0){
			strcpy(machine->latest_query_rpc.query,"");
			rpc_header = json_object_new_object();
			rpc_argument = json_object_new_object();
			json_object_object_add(rpc_argument,"resp",json_object_new_string("OK"));
			json_object_object_add(rpc_header,"y",json_object_new_string(KADEM_ANSWER));
			json_object_object_add(rpc_header,"r",rpc_argument);
			rpc_message.header = rpc_header;
			rpc_message.payloadLength = 0;
			strcpy(rpc_message.payload,"");
			kademSendMessage(machine->sock_local_rpc, &rpc_message, machine->latest_query_rpc.ip, machine->latest_query_rpc.port);
			json_object_put(rpc_header);
		}
	}

	//Answer to the ping made by KadMaintenance. Delete the node that was trying to be inserted
	store_file* temp;
	temp = machine->waiting_nodes;
	while(temp !=NULL){
		if(strcmp(temp->key, id)==0){
			machine->waiting_nodes = delete_key(machine->waiting_nodes, id);
		}
		temp = temp-> next;
	}

	kdm_trace("<<<< Handle pong\n");
	return 0; 
}

int kademFindNode(struct kademMachine * machine, char * target_id, char * addr, int port){

	kdm_trace("<<<< kademFindNode\n");
	struct kademMessage message;
	int ret;
	json_object *header, *argument;
	char transactionId[HASH_STRING_LENGTH+1]; 
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

	// Store Query in sent_queries
	head = json_object_to_json_string(message.header);
	query = create_store_file( transactionId, head, strlen(head)+1);
	machine->sent_queries = insert_to_tail_file(machine->sent_queries, query);

	json_object_put(header);
	json_object_put(argument);

	kdm_trace(">>>> kademFindNode\n");
	return ret;
}
//NO Segmentation Fault
int kademHandleFindNode(struct kademMachine * machine, struct kademMessage * message, char * addr, int port){

	 kdm_trace(">>>> kademHandleFindNode\n");
    struct kademMessage answer_message;
    int ret;
    json_object *header, *response, *node_array, *query;
    const char * transactionId, *key;
    node_details* nodes = NULL, *current_node;
    char node_string[15+6+3+1];

    transactionId = json_object_get_string(json_object_object_get(message->header,"t"));

    query = json_object_object_get(message->header,"a");
    key = json_object_get_string(json_object_object_get(query,"target"));

    header = json_object_new_object(); 
    json_object_object_add(header, "t",json_object_new_string(transactionId));
    json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

    response = json_object_new_object();
    json_object_object_add(response,"id",json_object_new_string(machine->id));

    node_array = json_object_new_array();
    //find nodes
    kdm_trace("key: %s\n",key);
    nodes = k_nearest_nodes(nodes,&machine->routes,machine->id,key); 
    current_node = nodes;
    while(current_node != NULL){
        concatenate(current_node,node_string);        
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

    kdm_trace("<<<< kademHandleFindNode\n");
    return ret;

}

int kademHandleAnswerFindNode(struct kademMachine * machine, struct kademMessage * message, char *ip, int port){

	 
    kdm_trace(">>>>>>kademHandleAnswerFindNode\n");

    int i;
    char node_string[HASH_STRING_LENGTH+15+6+3+1];
    const char *transaction_id, *sent_query_value, *node_id;
    store_file *sent_query, *find_query;
    node_details *current_nodes, *result_nodes, *loop_node;
    struct kademMessage rpc_msg;
    json_object *sent_query_msg, *rpc_msg_header, *rpc_node_array, *argument2, *response_content, *response_content_nodes, *loop_obj;


    transaction_id = json_object_get_string(json_object_object_get(message->header,"t"));
    sent_query= find_key(machine->sent_queries, transaction_id); //result->value is a kademMessage (result points 
    if(sent_query != NULL){

        //Find the query with the hash (value of sent query)
        sent_query_msg = json_tokener_parse(sent_query->value);
        argument2 = json_object_object_get(sent_query_msg,"a");
        sent_query_value = json_object_get_string(json_object_object_get(argument2,"target"));

        kdm_trace("Sent query value : %s\n", sent_query_value);
        response_content = json_object_object_get(message->header,"r");
        response_content_nodes = json_object_object_get(response_content,"nodes");

        kdm_trace("try to find the query\n");
        find_query = find_key(machine->store_find_queries, sent_query_value);

        if ((find_query != NULL) && response_content_nodes != NULL){ //sent_query_value is the hash of the searched value

            kdm_trace("Response content nodes %s\n",json_object_to_json_string(response_content_nodes));

            loop_node =  create_node_from_string(json_object_get_string(json_object_array_get_idx(response_content_nodes,0)));

            if(strcmp(loop_node->nodeID,sent_query_value) == 0){
                kdm_trace("Node Found !\n");
                if (strcpy(machine->latest_query_rpc.query, "find_node") == 0 ){
                    // Answer to the get from the RP
                    // Write the header 
                    rpc_msg_header = json_object_new_object();
                    json_object_object_add(rpc_msg_header,"resp",json_object_new_string("OK"));

                    loop_node = result_nodes;
                    rpc_node_array = json_object_new_array();
                    json_object_array_add(rpc_node_array, json_object_new_string(concatenate(loop_node ,node_string)));

                    // Write the message
                    rpc_msg.header = rpc_msg_header;
                    rpc_msg.payloadLength = 0;
                    kademSendMessage(machine->sock_local_rpc, &rpc_msg, machine->latest_query_rpc.ip, machine->latest_query_rpc.port);
                    json_object_put(rpc_msg_header);
                    json_object_put(rpc_node_array);
                }

                // Delete the find query
                machine->store_find_queries = delete_key(machine->store_find_queries, sent_query_value);

            }
            else {
                kdm_trace("Node not found\n");
                node_id = json_object_get_string(json_object_object_get(response_content,"id"));
                if(lookUpRound(machine,"", find_query, response_content_nodes, node_id, ip, port)!= 0)
                {
                    // Research algorithm is finished
                    find_query = find_key(machine->store_find_queries, sent_query_value);
                    current_nodes = (node_details*)(find_query->value);
                    loop_node = current_nodes;
                    result_nodes = NULL; 

                    //split list in two
                    while(loop_node != NULL && loop_node->next != NULL && loop_node->next->count < 2 )
                    {
                        loop_node = loop_node->next;
                    }
                    if(loop_node != NULL)
                    {
                        if(loop_node->count < 2)
                        {
                            result_nodes = loop_node->next;
                            loop_node->next = NULL;
                        } else {
                            result_nodes = loop_node;
                            current_nodes = NULL;
                        }
                    }

                    if (strcpy(machine->latest_query_rpc.query, "find_node") == 0 ){
                        // Answer to the get from the RP
                        // Write the header 
                        rpc_msg_header = json_object_new_object();
                        json_object_object_add(rpc_msg_header,"resp",json_object_new_string("NOK"));

                        loop_node = result_nodes;

                        rpc_node_array = json_object_new_array();
                        while(loop_node != NULL){
                            json_object_array_add(rpc_node_array, json_object_new_string(concatenate(loop_node ,node_string)));
                            loop_node = loop_node->next;
                        }

                        // Write the message
                        rpc_msg.header = rpc_msg_header;
                        rpc_msg.payloadLength = 0;
                        kademSendMessage(machine->sock_local_rpc, &rpc_msg, machine->latest_query_rpc.ip, machine->latest_query_rpc.port);
                        json_object_put(rpc_msg_header);
                        json_object_put(rpc_node_array);
                    }

                    // Delete the find query
                    machine->store_find_queries = delete_key(machine->store_find_queries, sent_query_value);
                    json_object_put(rpc_msg_header);
                }
            }
        }
        else {
            kdm_trace("No corresponding query found\n");
        }
        json_object_put(response_content);
        json_object_put(response_content_nodes);
        machine->sent_queries = delete_key(machine->sent_queries, transaction_id);

    }
    else {
        kdm_trace("Query not found\n");
    }
    kdm_trace("<<<<<<kademHandleAnswerFindNode\n");
    return 0; 
} 

int lookUpRound(struct kademMachine * machine, const char * token, store_file *find_query, json_object * response_content_nodes, const char* node_id, char * ip, int port){

	 kdm_trace(">>>>>> Lookup Round\n");
    
    node_details *loop_node, *current_nodes, *result_nodes;
    json_object *loop_obj;
    store_file *find_query2;
    int i, ret;

    // compare nodes with answered nodes: Knodes1 = Knodes2 - Knodes1

    current_nodes = (node_details*)(find_query->value);
    loop_node = current_nodes; 
    result_nodes = NULL;

    kdm_trace("Total nodes list\n");
    print_nodes(current_nodes,-1);

    //split list in two
    while(loop_node != NULL && loop_node->next != NULL && loop_node->next->count < 2 )
    {
        loop_node = loop_node->next;
    }
    if(loop_node != NULL)
    {
        if(loop_node->count < 2)
        {
            result_nodes = loop_node->next;
            loop_node->next = NULL;
        } else {
            result_nodes = loop_node;
            current_nodes = NULL;
        }
    }
    kdm_trace("Current nodes\n");
    print_nodes(current_nodes,-1);
    kdm_trace("Result nodes\n"); 
    print_nodes(result_nodes,-1);


    
    //kdm_trace("Node to delete %s\n", node_id);
    //Move the new node into the result
    current_nodes = delete_node(current_nodes, node_id);
    kdm_trace("Apres delete %s\n", node_id);

    kdm_trace("Current nodes\n");
    print_nodes(current_nodes,-1);
    kdm_trace("Result nodes\n"); 
    print_nodes(result_nodes,-1);
   // kdm_trace("Node deleted\n");
    loop_node = NULL;
    loop_node = create_node_details(loop_node, ip, port,node_id); 
    //kdm_trace("New node created\n");
    loop_node->count = 2;

    //kdm_trace("Node transferred in the confirmed node list\n");
    result_nodes = insert_acc_distance(result_nodes,loop_node, find_query->key);
    //kdm_trace("\n\nNode transferred in the confirmed node list\n");
    
    //kdm_trace("\n\n\nNodes : %s\n\n\n", json_object_to_json_string(response_content_nodes));
kdm_trace("%s\n",json_object_to_json_string(response_content_nodes));
    // Look for the new received nodes and put them into the current_node list
    for(i=0; i < json_object_array_length(response_content_nodes); i++) {
        loop_obj = json_object_array_get_idx(response_content_nodes, i);
        loop_node = create_node_from_string(json_object_get_string(loop_obj));
       
        // Insert in the list only of it is not already there and this is node the node itself 
        if(look_for_IP(current_nodes,loop_node->nodeID)==NULL && look_for_IP(result_nodes,loop_node->nodeID)==NULL && strcmp(loop_node->nodeID,machine->id)!=0){

            current_nodes = insert_acc_distance(current_nodes,loop_node , find_query->key );
        }
    }
    //kdm_trace("\n\nCurrent nodes list\n");
    //print_nodes(current_nodes,-1);
   // kdm_trace("Result nodes list\n");
   // print_nodes(result_nodes,-1);

    find_query->count--;
    // Go to the next round for the research
    if(current_nodes != NULL || find_query->count > 0){

        // Send find values request to nearest nodes and increment count
        kdm_trace("Algorithm not finished try another round (%d waiting)\n", find_query->count);
        i=0;
        loop_node = current_nodes;
        if(strcmp(token,"")==0){
            while ((i < KADEM_ALPHA) && (loop_node != NULL))
            {
                if(loop_node->count<1){
                    kademFindNode(machine, find_query->key, current_nodes->ip, current_nodes->port);
                    find_query->count++;
                    i++;
                    loop_node->count = 1;
                }
                loop_node = loop_node->next;
            }
        }
        else {
            while ((i < KADEM_ALPHA) && (loop_node != NULL))
            {
                if(loop_node->count<1){
                    kademFindValue(machine, find_query->key,token, current_nodes->ip, current_nodes->port);
                    find_query->count++;
                    i++;
                    loop_node->count = 1;
                }
                loop_node = loop_node->next;
            }
        }
        kdm_trace("Current nodes\n");
        print_nodes(current_nodes,-1);
        kdm_trace("Result nodes\n"); 
        print_nodes(result_nodes,-1);

        ret = 1;
    }
    else {
        //Algorithm finished
        kdm_trace("Algorithm finished\n");
        ret = 0;
    }
    
    // Stick current and result list together
    loop_node = current_nodes;
    if(current_nodes == NULL){
        current_nodes = result_nodes;
    }
    else{
        while(loop_node->next !=NULL){
            loop_node = loop_node->next;
        }
        loop_node->next = result_nodes;
    }

    find_query->value = (char *)current_nodes;
 
    find_query2 = create_store_file(find_query->key, find_query->value, find_query->value_len); 
    
    machine->store_find_queries = insert_to_tail_file(machine->store_find_queries, find_query2);

    kdm_trace("<<<<<< Lookup Round\n");
    return ret;
	
}

int kademFindValue(struct kademMachine * machine, char * value, char* token, char *addr, int port){

  kdm_trace(">>>> kademFindValue\n");
    struct kademMessage message;
    int ret;
    json_object *header, *argument;
    char transactionId[HASH_STRING_LENGTH+1]; 
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


    // Store Query in sent_queries
    head = json_object_to_json_string(message.header);
    query = create_store_file( transactionId, head, strlen(head)+1);
    machine->sent_queries = insert_to_tail_file(machine->sent_queries, query);

    json_object_put(header);
    json_object_put(argument);
    kdm_trace("<<<< kademFindValue\n");
    return ret;

}

//NO Segmentation
int kademHandleFindValue(struct kademMachine * machine, struct kademMessage * message,char *addr, int port){


	
    kdm_trace(">>>> kademHandleFindValue\n");
    struct kademMessage answer_message;
    int ret;
    json_object *header, *response, *node_array, *query;
    const char * transactionId, *key, *token;
    store_file * value; 
    char node_string[HASH_STRING_LENGTH+15+6+3+1];
    node_details* nodes=NULL, *current_node=NULL;

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
    kdm_trace("Looking for value : %s\n",key);
    
    store_file* find;
    print_values(machine->stored_values);
    find = find_key(machine->stored_values,key);
    char portt[6];
    //look for value
    if(find!=NULL){
        kdm_trace("Value found !\n");
        json_object_object_add(response,"value",json_object_new_string(key));
        kdm_trace("find->value_len: %i\n",find->value_len);
        json_object_object_add(response,"numbytes",json_object_new_int(find->value_len));
        answer_message.payloadLength = find->value_len;
        strcpy(answer_message.payload,find->value);
    }

    else {
        store_file* store_token;
        kdm_trace("Value not found\n");
        // Store token for future puts
        store_token = create_store_file(token,NULL,0);
        machine->token_rec = insert_to_tail_file(machine->token_rec,store_token);
        
        // Answer with the k_nearest_nodes 
        nodes = k_nearest_nodes(nodes,&machine->routes,machine->id,key); 
        current_node = nodes;
        while(current_node != NULL){
            strcpy(node_string,current_node->ip);
            strcat(node_string,"/");
            sprintf(portt,"%d",current_node->port);
	    strcat(node_string,portt);
            strcat(node_string,"/");
            strcat(node_string,current_node->nodeID);
	    
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

    kdm_trace("<<<< kademHandleFindValue\n");
    return ret;

}

int kademHandleAnswerFindValue(struct kademMachine * machine, struct kademMessage * message, char *ip ,int port){

	 kdm_trace(">>>> kademHandleAnswerFindValue\n");

    // Search in machine's store_find_queries the corresponding list of nodes (by hash)
    //Find the sent request corresponding to the answer in machine's sent_queries (by transactionID)
    int i;
    char node_string[HASH_STRING_LENGTH+15+6+3+1];
    const char *transaction_id, *sent_query_value, *token, *node_id;
    store_file *sent_query, *find_query, *_store_file, *token_sent;
    node_details *current_nodes, *result_nodes, *loop_node;
    struct kademMessage rpc_msg;
    json_object *sent_query_msg, *rpc_msg_header, *argument2, *response_content, *response_content_value, *response_content_nodes, *loop_obj;

    transaction_id = json_object_get_string(json_object_object_get(message->header,"t"));
    sent_query= find_key(machine->sent_queries, transaction_id); //result->value is a kademMessage (result points 
    if(sent_query != NULL){
        kdm_trace("Found query \n");
        //Find the query with the hash (value of sent query)
        sent_query_msg = json_tokener_parse(sent_query->value);
        argument2 = json_object_object_get(sent_query_msg,"a");
        sent_query_value = json_object_get_string(json_object_object_get(argument2,"value"));
        find_query = find_key(machine->store_find_queries, sent_query_value);
        
        if ((find_query != NULL)){ 
            kdm_trace("Found find query \n");
            //sent_query_value is the hash of the searched value

            token = json_object_get_string(json_object_object_get(argument2,"token"));
            response_content = json_object_object_get(message->header,"r");
            response_content_value = json_object_object_get(response_content,"value");
            response_content_nodes = json_object_object_get(response_content,"nodes");
            
            if (response_content_value != NULL) // If value found:
            {
                kdm_trace("Value %s found\n", sent_query_value);
                // store value
                message->payloadLength = atoi(json_object_get_string(json_object_object_get(response_content,"numbytes")));
                _store_file = create_store_file( sent_query_value, message->payload, message->payloadLength);
                machine->stored_values = insert_to_tail_file(machine->stored_values, _store_file);

                //Look if the query is the latest query from the RPC
                if((strcpy(machine->latest_query_rpc.query, "get") == 0 ) && (strcpy(machine->latest_query_rpc.value, sent_query_value) == 0))
                {
                    // Answer to the get from the RP
                    // Write the header 
                    rpc_msg_header = json_object_new_object();
                    json_object_object_add(rpc_msg_header,"resp",json_object_new_string("OK"));
                    json_object_object_add(rpc_msg_header,"numbytes",json_object_new_int(_store_file->value_len));
                    // Write the message
                    rpc_msg.header = rpc_msg_header;
                    rpc_msg.payloadLength = _store_file->value_len;
                    memcpy(rpc_msg.payload,_store_file,rpc_msg.payloadLength);
                    kademSendMessage(machine->sock_local_rpc, &rpc_msg, machine->latest_query_rpc.ip, machine->latest_query_rpc.port);

                    json_object_put(rpc_msg_header);
                }
                // Delete the find query
                machine->store_find_queries = delete_key(machine->store_find_queries, sent_query_value);
            }

            // If no value found: new set of nodes received
            else if (response_content_nodes != NULL) 
            {
                kdm_trace("Value not found\n");
                node_id = json_object_get_string(json_object_object_get(response_content,"id"));

                // Research algorithm is finished
                if(lookUpRound(machine,token, find_query, response_content_nodes, node_id, ip, port)== 0)
                {
                    // Research algorithm is finished
                    find_query = find_key(machine->store_find_queries, sent_query_value);
                    current_nodes = (node_details*)(find_query->value);
                    loop_node = current_nodes;
                    result_nodes = NULL; 

                    //split list in two
                    while(loop_node != NULL && loop_node->next != NULL && loop_node->next->count < 2 )
                    {
                        loop_node = loop_node->next;
                    }
                    if(loop_node != NULL)
                    {
                        if(loop_node->count < 2)
                        {
                            result_nodes = loop_node->next;
                            loop_node->next = NULL;
                        } else {
                            result_nodes = loop_node;
                            current_nodes = NULL;
                        }
                    }
                    //Look if the query is the latest query from the RPC
                    if(strcpy(machine->latest_query_rpc.query, "get") == 0 )
                    {
                        // Answer to the get from the RP
                        // Write the header 
                        rpc_msg_header = json_object_new_object();
                        json_object_object_add(rpc_msg_header,"resp",json_object_new_string("NOK"));

                        // Write the message
                        rpc_msg.header = rpc_msg_header;
                        rpc_msg.payloadLength = 0;
                        kademSendMessage(machine->sock_local_rpc, &rpc_msg, machine->latest_query_rpc.ip, machine->latest_query_rpc.port);
                        json_object_put(rpc_msg_header);
                    }

                    // Look if it was a put query originally
                    if((token_sent = find_key(machine->token_sent,token)) != NULL){
                        kademSendStoreValue(machine, result_nodes, sent_query_value, token_sent->key);
                    } 

                    // Delete the find query
                    machine->store_find_queries = delete_key(machine->store_find_queries, sent_query_value);
                }

            }
            else //TODO Handle error protocole 
            {

            }
            json_object_put(response_content);
            json_object_put(response_content_value);
            json_object_put(response_content_nodes);
        }
        else{
            kdm_trace("Find Value query not found\n");
        }
    }
    else
    {
        kdm_trace("Query not found\n");
    }
    machine->sent_queries = delete_key(machine->sent_queries, transaction_id);




    kdm_trace("<<<< kademHandleAnswerFindValue\n");
    return 0; 
}


//No segmentation fault
int kademStoreValue(struct kademMachine * machine, char * token, char * value, char * data, int data_len, char *dst_addr, int dst_port){

	kdm_trace(">>>> kademStoreValue\n");
	struct kademMessage message;
	int ret;
	json_object *header, *argument;
	char transactionId[HASH_STRING_LENGTH+1], num_bytes[10]; 
	store_file* query;
	char * head;

	generateTransactionId(transactionId,machine->id);
    sprintf(num_bytes,"%d",data_len);

	header = json_object_new_object(); 
	json_object_object_add(header, "t",json_object_new_string(transactionId));
	json_object_object_add(header, "y",json_object_new_string(KADEM_QUERY));
	json_object_object_add(header, KADEM_QUERY,json_object_new_string(KADEM_STORE));

	argument = json_object_new_object();
	json_object_object_add(argument,"id",json_object_new_string(machine->id));
	json_object_object_add(argument,"token",json_object_new_string(token));
	json_object_object_add(argument,"value",json_object_new_string(value));
	json_object_object_add(argument,"numbytes",json_object_new_string(num_bytes));

	json_object_object_add(header,"a",json_object_get(argument));

	message.header = header;
	message.payloadLength = data_len;
	memcpy(message.payload,data,data_len); 

	ret = kademSendMessage(machine->sock_p2p, &message, dst_addr, dst_port);

    // Store Query in sent_queries
    head = json_object_to_json_string(message.header);
    query = create_store_file( transactionId, head, strlen(head)+1);
    machine->sent_queries = insert_to_tail_file(machine->sent_queries, query);

	json_object_put(header);
	json_object_put(argument);
	kdm_trace("<<<< kademStoreValue\n");
	return ret;
}

//NO segmentation fault
int kademHandleStoreValue(struct kademMachine * machine, struct kademMessage * message,char * addr, int port){

	kdm_trace("<<<< kademHandleStoreValue\n");
	struct kademMessage answer_message;
	int ret,found = 0,i=0;
	json_object *header, *response, *query_argument;
	const char *transactionId, *token, *key, *num_bytes;
	char *data;
    int data_length;

	transactionId = json_object_get_string(json_object_object_get(message->header,"t"));
	query_argument = json_object_object_get(message->header,"a");

	token = json_object_get_string(json_object_object_get(query_argument,"token"));
    
    kdm_trace("Token to look for: %s\n",token);
    kdm_trace("Available tokens:\n");
    print_values(machine->token_rec);


	// Verify if token exists into token_rec.
	store_file* file;
	file = find_key(machine->token_rec, token);
	if(file!=NULL){
	    kdm_trace("Token found storing data\n");
        //fix payload + create_store_file
		key = json_object_get_string(json_object_object_get(query_argument,"value"));
		num_bytes = json_object_get_string(json_object_object_get(query_argument,"numbytes"));
		
        data = message->payload;
		data_length = atoi(num_bytes);
	    kdm_trace("Data length %d\n",data_length);
	
        machine->stored_values = insert_to_tail_file(machine->stored_values, create_store_file(key,data,data_length));
        print_values(machine->stored_values);

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
	kdm_trace(">>>> kademHandleStoreValue\n");
	return ret;

}


int kademHandleAnswerStoreValue(struct kademMachine * machine, struct kademMessage * message){

	return 0;
}

//NO segmentation Fault.
int kademSendStoreValue(struct kademMachine * machine, node_details* node_to_send, char* value, char* token){

	kdm_trace(">>>>> kademSendStoreValue\n");
	int i=0,number_sent;
	store_file* one_token_sent;
	node_details* temp=NULL;

	//look for the token into token_sent.
	one_token_sent = find_key(machine->token_sent, token);
	kdm_trace("Token found: %s\n",one_token_sent->key);

	//look how many nodes we sent the value to.
	number_sent = count_nodes_details(node_to_send); 
	kdm_trace("Number of sent queries: %i\n", number_sent);
	temp = node_to_send;
	kdm_trace("data to store: %s\n", one_token_sent->key); 

	//print_nodes(temp,-1);
	kdm_trace("one_token_sent length: %i\n", one_token_sent->value_len);

	while(temp!=NULL){
		i++;
		kademStoreValue(machine, token, value, one_token_sent->value, one_token_sent->value_len, temp->ip, temp->port);
		temp = temp->next;
	}
    kdm_trace("Sent to %i nodes\n", i);
	
    //delete the one_token_sent in the token_store list.
	machine->token_sent = delete_key(machine->token_sent, token);

	kdm_trace("<<<< kademSendStoreValue\n");
	return 0;
}


int startKademlia(struct kademMachine * machine){
	kdm_trace(">>>> startKademlia\n");
	int ret_select, num_read,from_port;
	socklen_t from_len;
	fd_set readfds;
	struct timeval tv;
	struct kademMessage message;
	struct sockaddr_in from;
	char udpPacket[KADEM_MAX_PAYLOAD_SIZE+100];
	const char * buffer;
	char * from_addr;
	char cmd[10],arg[50];

	from_len = sizeof(struct sockaddr_in);

	while(1){

		// clear the set ahead of time
		FD_ZERO(&readfds);
		// add our descriptors to the set
		FD_SET(machine->sock_local_rpc,&readfds);
		FD_SET(machine->sock_p2p,&readfds);
		FD_SET(0,&readfds);

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
            kdm_trace("\n\n################### LOOP #################\n\n\n");

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

				kdm_trace("Received RPC message from %s/%d\n", from_addr, from_port);
				// Determine the type of the message 
				buffer = json_object_get_string(json_object_object_get(message.header,"y"));
				// Message is a query
				if (strcmp(buffer,KADEM_QUERY) == 0){

					// Determine the type of the query
					buffer = json_object_get_string(json_object_object_get(message.header,KADEM_QUERY));
					if (strcmp(buffer,"print_routing_table") == 0){
						RPCHandlePrintRoutingTable(machine, &message, from_addr, from_port);  
					}  
					else if (strcmp(buffer,"print_object_ids") == 0){
						RPCHandlePrintObjects(machine, &message, from_addr, from_port); 
					}                
					else if (strcmp(buffer,"ping") == 0){
						RPCHandlePing(machine, &message, from_addr, from_port);  
					}                
					else if (strcmp(buffer,"kill_node") == 0){
						RPCHandleKillNode(machine, &message, from_addr, from_port);  
					}                
					else if (strcmp(buffer,"put") == 0){
						kdm_trace("RECEIVED PUT FROM RPC\n");
						RPCHandleStoreValue(machine, &message, from_addr, from_port); 
						kdm_trace("OBJECT_IDS LIST\n ");
						print_values(machine->stored_values);
					}
					else if (strcmp(buffer,"get") == 0){
						RPCHandleFindValue(machine, &message, from_addr, from_port);  
					}
					else if (strcmp(buffer,"find_node") == 0){
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
				kdm_trace("Received P2P message from %s/%d\n", from_addr, from_port);
				kademMaintenance(machine, &message, inet_ntoa(from.sin_addr), from_port); 
				// Determine the type of the message 
				buffer = json_object_get_string(json_object_object_get(message.header,"y"));

				// Message is a query
				if (strcmp(buffer,KADEM_QUERY) == 0){
					// Determine the type of the query
					buffer = json_object_get_string(json_object_object_get(message.header,KADEM_QUERY));

					if (strcmp(buffer,KADEM_PING) == 0){
						kademPong(machine, &message, from_addr, from_port);  
					}                
					else if (strcmp(buffer,KADEM_STORE) == 0){
						kademHandleStoreValue(machine, &message, from_addr, from_port);  
					}                
					else if (strcmp(buffer,KADEM_FIND_NODE) == 0){
						kademHandleFindNode(machine, &message, from_addr, from_port);  
					}                
					else if (strcmp(buffer,KADEM_FIND_VALUE) == 0){
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
						else if (strcmp(query_type,KADEM_STORE) == 0)
						{
							kademHandleAnswerStoreValue(machine, &message);  
						}                
						else if (strcmp(query_type,KADEM_FIND_NODE) == 0)
						{
							kademHandleAnswerFindNode(machine, &message, from_addr, from_port);  
						}                
						else if (strcmp(query_type,KADEM_FIND_VALUE) == 0)
						{
							kademHandleAnswerFindValue(machine, &message, from_addr, from_port);  
						} 
						else
						{
							kademSendError(machine, transID, KADEM_ERROR_METHOD_UNKNOWN, KADEM_ERROR_METHOD_UNKNOWN_VALUE, from_addr, from_port);
						}  

                        machine->sent_queries = delete_key(machine->sent_queries, transID); //Delete the sent query from sent_queries   
                    }  

				}

				// Message is an error
				else if (strcmp(buffer,KADEM_ERROR) == 0){
					json_object * error_msg;
					char *_code, *_value;
					const char *transID; 

					transID = json_object_get_string(json_object_object_get(message.header,"t"));
					error_msg = json_object_object_get(message.header,KADEM_ERROR);
					_code = json_object_get_string(json_object_object_get(error_msg,"code"));
					_value = json_object_get_string(json_object_object_get(error_msg,"value"));

					if (strcmp(_code,KADEM_ERROR_GENERIC) == 0){
						fprintf(stderr, "%s from address: %s and port: %i", _value, from_addr, from_port);  
					}
					else if (strcmp(_code,KADEM_ERROR_INVALID_TOKEN) == 0){
						fprintf(stderr, "%s from address: %s and port: %i", _value, from_addr, from_port);  
					}
					else if (strcmp(_code,KADEM_ERROR_PROTOCOL) == 0){
						fprintf(stderr, "%s from address: %s and port: %i", _value, from_addr, from_port);  
					}
					else if (strcmp(_code,KADEM_ERROR_METHOD_UNKNOWN) == 0){
						fprintf(stderr, "%s from address: %s and port: %i", _value, from_addr, from_port);  
					}
					else if (strcmp(_code,KADEM_ERROR_STORE) == 0){
						fprintf(stderr, "%s from address: %s and port: %i", _value, from_addr, from_port);  
					}   
					else
					{
						kademSendError(machine, transID, KADEM_ERROR_METHOD_UNKNOWN, KADEM_ERROR_METHOD_UNKNOWN_VALUE, from_addr, from_port);
					}
					machine->sent_queries= delete_key(machine->sent_queries, transID); //Delete the sent query from sent_queries                                     
				}

				// Message type is unknown
				else
				{
					const char *transID; 
					transID = json_object_get_string(json_object_object_get(message.header,"t"));
					kademSendError(machine, transID, KADEM_ERROR_PROTOCOL, KADEM_ERROR_PROTOCOL_VALUE, from_addr, from_port);
					machine->sent_queries=delete_key(machine->sent_queries, transID); //Delete the sent query from sent_queries
				}
			}

			if(FD_ISSET(fileno(stdin), &readfds)) {
				scanf("%s",cmd,arg);
				if (strcmp(cmd,"print_routing_table") == 0){
					print_routing_table(machine->routes);
				}  
				else if (strcmp(cmd,"print_object_ids") == 0){
					print_values(machine->stored_values);
				}                
				else if (strcmp(cmd,"ping") == 0){
					scanf("%s",cmd,arg);
				}                
				else if (strcmp(cmd,"kill_node") == 0){
				}                
				else if (strcmp(cmd,"put") == 0){
					scanf("%s",cmd,arg);
				}
				else if (strcmp(cmd,"get") == 0){
					scanf("%s",cmd,arg);
				}
				else if (strcmp(cmd,"find_node") == 0){
					scanf("%s",cmd,arg);
				}
				else{
					/*const char *transID;
					  transID = json_object_get_string(json_object_object_get(message.header,"t"));
					  kademSendError(machine, transID, KADEM_ERROR_METHOD_UNKNOWN, KADEM_ERROR_METHOD_UNKNOWN_VALUE, from_addr, from_port);*/
                }
                kdm_trace("Entry\n");
                fflush(stdin);
                fflush(stdout);
			}
            kdm_trace("\nWaiting queries\n");
            print_values(machine->sent_queries);
		}

	}
	kdm_trace("<<<< startKademlia\n");
	return 0;


}




/*============================================
  RPC communication
  ============================================*/

//No Segmentation Fault
int RPCHandleStoreValue(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){

	kdm_trace(">>>>>>>>> RPCHandleStoreValue\n");
	//look for value into the header of message.
	const char *rpc_query, *rpc_value , *num_bytes;
	json_object *rpc_argument,*header, *argument;
	store_file* new_value;
	char *data; 
	struct kademMessage answer_message;
	char ok[] = "OK";
	node_details* nearest_nodes = NULL;
	store_file* store_file_temp = NULL;

	data = message->payload;
	rpc_argument = json_object_object_get(message->header,"a");
	rpc_query = json_object_get_string(json_object_object_get(message->header,"q"));
	rpc_value = json_object_get_string(json_object_object_get(rpc_argument,"value"));
	num_bytes = json_object_get_string(json_object_object_get(rpc_argument,"numbytes"));
	new_value = create_store_file(rpc_value, data, atoi(num_bytes));
	machine->stored_values = insert_to_tail_file(machine->stored_values, new_value); //Value stored.
	
    //Answer to the RPC
	header = json_object_new_object(); 

	json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

	argument = json_object_new_object();
	json_object_object_add(argument,"resp",json_object_new_string(ok));
	json_object_object_add(header,KADEM_ANSWER,json_object_get(argument));

	answer_message.header = header;
	answer_message.payloadLength = 0;


	if(kademSendMessage(machine->sock_local_rpc, &answer_message, addr, port)<0){
		printf("Counldn't send message\n");
		return -1;
	}   
    //Answered to the RPC.

/*

	//Store the query in the machine last_query field. Store the data? Useless?
	strcpy(machine->latest_query_rpc.query, temp2);
	strcpy(machine->latest_query_rpc.value, value);		
	strcpy(machine->latest_query_rpc.ip, addr);   //useless because we already answered.
	machine->latest_query_rpc.port = port;
*/

	
	//Create a token for this query.
	char token[HASH_STRING_LENGTH+1];    
	generateTransactionId(token, machine->id);
	//kdm_trace("token: %s\n",token);
	store_file* store_token;
	//kdm_trace("message->payload: %s\n",message->payload);
	//store the token in the key and the value waiting in the value field.
	store_token = create_store_file(token, message->payload, atoi(num_bytes) );
	machine->token_sent = insert_to_tail_file(machine->token_sent, store_token);

	// Send find values request to nearest nodes and increment count
	//store_token->count = 0;
	
    //find the nearest nodes.
    //store the query GET into "store_find_queries".
    nearest_nodes = k_nearest_nodes(nearest_nodes, &machine->routes, machine->id, rpc_value);
    store_file_temp = create_store_file(rpc_value, NULL, 0);
    store_file_temp->value = (char * )nearest_nodes;
    store_file_temp->value_len = sizeof(node_details);
    kdm_trace("Test\n");
    kdm_trace("%s\n",((node_details *)store_file_temp->value)->nodeID);
	machine->store_find_queries = insert_to_tail_file(machine->store_find_queries, store_file_temp);
    store_file_temp->count = 0;
	while (nearest_nodes != NULL && store_file_temp->count < KADEM_ALPHA){
		kademFindValue(machine, rpc_value, token, nearest_nodes->ip, nearest_nodes->port);
		store_file_temp->count++;
		nearest_nodes = nearest_nodes->next;
	}
	json_object_put(header);
	json_object_put(argument);
	kdm_trace("<<<<<<<<<<<<< RPCHandleStoreValue\n\n");
	return 0;
}

//NO Segmentation fault.
int RPCHandlePing(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){


	kdm_trace(">>>> RPCHandlePing\n");
	// Extract value from KademMessage
	char* hash_value;	
	hash_value = json_object_get_string(json_object_object_get(json_object_object_get(message->header,"a"),"value"));
	kdm_trace("hash_value: %s\n",hash_value);
	// Use hash value and MachineID to find the right node_details using find_node_details and look_for_id functions
	int bucket_val;
	node_details* bucket;
	node_details* Kbucket;
	char* _ip;
	int _port;

	bucket_val = find_node_details(machine->id, hash_value);
	Kbucket = machine->routes.table[bucket_val];
	bucket = look_for_IP(Kbucket, hash_value);		
	if(bucket!=NULL){
		// Extract IP/Port from node_details
		_ip = bucket->ip;
		_port = bucket->port;

		// Send Kadem_ping to the right node
		kademPing(machine, _ip, _port);	
	}
	else{
		//Send a Find Node
		//find node dans Store find queries.  (hash: key / value: k_nearest_nodes en *)
		//RPC message last: changer.

		//k_nearest_nodes
		node_details* nearest_nodes=NULL;
		nearest_nodes = k_nearest_nodes(nearest_nodes, &machine->routes, machine->id, hash_value);
		kdm_trace("nearest_nodes: %s\n",nearest_nodes);

		//store into store_find_queries
		store_file* file;
		if(nearest_nodes==NULL){
			file = create_store_file(hash_value, NULL, 0);
		}else{
			file = create_store_file(hash_value, nearest_nodes, sizeof(node_details));
			machine->store_find_queries = insert_to_tail_file(machine->store_find_queries, file);
		}
		//Store into latest_query_rpc
		strcpy(machine->latest_query_rpc.query,KADEM_PING);
		strcpy(machine->latest_query_rpc.value,hash_value);
		strcpy(machine->latest_query_rpc.ip,addr);
		machine->latest_query_rpc.port = port;

		//Send the find Node
		file->count = 0;
		while (nearest_nodes != NULL){
			kdm_trace("<<<< kademFindNode\n");
			kdm_trace("nearest_nodes->ip: %s\n",nearest_nodes->ip);
			kademFindNode(machine, hash_value, nearest_nodes->ip, nearest_nodes->port);
			file->count++;
			nearest_nodes = nearest_nodes -> next;
		}

	}

	kdm_trace("<<<< RPCHandlePing\n");
	return 0;
}

//To test with messaging.
int RPCHandlePingRspn(struct kademMachine * machine, int answ, char *addr, int port){

	kdm_trace(">>>> RPCHandlePingRspn\n");
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
	kdm_trace("<<<< RPCHandlePingRspn\n");
	return 0;
}

//To test with messaging
int RPCHandlePrintRoutingTable(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){

	kdm_trace(">>>> RPCHandlePrintRoutingTable\n");
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

	kdm_trace("<<<< RPCHandlePrintRoutingTable\n");
	return 0;
}

//To test with messaging
int RPCHandlePrintObjects(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){

	kdm_trace(">>>> RPCHandlePrintObjects\n");
	//TOTEST: print objects	
	int check = -1;
	check = print_values(machine->stored_values);

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
	kdm_trace("<<<< RPCHandlePrintObjects\n");
	return 0;
}

//NO segmentation Fault.
int RPCHandleFindValue(struct kademMachine * machine, struct kademMessage * message, char * addr, int port)
{

	kdm_trace(">>>> RPCHandleFindValue\n");
	//TOTEST: look for the appropriate IP address and port into the store_files.
	char* value;
	const char* temp, *temp1, *temp2, *header2;
	char ok[] = "OK", leng_payload[5], token[HASH_STRING_LENGTH+1];
	struct kademMessage answer_message;
	json_object *header, *argument, *argument2, *argument3;
	store_file * result;
	node_details* nearest_nodes=NULL;
	store_file* store_file_temp;


	generateTransactionId(token,machine->id);
	//look for value into the header of message.
	argument2 = json_object_object_get(message->header,"a");
	temp = json_object_get_string(json_object_object_get(argument2,"value"));

	result = find_key(machine->stored_values, temp);

	if (result == NULL) // Send request to the k-nodes the nearest from the key
	{
		kdm_trace("Value not found\n");
		// Store the query in the machine last_query field
		argument3 = json_object_object_get(message->header,"a");
		temp1 = json_object_get_string(json_object_object_get(argument3,"value"));
		temp2 = json_object_get_string(json_object_object_get(message->header,"q"));

		strcpy(machine->latest_query_rpc.query, temp2);
		strcpy(machine->latest_query_rpc.value, temp1);		
		strcpy(machine->latest_query_rpc.ip, addr);
		machine->latest_query_rpc.port = port;

		// Search the nearest nodes and store it in store_find_queries machine's field
		nearest_nodes = k_nearest_nodes(nearest_nodes, &machine->routes, machine->id, temp);
		kdm_trace("nearest_nodes: %s\n",nearest_nodes);
		if(nearest_nodes != NULL){
			store_file_temp = create_store_file(temp, nearest_nodes, sizeof(struct store_file));
			machine->store_find_queries = insert_to_tail_file(machine->store_find_queries, store_file_temp);
			// Send find values request to nearest nodes and increment count
			store_file_temp->count = 0;
			while (nearest_nodes != NULL)
			{
				kademFindValue(machine, temp, token, nearest_nodes->ip, nearest_nodes->port);
				store_file_temp->count++;
				nearest_nodes = nearest_nodes -> next;
			}

		}
	}
	else
	{
        kdm_trace(">>>> Found value %s\n", temp);
		value = result->value;
		//create the payload: "ip_address/port":
		sprintf(leng_payload,"%d",result->value_len);

		header = json_object_new_object(); 

		json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

		argument = json_object_new_object();
		json_object_object_add(argument,"resp",json_object_new_string(ok));
		json_object_object_add(argument,"numbytes",json_object_new_string(leng_payload));
		json_object_object_add(header,"r",json_object_get(argument));

		answer_message.header = header;
		strcpy(answer_message.payload,value);
		kdm_trace("message.payload: %s\n",answer_message.payload);
		answer_message.payloadLength = result->value_len;

		header2 = json_object_to_json_string(answer_message.header);

		kdm_trace("message: %s\n",header2);

		if(kademSendMessage(machine->sock_local_rpc, &answer_message, addr, port)<0)
		{
			return -1;
		}
		json_object_put(header);
		json_object_put(argument);

		kdm_trace("<<<< RPCHandleFindValue\n");

	}
	return 0;
}


int RPCHandleKillNode(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){

	kdm_trace(">>>> RPCHandleKillNode\n");
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
	kdm_trace("<<<< RPCHandleKillNode\n");
	exit(0);
}


int RPCHandleFindNode(struct kademMachine * machine, struct kademMessage * message, char *addr, int port){


	kdm_trace(">>>> RPCHandleFindNode\n");
	//look for nodeID into the header of message
	int bucket_no;
	const char *temp ;
	char ok[] = "OK";
	json_object *argument2, *header, *argument;
	struct kademMessage answer_message;
	char ip_port[20];
	argument2 = json_object_object_get(message->header,"a");
	temp = json_object_get_string(json_object_object_get(argument2,"value"));

	//Look where the node might be in store_file.
	bucket_no = find_node_details(machine->id, temp);
	node_details* find;
	find = look_for_IP(machine->routes.table[bucket_no], temp);

	if (find != NULL){
		kdm_trace("Find node: %s/%s/%d\n",find->nodeID,find->ip,find->port);
		concatenate2(find, ip_port);

		header = json_object_new_object(); 

		json_object_object_add(header, "y",json_object_new_string(KADEM_ANSWER));

		argument = json_object_new_object();
		json_object_object_add(argument,"resp",json_object_new_string(ok));
		json_object_object_add(argument,"nodes",json_object_new_string(ip_port));
		json_object_object_add(header,"r",json_object_get(argument));

		answer_message.header = header;
		answer_message.payloadLength = 0;
		kdm_trace("Message created\n");

		if(kademSendMessage(machine->sock_local_rpc, &answer_message, addr, port)<0){
			kdm_trace("<<<< RPCHandleFindNode\n");
			return -1;
		}
		kdm_trace("<<<< RPCHandleFindNode\n");
		return 0;
	}

	if (find == NULL){
		kdm_trace("Node not found, looking for nearest...\n");

		// Store the query in the machine last_query field
		json_object *argument3;
		const char *query_value, *query_type;
		node_details* nearest_nodes = NULL;
		store_file* store_file_temp;

		query_type = json_object_get_string(json_object_object_get(message->header,KADEM_QUERY));
		argument3 = json_object_object_get(message->header,"a");
		query_value = json_object_get_string(json_object_object_get(argument3,"value"));

		strcpy(machine->latest_query_rpc.query, query_type);
		strcpy(machine->latest_query_rpc.value, query_value);		
		strcpy(machine->latest_query_rpc.ip, addr);
		machine->latest_query_rpc.port = port;

		kdm_trace("Stored rpc query : type %s, value %s, addr %s, port %d\n", query_type, query_value, addr, port);

		// Search the nearest nodes and store it in store_find_queries machine's field

		nearest_nodes = k_nearest_nodes(nearest_nodes, &machine->routes, machine->id, query_value);
		if(nearest_nodes != NULL){
			kdm_trace("Find node: %s/%s/%d\n",nearest_nodes->nodeID,nearest_nodes->ip,nearest_nodes->port);
			store_file_temp = create_store_file(temp, nearest_nodes, sizeof(node_details));
			kdm_trace("SIZE %d\nCAST NODE ID : %s\n",sizeof(node_details), ((node_details *)store_file_temp->value)->nodeID);
			machine->store_find_queries = insert_to_tail_file(machine->store_find_queries, store_file_temp);

			kdm_trace("Stored find query\n");

			// Send find values request to nearest nodes and increment count
			store_file_temp->count = 0;
			int i=0;
			while (nearest_nodes != NULL && i< KADEM_ALPHA)
			{
				kademFindNode(machine, temp, nearest_nodes->ip, nearest_nodes->port);
				i++;
				store_file_temp->count++;
				nearest_nodes->count = 1;
				nearest_nodes = nearest_nodes -> next;
			}
		} else {
			kdm_trace("No nodes\n");
		}
		kdm_trace("<<<< RPCHandleFindNode\n");
		return -2;

	}
	kdm_trace("<<<< RPCHandleFindNode\n");
	return 0; 
}

