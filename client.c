#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>



#define BUF_SIZE	1024
#define	SERVER_IP	"127.0.0.1"
#define SERVER_PORT	60000

void createSpreadsheet();



void clientMenu();

int main() {
    int	sock_send;
    struct sockaddr_in	addr_send;
    int	i;



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
    // int flag=1;
    // while (flag){
    //     /* send some data */
    //     printf("Enter the cell address you would like to edit (Please enter quit to leave): ");
    //     scanf("%s",text);
    //     if (strcmp(text,"quit") == 0){
    //         // strcpy(buf,text);
    //         printf("Bye!!!\n");
    //         strcpy(buf,"shutdown");
    //         send_len=strlen(text);
    //         bytes_sent=send(sock_send,buf,send_len,0);
    //         flag=0;
    //         break;
    //     }
    //     createSpreadsheet();
        
    // }


    clientMenu(sock_send);

    close(sock_send);
    return 0;
}

void createSpreadsheet() {
    char * const COLUMN_HEADINGS = "      A        B        C        D        E        F        G        H        I";
    char * const HORIZONTAL_LINE = "  +--------+--------+--------+--------+--------+--------+--------+--------+--------+";
    char * const VERTICAL__LINES = "|        |        |        |        |        |        |        |        |        |";
    
    printf("%s\n", COLUMN_HEADINGS);
    for(int i = 0; i < 9; i++) {
        printf("%s\n", HORIZONTAL_LINE);
        printf("%d %s\n",i+1, VERTICAL__LINES);
    }
    printf("%s\n", HORIZONTAL_LINE);
}


void clientMenu(int sock_send) {


    char text[80],buff[BUF_SIZE];
    char readBuff[BUF_SIZE];
    int	send_len,bytes_sent;


    for(;;) {

        bzero(buff, sizeof (buff));
        printf("Enter the cell address you would like to edit (Please enter quit to leave): ");
        scanf("%s",text);


        while (!strcmp(text, "quit"))
            ;
        strcpy(buff,text);
        send_len=strlen(text);

        createSpreadsheet();

        if(send_len == 2){


            bytes_sent=send(sock_send,buff,send_len,0);

           printf("met \n");

          /*  write(sock_send, buff, sizeof (buff));*/


        }else{
            printf("Invalid coordinate");
            close(sock_send);
        }

        bzero(buff, sizeof(buff));
        read(sock_send, readBuff, sizeof(buff));
        printf("From Server : %s", readBuff);
        if ((strncmp(buff, "quit", 4)) == 0) {
            printf("Client Exit...\n");
            break;
        }

    }
}
