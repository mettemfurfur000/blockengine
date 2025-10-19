CFLAGS += -O0 -Wall -g # -pg -no-pie
LDFLAGS += -lm -g # -pg

ifeq ($(OS),Windows_NT)
	CFLAGS += -IC:/msys64/mingw64/include/SDL2 -Dmain=SDL_main -IC:/msys64$(shell pwd)
	LDFLAGS += ~/../../mingw64/lib/liblua.a -LC:/msys64/mingw64/lib -lmingw32 -lws2_32
else
	CFLAGS += -I$(shell pwd)
	CFLAGS += -I/usr/include/lua5.4/
	CFLAGS += -I/usr/include/SDL2
endif

LDFLAGS += -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

ifeq ($(OS),Windows_NT)
	LDFLAGS += -lopengl32 -lepoxy.dll
else
	LDFLAGS += -lGL -lepoxy -llua5.4
endif

# sources := $(shell cd src;echo *.c)
sources_c := $(shell cd src;find . -name '*.c')
sources_cpp := $(shell cd src;find . -name '*.cpp')
sources += $(sources_c) $(sources_cpp)

objects_c := $(patsubst %.c,obj/%.o,$(sources_c))
objects_cpp := $(patsubst %.cpp,obj/%.o,$(sources_cpp))
objects := $(objects_c) $(objects_cpp)
objects := $(shell echo $(objects) | sed 's#/./#/#')
headers := $(shell cd include;echo *.h)

#our vector library
.PHONY: vec
vec:
	gcc -o obj/vec.o -c vec/src/vec.c -Wall -Wextra -Ivec/src

.PHONY: make_folders
make_folders:
	mkdir -p obj
	mkdir -p build

#part with registry copy
.PHONY: registry
registry: make_folders
	mkdir -p build
	cp -r instance/* build/

obj/%.o : src/%.c
	mkdir -p $(shell echo $@ | sed -r "s/(.+)\/.+/\1/")
	gcc $(CFLAGS) -c $^ -o $@

obj/%.o : src/%.cpp
	g++ -std=c++17 $(CFLAGS) -c $^ -o $@

# $(executable): $(objects)
# 	gcc $(CFLAGS) -o build/$@ $^ $(LDFLAGS)

.PHONY: test
test: mains/test.c registry vec $(objects)
	gcc -o obj/test.o -c mains/test.c ${CFLAGS}
	gcc ${CFLAGS} -o build/test obj/test.o obj/vec.o $(objects) $(LDFLAGS)

.PHONY: graphic
graphic: mains/graphics_test.c registry vec $(objects)
	gcc -o obj/g_test.o -c mains/graphics_test.c ${CFLAGS}
	gcc ${CFLAGS} -o build/g_test obj/g_test.o obj/vec.o $(objects) $(LDFLAGS)

.PHONY: test_lua
test_lua: mains/lua_test.c
	gcc -o obj/lua_test.o -c mains/lua_test.c
ifeq ($(OS),Windows_NT)
	gcc -o build/lua_test obj/lua_test.o ~/../../mingw64/lib/liblua.a -lm
else
	gcc -o build/lua_test obj/lua_test.o lua/src/liblua.a -lm
endif

.PHONY: client_app
client_app: mains/client.c registry vec $(objects)
	gcc -o obj/client.o -c mains/client.c ${CFLAGS}
	g++ ${CFLAGS} -o build/client obj/client.o obj/vec.o $(objects) $(LDFLAGS) -lstdc++
# no more -lbox2d 

.PHONY: run_client
run_client: client_app
ifeq ($(OS),Windows_NT)
	cd build;./client.exe
else
	cd build;./client
endif

.PHONY: grab_client_dlls
grab_client_dlls:
	-./grab_dlls.sh build/client.exe /mingw64/bin 2

.PHONY: texgen
texgen: mains/texgen.c registry vec $(objects)
	gcc -o obj/texgen.o -c mains/texgen.c ${CFLAGS}
	gcc ${CFLAGS} -o build/texgen obj/texgen.o obj/vec.o $(objects) $(LDFLAGS)
#	-./grab_dlls.sh build/client.exe /mingw64/bin 1

.PHONY: networking
networking: mains/network_test_server.c mains/network_test_client.c vec $(objects)
ifeq ($(OS),Windows_NT)
	gcc -o obj/network_test_server.o -c mains/network_test_server.c ${CFLAGS}
	gcc -o obj/network_test_client.o -c mains/network_test_client.c ${CFLAGS}
	gcc ${CFLAGS} -o build/test_server obj/network_test_server.o obj/vec.o $(objects) $(LDFLAGS)
	gcc ${CFLAGS} -o build/test_client obj/network_test_client.o obj/vec.o $(objects) $(LDFLAGS)
else
	gcc -o build/test_server mains/network_test_server.o -Wall
	gcc -o build/test_client mains/network_test_client.o -Wall
endif
	
clean:
	rm -rf build/*
	rm -rf obj/*

all: test graphic test_lua client_app networking