#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

int main(int argc, char *argv[]) {
	int fd[2];
	pipe(fd);

	printf("PID: %d - Main command is: man rm | grep -- -r > output.txt\n", (int) getpid());
	int rc = fork();
	if (rc < 0){
		fprintf(stderr, "fork failed\n");
		exit(1);
	}
	else if(rc==0){
		printf("I'm MAN process, with PID: %d - My command is: man rm\n", (int) getpid());
		int rc2 = fork();
		if(rc2<0) {
			fprintf(stderr, "fork failed\n");
			exit(1);
		}
		else if (rc2  == 0) {
			printf("I'm GREP process, with PID: %d - My command is: grep -- -r\n", (int) getpid());
			close(fd[1]);
			dup2(fd[0], STDIN_FILENO);
			close(fd[0]);
			
			close(STDOUT_FILENO);
			open("./output.txt", O_CREAT|O_WRONLY|O_TRUNC,S_IRWXU);
			printf("Hello world");
			
			char *myargs[] = {"grep", "--", "-r", NULL};
		
			execvp(myargs[0], myargs);
			
			
		}
	
		close(fd[0]);
		
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);
		
		char *myargs[3];
		myargs[0] = strdup("man");
		myargs[1] = strdup("rm");
		myargs[2] = NULL;
		
		execvp(myargs[0], myargs);
		printf("this shouldnt be printed");
		
	}
	else {
		close(fd[1]);
		close(fd[0]);
		
		
		wait(NULL);
		wait(NULL);
		printf("I'm SHELL process, with PID: %d - Execution is completed, you can find the results in output.txt", (int) getpid()); 
	}
	return 0;
}

