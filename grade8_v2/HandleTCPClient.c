#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>

#define RCVBUFSIZE 32   /* Size of receive buffer */
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

const char sem_mons[] = "/monitors"; //  имя объекта

void ntf_monitors(sem_t* sem_monitor, char* buffer, int* monitors_counter, int* monitors) {
    if (sem_wait(sem_monitor) == -1) {
        perror("sem_wait: Incorrect wait of book semaphore");
        exit(-1);
    }
    printf("%d\n", *monitors_counter);
    for (int i = 0; i < *monitors_counter; ++i) {
        printf("%d\n", monitors[i]);
        if (send(monitors[i], buffer, strlen(buffer) * sizeof(buffer[0]), 0) != strlen(buffer))
            DieWithError("send() failed");
    }
    if (sem_post(sem_monitor) < 0) {
        perror("sem_post: Librarian can not increment book semaphore");
        exit(-1);
    }
}


void HandleTCPClient(int clntSocket, int n, int* book_info, const char sem_name[], int number, int* monitors_counter, int* monitors)
{
    setbuf(stdout, NULL);

    sem_t* sem;
    if ((sem = sem_open(sem_name, O_RDWR)) == 0) {
        perror("sem_open: Can not get semaphore");
        exit(-1);
    }
    sem_t* sem_monitor;
    if ((sem_monitor = sem_open(sem_mons, O_RDWR)) == 0) {
        perror("sem_open: Can not get semaphore");
        exit(-1);
    }

    char buffer[RCVBUFSIZE];        /* Buffer for echo string */
    char info[INFOBUFSIZE];        /* Buffer for echo string */
    int recvMsgSize;                    /* Size of received message */
    int count;                          // Количество книг, необходимых читателю.
    int duration;                       // Срок, на который пользователь берет книгу.


    // Сначала нужно отправить клиенту его номер и количество книг, чтобы он мог решить, какие ему нужны.
    fill_message(buffer, number);

    if (send(clntSocket, buffer, strlen(buffer) * sizeof(buffer[0]), 0) != strlen(buffer))
        DieWithError("send() failed");

    fill_message(buffer, n);

    if (send(clntSocket, buffer, strlen(buffer) * sizeof(buffer[0]), 0) != strlen(buffer))
        DieWithError("send() failed");

    while(1) {
        // Теперь клиент отправляет количество книг, которые он хотел бы взять.
        /* Receive message from client */
        if ((recvMsgSize = recv(clntSocket, buffer, 3, 0)) < 0)
            DieWithError("recv() failed");
        buffer[recvMsgSize] = '\0';
        count = atoi(buffer);

        if ((recvMsgSize = recv(clntSocket, buffer, 3, 0)) < 0)
            DieWithError("recv() failed");
        buffer[recvMsgSize] = '\0';
        duration = atoi(buffer);

        sprintf(info, "Reader %d wants to take the following books for %d days:", number, duration);

        // Считываем номера нужных книг, выводим и запоминаем их.
        int book_number;
        int books[3];
        for (int i = 0; i < count; ++i) {
            if ((recvMsgSize = recv(clntSocket, buffer, 3, 0)) < 0)
                DieWithError("recv() failed");
            buffer[recvMsgSize] = '\0';
            book_number = atoi(buffer);
            books[i] = book_number;
            sprintf(buffer, " %d", book_number);
            sprintf(info, "%s%s", info, buffer);
        }

        sprintf(info, "%s\n", info);
        printf(info);
        ntf_monitors(sem_monitor, info, monitors_counter, monitors);
        printf("smth\n");

        // Забираем доступные книги.
        bool flag;      // Флаг, который обозначает, что все книги взяты.
        while (1) {
            if (sem_wait(sem) == -1) {
                perror("sem_wait: Incorrect wait of book semaphore");
                exit(-1);
            }
            for (int i = 0; i < count; ++i) {

                if (books[i] != -1 && !book_info[books[i]]) {     // Книга свободна.
                    book_info[books[i]] = duration;

                    // Отравляем клиенту информацию о том, что нужная ему книга взята, т.е. "отдаем ему книгу".
                    fill_message(buffer, books[i]);
                    if (send(clntSocket, buffer, strlen(buffer) * sizeof(buffer[0]), 0) != strlen(buffer))
                        DieWithError("send() failed");
                    sleep(1);

                    sprintf(info, "Reader %d takes the book number %d for %d days\n", number, books[i], duration);
                    printf(info);
                    ntf_monitors(sem_monitor, info, monitors_counter, monitors);

                    // Информация об этой книге нам больше не нужна, так что помечаем минус единицей, что она взята.
                    books[i] = -1;
                }
            }
            if (sem_post(sem) < 0) {
                perror("sem_post: Librarian can not increment book semaphore");
                exit(-1);
            }
            flag = 1;

            for (int i = 0; i < count; ++i) {
                if (books[i] != -1) {     // Книга еще не взята.
                    flag = false;
                    break;
                }
            }

            if (flag) {
                break;
            }
        }

        // Уведомляем клиента о том, что все книги им получены.
        sprintf(buffer, "ok!");

        if (send(clntSocket, buffer, strlen(buffer) * sizeof(buffer[0]), 0) != strlen(buffer))
                DieWithError("send() failed");

    }

    close(clntSocket);    /* Close client socket */
}
