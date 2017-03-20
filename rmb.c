#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char ** argv)
{
	struct sockaddr_in sid;
	struct in_addr *siip;
	struct hostent *h;
	int sipt = 59000; /*se o utilizador não puser nada, assume-se porto 59000 e IP adress do servidor tejo*/

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

		sid.sin_addr=*siip;
		sid.sin_port=sipt;

		printf("server IP: %s server port: %d \n", inet_ntoa(sid.sin_addr), sid.sin_port);

	return(0);

}
