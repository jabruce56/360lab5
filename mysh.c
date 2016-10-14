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
struct stat d;
struct tm *tm;
struct dirent *dp;
char *myargv[10], npath[256], cwd[128], **menv;
char *path[9]= {"/", "/bin", "/sbin", "/usr/bin", "/usr/sbin", "/usr/games", "/usr/local/bin", "/usr/local/sbin", "/usr/local/games"};


doredir(int re, char *cmd){
  //handle I/O redirection

  printf("REDIRECTING\n");
  int i, fd, r;
  char *token, *arg[10];
  token = strtok(cmd, " ");
  for(i=0;token!=NULL&&i<10;i++){
    arg[i]=token;
    if(!strncmp(arg[i], "<", 1)||!strncmp(arg[i], ">", 1)||!strncmp(arg[i], ">>", 2)){
      r = i;
    }
    token = strtok(NULL, " ");
  }
  if(re==0){
    close(0);
    if(arg[r+1][0]=='/'){
      fd = open(arg[r+1], O_RDONLY);
      if (fd < 0){
        printf("open failed\n");
        exit(1);
      }
    }
    else{
      getcwd(cwd, 128);
      strcpy(npath, cwd);
      strcat(npath, "/");
      strcat(npath, arg[r+1]);
      fd = open(npath, O_RDONLY);
      if (fd < 0){
        printf("open failed\n");
        exit(1);
      }
    }
  }
  else if(re==1){
    close(1);
    if(arg[r+1][0]=='/'){
      fd = open(arg[r+1], O_WRONLY|O_CREAT, 0644);
      if (fd < 0){
        printf("open failed\n");
        exit(1);
      }
    }
    else{
      getcwd(cwd, 128);
      strcpy(npath, cwd);
      strcat(npath, "/");
      strcat(npath, arg[r+1]);
      fd = open(npath, O_WRONLY|O_CREAT, 0644);
      if (fd < 0){
        printf("open failed\n");
        exit(1);
      }
    }
  }
  else if(re==3){
    close(1);
    if(arg[r+1][0]=='/'){
      fd = open(arg[r+1], O_WRONLY | O_APPEND, 0644);
      if (fd < 0){
        printf("open failed\n");
        exit(1);
      }
    }
    else{
      getcwd(cwd, 128);
      strcpy(npath, cwd);
      strcat(npath, "/");
      strcat(npath, arg[r+1]);
      fd = open(npath, O_WRONLY | O_APPEND, 0644);
      if (fd < 0){
        printf("open failed\n");
        exit(1);
      }
    }
  }//end i/o redir
}
//}
int docmd(char *cmd){
  char *arg[10], *token, *head, *redir[3]={"<", ">", ">>"};
  int re=-1, i;
  printf("in docmd");
  token = strtok(cmd, " ");
  for(i=0;token!=NULL&&i<10;i++){
    arg[i]=token;
    if(!strncmp(arg[i], "<", 1)){
      re=1;
    }
    else if(!strncmp(arg[i], ">", 1)){
      re=2;
    }
    else if(!strncmp(arg[i], ">>", 2)){
      re = 3;
    }
    token = strtok(NULL, " ");
  }
  if(re!=-1){
    token = strtok(cmd, redir[re]);
    for(i=0;token!=NULL&&i<10;i++){
      if(i==0){
        head = token;
      }
      token = strtok(NULL, redir[re]);
    }
  }
  else{
    printf("DOCMD:%s\n", cmd);
    for(i=0;i<9;i++){
      dir=opendir(path[i]);
      printf("%s\n", arg[0]);
      while((dp=readdir(dir))!=NULL){
        if(!strncmp(dp->d_name, arg[0], strlen(arg[0]))){
          printf("found\n");
          strcpy(npath, path[i]);
          strcat(npath, "/");
          strcat(npath, arg[0]);
          if(access(npath, X_OK)!=-1){
            printf("HAVE FUCKING ACCESS\n");
            execve(npath, arg, menv);
          }
          else{
            printf("no access\n");
          }
        }
      }
    }
    printf("NO COMMAND DONE\n");
  }
}
int mypipe(char *cmd, int *pd){
  char *arg[10], *token, *head, *tail;
  int lpd[2], i, p, r, pid;
  printf("%s\n", cmd);
  printf("here1\n");
  if(pd){
    printf("pd\n");
    close(pd[0]);
    dup2(pd[1], 1);
    close(pd[1]);
  }
  printf("here2\n");
  for(i=0,p=0;i<strlen(cmd);i++){
    if(cmd[i]=='|'){
      p=1;
    }
  }
  printf("here3\n");
  if(p){
    printf("p\n");
    token = strtok(cmd, "|");
    for(i=0, p=0;token!=NULL&&i<10;i++){
      if(i==0){
        head = token;
      }
      else{
        tail = token;
      }
      token = strtok(NULL, "|");
    }
    r=pipe(lpd);
    pid=fork();
    if(pid){
      close(lpd[1]);
      dup2(lpd[0], 0);
      close(lpd[0]);
      docmd(tail);
    }
    else{
      mypipe(head, lpd);
    }
  }
  else{

    printf("%s\n", cmd);
    docmd(cmd);
  }
}

