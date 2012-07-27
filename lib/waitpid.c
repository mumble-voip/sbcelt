// Copyright (C) 2012 The SBCELT Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE-file.

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

pid_t xwaitpid(pid_t pid, int *status, int opts) {
	int err;
	while ((err = waitpid(pid, status, opts)) == -1 && errno == EINTR);
	return err;
}
