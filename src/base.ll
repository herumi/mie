;declare { i64, i1 } @llvm.uadd.iwth.overflow.i64(i64, i64)

; void mie_fp_add128
define void @mie_fp_add128(i128* %pz, i128* %px, i128* %py, i128* %pp) {
entry:
	%x = load i128* %px
	%y = load i128* %py
	%p = load i128* %pp
	%x1 = zext i128 %x to i192
	%y1 = zext i128 %y to i192
	%p1 = zext i128 %p to i192
	%t0 = add i192 %x1, %y1
	%t1 = sub i192 %t0, %p1 ; x + y - p
	%t2 = lshr i192 %t1, 128
	%t3 = trunc i192 %t2 to i64
	%t4 = trunc i192 %t1 to i128 ; x + y - p
	%t5 = trunc i192 %t0 to i128 ; x + y
	%zero = icmp eq i64 %t3, 0
	br i1 %zero, label %nocarry, label %carry
carry:
	br label %exit
nocarry:
	br label %exit
exit:
	%t6 = phi i128 [%t5, %carry], [%t4, %nocarry]
	store i128 %t6, i128* %pz
	ret void
}


;declare { i64, i1 } @llvm.uadd.iwth.overflow.i64(i64, i64)

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
	%t4 = trunc i256 %t1 to i192 ; x + y - p
	%t5 = trunc i256 %t0 to i192 ; x + y
	%zero = icmp eq i64 %t3, 0
	br i1 %zero, label %nocarry, label %carry
carry:
	br label %exit
nocarry:
	br label %exit
exit:
	%t6 = phi i192 [%t5, %carry], [%t4, %nocarry]
	store i192 %t6, i192* %pz
	ret void
}


;declare { i64, i1 } @llvm.uadd.iwth.overflow.i64(i64, i64)

; void mie_fp_add256
define void @mie_fp_add256(i256* %pz, i256* %px, i256* %py, i256* %pp) {
entry:
	%x = load i256* %px
	%y = load i256* %py
	%p = load i256* %pp
	%x1 = zext i256 %x to i320
	%y1 = zext i256 %y to i320
	%p1 = zext i256 %p to i320
	%t0 = add i320 %x1, %y1
	%t1 = sub i320 %t0, %p1 ; x + y - p
	%t2 = lshr i320 %t1, 256
	%t3 = trunc i320 %t2 to i64
	%t4 = trunc i320 %t1 to i256 ; x + y - p
	%t5 = trunc i320 %t0 to i256 ; x + y
	%zero = icmp eq i64 %t3, 0
	br i1 %zero, label %nocarry, label %carry
carry:
	br label %exit
nocarry:
	br label %exit
exit:
	%t6 = phi i256 [%t5, %carry], [%t4, %nocarry]
	store i256 %t6, i256* %pz
	ret void
}


