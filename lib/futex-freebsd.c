// Copyright (C) 2012 The SBCELT Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE-file.

#include <errno.h>
#include <sys/types.h>
#include <sys/umtx.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

#include "debug.h"
#include "futex.h"

int futex_available() {
	return 1;
}

int futex_wake(int *futex) {
	int ret = _umtx_op(futex, UMTX_OP_WAKE, 1, 0, 0);
	if (ret != 0) {
		return errno;
	}
	return 0;
}

int futex_wait(int *futex, int val, struct timespec *ts) {
	int ret = _umtx_op(futex, UMTX_OP_WAIT_UINT, val, 0, (void *)ts);
	if (ret != 0) {
		return errno;
	}
	return 0;
}
