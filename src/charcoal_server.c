#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "../include/charcoal.h"
#include "../include/charcoal_msg.h"
#include "../include/charcoal_server.h"
#include "../include/charcoal_logger.h"


typedef struct server_t server_t;
typedef struct client_t client_t;

server_t * server_new(int port_num);
void server_destroy(server_t* s);
void server_run_loop(server_t *server);
void client_list_files_in_dir(client_t *client, char* dir_name);

struct charcoal_server_t{
    server_t *server;
    
};

charcoal_server_t * charcoal_server_new(){
    charcoal_server_t *self = malloc(sizeof(charcoal_server_t));
    
    self->server = NULL;
    
    return self;
}

void charcoal_server_destroy(charcoal_server_t *self){
    
    server_destroy(self->server);
    
    if(self != NULL){
        free(self);
    }
    
}

void charcoal_server_bind(charcoal_server_t *self,int port){
    
    fprintf(stdout, "charcoal v%s\n", CHARCOAL_VERSION);

    self->server = server_new(port);
    
    
    server_run_loop(self->server);
    
}

// Client state machine states
typedef enum { START, READY, WORKING } state_t;

// Client events
typedef enum { 
    terminate_event = -1,
    connected_event = 1,
    syn_event = 2,
    ack_event = 3,
    heartbeat_event = 4,
    list_files_event = 5,
    dispatching_event = 6
} event_t;

// Context to hold server socket
struct server_t{
    int  fd;
    struct sockaddr_in* address;
    state_t state;
    int running;
};

// Context to hold client connection
struct client_t{
    int  fd;
    state_t state;
    event_t next_event;
    event_t event;
    message_t *reply;
    message_t *request;
    long expires_at;
    int connected;
    
};


server_t* server_new(int port_num){
  
    server_t* server = malloc(sizeof(server_t));
  
    /* Call to create a socket and get a file descriptor
     * AF_INET -> IPV4 
     * SOCK_STREAM -> reliable, two way */  
    server->fd = socket(AF_INET, SOCK_STREAM, 0);
  
    if (server->fd < 0) 
    {
        logger_log(LOG_ERROR, "ERROR opening socket");
        server = NULL;
    } else {
      
      // Initialize socket address structure
      server->address = malloc(sizeof(struct sockaddr_in));

      server->address->sin_family = AF_INET;
      
      // a wildcard ip address of the host machine 
      server->address->sin_addr.s_addr = INADDR_ANY;
      server->address->sin_port = htons(port_num);

      logger_log(LOG_DEBUG,"[charcoal server] socket created");
    }

    return server;
}

void server_destroy(server_t* s){
  
    if(s->address != NULL){
        free(s->address);
    }

    if(s != NULL){
        free(s);
    }
}

int server_open(server_t* server){
    
    int result = 1;
    // Bind socket to and endpoint 
    if (bind(server->fd, (struct sockaddr *) server->address,
                        sizeof(struct sockaddr_in)) < 0) {
        logger_log(LOG_ERROR,"Failed to bind, %s", strerror(errno));
        result = 0;
    } else {
        logger_log(LOG_INFO,"[charcoal server] bind socket to localhost");
    }

    return result;
}


client_t* server_accept(server_t* server){
  
    int newsockfd;
    socklen_t clilen;
    struct sockaddr_in client_addr;
    client_t* client;
    
    // Listen for incoming connection 
    listen(server->fd,5);
    clilen = sizeof(client_addr);
  
    logger_log(LOG_INFO, "[charcoal server] listening on port, %d",
        ntohs(server->address->sin_port));

    // Accept connection, will block here
    newsockfd = accept(server->fd, (struct sockaddr *)&client_addr, 
                              &clilen);
    
    if (newsockfd < 0) {
        logger_log(LOG_ERROR, "Failed accept");
        client = NULL;
    } else {
        client = malloc(sizeof(client_t));
        client->fd = newsockfd;
        client->state = START;
        logger_log(LOG_INFO,"[charcoal server] client connected");
    }

    return client;
}

