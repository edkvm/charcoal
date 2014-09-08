#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <aio.h>
#include "../include/charcoal_msg.h"
#include "../include/charcoal_client.h"
#include "../include/charcoal_logger.h"

// Forward declaration
typedef struct server_t server_t;
typedef struct client_t client_t;
server_t * server_ref_new(char *hostname, int port_num);
int client_run_loop(server_t *server);

// Client Object
struct charcoal_client_t{
    server_t *server;
};

// Client API
charcoal_client_t * charcoal_client_new(){
    
    charcoal_client_t *self = malloc(sizeof(charcoal_client_t));
    
    self->server = NULL;
    
    return self;
}

void charcoal_client_destroy(charcoal_client_t *self){
    
    if(self->server != NULL){
        free(self->server);
    }
}

int charcoal_client_connect(charcoal_client_t *self, char *hostname, int port){
    
    // Get server ctx
    self->server = server_ref_new(hostname, port);
    
    // Run loop
    client_run_loop(self->server);
    
    return 0;
    
}


// Server state machine states ebum
typedef enum {
    START = 1,
    HANDSHAKE = 2,
    READY = 3,
    WORKING = 4
} state_t;

// Server events enum
typedef enum {
    connected_event = 1,
    syn_ack_event = 2,
    heartbeat_event = 3,
    text_message_event = 4,
    chuncked_message_event = 5
} event_t;

// Client context
struct client_t{
    int fd;
};

// Server context
struct server_t{
    int  fd;
    struct sockaddr_in* address;
    state_t state;
    event_t next_event;
    event_t event;
    message_t *reply;
    int running;
};


// Helper function for reading input
static char _readline_input[2048];

char* helper_readline(char* prompt){
    fputs(prompt, stdout);
    fgets(_readline_input, 2048, stdin);
    char *cpy = malloc(strlen(_readline_input)+1);
    strcpy(cpy, _readline_input);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}




// 'Private' functions
server_t * server_ref_new(char *hostname, int port_num){
    
    server_t* server = malloc(sizeof(server_t));
    struct hostent *host;

        
    // Call to create a socket and get a file descriptor
    // AF_INET -> IPV4 
    // SOCK_STREAM -> reliable, two way 
    server->fd = socket(AF_INET, SOCK_STREAM, 0);
  
    if (server->fd < 0) 
    {
        logger_log(LOG_ERROR, "[charcoal client] failed to open socket");
        server = NULL;
    } else {
        
        // Initialize socket address structure 
        server->address = malloc(sizeof(struct sockaddr_in));

        server->address->sin_family = AF_INET;
        
        // Get All ip interfaces of the host machine
        host = gethostbyname(hostname);
        if (host == NULL) {
            logger_log(LOG_ERROR, "[charcoal client] no such host");
        }

        // Copy the address value to socket_in struct
        bcopy((char *)host->h_addr, 
           (char *)&server->address->sin_addr.s_addr,
                host->h_length);
        
        // Assign port
        server->address->sin_port = htons(port_num);

        // Server state
        server->state = START;
        logger_log(LOG_DEBUG,"[charcoal client] socket created");

        
    }

    return server;
}

// Connect to server using the server context
int server_connect(server_t *server){
    int result = 1;
    
    // Connect to server
    if (connect(server->fd, (struct sockaddr *) server->address,sizeof(struct sockaddr_in)) < 0) 
    {
         logger_log(LOG_ERROR, "[charcoal client] failed to connect, %s",strerror(errno));
         result = 0;
    }
  
    return result;
}

void client_server_fsm(server_t *server, event_t event){
    
    message_t *msg;
    server->next_event = event;

    logger_log(LOG_DEBUG,"[charcoal server] state %x clinet event %x",
            server->state, server->event);
    while (server->next_event) {
        server->event = server->next_event;
        server->next_event = (event_t)0;
        
        logger_log(LOG_DEBUG,"[charcoal server] state %d clinet event %d",
                   server->state, server->event);

        switch(server->state){
            case START:
                if(server->event == connected_event){
                    msg = message_new(CHARC_MSG_SYN);
                    message_send(msg, server->fd);  
                    server->state = HANDSHAKE;        
                }
                
                break;
            case HANDSHAKE:
                if(server->event == syn_ack_event){
                    msg = message_new(CHARC_MSG_ACK);
                    message_send(msg, server->fd);
                    server->running = 1;
                    server->state = READY;   
                }  

                break;
            case READY:
                if(server->event == heartbeat_event){
                    msg = message_new(CHARC_MSG_HERATBEAT_ACK);
                    message_send(msg, server->fd);         
                } else if(server->event == text_message_event){
                    
                    if(message_has_more_chunks(server->reply)){
                        server->next_event = chuncked_message_event;
                        server->state = WORKING;
                    }
                }
                
                break;
            case WORKING:
                if(server->event == chuncked_message_event){
                    msg = message_recv(server->fd);
                    
                    // Combined chunk into main reply
                    message_combine_cunk(server->reply, msg);
                    
                    if(message_has_more_chunks(server->reply)){
                        server->next_event = chuncked_message_event;
                    } else {
                        server->state = READY;
                    }
                }

        }
    }
    

}

void client_parse_user_input(server_t *server){
    message_t *msg;
    int not_parsed = 1;
    
    while(not_parsed){
        
        char *input = helper_readline("charcoal> ");
        char *argv[2];

        argv[0] = strtok(input, " ");
        argv[1] = strtok(NULL, " ");
        
        if (strcmp(argv[0],"ls") == 0) {
            not_parsed = 0;
            if(argv[1] != NULL){
                char *dir_name = argv[1];
                msg = message_new(CHARC_MSG_LIST_FILE);
                
                message_set_body(msg, dir_name, strlen(dir_name));
                message_send(msg, server->fd);
            } else {
                fprintf(stdout, "Error: you must specify a directory");
            }
            
        } else if(strcmp(argv[0], "cp")){
            // TODO: implament a copy command
            not_parsed = 0;
        } else{
            fputs("no such command",stdout);
            not_parsed = 0;
        }
    }
    
}



int client_run_loop(server_t *server){
    
    message_t *msg;
    
    if(server_connect(server)){

        client_server_fsm(server, connected_event);
        

        server->reply = message_recv(server->fd);
        
        if(message_get_type(server->reply) == CHARC_MSG_SYN_ACK){
            client_server_fsm(server, syn_ack_event);
            if(server->running){
                logger_log(LOG_INFO, "[charcoal client] connected to server");
            }
        }
        
    } else {
        logger_log(LOG_INFO, "[charcoal client] server refused connection");
        exit(1);
    }
    
    
    while(server->running){
        
        client_parse_user_input(server);
        
        server->reply = message_recv(server->fd);
        
        if (message_get_type(server->reply) == CHARC_MSG_HERATBEAT){
            client_server_fsm(server, heartbeat_event);
        } else if(message_get_type(server->reply) == CHARC_MSG_TEXT){
            client_server_fsm(server, text_message_event);
            char *text = message_get_body(server->reply);
            fprintf(stdout, "%s\n", text);
        }
        
    }
    
    return 0;
}



