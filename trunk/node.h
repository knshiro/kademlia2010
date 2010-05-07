#include <sys/select.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>


//node_details structure
typedef struct node_details node_details;
struct node_details
{
    	char* ip;
	int port;
	int nodeID;
   	node_details* next;
};

// a k_bucket here is the pointer to the head of the k_bucket. 
typedef node_details* k_bucket;


//Declaration of functions
k_bucket insert_to_tail(k_bucket bucket, node_details* node);
node_details* create_node_details(node_details* node, char* _ip, int _port, int _nodeID);
void print_ports(k_bucket bucket);
k_bucket delete_head(k_bucket bucket);
int count_nodes_details(k_bucket bucket);
k_bucket delete_head_insert_tail(k_bucket bucket, node_details* node);
node_details* look_for_IP(k_bucket bucket, int nodeID);
k_bucket move_to_tail(k_bucket bucket, node_details* node);
k_bucket move_node_details(k_bucket bucket, int nodeID, char* ip, int port);


