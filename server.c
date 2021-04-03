#include<stdio.h>
#include <sys/types.h>	/* system type defintions */
#include <sys/socket.h>	/* network system functions */
#include <netinet/in.h>	/* protocol & struct definitions */
#include <stdlib.h>	/* exit() warnings */
#include <string.h>	/* memset warnings */
#include <unistd.h>
#include <ctype.h>

#define BUF_SIZE	1024
#define LISTEN_PORT	2121
#define NUM_RANGE 9

//function declarations
void getNewSpreadsheet();
void placeOnGrid(int x, int y, char* c);
int checkGridSpot(int x, int y);
int validatePosition(int x, int y);
int colLetterToNum(char letter);
int isNumber(char *string);
int power(int base, int exp);
int stringToNumber(char *string);
double average(char *start, char *end);

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
    int x, y;
    getNewSpreadsheet();
    while (1){
        x = 0;
        y = 0;
        //receive cell details (address:value)
        bytes_received=recv(sock_recv,buf,BUF_SIZE,0);
        buf[bytes_received]=0;
        if (strcmp(buf,"shutdown") == 0){
            printf("Received: %s ",buf);
            break;
        }
        
        cellAddr = strtok(buf, ":");
        cellVal = strtok(NULL, ":");
        printf("Received: %s -> %s\n", cellVal, cellAddr);

        if((cellVal[0] == '=') && ((cellVal[1] == 'A') || (cellVal[1] == 'a'))) { //checking for the average function

            char *avgParam1 = strtok(cellVal, "(");
            avgParam1 = strtok(NULL, "(");
            avgParam1 = strtok(avgParam1, ","); //first parameter stored here

            char *avgParam2 = strtok(NULL, ",");
            avgParam2[strlen(avgParam2)-1] = '\0'; //second parameter stored here

            printf("\nParams: %s, %s\n", avgParam1, avgParam2);

            if((strlen(avgParam1) != 2) || (strlen(avgParam2) != 2)) {
                strcpy(cellAddr, "00"); //will evoke a pre-handled error - message on the client side
            } else {
                double test = average(avgParam1, avgParam2); //TODO: assign return val to cellVal
                printf("\nTest Average: %.2lf\n", test);
            }


            strcpy(cellVal, "average");

        } else if((cellVal[0] == '=') && ((cellVal[1] == 'S') || (cellVal[1] == 's'))) { //checking for the sum function
            strcpy(cellVal, "sum");
        }

        if (strlen(cellAddr) == 2) {
            y = colLetterToNum(cellAddr[0]);
            x = cellAddr[1] - '0';
            if(validatePosition(x,y)) {
                placeOnGrid(x, y, cellVal);
            }
        }
        
        //broadcast cell details (address:value) to all clients - sends '00' as the coordinates if the cell address received is invalid
        char coordinates[] = {x + '0', y + '0', '\0'};
        strcpy(details, coordinates);
        strcat(details, ":");
        strcat(details, cellVal);
        strcpy(buf, details);
        send_len=strlen(details);
        bytes_sent=send(sock_recv,buf,send_len,0);       
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

//checks if coordinates is on the grid
int validatePosition(int x, int y){
    if(((x < 1) || (x > NUM_RANGE)) || ((y < 1) || (y > NUM_RANGE))) {
        return 0;
    } else {
        return 1;
    }    
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

int isNumber(char *string) {
    int flag = 1;
    for(int i = 0; i < strlen(string); i++) {
        if(isdigit(string[i]) == 0) {
            flag = 0;
            break;
        }
    }
    return flag;
}

int power(int base, int exp) {
    int result = 1;
    while(exp != 0) {
        result *= base;
        exp--;
    }
    return result;
}

int stringToNumber(char *string) {
    int number = 0;
    for(int i = 0; i < strlen(string); i++) {
        number += (string[i] - '0') * power(10, (strlen(string) - (i+1)));
    }
    return number;
}

double average(char *start, char *end) {
    double avg = 0, count = 0;
    int x1 = start[1] - '0', x2 = end[1] - '0';
    int y1 = colLetterToNum(start[0]), y2 = colLetterToNum(end[0]);
    printf("\n(x1,y1) (x2, y2): (%d, %d) (%d, %d)\n", x1, y1, x2, y2);
    for(int x = x1 - 1; x < ((x1 - 1) + ((x2 - x1)+1)); x++) {
        for(int y = y1 - 1; y <  ((y1 - 1) + ((y2 - y1)+1)); y++) {
            printf("\nGrid (%d, %d): %s\n", x, y, grid[x][y]);
            if(isNumber(grid[x][y])) {
                printf("\nString to Number: %d\n", stringToNumber(grid[x][y]));
                avg += stringToNumber(grid[x][y]);
                count++;
            } else if(strcmp(grid[x][y], " ") == 0) {
                count++;
            } else {
                return -1.0;
            }
        }
    }
    if(count == 0) {
        return 0.0;
    }
    return avg/count;
}