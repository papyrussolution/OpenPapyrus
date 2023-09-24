OPTION	DOTNAME
.text$	SEGMENT ALIGN(256) 'CODE'

EXTERN	OPENSSL_ia32cap_P:NEAR

PUBLIC	poly1305_init

PUBLIC	poly1305_blocks

PUBLIC	poly1305_emit



ALIGN	32
poly1305_init	PROC PUBLIC
	mov	QWORD PTR[8+rsp],rdi	;WIN64 prologue
	mov	QWORD PTR[16+rsp],rsi
	mov	rax,rsp
$L$SEH_begin_poly1305_init::
	mov	rdi,rcx
	mov	rsi,rdx
	mov	rdx,r8



	xor	rax,rax
	mov	QWORD PTR[rdi],rax
	mov	QWORD PTR[8+rdi],rax
	mov	QWORD PTR[16+rdi],rax

	cmp	rsi,0
	je	$L$no_key

	lea	r10,QWORD PTR[poly1305_blocks]
	lea	r11,QWORD PTR[poly1305_emit]
	mov	rax,00ffffffc0fffffffh
	mov	rcx,00ffffffc0ffffffch
	and	rax,QWORD PTR[rsi]
	and	rcx,QWORD PTR[8+rsi]
	mov	QWORD PTR[24+rdi],rax
	mov	QWORD PTR[32+rdi],rcx
	mov	QWORD PTR[rdx],r10
	mov	QWORD PTR[8+rdx],r11
	mov	eax,1
$L$no_key::
	mov	rdi,QWORD PTR[8+rsp]	;WIN64 epilogue
	mov	rsi,QWORD PTR[16+rsp]
	DB	0F3h,0C3h		;repret

$L$SEH_end_poly1305_init::
poly1305_init	ENDP


ALIGN	32
poly1305_blocks	PROC PUBLIC
	mov	QWORD PTR[8+rsp],rdi	;WIN64 prologue
	mov	QWORD PTR[16+rsp],rsi
	mov	rax,rsp
$L$SEH_begin_poly1305_blocks::
	mov	rdi,rcx
	mov	rsi,rdx
	mov	rdx,r8
	mov	rcx,r9



$L$blocks::
	shr	rdx,4
	jz	$L$no_data

	push	rbx

	push	rbp

	push	r12

	push	r13

	push	r14

	push	r15

$L$blocks_body::

	mov	r15,rdx

	mov	r11,QWORD PTR[24+rdi]
	mov	r13,QWORD PTR[32+rdi]

	mov	r14,QWORD PTR[rdi]
	mov	rbx,QWORD PTR[8+rdi]
	mov	rbp,QWORD PTR[16+rdi]

	mov	r12,r13
	shr	r13,2
	mov	rax,r12
	add	r13,r12
	jmp	$L$oop

ALIGN	32
$L$oop::
	add	r14,QWORD PTR[rsi]
	adc	rbx,QWORD PTR[8+rsi]
	lea	rsi,QWORD PTR[16+rsi]
	adc	rbp,rcx
	mul	r14
	mov	r9,rax
	mov	rax,r11
	mov	r10,rdx

	mul	r14
	mov	r14,rax
	mov	rax,r11
	mov	r8,rdx

	mul	rbx
	add	r9,rax
	mov	rax,r13
	adc	r10,rdx

	mul	rbx
	mov	rbx,rbp
	add	r14,rax
	adc	r8,rdx

	imul	rbx,r13
	add	r9,rbx
	mov	rbx,r8
	adc	r10,0

	imul	rbp,r11
	add	rbx,r9
	mov	rax,-4
	adc	r10,rbp

	and	rax,r10
	mov	rbp,r10
	shr	r10,2
	and	rbp,3
	add	rax,r10
	add	r14,rax
	adc	rbx,0
	adc	rbp,0
	mov	rax,r12
	dec	r15
	jnz	$L$oop

	mov	QWORD PTR[rdi],r14
	mov	QWORD PTR[8+rdi],rbx
	mov	QWORD PTR[16+rdi],rbp

	mov	r15,QWORD PTR[rsp]

	mov	r14,QWORD PTR[8+rsp]

	mov	r13,QWORD PTR[16+rsp]

	mov	r12,QWORD PTR[24+rsp]

	mov	rbp,QWORD PTR[32+rsp]

	mov	rbx,QWORD PTR[40+rsp]

	lea	rsp,QWORD PTR[48+rsp]

$L$no_data::
$L$blocks_epilogue::
	mov	rdi,QWORD PTR[8+rsp]	;WIN64 epilogue
	mov	rsi,QWORD PTR[16+rsp]
	DB	0F3h,0C3h		;repret

$L$SEH_end_poly1305_blocks::
poly1305_blocks	ENDP


ALIGN	32
poly1305_emit	PROC PUBLIC
	mov	QWORD PTR[8+rsp],rdi	;WIN64 prologue
	mov	QWORD PTR[16+rsp],rsi
	mov	rax,rsp
$L$SEH_begin_poly1305_emit::
	mov	rdi,rcx
	mov	rsi,rdx
	mov	rdx,r8



