#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wiringPi.h>
#include <time.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
 
#define BUFSIZE 100
#define COLOR_S0 0
#define COLOR_S1 1
#define COLOR_S2 2
#define COLOR_S3 3
#define COLOR_OUT 4
#define COLOR_LED 5
#define PIR_D 8 // 27
#define LED_RED 7 //4
#define LED_GREEN 21 // 5
#define LED_BLUE 22   //6
#define BUZZER 15

typedef struct senddata
{
    int passok;
    char message[BUFSIZE];
    int flag;
}s_data;

s_data send_message(int sock, s_data sData);
void * recv_message(void *arg);
void error_handling(char *message);
void wiringPi_Init();
void Count();
void Detect_Color( char c);
void PIR_interrupt();
void *Thread_Func();
void *buzzer();
void *led(void *arg);
void *pir();
 
char message[BUFSIZE];
char color = 0;
char pir_flag = 0;
int count_p = 0;
int passok = 0;
int i = 0;
 
unsigned int countR = 0, countG = 0, countB = 0;
unsigned int counter = 0;

int main(int argc, char **argv)
{
    if(wiringPiSetup () == -1) exit(1);

    int sock;
    struct sockaddr_in serv_addr;

    pthread_t rcv_thread;
    pthread_t thread_led, thread_pir, thread_buzzer, thread_color;
    void * thread_result;

    if(argc!=3){
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock=socket(PF_INET, SOCK_STREAM, 0);
    if(sock==-1) {
        error_handling("socket() error");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1) {
        error_handling("connect() error");
    }

    wiringPi_Init();

    pthread_create(&rcv_thread, NULL, recv_message, (void*)sock);
    pthread_create(&thread_pir, NULL, pir, NULL);
    pthread_create(&thread_led, NULL, led, (void*)sock);
    pthread_create(&thread_buzzer, NULL, buzzer, NULL);
    pthread_create(&thread_color, NULL, Thread_Func, NULL);

    pthread_join(rcv_thread, &thread_result);
    pthread_join(thread_pir, &thread_result);
    pthread_join(thread_led, &thread_result);
    pthread_join(thread_buzzer, &thread_result);
    pthread_join(thread_color, &thread_result);

    close(sock);
    return 0;
}

s_data send_message(int sock, s_data sData) /* 메시지 전송 쓰레드 실행 함수 */
{
    write(sock,(void*)&sData,sizeof(sData));

    s_data recv_data; memset(&recv_data, 0, sizeof(s_data));
    read(sock, (void*)&recv_data, sizeof(recv_data));

    printf("%s\n", recv_data.message);
    memset(recv_data.message, 0, BUFSIZE);
    fgets(recv_data.message, BUFSIZE, stdin);

    write(sock,(void*)&recv_data,sizeof(send_data));

    memset(&sData, 0, sizeof(sData));
    read(sock, (void*)&sData, sizeof(sData));

    return sData;
//    while(1) {
//        sdata.flag = 0;
//        sdata.passok = passok;
//        strcpy(sdata.message, "Warning");
//
//        write(sock,(char*)&sdata,sizeof(sdata));
//
//        memset(message, 0, BUFSIZE);
//
//        int str_len = read(sock, message, sizeof(message));
//        message[str_len] ="\0";
//
//        char client_msg[BUFSIZE] =
//
//        sleep(2);
//        if(s_data.flag == 0)
//        {
//
//        }
//        else if(s_data.flag ==1) {
//            message[0] = '\0';
//            memset(message, 0x00, BUFSIZE);
//            strcpy(message, "\n");
//            fgets(message, sizeof(message), stdin);
//        }
//        else if(s_data.flag==2)
//        {
//            pthread_create(&thread_pir, NULL, pir, NULL);
//            pthread_create(&thread_led, NULL, led, NULL);
//            pthread_create(&thread_buzzer, NULL, buzzer, NULL);
//            wiringPi_Init();
//            pthread_create(&thread_color, NULL, Thread_Func, NULL);
//        }
//    }
}
 
void * recv_message(void *arg) /* 메시지 수신 쓰레드 실행 함수 */
{
//  int sock = (int)arg;
//  //char send_message[BUFSIZE];
//  int str_len;
//  while(1){
//    str_len = read(sock, send_message, BUFSIZE-1);
//     //if(str_len==-1) return 1;
//    //send_message[str_len]=0;
//    //fputs(send_message, stdout);
//  }
}

void error_handling(char *message)
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}

