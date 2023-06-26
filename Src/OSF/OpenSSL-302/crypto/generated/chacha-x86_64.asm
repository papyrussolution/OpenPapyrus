OPTION	DOTNAME
.text$	SEGMENT ALIGN(256) 'CODE'

EXTERN	OPENSSL_ia32cap_P:NEAR

ALIGN	64
$L$zero::
	DD	0,0,0,0
$L$one::
	DD	1,0,0,0
$L$inc::
	DD	0,1,2,3
$L$four::
	DD	4,4,4,4
$L$incy::
	DD	0,2,4,6,1,3,5,7
$L$eight::
	DD	8,8,8,8,8,8,8,8
$L$rot16::
DB	02h,03h,00h,01h,06h,07h,04h,05h,0ah,0bh,08h,09h,0eh,0fh,0ch,0dh
$L$rot24::
DB	03h,00h,01h,02h,07h,04h,05h,06h,0bh,08h,09h,0ah,0fh,0ch,0dh,0eh
$L$twoy::
	DD	2,0,0,0,2,0,0,0
ALIGN	64
$L$zeroz::
	DD	0,0,0,0,1,0,0,0,2,0,0,0,3,0,0,0
$L$fourz::
	DD	4,0,0,0,4,0,0,0,4,0,0,0,4,0,0,0
$L$incz::
	DD	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
$L$sixteen::
	DD	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16
$L$sigma::
DB	101,120,112,97,110,100,32,51,50,45,98,121,116,101,32,107
DB	0
DB	67,104,97,67,104,97,50,48,32,102,111,114,32,120,56,54
DB	95,54,52,44,32,67,82,89,80,84,79,71,65,77,83,32
DB	98,121,32,60,97,112,112,114,111,64,111,112,101,110,115,115
DB	108,46,111,114,103,62,0
PUBLIC	ChaCha20_ctr32

ALIGN	64
ChaCha20_ctr32	PROC PUBLIC
	mov	QWORD PTR[8+rsp],rdi	;WIN64 prologue
	mov	QWORD PTR[16+rsp],rsi
	mov	rax,rsp
$L$SEH_begin_ChaCha20_ctr32::
	mov	rdi,rcx
	mov	rsi,rdx
	mov	rdx,r8
	mov	rcx,r9
	mov	r8,QWORD PTR[40+rsp]



	cmp	rdx,0
	je	$L$no_data
	mov	r10,QWORD PTR[((OPENSSL_ia32cap_P+4))]
	test	r10d,512
	jnz	$L$ChaCha20_ssse3

	push	rbx

	push	rbp

	push	r12

	push	r13

	push	r14

	push	r15

	sub	rsp,64+24

$L$ctr32_body::


	movdqu	xmm1,XMMWORD PTR[rcx]
	movdqu	xmm2,XMMWORD PTR[16+rcx]
	movdqu	xmm3,XMMWORD PTR[r8]
	movdqa	xmm4,XMMWORD PTR[$L$one]


	movdqa	XMMWORD PTR[16+rsp],xmm1
	movdqa	XMMWORD PTR[32+rsp],xmm2
	movdqa	XMMWORD PTR[48+rsp],xmm3
	mov	rbp,rdx
	jmp	$L$oop_outer

ALIGN	32
$L$oop_outer::
	mov	eax,061707865h
	mov	ebx,03320646eh
	mov	ecx,079622d32h
	mov	edx,06b206574h
	mov	r8d,DWORD PTR[16+rsp]
	mov	r9d,DWORD PTR[20+rsp]
	mov	r10d,DWORD PTR[24+rsp]
	mov	r11d,DWORD PTR[28+rsp]
	movd	r12d,xmm3
	mov	r13d,DWORD PTR[52+rsp]
	mov	r14d,DWORD PTR[56+rsp]
	mov	r15d,DWORD PTR[60+rsp]

	mov	QWORD PTR[((64+0))+rsp],rbp
	mov	ebp,10
	mov	QWORD PTR[((64+8))+rsp],rsi
DB	102,72,15,126,214
	mov	QWORD PTR[((64+16))+rsp],rdi
	mov	rdi,rsi
	shr	rdi,32
	jmp	$L$oop

