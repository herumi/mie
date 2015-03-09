VER=-3.4
LLC=llc$(VER)
OPT=opt$(VER)
DIS=llvm-dis$(VER)
ASM=llvm-as$(VER)

SRC = base.ll

OPT_LLC= $(ASM) -o - | $(OPT) -O3 -o - | $(DIS) -o - | $(LLC) -

x64:
	$(LLC) $(SRC) -o - -x86-asm-syntax=intel
x86:
	$(LLC) $(SRC) -o - -x86-asm-syntax=intel -march=x86
arm64:
	$(LLC) $(SRC) -o - -march=aarch64

arm:
	$(LLC) $(SRC) -o - -march=arm

#test: modp_test.cpp $(SRC)
#	clang++ -O3 modp_test.cpp $(SRC) -o test -Wall -Wextra -I ../../cybozulib/include -I ../../mie/include -lgmp -lgmpxx -march=native

opt: $(SRC)
	cat $(SRC)|$(OPT_LLC) -x86-asm-syntax=intel -mattr=bmi2
clean:
	rm -rf test

