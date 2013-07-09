include ../common.mk

ifeq ($(USE_MONT_FP),1)
  CFLAGS += -DUSE_MONT_FP
endif

TARGET=$(TEST_FILE)
LIBS=

SRC=$(wildcard *.cpp)

all: $(TARGET)

test: $(TARGET)
	@$(UNIT_TEST)

$(OBJDIR):
	@$(MKDIR) $(OBJDIR)

clean:
	$(CLEAN)

$(LIBS):
	$(MAKE) -C ../src

-include $(DEPEND_FILE)

