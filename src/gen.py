import sys, re

def gen_mulNx1(fo, unitN, n):
	inN = unitN * n
	outN = unitN * (n + 1)
	print>>fo, "define i%d @mul%dx%d(i%d %%x, i%d %%y) {" % (outN, inN, unitN, inN, unitN)
	for i in xrange(0, n):
		print>>fo, "  %%x%d = call i%d @extract%d(i%d %%x, i%d %d)" % (i, unitN, inN, inN, inN, i * unitN)
		print>>fo, "  %%x%dy = call i%d @mul%dx%d(i%d %%x%d, i%d %%y)" % (i, unitN * 2, unitN, unitN, unitN, i, unitN)
		print>>fo, "  %%x%dy0 = zext i%d %%x%dy to i%d" % (i, unitN * 2, i, outN)
	for i in xrange(1, n):
		print>>fo, "  %%x%dy1 = shl i%d %%x%dy0, %d" % (i, outN, i, i * unitN)

	print>>fo, "  %%t0 = add i%d %%x0y0, %%x1y1" % outN
	for i in xrange(1, n - 1):
		print>>fo, "  %%t%d = add i%d %%t%d, %%x%dy1" % (i, outN, i - 1, i + 1)
	print>>fo, "  ret i%d %%t%d" % (outN, n - 2)
	print>>fo, "}"

def gen_sub(fo, s, unitN, bitN):
	s = s.replace('@bitNpUm1', str(bitN + unitN - 1))
	s = s.replace('@bitNpU', str(bitN + unitN))
	s = s.replace('@bitN', str(bitN))
	s = s.replace('@unitN2', str(unitN * 2))
	s = s.replace('@unitN', str(unitN))
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
	gen(fo, 'once.txt', unitN, [unitN * 2])
	gen(fo, 'all.txt', unitN, [64, 128, 192, 256, 384, 512, 578])

	if unitN == 64:
		gen(fo, 'short.txt', unitN, [128, 192, 256, 384])
		gen(fo, 'long.txt', unitN, [448, 512, 578])
	elif unitN == 32:
		gen(fo, 'short.txt', unitN, [128])
		gen(fo, 'long.txt', unitN, [192, 384, 448, 512, 578])
	else:
		print "bad unitN", unitN
		exit(1)
	for i in xrange(2, 4):
		gen_mulNx1(fo, unitN, i)
	fo.close()

if __name__ == "__main__":
    main()

