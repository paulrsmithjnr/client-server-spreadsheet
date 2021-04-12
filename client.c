/*
COMP2130 Final Project

GROUP MEMBER            ID Number
Javon Ellis         -   620126389
Monique Satchwell   -   620119851
Paul Smith          -   620118115

Command to run program: gcc -o client client.c -lpthread
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>

//Defintion of constants
#define BUFFER_SIZE	1024
#define	SERVER_IP	"127.0.0.1"
#define SERVER_PORT	2121
#define NUM_RANGE 9

//function declarations - explanations for each are above each function definition below
void receiveFromServer();
void sendToServer();
void receiveUpdates();
void printPrompt();
void drawSpreadsheet();
void getNewSpreadsheet();
int colLetterToNum(char letter);
void placeOnGrid(int x, int y, char* c);
void displayMenu();
void displayFirstClientMenu();
void receiveNewSheet();

//global declaration structure grid
char * grid[NUM_RANGE][NUM_RANGE];

//global socket declaration
int sock_send;

//global variables to keep prompt state
int atMenu = 1, promptNo = 0;
char promptInput[4];

//global declaration of flag
static volatile int endFlag = 0;

char name[20];//display name

static int isFirstClient = 0;

char nameOfSpreadsheet[50];

static int clientCount = 0;

int main() {
    struct sockaddr_in	addr_send;
    int	i;

        /* create socket for sending data */
    sock_send=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_send < 0){
        printf("[-] Failed to create socket\n");
        exit(0);
    }
    printf("[+] Socket created\n");

        /* create socket address structure to connect to */
    memset(&addr_send, 0, sizeof (addr_send)); /* zero out structure */
    addr_send.sin_family = AF_INET; /* address family */
    addr_send.sin_addr.s_addr = inet_addr(SERVER_IP);
    addr_send.sin_port = htons((unsigned short)SERVER_PORT);

        /* connect to the server */
    i=connect(sock_send, (struct sockaddr *) &addr_send, sizeof (addr_send));
    if (i < 0){
        printf("[-] Failed to connect to the server\n");
        exit(0);
    }

    printf("[+] Successfully connected to the server\n");
    printf("Enter your display name: ");
    scanf("%s", name);

    //sends display name to the server
    int send_len=strlen(name);
    int bytes_sent = send(sock_send, name , send_len, 0);

    //receives info on whether or not the client is the first client
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(sock_send, buffer, BUFFER_SIZE, 0);
    buffer[bytes_received] = 0;

    if (strcmp(buffer, "first") == 0) { //if this client is the first client...
        isFirstClient = 1; //make it known globally that this is the first client

        //sends response to server
        strcpy(buffer, "received");
        send_len = strlen("received");
        bytes_sent = send(sock_send, buffer, send_len, 0);

        //receives the names of the previously stored files that are available to open
        bytes_received = recv(sock_send, buffer, BUFFER_SIZE, 0);
        buffer[bytes_received] = 0;

        char fileName[50]; //to store client input

        //prints all file names
        char *file = strtok(buffer, ":");
        printf("\n\nAvailable files:\n\n");
        while(file != NULL) {
            printf("\t** %s\n", file);
            file = strtok(NULL, ":");
        }

        printf("\nEnter the name of the file you would like to open\n(If you would like to create a new spreadsheet, enter the name to assign to it): ");
        scanf("%s", fileName);

        //sends the name of the file the client wants to open to the server
        strcpy(buffer, fileName);
        send_len = strlen(fileName);
        bytes_sent = send(sock_send, buffer, send_len, 0);

    }

    getNewSpreadsheet(); //refreshes spreadsheet
    receiveUpdates(); //receive all details of the opened spreadsheet from the server

    //Creates a thread that is responsible for send request to the server
    pthread_t sendThread;
    int sendVal = pthread_create(&sendThread, NULL, (void *) sendToServer, NULL);
    if(sendVal != 0){
		printf("\n[-] Error setting up sending thread\n");
        return -1;
	}

    //Creates a thread that is responsible for receiving information from the server
	pthread_t receiveThread;
    int receiveVal = pthread_create(&receiveThread, NULL, (void *) receiveFromServer, NULL);
    if(receiveVal != 0){
        printf("\n[-] Error setting up receiving thread\n");
        return -1;
    }

    //loop continuously until program ends (keeps session alive until the first client quits)
    while (1){
        if(endFlag) {
            break;
        }
    }
    printf("[+] Disconnected from the server\n");
    close(sock_send);
    printf("[+] Socket closed...Bye!\n");
    return 0;
}

