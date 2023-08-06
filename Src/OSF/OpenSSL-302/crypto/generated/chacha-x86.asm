%ifidn __OUTPUT_FORMAT__,obj
section	code	use32 class=code align=64
%elifidn __OUTPUT_FORMAT__,win32
$@feat.00 equ 1
section	.text	code align=64
%else
section	.text	code
%endif
global	_ChaCha20_ctr32
align	16
_ChaCha20_ctr32:
L$_ChaCha20_ctr32_begin:
	push	ebp
	push	ebx
	push	esi
	push	edi
	xor	eax,eax
	cmp	eax,DWORD [28+esp]
	je	NEAR L$000no_data
	call	L$pic_point
L$pic_point:
	pop	eax
	lea	ebp,[_OPENSSL_ia32cap_P]
	test	DWORD [ebp],16777216
	jz	NEAR L$001x86
	test	DWORD [4+ebp],512
	jz	NEAR L$001x86
	jmp	NEAR L$ssse3_shortcut
L$001x86:
	mov	esi,DWORD [32+esp]
	mov	edi,DWORD [36+esp]
	sub	esp,132
	mov	eax,DWORD [esi]
	mov	ebx,DWORD [4+esi]
	mov	ecx,DWORD [8+esi]
	mov	edx,DWORD [12+esi]
	mov	DWORD [80+esp],eax
	mov	DWORD [84+esp],ebx
	mov	DWORD [88+esp],ecx
	mov	DWORD [92+esp],edx
	mov	eax,DWORD [16+esi]
	mov	ebx,DWORD [20+esi]
	mov	ecx,DWORD [24+esi]
	mov	edx,DWORD [28+esi]
	mov	DWORD [96+esp],eax
	mov	DWORD [100+esp],ebx
	mov	DWORD [104+esp],ecx
	mov	DWORD [108+esp],edx
	mov	eax,DWORD [edi]
	mov	ebx,DWORD [4+edi]
	mov	ecx,DWORD [8+edi]
	mov	edx,DWORD [12+edi]
	sub	eax,1
	mov	DWORD [112+esp],eax
	mov	DWORD [116+esp],ebx
	mov	DWORD [120+esp],ecx
	mov	DWORD [124+esp],edx
	jmp	NEAR L$002entry
align	16
L$003outer_loop:
	mov	DWORD [156+esp],ebx
	mov	DWORD [152+esp],eax
	mov	DWORD [160+esp],ecx
L$002entry:
	mov	eax,1634760805
	mov	DWORD [4+esp],857760878
	mov	DWORD [8+esp],2036477234
	mov	DWORD [12+esp],1797285236
	mov	ebx,DWORD [84+esp]
	mov	ebp,DWORD [88+esp]
	mov	ecx,DWORD [104+esp]
	mov	esi,DWORD [108+esp]
	mov	edx,DWORD [116+esp]
	mov	edi,DWORD [120+esp]
	mov	DWORD [20+esp],ebx
	mov	DWORD [24+esp],ebp
	mov	DWORD [40+esp],ecx
	mov	DWORD [44+esp],esi
	mov	DWORD [52+esp],edx
	mov	DWORD [56+esp],edi
	mov	ebx,DWORD [92+esp]
	mov	edi,DWORD [124+esp]
	mov	edx,DWORD [112+esp]
	mov	ebp,DWORD [80+esp]
	mov	ecx,DWORD [96+esp]
	mov	esi,DWORD [100+esp]
	add	edx,1
	mov	DWORD [28+esp],ebx
	mov	DWORD [60+esp],edi
	mov	DWORD [112+esp],edx
	mov	ebx,10
	jmp	NEAR L$004loop
