#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <time.h>

#define BUFF_SIZE   1024
#define TRUE 1
#define FALSE 0

typedef struct _work
{
  int target_floor;
  int start_floor; 
  struct _work* next_work;
}work;

typedef work* work_ptr;

typedef struct _work_queue
{
  int work_count;
  work_ptr work_head;
}work_queue;

typedef struct _elevator
{
    work_ptr now_work;
    int target_count; //목표 횟수 (목표 횟수에 다다르면, 고장처리)
    int now_count; //현재 이동횟수
    int now_floor;
    int troubleFlag; // elevator malfunction
    int fullFlag; //is elevator full?
    int now_direction;
}elevator;

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


void enqueue_work(work_queue* work_queue_ptr,int add_start_floor,int add_target_floor){
  //TODO : 현재 들어온 Work를 큐에 넣어준다.
  work_ptr new_work_ptr = (work*)malloc(sizeof(work));
  new_work_ptr->target_floor = add_target_floor;
  new_work_ptr->start_floor = add_start_floor;
  new_work_ptr->next_work = work_queue_ptr->work_head;
  work_queue_ptr->work_head = new_work_ptr;
  work_queue_ptr->work_count++;
  printf("enqueue_work : %d\n",work_queue_ptr->work_count );
}

work_ptr dequeue_work(work_queue* work_queue_ptr){
  //TODO : 맨 처음 들어온걸 리턴하고, 큐에서 제거한다.  
  if(work_queue_ptr->work_count == 0){
    return NULL;
  }else{
    int i = 0;
    work_ptr tmp_work_ptr = work_queue_ptr->work_head;
    work_ptr prev_work_ptr;
    for(i = 0; i < work_queue_ptr->work_count; i++){
      prev_work_ptr = tmp_work_ptr;
      tmp_work_ptr = tmp_work_ptr->next_work;
    }

    work_queue_ptr->work_count--;
    if(work_queue_ptr->work_count == 0){
      work_queue_ptr->work_head = NULL;
    }else{
      prev_work_ptr->next_work = tmp_work_ptr->next_work;
    }
    printf("%d %d\n",prev_work_ptr->start_floor,prev_work_ptr->target_floor );
    return prev_work_ptr;
  }
}

work_ptr get_first_work(work_queue* work_queue_ptr){
  //TODO : 가장 첫번째 작업을 리턴해준다.
  if(work_queue_ptr->work_count == 0){
    return NULL;
  }else{
    int i = 0;
    work_ptr tmp_work_ptr = work_queue_ptr->work_head;
    for(i = 0; i < work_queue_ptr->work_count; i++){
      tmp_work_ptr = tmp_work_ptr->next_work;
    }
    return tmp_work_ptr;
  }
}

void init_work_queue(work_queue* work_queue_ptr){
  work_queue_ptr->work_count = 0;
  work_queue_ptr->work_head = NULL;
}

void init_elevator (elevator *elevator_ptr) {
  elevator_ptr->now_work = NULL;
  elevator_ptr->target_count = rand()%5+5;
  elevator_ptr->now_count = 0;
  elevator_ptr->now_floor = 1;
  elevator_ptr->troubleFlag = 0;
  elevator_ptr->fullFlag = 0;
  elevator_ptr->now_direction = 0;
}

