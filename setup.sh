#!/bin/bash

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
	apt update
	apt install build-essential make git libsdl2-2.0-0 \
		libsdl2-dev libsdl2-image-2.0-0 libsdl2-image-dev \
		libsdl2-ttf-2.0-0 libsdl2-ttf-dev
elif [[ "$OSTYPE" == "msys" ]]; then
	pacman -Syuu
	pacman -S make mingw-w64-x86_64-toolchain \
		mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_image
else
        echo "Unknown platform, aborting"
fi