ALIGN	32
$L$oop::
	add	eax,r8d
	xor	r12d,eax
	rol	r12d,16
	add	ebx,r9d
	xor	r13d,ebx
	rol	r13d,16
	add	esi,r12d
	xor	r8d,esi
	rol	r8d,12
	add	edi,r13d
	xor	r9d,edi
	rol	r9d,12
	add	eax,r8d
	xor	r12d,eax
	rol	r12d,8
	add	ebx,r9d
	xor	r13d,ebx
	rol	r13d,8
	add	esi,r12d
	xor	r8d,esi
	rol	r8d,7
	add	edi,r13d
	xor	r9d,edi
	rol	r9d,7
	mov	DWORD PTR[32+rsp],esi
	mov	DWORD PTR[36+rsp],edi
	mov	esi,DWORD PTR[40+rsp]
	mov	edi,DWORD PTR[44+rsp]
	add	ecx,r10d
	xor	r14d,ecx
	rol	r14d,16
	add	edx,r11d
	xor	r15d,edx
	rol	r15d,16
	add	esi,r14d
	xor	r10d,esi
	rol	r10d,12
	add	edi,r15d
	xor	r11d,edi
	rol	r11d,12
	add	ecx,r10d
	xor	r14d,ecx
	rol	r14d,8
	add	edx,r11d
	xor	r15d,edx
	rol	r15d,8
	add	esi,r14d
	xor	r10d,esi
	rol	r10d,7
	add	edi,r15d
	xor	r11d,edi
	rol	r11d,7
	add	eax,r9d
	xor	r15d,eax
	rol	r15d,16
	add	ebx,r10d
	xor	r12d,ebx
	rol	r12d,16
	add	esi,r15d
	xor	r9d,esi
	rol	r9d,12
	add	edi,r12d
	xor	r10d,edi
	rol	r10d,12
	add	eax,r9d
	xor	r15d,eax
	rol	r15d,8
	add	ebx,r10d
	xor	r12d,ebx
	rol	r12d,8
	add	esi,r15d
	xor	r9d,esi
	rol	r9d,7
	add	edi,r12d
	xor	r10d,edi
	rol	r10d,7
	mov	DWORD PTR[40+rsp],esi
	mov	DWORD PTR[44+rsp],edi
	mov	esi,DWORD PTR[32+rsp]
	mov	edi,DWORD PTR[36+rsp]
	add	ecx,r11d
	xor	r13d,ecx
	rol	r13d,16
	add	edx,r8d
	xor	r14d,edx
	rol	r14d,16
	add	esi,r13d
	xor	r11d,esi
	rol	r11d,12
	add	edi,r14d
	xor	r8d,edi
	rol	r8d,12
	add	ecx,r11d
	xor	r13d,ecx
	rol	r13d,8
	add	edx,r8d
	xor	r14d,edx
	rol	r14d,8
	add	esi,r13d
	xor	r11d,esi
	rol	r11d,7
	add	edi,r14d
	xor	r8d,edi
	rol	r8d,7
	dec	ebp
	jnz	$L$oop
	mov	DWORD PTR[36+rsp],edi
	mov	DWORD PTR[32+rsp],esi
	mov	rbp,QWORD PTR[64+rsp]
	movdqa	xmm1,xmm2
	mov	rsi,QWORD PTR[((64+8))+rsp]
	paddd	xmm3,xmm4
	mov	rdi,QWORD PTR[((64+16))+rsp]

	add	eax,061707865h
	add	ebx,03320646eh
	add	ecx,079622d32h
	add	edx,06b206574h
	add	r8d,DWORD PTR[16+rsp]
	add	r9d,DWORD PTR[20+rsp]
	add	r10d,DWORD PTR[24+rsp]
	add	r11d,DWORD PTR[28+rsp]
	add	r12d,DWORD PTR[48+rsp]
	add	r13d,DWORD PTR[52+rsp]
	add	r14d,DWORD PTR[56+rsp]
	add	r15d,DWORD PTR[60+rsp]
	paddd	xmm1,XMMWORD PTR[32+rsp]

	cmp	rbp,64
	jb	$L$tail

	xor	eax,DWORD PTR[rsi]
	xor	ebx,DWORD PTR[4+rsi]
	xor	ecx,DWORD PTR[8+rsi]
	xor	edx,DWORD PTR[12+rsi]
	xor	r8d,DWORD PTR[16+rsi]
	xor	r9d,DWORD PTR[20+rsi]
	xor	r10d,DWORD PTR[24+rsi]
	xor	r11d,DWORD PTR[28+rsi]
	movdqu	xmm0,XMMWORD PTR[32+rsi]
	xor	r12d,DWORD PTR[48+rsi]
	xor	r13d,DWORD PTR[52+rsi]
	xor	r14d,DWORD PTR[56+rsi]
	xor	r15d,DWORD PTR[60+rsi]
	lea	rsi,QWORD PTR[64+rsi]
	pxor	xmm0,xmm1

	movdqa	XMMWORD PTR[32+rsp],xmm2
	movd	DWORD PTR[48+rsp],xmm3

	mov	DWORD PTR[rdi],eax
	mov	DWORD PTR[4+rdi],ebx
	mov	DWORD PTR[8+rdi],ecx
	mov	DWORD PTR[12+rdi],edx
	mov	DWORD PTR[16+rdi],r8d
	mov	DWORD PTR[20+rdi],r9d
	mov	DWORD PTR[24+rdi],r10d
	mov	DWORD PTR[28+rdi],r11d
	movdqu	XMMWORD PTR[32+rdi],xmm0
	mov	DWORD PTR[48+rdi],r12d
	mov	DWORD PTR[52+rdi],r13d
	mov	DWORD PTR[56+rdi],r14d
	mov	DWORD PTR[60+rdi],r15d
	lea	rdi,QWORD PTR[64+rdi]

	sub	rbp,64
	jnz	$L$oop_outer

	jmp	$L$done

ALIGN	16
$L$tail::
	mov	DWORD PTR[rsp],eax
	mov	DWORD PTR[4+rsp],ebx
	xor	rbx,rbx
	mov	DWORD PTR[8+rsp],ecx
	mov	DWORD PTR[12+rsp],edx
	mov	DWORD PTR[16+rsp],r8d
	mov	DWORD PTR[20+rsp],r9d
	mov	DWORD PTR[24+rsp],r10d
	mov	DWORD PTR[28+rsp],r11d
	movdqa	XMMWORD PTR[32+rsp],xmm1
	mov	DWORD PTR[48+rsp],r12d
	mov	DWORD PTR[52+rsp],r13d
	mov	DWORD PTR[56+rsp],r14d
	mov	DWORD PTR[60+rsp],r15d

$L$oop_tail::
	movzx	eax,BYTE PTR[rbx*1+rsi]
	movzx	edx,BYTE PTR[rbx*1+rsp]
	lea	rbx,QWORD PTR[1+rbx]
	xor	eax,edx
	mov	BYTE PTR[((-1))+rbx*1+rdi],al
	dec	rbp
	jnz	$L$oop_tail

$L$done::
	lea	rsi,QWORD PTR[((64+24+48))+rsp]

	mov	r15,QWORD PTR[((-48))+rsi]

	mov	r14,QWORD PTR[((-40))+rsi]

	mov	r13,QWORD PTR[((-32))+rsi]

	mov	r12,QWORD PTR[((-24))+rsi]

	mov	rbp,QWORD PTR[((-16))+rsi]

	mov	rbx,QWORD PTR[((-8))+rsi]

	lea	rsp,QWORD PTR[rsi]

$L$no_data::
	mov	rdi,QWORD PTR[8+rsp]	;WIN64 epilogue
	mov	rsi,QWORD PTR[16+rsp]
	DB	0F3h,0C3h		;repret

$L$SEH_end_ChaCha20_ctr32::
ChaCha20_ctr32	ENDP

ALIGN	32
ChaCha20_ssse3	PROC PRIVATE
	mov	QWORD PTR[8+rsp],rdi	;WIN64 prologue
	mov	QWORD PTR[16+rsp],rsi
	mov	rax,rsp
$L$SEH_begin_ChaCha20_ssse3::
	mov	rdi,rcx
	mov	rsi,rdx
	mov	rdx,r8
	mov	rcx,r9
	mov	r8,QWORD PTR[40+rsp]



$L$ChaCha20_ssse3::
	mov	r9,rsp

	cmp	rdx,128
	je	$L$ChaCha20_128
	ja	$L$ChaCha20_4x

$L$do_sse3_after_all::
	sub	rsp,64+168
	movaps	XMMWORD PTR[(-40)+r9],xmm6
	movaps	XMMWORD PTR[(-24)+r9],xmm7
$L$ssse3_body::
	movdqa	xmm0,XMMWORD PTR[$L$sigma]
	movdqu	xmm1,XMMWORD PTR[rcx]
	movdqu	xmm2,XMMWORD PTR[16+rcx]
	movdqu	xmm3,XMMWORD PTR[r8]
	movdqa	xmm6,XMMWORD PTR[$L$rot16]
	movdqa	xmm7,XMMWORD PTR[$L$rot24]

	movdqa	XMMWORD PTR[rsp],xmm0
	movdqa	XMMWORD PTR[16+rsp],xmm1
	movdqa	XMMWORD PTR[32+rsp],xmm2
	movdqa	XMMWORD PTR[48+rsp],xmm3
	mov	r8,10
	jmp	$L$oop_ssse3

ALIGN	32
$L$oop_outer_ssse3::
	movdqa	xmm3,XMMWORD PTR[$L$one]
	movdqa	xmm0,XMMWORD PTR[rsp]
	movdqa	xmm1,XMMWORD PTR[16+rsp]
	movdqa	xmm2,XMMWORD PTR[32+rsp]
	paddd	xmm3,XMMWORD PTR[48+rsp]
	mov	r8,10
	movdqa	XMMWORD PTR[48+rsp],xmm3
	jmp	$L$oop_ssse3

