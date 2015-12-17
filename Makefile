# Redborder Makefile's wrapper

CFLAGS_EXTRA+=-DIGB_NO_LRO \
	-DRELEASE_TAG=' (rev $(shell git describe --abbrev=6  --always --dirty))'

INSTALL_MOD_PATH ?= /opt/rb

.PHONY=all clean install

all:
	make CFLAGS_EXTRA="${CFLAGS_EXTRA}" -C src/

clean:
	make -C src/ clean

install:
	make INSTALL_MOD_PATH="${INSTALL_MOD_PATH}" -C src/ install

