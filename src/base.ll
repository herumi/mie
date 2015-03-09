declare { i64, i1 } @llvm.uadd.iwth.overflow.i64(i64, i64)

; void mie_fp_add192
define void @mie_fp_add192(i192* %pz, i192* %px, i192* %py, i192* %pp) {
entry:
	%x = load i192* %px
	%y = load i192* %py
	%p = load i192* %pp
	%x1 = zext i192 %x to i256
	%y1 = zext i192 %y to i256
	%p1 = zext i192 %p to i256
	%t0 = add i256 %x1, %y1
	%t1 = sub i256 %t0, %p1 ; x + y - p
	%t2 = lshr i256 %t1, 192
	%t3 = trunc i256 %t2 to i64
	%t4 = trunc i256 %t1 to i192
	%t5 = trunc i256 %t0 to i192
	%zero = icmp eq i64 %t3, 0
	br i1 %zero, label %nocarry, label %carry
carry:
	br label %exit
nocarry:
	br label %exit
exit:
	%t6 = phi i192 [%t4, %carry], [%t5, %nocarry]
	store i192 %t6, i192* %pz
	ret void
}

