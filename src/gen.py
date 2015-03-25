import sys, re

RE_VAL = re.compile(r'\$\(([^)]+)\)')
RE_DEFINE = re.compile(r'@define\s+(\w+)\s*=(.*)')
RE_IF = re.compile(r'@if\s+(.*)')
RE_ELIF = re.compile(r'@elif\s+(.*)')

def evalStr(s, envG, envL={}):
	def eval2str(x):
		s = x.group(1)
		v = eval(s, envG, envL)
		return str(v)
	s = RE_VAL.sub(eval2str, s)
	return s

def parseFor(s, unitL, bitL):
	"""
	@for i, 0, 3
	exp
	@endif
	|
	v
	@define i = 0
	exp
	@define i = 1
	exp
	@define i = 2
	exp

	"""
	out = ""
	inFor = False
	# available variables in @(<expr>)
	envG = {
		'unit' : unitL,
		'bit' : bitL,
		'N' : bitL / unitL,
	}
	envL = {}
	def evalStrLoc(s):
		return evalStr(s, envG, envL)
	def evalIntLoc(s):
		return eval(s, envG, envL)
	# @for <var>, <begin>, <end>
	RE_FOR = re.compile(r'@for\s+(\w+)\s*,\s*([^ ]+)\s*,\s*([^ ]+)')
	for line in s.split('\n'):
		stripped = line.strip()
		p = RE_DEFINE.match(stripped)
		if p:
			lhs = p.group(1).strip()
			rhs = p.group(2).strip()
			envL[lhs] = evalIntLoc(rhs)
			# not continue for next parseIf
		if inFor:
			if line.strip() == '@endfor':
				inFor = False
				for i in xrange(b, e):
					out += "@define %s = %d\n" % (v, i)
					out += sub
			else:
				sub += line + '\n'
		else:
			p = RE_FOR.search(stripped)
			if p:
				v = p.group(1).strip()
				b = eval(p.group(2), envG)
				e = eval(p.group(3), envG)
				sub = ""
				inFor = True
			else:
				out += line + '\n'
	return out

def parseIf(s, unitL, bitL):
	out = ""
	inIf = False
	ifVar = False
	# available variables in @(<expr>)
	envG = {
		'unit' : unitL,
		'bit' : bitL,
		'N' : bitL / unitL,
	}
	envL = {}
	def evalIntLoc(s):
		return eval(s, envG, envL)
	# @for <var>, <begin>, <end>
	RE_FOR = re.compile(r'@for\s+(\w+)\s*,\s*([^ ]+)\s*,\s*([^ ]+)')
	for line in s.split('\n'):
		stripped = line.strip()
		p = RE_DEFINE.match(stripped)
		if p:
			lhs = p.group(1).strip()
			rhs = p.group(2).strip()
			envL[lhs] = evalIntLoc(rhs)
			continue
		if inIf:
			if stripped == '@endif':
				inIf = False
				continue
			p = RE_ELIF.match(stripped)
			if p:
				ifVar = evalIntLoc(p.group(1))
				continue
			if not ifVar:
				continue
		else:
			p = RE_IF.match(stripped)
			if p:
				inIf = True
				ifVar = evalIntLoc(p.group(1))
				continue
		out += evalStr(line, envG, envL) + '\n'
	return out

def parse(s, unitL, bitL):
	"""
		eval "@(<expr>)" to integer

		@for <var>, <begin>, <end>
		...
		@endfor

		REMARK : @for is not nestable

		@define <var> = <exp>
		REMARK : var is global

		@if <exp>
		@elif <exp>
		@endif

		REMARK : @if is not nestable
	"""
	s = parseFor(s, unitL, bitL)
	s = parseIf(s, unitL, bitL)
	return s

def gen(fo, inLame, unitL, bitLL):
	fi = open(inLame, 'r')
	s = fi.read()
	fi.close()
	for bitL in bitLL:
		t = parse(s, unitL, bitL)
		fo.write(t)

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
#	gen(fo, 't.txt', unitL, [unitL * 4])
#	exit(1)
	gen(fo, 'once.txt', unitL, [unitL * 2])

	bitLL = range(unitL, 576 + 1, unitL)
	gen(fo, 'all.txt', unitL, bitLL)
	gen(fo, 'short.txt', unitL, bitLL)
	gen(fo, 'long.txt', unitL, bitLL)
	gen(fo, 'mul.txt', unitL, bitLL[1:])
	fo.close()

if __name__ == "__main__":
    main()

