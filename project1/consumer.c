#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


int main(int argc, char *argv[]) {
    int i, M;
    int buf;

    if (argc != 2) {
        printf("Usage: ./consumer M");
        exit(-1);
    }

    M = atoi(argv[1]);
    
    for (i = 0; i < M; i++)
        scanf("%d", &buf);

}