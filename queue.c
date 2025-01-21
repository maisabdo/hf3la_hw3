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

Node deleteLast(Queue q) {
    if(q == NULL || q->last == NULL){
        return NULL;
    }

    Node lastNode = q->last;
    if(q->first == q->last){
        q->first = NULL;
        q->last = NULL;
    }
    else{
        q->last = lastNode->prev;
        q->last->next = NULL;
    }

    lastNode->prev = NULL;
    q->size--;

    return lastNode;
}


void removeByIndex(Queue q,int index){
    if(q==NULL){
        return;
    }
    if(index >= q->size || index < 0){
        return;
    }
    Node current = q->first;
    for (int i = 0; i < index; i++) {
        current=current->next;
    }
    removeByFd(q,current->fd);
}

void removeByFd(Queue q,int fd){
    if(q == NULL || q->first == NULL){
        return;
    }
    Node current = q->first;

    if(current->fd == fd){
        q->first = current->next;
        if(q->first == NULL){
            q->last = NULL;
        }
        else{
            q->first->prev = NULL;
        }
        close(current->fd);
        free(current);
        q->size--;
        return;
    }

    while(current!=NULL && current->fd != fd){
        current = current->next;
    }

    if(current == NULL){
        return;
    }

    if(current->next != NULL){
        current->next->prev = current->prev;
    }
    else{
        q->last = current->prev;
    }

    if(current->prev != NULL){
        current->prev->next = current->next;
    }

    close(current->fd);
    free(current);
    q->size--;
}