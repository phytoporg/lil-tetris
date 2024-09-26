#!/usr/bin/bash


if [[ -z "${BUILD_EMSCRIPTEN}" ]]; then
    rm -rf build
    mkdir build
    gcc -o build/lil-tetris src/lil-tetris.c `sdl2-config --cflags --libs` -lm -lSDL2_mixer -lSDL2_ttf
else
    rm -rf embuild
    mkdir embuild
    emcc src/lil-tetris.c --emrun -s USE_SDL=2 -s USE_SDL_TTF=2 -s USE_FREETYPE=1 -s USE_SDL_MIXER=2 -s TOTAL_MEMORY=1024MB --preload-file ./assets -o ./embuild/lil-tetris.html
fi

