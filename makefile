resources:
	cp -n -r textures/ build/textures/
	cp -n -r blocks/ build/blocks/
test: test.c resources
	mkdir -p build
	gcc -o build/test test.c -O0 -Wall -lSDL2 -lm -g
graphic: graphics_test.c resources
	gcc -o build/g_test graphics_test.c -Wall -lSDL2 -lm -O0
clean:
	rm -rf build/*