#ifndef __REQUEST_H__

typedef struct {
	int id, count, statics, dynamics;
} thread;

void requestHandle(int fd, long arrival, long dispatch, long start, long count);

#endif
