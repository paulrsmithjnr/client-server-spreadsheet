#include<stdio.h>
#include <sys/types.h>	/* system type defintions */
#include <sys/socket.h>	/* network system functions */
#include <netinet/in.h>	/* protocol & struct definitions */
#include <stdlib.h>	/* exit() warnings */
#include <string.h>	/* memset warnings */
#include <unistd.h>

#define BUF_SIZE	1024
#define LISTEN_PORT	60000

int main() {
    int	sock_listen,sock_recv;
    struct sockaddr_in	my_addr,recv_addr;
    int i,addr_size,bytes_received;
    int	incoming_len;
    struct sockaddr	remote_addr;
    int	recv_msg_size;
    char buf[BUF_SIZE];
    int const NUM_RANGE=9;

    //global declaration structure grid
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
    int flag=1;
    while (flag){
        bytes_received=recv(sock_recv,buf,BUF_SIZE,0);
        buf[bytes_received]=0;
        printf("Received: %s\n",buf);
        if (strcmp(buf,"shutdown") == 0){
            flag=0;
            break;
        }
    }
    
    close(sock_recv);
    close(sock_listen);
    return 0;
}

//place value on position
void placeOnGrid(int x, int y, char* c){
    grid[x-1][y-1]=c;
    return;
}

//checks if spot on grid is already filled
bool checkGridSpot(int x, int y){
    printf("%s \n",grid[x-1][y-1] );
    if(*grid[x-1][y-1] == ' '){
        return true;
    }else{
        return false;
    }

}

//checks if coordinates is on the board
bool validatePosition(int x, int y){
    if(x == 0 || y == 0){
        return false;
    }
    if(x <= NUM_RANGE && y <= NUM_RANGE ){
        return true;
    }else{return false;}

}

//TODO create function to convert grid to string to send to client