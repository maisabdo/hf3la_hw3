//
// Created by Mais on 14/01/2025.
//

#ifndef HW3_QUEUE_H
#define HW3_QUEUE_H

#include "segel.h"

typedef struct node_t {
    int fd;
    struct node_t *prev;
    struct node_t *next;
    struct timeval arrival_time;
    struct timeval dispatch_time;
    struct timeval pickup_time;
}*Node;

typedef  struct queue_t{
    int size;
    Node first;
    Node last;
}*Queue;

//creating and destroying the queue
Queue createQueue();
void destroyQueue(Queue q);

//creating the node
Node createNode(int fd_data, struct timeval arrival_time);

//adding a node to the queue
void addNode(Queue q,Node node);

//popping from queue
Node pop(Queue q);

Node deleteLast(Queue q);

//removing node based on fd or index
void removeByIndex(Queue q,int index);
void removeByFd(Queue q,int fd);

#endif //HW3_QUEUE_H
