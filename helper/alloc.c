// Copyright (C) 2012 The SBCELT Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE-file.

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define ALIGN_UP(addr,sz)    (((addr)+sz-1) & ~(sz-1))
#define ARENA_SIZE           8*1024*1024 // 8MB

static unsigned char arena[ARENA_SIZE];
static void *ptr = NULL;
static size_t remain = ARENA_SIZE;

void *malloc(size_t size) {
	if (ptr == NULL) {
		ptr = &arena[0];
	}

	if (size > remain) {
		_exit(50);
	}

	void *ret = ptr + sizeof(size_t);
	size_t *sz = (size_t *) ptr;

	ptr += sizeof(size_t) + size;
	ptr = (void *) ALIGN_UP((uintptr_t)ptr, 4);
	remain -= size;

	*sz = size;
	memset(ret, 0, size);

	return ret;
}

void *calloc(size_t nmemb, size_t size) {
	return malloc(nmemb*size);
}

void *realloc(void *ptr, size_t size) {
	size_t *oldsz = ptr-sizeof(size_t);
	void *dst = malloc(size);
	memcpy(dst, ptr, *oldsz);
	return dst;
}

void free(void *ptr) {
}