//this function runs on a separate thread and is responsible for
//receiving information from the server and taking the appropriate
//course of action
void receiveFromServer() {
    char buffer[BUFFER_SIZE], *addr, *val;
    int bytes_received;

    while(1) {
        //receives information from the server
        bytes_received=recv(sock_send, buffer, BUFFER_SIZE,0);
        buffer[bytes_received]=0;
        
        if(strcmp(buffer, "endsession") == 0) { //if the server sends endsession...
            printf("\n\n[+] The session was ended\n");

            //sends request to disconnect from the server
            strcpy(buffer, "shutdown");
            int send_len = strlen("shutdown");
            int bytes_sent = send(sock_send, buffer, send_len, 0);

            endFlag = 1;//ends program
            break;
        } else if (strcmp(buffer, "clear") == 0) { //if the server sends clear...
            //refreshes and redisplays the spreadsheet
            getNewSpreadsheet();
            drawSpreadsheet();
            printPrompt();
            continue;
        }

        //otherwise...
        //server sends cell edit details (<coordinates>:<cell_value>)

        //splits up and assigns cell edit details appropriately
        addr = strtok(buffer, ":");
        val = strtok(NULL, ":");

        //checks in case addr doesn't actually contain coordinates
        if(strcmp(addr, "update") == 0) { //if server sends an update...
            drawSpreadsheet(); //redraws spreadsheet
            printf("\n%s\n", val); //prints update from server
            printPrompt(); //prints prompt
            continue;

        } else if(strcmp(addr, "count") == 0){ //if server sends count...
            clientCount = atoi(val); //updates the current count value
            continue;

        } else if(strcmp(addr, "undo") == 0){ //if server sends undo...

            //gets coordinates and assign appropriately
            int xCoordinate = val[0] - '0';
            int yCoordinate = val[1] - '0';

            placeOnGrid(xCoordinate, yCoordinate, " "); //place an empty string at specified coordinates

            sleep(1);//adds a slight delay

            //redisplays spreadsheet and prompt
            drawSpreadsheet();
            printPrompt();
            continue;
        } 

        //otherwise
        //splits and assigns coordinates appropriately
        int x = addr[0] - '0';
        int y = addr[1] - '0';

        placeOnGrid(x, y, val); //add content to specified coordinates

        //redisplay spreadsheet and prompt
        drawSpreadsheet();
        printPrompt();

        if(endFlag) { //if session was ended....
            break; //leave while loop
        }
    }
}

