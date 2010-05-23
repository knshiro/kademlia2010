#include <sys/select.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "node.h"
#include "XORmetrics.h"
#include "md5.h"
#include <time.h>




// Insert a node_details to the tail of the bucket.
// Return the pointer to the head of the node_details * (i.e. a struct node_details *).
node_details * insert_to_tail(node_details * bucket, node_details* node){


	//Go through the node_details * to check if it is full
	node_details* temp = bucket;
	int i=1;

       	 while(temp != NULL){
           	temp = temp->next;
		i++;
        }

   	if(bucket == NULL){
        	//The node_details * is empty, so we only need to return.
		node->timestamp = time (NULL);
        	return node;
    	}
    	else if(i>6){
		//If it is already full, return bucket. How to notify it is full?
		printf("The node_details * is already full.\n");
		return bucket;
	}
   	else{
		//Go through the bucket until the end and add the new element there.
		node->timestamp = time (NULL);
        	temp = bucket;
        	while(temp->next != NULL){
           		temp = temp->next;
        	}
        	temp->next = node;
        	return bucket;
    	} 
}



//Delete the head of the bucket.
//Return the new head of the bucket.
node_details * delete_head(node_details * bucket){

	if(bucket != NULL){
        	// The new head of the bucket is the second node_details in the list.
        	node_details * new_head = bucket->next;
        
        	free(bucket);
        	return new_head;
    	}
    	else{
        	return NULL;
    	}
}



//Create a node_details struct
node_details* create_node_details(node_details* node, char* _ip, int _port, char* _nodeID){

	node = malloc(sizeof(node_details));

	node->ip = _ip;
	node->port = _port;
	node->nodeID = _nodeID;
	time_t _timestamp;
    	_timestamp = time (NULL);
	node->timestamp = _timestamp;
	node->next = NULL;

	return node;
}



//Print port of node_details in the node_details *
void print_ports(node_details * bucket){
    
	node_details* temp = NULL;
	temp = bucket;
	printf("nodeID in the k_bucket:\n");
    	while(temp != NULL){
        	printf("nodeID: %s\n", temp->nodeID);
        	temp = temp->next;
    	}
	printf("\n");
}


//Count the number of node_details in the node_details *. There have to be less than 6.
int count_nodes_details(node_details * bucket) {

	if(bucket == NULL){
		return 0;
 	}
  
    	return count_nodes_details(bucket->next)+1;
}



//Function that deletes the least recently seen node of the node_details * (the head) and add the new one (at the tail)
node_details * delete_head_insert_tail(node_details * bucket, node_details* node){
		
	bucket = delete_head(bucket);
	bucket = insert_to_tail(bucket, node);
	
	return bucket;
}


//Look for a nodeID in the bucket.
//Return a pointer to the node_details if found, NULL otherwise.
node_details* look_for_IP(node_details * bucket, char* nodeID){

    	node_details* temp = NULL;
	temp = bucket;
	
    	while(temp != NULL){
        	if(temp->nodeID == nodeID){
            		return temp;
        	}
        	temp = temp->next;
    	}

    	return NULL;
}


//Move an element from the list at the tail
node_details * move_to_tail(node_details * bucket, node_details* node){
	
	node_details* temp = bucket;
	if(bucket == node){
		bucket = bucket->next;
		node->next = NULL;
		bucket = insert_to_tail(bucket, node);
		
	}else{
		while(temp->next != node){
			temp = temp->next;
		}
		temp->next = node->next;
		node->next = NULL;
		bucket = insert_to_tail(bucket, node);
	}
	return bucket;
}



//Put the node_details at the right place in the node_details *
//after receiving a message from that node.
node_details * move_node_details(node_details * bucket, char* nodeID, char* ip, int port){
	
	node_details* temp = NULL;
	node_details * bucket2 = NULL;
	bucket2 = bucket;
	temp = look_for_IP(bucket, nodeID);
	
	//If the node already exists in that bucket -> move to the tail.
	if(temp !=NULL)	{
		bucket = move_to_tail(bucket, temp);
		return bucket;
	}
	//The node is not already in the node_details *
	else{
		int i = count_nodes_details(bucket);
		//if there are less than 6 node_details in the node_details *, insert it at the tail.
		if(i<6)	{
			temp = NULL;
			temp = create_node_details(temp, ip, port, nodeID);
			bucket = insert_to_tail(bucket, temp);
			return bucket;
		}
		//If the bucket is full, say it!
		else{
			return bucket2;
		}
	}
}



//look into the contact table (made of 160 node_details *s) the correct bucket for a value. take
int find_node_details(char* this_node, char* other_node){

	int res;
	char* distance = (char*)malloc(strlen(this_node)*sizeof(char));
	distance = XORmetrics (this_node, other_node);
	int length = strlen(distance);
	int i=0;
	while(distance[length-i]=='0'){
		i++;
	}
	char alpha = distance[length-i];
	int len = length-i;

	if(alpha=='1'){
		res = 4*len;
	}
	else if(alpha=='2' || alpha=='3'){
		res = 4*len+1;
	}
	else if(alpha=='4' || alpha=='5' || alpha=='6' || alpha=='7'){
		res = 4*len+2;
	}
	else{
		res = 4*len+3;
	}

	return res;
}


//insert the node into the right bucket. If inserted return the number of the bucket, -1 if the bucket is full, -2 if the node is too far.
int insert_into_contact_table(routing_table* table, char* this_nodeID, char* nodeID, char* ip, int port){
	
	int bucket_no,i;
	//insert the node into the k_bucket "bucket_no".
	bucket_no = find_node_details(this_nodeID,nodeID);
	if(bucket_no>159){
		return -2;
	}
	else{
		i = count_nodes_details(table->table[bucket_no]);
		if(i<6){
			table->table[bucket_no] = move_node_details(table->table[bucket_no], nodeID, ip, port);
			return bucket_no;
		}
		else{
			return -1;
		}
	}
	
}
 



