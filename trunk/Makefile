JSON_DIR=json-c-0.9/
JSON_LIB=$(JSON_DIR)arraylist.o \
			 $(JSON_DIR)debug.o \
			 $(JSON_DIR)json_object.o \
			 $(JSON_DIR)json_tokener.o \
			 $(JSON_DIR)json_util.o \
			 $(JSON_DIR)linkhash.o \
			 $(JSON_DIR)printbuf.o


LIBS = ${JSON_LIB}
CC = gcc -g


all:   kademlia test_md5 test_store_file test_local_rpc messaging test

test :   test_kademlia.o kademlia.o md5.o store_file.o node.o XORmetrics.o
	${CC} -o $@ $^ ${LIBS}

kademlia:    main_kademlia.o kademlia.o md5.o store_file.o node.o XORmetrics.o
	${CC} -o $@ $^ ${LIBS}

messaging: messaging.o utils.o md5.o
	${CC} -o $@ $^ ${LIBS}

test_local_rpc: test_local_rpc.o utils.o md5.o
	${CC} -o $@ $^ ${LIBS}

test_store_file: test_store_file.o store_file.o 
	${CC} -o $@ $^ ${LIBS}

test_md5: md5_t.o md5.o
	${CC} -o $@ $^ 

clean:
	rm *.o kademlia test_md5 test_store_file test_local_rpc messaging test

%.o: %.c
	${CC} -o $@ -c $< 
