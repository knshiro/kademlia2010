#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include "json-c-0.9/json.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "node.h"
#include "store_file.h"

/* max function*/
#ifndef max
    #define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif


/* Some sizes */
#define KADEM_MAX_PAYLOAD_SIZE          4096
#define KADEM_MAX_SEND_BUF_SIZE         8
#define KADEM_MAX_NB_TOKEN              50
#define KADEM_ALPHA                     2
#define HASH_STRING_LENGTH              32
#define HASH_SIGNATURE_LENGTH           16

/*Timeouts*/
#define KADEM_TIMEOUT_REFRESH_DATA      5*60
#define KADEM_TIMEOUT_REFRESH_ROUTE     5*60
#define KADEM_NB_TIMEOUT_REFRESH_ROUTE  2
#define KADEM_TIMEOUT_REFRESH_QUERY     2
#define KADEM_TIMEOUT_PING              2

/* Message types */
extern const char * const KADEM_QUERY;   
extern const char * const KADEM_ANSWER; 
extern const char * const KADEM_ERROR; 

/*Message query types */
extern const char * const KADEM_PING;
extern const char * const KADEM_STORE;
extern const char * const KADEM_FIND_NODE;
extern const char * const KADEM_FIND_VALUE;

/* Message error codes */
extern const char * const KADEM_ERROR_GENERIC;
extern const char * const KADEM_ERROR_INVALID_TOKEN;
extern const char * const KADEM_ERROR_PROTOCOL;
extern const char * const KADEM_ERROR_METHOD_UNKNOWN;
extern const char * const KADEM_ERROR_STORE;

/* Message error values */
extern const char * const KADEM_ERROR_GENERIC_VALUE;
extern const char * const KADEM_ERROR_INVALID_TOKEN_VALUE;
extern const char * const KADEM_ERROR_PROTOCOL_VALUE;
extern const char * const KADEM_ERROR_METHOD_UNKNOWN_VALUE;
extern const char * const KADEM_ERROR_STORE_VALUE;

/* Debug variable */
int _kdm_debug ;
int _kdm_trace;

/* Structure to store the message which was sent/received */

struct kademMessage {
    json_object *header;
    int payloadLength;    // Length of the payload and header
    char payload[KADEM_MAX_PAYLOAD_SIZE];    // Payload of packet
};

struct message_and_addr {
	char query[50];
	char value[50];
	char ip[16];
    	int port;
};

struct kademMachine {
    int sock_local_rpc;
    int sock_p2p;
    int port;
    char id[HASH_STRING_LENGTH+1];

    routing_table routes;
    store_file * stored_values;
// New fields for handleGet
	struct message_and_addr latest_query_rpc;
	store_file * store_find_queries;
	store_file * sent_queries;
	store_file * waiting_nodes;
//Fields for StoreValue
	//key=token/ value=NULL. Enable to check if the GET the DHT received has been issued by a STORE_VALUE.
	store_file* token_sent;	
	//store tokens from GET to check that the DHT received a GET before a PUT. key=token/ value=NULL
	store_file* token_rec;
};


/**
 * Print if in debug mode
*/

void kdm_debug(const char *msg, ...);


/**
 *
 * @param machine Struct kademMachine that host the machine in memory
 * @param port_local_rpc Number of the port to which the local rpc socket will be binded
 * @param port_p2p Number of the port to which the kademlia p2p socket will be binded
 * 
 * @return int  0   success
 *              -1  failure
 */
int initMachine(struct kademMachine * machine, int port_local_rpc, int port_p2p, char *peer_addr);

/**
 * 
 * @return int  0   success
 *              -1  failure
 */
int kademMaintenance(struct kademMachine * machine, struct kademMessage* message, char* addr, int port);

/**
 * Find a message by transaction id in the message buffer
 * @return int  0   success
 *              -1  failure
 */
struct kademMessage* findMessageByTransactionId(struct kademMachine * machine, char * transactionId);

/**
 * Functions which starts the service
 *
*/
int startKademlia(struct kademMachine * machine);



/*============================================
    Communication tools
  ============================================*/

/**
 * @return int  0   success
 *              -1  failure
 */
int kademSendMessage(int sockfd, struct kademMessage * message, char * addr, int port);

/**
 * Parses a udp packet to a kadmelia message 
 */
struct kademMessage kademUdpToMessage(char * udpPacket, int length);

/**
 * Sends an message containing an error
 * @return int  0   success
 *              -1  failure
 */
int kademSendError(struct kademMachine * machine, const char *transactionId, const char * const code, const char * const message, char *addr, int port);

/**
 * Generate a random transactionId and put it in the parameter
 *
 * @param   transactionId   string that will contain the transaction id.
 * @return  int  0   success
 *              -1  failure
 */
int generateTransactionId(char * transactionId, char *id);


/*============================================
    P2P communication
  ============================================*/

