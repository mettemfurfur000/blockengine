test: test.c
	mkdir -p build
	gcc -o build/test test.c
graphic: graphics_test.c
	gcc -o build/g_test graphics_test.c -Wall -lSDL2 -lm
	cp -n textures/test.png build/test.png
	cp -n textures/numbers.png build/numbers.png
clean:
	rm -rf build/*