#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>


int main (int argc, char *argv[]) {
    int i, M;
    int buf;
    time_t t;

    if (argc != 2) {
        printf("Usage: ./producer M");
        exit(-1);
    }

    M = atoi(argv[1]);
    srand((unsigned) time(&t));
    for (i = 0; i < M; i++) {
        buf = rand();
        printf("%d\n", buf);
    }
}