$L$emit::
	mov	r8,QWORD PTR[rdi]
	mov	r9,QWORD PTR[8+rdi]
	mov	r10,QWORD PTR[16+rdi]

	mov	rax,r8
	add	r8,5
	mov	rcx,r9
	adc	r9,0
	adc	r10,0
	shr	r10,2
	cmovnz	rax,r8
	cmovnz	rcx,r9

	add	rax,QWORD PTR[rdx]
	adc	rcx,QWORD PTR[8+rdx]
	mov	QWORD PTR[rsi],rax
	mov	QWORD PTR[8+rsi],rcx

	mov	rdi,QWORD PTR[8+rsp]	;WIN64 epilogue
	mov	rsi,QWORD PTR[16+rsp]
	DB	0F3h,0C3h		;repret

$L$SEH_end_poly1305_emit::
poly1305_emit	ENDP
DB	80,111,108,121,49,51,48,53,32,102,111,114,32,120,56,54
DB	95,54,52,44,32,67,82,89,80,84,79,71,65,77,83,32
DB	98,121,32,60,97,112,112,114,111,64,111,112,101,110,115,115
DB	108,46,111,114,103,62,0
ALIGN	16
PUBLIC	xor128_encrypt_n_pad

ALIGN	16
xor128_encrypt_n_pad	PROC PUBLIC

	sub	rdx,r8
	sub	rcx,r8
	mov	r10,r9
	shr	r9,4
	jz	$L$tail_enc
	nop
$L$oop_enc_xmm::
	movdqu	xmm0,XMMWORD PTR[r8*1+rdx]
	pxor	xmm0,XMMWORD PTR[r8]
	movdqu	XMMWORD PTR[r8*1+rcx],xmm0
	movdqa	XMMWORD PTR[r8],xmm0
	lea	r8,QWORD PTR[16+r8]
	dec	r9
	jnz	$L$oop_enc_xmm

	and	r10,15
	jz	$L$done_enc

$L$tail_enc::
	mov	r9,16
	sub	r9,r10
	xor	eax,eax
$L$oop_enc_byte::
	mov	al,BYTE PTR[r8*1+rdx]
	xor	al,BYTE PTR[r8]
	mov	BYTE PTR[r8*1+rcx],al
	mov	BYTE PTR[r8],al
	lea	r8,QWORD PTR[1+r8]
	dec	r10
	jnz	$L$oop_enc_byte

	xor	eax,eax
$L$oop_enc_pad::
	mov	BYTE PTR[r8],al
	lea	r8,QWORD PTR[1+r8]
	dec	r9
	jnz	$L$oop_enc_pad

$L$done_enc::
	mov	rax,r8
	DB	0F3h,0C3h		;repret

xor128_encrypt_n_pad	ENDP

PUBLIC	xor128_decrypt_n_pad

ALIGN	16
xor128_decrypt_n_pad	PROC PUBLIC

	sub	rdx,r8
	sub	rcx,r8
	mov	r10,r9
	shr	r9,4
	jz	$L$tail_dec
	nop
$L$oop_dec_xmm::
	movdqu	xmm0,XMMWORD PTR[r8*1+rdx]
	movdqa	xmm1,XMMWORD PTR[r8]
	pxor	xmm1,xmm0
	movdqu	XMMWORD PTR[r8*1+rcx],xmm1
	movdqa	XMMWORD PTR[r8],xmm0
	lea	r8,QWORD PTR[16+r8]
	dec	r9
	jnz	$L$oop_dec_xmm

	pxor	xmm1,xmm1
	and	r10,15
	jz	$L$done_dec

$L$tail_dec::
	mov	r9,16
	sub	r9,r10
	xor	eax,eax
	xor	r11,r11
$L$oop_dec_byte::
	mov	r11b,BYTE PTR[r8*1+rdx]
	mov	al,BYTE PTR[r8]
	xor	al,r11b
	mov	BYTE PTR[r8*1+rcx],al
	mov	BYTE PTR[r8],r11b
	lea	r8,QWORD PTR[1+r8]
	dec	r10
	jnz	$L$oop_dec_byte

	xor	eax,eax
$L$oop_dec_pad::
	mov	BYTE PTR[r8],al
	lea	r8,QWORD PTR[1+r8]
	dec	r9
	jnz	$L$oop_dec_pad

$L$done_dec::
	mov	rax,r8
	DB	0F3h,0C3h		;repret

xor128_decrypt_n_pad	ENDP
EXTERN	__imp_RtlVirtualUnwind:NEAR

