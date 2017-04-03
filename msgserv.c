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
#define MAX_CONNECTIONS 20
#define max(A,B) ((A)>=(B)?(A):(B))

int main(int argc, char** argv){

  char *name;
  char line[50], msg[100], join_msg[100], buffer[1000], protocol_msg[20], content[100];
  int upt = 0, tpt = 0;
  int i, n=0, k=0;
  int m, r, sipt, addrlen, lc=0, n_msgs=0, sock_num=0;
  int socket_idServ, socket_rmb, *socket_ms_c, socket_ms_s,  maxfd, newfd, counter;
  int n_servidores_ativos=0;
  fd_set readfds, writefds;
  struct timeval timeout;
  struct in_addr *siip, ip;
  struct hostent *h;
  struct sockaddr_in sid, rmb, *ms, ms_s ; /*estruturas para os servidores de identidades e de mensagens*/
  MESSAGE_SERVER my_server;
  MESSAGE_SERVER * others_ms;
  long elapsed_time=0;
  time_t t;
  char** history;
  int flag_ms_s = 0;

  if( argc < 9 ){
    printf("invalid number of arguments\n");
    exit(-1);
  }

  srand((unsigned) time(&t));

  name = (char*)malloc(sizeof(argv[2]+1)*sizeof(char));

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

  history = (char**)malloc( m * sizeof( char* ) );
  for(i=0 ; i<m ; i++){
    history[i] = ( char* )malloc( LINE_MAX * sizeof( char ) );
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
          memset(buffer, 0, sizeof(buffer));
          n = recvfrom( socket_idServ, buffer, 1000, 0, (struct sockaddr*)&sid, &addrlen );
          if( n == -1){
            printf( "recvfrom: %s\n", strerror( errno ) );
            exit(1);
          }

          /* 4) after receiving the online msgervers list a tcp session is established
           with each one*/
           n_servidores_ativos = new_ms_array( buffer, my_server.name, &others_ms );

           if(n_servidores_ativos!=0){
             ms = ( struct sockaddr_in * )malloc( MAX_CONNECTIONS * sizeof( struct sockaddr_in ) );
             socket_ms_c = ( int* )malloc( n_servidores_ativos * sizeof( int ) );
           }


           for(k=0; k<n_servidores_ativos; k++){
             socket_ms_c[k] = new_tcp_session_c( &(others_ms[k].ip_addr), others_ms[k].tcp_port, &ms[k]);
             flag_ms_s=1;
           }

           /* 5) gets the history of messages from a random server online, by sending a SGET_MESSAGES message*/
           /*k=rand()%n_servidores_ativos;*/
           /*if(n_servidores_ativos!=0){

             for(sock_num=0; sock_num<n_servidores_ativos;sock_num++){
   						if(!strcmp(others_ms[sock_num].name, "PORTALEGRE")) break;
             }
             send_tcp_message("SGET_MESSAGES",socket_ms_c[sock_num]);

             memset(buffer, 0, sizeof(buffer));
             read_tcp_message(buffer, socket_ms_c[sock_num]);

             printf("historico recebido:\n%s", buffer);
           }*/
           /* 6) creates tcp_server sockets to accept requests form new msgservers */

            /*    if(n_servidores_ativos < MAX_CONNECTIONS){
            socket_ms_s = new_tcp_session_s( INADDR_ANY, 55000, &ms_s);
          }*/


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
    if(flag_ms_s) FD_SET(socket_ms_s, &readfds);

    maxfd = max(socket_idServ, socket_rmb); /*STDIN_FILENO = 0*/
  /*  maxfd = max(maxfd, socket_ms_c[sock_num]);*/

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
        /*a saida fecham-se as sockets e liberta-se a memoria*/

        if( close( socket_idServ ) != 0 ){
          printf( "close socket idServ: %s\n", strerror( errno ) );
          exit( 1 );
        }
        if( close( socket_rmb ) != 0 ){
          printf( "close socket rmb: %s\n", strerror( errno ) );
          exit( 1 );
        }
        for(k=0; k<n_servidores_ativos; k++){
          if( close( socket_ms_c[k] ) != 0 ){
            printf( "close socket message server: %s\n", strerror( errno ) );
            exit(1);
          }
        }
        /*FREE's*/
        free(name);
        free(my_server.name);
        for(i=0 ; i<m ; i++){
          free(history[i]);
        }
        free(history);

        for(i=0 ; i<20 ; i++){
            free(others_ms[i].name);

        }
        free(others_ms);
        free(socket_ms_c);
        free(ms);



        return(0);
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
      n = recvfrom( socket_rmb, buffer, 1000, 0, (struct sockaddr*)&rmb, &addrlen );
      if( n == -1){
        printf( "recvfrom: %s\n", strerror( errno ) );
        exit(1);
      }

      sscanf(buffer, "%s %s", protocol_msg, content);


      if( !strcmp( protocol_msg, "PUBLISH") ){

        extract_message(buffer);
        lc = save_message(buffer, history, lc);

        printf("Recebi uma mensagem: %s", buffer );
        printf(">> ");
        fflush(stdout);
      }
      else if( !strcmp( protocol_msg, "GET_MESSAGES") ){

          n_msgs = atoi(content);
          strcpy(msg, "MESSAGES\n");

          for( i=0 ; i<n_msgs ; i++){
            strcat( msg, history[i]);
          }

            n = sendto( socket_rmb, msg, strlen(msg)+1, 0, (struct sockaddr*)&rmb, sizeof(rmb) );
            if( n == -1 ){
              printf("nao consegui enviar a mensagem para o rmb");
              exit(1);
            }
      }
    }
/*  if(flag_ms_s){
      if( FD_ISSET( socket_ms_s, &readfds)){
        addrlen = sizeof( ms[sock_num] );
        if( ( newfd = accept( socket_ms_c[sock_num], (struct sockaddr*)&ms[sock_num], &addrlen ) ) == -1 ) exit(1);

        memset(buffer, 0, sizeof(buffer));
        read_tcp_message( buffer, newfd );

        if( !strcmp("SGET_MESSAGES", buffer) ){

        memset(msg, 0, sizeof(msg));
        strcpy(msg, "SMESSAGES\n");

        for( i=0 ; i<lc ; i++){
          strcat( msg, history[i]);
        }

        send_tcp_message( msg, newfd );
        }
      }
    }*/
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

	//ms.name = (char*)malloc(sizeof(sizeof(name))*sizeof(char));

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
  char **name_servers, **ip;
  int *port_udp, *port_tcp;
  struct in_addr ms_ip;

/*Alocaçoes*/
  name_servers = ( char** )malloc( MAX_CONNECTIONS * sizeof( char* ) );
  for(j=0; j<20; j++)
    name_servers[j] = ( char* )malloc( MAX_NAME * sizeof( char ) );

  ip=(char**)malloc( MAX_CONNECTIONS *sizeof(char*));
  for(j=0; j < MAX_CONNECTIONS ; j++)
    ip[j]=(char*)malloc(50*sizeof(char));

  port_udp=(int*)malloc(MAX_CONNECTIONS*sizeof(int));
  port_tcp=(int*)malloc(MAX_CONNECTIONS*sizeof(int));

  (*others_ms)=(MESSAGE_SERVER*)malloc( MAX_CONNECTIONS * sizeof(MESSAGE_SERVER));

  for(i=0 ; i< MAX_CONNECTIONS ; i++){
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
  free(port_udp);
  free(port_tcp);

  for(j=0; j<20; j++){
    free(name_servers[j]);
  }

  free(name_servers);

  for(j=0; j<20; j++){
     free(ip[j]);
  }

  free(ip);

  return(i);
}

void extract_message(char * line)
{
	char* tok;

	tok=strtok(line, " ");
	if( (tok=strtok(NULL, "\0")) == NULL ){
		printf("erro no strtok");
		exit(1);
	}

	strcpy(line, tok);
}

int save_message(char* buffer, char** history, int lc){
  printf("tamanho history: %d\n", sizeof(history[lc]));
  strcpy( history[lc] , buffer );
  printf("tamanho history: %d\n", sizeof(history[lc]));

  return(lc+1);
}
