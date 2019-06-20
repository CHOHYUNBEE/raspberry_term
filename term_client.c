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

int send_message(int sock, s_data sData);
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
void *card_input(void *arg);
 
char message[BUFSIZE];
char color;
char pir_flag;
int count_p;
int passok;
int i = 0;
 
unsigned int countR = 0, countG = 0, countB = 0;
unsigned int counter = 0;

pthread_mutex_t mutx;

int main(int argc, char **argv)
{
    if(wiringPiSetup () == -1) exit(1);

    int sock;
    struct sockaddr_in serv_addr;

    pthread_t rcv_thread, cart_thread;
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
    pthread_create(&cart_thread, NULL, card_input, (void*)sock);

    pthread_join(rcv_thread, &thread_result);
    pthread_join(thread_pir, &thread_result);
    pthread_join(thread_led, &thread_result);
    pthread_join(thread_buzzer, &thread_result);
    pthread_join(thread_color, &thread_result);
    pthread_join(cart_thread, &thread_result);

    close(sock);
    return 0;
}

int send_message(int sock, s_data sData) /* 메시지 전송 쓰레드 실행 함수 */
{
    write(sock, (void *) &sData, sizeof(sData));
    printf("flag : %d\n", recv_data.flag);
    s_data recv_data;
    memset(&recv_data, 0, sizeof(s_data));
    read(sock, (void *) &recv_data, sizeof(recv_data));

    if (strcmp(recv_data.message, "Warning message if you mistake, you enter this number /1457/\n") == 0) {
        if( strcpy(sData.message, "Card Authentication Complete")!=0) {
            printf("%s\ninput password : ", recv_data.message);
            printf("flag : %d\n", recv_data.flag);
            printf("length : %d\n", strlen(sData.message));
            memset(recv_data.message, 0, BUFSIZE);
            fgets(recv_data.message, BUFSIZE, stdin);
        }
    }
    printf("flag : %d\n", recv_data.flag);

    if(passok == 0) return 2;

    write(sock,(void*)&recv_data,sizeof(recv_data));

    memset(&recv_data, 0, sizeof(recv_data));
    read(sock, (void*)&recv_data, sizeof(recv_data));

    return recv_data.flag;
}
 
void * recv_message(void *arg) /* 메시지 수신 쓰레드 실행 함수 */
{

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
   pinMode(BUZZER, OUTPUT);

   digitalWrite(LED_RED,0);
   digitalWrite(LED_GREEN,0);
   digitalWrite(LED_BLUE,1);
 
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

        if(count_p >= 15)
        {
            pthread_mutex_lock(&mutx);
            passok = 1;
            digitalWrite(LED_RED,1);
            digitalWrite(LED_GREEN,0);
            digitalWrite(LED_BLUE,0);

            s_data sData;
            if(strcmp(sData.message, "Undo Warning Mode")!=0 &&strcmp(sData.message, "Invalid security code")!=0) {
                memset(&sData, 0, sizeof(s_data));
                strcpy(sData.message, "Warning");
            }

            int recv_flag = send_message((int)arg, sData);

            if(recv_flag == 1)
            {
                count_p = 0;
                passok = 0;
                recv_flag = 0;
                digitalWrite(LED_RED,0);
                digitalWrite(LED_BLUE,1);
                digitalWrite(LED_GREEN,0);
            }
            pthread_mutex_unlock(&mutx);

        }
    }
}

void *card_input(void *arg) {
    while(1) {
        sleep(4);

        if(countR>500 && countR>countG && countR>countB)
        {
            digitalWrite(LED_RED,0);
            digitalWrite(LED_BLUE,0);
            digitalWrite(LED_GREEN,1);

            digitalWrite(LED_RED,0);
            digitalWrite(LED_BLUE,1);
            digitalWrite(LED_GREEN,0);

            if(passok == 0) continue;

            // admin card
            printf("\nCard Authentication Complete\n");
            count_p = 0;
            pir_flag = 0;
            passok = 0;
            digitalWrite(LED_RED,0);
            digitalWrite(LED_BLUE,0);
            digitalWrite(LED_GREEN,1);

            s_data sData; memset(&sData, 0, sizeof(s_data));
            sData.flag = 2;
            strcpy(sData.message, "Card Authentication Complete");
            write((int)arg, (void*)&sData, sizeof(sData));
        }
    }
    return NULL;
}

void *buzzer()
{
    while(1){
        if(passok == 0)
        {
            digitalWrite(BUZZER,0);
        }
        else
        {
            digitalWrite(BUZZER,1);
            sleep(1);
            digitalWrite(BUZZER,0);
            sleep(1);
        }
        sleep(1);
    }
}

void Detect_Color(char c)
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
 