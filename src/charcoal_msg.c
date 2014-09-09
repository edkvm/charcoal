#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../include/charcoal_logger.h"
#include "../include/charcoal_msg.h"
#include "../include/charcoal.h"


struct message_t{
    char *body;
    size_t size;
    int type;
    int chunk_seq;
    int total_chunks;
    int chuncked;
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
	msg->chunk_seq = 0;
    msg->total_chunks = 0;
	return msg;
}

message_t * message_new_text(char *body){
    
    message_t *msg = message_new(CHARC_MSG_TEXT);
    int msg_size = (int)strlen(body);
    
    // Set the body of the message
    message_set_body(msg, body, msg_size + 1);
    
    if(msg_size > CHARCOAL_MSG_BODY_SIZE){
        msg->total_chunks = msg_size / CHARCOAL_MSG_BODY_SIZE + (1 * msg_size % CHARCOAL_MSG_BODY_SIZE);
        msg->chuncked = 1;
    } else {
        msg->chunk_seq = 0;
        msg->total_chunks = 1;

    }
    
    return msg;
}

//
void message_destroy(message_t *msg){
	
	logger_log(LOG_DEBUG, "[charcoal message] freeing message");
	
	if(msg->body != NULL){
        free(msg->body);
	}
    
	if(msg != NULL){
		free(msg);
	}
}

// Converts message type to single elemnt byte array
char * convert_type_char(int type){
	char *encoded = malloc(1);
	
	if(encoded == NULL){
		logger_log(LOG_ERROR, "[charcoal message] failed to allocate buffer");
	} else {
		logger_log(LOG_DEBUG, "[charcoal message] buffer allocated");
	}
	
    // Copy type
	encoded[0] = type;
    
	return encoded;
}

// Encode a message to a byte array, and destroy it
char * message_encode(message_t *msg){
    
	logger_log(LOG_DEBUG, "[charcoal message] encodeing message");
	// Message size should include the type
	int msg_size = CHARCOAL_MSG_CMD_SIZE;
	char *encoded;
	
    switch(msg->type){
		case CHARC_MSG_SYN:
			encoded = convert_type_char(CHARC_MSG_SYN);
			break;
		case CHARC_MSG_SYN_ACK:
			encoded = convert_type_char(CHARC_MSG_SYN_ACK);
			break;
		case CHARC_MSG_ACK:
			encoded = convert_type_char(CHARC_MSG_ACK);
			break;
        case CHARC_MSG_LIST_FILE:
            msg_size += msg->size;
			
            encoded = (char *)malloc(sizeof(char) * msg_size);
            bzero(encoded, msg_size);
            
            encoded[0] = msg->type;
			
            if(msg->body != NULL){
                // Copy body
                bcopy(msg->body, &encoded[CHARCOAL_MSG_CMD_SIZE], msg->size);
            }
            break;
		case CHARC_MSG_TEXT:
			
            // Allocate memory
			encoded = (char *)malloc(sizeof(char) * CHARCOAL_MSG_SIZE);
			bzero(encoded, CHARCOAL_MSG_SIZE);
            
			if(encoded == NULL){
				logger_log(LOG_ERROR, "[charcoal message] failed to allocate buffer");
			} else {
				logger_log(LOG_DEBUG, "[charcoal message] buffer allocated");
			}
            
			if(encoded != NULL) {
				
                // Copy type
				encoded[0] = msg->type;
                
                // Copy total
				encoded[1] = msg->total_chunks;
                
                // Copy total
				encoded[2] = msg->chunk_seq;
                
                if(msg->body != NULL){
                    // Copy body
                    bcopy(msg->body+(msg->chunk_seq-1)*CHARCOAL_MSG_BODY_SIZE, &encoded[CHARCOAL_MSG_CMD_SIZE], CHARCOAL_MSG_BODY_SIZE);
                }
                
                
			}
            
			break;
	}
	
    // Destroy message only if it is the last chunk
    if(!message_has_more_chunks(msg)){
        message_destroy(msg);
    }
    
	return encoded;
}


