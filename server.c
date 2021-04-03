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
double sum(char *start, char *end);
void gridtoFile();

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

        if((cellVal[0] == '=')) { //checking for the average function

            char *function = strtok(cellVal, "=");
            function = strtok(function, "(");
            // printf("\nFunction: %s\n", function); //TODO: Remove
            if((strcmp(function, "average") == 0) || (strcmp(function, "AVERAGE") == 0)) {

                char *avgParam1 = strtok(NULL, "(");
                avgParam1 = strtok(avgParam1, ","); //first parameter stored here

                char *avgParam2 = strtok(NULL, ",");
                avgParam2[strlen(avgParam2)-1] = '\0'; //second parameter stored here

                // printf("\nParams: %s, %s\n", avgParam1, avgParam2); //TODO: Remove

                if((strlen(avgParam1) != 2) || (strlen(avgParam2) != 2)) {
                    strcpy(cellAddr, "00"); //will evoke a pre-handled error - message on the client side
                } else {
                    double resultAvg = average(avgParam1, avgParam2);
                    sprintf(cellVal, "%.2lf", resultAvg);
                }

            } else if((strcmp(function, "sum") == 0) || (strcmp(function, "SUM") == 0)) { //checking for the sum function
                
                char *sumParam1 = strtok(NULL, "(");
                sumParam1 = strtok(sumParam1, ","); //first parameter stored here

                char *sumParam2 = strtok(NULL, ",");
                sumParam2[strlen(sumParam2)-1] = '\0'; //second parameter stored here

                if((strlen(sumParam1) != 2) || (strlen(sumParam2) != 2)) {
                    strcpy(cellAddr, "00"); //will evoke a pre-handled error - message on the client side
                } else {
                    double resultSum = sum(sumParam1, sumParam2);
                    sprintf(cellVal, "%.2lf", resultSum);
                }

            }
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
    int flag = 1; //assumes number is an integer

    for(int i = 0; i < strlen(string); i++) {
        if(isdigit(string[i]) == 0) {
            if((string[i] == '-')) {
                continue;
            } else if(string[i] == '.') {
                flag = 2; //number is a double
                continue;
            }
            flag = 0;
            break;
        }
    }
    return flag;
}

double average(char *start, char *end) {
    double avg = 0, count = 0;
    int xCoords[2] = {start[1] - '0', end[1] - '0'};
    int yCoords[2] = {colLetterToNum(start[0]), colLetterToNum(end[0])};
    int x1, y1, x2, y2;

    ///////This block makes it so that the order of the parameters given does not matter///////
    if (xCoords[0] < xCoords[1]) {
        x1 = xCoords[0];
        x2 = xCoords[1];
    } else {
        x1 = xCoords[1];
        x2 = xCoords[0];
    }

    if (yCoords[0] < yCoords[1]) {
        y1 = yCoords[0];
        y2 = yCoords[1];
    } else {
        y1 = yCoords[1];
        y2 = yCoords[0];
    }
    /////////////////////////////////////////////////////////////////////////////////////////////

    for(int x = x1 - 1; x < ((x1 - 1) + ((x2 - x1)+1)); x++) {
        for(int y = y1 - 1; y <  ((y1 - 1) + ((y2 - y1)+1)); y++) {

            if(isNumber(grid[x][y]) == 1) {

                avg += atoi(grid[x][y]);
                count++;

            } else if(isNumber(grid[x][y]) == 2) {

                avg += atof(grid[x][y]);
                count++;

            } else if(strcmp(grid[x][y], " ") == 0) {
                count++;
            }
        }
    }
    if(count == 0) {
        return 0;
    }
    return avg/count;
}

double sum(char *start, char *end) {
    double sum = 0;
    int xCoords[2] = {start[1] - '0', end[1] - '0'};
    int yCoords[2] = {colLetterToNum(start[0]), colLetterToNum(end[0])};
    int x1, y1, x2, y2;

    ///////This block makes it so that the order of the parameters given does not matter///////
    if (xCoords[0] < xCoords[1]) {
        x1 = xCoords[0];
        x2 = xCoords[1];
    } else {
        x1 = xCoords[1];
        x2 = xCoords[0];
    }

    if (yCoords[0] < yCoords[1]) {
        y1 = yCoords[0];
        y2 = yCoords[1];
    } else {
        y1 = yCoords[1];
        y2 = yCoords[0];
    }
    /////////////////////////////////////////////////////////////////////////////////////////////

    for(int x = x1 - 1; x < ((x1 - 1) + ((x2 - x1)+1)); x++) {
        for(int y = y1 - 1; y <  ((y1 - 1) + ((y2 - y1)+1)); y++) {

            if(isNumber(grid[x][y]) == 1) {

                sum += atoi(grid[x][y]);

            } else if(isNumber(grid[x][y]) == 2) {

                sum += atof(grid[x][y]);

            }
        }
    }
    return sum;
}

//write the contents of the grid to a file
void gridtoFile(){
    FILE *fptr;// file pointer 
    fptr=fopen("gridfile.txt","w");
    if(fptr==NULL){
        printf("ERROR :File was not created");
        exit(EXIT_FAILURE);
    }
     for(int i = 0; i < NUM_RANGE; i++) {
        for(int j = 0; j < NUM_RANGE; j++) {
            fprintf(fptr,"%d%d:%s\n", i, j, grid[i][j]);
        }
     }
     fclose(fptr);
}