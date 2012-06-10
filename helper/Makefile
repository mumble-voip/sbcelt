include ../Make.conf

CFLAGS = -DHAVE_CONFIG_H -I. -I../celt-0.7.0/libcelt/
LDFLAGS = -lm -lpthread -lrt

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
	seccomp-sandbox.c

include ../Make.cmd
