// Copyright (C) 2012 The SBCELT Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE-file.

#ifndef __SBCELT_H__
#define __SBCELT_H__

#define SBCELT_PAGES 2
#define SBCELT_SLOTS 40

struct SBCELTWorkPage {
	int            slot;
	int            ready;
	int            busywait;
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
