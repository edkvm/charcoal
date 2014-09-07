#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../include/charcoal_logger.h"
#include "../include/charcoal_msg.h"



struct message_t{
    char *body;
    size_t size;
    int type;
};

message_t * message_new(int type){
	message_t* msg = malloc(sizeof(message_t));
    
	if(msg == NULL){
		logger_log(LOG_ERROR, "[charcoal message] failed to allocate memory for a new message");
	} else {
		logger_log(LOG_DEBUG, "[charcoal message] message allocated");
	}
    
	msg->body = NULL;
	msg->type = type;
	msg->size = 0;
    
	return msg;
}

void message_destroy(message_t *msg){
	
	logger_log(LOG_DEBUG, "[charcoal message] freeing message");
	
	if(msg->body != NULL){
        //	free(msg->body);
	}
    
	if(msg != NULL){
		free(msg);
	}
}

char * convert_type_char(char *type){
	char *encoded = malloc(1);
	
	if(encoded == NULL){
		logger_log(LOG_ERROR, "[charcoal message] failed to allocate buffer");
	} else {
		logger_log(LOG_DEBUG, "[charcoal message] buffer allocated");
	}
	
	bzero(encoded,1);
	
	// Copy type
	bcopy(type, encoded, 1);
    
	return encoded;
}

/* Encode a message to a byte array, and destroy it */
char * message_encode(message_t *msg){
    
	logger_log(LOG_DEBUG, "[charcoal message] encodeing message");
	// Message size should include the type
	int msg_size = 1;
	char type[1];
	char *encoded;
	
	
	type[0] = msg->type;
	
	
	switch(msg->type){
		case CHARC_MSG_SYN:
			encoded = convert_type_char(type);
			break;
		case CHARC_MSG_SYN_ACK:
			encoded = convert_type_char(type);
			break;
		case CHARC_MSG_ACK:
			encoded = convert_type_char(type);
			break;
		case CHARC_MSG_ECHO:
			msg_size += msg->size;
			// Allocate memory
			encoded = malloc(msg_size);
			bzero(encoded,msg_size);
            
			if(encoded == NULL){
				logger_log(LOG_ERROR, "[charcoal message] failed to allocate buffer");
			} else {
				logger_log(LOG_DEBUG, "[charcoal message] buffer allocated");
			}
            
			if(encoded != NULL) {
				// Copy type
				bcopy(type, encoded, 1);
                
				// Copy body
				bcopy(msg->body, encoded+1, msg_size);
                
                
				message_destroy(msg);
			}
			break;
	}
	
	return encoded;
}



message_t * message_decode(char *raw_data){
    
	message_t *msg;
    
    
	if(raw_data == NULL){
        
		msg = message_new(CHARC_MSG_UNDEFINED);
	} else {
		switch(raw_data[0]){
			case CHARC_MSG_SYN:
				msg = message_new(raw_data[0]);
				break;
			case CHARC_MSG_SYN_ACK:
				msg = message_new(raw_data[0]);
				break;
			case CHARC_MSG_ACK:
				msg = message_new(raw_data[0]);
				break;
            case CHARC_MSG_ECHO:
                msg = message_new(raw_data[0]);
                break;
			default:
				msg = message_new(CHARC_MSG_UNDEFINED);
				break;
		}
	}
    
	return msg;
}

int message_send(message_t *msg, int fd){
	
	int send_num;
	
	// Encode message to buffer
	char *buffer = message_encode(msg);
	
	send_num = write(fd,buffer, sizeof(buffer));
  	
  	if (send_num < 0){
    	logger_log(LOG_ERROR, "failed writing to socket, %s",strerror(errno));
    	return -1;
  	}
  	
  	return 0;
}


message_t * message_recv(int fd){
	
	message_t *msg = NULL;
	char buffer[256];
    int recv_num;
    
    // Clear buffer
    bzero(buffer,256);
    
    // Block on read
    recv_num = read(fd,buffer,255);
    if (recv_num < 0)
    {
        // Will create a undefined message
        msg = message_decode(NULL);
        logger_log(LOG_ERROR, "failed reading from socket, %s", strerror(errno));
        
    } else {
        logger_log(LOG_DEBUG, "[charcoal message] message type: %d", buffer[0]);
    }
    
    return msg;
}

int message_get_type(message_t *msg){
    
    int type;
    
    if(msg != NULL){
        type = msg->type;
    } else {
        type = CHARC_MSG_UNDEFINED;
    }
    
    return type;
    
    
}


