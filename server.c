/*
COMP2130 Final Project

GROUP MEMBER            ID Number
---------------------------------
Javon Ellis         -   620126389
Monique Satchwell   -   620119851
Paul Smith          -   620118115

** BONUS FEATURES **

    -- The first client can open previously saved spreadsheets or create a new one.
    -- A menu is displayed to each client.
    -- The first client sees a different menu from the rest of the clients.
    -- Functions accept cell ranges that span across multiple rows and columns.
    -- The first client can clear the contents of the current spreadsheet.
    -- Clients can undo their most recent additions.
    -- Each client can clear the contents of a particular cell.

Command to run program: gcc -o server server.c -lpthread
*/
#include <stdio.h>
#include <sys/types.h>	/* system type defintions */
#include <sys/socket.h>	/* network system functions */
#include <netinet/in.h>	/* protocol & struct definitions */
#include <stdlib.h>	/* exit() warnings */
#include <string.h>	/* memset warnings */
#include <unistd.h>
#include <ctype.h>
#include <float.h>
#include <pthread.h>

//Definition of constants
#define BUFFER_SIZE	1024
#define LISTEN_PORT	2121
#define NUM_RANGE   9
#define MAX_CLIENTS 100
#define EDIT_STACK_SIZE 20

//structure definition for a client
typedef struct client_t{
    struct sockaddr_in address; //stores the client's address
	int sockfd; //stores the client's socket file descriptor
    char name[20]; //stores client's display name
    int uid; //stores client uid
    char *editsStack[EDIT_STACK_SIZE]; //stores the cell addresses that the client edited in order to facilitate the undo feature
} client_t;

//function declarations - explanations for each are above each function definition below
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
void getFileNames();
void loadFileToSpreadsheet(char *file);
void updateAllClientsSpreadsheet();

//global declaration of the spreadsheet's grid structure
char * grid[NUM_RANGE][NUM_RANGE];

//global declaration of the clients array
client_t *clients[MAX_CLIENTS];

//keeps track of the number of clients connected
static int clientCount = 0;

//generates unique ID for each client
static int genUID = 0;

//global declaration of endFlag - helps with cleanly closing sockets and ending the session
static int endFlag = 0;

//client mutex to facilitate multithreading
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

//global declaration of sockets
int	sock_listen, sock_recv;

//global declaration of the name of current spreadsheet
char nameOfSpreadsheet[50];

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

    //bonus features listing
    printf("\n\n ** BONUS FEATURES **\n\n");
    printf("    -- The first client can open previously saved spreadsheets or create a new one.\n");
    printf("    -- A menu is displayed to each client.\n");
    printf("    -- The first client sees a different menu from the rest of the clients.\n");
    printf("    -- Functions accept cell ranges that span across multiple rows and columns.\n");
    printf("    -- The first client can clear the contents of the current spreadsheet.\n");
    printf("    -- Clients can undo their most recent additions.\n");
    printf("    -- Each client can clear the contents of a particular cell.\n\n");
    printf("[+] Listening for clients...\n");

    //gets new spreadsheet
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

        //ensures the number of clients connected does not exceed limit
        if((clientCount + 1) > MAX_CLIENTS) {
            printf("\n[-] Max number of clients reached.\n");
            close(sock_recv);
            continue;
        }

        //creates and initializes a client
        client_t *client = (client_t *)malloc(sizeof(client_t));
        client->address = recv_addr;
        client->sockfd = sock_recv;
        client->uid = genUID;
        genUID++;

        //adds client to global client array
        addToClientArray(client);

        //creates a thread specifically for the client created that runs the handleClient function
        pthread_create(&tid, NULL, &handleClient, (void *)client);
        
    }
    
    return 0;
}

