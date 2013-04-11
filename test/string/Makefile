GCC_VER=$(shell gcc -dumpversion)
ifeq ($(shell expr $(GCC_VER) \>= 4.2),1)
    ADD_OPT+=-mtune=native
endif

TARGET=string_test

BIT=32
ifeq ($(shell uname -m),x86_64)
BIT=64
endif
ifeq ($(shell uname -s),Darwin)
BIT=64
endif
ifeq ($(BIT),64)
TARGET += string_test64
endif
# ----------------------------------------------------------------
CFLAGS += -O3 -fomit-frame-pointer -DNDEBUG -msse4 $(ADD_OPT) -I../../include
CFLAGS_WARN=-Wall -Wextra -Wformat=2 -Wcast-qual -Wwrite-strings -Wfloat-equal -Wpointer-arith #-Wlong-long # -Wcast-align
CFLAGS+=$(CFLAGS_WARN)
# ----------------------------------------------------------------

HEADER=../../include/mie/string.hpp benchmark.hpp util.hpp
all:$(TARGET)

.SUFFIXES: .cpp

string_test: string_test.cpp $(HEADER)
	$(CXX) $(CFLAGS) string_test.cpp -o $@ -m32

string_test64: string_test.cpp $(HEADER)
	$(CXX) $(CFLAGS) string_test.cpp -o $@ -m64

.cpp.o:
	$(CXX) -c $< -o $@ $(CFLAGS)

.c.o:
	$(CXX) -c $< -o $@ $(CFLAGS)

clean:
	$(RM) *.o $(TARGET)

