#include "UDPserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int new_udp_serv( struct in_addr * ip , int port, struct sockaddr_in *addr){

    int fd_udp;

    fd_udp = new_socket(ip, port, addr);

    /*atribuir ao descritor do socket a adress do servidor*/
    if( bind( fd_udp, (struct sockaddr*)addr, sizeof(*addr) ) == -1){
      printf("Error: Unable to bind socket and server adress\n");
      exit(1);
    }

    return(fd_udp);

}

int new_socket(struct in_addr * ip, int port, struct sockaddr_in *addr){

  int fd;

    /*AF_INET diz que ´e uma socket de internet, podia ser uma socket de UNIX*/
    /*SOCK_DGRAM diz que ´e um datagram socket*/
    /* 0 protocolo pedido*/
    /*retorna um inteiro nao negativo se obtiver sucesso*/

  if(ip==NULL){
    printf("Error: Undefined IP to create socket");
    exit(1);
  }

  fd=socket(AF_INET,SOCK_DGRAM,0);
  if(fd==-1){
        printf("Error: unable to create socket\n");
        exit(1);
    }

  memset((void*)addr,(int)'\0',sizeof(*addr));

  addr->sin_family=AF_INET;
  addr->sin_addr.s_addr = ip->s_addr;
/*  addr->sin_addr.s_addr = inet_addr("192.168.0.1");*/
  addr->sin_port=htons((u_short)port);

  return(fd);
}