//This function handles the requests made by a clients connected to the server
//Each client will be on a unique thread that is running this function
void *handleClient(void *arg) {
    //declarations
    client_t *client = (client_t *)arg; //assigns client
    char buffer[BUFFER_SIZE], *cellAddr, *cellVal, details[90], message[100];
    int bytes_received, x, y;

    clearClientEditStack(client->uid); //empties the client's edit stack (for unddo feature)

    //retrieves and stores the client's display name
    bytes_received=recv(client->sockfd, buffer, BUFFER_SIZE,0);
    buffer[bytes_received] = 0;
    strcpy(client->name, buffer);

    clientCount++; //increments client count

    //prints update
    printf("\n[+] %s (client %d) connected successfully\n", client->name, client->uid);
    printf("[+] Total number of clients: %d\n", clientCount);


    if(client->uid == 0) { //if the current client is the first client...
        messageClient("first", client->uid); //lets the client know that they are the first

        bytes_received=recv(client->sockfd, buffer, BUFFER_SIZE,0);//waits on response from client

        getFileNames(); //gets the names of the files previously saved and sends them to the client

        //gets the file name that the client wants to open
        bytes_received=recv(client->sockfd, buffer, BUFFER_SIZE,0);
        buffer[bytes_received] = 0;

        char file[30];
        strcpy(file, buffer);
        loadFileToSpreadsheet(file); //load the file to the spreadsheet if it exists (if not then it creates the file and initializes the grid)

        strcpy(nameOfSpreadsheet, file);//stores the name of the file

        //prints update
        printf("\n[+] %s (client %d) opened the %s spreadsheet\n", client->name, client->uid, nameOfSpreadsheet);
    } else {
        messageClient("not_first", client->uid);//tells client they are not the first
    }
    sleep(0.5); //add delay to prevent messages from combining

    updateClientSpreadsheet(client->uid); //sends the updated spreadsheet details to the client

    //sends count update to the clients
    sprintf(message, "count:%d", clientCount);
    broadcastMessage(message);

    sleep(1);//add delay to prevent messages from combining

    //sends update to client
    sprintf(message, "update:[+] %s (client %d) joined the session", client->name, client->uid);
    broadcastMessageToAllExcept(message, client->uid);
    
    while (1) { //loop infinitely until break keyword is encountered
        //initialized coordinates
        x = 0;
        y = 0;
        
        //receive cell details (address:value)
        bytes_received=recv(client->sockfd, buffer, BUFFER_SIZE, 0);
        buffer[bytes_received]=0;

        if (strcmp(buffer, "shutdown") == 0) { //if client sent shutdown...
            if(client->uid == 0) { //if they are the first client...

                //prints update
                printf("\n[+] %s (client %d) ended the session\n", client->name, client->uid);
                
                //tells all clients to disconnect
                char endSessionMsg[11];
                strcpy(endSessionMsg, "endsession"); 
                broadcastMessage(endSessionMsg);
                endFlag = 1; //to end the while loop on the main thread and cleanly close sockets
            }
            break; //leave while loop

        } else if (strcmp(buffer, "clearSheet") == 0) { //if client sends clearSheet...
            if(client->uid == 0) { //if it is the first client...

                //prints update
                printf("\n[+] %s (client %d) cleared the spreadsheet\n", client->name, client->uid);

                //tells every client to clear their copy of the spreadsheet
                char clearMessage[11];
                strcpy(clearMessage, "clear");
                broadcastMessage(clearMessage);

                //clears server's spreadsheet
                getNewSpreadsheet();


                sleep(0.5); //adds delay to prevent messages from combining

                //sends update to clients
                sprintf(message, "update:[+] %s (client %d) cleared the spreadsheet", client->name, client->uid);
                broadcastMessageToAllExcept(message, client->uid);
            }
            continue; //go to next iteration

        } else if (strcmp(buffer, "undo") == 0){ //if client sent undo...

            char *coordinates = popFromClientEditStack(client->uid); //pop the last addition to their edit stack

            if(coordinates) { //if there was actually an edit found to undo...

                //undoes the edit on the server side
                char undoMessage[11];
                int xCoordinate = coordinates[0] - '0', yCoordinate = coordinates[1] - '0';
                undo(xCoordinate, yCoordinate);

                //tells all client to undo the edit
                strcpy(undoMessage, "undo:");
                strcat(undoMessage, coordinates);
                broadcastMessage(undoMessage);

                //prints update
                printf("\n[+] %s (client %d) undid their most recent edit\n", client->name, client->uid);

                sleep(0.5); //adds delay to prevent messages from combining
                
                //sends update to clients
                sprintf(message, "update:[+] %s (client %d) undid their most recent edit", client->name, client->uid);
                broadcastMessageToAllExcept(message, client->uid);

            
            }
            continue; //go to next iteration
        }
        
        //splits the contents of the buffer into two components (buffer received: '<info>:<info>')
        cellAddr = strtok(buffer, ":"); //first component - usually the cell address to edit
        cellVal = strtok(NULL, ":"); //second component - usually the value to add to the cell being edited

        //checks done when the first component isn't a cell address...
        if(strcmp(cellAddr, "saveSheet")==0){ //if the first component is 'saveSheet'...
            if(client->uid == 0) { //if the current client is the first client...

                //prints update
                printf("\n[+] %s (client %d) saved the spreadsheet\n", client->name, client->uid);
                
                sleep(0.5); //adds delay to prevent messages from combining

                //sends update to all clients
                sprintf(message, "update:[+] %s (client %d) saved the spreadsheet", client->name, client->uid);
                broadcastMessageToAllExcept(message, client->uid);
                saveWorksheet(cellVal);
            }
            continue; //go to next iteration
        }

        //otherwise...

        //prints update
        printf("[+] %s (client %d): %s -> %s\n", client->name, client->uid, cellVal, cellAddr);


        if((cellVal[0] == '=')) { //checking for the average function

            char *function = strtok(cellVal, "=");
            function = strtok(function, "(");
            if((strcmp(function, "average") == 0) || (strcmp(function, "AVERAGE") == 0)) { //case insensitive

                char *avgParam1 = strtok(NULL, "(");
                avgParam1 = strtok(avgParam1, ","); //first parameter stored here

                char *avgParam2 = strtok(NULL, ",");
                avgParam2[strlen(avgParam2)-1] = '\0'; //second parameter stored here

                if((strlen(avgParam1) != 2) || (strlen(avgParam2) != 2)) { //if cell addresses are invalid...
                    continue; //go to next iteration
                } else {
                    double resultAvg = average(avgParam1, avgParam2); //calculates average
                    sprintf(cellVal, "%.2lf", resultAvg); //updates cell value to the results
                }

            } else if((strcmp(function, "sum") == 0) || (strcmp(function, "SUM") == 0)) { //checking for the sum function
                
                char *sumParam1 = strtok(NULL, "(");
                sumParam1 = strtok(sumParam1, ","); //first parameter stored here

                char *sumParam2 = strtok(NULL, ",");
                sumParam2[strlen(sumParam2)-1] = '\0'; //second parameter stored here

                if((strlen(sumParam1) != 2) || (strlen(sumParam2) != 2)) { //if cell addresses are invalid...
                    continue; //skip iteration
                } else {
                    double resultSum = sum(sumParam1, sumParam2); //calulates sum
                    sprintf(cellVal, "%.2lf", resultSum); //updates cellVal
                }

            } else if((strcmp(function, "range") == 0) || (strcmp(function, "RANGE") == 0)) { //checking for the range function
                
                char *rngParam1 = strtok(NULL, "(");
                rngParam1 = strtok(rngParam1, ","); //first parameter stored here

                char *rngParam2 = strtok(NULL, ",");
                rngParam2[strlen(rngParam2)-1] = '\0'; //second parameter stored here

                if((strlen(rngParam1) != 2) || (strlen(rngParam2) != 2)) { //if cell addresses are invalid...
                    continue; //skip iteration
                } else {
                    double resultRng = range(rngParam1, rngParam2); //calculates results
                    sprintf(cellVal, "%.2lf", resultRng); //updates cellVal
                }

            }
        }

        
        if(isNumber(cellVal)) { //if cellVal is a number...
            double value = atof(cellVal);
            sprintf(cellVal, "%.2lf", value); //limits cellVal to 2 d.p.
        }
        
        if (strlen(cellAddr) == 2) { //if cell address is valid...
            
            //extracts coordinates...
            y = colLetterToNum(cellAddr[0]);
            x = cellAddr[1] - '0';

            if(validatePosition(x,y)) { //validates coordinates
                placeOnGrid(x, y, cellVal); //place on grid
            }
        }
        
        //broadcast cell details (address:value) to all clients
        char coordinates[] = {x + '0', y + '0', '\0'};
        strcpy(details, coordinates);
        strcat(details, ":");
        strcat(details, cellVal);
        broadcastMessage(details);
        pushToClientEditStack(client->uid, coordinates); //add edit to current client's edit stack

        sleep(0.5); //adds delays to prevent messages from combining

        //sends update to all clients
        sprintf(message, "update:[+] %s (client %d) updated spreadsheet with %s -> %s", client->name, client->uid, cellVal, cellAddr);
        broadcastMessageToAllExcept(message, client->uid);
        
    }
    //Outside the loop...
    //prints update
    printf("\n[-] %s (client %d) disconnected", client->name, client->uid);
    clientCount--;

    if(client->uid != 0) { //if current client is not the first client
        if(!endFlag) { //if session was not ended...

            //prints update
            printf("\n[-] Total number of clients: %d\n", clientCount);

            //sends count update to all clients
            sprintf(message, "count:%d", clientCount);
            broadcastMessage(message);

            sleep(0.5); //adds delay

            //sends update to all clients
            sprintf(message, "update:[+] %s (client %d) left the session", client->name, client->uid);
            broadcastMessageToAllExcept(message, client->uid);
        }
    } else { //if current client is the first client...

        while(clientCount > 0); //waits until all other clients have disconnected

        printf("\n[-] Total number of clients: %d\n", clientCount);
        removeFromClientArray(client->uid);
        free(client);

        pthread_detach(pthread_self());
        printf("\n[+] Server shutdown successfully\n\n");
        close(sock_listen);
        close(sock_recv);
        exit(0); //exits program
    }
    close(client->sockfd);
    removeFromClientArray(client->uid);
    free(client);
    pthread_detach(pthread_self());
}

