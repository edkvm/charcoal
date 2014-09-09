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
server_t * server_ctx_new(char *hostname, int port_num);
int client_run(charcoal_client_t *self);

// Client Object
struct charcoal_client_t{
    server_t *server;
    int running;
    
};

// ------ Client API ---------

// Creates new client
charcoal_client_t * charcoal_client_new(){
    
    charcoal_client_t *self = malloc(sizeof(charcoal_client_t));
    
    self->server = NULL;
    
    return self;
}

// Releases memory
void charcoal_client_destroy(charcoal_client_t *self){
    
    if(self->server != NULL){
        free(self->server);
    }
}

// Connectes the client
int charcoal_client_connect(charcoal_client_t *self, char *hostname, int port){
    
    // Get server ctx
    self->server = server_ctx_new(hostname, port);
    
    // Run loop
    client_run(self);
    
    return 0;
    
}


// Server ctx states
typedef enum {
    START = 1,
    HANDSHAKE = 2,
    READY = 3,
    WORKING = 4
} state_t;

// Server events
typedef enum {
    connected_event = 1,
    syn_ack_event = 2,
    heartbeat_event = 3,
    text_message_event = 4,
    chuncked_message_event = 5
} event_t;

// Server context
struct server_t{
    int  fd;
    struct sockaddr_in* address;
    state_t state;
    event_t next_event;
    event_t event;
    message_t *reply;
    int connected;
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




// Creates new Server context with server port and host
server_t * server_ctx_new(char *hostname, int port_num){
    
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

// Process events that occur on the server ctx
void server_process_events(server_t *server, event_t event){
    
    message_t *msg;
    server->next_event = event;

    
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
                    server->state = READY;
                }  

                break;
            case READY:
                if(server->event == heartbeat_event){
                    msg = message_new(CHARC_MSG_HERATBEAT_ACK);
                    message_send(msg, server->fd);         
                } else if(server->event == text_message_event){
                    
                    // if message has more than one part
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
                    
                    // if message has more than one part
                    if(message_has_more_chunks(server->reply)){
                        server->next_event = chuncked_message_event;
                    } else {
                        server->state = READY;
                    }
                }

        }
    }
}

// Prase input from the client
void parse_user_input(server_t *server){
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
                
                message_set_body(msg, dir_name, (int)strlen(dir_name));
                message_send(msg, server->fd);
            } else {
                fprintf(stdout, "Error: you must specify a directory");
            }
            
        } else if(strcmp(argv[0], "cp")){
            // TODO: implament a copy command
            not_parsed = 0;
        } else{
            fputs("no such command\n",stdout);
            not_parsed = 0;
        }
    }
    
}


// Client execution
int client_run(charcoal_client_t *self){
    
    server_t *server = self->server;
    
    // Try connect to server
    if(server_connect(server)){
    
        // if connected, process connected event
        server_process_events(server, connected_event);
        
        // Client should run
        self->running = 1;
        
    } else {
        logger_log(LOG_INFO, "[charcoal client] server refused connection");
        exit(1);
    }
    
    
    while(self->running){
        
        server->reply = message_recv(server->fd);
        
        if (message_get_type(server->reply) == CHARC_MSG_HERATBEAT){
            server_process_events(server, heartbeat_event);
        } else if(message_get_type(server->reply) == CHARC_MSG_TEXT){
            server_process_events(server, text_message_event);
            char *text = message_get_body(server->reply);
            fprintf(stdout, "%s\n", text);
        } else if(message_get_type(server->reply) == CHARC_MSG_SYN_ACK){
            server_process_events(server, syn_ack_event);
            if(self->running){
                logger_log(LOG_INFO, "[charcoal client] connected to server");
            }
        }
        
        parse_user_input(server);
        
    }
    
    return 0;
}



