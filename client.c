#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <termios.h>



#define  BUFF_SIZE   1024
#define  UP 1
#define  DOWN 2
#define  FLOOR_MAX 10
#define  FLOOR_MIN 0

/*
* Protocol
* length : 3Byte 
* Author : Suchang Ko
* Date   : 2016.06.08
* ----------------
* 1. 상/하 이동
* [0]Byte : 1
* [1]Byte : 현재 층 수
* [2]Byte : 목적 층 수
* 비고 : 목적 층 수는 0층(지하1층)~10층 범위에서 Random.
* ----------------
* 2. 시작/일시정지
* [0]Byte : 2
* [1]Byte : Not Used
* [2]Byte : Not Used
* 비고 : 시작상태에서 이 명령이 들어오면 일시정지, 
* 일시정지 상태에서 이 명령이 들어오면 시작
* ----------------
* 3. 속도 조절
* [0]Byte : 3
* [1]Byte : 0 -> 0.5s | 1 -> 1s | 2 -> 2s 
* [2]Byte : Not Used
* 비고 : 없음
* ----------------
* 4. 수리
* [0]Byte : 4
* [1]Byte : Not Used
* [2]Byte : Not Used
* 비고 : 수리 명령을 받으면 Server측의 현재 고장도 초기화
* ----------------
* 5. 종료
* [0]Byte : 5
* [1]Byte : Not Used
* [2]Byte : Not Used
* 비고 : 프로그램을 종료한다.
* ----------------
*/

int mygetch(void)
{
   struct termios oldt,
   newt;
   int ch;
   tcgetattr( STDIN_FILENO, &oldt );
   newt = oldt;
   newt.c_lflag &= ~( ICANON | ECHO );
   tcsetattr( STDIN_FILENO, TCSANOW, &newt );
   ch = getchar();
   tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
   return ch;
}

void tts(char msg[]) {
  char buff[256];
  sprintf(buff, "pico2wave -w test.wav \"%s\"", msg);
  system(buff);
  system("aplay -q test.wav");
  memset(buff, '\0', 256);
}

int   main( int argc, char **argv)
{
   system("clear");
   int   sock;
   int   server_addr_size;

   struct sockaddr_in   server_addr;

   char   buff_rcv[BUFF_SIZE+5];


   sock  = socket( PF_INET, SOCK_DGRAM, 0);

   if( -1 == sock)
   {
      printf( "socket 생성 실패n");
      exit( 1);
   }

   memset( &server_addr, 0, sizeof( server_addr));
   server_addr.sin_family     = AF_INET;
   server_addr.sin_port       = htons( 4000);
   server_addr.sin_addr.s_addr= inet_addr( "127.0.0.1");
   while(1){
      int cmd;
      char cmd_msg[3] = {0,0,0};

      printf("=================\n");
      printf("[명령어를 입력해주세요]\n");    tts("Insert Command.");
      printf("[1] 이동\n");               tts("1, Move.");
      printf("[2] 시작 / 일시정지\n");      tts("2, Pause or Resume.");
      printf("[3] 속도 조절\n");           tts("3, Adjust Velocity.");
      printf("[4] 수리\n");               tts("4, Repair.");
      printf("[5] 종료\n");               tts("5, Exit.");
      printf("=================\n");
      printf("[INPUT] : ");              tts("Input your number.");
      scanf("%d",&cmd);
      switch(cmd){
      case 1:{ // 상하 이동
         int current_floor, target_floor;
         int direction;
         printf("현재 몇 층에 있나요? (지하1층 -> -1 입력) : "); tts("Which floor are you in?");
         scanf("%d",&current_floor);
         if(current_floor == 0){
            printf("0층은 지하 1층으로 자동 변경됩니다.\n");
            current_floor = -1;
         }
         printf("올라가기[1] 내려가기[2] : "); tts("Input 1 for going up or Input 2 for going down.");
         scanf("%d",&direction);

         srand(time(NULL));

         if(direction == UP){
            if(current_floor == FLOOR_MAX){
               printf("여기가 끝층이야 못올라가\n");
               break;
            }else{
               target_floor = current_floor + rand()%(FLOOR_MAX - current_floor) + 1;
            }
         }else if(direction == DOWN){
            if(current_floor == FLOOR_MIN || current_floor == -1){
               printf("여기가 끝층이야 못내려가\n");
               break;
            }else{
               target_floor = current_floor - rand()%(current_floor - FLOOR_MIN) - 1;
            }
         }

         printf("[%d층] -> [%d층] 으로 이동\n",current_floor,target_floor);
         if(current_floor > target_floor){
            printf("내려갑니다\n");   tts("Going down.");
         }else if(current_floor < target_floor){
            printf("올라갑니다\n");   tts("Going up.");
         }

         cmd_msg[0] = 1;
         cmd_msg[1] = current_floor;
         cmd_msg[2] = target_floor;
         break;
      }
      case 2:{ // 시작 일시정지 
         cmd_msg[0] = 2;
         cmd_msg[1] = 1; //Not Used
         cmd_msg[2] = 0; //Not Used
         break;
      }
      case 3:{ // 속도 조절
         int target_speed;
         printf("1 : 0.5sec | 2 : 1sec | 3 : 2sec\n"); tts("Input 1 for 0.5 seconds, Input 2 for 1 second, or Input 3 for 2 seconds.");
         printf("엘레베이터 속도를 입력하세요 : ");
         scanf("%d",&target_speed); 
         cmd_msg[0] = 3;
         cmd_msg[1] = target_speed; 
         cmd_msg[2] = 0; //Not Used
         break;
      }
      case 4:{ // 수리 
         cmd_msg[0] = 4;
         cmd_msg[1] = 0; //Not Used
         cmd_msg[2] = 0; //Not Used
         break;
      }
      case 5:{ // 종료
         cmd_msg[0] = 5; 
         cmd_msg[1] = 0; //Not Used
         cmd_msg[2] = 0; //Not Used
         break;
         }
      }



      sendto( sock, &cmd_msg, strlen(&cmd_msg)+1, 0,    // +1: NULL까지 포함해서 전송
         ( struct sockaddr*)&server_addr, sizeof( server_addr));
      if(cmd == 5){
         return 0;
      }

      /*
      server_addr_size  = sizeof( server_addr);
      recvfrom( sock, buff_rcv, BUFF_SIZE, 0 , 
      ( struct sockaddr*)&server_addr, &server_addr_size);
      printf( "receive: %s\n", buff_rcv);
      */
   }
   close( sock);

   return 0;
}