#include "UDPserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>


int new_udp_serv( struct in_addr *ip , int port, struct sockaddr_in *serveraddr){

    int fd;

    /*AF_INET diz que ´e uma socket de internet, podia ser uma socket de UNIX*/
    /*SOCK_DGRAM diz que ´e um datagram socket*/
    /* 0 protocolo pedido*/
    /*retorna um inteiro nao negativo se obtiver sucesso*/
    fd = socket( AF_INET, SOCK_DGRAM, 0);
    if(fd==-1){
        printf("Error: unable to create socket\n");
        exit(1); /*error*/
    }

    memset( (void*)serveraddr, (int)'\0', sizeof(*serveraddr) );

    serveraddr->sin_family = AF_INET;
    serveraddr->sin_addr.s_addr = htonl( ip->s_addr );
    serveraddr->sin_port = htons( port );

    /*atribuir ao descritor do socket a adress do servidor*/
    if( bind( fd, (struct sockaddr*)serveraddr, sizeof(*serveraddr) ) == -1){
      printf("Error: Unable to bind socket and server adress\n");
      exit(1);
    }

    return(fd);
}
