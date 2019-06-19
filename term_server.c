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

typedef struct client
{
    int sock;
    char addr[16];
} client;

char* what_time(){
    time_t times;
    struct tm *t;
    char *rtime= malloc(sizeof(char) * 30);
    char *tmp= malloc(sizeof(char) * 30);

    times = time(NULL); // 현재 시각을 초 단위로 얻기
    strcpy(rtime, "[");
    t = localtime(&times); // 초 단위의 시간을 분리하여 구조체에 넣기
    switch(t->tm_wday){
        case 0: sprintf(tmp, "MON"); break;
        case 1: sprintf(tmp, "TUE"); break;
        case 2: sprintf(tmp, "WED"); break;
        case 3: sprintf(tmp, "THU"); break;
        case 4: sprintf(tmp, "FRI"); break;
        case 5: sprintf(tmp, "SAT"); break;
        case 6: sprintf(tmp, "SUN"); break;
    }//요일
    strcat(rtime, tmp);
    sprintf(tmp, " %d/%d/%d ",   t->tm_year + 1900, t->tm_mon + 1, t->tm_mday); //년/월/일
    strcat(rtime, tmp);
    sprintf(tmp, "%d:%d:%d]",   t->tm_hour, t->tm_min, t->tm_sec); //시:분:초
    strcat(rtime, tmp);

    return rtime;
}

int main(int argc, char **argv)
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    int clnt_addr_size;
    pthread_t thread;
    char ipaddress[100]; //접속한 클라이언트의 ip주소
    char *acctime; //접속 시간
    FILE *fp; //file pointer

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

    fp=fopen("../log.txt","a+"); //file open
    if (fp == NULL)
        printf ("File Open ERROR.... \n");

    while (1) {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
//        inet_ntop(AF_INET, &clnt_addr.sin_addr, ipaddress, INET_ADDRSTRLEN); //ip address

        acctime = what_time();
        fputs(acctime, fp);
        fputs(" [ip] ", fp);
        fputs(ipaddress , fp);
        fputs(" connect",fp);
        fputs("\n", fp);
        fclose(fp);

        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_number++] = clnt_sock;
        pthread_mutex_unlock(&mutx);

        client clnt; memset(&clnt, 0, sizeof(client));
        clnt.sock = clnt_sock;
        strcpy(clnt.addr, inet_ntoa(clnt_addr.sin_addr));
        pthread_create(&thread, NULL, clnt_connection, (void*)&clnt);
        printf("새로운 연결, 클라이언트 IP : %s \n", inet_ntoa(clnt_addr.sin_addr));
    }
    return 0;
}

void * clnt_connection(void *arg)
{
    client *clnt = (client*)arg;

    int clnt_sock = clnt->sock;
    s_data sData; memset(&sData, 0, sizeof(s_data));
    int str_len = 0;
    char message[BUFSIZE] = "";
    char sendmsg[BUFSIZE] = "";
    char *acctime; //접속 시간
    FILE *fp; //file pointer
    int i;

    while ((str_len = read(clnt_sock, (void*)&sData, sizeof(sData))) != 0) {
        printf("[Client : %s] %s\n", clnt->addr,sData.message);
        fp=fopen("../log.txt","a+"); //file open
        if (fp == NULL)
            printf ("File Open ERROR.... \n");

        if (sData.flag == 0) { //경고 모드(침입자 발생)
            if (strcmp(sData.message, "Warning") == 0) {
                strcpy(sendmsg, "Warning message if you mistake, you enter this number /1457/\n");
                sData.flag = 1;

                acctime = what_time();
                fputs(acctime, fp);
                fputs("\n Client : ", fp);
                fputs(message, fp);
                fputs("\n Server : ", fp);
                fputs(sendmsg, fp);

                write(clnt_sock, (void*)&sData, sizeof(sData));
            }
        }
        else if (sData.flag == 1) { //보안코드 확인 모드
            if (strcmp(sData.message, "1457\n") == 0) { //경고 모드 해제
                memset(sData.message, 0, BUFSIZE);
                strcpy(sData.message, "Undo Warning Mode");
                sData.flag = 1;

                acctime = what_time();
                fputs(acctime, fp);
                fputs("\n Client : ", fp);
                fputs(message, fp);
                fputs("\n Server : ", fp);
                fputs(sendmsg, fp);

                write(clnt_sock, (void*)&sData, sizeof(sData));
            }
            else{ //잘못된 보안코드 -> 다시 경고 모드로
                memset(sData.message, 0, BUFSIZE);
                strcpy(sData.message, "Invalid security code");
                sData.flag = 0;

                acctime = what_time();
                fputs(acctime, fp);
                fputs("\n Client : ", fp);
                fputs(message, fp);
                fputs("\n Server : ", fp);
                fputs(sendmsg, fp);

                write(clnt_sock, (void*)&sData, sizeof(sData));
            }
        }

        memset(message, 0x00, BUFSIZE);
        memset(sendmsg, 0x00, BUFSIZE);
        memset(&sData, 0, sizeof(s_data));
        fclose(fp);
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


void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
 