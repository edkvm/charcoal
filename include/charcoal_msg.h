#ifndef __CHARCOAL_MSG_H__
#define __CHARCOAL_MSG_H__

#define CHARC_MSG_UNDEFINED      -1
#define CHARC_MSG_SYN            65
#define CHARC_MSG_SYN_ACK        66
#define CHARC_MSG_ACK  		     67
#define CHARC_MSG_HERATBEAT      68
#define CHARC_MSG_HERATBEAT_ACK  69
#define CHARC_MSG_ECHO			 70
#define CHARC_MSG_LIST_FILE		 71

typedef struct message_t message_t;

message_t * message_new(int type);
void message_destroy(message_t *msg);
char * convert_type_char(char *type);
char * message_encode(message_t *msg);
message_t * message_decode(char *raw_data);
int message_send(message_t *msg, int fd);
message_t * message_recv(int fd);
int message_get_type(message_t *msg);

#endif