align	16
L$004loop:
	add	eax,ebp
	mov	DWORD [128+esp],ebx
	mov	ebx,ebp
	xor	edx,eax
	rol	edx,16
	add	ecx,edx
	xor	ebx,ecx
	mov	edi,DWORD [52+esp]
	rol	ebx,12
	mov	ebp,DWORD [20+esp]
	add	eax,ebx
	xor	edx,eax
	mov	DWORD [esp],eax
	rol	edx,8
	mov	eax,DWORD [4+esp]
	add	ecx,edx
	mov	DWORD [48+esp],edx
	xor	ebx,ecx
	add	eax,ebp
	rol	ebx,7
	xor	edi,eax
	mov	DWORD [32+esp],ecx
	rol	edi,16
	mov	DWORD [16+esp],ebx
	add	esi,edi
	mov	ecx,DWORD [40+esp]
	xor	ebp,esi
	mov	edx,DWORD [56+esp]
	rol	ebp,12
	mov	ebx,DWORD [24+esp]
	add	eax,ebp
	xor	edi,eax
	mov	DWORD [4+esp],eax
	rol	edi,8
	mov	eax,DWORD [8+esp]
	add	esi,edi
	mov	DWORD [52+esp],edi
	xor	ebp,esi
	add	eax,ebx
	rol	ebp,7
	xor	edx,eax
	mov	DWORD [36+esp],esi
	rol	edx,16
	mov	DWORD [20+esp],ebp
	add	ecx,edx
	mov	esi,DWORD [44+esp]
	xor	ebx,ecx
	mov	edi,DWORD [60+esp]
	rol	ebx,12
	mov	ebp,DWORD [28+esp]
	add	eax,ebx
	xor	edx,eax
	mov	DWORD [8+esp],eax
	rol	edx,8
	mov	eax,DWORD [12+esp]
	add	ecx,edx
	mov	DWORD [56+esp],edx
	xor	ebx,ecx
	add	eax,ebp
	rol	ebx,7
	xor	edi,eax
	rol	edi,16
	mov	DWORD [24+esp],ebx
	add	esi,edi
	xor	ebp,esi
	rol	ebp,12
	mov	ebx,DWORD [20+esp]
	add	eax,ebp
	xor	edi,eax
	mov	DWORD [12+esp],eax
	rol	edi,8
	mov	eax,DWORD [esp]
	add	esi,edi
	mov	edx,edi
	xor	ebp,esi
	add	eax,ebx
	rol	ebp,7
	xor	edx,eax
	rol	edx,16
	mov	DWORD [28+esp],ebp
	add	ecx,edx
	xor	ebx,ecx
	mov	edi,DWORD [48+esp]
	rol	ebx,12
	mov	ebp,DWORD [24+esp]
	add	eax,ebx
	xor	edx,eax
	mov	DWORD [esp],eax
	rol	edx,8
	mov	eax,DWORD [4+esp]
	add	ecx,edx
	mov	DWORD [60+esp],edx
	xor	ebx,ecx
	add	eax,ebp
	rol	ebx,7
	xor	edi,eax
	mov	DWORD [40+esp],ecx
	rol	edi,16
	mov	DWORD [20+esp],ebx
	add	esi,edi
	mov	ecx,DWORD [32+esp]
	xor	ebp,esi
	mov	edx,DWORD [52+esp]
	rol	ebp,12
	mov	ebx,DWORD [28+esp]
	add	eax,ebp
	xor	edi,eax
	mov	DWORD [4+esp],eax
	rol	edi,8
	mov	eax,DWORD [8+esp]
	add	esi,edi
	mov	DWORD [48+esp],edi
	xor	ebp,esi
	add	eax,ebx
	rol	ebp,7
	xor	edx,eax
	mov	DWORD [44+esp],esi
	rol	edx,16
	mov	DWORD [24+esp],ebp
	add	ecx,edx
	mov	esi,DWORD [36+esp]
	xor	ebx,ecx
	mov	edi,DWORD [56+esp]
	rol	ebx,12
	mov	ebp,DWORD [16+esp]
	add	eax,ebx
	xor	edx,eax
	mov	DWORD [8+esp],eax
	rol	edx,8
	mov	eax,DWORD [12+esp]
	add	ecx,edx
	mov	DWORD [52+esp],edx
	xor	ebx,ecx
	add	eax,ebp
	rol	ebx,7
	xor	edi,eax
	rol	edi,16
	mov	DWORD [28+esp],ebx
	add	esi,edi
	xor	ebp,esi
	mov	edx,DWORD [48+esp]
	rol	ebp,12
	mov	ebx,DWORD [128+esp]
	add	eax,ebp
	xor	edi,eax
	mov	DWORD [12+esp],eax
	rol	edi,8
	mov	eax,DWORD [esp]
	add	esi,edi
	mov	DWORD [56+esp],edi
	xor	ebp,esi
	rol	ebp,7
	dec	ebx
	jnz	NEAR L$004loop
	mov	ebx,DWORD [160+esp]
	add	eax,1634760805
	add	ebp,DWORD [80+esp]
	add	ecx,DWORD [96+esp]
	add	esi,DWORD [100+esp]
	cmp	ebx,64
	jb	NEAR L$005tail
	mov	ebx,DWORD [156+esp]
	add	edx,DWORD [112+esp]
	add	edi,DWORD [120+esp]
	xor	eax,DWORD [ebx]
	xor	ebp,DWORD [16+ebx]
	mov	DWORD [esp],eax
	mov	eax,DWORD [152+esp]
	xor	ecx,DWORD [32+ebx]
	xor	esi,DWORD [36+ebx]
	xor	edx,DWORD [48+ebx]
	xor	edi,DWORD [56+ebx]
	mov	DWORD [16+eax],ebp
	mov	DWORD [32+eax],ecx
	mov	DWORD [36+eax],esi
	mov	DWORD [48+eax],edx
	mov	DWORD [56+eax],edi
	mov	ebp,DWORD [4+esp]
	mov	ecx,DWORD [8+esp]
	mov	esi,DWORD [12+esp]
	mov	edx,DWORD [20+esp]
	mov	edi,DWORD [24+esp]
	add	ebp,857760878
	add	ecx,2036477234
	add	esi,1797285236
	add	edx,DWORD [84+esp]
	add	edi,DWORD [88+esp]
	xor	ebp,DWORD [4+ebx]
	xor	ecx,DWORD [8+ebx]
	xor	esi,DWORD [12+ebx]
	xor	edx,DWORD [20+ebx]
	xor	edi,DWORD [24+ebx]
	mov	DWORD [4+eax],ebp
	mov	DWORD [8+eax],ecx
	mov	DWORD [12+eax],esi
	mov	DWORD [20+eax],edx
	mov	DWORD [24+eax],edi
	mov	ebp,DWORD [28+esp]
	mov	ecx,DWORD [40+esp]
	mov	esi,DWORD [44+esp]
	mov	edx,DWORD [52+esp]
	mov	edi,DWORD [60+esp]
	add	ebp,DWORD [92+esp]
	add	ecx,DWORD [104+esp]
	add	esi,DWORD [108+esp]
	add	edx,DWORD [116+esp]
	add	edi,DWORD [124+esp]
	xor	ebp,DWORD [28+ebx]
	xor	ecx,DWORD [40+ebx]
	xor	esi,DWORD [44+ebx]
	xor	edx,DWORD [52+ebx]
	xor	edi,DWORD [60+ebx]
	lea	ebx,[64+ebx]
	mov	DWORD [28+eax],ebp
	mov	ebp,DWORD [esp]
	mov	DWORD [40+eax],ecx
	mov	ecx,DWORD [160+esp]
	mov	DWORD [44+eax],esi
	mov	DWORD [52+eax],edx
	mov	DWORD [60+eax],edi
	mov	DWORD [eax],ebp
	lea	eax,[64+eax]
	sub	ecx,64
	jnz	NEAR L$003outer_loop
	jmp	NEAR L$006done
