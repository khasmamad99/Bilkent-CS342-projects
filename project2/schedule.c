#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "linkedList.h"

#define MAXFILENAME 100
#define MAXLINELEN 100


struct s_params {
    long *records;
    char *alg;
};

struct w_params {
    int bCount;
    int minB;
    int avgB;
    int minA;
    int avgA;
    int t_id;
};

struct w_params_fromfile {
    char filename[MAXFILENAME];
    int t_id;
};

void* worker(void *w_params);
void* worker_fromfile(void *w_params);
void* scheduler(void *w_params);
double randExp(double lambda);
long long getTimeDiff(struct timeval first, struct timeval second);

struct node *rq_head;

pthread_cond_t cond_hasitem;
pthread_mutex_t mutex_rq;


int main(int argc, char *argv[]) {
    int N, bCount, minB, avgB, minA, avgA;
    char *alg;
    char *inprefix;
    int fromFile = 0;

    if (argc == 8) {
        N = atoi(argv[1]);
        bCount = atoi(argv[2]);
        minB = atoi(argv[3]);
        avgB = atoi(argv[4]);
        minA = atoi(argv[5]);
        avgA = atoi(argv[6]);
        alg = argv[7];

        if (minB < 100 || minA < 100) {
            printf("minA and minB cannot be less than 100\n");
            exit(EXIT_FAILURE);
        }

        printf("%d %d %d %d %d %d %s\n", N, bCount, minB, avgB, minA, avgA, alg);

    } else if (argc == 5) {
        N = atoi(argv[1]);
        alg = argv[2];
        if (strcmp(argv[3], "-f") != 0) {
            printf("Unrecognized option %s", argv[3]);
            exit(EXIT_FAILURE);
        } else
            inprefix = argv[4];
        
        fromFile = 1;
        bCount = minB = avgB = minA = avgA = -1;
        printf("%d %s %s\n", N, alg, inprefix);
        
    } else {
        printf(
            "Usage: ./schedule <N> <Bcount> <minB> <avgB> <minA> <avgA> <ALG>\n"
            "   or: ./schedule <N> <ALG> -f <inprefix>\n");
        exit(EXIT_FAILURE);
    }

    if (N < 1 || N > 10) {
        printf("N should be between 1 and 10.");
        exit(EXIT_FAILURE);
    } 

    int i, rc;
    pthread_t scheduler_th;
    pthread_t w_threads[N];
    struct w_params_fromfile params_fromfile[N];
    struct w_params params[N];

    pthread_mutex_init(&mutex_rq, NULL);
	pthread_cond_init(&cond_hasitem, NULL);

    // set the queue head to NULL
    rq_head = NULL;
    
    // create the scheduler thread
    struct s_params s_params;
    s_params.alg = alg;
    long long records[N];
    for (i = 0; i < N; i++)
        records[i] = 0;
    s_params.records = records;
    rc = pthread_create(&scheduler_th, NULL, scheduler, (void *) &s_params);
    if (rc) {
        printf("Unable to create thread with thread id %d\n", rc);
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < N; i++) {
        if (fromFile == 1) {
            params_fromfile[i].t_id = i;
            char filename[MAXFILENAME];
            sprintf(filename, "%s-%d.txt", inprefix, i);
            strcpy(params_fromfile[i].filename, filename);
            rc = pthread_create(&w_threads[i], NULL, worker_fromfile, (void *) &params_fromfile[i]);
            if (rc) {
                printf("Unable to create thread no %d with thread id %d\n", i, rc);
                exit(EXIT_FAILURE);
            }
        } else {
            params[i].bCount = bCount;
            params[i].avgA = avgA;
            params[i].avgB = avgB;
            params[i].minA = minA;
            params[i].minB = minB;
            params[i].t_id = i;
            rc = pthread_create(&w_threads[i], NULL, worker, (void *) &params[i]);
            if (rc) {
                printf("Unable to create thread no %d with thread id %d\n", i, rc);
                exit(EXIT_FAILURE);
            }
        }
    }

    // wait for threads to terminate
    for (i = 0; i < N; i++)
	    pthread_join(w_threads[i], NULL); 

	// pthread_join(scheduler_th, NULL);

    // wait for the scheduler to finish
    sleep(1);

    // destroy the mutex & condition variable
	// pthread_mutex_destroy(&mutex_rq);
	// pthread_cond_destroy(&cond_hasitem);


    // Experiment
    long long total_waitTime = 0;
    double avg_waitTime;
    printf("\n*************** WAIT TIMES ***************\n");
    for (i = 0; i < N; i++) {
        printf("THREAD # %d : %ld\n", i, records[i]);
        total_waitTime += records[i];
    }
    avg_waitTime = (double) total_waitTime / N;
    printf("\nTotal Wait Time : %lld\n", total_waitTime);
    printf("Avg Wait Time : %.2f\n", avg_waitTime);

    return 0;
}


