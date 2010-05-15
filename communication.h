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

/* Some sizes */
#define KADEM_MAX_PAYLOAD_SIZE  4096
#define KADEM_MAX_SEND_BUF_SIZE 8


/* Message types */
extern const char * const KADEM_QUERY;   
extern const char * const KADEM_ANSWER; 

/*Message query types */
extern const char * const KADEM_PING;
extern const char * const KADEM_STORE;
extern const char * const KADEM_FIND_NODE;
extern const char * const KADEM_FIND_VALUE;

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
    char id[160];
    struct kademMessage messageBuffer[KADEM_MAX_SEND_BUF_SIZE];
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
 * @return int  0   success
 *              -1  failure
*/
int kademSendMessage(int sockfd, struct kademMessage * message, char * addr, int port);


struct kademMessage kademUdpToMessage(char * udpPacket, int length);


/**
 * @return int  0   success
 *              -1  failure
*/
int kademPing(struct kademMachine * machine, char * addr, int port);

/**
 * @return int  0   success
 *              -1  failure
*/
int kademPong(struct kademMachine machine, struct kademMessage *message, char * addr, int port);

/**
 * @return int  0   success
 *              -1  failure
*/
int kademHandlePong(char * addr, int port);




/**
 * Send a FIND_NODE request 
 * @param string key : key to look for
 *
 * @return int  0   success
 *              -1  failure
*/
int kademFindNode(struct kademMachine * machine, char * Id);

/**
 * Called by the message listener and handle a FIND_NODE request
 *
 * @return int  0   success
 *              -1  failure
*/
int kademHandleFindNode(struct kademMachine * machine, struct kademMessage * message);

/**
 * Called by the message listener and handle a FIND_NODE answer
 *
 * @return int  0   success
 *              -1  failure
*/
int kademHandleAnswerFindNode(struct kademMachine * machine, struct kademMessage * message);


/**
 * Send a FIND_VALUE request 
 * @param string key : key to look for
 *
 * @return int  0   success
 *              -1  failure
*/
int kademFindValue(struct kademMachine * machine, char * Id);

/**
 * Called by the message listener and handle a FIND_VALUE request
 *
 * @return int  0   success
 *              -1  failure
*/
int kademHandleFindValue(struct kademMachine * machine, struct kademMessage * message);

/**
 * Called by the message listener and handle a FIND_VALUE answer
 *
 * @return int  0   success
 *              -1  failure
*/
int kademHandleAnswerFindValue(struct kademMachine * machine, struct kademMessage * message);


#endif