L$005tail:
	add	edx,DWORD [112+esp]
	add	edi,DWORD [120+esp]
	mov	DWORD [esp],eax
	mov	DWORD [16+esp],ebp
	mov	DWORD [32+esp],ecx
	mov	DWORD [36+esp],esi
	mov	DWORD [48+esp],edx
	mov	DWORD [56+esp],edi
	mov	ebp,DWORD [4+esp]
	mov	ecx,DWORD [8+esp]
	mov	esi,DWORD [12+esp]
	mov	edx,DWORD [20+esp]
	mov	edi,DWORD [24+esp]
	add	ebp,857760878
	add	ecx,2036477234
	add	esi,1797285236
	add	edx,DWORD [84+esp]
	add	edi,DWORD [88+esp]
	mov	DWORD [4+esp],ebp
	mov	DWORD [8+esp],ecx
	mov	DWORD [12+esp],esi
	mov	DWORD [20+esp],edx
	mov	DWORD [24+esp],edi
	mov	ebp,DWORD [28+esp]
	mov	ecx,DWORD [40+esp]
	mov	esi,DWORD [44+esp]
	mov	edx,DWORD [52+esp]
	mov	edi,DWORD [60+esp]
	add	ebp,DWORD [92+esp]
	add	ecx,DWORD [104+esp]
	add	esi,DWORD [108+esp]
	add	edx,DWORD [116+esp]
	add	edi,DWORD [124+esp]
	mov	DWORD [28+esp],ebp
	mov	ebp,DWORD [156+esp]
	mov	DWORD [40+esp],ecx
	mov	ecx,DWORD [152+esp]
	mov	DWORD [44+esp],esi
	xor	esi,esi
	mov	DWORD [52+esp],edx
	mov	DWORD [60+esp],edi
	xor	eax,eax
	xor	edx,edx
L$007tail_loop:
	mov	al,BYTE [ebp*1+esi]
	mov	dl,BYTE [esi*1+esp]
	lea	esi,[1+esi]
	xor	al,dl
	mov	BYTE [esi*1+ecx-1],al
	dec	ebx
	jnz	NEAR L$007tail_loop
L$006done:
	add	esp,132
L$000no_data:
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret
global	_ChaCha20_ssse3
align	16
_ChaCha20_ssse3:
L$_ChaCha20_ssse3_begin:
	push	ebp
	push	ebx
	push	esi
	push	edi
