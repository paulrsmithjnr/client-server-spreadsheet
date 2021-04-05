#include<stdio.h>
#include <sys/types.h>	/* system type defintions */
#include <sys/socket.h>	/* network system functions */
#include <netinet/in.h>	/* protocol & struct definitions */
#include <stdlib.h>	/* exit() warnings */
#include <string.h>	/* memset warnings */
#include <unistd.h>
#include <ctype.h>
#include <float.h>
#include <pthread.h>

#define BUFFER_SIZE	1024
#define LISTEN_PORT	2123
#define NUM_RANGE   9
#define MAX_CLIENTS 100

//structure definition for a client
typedef struct client_t{
    struct sockaddr_in address;
	int sockfd;
    char name[20];
    int uid;
} client_t;

//function declarations
void *handle_client(void *arg);
void addToClientArray(client_t *client);
void removeFromClientArray(int uid);
void messageClient(char *message, int uid);
void broadcastMessage(char *message);
void getNewSpreadsheet();
void placeOnGrid(int x, int y, char* c);
int isEmptyCell(int x, int y);
int validatePosition(int x, int y);
int colLetterToNum(char letter);
int isNumber(char *string);
int power(int base, int exp);
int stringToNumber(char *string);
double average(char *start, char *end);
double sum(char *start, char *end);
double range(char *start, char *end);
void gridtoFile();

//global declaration of the spreadsheet's grid structure
char * grid[NUM_RANGE][NUM_RANGE];

//global declaration of the clients array
client_t *clients[MAX_CLIENTS];

//keeps track of the number of clients connected
static int clientCount = 0;

//generates unique ID for each client
static int genUID = 0;

//client mutex to facilitate multithreading
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

