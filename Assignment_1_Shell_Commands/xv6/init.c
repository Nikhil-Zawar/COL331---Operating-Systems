// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char *argv[] = { "sh", 0 };

#ifndef USERNAME
#define USERNAME "NONE"
#endif

#ifndef PASSWORD
#define PASSWORD "NONE"
#endif

#define MAX_ATTEMPTS 3

int compare_strings(const char *str1, const char *str2) {
  while (*str1 && *str2) {
      if (*str1 != *str2) {
          return 0;
      }
      str1++;
      str2++;
  }
  if (*str1 == '\n' && *str2 == '\0') {
      return 1;
  } else {
      return 0;
  }
}

int authenticate(void) {
    char user[32], pass[32];
    int mistakes_count = 0;
    while(1){
      printf(1, "Enter Username: ");
      gets(user, sizeof(user));
      if(compare_strings(user, USERNAME) == 0) {
        printf(1, "Invalid username\n");
        mistakes_count += 1;
      }else{
        int temp_count = mistakes_count;
        for(int i=temp_count; i<MAX_ATTEMPTS; i++){
          printf(1, "Enter Password: ");
          gets(pass, sizeof(pass));
          if(compare_strings(pass, PASSWORD) == 0){
            printf(1, "Invalid password\n");
            mistakes_count += 1;
          }else{
            // successful login
            return 1;
          }
        }
      }
      if(mistakes_count == MAX_ATTEMPTS){
        break;
      }
    }
    return 0;
}


int main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr
  
  
  // printf(1, "entering init: main function\n");
  int result = authenticate();
  if(result == 0){
    while(1){
      sleep(100);
    }
  }
  // printf(1, "exiting init: main function\n");
  for(;;){
    // printf(1, "init: starting sh\n");
    pid = fork();
    if(pid < 0){
      // printf(1, "init: fork failed\n");
      exit();
    }
    if(pid == 0){
      exec("sh", argv);
      // printf(1, "init: exec sh failed\n");
      exit();
    }
    while((wpid=wait()) >= 0 && wpid != pid)
      printf(1, "zombie!\n");
  }
}
