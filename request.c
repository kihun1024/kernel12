//
// request.c: Does the bulk of the work for the web server.
// 

#include "stems.h"
#include "request.h"

#define MAX_REQUEST 100000

request_struct reqList[MAX_REQUEST];
int c_static[35] = {0, };
int c_dynamic[35] = {0, };
int reqListCount =0;

void requestError(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) 
{
  char buf[MAXLINE], body[MAXBUF];

  printf("Request ERROR\n");

  // Create the body of the error message
  sprintf(body, "<html><title>Error</title>");
  sprintf(body, "%s<body bgcolor=""fffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr>My Web Server\r\n", body);

  // Write out the header information for this response
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  printf("%s", buf);

  sprintf(buf, "Content-Type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  printf("%s", buf);

  sprintf(buf, "Content-Length: %lu\r\n\r\n", (long unsigned int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  printf("%s", buf);

  // Write out the content
  Rio_writen(fd, body, strlen(body));
  printf("%s", body);

}


//
// Reads and discards everything up to an empty text line
//
void requestReadhdrs(rio_t *rp)
{
  char buf[MAXLINE];
  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
  }
  return;
}

//
// Return 1 if static (currently), 0 if dynamic content (by you)
// Calculates filename (and cgiargs, for dynamic) from uri
// Currently, it just processes static content and does not return cgiargs
// You must add codes that decides content type and operations for dynamic content
// Also, args for dynamic content should be returned
//
int parseURI(char *uri, char *filename, char *cgiargs) 
{
  char *cgi = strstr(uri, ".cgi");
  strcpy(cgiargs, "");
  
  if(cgi == NULL)
  {
//    printf("static");
    sprintf(filename, ".%s", uri);
    if (uri[strlen(uri)-1] == '/') 
      strcat(filename, "index.html");
    return 1;
  }
  else
  {
//    printf("dynamic");
    cgi = strchr(cgi, '?');
    if(cgi)
    {
      strcpy(cgiargs, cgi+1);
      *cgi = '\0';
    }
    sprintf(filename, ".%s", uri);
    return 0;
  }
}

//
// Fills in the filetype given the filename
//
void requestGetFiletype(char *filename, char *filetype)
{
  if (strstr(filename, ".html")) 
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif")) 
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".jpg")) 
    strcpy(filetype, "image/jpeg");
  else 
    strcpy(filetype, "test/plain");
}

//
// You must implement code that process dynamic request 
//
void requestServeDynamic(int fd, char *filename, char *cgiargs, long arrival, long dispatch, long start, long count, int id)
{
  struct timeval afterread;
  char buf[MAXLINE];
  int complete;
  long after_read_time;

  gettimeofday(&afterread, NULL);
  after_read_time = (afterread.tv_sec * 1000 + afterread.tv_usec/1000.0) + 0.5;
  complete = (((afterread.tv_sec) * 1000 + (afterread.tv_usec)/1000.0) + 0.5) - arrival;

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: My Web Server\r\n", buf);

  reqList[reqListCount].stat_req_arrival_count = count;
  reqList[reqListCount].stat_req_arrival_time = arrival - start;
  reqList[reqListCount].stat_req_complete_time = dispatch + complete - start;

  sprintf(buf, "%sStat-req-arrival:%ld\r\n", buf, arrival);
  sprintf(buf, "%sStat-req-end:%ld\r\n",buf,after_read_time);
  sprintf(buf, "%sStat_wait_time_D: %ld\r\n",buf,dispatch - arrival);
  sprintf(buf, "%sStat_req_arrival_count: %ld\r\n", buf, reqList[reqListCount].stat_req_arrival_count);
  sprintf(buf, "%sStat_req_arrival_time: %ld\r\n", buf, reqList[reqListCount].stat_req_arrival_time);
  sprintf(buf, "%sStat_req_complete_time: %ld\r\n", buf, reqList[reqListCount].stat_req_complete_time);
  sprintf(buf, "%sStat_req_dispatch_time: %ld\r\n",buf, dispatch - start);
  sprintf(buf, "%sStat_req_complete_count: %ld\r\n",buf, reqListCount);
  sprintf(buf, "%sStat_thread_id: %d\r\n",buf,id);
  sprintf(buf, "%sStat_thread_static: %d\r\n",buf,c_static[id]);
  sprintf(buf, "%sStat_thread_dynamic: %d\r\n",buf,++c_dynamic[id]);

  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0)
  {
    Setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, NULL, environ);
  }
  Wait(NULL);
}


