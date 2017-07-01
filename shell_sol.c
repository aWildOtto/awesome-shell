#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
#define WD_LENGTH 1024
#define dlim " \t\r\n\a"
#define HISTORY_MAX 10

char history[HISTORY_MAX][COMMAND_LENGTH];
int historyIndex = 0;
int command_launch_history(char* buff, char* tokens[],  _Bool in_background,char* wd);
int command_history();
/**
* Read a command from the keyboard into the buffer 'buff' and tokenize it
* such that 'tokens[i]' points into 'buff' to the i'th token in the command.
* buff: Buffer allocated by the calling code. Must be at least
* COMMAND_LENGTH bytes long.
* tokens[]: Array of character pointers which point into 'buff'. Must be at
* least NUM_TOKENS long. Will strip out up to one final '&' token.
* 'tokens' will be NULL terminated.
* in_background: pointer to a boolean variable. Set to true if user entered
* an & as their last token; otherwise set to false.
*/
void handle_SIGINT(){
  write(STDOUT_FILENO,"\n",strlen("\n"));
  command_history();
  return;
}

char* getHistory(int ind){
  int cirCInd = ind%10;
  return history[cirCInd];
}
void recordHistory(char* cmd){
  int cirCInd = historyIndex%10;
  strcpy(history[cirCInd],cmd);
  historyIndex++;
}
void tokenize(char* buff, char* tokens[]){
  int i = 0;

  char *newToken;
  newToken = strtok(buff, dlim);
  //printf("%s\n",newToken );
  while (newToken!=NULL) {
      tokens[i] = newToken;
      //printf("token %d is %s\n",i, tokens[i] );
      i++;
      newToken = strtok(NULL,dlim);
  }

  tokens[i] = NULL;
}

void removeWhiteSpace(char* buff){
  char *temp = buff;
  while (isspace(*buff)) {
    buff++;
  }
  int i = 0;
  while (*buff != 0) {
    temp[i] = *buff;
    buff++;
    i++;
  }
  temp[i] = 0;
}

int read_command(char *buff, char *tokens[], _Bool *in_background)
{
  int c = 0, i = 0;
  while (1) {
    c = getchar();
    if (c == '\n'||c==EOF) {
      buff[i] = '\0';
      break;
    }
    buff[i] = c;
    i++;
  }
  if (strlen(buff)==0) {
    return -1;
  }

  removeWhiteSpace(buff);
  // printf("buff is %s\n",buff );
  // printf("buff[1] is %c\n",buff[1] );
  if (buff[0]=='!') {//check if there is '!' command even when there are white spaces
    return 2;
  }
  recordHistory(buff);
  if (buff[i-1] == '&') {
    *in_background = true;
    buff[i-1] = '\0';
  }
  // now buff contains the whole command
  //printf("whole command is %s\n", buff);
  tokenize(buff,tokens);
  return 0;
}

//-------------------------------------------
//commands
int launch_command(char* tokens[], _Bool in_background){
  pid_t pid;
  //
  //printf("11\n" );
  pid = fork();
  if(pid ==0){
    if (execvp(tokens[0],tokens) == -1) {
      perror("execvp error: ");
      exit(-1);
    }
    exit(0);
  }
  else if (pid <0) {
    write(STDOUT_FILENO,"fork error\n",strlen("fork error\n"));
    return 1;
  }
  else {
    if (in_background==false) {
      int state;
      waitpid(pid,&state,0);
    }
    return 0;
  }

}

int command_cd(char* tokens[]){
  if (tokens[1]==NULL) {
    write(STDOUT_FILENO,"cd failed: expected argument to \"cd\"\n",\
     strlen("cd failed: expected argument to \"cd\"\n"));
     return 1;
  }
  else{
    if (chdir(tokens[1])!=0) {
      perror("cd failed: ");
      return 2;
    }
    else
      return 0;
  }
  return 3;
}

void command_exit(){
  exit(0);
}

int command_pwd(char* wd){
  wd = getcwd(wd,WD_LENGTH);
  if (wd!=NULL) {
    write(STDOUT_FILENO,wd,strlen(wd));
    write(STDOUT_FILENO,"\n",strlen("\n"));
    return 0;
  }
  else{
    write(STDOUT_FILENO,"pwd failed: buffer error\n",strlen("pwd failed: buffer error\n"));
    return 1;
  }
  return 2;
}

