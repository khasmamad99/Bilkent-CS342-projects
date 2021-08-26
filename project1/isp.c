#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>

#define PROMPT "isp$"
#define MAXCOM 128
#define MAXBUF 4096



int tokenize(char *str, char *delim, char ***strv);
void makePipe(int pipefd[2]);


int main(int argc, char *argv[]) {
    int N, mode, cpid, noTokensPipe;
    int n, charCnt, rwCnt;
    char prompt[] = PROMPT;

    char **strv, **strvc1, **strvc2;;
    char *space, *pip, *temp;
    char command[MAXCOM];
    char buf[MAXBUF];

    struct timeval start, end;

    space = " ";
    pip = "|";
    
    charCnt = rwCnt = 0;

    int pipefd1[2], pipefd2[2];

    if (argc != 3) {
        printf("Usage: ./isp <N> <mode>");
        exit(1);
    }

    N = atoi(argv[1]);
    mode = atoi(argv[2]);

    while(1) {
        printf("%s ", prompt);
        if (fgets(command, MAXCOM, stdin) != NULL) {
            // replace '\n' with '\0'
            temp = command - 1;
            while(*++temp != '\n')
                ;
            *temp = '\0';

            noTokensPipe = tokenize(command, pip, &strv);

            if (noTokensPipe == 1) {
                tokenize(strv[0], space, &strvc1);
                cpid = fork();
                if (cpid == 0) {
                    execvpe(strvc1[0], strvc1, __environ);
                }
                wait(NULL);
                free(strvc1);
            } 
            
            else if (noTokensPipe == 2) {
                if (mode == 1) {
                    makePipe(pipefd1);
                    tokenize(strv[0], space, &strvc1);
                    gettimeofday(&start, NULL);

                    cpid = fork();
                    if (cpid == 0) {
                        close(pipefd1[0]);               // close read end
                        if (dup2(pipefd1[1], 1) != -1)   // redirect output end to the pipe
                            close(pipefd1[1]);           // and close
                        exit(execvpe(strvc1[0], strvc1, __environ));
                    }
                    close(pipefd1[1]);   // close the write end for parent
                    // get rid of spaces
                    while (*strv[1] == ' ')
                        strv[1]++;
                    printf("%s\n", strv[1]);
                    tokenize(strv[1], space, &strvc2);

                    cpid = fork();
                    if (cpid == 0) {
                        close(pipefd1[1]);     // close write end
                        dup2(pipefd1[0], 0);   // redirect input end to the pipe
                        close(pipefd1[0]);          // and close
                        execvpe(strvc2[0], strvc2, __environ);
                    }

                    close(pipefd1[0]);   // close the read end for parent


                    wait(NULL);
                    wait(NULL);

                    gettimeofday(&end, NULL);

                    printf("TIME: %ld ms\n", 
                        ((end.tv_sec * 1000000 + end.tv_usec) -
                         (start.tv_sec * 1000000 + start.tv_usec)));

                    free(strvc1);
                    free(strvc2);
                
                } else if (mode == 2) {
                    makePipe(pipefd1);
                    makePipe(pipefd2);

                    tokenize(strv[0], space, &strvc1);
                    // get rid of spaces
                    while (*strv[1] == ' ')
                        strv[1]++;
                    printf("%s\n", strv[1]);
                    tokenize(strv[1], space, &strvc2);


                    gettimeofday(&start, NULL);

                    // first child
                    cpid = fork();
                    if (cpid == 0) {
                        close(pipefd2[0]);
                        close(pipefd2[1]);

                        close(pipefd1[0]);   // close the read end
                        dup2(pipefd1[1], 1); // redirect the output end to the pipe
                        close(pipefd1[1]);
                        execvpe(strvc1[0], strvc1, __environ);
                    }

                    // second child
                    cpid = fork();
                    if (cpid == 0) {
                        close(pipefd1[0]);
                        close(pipefd1[1]);

                        close(pipefd2[1]);   // close the write end
                        dup2(pipefd2[0], 0); // redirect the input end to the pipe
                        close(pipefd2[0]);
                        execvpe(strvc2[0], strvc2, __environ);
                    }

                    // parent
                    close(pipefd1[1]);  // close the write end of the first pipe for parent
                    close(pipefd2[0]);  // close the read end of the second pipe for parent

                    while((n = read(pipefd1[0], buf, N)) > 0) {
                        if (n <= 0) {
                            printf("N IS LESS THAN OR EQUAL TO ZERO");
                        }
                        write(pipefd2[1], buf, n);
                        charCnt += n;
                        ++rwCnt;
                    }

                    close(pipefd1[0]);
                    close(pipefd2[1]);


                    wait(NULL);
                    wait(NULL);

                    gettimeofday(&end, NULL);

                    printf("TIME: %ld ms\n", 
                        ((end.tv_sec * 1000000 + end.tv_usec) -
                         (start.tv_sec * 1000000 + start.tv_usec)));



                    free(strvc1);
                    free(strvc2);

                    printf(
                        "character-count: %d\n"
                        "read-call-count: %d\n"
                        "write-call-count: %d\n",
                        charCnt, rwCnt, rwCnt
                    );
                } else {
                    printf("mode number %d is not correct, try 1 or 2\n", mode);
                    exit(-1);
                }

            } 
            
            else
                printf("ERROR:"
                       "Unexpected number of pipe separated commands");
            
            free(strv);
        }
    }

}


void makePipe(int pipefd[2]) {
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
}


int tokenize(char *str, char *delim, char ***strv) {
    int i, count;
    char *token, *temp;

    count = 0;
    token = strtok(str, delim);
    while(token) {
        count++;
        token = strtok(NULL, delim);
    }

    *strv = (char **) malloc((count+1) * sizeof(char **));
    temp = str;
    for (i = 0; i < count; i++) {
        (*strv)[i] = temp;
        while(*temp++)
            ;
    }

    (*strv)[count] = NULL;

    return count;
}