/**
 *  Operate one round of kademlia lookup algorithm
 *  @return int  0   Finished
 *               1   Not finished
 *               -1  failure
*/
int lookUpRound(struct kademMachine * machine, const char * token, store_file *find_query, json_object * response_content_nodes, const char* node_id, char * ip, int port);

/**
 *  sends a kadmelia ping request to addr, port
 *  @return int  0   success
 *               -1  failure
*/
int kademPing(struct kademMachine * machine, char * addr, int port);

/**
 *  Sends a kadmelia ping answer to addr, port
 * @return int  0   success
 *              -1  failure
*/
int kademPong(struct kademMachine *machine, struct kademMessage *message, char * addr, int port);

/**
 * @return int  0   success
 * -1  failure
 */
int kademHandlePong(struct kademMachine *machine, struct kademMessage *message, char *ip, int port);


/**
 * Send a FIND_NODE request 
 * @param string key : key to look for
 * 
 * @return int  0   success
 * -1  failure
 */
int kademFindNode(struct kademMachine * machine, char * id, char *dst_addr, int dst_port);

/**
 * Called by the message listener and handle a FIND_NODE request
 * 
 * @return int  0   success
 * -1  failure
 */
int kademHandleFindNode(struct kademMachine * machine, struct kademMessage * message,char *addr, int port);

/**
 * Called by the message listener and handle a FIND_NODE answer
 * 
 * @return int  0   success
 *              -1  failure
 */
int kademHandleAnswerFindNode(struct kademMachine * machine, struct kademMessage * message, char *ip, int port);


/**
 * Send a FIND_VALUE request 
 *
 * @param string key : key to look for 
 * @return int  0   success
 * -1  failure
 */
int kademFindValue(struct kademMachine * machine, char * value, char* token, char *dst_addr, int dst_port);

/**
 * Called by the message listener and handle a FIND_VALUE request
 *
 * @return int  0   success
 *              -1  failure
 */
int kademHandleFindValue(struct kademMachine * machine, struct kademMessage * message,char *addr, int port);

/**
 * Called by the message listener and handle a FIND_VALUE answer
 *
 * @return int  0   success
 *              -1  failure
 */
int kademHandleAnswerFindValue(struct kademMachine * machine, struct kademMessage * message, char *ip , int port);

/**
 * Send a STORE_VALUE request 
 *
 * @param string key : key to look for 
 * @return int  0   success
 * -1  failure
 */
int kademStoreValue(struct kademMachine * machine, char * token, char * value, char * data, int data_len, char *dst_addr, int dst_port);

/**
 * Called by the message listener and handle a STORE_VALUE request
 *
 * @return int  0   success
 *              -1  failure
 */
int kademHandleStoreValue(struct kademMachine * machine, struct kademMessage * message, char *addr, int port);

/**
 * Called by the message listener and handle a STORE_VALUE answer
 *
 * @return int  0   success
 *              -1  failure
 */
int kademHandleAnswerStoreValue(struct kademMachine * machine, struct kademMessage * message);
/**
 * Called by the HandleFindValue when it knows which node(s) the value has to be sent to.
 *
 * @return int  0   success
 *              -1  failure
 */
int kademSendStoreValue(struct kademMachine * machine, node_details* node_to_send, char* value, char* token);
/*============================================
    RPC communication
  ============================================*/

/**
 * Called by the message listener and handle a STORE_VALUE request
 *
 * @return int  0   success
 *              -1  failure
 */
int RPCHandleStoreValue(struct kademMachine * machine, struct kademMessage * message, char *addr, int port);

/**
 *
 * @return int  0   success
 *              -1  failure
 */
int RPCHandlePing(struct kademMachine * machine, struct kademMessage * message, char *addr, int port);


/**
 *
 * @return int  0   success
 *              -1  failure
 */
int RPCHandlePrintRoutingTable(struct kademMachine * machine, struct kademMessage * message, char *addr, int port);

/**
 *
 * @return int  0   success
 *              -1  failure
 */
int RPCHandlePrintObjects(struct kademMachine * machine, struct kademMessage * message, char *addr, int port);


/**
 *
 * @return int  0   success
 *              -1  failure
 */
int RPCHandleFindValue(struct kademMachine * machine, struct kademMessage * message, char * addr,int port);
/**
 *
 * @return int  0   value successfully found in local table and returned
 *              -1  failure
 *              -2  Value not found in local table and request sent to nearest nodes
 */

int RPCHandleFindValue(struct kademMachine * machine, struct kademMessage * message, char *addr, int port);

/**
 *
 * @return int  0   success
 *              -1  failure
 */
int RPCHandleKillNode(struct kademMachine * machine, struct kademMessage * message, char *addr, int port);

/**
 *
 * @return int  0   success
 *              -1  failure
 */
int RPCHandleFindNode(struct kademMachine * machine, struct kademMessage * message, char *addr, int port);


#endif
