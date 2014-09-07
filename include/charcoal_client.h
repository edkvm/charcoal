#ifndef __CHARCOAL_CLIENT_H__
#define __CHARCOAL_CLIENT_H__


typedef struct charcoal_client_t charcoal_client_t;


// Instance method
charcoal_client_t * charcoal_client_new();
void charcoal_client_destroy(charcoal_client_t *self);
int charcoal_client_connect(charcoal_client_t *self, char *hostname, int port);



#endif
