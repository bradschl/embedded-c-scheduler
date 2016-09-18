#!/bin/bash

THIS_SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )


# Check that clib is installed
command -v clib >/dev/null 2>&1 || {
    printf 'clib is not installed. Aborting.\n' >&2
    printf 'See https://github.com/clibs/clib\n' >&2
    exit 1
}


# Install
DEPS_PACKAGES="clibs/strdup bradschl/timermath.h bradschl/metamake stephenmathieson/describe.h"

clib install ${DEPS_PACKAGES} -o ${THIS_SCRIPT_DIR}/deps
if (($? > 0)); then
    printf 'Failed to install packages\n' >&2
    exit 1
fi
