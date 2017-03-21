#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <netinet/in.h>


int new_udp_serv( struct in_addr*, int, struct sockaddr_in*);


#endif
