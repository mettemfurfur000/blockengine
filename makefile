test: test.c
	mkdir -p build
	gcc -o build/test test.c
graphic: graphics_test.c
	gcc -o build/g_test graphics_test.c -w -lSDL2
clean:
	rm -rf build/*