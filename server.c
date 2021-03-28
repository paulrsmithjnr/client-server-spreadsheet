#include<stdio.h>
#include <sys/types.h>	/* system type defintions */
#include <sys/socket.h>	/* network system functions */
#include <netinet/in.h>	/* protocol & struct definitions */
#include <stdlib.h>	/* exit() warnings */
#include <string.h>	/* memset warnings */
#include <unistd.h>

#define BUF_SIZE	1024
#define LISTEN_PORT	60000

void serverMenu();

int main() {
    int	sock_listen,sock_recv;
    struct sockaddr_in	my_addr,recv_addr;
    int i,addr_size,bytes_received;
    int	incoming_len;
    struct sockaddr	remote_addr;
    int	recv_msg_size;
    char buf[BUF_SIZE];


    //const for grid num reference
    int const NUM_RANGE=9;


    //grid global
    char * grid[NUM_RANGE][NUM_RANGE];


            /* create socket for listening */
    sock_listen=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_listen < 0){
        printf("socket() failed\n");
        exit(0);
    }
        /* make local address structure */
    memset(&my_addr, 0, sizeof (my_addr));	/* zero out structure */
    my_addr.sin_family = AF_INET;	/* address family */
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);  /* current machine IP */
    my_addr.sin_port = htons((unsigned short)LISTEN_PORT);

        /* bind socket to the local address */
    i=bind(sock_listen, (struct sockaddr *) &my_addr, sizeof (my_addr));
    if (i < 0){
        printf("bind() failed\n");
        exit(0);
    }
    
    
        /* listen ... */
    
    i=listen(sock_listen, 5);
    if (i < 0){
        printf("listen() failed\n");
        exit(0);
    }
     
 
        /* get new socket to receive data on */
        /* It extracts the first connection request from the  */
        /* listening socket  */
    addr_size=sizeof(recv_addr);
    sock_recv=accept(sock_listen, (struct sockaddr *) &recv_addr, &addr_size);
    // int flag=1;
    // while (flag){
    //     bytes_received=recv(sock_recv,buf,BUF_SIZE,0);
    //     buf[bytes_received]=0;
    //     printf("Received: %s\n",buf);
    //     if (strcmp(buf,"shutdown") == 0){
    //         flag=0;
    //         break;
    //     }
    // }


    serverMenu(sock_listen, sock_recv);

    close(sock_recv);
    close(sock_listen);
    return 0;
}

void serverMenu(int sock_listen, int sock_recv )
{

    char buff[BUF_SIZE];
    int bytes_received;
    char test_text[80];
    // infinite loop for chat
    for (;;) {

        // read the message from client and copy it in buffer
        bytes_received=recv(sock_recv,buff,BUF_SIZE,0);
        buff[bytes_received]=0;

       /* read(sock_recv, buff, sizeof(buff));*/
        // print buffer which contains the client contents
        printf("From client: %s\t To client : ", buff);
        bzero(buff, BUF_SIZE);

        // copy server message in the buffer
        while (!strcmp(buff, "quit"))
            ;

        strcpy(test_text,"Noiice");
        strcpy(buff,test_text);
        // and send that buffer to client
        write(sock_listen, buff, sizeof(buff));

        // if msg contains "Exit" then server exit and chat ended.
        if (strncmp("quit", buff, 4) == 0) {
            printf("Server Exit...\n");
            break;
        }
    }
}