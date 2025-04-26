CFLAGS += -O2 -Wall -pg -no-pie
LDFLAGS += -lm -pg

ifeq ($(OS),Windows_NT)
	CFLAGS += -IC:/msys64/mingw64/include/SDL2 -Dmain=SDL_main
	LDFLAGS += ~/../../mingw64/lib/liblua.a -LC:/msys64/mingw64/lib -lmingw32 -lws2_32
endif

LDFLAGS += -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lopengl32 -lepoxy.dll

# get shared libs here
ifeq ($(OS),Windows_NT)
%.dll:
	cp C:/msys64/mingw64/bin/$@ build/
endif

# sources := $(shell cd src;echo *.c)
sources += $(shell cd src;find . -name '*.c')
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

#part with registry copy
.PHONY: registry
registry: make_folders
	mkdir -p build
	cp -r instance/* build/

obj/%.o : src/%.c
	gcc $(CFLAGS) -c $^ -o $@

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
	gcc ${CFLAGS} -o build/client obj/client.o obj/vec.o $(objects) $(LDFLAGS)
#	-./grab_dlls.sh build/client.exe /mingw64/bin 1

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