#ifndef MSGSERV_H
#define MSGSERV_H

  #include <netinet/in.h>

  typedef struct _message_server{
    char *name;
    u_int16_t udp_port;
    u_int16_t tcp_port;
    struct in_addr ip_addr;
  }MESSAGE_SERVER;

  void reset_buffer( char** );
  MESSAGE_SERVER new_message_server( int );
  MESSAGE_SERVER init_message_server( MESSAGE_SERVER );
  MESSAGE_SERVER fill_message_server( MESSAGE_SERVER, char* , int, int, struct in_addr );
  int new_ms_array(char*, char*, MESSAGE_SERVER**);

#endif