//this function adds a client to the client array
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

//this function removes a client from the client array given their uid
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

//this function sends a message to the client that is associated with the given uid
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

//this function sends a message to all the clients in the client array
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

//this function sends a message to all clients in the client array except 
//to the one that is associated with the given uid
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

//this function sends all the details of the current spreadsheet to the client that
//is associated with the uid given
void updateClientSpreadsheet(int uid) {
    // sprintf(cellVal, "%.2lf", resultAvg);
    char coordinates[3], details[90], buffer[BUFFER_SIZE];
    int bytes_received, position = getPosition(uid);
    
    messageClient(nameOfSpreadsheet, uid);
    bytes_received=recv(clients[position]->sockfd, buffer, BUFFER_SIZE, 0);
    buffer[bytes_received]=0;

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

//this function clears the edit stack of the client associated with the uid given
void clearClientEditStack(int uid) {
    int position = getPosition(uid);

    for(int i = 0; i < EDIT_STACK_SIZE; i++) {
        clients[position]->editsStack[i] = NULL;
    }
}

//this function clears all clients' edit stacks
void clearAllClientsEditStacks() {
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(clients[i]) {
            clearClientEditStack(clients[i]->uid);
        }
    }
}

//this function push the coordinates of a cell on the spreadsheet to the 
//edit stack of the client associated with the given uid
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

