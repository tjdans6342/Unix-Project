#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include "locker.h"

#define DEFAULT_PROTOCOL 0
#define MAXLINE 1000
#define LOCK_TIME 30
#define true 1
#define false 0

// connect variables
struct sockaddr_un serverUNIXaddr; int result;

int clientfd;
char cmsg[1000][MAXLINE], smsg[1000][MAXLINE];
char trash[MAXLINE];


// 전역변수들
int num_try_id, num_try_pw;
int is_exist;


// 정의된 함수들
void Read(char *s);
void Write(char *s);
void sigstmpHandler(int singo);
void alarmHandler(int singo);
void make_contents_list(int *num_of_content, char contents_list[][30], char *sss);


int t;
int pid, child_pid;
int flag = 0;

int main ( )
{
  signal(SIGALRM, alarmHandler);

  // connect process with server --------------------------------------------
  clientfd = socket(AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
  serverUNIXaddr.sun_family = AF_UNIX;
  strcpy(serverUNIXaddr.sun_path, "convert");
  do { /* 연결 요청 */
    result = connect(clientfd, &serverUNIXaddr, sizeof(serverUNIXaddr));
    if (result == -1) sleep(1);
  } while (result == -1);
  // ------------------------------------------------------------------------


  // 0. 라커 정보 보여주기 (id) -------------------------------------------------
  printf("---------- [프로그램 시작!!!] ----------\n");

  START: // READ

  Read(smsg[0]);
  printf("\n존재하는 사물함 ID: %s\n", smsg[0]);

  // 1. 로그인/회원가입 과정 ----------------------------------------------------
  printf("1. 로그인\n");
  printf("2. 회원가입\n");
  printf("진행할 과정을 고르시오 (1또는 2를 입력): ");

  while (1) {
    scanf("%s", cmsg[0]);
    if (cmsg[0][0] == '1' || cmsg[0][0] == '2') break;
    printf("1또는 2를 입력하시오: ");
  }
  Write(cmsg[0]); // 1 or 2


  if (cmsg[0][0] == '1') { // 1. 로그인
    char contents_list[50][30]; // content를 담은 배열
    int num_of_content = 0; // contents_list의 크기

    LOGIN:
    Read(trash);
    printf("\n[아이디 입력 과정]\n");

    num_try_id = 0;

    do {
      printf("\n[server] [try:%d] 아이디를 입력하시오: ", ++num_try_id);
      scanf(" %s", cmsg[1]);
      Write(cmsg[1]);  // id
      Read(smsg[1]); // 'id is exist' or 'id is not exist'
      if (strcmp(smsg[1], "id is exist") == 0) break;
      printf("[server] 아이디를 존재하지 않습니다.\n");
      if (num_try_id >= 3) {
        goto REGISTER;
      }
    } while(1);

    if (strcmp(smsg[1], "id is exist") == 0) { // 로그인진행

      ID_SUCESS:

      num_try_pw = 0;

      do {
        printf("[server] 비밀번호를 입력하시오 (%d/3): ", ++num_try_pw);
        scanf(" %s", cmsg[2]);

        Write(cmsg[2]); // password
        Read(smsg[2]); // 'correct' or 'not correct'
        // printf("%s\n", smsg[2]);

        if (strcmp(smsg[2], "correct") == 0) break;
        else if (strcmp(smsg[2], "already used") == 0) {
          printf("※ [server] 해당 아이디 '%s'는 이미 사용중입니다.\n", cmsg[1]);
          Write(trash);
          goto START;
        }
        else {
          printf("[server] 비밀번호 틀렸음!\n");

          if (strcmp(smsg[2], "go to back") == 0) {
            int child, status;

            t = LOCK_TIME;
            printf("\n[server] %d초 잠금이 시작되었습니다!\n", t);

            pid = fork();
            if (pid == 0) {
              signal(SIGALRM, alarmHandler);
              alarmHandler(SIGALRM);
              while (flag != 1);
              exit(0);
            }
            else {
              child = wait(&status);
              printf("\n[server] 부모 프로세스 다시 시작!\n");
            }

            goto ID_SUCESS;
          }
        }

      } while(1);

      // pw 일치
      Write(trash); Read(smsg[3]); // contents
      make_contents_list(&num_of_content, contents_list, smsg[3]);

      printf("\n---------- [로그인 성공!] [ID: %s] ----------\n\n", cmsg[1]);


      LOGIN_SUCCESS: // Write

      do {
        printf(" \n 〓〓〓〓〓〓〓〓〓〓〓〓〓〓〓〓〓〓〓〓\n");
        printf("【 1. 컨텐츠 조회%26s\n", "】");
        printf("【 2. 컨텐츠 추가%26s\n", "】");
        printf("【 3. 컨텐츠 삭제%26s\n", "】");
        printf("【 4. 로그아웃%29s\n", "】");
        printf(" 〓〓〓〓〓〓〓〓〓〓〓〓〓〓〓〓〓〓〓〓\n");
        printf("\n[server] 진행할 과정을 고르시오 (1,2,3,4 중에 입력): ");
        scanf(" %s", cmsg[6]);
        if ((strcmp(cmsg[6], "1") && strcmp(cmsg[6], "2") && 
            strcmp(cmsg[6], "3") && strcmp(cmsg[6], "4")) == 0) {
          break;
        }
        printf("\n※ [server] 1, 2, 3, 4 중에 입력해주세요!\n");
      } while(1);

      if (cmsg[6][0] == '2' && num_of_content == 50) {
        printf("\n※ [server] 컨텐츠가 모두 차서 더이상 추가할 수 없습니다. (%d/50)\n", num_of_content);
        goto LOGIN_SUCCESS;
      }

      Write(cmsg[6]);
      Read(trash);

      switch(cmsg[6][0]) {

        case '1': // 컨텐츠 조회
          printf("\n\n[해당 사용자의 사물함 정보] (%d/50)\n", num_of_content);
          for (int i=0; i<num_of_content; i++) {
            printf("[%d] %s\n", i, contents_list[i]);
          }
          printf("\n");
          goto LOGIN_SUCCESS;
          break;


        case '2': // 컨텐츠 추가
          do {
            printf("\n\n[server] 추가할 컨텐츠를 입력하시오: ");
            scanf(" %s", cmsg[7]); // content

            is_exist = false;
            for (int i=0; i<num_of_content; i++) {
              if (strcmp(cmsg[7], contents_list[i]) == 0) 
                is_exist = true;
            }

            if (!is_exist) break;
            printf("\n[server] 해당 아이템 '%s'는 이미 존재합니다!\n", cmsg[7]);
          } while(1);

          Write(cmsg[7]);
          printf("\n※ [server] 컨텐츠 '%s'이 추가되었습니다!\n", cmsg[7]);
          strcpy(contents_list[num_of_content++], cmsg[7]);

          printf("\n");
          Read(trash);
          goto LOGIN_SUCCESS;
          break;


        case '3': // 컨텐츠 삭제
          printf("\n\n[server] 삭제할 컨텐츠를 입력하시오: ");
          scanf(" %s", cmsg[8]); // content
          Write(cmsg[8]);
          Read(trash);

          is_exist = false;
          for (int i=0; i<num_of_content; i++) {
            if (strcmp(cmsg[8], contents_list[i]) == 0) 
              is_exist = true;
          }

          if (is_exist) {
            printf("\n※ [server] 컨텐츠 '%s'이 삭제되었습니다!\n", cmsg[8]);
            Write("delete"); // cmsg[9]

            int is_pull = false;
            for (int i=0; i<num_of_content; i++) {
              if (is_pull) strcpy(contents_list[i-1], contents_list[i]);
              if (strcmp(cmsg[8], contents_list[i]) == 0) is_pull = true;
            }
            num_of_content--;
          }
          else {
            printf("\n[server] 컨텐츠 '%s'은 존재하지 않는 컨텐츠입니다!\n", cmsg[8]);
            Write("not delete"); // cmsg[9];
          }

          goto LOGIN_SUCCESS;
          break;


        case '4': // 로그아웃
          Write(trash);
          printf("\n※ [server] 아이디 '%s'에서 로그아웃 되었습니다. \n\n", cmsg[1]);
          goto START;
          break;


        default: // 예외처리
          exit(0);
      }

    }
  }


  else { // 2. 회원가입

    REGISTER:
    Read(trash);
    printf("\n---------- [회원가입 과정 시작!] ----------\n");

    do {
      printf("\n[server] 사용할 사물함의 아이디를 입력하시오: ");
      scanf(" %s", cmsg[3]); // register ID

      Write(cmsg[3]);
      Read(smsg[4]);

      if (strcmp(smsg[4], "id is not exist!") == 0) break;
      printf("[server] 해당 아이디는 이미 존재합니다.\n");
    } while(1);

    REGISTER_PW:

    printf("\n[server] 사용하실 비밀번호를 입력하시오 (길이 5이상, 영어와 숫자 포함): ");
    scanf(" %s", cmsg[4]); // register PW

    Write(cmsg[4]);
    Read(smsg[5]);

    if (strcmp(smsg[5], "satisfy") == 0) {
      int try = 0;
      do {
        printf("[server] 비밀번호를 한 번더 입력해주세요 (%d/3):", ++try);
        scanf(" %s", trash);
        if (strcmp(trash, cmsg[4]) == 0) break;
        if (try >= 3) {
          Write("not satisfy register condition"); // cmsg[5]
          printf("\n[server] 3번 틀려서 비밀번호 생성을 다시 해주세요!");
          goto REGISTER_PW;
        }
      } while(1);

      Write("satisfy register condition"); // cmsg[5]
      printf("※ [server] 회원가입 완료!\n");
      printf("\n\n\n");
      goto START;
    }
    else {
      printf("\n[server] 비밀번호 조건에 맞게 생성해주세요!");
      goto REGISTER_PW;
    }
  } 



  // 서버와 연결 해제
  close(clientfd);
  exit(0);
}


void Read(char *s) {
  read(clientfd, s, MAXLINE);
}

void Write(char *s) {
  write(clientfd, s, MAXLINE);
}

void sigstmpHandler(int signo) {
  printf("[server] %d초 뒤에 잠금이 풀립니다람쥐!\n", t);
}

void alarmHandler(int signo) {
  if (t == 0) {
    printf("※ [server] %d초 완료!\n", LOCK_TIME);
    kill(child_pid, SIGINT);
    flag = 1;
    return;
  }

  if (t == LOCK_TIME) {
    signal(SIGTSTP, sigstmpHandler);
    int p_pid = getpid();
    child_pid = fork();
    if (child_pid == 0) { // 자식
      char s[100];
      while (1) {
        scanf(" %s", s);
        kill(p_pid, SIGTSTP);
      }
      exit(0);
    }
  }

  t -= 1;
  alarm(1);

}


void make_contents_list(int *num_of_content, char contents_list[][30], char *sss) {
  char tmp[100];
  int tidx = 0;
  int cur = 0;
  while (sss[cur] != 0) {
    if (sss[cur] == ' ') {
      tmp[tidx] = 0;
      strcpy(contents_list[(*num_of_content)++], tmp);
      tidx = 0;
    }
    else {
      tmp[tidx++] = sss[cur];
    }
    cur ++;
  }
}
