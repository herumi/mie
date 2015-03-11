import sys, re

def gen_sub(fo, s, unitN, bitN):
	sBitN = str(bitN)
	sBitNpU = str(bitN + unitN)
	sBitNpUm1 = str(bitN + unitN - 1)
	sUnitN = str(unitN)
	
	s = s.replace('@BitNpUm1', sBitNpUm1)
	s = s.replace('@BitNpU', sBitNpU)
	s = s.replace('@BitN', sBitN)
	s = s.replace('@UnitN', sUnitN)
	fo.write(s)

def gen(fo, inName, unitN, bitNL):
	fi = open(inName, 'r')
	s = fi.read()
	fi.close()
	for bitN in bitNL:
		gen_sub(fo, s, unitN, bitN)


def main():
	argv = sys.argv
	args = len(argv)
	unitN = 64
	if args == 2:
		unitN = int(argv[1])

	fo = open('base.ll', 'w')

	if unitN == 64:
		gen(fo, 'base.short.txt', unitN, [128, 192, 256, 384])
		gen(fo, 'base.long.txt', unitN, [448, 512, 578])
	elif unitN == 32:
		gen(fo, 'base.short.txt', unitN, [128])
		gen(fo, 'base.long.txt', unitN, [192, 384, 448, 512, 578])
	else:
		print "bad unitN", unitN
		exit(1)
	fo.close()

if __name__ == "__main__":
    main()