// Decode a message from byte array
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
            case CHARC_MSG_LIST_FILE:
                msg = message_new(raw_data[0]);
                // Allocate memory for body
                msg->body = (char *)malloc(sizeof(char) * CHARCOAL_MSG_BODY_SIZE + 1);

                // Copy body
				bcopy(&raw_data[CHARCOAL_MSG_CMD_SIZE], msg->body, CHARCOAL_MSG_BODY_SIZE);
                
                // Stringify
                msg->body[CHARCOAL_MSG_BODY_SIZE] = '\0';
                
                break;
            case CHARC_MSG_TEXT:
                
                // Create new message
                msg = message_new(raw_data[0]);
                
                // Get total msgs
                msg->total_chunks = raw_data[1];
                
                // Get current seq
                msg->chunk_seq = raw_data[2];
                
                // Allocate memory for body
                msg->body = (char *)malloc(sizeof(char) * CHARCOAL_MSG_BODY_SIZE + 1);
                
                // Clear body
                bzero(msg->body, CHARCOAL_MSG_BODY_SIZE);

                // Copy body
				bcopy(&raw_data[CHARCOAL_MSG_CMD_SIZE], msg->body, CHARCOAL_MSG_BODY_SIZE);
                
                // Stringify
                msg->body[CHARCOAL_MSG_BODY_SIZE] = '\0';
                
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
	
    // Increment cunk seq
    msg->chunk_seq++;
    
	// Encode message to buffer
	char *buffer = message_encode(msg);
	
	send_num = write(fd,buffer, CHARCOAL_MSG_SIZE);
  	
  	if (send_num < 0){
    	logger_log(LOG_ERROR, "failed writing to socket, %s",strerror(errno));
    	return -1;
  	}
    
  	return 0;
}


message_t * message_recv(int fd){
	
	message_t *msg = NULL;
	char buffer[CHARCOAL_MSG_SIZE];
    int recv_num;
    
    // Clear buffer
    bzero(buffer,CHARCOAL_MSG_SIZE);
    
    // Block on read
    recv_num = read(fd,buffer,CHARCOAL_MSG_SIZE);
    
    if (recv_num < 0)
    {
        logger_log(LOG_ERROR, "failed reading from socket, %s", strerror(errno));
        // Will create a undefined message
        msg = message_decode(NULL);

        
    } else {
       logger_log(LOG_DEBUG, "[charcoal message] message type: %d", buffer[0]);
        msg = message_decode(buffer);

    }
    
    return msg;
}

// Gets the type of the message
int message_get_type(message_t *msg){
    
    int type;
    
    if(msg != NULL){
        type = msg->type;
    } else {
        type = CHARC_MSG_UNDEFINED;
    }
    
    return type;
    
}

// Set the message body
void message_set_body(message_t *msg, char *data, int size){
    
    msg->body = malloc(sizeof(char) * size);
    bcopy(data, msg->body, size);
	msg->size = size;
}

// Returns true if the message has more chunks
int message_has_more_chunks(message_t *msg){
    
    return msg->total_chunks > msg->chunk_seq;
}

// Combines the second message body into the first, destroys seond message
void message_combine_cunk(message_t *base, message_t *chunk){

    int prev_msg_seq = base->chunk_seq;
    int compund_msg_size;

    base->chunk_seq = chunk->chunk_seq;
    compund_msg_size = (CHARCOAL_MSG_BODY_SIZE * base->chunk_seq) + 1;
    
    base->body = realloc(base->body, compund_msg_size);
    
    if(base->body == NULL){
        logger_log(LOG_ERROR, "[charcoal message] failed to re allocate buffer");
    } else {
        logger_log(LOG_DEBUG, "[charcoal message] buffer allocated");
    }
    
    bcopy(chunk->body, &base->body[CHARCOAL_MSG_BODY_SIZE * prev_msg_seq], CHARCOAL_MSG_BODY_SIZE);
    
    base->body[compund_msg_size - 1] = '\0';
    
    message_destroy(chunk);
}

// Returns message body
char * message_get_body(message_t *msg){
    return msg->body;
}





