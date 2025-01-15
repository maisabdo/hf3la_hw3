#include "segel.h"
#include "request.h"
#include "queue.h"
#define MIN_PNUM 1024
#define MAX_PNUM 65535

Queue waitingRequests_regular;
Queue workingRequests_regular;
Queue waitingRequests_vip;
pthread_mutex_t lock;
pthread_cond_t waitCond;
pthread_cond_t blockCond;

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// HW3: Parse the new arguments too
void getargs(int *port, int argc, char *argv[], int *queue_size, char* schedalg, int* threads)
{
    if (argc < 5) {
	    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	    exit(1);
    }
    int portNumber= atoi(argv[1]);
    int threadNumber = atoi(argv[2]);
    int queueSizeNum = atoi(argv[3]);


    if (portNumber< MIN_PNUM || portNumber > MAX_PNUM){
        fprintf(stderr, "invalid port number %s", argv[1]);
        exit(1);
    }
    *port = portNumber;

    if(threadNumber<0){
        fprintf(stderr, "invalid number of threads  %s", argv[2]);
        exit(1);
    }
    *threads = threadNumber;

    if(queueSizeNum<0){
        fprintf(stderr, "invalid queue size  %s", argv[3]);
        exit(1);
    }
    *queue_size = queueSizeNum;

    if( (strcmp(argv[4],"random") != 0) &&  (strcmp(argv[4],"block") != 0) &&
    (strcmp(argv[4],"dh") != 0) && (strcmp(argv[4],"dt") != 0)){
        fprintf(stderr, "invalid schedalg  %s", argv[4]);
        exit(1);
    }
    strcpy(schedalg, argv[4]);
}

void* threadAux(void* t){
        threads_stats thread = (threads_stats)t;
        int isVIP = (thread->id == VIP_THREAD_ID);

        while(1){
            pthread_mutex_lock(&lock);

            while((isVIP && waitingRequests_vip->size == 0) ||
                   (!isVIP && (waitingRequests_vip->size > 0 ||
                           waitingRequests_regular->size == 0))) {
                pthread_cond_wait(&waitCond, &lock);
            }

            Node request = NULL;

            if(isVIP){
                if (waitingRequests_vip->size > 0) {
                    request = pop(waitingRequests_vip);
                }
            }
            else{
                if(waitingRequests_regular->size > 0 && waitingRequests_vip->size == 0){
                    request = pop(waitingRequests_regular);
                    if(request){
                        addNode(workingRequests_regular, request);
                    }

                }

            }
            pthread_mutex_unlock(&lock);

            if(request){
                gettimeofday(&(request->pickup_time), NULL);
                timersub(&(request->pickup_time), &(request->arrival_time), &(request->dispatch_time));
                requestHandle(request->fd, request->arrival_time, request->dispatch_time, thread);
                pthread_mutex_lock(&lock);

                if(!isVIP){
                    removeByFd(workingRequests_regular, request->fd);
                }

                pthread_mutex_unlock(&lock);

                close(request->fd);
                free(request);
            }

            pthread_cond_signal(&blockCond);
            pthread_cond_signal(&waitCond);
        }
        return NULL;
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, threads, queue_size;
    struct sockaddr_in clientaddr;
    char schedalg[7];
    struct timeval currtime;

    getargs(&port, argc, argv , &queue_size, schedalg,&threads);

    if(pthread_cond_init(&waitCond, NULL) != 0){
        return 0;
    }

    if(pthread_cond_init(&blockCond, NULL) != 0){
        pthread_cond_destroy(&waitCond);
        return 0;
    }
    if(pthread_mutex_init(&lock, NULL)){
        pthread_cond_destroy(&waitCond);
        pthread_cond_destroy(&blockCond);
        return 0;
    }

    waitingRequests_regular = createQueue();
    waitingRequests_vip=createQueue();
    workingRequests_regular = createQueue();

    if(!waitingRequests_regular || !waitingRequests_vip || !workingRequests_regular){
        pthread_cond_destroy(&waitCond);
        pthread_cond_destroy(&blockCond);
        pthread_mutex_destroy(&lock);
        return 0;
    }

    pthread_t* worker_threads = malloc(threads*(sizeof(pthread_t)));
    if(!worker_threads)
    {
        pthread_cond_destroy(&waitCond);
        pthread_cond_destroy(&blockCond);
        pthread_mutex_destroy(&lock);
        destroyQueue(waitingRequests_regular);
        destroyQueue(workingRequests_regular);
        destroyQueue(waitingRequests_vip);
        return 0;
    }

    for(int i = 0; i < threads; i++)
    {
        threads_stats t = malloc(sizeof(threads_stats));
        t->id = i;
        t->stat_req = 0;
        t->dynm_req = 0;
        t->total_req =0;
        pthread_create(&worker_threads[i], NULL, threadAux, (void*)t);
    }
    ///////////////////////////NOT SURE IT WORKS CORRECTLY
    threads_stats vip_t = malloc(sizeof(threads_stats));
    vip_t->id = threads+1; ///hl bnf3??
    vip_t->stat_req = 0;
    vip_t->dynm_req = 0;
    vip_t->total_req =0;
    pthread_create(&vip_t, NULL, threadAux, (void*)vip_t);
   //////////////////////////////

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);


    }
}



    


 
