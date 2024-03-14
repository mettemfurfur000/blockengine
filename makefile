CFLAGS += -O0 -Wall -g

ifeq ($(OS),Windows_NT)
	CFLAGS += -IC:/msys64/mingw64/include/SDL2 -Dmain=SDL_main -LC:/msys64/mingw64/lib -lmingw32 -lws2_32 
endif

CFLAGS += -lSDL2main -lSDL2

# get shared libs here
%.dll:
	cp C:/msys64/mingw64/bin/$@ build/

.PHONY: win_get_libs
win_get_libs: SDL2.dll libwinpthread-1.dll

# preparations
.PHONY: prep
prep:
	mkdir -p obj

#our vector library
.PHONY: vec
vec:
	gcc -o obj/vec.o -c vec/src/vec.c -Wall -Wextra -Ivec/src

#part with resources copy
.PHONY: resources
resources: prep
	mkdir -p build/resources
	mkdir -p build/resources/textures
	mkdir -p build/resources/blocks
	cp -r resources/* build/resources

.PHONY: test
test: test.c resources vec
	mkdir -p build
	gcc -o obj/test.o -c test.c ${CFLAGS}
	gcc -o build/test obj/test.o obj/vec.o ${CFLAGS} -lm

.PHONY: graphic
graphic: graphics_test.c resources
	gcc -o obj/g_test.o -c graphics_test.c ${CFLAGS}
	gcc -o build/g_test obj/g_test.o ${CFLAGS} -lm -g
	./build/g_test

.PHONY: test_lua
test_lua:
	gcc -o obj/lua_test.o -c lua_test.c ${CFLAGS}
	gcc -o build/lua_test obj/lua_test.o lua/src/liblua.a -lm

.PHONY: client_app
client_app: client.c resources vec
	gcc -o build/client client.c obj/vec.o -Wall ${CFLAGS} -lm -g
	./build/client

.PHONY: networking
networking:
	gcc -o build/test_server network_test_server.c -Wall
	gcc -o build/test_client network_test_client.c -Wall

clean:
	rm -rf build/*
	rm -rf obj/*

all: test graphic test_lua client_app networking