int main() {
    int	sock_listen,sock_recv;
    struct sockaddr_in	my_addr,recv_addr;
    int i,addr_size;
    int	incoming_len;
    struct sockaddr	remote_addr;
    int	recv_msg_size;
    int send_len,bytes_sent;
    char buffer[BUFFER_SIZE];
    pthread_t tid;

    /* create socket for listening */
    sock_listen=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_listen < 0){
        printf("socket() failed\n");
        exit(0);
    }
        
    /* make local address structure */
    memset(&my_addr, 0, sizeof(my_addr));	/* zero out structure */
    my_addr.sin_family = AF_INET;	/* address family */
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);  /* current machine IP */
    my_addr.sin_port = htons((unsigned short)LISTEN_PORT);

    /* bind socket to the local address */
    i=bind(sock_listen, (struct sockaddr *) &my_addr, sizeof (my_addr));
    if (i < 0){
        printf("[-] ERROR: Socket binding failed\n");
        exit(0);
    }
    
    /* listen ... */
    i=listen(sock_listen, 5);
    if (i < 0){
        printf("[-] ERROR: Socket listening failed\n");
        exit(0);
    }
    
    getNewSpreadsheet();
    while(1) {
        /* get new socket to receive data on */
        /* It extracts the first connection request from the  */
        /* listening socket  */
        addr_size=sizeof(recv_addr);
        sock_recv=accept(sock_listen, (struct sockaddr *) &recv_addr, &addr_size);

        if((clientCount + 1) > MAX_CLIENTS) {
            printf("[-] Max number of clients reached.");
            close(sock_recv);
            continue;
        }

        client_t *client = (client_t *)malloc(sizeof(client_t));
        client->address = recv_addr;
        client->sockfd = sock_recv;
        // strcpy(client->name, "sample_name");
        client->uid = genUID;
        genUID++;

        //TODO: Add client to array here
        addToClientArray(client);
        pthread_create(&tid, NULL, &handle_client, (void *)client);
    }
    printf("\n[+] Server shutdown successfully\n");
    close(sock_listen);
    
    // int x, y;
    // getNewSpreadsheet();
    // while (1){
    //     x = 0;
    //     y = 0;

    //     //receive cell details (address:value)
    //     bytes_received=recv(sock_recv,buffer,BUFFER_SIZE,0);
    //     buffer[bytes_received]=0;
    //     if (strcmp(buffer,"shutdown") == 0){
    //         printf("Received: %s ",buffer);
    //         break;
    //     }
        
    //     cellAddr = strtok(buffer, ":");
    //     cellVal = strtok(NULL, ":");
    //     printf("Received: %s -> %s\n", cellVal, cellAddr);

    //     if((cellVal[0] == '=')) { //checking for the average function

    //         char *function = strtok(cellVal, "=");
    //         function = strtok(function, "(");
    //         // printf("\nFunction: %s\n", function); //TODO: Remove
    //         if((strcmp(function, "average") == 0) || (strcmp(function, "AVERAGE") == 0)) {

    //             char *avgParam1 = strtok(NULL, "(");
    //             avgParam1 = strtok(avgParam1, ","); //first parameter stored here

    //             char *avgParam2 = strtok(NULL, ",");
    //             avgParam2[strlen(avgParam2)-1] = '\0'; //second parameter stored here

    //             // printf("\nParams: %s, %s\n", avgParam1, avgParam2); //TODO: Remove

    //             if((strlen(avgParam1) != 2) || (strlen(avgParam2) != 2)) {
    //                 strcpy(cellAddr, "00"); //will evoke a pre-handled error - message on the client side
    //             } else {
    //                 double resultAvg = average(avgParam1, avgParam2);
    //                 sprintf(cellVal, "%.2lf", resultAvg);
    //             }

    //         } else if((strcmp(function, "sum") == 0) || (strcmp(function, "SUM") == 0)) { //checking for the sum function
                
    //             char *sumParam1 = strtok(NULL, "(");
    //             sumParam1 = strtok(sumParam1, ","); //first parameter stored here

    //             char *sumParam2 = strtok(NULL, ",");
    //             sumParam2[strlen(sumParam2)-1] = '\0'; //second parameter stored here

    //             if((strlen(sumParam1) != 2) || (strlen(sumParam2) != 2)) {
    //                 strcpy(cellAddr, "00"); //will evoke a pre-handled error - message on the client side
    //             } else {
    //                 double resultSum = sum(sumParam1, sumParam2);
    //                 sprintf(cellVal, "%.2lf", resultSum);
    //             }

    //         } else if((strcmp(function, "range") == 0) || (strcmp(function, "RANGE") == 0)) { //checking for the range function
                
    //             char *rngParam1 = strtok(NULL, "(");
    //             rngParam1 = strtok(rngParam1, ","); //first parameter stored here

    //             char *rngParam2 = strtok(NULL, ",");
    //             rngParam2[strlen(rngParam2)-1] = '\0'; //second parameter stored here

    //             if((strlen(rngParam1) != 2) || (strlen(rngParam2) != 2)) {
    //                 strcpy(cellAddr, "00"); //will evoke a pre-handled error - message on the client side
    //             } else {
    //                 double resultRng = range(rngParam1, rngParam2);
    //                 sprintf(cellVal, "%.2lf", resultRng);
    //             }

    //         }
    //     }

    //     if (strlen(cellAddr) == 2) {
    //         y = colLetterToNum(cellAddr[0]);
    //         x = cellAddr[1] - '0';
    //         if(validatePosition(x,y)) {
    //             placeOnGrid(x, y, cellVal);
    //         }
    //     }
        
    //     //broadcast cell details (address:value) to all clients - sends '00' as the coordinates if the cell address received is invalid
    //     char coordinates[] = {x + '0', y + '0', '\0'};
    //     strcpy(details, coordinates);
    //     strcat(details, ":");
    //     strcat(details, cellVal);
    //     strcpy(buffer, details);
    //     send_len=strlen(details);
    //     bytes_sent=send(sock_recv,buffer,send_len,0);       
    // }
    // printf("\n");
    // close(sock_recv);
    // close(sock_listen);
    return 0;
}

