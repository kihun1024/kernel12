/*
 * client.c: A very, very primitive HTTP client.
 * 
 * To run, try: 
 *      client pintos.kumoh.ac.kr 80 /~choety/
 *
 * Sends one HTTP request to the specified HTTP server.
 * Prints out the HTTP response.
 *
 * For testing your server, you will want to modify this client.  
 *
 * When we test your server, we will be using modifications to this client.
 *
 */

#include "stems.h"
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h> 
//barrier 사용하기위한 전역변수
pthread_barrier_t barrier;
sem_t sem_time;
long  last;
int tcount, modcount = 1;
typedef struct data_struct{
	char * host;
	int port;
	char *filename[2];
	int m;
	int iNumber;
	int n;
}data_struct;


//FIFO 에서 사용하는 세마폴 변수
sem_t sem;
sem_t * semList;

/*
 * Send an HTTP request for the specified file 
 */
void clientSend(int fd, char *filename)
{
  char buf[MAXLINE];
  char hostname[MAXLINE];

  Gethostname(hostname, MAXLINE);

  /* Form and send the HTTP request */
  sprintf(buf, "GET %s HTTP/1.1\n", filename);
  sprintf(buf, "%shost: %s\n\r\n", buf, hostname);
  Rio_writen(fd, buf, strlen(buf));
}
  
/*
 * Read the HTTP response and print it out
 */
void clientPrint(int fd)
{
  rio_t rio;
  char buf[MAXBUF];  
  int length = 0;
  int n;
  
  Rio_readinitb(&rio, fd);

  /* Read and display the HTTP Header */
  n = Rio_readlineb(&rio, buf, MAXBUF);
  while (strcmp(buf, "\r\n") && (n > 0)) {
    printf("Header: %s", buf);
    n = Rio_readlineb(&rio, buf, MAXBUF);

    /* If you want to look for certain HTTP tags... */
    if (sscanf(buf, "Content-Length: %d ", &length) == 1) {
      printf("Length = %d\n", length);
    }
  }

  /* Read and display the HTTP Body */
  n = Rio_readlineb(&rio, buf, MAXBUF);
  while (n > 0) {
    printf("%s", buf);
    n = Rio_readlineb(&rio, buf, MAXBUF);
  }
}



/* CONCUR 방법 스레드 함수*/
void * concur_thread(void * data){
	struct timeval start , end;
	long startTime, endTime;
	int res;
	int clientfd;
	int i;
	data_struct *ds = (data_struct*)data;

	for(i=0; i<ds->m; i++)
	{
		//barrier_wait 스레드 다 생성 될때까지 대기
		res = pthread_barrier_wait(&barrier);
//		printf("forM = %d threadN  = %d \n",i , pthread_self());
		clientfd = Open_clientfd(ds->host,ds->port);
		gettimeofday(&start,NULL);
		clientSend(clientfd,ds->filename[tcount++%modcount]);
		clientPrint(clientfd);
		gettimeofday(&end,NULL);
		Close(clientfd);
		startTime = ((start.tv_sec)*1000 + start.tv_usec/1000.0) +0.5;
		endTime = ((end.tv_sec)*1000+end.tv_usec/1000.0) +0.5;
	
		sem_wait(&sem_time);
		last += endTime - startTime;
		sem_post(&sem_time);
	}
}

/* fifo 에서 사용하는 스레드 함수*/
void * fifo_thread(void *data){
	int clientfd;
	long startTime, endTime;
	struct timeval start , end;
	int i;
	data_struct * ds = (data_struct*)data;
	for(i = 0 ; i< ds->m ; i++){
		//세마폴 사용 부분 fd 생성 및 요청 부분
		sem_wait(&semList[ds->iNumber]);
//		printf(" ForM = %d threadN = %d\n",i,ds->iNumber);
		clientfd = Open_clientfd(ds->host,ds->port);
		gettimeofday(&start,NULL);
		clientSend(clientfd,ds->filename[tcount++%modcount]);
		sem_post(&semList[((ds->iNumber)+1)%ds->n]);
		

		
		// 요청에의한 응답부분 
		clientPrint(clientfd);
		gettimeofday(&end,NULL);
		Close(clientfd);


		startTime = ((start.tv_sec)*1000 + start.tv_usec/1000.0) +0.5;
		endTime = ((end.tv_sec)*1000+end.tv_usec/1000.0) +0.5;
	
		sem_wait(&sem_time);
		last += endTime - startTime;
		sem_post(&sem_time);

		pthread_barrier_wait(&barrier);


	}
}

