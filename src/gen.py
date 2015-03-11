import sys, re

def gen(fo, s, unitN, bitN):
	sBitN = str(bitN)
	sBitNpU = str(bitN + unitN)
	sBitNpUm1 = str(bitN + unitN - 1)
	sUnitN = str(unitN)
	
	s = s.replace('@BitNpUm1', sBitNpUm1)
	s = s.replace('@BitNpU', sBitNpU)
	s = s.replace('@BitN', sBitN)
	s = s.replace('@UnitN', sUnitN)
	fo.write(s)

def main():
	argv = sys.argv
	args = len(argv)
	unitN = 64
	if args == 2:
		unitN = int(argv[1])

	fi = open('base.txt')
	s = fi.read()
	fi.close()

	fo = open('base.ll', 'w')

	for bitN in [128, 192, 256]: #[128, 192, 256]:
		gen(fo, s, unitN, bitN)
	fo.close()

if __name__ == "__main__":
    main()
