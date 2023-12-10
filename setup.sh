#!/bin/bash

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
	echo "Linux installation script is not done yet"
elif [[ "$OSTYPE" == "msys" ]]; then
	pacman -Syuu
	pacman -S make git mingw-w64-x86_64-toolchain \
		mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_image
else
        echo "Unknown platform, aborting"
fi

