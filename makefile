.PHONY: vec
vec:
	gcc -o obj/vec.o -c vec/src/vec.c -Wall -Wextra -Ivec/src

.PHONY: resources
resources:
	cp -n -r textures/ build/textures/
	cp -n -r blocks/ build/blocks/
	cp test.prop build/test.prop

test: test.c resources vec
	mkdir -p build
	gcc -o obj/test.o -c test.c -O0 -Wall
	gcc -o build/test obj/test.o obj/vec.o -lSDL2 -lm -g

graphic: graphics_test.c resources
	gcc -c obj/g_test.o graphics_test.c -O0 -Wall
	gcc -o build/g_test obj/g_test.o -lSDL2 -lm -g

clean:
	rm -rf build/*
