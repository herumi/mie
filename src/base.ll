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
	%t0 = add i192 %x1, %y1 ; x + y
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

define { i128, i64 } @mie_local_sbb128(i128 %x, i128 %y) {
	%x1 = zext i128 %x to i192
	%y1 = zext i128 %y to i192
	%v1 = sub i192 %x1, %y1
	%v = trunc i192 %v1 to i128
	%c = lshr i192 %v1, 128
	%c1 = trunc i192 %c to i64
	%r1 = insertvalue { i128, i64 } undef, i128 %v, 0
	%r2 = insertvalue { i128, i64 } %r1, i64 %c1, 1
	ret { i128, i64 } %r2
}

; void mie_fp_sub128
define void @mie_fp_sub128(i128* %pz, i128* %px, i128* %py, i128* %pp) {
entry:
	%x = load i128* %px
	%y = load i128* %py
	%p = load i128* %pp
	%vc = call { i128, i64 } @mie_local_sbb128(i128 %x, i128 %y)
	%v = extractvalue { i128, i64 } %vc, 0 ; x - y
	%c = extractvalue { i128, i64 } %vc, 1
	%t = add i128 %v, %p ; x - y + p
	%zero = icmp eq i64 %c, 0
	br i1 %zero, label %nocarry, label %carry
carry:
	br label %exit
nocarry:
	br label %exit
exit:
	%t1 = phi i128 [%t, %carry], [%v, %nocarry]
	store i128 %t1, i128* %pz
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
	%t0 = add i256 %x1, %y1 ; x + y
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

define { i192, i64 } @mie_local_sbb192(i192 %x, i192 %y) {
	%x1 = zext i192 %x to i256
	%y1 = zext i192 %y to i256
	%v1 = sub i256 %x1, %y1
	%v = trunc i256 %v1 to i192
	%c = lshr i256 %v1, 192
	%c1 = trunc i256 %c to i64
	%r1 = insertvalue { i192, i64 } undef, i192 %v, 0
	%r2 = insertvalue { i192, i64 } %r1, i64 %c1, 1
	ret { i192, i64 } %r2
}

; void mie_fp_sub192
define void @mie_fp_sub192(i192* %pz, i192* %px, i192* %py, i192* %pp) {
entry:
	%x = load i192* %px
	%y = load i192* %py
	%p = load i192* %pp
	%vc = call { i192, i64 } @mie_local_sbb192(i192 %x, i192 %y)
	%v = extractvalue { i192, i64 } %vc, 0 ; x - y
	%c = extractvalue { i192, i64 } %vc, 1
	%t = add i192 %v, %p ; x - y + p
	%zero = icmp eq i64 %c, 0
	br i1 %zero, label %nocarry, label %carry
carry:
	br label %exit
nocarry:
	br label %exit
exit:
	%t1 = phi i192 [%t, %carry], [%v, %nocarry]
	store i192 %t1, i192* %pz
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
	%t0 = add i320 %x1, %y1 ; x + y
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

define { i256, i64 } @mie_local_sbb256(i256 %x, i256 %y) {
	%x1 = zext i256 %x to i320
	%y1 = zext i256 %y to i320
	%v1 = sub i320 %x1, %y1
	%v = trunc i320 %v1 to i256
	%c = lshr i320 %v1, 256
	%c1 = trunc i320 %c to i64
	%r1 = insertvalue { i256, i64 } undef, i256 %v, 0
	%r2 = insertvalue { i256, i64 } %r1, i64 %c1, 1
	ret { i256, i64 } %r2
}

; void mie_fp_sub256
define void @mie_fp_sub256(i256* %pz, i256* %px, i256* %py, i256* %pp) {
entry:
	%x = load i256* %px
	%y = load i256* %py
	%p = load i256* %pp
	%vc = call { i256, i64 } @mie_local_sbb256(i256 %x, i256 %y)
	%v = extractvalue { i256, i64 } %vc, 0 ; x - y
	%c = extractvalue { i256, i64 } %vc, 1
	%t = add i256 %v, %p ; x - y + p
	%zero = icmp eq i64 %c, 0
	br i1 %zero, label %nocarry, label %carry
carry:
	br label %exit
nocarry:
	br label %exit
exit:
	%t1 = phi i256 [%t, %carry], [%v, %nocarry]
	store i256 %t1, i256* %pz
	ret void
}

