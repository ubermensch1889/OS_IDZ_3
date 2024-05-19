#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <signal.h>

#define INFOBUFSIZE 128   /* Size of receive buffer */

void DieWithError(char *errorMessage);  /* Error handling function */

// Записываем в буфер сообщение длины 3.
void fill_message(char* buffer, int n) {
    int i = 2;
    while (n) {
        int digit = n % 10;
        n /= 10;
        buffer[i] = digit + '0';
        --i;
    }

    if (i != -1) {
        while (i != -1) {
            buffer[i--] = '0';
        }
    }
    buffer[3] = '\0';
}

int sock;                        /* Socket descriptor */

// Функция, осуществляющая обработку сигнала прерывания работы
// Осществляет удаление всех семафоров и памяти.
void sigfunc(int sig) {
    close(sock);

    printf("Monitor: bye!!!\n");
    exit (10);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigfunc);
    signal(SIGTERM,sigfunc);
    setbuf(stdout, NULL);

    struct sockaddr_in echoServAddr; /* Echo server address */
    unsigned short echoServPort;     /* Echo server port */
    char *servIP;                    /* Server IP address (dotted quad) */
    char buffer[INFOBUFSIZE];     /* Buffer for echo string */
    int recvMsgSize;

    if (argc != 3)    /* Test for correct number of arguments */
    {
       fprintf(stderr, "Usage: %s <IP> <Port>\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];             /* First arg: server IP address (dotted quad) */
    echoServPort = atoi(argv[2]); /* Use given port, if any */

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));     /* Zero out structure */
    echoServAddr.sin_family      = AF_INET;             /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    echoServAddr.sin_port        = htons(echoServPort); /* Server port */

    /* Establish the connection to the echo server */
    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("connect() failed");

    // Отправляем серверу сообщение с типом клиента. Мы - монитор (2).
    fill_message(buffer, 2);
    if (send(sock, buffer, strlen(buffer) * sizeof(buffer[0]), 0) != strlen(buffer))
        DieWithError("send() sent a different number of bytes than expected");



    for(;;) {
        if ((recvMsgSize = recv(sock, buffer, INFOBUFSIZE, 0)) < 0)
            DieWithError("recv() failed");
        buffer[recvMsgSize] = '\0';
        printf(buffer);
    }

    close(sock);
    exit(0);
}

