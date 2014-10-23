/* 
    p0f hpfeed native reporting
*/ 

#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/poll.h>

#include "types.h"
#include "debug.h"
#include "hpfeed.h"

#define READ_BLOCK_SIZE 32767

static hpfeed_session_state_t hpfeed_state;
static s8 default_hpfeed_channel[] = "p0f.events";
static s32 s = -1;

static observation_node_t *observation_root = NULL;

static u8 init_publish_done = 0;

static u64 observations_sent = 0;
static u64 observations_duplicate = 0;
static u64 observations_received = 0;

struct pollfd pfd;

s8  *hpfeed_host,                  
    *hpfeed_ident,             
    *hpfeed_secret,        
    *hpfeed_channel;            

u16 hpfeed_port = 10000,
    hpfeed_delta = 60;

/* read message from socket */

u8 *hpfeed_read_msg(int sock) {

  u8 *buffer;
  u32 msglen;

  s32 len;
  s32 templen;
  s8 tempbuf[READ_BLOCK_SIZE];

  if (read(s, &msglen, 4) != 4)
    FATAL("[+] p0f.hpfeed: Fatal read()");

  if ((buffer = malloc(ntohl(msglen))) == NULL)
    FATAL("[+] p0f.hpfeed: Fatal malloc()");

  *(u32 *) buffer = msglen;
  msglen = ntohl(msglen);

    len = 4;
    templen = len;
    while ((templen > 0) && (len < msglen)) {
        templen = read(s, tempbuf, READ_BLOCK_SIZE);
        memcpy(buffer + len, tempbuf, templen);
        len += templen;
    }

  if (len != msglen)
    FATAL("[+] p0f.hpfeed: Fatal read()");

  return buffer;
}

void hpfeed_get_error(hpf_msg_t *msg) {

  s8 *errmsg;

  if (msg) {
    if ((errmsg = calloc(1, msg->hdr.msglen - sizeof(msg->hdr))) == NULL)
      FATAL("[+] p0f.hpfeed: Fatal write()");
          
    memcpy(errmsg, msg->data, ntohl(msg->hdr.msglen) - sizeof(msg->hdr));

    SAYF("[+] p0f.hpfeed: server error: '%s'\n", errmsg);

    free(errmsg);
    free(msg);
  }
}

void hpfeed_close() {

  SAYF("[+] p0f.hpfeed: sent %llu, received %llu, duplicated %llu observations\n", \
        observations_sent, observations_received, observations_duplicate);

  if (s != -1) close(s);
}

/* open connection to hpfeed */

void hpfeed_connect() {

  /* socket already on - returning */
  if (s != -1) return;

  hpf_msg_t *msg;
  hpf_chunk_t *chunk;
  u8 *data;

  u32 nonce = 0;

  struct hostent *he;
  struct sockaddr_in host;

  if (!hpfeed_channel)
    hpfeed_channel = default_hpfeed_channel;

  memset(&host, 0, sizeof(struct sockaddr_in));
  host.sin_family = AF_INET;
  host.sin_port = htons(hpfeed_port);

  if ((he = gethostbyname((char *)hpfeed_host)) == NULL)
    FATAL("[+] p0f.hpfeed: Fatal gethostbyname()");

  host.sin_addr = *(struct in_addr *) he->h_addr;

  if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    FATAL("[+] p0f.hpfeed: Fatal socket()");

  if (connect(s, (struct sockaddr *) &host, sizeof(host)) == -1)
    FATAL("[+] p0f.hpfeed: Fatal connect()");

  /* Set poll fd */
  pfd.fd = s;
  pfd.events = POLLIN;
  pfd.revents = 0;

  /* Set connection keep alive */
  int optval = 1;

  if(setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
      FATAL("[+] p0f.hpfeed: Fatal setsockopt()");
      close(s);
      s = -1;
      return;
   }

  hpfeed_state = S_INIT;

  for (;;) { 

    switch (hpfeed_state) {

      case S_INIT:

        if ((data = hpfeed_read_msg(s)) == NULL) 
          break;

        msg = (hpf_msg_t *) data;

        switch (msg->hdr.opcode) {

          case OP_INFO:

            chunk = hpf_msg_get_chunk(data + sizeof(msg->hdr), ntohl(msg->hdr.msglen) - sizeof(msg->hdr));

            if (!chunk) { 
              SAYF("[+] p0f.hpfeed: invalid message format");
              hpfeed_state = S_TERMINATE;
              break;
            }

            nonce = *(u_int32_t *) (data + sizeof(msg->hdr) + chunk->len + 1);
            hpfeed_state = S_AUTH;

            free(data);
            break;

          case OP_ERROR:
            hpfeed_state = S_ERROR;
            break;

          default:
            
            hpfeed_state = S_TERMINATE;
            SAYF("[+] p0f.hpfeed: unknown server message (type %u)\n", msg->hdr.opcode);
            break;
        }

      case S_AUTH:

        SAYF("[+] p0f.hpfeed: sending authentication.\n");

        msg = hpf_msg_auth(nonce, (u8 *) hpfeed_ident, strlen((char *)hpfeed_ident) \
                           ,(u8 *) hpfeed_secret, strlen((char *)hpfeed_secret));

        if (write(s, (u_char *) msg, ntohl(msg->hdr.msglen)) == -1)
          FATAL("[+] p0f.hpfeed: Fatal write()");

        int rv = poll(&pfd, 1, 1000);

        if (rv == 0) {
          hpfeed_state = S_AUTH_DONE;
          SAYF("[+] p0f.hpfeed: Authentication done.\n");
          hpf_msg_delete(msg);
        }
        else if (rv > 0 && pfd.revents && POLLIN) {
          hpfeed_state = S_ERROR;

          if ((msg = (hpf_msg_t *) hpfeed_read_msg(s)) == NULL) 
            break;
        }

        break;

      case S_ERROR:

        if (msg)
          hpfeed_get_error(msg);

        hpfeed_state = S_TERMINATE;
        break;

      case S_TERMINATE:
      default:
        close(s);
        s = -1;
        SAYF("[+] p0f.hpfeed: connection terminated...\n");
        break;
      }

    if (hpfeed_state == S_AUTH_DONE || s == -1)
      break;
  }
}