ALIGN	32
$L$oop_ssse3::
	paddd	xmm0,xmm1
	pxor	xmm3,xmm0
DB	102,15,56,0,222
	paddd	xmm2,xmm3
	pxor	xmm1,xmm2
	movdqa	xmm4,xmm1
	psrld	xmm1,20
	pslld	xmm4,12
	por	xmm1,xmm4
	paddd	xmm0,xmm1
	pxor	xmm3,xmm0
DB	102,15,56,0,223
	paddd	xmm2,xmm3
	pxor	xmm1,xmm2
	movdqa	xmm4,xmm1
	psrld	xmm1,25
	pslld	xmm4,7
	por	xmm1,xmm4
	pshufd	xmm2,xmm2,78
	pshufd	xmm1,xmm1,57
	pshufd	xmm3,xmm3,147
	nop
	paddd	xmm0,xmm1
	pxor	xmm3,xmm0
DB	102,15,56,0,222
	paddd	xmm2,xmm3
	pxor	xmm1,xmm2
	movdqa	xmm4,xmm1
	psrld	xmm1,20
	pslld	xmm4,12
	por	xmm1,xmm4
	paddd	xmm0,xmm1
	pxor	xmm3,xmm0
DB	102,15,56,0,223
	paddd	xmm2,xmm3
	pxor	xmm1,xmm2
	movdqa	xmm4,xmm1
	psrld	xmm1,25
	pslld	xmm4,7
	por	xmm1,xmm4
	pshufd	xmm2,xmm2,78
	pshufd	xmm1,xmm1,147
	pshufd	xmm3,xmm3,57
	dec	r8
	jnz	$L$oop_ssse3
	paddd	xmm0,XMMWORD PTR[rsp]
	paddd	xmm1,XMMWORD PTR[16+rsp]
	paddd	xmm2,XMMWORD PTR[32+rsp]
	paddd	xmm3,XMMWORD PTR[48+rsp]

	cmp	rdx,64
	jb	$L$tail_ssse3

	movdqu	xmm4,XMMWORD PTR[rsi]
	movdqu	xmm5,XMMWORD PTR[16+rsi]
	pxor	xmm0,xmm4
	movdqu	xmm4,XMMWORD PTR[32+rsi]
	pxor	xmm1,xmm5
	movdqu	xmm5,XMMWORD PTR[48+rsi]
	lea	rsi,QWORD PTR[64+rsi]
	pxor	xmm2,xmm4
	pxor	xmm3,xmm5

	movdqu	XMMWORD PTR[rdi],xmm0
	movdqu	XMMWORD PTR[16+rdi],xmm1
	movdqu	XMMWORD PTR[32+rdi],xmm2
	movdqu	XMMWORD PTR[48+rdi],xmm3
	lea	rdi,QWORD PTR[64+rdi]

	sub	rdx,64
	jnz	$L$oop_outer_ssse3

	jmp	$L$done_ssse3

ALIGN	16
$L$tail_ssse3::
	movdqa	XMMWORD PTR[rsp],xmm0
	movdqa	XMMWORD PTR[16+rsp],xmm1
	movdqa	XMMWORD PTR[32+rsp],xmm2
	movdqa	XMMWORD PTR[48+rsp],xmm3
	xor	r8,r8

$L$oop_tail_ssse3::
	movzx	eax,BYTE PTR[r8*1+rsi]
	movzx	ecx,BYTE PTR[r8*1+rsp]
	lea	r8,QWORD PTR[1+r8]
	xor	eax,ecx
	mov	BYTE PTR[((-1))+r8*1+rdi],al
	dec	rdx
	jnz	$L$oop_tail_ssse3

$L$done_ssse3::
	movaps	xmm6,XMMWORD PTR[((-40))+r9]
	movaps	xmm7,XMMWORD PTR[((-24))+r9]
	lea	rsp,QWORD PTR[r9]

$L$ssse3_epilogue::
	mov	rdi,QWORD PTR[8+rsp]	;WIN64 epilogue
	mov	rsi,QWORD PTR[16+rsp]
	DB	0F3h,0C3h		;repret

$L$SEH_end_ChaCha20_ssse3::
ChaCha20_ssse3	ENDP

ALIGN	32
ChaCha20_128	PROC PRIVATE
	mov	QWORD PTR[8+rsp],rdi	;WIN64 prologue
	mov	QWORD PTR[16+rsp],rsi
	mov	rax,rsp
$L$SEH_begin_ChaCha20_128::
	mov	rdi,rcx
	mov	rsi,rdx
	mov	rdx,r8
	mov	rcx,r9
	mov	r8,QWORD PTR[40+rsp]



$L$ChaCha20_128::
	mov	r9,rsp

	sub	rsp,64+104
	movaps	XMMWORD PTR[(-104)+r9],xmm6
	movaps	XMMWORD PTR[(-88)+r9],xmm7
	movaps	XMMWORD PTR[(-72)+r9],xmm8
	movaps	XMMWORD PTR[(-56)+r9],xmm9
	movaps	XMMWORD PTR[(-40)+r9],xmm10
	movaps	XMMWORD PTR[(-24)+r9],xmm11
$L$128_body::
	movdqa	xmm8,XMMWORD PTR[$L$sigma]
	movdqu	xmm9,XMMWORD PTR[rcx]
	movdqu	xmm2,XMMWORD PTR[16+rcx]
	movdqu	xmm3,XMMWORD PTR[r8]
	movdqa	xmm1,XMMWORD PTR[$L$one]
	movdqa	xmm6,XMMWORD PTR[$L$rot16]
	movdqa	xmm7,XMMWORD PTR[$L$rot24]

	movdqa	xmm10,xmm8
	movdqa	XMMWORD PTR[rsp],xmm8
	movdqa	xmm11,xmm9
	movdqa	XMMWORD PTR[16+rsp],xmm9
	movdqa	xmm0,xmm2
	movdqa	XMMWORD PTR[32+rsp],xmm2
	paddd	xmm1,xmm3
	movdqa	XMMWORD PTR[48+rsp],xmm3
	mov	r8,10
	jmp	$L$oop_128

ALIGN	32
$L$oop_128::
	paddd	xmm8,xmm9
	pxor	xmm3,xmm8
	paddd	xmm10,xmm11
	pxor	xmm1,xmm10
DB	102,15,56,0,222
DB	102,15,56,0,206
	paddd	xmm2,xmm3
	paddd	xmm0,xmm1
	pxor	xmm9,xmm2
	pxor	xmm11,xmm0
	movdqa	xmm4,xmm9
	psrld	xmm9,20
	movdqa	xmm5,xmm11
	pslld	xmm4,12
	psrld	xmm11,20
	por	xmm9,xmm4
	pslld	xmm5,12
	por	xmm11,xmm5
	paddd	xmm8,xmm9
	pxor	xmm3,xmm8
	paddd	xmm10,xmm11
	pxor	xmm1,xmm10