main(int argc, char *argv[], char *env[]){
  char line[256], *token, *hdir, *head[10], *tail[10];
  int ex=0, i, pid, status, j, r;
  menv=env;
  while(!ex){
    r=0;
    printf("jbsh :");
    bzero(line, 256);
    for(i=0;i<10;i++){myargv[i]=NULL;head[i]=NULL;tail[i]=NULL;}
    fgets(line, 256, stdin);
    line[strlen(line)-1] = 0;        // kill \n at end
    if (line[0]==0){                  // exit if NULL line
      continue;
    }
    token = strtok(line, " ");
    for(i=0;token!=NULL&&i<10;i++){
      myargv[i]=token;
      token = strtok(NULL, " ");
    }
    //myargv[i]=NULL;



    if(!strncmp(myargv[0], "cd", 2)){
      if(i<=1){//only cd
        hdir = getenv("HOME");
        chdir(hdir);//go to HOME
        getcwd(cwd, 128);
        printf("%s\n", cwd);
      }
      else{
        if(myargv[1][0]=='/'){//new path
          chdir(myargv[1]);
          getcwd(cwd, 128);
          printf("%s\n", cwd);
        }
        else{//go from cwd
          getcwd(cwd, 128);
          strcat(cwd, "/");
          strcat(cwd, myargv[1]);
          chdir(cwd);
          getcwd(cwd, 128);
          printf("%s\n", cwd);
        }
      }
    }
    if(!strncmp(myargv[0], "exit", 4)){
      exit(1);
    }
    pid = fork();
    if(pid==-1){//fork error
      fprintf(stderr, "fork error %d\n", errno);
      continue;
    }

    else if(pid){//parent process no pipe
      pid=wait(&status);
      continue;
    }

    else{//child process
      mypipe(line, 0);
    }















    // else{//fork
    //   if(p>0){//pipin time
    //
    //
    //     r = pipe(pd);//create pipe with p[0]=read, p[1]=write
    //     pid = fork();
    //     if(pid==-1){//fork error
    //       fprintf(stderr, "fork error %d\n", errno);
    //       continue;
    //     }
    //     else if(pid){//parent of pipe
    //       close(pd[0]);
    //       dup2(pd[1], 1);
    //       close(pd[1]);
    //       for(i=0;i<9;i++){
    //         dir=opendir(path[i]);
    //         while((dp=readdir(dir))!=NULL){
    //           if(!strncmp(dp->d_name, head[0], strlen(head[0]))){
    //             if (findredir(head)>-1){
    //               doredir(head);
    //             }
    //             strcpy(npath, path[i]);
    //             strcat(npath, "/");
    //             strcat(npath, head[0]);
    //             if(access(npath, X_OK)!=-1){
    //               execve(npath, head, env);
    //             }
    //           }
    //         }
    //       }
    //     }
    //     else{//child of pipe
    //       close(pd[1]);
    //       dup2(pd[0], 0);
    //       close(pd[0]);
    //       for(i=0;i<9;i++){
    //         dir=opendir(path[i]);
    //         while((dp=readdir(dir))!=NULL){
    //           if(!strncmp(dp->d_name, tail[0], strlen(tail[0]))){
    //             if (findredir(tail)>-1){
    //               doredir(tail);
    //             }
    //             strcpy(npath, path[i]);
    //             strcat(npath, "/");
    //             strcat(npath, tail[0]);
    //             if(access(npath, X_OK)!=-1){
    //               execve(npath, tail, env);
    //             }
    //           }
    //         }
    //       }
    //     }
    //   }
    //   else{//no pipe
    //
    //   }
    // }
  }
}
