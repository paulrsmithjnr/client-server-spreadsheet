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
#define EDIT_STACK_SIZE 20

//structure definition for a client
typedef struct client_t{
    struct sockaddr_in address;
	int sockfd;
    char name[20];
    int uid;
    char *editsStack[EDIT_STACK_SIZE];
} client_t;

//function declarations
void *handleClient(void *arg);
void addToClientArray(client_t *client);
void removeFromClientArray(int uid);
void messageClient(char *message, int uid);
void broadcastMessage(char *message);
void updateClientSpreadsheet(int uid);
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
void saveWorksheet(char *filename);
int getPosition(int uid);
void broadcastMessageToAllExcept(char *message, int uid);
void pushToClientEditStack(int uid, char *coordinates);
char *popFromClientEditStack(int uid);
void undo(int x, int y);
void clearClientEditStack(int uid);
void clearAllClientsEditStacks();

//global declaration of the spreadsheet's grid structure
char * grid[NUM_RANGE][NUM_RANGE];

//global declaration of the clients array
client_t *clients[MAX_CLIENTS];

//keeps track of the number of clients connected
static int clientCount = 0;

//generates unique ID for each client
static int genUID = 0;

//End Flag
static int endFlag = 0;

//client mutex to facilitate multithreading
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

//sockets
int	sock_listen, sock_recv;

int main() {
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
        printf("\n[-] ERROR: Failed to create socket\n\n");
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
        printf("\n[-] ERROR: Socket binding failed\n\n");
        exit(0);
    }
    
    /* listen ... */
    i=listen(sock_listen, 5);
    if (i < 0){
        printf("\n[-] ERROR: Socket listening failed\n\n");
        exit(0);
    }
    
    printf("[+] Server started successfully\n");
    printf("[+] Listening for clients...\n");
    getNewSpreadsheet();
    while(1) {
        /* get new socket to receive data on */
        /* It extracts the first connection request from the  */
        /* listening socket  */
        addr_size=sizeof(recv_addr);
        sock_recv=accept(sock_listen, (struct sockaddr *) &recv_addr, &addr_size);

        if(endFlag) {
            break;
        }

        if((clientCount + 1) > MAX_CLIENTS) {
            printf("\n[-] Max number of clients reached.\n");
            close(sock_recv);
            continue;
        }

        client_t *client = (client_t *)malloc(sizeof(client_t));
        client->address = recv_addr;
        client->sockfd = sock_recv;
        client->uid = genUID;
        genUID++;

        addToClientArray(client);
        pthread_create(&tid, NULL, &handleClient, (void *)client);
        
    }
    
    return 0;
}