void *handle_client(void *arg) {
    client_t *client = (client_t *)arg;
    char buffer[BUFFER_SIZE], *cellAddr, *cellVal, details[90];
    int bytes_received, x, y;

    bytes_received=recv(client->sockfd, buffer, BUFFER_SIZE,0);
    buffer[bytes_received] = 0;
    strcpy(client->name, buffer);

    clientCount++;
    printf("\n[+] %s (client %d) connected successfully\n", client->name, client->uid);
    printf("[+] Total number of clients: %d\n\n", clientCount);
    
    while (1){
        x = 0;
        y = 0;
        
        //receive cell details (address:value)
        bytes_received=recv(client->sockfd, buffer, BUFFER_SIZE, 0);
        buffer[bytes_received]=0;
        if (strcmp(buffer, "shutdown") == 0) {
            if(client->uid == 0) {
                printf("\n[+] %s (client %d) ended the session\n\n", client->name, client->uid);
            }
            break;
        }
        
        cellAddr = strtok(buffer, ":");
        cellVal = strtok(NULL, ":");
        printf("[+] %s (client %d): %s -> %s\n", client->name, client->uid, cellVal, cellAddr);

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

            } else if((strcmp(function, "range") == 0) || (strcmp(function, "RANGE") == 0)) { //checking for the range function
                
                char *rngParam1 = strtok(NULL, "(");
                rngParam1 = strtok(rngParam1, ","); //first parameter stored here

                char *rngParam2 = strtok(NULL, ",");
                rngParam2[strlen(rngParam2)-1] = '\0'; //second parameter stored here

                if((strlen(rngParam1) != 2) || (strlen(rngParam2) != 2)) {
                    strcpy(cellAddr, "00"); //will evoke a pre-handled error - message on the client side
                } else {
                    double resultRng = range(rngParam1, rngParam2);
                    sprintf(cellVal, "%.2lf", resultRng);
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
        broadcastMessage(details);
        // strcpy(buffer, details);
        // send_len=strlen(details);
        // bytes_sent=send(sock_recv,buffer,send_len,0);       
    }
    printf("\n[-] %s (client %d) disconnected\n", client->name, client->uid);
    close(client->sockfd);
    clientCount--;
    if(client->uid != 0) {
        printf("[-] Total number of clients: %d\n\n", clientCount);
    }
    removeFromClientArray(client->uid);
    free(client);
    pthread_detach(pthread_self());
}

void addToClientArray(client_t *client) {
    pthread_mutex_lock(&clients_mutex);

    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(!clients[i]) {
            clients[i] = client;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void removeFromClientArray(int uid) {
    pthread_mutex_lock(&clients_mutex);

    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(clients[i]->uid == uid) {
            clients[i] = NULL;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void messageClient(char *message, int uid) {
    pthread_mutex_lock(&clients_mutex);

    char buffer[BUFFER_SIZE];
    int bytes_sent, send_len = strlen(message);

    strcpy(buffer, message);
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(clients[i]) {
            if(clients[i]->uid == uid) {
                bytes_sent = send(clients[i]->sockfd, buffer, send_len, 0);
                if(bytes_sent < 0) {
                    printf("\n[-] Error in sending message to Client %d: %s\n", clients[i]->uid, clients[i]->name);
                }
                break;
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void broadcastMessage(char *message) {
    pthread_mutex_lock(&clients_mutex);

    char buffer[BUFFER_SIZE];
    int bytes_sent, send_len = strlen(message);

    strcpy(buffer, message);
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(clients[i]) {
            bytes_sent = send(clients[i]->sockfd, buffer, send_len, 0);
            if(bytes_sent < 0) {
                printf("\n[-] Error in sending message to Client %d: %s\n", clients[i]->uid, clients[i]->name);
                break;
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
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

//checks if selected cell is empty
int isEmptyCell(int x, int y){
    int flag = 0;
    if(strcmp(grid[x-1][y-1], " ") == 0){
        flag = 1;
    }
    return flag;
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

double range(char *start, char *end) {
    double range, largest, smallest;
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

    largest = -DBL_MAX; //assigns smallest possible double value
    smallest = DBL_MAX; //assigns largest possible double value
    for(int x = x1 - 1; x < ((x1 - 1) + ((x2 - x1)+1)); x++) {
        for(int y = y1 - 1; y <  ((y1 - 1) + ((y2 - y1)+1)); y++) {

            if(isNumber(grid[x][y]) >= 1) {
                if(atof(grid[x][y]) < smallest) {
                    smallest = atof(grid[x][y]);
                }
                if(atof(grid[x][y]) > largest) {
                    largest = atof(grid[x][y]);
                }
            } else if(strcmp(grid[x][y], " ") == 0) {
                if(0 < smallest) {
                    smallest = 0;
                }
                if(0 > largest) {
                    largest = 0;
                }
            }
        }
    }
    range = largest - smallest;
    return range;
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