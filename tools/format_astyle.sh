#!/bin/sh
set -eu
if ! command -v astyle >/dev/null 2>&1; then
    echo "astyle was not found. Install Artistic Style, then rerun this script." >&2
    exit 1
fi
astyle --style=allman --indent=spaces=4 --pad-oper --pad-header --unpad-paren --align-pointer=name --suffix=none \
    include/Neutrino/Neutrino.hpp \
    src/*.cpp \
    examples/*.cpp