L$ssse3_shortcut:
	mov	edi,DWORD [20+esp]
	mov	esi,DWORD [24+esp]
	mov	ecx,DWORD [28+esp]
	mov	edx,DWORD [32+esp]
	mov	ebx,DWORD [36+esp]
	mov	ebp,esp
	sub	esp,524
	and	esp,-64
	mov	DWORD [512+esp],ebp
	lea	eax,[(L$ssse3_data-L$pic_point)+eax]
	movdqu	xmm3,[ebx]
L$0081x:
	movdqa	xmm0,[32+eax]
	movdqu	xmm1,[edx]
	movdqu	xmm2,[16+edx]
	movdqa	xmm6,[eax]
	movdqa	xmm7,[16+eax]
	mov	DWORD [48+esp],ebp
	movdqa	[esp],xmm0
	movdqa	[16+esp],xmm1
	movdqa	[32+esp],xmm2
	movdqa	[48+esp],xmm3
	mov	edx,10
	jmp	NEAR L$009loop1x
align	16
L$010outer1x:
	movdqa	xmm3,[80+eax]
	movdqa	xmm0,[esp]
	movdqa	xmm1,[16+esp]
	movdqa	xmm2,[32+esp]
	paddd	xmm3,[48+esp]
	mov	edx,10
	movdqa	[48+esp],xmm3
	jmp	NEAR L$009loop1x
align	16
L$009loop1x:
	paddd	xmm0,xmm1
	pxor	xmm3,xmm0
db	102,15,56,0,222
	paddd	xmm2,xmm3
	pxor	xmm1,xmm2
	movdqa	xmm4,xmm1
	psrld	xmm1,20
	pslld	xmm4,12
	por	xmm1,xmm4
	paddd	xmm0,xmm1
	pxor	xmm3,xmm0
db	102,15,56,0,223
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
db	102,15,56,0,222
	paddd	xmm2,xmm3
	pxor	xmm1,xmm2
	movdqa	xmm4,xmm1
	psrld	xmm1,20
	pslld	xmm4,12
	por	xmm1,xmm4
	paddd	xmm0,xmm1
	pxor	xmm3,xmm0
db	102,15,56,0,223
	paddd	xmm2,xmm3
	pxor	xmm1,xmm2
	movdqa	xmm4,xmm1
	psrld	xmm1,25
	pslld	xmm4,7
	por	xmm1,xmm4
	pshufd	xmm2,xmm2,78
	pshufd	xmm1,xmm1,147
	pshufd	xmm3,xmm3,57
	dec	edx
	jnz	NEAR L$009loop1x
	paddd	xmm0,[esp]
	paddd	xmm1,[16+esp]
	paddd	xmm2,[32+esp]
	paddd	xmm3,[48+esp]
	cmp	ecx,64
	jb	NEAR L$011tail
	movdqu	xmm4,[esi]
	movdqu	xmm5,[16+esi]
	pxor	xmm0,xmm4
	movdqu	xmm4,[32+esi]
	pxor	xmm1,xmm5
	movdqu	xmm5,[48+esi]
	pxor	xmm2,xmm4
	pxor	xmm3,xmm5
	lea	esi,[64+esi]
	movdqu	[edi],xmm0
	movdqu	[16+edi],xmm1
	movdqu	[32+edi],xmm2
	movdqu	[48+edi],xmm3
	lea	edi,[64+edi]
	sub	ecx,64
	jnz	NEAR L$010outer1x
	jmp	NEAR L$012done
L$011tail:
	movdqa	[esp],xmm0
	movdqa	[16+esp],xmm1
	movdqa	[32+esp],xmm2
	movdqa	[48+esp],xmm3
	xor	eax,eax
	xor	edx,edx
	xor	ebp,ebp
L$013tail_loop:
	mov	al,BYTE [ebp*1+esp]
	mov	dl,BYTE [ebp*1+esi]
	lea	ebp,[1+ebp]
	xor	al,dl
	mov	BYTE [ebp*1+edi-1],al
	dec	ecx
	jnz	NEAR L$013tail_loop
L$012done:
	mov	esp,DWORD [512+esp]
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret
align	64
L$ssse3_data:
db	2,3,0,1,6,7,4,5,10,11,8,9,14,15,12,13
db	3,0,1,2,7,4,5,6,11,8,9,10,15,12,13,14
dd	1634760805,857760878,2036477234,1797285236
dd	0,1,2,3
dd	4,4,4,4
dd	1,0,0,0
dd	4,0,0,0
dd	0,-1,-1,-1
align	64
db	67,104,97,67,104,97,50,48,32,102,111,114,32,120,56,54
db	44,32,67,82,89,80,84,79,71,65,77,83,32,98,121,32
db	60,97,112,112,114,111,64,111,112,101,110,115,115,108,46,111
db	114,103,62,0
segment	.bss
common	_OPENSSL_ia32cap_P 16
