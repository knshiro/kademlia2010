#include <sys/select.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "store_file.h"


//Create a store_file struct
store_file* create_store_file(store_file* file, char* _key, char* _value, int _timestamp){

	

	file->key = _key;
	file->value = _value;
	file->timestamp = _timestamp;
	file->next = NULL;

	return file;
}


// Insert a store_file to the tail of the stored_values.
// Return the pointer to the head of the stored_values.
stored_values insert_to_tail_file(stored_values values, store_file* file)
{
	//Go through the stored_values to store the last store_file at the end of the list
	stored_values temp = values;

        while(temp->next != NULL)
	{
	      	temp = temp->next;
       	}

       	temp->next = file;

       	return values;
}


//Delete the head of the stored_values.
//Return the new head of the stored_values.
stored_values delete_head_file(stored_values values)
{
	if(values != NULL)
	{
        	// The new head of the stored_values is the second store_file in the list.
        	stored_values new_head = values->next;
        
        	free(values);
        	return new_head;
    	}

    	else
	{
        	return NULL;
    	}
}


//Delete the store_file older than "age" seconds
//Return the new head of the stored_values
stored_values clean(stored_values values, int age)
{
	stored_values temp = values;
	time_t timestamp;
	timestamp = time (NULL);

	while( (temp != NULL) || (temp->timestamp < timestamp - age) )
	{
		temp = delete_head_file(values);
       	}

	return temp;
}

store_file* find_key(stored_values values, char* key)
{
	stored_values temp = values;
	while(temp != NULL)
	{
		if(temp->key == key)
		{
			return temp;
		}		
		temp = temp->next;
	}
	return NULL;
}


