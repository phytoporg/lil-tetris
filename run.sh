#!/usr/bin/bash

if [[ -z "${BUILD_EMSCRIPTEN}" ]]; then
    ./build/lil-tetris $(pwd)/assets &
else 
    emrun ./embuild/lil-tetris.html ./assets
fi
