#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <arpa/inet.h>
#include "messaging.h"
#include <dirent.h>
#include <sys/wait.h>



/* create a socket and bind it to the local address and a port  */
int create_socket(int port){
  int sockfd;
  struct sockaddr_in client_addr;
  int length = sizeof(struct sockaddr_in);

  /*create socket*/
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
	  perror("ERROR opening socket");
  }
  printf("Socket created!\n");

  /* prepare attachment addrese = local addresse */
  client_addr.sin_family = AF_INET;

  /*  Give the local IP address and assign a port*/
  client_addr.sin_addr.s_addr=htonl(INADDR_ANY);
  

  if(port>-1)
  {
	client_addr.sin_port = htons(port);
  	if(bind(sockfd,(struct sockaddr *)&client_addr,length) == -1) {
 		perror("Impossible to bind socket to local address\n");
  		close(sockfd);
  		return -1;
 	}
  }

  return sockfd;
}


//////////////////////////////////////////////////////////



int main(int* argc, char argv[]){

struct rtlp_client_pcb cpcb;
int sockfd =  create_socket(1200);

printf("socket: %i\n",sockfd);
cpcb.sockfd = sockfd;


// current contact address:
struct sockaddr_in their_addr;
their_addr.sin_family = AF_INET;   
their_addr.sin_port = htons(4200); 
inet_aton("127.0.0.1", &their_addr.sin_addr);


int flags = 0;

unsigned int fromlen, numread;
fd_set readfds;
struct sockaddr_in from;
fromlen = sizeof(from);
char buf[400];
struct timeval tv;
tv.tv_sec = 0;
int srv;
		//stdin or socket!
		while(1){

			FD_ZERO(&readfds);
			FD_SET(cpcb.sockfd, &readfds);
			FD_SET(fileno(stdin), &readfds);

			//stdin or socket!
			srv = select(cpcb.sockfd+1, &readfds, NULL, NULL, &tv);
			//printf("srv: %i\n", srv);
			if (srv == -1) {
				perror("select"); // error occurred in select()
			} else {
				//Socket->printf
				if(FD_ISSET(cpcb.sockfd, &readfds)) {

					bzero(buf, sizeof(buf));
					if((numread = recvfrom(cpcb.sockfd,buf,sizeof(buf), 0, (struct sockaddr*)&from, &fromlen)) < 0){
						error("Couldnt' receive from socket");	
					}
				printf("%s",buf);
				}

				//stdin->send
				else if(FD_ISSET(fileno(stdin), &readfds)){
					fgets(buf, sizeof buf, stdin);
					//printf("buf: %s",buf);
					//trouver le pseudo et  l'extraire.
					/*
					char delims[] = " ";
					char* ID=(char*)malloc(10*sizeof(char));
					ID = strtok(buf,delims);
					printf("name: %s\n",ID);
					printf("buf: %s\n",buf);
					delims = ":";
					ID = strtok(ID,delims);
					printf("name2: %s\n",ID);
					*/
					if(sendto(sockfd, buf, sizeof(buf), flags, (struct sockaddr *)&their_addr, sizeof their_addr)<0){
						error("Couldnt' send");
					}
					//printf("Message sent\n");
				}
			}
		}

return 0;
}


