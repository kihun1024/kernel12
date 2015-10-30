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

// barrier 사용 하기위한 전역변수
pthread_barrier_t barrier;
typedef struct barrier_struct{
	char * host;
	int port;
	char * fileName;
}barrier_struct;

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

/* CONCUR 방법 스레드 함수 */
void * concur_thread(void * data){
	int res;
	int clientfd;
	barrier_struct *bs = (barrier_struct*)data;

	//barrier_wait 스레드 다 생성될때까지 기다림
	res = pthread_barrier_wait(&barrier);

	if (res == PTHREAD_BARRIER_SERIAL_THREAD){
		printf("execute concur_thread \n");
		clientfd = Open_clientfd(bs->host, bs->port);
		clientSend(clientfd, bs->fileName);
		clientPrint(clientfd);
		Close(clientfd);
	}
	else if (res != 0){
		printf("pthread barrier wait error occured\n");
	}
	else{
		printf("none serial thread release\n");
	}
}

/*CONCUR 방법 사용 함수 N개 스레드 생성 , 동시에 같은 파일에 대해 하나씩 요청 보냄 */
void concur(char * fileName, int threadN, int forM, int port, char*host){
	int i;
	void *status;
	pthread_t threads[threadN];
	// barrier init 함수 
	pthread_barrier_init(&barrier, NULL, threadN);

	barrier_struct bs;
	bs.fileName = fileName;
	bs.port = port;
	bs.host = host;

	for (i = 0; i <threadN; i++){
		pthread_create(&threads[i], NULL, concur_thread, (void*)&bs);
	}

	for (i = 0; i < threadN; i++){
		pthread_join(threads[i], &status);
	}

}

int main(int argc, char *argv[])
{
	char *host, *filename;
	int port;
	int clientfd;
	int threadN;	//스레드 개수 [N] 값 대입
	int forM; 	// 요청 반복 개수 [M] 값 대입
	char * schedalg; // 스케줄링 알고리즘 방법 [schedalg] 대입
	/* client [host] [portNum] [N] [M] [schedalg] [fileName]  변경 부분*/
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

	//concur test
	concur(filename, threadN, forM, port, host);

	/* Open a single connection to the specified host and port */
	clientfd = Open_clientfd(host, port);

	clientSend(clientfd, filename);
	clientPrint(clientfd);

	Close(clientfd);

	exit(0);
}
