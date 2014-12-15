include ../common.mk

ifeq ($(USE_MONT_FP),1)
  CFLAGS += -DUSE_MONT_FP
endif

TARGET=$(TEST_FILE)
LIBS=

SRC=fp_test.cpp ec_test.cpp fp_util_test.cpp math_test.cpp paillier_test.cpp
ifeq ($(CPU),x64)
  SRC+=fp_generator_test.cpp mont_fp_test.cpp
endif

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

