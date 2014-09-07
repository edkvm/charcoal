#include "charcoal.h"

int main(void){
	
	message_t *msg = message_new(CHARC_MSG_ECHO);

	msg->body = "Hello";
	msg->size = 5;
	char *buffer = message_encode(msg);	

	printf("Result: %s\n", buffer);

	message_t *r_msg = message_decode(buffer);	

	if(r_msg->type == CHARC_MSG_ECHO){
		printf("decoded syn message");
	} else {
		printf("error decoding message %s\n",buffer);
	}
}