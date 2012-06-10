# Copyright (C) 2012 The SBCELT Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE-file.

include Make.conf

default:
	make -C lib
	make -C helper

install:
	make -C lib install
	make -C helper install

clean:
	make -C lib clean
	make -C helper clean
