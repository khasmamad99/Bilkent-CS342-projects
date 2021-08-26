
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>
#include <semaphore.h>

// Define a name for your shared memory; you can give any name that start with a slash character; it will be like a filename.
#define MEMNAME "/buddymem"
#define SEMNAME "/buddysem"
#define MAX_M 18
#define MIN_M 13
#define MAX_PROC 10

// Define semaphore(s)


// Define your stuctures and variables.
struct proc_info {
    int id;
    void *start_addr;
};
struct proc_info processes[MAX_PROC];
struct block_attr {
    int tag;
    int kval;
    uintptr_t loc;
    uintptr_t next;
    uintptr_t prev;
};

struct block_attr avail[MAX_M];
void *start_ptr;
int fd, m;
int check_access();

int sbmem_init(int segmentsize)
{
    printf ("sbmem init called"); // remove all printfs when you are submitting to us.  

    if (segmentsize < pow(2, MIN_M) || segmentsize > pow(2, MAX_M)) {
        printf("segmentsize should be between 32KB (2^13B) and 256KB (2^18B)\n");
        return -1;
    }

    if ((fd = shm_open(MEMNAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | O_EXCL)) == -1) {
        if (errno == EEXIST) {
            shm_unlink(MEMNAME);
            if ((fd = shm_open(MEMNAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | O_EXCL)) == -1) {
                printf("shm_open failed\n");
                return -1;
            }
        } else {
            printf("shm_open failed\n");
            return -1;
        }
    }

    m = (int) log2(segmentsize) + 1;
    int size = (int) pow(2, MAX_M) + MAX_PROC * sizeof(struct proc_info)
                               + MAX_M * sizeof(struct block_attr);
    if (ftruncate(fd, size) == -1) {
        printf("ftrunctate failed\n");
        return -1;
    }

    // initialize the list of processes to -1 (no processes currently)
    start_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (start_ptr == (void *) -1) {
        printf("mmap failed\n");
        return -1;
    }
    struct proc_info* processes = start_ptr;
    for (int i = 0; i < MAX_PROC; i++) {
        processes[i].id = -1;
        processes[i].start_addr = NULL;
    }

    struct block_attr* avail = processes + MAX_PROC * sizeof(struct proc_info);

    for (int i = 0; i <= MAX_M; i++) {
        avail[i].tag = 0;
        avail[i].kval = i;
        avail[i].loc = -1;
        avail[i].prev = avail[i].next = avail[i].loc;
    }

    // first size m block
    struct block_attr *cur_block_ptr = avail + m * sizeof(struct block_attr);
    cur_block_ptr->tag = 1;
    cur_block_ptr->kval = m;
    cur_block_ptr->loc = 0;
    cur_block_ptr->prev = cur_block_ptr->next = avail[m].loc;
    avail[m].next = 0;
    avail[m].prev = 0;

    // create the semaphore
    sem_t *sem = sem_open(SEMNAME, O_CREAT | O_EXCL, 0666, 1);
    if (sem == SEM_FAILED) {
        if (errno == EEXIST) {
            sem_unlink(SEMNAME);
            sem = sem_open(SEMNAME, O_CREAT | O_EXCL, 0666, 1);
        } else {
            printf("semopen failed\n");
            return -1;
        }
    }
    

    return (0);
}

int sbmem_remove()
{
    shm_unlink(MEMNAME);

    // remove the semaphores
    sem_unlink(SEMNAME);
    return (0); 
}

int sbmem_open()
{
    // find an empty slot
    // printf("ALALAALLA\n");
    // int i;
    // for (i = 0; i < MAX_PROC && processes[i].id != -1; i++)
    //     ;
    // if (i == MAX_PROC) {
    //     printf("i == MAX_PRO\n");
    //     return -1;
    // }
    
    // processes[i].id = getpid();
    sem_t *sem = sem_open(SEMNAME, O_RDWR | O_CREAT);
    sem_wait(sem);
    if ((fd = shm_open(MEMNAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | O_EXCL)) == -1) {
        if (errno == EEXIST) {
            shm_unlink(MEMNAME);
            if ((fd = shm_open(MEMNAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | O_EXCL)) == -1) {
                printf("shm_open failed\n");
                return -1;
            }
        } else {
            printf("shm_open failed\n");
            return -1;
        }
    }
    int size = (int) pow(2, MAX_M) + MAX_PROC * sizeof(struct proc_info)
                               + MAX_M * sizeof(struct block_attr);
    start_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (start_ptr == (void *) -1) {
        printf("mmap failed\n");
        return -1;
    }

    int i;
    struct proc_info* processes = start_ptr;
    for (i = 0; i < MAX_PROC && processes[i].id != -1; i++)
        ;
    if (i == MAX_PROC) {
        printf("sbmem_open failed\n");
        return -1;
    }
    processes[i].start_addr = start_ptr;
    processes[i].id = getpid();

    sem_post(sem);
    return (0); 
}


