
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/charcoal.h"




int main(int argc, char *argv[]){
    
        
    if(argc < 2){
        fprintf(stderr,"usage %s server port\n", argv[0]);
        fprintf(stderr,"usage %s client hostname port\n", argv[0]);
        exit(0);
    }
    
    if(strcmp(argv[1], "client")){
        if (argc < 4) {

            fprintf(stderr,"usage %s hostname port\n", argv[0]);
            exit(0);
        } else {
            charcoal_client_t *charcoal_client = charcoal_client_new();
            int port = atoi(argv[3]);
            
            // Will block here till the client exits
            charcoal_client_connect(charcoal_client, argv[2], port);
            
            // Destroy client object
            charcoal_client_destroy(charcoal_client);
        }

    } else if(strcmp(argv[1], "server")){
        if (argc < 3) {
            fprintf(stderr,"port nummber is missing");

            exit(0);
        }
    }
    
}