//this function runs on a separate thread and is responsible for
//sending requests to the server
void sendToServer() {
    int	send_len, bytes_sent;
    char buffer[BUFFER_SIZE], cellAddr[4], cellVal[80], details[90], menuResponse[2];

    printf("[+] Loading spreadsheet...\n");
    sleep(1); //wait for count to be updated
    drawSpreadsheet();
    
    //this block of code makes an initial save of the current spreadsheet if this is the first client
    char fistSave[50];
    if(isFirstClient) {
        strcpy(fistSave, "saveSheet:");
        strcat(fistSave, nameOfSpreadsheet);
        strcpy(buffer,fistSave);
        send_len = strlen(fistSave);
        bytes_sent = send(sock_send, buffer, send_len, 0);
    }


    while(1) { //loop continuously until session has ended...
        if(endFlag) { //if session was ended...
            break; //leave while loop
        }

        //prints prompt and waits for response...
        printPrompt();
        scanf("%s", menuResponse);

        //this block of code is reponsible for accepting menu responses based on whether or not this is the first client
        if (isFirstClient) { //if this is the first client...

            //declarations
            char filename[20];
            char info[30];

            switch(menuResponse[0]){
                case '1':
                    //sends request to the server to clear the spreadsheet
                    strcpy(buffer, "clearSheet");
                    send_len = strlen("clearSheet");
                    bytes_sent = send(sock_send, buffer, send_len, 0);
                    break;
                
                case '2':
                    //sends request to the server to save the spreadsheet
                    strcpy(info, "saveSheet:");
                    strcat(info, nameOfSpreadsheet);
                    strcpy(buffer,info);
                    send_len = strlen(info);
                    bytes_sent = send(sock_send, buffer, send_len, 0);
                    drawSpreadsheet();
                    printf("\n[+] You have saved the spreadsheet\n");
                    break;

                case '3': 
                    atMenu = 0; //helps with saving the current prompt state (in case of redisplays)

                    
                    printPrompt();//prints appropriate prompt based on state
                    scanf("%s",cellAddr); //gets cell address


                    strcpy(promptInput, cellAddr);//saves input state

                    //validates cell address
                    if(strlen(cellAddr) != 2) {
                        drawSpreadsheet();
                        printf("\n[-] ERROR: Invalid cell address \n");

                        atMenu = 1;
                        break;
                    } else if((isalpha(cellAddr[0]) == 0) || isdigit(cellAddr[1]) == 0) {
                        drawSpreadsheet();
                        printf("\n[-] ERROR: Invalid cell address \n");

                        atMenu = 1;
                        break;
                    } else if(!( (cellAddr[0] >= 65 && cellAddr[0] <= 73) || (cellAddr[0] >= 97 && cellAddr[0] <= 105) )) {
                        drawSpreadsheet();
                        printf("\n[-] ERROR: Invalid cell address \n");

                        atMenu = 1;
                        break;
                    }
                    promptNo = 1;

                    //gets cell value from user
                    printf("Enter value to input into the selected cell: ");
                    scanf("%s", cellVal);

                    //send cell details (address:value) to server
                    strcpy(details, cellAddr);
                    strcat(details, ":");
                    strcat(details, cellVal);
                    strcpy(buffer, details);
                    send_len=strlen(details);
                    bytes_sent=send(sock_send,buffer,send_len,0);

                    //refreshes prompt state
                    promptNo = 0;
                    promptInput[0] = '\0';
                    atMenu = 1;
                    break;

                case '4':
                    //sends request to the server to undo last addition made to the spreadsheet
                    strcpy(buffer, "undo");
                    send_len = strlen("undo");
                    bytes_sent = send(sock_send, buffer, send_len, 0);
                    drawSpreadsheet();
                    break;

                case '5':
                    //gets address of cell to clear
                    printf("\nPlease enter the cell address you want to clear: ");
                    scanf("%s",cellAddr);

                    //validates cell address
                    if(strlen(cellAddr) != 2) {
                        drawSpreadsheet();
                        printf("\n[-] ERROR: Invalid cell address \n");

                        atMenu = 1;
                        break;
                    } else if((isalpha(cellAddr[0]) == 0) || isdigit(cellAddr[1]) == 0) {
                        drawSpreadsheet();
                        printf("\n[-] ERROR: Invalid cell address \n");

                        atMenu = 1;
                        break;
                    } else if(!( (cellAddr[0] >= 65 && cellAddr[0] <= 73) || (cellAddr[0] >= 97 && cellAddr[0] <= 105) )) {
                        drawSpreadsheet();
                        printf("\n[-] ERROR: Invalid cell address \n");

                        atMenu = 1;
                        break;
                    }

                    //send cell details (address:value) to server
                    strcpy(details, cellAddr);
                    strcat(details, ": ");
                    strcpy(buffer, details);
                    send_len=strlen(details);
                    bytes_sent=send(sock_send,buffer,send_len,0);

                    atMenu = 1;
                    break;

                case '6':
                    //sends request to end session and disconnect from the server
                    strcpy(buffer, "shutdown");
                    send_len = strlen("shutdown");
                    bytes_sent = send(sock_send, buffer, send_len, 0);
                    endFlag = 1;
                    break;

                default:
                    //default case for error trapping
                    drawSpreadsheet();
                    printf("\n[-] Invalid response! Try again\n");
                    break;
            }

        } else { //else if this is not the first client...

            switch(menuResponse[0]){
                case '1': 
                    atMenu = 0;

                    //gets cell address
                    printPrompt();//prints appropriate prompt based on state
                    scanf("%s",cellAddr);

                    strcpy(promptInput, cellAddr); //saves input

                    ///validates cell address
                    if(strlen(cellAddr) != 2) {
                        drawSpreadsheet();
                        printf("\n[-] ERROR: Invalid cell address \n");

                        atMenu = 1;
                        break;
                    } else if((isalpha(cellAddr[0]) == 0) || isdigit(cellAddr[1]) == 0) {
                        drawSpreadsheet();
                        printf("\n[-] ERROR: Invalid cell address \n");

                        atMenu = 1;
                        break;
                    } else if(!( (cellAddr[0] >= 65 && cellAddr[0] <= 73) || (cellAddr[0] >= 97 && cellAddr[0] <= 105) )) {
                        drawSpreadsheet();
                        printf("\n[-] ERROR: Invalid cell address \n");

                        atMenu = 1;
                        break;
                    }
                    promptNo = 1;

                    //gets value
                    printf("Enter value to input into the selected cell: ");
                    scanf("%s", cellVal);

                    //send cell details (address:value) to server
                    strcpy(details, cellAddr);
                    strcat(details, ":");
                    strcat(details, cellVal);
                    strcpy(buffer, details);
                    send_len=strlen(details);
                    bytes_sent=send(sock_send,buffer,send_len,0);

                    promptNo = 0;
                    promptInput[0] = '\0';

                    atMenu = 1;
                    break;

                case '2':
                    //sends request to server to undo last addition made to the spreadsheet
                    strcpy(buffer, "undo");
                    send_len = strlen("undo");
                    bytes_sent = send(sock_send, buffer, send_len, 0);
                    sleep(0.5);
                    drawSpreadsheet();
                    break;

                case '3':
                    //gets cell address to be cleared
                    printf("\nPlease enter the cell address you want to clear: ");
                    scanf("%s",cellAddr);

                    //validates cell address
                    if(strlen(cellAddr) != 2) {
                        drawSpreadsheet();
                        printf("\n[-] ERROR: Invalid cell address \n");

                        atMenu = 1;
                        break;
                    } else if((isalpha(cellAddr[0]) == 0) || isdigit(cellAddr[1]) == 0) {
                        drawSpreadsheet();
                        printf("\n[-] ERROR: Invalid cell address \n");

                        atMenu = 1;
                        break;
                    } else if(!( (cellAddr[0] >= 65 && cellAddr[0] <= 73) || (cellAddr[0] >= 97 && cellAddr[0] <= 105) )) {
                        drawSpreadsheet();
                        printf("\n[-] ERROR: Invalid cell address \n");

                        atMenu = 1;
                        break;
                    }

                    //send cell details (address:value) to server
                    strcpy(details, cellAddr);
                    strcat(details, ": ");
                    strcpy(buffer, details);
                    send_len=strlen(details);
                    bytes_sent=send(sock_send,buffer,send_len,0);

                    atMenu = 1;
                    break;

                case '4':
                    //sends request to the server to leave session and disconnect
                    strcpy(buffer, "shutdown");
                    send_len = strlen("shutdown");
                    bytes_sent = send(sock_send, buffer, send_len, 0);
                    endFlag = 1;
                    break;

                default:
                    //default case for error trapping
                    drawSpreadsheet();
                    printf("\n[-] Invalid response! Try again\n");
                    break;
            }
        }
        
        ////////////////////////////////////
    }
}

