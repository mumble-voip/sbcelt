// Copyright (C) 2012 The SBCELT Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE-file.

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "celt.h"
#include "../sbcelt-internal.h"
#include "../sbcelt.h"

#include "futex.h"
#include "mtime.h"
#include "debug.h"

static struct SBCELTWorkPage *workpage = NULL;
static struct SBCELTDecoderPage *decpage = NULL;
static int running = 0;
static int lastslot = 0;
static uint64_t lastrun = 3000; // 3 ms
static int fdin = -1;
static int fdout = -1;
static pthread_t monitor;

int SBCELT_FUNC(celt_decode_float_rw)(CELTDecoder *st, const unsigned char *data, int len, float *pcm);
int SBCELT_FUNC(celt_decode_float_futex)(CELTDecoder *st, const unsigned char *data, int len, float *pcm);
int SBCELT_FUNC(celt_decode_float_picker)(CELTDecoder *st, const unsigned char *data, int len, float *pcm);
int (*SBCELT_FUNC(celt_decode_float))(CELTDecoder *, const unsigned char *, int, float *) = SBCELT_FUNC(celt_decode_float_picker);

// SBCELT_CheckSeccomp checks for kernel support for
// SECCOMP.
//
// On success, the function returns a valid sandbox
// mode (see SBCELT_SANDBOX_*).
//
// On failure, the function returns
//  -1 if the helper process did not execute correctly.
//  -2 if the fork system call failed. This signals that the
//     host system is running low on memory. This is a
//     recoverable error, and in our case we should simply
//     wait a bit and try again.
int SBCELT_CheckSeccomp() {
 	int status, err;
	pid_t child;

	child = fork();
	if (child == -1) {
 		return -2;
	} else if (child == 0) {
		char *helper = getenv("SBCELT_HELPER_BINARY");
		if (helper == NULL) {
			helper = "/usr/bin/sbcelt-helper";
		}
		char *const argv[] = {
			helper,
			"seccomp-detect",
			NULL,
		};
		execv(argv[0], argv);
		_exit(100);
	}

	while ((err = waitpid(child, &status, 0)) == -1 && errno == EINTR);
	if (err == -1) {
		return -1;
	}

	if (!WIFEXITED(status)) {
		return -1;
	}

	int code = WEXITSTATUS(status);
	if (!SBCELT_SANDBOX_VALID(code)) {
		return -1;
	}

	return code;
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
		int err;
		while ((err = waitpid(child, &status, 0)) == -1 && errno == EINTR);
		if (err == 0) {
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

int SBCELT_RelaunchHelper() {
	char *helper = getenv("SBCELT_HELPER_BINARY");
	if (helper == NULL) {
		helper = "/usr/bin/sbcelt-helper";
	}

	int fds[2];
	int chin, chout;

	if (pipe(fds) == -1)
		return -1;
	fdin = fds[0];
	chout = fds[1];

	if (pipe(fds) == -1)
		return -1;
	fdout = fds[1];
	chin = fds[0];

	debugf("SBCELT_RelaunchHelper; fdin=%i, fdout=%i", fdin, fdout);

	pid_t child = fork();
	if (child == -1) {
		// We're memory constrained... TODO
	} else if (child == 0) {
		close(0);
		close(1);
		if (dup2(chin, 0) == -1)
			exit(100);
		if (dup2(chout, 1) == -1)
			exit(101);

		char *const argv[] = {
			helper,
			NULL,
		};
		execv(argv[0], argv);
		exit(1);
	}

	debugf("SBCELT_RelaunchHelper; relaunched helper (pid=%li)", child);

	return 0;
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
		if (ftruncate(fd, SBCELT_PAGES*SBCELT_PAGE_SIZE) == -1) {
			debugf("unable to truncate");
			return -1;
		}

		void *addr = mmap(NULL, SBCELT_PAGES*SBCELT_PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fd, 0);
		memset(addr, 0, SBCELT_PAGES*SBCELT_PAGE_SIZE);

		workpage = addr;
		decpage = addr+SBCELT_PAGE_SIZE;

		workpage->busywait = sysconf(_SC_NPROCESSORS_ONLN) > 0;

		int i;
		for (i = 0; i < SBCELT_SLOTS; i++) {
			decpage->slots[i].available = 1;
		}
	}

	return 0;
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

// celt_decode_float_picker is the initial value for the celt_decode_float function pointer.
// It checks the available sandbox modes on the system, picks an appropriate celt_decode_float
// implementation to use according to the available sandbox modes, and makes those choices
// available to the helper process in the work page.
int SBCELT_FUNC(celt_decode_float_picker)(CELTDecoder *st, const unsigned char *data, int len, float *pcm) {
	int sandbox = SBCELT_CheckSeccomp();
	if (sandbox == -1) {
		return CELT_INTERNAL_ERROR;
 	// If the system is memory constrained, pretend that we were able
	// decode a frame correctly, and delegate the seccomp availability
	// check to sometime in the future.
	} else if (sandbox == -2) {
		memset(pcm, 0, sizeof(float)*480);
		return CELT_OK;
	}

	// For benchmarking and testing purposes, it's beneficial for us
	// to force a SECCOMP_STRICT sandbox, and therefore be force to run
	// in the rw mode.
	if (getenv("SBCELT_PREFER_SECCOMP_STRICT") != NULL) {
		if (sandbox == SBCELT_SANDBOX_SECCOMP_BPF) {
			sandbox = SBCELT_SANDBOX_SECCOMP_STRICT;
		}
	}

	debugf("picker: chose sandbox=%i", sandbox);
	workpage->sandbox = sandbox;

	// If we're without a sandbox, or is able to use seccomp
	// with BPF filters, we can use our fast futex mode.
	// For seccomp strict mode, we're limited to rw mode.
	switch (workpage->sandbox) {
		case SBCELT_SANDBOX_SECCOMP_STRICT:
			workpage->mode = SBCELT_MODE_RW;
			SBCELT_FUNC(celt_decode_float) = SBCELT_FUNC(celt_decode_float_rw);
			break;
		default:
			workpage->mode = SBCELT_MODE_FUTEX;
			SBCELT_FUNC(celt_decode_float) = SBCELT_FUNC(celt_decode_float_futex);
			break;
	}

	debugf("picker: chose mode=%i", workpage->mode);

	if (workpage->mode == SBCELT_MODE_FUTEX) {
		pthread_t tmp;
		if (pthread_create(&monitor, NULL, SBCELT_HelperMonitor, NULL) != 0) {
			return -1;
		}
	}

	return SBCELT_FUNC(celt_decode_float)(st, data, len, pcm);
}

int SBCELT_FUNC(celt_decode_float_futex)(CELTDecoder *st, const unsigned char *data, int len, float *pcm) {
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

int SBCELT_FUNC(celt_decode_float_rw)(CELTDecoder *st, const unsigned char *data, int len, float *pcm) {
	int slot = (int)((uintptr_t)st);
	ssize_t remain;
	void *dst;

	debugf("decode_float; len=%i", len);

	workpage->slot = slot;
	memcpy(&workpage->encbuf[0], data, len);
	workpage->len = len;

retry:
	if (fdout == -1 || fdin == -1) {
		if (SBCELT_RelaunchHelper() == -1) {
			goto retry;
		}
	}

	// signal to the helper that we're ready to work
	if (write(fdout, &workpage->pingpong, 1) == -1) {
		debugf("decode_float; write failed: %i", errno);
		close(fdout);
		fdout = -1;
		goto retry;
	}

	// read decoded frame from helper
	if (read(fdin, &workpage->pingpong, 1) == -1) {
		debugf("decode_float; read failed: %i", errno);
		close(fdin);
		fdin = -1;
		goto retry;
	}

	memcpy(pcm, workpage->decbuf, sizeof(float)*480);

	return CELT_OK;
}

