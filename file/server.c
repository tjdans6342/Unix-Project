#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <fcntl.h>
#include "locker.h"
#define DEFAULT_PROTOCOL 0
#define MAXLINE 1000
#define true 1
#define false 0

// connect variables
struct sockaddr_un serverUNIXaddr, clientUNIXaddr; int listenfd, clientlen;

int connfd;
char cmsg[100][MAXLINE], smsg[100][MAXLINE]; // read, write
char trash[MAXLINE];

struct flock lock;

// 전역변수들
struct locker record;
FILE *fp;
int num_list[MAXLINE], is_used_list[MAXLINE];
char pw_list[MAXLINE][100], id_list[MAXLINE][100];
char contents_list[MAXLINE][50][30];
int num_of_locker = 0;

int client_num;
int num_try_id, num_try_pw;

int is_lock_success = false, lock_idx = -1;
int ridx = -1;

// 정의된 함수들
void reset_record();
void update_record(int idx);
void update_record_to_db(int idx);
void update_db();
void print_locker_info();
void Read(char *s);
void Write(char *s);
int satify_pw(char *pw);



int main ()
{
         
   update_db();

   fp = fopen("lockerdb", "r+");
   for(int i=0; i<num_of_locker; i++) {
      fseek(fp, (i)*sizeof(record), SEEK_SET);
      fread(&record, sizeof(record), 1, fp);

      // printf("%d :  record_is_used: %d\n", i, record.is_used);
      record.is_used = false;
      // printf("%d :  record_is_used: %d\n", i, record.is_used);

      fseek(fp, (i)*sizeof(record), SEEK_SET);
      fwrite((char *)&record, sizeof(record), 1, fp);
   }
   fclose(fp);

   // primary setting --------------------------------------------
   signal(SIGCHLD, SIG_IGN); // SIGCHLD signal is ignored
   clientlen = sizeof(clientUNIXaddr); // size of struct sockaddr_un

   listenfd = socket(AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
   serverUNIXaddr.sun_family = AF_UNIX;
   strcpy(serverUNIXaddr.sun_path, "convert");
   unlink("convert");
   bind(listenfd, &serverUNIXaddr, sizeof(serverUNIXaddr));

   listen(listenfd, 5);
   // ------------------------------------------------------------------------


   while (1) {
      connfd = accept(listenfd, &clientUNIXaddr, &clientlen); // 소켓 연결 요청 수락
      printf("[%d] Connect is succes sed!\n", ++client_num);


      if (fork () == 0) { // 클라이언트를 자식 프로세스를 생성하여 할당
         // 클라이언트와 연결 성공


         // 0. 라커 정보 보여주기 (id, contents) -----------------------------------
         START: // Write

         if (lock_idx != -1 && is_used_list[lock_idx]) {
            is_used_list[lock_idx] = false;

            reset_record();
            update_record(lock_idx);
            record.is_used = false;
            update_record_to_db(lock_idx);
         }
         update_db();

         // printf("is_used_list[0] : %d\n", is_used_list[0]);
         print_locker_info();

         // 1. 로그인/회원가입 과정 -------------------------------------------
         printf("-------------- 로그인/회원가입 과정 --------------\n");
         Read(cmsg[0]); // 1 or 2


         if (cmsg[0][0] == '1') { // 1. 로그인
            LOGIN:
            update_db();
            Write(trash);

            num_try_id = 0;
            printf("[client %d] 로그인 과정 시작\n", client_num);

            int is_exist;
            do { // smsg[2]
               is_exist = false;
               Read(cmsg[1]); // lockerID
               printf("[client %d] [try:%d] 받은 아이디: %s\n", client_num, ++num_try_id, cmsg[1]);
               for (int i=0; i<num_of_locker; i++) {
                  if (strcmp(cmsg[1], id_list[i]) == 0) {
                      is_exist = true;
                      ridx = i; break;
                  }
               }
               if (is_exist) {
                  Write("id is exist"); 
                  break;
               }
               else {
                  Write("id is not exist");
                  if (num_try_id >= 3) {
                     goto REGISTER;
                  }
               }

            } while(1);


            if (is_exist) { // 로그인 진행

               ID_SUCCESS:
               update_db();

               num_try_pw = 0;

               printf("[client %d] 아이디 맞음\n", client_num);
               printf("사용자 아이디는 %d번째 아이디임.\n", ridx);

               do {
                  Read(cmsg[2]); // password
                  printf("[client %d] 비밀번호 받음\n", client_num);

                  if (strcmp(cmsg[2], pw_list[ridx]) == 0) {

                     if (!is_used_list[ridx]) {
                        Write("correct");
                        is_used_list[ridx] = true; lock_idx = ridx;

                        reset_record();
                        update_record(ridx);
                        record.is_used = true;
                        update_record_to_db(ridx);
                        update_db();

                        break;
                     }
                     else { // 해당 아이디가 이미 사용중
                        Write("already used");
                        printf("[client %d] 아이디 '%s'는 이미 사용중\n", client_num, cmsg[1]);
                        Read(trash);
                        goto START;
                     }
                  }
                  else {
                     num_try_pw++;
                     printf("비밀번호 %d번 틀림\n", num_try_pw);

                     if (num_try_pw >= 3) {
                        Write("go to back");
                        goto ID_SUCCESS;
                     }
                     else Write("not correct");
                  }
               } while (1);

               // pw 일치
               printf("[client %d] 비밀번호 일치 (%s = %s)\n", client_num, pw_list[ridx], cmsg[2]);
               
               strcpy(smsg[3], "");
               for(int i=0; i<num_list[ridx]; i++) {
                  strcat(smsg[3], contents_list[ridx][i]);
                  strcat(smsg[3], " ");
               }
               printf("[client %d] %s\n", client_num, smsg[3]);
               Read(trash); Write(smsg[3]); // contents

               printf("[client %d] 로그인 성공\n", client_num);


               LOGIN_SUCCESS: // Read
               update_db();

               printf("[client %d] '%s' 로 로그인 하였음.\n", client_num, cmsg[1]);
               Read(cmsg[6]); // case 1, 2, 3, 4
               Write(trash);

               switch(cmsg[6][0]) {

                  case '1': // 컨텐츠 조회 (서버에서 수행할 필요x)
                     printf("[client %d] 컨텐츠 조회 진행\n", client_num);
                     goto LOGIN_SUCCESS;
                     break;


                  case '2': // 컨텐츠 추가
                     printf("[client %d] 컨텐츠 추가 진행\n", client_num);
                     Read(cmsg[7]); // content (that will be add)

                     update_record(ridx);
                     strcpy(record.contents[record.num++], cmsg[7]);

                     update_record_to_db(ridx);
                     update_db();

                     Write(trash);
                     goto LOGIN_SUCCESS;
                     break;


                  case '3': // 컨텐츠 삭제
                     Read(cmsg[8]); // content (that will be delete)
                     Write(trash);
                     Read(cmsg[9]); // "delete" or "not delete"

                     update_record(ridx);

                     int is_pull = false;
                     if (strcmp(cmsg[9], "delete") == 0) {
                        for (int i=0; i<record.num; i++) {
                           if (is_pull) {
                              strcpy(record.contents[i-1], record.contents[i]);
                           }
                           if (strcmp(cmsg[8], record.contents[i]) == 0) is_pull = true;
                        }
                        memset(record.contents[--record.num], '\0', 30);

                        update_record_to_db(ridx);
                        update_db();
                        printf("[client %d] 컨텐츠 '%s'를 삭제했습니다\n", client_num, cmsg[8]);
                     }

                     goto LOGIN_SUCCESS;
                     break;
                  

                  case '4': // 로그아웃
                     Read(trash);
                     goto START;
                     break; 


                  default: // 예외처리
                     return 0;
               }
            }
         }


         else { // 2. 회원가입

            REGISTER:
            update_db();

            Write(trash);
            printf("회원가입 과정 시작!\n");

            do {
               Read(cmsg[3]); // register ID

               int is_exist = false;
               for (int i=0; i<num_of_locker; i++) {
                  if (strcmp(cmsg[3], id_list[i]) == 0) is_exist = true;
               }
               if (!is_exist) {
                  Write("id is not exist!"); // smsg[4]
                  break;
               }
               else {
                  Write("id is already exist"); // smsg[4]
                  printf("[client %d] 아이디가 존재함\n", client_num);
               }
            } while(1);

            REGISTER_PW:
            update_db();

            do {
               Read(cmsg[4]); // register PW

               if (satify_pw(cmsg[4]) == 1) { // 비밀번호 조건 만족
                  Write("satisfy");
                  Read(cmsg[5]); // cmsg[5]

                  if (strcmp(cmsg[5], "satisfy register condition") == 0) {
                     reset_record();

                     strcpy(record.id, cmsg[3]);
                     strcpy(record.pw, cmsg[4]);
                     record.num = 0;

                     update_record_to_db(num_of_locker);
                     update_db();
                     break;
                  }
                  else goto REGISTER_PW;
               }
               else {
                  Write("not satisfy"); // smsg[5]
               }
            } while(1);

            printf("[client %d] 회원가입 완료!\n", client_num);
            printf("\n\n\n");
            goto START;
         }

         // 클라이언트와 연결 해제
         pclose(connfd);
         printf("[client %d] 연결해제됨.\n", client_num);
         exit (0);
      } else close(connfd);
   }

   return 0;
}









void reset_record() {
   record.num = 0;
   memset(record.id, '\n', 100);
   memset(record.pw, '\n', 100);
   memset(record.contents, '\0', 50*30);
}


void update_record(int idx) {
   reset_record();

   strcpy(record.id, id_list[idx]); // id
   strcpy(record.pw, pw_list[idx]); // pw
   record.num = num_list[idx]; // num

   for (int i=0; i<num_list[idx]; i++) { // contents
      strcpy(record.contents[i], contents_list[idx][i]);
   }
}


void update_record_to_db(int idx) {
   fp = fopen("lockerdb", "r+");
   fseek(fp, (idx)*sizeof(record), SEEK_SET);
   fwrite((char *)&record, sizeof(record), 1, fp);

   fflush(fp);
   fclose(fp);
}


void update_db() {
   reset_record();

   num_of_locker = 0;
   fp = fopen("lockerdb", "r+");

   for (; 1; num_of_locker++) {
      fseek(fp, num_of_locker*sizeof(record), SEEK_SET);
      if (fread(&record, sizeof(record), 1, fp) == 0) break;
      is_used_list[num_of_locker] = record.is_used;
      strcpy(id_list[num_of_locker], record.id); // id
      strcpy(pw_list[num_of_locker], record.pw); // pw
      num_list[num_of_locker] = record.num; // num
      for (int i=0; i<record.num; i++) { // contents
         strcpy(contents_list[num_of_locker][i], record.contents[i]);
      }
   }

   fclose(fp);
}

void print_locker_info() {
   fp = fopen("lockerdb", "r+");
   strcpy(smsg[0], "{");
   for(int i=0; i<num_of_locker; i++) {
      strcat(smsg[0], id_list[i]);
      if (i == num_of_locker - 1) strcat(smsg[0], "}");
      else strcat(smsg[0], ", ");
   }

   Write(smsg[0]);
   printf("num of locker: %d\n", num_of_locker);

   printf("--------------- locker info ---------------\n");
   for (int i=0; i<num_of_locker; i++) {
      fseek(fp, i*sizeof(record), SEEK_SET);
      fread(&record, sizeof(record), 1, fp);

      printf("[ID: %s] [PW: %s] [number of items: %d] [is_used: %d]\n", record.id, record.pw, record.num, record.is_used);

      printf("{");
      for (int j=0; j<record.num; j++) {
         if (j == record.num-1) printf("%s", record.contents[j]);
         else printf("%s, ", record.contents[j]);
      }
      printf("}\n\n");
   }
   fclose(fp);
}

void Read(char *s) {
  read(connfd, s, MAXLINE);
}

void Write(char *s) {
  write(connfd, s, MAXLINE);
}

int satify_pw(char *pw) {
   int is_ok = true;

   // cond1. satisfy that length >= 5
   int pw_size = strlen(pw);
   if (pw_size < 5) is_ok = false;

   // cond2. satisfy that contain num and english.
   int is_num, is_engish;
   is_num = is_engish = false;
   for (int i=0; i<pw_size; i++) {
      if (pw[i]>='0' && pw[i]<='9') is_num = true;
      if ((pw[i]>='A' && pw[i]<='Z') || (pw[i]>='a' && pw[i]<='z')) is_engish = true;
   }
   is_ok &= (is_num & is_engish);

   return is_ok;
}