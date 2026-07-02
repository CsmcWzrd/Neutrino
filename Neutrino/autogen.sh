#!/bin/sh
set -e
autoconf
printf '%s\n' 'Generated ./configure. Run: ./configure && make -f Makefile.autoconf'
