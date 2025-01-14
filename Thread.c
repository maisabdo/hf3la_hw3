//
// Created by Mona Nahas on 14/01/2025.
//
#include "Thread.h"
#include <stdlib.h>

threads_stats createThread(int id)
{
    threads_stats newTh = malloc(sizeof(struct Threads_stats));
    if (!newTh){
        return NULL;
    }

    newTh->id = id;
    newTh->stat_req = 0;
    newTh->dynm_req = 0;
    newTh->total_req = 0;

    return newTh;
}

