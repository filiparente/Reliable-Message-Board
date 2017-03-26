#ifndef TCP_H
#define TCP_H

  #include <netinet/in.h>

  int new_tcp_session_c( struct in_addr *, int, struct sockaddr_in*);
  int new_tcp_session_s( struct in_addr*, int, struct sockaddr_in*);
  void send_tcp_message( char*, int );
  void read_tcp_message(char*, int);

#endif
