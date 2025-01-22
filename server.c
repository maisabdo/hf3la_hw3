#include "segel.h"
#include "request.h"
#include "queue.h"
#define MIN_PNUM 1024
#define MAX_PNUM 65535


Queue waitingRequests_regular;
Queue workingRequests_regular;
Queue waitingRequests_vip;
pthread_mutex_t lock;
pthread_cond_t waitCond; //empty
pthread_cond_t blockCond; //full
pthread_cond_t waitCond_vip; //vip empty
pthread_cond_t blockCond_vip; //vip working
//// 22/1 mona
int activeRequests = 0;
pthread_cond_t allIdleCond;

bool vip_working_curr;

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

void* threadAux(void* t) {
    threads_stats thread = (threads_stats)t;

    // worker thread
    if(!thread->is_vip){
        while(1){
            pthread_mutex_lock(&lock);

            while((vip_working_curr) || (waitingRequests_vip->size>0)){   ////21/1
                pthread_cond_wait(&blockCond_vip,&lock);
            }

            while (waitingRequests_regular->size == 0) {
                pthread_cond_wait(&waitCond, &lock);
            }

            Node request = pop(waitingRequests_regular);
            pthread_cond_signal(&blockCond);

            if(!request){
                pthread_mutex_unlock(&lock);
                return NULL;
            }

            if(gettimeofday(&(request->pickup_time), NULL) != 0){
                close(request->fd);
                pthread_mutex_unlock(&lock);
                return NULL;
            }

            activeRequests++; /// 22/1 mona
            timersub(&(request->pickup_time), &(request->arrival_time), &(request->dispatch_time));
            addNode(workingRequests_regular, request);
            int fd = request->fd;
            struct timeval arrival = request->arrival_time;
            struct timeval dispatch = request->dispatch_time;

            pthread_mutex_unlock(&lock);
            requestHandle(fd, arrival, dispatch, thread,waitingRequests_regular);

            pthread_mutex_lock(&lock); /// 22/1 mona
            activeRequests--; /// 22/1 mona
            if (waitingRequests_regular->size == 0 && waitingRequests_vip->size == 0 && activeRequests == 0) {
                pthread_cond_signal(&allIdleCond);
            }

            // pthread_mutex_lock(&lock); /// 22/1 mona removed this
            /////
            removeByFd(workingRequests_regular, fd);
            pthread_cond_signal(&blockCond);
            //pthread_cond_signal(&waitCond);   /////21/1
            pthread_mutex_unlock(&lock);
        }
    }
    else{ //vip thread
        while(1){
            pthread_mutex_lock(&lock);
            while(waitingRequests_vip->size==0){
                vip_working_curr=false;     //////21/1 maybe wrong
                pthread_cond_signal(&blockCond_vip);
                pthread_cond_wait(&waitCond_vip,&lock);
            }
            ///she waited again
            vip_working_curr=true;

            Node request=pop(waitingRequests_vip);
            if(!request){
                pthread_mutex_unlock(&lock);
                return NULL;
            }

            if(gettimeofday(&(request->pickup_time), NULL) != 0){
                close(request->fd);
                pthread_mutex_unlock(&lock);
                return NULL;
            }

            activeRequests++; /// 22/1 mona
            timersub(&(request->pickup_time), &(request->arrival_time), &(request->dispatch_time));
            int fd = request->fd;
            struct timeval arrival = request->arrival_time;
            struct timeval dispatch = request->dispatch_time;

            pthread_mutex_unlock(&lock);
            requestHandle(fd, arrival, dispatch, thread,waitingRequests_vip);

            pthread_mutex_lock(&lock); /// 22/1 mona
            activeRequests--; /// 22/1 mona
            if (waitingRequests_regular->size == 0 && waitingRequests_vip->size == 0 && activeRequests == 0) {
                pthread_cond_signal(&allIdleCond);
            }
            pthread_mutex_unlock(&lock);
            /////////// 22/1 mona
            close(fd);
            free(request);
            pthread_cond_signal(&blockCond_vip);   ////21/1 mutix? lazm?

            //pthread_mutex_lock(&lock);
            //removeByFd(workingRequests_regular, fd);
            //pthread_cond_signal(&blockCond);
            //pthread_cond_signal(&waitCond);   /////21/1
            //pthread_mutex_unlock(&lock);
        }
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
    if(pthread_cond_init(&waitCond_vip, NULL) != 0){
        pthread_cond_destroy(&waitCond);
        pthread_cond_destroy(&blockCond);
        return 0;
    }

    if(pthread_cond_init(&blockCond_vip, NULL) != 0){
        pthread_cond_destroy(&waitCond);
        pthread_cond_destroy(&blockCond);
        pthread_cond_destroy(&waitCond_vip);
        return 0;
    }
    if(pthread_mutex_init(&lock, NULL)){
        pthread_cond_destroy(&waitCond);
        pthread_cond_destroy(&blockCond);
        pthread_cond_destroy(&waitCond_vip);
        pthread_cond_destroy(&blockCond_vip);
        return 0;
    }
    ///// 22/1 mona
    if (pthread_cond_init(&allIdleCond, NULL)) {
        fprintf(stderr, "Failed to initialize allIdleCond\n");
        return 1;
    }
    ///////

    waitingRequests_regular = createQueue();
    waitingRequests_vip=createQueue();
    workingRequests_regular = createQueue();

    if(!waitingRequests_regular || !waitingRequests_vip || !workingRequests_regular){
        pthread_cond_destroy(&waitCond);
        pthread_cond_destroy(&blockCond);
        pthread_cond_destroy(&waitCond_vip);
        pthread_cond_destroy(&blockCond_vip);
        pthread_mutex_destroy(&lock);
        return 0;
    }

    pthread_t* worker_threads = malloc(threads*(sizeof(pthread_t)));
    if(!worker_threads)
    {
        pthread_cond_destroy(&waitCond);
        pthread_cond_destroy(&blockCond);
        pthread_cond_destroy(&waitCond_vip);
        pthread_cond_destroy(&blockCond_vip);
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
        t->is_vip=false;
        pthread_create(&worker_threads[i], NULL, threadAux, (void*)t);
    }
    ///////////////////////////NOT SURE IT WORKS CORRECTLY (first argument in pthread_create
    threads_stats vip_t = malloc(sizeof(threads_stats));
    vip_t->id = threads; ///hl bnf3??
    vip_t->stat_req = 0;
    vip_t->dynm_req = 0;
    vip_t->total_req =0;
    vip_t->is_vip=true;
    pthread_create(&worker_threads[threads], NULL, threadAux, (void*)vip_t);
   //////////////////////////////


    listenfd = Open_listenfd(port);
    while(1){
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

        if(gettimeofday(&currtime, NULL) != 0)
        {
            pthread_cond_destroy(&blockCond);
            pthread_cond_destroy(&waitCond);
            pthread_mutex_destroy(&lock);
            destroyQueue(waitingRequests_vip);
            destroyQueue(waitingRequests_regular);
            destroyQueue(workingRequests_regular);
            free(worker_threads);
            return 0;
        }

        pthread_mutex_lock(&lock);


        ///if the bufferSize+1 request is vip what sould happen in drop tail?

        if(waitingRequests_regular->size + workingRequests_regular->size + waitingRequests_vip->size >= queue_size){
            if(strcmp(schedalg, "block") == 0  || waitingRequests_regular->size == 0){
                while(waitingRequests_regular->size + workingRequests_regular->size + waitingRequests_vip->size >= queue_size)
                {
                    pthread_cond_wait(&blockCond, &lock);
                }
            }
            else if(strcmp(schedalg, "dt") == 0){
                if(getRequestMetaData(connfd)){
                    Node request = pop(waitingRequests_regular);
                    pthread_cond_signal(&blockCond); /// 21 mona
                    close(request->fd);
                }
                else{
                    Close(connfd);
                }
                pthread_mutex_unlock(&lock);
                continue;
            }
            else if(strcmp(schedalg, "dh") == 0){
                Node request = pop(waitingRequests_regular);
                pthread_cond_signal(&blockCond); /// 21 mona
                if(!request)
                {
                    close(connfd);
                    pthread_mutex_unlock(&lock);
                    continue;
                }
                close(request->fd);
                free(request);
            }
            else if(strcmp(schedalg, "random") == 0)
            {
                if(waitingRequests_regular->size == 0)
                {
                    close(connfd);
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                //int amount = (int)ceil(waitingRequests_regular->size * 0.5);
                int amount = (waitingRequests_regular->size + 1)/2;

                for(int i = 0; i < amount; i++)
                {
                    int index = rand()%(waitingRequests_regular->size);
                    removeByIndex(waitingRequests_regular, index);
                    pthread_cond_signal(&blockCond); /// 21 mona
                }
            }
            /////// 22/1 mona newwwww
            else if(strcmp(schedalg, "bf") == 0){
                if (getRequestMetaData(connfd)) {
                    while (waitingRequests_regular->size > 0 || waitingRequests_vip->size > 0 || activeRequests > 0) {
                        pthread_cond_wait(&allIdleCond, &lock);
                    }
                }
                else {
                    while (waitingRequests_regular->size > 0 || waitingRequests_vip->size > 0 || activeRequests > 0) {
                        pthread_cond_wait(&allIdleCond, &lock);
                    }

                    close(connfd);

                }
                pthread_mutex_unlock(&lock);
                continue;
            }
            ////////


        }

        int isRequestVIP = getRequestMetaData(connfd);
        Node newRequest = createNode(connfd, currtime);
        if(isRequestVIP){
            addNode(waitingRequests_vip, newRequest);
            ///should add signal so that the vip thread know that there is a vip request in the waitingRequests_vip (vip_queue->size>0)
            pthread_cond_signal(&waitCond_vip); /// 21 mona
        }
        else{
            addNode(waitingRequests_regular, newRequest);
            pthread_cond_signal(&waitCond);
        }

        pthread_mutex_unlock(&lock);
    }
    pthread_cond_destroy(&waitCond);
    pthread_cond_destroy(&blockCond);
}



    


 
