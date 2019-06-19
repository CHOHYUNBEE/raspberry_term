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
 
void * send_message(void *arg);
void * recv_message(void *arg);
void error_handling(char *message);
void wiringPi_Init();
void Count();
void Detect_Color( char c);
void PIR_interrupt();
void *Thread_Func();
void *buzzer();
void *led();
void *pir();
 
char message[BUFSIZE];
char color = 0;
char pir_flag = 0;
int count_p = 0;
int passok = 0;
int i = 0;
 
unsigned int countR = 0, countG = 0, countB = 0;
unsigned int counter = 0;
typedef struct senddata
{
  int passok;
  char message;
  int flag;
}s_data;
int main(int argc, char **argv)
{
  if(wiringPiSetup () == -1)
    exit(1);
  int sock;
  struct sockaddr_in serv_addr;
  pthread_t snd_thread, rcv_thread;
  void * thread_result;
  if(argc!=3){
    printf("Usage : %s <IP> <port>\n", argv[0]);
    exit(1);
  }
 
  sock=socket(PF_INET, SOCK_STREAM, 0);
  if(sock==-1)
     error_handling("socket() error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family=AF_INET;
  serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
  serv_addr.sin_port=htons(atoi(argv[2]));
  
  if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
    error_handling("connect() error");
  pthread_create(&snd_thread, NULL, send_message, (void*)sock);
  pthread_create(&rcv_thread, NULL, recv_message, (void*)sock);
  pthread_join(snd_thread, &thread_result);
  pthread_join(rcv_thread, &thread_result);
  close(sock);  return 0;
}
void * send_message(void *arg) /* 메시지 전송 쓰레드 실행 함수 */
{
   int sock = (int)arg;
   struct senddata s_data;
  
  
   pthread_t thread_led, thread_pir, thread_buzzer, thread_color;
   pthread_create(&thread_pir, NULL, pir, NULL);
   pthread_create(&thread_led, NULL, led, NULL);
   pthread_create(&thread_buzzer, NULL, buzzer, NULL);
   wiringPi_Init();
   pthread_create(&thread_color, NULL, Thread_Func, NULL);
   while(1) {
     s_data.flag = 0;
     s_data.passok = passok;
     s_data.message = "Warning";
     write(sock,(char*)&s_data,sizeof(senddata));
     str_len = read(sock, message, sizeof(message));
     message[str_len] ="\0";
     s_data = (senddata*)message;
     sleep(2);
     if(s_data.flag == 0)
  {
      
  }
      else if(s_data.flag ==1)
  {
       message[0] = '\0';
       memset(message, 0x00, BUFSIZE);
       strcpy(message, "\n");
       fflush(stdin);
       fgets(message, sizeof(message), stdin);
  }
  else if(s_data.flag==2)
  {
     pthread_create(&thread_pir, NULL, pir, NULL);
     pthread_create(&thread_led, NULL, led, NULL);
     pthread_create(&thread_buzzer, NULL, buzzer, NULL);
     wiringPi_Init();
     pthread_create(&thread_color, NULL, Thread_Func, NULL);
  }
   }
}
 
void * recv_message(void *arg) /* 메시지 수신 쓰레드 실행 함수 */
{
  int sock = (int)arg;
  //char send_message[BUFSIZE];
  int str_len;
  while(1){
    str_len = read(sock, send_message, BUFSIZE-1);
     //if(str_len==-1) return 1;
    //send_message[str_len]=0;
    //fputs(send_message, stdout);
  }
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
void *led()
{ 
   senddata s_data;
   pinMode(LED_RED,OUTPUT);
   pinMode(LED_GREEN,OUTPUT);
   pinMode(LED_BLUE,OUTPUT);
   while(1)
  {
   if(countR>500 && countR>countG && countR>countB)
   {
      //통과 
    passok = 0;
    digitalWrite(LED_RED,0);
    digitalWrite(LED_BLUE,0);
    digitalWrite(LED_GREEN,1);
    count_p=0;
    pir_flag = 0;
    sleep(5);
   }
   else if(count_p>=5)
   {
    passok=1;
    digitalWrite(LED_RED,1);
    digitalWrite(LED_GREEN,0);
    digitalWrite(LED_BLUE,0);
   }
  else if(s_data.flag==2)
  {
    digitalWrite(LED_RED,0);
    digitalWrite(LED_BLUE,1);
    digitalWrite(LED_GREEN,0);
  }
   else
   {
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
 