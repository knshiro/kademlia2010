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
	struct store_file* file1;
	struct store_file* file2;
	struct store_file* file3;
	struct store_file* file4;
	
	char* _key = "100";
	char* _value = "bonjour";
	
	char* _key2 = "200";
	char* _value2 = "bonjour2";

	char* _key3 = "300";
	char* _value3 = "bonjour3";
	char* _key4 = "400";
	char* _value4 = "bonjour4";

    file1 = NULL;
	file2 = create_store_file(_key2, _value2, 8);
    print_values(file1);
	file1 = insert_to_tail_file(file1, file2);
	
    file1 = create_store_file( _key, _value, 7);
	file2 = create_store_file(_key2, _value2, 8);
	printf("key: %s\n value: %s\n timestamp: %d\n", file1->key, file1->value, file1->timestamp);
	printf("key2: %s\n value2: %s\n timestamp2: %d\n", file2->key, file2->value, file2->timestamp);
	
	file1 = insert_to_tail_file(file1, file2);
	printf("key2: %s\n value2: %s\n timestamp2: %d\n", file1->next->key, file1->next->value, file1->next->timestamp);
    print_values(file1);
	
    store_file* find_value = malloc(sizeof(store_file));
	char* _find_key = "200";
	find_value = find_key(file1, _find_key);
	printf("cherche key %s: %s\n", _find_key, find_value->key);
    printFiles(file1);

	file1 = delete_head_file(file1);
	printf("value after deletion: %s\n",file1->value);

    printFiles(file1);
	file3 = create_store_file(_key3, _value3, 8);
	file1 = insert_to_tail_file(file1, file3);
	file4 = create_store_file(_key4, _value4, 8);
	file1 = insert_to_tail_file(file1, file4);
    
    printFiles(file1);
	/*int i = 3;
	time_t _timestamp;
	time_t _timestamp2;
	_timestamp = time (NULL);
	_timestamp2 = time (NULL)-i;
	
	printf("timestamp: %d\ntimestamp-3: %d\n", _timestamp, _timestamp2);
	*/
	printf("ok\n");
	sleep(2);
    
    file4 = create_store_file(_key4, _value4, 8);
	file1 = insert_to_tail_file(file1, file4);

    printFiles(file1);

	//file1 = clean(file1, 1);
	
    printFiles(file1);
    printf("value: %s\n", file1->value);

	print_values(file1);

	/* Test delete function
	while(1)
		{
			struct store_file* file1;
			char* _key = "100";
			char* _value = "bonjour";
			file1 = create_store_file( _key, _value);
			delete_store_file(file1);
		}
	*/	
	
	return 0;
}
