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
#include "pthread.h"
#include "semaphore.h"
#include "string.h"

//barrier 사용하기위한 전역변수
pthread_barrier_t barrier;

typedef struct data_struct{
	char * host;
	int port;
	char *filename;
	int m;
}data_struct;


//FIFO 에서 사용하는 세마폴 변수
sem_t sem;

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
	int res;
	int clientfd;
	int i;
	data_struct *ds = (data_struct*)data;

	for(i=0; i<ds->m; i++)
	{
		//barrier_wait 스레드 다 생성 될때까지 대기
		res = pthread_barrier_wait(&barrier);

		clientfd = Open_clientfd(ds->host,ds->port);
		clientSend(clientfd,ds->filename);
		clientPrint(clientfd);
		Close(clientfd);
	}
}

/*CONCUR 방법 사용 함수 N개 thread 생성, 동시에 같은 파일에 대해 하나씩 요청 보냄*/
void concur(char * filename, int threadN, int forM, int port, char * host){
	int i;
	void *status;

	pthread_t threads[threadN];
	//barrier init  함수
	pthread_barrier_init(&barrier,NULL,threadN);

	data_struct ds;
	ds.filename = filename;
	ds.port = port;
	ds.host =host;
	ds.m = forM;

	for(i = 0 ; i < threadN; i++){
		pthread_create(&threads[i],NULL,concur_thread,(void*)&ds);
	}


	for(i =0 ; i <threadN ;i++){
		pthread_join(threads[i],&status);
	}
}

/* fifo 에서 사용하는 스레드 함수*/
void * fifo_thread(void *data){
	int clientfd;
	int i;
	data_struct * ds = (data_struct*)data;

	for(i=0; i<ds->m; i++)
	{
		//세마폴 사용 부분 fd 생성 및 요청 부분
		sem_wait(&sem);
		clientfd = Open_clientfd(ds->host,ds->port);
		clientSend(clientfd,ds->filename);
		sem_post(&sem);

		// 요청에의한 응답부분 
		clientPrint(clientfd);
		Close(clientfd);
	}
}

/* FIFO 사용 함수 */
void fifo(char * filename,int threadN, int forM, int port, char *host){
	pthread_t threads[threadN];
	int i;
	void * status;
	data_struct  ds;
	ds.filename = filename;
	ds.port = port;
	ds.host = host;
	ds.m = forM;

	//세마폴 init 함수
	sem_init(&sem,0,1);

	for(i = 0; i < threadN; i++){
		pthread_create(&threads[i],NULL,fifo_thread,(void*)&ds);
	}


	for(i = 0 ; i < threadN; i++){
		pthread_join(threads[i],&status);
	}
}


/* random 에서 사용하는 스레드 함수*/
void * random_thread(void *data){
	int clientfd;
	int i;
	data_struct * ds = (data_struct*)data;

	for(i=0; i<ds->m; i++)
	{
		clientfd = Open_clientfd(ds->host,ds->port);
		clientSend(clientfd,ds->filename);
		clientPrint(clientfd);
		sleep(1);
		Close(clientfd);
	}
}

/* RANDOM 사용 함수 */
void randomf(char * filename,int threadN, int forM, int port, char *host){
	pthread_t threads[threadN];
	int i;
	void * status;
	data_struct  ds;
	ds.filename = filename;
	ds.port = port;
	ds.host = host;
	ds.m = forM;

	for(i = 0; i < threadN; i++){
		pthread_create(&threads[i],NULL,random_thread,(void*)&ds);
	}


	for(i = 0 ; i < threadN; i++){
		pthread_join(threads[i],&status);
	}
}


int main(int argc, char *argv[])
{
  char *host, *filename;
  int port;
  int clientfd;
  int threadN, forM;
  char * schedalg;


  if (argc != 7) {
    fprintf(stderr, "Usage: %s <host> <port> <N> <M> <schedalg> <filename>\n", argv[0]);
    exit(1);
  }

  host = argv[1];
  port = atoi(argv[2]);
  threadN = atoi(argv[3]);
  forM = atoi(argv[4]);
  schedalg = argv[5];
  filename = argv[6];
  printf("schedalg : %s\n",schedalg);
  if(!strcmp(schedalg, "CONCUR") ){concur(filename,threadN,forM,port,host);}
  else if(!strcmp(schedalg,"FIFO")){fifo(filename,threadN,forM,port,host);}
  else if(!strcmp(schedalg,"RANDOM")){randomf(filename,threadN,forM,port,host);}
 

  /* Open a single connection to the specified host and port */
  /*
  clientfd = Open_clientfd(host, port);
  
  clientSend(clientfd, filename);
  clientPrint(clientfd);
    
  Close(clientfd);
  */
  exit(0);
}
