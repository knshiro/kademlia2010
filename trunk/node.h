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


//node_details structure
typedef struct _node_details node_details;
struct _node_details
{
    	char* ip;
	int port;
	char* nodeID;
	time_t timestamp;
   	node_details* next;
};

// a node_details * here is the pointer to the head of the node_details *. 
//typedef node_details* node_details *;

typedef struct _routing_table routing_table;
struct _routing_table{
	node_details *table[160];
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



#endif
