#include "stems.h"
#include "request.h"
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
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
  int priority;
} request;

enum {FIFO, HPSC, HPDC};

sem_t empty;
sem_t full;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
request **buffer;
int heapSize;
int bufferSize;

void getargs(int *port, int *threads, int *buffers, int *alg,int argc, char *argv[])
{
  if (argc != 5) {
    fprintf(stderr, "Usage: %s <port> <P> <Q> <Sched-alg>\n", argv[0]);
    exit(1);
  }
  *port = atoi(argv[1]);
  *threads = atoi(argv[2]);
  *buffers = atoi(argv[3]);
  bufferSize = *buffers;
  if(strcmp(argv[4], "FIFO") ==0){
  	*alg= FIFO;
  }else if(strcmp(argv[4],"HPSC") == 0){
  	*alg = HPSC;
  }else if(strcmp(argv[4],"HPDC") == 0){
  	*alg = HPDC;
  }
}

void enqueue(request * req , int alg){
	int i;
	char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
	
	recv(req->fd,buf,MAXLINE,MSG_PEEK);
	sscanf(buf,"%s %s %s",method,uri,version);

	req->priority = (int)req->count;
	if(alg == HPSC){
		if(strstr(uri,".cgi") != NULL)
			req->priority += 10000;
	}else if(alg == HPDC){
		if(strstr(uri,".cgi") == NULL)
			req->priority += 10000;
	}
	i = ++heapSize;
	while ((i != 1) && req->priority < buffer[i / 2]->priority) {
		buffer[i] = buffer[i / 2];
		i = i / 2;
	}
	buffer[i] = req;
}

request dequeue(){
	request * req , * reqTemp;
	int parent, child;
	req = buffer[1];
	reqTemp = buffer[heapSize--];
		
	parent = 1;
	child = 2;

	while(child <= heapSize)
	{
		if((child < heapSize) && (buffer[child]->priority >buffer[child+1]->priority))
			child++;
		if(reqTemp->priority <= buffer[child]->priority)
			break;
		
		buffer[parent] = buffer[child];
		parent = child;
		child *=2;
	}
	buffer[parent] = reqTemp;
	return *req;
}
void *consumer(void * data) {
  request req ;
  struct timeval dispatch;
  while(1){
  //request wait;
  sem_wait(&full);
  
  pthread_mutex_lock(&mutex);
  req = dequeue();
  pthread_mutex_unlock(&mutex);
  sem_post(&empty);

  gettimeofday(&dispatch, NULL);
  req.dispatch = ((dispatch.tv_sec) * 1000 + dispatch.tv_usec/1000.0) + 0.5;	// dispatch time Setting

  requestHandle(req.fd, req.arrival, req.dispatch, req.start, req.count, (int)data);
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
  request * req;
  heapSize = 0;

  getargs(&port, &threads, &buffers, &alg, argc, argv);
  
  buffer = (request**)malloc(sizeof(request*)*(buffers+1));
  
  //  thread 생성
  sem_init(&empty,0,buffers);
  sem_init(&full,0,0);

  for(i = 0 ; i < threads ;i++){
    thread = (pthread_t*)malloc(sizeof(pthread_t));
    pthread_create(thread,NULL,&consumer,(void *)i);
    pthread_detach(*thread);
  }


  listenfd = Open_listenfd(port);
 
  gettimeofday(&arrival, NULL);
  start = ((arrival.tv_sec) * 1000 + arrival.tv_usec/1000.0) + 0.5;	//server start time setting
   while (1) {
    clientlen = sizeof(clientaddr);
    sem_wait(&empty);
    connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
 
    gettimeofday(&arrival, NULL);
    pthread_mutex_lock(&mutex);
    req = (request*)malloc(sizeof(request));
    req->start = start; 
    req->fd = connfd;
    req->arrival = ((arrival.tv_sec) * 1000 + arrival.tv_usec/1000.0) + 0.5;	//request time setting
    req->count = count++;
    enqueue(req,alg);
    pthread_mutex_unlock(&mutex);

   sem_post(&full);
  }
}