#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

struct ThreadArgs {
    int fd1;
    int fd0;
};
struct MainArgs {
    int fd1;
    int fd0;
    char redirection;
    char *input;
};
struct ParseArgs {
    char *input;
    char *option;
    char redirection;
    char *args;
    char background;
};


void write_parse(struct ParseArgs *parseArgs) {
    pthread_mutex_lock(&lock);
    char *input = parseArgs->input;
    char *option = parseArgs->option;
    char redirection = parseArgs->redirection;
    char *args = parseArgs->args;
    char background = parseArgs->background;

    FILE *parse = fopen("./parse.txt", "a");

    if (parse == NULL) {
        perror("fopen for write_parse");
        pthread_mutex_unlock(&lock);
        return;
    }

    fprintf(parse, "----------\n");
    fprintf(parse, "Command: %s\n", args);
    fprintf(parse, "Input: %s\n", input);
    fprintf(parse, "Option: %s\n", option);
    fprintf(parse, "Redirection: %c\n", redirection);
    fprintf(parse, "Background: %c\n", background);
    fprintf(parse, "----------\n");
    fflush(parse);
    fclose(parse);
    pthread_mutex_unlock(&lock);
}
void *write_parse_thread(void *arg) {
    struct ParseArgs *parseArgs = (struct ParseArgs *)arg;
    write_parse(parseArgs);
    return NULL;
}
void *mythread(void *arg) {
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)arg;
    int pipeRead = threadArgs->fd0;
    int pipeWrite = threadArgs->fd1;

    pthread_mutex_lock(&lock);
    printf("---- tid %ld\n", (long)pthread_self());
	
    close(pipeWrite);
    FILE *pipeFile = fdopen(pipeRead, "r");
    if (pipeFile == NULL) {
        perror("fopen");
        pthread_mutex_unlock(&lock);
        return NULL;
    }

    char buf[1000];
    while(fgets(buf, sizeof(buf), pipeFile) != NULL) {
        printf("%s\n", buf);
        fflush(stdout);
    }

    fclose(pipeFile);
    printf("---- tid %ld\n", (long)pthread_self());
    pthread_mutex_unlock(&lock);
    return NULL;
}
void *mythread_print(void *arg) {
    struct MainArgs *mainArgs = (struct MainArgs *)arg;
    int pipeRead = mainArgs->fd0;
    int pipeWrite = mainArgs->fd1;
    char redirection = mainArgs->redirection;
    char *input = mainArgs->input;
    
    pthread_mutex_lock(&lock);
    printf("---- tid %ld\n", (long)pthread_self());
	
    close(pipeWrite);
    FILE *pipeFile = fdopen(pipeRead, "r");
    if (pipeFile == NULL) {
        perror("fopen");
        pthread_mutex_unlock(&lock);
        return NULL;
    }
     
     if (redirection == '>') {
        int output_fd = open(input, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
        if (output_fd == -1) {
               perror("open for redirection");
               exit(EXIT_FAILURE);
        }
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }
    else{
     
    char buf[1000];
    while(fgets(buf, sizeof(buf), pipeFile) != NULL) {
        printf("%s\n", buf);
        fflush(stdout);
    }
}
    fclose(pipeFile);
    printf("---- tid %ld\n", (long)pthread_self());
    pthread_mutex_unlock(&lock);
    return NULL;
}
int main(int argc, char *argv[]) {
    FILE *commands = fopen("commands.txt", "r");
    char line[250];

    while (fgets(line, sizeof(line), commands) != NULL) {
        int len = strlen(line);
        if (line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        char *token = strtok(line, " ");
        char *args[7];
        char background = 'n';
        char *extension = ".txt";
        char *extensionc = ".c";
        char *option = NULL;
        char *input = NULL;
        int arg = -1;
        char redirection = '-';

        while (token != NULL) {
            if (strchr(token, '&') != NULL) {
                background = 'y';
         
            } else if (strchr(token, '-') != NULL) {
                arg++;
                option = token;
                args[arg] = strdup(token);
   
            } else if (strchr(token, '<') != NULL) {
                redirection = '<';
                token = strtok(NULL, " ");
                input = token;
          
                token = strtok(NULL, " ");
            } else if (strchr(token, '>') != NULL) {
                redirection = '>';
                token = strtok(NULL, " ");
                input = token;
    
                char *output = (char *)malloc(len + 3);
                strcpy(output, "./");
                strcat(output, input);
                strcpy(input, output);

                token = strtok(NULL, " ");
            } else if (strcmp(token, "wc") == 0) {
                arg++;
                args[arg] = strdup(token);

                token = strtok(NULL, " ");
                input = token;


                arg++;
                args[arg] = strdup(token);
            } else {
                arg++;
                args[arg] = strdup(token);
            }

            token = strtok(NULL, " ");
        }
        arg++;
        args[arg] = NULL;

        struct ParseArgs parseArgs = {
            .input = args[1],
            .option = option,
            .redirection = redirection,
            .args = args[0],
            .background = background};

        pthread_t parseThread;
        pthread_create(&parseThread, NULL, write_parse_thread, &parseArgs);

	int fd_print[2];

            if (pipe(fd_print) == -1) {
                perror("pipe error");
                exit(EXIT_FAILURE);
            }
        struct MainArgs mainArgs = {fd_print[1], fd_print[0], redirection, input};

        if (redirection != '-') {
            int rc1 = fork();
            if (rc1 == 0) {
		if (redirection == '<') {
                    FILE *input_file = fopen(input, "r");
                    if (input_file == NULL) {
                        perror("fopen for execute");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fileno(input_file), STDIN_FILENO);
                    fclose(input_file);
                }
                
                close(fd_print[0]);
                dup2(fd_print[1], STDOUT_FILENO);
                close(fd_print[1]);
                
                execvp(args[0], args);
            }
                       
            else if (rc1 > 0) {
                if (background != 'y') {
                    waitpid(rc1, NULL, 0);
                }
                pthread_t tid_print;
                pthread_create(&tid_print, NULL, mythread_print, &mainArgs);
                
            }
        } else {
            int fd[2];

            if (pipe(fd) == -1) {
                perror("pipe error");
                exit(EXIT_FAILURE);
            }

            struct ThreadArgs threadArgs = {fd[1], fd[0]};

            int rc = fork();
            if (rc > 0) {
            
                if (background != 'y') {
                    waitpid(rc, NULL, 0);
                }
                pthread_t tid;
                pthread_create(&tid, NULL, mythread, &threadArgs);

            } else if (rc == 0) {
          	
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);

                execvp(args[0], args);
            }
        }
    }

    return 0;
}

