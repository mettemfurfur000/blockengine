#our vector library
.PHONY: vec
vec:
	gcc -o obj/vec.o -c vec/src/vec.c -Wall -Wextra -Ivec/src

#part with resources copy
.PHONY: resources
resources:
	mkdir -p build/textures
	mkdir -p build/blocks
	cp resources/* build/resources
	cp test.prop build/resources/properties/test.prop

.PHONY: test
test: test.c resources vec
	mkdir -p build
	gcc -o obj/test.o -c test.c -O0 -Wall
	gcc -o build/test obj/test.o obj/vec.o -lSDL2 -lm -g

.PHONY: graphic
graphic: graphics_test.c resources
	gcc -o obj/g_test.o -c graphics_test.c -O0 -Wall
	gcc -o build/g_test obj/g_test.o -lSDL2 -lm -g

.PHONY: test_lua
test_lua:
	gcc -o obj/lua_test.o -c lua_test.c -O0 -Wall
	gcc -o build/lua_test obj/lua_test.o lua/src/liblua.a -lm

clean:
	rm -rf build/*
