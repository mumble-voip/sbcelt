// Copyright (C) 2012 The SBCELT Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE-file.

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

static void *pdeath_monitor(void *udata) {
	int kqfd = *((int *)udata);
	while (1) {
		struct kevent ke;
		if (kevent(kqfd, NULL, 0, &ke, 1, NULL) == 0) {
			_exit(0);
		}
	}
	return NULL;
}

int pdeath() {
	struct kevent ke;
	pid_t ppid;
	int kqfd;

	kqfd = kqueue();
	if (kqfd == -1) {
		return -1;
	}

	ppid = getppid();
	if (ppid <= 1) {
		_exit(0);
	}

	EV_SET(&ke, ppid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, NULL);
	if (kevent(kqfd, &ke, 1, NULL, 0, NULL) == -1) {
		return -1;
	}

	pthread_t thr;
	if (pthread_create(&thr, NULL, pdeath_monitor, &kqfd) == -1) {
		return -1;
	}

	if (getppid() <= 1) {
		_exit(0);
	}

	return 0;
}
