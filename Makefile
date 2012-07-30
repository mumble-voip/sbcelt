# Copyright (C) 2012 The SBCELT Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE-file.

include Make.conf

.PHONY: default
default:
	$(MAKE) -C lib
	$(MAKE) -C helper

.PHONY: clean
clean:
	$(MAKE) -C lib clean
	$(MAKE) -C helper clean
	@cd test && ./clean.bash

.PHONY: test
test:
	@cd test && ./run.bash
	
