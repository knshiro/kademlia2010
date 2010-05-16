#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "XORmetrics.h"
#include "XORmetrics.c"

/* Main */
int main(int argc, char **argv){

char* hash1 = "AC1059B1";
char* hash2 = "01E15485";

char* result = XORmetrics(hash1,hash2);

	printf("%s\n", result);
	getch();
}
