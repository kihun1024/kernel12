//
// request.c: Does the bulk of the work for the web server.
// 

#include "stems.h"
#include "request.h"

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
  strcpy(cgiargs, "");
  sprintf(filename, ".%s", uri);
  if (uri[strlen(uri)-1] == '/') 
    strcat(filename, "index.html");
  return 1;
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
void requestServeDynamic(int fd, char *filename, char *cgiargs, long arrival, long dispatch)
{
  //
  // ...
  //
}


void requestServeStatic(int fd, char *filename, int filesize, long arrival, long dispatch) 
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];
  struct timeval beforeread, afterread;
  int read, complete;

  gettimeofday(&beforeread, NULL);

  requestGetFiletype(filename, filetype);

  srcfd = Open(filename, O_RDONLY, 0);

  // Rather than call read() to read the file into memory, 
  // which would require that we allocate a buffer, we memory-map the file
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);

  gettimeofday(&afterread, NULL);

  read = ((afterread.tv_sec - beforeread.tv_sec) * 1000 + (afterread.tv_usec - beforeread.tv_usec)/1000.0) + 0.5;
  complete = (((afterread.tv_sec) * 1000 + (afterread.tv_usec)/1000.0) + 0.5) - arrival;

  // put together response
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: My Web Server\r\n", buf);

  // Your statistics go here -- fill in the 0's with something useful!
  sprintf(buf, "%s Stat-req-arrival: %ld\r\n", buf, arrival);
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
void requestHandle(int fd, long arrival, long dispatch)
{

  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  sscanf(buf, "%s %s %s", method, uri, version);

  printf("%s %s %s\n", method, uri, version);

  if (strcasecmp(method, "GET")) {
    requestError(fd, method, "501", "Not Implemented", "My Server does not implement this method");
    return;
  }
  requestReadhdrs(&rio);

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
    requestServeStatic(fd, filename, sbuf.st_size, arrival, dispatch);
  } else {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      requestError(fd, filename, "403", "Forbidden", "My Server could not run this CGI program");
      return;
    }
    requestServeDynamic(fd, filename, cgiargs, arrival, dispatch);
  }
}


