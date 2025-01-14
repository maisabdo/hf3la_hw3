//
// Created by Mona Nahas on 14/01/2025.
//

#ifndef HW3_THREAD_H
#define HW3_THREAD_H

typedef struct Threads_stats
{
    int id;
    int stat_req;
    int dynm_req;
    int total_req;
} *threads_stats;

threads_stats createThread(int id);

#endif //HW3_THREAD_H
