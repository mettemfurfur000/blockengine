CFLAGS += -O1 -Wall -g  # -pg -no-pie
LDFLAGS += -lm # -pg

ifeq ($(OS),Windows_NT)
	CFLAGS += -IC:/msys64/mingw64/include/SDL2 -Dmain=SDL_main
	LDFLAGS += ~/../../mingw64/lib/liblua.a -LC:/msys64/mingw64/lib -lmingw32 -lws2_32
endif

LDFLAGS += -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf

# get shared libs here
ifeq ($(OS),Windows_NT)
%.dll:
	cp C:/msys64/mingw64/bin/$@ build/
endif

sources := $(shell cd src;echo *.c)
objects := $(patsubst %.c,obj/%.o,$(sources))
headers := $(shell cd include;echo *.h)

.PHONY: win_get_libs
win_get_libs: SDL2.dll libwinpthread-1.dll

#our vector library
.PHONY: vec
vec:
	gcc -o obj/vec.o -c vec/src/vec.c -Wall -Wextra -Ivec/src

.PHONY: make_folders
make_folders:
	mkdir -p obj
	mkdir -p build

#part with resources copy
.PHONY: resources
resources: make_folders
	mkdir -p build/resources
	mkdir -p build/resources/textures
	mkdir -p build/resources/blocks
	cp -r resources/* build/resources

obj/%.o : src/%.c
	gcc $(CFLAGS) -c $^ -o $@

# $(executable): $(objects)
# 	gcc $(CFLAGS) -o build/$@ $^ $(LDFLAGS)

.PHONY: test
test: mains/test.c resources vec $(objects)
	gcc -o obj/test.o -c mains/test.c ${CFLAGS}
	gcc ${CFLAGS} -o build/test obj/test.o obj/vec.o $(objects) $(LDFLAGS)

.PHONY: graphic
graphic: mains/graphics_test.c resources vec $(objects)
	gcc -o obj/g_test.o -c mains/graphics_test.c ${CFLAGS}
	gcc ${CFLAGS} -o build/g_test obj/g_test.o obj/vec.o $(objects) $(LDFLAGS)
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
client_app: mains/client.c resources vec $(objects)
	gcc -o obj/client.o -c mains/client.c ${CFLAGS}
	gcc ${CFLAGS} -o build/client mains/client.c obj/vec.o $(objects) $(LDFLAGS)
#	-./grab_dlls.sh build/client.exe /mingw64/bin 1
ifeq ($(OS),Windows_NT)
	./build/client.exe
else
	./build/client
endif

.PHONY: networking
networking: mains/network_test_server.c mains/network_test_client.c vec $(objects)
ifeq ($(OS),Windows_NT)
	gcc -o obj/network_test_server.o -c mains/network_test_server.c ${CFLAGS}
	gcc -o obj/network_test_client.o -c mains/network_test_client.c ${CFLAGS}
	gcc ${CFLAGS} -o build/test_server obj/network_test_server.o obj/vec.o $(objects) $(LDFLAGS)
	gcc ${CFLAGS} -o build/test_client obj/network_test_client.o obj/vec.o $(objects) $(LDFLAGS)
else
	gcc -o build/test_server mains/network_test_server.c -Wall
	gcc -o build/test_client mains/network_test_client.c -Wall
endif
	
clean:
	rm -rf build/*
	rm -rf obj/*

all: test graphic test_lua client_app networking