//change number into string for write() to print
void reverse(char str[], int length)
{
    int start = 0;
    int end = length -1;
    while (start < end)
    {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void intToStr(int num, char* str){
  if (num<10) {
    str[0] = num+48;
    str[1] = '\0';
    return;
  }
  else{
    int i = 0;
    while (num != 0)
      {
          int rem = num % 10;
          str[i] = rem + '0';
          num = num/10;
          i++;
      }
      str[i] = '\0';
      reverse(str, i);
    }
}
//-----------------------------------------
int command_history(){
  int frontIndex;
  if (historyIndex>=10) {
     frontIndex = historyIndex - 10;
  }
  else{
    frontIndex = 0;
  }
  for (int i = frontIndex; i < historyIndex; i++) {
    char iStr[HISTORY_MAX];
    char* hs = getHistory(i);
    intToStr(i+1,iStr);
    write(STDOUT_FILENO,iStr,strlen(iStr));
    write(STDOUT_FILENO,"\t",strlen("\t"));
    write(STDOUT_FILENO,hs,strlen(hs));
    write(STDOUT_FILENO,"\n",strlen("\n"));
  }
  return 0;
}


//----------------------------------------------
int command_excute(char* tokens[], _Bool in_background,char* wd){
  if (tokens == NULL) {
    return 0;
  }
  if (strcmp(tokens[0],"cd")==0) {
    return command_cd(tokens);
  }
  if (strcmp(tokens[0],"pwd")==0) {
    return command_pwd(wd);
  }
  if (strcmp(tokens[0],"exit")==0) {
    command_exit();
  }
  if (strcmp(tokens[0],"history")==0) {
    return command_history();
  }
  // if (strcmp(tokens[0],"holy")==0) {
  //   write(STDOUT_FILENO,"Jesus\n",strlen("Jesus\n"));
  //   return 0;
  // }
  return launch_command(tokens,in_background);
}

/**in_backgroundin_background
* Main and Execute Commands
*/
void prompWD(char* wd){
  wd = getcwd(wd,WD_LENGTH);
  if (wd!=NULL) {
    return;
  }
  else{
    write(STDOUT_FILENO,"shell promt fail, please restart\n",\
    strlen("shell promt fail, please restart\n"));
    exit(1);
  }
}


int command_launch_history(char* buff, char* tokens[],  _Bool in_background,char* wd){
  if (historyIndex<1) {
    write(STDOUT_FILENO,"! launch failed, no history\n",strlen("! launch failed, no history\n"));
    return -1;
  }

  if(buff[1]=='!'){
    int len = strlen(getHistory(historyIndex-1));
    char cmdToExec[len];
    strcpy(cmdToExec, getHistory(historyIndex-1));
    write(STDOUT_FILENO,"relaunching ",strlen("relaunching "));
    write(STDOUT_FILENO,cmdToExec,strlen(cmdToExec));
    write(STDOUT_FILENO,"\n",strlen("\n"));
    if (cmdToExec[len-1]=='&') {
      cmdToExec[len-1] = '\0';
      in_background = true;
    }
    tokenize(cmdToExec,tokens);

    if (command_excute(tokens,in_background,wd) != 0) {
      write(STDOUT_FILENO,"!! launch failed\n",strlen("!! launch failed\n"));
      return -1;
    }
    else
      return 0;
  }
  else if (buff[1]<49||buff[1]>57) {
    write(STDOUT_FILENO,"! command can only be followed by number > 1 or !\n",strlen("! command can only be followed by number > 1 or !\n"));
    return -2;
  }
  else {
    char c = buff[1];
    char s[strlen(buff)-1];
    int i = 0;
    int j = 1;
    while(c!='\0'){
      c = buff[j];
      s[i] = c;
      i++;
      j++;
    }
    int ind = atoi(s)-1;
    if (ind>=historyIndex || ind<=(historyIndex-10)) {
      write(STDOUT_FILENO,"! failed: command couldn't find\n",strlen("! failed: command couldn't find\n"));
      return -3;
    }
    write(STDOUT_FILENO,"relaunching ",strlen("relaunching "));
    write(STDOUT_FILENO,history[ind],strlen(history[ind]));
    write(STDOUT_FILENO,"\n",strlen("\n"));
    int cmdlen = strlen(getHistory(ind));
    char cmd[cmdlen];
    strcpy(cmd,getHistory(ind));
    if (cmd[cmdlen-1]=='&') {
      cmd[cmdlen-1] = '\0';
      in_background = true;
    }
    tokenize(cmd,tokens);
    if (command_excute(tokens, in_background, wd)!= 0) {
      write(STDOUT_FILENO,"!N launch failed\n",strlen("!N launch failed\n"));
      return -1;
    }
    else
      return 0;
  }
}
int main(int argc, char* argv[])
{
char input_buffer[COMMAND_LENGTH];
char *tokens[NUM_TOKENS];
char wd[WD_LENGTH];

struct sigaction handler;
handler.sa_handler = handle_SIGINT;
handler.sa_flags = 0;
sigemptyset(&handler.sa_mask);
sigaction(SIGINT, &handler, NULL);

while (true) {
// Get command
// Use write because we need to use read()/write() to work with
// signals, and they are incompatible with printf().

  prompWD(wd);
  write(STDOUT_FILENO, "Awesome shell:", strlen("Awesome shell:"));
  write(STDOUT_FILENO,wd,strlen(wd));
  write(STDOUT_FILENO,"> ",strlen("> "));
  _Bool in_background = false;

  int res = read_command(input_buffer, tokens, &in_background);

  if (res!=2&&res!= -1) {
    command_excute(tokens,in_background,wd);
      //write(STDOUT_FILENO,"Execution failed\n",strlen("Execution failed\n"));
  }

  else if (res ==2) {
    command_launch_history(input_buffer, tokens,in_background,wd);
  }


/**
* Steps For Basic Shell:
* 1. Fork a child process
* 2. Child process invokes execvp() using results in token array.
* 3. If in_background is false, parent waits for
* child to finish. Otherwise, parent loops back to
* read_command() again immediately.
*/

  while (waitpid(WAIT_ANY, NULL, WNOHANG) > 0)
  ; // do nothing.
  }

}
