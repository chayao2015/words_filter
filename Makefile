CFLAGS  = -g -Wall -fno-strict-aliasing 
LDFLAGS = -lpthread -lm
DEPDIR  = 
INCLUDE = -I../src -I../deps/lua-5.3.0/src
DEFINE  =
CC      =
LIBRARY = -L../deps/lua-5.3.0/src -L./

# Platform-specific overrides
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(uname_S),Linux)
	CC += gcc
	DEFINE += -D_LINUX
	LDFLAGS += -lrt -ldl
	SHAREFLAGS += -shared
endif

ifeq ($(uname_S),Darwin)
	CC += clang
	DEFINE += -D_MACH
	SHAREFLAGS += -bundle -undefined dynamic_lookup	
endif

words_filter.so:
	$(CC) $(CFLAGS) -fpic -c words_filter.c $(INCLUDE) $(DEFINE) -D_LUA
	$(CC) $(CFLAGS) $(SHAREFLAGS) -o words_filter.so words_filter.o $(LDFLAGS) $(LIBRARY)
	rm *.o		

clean:
	rm *.o
	rm *.so	