//가장 중요해!!!
void worker(elevator *elevator_array[], work_queue* work_queue_ptr){
  
  int i = 0;
  //Work 배정
  printf("work_count %d\n",work_queue_ptr->work_count );
  
    
  for(i=0; i<3; i++) {
    if(work_queue_ptr->work_count > 0){
      if(elevator_array[i]->now_work == NULL && elevator_array[i]->troubleFlag == FALSE){
  
        elevator_array[i]->now_work = dequeue_work(work_queue_ptr);
        printf("배정 : %d번 || %d -> %d\n",i,elevator_array[i]->now_work->start_floor,elevator_array[i]->now_work->target_floor);
      }
    }  
  }
  printf("[DEBUG] WORK 1\n");
  //엘레베이터 이동
  for(i=0; i<3; i++) {
    printf("[DEBUG] WORK 2\n");
    if(elevator_array[i]->now_work != NULL){
      printf("[DEBUG] WORK 3\n");
      int direction;
      if(elevator_array[i]->fullFlag == TRUE){ //동작중
        if(elevator_array[i]->now_work->target_floor > elevator_array[i]->now_work->start_floor){
          direction = 1; //올라감
        }else if(elevator_array[i]->now_work->target_floor < elevator_array[i]->now_work->start_floor){
          direction = -1; //내려감
        }else{
          direction = 0;
          printf("elevator_array[i] %d\n",elevator_array[i]->now_floor);
          printf("%d %d\n",elevator_array[i]->now_work->target_floor , elevator_array[i]->now_work->start_floor );
          printf("1Error!!!!\n");
        }
        elevator_array[i]->now_direction = direction;
        elevator_array[i]->now_floor += direction; //한층씩 이동
        //도착함
        if(elevator_array[i]->now_floor == elevator_array[i]->now_work->target_floor){
          free(elevator_array[i]->now_work);
          elevator_array[i]->now_work = NULL;
          elevator_array[i]->now_count++;
          elevator_array[i]->fullFlag = FALSE;

          //고장신고는 111
          if(elevator_array[i]->now_count == elevator_array[i]->target_count){
            elevator_array[i]->troubleFlag = 1;
          }
        }
      }else{ //동작을 위해 이동
        elevator_array[i]->fullFlag = FALSE;
        if(elevator_array[i]->now_floor < elevator_array[i]->now_work->start_floor){
          direction = 1; //올라감
        }else if(elevator_array[i]->now_floor > elevator_array[i]->now_work->start_floor){
          direction = -1; //내려감
        }else if(elevator_array[i]->now_floor == elevator_array[i]->now_work->start_floor ){
          direction = 0;
          printf("%d %d\n",elevator_array[i]->now_floor , elevator_array[i]->now_work->start_floor );
          printf("same!!!!\n");
        }else{
          printf("Error2\n");
        }
        elevator_array[i]->now_direction = direction;
        elevator_array[i]->now_floor += direction; //한층씩 이동
        if(elevator_array[i]->now_floor == elevator_array[i]->now_work->start_floor){
          elevator_array[i]->fullFlag = TRUE;
        } 
      }
    }
  }

}

/*
void delay1(clock_t n)
{
  clock_t start = clock();
  while(clock() - start < n);
}
*/

void show(elevator *elevator_ptr[])
{
  // boolean elevatorExists;  : 0 = not exists  / 1 = exists
  // => elevator_ptr[j]->now_floor == i
  // boolean elevatorMove;    : 0 = not move    / 1 = move
  // => elevator_ptr[j]->now_work == NULL
  // boolean elevatorFull;    : 0 = Empty       / 1 = Full
  // => elevator_ptr[j]->fullFlag
  // boolean elevatorUp;      : 0 = going down  / 1 = going up
  // => elevator_ptr[j]->now_work->target_floor >= elevator_ptr[j]->now_work->start_floor
  // boolean elevatorTrouble; : 0 = normal      / 1 = abnormal
  // => elevator_ptr[j]->trounbleFlag
  // must be initiallized.
  // boolean values are from elevator Structure
  // ex. elevator->elevatorExists

  // floor 0 is floor -1
  
  int i, j;
  system("clear");
  printf("\n\n\n");
  printf("                       Elevator Simulation                      \n\n\n");
  //building ceiling
  printf("╔════════════════════╦════════════════════╦════════════════════╗\n");
  for (i = 10; i >= 0; i--) {
    //elevator top part
    for (j = 0; j<3; j++) {
      printf("║");
      if (elevator_ptr[j]->now_floor == i) { //elevatorExists
        //If there is person in elevator, then elevator is moving(elevatorMove is true).
        printf("┌──────────────────┐");
      }
      else {
        printf("                    ");
      }
    }
    printf("║\n");

    //elevator middle part
    for (j = 0; j<3; j++) {
      printf("║");
      if (elevator_ptr[j]->now_floor == i) { //elevatorExists
        //If there is person in elevator, then elevator is taken(elevatorMove is true).
        if(elevator_ptr[j]->troubleFlag) { //elevatorTrouble
          printf("│(!Trou)(%2dF)(BLE!)│", elevator_ptr[j]->now_floor);
        }
        else {
          if (elevator_ptr[j]->now_work == NULL) { //elevatorMove
            printf("│(EMPTY)(%2dF)(STOP)│", elevator_ptr[j]->now_floor);
          }
          else {
            if(elevator_ptr[j]->fullFlag) {
              //When elevator is going up, elevatorUp is true.
              if (elevator_ptr[j]->now_direction == 1) { //elevatorUp
                printf("│(FULL!)(%2dF)( UP )│", elevator_ptr[j]->now_work->target_floor);
              }
              else if(elevator_ptr[j]->now_direction == -1) {
                printf("│(FULL!)(%2dF)(DOWN)│", elevator_ptr[j]->now_work->target_floor);
              }
              else {
                printf("│(FULL!)(%2dF)(STOP)│", elevator_ptr[j]->now_work->target_floor);
              }
            }
            else {
              if (elevator_ptr[j]->now_direction == 1) { //elevator Up
                printf("│(EMPTY)(%2dF)( UP )│", elevator_ptr[j]->now_work->target_floor);
              }
              else if(elevator_ptr[j]->now_direction == -1){
                printf("│(EMPTY)(%2dF)(DOWN)│", elevator_ptr[j]->now_work->target_floor);
              }
              else {
                printf("│(EMPTY)(%2dF)(STOP)│", elevator_ptr[j]->now_work->target_floor);
              }
            }
          }
        }
      }
      else {
        printf("                    ");
      }
    }
    printf("║\n");

    //elevator bottom part
    for (j = 0; j<3; j++) {
      printf("║");
      if (elevator_ptr[j]->now_floor == i) { //elevatorExists
        //If there is person in elevator, then elevator is moving(elevatorMove is true).
        printf("└──────────────────┘");
      }
      else {
        printf("                    ");
      }
    }
    printf("║\n");

    //building bottom with floor marks
    if(i==0) printf("╠════════════════════╬════════════════════╬════════════════════╣ %2dF\n", i-1);
    else printf("╠════════════════════╬════════════════════╬════════════════════╣ %2dF\n", i);
  }
  for(j=0; j<3; j++) {
    printf("[Elevator%d] %d/%d Now :: %dF", j, elevator_ptr[j]->now_count ,elevator_ptr[j]->target_count ,elevator_ptr[j]->now_floor);
    if(elevator_ptr[j]->now_work != NULL) {
      printf("  /  Objective :: From %dF To %dF", elevator_ptr[j]->now_work->start_floor, elevator_ptr[j]->now_work->target_floor);
    }
    printf("\n");
  }
}

