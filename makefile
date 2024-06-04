CFLAGS += -O0 -Wall -g

ifeq ($(OS),Windows_NT)
	CFLAGS += -IC:/msys64/mingw64/include/SDL2 -Dmain=SDL_main -LC:/msys64/mingw64/lib -lmingw32 -lws2_32 
endif

CFLAGS += -lSDL2main -lSDL2

# get shared libs here
ifeq ($(OS),Windows_NT)
%.dll:
	cp C:/msys64/mingw64/bin/$@ build/
endif

.PHONY: win_get_libs
win_get_libs: SDL2.dll libwinpthread-1.dll

# preparations
.PHONY: prepare
prepare:
	mkdir -p build
	mkdir -p obj

#our vector library
.PHONY: vec
vec:
	gcc -o obj/vec.o -c vec/src/vec.c -Wall -Wextra -Ivec/src

#part with resources copy
.PHONY: resources
resources: prepare
	mkdir -p build/resources
	mkdir -p build/resources/textures
	mkdir -p build/resources/blocks
	cp -r resources/* build/resources

.PHONY: test
test: mains/test.c resources vec
	gcc -o obj/test.o -c mains/test.c ${CFLAGS}
	gcc -o build/test obj/test.o obj/vec.o ${CFLAGS} -lm

.PHONY: graphic
graphic: mains/graphics_test.c resources
	gcc -o obj/g_test.o -c mains/graphics_test.c ${CFLAGS}
	gcc -o build/g_test obj/g_test.o ${CFLAGS} -lm -g
	./build/g_test

.PHONY: test_lua
test_lua: mains/lua_test.c
	gcc -o obj/lua_test.o -c mains/lua_test.c
ifeq ($(OS),Windows_NT)
	gcc -o build/lua_test obj/lua_test.o ~/../../mingw64/lib/liblua.a -lm
else
	gcc -o build/lua_test obj/lua_test.o lua/src/liblua.a -lm
endif

.PHONY: client_app
client_app: mains/client.c resources vec
	gcc -o build/client mains/client.c obj/vec.o -Wall ${CFLAGS} -lm -g
ifeq ($(OS),Windows_NT)
	./build/client.exe
else
	./build/client
endif

.PHONY: networking
networking:
ifeq ($(OS),Windows_NT)
	gcc -o build/test_server mains/network_test_server.c -Wall -lmingw32 -lws2_32 
	gcc -o build/test_client mains/network_test_client.c -Wall -lmingw32 -lws2_32 
else
	gcc -o build/test_server mains/network_test_server.c -Wall
	gcc -o build/test_client mains/network_test_client.c -Wall
endif
	

clean:
	rm -rf build/*
	rm -rf obj/*

all: test graphic test_lua client_app networking