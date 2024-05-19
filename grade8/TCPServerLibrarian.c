#include "TCPServerLibrarian.h"  /* TCP echo server includes */
#include <sys/wait.h>       /* for waitpid() */


int* books_info;
const char shm_object[] = "books_info"; //  имя объекта
const char message_name[] = "message"; //  имя объекта
const char counter_name[] = "counter"; //  имя объекта
const char monitors_number_name[] = "monitors_number"; //  имя объекта
const char curr_monitors_number_name[] = "curr_monitors_number"; //  имя объекта
const char sem_name[] = "/sem"; //  имя объекта
const char sem__msg_name[] = "/message"; //  имя объекта
const char sem__monitors_name[] = "/curr_monitors"; //  имя объекта
int n;
int servSock;

#define RCVBUFSIZE 32   /* Size of receive buffer */
#define INFOBUFSIZE 128   /* Size of receive buffer */

void end() {
    if(shm_unlink(shm_object) == -1) {
        exit(-1);
    }

    if(shm_unlink(counter_name) == -1) {
        exit(-1);
    }

    if(shm_unlink(monitors_number_name) == -1) {
        exit(-1);vb
    }

    if(shm_unlink(curr_monitors_number_name) == -1) {
        exit(-1);
    }

    sem_t* sem;
    if ((sem = sem_open(sem_name, O_CREAT, 0666, 1)) == 0) {
      perror("sem_open: Can not get semaphore");
      exit(-1);
    }
    if(sem_close(sem) == -1) {
      perror("sem_close: Incorrect close of book semaphore");
      exit(-1);
    };

    if(sem_unlink(sem_name) == -1) {
      perror("sem_unlink: Incorrect unlink of book semaphore");
      exit(-1);
    };

    sem_t* sem_msg;
    if ((sem_msg = sem_open(sem__msg_name, O_CREAT, 0666, 1)) == 0) {
      perror("sem_open: Can not get semaphore");
      exit(-1);
    }
    if(sem_close(sem_msg) == -1) {
      perror("sem_close: Incorrect close of book semaphore");
      exit(-1);
    };

    if(sem_unlink(sem__msg_name) == -1) {
      perror("sem_unlink: Incorrect unlink of book semaphore");
      exit(-1);
    };

    if ((sem_msg = sem_open(sem__monitors_name, O_CREAT, 0666, 1)) == 0) {
      perror("sem_open: Can not get semaphore");
      exit(-1);
    }
    if(sem_close(sem_msg) == -1) {
      perror("sem_close: Incorrect close of book semaphore");
      exit(-1);
    };

    if(sem_unlink(sem__monitors_name) == -1) {
      perror("sem_unlink: Incorrect unlink of book semaphore");
      exit(-1);
    };


    close(servSock);

    printf("Librarian: bye!!!\n");
}

// Функция, осуществляющая обработку сигнала прерывания работы
// Осществляет удаление всех семафоров и памяти.
void sigfunc(int sig) {
    end();
    exit (10);
}


void message_cpy(sem_t* sem, sem_t* monitor_sem, char* message, char* source, int* monitors_number, int* curr) {
    // Ждем, пока другое сообщение передастся всем мониторам.
    while (1) {
        if (sem_wait(monitor_sem) == -1) {
            perror("sem_wait: Incorrect wait of book semaphore");
            exit(-1);
        }
        if (*monitors_number == *curr) {        // Если равенство выполняется, то сейчас не происходит передача сообщений.
            if (sem_post(monitor_sem) < 0) {
                perror("sem_post: Librarian can not increment book semaphore");
                exit(-1);
            }
            break;
        }
        if (sem_post(monitor_sem) < 0) {
            perror("sem_post: Librarian can not increment book semaphore");
            exit(-1);
        }
    }

    if (sem_wait(sem) == -1) {
        perror("sem_wait: Incorrect wait of book semaphore");
        exit(-1);
    }
    strcpy(message, source);
    if (sem_post(sem) < 0) {
        perror("sem_post: Librarian can not increment book semaphore");
        exit(-1);
    }

    if (sem_wait(monitor_sem) == -1) {
        perror("sem_wait: Incorrect wait of book semaphore");
        exit(-1);
    }
    *curr = 0;
    if (sem_post(monitor_sem) < 0) {
        perror("sem_post: Librarian can not increment book semaphore");
        exit(-1);
    }

    // Ждем, пока наше сообщение передастся всем мониторам.
    while (1) {
        if (sem_wait(monitor_sem) == -1) {
            perror("sem_wait: Incorrect wait of book semaphore");
            exit(-1);
        }
        if (*monitors_number == *curr) {        // Если равенство выполняется, то сейчас не происходит передача сообщений.
            if (sem_post(monitor_sem) < 0) {
                perror("sem_post: Librarian can not increment book semaphore");
                exit(-1);
            }
            break;
        }
        if (sem_post(monitor_sem) < 0) {
            perror("sem_post: Librarian can not increment book semaphore");
            exit(-1);
        }
    }
}


