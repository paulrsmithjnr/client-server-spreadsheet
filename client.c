#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>


#define BUF_SIZE	1024
#define	SERVER_IP	"127.0.0.1"
#define SERVER_PORT	2121
#define NUM_RANGE 9

//function declarations
void drawSpreadsheet();
void getNewSpreadsheet();
int colLetterToNum(char letter);
void placeOnGrid(int x, int y, char* c);

//global declaration structure grid
char * grid[NUM_RANGE][NUM_RANGE];

int main() {
    int	sock_send;
    struct sockaddr_in	addr_send;
    int	i;
    char cellAddr[4], *addr, *val, cellVal[80], details[90], buf[BUF_SIZE];
    int	send_len,bytes_sent, bytes_received;

        /* create socket for sending data */
    sock_send=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_send < 0){
        printf("socket() failed\n");
        exit(0);
    }

        /* create socket address structure to connect to */
    memset(&addr_send, 0, sizeof (addr_send)); /* zero out structure */
    addr_send.sin_family = AF_INET; /* address family */
    addr_send.sin_addr.s_addr = inet_addr(SERVER_IP);
    addr_send.sin_port = htons((unsigned short)SERVER_PORT);

        /* connect to the server */
    i=connect(sock_send, (struct sockaddr *) &addr_send, sizeof (addr_send));
    if (i < 0){
        printf("connect() failed\n");
        exit(0);
    }
    getNewSpreadsheet();
    drawSpreadsheet();

    int flag=1;
    while (flag){
        /* send some data */
        printf("\nEnter the cell address you would like to edit (Please enter quit to leave): ");
        scanf("%s",cellAddr);
        if (strcmp(cellAddr,"quit") == 0){
            printf("Bye!!!\n");
            strcpy(buf,"shutdown");
            send_len=strlen("shutdown");
            bytes_sent=send(sock_send,buf,send_len,0);
            flag=0;
            break;
        } else if(strlen(cellAddr) != 2) {
            printf("\n ** ERROR: Invalid cell address **\n");
            continue;
        } else if((isalpha(cellAddr[0]) == 0) || isdigit(cellAddr[1]) == 0) {
            printf("\n ** ERROR: Invalid cell address **\n");
            continue;
        }

        printf("Enter value to input into the selected cell: ");
        scanf("%s", cellVal);

        //send cell details (address:value) to server
        strcpy(details, cellAddr);
        strcat(details, ":");
        strcat(details, cellVal);
        strcpy(buf, details);
        send_len=strlen(details);
        bytes_sent=send(sock_send,buf,send_len,0);

        //receive broadcasted cell details from server
        bytes_received=recv(sock_send,buf,BUF_SIZE,0);
        buf[bytes_received]=0;
        addr = strtok(buf, ":");
        val = strtok(NULL, ":");

        int x = addr[0] - '0';
        int y = addr[1] - '0';
        if((x == 0) || (y == 0)) {
            printf("\n ** ERROR: Invalid request (ignored) **\n");
            continue;
        }
        placeOnGrid(x, y, val);
        drawSpreadsheet();
    }

    close(sock_send);
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

void drawSpreadsheet() {
    char * const COLUMN_HEADINGS = "      A        B        C        D        E        F        G        H        I";
    char * const HORIZONTAL_LINE = "  +--------+--------+--------+--------+--------+--------+--------+--------+--------+";
    int const cellWidthPaddingVal = 8;

    printf("\n\n%s\n", COLUMN_HEADINGS);
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