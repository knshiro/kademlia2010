#include <sys/select.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "store_file.h"

int main(void)
{
	struct store_file* file;
	struct store_file* file2;
	
	/*char* _key = malloc(400*sizeof(char));
	char* _value = malloc(400*sizeof(char));*/
	char* _key = "100";
	char* _value = "bonjour";
	int _timestamp = 1;
	
	/*char* _key2 = malloc(400*sizeof(char));
	char* _value2 = malloc(400*sizeof(char));*/
	char* _key2 = "200";
	char* _value2 = "bonjour2";
	int _timestamp2 = 2;	

	file = malloc(sizeof(store_file));
	file2 = malloc(sizeof(store_file));
	create_store_file(file, _key, _value, _timestamp);
	create_store_file(file2, _key2, _value2, _timestamp2);
	printf("key: %s\n value: %s\n timestamp: %d\n", file->key, file->value, file->timestamp);
	
	stored_values values = malloc(sizeof(stored_values));
	values = insert_to_tail_file(file, file2);
	printf("key2: %s\n value2: %s\n timestamp2: %d\n", values->next->key, values->next->value, values->next->timestamp);
	
	printf("ok\n");
	store_file* find_value = malloc(sizeof(store_file));
	char* _find_key = "200";
	find_value = find_key(values, _find_key);
	printf("cherche key 200: %s\n", find_value->key);

	values = delete_head_file(values);
	printf("value: %s\n",values->value);

	
	
	return 0;
}