DB	102,15,56,0,223
DB	102,15,56,0,207
	paddd	xmm2,xmm3
	paddd	xmm0,xmm1
	pxor	xmm9,xmm2
	pxor	xmm11,xmm0
	movdqa	xmm4,xmm9
	psrld	xmm9,25
	movdqa	xmm5,xmm11
	pslld	xmm4,7
	psrld	xmm11,25
	por	xmm9,xmm4
	pslld	xmm5,7
	por	xmm11,xmm5
	pshufd	xmm2,xmm2,78
	pshufd	xmm9,xmm9,57
	pshufd	xmm3,xmm3,147
	pshufd	xmm0,xmm0,78
	pshufd	xmm11,xmm11,57
	pshufd	xmm1,xmm1,147
	paddd	xmm8,xmm9
	pxor	xmm3,xmm8
	paddd	xmm10,xmm11
	pxor	xmm1,xmm10
DB	102,15,56,0,222
DB	102,15,56,0,206
	paddd	xmm2,xmm3
	paddd	xmm0,xmm1
	pxor	xmm9,xmm2
	pxor	xmm11,xmm0
	movdqa	xmm4,xmm9
	psrld	xmm9,20
	movdqa	xmm5,xmm11
	pslld	xmm4,12
	psrld	xmm11,20
	por	xmm9,xmm4
	pslld	xmm5,12
	por	xmm11,xmm5
	paddd	xmm8,xmm9
	pxor	xmm3,xmm8
	paddd	xmm10,xmm11
	pxor	xmm1,xmm10
DB	102,15,56,0,223
DB	102,15,56,0,207
	paddd	xmm2,xmm3
	paddd	xmm0,xmm1
	pxor	xmm9,xmm2
	pxor	xmm11,xmm0
	movdqa	xmm4,xmm9
	psrld	xmm9,25
	movdqa	xmm5,xmm11
	pslld	xmm4,7
	psrld	xmm11,25
	por	xmm9,xmm4
	pslld	xmm5,7
	por	xmm11,xmm5
	pshufd	xmm2,xmm2,78
	pshufd	xmm9,xmm9,147
	pshufd	xmm3,xmm3,57
	pshufd	xmm0,xmm0,78
	pshufd	xmm11,xmm11,147
	pshufd	xmm1,xmm1,57
	dec	r8
	jnz	$L$oop_128
	paddd	xmm8,XMMWORD PTR[rsp]
	paddd	xmm9,XMMWORD PTR[16+rsp]
	paddd	xmm2,XMMWORD PTR[32+rsp]
	paddd	xmm3,XMMWORD PTR[48+rsp]
	paddd	xmm1,XMMWORD PTR[$L$one]
	paddd	xmm10,XMMWORD PTR[rsp]
	paddd	xmm11,XMMWORD PTR[16+rsp]
	paddd	xmm0,XMMWORD PTR[32+rsp]
	paddd	xmm1,XMMWORD PTR[48+rsp]

	movdqu	xmm4,XMMWORD PTR[rsi]
	movdqu	xmm5,XMMWORD PTR[16+rsi]
	pxor	xmm8,xmm4
	movdqu	xmm4,XMMWORD PTR[32+rsi]
	pxor	xmm9,xmm5
	movdqu	xmm5,XMMWORD PTR[48+rsi]
	pxor	xmm2,xmm4
	movdqu	xmm4,XMMWORD PTR[64+rsi]
	pxor	xmm3,xmm5
	movdqu	xmm5,XMMWORD PTR[80+rsi]
	pxor	xmm10,xmm4
	movdqu	xmm4,XMMWORD PTR[96+rsi]
	pxor	xmm11,xmm5
	movdqu	xmm5,XMMWORD PTR[112+rsi]
	pxor	xmm0,xmm4
	pxor	xmm1,xmm5

	movdqu	XMMWORD PTR[rdi],xmm8
	movdqu	XMMWORD PTR[16+rdi],xmm9
	movdqu	XMMWORD PTR[32+rdi],xmm2
	movdqu	XMMWORD PTR[48+rdi],xmm3
	movdqu	XMMWORD PTR[64+rdi],xmm10
	movdqu	XMMWORD PTR[80+rdi],xmm11
	movdqu	XMMWORD PTR[96+rdi],xmm0
	movdqu	XMMWORD PTR[112+rdi],xmm1
	movaps	xmm6,XMMWORD PTR[((-104))+r9]
	movaps	xmm7,XMMWORD PTR[((-88))+r9]
	movaps	xmm8,XMMWORD PTR[((-72))+r9]
	movaps	xmm9,XMMWORD PTR[((-56))+r9]
	movaps	xmm10,XMMWORD PTR[((-40))+r9]
	movaps	xmm11,XMMWORD PTR[((-24))+r9]
	lea	rsp,QWORD PTR[r9]

$L$128_epilogue::
	mov	rdi,QWORD PTR[8+rsp]	;WIN64 epilogue
	mov	rsi,QWORD PTR[16+rsp]
	DB	0F3h,0C3h		;repret

$L$SEH_end_ChaCha20_128::
ChaCha20_128	ENDP

ALIGN	32
ChaCha20_4x	PROC PRIVATE
	mov	QWORD PTR[8+rsp],rdi	;WIN64 prologue
	mov	QWORD PTR[16+rsp],rsi
	mov	rax,rsp
$L$SEH_begin_ChaCha20_4x::
	mov	rdi,rcx
	mov	rsi,rdx
	mov	rdx,r8
	mov	rcx,r9
	mov	r8,QWORD PTR[40+rsp]



$L$ChaCha20_4x::
	mov	r9,rsp

	mov	r11,r10
	cmp	rdx,192
	ja	$L$proceed4x

	and	r11,71303168
	cmp	r11,4194304
	je	$L$do_sse3_after_all

$L$proceed4x::
	sub	rsp,0140h+168
	movaps	XMMWORD PTR[(-168)+r9],xmm6
	movaps	XMMWORD PTR[(-152)+r9],xmm7
	movaps	XMMWORD PTR[(-136)+r9],xmm8
	movaps	XMMWORD PTR[(-120)+r9],xmm9
	movaps	XMMWORD PTR[(-104)+r9],xmm10
	movaps	XMMWORD PTR[(-88)+r9],xmm11
	movaps	XMMWORD PTR[(-72)+r9],xmm12
	movaps	XMMWORD PTR[(-56)+r9],xmm13
	movaps	XMMWORD PTR[(-40)+r9],xmm14
	movaps	XMMWORD PTR[(-24)+r9],xmm15
