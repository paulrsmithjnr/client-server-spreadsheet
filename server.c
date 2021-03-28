#include<stdio.h>
#include <sys/types.h>	/* system type defintions */
#include <sys/socket.h>	/* network system functions */
#include <netinet/in.h>	/* protocol & struct definitions */
#include <stdlib.h>	/* exit() warnings */
#include <string.h>	/* memset warnings */
#include <unistd.h>

#define BUF_SIZE	1024
#define LISTEN_PORT	2122
#define NUM_RANGE 9

//function declarations
void getNewSpreadsheet();
void placeOnGrid(int x, int y, char* c);
int checkGridSpot(int x, int y);
int validatePosition(int x, int y);
int colLetterToNum(char letter);

//global declaration structure grid
char * grid[NUM_RANGE][NUM_RANGE];

int main() {
    int	sock_listen,sock_recv;
    struct sockaddr_in	my_addr,recv_addr;
    int i,addr_size,bytes_received;
    int	incoming_len;
    struct sockaddr	remote_addr;
    int	recv_msg_size;
    int send_len,bytes_sent;
    char buf[BUF_SIZE];
    char *cellAddr, *cellVal, details[90];
    // int const NUM_RANGE=9;

    //global declaration structure grid
    // char * grid[NUM_RANGE][NUM_RANGE];


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
    getNewSpreadsheet();
    while (flag){

        //receive cell details (address:value)
        bytes_received=recv(sock_recv,buf,BUF_SIZE,0);
        buf[bytes_received]=0;
        if (strcmp(buf,"shutdown") == 0){
            printf("Received: %s ",buf);
            flag=0;
            break;
        }
        cellAddr = strtok(buf, ":");
        cellVal = strtok(NULL, ":");
        printf("Received: %s -> %s\n", cellVal, cellAddr);

        if (strlen(cellAddr) == 2) {
            int y = colLetterToNum(cellAddr[0]);
            int x = cellAddr[1] - '0';
            placeOnGrid(x, y, cellVal);

            //broadcast cell details (address:value) to all clients
            strcpy(details, cellAddr);
            strcat(details, ":");
            strcat(details, cellVal);
            strcpy(buf, details);
            send_len=strlen(details);
            bytes_sent=send(sock_recv,buf,send_len,0);
        }
        
    }
    printf("\n");
    close(sock_recv);
    close(sock_listen);
    return 0;
}

void getNewSpreadsheet() {
    for(int i = 0; i < NUM_RANGE; i++) {
        for(int j = 0; j < NUM_RANGE; j++) {
            grid[i][j] = malloc(sizeof(char *));
            strcpy(grid[i][j], " ");
        }
    }
}

//place value on position
void placeOnGrid(int x, int y, char* c){
    char *string = malloc(sizeof(char *));
    strcpy(string, c);
    grid[x-1][y-1]=string;
    return;
}

//checks if spot on grid is already filled
int checkGridSpot(int x, int y){
    printf("%s \n",grid[x-1][y-1] );
    if(*grid[x-1][y-1] == ' '){
        return 1;
    }else{
        return 0;
    }

}

//checks if coordinates is on the board
int validatePosition(int x, int y){
    if(x == 0 || y == 0){
        return 0;
    }
    if(x <= NUM_RANGE && y <= NUM_RANGE ){
        return 1;
    }else{return 0;}
}

int colLetterToNum(char letter) {
    switch(letter) {
        case 'A':
        case 'a':
            return 1;
        
        case 'B':
        case 'b':
            return 2;
        
        case 'C':
        case 'c':
            return 3;

        case 'D':
        case 'd':
            return 4;

        case 'E':
        case 'e':
            return 5;

        case 'F':
        case 'f':
            return 6;

        case 'G':
        case 'g':
            return 7;

        case 'H':
        case 'h':
            return 8;

        case 'I':
        case 'i':
            return 9;
        
        default:
            return 0;
    }
}