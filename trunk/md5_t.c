

#include <stdio.h>
#include <stdlib.h>

#include "md5.h"


int	main(int argc, char **argv){
 
const char* buffer = "1244.33555.";
const unsigned int buf_len = strlen(buffer);
char* signature= (char*)malloc(128*sizeof(char));
char* str = (char*)malloc(35*sizeof(char));

md5_buffer(buffer,buf_len,signature);
md5_sig_to_string(signature, str, 33);
printf("signature: %s\n", str);

}
