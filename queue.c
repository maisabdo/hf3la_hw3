//
// Created by Mais on 14/01/2025.
//

#include "queue.h"

Queue createQueue(){
    Queue queue = malloc(sizeof(struct queue_t));
    if (!queue){
        return NULL;
    }
    queue->size = 0;
    queue->first = NULL;
    queue->last = NULL;

    return queue;
}

void destroyQueue(Queue q){
    if(!q){
        return;
    }
    Node curr = q->first;
    while(curr)
    {
        Node temp = curr->next;
        close(curr->fd);
        free(curr);
        curr = temp;
    }
    free(q);
}

Node createNode(int fd_data, struct timeval arrival_time){
    Node node = malloc(sizeof(struct node_t));
    if(!node){
        return NULL;
    }
    node->fd = fd_data;
    node->prev=NULL;
    node->next=NULL;
    node->arrival_time = arrival_time;

    return node;
}

void addNode(Queue q,Node node){
    if(q == NULL || node == NULL){
        return;
    }

    //the queue is empty
    if(q->first == NULL){
        q->first = node;
        q->last = node;
    }
    else{
        q->last->next = node;
        node->prev = q->last;
        q->last = node;
    }

    q->size++;
}

Node pop(Queue q){
    if(!q->first){
        return NULL;
    }
    Node first=q->first;
    q->first=first->next;

    if(q->first==NULL){
        q->first=NULL;
        q->last=NULL;
    }
    else{
        q->first->prev = NULL;
    }
    first->next=NULL;
    q->size--;

    return first;
}

void removeByIndex(Queue q,int index){

}

void removeByFd(Queue q,int fd){

}