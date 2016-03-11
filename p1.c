#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

extern int errno;

typedef void (*sighandler_t)(int);
static char *my_envp[100];
char cwd[1024];


struct node {
	int val;
	char arguments[256];
	char cmd[256];
	struct node *next;
	  
};


struct node *head = NULL;
struct node *current = NULL; 

// Adds new node to linked list with specified pid command info **/
struct node* add_to_list(int val, char *cmd, char * args){


    struct node *ptr = (struct node*)malloc(sizeof(struct node));
    if(NULL == ptr){
        printf("\n Node creation failed \n");
        return NULL;
    }
    ptr->val = val;
    strcpy(ptr->arguments, args);
    strcpy(ptr->cmd, cmd);
    ptr->next = NULL;
    current->next = ptr;
    current = ptr;

   
    return ptr;
}





/** moves through linked list to find node with specified value **/
struct node* find_node(int val, struct node **prev){
    struct node *ptr = head;
    struct node *tmp = NULL;
    int found = 0;

    while(ptr != NULL){
        if(ptr->val == val){
            found = 1;
            break;
        }
        else{
            tmp = ptr;
            ptr = ptr->next;
        }
    }

    if(1 == found){
        if(prev)
            *prev = tmp;
        return ptr;
    }
    else{
        return NULL;
    }
}


/** Removes node with specified PID from linked list of background processes **/
int delete_from_list(int val){

    struct node *prev = NULL;
    struct node *del = NULL;


    del = find_node(val,&prev);
    if(del == NULL){
        return -1;
    } else{
        if(prev != NULL)
            prev->next = del->next;

        if(del == current){
            current = prev;
        } else if(del == head){
            head = del->next;
        }
    }
    
    printf("\nValue %d removed from list.\n",val);

    free(del);
    del = NULL;

    return 0;
}


/** copies environment variables into envp **/
void copy_envp(char **envp)
{
	int index = 0;
	for(;envp[index] != NULL; index++) {
		my_envp[index] = (char *)malloc(sizeof(char) * (strlen(envp[index]) + 1));
		memcpy(my_envp[index], envp[index], strlen(envp[index]));
	}
}




/**** "Find and replace" method.  Takes in original string, finds 
occurences of 'rep', then stores new string in str. ******/

char *replace_str(char *str, char *orig, char *rep)
{
  static char buffer[4096];
  char *p;

  if(!(p = strstr(str, orig)))  /* Is 'orig' even in 'str'? */
    return str;

  strncpy(buffer, str, p-str); /* Copy characters from 'str' start to 'orig' st$ */
  buffer[p-str] = '\0';
  sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

  return buffer;
}





/** function to change directory, inserts home directory into ~ spot if needed **/

void change_directory(char **args , char changepath[1024], char **envp ){

	strcpy(cwd,"");
	if (getcwd(cwd, sizeof(cwd)) == NULL){
		perror("getcwd() error");
	}
	strcpy(changepath,""); 
	char * onemore;
	char final[256]="";

	strcat(changepath, args[1]);
	memmove(final, (envp[28]+5),strlen(envp[28])-3);
	onemore = replace_str(args[1], "~", final);
	strcpy(changepath, onemore);
	chdir(onemore);
	


}

/** lists all current background processes **/
void bglist(){
	int jobcount = 0;
	struct node * pointer = head->next;
	printf("Background Jobs Currently Executing:\n");
	while(pointer!=NULL){
		printf("%d: %10s %s\n", pointer->val, pointer->cmd, pointer->arguments);
		jobcount ++;
		pointer=pointer->next;
	} 
	printf("Total Background Jobs: %d\n", jobcount);

}



int main(int argc, char*argv[], char*envp[]){

	char next_char;
	int i, fd;
	copy_envp(envp);
	char changepath[1024]; 
	head = (struct node *) malloc( sizeof(struct node) ); 
	head->next = NULL;   
    head->val = 0;
	struct node *ptr = (struct node *)malloc(sizeof(struct node *));
	ptr->val=0;
	ptr->next = NULL;
	head = current = ptr;

	/** clears screen for RSI **/
	if(fork() == 0) {
		execvp("clear", argv);
		exit(1);
	} else {
		wait(NULL);
	}


	/** Prints prompt for first time **/
	strcpy(cwd,"");
	if (getcwd(cwd, sizeof(cwd)) == NULL){
		perror("getcwd() error");
	}
	printf("RSI: %s >  ", cwd);
	fflush(stdout);	
	




	char word[24] = "\0";
	char* args[256];
	int m  = 0;
	int child_pid;
	int h;
	char command[24];
	static char arg_buffer[256] = "\0";




    /* while user has not pushed ctrl-d */
	while(next_char != EOF){



		/* gets next char typed in cmd line, adds to words. */
		/* puts the words (arguments) into argv *************/
		next_char=getchar();
		if(isspace(next_char) == 0){
			strncat(word, &next_char, 1);

		} else{
			strcpy(command, word);
			args[m] = (char *)malloc(sizeof(char) * 100);
			strcpy(args[m], command);
			strcpy(word, "");
			m++;
		}
		
		/* if the line is done, ...*/
		if(next_char == '\n'){

			args[m] = (char *)malloc(sizeof(char) * 2);
			args[m] = NULL;
			int r = 0;
			int j = 0;
			int wpid;

			/* if statements check if cmd is bg, bglist, or cd.  */
			/* if not, it executes the command, waits for the child to complete, then prompts again */

			if (strcmp(args[0], "cd" ) == 0){
				change_directory(args, changepath, envp);
			}

			else if(strcmp(args[0], "\0")==0){
				printf("\nNo Command Entered.\n");
			}

			else if(strcmp(args[0], "bglist") == 0){
				bglist();
			}

			/** if it's a bg process, execute process, add info to list, then continue. **/
			else if(strcmp(args[0], "bg") == 0){

				for(r = 0; r<m-1; r++){
					bzero(args[r], sizeof(args[r]));
					strcpy(args[r],args[r+1]);
					
				}
				bzero(args[m-1], sizeof(args[m-1]));
				args[m-1] = NULL;

				if ((child_pid=fork()) == 0) {
					h = execvp(args[0], args);
					printf("errno is %d\n", errno);	
				}

				else {
					int c = 1;
					/* combine separate args to single string for node and future printout at bglist */
					for(c=1; c<m-1; c++){
						strcat(arg_buffer, args[c]);
						strcat(arg_buffer, " ");
					}
					
					add_to_list(child_pid, args[0], arg_buffer);
					bzero(arg_buffer, 256);
				}	
			}	

			/* execute process, then wait until process has finished to continue */
			else {
				pid_t num;
				if ((num = fork()) == 0){
					h = execvp(args[0], args);
					printf("errno is %d\n", errno);
				}
				else {
					waitpid(num, NULL, 0); 
				}

			}

			m = 0;


			/* gets cwd and checks for finished processes.  If there are, remove them from the list of bg processes. */
			strcpy(cwd,"");
			if (getcwd(cwd, sizeof(cwd)) == NULL){
				perror("getcwd() error");
			}
			int status;
		   	int pid;
		    	/* waitpid() returns a PID on success*/
		    	for(;;){
		    		pid = waitpid(-1, &status, WNOHANG);
		    		if(pid <= 0){
		    			break;
		    		}
		    		printf("[proc %d exited with code %d]\n", pid, WEXITSTATUS(status));
				delete_from_list(pid);
		    	}
		    /* prints prompt */
			printf("\nRSI: %s >  ", cwd);
			fflush(stdout);

		}		
	}

}


	
