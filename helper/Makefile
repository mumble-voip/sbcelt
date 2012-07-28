include ../Make.conf

CFLAGS = -O2 -DHAVE_CONFIG_H -I. -I.. -I../lib -I../celt-0.7.0/libcelt/
LDFLAGS = -lm -lpthread

CELT = ../celt-0.7.0/libcelt
TARGET = sbcelt-helper

SOURCES = \
	$(CELT)/bands.c \
	$(CELT)/celt.c \
	$(CELT)/cwrs.c \
	$(CELT)/entcode.c \
	$(CELT)/entdec.c \
	$(CELT)/entenc.c \
	$(CELT)/header.c \
	$(CELT)/kiss_fft.c \
	$(CELT)/kiss_fftr.c \
	$(CELT)/laplace.c \
	$(CELT)/mdct.c \
	$(CELT)/modes.c \
	$(CELT)/pitch.c \
	$(CELT)/psy.c \
	$(CELT)/quant_bands.c \
	$(CELT)/rangedec.c \
	$(CELT)/rangeenc.c \
	$(CELT)/rate.c \
	$(CELT)/vq.c \
	sbcelt-helper.c \
	alloc.c

ifeq ($(shell uname -s),Linux)
SOURCES += \
	../lib/futex-linux.c \
	seccomp-sandbox.c \
	sbcelt-sandbox-linux.c
LDFLAGS += -lrt
endif

ifeq ($(shell uname -s),Darwin)
CFLAGS += -Wno-deprecated-declarations
SOURCES += \
	../lib/futex-stub.c \
	sbcelt-sandbox-darwin.c
endif

include ../Make.cmd
