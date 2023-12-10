ifdef OS
	INCFLAGS := -I./win_msys2_headers/
else

endif

CFLAGS :=
CFLAGS += -O0 -Wall -g
CFLAGS += -IC:/msys64/mingw64/include/SDL2 -Dmain=SDL_main -LC:/msys64/mingw64/lib -lmingw32 -lSDL2main -lSDL2

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
	gcc -o obj/vec.o -c vec/src/vec.c -Wall -Wextra -Ivec/src ${INCFLAGS}

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
	gcc ${INCFLAGS} -o obj/test.o -c test.c ${CFLAGS}
	gcc ${INCFLAGS} -o build/test obj/test.o obj/vec.o ${CFLAGS} -lm -lws2_32 

.PHONY: graphic
graphic: graphics_test.c resources
	gcc ${INCFLAGS} -o obj/g_test.o -c graphics_test.c ${CFLAGS}
	gcc ${INCFLAGS} -o build/g_test obj/g_test.o ${CFLAGS} -lm -g

.PHONY: test_lua
test_lua:
	gcc ${INCFLAGS} -o obj/lua_test.o -c lua_test.c ${CFLAGS}
	gcc ${INCFLAGS} -o build/lua_test obj/lua_test.o lua/src/liblua.a -lm

.PHONY: networking
networking:
	gcc ${INCFLAGS} -o build/test_server network_test_server.c -Wall
	gcc ${INCFLAGS} -o build/test_client network_test_client.c -Wall

clean:
	rm -rf build/*

all: test graphic test_lua