void requestServeStatic(int fd, char *filename, int filesize, long arrival, long dispatch, long start, long count, int id) 
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];
  struct timeval beforeread, afterread;
  int  complete;
  long after_read_time;
  
  gettimeofday(&beforeread, NULL);

  requestGetFiletype(filename, filetype);
  srcfd = Open(filename, O_RDONLY, 0);
  // Rather than call read() to read the file into memory, 
  // which would require that we allocate a buffer, we memory-map the file
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);

  gettimeofday(&afterread, NULL);
  after_read_time = (afterread.tv_sec * 1000 + afterread.tv_usec/1000.0) + 0.5;
  //read = ((afterread.tv_sec - beforeread.tv_sec) * 1000 + (afterread.tv_usec - beforeread.tv_usec)/1000.0) + 0.5;
  complete = (((afterread.tv_sec) * 1000 + (afterread.tv_usec)/1000.0) + 0.5) - arrival;
  // put together response
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: My Web Server\r\n", buf);
 
  reqList[reqListCount].stat_req_arrival_count = count;
  reqList[reqListCount].stat_req_arrival_time = arrival - start;
  reqList[reqListCount].stat_req_complete_time = dispatch + complete - start;

  // Your statistics go here -- fill in the 0's with something useful!
  sprintf(buf, "%sStat-req-arrival:%ld\r\n", buf, arrival);
  sprintf(buf, "%sStat-req-end:%ld\r\n",buf,after_read_time);
  sprintf(buf, "%sStat_wait_time_S: %ld\r\n",buf,dispatch - arrival);
  sprintf(buf, "%sStat_req_arrival_count: %ld\r\n", buf, reqList[reqListCount].stat_req_arrival_count);
  sprintf(buf, "%sStat_req_arrival_time: %ld\r\n", buf, reqList[reqListCount].stat_req_arrival_time);
  sprintf(buf, "%sStat_req_complete_time: %ld\r\n", buf, reqList[reqListCount].stat_req_complete_time);
  sprintf(buf, "%sStat_req_dispatch_time: %ld\r\n",buf, dispatch - start);
  // req_complete_count? req_arrival_count?
  sprintf(buf, "%sStat_req_complete_count: %ld\r\n",buf,reqListCount);
  sprintf(buf, "%sStat_thread_id: %d\r\n",buf,id);
  sprintf(buf, "%sStat_thread_static: %d\r\n",buf,++c_static[id]);
  sprintf(buf, "%sStat_thread_dynamic: %d\r\n",buf,c_dynamic[id]);

  reqListCount++;


  // Add additional statistic information here like above
  // ...
  //

  sprintf(buf, "%sContent-Length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-Type: %s\r\n\r\n", buf, filetype);

  Rio_writen(fd, buf, strlen(buf));

  //  Writes out to the client socket the memory-mapped file 
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
}

// handle a request
void requestHandle(int fd, long arrival, long dispatch, long start, long count, int id)
{

  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t * rio;
  rio = (rio_t*)malloc(sizeof(rio_t));
  Rio_readinitb(rio, fd);
  Rio_readlineb(rio, buf, MAXLINE);
//   printf("handle in... %s\n ",buf);
 sscanf(buf, "%s %s %s", method, uri, version);

  printf("%ld %s %s %s\n", count + 1, method, uri, version);

  if (strcasecmp(method, "GET")) {
    requestError(fd, method, "501", "Not Implemented", "My Server does not implement this method");
    return;
  }
  requestReadhdrs(rio);

  is_static = parseURI(uri, filename, cgiargs);
  if (stat(filename, &sbuf) < 0) {
    requestError(fd, filename, "404", "Not found", "My Server could not find this file");
    return;
  }

  if (is_static) {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      requestError(fd, filename, "403", "Forbidden", "My Server could not read this file");
      return;
    }
    requestServeStatic(fd, filename, sbuf.st_size, arrival, dispatch, start, count, id);
   } else {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      requestError(fd, filename, "403", "Forbidden", "My Server could not run this CGI program");
      return;
    }
    requestServeDynamic(fd, filename, cgiargs, arrival, dispatch, start, count, id);
  }
}
