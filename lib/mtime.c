#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

#include "mtime.h"

// The expectation is that gettimeofday() uses
// the VDSO and/or the TSC (on modern x86) to
// avoid system calls.
uint64_t mtime() {
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1) {
		return 0;
	}
	return ((uint64_t)tv.tv_sec * USEC_PER_SEC) + (uint64_t)tv.tv_usec;
}

