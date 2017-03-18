#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char ** argv)
{
	struct sockaddr_in sid;
	struct in_addr siip;
	struct hostent *h;
	int sipt = 59000;

	h = gethostbyname("tejo.tecnico.ulisboa.pt");

	if(!inet_aton(h->h_addr_list[0] , &siip)){
		exit(-1);
	}
/*teste*/

	if(argc==3) /*caso em que so dão o ip ou o porto*/
		{
			if(!strcmp(argv[1],"-i"))
				{
					if(!inet_aton(argv[2] , &siip))
					{
						exit(-1);
					}
				}
			else
				{
					sipt=atoi(argv[2]);
				}

		}
	else if(argc==5)
		{
			if(!inet_aton(argv[2] , &siip)){
				exit(-1);
			}
			sipt=atoi(argv[4]);
		}

		sid.sin_addr=siip;
		sid.sin_port=sipt;

		printf("server IP: %s server port: %d \n", inet_ntoa(sid.sin_addr), sid.sin_port);

	return(0);

}
