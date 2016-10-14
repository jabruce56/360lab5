//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <sys/wait.h>
// typedef  struct proc{
//   struct proc * next;
//   int pid;         // process pid
//   int ppid;        // parent pid
//   int status;      // FREE|READY|RUNNING|SLEEP|STOPPED|ZOMBIE, etc.
//
//   (many other fileds)
//
//   OFT fd[NFD];     // opened file descriptors
// }PROC;
DIR *dir;
#define BLKSIZE 1024
char *myargv[10], npath[256], cwd[128];
char *path[9]= {"/", "/bin", "/sbin", "/usr/bin", "/usr/sbin", "/usr/games", "/usr/local/bin", "/usr/local/sbin", "/usr/local/games"};
int pd[2], fd;

int findredir(int i){
  //find redirection and save for later
  for(i;i<10&&myargv[i]!=NULL;i++){
    if(!strncmp(myargv[i], "<", 1)){
      return i;
    }
    else if(!strncmp(myargv[i], ">", 1)){
      return i;
    }
    else if(!strncmp(myargv[i], ">>", 2)){
      return i;
    }
  }
  return -1;
}
doredir(){
  //handle I/O redirection
  int r;
  r=findredir(1);
  if(!strncmp(myargv[r], "<", 1)){
    close(0);
    if(myargv[r+1][0]=='/'){
      fd = open(myargv[r+1], O_RDONLY);
      if (fd < 0){
        printf("open failed\n");
        exit(1);
      }
    }
    else{
      getcwd(cwd, 128);
      strcpy(npath, cwd);
      strcat(npath, "/");
      strcat(npath, myargv[r+1]);
      fd = open(npath, O_RDONLY);
      if (fd < 0){
        printf("open failed\n");
        exit(1);
      }
    }
    myargv[r]=NULL;
    myargv[r+1]=NULL;
  }
  else if(!strncmp(myargv[findredir(1)], ">", 1)){
    close(1);
    if(myargv[r+1][0]=='/'){
      fd = open(myargv[r+1], O_WRONLY|O_CREAT, 0644);
      if (fd < 0){
        printf("open failed\n");
        exit(1);
      }
    }
    else{
      getcwd(cwd, 128);
      strcpy(npath, cwd);
      strcat(npath, "/");
      strcat(npath, myargv[r+1]);
      fd = open(npath, O_WRONLY|O_CREAT, 0644);
      if (fd < 0){
        printf("open failed\n");
        exit(1);
      }
    }
    myargv[r]=NULL;
    myargv[r+1]=NULL;
  }
  else if(!strncmp(myargv[findredir(1)], ">>", 2)){
    close(1);
    if(myargv[r+1][0]=='/'){
      fd = open(myargv[r+1], O_WRONLY | O_APPEND, 0644);
      if (fd < 0){
        printf("open failed\n");
        exit(1);
      }
    }
    else{
      getcwd(cwd, 128);
      strcpy(npath, cwd);
      strcat(npath, "/");
      strcat(npath, myargv[r+1]);
      fd = open(npath, O_WRONLY | O_APPEND, 0644);
      if (fd < 0){
        printf("open failed\n");
        exit(1);
      }
    }
    myargv[r]=NULL;
    myargv[r+1]=NULL;
  }//end i/o redir
}

main(int argc, char *argv[], char *env[]){
  char line[256], *token, *hdir, *head[10], *tail[10];
  int ex=0, i, pid, status, redir, r, p;
  struct stat d;
  struct tm *tm;
  struct dirent *dp;


  while(!ex){
    r=0;
    printf("jbsh :");
    bzero(line, 256);
    token = NULL;
    for(i=0;i<10;i++){myargv[i]=NULL;head[i]=NULL;tail[i]=NULL;}
    fgets(line, 256, stdin);
    line[strlen(line)-1] = 0;        // kill \n at end
    if (line[0]==0){                  // exit if NULL line
      continue;
    }
    token = strtok(line, " ");
    for(i=0, p=0;token!=NULL&&i<10;i++){
      myargv[i]=token;
      if(!strncmp(token, "|", 1)){
        p=i;
      }
      token = strtok(NULL, " ");
    }
    //myargv[i]=NULL;



    // if(!strncmp(myargv[0], "cd", 2)){
    //   if(i<=1){//only cd
    //     hdir = getenv("HOME");
    //     chdir(hdir);//go to HOME
    //     getcwd(cwd, 128);
    //     printf("%s\n", cwd);
    //   }
    //   else{
    //     if(myargv[1][0]=='/'){//new path
    //       chdir(myargv[1]);
    //       getcwd(cwd, 128);
    //       printf("%s\n", cwd);
    //     }
    //     else{//go from cwd
    //       getcwd(cwd, 128);
    //       strcat(cwd, "/");
    //       strcat(cwd, myargv[1]);
    //       chdir(cwd);
    //       getcwd(cwd, 128);
    //       printf("%s\n", cwd);
    //     }
    //   }
    // }
    if(!strncmp(myargv[0], "exit", 4)){
      exit(1);
    }

    else{//fork
      if(p>0){//pipin time
        for(i=0;i<10;i++){

        }
        r = pipe(pd);//create pipe with p[0]=read, p[1]=write
        pid = fork();
        if(pid==-1){//fork error
          fprintf(stderr, "fork error %d\n", errno);
          continue;
        }
        else if(pid){//parent of pipe
          close(pd[0]);
          close(1);
          dup(pd[1]);
        }
        else{//child of pipe
          close(pd[1]);
          close(0);
          dup(pd[0]);
        }
      }
      else{//no pipe
        pid = fork();
        if(pid==-1){//fork error
          fprintf(stderr, "fork error %d\n", errno);
          continue;
        }

        else if(pid){//parent process
          pid=wait(&status);
          printf("status %d\n", status);
          continue;
        }

        else{//child process
          //find command and execute
          for(i=0;i<9;i++){
            dir=opendir(path[i]);
            while((dp=readdir(dir))!=NULL){
              if(!strncmp(dp->d_name, myargv[0], strlen(myargv[0]))){
                if (findredir(0)>-1){
                  doredir();
                }
                strcpy(npath, path[i]);
                strcat(npath, "/");
                strcat(npath, myargv[0]);
                if(access(npath, X_OK)!=-1){
                  execve(npath, myargv, env);
                }
                // printf("%d\n", findredir(0));
                // if(findredir(0)>-1){//if there was redirection close file and reopen stdin or stdout
                //   if(!strncmp(myargv[findredir(0)], "<", 1)){
                //     close(fd);
                //     open(0, O_WRONLY);
                //   }
                //   else{
                //     close(fd);
                //     open(1, O_RDONLY);
                //   }
                // }
                // break;
              }
            }
          }

        }
      }
    }
  }
}
