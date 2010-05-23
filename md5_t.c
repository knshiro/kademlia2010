

#include <stdio.h>
#include <stdlib.h>

#include "md5.h"


int	main(int argc, char **argv){
    char * test; 
    const char* buffer = "1244.33555.";
    char array[20]; 
    const unsigned int buf_len = strlen(buffer);
    char* signature= (char*)malloc(128*sizeof(char));
    char* str = (char*)malloc(35*sizeof(char));

    /* md5_buffer(buffer,buf_len,signature);
       md5_sig_to_string(signature, str, 33);
       */
    hash(buf_len, buffer, str, signature);

    printf("signature: %s (%d chars)\n", str, strlen(signature));
    if (signature[16] == '\0') {
        printf("c'est bien une string\n");
    }

    printf("Size test : %d\n",sizeof(test));
    printf("Size buffer : %d\n",sizeof(buffer));
    test = buffer;
    printf("Size test : %d\n\n",sizeof(test));
    printf("Size signature : %d\n",sizeof(signature));
    test = signature;
    printf("Size test : %d\n\n",sizeof(test));
    printf("Size array : %d\n",sizeof(array));
    test = array;
    printf("Size test : %d\n",sizeof(test));
    
}