$L$4x_body::
	movdqa	xmm11,XMMWORD PTR[$L$sigma]
	movdqu	xmm15,XMMWORD PTR[rcx]
	movdqu	xmm7,XMMWORD PTR[16+rcx]
	movdqu	xmm3,XMMWORD PTR[r8]
	lea	rcx,QWORD PTR[256+rsp]
	lea	r10,QWORD PTR[$L$rot16]
	lea	r11,QWORD PTR[$L$rot24]

	pshufd	xmm8,xmm11,000h
	pshufd	xmm9,xmm11,055h
	movdqa	XMMWORD PTR[64+rsp],xmm8
	pshufd	xmm10,xmm11,0aah
	movdqa	XMMWORD PTR[80+rsp],xmm9
	pshufd	xmm11,xmm11,0ffh
	movdqa	XMMWORD PTR[96+rsp],xmm10
	movdqa	XMMWORD PTR[112+rsp],xmm11

	pshufd	xmm12,xmm15,000h
	pshufd	xmm13,xmm15,055h
	movdqa	XMMWORD PTR[(128-256)+rcx],xmm12
	pshufd	xmm14,xmm15,0aah
	movdqa	XMMWORD PTR[(144-256)+rcx],xmm13
	pshufd	xmm15,xmm15,0ffh
	movdqa	XMMWORD PTR[(160-256)+rcx],xmm14
	movdqa	XMMWORD PTR[(176-256)+rcx],xmm15

	pshufd	xmm4,xmm7,000h
	pshufd	xmm5,xmm7,055h
	movdqa	XMMWORD PTR[(192-256)+rcx],xmm4
	pshufd	xmm6,xmm7,0aah
	movdqa	XMMWORD PTR[(208-256)+rcx],xmm5
	pshufd	xmm7,xmm7,0ffh
	movdqa	XMMWORD PTR[(224-256)+rcx],xmm6
	movdqa	XMMWORD PTR[(240-256)+rcx],xmm7

	pshufd	xmm0,xmm3,000h
	pshufd	xmm1,xmm3,055h
	paddd	xmm0,XMMWORD PTR[$L$inc]
	pshufd	xmm2,xmm3,0aah
	movdqa	XMMWORD PTR[(272-256)+rcx],xmm1
	pshufd	xmm3,xmm3,0ffh
	movdqa	XMMWORD PTR[(288-256)+rcx],xmm2
	movdqa	XMMWORD PTR[(304-256)+rcx],xmm3

	jmp	$L$oop_enter4x

ALIGN	32
$L$oop_outer4x::
	movdqa	xmm8,XMMWORD PTR[64+rsp]
	movdqa	xmm9,XMMWORD PTR[80+rsp]
	movdqa	xmm10,XMMWORD PTR[96+rsp]
	movdqa	xmm11,XMMWORD PTR[112+rsp]
	movdqa	xmm12,XMMWORD PTR[((128-256))+rcx]
	movdqa	xmm13,XMMWORD PTR[((144-256))+rcx]
	movdqa	xmm14,XMMWORD PTR[((160-256))+rcx]
	movdqa	xmm15,XMMWORD PTR[((176-256))+rcx]
	movdqa	xmm4,XMMWORD PTR[((192-256))+rcx]
	movdqa	xmm5,XMMWORD PTR[((208-256))+rcx]
	movdqa	xmm6,XMMWORD PTR[((224-256))+rcx]
	movdqa	xmm7,XMMWORD PTR[((240-256))+rcx]
	movdqa	xmm0,XMMWORD PTR[((256-256))+rcx]
	movdqa	xmm1,XMMWORD PTR[((272-256))+rcx]
	movdqa	xmm2,XMMWORD PTR[((288-256))+rcx]
	movdqa	xmm3,XMMWORD PTR[((304-256))+rcx]
	paddd	xmm0,XMMWORD PTR[$L$four]

$L$oop_enter4x::
	movdqa	XMMWORD PTR[32+rsp],xmm6
	movdqa	XMMWORD PTR[48+rsp],xmm7
	movdqa	xmm7,XMMWORD PTR[r10]
	mov	eax,10
	movdqa	XMMWORD PTR[(256-256)+rcx],xmm0
	jmp	$L$oop4x

ALIGN	32
$L$oop4x::
	paddd	xmm8,xmm12
	paddd	xmm9,xmm13
	pxor	xmm0,xmm8
	pxor	xmm1,xmm9
DB	102,15,56,0,199
DB	102,15,56,0,207
	paddd	xmm4,xmm0
	paddd	xmm5,xmm1
	pxor	xmm12,xmm4
	pxor	xmm13,xmm5
	movdqa	xmm6,xmm12
	pslld	xmm12,12
	psrld	xmm6,20
	movdqa	xmm7,xmm13
	pslld	xmm13,12
	por	xmm12,xmm6
	psrld	xmm7,20
	movdqa	xmm6,XMMWORD PTR[r11]
	por	xmm13,xmm7
	paddd	xmm8,xmm12
	paddd	xmm9,xmm13
	pxor	xmm0,xmm8
	pxor	xmm1,xmm9
DB	102,15,56,0,198
DB	102,15,56,0,206
	paddd	xmm4,xmm0
	paddd	xmm5,xmm1
	pxor	xmm12,xmm4
	pxor	xmm13,xmm5
	movdqa	xmm7,xmm12
	pslld	xmm12,7
	psrld	xmm7,25
	movdqa	xmm6,xmm13
	pslld	xmm13,7
	por	xmm12,xmm7
	psrld	xmm6,25
	movdqa	xmm7,XMMWORD PTR[r10]
	por	xmm13,xmm6
	movdqa	XMMWORD PTR[rsp],xmm4
	movdqa	XMMWORD PTR[16+rsp],xmm5
	movdqa	xmm4,XMMWORD PTR[32+rsp]
	movdqa	xmm5,XMMWORD PTR[48+rsp]
	paddd	xmm10,xmm14
	paddd	xmm11,xmm15
	pxor	xmm2,xmm10
	pxor	xmm3,xmm11
DB	102,15,56,0,215
DB	102,15,56,0,223
	paddd	xmm4,xmm2
	paddd	xmm5,xmm3
	pxor	xmm14,xmm4
	pxor	xmm15,xmm5
	movdqa	xmm6,xmm14
	pslld	xmm14,12
	psrld	xmm6,20
	movdqa	xmm7,xmm15
	pslld	xmm15,12
	por	xmm14,xmm6
	psrld	xmm7,20
	movdqa	xmm6,XMMWORD PTR[r11]
	por	xmm15,xmm7
	paddd	xmm10,xmm14
	paddd	xmm11,xmm15
	pxor	xmm2,xmm10
	pxor	xmm3,xmm11
DB	102,15,56,0,214
DB	102,15,56,0,222
	paddd	xmm4,xmm2
	paddd	xmm5,xmm3
	pxor	xmm14,xmm4
	pxor	xmm15,xmm5
	movdqa	xmm7,xmm14
	pslld	xmm14,7
	psrld	xmm7,25
	movdqa	xmm6,xmm15
	pslld	xmm15,7
	por	xmm14,xmm7
	psrld	xmm6,25
	movdqa	xmm7,XMMWORD PTR[r10]
	por	xmm15,xmm6
	paddd	xmm8,xmm13
	paddd	xmm9,xmm14
	pxor	xmm3,xmm8
	pxor	xmm0,xmm9
