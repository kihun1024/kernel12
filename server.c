#include "stems.h"
#include "request.h"

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

request **buffer;

void getargs(int *port, int *threads, int *buffers, int *alg, int argc, char *argv[])
{
  if (argc != 2) { /* You will change 2 to 5 */
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }
  *port = atoi(argv[1]);
  *threads = 1;
  *buffers = 1;
  *alg = FIFO;
}

void consumer(request *req) {
  struct timeval dispatch;

  gettimeofday(&dispatch, NULL);
  req->dispatch = ((dispatch.tv_sec) * 1000 + dispatch.tv_usec/1000.0) + 0.5;
  requestHandle(req->fd, req->arrival, req->dispatch, req->start, req->count++);
  Close(req->fd);
}

int main(int argc, char *argv[])
{
  int listenfd, connfd, port, threads, buffers, alg, clientlen;
  struct sockaddr_in clientaddr;
  struct timeval arrival;
  request req;

  getargs(&port, &threads, &buffers, &alg, argc, argv);

  listenfd = Open_listenfd(port);
  req.count = 0;
  gettimeofday(&arrival, NULL);
  req.start = ((arrival.tv_sec) * 1000 + arrival.tv_usec/1000.0) + 0.5;
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
    gettimeofday(&arrival, NULL);

    req.fd = connfd;
    req.arrival = ((arrival.tv_sec) * 1000 + arrival.tv_usec/1000.0) + 0.5;
    consumer(&req);
  }
}


    


 
