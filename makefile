test: test.c
	mkdir -p build
	gcc -o build/test test.c -O0 -Wall -lSDL2 -lm -g
graphic: graphics_test.c
	gcc -o build/g_test graphics_test.c -Wall -lSDL2 -lm -O0
	cp -n -r textures/ build/textures/
clean:
	rm -rf build/*