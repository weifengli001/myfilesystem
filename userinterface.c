/********************************************************
  File name: project1.c
  Description: UNIX Shell and Program Selection Feature
  Author: Weifeng Li 984558 weifli@my.bridgeport.edu
  Version: 1.0
  Date: 10/18/2015
  History:
**********************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
 

#define MAX_LINE 80
#define FAILURE 1

/***********************************************************
Function: run_shells
Description:
	using pipes running all the command in chain model,
    saving the final result in file.
Input: 
	char *** cmd, command list; char* filename, the file 
	name of result to save.
Output :
Return: void
************************************************************/
void    run_shells(char ***cmd, char* filename)
{
	int   p[2];
	pid_t pid;
	int   fd_in = 0;
	int q;

	while (*cmd != NULL)
	{
		pipe(p);
		pid = fork();
		if ( pid == -1)
		{
			exit(FAILURE);
		}
		else if (pid == 0)/*child process*/
		{
			dup2(fd_in, 0); //change the input according to the old one
			if (*(cmd + 1) != NULL)
				dup2(p[1], 1);/*change standard output*/
			else {
				q = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);/*create temp file that is used to save the final result*/
				dup2(q, 1);/*change the standard output to file just opened*/
			}
			close(p[0]);
			execvp((*cmd)[0], *cmd);
			close(q);
			exit(FAILURE);
		}
		else/*parent process*/
		{
			wait(NULL);
			close(p[1]);
			fd_in = p[0]; //save the input for the next command
			cmd++;
		}
	}
}

/***********************************************************
Function: getLine
Description:
	get one line from standard input.
Input:
	char* buff, user inputs are stored in it.
	size_t, max length to get.
Output : buff
Return: int
************************************************************/
int getLine(char* buff, size_t sz) {
	fgets(buff, sz, stdin);
	buff[strlen(buff) - 1] = 0;
	return 0;
}

int main(void)
{
	int should_run = 1;
	char *ls[] = { "ls", NULL };
	char *grep[] = { "grep", "wait", NULL };
	char *wc[] = { "wc","-l", NULL };
	char *find[] = { "find",".","-type", "f", "-executable", NULL };
	char *cut[] = { "cut", "-d" , "/", "-f", "2", NULL };
	char *sort[] = { "sort", "-n", NULL };
	char *nl[] = { "nl", NULL };
	char **cmd[] = { find, cut, sort, nl, NULL };/*command chain: find . -type f -executable | cut -d / -f 2 | sort -n | nl*/
	char filename[] = "d.txt";
	int c;


	while (should_run) {
		printf("ubos>");
		fflush(stdout);
		int i = 0;
		char buff[MAX_LINE];
		memset(buff, 0, sizeof(buff));
		char *args[MAX_LINE / 2 + 1] = { NULL };/*user input command and arguments*/
		pid_t pid, wpid;
		int status = 0;
		char* p;
		getLine(buff, MAX_LINE);
                if(buff == NULL || strcmp(buff, " ") == 0 || strcmp(buff, "\0") == 0 || strcmp(buff, "\n") == 0)continue;
		if (strcmp(buff, "exit") == 0) {/*if input exit, exit the program*/
			should_run = 0;
			exit(0);
		}
		if (strcmp(buff, "selection") == 0) {/*if input selection, create new process and perform program selection function*/
			pid = fork();
			if (pid < 0) {
				printf("Error\n");
				exit(FAILURE);;
			}
			else if (pid == 0) {/*child process*/
				run_shells(cmd, filename);/*find all executable files in alphabetical order, store the results in a file*/

				FILE *file;
				file = fopen(filename, "r");
				if (file != NULL) {
					char line[128];
					while (fgets(line, sizeof(line), file) != NULL) {
						printf("%s", line);/*read the result file and print it on the screen line by line*/
					}
					fclose(file);
				}
				else {
					perror(filename);
				}
				printf("please choose a number:");
				scanf("%d", &c);
				char tmpstr[15];/*store the number that user inputed*/
				sprintf(tmpstr, "%d", c);

				char str[80];/*used to store the command: for example cat ./b.txt | awk ' $1 == 3 {print $2}'*/
				strcpy(str, "cat ");
				strcat(str, filename);
				strcat(str, "| awk '$1 == ");
				strcat(str, tmpstr);
				strcat(str, "{print $2}'");
				int pfds[2];
				pipe(pfds);
				/*child process, execute shell commond get the name of executable file 
					according to user's choice from file, then write the result to pipe */
				if (!fork()) {
					close(1);
					dup(pfds[1]);
					close(pfds[1]);
					printf("%s", str);
					if (execl("/bin/sh", "/bin/sh", "-c", str, NULL) != 0) {
						printf("error!");
						exit(FAILURE);;
					}
				}
				else {/*parent process, get the executable file name from pipe and then execute the file*/
					close(0);
					dup(pfds[0]);
					close(pfds[1]);
					char buffer[1024];
					memset(buffer, 0, sizeof(buffer));
					if (read(pfds[0], buffer, sizeof(buffer)) != 0) {
						buffer[strlen(buffer) - 1] = '\0';/*remove the last new line character*/
						if (execl(buffer, NULL) != 0) {
							printf("error");
							exit(FAILURE);;
						}

					}
					else {
						printf("input error\n");
					}
					wpid = wait(&status);
					while (wpid < 0);

				}

			}
			else {/*selection parent process*/
				wpid = wait(&status);
				while (wpid < 0);
			}

		}
		else {/*execute the first task*/
			p = strtok(buff, " ");
			while (p != NULL) {
				args[i] = p;/*split user's input and store it in args*/
				i++;
				p = strtok(NULL, " ");

			}

			pid = fork();/*create new process*/
			if (pid < 0) {
				printf("Error");
			}
			else if (pid == 0) {/*child process*/
				if (execvp(args[0], args) < 0) {
					printf("commond not found\n");
					exit(FAILURE);;
				}
			}
			else {/*parent process*/
				wpid = wait(&status);
				while (wpid < 0);
			}

		}/*else end*/
	}
	return 0;
}

