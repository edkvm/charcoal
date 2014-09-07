#ifndef __CHARCOAL_SERVER_H__
#define __CHARCOAL_SERVER_H__

typedef struct charcoal_server_t charcoal_server_t;

charcoal_server_t * charcoal_server_new();
void charcoal_server_destroy(charcoal_server_t *self);
void charcoal_server_bind(charcoal_server_t *self,int port);

#endif
