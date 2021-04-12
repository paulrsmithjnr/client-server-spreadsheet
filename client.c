#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>


#define BUFFER_SIZE	1024
#define	SERVER_IP	"127.0.0.1"
#define SERVER_PORT	2124
#define NUM_RANGE 9

//function declarations
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

//Display name
char name[20];
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

    int send_len=strlen(name);
    int bytes_sent = send(sock_send, name , send_len, 0);

    char buffer[BUFFER_SIZE];
    int bytes_received = recv(sock_send, buffer, BUFFER_SIZE, 0);
    buffer[bytes_received] = 0;

    if (strcmp(buffer, "first") == 0) {
        isFirstClient = 1;

        strcpy(buffer, "received");
        send_len = strlen("received");
        bytes_sent = send(sock_send, buffer, send_len, 0);

        bytes_received = recv(sock_send, buffer, BUFFER_SIZE, 0);
        buffer[bytes_received] = 0;

        char fileName[50];
        char *file = strtok(buffer, ":");

        printf("\n\nAvailable files:\n\n");
        while(file != NULL) {
            printf("\t** %s\n", file);
            file = strtok(NULL, ":");
        }

        printf("\nEnter the name of the file you would like to open\n(If you would like to create a new spreadsheet, enter the name to assign to it): ");
        scanf("%s", fileName);
        strcpy(buffer, fileName);
        send_len = strlen(fileName);
        bytes_sent = send(sock_send, buffer, send_len, 0);

    }

    getNewSpreadsheet();
    receiveUpdates();
    // drawSpreadsheet();

    pthread_t sendThread;
    int sendVal = pthread_create(&sendThread, NULL, (void *) sendToServer, NULL);
    if(sendVal != 0){
		printf("\n[-] Error setting up sending thread\n");
        return -1;
	}

	pthread_t receiveThread;
    int receiveVal = pthread_create(&receiveThread, NULL, (void *) receiveFromServer, NULL);
    if(receiveVal != 0){
        printf("\n[-] Error setting up receiving thread\n");
        return -1;
    }

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

void receiveFromServer() {
    char buffer[BUFFER_SIZE], *addr, *val;
    int bytes_received;

    //receive broadcasted cell details from server
    while(1) {
        bytes_received=recv(sock_send, buffer, BUFFER_SIZE,0);
        buffer[bytes_received]=0;
        
        if(strcmp(buffer, "endsession") == 0) {
            printf("\n\n[+] The session was ended\n");
            strcpy(buffer, "shutdown");
            int send_len = strlen("shutdown");
            int bytes_sent = send(sock_send, buffer, send_len, 0);
            endFlag = 1;
            break;
        } else if (strcmp(buffer, "clear") == 0) {
            getNewSpreadsheet();
            drawSpreadsheet();
            printPrompt();
            continue;
        }

        addr = strtok(buffer, ":");
        val = strtok(NULL, ":");

        if(strcmp(addr, "update") == 0) {
            drawSpreadsheet();
            printf("\n%s\n", val);
            printPrompt();
            continue;
        } else if(strcmp(addr, "count") == 0){
            clientCount = atoi(val);
            // drawSpreadsheet();
            // printPrompt();
            continue;
        } else if(strcmp(addr, "undo") == 0){
            int xCoordinate = val[0] - '0';
            int yCoordinate = val[1] - '0';
            placeOnGrid(xCoordinate, yCoordinate, " ");

            sleep(1);
            drawSpreadsheet();
            printPrompt();
            continue;
        } 

        int x = addr[0] - '0';
        int y = addr[1] - '0';
        placeOnGrid(x, y, val);
        drawSpreadsheet();
        printPrompt();

        if(endFlag) {
            break;
        }
    }
}

