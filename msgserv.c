#include "msgserv.h"
#include "tcp.h"
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
#include <time.h>

#define LINE_MAX 50
#define MAX_NAME 20
#define max(A,B) ((A)>=(B)?(A):(B))

int main(int argc, char** argv){

  char *name;
  char line[50], msg[100], join_msg[100], buffer[1000];
  int upt = 0, tpt = 0;
  int i, n=0, k=0;
  int m, r, sipt, addrlen;
  int socket_idServ, socket_rmb, *socket_tcp_c, maxfd, counter;
  int n_servidores_ativos=0;
  fd_set readfds;
  struct timeval timeout;
  struct in_addr *siip, ip;
  struct hostent *h;
  struct sockaddr_in sid, rmb, *ms ; /*estruturas para os servidores de identidades e de mensagens*/
  MESSAGE_SERVER my_server;
  MESSAGE_SERVER * others_ms;
  long elapsed_time=0;
  time_t t;

  if( argc < 9 ){
    printf("invalid number of arguments\n");
    exit(-1);
  }

  srand((unsigned) time(&t));

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
  my_server = new_message_server(sizeof(name));
  my_server = fill_message_server( my_server, name, upt, tpt, ip);

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

          /*AFTER THE JOIN COMMAND:*/
          /* 1) two new sockets are created, one for the ID server (as client), another to accept rmb requests (as server)*/
          socket_idServ = new_socket( siip , sipt , &sid );
          socket_rmb = new_udp_serv( INADDR_ANY , my_server.udp_port, &rmb);

          snprintf( join_msg, sizeof(join_msg), "%s %s;%s;%d;%d", "REG", my_server.name, inet_ntoa(my_server.ip_addr), my_server.udp_port, my_server.tcp_port );
          
          /* 2) a REG message is sent to the ID server*/
          n = sendto( socket_idServ, join_msg, strlen(join_msg), 0, (struct sockaddr*)&sid, sizeof(sid) );
          if( n == -1){
            printf( "sendto: %s\n", strerror( errno ) );
            exit(1);
          }

          /* 3) a GET_SERVERS message is sent to the ID server*/
          strcpy(msg, "GET_SERVERS");
          n = sendto( socket_idServ, msg, strlen(msg)+1, 0, (struct sockaddr*)&sid, sizeof(sid) );
          if( n == -1){
            printf( "sendto: %s\n", strerror( errno ) );
            exit(1);
          }

          addrlen = sizeof(sid);
          n = recvfrom( socket_idServ, buffer, 1000, 0, (struct sockaddr*)&sid, &addrlen );
          if( n == -1){
            printf( "recvfrom: %s\n", strerror( errno ) );
            exit(1);
          }



          /* 4) after receiving the online msgervers list a tcp session is established
           with each one*/
           n_servidores_ativos = new_ms_array( buffer, my_server.name, &others_ms );


           printf("estão %d servidores ativos e são:\n", n_servidores_ativos);

           ms = ( struct sockaddr_in * )malloc( n_servidores_ativos * sizeof( struct sockaddr_in ) );
           socket_tcp_c = ( int* )malloc( n_servidores_ativos * sizeof( int ) );

           for(k=0; k<n_servidores_ativos; k++){
            printf("servidor %d: %s %s %d %d\n", k+1, others_ms[k].name, inet_ntoa(others_ms[k].ip_addr), others_ms[k].udp_port, others_ms[k].tcp_port);
             socket_tcp_c[k] = new_tcp_session_c( &(others_ms[k].ip_addr), others_ms[k].tcp_port, &ms[k]);
           }

           /* 5) gets the history of messages from a random server online, by sending a SGET_MESSAGES message*/
           /*k=rand()%n_servidores_ativos;
           send_tcp_message("SGET_MESSAGES",socket_tcp_c[k]);*/

           /*memset(buffer, 0, sizeof(buffer));
           read_tcp_message(buffer, socket_tcp_c[k]);*/

           /*printf("mensagens:\n %s\n", buffer);*/
           printf(">> ");
           fflush(stdout);

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
    FD_SET(socket_idServ, &readfds);
    FD_SET(socket_rmb, &readfds);
    FD_SET(STDIN_FILENO, &readfds);

    //maxfd = socket_idServ;
    maxfd=max(socket_idServ, socket_rmb); /*STDIN_FILENO = 0*/

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
      n = sendto( socket_idServ, join_msg, strlen(join_msg), 0, (struct sockaddr*)&sid, sizeof(sid) );
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
        if( close( socket_idServ ) == 0 ){
          printf( "close socket: %s\n", strerror( errno ) );
          exit( 1 );
        }
      }

      else if( !strcmp( line, "show_servers\n")){
          strcpy(msg, "GET_SERVERS");
          n = sendto( socket_idServ, msg, strlen(msg)+1, 0, (struct sockaddr*)&sid, sizeof(sid) );
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
    if(FD_ISSET( socket_idServ, &readfds )){
      addrlen = sizeof(sid);
      n = recvfrom( socket_idServ, buffer, 1000, 0, (struct sockaddr*)&sid, &addrlen );
      if( n == -1){
        printf( "recvfrom: %s\n", strerror( errno ) );
        exit(1);
      }
      printf("%s", buffer);
      printf(">> ");
      fflush(stdout);

    }

    if( FD_ISSET( socket_rmb, &readfds) ){
      addrlen = sizeof(rmb);
      //memset(buffer, 0, sizeof(buffer));
      n = recv( socket_rmb, buffer, 1000, 0 );
      if( n == -1){
        printf( "recvfrom: %s\n", strerror( errno ) );
        exit(1);
      }
      extract_message(buffer);

      printf("Recebi uma mensagem: %s\n", buffer );
      printf(">> ");
      fflush(stdout);
    }


  }

  if( close(socket_idServ) == -1){
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
MESSAGE_SERVER new_message_server(int size){

    MESSAGE_SERVER ms;
    ms.name = (char*)malloc( size * sizeof( char ) );

    return ms;
}

MESSAGE_SERVER init_message_server(MESSAGE_SERVER ms){

    strcpy( ms.name , "-" );
    ms.udp_port = -1;
    ms.tcp_port = -1;
    if( memset(&ms.ip_addr, 0, sizeof(struct in_addr)) == NULL){
      printf("ERRO: inicializacao MESSAGE_SERVER\n");
      exit(1);
    }

    return ms;
}


MESSAGE_SERVER fill_message_server(MESSAGE_SERVER ms, char* name, int upt, int tpt, struct in_addr ip ){

	ms.name = (char*)malloc(sizeof(sizeof(name))*sizeof(char));
	/*  struct in_addr *ip;  struct in_addr *ip;*/

    strcpy( ms.name , name );
    ms.udp_port = upt;
    ms.tcp_port = tpt;
    ms.ip_addr = ip;

    return ms;
}

int new_ms_array(char* buffer, char* our_name, MESSAGE_SERVER ** others_ms){
  char *tok;
  char *delims = "\n";
  int i=0, j=0;
  char**name_servers, **ip;
  int *port_udp, *port_tcp;
  struct in_addr ms_ip;

/*Alocaçoes*/
  name_servers = ( char** )malloc( 20 * sizeof( char* ) );
  for(j=0; j<20; j++)
    name_servers[j] = ( char* )malloc( 20 * sizeof( char ) );

  ip=(char**)malloc(20*sizeof(char*));
  for(j=0; j<20; j++)
    ip[j]=(char*)malloc(50*sizeof(char));

  port_udp=(int*)malloc(20*sizeof(int));
  port_tcp=(int*)malloc(20*sizeof(int));

  (*others_ms)=(MESSAGE_SERVER*)malloc(20*sizeof(MESSAGE_SERVER));

  for(i=0 ; i<20 ; i++){
      (*others_ms)[i] = new_message_server(MAX_NAME);
      (*others_ms)[i] = init_message_server((*others_ms)[i]);
  }

  tok = strtok(buffer, delims);

  for( i=0 ; ( tok = strtok( NULL, delims ) ) != NULL ; ) {

    // process the line
    if(sscanf(tok, "%[^;]; %[^;]; %d; %d", name_servers[i], ip[i], &port_udp[i], &port_tcp[i]) != 4) break;

    if(!inet_aton(ip[i], &ms_ip)){
      printf("erro no inet_aton");
      exit(1);
    }
    if(strcmp(name_servers[i], our_name)){
      (*others_ms)[i] = fill_message_server( (*others_ms)[i], name_servers[i], port_udp[i], port_tcp[i], ms_ip);
      i++;
    }
  }

  return(i);
}

void extract_message(char * line)
{
	char* tok;

	tok=strtok(line, " ");
	if( (tok=strtok(NULL, "\n")) ==NULL )
	{
		printf("erro no strtok");
		exit(1);
	}

	strcpy(line, tok);
}