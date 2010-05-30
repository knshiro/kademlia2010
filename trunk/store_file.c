#include <sys/select.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "store_file.h"
#include <time.h>

//Create a store_file struct
store_file* create_store_file( char* _key, char* _value, int _value_len){
    time_t _timestamp;
    store_file* file =malloc(sizeof(store_file));

    file->key = malloc((strlen(_key)+1)*sizeof(char));
    strcpy(file->key, _key);
    
    if(_value_len!=0){
        file->value = malloc(_value_len*sizeof(char));
        memcpy(file->value, _value, _value_len);
    }
    else {
        file->value = NULL;
    }
    
    _timestamp = time (NULL);
    file->timestamp = _timestamp;
    file->next = NULL;

    return file;
}

//Delete a store file
void delete_store_file(store_file* store_file)
{
	free(store_file->key);
	free(store_file->value);
	free(store_file);
	//printf("delete ok\n");
}


/** Insert a store_file to the tail of the stored_values.
 * If 'value' is already in the list, delete the corresponding store_file and store it
 * at the tail of the list
 * Return the pointer to the head of the stored_values.
 */
stored_values insert_to_tail_file(stored_values values, store_file* file)
{
    //Go through the stored_values to store the last store_file at the end of the list
    if(file != NULL)
    {
        file->timestamp = time(NULL);
        stored_values temp = values;
    
        if(values != NULL)
        {
            values = delete_key(values, file->key);
            while(temp->next != NULL)
            {
                temp = temp->next;
            }
    
            temp->next = file;
    
            return values;
        }   
        else {
            values = file;
            return values;
        }
    }
    else 
    {
        return values;
    }
    
}
/*
//Go through the stored_values to store the last store_file at the end of the list
// Return the pointer to the head of the stored_values.
stored_values delete_key(stored_values values, store_file* file)
{
store_file* temp;
store_file* temp2;
temp = values;
temp2 = NULL;
printf("\nok\n");

if(values ->next == NULL)
{
printf("\nok2\n");
fflush(stdout);
return values;
}
else
{
printf("\nok3\n");
fflush(stdout);
if(values->next->next == NULL)
{
printf("\nNext next null\n");
temp2 = values;
values = temp->next;
free(temp2);
return values;
}
else
{
printf("\nok\n");
while(temp->next->next != NULL)
{	
if(strcmp(temp->next->value, file->value)==0)
{
temp2 = temp;
temp->next = temp->next->next;
free(temp2);
}
}
return values;
}
}
}
*/

/** Go through the stored_values to delete any store_file that has the same
 * value as file
 * @param file file containing the value to be deleted
 * @return the pointer to the head of the list cleaned.
 */
stored_values delete_key(stored_values values, char *key){

    store_file* temp;
    store_file* temp2=NULL;
    temp = values;

    if(strcmp(temp->key, key)==0){
	 temp2 = temp->next;
         delete_store_file(temp);
	 return temp2; 
    }

    if(values->next == NULL)
    {
        fflush(stdout);
    }
    else {
        while(temp->next != NULL)
        {
            temp2 = temp->next;
            if(strcmp(temp->next->key, key)==0)
            {
                temp->next = temp->next->next;
                delete_store_file(temp2);
            }
            temp = temp->next;
            if(temp == NULL){
                break;
            }
        }
    }
    return values;

}

/**Delete the head of the stored_values.
  *Return the new head of the stored_values.
  *stored_values delete_head_file(stored_values values)
 */
stored_values delete_head_file(stored_values values)
{
    stored_values new_head = NULL;
    if(values != NULL)
    {
        // The new head of the stored_values is the second store_file in the list.
        new_head = values->next;
        delete_store_file(values);
    }

    return new_head;
}


/**Delete the store_file older than "age" seconds
  *Return the new head of the stored_values
 */
stored_values clean(stored_values values, int age)
{
    time_t _timestamp;
    _timestamp = time (NULL)-age;

    while( (values != NULL) && (values->timestamp < _timestamp) )
    {
        values = delete_head_file(values);
    }

    return values;
}

store_file* find_key(stored_values values, char* key)
{
    stored_values temp = values;
    while(temp != NULL)
    {
        if(strcmp(temp->key, key)==0)
        {
            return temp;
        }		
        temp = temp->next;
    }
    return NULL;
}

void printFiles(stored_values values)
{
    stored_values temp = values;
    printf("Stored values :"); 
    while(temp != NULL)
    {
        printf("%s : %s -> ",temp->key,temp->value);
        temp = temp->next;
    }
    printf("NULL\n");
}

int print_values(stored_values values)
{
    printf("Printing stored values\n");
	stored_values temp = values;
	while (temp != NULL)
	{
		printf("Key: %s  ;  Value: %s\n", temp->key,temp->value);
		temp = temp->next;
	}
return 0;
}



