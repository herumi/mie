VER=-3.5
LLC=llc$(VER)
OPT=opt$(VER)
DIS=llvm-dis$(VER)
ASM=llvm-as$(VER)
OPT_LLC= $(OPT) -O3 -o - | $(LLC) -O3 -o -

SRC = once.txt all.txt long.txt short.txt mul.txt
TARGET=x64.s x86.s arm.s arm64.s
AFLAGS=-mattr=bmi2
all: $(TARGET)

base64.ll: gen.py $(SRC)
	python gen.py 64

base32.ll: gen.py $(SRC)
	python gen.py 32

x64: base64.ll
	$(LLC) base64.ll -o - -x86-asm-syntax=intel
x86: base32.ll
	$(LLC) base32.ll -o - -x86-asm-syntax=intel -march=x86
arm64: base64.ll
	$(LLC) base64.ll -o - -march=aarch64

arm: base32.ll
	$(LLC) base32.ll -o - -march=arm

opt: base64.ll
	cat base64.ll|$(OPT_LLC) -x86-asm-syntax=intel $(AFLAGS)

x64.s: base64.ll
	cat base64.ll|$(OPT_LLC) $(AFLAGS) > x64.s
x86.s: base32.ll
	cat base32.ll|$(OPT_LLC) $(AFLAGS) -march=x86 > x86.s
arm.s: base32.ll
	cat base32.ll|$(OPT_LLC) -march=arm > arm.s
arm64.s: base32.ll
	cat base64.ll|$(OPT_LLC) -march=aarch64 > arm64.s
clean:
	rm -rf base*.ll *.s

