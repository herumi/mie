VER=-3.4
LLC=llc$(VER)
OPT=opt$(VER)
DIS=llvm-dis$(VER)
ASM=llvm-as$(VER)
OPT_LLC= $(ASM) -o - | $(OPT) -O3 -o - | $(DIS) -o - | $(LLC) -

SRC = base.txt base.long.txt base.short.txt

base.ll: gen.py $(SRC)
	python gen.py

x64:
	$(LLC) base.ll -o - -x86-asm-syntax=intel
x86:
	$(LLC) base.ll -o - -x86-asm-syntax=intel -march=x86
arm64:
	$(LLC) base.ll -o - -march=aarch64

arm:
	$(LLC) base.ll -o - -march=arm

#test: modp_test.cpp base.ll
#	clang++ -O3 modp_test.cpp base.ll -o test -Wall -Wextra -I ../../cybozulib/include -I ../../mie/include -lgmp -lgmpxx -march=native

opt: base.ll
	cat base.ll|$(OPT_LLC) -x86-asm-syntax=intel -mattr=bmi2

base.x64.s: base.ll
	cat base.ll|$(OPT_LLC) -mattr=bmi2 > base.x64.s
clean:
	rm -rf test

