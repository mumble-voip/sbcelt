// Copyright (C) 2012 The SBCELT Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE-file.

#ifndef __SBCELT_INTERNAL_H__
#define __SBCELT_INTERNAL_H__

#define SBCELT_PAGES 2
#define SBCELT_SLOTS 40

#define SBCELT_MODE_FUTEX 1
#define SBCELT_MODE_RW    2

struct SBCELTWorkPage {
	int            slot;
	int            ready;
	unsigned char  busywait;
	unsigned char  mode;
	short          _;
	unsigned int   len;
	unsigned char  encbuf[2036];
	float          decbuf[511];
};

struct SBCELTDecoderSlot {
	int   available;
	int   dispose;
};

struct SBCELTDecoderPage {
	struct SBCELTDecoderSlot slots[SBCELT_SLOTS];
};

int SBCELT_Init();

#endif