ALIGN	16
se_handler	PROC PRIVATE
	push	rsi
	push	rdi
	push	rbx
	push	rbp
	push	r12
	push	r13
	push	r14
	push	r15
	pushfq
	sub	rsp,64

	mov	rax,QWORD PTR[120+r8]
	mov	rbx,QWORD PTR[248+r8]

	mov	rsi,QWORD PTR[8+r9]
	mov	r11,QWORD PTR[56+r9]

	mov	r10d,DWORD PTR[r11]
	lea	r10,QWORD PTR[r10*1+rsi]
	cmp	rbx,r10
	jb	$L$common_seh_tail

	mov	rax,QWORD PTR[152+r8]

	mov	r10d,DWORD PTR[4+r11]
	lea	r10,QWORD PTR[r10*1+rsi]
	cmp	rbx,r10
	jae	$L$common_seh_tail

	lea	rax,QWORD PTR[48+rax]

	mov	rbx,QWORD PTR[((-8))+rax]
	mov	rbp,QWORD PTR[((-16))+rax]
	mov	r12,QWORD PTR[((-24))+rax]
	mov	r13,QWORD PTR[((-32))+rax]
	mov	r14,QWORD PTR[((-40))+rax]
	mov	r15,QWORD PTR[((-48))+rax]
	mov	QWORD PTR[144+r8],rbx
	mov	QWORD PTR[160+r8],rbp
	mov	QWORD PTR[216+r8],r12
	mov	QWORD PTR[224+r8],r13
	mov	QWORD PTR[232+r8],r14
	mov	QWORD PTR[240+r8],r15

	jmp	$L$common_seh_tail
se_handler	ENDP


ALIGN	16
avx_handler	PROC PRIVATE
	push	rsi
	push	rdi
	push	rbx
	push	rbp
	push	r12
	push	r13
	push	r14
	push	r15
	pushfq
	sub	rsp,64

	mov	rax,QWORD PTR[120+r8]
	mov	rbx,QWORD PTR[248+r8]

	mov	rsi,QWORD PTR[8+r9]
	mov	r11,QWORD PTR[56+r9]

	mov	r10d,DWORD PTR[r11]
	lea	r10,QWORD PTR[r10*1+rsi]
	cmp	rbx,r10
	jb	$L$common_seh_tail

	mov	rax,QWORD PTR[152+r8]

	mov	r10d,DWORD PTR[4+r11]
	lea	r10,QWORD PTR[r10*1+rsi]
	cmp	rbx,r10
	jae	$L$common_seh_tail

	mov	rax,QWORD PTR[208+r8]

	lea	rsi,QWORD PTR[80+rax]
	lea	rax,QWORD PTR[248+rax]
	lea	rdi,QWORD PTR[512+r8]
	mov	ecx,20
	DD	0a548f3fch

$L$common_seh_tail::
	mov	rdi,QWORD PTR[8+rax]
	mov	rsi,QWORD PTR[16+rax]
	mov	QWORD PTR[152+r8],rax
	mov	QWORD PTR[168+r8],rsi
	mov	QWORD PTR[176+r8],rdi

	mov	rdi,QWORD PTR[40+r9]
	mov	rsi,r8
	mov	ecx,154
	DD	0a548f3fch

	mov	rsi,r9
	xor	rcx,rcx
	mov	rdx,QWORD PTR[8+rsi]
	mov	r8,QWORD PTR[rsi]
	mov	r9,QWORD PTR[16+rsi]
	mov	r10,QWORD PTR[40+rsi]
	lea	r11,QWORD PTR[56+rsi]
	lea	r12,QWORD PTR[24+rsi]
	mov	QWORD PTR[32+rsp],r10
	mov	QWORD PTR[40+rsp],r11
	mov	QWORD PTR[48+rsp],r12
	mov	QWORD PTR[56+rsp],rcx
	call	QWORD PTR[__imp_RtlVirtualUnwind]

	mov	eax,1
	add	rsp,64
	popfq
	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	rbp
	pop	rbx
	pop	rdi
	pop	rsi
	DB	0F3h,0C3h		;repret
avx_handler	ENDP

.text$	ENDS
.pdata	SEGMENT READONLY ALIGN(4)
ALIGN	4
	DD	imagerel $L$SEH_begin_poly1305_init
	DD	imagerel $L$SEH_end_poly1305_init
	DD	imagerel $L$SEH_info_poly1305_init

	DD	imagerel $L$SEH_begin_poly1305_blocks
	DD	imagerel $L$SEH_end_poly1305_blocks
	DD	imagerel $L$SEH_info_poly1305_blocks

	DD	imagerel $L$SEH_begin_poly1305_emit
	DD	imagerel $L$SEH_end_poly1305_emit
	DD	imagerel $L$SEH_info_poly1305_emit
.pdata	ENDS
.xdata	SEGMENT READONLY ALIGN(8)
ALIGN	8
$L$SEH_info_poly1305_init::
DB	9,0,0,0
	DD	imagerel se_handler
	DD	imagerel $L$SEH_begin_poly1305_init,imagerel $L$SEH_begin_poly1305_init

$L$SEH_info_poly1305_blocks::
DB	9,0,0,0
	DD	imagerel se_handler
	DD	imagerel $L$blocks_body,imagerel $L$blocks_epilogue

$L$SEH_info_poly1305_emit::
DB	9,0,0,0
	DD	imagerel se_handler
	DD	imagerel $L$SEH_begin_poly1305_emit,imagerel $L$SEH_begin_poly1305_emit

.xdata	ENDS
END
