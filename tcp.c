#include "tcp.h"
#include "UDPserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>




int new_tcp_session_c( struct in_addr * ip, int port, struct sockaddr_in *addr){
  /*os parametros ip, port e addr sao do servidor ao qual o cliente se vai ligar*/
  int fd_tcp_c, n;

  fd_tcp_c = new_socket(ip, port, addr);

  n = connect( fd_tcp_c, (struct sockaddr*)addr, sizeof(*addr) );
  if( n == -1){
    printf("connect: %s\n", strerror(errno));
    exit(1);
  }

  return(fd_tcp_c);

}

int new_tcp_session_s( struct in_addr * ip, int port, struct sockaddr_in *addr){
  /*os parametros ip, port e addr sao do cliente ao qual o servidor se vai ligar*/
  int fd_tcp_s, n;

  fd_tcp_s = new_socket(ip, port, addr);
  /*addr ´e a estrutura que tem os parametros do cliente*/

  if( bind( fd_tcp_s, (struct sockaddr*)addr, sizeof(addr) ) == -1 ){
    printf("bind: %s", strerror(errno));
    exit(1);
  }

  if( listen( fd_tcp_s, 10) == -1){
    printf("listen: %s\n", strerror(errno));
  }

  return(fd_tcp_s);

}

void send_tcp_message( char* message, int fd ){

  int nleft = sizeof( message );
  int nwritten;
  char* ptr;

  ptr = message;

  while( nleft > 0 ){

    nwritten = write(fd, ptr, nleft);

    if( nwritten <= 0 ){
      printf( "write: %s", strerror( errno ) );
      exit(1);
    }
    nleft -= nwritten;
    ptr += nwritten;

  }
}

void read_tcp_message(char* buffer, int fd){

  int nleft = 1000;
  int nread;
  char* ptr;

  ptr = buffer;

  while( nleft > 0 ){

    nread = read( fd, ptr, nleft );

    if( nread == -1 ){
      printf( "read: %s", strerror( errno ) );
      exit( 1 );
    }
    else if( nread == 0 ) break;
    nleft -= nread;
    ptr += nread;

  }

}
