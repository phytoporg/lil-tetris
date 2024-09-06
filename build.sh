#!/usr/bin/bash

rm -rf build
mkdir build
gcc -o build/lil-tetris src/lil-tetris.c `sdl2-config --cflags --libs` -lm -lSDL2_mixer -lSDL2_ttf
