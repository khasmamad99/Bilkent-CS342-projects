struct burst {
    int t_index;
    int b_index;
    int length;
    struct timeval wtime;
    double *vruntime;
};


struct node {
    struct burst burst;
    struct node *next;
};


void printList(struct node *);
void printBurst(struct burst);
void insertFirst(struct node **, struct burst);
void insertLast(struct node **, struct burst);
struct node* deleteLast(struct node **);
struct node* deleteFirst(struct node **);
struct node* deleteShortestJob(struct node **);
struct node* deleteHighestPrio(struct node **);
struct node* deleteLowestVruntime(struct node **);