void Count(){
    counter ++;
}

void wiringPi_Init(){
   pinMode(COLOR_S0,OUTPUT);
   pinMode(COLOR_S1,OUTPUT);
   pinMode(COLOR_S2,OUTPUT);
   pinMode(COLOR_S3,OUTPUT);
   pinMode(COLOR_OUT,INPUT);
   pinMode(COLOR_LED,OUTPUT);
   digitalWrite(LED_RED,0);
   digitalWrite(LED_GREEN,0);
   digitalWrite(LED_BLUE,0);
 
   digitalWrite (COLOR_S0, 0);
   digitalWrite(COLOR_S1, 1);
   digitalWrite(COLOR_LED, 1);
   wiringPiISR(COLOR_OUT,INT_EDGE_RISING,&Count);
}

void *pir()
{
    pinMode(PIR_D, INPUT);
    wiringPiISR(PIR_D, INT_EDGE_RISING, &PIR_interrupt);
    while(1)
    {
    
       if(pir_flag == 1)
       {
         count_p++;
         pir_flag = 0;
       }
  }
}

void *led(void *arg)
{
    pinMode(LED_RED,OUTPUT);
    pinMode(LED_GREEN,OUTPUT);
    pinMode(LED_BLUE,OUTPUT);

    while(1)
    {
        sleep(1);

        if(countR>500 && countR>countG && countR>countB)
        {
            // admin card
            passok = 0;
            digitalWrite(LED_RED,0);
            digitalWrite(LED_BLUE,0);
            digitalWrite(LED_GREEN,1);
            count_p = 0;
            pir_flag = 0;
            sleep(5);
        }
        else if(count_p>=5)
        {
            // warning
            passok=1;

            digitalWrite(LED_RED,1);
            digitalWrite(LED_GREEN,0);
            digitalWrite(LED_BLUE,0);

            s_data sData; memset(&sData, 0, sizeof(s_data));
            strcpy(sData, "Warning");
            send_message((int)arg, sData);

            printf("sdata.flag = %d\n", sData.flag);
            printf("sdata.msg = %s\n", sData.message);

        }
        else if(sData.flag == 2)
        {
            // nomal
            digitalWrite(LED_RED,0);
            digitalWrite(LED_BLUE,1);
            digitalWrite(LED_GREEN,0);
        }
        else
        {
            // nomal
            digitalWrite(LED_RED,0);
            digitalWrite(LED_BLUE,1);
            digitalWrite(LED_GREEN,0);
        }
    }
}

void *buzzer()
{
    while(1){
        if(passok==0)
        {
            digitalWrite(BUZZER,0);
        }
        else if(passok==1)
        {
            while(passok!=0)
            {
                digitalWrite(BUZZER,1);
                sleep(1);
                digitalWrite(BUZZER,0);
                sleep(2);
            }
        }
    }
}

void Detect_Color( char c)
{
    if(c==0) { // RED
        digitalWrite(COLOR_S2, 0);
        digitalWrite(COLOR_S3, 0);
    }
    else if(c==1) { //GREEN
        digitalWrite(COLOR_S2, 1);
        digitalWrite(COLOR_S3, 1);
    }
    else if(c==2) { //BLUE
        digitalWrite(COLOR_S2, 0);
        digitalWrite(COLOR_S3, 1);
    }
}
 
void PIR_interrupt() 
{
    pir_flag = 1 ;
}
 
 
void *Thread_Func(){
    while(1){
        if(color==0) { //RED
            countR = counter ;
            color = 1; //Request Green
            Detect_Color(color); //S2 =1, S3 = 1
        }
        else if(color==1) { //Green
            countG = counter ;
            color = 2; // Request Blue
            Detect_Color(color); //S2 = 0, S3 =1
        }
        else if(color==2) { //BLEU
            countB = counter ;
            color = 0; //Request Red
            Detect_Color(color);//S2 = 0, S3 = 0
        }
        counter = 0 ;
        delay(1000);
    }
}
 