DB	102,15,56,0,223
DB	102,15,56,0,199
	paddd	xmm4,xmm3
	paddd	xmm5,xmm0
	pxor	xmm13,xmm4
	pxor	xmm14,xmm5
	movdqa	xmm6,xmm13
	pslld	xmm13,12
	psrld	xmm6,20
	movdqa	xmm7,xmm14
	pslld	xmm14,12
	por	xmm13,xmm6
	psrld	xmm7,20
	movdqa	xmm6,XMMWORD PTR[r11]
	por	xmm14,xmm7
	paddd	xmm8,xmm13
	paddd	xmm9,xmm14
	pxor	xmm3,xmm8
	pxor	xmm0,xmm9
DB	102,15,56,0,222
DB	102,15,56,0,198
	paddd	xmm4,xmm3
	paddd	xmm5,xmm0
	pxor	xmm13,xmm4
	pxor	xmm14,xmm5
	movdqa	xmm7,xmm13
	pslld	xmm13,7
	psrld	xmm7,25
	movdqa	xmm6,xmm14
	pslld	xmm14,7
	por	xmm13,xmm7
	psrld	xmm6,25
	movdqa	xmm7,XMMWORD PTR[r10]
	por	xmm14,xmm6
	movdqa	XMMWORD PTR[32+rsp],xmm4
	movdqa	XMMWORD PTR[48+rsp],xmm5
	movdqa	xmm4,XMMWORD PTR[rsp]
	movdqa	xmm5,XMMWORD PTR[16+rsp]
	paddd	xmm10,xmm15
	paddd	xmm11,xmm12
	pxor	xmm1,xmm10
	pxor	xmm2,xmm11
DB	102,15,56,0,207
DB	102,15,56,0,215
	paddd	xmm4,xmm1
	paddd	xmm5,xmm2
	pxor	xmm15,xmm4
	pxor	xmm12,xmm5
	movdqa	xmm6,xmm15
	pslld	xmm15,12
	psrld	xmm6,20
	movdqa	xmm7,xmm12
	pslld	xmm12,12
	por	xmm15,xmm6
	psrld	xmm7,20
	movdqa	xmm6,XMMWORD PTR[r11]
	por	xmm12,xmm7
	paddd	xmm10,xmm15
	paddd	xmm11,xmm12
	pxor	xmm1,xmm10
	pxor	xmm2,xmm11
DB	102,15,56,0,206
DB	102,15,56,0,214
	paddd	xmm4,xmm1
	paddd	xmm5,xmm2
	pxor	xmm15,xmm4
	pxor	xmm12,xmm5
	movdqa	xmm7,xmm15
	pslld	xmm15,7
	psrld	xmm7,25
	movdqa	xmm6,xmm12
	pslld	xmm12,7
	por	xmm15,xmm7
	psrld	xmm6,25
	movdqa	xmm7,XMMWORD PTR[r10]
	por	xmm12,xmm6
	dec	eax
	jnz	$L$oop4x

	paddd	xmm8,XMMWORD PTR[64+rsp]
	paddd	xmm9,XMMWORD PTR[80+rsp]
	paddd	xmm10,XMMWORD PTR[96+rsp]
	paddd	xmm11,XMMWORD PTR[112+rsp]

	movdqa	xmm6,xmm8
	punpckldq	xmm8,xmm9
	movdqa	xmm7,xmm10
	punpckldq	xmm10,xmm11
	punpckhdq	xmm6,xmm9
	punpckhdq	xmm7,xmm11
	movdqa	xmm9,xmm8
	punpcklqdq	xmm8,xmm10
	movdqa	xmm11,xmm6
	punpcklqdq	xmm6,xmm7
	punpckhqdq	xmm9,xmm10
	punpckhqdq	xmm11,xmm7
	paddd	xmm12,XMMWORD PTR[((128-256))+rcx]
	paddd	xmm13,XMMWORD PTR[((144-256))+rcx]
	paddd	xmm14,XMMWORD PTR[((160-256))+rcx]
	paddd	xmm15,XMMWORD PTR[((176-256))+rcx]

	movdqa	XMMWORD PTR[rsp],xmm8
	movdqa	XMMWORD PTR[16+rsp],xmm9
	movdqa	xmm8,XMMWORD PTR[32+rsp]
	movdqa	xmm9,XMMWORD PTR[48+rsp]

	movdqa	xmm10,xmm12
	punpckldq	xmm12,xmm13
	movdqa	xmm7,xmm14
	punpckldq	xmm14,xmm15
	punpckhdq	xmm10,xmm13
	punpckhdq	xmm7,xmm15
	movdqa	xmm13,xmm12
	punpcklqdq	xmm12,xmm14
	movdqa	xmm15,xmm10
	punpcklqdq	xmm10,xmm7
	punpckhqdq	xmm13,xmm14
	punpckhqdq	xmm15,xmm7
	paddd	xmm4,XMMWORD PTR[((192-256))+rcx]
	paddd	xmm5,XMMWORD PTR[((208-256))+rcx]
	paddd	xmm8,XMMWORD PTR[((224-256))+rcx]
	paddd	xmm9,XMMWORD PTR[((240-256))+rcx]

	movdqa	XMMWORD PTR[32+rsp],xmm6
	movdqa	XMMWORD PTR[48+rsp],xmm11

	movdqa	xmm14,xmm4
	punpckldq	xmm4,xmm5
	movdqa	xmm7,xmm8
	punpckldq	xmm8,xmm9
	punpckhdq	xmm14,xmm5
	punpckhdq	xmm7,xmm9
	movdqa	xmm5,xmm4
	punpcklqdq	xmm4,xmm8
	movdqa	xmm9,xmm14
	punpcklqdq	xmm14,xmm7
	punpckhqdq	xmm5,xmm8
	punpckhqdq	xmm9,xmm7
	paddd	xmm0,XMMWORD PTR[((256-256))+rcx]
	paddd	xmm1,XMMWORD PTR[((272-256))+rcx]
	paddd	xmm2,XMMWORD PTR[((288-256))+rcx]
	paddd	xmm3,XMMWORD PTR[((304-256))+rcx]

	movdqa	xmm8,xmm0
	punpckldq	xmm0,xmm1
	movdqa	xmm7,xmm2
	punpckldq	xmm2,xmm3
	punpckhdq	xmm8,xmm1
	punpckhdq	xmm7,xmm3
	movdqa	xmm1,xmm0
	punpcklqdq	xmm0,xmm2
	movdqa	xmm3,xmm8
	punpcklqdq	xmm8,xmm7
	punpckhqdq	xmm1,xmm2
	punpckhqdq	xmm3,xmm7
	cmp	rdx,64*4
	jb	$L$tail4x

	movdqu	xmm6,XMMWORD PTR[rsi]
	movdqu	xmm11,XMMWORD PTR[16+rsi]
	movdqu	xmm2,XMMWORD PTR[32+rsi]
	movdqu	xmm7,XMMWORD PTR[48+rsi]
	pxor	xmm6,XMMWORD PTR[rsp]
	pxor	xmm11,xmm12
	pxor	xmm2,xmm4
	pxor	xmm7,xmm0

	movdqu	XMMWORD PTR[rdi],xmm6
	movdqu	xmm6,XMMWORD PTR[64+rsi]
	movdqu	XMMWORD PTR[16+rdi],xmm11
	movdqu	xmm11,XMMWORD PTR[80+rsi]
	movdqu	XMMWORD PTR[32+rdi],xmm2
	movdqu	xmm2,XMMWORD PTR[96+rsi]
	movdqu	XMMWORD PTR[48+rdi],xmm7
	movdqu	xmm7,XMMWORD PTR[112+rsi]
	lea	rsi,QWORD PTR[128+rsi]
	pxor	xmm6,XMMWORD PTR[16+rsp]
	pxor	xmm11,xmm13
	pxor	xmm2,xmm5
	pxor	xmm7,xmm1

	movdqu	XMMWORD PTR[64+rdi],xmm6
	movdqu	xmm6,XMMWORD PTR[rsi]
	movdqu	XMMWORD PTR[80+rdi],xmm11
	movdqu	xmm11,XMMWORD PTR[16+rsi]
	movdqu	XMMWORD PTR[96+rdi],xmm2
	movdqu	xmm2,XMMWORD PTR[32+rsi]
	movdqu	XMMWORD PTR[112+rdi],xmm7
	lea	rdi,QWORD PTR[128+rdi]
	movdqu	xmm7,XMMWORD PTR[48+rsi]
	pxor	xmm6,XMMWORD PTR[32+rsp]
	pxor	xmm11,xmm10
	pxor	xmm2,xmm14
	pxor	xmm7,xmm8

	movdqu	XMMWORD PTR[rdi],xmm6
	movdqu	xmm6,XMMWORD PTR[64+rsi]
	movdqu	XMMWORD PTR[16+rdi],xmm11
	movdqu	xmm11,XMMWORD PTR[80+rsi]
	movdqu	XMMWORD PTR[32+rdi],xmm2
	movdqu	xmm2,XMMWORD PTR[96+rsi]
	movdqu	XMMWORD PTR[48+rdi],xmm7
	movdqu	xmm7,XMMWORD PTR[112+rsi]
	lea	rsi,QWORD PTR[128+rsi]
	pxor	xmm6,XMMWORD PTR[48+rsp]
	pxor	xmm11,xmm15
	pxor	xmm2,xmm9
	pxor	xmm7,xmm3
	movdqu	XMMWORD PTR[64+rdi],xmm6
	movdqu	XMMWORD PTR[80+rdi],xmm11
	movdqu	XMMWORD PTR[96+rdi],xmm2
	movdqu	XMMWORD PTR[112+rdi],xmm7
	lea	rdi,QWORD PTR[128+rdi]

	sub	rdx,64*4
	jnz	$L$oop_outer4x

	jmp	$L$done4x

