// Copyright (C) 2012 The SBCELT Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE-file.

#ifndef __SBCELT_H__
#define __SBCELT_H__

#define SBCELT_PAGES 2
#define SBCELT_SLOTS 40

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef SBCELT_PREFIX_API
# define SBCELT_FUNC(x) sb ## x
#else
# define SBCELT_FUNC(x) x
#endif

CELTMode *SBCELT_FUNC(sbcelt_mode_create)(celt_int32 Fs, int frame_size, int *error);
int SBCELT_FUNC(celt_decode_float)(CELTDecoder *st, const unsigned char *data, int len, float *pcm);
void SBCELT_FUNC(sbcelt_mode_destroy)(CELTMode *mode);
int SBCELT_FUNC(celt_mode_info)(const CELTMode *mode, int request, celt_int32 *value);
CELTEncoder *SBCELT_FUNC(celt_encoder_create)(const CELTMode *mode, int channels, int *error);
void SBCELT_FUNC(celt_encoder_destroy)(CELTEncoder *st);
int SBCELT_FUNC(celt_encode_float)(CELTEncoder *st, const float *pcm, float *optional_synthesis,
                        unsigned char *compressed, int nbCompressedBytes);
int SBCELT_FUNC(celt_encode)(CELTEncoder *st, const celt_int16 *pcm, celt_int16 *optional_synthesis,
                  unsigned char *compressed, int nbCompressedBytes);
int SBCELT_FUNC(celt_encoder_ctl)(CELTEncoder * st, int request, ...);
CELTDecoder *SBCELT_FUNC(celt_decoder_create)(const CELTMode *mode, int channels, int *error);
void SBCELT_FUNC(celt_decoder_destroy)(CELTDecoder *st);
int SBCELT_FUNC(celt_decode_float)(CELTDecoder *st, const unsigned char *data, int len, float *pcm);
int SBCELT_FUNC(celt_decode)(CELTDecoder *st, const unsigned char *data, int len, celt_int16 *pcm);
int SBCELT_FUNC(celt_decoder_ctl)(CELTDecoder * st, int request, ...);
const char *SBCELT_FUNC(celt_strerror)(int error);

#ifdef __cplusplus
}
#endif

#endif
