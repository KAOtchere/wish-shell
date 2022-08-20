/**
* Kwabena Aboagye-Otchere
* Ashesi University
* Operating Systems Individual Project
* Wish shell
*/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>


char *builtinStr[] = { "cd","exit", "path"};
char **paths;
int redirect =-1; //-1 if false, 0 for true

char* filename;

//function definitions
int cdFunc(char **args);
int exitFunc();
int pathFunc(char **args);
void availablePaths();

int (*builtinFuncs[]) (char **) = {
  &cdFunc,
  &exitFunc,
  &pathFunc
};

int builtinCount = 3;
int pathCount; //number of paths entered by user
int argCount; //number of arguments per line
int cmdCount; //count of commands to be executed in parallel

//built in cd command definition
int cdFunc(char **args){
    if(argCount < 1){
        fprintf(stderr, "An error has occurred\n"); //expecting a directory to be entered with cd command
    }
    else{
        char *temp = args[1];


        if(chdir(temp) != 0){
            fprintf(stderr, "An error has occurred\n"); //unable to change directory
        }


    }
    return 1;
}

//built in exit command definition
int exitFunc(void){return 0;}

//built in path command definition
int pathFunc(char **args){
    if(paths != NULL){free(paths);}
    paths = malloc((argCount+1) * sizeof(char*));
    if(paths == NULL){printf("An error has occurred\n");}
    char* buffer;
    pathCount = 0;

    for(int i = 1; i < argCount; i++){

        if(access(args[i], X_OK) == 0){

            int pathlength = strlen(args[i]) + 1;
            buffer = (char*)malloc(pathlength * sizeof(char));
            sprintf(buffer, "%s/", args[i]);


            paths[i-1] = buffer;

            pathCount++;

        }else{
            fprintf(stderr, "An error has occurred\n"); //unable to add new path/paths
        }

    }





    return 1;
}

//running functions that are not builtins
int exeCmd(char **args){
  pid_t pid;
  int status;

  pid = fork();

  if(pid == 0){

    // Child process
    int i;



    //check state of redirect. If 0, write to file
    char *cmd = args[0];
    char *buffer;

    for(i = 0; i < pathCount; i++){
        int pathlength = strlen(paths[i]) + strlen(cmd) + 1;
        buffer = (char*)malloc(pathlength * sizeof(char));
        sprintf(buffer, "%s%s", paths[i], cmd);


        args[0] = buffer;

        if(execv(args[0], args) == 0){
            break;
        }
        i++;
    }

    if(redirect==0){
        int tmpFd = open(filename, O_WRONLY);
        dup2(tmpFd, 1);
        execv(args[0], args);
        close(tmpFd);
    }

    if(i==pathCount){fprintf(stderr, "An error has occurred\n");}
    exit(0);
  }
   else if(pid < 0){
    //fork unsuccessful
    fprintf(stderr, "An error has occurred\n");
  }
  else{
    // Parent process
    wait(NULL);
    redirect = -1;
  }

  return 1;
}

//wrapper function to execute commands entered to wish shell
int execute(char **args){

  int i;

  if (args[0] == NULL){
    // An empty command was entered.
    return 1;
  }

  for(i = 0; i < builtinCount; i++){

    if (strcmp(args[0], builtinStr[i]) == 0){
      return (*builtinFuncs[i])(args);
    }
  }


  return exeCmd(args);
}


