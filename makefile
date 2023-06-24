test: test.c
	mkdir -p build
	gcc -o build/test test.c
clean:
	rm -rf build/*