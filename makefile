CFLAGS += -O0 -Wall -MMD
LDFLAGS += -lm -lz -lunwind

LDFLAGS += -lstdc++

CFLAGS += -g
LDFLAGS += -g

ifeq ($(PERF),1)
    CFLAGS += -pg -no-pie
    LDFLAGS += -pg
endif

ifeq ($(DEBUG),1)
    CFLAGS += -DRENDER_ENTITY_COLLISION_DEBUG
endif

ifeq ($(OS),Windows_NT)
# this trick is required for clang, since it doesn really know what to do with unix paths on windows
    MSYSINSTALLDIR := $(shell cd /;pwd -W)
    ROOTDIR := $(MSYSINSTALLDIR)$(CURDIR)
else
    ROOTDIR := $(CURDIR) 
endif

# enables including from various places 
CFLAGS += -I$(ROOTDIR)
CFLAGS += -I$(ROOTDIR)/libs
# CFLAGS += -I$(ROOTDIR)/include

ifeq ($(OS),Windows_NT)
    CFLAGS += -IC:/msys64/mingw64/include/SDL2 
    CFLAGS += -Dmain=SDL_main
    
    LDFLAGS += -LC:/msys64/mingw64/lib
    LDFLAGS += -llua
    LDFLAGS += -lmingw32
    LDFLAGS += -lws2_32
    LDFLAGS += -lbacktrace

    LDFLAGS += -lopengl32 -lepoxy.dll
    LDFLAGS += -lWinmm # sum weird time lib that enet uses
else
    CFLAGS += -I/usr/include/lua5.4/
    CFLAGS += -I/usr/include/SDL2

    LDFLAGS += -lbacktrace
    LDFLAGS += -lGL -lepoxy -llua5.4
endif

# general libraries 

LDFLAGS += -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lbox2d

SRCS_C := $(shell cd src;find . -name '*.c')
SRCS_CPP := $(shell cd src;find . -name '*.cpp')
SRCS := $(SRCS_C) $(SRCS_CPP)

OBJS_C := $(patsubst %.c,obj/%.o,$(SRCS_C))
OBJS_CPP := $(patsubst %.cpp,obj/%.o,$(SRCS_CPP))
OBJS := $(OBJS_C) $(OBJS_CPP)

MAIN_C := $(shell cd mains;find . -name '*.c')
MAIN_CPP := $(shell cd mains;find . -name '*.cpp')
MAINS := $(MAIN_C) $(MAIN_CPP)

DEPS_MAIN_C := $(patsubst %.c,obj/%.d,$(MAIN_C))
DEPS_MAIN_CPP := $(patsubst %.cpp,obj/%.d,$(MAIN_CPP))
DEPS_MAIN := $(DEPS_MAIN_C) $(DEPS_MAIN_CPP)
DEPS_C := $(patsubst %.c,obj/%.d,$(SRCS_C))
DEPS_CPP := $(patsubst %.cpp,obj/%.d,$(SRCS_CPP))
DEPS := $(DEPS_C) $(DEPS_CPP) $(DEPS_MAIN)
# DEPS := $(shell echo $(DEPS) | sed 's#/./#/#')

all: blockengine builder

-include $(DEPS)

# our vector library
.PHONY: vec
vec:
	gcc -o obj/vec.o -c libs/vec/src/vec.c -Wall -Wextra -Ilibs/vec/src

#part with copy_instance copy
.PHONY: copy_instance
copy_instance:
	mkdir -p build
	rsync -avh instance/ build/

obj/%.o : src/%.c
	mkdir -p $(shell echo $@ | sed -r "s/(.+)\/.+/\1/")
	gcc $(CFLAGS) -c $< -o $@

obj/%.o : src/%.cpp
	g++ -std=c++17 $(CFLAGS) -c $< -o $@

.PHONY: blockengine
blockengine: mains/blockengine_base.c copy_instance vec $(OBJS)
	gcc -o obj/blockengine_base.o -c mains/blockengine_base.c ${CFLAGS}
	g++ ${CFLAGS} -o build/blockengine_base obj/blockengine_base.o obj/vec.o $(OBJS) $(LDFLAGS) -lstdc++

.PHONY: builder
builder: mains/builder.c copy_instance vec $(OBJS)
	gcc -o obj/builder.o -c mains/builder.c ${CFLAGS}
	g++ ${CFLAGS} -o build/builder obj/builder.o obj/vec.o $(OBJS) $(LDFLAGS) -lstdc++

.PHONY: tex_gen
tex_gen: mains/tex_gen.c copy_instance vec $(OBJS)
	gcc -o obj/tex_gen.o -c mains/tex_gen.c ${CFLAGS}
	g++ ${CFLAGS} -o build/tex_gen obj/tex_gen.o obj/vec.o $(OBJS) $(LDFLAGS) -lstdc++

# use this when packaging to get all the dll-s used
.PHONY: grab_dlls
grab_dlls:
	-./scripts/grab_dlls.sh build/blockengine_base.exe /mingw64/bin 2

clean:
	rm -rf build/*
	rm -rf obj/*