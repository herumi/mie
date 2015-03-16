import sys, re

def gen_mulNx1(fo, unitN, n):
	inN = unitN * n
	outN = unitN * (n + 1)
	if n > 4:
		attr = "noinline"
	else:
		attr = ""
	print>>fo, "define private i%d @mul%dx%d(i%d %%x, i%d %%y) %s {" % (outN, inN, unitN, inN, unitN, attr)
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

def gen_mulNxN(fo, unitN, n):
	inN = unitN * n
	bitNpU = inN + unitN
	outN = inN * 2
	print>>fo, "define void @mie_fp_mul%dpre(i%d* %%pz, i%d* %%px, i%d* %%py) {" % (inN, unitN, inN, inN)
	print>>fo, "  %%x = load i%d* %%px" % inN
	print>>fo, "  %%y = load i%d* %%py" % inN
	for i in xrange(0, n):
		print>>fo, "  %%y%d = call i%d @extract%d(i%d %%y, i%d %d)" % (i, unitN, inN, inN, inN, i * unitN)

	print>>fo, "  %%sum0 = call i%d @mul%dx%d(i%d %%x, i%d %%y0)" % (bitNpU, inN, unitN, inN, unitN)
	print>>fo, "  %%t0 = trunc i%d %%sum0 to i%d" % (bitNpU, unitN)
	print>>fo, "  store i%d %%t0, i%d* %%pz" % (unitN, unitN)

	for i in xrange(1, n):
		print>>fo
		print>>fo, "  %%s%d = lshr i%d %%sum%d, %d" % (i - 1, bitNpU, i - 1, unitN)
		print>>fo, "  %%xy%d = call i%d @mul%dx%d(i%d %%x, i%d %%y%d)" % (i, bitNpU, inN, unitN, inN, unitN, i)
		print>>fo, "  %%sum%d = add i%d %%s%d, %%xy%d" % (i, bitNpU, i - 1, i)
		print>>fo, "  %%z%d = getelementptr i%d* %%pz, i32 %d" % (i, unitN, i)
		if i == n - 1:
			break
		print>>fo, "  %%ts%d = trunc i%d %%sum%d to i%d" % (i, bitNpU, i, unitN)
		print>>fo, "  store i%d %%ts%d, i%d* %%z%d" % (unitN, i, unitN, i)

	print>>fo, "  %%p = bitcast i%d* %%z%d to i%d*" % (unitN, n - 1, bitNpU)
	print>>fo, "  store i%d %%sum%d, i%d* %%p" % (bitNpU, n - 1, bitNpU)
	print>>fo, "  ret void"
	print>>fo, "}"

def gen_mul(fo, unitN):
	bitN = 576
	n = (bitN + unitN - 1) / unitN
	for i in xrange(2, n + 1):
		gen_mulNx1(fo, unitN, i)
		gen_mulNxN(fo, unitN, i)


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
	if unitN not in [32, 64]:
		print "bad unitN", unitN
		exit(1)

	outName = 'base%d.ll' % unitN
	fo = open(outName, 'w')
	gen(fo, 'once.txt', unitN, [unitN * 2])

	bitNL = range(unitN, 576 + 1, unitN)
	gen(fo, 'all.txt', unitN, bitNL)
	gen(fo, 'short.txt', unitN, bitNL)
	gen(fo, 'long.txt', unitN, bitNL)
	gen_mul(fo, unitN)
	fo.close()

if __name__ == "__main__":
    main()

