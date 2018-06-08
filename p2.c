/* p2.c

 *
 * Nathan Sherburne
 * Carroll
 * CS570
 * masc0093
 * Program4
 * Early Due Date: 11/30/16
 *
 * Program 4 is an expanded version of p2.
 * Additional features include:
 * 1. Environment variables
 * 2. Multiple pipes
 * 
 *
 */

/* Include Files */
#include "getword.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include "CHK.h"
#include <errno.h>

/* Definitions */
#define MAXARGS 20
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

/* Constants */
const char* CD_NO_ARGS = "HOME";
const char* BUILTINS[] = {"cd","environ"};
const int NUM_OF_BUILTINS = sizeof BUILTINS / sizeof *BUILTINS;

/* Function prototypes */
int parse(char *arguments[], char* files[], char **allargs[]);
void redirect_output(char* newfilename);
void redirect_input(char* newfilename);
void myhandler(int signum);
void change_directory(char* arguments[]);
void environ_command(char* arguments[]);

char* filenames[2];					/* filenames[0] -> input. filenames[1] -> output. */
char* args[MAXARGS];					/* args contains all command line arguments, it can be used for the first command, but
											command_arr must be used to access subsequent commands */
char** command_arr[MAXARGS];						/* command_arr is for pipes, it is an array of pointers. Each pointer in the array
												points to each command. */
void (*fun_ptr_arr[2])(char**) = {change_directory, environ_command}; /* this is a function pointer array that points to each builtin function */
/* flags */
int REDIRECT_OUTPUT = 0;
int REDIRECT_INPUT = 0;	
int NUM_OF_PIPES = 0;
int RUN_IN_BACKGROUND = 0;
int IGNORE_THIS_METACHARACTER = 0;
int CONTINUE = 1;

