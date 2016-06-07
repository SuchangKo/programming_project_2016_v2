#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <time.h>

#define  BUFF_SIZE   1024

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




void delay1(clock_t n)
{
  clock_t start = clock();
  while(clock() - start < n);
}

int   main( void)
{
   int   sock;
   int   client_addr_size;

   struct sockaddr_in   server_addr;
   struct sockaddr_in   client_addr;

   char   buff_rcv[BUFF_SIZE+5];
   char   buff_snd[BUFF_SIZE+5];



   sock  = socket( PF_INET, SOCK_DGRAM, 0);
   fcntl(sock, F_SETFL, O_NONBLOCK);
   if( -1 == sock)
   {
      printf( "socket 생성 실패n");
      exit( 1);
   }

   memset( &server_addr, 0, sizeof( server_addr));
   server_addr.sin_family     = AF_INET;
   server_addr.sin_port       = htons( 4000);
   server_addr.sin_addr.s_addr= htonl( INADDR_ANY);

   if( -1 == bind( sock, (struct sockaddr*)&server_addr, sizeof( server_addr) ) )
   {
      printf( "bind() 실행 에러n");
      exit( 1);
   }

   while( 1)
   {
      client_addr_size  = sizeof( client_addr);
      int i = recvfrom( sock, buff_rcv, BUFF_SIZE, 0 , 
                     ( struct sockaddr*)&client_addr, &client_addr_size);
      //clearScreen();
        if(i > 1){
            system("clear");
            switch(buff_rcv[0]){
                case 1:{
                    printf("[이동] %d층 -> %d층 \n",buff_rcv[1],buff_rcv[2]);
                    break;
                }
                case 2:{
                    if(buff_rcv[1] == 1){
                        printf("[일시정지]\n");
                    }else{
                        printf("[시작]\n");
                    }
                    break;
                }
                case 3:{
                    printf("[속도 조절] %d단계\n",buff_rcv[1]);
                    break;
                }
                case 4:{
                    printf("[수리]\n");
                    break;
                }
                case 5:{
                    printf("[종료]\n");
                    return 0;
                    break;
                }
            }
            //printf( "1eceive: %d %s %d \n",i, buff_rcv,strlen(buff_rcv));

          
            sprintf( buff_snd, "%s%s", buff_rcv, buff_rcv);
            sendto( sock, buff_snd, strlen( buff_snd)+1, 0,  // +1: NULL까지 포함해서 전송
                         ( struct sockaddr*)&client_addr, sizeof( client_addr)); 
    }
      delay1(100000);
   }
}