void* worker_fromfile(void *w_params_fromfile) {
    struct w_params_fromfile *params = (struct w_params_fromfile *) w_params_fromfile;
    FILE *fp;

    printf("Worker_fromfile %d is initialized\n", params->t_id);

    if ((fp = fopen(params->filename, "r")) == NULL) {
        printf("Worker %d could not open the file %s\n", params->t_id, params->filename);
        exit(EXIT_FAILURE);
    }

    int i = 0;
    double vruntime = 0;
    struct timeval curTime;
    int waitTime, burstTime;
    char line[MAXLINELEN];
    while (fgets(line, MAXLINELEN, fp)) {
        if (sscanf(line, "%d %d", &waitTime, &burstTime) == 2) {
            usleep(waitTime);

            gettimeofday(&curTime, NULL);
            struct burst burst = {
                params->t_id, i, burstTime, curTime, &vruntime
            };

            // get mutex lock
            pthread_mutex_lock(&mutex_rq);

            insertLast(&rq_head, burst);
            printf("Worker %d produced a burst with length %d, vruntime = %f\n", params->t_id, burst.length, vruntime);

            pthread_cond_signal(&cond_hasitem);     

            // release mutex lock
            pthread_mutex_unlock(&mutex_rq);
            ++i;
        } else
            printf("Wrong line format %s in file %s\n", line, params->filename);
    }
}


void* worker(void *w_params) {
    struct w_params *params = (struct w_params *) w_params;
    int i;
    int randTime;
    struct timeval curTime;
    
    double vruntime = 0;
    srand(params->t_id);

    for (i = 0; i < params->bCount; i++) {
        // sleep
        while((randTime = (int) randExp(1.0 / params->avgA)) < params->minA)
            ;
        usleep(randTime);

        while((randTime = (int) randExp(1.0/ params->avgB)) < params->minB)
            ;
        gettimeofday(&curTime, NULL);
        struct burst burst = {
            params->t_id, i, randTime, curTime, &vruntime
        };

        // get mutex lock
        pthread_mutex_lock(&mutex_rq);

        insertLast(&rq_head, burst);
        printf("Worker %d produced a burst with length %d, vruntime = %f\n", params->t_id, burst.length, vruntime);

        pthread_cond_signal(&cond_hasitem);     

        // release mutex lock
        pthread_mutex_unlock(&mutex_rq);
    }
}


void* scheduler(void *args) {
    struct s_params *params = (struct s_params *) args;
    struct node *node;
    double vruntime = 0;

    struct timeval curTime;

    printf("Scheduler initialized\n");

    while (1) {
        // acquire the mutex lock
        pthread_mutex_lock(&mutex_rq);


        // wait if there is no item
        if (rq_head == NULL) {
            printf("Queue is empty\n");
            pthread_cond_wait(&cond_hasitem, &mutex_rq);
        }

        if (strcmp(params->alg, "FCFS") == 0)
            node = deleteFirst(&rq_head);

        else if (strcmp(params->alg, "SJF") == 0)
            node = deleteShortestJob(&rq_head); 

        else if (strcmp(params->alg, "PRIO") == 0)
            node = deleteHighestPrio(&rq_head);

        else if (strcmp(params->alg, "VRUNTIME") == 0) {
            node = deleteLowestVruntime(&rq_head);
            vruntime = *(node->burst.vruntime);
            *(node->burst.vruntime) += 
                node->burst.length * (0.7 + 0.3*node->burst.t_index);
        }

        else {
            printf("Alg %s is not recognized\n", params->alg);
            exit(EXIT_FAILURE);
        }

        // Experiment
        gettimeofday(&curTime, NULL);
        long long waitTime = getTimeDiff(node->burst.wtime, curTime);
        params->records[node->burst.t_index] += waitTime;

        usleep(node->burst.length);
        printf("Scheduler ran a burst with length %d and vruntime %f\n", node->burst.length, vruntime);
        free((void *) node);

        pthread_mutex_unlock(&mutex_rq);
    }
}

double randExp(double lambda) {
    double x;
    double rand_num = (double) rand() / RAND_MAX;
    x = log(1 - rand_num) / (-lambda);
    return x;
}


long long getTimeDiff(struct timeval first, struct timeval second) {
    long long first_usec, second_usec;
    first_usec = first.tv_sec % 100 * 1000000 + first.tv_usec;
    second_usec = second.tv_sec % 100 * 1000000 + second.tv_usec;
    return second_usec - first_usec;
}