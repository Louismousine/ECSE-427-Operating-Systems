#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

struct history
{
  char* args[MAX_LINE/2];
  int background;
} commandHist[10];

int cmdHist = -1;

struct job
{
  char* name;
  pid_t pid;
} jobs[10]; // Lets say there wont be more jobs running than 10.

int jobsIndex = 0;

void exec(char* args[], int background);
/**
* setup() reads in the next command line, separating it into distinct tokens
* using whitespace as delimiters. setup() sets the args parameter as a
* null-terminated string.
*/
int setup(char inputBuffer[], char *args[],int *background)
{
  int length, /* # of characters in the command line */
  i, /* loop index for accessing inputBuffer array */
  start, /* index where beginning of next command parameter is */
  ct; /* index of where to place the next parameter into args[] */

  ct = 0;

  /* read what the user enters on the command line */
  length = read(STDIN_FILENO, inputBuffer, MAX_LINE);
  start = -1;
  if (length == 0)
    exit(0); /* ^d was entered, end of user command stream */
    if (length < 0)
    {
      perror("error reading the command");
      exit(-1); /* terminate with error code of -1 */
    }
    /* examine every character in the inputBuffer */
    for (i=0;i<length;i++)
    {
      switch (inputBuffer[i])
    {
      case ' ':
      case '\t' : /* argument separators */
      if(start != -1)
      {
        args[ct] = &inputBuffer[start]; /* set up pointer */
        ct++;
      }
      inputBuffer[i] = '\0'; /* add a null char; make a C string */
      start = -1;
      break;
      case '\n': /* should be the final char examined */

      if (ct==0 && start == -1) //if the user fails to enter  command before pressing enter
      {
        return 0;
      }
      if (start != -1)
      {
        args[ct] = &inputBuffer[start];
        ct++;
      }
      inputBuffer[i] = '\0';
      args[ct] = NULL; /* no more arguments to this command */
      break;
      default : /* some other character */
      if (start == -1)
        start = i;
        if (inputBuffer[i] == '&')
        {
          *background = 1;
          inputBuffer[i] = '\0';
        }
      }
    }
    args[ct] = NULL; /* just in case the input line was > 80 */
    return ct;
  }
  int main(void)
  {
      char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
      int background; /* equals 1 if a command is followed by '&' */
      char *args[MAX_LINE/2]; /* command line (of 80) has max of 40 arguments */
      char *hold[MAX_LINE/2];
      pid_t pid;
      //Initial set up for history and jobs
      commandHist[0].args[0] = NULL;
      int i;
      for (i=0; i<10;i++)
      {
        jobs[i].name=NULL;
      }

      while (1)
      { /* Program terminates normally inside setup */
        background = 0;
        int found = 0;
        printf(" COMMAND->\n");
        while(setup(inputBuffer, args, &background) == 0) /* get next command */
        {
          printf(" COMMAND->\n");
        }
        for (i = 0; i < 10; i++)
        {
          if (jobs[i].name != NULL)
          {
            pid_t checkPid = waitpid(jobs[i].pid, NULL, WNOHANG);
            if (checkPid == 0)
            {
                //do nothing as child is still running
            } else
            {
              jobs[i].name = NULL; //free up space in jobs list
              jobsIndex--;
            }
          }
        }
        if (strcmp(args[0], "r") == 0)
        {
          if (commandHist[0].args[0] == NULL)
          {
            fprintf(stderr, "No elements currently in history...\n");
          }else
          {
            fprintf(stderr,"entered history...\n");
            if (args[1] == NULL)
            {
              found = -1;
              if (cmdHist < 10)
              {
                cmdHist++;
              }
              if (cmdHist > 9)
              {
                fprintf(stderr,"Shifting previous commands... \n");

                int j;
                for (j = 0; j < 9; j++)
                {
                  commandHist[j].background = commandHist[j+1].background;
                  memmove(commandHist[j].args, commandHist[j+1].args, sizeof(commandHist[j+1].args));
                  fprintf(stderr, "shifted an element\n");
                }
                cmdHist = 9;
              }else
                {
                  int l;
                  commandHist[cmdHist].background = commandHist[cmdHist-1].background;
                  for (l = 0; l < sizeof(commandHist[cmdHist-1])/2; l++)
                  {
                    if(commandHist[cmdHist-1].args[l] == NULL)
                    {
                      commandHist[cmdHist].args[l] = NULL;
                      break;
                    }else
                    {
                      fprintf(stderr, "qwe");
                      commandHist[cmdHist].args[l] = strdup(commandHist[cmdHist-1].args[l]);
                    }
                  }
                }
                int k;
                fprintf(stderr,"copying complete printing current history by first letter ...\n");
                for(k = cmdHist; k > -1; k--)
                {
                  fprintf(stderr, "%c\n", commandHist[k].args[0][0]);
                }
              }else
              {
                if (cmdHist > 9)
                  cmdHist = 9;
                  for( i = cmdHist; i > -1; i--) //Search for corresponding command
                  {
                    fprintf(stderr,"searching...\n");
                    //fprintf(stderr,"%c\n",args[1][0]);
                    fprintf(stderr, "%c\n",commandHist[i].args[0][0]);
                    if(commandHist[i].args[0][0] == args[1][0])
                    {
                      found = 1;
                      int p;
                      if (commandHist[i].background == 1)
                        background = 1;
                      for (p = 0; p < sizeof(commandHist[i].args)/4; p++)
                      {
                        if(commandHist[i].args[p] == NULL)
                        {
                          hold[p] = NULL;
                          break;
                        }else
                        {
                          fprintf(stderr, "qwe");
                          hold[p] = strdup(commandHist[i].args[p]);
                        }
                      }
                      fprintf(stderr, "entry found entering command....\n");
                      if (cmdHist < 10)
                      {
                        cmdHist++;
                      }
                      if (cmdHist > 9)
                      {
                        fprintf(stderr,"Shifting previous commands... \n");

                        int j;
                        for (j = 0; j < 9; j++)
                        {
                          commandHist[j].background = commandHist[j+1].background;
                          memmove(commandHist[j].args, commandHist[j+1].args, sizeof(commandHist[j+1].args));
                          fprintf(stderr, "shifted an element\n");
                        }
                        cmdHist = 9;
                      }
                      fprintf(stderr,"copying ...\n");
                      int l;
                      if (background == 1)
                        commandHist[cmdHist].background = 1;
                      for (l = 0; l < sizeof(hold)/2; l++)
                      {
                        if(hold[l] == NULL)
                        {
                          commandHist[cmdHist].args[l] = NULL;
                          break;
                        }else
                        {
                          fprintf(stderr, "qwe");
                          commandHist[cmdHist].args[l] = strdup(hold[l]);
                        }
                      }
                      int k;
                      fprintf(stderr,"copying complete printing current history by first letter ...\n");
                      for(k = cmdHist; k > -1; k--)
                      {
                        fprintf(stderr, "%c\n", commandHist[k].args[0][0]);
                      }
                      fprintf(stderr,"count is: %d", cmdHist);
                      break;

                    }

                  }
                }
              }
              if(found == 0 || commandHist[0].args[0] == NULL)
              {
                fprintf(stderr,"no match found.\n");

              } else
              {
                fprintf(stderr,"executing... \n");
                if(commandHist[cmdHist].background == 1)
                {
                  background = 1;
                }
                exec(commandHist[cmdHist].args,background);
              }

          }else
          {
            if (cmdHist < 10)
            {
              cmdHist++;
            }
            fprintf(stderr,"entering command... \n");
            if (cmdHist > 9)
            {
              fprintf(stderr,"Shifting previous commands... \n");
              int i;
              for (i = 0; i < 9; i++)
              {
                commandHist[i].background = commandHist[i+1].background;
                memmove(commandHist[i].args,commandHist[i+1].args, sizeof(commandHist[i+1].args));
                fprintf(stderr, "shifted an element\n");
              }
              cmdHist = 9;
            }

            fprintf(stderr,"copying ...\n");
            int j;
            if (background = 1)
              commandHist[cmdHist].background = 1;
            for (j = 0; j < sizeof(args)/4; j++)
            {
              if(args[j] == NULL)
              {
                commandHist[cmdHist].args[j] = NULL;
                break;
              }else
              {
                commandHist[cmdHist].args[j] = strdup(args[j]);
              }
            }


            int i;
            fprintf(stderr,"copying complete printing current history by first letter ...\n");

            for(i = cmdHist; i > -1; i--)
            {
              fprintf(stderr, "%c\n", commandHist[i].args[0][0]);
            }
            fprintf(stderr,"count is: %d", cmdHist);
            fprintf(stderr,"executing... \n");

            exec(args,background);

          }
        }
      }

      void exec(char* args[],int background)
      {
        int i;
        pid_t pid;
        if (strcmp(args[0],"cd") == 0) //If command entered is 'cd' then change directory
        {
          int ret;

          ret = chdir(args[1]);       //If directory entered is valid change directory otherwise print error
          if (ret != 0)
          {
            fprintf(stderr, "Error, Not a valid directory.\n");

          }

        }
        else if (strcmp(args[0], "history") == 0)  //If command entered is 'history' then print history
        {
          fprintf(stderr, "printing command history...\n");
          for(int k = cmdHist; k > -1; k--)         //Prints history in most recent order of execution, including the history command itself
          {
            for (int j = 0; j < sizeof(commandHist[k].args); j++)
            {
              if(commandHist[k].args[j] == NULL)
              {
                break;
              }else
                fprintf(stderr, "%s ", commandHist[k].args[j]);
            }
            fprintf(stderr, "\n");
          }
        }
        else if(strcmp(args[0], "pwd") == 0) //If command entered is 'pwd' print current directory
        {
          fprintf(stderr, "%s\n", getcwd(NULL,NULL));
        }
        else if(strcmp(args[0], "exit") == 0) //If comand entered is 'exit' exit the shell
        {
          exit(0);
        }
        else if(strcmp(args[0], "jobs") == 0)
        {
          //fprintf(stderr, "test\n");
          int isJob = 0;
          for (i= 0; i<= jobsIndex +1 ;i++)
          {
            if(jobs[i].name != NULL)
            {
              fprintf(stderr," [%d] %s\n", i, jobs[i].name);
              isJob = 1;
            }
          }
          if (isJob != 1)
            printf("No active jobs\n");
        }
        else if(strcmp(args[0], "fg")==0)
        {
          int isJob = 0;
          int y = (int) strtol(args[1], NULL, 10);
          fprintf(stderr, "%d\n", y);
          for (i= 0 ; i <= jobsIndex + 1; i++)
          {
            if (jobs[i].name != NULL && i == y)
            {
                isJob = 1;
                printf("Moving [%d] %s to foreground\n", y, jobs[i].name);
                waitpid(jobs[i].pid, NULL, 0);
                free(jobs[i].name);
                jobs[i].name = NULL;
                jobsIndex--;
            }
          }
          if (!isJob)
          {
            fprintf(stderr, "No job matching ID found.\n");
          }
        }else
        {
          pid = fork(); //(1) fork a child process using fork()
          if (pid == 0)
          {
            execvp(args[0], args); //(2) the child process will invoke execvp()
          } else if (pid > 0)/*(3) if background == 0, the parent will wait, otherwise returns to the setup() function.*/
          {
            if (background == 0)
            {
              waitpid(0, NULL, 0);
            }else
            {
              for (i= 0; i < 1 + jobsIndex; i++)
              {
                if (jobs[i].name == NULL)
                {
                  jobs[i].pid = pid;
                  jobs[i].name = strdup(args[0]);
                  jobsIndex++;
                  break;
                }
              }
            }
          } else
            printf("Error: Fork failed.\n");
        }
      }
