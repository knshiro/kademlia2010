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
#define KADEM_MAX_PAYLOAD_SIZE  4096
#define KADEM_MAX_SEND_BUF_SIZE 8
#define KADEM_TIMEOUT_REFRESH_DATA    5*60

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


/* Structure to store the message which was sent/received */

struct kademMessage {
    json_object *header;
    int payloadLength;    // Length of the payload and header
    char payload[KADEM_MAX_PAYLOAD_SIZE];    // Payload of packet
};


struct kademMachine {
    int sock_local_rpc;
    int sock_p2p;
    int port;
    char id[35];
    struct kademMessage messageBuffer[KADEM_MAX_SEND_BUF_SIZE];
    node_details routing_table[160];
    store_file * stored_values;
};


/**
 * Print if in debug mode
*/

void kdm_debug(const char *msg, ...);


/**
 * @return int  0   success
 *              -1  failure
 */
int initMachine(struct kademMachine * machine, int port_local_rpc, int port_p2p);

/**
 * 
 * @return int  0   success
 *              -1  failure
 */
int kademMaintenance(struct kademMachine * machine);

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
int kademSendError(struct kademMachine * machine, char *transactionId, char *code, char *message, char *addr, int port);


/*============================================
    P2P communication
  ============================================*/

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
int kademHandlePong(char * addr, int port);


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
int kademHandleAnswerFindNode(struct kademMachine * machine, struct kademMessage * message);


/**
 * Send a FIND_VALUE request 
 *
 * @param string key : key to look for 
 * @return int  0   success
 * -1  failure
 */
int kademFindValue(struct kademMachine * machine, char * value, char *dst_addr, int dst_port);

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
int kademHandleAnswerFindValue(struct kademMachine * machine, struct kademMessage * message);

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
