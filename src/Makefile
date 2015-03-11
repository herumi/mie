VER=-3.4
LLC=llc$(VER)
OPT=opt$(VER)
DIS=llvm-dis$(VER)
ASM=llvm-as$(VER)
OPT_LLC= $(ASM) -o - | $(OPT) -O3 -o - | $(DIS) -o - | $(LLC) -

SRC = once.txt all.txt long.txt short.txt

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
	cat base64.ll|$(OPT_LLC) -x86-asm-syntax=intel -mattr=bmi2

x64.s: base64.ll
	cat base64.ll|$(OPT_LLC) -mattr=bmi2 > x64.s
x32.s: base32.ll
	cat base32.ll|$(OPT_LLC) -mattr=bmi2 > x32.s
clean:
	rm -rf base*.ll *.s