$L$tail4x::
	cmp	rdx,192
	jae	$L$192_or_more4x
	cmp	rdx,128
	jae	$L$128_or_more4x
	cmp	rdx,64
	jae	$L$64_or_more4x


	xor	r10,r10

	movdqa	XMMWORD PTR[16+rsp],xmm12
	movdqa	XMMWORD PTR[32+rsp],xmm4
	movdqa	XMMWORD PTR[48+rsp],xmm0
	jmp	$L$oop_tail4x

ALIGN	32
$L$64_or_more4x::
	movdqu	xmm6,XMMWORD PTR[rsi]
	movdqu	xmm11,XMMWORD PTR[16+rsi]
	movdqu	xmm2,XMMWORD PTR[32+rsi]
	movdqu	xmm7,XMMWORD PTR[48+rsi]
	pxor	xmm6,XMMWORD PTR[rsp]
	pxor	xmm11,xmm12
	pxor	xmm2,xmm4
	pxor	xmm7,xmm0
	movdqu	XMMWORD PTR[rdi],xmm6
	movdqu	XMMWORD PTR[16+rdi],xmm11
	movdqu	XMMWORD PTR[32+rdi],xmm2
	movdqu	XMMWORD PTR[48+rdi],xmm7
	je	$L$done4x

	movdqa	xmm6,XMMWORD PTR[16+rsp]
	lea	rsi,QWORD PTR[64+rsi]
	xor	r10,r10
	movdqa	XMMWORD PTR[rsp],xmm6
	movdqa	XMMWORD PTR[16+rsp],xmm13
	lea	rdi,QWORD PTR[64+rdi]
	movdqa	XMMWORD PTR[32+rsp],xmm5
	sub	rdx,64
	movdqa	XMMWORD PTR[48+rsp],xmm1
	jmp	$L$oop_tail4x

ALIGN	32
$L$128_or_more4x::
	movdqu	xmm6,XMMWORD PTR[rsi]
	movdqu	xmm11,XMMWORD PTR[16+rsi]
	movdqu	xmm2,XMMWORD PTR[32+rsi]
	movdqu	xmm7,XMMWORD PTR[48+rsi]
	pxor	xmm6,XMMWORD PTR[rsp]
	pxor	xmm11,xmm12
	pxor	xmm2,xmm4
	pxor	xmm7,xmm0

	movdqu	XMMWORD PTR[rdi],xmm6
	movdqu	xmm6,XMMWORD PTR[64+rsi]
	movdqu	XMMWORD PTR[16+rdi],xmm11
	movdqu	xmm11,XMMWORD PTR[80+rsi]
	movdqu	XMMWORD PTR[32+rdi],xmm2
	movdqu	xmm2,XMMWORD PTR[96+rsi]
	movdqu	XMMWORD PTR[48+rdi],xmm7
	movdqu	xmm7,XMMWORD PTR[112+rsi]
	pxor	xmm6,XMMWORD PTR[16+rsp]
	pxor	xmm11,xmm13
	pxor	xmm2,xmm5
	pxor	xmm7,xmm1
	movdqu	XMMWORD PTR[64+rdi],xmm6
	movdqu	XMMWORD PTR[80+rdi],xmm11
	movdqu	XMMWORD PTR[96+rdi],xmm2
	movdqu	XMMWORD PTR[112+rdi],xmm7
	je	$L$done4x

	movdqa	xmm6,XMMWORD PTR[32+rsp]
	lea	rsi,QWORD PTR[128+rsi]
	xor	r10,r10
	movdqa	XMMWORD PTR[rsp],xmm6
	movdqa	XMMWORD PTR[16+rsp],xmm10
	lea	rdi,QWORD PTR[128+rdi]
	movdqa	XMMWORD PTR[32+rsp],xmm14
	sub	rdx,128
	movdqa	XMMWORD PTR[48+rsp],xmm8
	jmp	$L$oop_tail4x

