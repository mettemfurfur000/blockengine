CFLAGS += -O0 -Wall -g -MMD
LDFLAGS += -lm -g -lz -lunwind

# uncomment to do sum 
ifeq ($(PERF),1)
    CFLAGS += -pg -no-pie
    LDFLAGS += -pg
endif

ifeq ($(OS),Windows_NT)
# this trick is required for clang, since it doesn really know what to do with unix paths on windows
    MSYSINSTALLDIR := $(shell cd /;pwd -W)
    ROOTDIR := $(MSYSINSTALLDIR)$(CURDIR)
else
    ROOTDIR := $(CURDIR) 
endif

# enables including from projects root directory
CFLAGS += -I$(ROOTDIR)
# enables including directly from libs folder
CFLAGS += -I$(ROOTDIR)/libs

ifeq ($(OS),Windows_NT)
    CFLAGS += -IC:/msys64/mingw64/include/SDL2 
    CFLAGS += -Dmain=SDL_main
    
    LDFLAGS += -LC:/msys64/mingw64/lib
    LDFLAGS += -llua
    LDFLAGS += -lmingw32
    LDFLAGS += -lws2_32
    LDFLAGS += -lbacktrace
else
    CFLAGS += -I/usr/include/lua5.4/
    CFLAGS += -I/usr/include/SDL2

    LDFLAGS += -lbacktrace
endif

# general libraries 

LDFLAGS += -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

# opengl stuff
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lopengl32 -lepoxy.dll
    LDFLAGS += -lWinmm # sum weird time lib that enet uses
else
    LDFLAGS += -lGL -lepoxy -llua5.4
endif

SRCS_C := $(shell cd src;find . -name '*.c')
SRCS_CPP := $(shell cd src;find . -name '*.cpp')
SRCS := $(SRCS_C) $(SRCS_CPP)

OBJS_C := $(patsubst %.c,obj/%.o,$(SRCS_C))
OBJS_CPP := $(patsubst %.cpp,obj/%.o,$(SRCS_CPP))
OBJS := $(OBJS_C) $(OBJS_CPP)
OBJS := $(shell echo $(OBJS) | sed 's#/./#/#')

DEPS_C := $(patsubst %.c,obj/%.d,$(SRCS_C))
DEPS_CPP := $(patsubst %.cpp,obj/%.d,$(SRCS_CPP))
DEPS := $(DEPS_C) $(DEPS_CPP)
# DEPS := $(shell echo $(DEPS) | sed 's#/./#/#')

all: client_app builder

-include $(DEPS)

# our vector library
.PHONY: vec
vec:
	gcc -o obj/vec.o -c libs/vec/src/vec.c -Wall -Wextra -Ilibs/vec/src

#part with copy_instance copy
.PHONY: copy_instance
copy_instance:
	mkdir -p build
	cp -r instance/* build/

obj/%.o : src/%.c
	mkdir -p $(shell echo $@ | sed -r "s/(.+)\/.+/\1/")
	gcc $(CFLAGS) -c $< -o $@

obj/%.o : src/%.cpp
	g++ -std=c++17 $(CFLAGS) -c $< -o $@

.PHONY: client_app
client_app: mains/client.c copy_instance vec $(OBJS)
	gcc -o obj/client.o -c mains/client.c ${CFLAGS}
	g++ ${CFLAGS} -o build/client obj/client.o obj/vec.o $(OBJS) $(LDFLAGS) -lstdc++

.PHONY: builder
builder: mains/builder.c copy_instance vec $(OBJS)
	gcc -o obj/builder.o -c mains/builder.c ${CFLAGS}
	g++ ${CFLAGS} -o build/builder obj/builder.o obj/vec.o $(OBJS) $(LDFLAGS) -lstdc++

# use this when packaging to get all the dll-s used
.PHONY: grab_client_dlls
grab_client_dlls:
	-./grab_dlls.sh build/client.exe /mingw64/bin 2

clean:
	rm -rf build/*
	rm -rf obj/*