char *interactiveRead(void){ //reading lines from "shell"

  char *line = NULL;
  ssize_t bufsize = 0;

  if(getline(&line, &bufsize, stdin) == -1){
    if(feof(stdin)){
      exit(EXIT_SUCCESS);
    }
    else{
      fprintf(stderr, "An error has occurred\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
}


char **splitCmd(char *line){ //splitting command string into arguments
  int argc = 20;
  int i = 0;
  char *token;
  argCount = 0;

  char **tokens = malloc(argc * sizeof(char*));

  if(!tokens){
    fprintf(stderr, "An error has occurred\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, " ");
  while(token != NULL){
    tokens[i] = token;
    i++;
    argCount++;

    if(i >= argc){
      argc += 10;
      char **temp;
      temp = tokens;

      tokens = realloc(tokens, argc * sizeof(char*));

      if(!tokens){
		free(temp);
        fprintf(stderr, "An error has occurred\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, " ");
  }
  tokens[i] = NULL;
  return tokens;
}

char **splitCmds(char *line){ 
    //splitting a group of commands to be run concurrently. Seperated by "&"
    int argc = 20;
    int i = 0;
    char *token;
    cmdCount = 0;

    char **tokens = malloc(argc * sizeof(char*));

    if(!tokens){
        fprintf(stderr, "An error has occurred\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, "&");
    while(token != NULL){
        tokens[i] = token;
        i++;
        cmdCount++;

    if(i >= argc){
        argc += 10;
        char **temp;
        temp = tokens;

        tokens = realloc(tokens, argc * sizeof(char*));

        if(!tokens){
            free(temp);
            fprintf(stderr, "An error has occurred\n");
            exit(EXIT_FAILURE);
        }
    }

        token = strtok(NULL, "&");
    }
    tokens[i] = NULL;
    return tokens;
}

char *checkRedirect(char *line){ 
    //checks for ">", if found, assigns the value after to file path and truncates from string
    int argc = 5;
    int i = 0;
    redirect = -1; //set redirect back to default (false)
    char **tokens = malloc(argc * sizeof(char*));
    char *token;

    if(!tokens){
        fprintf(stderr, "An error has occurred\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, ">");
    while(token != NULL){
        tokens[i] = token;
        i++;
        token = strtok(NULL, ">");
    }
    tokens[i] = NULL;

    if(i>1){
        char *temp = tokens[1];


        filename = strtok(temp, " ");

        redirect = 0;
    }

    return tokens[0];




}

//clears unnecessary whitespace and tabs to make shell robust
char* sanitize(char* line){

    int i, j;
    char * newString;

    newString = (char *)malloc(100);

    i = 0;
    j = 0;

    while(line[i] != '\0')
    {
        /* If blank space is found */
        if(line[i] == ' ')
        {
            newString[j] = ' ';
            j++;

            /* Skip all consecutive spaces */
            while(line[i] == ' ' || line[i] == '\t')
                i++;
        }

        newString[j] = line[i];

        i++;
        j++;
    }
    // NULL terminate the new string
    newString[j] = '\0';

    return newString;

}


//thread function for parallel executions
void *paraExecute(void *vargp){
    char *line = (char*)vargp;
    char *cleanline = sanitize(line);
    char *checkedline = checkRedirect(cleanline); //removing text after a > if it exists
    char **args = splitCmd(checkedline);
    execute(args);
}

void availablePaths(void){
    printf("Current path count: %d", pathCount);
    for(int i =0; i < pathCount; i++){
        printf("%s\n", paths[i]);
    }
}

int main(int argc, char **argv){
    int status = 1;

    if(argc > 1){
        printf("You are in batch mode\n");
        int bufferLength = 100;
        char buffer[bufferLength];

        FILE* fp = fopen(argv[1], "r");


        if (!fp){
            fprintf(stderr, "An error has occurred\n");
            exit(-1);
        }

        char *line;

        while(fgets(buffer, bufferLength, fp)){
            if(status != 1){break;}

            buffer[strcspn(buffer, "\r\n")] = 0;
            line = buffer;


            char **cmds = splitCmds(line);
            if(cmdCount>1){
                pthread_t tid[cmdCount];
                for(int i=0; i < cmdCount; i++){
                    pthread_create(&(tid[i]), NULL, paraExecute, cmds[i]);
                    pthread_join(tid[i], NULL);
                }
            }else{
                char *cleanline = sanitize(line);
                char *checkedline = checkRedirect(cleanline);
                char **args = splitCmd(checkedline);
                status = execute(args);
                free(args);

            }


            free(cmds);

        }

    }else{
        printf("You are in interactive mode\n");

        do{
            printf("wish> ");
            char *line = interactiveRead();
            line[strcspn(line,"\n")] = '\0';

            char **cmds = splitCmds(line);
            if(cmdCount>1){
                pthread_t tid[cmdCount];
                for(int i=0; i < cmdCount; i++){
                    pthread_create(&(tid[i]), NULL, paraExecute, cmds[i]);
                    pthread_join(tid[i], NULL);
                }
            }else{
                char *cleanline = sanitize(line);
                char *checkedline = checkRedirect(cleanline);
                char **args = splitCmd(checkedline);
                status = execute(args);
                free(args);
            }


            free(cmds);
            free(line);

        }while(status);
    }


}
