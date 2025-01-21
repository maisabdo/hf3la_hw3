#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "stdbool.h"
#include "queue.h"
typedef struct Threads_stats{
	int id;
	int stat_req;
	int dynm_req;
	int total_req;
    bool is_vip;
} * threads_stats;

// handle a request
void requestHandle(int fd, struct timeval arrival, struct timeval dispatch, threads_stats t_stats, Queue queue);

//  Returns True/False if realtime event
int getRequestMetaData(int fd);

#endif
