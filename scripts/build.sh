#!/usr/bin/bash

if [[ -z "${BUILD_EMSCRIPTEN}" ]]; then
    rm -rf build
    mkdir build
    gcc -o build/lil-tetris src/lil-tetris.c `sdl2-config --cflags --libs` -lm -lSDL2_mixer -lSDL2_ttf
else
    rm -rf embuild
    mkdir embuild
    emcc src/lil-tetris.c -Os --shell-file ./wasm/index.html -s USE_SDL=2 -s USE_SDL_TTF=2 -s USE_FREETYPE=1 -s USE_SDL_MIXER=2 -s TOTAL_MEMORY=1024MB -s ALLOW_MEMORY_GROWTH -s FORCE_FILESYSTEM=1 -s ASYNCIFY --preload-file ./assets -o ./embuild/index.html
fi