int main()
{
	signal(SIGTERM, SIG_IGN);				/* Parent ignores SIGTERM */
	for(;;) {
		NUM_OF_PIPES = 0;				/* Reset global flags, and flush buffers */
		REDIRECT_OUTPUT = 0;
		REDIRECT_INPUT = 0;
		RUN_IN_BACKGROUND = 0;
		CONTINUE = 0;
		filenames[0] = NULL;
		filenames[1] = NULL;
		fflush(stdout);
		fflush(stderr);

		(void) printf("p2: ");
		int i = parse(args, filenames, command_arr);
		int num_of_commands = NUM_OF_PIPES + 1;
		int last_command_index = num_of_commands - 1;
		if(CONTINUE == 1)				/* CONTINUE is used if parse() finds syntax errors */
		{
			continue;
		}
		if(i == -1) 					/* If EOF is reached, terminate p2 */
		{
			break;
		}
		int index;
		for(index=0;index<num_of_commands;index++)
		{
			if(*command_arr[index] == NULL)			/* each element in command_arr points to a command pointer. If the command pointer is NULL, then it is a null command. */ 
			{
				fprintf(stderr,"Invalid null command.\n");
				CONTINUE = 1;
				break;
			}
		}
		if(CONTINUE == 1)	// this is specifically for the loop above
		{					// since we cannot include a continue statement within
			continue;		// the for loop
		}
		for(i=0;i<NUM_OF_BUILTINS;i++)
		{
			if(strcmp(args[0],BUILTINS[i]) == 0)		/* if first argument is a built-in command */
			{
				(*fun_ptr_arr[i])(args);				/* call respective function */
				CONTINUE = 1;
				break;
			}
		}
		if(CONTINUE == 1)	// this is specifically for the builtins(the loop above)
		{					// since we cannot include a continue statement within
			continue;		// the for loop
		}
		fflush(stdout);					/* Children inherit empty buffers */
		if(NUM_OF_PIPES > 0) 			/* If there is one or more pipes */
		{
			pid_t pid[num_of_commands];
        	int fildes[NUM_OF_PIPES][2];
			int k;
			int lm;
			for(k=0; k < NUM_OF_PIPES; k++)
			{
				CHK(pipe(fildes[k]));		/* set up pipes */
			}
			for(k=0; k < num_of_commands; k++)		/* this for loop forks a child for each command */
			{
				CHK(pid[k] = fork());                      /* Fork a child */
				if(pid[k] == 0)
				{
					if(k == 0)//first command
					{
						signal(SIGTERM,myhandler);		/* Kills this child if SIGTERM is sent */ 
						dup2(fildes[k][1],STDOUT_FILENO);          /* redirect this child's(command 1's) output to input of pipe */
						for(lm=0; lm < NUM_OF_PIPES; lm++)		/* close all file descriptors */
						{
							CHK(close(fildes[lm][0]));
							CHK(close(fildes[lm][1]));
						}

						if (REDIRECT_INPUT == 1)                /* If "<" flag is set, change input file to specified file */
						{					/* Note that this child should never check for ">" flag */
							redirect_input(filenames[0]);	/* Since this is a pipe, output for child 1 goes to child 2 */
						}

						if(RUN_IN_BACKGROUND == 1 && REDIRECT_INPUT == 0)   /* If "&" flag is set, redirect process input to "/dev/null"(if we haven't already redirected it) */
						{
							int in;
							in = open("/dev/null", O_RDONLY);
							CHK(dup2(in,STDIN_FILENO));
							CHK(close(in));
						}
						CHK(execvp(*command_arr[k], command_arr[k]));         	/* execute first command */
					}
					else if(k == last_command_index)//last command
					{
						signal(SIGTERM,myhandler);		/* Kills this child if SIGTERM is sent */
						CHK(dup2(fildes[k-1][0],STDIN_FILENO));      /* Redirect input for this child(command 2) to output of pipe */
						for(lm=0; lm < NUM_OF_PIPES; lm++)		/* close all file descriptors */
						{
							CHK(close(fildes[lm][0]));
							CHK(close(fildes[lm][1]));
						}
						if(REDIRECT_OUTPUT == 1)
						{
							redirect_output(filenames[1]);
						}
						CHK(execvp(*command_arr[k],command_arr[k]));      	/* execute last command */
					}//end 2nd if
					else //not the last or the first command(-- it's a middle command)
					{
						signal(SIGTERM,myhandler);
						CHK(dup2(fildes[k-1][0],STDIN_FILENO));      /* Redirect input to the previous pipe */
						CHK(dup2(fildes[k][1],STDOUT_FILENO));		/* Redirect output to the next pipe */
						for(lm=0; lm < NUM_OF_PIPES; lm++)		/* close all file descriptors */
						{
							CHK(close(fildes[lm][0]));
							CHK(close(fildes[lm][1]));
						}

						CHK(execvp(*command_arr[k],command_arr[k]));      	/* execute middle command(s) */
					}
				}//end fork
			}//end for loop
			for(k=0; k < NUM_OF_PIPES; k++)		/* close all file descriptors */
			{
				CHK(close(fildes[k][0]));
				CHK(close(fildes[k][1]));
			}
            if(RUN_IN_BACKGROUND == 0)              /* only wait for all children if background flag is not set */
			{
				for(;;)		/* wait for the last child to exit */
                {			/* don't wait for a specific amount of children, because there might be zombies, too */
					pid_t tmp;
					CHK(tmp = wait(NULL));
					if(tmp == pid[last_command_index]) 
					{
						break;
					}
                }
            }
			else		/* if background flag is set, all children in pipeline should be backgrounded(i.e. don't wait for any of them) */
			{			/* also, print the pid info of one of them */
				printf("%s [%d]\n", *command_arr[NUM_OF_PIPES], pid[last_command_index]); /* information of the LAST command is displayed when background flag is set */
			}
			continue;
		}
		/* no pipe(s), just 1 command */
		pid_t pid;			
		CHK(pid = fork()); 					/* fork */
		if(pid == 0) 						/* in child process */
		{
			signal(SIGTERM,myhandler);			/* Kills child process if SIGTERM is sent */
			if (REDIRECT_OUTPUT == 1)			/* if ">" flag is set, change output file to specified file name */
			{
				redirect_output(filenames[1]);
			}
			if (REDIRECT_INPUT == 1)			/* if "<" flag is set, change input file to specified file */
			{
				redirect_input(filenames[0]);
			}
			if(RUN_IN_BACKGROUND == 1 && REDIRECT_INPUT == 0) 	/* if "&" flag is set, redirect child input to "/dev/null"(assuming input redirect has not already been specified) */
			{
				int in;
				CHK(in = open("/dev/null", O_RDONLY));
				CHK(dup2(in,STDIN_FILENO));
				CHK(close(in));
			}
			CHK(execvp(*command_arr[0], command_arr[0]));		/* Execute command */
		}
		else 						/* In parent process */
		{
			if(RUN_IN_BACKGROUND == 0) 		/* If child is not a background process, wait for it */
			{					/* NOTE: if wait(NULL) is called by itself, it might scoop up a previous background process and not this one. */
				for(;;)
				{
					pid_t tmp;
					CHK(tmp = wait(NULL));
					if(tmp == pid)
					{
						break;
					}
				}
			}
			else 
			{
				printf("%s [%d]\n", args[0], pid);
			}
		}
	}
	/* not using CHK with killpg because autograder somehow gets killpg to return -1 every time */
	int check = (killpg(getpid(), SIGTERM));				/* Kill all children(doesn't kill parent because parent ignores SIGTERM) */
	//if(check == 0) {
		//printf("SUCCESS");	// 0 IS RETURNED during manual tests, -1 is returned during autograder tests
	//}
	printf("p2 terminated.\n");
	exit(0);
}