void server_close(server_t* server){
    
    close(server->fd);
}

void server_client_fsm(client_t* client, event_t event){
    
    message_t *msg;
    client->next_event = event;
    
    logger_log(LOG_DEBUG,"[charcoal server] state %d event %d",
            client->state, client->event);
    
    
    while (client->next_event) {
        client->event = client->next_event;
        // Clear event
        client->next_event = (event_t)0;
        switch(client->state){
            case START:
                logger_log(LOG_DEBUG, "[charcoal server] client at START state");
            
                if(client->event == connected_event){
                    client->connected = 1;
                } else if(client->event == syn_event){
                    // SYN acknowledge message to client and wait for response 
                    msg = message_new(CHARC_MSG_SYN_ACK);
                    message_send(msg, client->fd);
                } else if(client->event == ack_event) {
                    logger_log(LOG_DEBUG, "[charcoal server] successful handshake ");
                    client->state = READY;
                } else if(client->event == terminate_event){
                    client->connected=0;
                }
                break;
            case READY:
                logger_log(LOG_DEBUG, "[charcoal server] client at READY state");
                if(client->event == heartbeat_event) {
                
                } else if(client->event == terminate_event){
                    client->state = START;
                    client->connected=0;
                } else if(client->event == list_files_event){
                    
                    char *dir_name = message_get_body(client->request);

                    client_list_files_in_dir(client, dir_name);
                    
                    client->next_event = dispatching_event;
                    
                    client->state = WORKING;
                }
                break;
            case WORKING:
                logger_log(LOG_DEBUG, "[charcoal server] client at WORKING state");
                if(client->event == terminate_event){
                    client->state = START;
                    client->connected=0;
                } else if(client->event == dispatching_event){
                    
                    message_send(client->reply, client->fd);

                    if(message_has_more_chunks(client->reply)){
                        client->next_event = dispatching_event;
                    } else{
                        client->state=READY;
                    }
                }
                break;

        }
    }

}

void client_list_files_in_dir(client_t *client, char* dir_name){
    
    // A pointer to a directory
    DIR *d;
    
    // Represents an entry
    struct dirent *dir;
    
    // Open the dir
    d = opendir(dir_name);
    
    //
    char *buffer = malloc(sizeof(char) * 1024);
    int buffer_mul = 1;
    if(d){
        // Read dir content
        while((dir = readdir(d)) != NULL){
            // Do not record sub directories
            if((strlen(buffer) + 128) > 1024){
                    buffer_mul++;
                    realloc(buffer, sizeof(char) * buffer_mul * 1024);
            }
            strcat(buffer, dir->d_name);
            strcat(buffer, "\n");
            
        }
        
        // Close dir after reading all file names
        closedir(d);
    }
    
    client->reply = message_new_text(buffer);
}

void client_destroy(client_t *client){
    
    logger_log(LOG_INFO, "[charcoal server] cleaning client");
    
    if(client != NULL) {
        close(client->fd);
        free(client);
    }
}   


void server_run_loop(server_t *server){
    
    message_t *msg;
    client_t *client;
    
    if(server_open(server)){
        server->running = 1;
    } else {
        server->running = 0;
    }
    
    while(server->running){
        
        client = server_accept(server);
        
        if(client!= NULL){
            server_client_fsm(client, connected_event);
        }
        
        while(client->connected){
            
            client->request = message_recv(client->fd);
            int msg_type = message_get_type(client->request);
            
            if(msg_type == CHARC_MSG_SYN){
                server_client_fsm(client, syn_event);
            } else if (msg_type == CHARC_MSG_ACK){
                server_client_fsm(client, ack_event);
            } else if(msg_type == CHARC_MSG_UNDEFINED){
                server_client_fsm(client, terminate_event);
                
            } else if(msg_type == CHARC_MSG_LIST_FILE){
                server_client_fsm(client, list_files_event);
                
            }
            
        }
        
        // Clean client context
        client_destroy(client);
    }
    
    server_close(server);
    
}




