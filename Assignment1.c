#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

char *history[10][MAX_LINE];
int cmdHist = 1;
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
      char *args[MAX_LINE/+1]; /* command line (of 80) has max of 40 arguments */
      pid_t pid;

      while (1)
      { /* Program terminates normally inside setup */
        background = 0;
        printf(" COMMAND->\n");
        setup(inputBuffer,args,&background); /* get next command */

        pid = fork(); //(1) fork a child process using fork()

        if (pid == 0)
        {
          if (strcmp(args[0], 'r') != 0)
          {
            char x = args[1][0];

            for(int i = 9; i >= 0; i--) //Search for corresponding command
            {
              if(strcmp(history[i][1], 'x'))
              {
                execvp(history[i][0], history[i]);
              }
            }

          }else
          {
            int count = cmdHist;
            if (cmdHist >= 10)
            {
              for (int i = 9; i > 0; i)
              {
                *history[i-1] = *history[i];
              }
              count = 9;
            }

            for (int i = 0; i < sizeof(args); i++)
            {
              history[count][i] = args[i];
            }
            printf("hey .\n");
            execvp(args[0], args); //(2) the child process will invoke execvp()
          }


        } else if (pid > 0)/*(3) if background == 1, the parent will wait,
                              otherwise returns to the setup() function. */
        {
          if (background == 1)
          {
            waitpid(0, NULL, NULL);

          }
        } else
        {
          printf("Error: Fork failed.\n");
        }

      }







  }