ALIGN	32
$L$192_or_more4x::
	movdqu	xmm6,XMMWORD PTR[rsi]
	movdqu	xmm11,XMMWORD PTR[16+rsi]
	movdqu	xmm2,XMMWORD PTR[32+rsi]
	movdqu	xmm7,XMMWORD PTR[48+rsi]
	pxor	xmm6,XMMWORD PTR[rsp]
	pxor	xmm11,xmm12
	pxor	xmm2,xmm4
	pxor	xmm7,xmm0

	movdqu	XMMWORD PTR[rdi],xmm6
	movdqu	xmm6,XMMWORD PTR[64+rsi]
	movdqu	XMMWORD PTR[16+rdi],xmm11
	movdqu	xmm11,XMMWORD PTR[80+rsi]
	movdqu	XMMWORD PTR[32+rdi],xmm2
	movdqu	xmm2,XMMWORD PTR[96+rsi]
	movdqu	XMMWORD PTR[48+rdi],xmm7
	movdqu	xmm7,XMMWORD PTR[112+rsi]
	lea	rsi,QWORD PTR[128+rsi]
	pxor	xmm6,XMMWORD PTR[16+rsp]
	pxor	xmm11,xmm13
	pxor	xmm2,xmm5
	pxor	xmm7,xmm1

	movdqu	XMMWORD PTR[64+rdi],xmm6
	movdqu	xmm6,XMMWORD PTR[rsi]
	movdqu	XMMWORD PTR[80+rdi],xmm11
	movdqu	xmm11,XMMWORD PTR[16+rsi]
	movdqu	XMMWORD PTR[96+rdi],xmm2
	movdqu	xmm2,XMMWORD PTR[32+rsi]
	movdqu	XMMWORD PTR[112+rdi],xmm7
	lea	rdi,QWORD PTR[128+rdi]
	movdqu	xmm7,XMMWORD PTR[48+rsi]
	pxor	xmm6,XMMWORD PTR[32+rsp]
	pxor	xmm11,xmm10
	pxor	xmm2,xmm14
	pxor	xmm7,xmm8
	movdqu	XMMWORD PTR[rdi],xmm6
	movdqu	XMMWORD PTR[16+rdi],xmm11
	movdqu	XMMWORD PTR[32+rdi],xmm2
	movdqu	XMMWORD PTR[48+rdi],xmm7
	je	$L$done4x

	movdqa	xmm6,XMMWORD PTR[48+rsp]
	lea	rsi,QWORD PTR[64+rsi]
	xor	r10,r10
	movdqa	XMMWORD PTR[rsp],xmm6
	movdqa	XMMWORD PTR[16+rsp],xmm15
	lea	rdi,QWORD PTR[64+rdi]
	movdqa	XMMWORD PTR[32+rsp],xmm9
	sub	rdx,192
	movdqa	XMMWORD PTR[48+rsp],xmm3

$L$oop_tail4x::
	movzx	eax,BYTE PTR[r10*1+rsi]
	movzx	ecx,BYTE PTR[r10*1+rsp]
	lea	r10,QWORD PTR[1+r10]
	xor	eax,ecx
	mov	BYTE PTR[((-1))+r10*1+rdi],al
	dec	rdx
	jnz	$L$oop_tail4x

$L$done4x::
	movaps	xmm6,XMMWORD PTR[((-168))+r9]
	movaps	xmm7,XMMWORD PTR[((-152))+r9]
	movaps	xmm8,XMMWORD PTR[((-136))+r9]
	movaps	xmm9,XMMWORD PTR[((-120))+r9]
	movaps	xmm10,XMMWORD PTR[((-104))+r9]
	movaps	xmm11,XMMWORD PTR[((-88))+r9]
	movaps	xmm12,XMMWORD PTR[((-72))+r9]
	movaps	xmm13,XMMWORD PTR[((-56))+r9]
	movaps	xmm14,XMMWORD PTR[((-40))+r9]
	movaps	xmm15,XMMWORD PTR[((-24))+r9]
	lea	rsp,QWORD PTR[r9]

$L$4x_epilogue::
	mov	rdi,QWORD PTR[8+rsp]	;WIN64 epilogue
	mov	rsi,QWORD PTR[16+rsp]
	DB	0F3h,0C3h		;repret

$L$SEH_end_ChaCha20_4x::
ChaCha20_4x	ENDP
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

	lea	r10,QWORD PTR[$L$ctr32_body]
	cmp	rbx,r10
	jb	$L$common_seh_tail

	mov	rax,QWORD PTR[152+r8]

	lea	r10,QWORD PTR[$L$no_data]
	cmp	rbx,r10
	jae	$L$common_seh_tail

	lea	rax,QWORD PTR[((64+24+48))+rax]

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
se_handler	ENDP


ALIGN	16
simd_handler	PROC PRIVATE
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

	mov	rax,QWORD PTR[192+r8]

	mov	r10d,DWORD PTR[4+r11]
	mov	ecx,DWORD PTR[8+r11]
	lea	r10,QWORD PTR[r10*1+rsi]
	cmp	rbx,r10
	jae	$L$common_seh_tail

	neg	rcx
	lea	rsi,QWORD PTR[((-8))+rcx*1+rax]
	lea	rdi,QWORD PTR[512+r8]
	neg	ecx
	shr	ecx,3
	DD	0a548f3fch

	jmp	$L$common_seh_tail
simd_handler	ENDP

.text$	ENDS
.pdata	SEGMENT READONLY ALIGN(4)
ALIGN	4
	DD	imagerel $L$SEH_begin_ChaCha20_ctr32
	DD	imagerel $L$SEH_end_ChaCha20_ctr32
	DD	imagerel $L$SEH_info_ChaCha20_ctr32

	DD	imagerel $L$SEH_begin_ChaCha20_ssse3
	DD	imagerel $L$SEH_end_ChaCha20_ssse3
	DD	imagerel $L$SEH_info_ChaCha20_ssse3

	DD	imagerel $L$SEH_begin_ChaCha20_128
	DD	imagerel $L$SEH_end_ChaCha20_128
	DD	imagerel $L$SEH_info_ChaCha20_128

	DD	imagerel $L$SEH_begin_ChaCha20_4x
	DD	imagerel $L$SEH_end_ChaCha20_4x
	DD	imagerel $L$SEH_info_ChaCha20_4x
.pdata	ENDS
.xdata	SEGMENT READONLY ALIGN(8)
ALIGN	8
$L$SEH_info_ChaCha20_ctr32::
DB	9,0,0,0
	DD	imagerel se_handler

$L$SEH_info_ChaCha20_ssse3::
DB	9,0,0,0
	DD	imagerel simd_handler
	DD	imagerel $L$ssse3_body,imagerel $L$ssse3_epilogue
	DD	020h,0

$L$SEH_info_ChaCha20_128::
DB	9,0,0,0
	DD	imagerel simd_handler
	DD	imagerel $L$128_body,imagerel $L$128_epilogue
	DD	060h,0

$L$SEH_info_ChaCha20_4x::
DB	9,0,0,0
	DD	imagerel simd_handler
	DD	imagerel $L$4x_body,imagerel $L$4x_epilogue
	DD	0a0h,0

.xdata	ENDS
END