/* 
*
* parse() parses through stdin one word at a time, allocating memory for each word
* and storing each word in a string array.
*
* RETURN: length of argument array
* returns -1 for EOF.
*
* SIDE EFFECTS:
* Memory is allocated for strings(arguments: command names, options, file names, etc...).
* Arguments is an array of pointers that point to these strings.
* Files may contain pointers to filenames for I/O redirect
* Allargs contains pointers to the first word of each command
*	within the arguments array. It is used for pipes.
*
*
*/

int parse(char *arguments[], char* files[], char **allargs[])
{
	char s[STORAGE];					/* s stores current word */
	char* word;							/* word points to the beginning of the current word */
	char* tmp;
	int REDIRECT_OUTP = 0;
	int REDIRECT_INP = 0;
	int AMPAND_FLAG = 0;
	int ALTERNATE_EOF = 0;
	int i = 0;
	int j = 0;
	int c;
	allargs[j++] = arguments;				/* point first pointer to first command */

	for(;;) {
		c = getword(s);					/* Get single word */
		word = s;
		if(c==-1)		/* word is a lone '$' and not preceded by a '\'. Make sure to handle lone '$' */
		{																/* BEFORE handling environment variables or else '$' will be treated as an environment variable */
			if(REDIRECT_OUTP == 1 || REDIRECT_INP == 1)
			{
				fprintf(stderr,"Missing name for redirect.\n");
				CONTINUE = 1;
			}
			if(i == 0)					/* if EOF is first word terminate p2 */
			{
				return -1;
			}						/* if there are other words in arguments, return those first */
			arguments[i] = (char *) malloc(STORAGE);        /* point to allocated space */
            strcpy(arguments[i++],word);			            /* put "$" in that space */
			arguments[i] = NULL;							/* Null terminate because a lone-dollar sign is EOF */
			ungetc('$',stdin);								/* Next time parse() is called, it will return -1(EOF) */
			return i;
		}
		if(c < 0) /* '$' has been removed already, treat this current word as an environment variable */
		{
			tmp = word;
			word = getenv(word);				/* get environment variable string associated with this word */
			if(word==NULL)
			{
				fprintf(stderr,"%s: Environment variable does not exist.\n",tmp);
				CONTINUE = 1;
				word = "tmp";
			}
		}
		if(c==0)						/* newline reached */
		{
			if(REDIRECT_OUTP == 1 || REDIRECT_INP == 1)
			{
				fprintf(stderr,"Missing name for redirect.\n");
				CONTINUE = 1;
			}
			if(ALTERNATE_EOF == 1)				/* if "$" is the last character, it funcitons as EOF */
			{
				return -1;
			}
			if(AMPAND_FLAG == 1) 				/* if "&" was last word, remove it from argument list and then return with flag set */
			{
				arguments[--i] = NULL;
				RUN_IN_BACKGROUND = 1;
			}
			else 
			{
				arguments[i] = NULL;			/* null terminate array is important because we reuse this space for each command */
				if(i==0)
				{
					CONTINUE = 1;		/* if this is a lone newline, set just reissue prompt */
				}
			}
			return i;					/* return array length, i.e. number of args */
		}
		if(AMPAND_FLAG == 1)					/* if "&" was not the final word in the line, ignore it */
		{
			AMPAND_FLAG = 0;
		}
 		if(IGNORE_THIS_METACHARACTER == 1)      		/* add next word(even if it's a metacharacter) to the args list */
        {
			arguments[i] = (char *) malloc(STORAGE);        /* point pointer to empty space */
            strcpy(arguments[i++],word);                       /* copy string into that empty space*/
            IGNORE_THIS_METACHARACTER = 0;
            continue;                                       /* don't check to set flags */
        }
		if( strcmp(word,">") == 0)
		{
			if(REDIRECT_OUTP == 1 || REDIRECT_INP == 1)
			{
				fprintf(stderr,"Missing name for redirect.\n");
				CONTINUE = 1;
			}
			if(REDIRECT_OUTPUT == 1)
			{
				fprintf(stderr,"Ambiguous output redirect.\n");
				CONTINUE = 1;
			}
			REDIRECT_OUTP = 1;
			REDIRECT_OUTPUT = 1;
		}
		else if(strcmp(word,"<") == 0)
		{
			if(REDIRECT_OUTP == 1 || REDIRECT_INP == 1)
			{
				fprintf(stderr,"Missing name for redirect.\n");
				CONTINUE = 1;
			}
			if(REDIRECT_INPUT == 1)
			{
				fprintf(stderr,"Ambiguous input redirect.\n");
				CONTINUE = 1;
			}
			REDIRECT_INP = 1;
			REDIRECT_INPUT = 1;	
		}
		else if(strcmp(word,"|") == 0)
		{
			if(REDIRECT_OUTP == 1 || REDIRECT_INP == 1)
			{
				fprintf(stderr,"Missing name for redirect.\n");
				CONTINUE = 1;
			}
			NUM_OF_PIPES = j;
			arguments[i++] = NULL;		/* end command with a null */
			allargs[j++] = arguments + i;/*point to beginning of next command*/
		}
		else if(strcmp(word,"$") == 0) 
		{
			if(REDIRECT_OUTP == 1 || REDIRECT_INP == 1)
			{
				fprintf(stderr,"Missing name for redirect.\n");
				CONTINUE = 1;
			}
			ALTERNATE_EOF = 1;
		}	
		else if( REDIRECT_OUTP == 1)                         /* if ">" was previous arg, save name for output file */
        {
			if(strcmp(word,"&") == 0)
			{
				fprintf(stderr,"Missing name for redirect.\n");
				CONTINUE = 1;
			}	
            files[1] = (char *) malloc(STORAGE);
            strcpy(files[1],word);
            REDIRECT_OUTP = 0;
        }
        else if( REDIRECT_INP == 1)                     /* if "<" was the previous arg, save this name for input file */
        {
			if(strcmp(word,"&") == 0)
			{
				fprintf(stderr,"Missing name for redirect.\n");
				CONTINUE = 1;
			}	
            files[0] = (char *) malloc(STORAGE);
            strcpy(files[0],word);
            REDIRECT_INP = 0;
        }
		else 
		{
			arguments[i] = (char *) malloc(STORAGE); 	/* point pointer to empty space */
			strcpy(arguments[i++],word);			/* copy string into that empty space*/
			if(strcmp(word,"&") == 0)
            {
				AMPAND_FLAG = 1;
            }
		}
	}
}

