TARGET = libsbcelt.a

CFLAGS = -O2 -I../celt-0.7.0/libcelt/ -DSBCELT_PREFIX

SOURCES = \
	libsbcelt.c \
	closefrom.c \
	mtime.c \
	stub.c

ifeq ($(shell uname -s),Linux)
SOURCES += futex-linux.c
else
SOURCES += futex-stub.c
endif

include ../Make.lib
