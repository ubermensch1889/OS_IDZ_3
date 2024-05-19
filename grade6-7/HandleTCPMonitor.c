#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>

#define INFOBUFSIZE 128   /* Size of receive buffer */

void DieWithError(char *errorMessage);  /* Error handling function */

const char name[] = "/message"; //  имя объекта


void HandleTCPMonitor(int clntSocket, char* message)
{
    setbuf(stdout, NULL);

    char prev[INFOBUFSIZE];           // Храним предыдущее отправленное сообщение.
    int recvMsgSize;                    /* Size of received message */
    prev[0] = '\0';

    sem_t* sem_msg;
    if ((sem_msg = sem_open(name, O_RDWR)) == 0) {
        perror("sem_open: Can not get semaphore");
        exit(-1);
    }


    while(1) {
        if (sem_wait(sem_msg) == -1) {
            perror("sem_wait: Incorrect wait of book semaphore");
            exit(-1);
        }
        // Если строка из разделяемой памяти изменилась, то отправляем ее монитору.
        if (strcmp(prev, message)) {

            if (send(clntSocket, message, strlen(message) * sizeof(message[0]), 0) != strlen(message))
                DieWithError("send() failed");

            strcpy(message, prev);

        }
        if (sem_post(sem_msg) < 0) {
            perror("sem_post: Librarian can not increment book semaphore");
            exit(-1);
        }
    }

    close(clntSocket);    /* Close client socket */
}