void *sbmem_alloc (int size)
{
    sem_t *sem = sem_open(SEMNAME, O_RDWR | O_CREAT);
    sem_wait(sem);
    int i, j;
    if ((i = check_access()) == -1)
        return NULL;

    void *start_addr = start_ptr + MAX_PROC * sizeof(struct proc_info) + MAX_M * sizeof(struct block_attr);
    struct block_attr *avail = start_ptr + MAX_PROC * sizeof(struct proc_info);
    int k = (int) log2(size) + 1;
    // find block
    for (j = MAX_M; j >= k && avail[j].next != avail[j].loc; j--)
        ;
    
    // no such block exists
    if (j < k)
        return NULL;

    // remove from the list
    uintptr_t L, P;
    struct block_attr *L_ptr, *P_ptr;
    L = avail[j].prev;
    L_ptr = start_addr + L;
    P = L_ptr->prev;
    P_ptr = start_addr + P;
    avail[j].prev = P;
    P_ptr->next = avail[j].loc;
    L_ptr->tag = 0;

    while (1) {
        if (j == k) {
            // no split required
            // void *addr = mmap(NULL, (int) pow(2, m), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            // if (addr == (void *) -1) {
            //     printf("mmap failed\n");
            //     return NULL;
            // }
            // do not allow the process to access the block information
            void *return_addr = start_addr + L + sizeof(struct block_attr);
            sem_post(sem);
            return return_addr;
        }
        // split required
        j--;
        P = L + (int) pow(2, j);
        struct block_attr *P_ptr = start_addr + P;
        P_ptr->tag = 1;
        P_ptr->kval = j;
        P_ptr->next = P_ptr->prev = avail[j].loc;
        avail[j].next = avail[j].prev = P;
    }

    sem_post(sem);
    return (NULL);
}


void sbmem_free (void *p)
{
    // account for the block information
    sem_t *sem = sem_open(SEMNAME, O_RDWR | O_CREAT);
    sem_wait(sem);
    int i;
    if ((i = check_access()) != -1) {
        
        void *start_addr = start_ptr + MAX_PROC * sizeof(struct proc_info) + m * sizeof(struct block_attr);
        struct block_attr *avail = start_ptr + MAX_PROC * sizeof(struct proc_info);
        uintptr_t L, P;
        int k;
        struct block_attr *L_ptr, *P_ptr, *P_prev_ptr, *P_next_ptr;

        L = (uintptr_t) p - (uintptr_t) start_ptr - sizeof(struct block_attr);
        L_ptr = p - sizeof(struct block_attr);
        k = L_ptr->kval;

        while(1) {
            // is buddy available?
            P = L ^ (int) pow(2, k);
            P_ptr = start_addr + P;

            if (k == m || P_ptr->tag == 0 || (P_ptr->tag == 1 && P_ptr->kval != k)) {
                // put L on list
                L_ptr->tag = 1;
                P = avail[k].next;
                P_ptr = start_addr + P;
                L_ptr->next = P;
                P_ptr->prev = L;
                L_ptr->kval = k;
                L_ptr->prev = avail[k].loc;
                avail[k].next = L;
                break;
            } else {
                // combine with buddy
                // remove P
                P_prev_ptr = start_addr + P_ptr->prev;
                P_prev_ptr->next = P_ptr->next;

                P_next_ptr = start_addr + P_ptr->next;
                P_next_ptr->prev = P_ptr->prev;

                k++;
                if (P < L)
                    L_ptr->loc = P;
            }
        }
    }
    sem_post(sem);

 
}

int sbmem_close()
{
    sem_t *sem = sem_open(SEMNAME, O_RDWR | O_CREAT);
    sem_wait(sem);
    struct proc_info *processes = start_ptr;
    int i = check_access();
    if (i == -1)
        return -1;

    processes[i].id = -1;
    processes[i].start_addr = NULL;

    sem_post(sem);
    return (0); 
}

int check_access() {
    struct proc_info *processes = start_ptr;
    int i;
    int pid = getpid();
    for (i = 0; i < MAX_PROC && processes[i].id != pid; i++)
        ;
    if (i == MAX_PROC)
        return -1;

    return i;
}
