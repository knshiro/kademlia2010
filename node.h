#ifndef NODE_H
#define NODE_H

#include <sys/select.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
 
#define length_bucket      6
#define NUMBER_OF_BUCKETS  128

//node_details structure
typedef struct _node_details node_details;
struct _node_details
{
    	char* ip;
	int port;
	int count;
	char* nodeID;
	time_t timestamp;
   	node_details* next;
};

// a node_details * here is the pointer to the head of the node_details *. 
//typedef node_details* node_details *;

typedef struct _routing_table routing_table;
struct _routing_table{
	node_details *table[NUMBER_OF_BUCKETS];
};


//Declaration of functions
//@return: 0 node inserted 1 node rejected.
node_details * insert_to_tail(node_details * bucket, node_details* node);
node_details* create_node_details(node_details* node, char* _ip, int _port, char* _nodeID);
void print_nodes(node_details * bucket, int kbucket_no);
node_details * delete_head(node_details * bucket);
int count_nodes_details(node_details * bucket);
node_details * delete_head_insert_tail(node_details * bucket, node_details* node);
node_details* look_for_IP(node_details * bucket, char* nodeID);
node_details * move_to_tail(node_details * bucket, node_details* node);
node_details * move_node_details(node_details * bucket, char* nodeID, char* ip, int port);

int insert_into_contact_table(routing_table* table, char* this_nodeID, char* nodeID, char* ip, int port);
int find_node_details(char* this_node, char* other_node);

int print_routing_table(routing_table);
void free_k_bucket(node_details* k_bucket);
void free_node(node_details* node);
node_details* k_nearest_nodes(node_details* result, routing_table* routes, char* this_node, char* node_to_find);
//concatenate ip/port/nodeID
char* concatenate(node_details* node,char * result);
//concatenate ip/port
char* concatenate2(node_details* node, char * result);

node_details* create_node_from_string(char* concatenated);
node_details* delete_node(node_details* node, char *nodeID);

//return the node_details if it exists, NULL otherwise.
node_details* return_node(routing_table* table,char* this_nodeID, char* nodeID);


//Insert node_details in a ordered bucket according to the distance.
node_details* insert_acc_distance(node_details* bucket, node_details* node_to_insert, char* nodeID);


#endif