/* random 에서 사용하는 스레드 함수*/
void * random_thread(void *data){
	int clientfd;
	long startTime, endTime;
	struct timeval start,end;
	int i;
	data_struct * ds = (data_struct*)data;
	
	for(i=0; i<ds->m; i++)
	{
		clientfd = Open_clientfd(ds->host,ds->port);
		gettimeofday(&start,NULL);
		clientSend(clientfd,ds->filename[tcount++%modcount]);
		clientPrint(clientfd);
		gettimeofday(&end,NULL);
		Close(clientfd);


		startTime = ((start.tv_sec)*1000 + start.tv_usec/1000.0) +0.5;
		endTime = ((end.tv_sec)*1000+end.tv_usec/1000.0) +0.5;
	
		sem_wait(&sem_time);
		last += endTime - startTime;
		sem_post(&sem_time);
		sleep((rand() % 5) + 1);
	}
}



/* RANDOM 사용 함수 */
void client(char *host, int port, int threadN, int forM, char * sched, char * filename[2]){
	pthread_t threads[threadN];
	int i;
	void * status;
	data_struct  ds;
	ds.filename[0] = filename[0];
	ds.filename[1] = filename[1];
	ds.port = port;
	ds.host = host;
	ds.m = forM;
	data_struct *dataList;

	if(!strcmp(sched, "CONCUR")){
		pthread_barrier_init(&barrier,NULL,threadN);

		for(i = 0 ; i < threadN; i++){
			pthread_create(&threads[i],NULL,concur_thread,(void*)&ds);
		}
	}
	else if(!strcmp(sched, "FIFO")){
		//sem_init(&sem,0,1); //세마폴 init 함수
		pthread_barrier_init(&barrier,NULL,threadN);
		semList = (sem_t*)malloc(sizeof(sem_t)*threadN);
		sem_init(&semList[0],0,1);
		
		dataList = (data_struct*)malloc(sizeof(data_struct)*threadN);
		for(i = 1 ; i < threadN ; i++){
			sem_init(&semList[i],0,0);
		}

		for(i = 0; i < threadN; i++){
			dataList[i].filename[0] = filename[0];
			dataList[i].filename[1] = filename[1];
			dataList[i].port = port;
			dataList[i].host = host;
			dataList[i].m = forM;
			dataList[i].iNumber = i;
			dataList[i].n = threadN;
			pthread_create(&threads[i],NULL,fifo_thread,(void*)&dataList[i]);
		}
	}
	else if(!strcmp(sched, "RANDOM")){
		for(i = 0; i < threadN; i++){
			pthread_create(&threads[i],NULL,random_thread,(void*)&ds);
		}
	}

	for(i = 0 ; i < threadN; i++){
		pthread_join(threads[i],&status);
	}
}

int main(int argc, char *argv[])
{
  char *host, *filename[2];
  int port;
  int threadN, forM;
  char * schedalg;


  if (argc != 7 && argc != 8) {
    fprintf(stderr, "Usage: %s [host] [portnum] [N] [M] [schedalg] [filename1] <filename2>\n", argv[0]);
    exit(1);
  }

  host = argv[1];
  port = atoi(argv[2]);
  threadN = atoi(argv[3]);
  forM = atoi(argv[4]);
  schedalg = argv[5];
  filename[0] = argv[6];
  if (argc == 8)
  {
    modcount = 2;
    filename[1] = argv[7];
  }
  sem_init(&sem_time, 0, 1);
  client(host, port, threadN, forM, schedalg, filename);


  printf("first : %ld   , last : %ld \n",first,last);
  exit(0);
}
