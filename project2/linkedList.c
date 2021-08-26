#include <stdlib.h>
#include <stdio.h>
#include "linkedList.h"


void printBurst(struct burst burst) {
    printf(
        "Thread index: %d\n"
        "Burst index:  %d\n"
        "Length:       %d\n"
        "Wall time:    %ld\n"
        "VRunTime:     %f\n",
        burst.t_index, burst.b_index, burst.length, burst.wtime.tv_usec, *burst.vruntime
    );
}

void printList(struct node *head) {
    struct node *ptr = head;
    printf("\n******************************************\n");
    while (ptr != NULL) {
        printBurst(ptr->burst);
        printf("\n");
        ptr = ptr->next;
    }
    printf("*****************************************\n\n");
}

void insertFirst(struct node **head, struct burst burst) {
    struct node *link = (struct node*) malloc(sizeof(struct node));

    link->burst = burst;
    link->next = *head;
    *head = link;
}


void insertLast(struct node **head, struct burst burst) {
    struct node *link = (struct node*) malloc(sizeof(struct node));
    link->burst = burst;
    link->next = NULL;
    
    struct node *ptr = *head;
    if (ptr == NULL)
        *head = link;
    else {
        while (ptr->next != NULL)
            ptr = ptr->next;
        ptr->next = link;
    }
}


struct node* deleteLast(struct node **head) {
    struct node *ptr = *head;
    if (ptr == NULL)
        return NULL;

    if (ptr->next == NULL) {
        *head = NULL;
        return ptr;
    }

    while (ptr->next->next != NULL)
        ptr = ptr->next;
    
    struct node *tmp = ptr->next;
    ptr->next = NULL;
    return tmp;
}


struct node* deleteFirst(struct node **head) {
    if (head == NULL)
        return NULL;

    struct node *tmp = *head;
    *head = tmp->next;

    return tmp;
}


struct node* deleteShortestJob(struct node **head) {
    if (*head == NULL)
        return NULL;

    struct node *shortest = *head;
    struct node *ptr = (*head)->next;

    while (ptr != NULL) {
        if (ptr->burst.length <= shortest->burst.length &&
                ptr->burst.t_index != shortest->burst.t_index) {
            shortest = ptr;
        }
        ptr = ptr->next;
    }

    if (*head == shortest)
        return deleteFirst(head);

    ptr = *head;
    while (ptr->next != shortest) 
        ptr = ptr->next;

    ptr->next = shortest->next;
    return shortest;
}

struct node* deleteHighestPrio(struct node **head) {
    if (*head == NULL)
        return NULL;

    struct node *highestPrio = *head;
    struct node *ptr = (*head)->next;

    while (ptr != NULL) {
        if (ptr->burst.t_index < highestPrio->burst.t_index)
            highestPrio = ptr;
        ptr = ptr->next;
    }    

    if (*head == highestPrio)
        return deleteFirst(head);
    
    ptr = *head;
    while (ptr->next != highestPrio)
        ptr = ptr->next;

    ptr->next = highestPrio->next;
    return highestPrio;
}


struct node* deleteLowestVruntime(struct node **head) {
    if (*head == NULL)
        return NULL;

    struct node *lowestVruntime = *head;
    struct node *ptr = (*head)->next;

    while (ptr != NULL) {
        if ( *(ptr->burst.vruntime) < *(lowestVruntime->burst.vruntime) )
            lowestVruntime = ptr;
        ptr = ptr->next;
    }

    if (*head == lowestVruntime)
        return deleteFirst(head);

    ptr = *head;
    while (ptr->next != lowestVruntime)
        ptr = ptr->next;

    ptr->next = lowestVruntime->next;
    return lowestVruntime;
}