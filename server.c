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
  int threads;
} request;

enum {FIFO, HPSC, HPDC};

sem_t sem;
sem_t sem_req;
sem_t sem_file;
request **buffer;

void getargs(int *port, int *threads, int *buffers, int *alg, int argc, char *argv[])
{
  if (argc != 3) { /* You will change 2 to 5 */
    fprintf(stderr, "Usage: %s <port> <P>\n", argv[0]);
    exit(1);
  }
  *port = atoi(argv[1]);
  *threads = atoi(argv[2]);
  *buffers = 1;
  *alg = FIFO;
}


void consumer(void * data) {
  request * req ,*reqTemp;
  int inum;
  struct timeval dispatch;

  
  while(1){
  printf("wait consumer\n");

  //request wait;
  sem_wait(&sem);
  
  //struct req setting 설정
  reqTemp = (request * )data;
  req = (request*)malloc(sizeof(request));
  req->fd = reqTemp->fd;
  req->size = reqTemp->size;
  req->start = reqTemp->start;
  req->count = reqTemp->count;
  req->arrival = reqTemp->arrival;
  sem_post(&sem_req);

  gettimeofday(&dispatch, NULL);
  req->dispatch = ((dispatch.tv_sec) * 1000 + dispatch.tv_usec/1000.0) + 0.5;

  requestHandle(req->fd, req->arrival, req->dispatch, req->start, req->count++);
  Close(req->fd);
  }
}

int main(int argc, char *argv[])
{
  int i;
  int listenfd, connfd, port, threads, buffers, alg, clientlen;
  struct sockaddr_in clientaddr;
  struct timeval arrival;
  request req;
  pthread_t * thread;
  getargs(&port, &threads, &buffers, &alg, argc, argv);

  listenfd = Open_listenfd(port);
 
  req.count = 0;
  gettimeofday(&arrival, NULL);
  req.start = ((arrival.tv_sec) * 1000 + arrival.tv_usec/1000.0) + 0.5;
 


//  thread 생성
  sem_init(&sem,0,0);
  sem_init(&sem_req,0,0);
  sem_init(&sem_file,0,1);

  for(i = 0 ; i < threads ;i++){
    thread = (pthread_t*)malloc(sizeof(pthread_t));
    pthread_create(thread,NULL,consumer,(void*)&req);
    pthread_detach(thread);
  }



   while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
    gettimeofday(&arrival, NULL);
    req.threads = threads;
    req.fd = connfd;
    req.arrival = ((arrival.tv_sec) * 1000 + arrival.tv_usec/1000.0) + 0.5;
    sem_post(&sem);
    sem_wait(&sem_req);
  }
}


    


 
