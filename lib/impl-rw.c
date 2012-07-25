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
#include "../sbcelt-internal.h"
#include "../sbcelt.h"

#include "mtime.h"
#include "debug.h"

#define PAGE_SIZE 4096

static struct SBCELTWorkPage *workpage = NULL;
static struct SBCELTDecoderPage *decpage = NULL;
static int running = 0;
static int lastslot = 0;
static int fdin = -1;
static int fdout = -1;

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


int SBCELT_FUNC(celt_decode_float)(CELTDecoder *st, const unsigned char *data, int len, float *pcm) {
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
	unsigned char _ = slot;
	if (write(fdout, &_, 1) == -1) {
		debugf("decode_float; write failed: %i", errno);
		close(fdout);
		fdout = -1;
		goto retry;
	}

	// read decoded frame from helper
	remain = sizeof(float)*480;
	dst = pcm;
	do {
		ssize_t nread = read(fdin, dst, remain);
		if (nread == -1) {
			debugf("decode_float; read failed: %i", errno);
			close(fdin);
			fdin = -1;
			goto retry;
		}
		dst += nread;
		remain -= nread;
	} while (remain > 0);

	return CELT_OK;
}
