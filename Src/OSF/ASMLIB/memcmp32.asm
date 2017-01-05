;*************************  memcmp32.asm  *************************************
; Author:           Agner Fog
; Date created:     2013-10-03
; Last modified:    2013-10-03
; Description:
; Faster version of the standard memcmp function:
;
; int A_memcmp (const void * ptr1, const void * ptr2, size_t count);
;
; Compares two memory blocks of size num.
; The return value is zero if the two memory blocks ptr1 and ptr2 are equal
; The return value is positive if the first differing byte of ptr1 is bigger 
; than ptr2 when compared as unsigned bytes.
; The return value is negative if the first differing byte of ptr1 is smaller 
; than ptr2 when compared as unsigned bytes.
;
; Overriding standard function memcmp:
; The alias ?OVR_memcmp is changed to _memcmp in the object file if
; it is desired to override the standard library function memcmp.
;
; Optimization:
; Uses XMM registers if SSE2 is available, uses YMM registers if AVX2.
;
; The latest version of this file is available at:
; www.agner.org/optimize/asmexamples.zip
; Copyright (c) 2013 GNU General Public License www.gnu.org/licenses
;******************************************************************************

global _A_memcmp: function             ; Function memcmp
global ?OVR_memcmp: function           ; ?OVR removed if standard function memcmp overridden
; Direct entries to CPU-specific versions
global _memcmp386:  function           ; version for old CPUs without SSE
global _memcmpSSE2: function           ; SSE2 version
global _memcmpAVX2: function           ; AVX2 version

; Imported from instrset32.asm
extern _InstructionSet                 ; Instruction set for CPU dispatcher


SECTION .text  align=16

; extern "C" int A_memcmp (const void * ptr1, const void * ptr2, size_t count);
; Function entry:
_A_memcmp:
?OVR_memcmp:
%IFNDEF POSITIONINDEPENDENT
        jmp     dword [memcmpDispatch] ; Go to appropriate version, depending on instruction set

%ELSE   ; Position-independent code
        call    get_thunk_edx          ; get reference point for position-independent code
RP:                                    ; reference point edx = offset RP
; Make the following instruction with address relative to RP:
        jmp     dword [edx+memcmpDispatch-RP]
%ENDIF


align 16
_memcmpAVX2:   ; AVX2 version. Use ymm register
memcmpAVX2@:   ; internal reference
        push    esi
        push    edi
        mov     esi, [esp+12]                    ; ptr1
        mov     edi, [esp+16]                    ; ptr2
        mov     ecx, [esp+20]                    ; size
        add     esi, ecx                         ; use negative index from end of memory block
        add     edi, ecx
        neg     ecx
        jz      A900
        mov     edx, 0FFFFH 
        cmp     ecx, -32
        ja      A100
        
A000:   ; loop comparing 32 bytes
        vmovdqu   ymm1, [esi+ecx]
        vpcmpeqb  ymm0, ymm1, [edi+ecx]          ; compare 32 bytes
        vpmovmskb eax, ymm0                      ; get byte mask
        xor     eax, -1                          ; not eax would not set flags
        jnz     A700                             ; difference found
        add     ecx, 32
        jz      A900                             ; finished, equal
        cmp     ecx, -32
        jna     A000                             ; next 32 bytes
        vzeroupper                               ; end ymm state
        
A100:   ; less than 32 bytes left
        cmp     ecx, -16
        ja      A200
        movdqu  xmm1, [esi+ecx]
        movdqu  xmm2, [edi+ecx]
        pcmpeqb xmm1, xmm2                       ; compare 16 bytes
        pmovmskb eax, xmm1                       ; get byte mask
        xor     eax, edx                         ; not ax
        jnz     A701                             ; difference found
        add     ecx, 16
        jz      A901                             ; finished, equal
        
A200:   ; less than 16 bytes left
        cmp     ecx, -8
        ja      A300
        ; compare 8 bytes
        movq    xmm1, [esi+ecx]
        movq    xmm2, [edi+ecx]
        pcmpeqb xmm1, xmm2                       ; compare 8 bytes
        pmovmskb eax, xmm1                       ; get byte mask
        xor     eax, edx                         ; not ax
        jnz     A701                             ; difference found
        add     ecx, 8
        jz      A901 
        
