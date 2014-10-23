/*
    p0f hpfeed support
*/

#ifndef _P0F_HPFEED_SUPPORT_
#define _P0F_HPFEED_SUPPORT_

#include <jansson.h>
#include <hpfeeds.h>

typedef enum { 
  S_INIT,
  S_AUTH,
  S_AUTH_DONE,
  S_ERROR,
  S_TERMINATE
} hpfeed_session_state_t;

struct observation_node {
  struct observation_node *next;
  struct observation_node *previous;
  time_t timestamp;
  json_t *data;
};

typedef struct observation_node observation_node_t;

/* hpfeed parameters */

extern s8   *hpfeed_host,                  
            *hpfeed_ident,             
            *hpfeed_secret,        
            *hpfeed_channel; 

extern u16  hpfeed_port,
            hpfeed_delta;

/* hpfeed functions */

void  hpfeed_connect();
void  hpfeed_close();
u8    compare_observation(json_t *first, json_t *second);
void  hpfeed_add_observation(json_t * json);

#endif /* _P0F_HPFEED_SUPPORT_ */