import sys, re

def gen(fo, s, unit, bitN):
	sBitN = str(bitN)
	sBitNP = str(bitN + unit)
	sUnit = str(unit)
	
	s = s.replace('@BitNP', sBitNP)
	s = s.replace('@BitN', sBitN)
	s = s.replace('@Unit', sUnit)
	fo.write(s)

def main():
	argv = sys.argv
	args = len(argv)
	unit = 64
	if args == 2:
		unit = int(argv[1])

	fi = open('base.txt')
	s = fi.read()
	fi.close()

	fo = open('base.ll', 'w')

	for bitN in [128, 192, 256]: #[128, 192, 256]:
		gen(fo, s, unit, bitN)
	fo.close()

if __name__ == "__main__":
    main()