int main(int argc, char *argv[])
{
    signal(SIGINT, sigfunc);
    signal(SIGTERM,sigfunc);


    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    unsigned short echoServPort;     /* Server port */
    pid_t processID;                 /* Process ID from fork() */
    unsigned int childProcCount = 0; /* Number of child processes */ 
    char buffer[RCVBUFSIZE];     /* Buffer for echo string */
    int recvMsgSize;

    if (argc != 4)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <Number of books> <Number of days> <Server Port>\n", argv[0]);
        exit(1);
    }

    // Число книг
    n = atoi(argv[1]);
    int days = atoi(argv[2]);

    int shm_id;

    // Инициализируем массив с информацией о книгах.
    if ( (shm_id = shm_open(shm_object, O_CREAT|O_RDWR, 0666)) == -1 ) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_id, n * sizeof(int)) == -1) {
        perror("ftruncate");
        return 1;
    }

    books_info = mmap(0, n * sizeof(int), PROT_WRITE|PROT_READ, MAP_SHARED, shm_id, 0);
    if (books_info == (int*)-1 ) {
        perror("mmap");
        exit(-1);
    }
    for (int i = 0; i < n; ++i) {
        books_info[i] = 0;
    }

    // Инициализируем счетчик.
    if ( (shm_id = shm_open(counter_name, O_CREAT|O_RDWR, 0666)) == -1 ) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_id, sizeof(int)) == -1) {
        perror("ftruncate");
        return 1;
    }

    int* counter = mmap(0, sizeof(int), PROT_WRITE|PROT_READ, MAP_SHARED, shm_id, 0);
    if (counter == (int*)-1 ) {
        perror("mmap");
        exit(-1);
    }

    // Инициализируем буфер с сообщениями о сервере.
    if ( (shm_id = shm_open(message_name, O_CREAT|O_RDWR, 0666)) == -1 ) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_id, sizeof(char) * INFOBUFSIZE) == -1) {
        perror("ftruncate");
        return 1;
    }

    char* message = mmap(0, sizeof(char) * INFOBUFSIZE, PROT_WRITE|PROT_READ, MAP_SHARED, shm_id, 0);
    if ((int*)message == (int*)-1 ) {
        perror("mmap");
        exit(-1);
    }

    // Количество мониторов.
    if ( (shm_id = shm_open(monitors_number_name, O_CREAT|O_RDWR, 0666)) == -1 ) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_id, sizeof(int)) == -1) {
        perror("ftruncate");
        return 1;
    }

    int* monitors_number = mmap(0, sizeof(int), PROT_WRITE|PROT_READ, MAP_SHARED, shm_id, 0);
    if (monitors_number == (int*)-1 ) {
        perror("mmap");
        exit(-1);
    }

    // Количество мониторов, получивших текущее сообщение.
    if ( (shm_id = shm_open(curr_monitors_number_name, O_CREAT|O_RDWR, 0666)) == -1 ) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_id, sizeof(int)) == -1) {
        perror("ftruncate");
        return 1;
    }

    int* curr_monitors_number = mmap(0, sizeof(int), PROT_WRITE|PROT_READ, MAP_SHARED, shm_id, 0);
    if (monitors_number == (int*)-1 ) {
        perror("mmap");
        exit(-1);
    }

    if ((sem_open(sem_name, O_CREAT, 0666, 1)) == 0) {
        perror("sem_open: Can not create semaphore");
        exit(-1);
    }

    if ((sem_open(sem__msg_name, O_CREAT, 0666, 1)) == 0) {
        perror("sem_open: Can not get semaphore");
        exit(-1);
    }

    if ((sem_open(sem__monitors_name, O_CREAT, 0666, 1)) == 0) {
        perror("sem_open: Can not get semaphore");
        exit(-1);
    }

    pid_t p = fork();

    if (p < 0) {
        perror("fork error");
        exit(-1);
    }
    else if (p == 0) {
        // Обработка запросов клиентов.
        echoServPort = atoi(argv[3]);  /* Third arg:  local port */

        servSock = CreateTCPServerSocket(echoServPort);

        sem_t* sem_msg;
        if ((sem_msg = sem_open(sem__msg_name, O_RDWR)) == 0) {
            perror("sem_open: Can not get semaphore");
            exit(-1);
        }

        sem_t* sem_monitor;
        if ((sem_msg = sem_open(sem__monitors_name, O_RDWR)) == 0) {
            perror("sem_open: Can not get semaphore");
            exit(-1);
        }


        for (;;) /* Run forever */
        {
            clntSock = AcceptTCPConnection(servSock);
            /* Fork child process and report any errors */
            if ((processID = fork()) < 0)
                DieWithError("fork() failed");
            else if (processID == 0)  /* If this is the child process */
            {

                close(servSock);   /* Child closes parent socket */
                if ((recvMsgSize = recv(clntSock, buffer, 3, 0)) < 0)
                    DieWithError("recv() failed");
                buffer[recvMsgSize] = '\0';
                int code = atoi(buffer);

                if (code == 1) {    // Подключился читатель.
                    ++(*counter);

                    sprintf(buffer, "Reader %d connected\n", *counter);
                    printf(buffer);
                    message_cpy(sem_msg, sem_monitor, message, buffer, monitors_number, curr_monitors_number);

                    HandleTCPClient(clntSock, n, books_info, sem_name, *counter, message, monitors_number, curr_monitors_number);
                }
                else {
                    *monitors_number += 1;
                    sprintf(buffer, "Monitor connected\n");
                    printf(buffer);
                    message_cpy(sem_msg, sem_monitor, message, buffer, monitors_number, curr_monitors_number);

                    HandleTCPMonitor(clntSock, message, curr_monitors_number);
                }


                exit(0);           /* Child process terminates */
            }


            printf("with process: %d\n", (int) processID);
            close(clntSock);       /* Parent closes child socket descriptor */
            childProcCount++;      /* Increment number of outstanding child processes */

            while (childProcCount) /* Clean up all zombies */
            {
                processID = waitpid((pid_t) -1, NULL, WNOHANG);  /* Non-blocking wait */
                if (processID < 0)  /* waitpid() error? */
                    DieWithError("waitpid() failed");
                else if (processID == 0)  /* No zombie to wait on */
                    break;

                else
                    childProcCount--;  /* Cleaned up after a child */
            }
        }
    }
    else {
        // Смена дней и подсчет времени до возвращений книг.
        sem_t* sem;
        if ((sem = sem_open(sem_name, O_RDWR)) == 0) {
            perror("sem_open: Can not get semaphore");
            exit(-1);
        }

        sem_t* sem_msg;
        if ((sem_msg = sem_open(sem__msg_name, O_RDWR)) == 0) {
            perror("sem_open: Can not get semaphore");
            exit(-1);
        }

        sem_t* sem_monitor;
        if ((sem_msg = sem_open(sem__monitors_name, O_RDWR)) == 0) {
            perror("sem_open: Can not get semaphore");
            exit(-1);
        }

        int day = 1;

        while(1) {
            // Ждем 3 секунды до завершения дня.
            sleep(3);

            // На утро смотрим, кто вернул книги, чтобы уведомить ожидающих читателей.
            for (int i = 0; i < n; ++i) {
                if (sem_wait(sem) == -1) {
                    perror("sem_wait: Incorrect wait of book semaphore");
                    exit(-1);
                }
                if (books_info[i]) {
                    books_info[i] -= 1;
                }
                if (sem_post(sem) < 0) {
                    perror("sem_post: Librarian can not increment book semaphore");
                    exit(-1);
                }
            }
            printf("\n");
            sprintf(buffer, "Day %d is over\n", day);
            printf(buffer);
            message_cpy(sem_msg, sem_monitor, message, buffer, monitors_number, curr_monitors_number);
            ++day;
            if (days < day) {
                printf("The end!\n");
                *counter = -1;
                end();
                kill(p, SIGTERM);       // Уничтожаем процесс-потомок.
                exit(0);
            }
        }
    }


    /* NOT REACHED */
}
