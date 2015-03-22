define i64 @extract128(i128 %x, i128 %shift) {
	%t0 = lshr i128 %x, %shift
	%t1 = trunc i128 %t0 to i64
	ret i64 %t1
}
define i128 @mul64x64(i64 %x, i64 %y) {
	%x0 = zext i64 %x to i128
	%y0 = zext i64 %y to i128
	%z = mul i128 %x0, %y0
	ret i128 %z
}

define i192 @mul128x64(i128 %x, i64 %y)  {
  %x0 = call i64 @extract128(i128 %x, i128 0)
  %x0y = call i128 @mul64x64(i64 %x0, i64 %y)
  %x0y0 = zext i128 %x0y to i192
  %x1 = call i64 @extract128(i128 %x, i128 64)
  %x1y = call i128 @mul64x64(i64 %x1, i64 %y)
  %x1y0 = zext i128 %x1y to i192
  %x1y1 = shl i192 %x1y0, 64
  %t0 = add i192 %x0y0, %x1y1
  ret i192 %t0
}

define void @mie_fp_mont128(i128* %pz, i128* %px, i64* %py, i128* %pp, i64 %r) {
	%p = load i128* %pp
	%x = load i128* %px
	%py0 = getelementptr i64* %py, i64 0
	%y0 = load i64* %py0
	%c0 = call i192 @mul128x64(i128 %x, i64 %y0)
	%c00 = trunc i192 %c0 to i64
	%q0 = mul i64 %c00, %r
	%pq0 = call i192 @mul128x64(i128 %p, i64 %q0)
	%c01 = zext i192 %c0 to i256
	%pq01 = zext i192 %pq0 to i256
	%t0 = add i256 %c01, %pq01
	%s0 = lshr i256 %t0, 64

	%py1 = getelementptr i64* %py, i64 1
	%y1 = load i64* %py1
	%c1 = call i192 @mul128x64(i128 %x, i64 %y1)
	%c10 = trunc i192 %c1 to i64
	%q1 = mul i64 %c10, %r
	%pq1 = call i192 @mul128x64(i128 %p, i64 %q1)
	%c11 = zext i192 %c1 to i256
	%pq11 = zext i192 %pq1 to i256
	%t1 = add i256 %s0, %pq11
	%s1 = lshr i256 %t1, 64

	%p0 = zext i128 %p to i256
	%vc = sub i256 %s1, %p0
	%c = lshr i256 %vc, 192
	%zero = icmp eq i256 %c, 0
	br i1 %zero, label %carry, label %nocarry
nocarry:
	%zn = trunc i256 %vc to i128
	store i128 %zn, i128* %pz
	ret void
carry:
	%zc = trunc i256 %s1 to i128
	store i128 %zc, i128* %pz
	ret void
}

