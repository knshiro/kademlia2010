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
        	free(bucket->ip);
		free(bucket->nodeID);
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

	node->ip = malloc((strlen(_ip)+1)*sizeof(char));
	strcpy(node->ip,_ip);
	node->port = _port;	
	node->nodeID = malloc((strlen(_nodeID)+1)*sizeof(char));
	strcpy(node->nodeID,_nodeID);
	time_t _timestamp;
    	_timestamp = time (NULL);
	node->timestamp = _timestamp;
	node->next = NULL;

	return node;
}



//Print port of node_details in the node_details *
void print_nodes(node_details * bucket, int kbucket_no){
    
	node_details* temp = NULL;
	temp = bucket;
	printf("nodeID in the k_bucket %i:\n",kbucket_no);
    	while(temp != NULL){
        	printf("nodeID: %s, IP_address: %s, port: %i\n", temp->nodeID,temp->ip,temp->port);
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



//look into the contact table (made of 160 node_details *s) the correct bucket for a value.
int find_node_details(char* this_node, char* other_node){

	int res;
	char* distance = (char*)malloc(strlen(this_node)*sizeof(char));
	distance = XORmetrics (distance, this_node, other_node);
	printf("distance: %s\n", distance);
	int length = strlen(distance);
	int i=0;
	while(distance[i]=='0'){
		i++;
	}
	char alpha = distance[i];
	int len = length-1-i;

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
	
	printf("resultat: %i\n", res);
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
 
//Print the routing table.
int print_routing_table(routing_table r_table){

	int i=0;
	for(i;i<159;i++){
		print_nodes(r_table.table[i],i);
	}
	return 0;
}

//Take this hash and another hash and return the 6 nearest node_details into a chained list.
node_details* k_nearest_nodes(node_details* result, routing_table* routes, char* this_node, char* node_to_find){

	int bucket_no;
	int num_nodes_found=0;
	int bucket_minus=1;
	int go_back=1;

	//find the k_bucket where the node should be.
	bucket_no = find_node_details(this_node, node_to_find);

	//look for the node into the bucket
	node_details* find=NULL;
	find = look_for_IP(routes->table[bucket_no], node_to_find);

	//if the node is found
	if (find != NULL){
		result = find;
		return find;
	}
	else{
		node_details* look_into=NULL;
		look_into = routes->table[bucket_no];

		//temp= node_details inserted into the weird bucket.
		node_details* temp;
		
		while(num_nodes_found<6){
			if(look_into != NULL){
				//insert the ip/port/nodeID into temporary values.
				temp = create_node_details(temp, look_into->ip, look_into->port, look_into->nodeID);	
				result = insert_to_tail(result,temp);
				look_into = look_into->next;
				num_nodes_found++;
				printf("test\n");
				print_nodes(result,-1);
			}
			if(look_into == NULL){
				if(bucket_no - bucket_minus < 0 && go_back == 1){
					go_back = 0;
					bucket_minus = 1;
					look_into=routes->table[bucket_no+1];
				}
				if(go_back == 1){
					printf("bucket_no-bucket_minus: %i\n",bucket_no-bucket_minus);
					look_into = routes->table[bucket_no-bucket_minus];
					printf("test2\n");
					bucket_minus++;
				}else{
					printf("bucket_no-bucket_minus: %i\n",bucket_no+bucket_minus);
					look_into = routes->table[bucket_no + bucket_minus];
					bucket_minus++;
				}
			}
		}
	}
	return result;
}



