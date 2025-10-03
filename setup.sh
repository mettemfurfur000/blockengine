#!/bin/bash

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
	apt update
	apt install build-essential make git libsdl2-2.0-0 \
		libsdl2-dev libsdl2-image-2.0-0 libsdl2-image-dev \
		libsdl2-ttf-2.0-0 libsdl2-ttf-dev libsdl2-mixer-dev \
		lua5.4 liblua5.4-dev \
		libbox2d-dev libbox2d-doc
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
	pacman -Syuu
	pacman -S make mingw-w64-x86_64-toolchain \
		mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_image \
		mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_mixer \
		mingw-w64-x86_64-libepoxy \
		mingw-w64-x86_64-llvm-tools
else
        echo "Unknown platform, aborting"
		echo "Got: $OSTYPE"
fi