//this function is responsible for receiving all the details of the current spreadsheet
//and making the necessary updates
void receiveUpdates() {
    char buffer[BUFFER_SIZE], *addr, *val;
    int bytes_received;

    //receives name of spreadsheet from the server and makes assignment
    bytes_received=recv(sock_send, buffer, BUFFER_SIZE,0);
    buffer[bytes_received]=0;
    strcpy(nameOfSpreadsheet, buffer);

    send(sock_send, "Received", strlen("Received"), 0); //sends response to the server

    //receive broadcasted cell details from server
    while(1) {

        //receives cell details
        memset(&buffer, 0, sizeof(buffer));
        bytes_received=recv(sock_send, buffer, BUFFER_SIZE,0);
        buffer[bytes_received]=0;

        if(strcmp(buffer, "Done") == 0) { //if server sends Done...
            break; //leave while loop
        }

        //splits cell information and assign appropriately
        addr = strtok(buffer, ":");
        val = strtok(NULL, ":");

        //splits coordinates and assign appropriately
        int x = addr[0] - '0';
        int y = addr[1] - '0';

        //update cell on spreadsheet
        placeOnGrid(x, y, val);
        
        send(sock_send, "Received", strlen("Received"), 0);//send response
    }
}

//this function is responsible for printing the appropriate prompts based on the current
//state of the program
void printPrompt() {
    if(atMenu) {
        if(isFirstClient) {
            printf("Total number of clients online: %d", clientCount);
            displayFirstClientMenu();
        } else {
            printf("Total number of clients online: %d", clientCount);
            displayMenu();
        }
        
    } else if(promptNo) {
        printf("\nEnter the cell address you would like to edit: %s\n", promptInput);
        printf("Enter value to input into the selected cell: ");
    } else {
        printf("\nEnter the cell address you would like to edit: ");
    }
    fflush(stdout);
}

