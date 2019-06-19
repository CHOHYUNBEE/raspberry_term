#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#define BUFSIZE 100

void * clnt_connection(void *arg);
void send_message(char * message, int len);
void error_handling(char *message);
int clnt_number = 0;
int clnt_socks[10];
pthread_mutex_t mutx;

typedef struct senddata
{
   int passok; //통과유무
   char message[BUFSIZE];
   int flag; //행동 플래그
}s_data;

int main(int argc, char **argv)
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    int clnt_addr_size;
    pthread_t thread;

    if (argc != 2) {
        printf("Usage : %s <port> \n", argv[0]);
        exit(1);
    }

    if (pthread_mutex_init(&mutx, NULL)) {
        error_handling("mutex init error");
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1) {
        error_handling("bind() error");
    }

    if (listen(serv_sock, 5) == -1) {
        error_handling("listen() error");
    }

    while (1) {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_number++] = clnt_sock;
        pthread_mutex_unlock(&mutx);

        pthread_create(&thread, NULL, clnt_connection, (void*)clnt_sock);
        printf("새로운 연결, 클라이언트 IP : %s \n", inet_ntoa(clnt_addr.sin_addr));
    }
    return 0;
}

void * clnt_connection(void *arg)
{
    int clnt_sock = (int)arg;
    s_data sData; memset(&sData, 0, sizeof(s_data));
    int str_len = 0;
    char message[BUFSIZE] = "";
    char sendmsg[BUFSIZE] = "";
    int i;

    while ((str_len = read(clnt_sock, (void*)&sData, sizeof(sData))) != 0) {
        fflush(stdout);
        printf("[Client] %s\n", sData.message);
        if (sData.flag == 0) { //경고 모드(침입자 발생)
            if (strcmp(sData.message, "Warning") == 0) {
                strcpy(sendmsg, "Warning message if you mistake, you enter this number /1457/\n");
                sData.flag = 1;

                write(clnt_sock, (void*)&sData, sizeof(sData));
            }
        }
        else if (sData.flag == 1) { //보안코드 확인 모드
            if (strcmp(sData.message, "1457\n") == 0) { //경고 모드 해제
                memset(sData.message, 0, BUFSIZE);
                strcpy(sData.message, "Undo Warning Mode");
                sData.flag = 1;

                write(clnt_sock, (void*)&sData, sizeof(sData));
            }
            else{ //잘못된 보안코드 -> 다시 경고 모드로
                memset(sData.message, 0, BUFSIZE);
                strcpy(sData.message, "Invalid security code");
                sData.flag = 0;
                write(clnt_sock, (void*)&sData, sizeof(sData));
            }
        }
        else if(sData.flag == 2){ //보안 카드가 입력됨 -> 보안 해제
            memset(sData.message, 0, BUFSIZE);
            strcpy(sData.message, "Security card entered. Unsecure");
            sData.flag = 2;
            write(clnt_sock, (void*)&sData, sizeof(sData));
        }

//        send_message(message, str_len);
        memset(message, 0x00, BUFSIZE);
        memset(sendmsg, 0x00, BUFSIZE);
        memset(&sData, 0, sizeof(s_data));
        fflush(stdout);
    }

    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_number; i++) { /* 클라이언트 연결 종료 시 */
      if (clnt_sock == clnt_socks[i]) {
         for (; i < clnt_number - 1; i++)
            clnt_socks[i] = clnt_socks[i + 1];
         break;
      }
    }
    clnt_number--;
    pthread_mutex_unlock(&mutx);

    close(clnt_sock);
    return 0;
}
void send_message(char * message, int len)
{
   int i;
   pthread_mutex_lock(&mutx);
   for (i = 0; i < clnt_number; i++)
      write(clnt_socks[i], message, len);
   pthread_mutex_unlock(&mutx);
}
void error_handling(char *message)
{
   fputs(message, stderr);
   fputc('\n', stderr);
   exit(1);
}
 