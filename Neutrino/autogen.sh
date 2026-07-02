#!/bin/sh
set -e
autoconf
autoheader
printf '%s\n' 'Generated ./configure. Run: ./configure && make -f Makefile.autoconf'
