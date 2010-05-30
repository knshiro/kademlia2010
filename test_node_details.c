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
	routing_table table;
    node_details * node_list = NULL;
    printf("***************** Start node test  *************\n");
	
	//Create the contact information table and 160 node_details *:
    
    for(i=0;i<NUMBER_OF_BUCKETS;i++){
        table.table[i] = NULL;
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
        /*TEST2:
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
	*/

	printf("***************** third series of test  *************\n");
	//int u = print_routing_table(table);

	printf("***************** fourth series of test  *************\n");
	// Remplissage k-bucket 1-2
	/*bucket_no = insert_into_contact_table(&table, "00000000000","00000000001", "127.0.0.1", 1);
	printf("bucket_no: %i\n",bucket_no);
	print_nodes(table.table[bucket_no],1);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000002", "127.0.0.1", 2);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000003", "127.0.0.1", 3);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000004", "127.0.0.1", 4);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000005", "127.0.0.1", 5);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000006", "127.0.0.1", 6);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000007", "127.0.0.1", 7);
	printf("bucket_no: %i\n",bucket_no);*/

	// Remplissage k-bucket 3-5-6
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000008", "127.0.0.1", 8);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000009", "127.0.0.1", 9);
	printf("bucket_no: %i\n",bucket_no);
	/*bucket_no = insert_into_contact_table(&table, "00000000000","0000000000A", "127.0.0.1", 10);
	printf("bucket_no: %i\n",bucket_no);	
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000000D", "127.0.0.1", 13);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000000E", "127.0.0.1", 14);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000000F", "127.0.0.1", 15);
	printf("bucket_no: %i\n",bucket_no);*/
	
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000020", "127.0.0.1", 32);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000002A", "127.0.0.1", 42);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000002E", "127.0.0.1", 46);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000034", "127.0.0.1", 52);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000039", "127.0.0.1", 57);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000003F", "127.0.0.1", 63);
	printf("bucket_no: %i\n",bucket_no);

	bucket_no = insert_into_contact_table(&table, "00000000000","00000000040", "127.0.0.1", 64);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000050", "127.0.0.1", 80);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000060", "127.0.0.1", 96);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000069", "127.0.0.1", 105);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000070", "127.0.0.1", 112);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000007F", "127.0.0.1", 127);
	printf("bucket_no: %i\n",bucket_no);

	// Semi-remplissage de 4-7 bucket
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000010", "127.0.0.1", 16);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000001B", "127.0.0.1", 27);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000014", "127.0.0.1", 20);
	printf("bucket_no: %i\n",bucket_no);

	bucket_no = insert_into_contact_table(&table, "00000000000","00000000080", "127.0.0.1", 128);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000093", "127.0.0.1", 147);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","000000000D2", "127.0.0.1", 210);
	printf("bucket_no: %i\n",bucket_no);
	int u = print_routing_table(table);	

	//look for a node into the bucket_7.
	node_details* result=NULL;
	result = k_nearest_nodes(result, &table, "00000000000", "00000000015");
	print_nodes(result,-1);
		
// C
// reate buckets
	//Create the contact information table and 160 node_details *:
    
	for(i=0;i<NUMBER_OF_BUCKETS;i++)
		{
			table.table[i]=NULL;
		}
	
	printf("\n***************** first series of test  *************\n");
	//TEST1: remplissage de k-buckets
	// Remplissage k-bucket 1-2
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000001", "127.0.0.1", 1);
	printf("bucket_no: %i\n",bucket_no);
	print_nodes(table.table[bucket_no],1);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000002", "127.0.0.1", 2);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000003", "127.0.0.1", 3);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000004", "127.0.0.1", 4);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000005", "127.0.0.1", 5);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000006", "127.0.0.1", 6);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000007", "127.0.0.1", 7);
	printf("bucket_no: %i\n",bucket_no);

	// Remplissage k-bucket 3-5-6
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000008", "127.0.0.1", 8);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000009", "127.0.0.1", 9);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000000A", "127.0.0.1", 10);
	printf("bucket_no: %i\n",bucket_no);	
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000000D", "127.0.0.1", 13);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000000E", "127.0.0.1", 14);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000000F", "127.0.0.1", 15);
	printf("bucket_no: %i\n",bucket_no);
	
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000020", "127.0.0.1", 32);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000002A", "127.0.0.1", 42);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000002E", "127.0.0.1", 46);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000034", "127.0.0.1", 52);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000039", "127.0.0.1", 57);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000003F", "127.0.0.1", 63);
	printf("bucket_no: %i\n",bucket_no);

	bucket_no = insert_into_contact_table(&table, "00000000000","00000000040", "127.0.0.1", 64);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000050", "127.0.0.1", 80);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000060", "127.0.0.1", 96);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000069", "127.0.0.1", 105);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000070", "127.0.0.1", 112);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000007F", "127.0.0.1", 127);
	printf("bucket_no: %i\n",bucket_no);

	// Semi-remplissage de 4-7 bucket
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000010", "127.0.0.1", 16);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","0000000001B", "127.0.0.1", 27);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000014", "127.0.0.1", 20);
	printf("bucket_no: %i\n",bucket_no);

	bucket_no = insert_into_contact_table(&table, "00000000000","00000000080", "127.0.0.1", 128);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","00000000093", "127.0.0.1", 147);
	printf("bucket_no: %i\n",bucket_no);
	bucket_no = insert_into_contact_table(&table, "00000000000","000000000D2", "127.0.0.1", 210);
	printf("bucket_no: %i\n",bucket_no);

	u = print_routing_table(table);

    node_list = k_nearest_nodes(node_list,&table,"00000000000","00000000041");


	return 0;
}




