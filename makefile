#our vector library
.PHONY: vec
vec:
	gcc -o obj/vec.o -c vec/src/vec.c -Wall -Wextra -Ivec/src

#part with resources copy
.PHONY: resources
resources:
	mkdir -p build/resources
	mkdir -p build/resources/textures
	mkdir -p build/resources/blocks
	cp -r resources/* build/resources

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

.PHONY: networking
networking:
	gcc -o build/test_server network_test_server.c -Wall
	gcc -o build/test_client network_test_client.c -Wall

clean:
	rm -rf build/*

all: test graphic test_lua