void sendToServer() {
    int	send_len, bytes_sent;
    char buffer[BUFFER_SIZE], cellAddr[4], cellVal[80], details[90], menuResponse[2];

    printf("[+] Loading spreadsheet...\n");
    sleep(1); //wait for count to be updated
    drawSpreadsheet();
    while(1) {
        if(endFlag) {
            break;
        }

        printPrompt();
        scanf("%s", menuResponse);

        if (isFirstClient) {
            char filename[20];
            char info[30];

            switch(menuResponse[0]){
                case '1':
                    strcpy(buffer, "clearSheet");
                    send_len = strlen("clearSheet");
                    bytes_sent = send(sock_send, buffer, send_len, 0);
                    break;
                
                case '2':
                    // printf("\nPlease enter the name you would like to save \nthe spreadsheet as (without file extension): ");
                    // scanf("%s", filename);
                    strcpy(info, "saveSheet:");
                    strcat(info, nameOfSpreadsheet);
                    strcpy(buffer,info);
                    send_len = strlen(info);
                    bytes_sent = send(sock_send, buffer, send_len, 0);
                    drawSpreadsheet();
                    printf("\n[+] You have saved the spreadsheet\n");
                    break;

                case '3': 
                    atMenu = 0;

                    printPrompt();
                    scanf("%s",cellAddr);

                    strcpy(promptInput, cellAddr);
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

                case '4':
                    strcpy(buffer, "undo");
                    send_len = strlen("undo");
                    bytes_sent = send(sock_send, buffer, send_len, 0);
                    drawSpreadsheet();
                    break;

                case '5':

                    printf("\nPlease enter the cell address you want to clear: ");
                    scanf("%s",cellAddr);

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
                    strcpy(buffer, "shutdown");
                    send_len = strlen("shutdown");
                    bytes_sent = send(sock_send, buffer, send_len, 0);
                    endFlag = 1;
                    break;

                default:
                    drawSpreadsheet();
                    printf("\n[-] Invalid response! Try again\n");
                    break;
            }

        } else {

            switch(menuResponse[0]){
                case '1': 
                    atMenu = 0;

                    printPrompt();
                    scanf("%s",cellAddr);

                    strcpy(promptInput, cellAddr);
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
                    strcpy(buffer, "undo");
                    send_len = strlen("undo");
                    bytes_sent = send(sock_send, buffer, send_len, 0);
                    sleep(0.5);
                    drawSpreadsheet();
                    break;

                case '3':
                    printf("\nPlease enter the cell address you want to clear: ");
                    scanf("%s",cellAddr);

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
                    strcpy(buffer, "shutdown");
                    send_len = strlen("shutdown");
                    bytes_sent = send(sock_send, buffer, send_len, 0);
                    endFlag = 1;
                    break;

                default:
                    drawSpreadsheet();
                    printf("\n[-] Invalid response! Try again\n");
                    break;
            }
        }
        
        ////////////////////////////////////
    }
}

void receiveUpdates() {
    char buffer[BUFFER_SIZE], *addr, *val;
    int bytes_received;

    bytes_received=recv(sock_send, buffer, BUFFER_SIZE,0);
    buffer[bytes_received]=0;
    strcpy(nameOfSpreadsheet, buffer);

    send(sock_send, "Received", strlen("Received"), 0);

    //receive broadcasted cell details from server
    while(1) {
        memset(&buffer, 0, sizeof(buffer));
        bytes_received=recv(sock_send, buffer, BUFFER_SIZE,0);
        buffer[bytes_received]=0;

        if(strcmp(buffer, "Done") == 0) {
            break;
        }

        // printf("Update received: %s\n", buffer);
        addr = strtok(buffer, ":");
        val = strtok(NULL, ":");

        int x = addr[0] - '0';
        int y = addr[1] - '0';
        placeOnGrid(x, y, val);
        
        send(sock_send, "Received", strlen("Received"), 0);
    }
}

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

void getNewSpreadsheet() {
    for(int i = 0; i < NUM_RANGE; i++) {
        for(int j = 0; j < NUM_RANGE; j++) {
            grid[i][j] = malloc(sizeof(char *));
            strcpy(grid[i][j], " ");
        }
    }
}

void drawSpreadsheet() {
    char * const COLUMN_HEADINGS = "      A        B        C        D        E        F        G        H        I";
    char * const HORIZONTAL_LINE = "  +--------+--------+--------+--------+--------+--------+--------+--------+--------+";
    int const cellWidthPaddingVal = 8;
    
    for(int i = 0; i < 40; i++) {
        printf("\n");
    }
    printf("\nSPREADSHEET TITLE: %s\n\n", nameOfSpreadsheet);
    printf("%s\n", COLUMN_HEADINGS);
    for(int i = 0; i < NUM_RANGE; i++) {
        printf("%s\n", HORIZONTAL_LINE);
        printf("%d ", i+1);
        for(int j = 0; j < NUM_RANGE; j++) {
            if(strcmp(grid[i][j], " ") == 0) {
                printf("|        ");
            } else {
                printf("|");
                int len = strlen(grid[i][j]);
                if(len > cellWidthPaddingVal) {

                    //cell content truncation
                    for(int l = 0; l < cellWidthPaddingVal - 3; l++) {
                        printf("%c", grid[i][j][l]);
                    }
                    printf("...");

                } else {
                    printf("%s", grid[i][j]);
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

void placeOnGrid(int x, int y, char* c){
    char *string = malloc(sizeof(char *));
    strcpy(string, c);
    grid[x-1][y-1]=string;
    return;
}

void displayMenu() {
    printf("\n\n****************    Hi, %s!   ****************\n\n", name);
    printf("Please enter the number that corresponds with your choice:\n\n");
    printf("\t(1) Update the spreadsheet\n");
    printf("\t(2) Undo your most recent addition\n");
    printf("\t(3) Clear cell content\n");
    printf("\t(4) Leave session\n\n");
    printf("Choice: ");
}

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