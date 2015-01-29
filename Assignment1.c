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
} commandHist[10];

int cmdHist = -1;
/**
* setup() reads in the next command line, separating it into distinct tokens
* using whitespace as delimiters. setup() sets the args parameter as a
* null-terminated string.
*/
void setup(char inputBuffer[], char *args[],int *background)
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
  }
  int main(void)
  {
      char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
      int background; /* equals 1 if a command is followed by '&' */
      char *args[MAX_LINE/2]; /* command line (of 80) has max of 40 arguments */
      char *hold[MAX_LINE/2];
      pid_t pid;


      commandHist[0].args[0] = NULL;

      while (1)
      { /* Program terminates normally inside setup */
        background = 0;
        int found = 0;
        printf(" COMMAND->\n");
        setup(inputBuffer,args,&background); /* get next command */

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
                  *commandHist[j].args = strdup(*commandHist[j+1].args);
                  fprintf(stderr, "shifted an element\n");
                }
                cmdHist = 9;
              }else
                {
                  int l;
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
              // fprintf(stderr,"count is: %d\n", cmdHist);
              // fprintf(stderr,"executing... \n");
              //
              // pid = fork(); //(1) fork a child process using fork()
              // if (pid == 0)
              // {
              //   execvp(commandHist[cmdHist].args[0], commandHist[cmdHist].args); //(2) the child process will invoke execvp()
              //
              // } else if (pid > 0)/*(3) if background == 0, the parent will wait,
              //                             otherwise returns to the setup() function.*/
              // {
              //   if (background == 0)
              //   {
              //     waitpid(0, NULL, NULL);
              //     }
              //   } else
              //   {
              //     printf("Error: Fork failed.\n");
              //   }
              }else
              {
                int i;
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
                          *commandHist[j].args = strdup(*commandHist[j+1].args);
                          fprintf(stderr, "shifted an element\n");
                        }
                        cmdHist = 9;
                      }
                      fprintf(stderr,"copying ...\n");
                      int l;
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

                pid = fork(); //(1) fork a child process using fork()
                if (pid == 0)
                {
                  execvp(commandHist[cmdHist].args[0], commandHist[cmdHist].args); //(2) the child process will invoke execvp()

                }else if (pid > 0)/*(3) if background == 0, the parent will wait,
                                        otherwise returns to the setup() function.*/
                  {
                    if (background == 0)
                    {
                      waitpid(0, NULL, NULL);
                    }
                  } else
                    {
                      printf("Error: Fork failed.\n");
                    }

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

                *commandHist[i].args = strdup(*commandHist[i+1].args);
                fprintf(stderr, "shifted an element\n");
              }
              cmdHist = 9;
            }

            fprintf(stderr,"copying ...\n");
            int j;
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


            pid = fork(); //(1) fork a child process using fork()
            if (pid == 0)
            {
              execvp(args[0], args); //(2) the child process will invoke execvp()
            } else if (pid > 0)/*(3) if background == 0, the parent will wait,
                                      otherwise returns to the setup() function.*/
            {
              if (background == 0)
              {
                waitpid(0, NULL, NULL);

              }
            } else
            {
              printf("Error: Fork failed.\n");
            }
          }
        }
      }
