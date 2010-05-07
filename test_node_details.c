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

	
	int i;

	//Create the contact information table and 160 k_buckets:
	//I chose to implement the contact_table with a table and not a list chainée because elle est fixe (on ne supprime rien et rajoute rien). 
	k_bucket* contact_table = malloc(160*sizeof(k_bucket));
	for(i=0;i<160;i++){
		contact_table[i] = NULL;
	}


	//TEST:
	contact_table[3] = move_node_details(contact_table[3], 22, "adress", 22);
	print_ports(contact_table[3]);
	contact_table[3] = move_node_details(contact_table[3], 12, "adress", 12);
	print_ports(contact_table[3]);
	contact_table[3] = move_node_details(contact_table[3], 34, "adress", 34);
	print_ports(contact_table[3]);
	contact_table[3] = move_node_details(contact_table[3], 12, "adress", 12);
	print_ports(contact_table[3]);
	contact_table[3] = move_node_details(contact_table[3], 65, "adress", 65);
	print_ports(contact_table[3]);
	contact_table[3] = move_node_details(contact_table[3], 67, "adress", 67);
	print_ports(contact_table[3]);
	contact_table[3] = move_node_details(contact_table[3], 46, "adress", 46);
	print_ports(contact_table[3]);
	contact_table[3] = move_node_details(contact_table[3], 68, "adress", 68);
	print_ports(contact_table[3]);

	return 0;
}