/*
void tts(char msg[]) {
  char buff[256];
  sprintf(buff, "pico2wave -w test.wav \"%s\"", msg);
  system(buff);
  system("aplay -q test.wav");
  memset(buff, '\0', 256);
}
*/

void elevatorTroubleCheck(elevator *elevator_ptr[]) {
// int target_count; //목표 횟수 (목표 횟수에 다다르면, 고장처리)
// int now_count; //현재 이동횟수
// int troubleFlag; // elevator malfunction
  int i;
  for(i=0; i<3; i++) {
    if(elevator_ptr[i]->now_count >= elevator_ptr[i]->target_count) {
      elevator_ptr[i]->troubleFlag = 1;
    }
  }
}

int main( void)
{
  //Socket
  int   sock;
  int   client_addr_size;

  struct sockaddr_in   server_addr;
  struct sockaddr_in   client_addr;

  char   buff_rcv[BUFF_SIZE+5];
  char   buff_snd[BUFF_SIZE+5];

  //Elevator
  elevator *elevator_array[3];
  work_queue *work_queue_ptr;
  work_queue_ptr = (work_queue*)malloc(sizeof(work_queue));

  //Work 
  int velocity = 1000;

  //View
  int   holdFlag;

  int i;
  

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
  
  //Setting
  for(i=0; i<3; i++) {
    elevator_array[i] = (elevator*) malloc(sizeof(elevator));
    init_elevator(elevator_array[i]);
    
  }
  
  init_work_queue(work_queue_ptr);
  
  while( 1)
  {
    client_addr_size  = sizeof( client_addr);
    int i = recvfrom( sock, buff_rcv, BUFF_SIZE, 0 , 
      ( struct sockaddr*)&client_addr, &client_addr_size);
    if(holdFlag == 0) {
      
      worker(elevator_array,work_queue_ptr);

      if(i > 1){
        switch(buff_rcv[0]){
        case 1:{ //move
          printf("[이동] %d층 -> %d층 \n",buff_rcv[1],buff_rcv[2]);
          if(buff_rcv[1] == -1){
            buff_rcv[1] = 0;
          }
          if(buff_rcv[2] == -1){
            buff_rcv[2] = 0;
          }
          enqueue_work(work_queue_ptr,buff_rcv[1],buff_rcv[2]);
          
          break;
        }
        case 2:{ //hold
          printf("[일시정지]\n");
          holdFlag = 1;
          break;
        }
        case 3:{ //velocity
          switch(buff_rcv[1]) {
            case 1: velocity = 500;  break;
            case 2: velocity = 1000; break;
            case 3: velocity = 1500; break;
          }
          printf("[속도 조절] %d단계\n",buff_rcv[1]);
          break;
        }
        case 4:{ //repair
          printf("[수리]\n");
          int index = 0;
          for(index = 0; index < 3; index++){
            elevator_array[index]->troubleFlag = FALSE;
            elevator_array[index]->target_count = rand()%5+5;//+ 20;
            elevator_array[index]->now_count = 0;            
          }
          break;
        }
        case 5:{ //quit
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

      show(elevator_array);
    } else {
      system("clear");
      printf("System :: 현재 일시정지중입니다.\n");
      if(i > 1) {
        if(buff_rcv[0] == 2) {
          holdFlag = 0;
          printf("System :: 일시정지 해제하였습니다.\n");
        }
      }
    }
    usleep(velocity*1000);
  }
}
