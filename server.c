#include "stems.h"
#include "request.h"
#include <pthread.h>
#include <semaphore.h>
// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

typedef struct {
  int fd;
  long size, arrival, dispatch;
  long start, count;
} request;

enum {FIFO, HPSC, HPDC};

sem_t empty;
sem_t full;
request **buffer;


void getargs(int *port, int *threads, int *buffers, int *alg, int argc, char *argv[])
{
  if (argc != 5) { /* You will change 2 to 5 */
    fprintf(stderr, "Usage: %s <port> <P>\n", argv[0]);
    exit(1);
  }
  *port = atoi(argv[1]);
  *threads = atoi(argv[2]);
  *buffers = atoi(argv[3]);
  *alg = FIFO;
}


void *consumer(void * data) {
  request req ;
  struct timeval dispatch;
  while(1){
  //request wait;
  sem_wait(&full);
  req.fd = buffer[0]->fd;
  req.count = buffer[0]->count;
  req.arrival = buffer[0]->arrival;
  sem_post(&empty);

  req.start = buffer[0]->start;

  gettimeofday(&dispatch, NULL);
  req.dispatch = ((dispatch.tv_sec) * 1000 + dispatch.tv_usec/1000.0) + 0.5;	// dispatch time Setting

  requestHandle(req.fd, req.arrival, req.dispatch, req.start, req.count, data);
  Close(req.fd);
  }
}

int main(int argc, char *argv[])
{
  int i;
  int listenfd, connfd, port, threads, buffers, alg, clientlen;
  struct sockaddr_in clientaddr;
  struct timeval arrival;
  long count = 0,start;
  pthread_t * thread;
  getargs(&port, &threads, &buffers, &alg, argc, argv);
  
  buffer = (request**)malloc(sizeof(request*)*buffers);
  for(i = 0 ; i < buffers; i++){
    buffer[i] = (request*)malloc(sizeof(request));
  }

  //  thread 생성
  sem_init(&empty,0,1);
  sem_init(&full,0,0);

  for(i = 0 ; i < threads ;i++){
    thread = (pthread_t*)malloc(sizeof(pthread_t));
    pthread_create(thread,NULL,&consumer,i);
    pthread_detach(*thread);
  }


  listenfd = Open_listenfd(port);
  gettimeofday(&arrival, NULL);
  start = ((arrival.tv_sec) * 1000 + arrival.tv_usec/1000.0) + 0.5;	//server start time setting
  buffer[0]->start = start; 
   while (1) {
    clientlen = sizeof(clientaddr);
    sem_wait(&empty);
    connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
 
    gettimeofday(&arrival, NULL);
    buffer[0]->fd = connfd;
    buffer[0]->arrival = ((arrival.tv_sec) * 1000 + arrival.tv_usec/1000.0) + 0.5;	//request time setting
    buffer[0]->count = count++;
   sem_post(&full);
  }
}


    


 
