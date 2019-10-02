CC=gcc
CCFLAGS=-std=gnu11 -Wuninitialized -Wall -Werror -Wno-unused-label
LDFLAGS=-lpthread -lm -lmicrohttpd -lcrypto

GIT_COMMIT ?= "unknown"
BUILD_TIME := $(shell date +%s)
CCFLAGS += -DGIT_COMMIT=\"$(GIT_COMMIT)\" -DBUILD_TIME=\"$(BUILD_TIME)\"


DUMPSRC=$(wildcard src/common/*.c src/dump/*.c)
DUMPOBJ=$(DUMPSRC:%.c=%.o)

SMOKESRC=$(wildcard src/common/*.c src/test_common/*.c src/client/*.c src/smoketest/*.c)
SMOKEOBJ=$(SMOKESRC:%.c=%.o)

COMPSRC=$(wildcard src/common/*.c src/client/*.c src/comptest/*.c)
COMPOBJ=$(COMPSRC:%.c=%.o)

SAMPLEEXTRACTORSRC=$(wildcard src/common/*.c src/client/*.c src/sample-extractor/*.c)
SAMPLEEXTRACTOROBJ=$(SAMPLEEXTRACTORSRC:%.c=%.o)

SERVERSRC=$(wildcard src/common/*.c src/server/*.c)
SERVEROBJ=$(SERVERSRC:%.c=%.o)

TESTSRC=$(wildcard src/common/*.c src/test_common/*.c src/test/*.c)
TESTOBJ=$(TESTSRC:%.c=%.o)

OBJWITHOUTMAIN := $(filter-out src/main.o,$(OBJ))

debug: CCFLAGS += -DDEBUG_BUILD -DSERVER_BUILD -g
debug: all

release: CCFLAGS += -O3
release: all

all: dump server test smoke comp sample-extractor

dump: $(DUMPOBJ)
	$(CC) $(CCFLAGS) -o dump $(DUMPOBJ) $(LDFLAGS)

smoke: $(SMOKEOBJ)
	$(CC) $(CCFLAGS) -o smoke $(SMOKEOBJ) $(LDFLAGS)

comp: $(COMPOBJ)
	$(CC) $(CCFLAGS) -o comp $(COMPOBJ) $(LDFLAGS)

sample-extractor: $(SAMPLEEXTRACTOROBJ)
	$(CC) $(CCFLAGS) -o sample-extractor $(SAMPLEEXTRACTOROBJ) $(LDFLAGS)

server: $(SERVEROBJ)
	$(CC) $(CCFLAGS) -o server $(SERVEROBJ) $(LDFLAGS)

test: $(TESTOBJ)
	$(CC) $(CCFLAGS) -o test $(TESTOBJ) $(LDFLAGS) -lcunit


.PHONY: reformat
reformat:
	find -regex '.*/.*\.\(c\|h\)$$' -exec clang-format-7 -i {} \;

# To obtain object files
%.o: %.c
	$(CC) -I./src/client -I./src/common -I./src/test_common -c $(CCFLAGS) $< -o $@

clean:
	rm -f dump server test smoke $(DUMPOBJ) $(SMOKEOBJ) $(COMPOBJ) $(SAMPLEEXTRACTOROBJ) $(SERVEROBJ) $(TESTOBJ)

run-valgrind: server
	#valgrind --track-origins=yes ./server
	valgrind --leak-check=full --show-leak-kinds=all ./server

