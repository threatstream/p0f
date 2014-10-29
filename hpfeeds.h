/*
    p0f hpfeeds support
*/

#ifndef _P0F_HPFEEDS_SUPPORT_
#define _P0F_HPFEEDS_SUPPORT_

#include <jansson.h>
#include <hpfeeds.h>

typedef enum { 
  S_INIT,
  S_AUTH,
  S_AUTH_DONE,
  S_ERROR,
  S_TERMINATE
} hpfeeds_session_state_t;

struct observation_node {
  struct observation_node *next;
  struct observation_node *previous;
  time_t timestamp;
  json_t *data;
};

typedef struct observation_node observation_node_t;

/* hpfeeds parameters */

extern s8   *hpfeeds_host,                  
            *hpfeeds_ident,             
            *hpfeeds_secret,        
            *hpfeeds_channel; 

extern u16  hpfeeds_port,
            hpfeeds_delta;

/* hpfeeds functions */

void  hpfeeds_connect();
void  hpfeeds_close();
u8    compare_observation(json_t *first, json_t *second);
void  hpfeeds_add_observation(json_t * json);
void hpfeeds_publish(struct observation_node * observation);

#endif /* _P0F_HPFEEDS_SUPPORT_ */