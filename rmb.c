#include "UDPserver.h"
#include "rmb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define LINE_MAX 50
#define MAX_NAME 20

typedef enum {FALSE=0, TRUE=1} bool;

int main(int argc, char ** argv)
{
	struct sockaddr_in sid, msgserver;
	MESSAGE_SERVER * OnlineMsgServers;
	struct sockaddr_in user_msgserver;
	struct in_addr *siip;
	struct hostent *h;
	int sipt = 59000; /*se o utilizador não puser nada, assume-se porto 59000 e IP adress do servidor tejo*/

	char line[200], buffer[1000];
	char*line_copy;
	char message[200];
	char tok[21];
	int socket_idServ, socket_msgServ;
	int addrlen;
	int n, k=0;
	int n_servidores_ativos;
	bool knowsOnlineMsgservs=FALSE;
	bool hasMsgserv = FALSE;
	void (*old_handler)(int); /*interrupt handler*/
	time_t t;

	if( ( old_handler = signal( SIGPIPE, SIG_IGN ) ) == SIG_ERR ) exit(1);

	if((h = gethostbyname("tejo.tecnico.ulisboa.pt"))==NULL) exit(1);

	siip=(struct in_addr*)h->h_addr_list[0];


	if(argc==3) /*caso em que so dão o ip ou o porto*/
	{
			if(!strcmp(argv[1],"-i"))
				{
					if(!inet_aton(argv[2] , siip))
					{
						exit(-1);
					}
				}
			else
				{
					sipt=atoi(argv[2]);
				}

	}
	else if(argc==5) /*caso em que dão o IP e o porto*/
	{
			if(!inet_aton(argv[2] , siip)){
				exit(-1);
			}
			sipt=atoi(argv[4]);
	}

	srand((unsigned) time(&t));

	printf(">> ");

	while(fgets(line, LINE_MAX, stdin) != NULL){
			if(!strcmp( line, "exit\n" ) ){
				break;
			}

			line_copy=strdup(line);
			if( invalid_command(line_copy, tok) ){
				printf("Error: command not defined\n");
				printf(">> ");
			}
			else{

				if( knowsOnlineMsgservs == FALSE ){

					socket_idServ = new_socket( siip , sipt , &sid );

					/*1) ask IDserv what msgservers are online*/
					n = sendto( socket_idServ, "GET_SERVERS", strlen("GET_SERVERS"), 0, (struct sockaddr*)&sid, sizeof(sid) );
					if( n == -1){
						printf( "sendto: %s\n", strerror( errno ) );
						exit(1);
					}

					/*2) read the answer, save it in a buffer */
					memset(buffer, 0, sizeof(buffer));
					addrlen = sizeof( sid );

					n = recvfrom( socket_idServ, buffer, 1000, 0, (struct sockaddr*)&sid, &addrlen );
					if( n == -1){
						printf( "recvfrom: %s\n", strerror( errno ) );
						exit(1);
					}

					knowsOnlineMsgservs = TRUE;
				}

				if( (hasMsgserv == FALSE) && (strcmp( line, "show_servers\n" )) ){
					/*3) create an online msgservers array*/
					n_servidores_ativos = get_OnlineMsgServers( buffer, &OnlineMsgServers );

					/*de entre o numero de msgservers online escolhe um aleatorio para ser ligar*/
					if(n_servidores_ativos==0){
						printf("No server available, try again later\n");
						printf(">> ");
						continue;
					}
					/*k=rand()%n_servidores_ativos;*/

					for(k=0; k<n_servidores_ativos;k++)
					{
						if(!strcmp(OnlineMsgServers[k].name, "FILIPA")) break;
					}

					socket_msgServ = new_socket( &(OnlineMsgServers[k].ip_addr) , OnlineMsgServers[k].udp_port , &user_msgserver );

					hasMsgserv = TRUE;
				}

				if(!strcmp( line, "show_servers\n" ) ){

					n = sendto( socket_idServ, "GET_SERVERS", strlen("GET_SERVERS"), 0, (struct sockaddr*)&sid, sizeof(sid) );
					if( n == -1){
						printf( "sendto: %s\n", strerror( errno ) );
						exit(1);
					}

					memset(buffer, 0, sizeof(buffer));
					addrlen = sizeof( sid );

					n = recvfrom( socket_idServ, buffer, 1000, 0, (struct sockaddr*)&sid, &addrlen );
					if( n == -1){
						printf( "recvfrom: %s\n", strerror( errno ) );
						exit(1);
					}

					printf("%s", buffer);
					printf(">> ");
				}
				else if(!strcmp( line_copy, "publish" ) ){

					extract_message(line);

					if(strlen(line)>140){
						printf("Tamanho da mensagem superior ao permitido(140 caracteres)");
						printf(">> ");
						fflush(stdout);
						continue;
					}

					snprintf( message, sizeof(message), "%s %s", "PUBLISH", line);

		          	n = sendto( socket_msgServ, message, strlen(message), 0, (struct sockaddr*)&user_msgserver, sizeof(user_msgserver) );

		          	printf("enviei a mensagem %s para %s com ip %s e porto %d\n", message, OnlineMsgServers[k].name, inet_ntoa(user_msgserver.sin_addr), user_msgserver.sin_port);
		          
		          	if( n == -1){
		            printf( "sendto: %s\n", strerror( errno ) );
		            
		           	 if( errno == EPIPE){
									/*if the message server stops responding a new msgerver must be found*/
									knowsOnlineMsgservs = FALSE;
									hasMsgserv = FALSE;
						}
		          	}
					printf(">> ");
				}

				memset(line_copy, 0, sizeof(line_copy));
				memset(line, 0, sizeof(line));
				/*show_latest_messages n*/
			}
	}

	return(0);

}

int invalid_command(char* line, char* tok){

	char* delims=" ";

	if(!strcmp(line, "show_servers\n")) return(0); /*comando  valido*/

	//strcpy(tok, line);

	//strcpy( tok, strtok(tok, delims) );
	tok=strtok(line, delims);

	if(!strcmp(tok, "publish") || !strcmp(line, "show_latest_messages")) return(0); /*comando valido*/

	return(1);

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


int get_OnlineMsgServers(char* buffer, MESSAGE_SERVER ** ms_array){
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

  (*ms_array)=(MESSAGE_SERVER*)malloc(20*sizeof(MESSAGE_SERVER));

  for(i=0 ; i<20 ; i++){
      (*ms_array)[i] = new_message_server(MAX_NAME);
      (*ms_array)[i] = init_message_server((*ms_array)[i]);
  }

  tok = strtok(buffer, delims);

  for( i=0 ; ( tok = strtok( NULL, delims ) ) != NULL ; ) {

    // process the line
    if(sscanf(tok, "%[^;]; %[^;]; %d; %d", name_servers[i], ip[i], &port_udp[i], &port_tcp[i])!=4) break;

    if(!inet_aton(ip[i], &ms_ip)){
      printf("erro no inet_aton");
      exit(1);
    }
      (*ms_array)[i] = fill_message_server( (*ms_array)[i], name_servers[i], port_udp[i], port_tcp[i], ms_ip);
      i++;
  }

  return(i);
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

    strcpy( ms.name , name );
    ms.udp_port = upt;
    ms.tcp_port = tpt;
    ms.ip_addr = ip;

    return ms;
}
