// Copyright (C) 2012 The SBCELT Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE-file.

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/futex.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <linux/futex.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "celt.h"
#include "../sbcelt.h"

#ifdef DEBUG
# define debugf(fmt, ...) \
	do { \
		fprintf(stderr, "libsbcelt:%s():%u: " fmt "\n", \
		    __FILE__, __LINE__, ## __VA_ARGS__); \
		fflush(stderr); \
	} while (0)
#else
# define debugf(s, ...) do{} while (0)
#endif

struct CELTMode {
};

struct CELT_Encoder {
};

struct CELTDecoder {
};

static struct SBCELTWorkPage *workpage = NULL;
static struct SBCELTDecoderPage *decpage = NULL;
static int running = 0;
static int lastslot = 0;
static uint64_t lastrun = 3000; // 3 ms
static pthread_t monitor;

#define PAGE_SIZE  4096

static int futex_wake(int *futex) {
	return syscall(SYS_futex, futex, FUTEX_WAKE, 1, NULL, NULL, 0);
}

static int futex_wait(int *futex, int val, struct timespec *ts) {
	return syscall(SYS_futex, futex, FUTEX_WAIT, val, ts, NULL, 0);
}

#define USEC_PER_SEC  1000000
#define NSEC_PER_USEC 1000

// Monotonic microsecond timestamp generator.
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

void *SBCELT_HelperMonitor(void *udata) {
	uint64_t lastdead = 0;
	(void) udata;

	char *helper = getenv("SBCELT_HELPER_BINARY");
	if (helper == NULL) {
		helper = "/usr/bin/sbcelt-helper";
	}

	while (1) {
		uint64_t now = mtime();
		uint64_t elapsed = now - lastdead;
		lastdead = now;

		// Throttle child deaths to around 1 per sec.
		if (elapsed < 1*USEC_PER_SEC) {
			usleep(1*USEC_PER_SEC);
		}

		debugf("restarted sbcelt-helper; %lu usec since last death", elapsed);

		pid_t child = fork();
		if (child == -1) {
			// We're memory constrained. Wait and try again...
			usleep(5*USEC_PER_SEC);
			continue;
		} else if (child == 0) {
			char *const argv[] = {
				helper,
				NULL,
			};
			execv(argv[0], argv);
			exit(1);
		}

		int status;
		if (waitpid(child, &status, 0) == 0) {
			if (WIFEXITED(status)) {
				debugf("sbcelt-helper died with exit status: %i", WEXITSTATUS(status));
			} else if (WIFSIGNALED(status)) {
				debugf("sbcelt-helper died with signal: %i", WTERMSIG(status));
			}
		} else if (errno == EINVAL) {
			fprintf(stderr, "libsbcelt: waitpid() failed with EINVAL.\n");
			fflush(stderr);
			exit(1);
		}
	}
}

int SBCELT_Init() {
	char shmfn[50];
	if (snprintf(&shmfn[0], 50, "/sbcelt-%lu", (unsigned long) getpid()) < 0) {
		return -1;
	}

	shm_unlink(&shmfn[0]);
	int fd = shm_open(&shmfn[0], O_CREAT|O_RDWR, 0600);
	if (fd == -1) {
		return -1;
	} else {
		if (ftruncate(fd, SBCELT_PAGES*PAGE_SIZE) == -1) {
			debugf("unable to truncate");
			return -1;
		}

		void *addr = mmap(NULL, SBCELT_PAGES*PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fd, 0);
		memset(addr, 0, SBCELT_PAGES*PAGE_SIZE);

		workpage = addr;
		decpage = addr+PAGE_SIZE;

		workpage->busywait = sysconf(_SC_NPROCESSORS_ONLN) > 0;

		int i;
		for (i = 0; i < SBCELT_SLOTS; i++) {
			decpage->slots[i].available = 1;
		}

		pthread_t tmp;
		if (pthread_create(&monitor, NULL, SBCELT_HelperMonitor, NULL) != 0)
			return -1;
	}

	return 0;
}

CELTMode *SBCELT_FUNC(celt_mode_create)(celt_int32 Fs, int frame_size, int *error) {
	return (CELTMode *) 0x1;
}

void SBCELT_FUNC(celt_mode_destroy)(CELTMode *mode) {
}

int SBCELT_FUNC(celt_mode_info)(const CELTMode *mode, int request, celt_int32 *value) {
	if (request == CELT_GET_BITSTREAM_VERSION) {
		*value = 0x8000000b;
		return CELT_OK;
	}
	return CELT_INTERNAL_ERROR;
}

CELTEncoder *SBCELT_FUNC(celt_encoder_create)(const CELTMode *mode, int channels, int *error) {
	return NULL;
}

void SBCELT_FUNC(celt_encoder_destroy)(CELTEncoder *st) {
}

int SBCELT_FUNC(celt_encode_float)(CELTEncoder *st, const float *pcm, float *optional_synthesis,
                      unsigned char *compressed, int nbCompressedBytes) {
	return CELT_INTERNAL_ERROR;
}

int SBCELT_FUNC(celt_encode)(CELTEncoder *st, const celt_int16 *pcm, celt_int16 *optional_synthesis,
                unsigned char *compressed, int nbCompressedBytes) {
}

int SBCELT_FUNC(celt_encoder_ctl)(CELTEncoder * st, int request, ...) {
	return CELT_INTERNAL_ERROR;
}

CELTDecoder *SBCELT_FUNC(celt_decoder_create)(const CELTMode *mode, int channels, int *error) {
	if (!running) {
		SBCELT_Init();
		running = 1;
	}

	// Find a free slot.
	int i, slot = -1;
	for (i = 0; i < SBCELT_SLOTS; i++) {
		int idx = (lastslot+i) % SBCELT_SLOTS;
		if (decpage->slots[idx].available) {
			decpage->slots[idx].available = 0;
			lastslot = idx;
			slot = idx;
			break;
		}
	}
	if (slot == -1) {
		debugf("decoder_create: no free slots");
		return NULL;
	}

	debugf("decoder_create: slot=%i", slot);

	return (CELTDecoder *)((uintptr_t)slot);
}

void SBCELT_FUNC(celt_decoder_destroy)(CELTDecoder *st) {
	int slot = (int)((uintptr_t)st);
	decpage->slots[slot].available = 1;

	debugf("decoder_destroy: slot=%i", slot);
}

int SBCELT_FUNC(celt_decode_float)(CELTDecoder *st, const unsigned char *data, int len, float *pcm) {
	int slot = (int)((uintptr_t)st);

	debugf("decode_float; len=%i", len);

	workpage->slot = slot;
	memcpy(&workpage->encbuf[0], data, len);
	workpage->len = len;

	uint64_t begin = mtime();

	// Wake up the helper, if necessary...
	workpage->ready = 0;
	futex_wake(&workpage->ready);

	int bad = 0;
	if (workpage->busywait) {
		while (!workpage->ready) {
			uint64_t elapsed = mtime() - begin;
			if (elapsed > lastrun*2) {
				bad = 1;
				break;
			}
		}
	} else {
		do {
			struct timespec ts = { 0, (lastrun*2) * NSEC_PER_USEC };
			int err = futex_wait(&workpage->ready, 0, &ts);
			if (err == 0 || err == EWOULDBLOCK) {
				break;
			} else if (err == ETIMEDOUT) {
				bad = 1;
				break;
			}
		} while (!workpage->ready);
	}

	if (!bad) {
#ifdef DYNAMIC_TIMEOUT
		lastrun = mtime() - begin;
#endif
		debugf("spent %lu usecs in decode\n", lastrun);
		memcpy(pcm, workpage->decbuf, sizeof(float)*480);
	} else {
#ifdef DYNAMIC_TIMEOUT
		lastrun = 3000;
#endif
		memset(pcm, 0, sizeof(float)*480);
	}

	return CELT_OK;
}

int SBCELT_FUNC(celt_decode)(CELTDecoder *st, const unsigned char *data, int len, celt_int16 *pcm) {
	return CELT_INTERNAL_ERROR;
}

int SBCELT_FUNC(celt_decoder_ctl)(CELTDecoder * st, int request, ...) {
	return CELT_INTERNAL_ERROR;
}

const char *SBCELT_FUNC(celt_strerror)(int error) {
	return "celt: unknown error";
}