//this function removes the last addition made to the edit stack
//of the client associated with the given uid
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

//this function replaces the contents at the specified 
//coordinates of the grid with an empty string
void undo(int x, int y) {
    placeOnGrid(x, y, " ");
}

//gets the index position of the client associated with the 
//given uid in the client array
int getPosition(int uid) {
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(clients[i]) {
            if(clients[i]->uid == uid) {
                return i;
            }
        }
    }
}

//this function clears all clients' edit stacks and assigns
//an empty string to all cells on the spreadsheet
void getNewSpreadsheet() {
    clearAllClientsEditStacks();

    for(int i = 0; i < NUM_RANGE; i++) {
        for(int j = 0; j < NUM_RANGE; j++) {
            grid[i][j] = malloc(sizeof(char *));
            strcpy(grid[i][j], " ");
        }
    }
}

//this function assigns the given string to the specified coordinates
//on the grid
void placeOnGrid(int x, int y, char* c){
    char *string = malloc(sizeof(char *));
    strcpy(string, c);
    grid[x-1][y-1]=string;
    return;
}

//this function checks if selected cell is empty - not used
int isEmptyCell(int x, int y){
    int flag = 0;
    if(strcmp(grid[x-1][y-1], " ") == 0){
        flag = 1;
    }
    return flag;
}

