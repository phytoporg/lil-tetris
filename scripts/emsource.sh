#!/usr/bin/bash

if [[ -z "${EMSDK_ROOT}" ]]; then
    >&2 echo EMSDK_ROOT is not set
    exit -1
fi

source $EMSDK_ROOT/emsdk_env.sh && export BUILD_EMSCRIPTEN=1
