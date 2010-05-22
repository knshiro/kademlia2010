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
	bucket_no = insert_into_contact_table(&table, "ABE34AE3","12345665", "adress", 9876);
	printf("bucket_no: %i\n",bucket_no);
	print_ports(table.table[bucket_no]);
	bucket_no = insert_into_contact_table(&table, "1234683222222222","12346340E22ABC22", "adress", 2344);
	printf("bucket_no: %i\n",bucket_no);
	print_ports(table.table[bucket_no]);
	bucket_no = insert_into_contact_table(&table, "2323","FFFF", "adress", 12);
	printf("bucket_no: %i\n",bucket_no);
	print_ports(table.table[bucket_no]);
	bucket_no = insert_into_contact_table(&table, "ABE34AE3","12345665", "adress", 9876);
	printf("bucket_no: %i\n",bucket_no);
	print_ports(table.table[bucket_no]);
	
	

	printf("***************** second series of test  *************\n");
        //TEST2:
        table.table[1] = move_node_details(table.table[1], "22", "adress", 22);
        print_ports(table.table[1]);
	table.table[1] = move_node_details(table.table[1], "22", "adress", 22);
        print_ports(table.table[1]);
        table.table[1] = move_node_details(table.table[1], "12", "adress", 12);
        print_ports(table.table[1]);
        table.table[1] = move_node_details(table.table[1], "34", "adress", 34);
        print_ports(table.table[1]);
        table.table[1] = move_node_details(table.table[1], "12", "adress", 12);
        print_ports(table.table[1]);
        table.table[1] = move_node_details(table.table[1], "65", "adress", 65);
        print_ports(table.table[1]);
        table.table[1] = move_node_details(table.table[1], "67", "adress", 67);
        print_ports(table.table[1]);
        table.table[1] = move_node_details(table.table[1], "46", "adress", 46);
        print_ports(table.table[1]);
        table.table[1] = move_node_details(table.table[1], "68"	, "adress", 68);
      	print_ports(table.table[1]);
	

	return 0;
}