A300:   ; less than 8 bytes left
        cmp     ecx, -4
        ja      A400
        ; compare 4 bytes
        movd    xmm1, [esi+ecx]
        movd    xmm2, [edi+ecx]
        pcmpeqb xmm1, xmm2                       ; compare 4 bytes
        pmovmskb eax, xmm1                       ; get byte mask
        xor     eax, edx                         ; not ax
        jnz     A701                             ; difference found
        add     ecx, 4
        jz      A901 

A400:   ; less than 4 bytes left
        cmp     ecx, -2
        ja      A500
        movzx   eax, word [esi+ecx]
        movzx   edx, word [edi+ecx]
        sub     eax, edx
        jnz     A800                             ; difference in byte 0 or 1
        add     ecx, 2
        jz      A901 
        
A500:   ; less than 2 bytes left
        test    ecx, ecx
        jz      A901                             ; no bytes left
        
A600:   ; one byte left
        movzx   eax, byte [esi+ecx]
        movzx   edx, byte [edi+ecx]
        sub     eax, edx                         ; return result
        pop     edi
        pop     esi
        ret

A700:   ; difference found. find position
        vzeroupper
A701:   
        bsf     eax, eax
        add     ecx, eax
        movzx   eax, byte [esi+ecx]
        movzx   edx, byte [edi+ecx]
        sub     eax, edx                         ; return result
        pop     edi
        pop     esi
        ret

A800:   ; difference in byte 0 or 1
        neg     al
        sbb     ecx, -1                          ; add 1 to ecx if al == 0
        movzx   eax, byte [esi+ecx]
        movzx   edx, byte [edi+ecx]
        sub     eax, edx                         ; return result
        pop     edi
        pop     esi
        ret

A900:   ; equal
        vzeroupper
A901:   xor     eax, eax        
        pop     edi
        pop     esi
        ret
        

_memcmpSSE2:   ; SSE2 version. Use xmm register
memcmpSSE2@:   ; internal reference

        push    esi
        push    edi
        mov     esi, [esp+12]                    ; ptr1
        mov     edi, [esp+16]                    ; ptr2
        mov     ecx, [esp+20]                    ; size
        add     esi, ecx                         ; use negative index from end of memory block
        add     edi, ecx
        neg     ecx
        jz      S900 
        mov     edx, 0FFFFH
        cmp     ecx, -16
        ja      S200
        
S100:   ; loop comparing 16 bytes
        movdqu  xmm1, [esi+ecx]
        movdqu  xmm2, [edi+ecx]
        pcmpeqb xmm1, xmm2                       ; compare 16 bytes
        pmovmskb eax, xmm1                       ; get byte mask
        xor     eax, edx                         ; not ax
        jnz     S700                             ; difference found
        add     ecx, 16
        jz      S900                             ; finished, equal
        cmp     ecx, -16
        jna     S100                             ; next 16 bytes
        
S200:   ; less than 16 bytes left
        cmp     ecx, -8
        ja      S300
        ; compare 8 bytes
        movq    xmm1, [esi+ecx]
        movq    xmm2, [edi+ecx]
        pcmpeqb xmm1, xmm2                       ; compare 8 bytes
        pmovmskb eax, xmm1                       ; get byte mask
        xor     eax, edx                         ; not ax
        jnz     S700                             ; difference found
        add     ecx, 8
        jz      S900 
        
S300:   ; less than 8 bytes left
        cmp     ecx, -4
        ja      S400
        ; compare 4 bytes
        movd    xmm1, [esi+ecx]
        movd    xmm2, [edi+ecx]
        pcmpeqb xmm1, xmm2                       ; compare 4 bytes
        pmovmskb eax, xmm1                       ; get byte mask
        xor     eax, edx                         ; not ax
        jnz     S700                             ; difference found
        add     ecx, 4
        jz      S900 

S400:   ; less than 4 bytes left
        cmp     ecx, -2
        ja      S500
        movzx   eax, word [esi+ecx]
        movzx   edx, word [edi+ecx]
        sub     eax, edx
        jnz     S800                             ; difference in byte 0 or 1
        add     ecx, 2
        jz      S900 
        
