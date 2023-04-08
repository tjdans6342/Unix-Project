#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include "locker.h"

void intHandler();


/* Save locker information */
int main(int argc, char *argv[])
{
   int pid;
   FILE *fp;
   struct locker record;
   fp = fopen("lockerdb", "wb");

   printf("%-9s %-8s %-4s\n", "ID", "PW", "contents");
   int idx = 0;
   while (scanf("%s %s", record.id, record.pw) == 2) {

      int num;
      printf("input number of contents: ");
      scanf(" %d", &num);

      record.is_used = 0;

      record.num = num;
      int i;
      for (i=0; i<num; i++) {
         char c[30];
         scanf(" %s", c);
         strcpy(record.contents[i], c);
      }

      idx++;
      fseek(fp, (long)(idx - START_IDX)*sizeof(record), SEEK_SET);
      fwrite((char *)&record, sizeof(record), 1, fp);
   }
   fflush(fp);
   fclose(fp);
}

void intHandler(int signo, char *argv[])
{
   char *args[] = {"./ac_update", "acdb", NULL};
   execv(args[0], args);
}