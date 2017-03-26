#include "UDPserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>


#define LINE_MAX 50

typedef struct message_server{
  char *name;
  u_int16_t udp_port;
  u_int16_t tcp_port;
  struct in_addr ip_addr;
}message_server;

void reset_buffer( char** );
struct message_server init_message_server(char*, int, int, struct in_addr);

int main(int argc, char** argv){

  char *name;
  char line[50], msg[100], join_msg[100], buffer[1000];
  int upt = 0, tpt = 0;
  int i, n=0;
  int m, r, sipt, addrlen;
  int socket_udp_c, maxfd, counter;
  fd_set readfds;
  struct timeval timeout;
  struct in_addr *siip, ip;
  struct hostent *h;
  struct sockaddr_in sid; /*estruturas para os servidores de identidades e de mensagens*/
  struct message_server ms;
  long elapsed_time=0;

  if( argc < 9 ){
    printf("invalid number of arguments\n");
    exit(-1);
  }

  name = (char*)malloc(sizeof(argv[2]+1)*sizeof(char));
/*  buffer = (char*)malloc(1000*sizeof(char));*/
  /*  struct in_addr *ip;  struct in_addr *ip;*/
  strcpy( name , argv[2] );
  upt = atoi( argv[6] );
  tpt = atoi( argv[8] );
  if( !inet_aton( argv[4] , &ip ) ){
    exit(-1);
  }

  /*atribuir os parametros a uma variavel message_server*/
  ms = init_message_server( name, upt, tpt, ip);

  strcpy(msg, "GET_SERVERS");

  /*definir parametros opcionais, caso nao sejam dados na invocacao*/
  if((h = gethostbyname("tejo.tecnico.ulisboa.pt")) == NULL) {
      printf("gethostbyname: %s\n", strerror(h_errno) );
      exit(-1);
  }

  siip = (struct in_addr*)h->h_addr_list[0];
  sipt = 59000;
  m = 200;
  r = 10;

  /*verificar quais os parametros opcionais que sao dados*/
  for(i = 9 ; i < argc && argc > 9 ; i+=2){
    if(!strcmp( argv[i] , "-i")){
      if( !inet_aton( argv[i+1] , siip ) ){
        exit(-1);
      }
    }
    else if(!strcmp( argv[i] , "-p")){
       sipt = atoi(argv[i+1]);
    }
    else if(!strcmp( argv[i] , "-m")){
       m = atoi(argv[i+1]);
    }
    else if(!strcmp( argv[i] , "-r")){
      r = atoi(argv[i+1]);
    }
  }

  printf(">> ");
  fflush(stdout);
  while(fgets(line, LINE_MAX, stdin) != NULL){
  		if(!strcmp( line, "join\n" )){
  				socket_udp_c = new_socket( siip , sipt , &sid );
          snprintf( join_msg, sizeof(join_msg), "%s %s;%s;%d;%d", "REG", ms.name, inet_ntoa(ms.ip_addr), ms.udp_port, ms.tcp_port );
  				n = sendto( socket_udp_c, join_msg, strlen(join_msg), 0, (struct sockaddr*)&sid, sizeof(sid) );
  				printf(">> ");
          fflush(stdout);
          if( n == -1 ){
  					exit(1);
  				}
          break;
  		}
      else if( !strcmp( line, "exit\n" ) ){
        exit(1);
      }
      else{
        printf("Error: command not defined\n");
        fflush(stdout);
      }
      printf(">> ");
      fflush(stdout);
  }

  while(1){
    /*reset_buffer(&buffer);*/
    memset(buffer, 0, sizeof(buffer));


    FD_ZERO(&readfds);
    FD_SET(socket_udp_c, &readfds);
    FD_SET(STDIN_FILENO, &readfds);

    maxfd = socket_udp_c; /*porque STDIN_FILENO=0*/

    timeout.tv_sec = r;
    timeout.tv_usec = 0;

    counter = select( maxfd+1, &readfds, (fd_set*)NULL, (fd_set*)NULL, &timeout );

    if(counter == -1){
      printf("select: %s\n", strerror(errno) );
      exit(1);
    }

    elapsed_time += (r-timeout.tv_sec);
    if(elapsed_time >= r ){
      elapsed_time-=r;
      n = sendto( socket_udp_c, join_msg, strlen(join_msg), 0, (struct sockaddr*)&sid, sizeof(sid) );
      if( n == -1 ){
        exit(1);
      }
    }

    if(FD_ISSET( STDIN_FILENO, &readfds )){

      printf(">> ");
      fflush(stdout);
      fgets( line, LINE_MAX, stdin );
      /*os comandos do teclado exit, GET_SERVERS e GET_MESSAGES so podem acontecer depois
        de uma leitura do teclado*/
      if( !strcmp( line, "exit\n" )){
        if( close( socket_udp_c ) == 0 ){
          printf( "close socket: %s\n", strerror( errno ) );
          exit( 1 );
        }
      }

      else if( !strcmp( line, "show_servers\n")){
          strcpy(msg, "GET_SERVERS");
          n = sendto( socket_udp_c, msg, strlen(msg)+1, 0, (struct sockaddr*)&sid, sizeof(sid) );
          if( n == -1 ){
            exit(1);
          }

      }
      else if( !strcmp( line, "show_messages\n")){
          printf("received show_messages\n");
          printf(">> ");
          fflush(stdout);
      }
      else{
        printf("Error: command not defined\n");
        printf(">> ");
        fflush(stdout);
      }
    }

    /*a leitura nao tem de estar dentro do if fgets porque so depende de receber a mensagem*/
    if(FD_ISSET( socket_udp_c, &readfds )){
      addrlen = sizeof(sid);
      fflush(stdout);
      n = recvfrom( socket_udp_c, buffer, 1000, 0, (struct sockaddr*)&sid, &addrlen );

      if( n == -1){
        exit(1);
      }
      printf("%s", buffer);

      printf(">> ");
      fflush(stdout);
    }
  }

  if( close(socket_udp_c) == 0){
    printf("close socket: %s\n", strerror(errno) );
    exit(1);
  }
  return(0);

}

/*resets the message receiving buffer*/
void reset_buffer(char** buffer){
    memset(*buffer, 0, sizeof(*buffer));
}

/*initializes a message_server struct*/
struct message_server init_message_server(char* name, int upt, int tpt, struct in_addr ip ){
    struct message_server ms;

    ms.name = (char*)malloc(sizeof(name)*sizeof(char));

    strcpy( ms.name , name );
    ms.udp_port = upt;
    ms.tcp_port = tpt;
    ms.ip_addr = ip;

    return ms;

}
