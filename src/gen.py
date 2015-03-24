import sys, re

def gen_mulNx1(fo, unitL, n):
	inL = unitL * n
	outL = unitL * (n + 1)
	if n > 4:
		attr = "noinline"
	else:
		attr = ""
	print>>fo, "define private i%d @mul%dx%d(i%d %%x, i%d %%y) %s {" % (outL, inL, unitL, inL, unitL, attr)
	for i in xrange(0, n):
		print>>fo, "  %%x%d = call i%d @extract%d(i%d %%x, i%d %d)" % (i, unitL, inL, inL, inL, i * unitL)
		print>>fo, "  %%x%dy = call i%d @mul%dx%d(i%d %%x%d, i%d %%y)" % (i, unitL * 2, unitL, unitL, unitL, i, unitL)
		print>>fo, "  %%x%dy0 = zext i%d %%x%dy to i%d" % (i, unitL * 2, i, outL)
	for i in xrange(1, n):
		print>>fo, "  %%x%dy1 = shl i%d %%x%dy0, %d" % (i, outL, i, i * unitL)

	print>>fo, "  %%t0 = add i%d %%x0y0, %%x1y1" % outL
	for i in xrange(1, n - 1):
		print>>fo, "  %%t%d = add i%d %%t%d, %%x%dy1" % (i, outL, i - 1, i + 1)
	print>>fo, "  ret i%d %%t%d" % (outL, n - 2)
	print>>fo, "}"

def gen_mulNxN(fo, unitL, n):
	inL = unitL * n
	bitLpU = inL + unitL
	outL = inL * 2
	print>>fo, "define void @mie_fp_mul%dpre(i%d* %%pz, i%d* %%px, i%d* %%py) {" % (inL, unitL, inL, inL)
	print>>fo, "  %%x = load i%d* %%px" % inL
	print>>fo, "  %%y = load i%d* %%py" % inL
	for i in xrange(0, n):
		print>>fo, "  %%y%d = call i%d @extract%d(i%d %%y, i%d %d)" % (i, unitL, inL, inL, inL, i * unitL)

	print>>fo, "  %%sum0 = call i%d @mul%dx%d(i%d %%x, i%d %%y0)" % (bitLpU, inL, unitL, inL, unitL)
	print>>fo, "  %%t0 = trunc i%d %%sum0 to i%d" % (bitLpU, unitL)
	print>>fo, "  store i%d %%t0, i%d* %%pz" % (unitL, unitL)

	for i in xrange(1, n):
		print>>fo
		print>>fo, "  %%s%d = lshr i%d %%sum%d, %d" % (i - 1, bitLpU, i - 1, unitL)
		print>>fo, "  %%xy%d = call i%d @mul%dx%d(i%d %%x, i%d %%y%d)" % (i, bitLpU, inL, unitL, inL, unitL, i)
		print>>fo, "  %%sum%d = add i%d %%s%d, %%xy%d" % (i, bitLpU, i - 1, i)
		print>>fo, "  %%z%d = getelementptr i%d* %%pz, i32 %d" % (i, unitL, i)
		if i == n - 1:
			break
		print>>fo, "  %%ts%d = trunc i%d %%sum%d to i%d" % (i, bitLpU, i, unitL)
		print>>fo, "  store i%d %%ts%d, i%d* %%z%d" % (unitL, i, unitL, i)

	print>>fo, "  %%p = bitcast i%d* %%z%d to i%d*" % (unitL, n - 1, bitLpU)
	print>>fo, "  store i%d %%sum%d, i%d* %%p" % (bitLpU, n - 1, bitLpU)
	print>>fo, "  ret void"
	print>>fo, "}"

def gen_mul(fo, unitL):
	bitL = 576
	n = (bitL + unitL - 1) / unitL
	for i in xrange(2, n + 1):
		gen_mulNx1(fo, unitL, i)
		gen_mulNxN(fo, unitL, i)

RE_VAL = re.compile(r'\$\(([^)]+)\)')

def evalStr(s, envG, envL={}):
	def eval2str(x):
		return str(eval(x.group(1), envG, envL))
	return RE_VAL.sub(eval2str, s)

def expandFor(s, unitL, bitL):
	out = []
	inFor = False
	b = 0
	e = 0
	N = bitL / unitL
	envG = {
		'N' : N,
		'unitL' : unitL,
		'bit_unitL' : bitL + unitL,
		'bitL' : bitL,
		'bit' : bitL,
		'unit' : unitL,
		'unitL2' : unitL * 2,
	}
	RE_FOR = re.compile(r'@for\s+([^ ]+)\s*,\s*([^ ]+)')
	for line in s.split('\n'):
		if inFor:
			if line.strip() == '@end_for':
				inFor = False
				for i in xrange(b, e):
					def eval2str(x):
						v = eval(x.group(1), envG, { 'I' : str(i)})
						print "v=", v
						return str(v)
					si = str(i)
					for s in sub:
						s = s.replace('@I', si)
						s = evalStr(s, envG, { 'I' : str(i)})
						out.append(s)
			else:
				sub.append(line)
		else:
			p = RE_FOR.search(line)
			if p:
				v = RE_VAL.match(p.group(1))
				b = int(p.group(1))
				e = int(p.group(2))
				print "for begin = %d, end = %d" % (b, e)
				sub = []
				inFor = True
			else:
				out.append(evalStr(line, envG))
	return '\n'.join(out)

def gen_sub(fo, s, unitL, bitL):
	bitLpU = bitL + unitL
	s = s.replace('@bitLpU', str(bitLpU))
	s = s.replace('@bitL', str(bitL))
	s = s.replace('@unitL2', str(unitL * 2))
	s = s.replace('@unitL', str(unitL))
	s = expandFor(s, unitL, bitL)
	fo.write(s)

def gen(fo, inLame, unitL, bitLL):
	fi = open(inLame, 'r')
	s = fi.read()
	fi.close()
	for bitL in bitLL:
		gen_sub(fo, s, unitL, bitL)

def main():
	argv = sys.argv
	args = len(argv)
	unitL = 64
	if args == 2:
		unitL = int(argv[1])
	if unitL not in [32, 64]:
		print "bad unitL", unitL
		exit(1)

	outLame = 'base%d.ll' % unitL
	fo = open(outLame, 'w')
	gen(fo, 'once.txt', unitL, [unitL * 2])

	bitLL = range(unitL, 576 + 1, unitL)
	gen(fo, 'all.txt', unitL, bitLL)
	gen(fo, 'short.txt', unitL, bitLL)
	gen(fo, 'long.txt', unitL, bitLL)
#	gen(fo, 'mul.txt', unitL, bitLL)
	gen_mul(fo, unitL)
	fo.close()

if __name__ == "__main__":
    main()

