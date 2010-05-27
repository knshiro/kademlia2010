#include <sys/select.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "node.h"




int main(int argc, char **argv){

	
	
	//Create the contact information table and 160 node_details *:
	int i=0;
	routing_table table;
	for(i;i<159;i++){
		table.table[i]=NULL;
	}

	printf("***************** first series of test  *************\n");
	//TEST1:
	int bucket_no;
	bucket_no = insert_into_contact_table(&table, "ABE34AE3","12345665", "127.0.0.1", 9876);
	printf("bucket_no: %i\n",bucket_no);
	print_nodes(table.table[bucket_no],1);
	bucket_no = insert_into_contact_table(&table, "1234683222222222","12346340E22ABC22", "127.0.0.1", 2344);
	printf("bucket_no: %i\n",bucket_no);
	print_nodes(table.table[bucket_no],1);
	bucket_no = insert_into_contact_table(&table, "2323","FFFF", "127.0.0.1", 12);
	printf("bucket_no: %i\n",bucket_no);
	print_nodes(table.table[bucket_no],1);
	bucket_no = insert_into_contact_table(&table, "ABE34AE3","12345665", "127.0.0.1", 9876);
	printf("bucket_no: %i\n",bucket_no);
	print_nodes(table.table[bucket_no],1);
	
	

	printf("***************** second series of test  *************\n");
        //TEST2:
        table.table[1] = move_node_details(table.table[1], "22", "127.0.0.1", 22);
        print_nodes(table.table[1],1);
	table.table[1] = move_node_details(table.table[1], "22", "127.0.0.1", 22);
        print_nodes(table.table[1],1);
        table.table[1] = move_node_details(table.table[1], "12", "127.0.0.1", 12);
        print_nodes(table.table[1],1);
        table.table[1] = move_node_details(table.table[1], "34", "127.0.0.1", 34);
        print_nodes(table.table[1],1);
        table.table[1] = move_node_details(table.table[1], "12", "127.0.0.1", 12);
        print_nodes(table.table[1],1);
        table.table[1] = move_node_details(table.table[1], "65", "127.0.0.1", 65);
        print_nodes(table.table[1],1);
        table.table[1] = move_node_details(table.table[1], "67", "127.0.0.1", 67);
        print_nodes(table.table[1],1);
        table.table[1] = move_node_details(table.table[1], "46", "127.0.0.1", 46);
        print_nodes(table.table[1],1);
        table.table[1] = move_node_details(table.table[1], "68"	, "127.0.0.1", 68);
      	print_nodes(table.table[1],1);
	

	printf("***************** third series of test  *************\n");
	int u = print_routing_table(table);

	return 0;
}




