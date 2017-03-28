#include "UDPserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#define LINE_MAX 50

int main(int argc, char ** argv)
{
	struct sockaddr_in sid;
	struct in_addr *siip;
	struct hostent *h;
	int sipt = 59000; /*se o utilizador não puser nada, assume-se porto 59000 e IP adress do servidor tejo*/

	char line[50], buffer[1000];
	int socket_udp_c;
	int addrlen;
	int n;

	if((h = gethostbyname("tejo.tecnico.ulisboa.pt"))==NULL) exit(-1);

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
	/*TENDO EM CONTA O QUE FAZEMOS NA FUNCAO NEW_SOCKET AINDA PRECISAMOS DISTO?*/
	/*	sid.sin_addr=*siip;
			sid.sin_port=sipt;

		printf("server IP: %s server port: %d \n", inet_ntoa(sid.sin_addr), sid.sin_port);*/

	printf(">> ");
	/*fflush(stdout);*/
	while(fgets(line, LINE_MAX, stdin) != NULL){
  		if(!strcmp( line, "show_servers\n" )){

				/*AFTER THE show_servers COMMAND:*/
        /* 1) a new socket is creted*/
        socket_udp_c = new_socket( siip , sipt , &sid );

				/* 2) a GET_SERVERS message is sento to the ID server */
				n = sendto( socket_udp_c, "GET_SERVERS", strlen("GET_SERVERS"), 0, (struct sockaddr*)&sid, sizeof(sid) );
				if( n == -1){
					printf( "sendto: %s\n", strerror( errno ) );
					exit(1);
				}

				/* 3) the ID server responds with a string containing the name, ip and ports of
							the online servers */
				memset(buffer, 0, sizeof(buffer));
				addrlen = sizeof( sid );

        n = recvfrom( socket_udp_c, buffer, 1000, 0, (struct sockaddr*)&sid, &addrlen );
        if( n == -1){
          printf( "recvfrom: %s\n", strerror( errno ) );
          exit(1);
        }

				printf("%s", buffer);
				printf(">> ");

				if( close(socket_udp_c) == -1){
			    printf("close socket: %s\n", strerror(errno) );
					exit(1);
			  }
			}
			else if(!strcmp( line, "exit\n" ) ){
				break;
			}
			else{
				printf("Error: command not defined\n");
				printf(">> ");
			}
	}

	return(0);

}