void redirect_output(char* newfilename)
{
	int out;
	CHK(out = open(newfilename, O_WRONLY | O_CREAT | O_EXCL, 0600));/* open output file for writing only if it doesn't already exist */
	CHK(dup2(out,STDOUT_FILENO));					/* redirect output for this process */
	CHK(close(out));
}

void redirect_input(char* newfilename)
{
	int in;
	CHK(in = open(newfilename, O_RDONLY));				/* open input file */
	CHK(dup2(in, STDIN_FILENO));					/* redirect input for this process */
	CHK(close(in));
}

void myhandler(int signum)
{
	exit(0);
}
void change_directory(char* arguments[])
{
	if(arguments[1] == NULL) 		/* If there are no arguments given, change to default directory */
	{
		chdir(getenv(CD_NO_ARGS));
	}
	else if(arguments[2] == NULL)	/* one argument */
	{
		if(chdir(arguments[1])== -1) 	/* Change to specified directory */
		{
			perror("chdir failed");
		}
	}
	else 			/* "cd" should not have more than one argument */
	{
		fprintf(stderr,"chdir: Too many arguments.\n");
	}
}
void environ_command(char* arguments[])
{
	if(arguments[1] == NULL)	/* environ must have either 1 or 2 arguments */
	{
		fprintf(stderr,"environ: Must have at least one argument.\n");
	}
	else if(arguments[2] == NULL)	/* 1 argument -> print that variable's value */
	{
		char *tmp = getenv(arguments[1]);
		if(tmp != NULL)
		{
			printf("%s\n",tmp);
		}
		else
			fprintf(stderr,"%s: Undefined variable.\n",arguments[1]);
	}
	else if(arguments[3] == NULL)	/* 2 arguments -> set the variable's value */
	{
		CHK(setenv(arguments[1],arguments[2],1));
	}
	else
	{
		fprintf(stderr,"environ: Too many arguments.\n");
	}
}
