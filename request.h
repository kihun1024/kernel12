#ifndef __REQUEST_H__

typedef struct {
	int id, count, statics, dynamics;
} thread;

typedef struct {
	long stat_req_arrival_time;
	long stat_req_arrival_count;
	long stat_req_complete_time;
	double throughput;
}request_struct;

void requestHandle(int fd, long arrival, long dispatch, long start, long count);

#endif