S500:   ; less than 2 bytes left
        test    ecx, ecx
        jz      S900                             ; no bytes left
        
        ; one byte left
        movzx   eax, byte [esi+ecx]
        movzx   edx, byte [edi+ecx]
        sub     eax, edx                         ; return result
        pop     edi
        pop     esi
        ret

S700:   ; difference found. find position
        bsf     eax, eax
        add     ecx, eax
        movzx   eax, byte [esi+ecx]
        movzx   edx, byte [edi+ecx]
        sub     eax, edx                         ; return result
        pop     edi
        pop     esi
        ret

S800:   ; difference in byte 0 or 1
        neg     al
        sbb     ecx, -1                          ; add 1 to ecx if al == 0
S820:   movzx   eax, byte [esi+ecx]
        movzx   edx, byte [edi+ecx]
        sub     eax, edx                         ; return result
        pop     edi
        pop     esi
        ret

S900:   ; equal
        xor     eax, eax        
        pop     edi
        pop     esi
        ret


_memcmp386:    ; 80386 version
memcmp386@:    ; internal reference
        ; This is not perfectly optimized because it is unlikely to ever be used
        push    esi
        push    edi
        mov     esi, [esp+12]                    ; ptr1
        mov     edi, [esp+16]                    ; ptr2
        mov     ecx, [esp+20]                    ; size
        mov     edx, ecx
        shr     ecx, 2                           ; size/4 = number of dwords
        repe    cmpsd                            ; compare dwords
        jnz     M700
        mov     ecx, edx
        and     ecx, 3                           ; remainder
M600:   repe    cmpsb                            ; compare bytes
        je      M800                             ; equal
        movzx   eax, byte [esi-1]                ; esi, edi point past the differing byte. find difference
        movzx   edx, byte [edi-1]
        sub     eax, edx                         ; calculate return value
        pop     edi
        pop     esi
        ret
        
M700:   ; dwords differ. search in last 4 bytes
        mov     ecx, 4
        sub     esi, ecx
        sub     edi, ecx
        jmp     M600
        
M800:   ; equal. return zero
        xor     eax, eax        
        pop     edi
        pop     esi
        ret
        
        
; CPU dispatching for memcmp. This is executed only once
memcmpCPUDispatch:

%IFNDEF POSITIONINDEPENDENT
        call    _InstructionSet                         ; get supported instruction set
        ; Point to generic version of memcmp
        mov     dword [memcmpDispatch],  memcmp386@
        cmp     eax, 4                 ; check SSE2
        jb      Q100
        ; SSE2 supported
        mov     dword [memcmpDispatch],  memcmpSSE2@
        cmp     eax, 13                ; check AVX2
        jb      Q100
        ; AVX2 supported
        mov     dword [memcmpDispatch],  memcmpAVX2@
Q100:   ; Continue in appropriate version of memcmp
        jmp     dword [memcmpDispatch]

%ELSE   ; Position-independent version
        push    edx
        call    _InstructionSet 
        pop     edx
                
        ; Point to generic version of memcmp
        lea     ecx, [edx+memcmp386@-RP]
        cmp     eax, 4                 ; check SSE2
        jb      Q100
        ; Point to SSE2 version of memcmp
        lea     ecx, [edx+memcmpSSE2@-RP]
        cmp     eax, 13                ; check AVX2
        jb      Q100
        ; Point to AVX2 version of memcmp
        lea     ecx, [edx+memcmpAVX2@-RP]
Q100:   mov     [edx+memcmpDispatch-RP], ecx
        ; Continue in appropriate version of memcmp
        jmp     ecx
        
get_thunk_edx: ; load caller address into edx for position-independent code
        mov     edx, [esp]
        ret        
%ENDIF


SECTION .data
align 16


; Pointer to appropriate version.
; This initially points to memcmpCPUDispatch. memcmpCPUDispatch will
; change this to the appropriate version of memcmp, so that
; memcmpCPUDispatch is only executed once:
memcmpDispatch DD memcmpCPUDispatch

 