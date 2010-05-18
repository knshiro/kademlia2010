JSON_DIR=json-c-0.9/
JSON_LIB=$(JSON_DIR)arraylist.o \
			 $(JSON_DIR)debug.o \
			 $(JSON_DIR)json_object.o \
			 $(JSON_DIR)json_tokener.o \
			 $(JSON_DIR)json_util.o \
			 $(JSON_DIR)linkhash.o \
			 $(JSON_DIR)printbuf.o


LIBS = ${JSON_LIB}
CC = gcc -Wall -g


all:   kadmelia 

test :   test.o kademlia.o md5.o 
	${CC} -o $@ $^ ${LIBS}

kadmelia:    main_kademlia.o kademlia.o md5.o
	${CC} -o $@ $^ ${LIBS}

messaging: messaging.o utils.o md5.o
	${CC} -o $@ $^ ${LIBS}
clean:
	rm *.o kadmelia test

%.o: %.c
	${CC} -o $@ -c $< 
