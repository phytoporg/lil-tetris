#!/usr/bin/bash

which butler
if [ $? -ne 0 ]; then
    >&2 echo Cannot deploy: please install butler
    exit -1
fi

butler push ./embuild fivelarge/lil-tetris:html
