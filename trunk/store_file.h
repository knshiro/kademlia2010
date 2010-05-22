#include <sys/select.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>


//node_details structure
typedef struct store_file store_file;
struct store_file
{
    	char* key;
	char* value;
	int timestamp;
   	store_file* next;
};

// the pointer to the head of the store_file gives the stored_values. 
typedef store_file* stored_values;


//Declaration of functions
store_file* create_store_file(store_file* file, char* _key, char* _value, int _timestamp);
stored_values insert_to_tail_file(stored_values stored_values, store_file* store_file);
stored_values delete_head_file(stored_values stored_values);
stored_values clean(stored_values values, int age);
store_file* find_key(stored_values values, char* key);