void *handleClient(void *arg) {
    client_t *client = (client_t *)arg;
    char buffer[BUFFER_SIZE], *cellAddr, *cellVal, details[90], message[100];
    int bytes_received, x, y;

    clearClientEditStack(client->uid);

    bytes_received=recv(client->sockfd, buffer, BUFFER_SIZE,0);
    buffer[bytes_received] = 0;
    strcpy(client->name, buffer);

    clientCount++;

    printf("\n[+] %s (client %d) connected successfully\n", client->name, client->uid);
    printf("[+] Total number of clients: %d\n", clientCount);

    if(client->uid == 0) {
        messageClient("first", client->uid);
    } else {
        messageClient("not_first", client->uid);
    }
    sleep(0.5);

    updateClientSpreadsheet(client->uid);

    sprintf(message, "count:%d", clientCount);
    broadcastMessage(message);
    sleep(1);
    sprintf(message, "update:[+] %s (client %d) joined the session", client->name, client->uid);
    broadcastMessageToAllExcept(message, client->uid);
    
    while (1) {
        x = 0;
        y = 0;
        
        //receive cell details (address:value)
        bytes_received=recv(client->sockfd, buffer, BUFFER_SIZE, 0);
        buffer[bytes_received]=0;
        if (strcmp(buffer, "shutdown") == 0) {
            if(client->uid == 0) {
                printf("\n[+] %s (client %d) ended the session\n", client->name, client->uid);
                char endSessionMsg[11];
                strcpy(endSessionMsg, "endsession"); 
                broadcastMessage(endSessionMsg);
                endFlag = 1;
            }
            break;
        }else if (strcmp(buffer, "clearSheet") == 0){
            if(client->uid == 0) {
                printf("\n[+] %s (client %d) cleared the spreadsheet\n", client->name, client->uid);
                char clearMessage[11];
                strcpy(clearMessage, "clear");
                broadcastMessage(clearMessage);
                getNewSpreadsheet();
                sleep(0.5);
                sprintf(message, "update:[+] %s (client %d) cleared the spreadsheet", client->name, client->uid);
                broadcastMessageToAllExcept(message, client->uid);
            }
            continue;
        } else if (strcmp(buffer, "undo") == 0){
            char *coordinates = popFromClientEditStack(client->uid);
            if(coordinates) {


                char undoMessage[11];
                // char *coordinates = popFromClientEditStack(client->uid);
                int xCoordinate = coordinates[0] - '0', yCoordinate = coordinates[1] - '0';

                undo(xCoordinate, yCoordinate);

                strcpy(undoMessage, "undo:");
                strcat(undoMessage, coordinates);
                broadcastMessage(undoMessage);

                printf("\n[+] %s (client %d) undid their most recent edit\n", client->name, client->uid);

                sleep(0.5);
                sprintf(message, "update:[+] %s (client %d) undid their most recent edit", client->name, client->uid);
                broadcastMessageToAllExcept(message, client->uid);

            
            }
            continue; 
        }
        
        cellAddr = strtok(buffer, ":");
        cellVal = strtok(NULL, ":");

        if(strcmp(cellAddr, "saveSheet")==0){
            if(client->uid == 0) {
                printf("\n[+] %s (client %d) saved the spreadsheet\n", client->name, client->uid);
                sleep(0.5);
                sprintf(message, "update:[+] %s (client %d) saved the spreadsheet", client->name, client->uid);
                broadcastMessageToAllExcept(message, client->uid);
                saveWorksheet(cellVal);
            }
            continue;
        }

        printf("[+] %s (client %d): %s -> %s\n", client->name, client->uid, cellVal, cellAddr);

        if((cellVal[0] == '=')) { //checking for the average function

            char *function = strtok(cellVal, "=");
            function = strtok(function, "(");
            if((strcmp(function, "average") == 0) || (strcmp(function, "AVERAGE") == 0)) {

                char *avgParam1 = strtok(NULL, "(");
                avgParam1 = strtok(avgParam1, ","); //first parameter stored here

                char *avgParam2 = strtok(NULL, ",");
                avgParam2[strlen(avgParam2)-1] = '\0'; //second parameter stored here

                if((strlen(avgParam1) != 2) || (strlen(avgParam2) != 2)) {
                    // strcpy(cellAddr, "00"); //will evoke a pre-handled error - message on the client side
                    continue;
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
                    // strcpy(cellAddr, "00"); //will evoke a pre-handled error - message on the client side
                    continue;
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
                    // strcpy(cellAddr, "00"); //will evoke a pre-handled error - message on the client side
                    continue;
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
        pushToClientEditStack(client->uid, coordinates);

        sleep(0.5);
        sprintf(message, "update:[+] %s (client %d) updated spreadsheet with %s -> %s", client->name, client->uid, cellVal, cellAddr);
        broadcastMessageToAllExcept(message, client->uid);
        
    }
    
    printf("\n[-] %s (client %d) disconnected", client->name, client->uid);
    clientCount--;
    if(client->uid != 0) {
        if(!endFlag) {
            printf("\n[-] Total number of clients: %d\n", clientCount);
            sprintf(message, "count:%d", clientCount);
            broadcastMessage(message);
            sleep(0.5);
            sprintf(message, "update:[+] %s (client %d) left the session", client->name, client->uid);
            broadcastMessageToAllExcept(message, client->uid);
        }
    } else {
        while(clientCount > 0);
        printf("\n[-] Total number of clients: %d\n", clientCount);
        removeFromClientArray(client->uid);
        free(client);

        pthread_detach(pthread_self());
        printf("\n[+] Server shutdown successfully\n\n");
        close(sock_listen);
        close(sock_recv);
        exit(0);
    }
    close(client->sockfd);
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
        if(clients[i]) {
            if(clients[i]->uid == uid) {
                clients[i] = NULL;
                break;
            }
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
        if(clients[i]->uid == uid) {
            bytes_sent = send(clients[i]->sockfd, buffer, send_len, 0);
            if(bytes_sent < 0) {
                printf("\n[-] Error in sending message to Client %d: %s\n", clients[i]->uid, clients[i]->name);
                break;
            }
            break;
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

void broadcastMessageToAllExcept(char *message, int uid) {
    pthread_mutex_lock(&clients_mutex);

    char buffer[BUFFER_SIZE];
    int bytes_sent, send_len = strlen(message);

    strcpy(buffer, message);
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(clients[i]) {
            if(clients[i]->uid != uid) {
                bytes_sent = send(clients[i]->sockfd, buffer, send_len, 0);
                if(bytes_sent < 0) {
                    printf("\n[-] Error in sending message to Client %d: %s\n", clients[i]->uid, clients[i]->name);
                    break;
                }
            }
            
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void updateClientSpreadsheet(int uid) {
    // sprintf(cellVal, "%.2lf", resultAvg);
    char coordinates[3], details[90], buffer[BUFFER_SIZE];
    int bytes_received, position = getPosition(uid);
    
    // broadcastMessage(details);
    for(int x = 0; x < NUM_RANGE; x++) {
        for(int y = 0; y < NUM_RANGE; y ++) {
            memset(&details, 0, sizeof(details));

            coordinates[0] = (x+1) + '0';
            coordinates[1] = (y+1) + '0';
            coordinates[2] = '\0';

            strcpy(details, coordinates);
            strcat(details, ":");
            strcat(details, grid[x][y]);

            messageClient(details, uid);
            bytes_received=recv(clients[position]->sockfd, buffer, BUFFER_SIZE, 0);
            buffer[bytes_received]=0;
        }
    }
    messageClient("Done", uid);
}

void clearClientEditStack(int uid) {
    int position = getPosition(uid);

    for(int i = 0; i < EDIT_STACK_SIZE; i++) {
        clients[position]->editsStack[i] = NULL;
    }
}

void clearAllClientsEditStacks() {
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(clients[i]) {
            clearClientEditStack(clients[i]->uid);
        }
    }
}

void pushToClientEditStack(int uid, char *coordinates) {
    int position =  getPosition(uid);
    char *string = malloc(sizeof(char *));
    strcpy(string, coordinates);

    if(clients[position]->editsStack[EDIT_STACK_SIZE-1] != NULL) {
        clearClientEditStack(uid);
    }

    for(int i = 0; i < EDIT_STACK_SIZE; i++) {
        if (!clients[position]->editsStack[i]) {
            clients[position]->editsStack[i] = string;
            break;
        }
    }
}

char *popFromClientEditStack(int uid) {
    int position =  getPosition(uid);
    char *coordinates = malloc(sizeof(char *));

    for(int i = (EDIT_STACK_SIZE-1); i >= 0 ; i--) {

        if (clients[position]->editsStack[i]) {

            coordinates = clients[position]->editsStack[i];
            clients[position]->editsStack[i] = NULL;
            return coordinates;

        }

    }

    return NULL;
}

void undo(int x, int y) {
    placeOnGrid(x, y, " ");
}

int getPosition(int uid) {
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(clients[i]) {
            if(clients[i]->uid == uid) {
                return i;
            }
        }
    }
}

void getNewSpreadsheet() {
    clearAllClientsEditStacks();

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
void saveWorksheet(char *filename){
    char file[50];
    FILE *fptr, *fptr2;// file pointers

    strcpy(file, filename);
    strcat(file, ".txt");

    fptr=fopen(file,"w");
    if(fptr==NULL){
        printf("\n[-]ERROR: File was not created\n");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < NUM_RANGE; i++) {
        for(int j = 0; j < NUM_RANGE; j++) {
            if(strcmp(grid[i][j], " ") != 0) {
                fprintf(fptr,"%d%d:%s\n", i, j, grid[i][j]);
            }
        }
     }
     fclose(fptr);

     fptr2 = fopen("savedSpreadsheets.txt", "w");
     if (fptr2 == NULL) {
        printf("\n[-] 'savedSpreadsheets.txt' file not found\n");
    } else {
        fprintf(fptr,"%s\n", file);
    }
    fclose(fptr2);
}

void getFileNames() {
    FILE *fptr;
    char fileName[20];
    char message[200];

    fptr = fopen("savedSpreadsheets.txt", "r");

    int i = 0;
    if (fptr == NULL) {
        printf("\n[-] 'savedSpreadsheets.txt' file not found\n");
    } else {
        strcpy(message, "files:");

        fscanf(fptr, "%s\n", fileName);
        strcat(message, fileName);
        while(!feof(fptr)) {
            
            fscanf(fptr, "%s\n", fileName);
            strcat(message, ":");
            strcat(message, fileName);            
        }
        messageClient(message, 0);
    }
    fclose(fptr);
}