/* Fields used for observation compare */

static char * comparsion_keys[] = {"client_ip", \
                                   "server_ip", \
                                   "app",       \
                                   "dist",      \
                                   "lang",      \
                                   "link",      \
                                   "mod",       \
                                   "os",        \
                                   "params",    \
                                   "raw_hits",  \
                                   "raw_mtu",   \
                                   "raw_sig",   \
                                   "reason",    \
                                   "subject", 0 };

/* Custom compare of two observations */

u8 compare_observation(json_t *first, json_t *second) {

  int position = 0;

  for ( ; comparsion_keys[position] ; position++)
  {

    char *compare_key = comparsion_keys[position];

    json_t *value_first = json_object_get(first, compare_key);
    json_t *value_second = json_object_get(second, compare_key);

    if (value_first == NULL || value_second == NULL)
      continue;

    if (!(value_first && value_second))
      return 0;

    if (!json_equal(value_first, value_second))
      return 0;
  }

  return 1;
}

/* Init observation list */

void init_list(json_t *observation, time_t timestamp) {

    observation_root = (observation_node_t *)malloc(sizeof(observation_node_t));

    observation_root->next = NULL;
    observation_root->previous = NULL;
    observation_root->data = observation;
    observation_root->timestamp = timestamp;

    hpfeed_publish(observation_root);
}

/* publish hpfeed observation to p0f */

void hpfeed_add_observation(json_t *observation) {

  if (s == -1) return;

  observations_received++;

  /* Store timestamp */

  time_t observation_timestamp = (time_t) json_integer_value(json_object_get(observation, "timestamp_raw"));

  /* Remove these object so we can do json_compare */
  json_object_del(observation, "timestamp_raw");

  /* Empty list - initial element */
  if (observation_root == NULL) {
    init_list(observation, observation_timestamp);
    return;
  }
  
  u8 found_object = 0;
  u8 empty_list = 0;

  observation_node_t *current = observation_root;

  while(current != NULL) {

    /* There are older entries than current one */
    if (observation_timestamp > current->timestamp + hpfeed_delta) {

      if (current == observation_root)
        empty_list = 1;   

      if (current->previous)
        current->previous->next = current->next;

      if (current->next)
        current->next->previous = current->previous;

      /* Free memory */
      json_decref(current->data);

      observation_node_t *next = current->next;

      free(current);

      current = next;
    }
    
    /* Found same entry before end */
    else if (found_object == 0 && compare_observation(observation, current->data)) {
      observations_duplicate++;
      found_object = 1;
      current = current->next;
      json_decref(observation);
    }

    /* We reached at the end of list and didn't find same observation */
    else if (current->next == NULL && found_object == 0) {

      observation_node_t * new = (observation_node_t *)malloc(sizeof(observation_node_t));

      new->next = observation_root;
      new->next->previous = new;
      new->previous = NULL;

      observation_root = new;

      new->data = observation;
      new->timestamp = observation_timestamp;

      hpfeed_publish(observation_root);

      current = current->next;
    }

    /* Continue to next node */
    else {
      current = current->next;
    }

    if (current == NULL && empty_list)
      init_list(observation, observation_timestamp);
  }

}


/* publish message to hpfeed channel */

void hpfeed_publish(struct observation_node * observation) {

  char *data = json_dumps(observation->data, 0);
  u32 len = strlen((char *)data);

  hpf_msg_t *msg;

  msg = hpf_msg_publish((u8 *) hpfeed_ident, strlen((char *)hpfeed_ident) \
                        ,(u8 *) hpfeed_channel, strlen((char *)hpfeed_channel), (u8 *)data, len);
  
  if (write(s, (u8 *) msg, ntohl(msg->hdr.msglen)) == -1)
    FATAL("[+] p0f.hpfeed: Fatal write()");


  /* Do another socket poll - in case of wrong channel */
  if (!init_publish_done) {
    hpf_msg_t *error_msg;

    int rv = poll(&pfd, 1, 1000);

    if (rv == 0) {
      init_publish_done = 1;
      SAYF("[+] p0f.hpfeed: Initial publish done.\n");
    }
    else if (rv > 0 && pfd.revents && POLLIN) {
            
      if ((error_msg = (hpf_msg_t *) hpfeed_read_msg(s)) != NULL) {
        
        hpfeed_get_error(error_msg);

        SAYF("[+] p0f.events: Failed to publish.\n");

        close(s);
        s = -1;
      }
      else {
        FATAL("[+] p0f.events: Something went wrong\n");
      }

    } 
  }

  observations_sent++;

  free(data);
  hpf_msg_delete(msg);
}