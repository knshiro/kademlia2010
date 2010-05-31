#include "kademlia.h"
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <ctype.h>
#include <unistd.h>

//extern _kdm_debug;

int main(int argc, char *argv[]){
    int c;
    opterr = 0;	
    struct kademMachine machine;
    char *dhtnode;
    int dhtport, localrpcport; 
    _kdm_trace = 1;
    
    while ((c = getopt (argc, argv, "d")) != -1) {
        switch (c)
        {
            case 'd':
                _kdm_debug = 1;
                break;
            /*case 'w':
                wvalue = optarg; 
                break;
            case 'l':
                lvalue = optarg;
                break;
               */ 
            case '?':
                /*if (optopt == 'w' || optopt == 'l')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else*/ 
                if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,
                            "Unknown option character `\\x%x'.\n",
                            optopt);
                return 1;
                
            default:
                abort ();
        }
    }
    if(argc - optind< 3) {
        fprintf(stderr,"Usage %s [-d] <dhtport> <localrpcport> <dhtnode>\n", argv[0]); 
        return 1;
    } else {
        dhtport = atoi(argv[optind]);
	kdm_debug("inside main: %i\n",dhtport);
        localrpcport = atoi(argv[optind+1]);
        dhtnode = argv[optind+2];
    }
    
    initMachine(&machine,localrpcport,dhtport,dhtnode);
    startKademlia(&machine);
    return 0;
}
