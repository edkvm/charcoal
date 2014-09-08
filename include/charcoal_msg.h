#ifndef __CHARCOAL_MSG_H__
#define __CHARCOAL_MSG_H__

#define CHARC_MSG_UNDEFINED      -1
#define CHARC_MSG_SYN            65
#define CHARC_MSG_SYN_ACK        66
#define CHARC_MSG_ACK  		     67
#define CHARC_MSG_HERATBEAT      68
#define CHARC_MSG_HERATBEAT_ACK  69
#define CHARC_MSG_TEXT 			 70
#define CHARC_MSG_LIST_FILE		 71

// Message object
typedef struct message_t message_t;

// Creates a new message with the given type
message_t * message_new(int type);

// Creates a new text type message
message_t * message_new_text(char *body);

// Send message on socket
int message_send(message_t *msg, int fd);

// Receiv message from socket
message_t * message_recv(int fd);

// Returns the type of the message
int message_get_type(message_t *msg);

// Set the message body
void message_set_body(message_t *msg, char *data, int size);

// Get message body
char * message_get_body(message_t *msg);

// Returns true if the message has more chunks
int message_has_more_chunks(message_t *msg);

// Combines the second message body into the first, destroys seond message
void message_combine_cunk(message_t *base, message_t *chunk);

// TODO: Remove, here only for test
char * message_encode(message_t *msg);
message_t * message_decode(char *raw_data);

#endif