//this function is responsible for refreshing the spreadsheet
//ie. assigns empty string to all cells
void getNewSpreadsheet() {
    for(int i = 0; i < NUM_RANGE; i++) {
        for(int j = 0; j < NUM_RANGE; j++) {
            grid[i][j] = malloc(sizeof(char *));
            strcpy(grid[i][j], " ");
        }
    }
}

//this function is responsible for visually displaying the current state
//of the spreadsheet
void drawSpreadsheet() {

    char * const COLUMN_HEADINGS = "      A        B        C        D        E        F        G        H        I";
    char * const HORIZONTAL_LINE = "  +--------+--------+--------+--------+--------+--------+--------+--------+--------+";
    int const cellWidthPaddingVal = 8;
    
    //clears screen
    for(int i = 0; i < 40; i++) {
        printf("\n");
    }

    printf("\nSPREADSHEET TITLE: %s\n\n", nameOfSpreadsheet);


    printf("%s\n", COLUMN_HEADINGS); //prints column letters
    for(int i = 0; i < NUM_RANGE; i++) {

        printf("%s\n", HORIZONTAL_LINE);
        printf("%d ", i+1); //prints row numbers

        for(int j = 0; j < NUM_RANGE; j++) {

            if(strcmp(grid[i][j], " ") == 0) { //if current cell is empty...
                printf("|        ");
            } else {

                printf("|");

                int len = strlen(grid[i][j]);
                if(len > cellWidthPaddingVal) { //if length of cell content is larger than the width of the cell...

                    //cell content truncation
                    for(int l = 0; l < cellWidthPaddingVal - 3; l++) {
                        printf("%c", grid[i][j][l]);
                    }
                    printf("...");

                } else {

                    printf("%s", grid[i][j]);

                    //adds padding to the cell
                    for(int k = 0; k < cellWidthPaddingVal - strlen(grid[i][j]); k++) {
                        printf(" ");
                    }

                }
            }
        }
        printf("|\n");
    }
    printf("%s\n", HORIZONTAL_LINE);
}

//this function is responsible for placing the given string
//at the given coordinates on the spreadsheets
void placeOnGrid(int x, int y, char* c){
    char *string = malloc(sizeof(char *));
    strcpy(string, c);
    grid[x-1][y-1]=string;
    return;
}

//this function displays the menu for the ordinary client
void displayMenu() {
    printf("\n\n****************    Hi, %s!   ****************\n\n", name);
    printf("Please enter the number that corresponds with your choice:\n\n");
    printf("\t(1) Update the spreadsheet\n");
    printf("\t(2) Undo your most recent addition\n");
    printf("\t(3) Clear cell content\n");
    printf("\t(4) Leave session\n\n");
    printf("Choice: ");
}

//this function displays the menu for the first client
void displayFirstClientMenu() {
    printf("\n\n****************    Hi, %s!   ****************\n\n", name);
    printf("Please enter the number that corresponds with your choice:\n\n");
    printf("\t(1) Clear spreadsheet\n");
    printf("\t(2) Save spreadsheet\n");
    printf("\t(3) Update spreadsheet\n");
    printf("\t(4) Undo your most recent addition to spreadsheet\n");
    printf("\t(5) Clear cell content\n");
    printf("\t(6) End session\n\n");
    printf("Choice: ");
}