//this function checks if coordinates is on the grid
int validatePosition(int x, int y){
    if(((x < 1) || (x > NUM_RANGE)) || ((y < 1) || (y > NUM_RANGE))) {
        return 0;
    } else {
        return 1;
    }    
}

//this function takes a letter and returns its number equivalent
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

//this function checks if the contents of the given string is
//a number and returns 1 if true and 0 if false
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

//this function finds and returns the average of the values within
//the given cell range of the spreadsheet
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

//this function finds and returns the sum of the values within
//the given cell range of the spreadsheet
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

//this function finds and returns the range 
//(difference between the largest and smallest values) of the values within
//the given cell range of the spreadsheet
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

//this function writes the contents of the grid to a the specified file
//and adds the filename to the list of saved spreadsheets.
void saveWorksheet(char *filename){
    char file[30], fileName[30];
    FILE *fptr, *fptr2, *fptr3;// file pointers

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


    fptr2 = fopen("savedSpreadsheets.txt", "r");

    if (fptr2 == NULL) {
        printf("\n[-] 'savedSpreadsheets.txt' file not found\n");
    } else {
        fscanf(fptr2, "%s\n", fileName);
        while(!feof(fptr2)) {
            if(strcmp(filename, fileName) == 0) {
                fclose(fptr2);
                return;
            }
            fscanf(fptr2, "%s\n", fileName);
        }
        if(strcmp(filename, fileName) == 0) {
            fclose(fptr2);
            return;
        }
    }
    fclose(fptr2);


    fptr3 = fopen("savedSpreadsheets.txt", "a");
    if (fptr3 == NULL) {
        printf("\n[-] 'savedSpreadsheets.txt' file not found\n");
    } else {
        fprintf(fptr3,"%s\n", filename);
    }
    fclose(fptr3);
}

//this function gets the list of saved spreadsheets and
//sends them to the first client
void getFileNames() {
    FILE *fptr;
    char fileName[20];
    char message[200];

    fptr = fopen("savedSpreadsheets.txt", "r");

    int i = 0;
    if (fptr == NULL) {
        printf("\n[-] 'savedSpreadsheets.txt' file not found\n");
    } else {
        // strcpy(message, "files:");
        message[0] = '\0';

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

//this function loads the content of the given to the current spreadsheet
void loadFileToSpreadsheet(char *file) {
    char filename[30], line[100], *content, *coords;
    FILE *fptr;// file pointers
    int x, y;

    strcpy(filename, file);
    strcat(filename, ".txt");

    getNewSpreadsheet();

    fptr=fopen(filename,"r");
    if(fptr==NULL){
        printf("\n[+]Creating %s file\n", filename);
        fptr=fopen(filename,"w");
        fclose(fptr);
        return;
    }

    fscanf(fptr, "%s\n", line);

    coords = strtok(line, ":");
    content = strtok(NULL, ":");
    x = (coords[0] - '0') + 1;
    y = (coords[1] - '0') + 1;

    placeOnGrid(x, y, content);
    while(!feof(fptr)) {       

        fscanf(fptr, "%s\n", line);
    
        coords = strtok(line, ":");
        content = strtok(NULL, ":");
        x = (coords[0] - '0') + 1;
        y = (coords[1] - '0') + 1;

        placeOnGrid(x, y, content);

    }
    fclose(fptr);
}