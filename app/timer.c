#include <sys/time.h>

#include "timer.h"

long start_us() {
	struct timeval te;
	gettimeofday(&te, 0);
	return te.tv_usec;
}

long us_since(long us) {
	long newtime;
	newtime = start_us();
	return newtime - us;
}

void print_time_since(char* label, long us) {
	long dur;
	dur = us_since(us);
	if (dur < 0) {
		printf("%s: us rollover, bleh\n", label);
	} else {
		printf("%s: timed %d us\n", label, dur);
	}
}