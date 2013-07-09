GCC_VER=$(shell gcc -dumpversion)
ifeq ($(shell expr $(GCC_VER) \> 4.2.1),1)
  CFLAGS_OPT +=-march=native
endif
ifeq ($(shell uname -s),Linux)
  OS=Linux
endif
ifeq ($(CXX),clang++)
  CFLAGS_OPT += -O3
else
  ifeq ($(shell expr $(GCC_VER) \> 4.6.0),1)
    CFLAGS_OPT +=-Ofast
  else
    CFLAGS_OPT += -O3
  endif
endif
BIT=64
ifeq ($(shell uname -o),Cygwin)
  BIT=32
endif
CP = cp -f
AR = ar r
MKDIR=mkdir -p
RM=rm -fr
CFLAGS_OPT += -O3 -fomit-frame-pointer -DNDEBUG
#CFLAGS_WARN=-Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith #-Wswitch-enum -Wstrict-aliasing=2
CFLAGS_WARN=-Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wfloat-equal -Wpointer-arith #-Wswitch-enum -Wstrict-aliasing=2
CFLAGS = -g -D_FILE_OFFSET_BITS=64 -msse4.2 -m$(BIT)
CFLAGS+=$(CFLAGS_WARN)
ifeq ($(shell uname -s),Darwin)
  LDFLAGS += -lpthread -lssl -m$(BIT) -lgmp -lgmpxx
else
  LDFLAGS += -lpthread -lssl -m$(BIT) -lrt -lgmp -lgmpxx
endif

DEBUG=1
ifeq ($(RELEASE),1)
  DEBUG=0
endif

ifeq ($(DEBUG),0)
  CFLAGS+=$(CFLAGS_OPT)
  OBJDIR=release
  OBJSUF=
else
  ifeq ($(OS),Linux)
    LDFLAGS+=-rdynamic
  endif
  OBJDIR=debug
  OBJSUF=d
endif

TOPDIR=$(shell 'pwd' | sed "s@mie/.*@mie/@")
EXTDIR=$(TOPDIR)../cybozulib_ext/
#CFLAGS+= -I$(TOPDIR)include -I$(EXTDIR)icu4c/icu/include -I$(EXTDIR)openssl/openssl/include
#LDFLAGS+= -L$(TOPDIR)lib -lssl -lcrypto -ldl -licuuc -licudata -licui18n -lz -Wl,-rpath,'$$ORIGIN/../lib'
CFLAGS+= -I$(TOPDIR)include  -I$(TOPDIR)../cybozulib/include/ -I$(TOPDIR)../xbyak/
LDFLAGS+= -L$(TOPDIR)lib -Wl,-rpath,'$$ORIGIN/../lib'

MKDEP = sh -ec '$(CC) -MM $(CFLAGS) $< | sed "s@\($*\)\.o[ :]*@$(OBJDIR)/\1.o $@ : @g" > $@; [ -s $@ ] || rm -f $@'

CLEAN=$(RM) $(TARGET) $(OBJDIR)

define UNIT_TEST
sh -ec 'for i in $(TARGET); do $$i|grep "ctest:name"; done' > result.txt
grep -v "ng=0, exception=0" result.txt || echo "all unit tests are ok"
endef

define SAMPLE_TEST
sh -ec 'for i in $(TARGET); do $$i; done'
endef

.SUFFIXES: .cpp .d .exe

$(OBJDIR)/%.o: %.cpp
	$(CXX) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/%.d: %.cpp $(OBJDIR)
	@$(MKDEP)

$(TOPDIR)bin/%$(OBJSUF).exe: $(OBJDIR)/%.o $(LIBS)
	$(CXX) $< -o $@ $(LIBS) $(LDFLAGS)

OBJS=$(addprefix $(OBJDIR)/,$(SRC:.cpp=.o))

DEPEND_FILE=$(addprefix $(OBJDIR)/, $(SRC:.cpp=.d))
TEST_FILE=$(addprefix $(TOPDIR)bin/, $(SRC:.cpp=$(OBJSUF).exe))

.PHONY: test

