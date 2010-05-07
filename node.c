#include <sys/select.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "node.h"






// Insert a node_details to the tail of the bucket.
// Return the pointer to the head of the k_bucket (i.e. a struct k_bucket).
k_bucket insert_to_tail(k_bucket bucket, node_details* node){


	//Go through the k_bucket to check if it is full
	node_details* temp = bucket;
	int i=1;

       	 while(temp != NULL){
           	temp = temp->next;
		i++;
        }

   	if(bucket == NULL){
        	//The k_bucket is empty, so we only need to return.
        	return node;
    	 }
    	else if(i>6){
		//If it is already full, return bucket. How to notify it is full?
		printf("The k_bucket is already full.\n");
		return bucket;
	}
   	else{
		//Go through the bucket until the end and add the new element there.
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
k_bucket delete_head(k_bucket bucket){

	if(bucket != NULL){
        	// The new head of the bucket is the second node_details in the list.
        	k_bucket new_head = bucket->next;
        
        	free(bucket);
        	return new_head;
    	}
    	else{
        	return NULL;
    	}
}



//Create a node_details struct
node_details* create_node_details(node_details* node, char* _ip, int _port, int _nodeID){

	node = malloc(sizeof(node_details));

	node->ip = _ip;
	node->port = _port;
	node->nodeID = _nodeID;
	node->next = NULL;

	return node;
}



//Print port of node_details in the k_bucket
void print_ports(k_bucket bucket){
    
	node_details* temp = NULL;
	temp = bucket;
	printf("nodeID in the k_bucket:\n");
    	while(temp != NULL){
        	printf("nodeID: %d\n", temp->nodeID);
        	temp = temp->next;
    	}
	printf("\n");
}


//Count the number of node_details in the k_bucket. There have to be less than 6.
int count_nodes_details(k_bucket bucket) {

	if(bucket == NULL){
		return 0;
 	}
  
    	return count_nodes_details(bucket->next)+1;
}



//Function that deletes the least recently seen node of the k_bucket (the head) and add the new one (at the tail)
k_bucket delete_head_insert_tail(k_bucket bucket, node_details* node){
	
	bucket = delete_head(bucket);
	bucket = insert_to_tail(bucket, node);
	
	return bucket;
}


//Look for a nodeID in the bucket.
//Return a pointer to the node_details if found, NULL otherwise.
node_details* look_for_IP(k_bucket bucket, int nodeID){

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
k_bucket move_to_tail(k_bucket bucket, node_details* node){
	
	node_details* temp = bucket;
	while(temp->next != node){
		temp = temp->next;
	}

	temp->next = node->next;
	node->next = NULL;
	bucket = insert_to_tail(bucket, node);
	
	return bucket;
}



//Put the node_details at the right place in the k_bucket
//after receiving a message from that node.
k_bucket move_node_details(k_bucket bucket, int nodeID, char* ip, int port){

	node_details* temp = NULL;

	temp = look_for_IP(bucket, nodeID);
	
	//If the node already exists in that bucket -> move to the tail.
	if(temp !=NULL)	{
		bucket = move_to_tail(bucket, temp);
		return bucket;
	}
	//The node is not already in the k_bucket
	else{
		int i = count_nodes_details(bucket);
		//if there are less than 6 node_details in the k_bucket, insert it at the tail.
		if(i<6)	{
			temp = NULL;
			temp = create_node_details(temp, ip, port, nodeID);
			bucket = insert_to_tail(bucket, temp);
			return bucket;
		}
		//If the bucket is full, ping the least recently seen to see if it is still here.
		else{
			int check_ping = 0;  // replace by a ping function -> return 1 if ping worked, 0 otherwise.
			//if the least-recently node, answered -> do nothing.
			if(check_ping == 1){
				return bucket;
			}
			else{
				bucket = delete_head(bucket);
				temp = NULL;
				temp = create_node_details(temp, ip, port, nodeID);
				bucket = insert_to_tail(bucket, temp);
				return bucket;
			}
		}

	}
}



