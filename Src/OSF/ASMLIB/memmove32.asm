;*************************  memmove32.asm  ***********************************
; Author:           Agner Fog
; Date created:     2008-07-18
; Last modified:    2016-11-11
; Description:
; Faster version of the standard memmove function:
; void * A_memmove(void *dest, const void *src, size_t count);
; Moves 'count' bytes from 'src' to 'dest'. src and dest may overlap.
;
; Overriding standard function memmove:
; The alias ?OVR_memmove is changed to _memmove in the object file if
; it is desired to override the standard library function memmove.
;
; Position-independent code is generated if POSITIONINDEPENDENT is defined.
;
; CPU dispatching included for different CPUs
;
; Copyright (c) 2008-2016 GNU General Public License www.gnu.org/licenses
;******************************************************************************

global _A_memmove                      ; Function A_memmove
global ?OVR_memmove                    ; ?OVR removed if standard function memmove overridden

; Direct entries to CPU-specific versions
global _memmove386                     ; Version for processors without SSE2
global _memmoveSSE2                    ; Version for processors with SSE2
global _memmoveSSSE3                   ; Version for processors with SSSE3
global _memmoveU                       ; Version for processors with fast unaligned read
global _memmoveU256                    ; Version for processors with fast 256-bit read/write
global _memmoveAVX512F                 ; Version for processors with fast 512-bit read/write
global _memmoveAVX512BW                ; Version for processors with fast 512-bit read/write
global _SetMemcpyCacheLimit            ; Change limit for bypassing cache

; Imported from memcpy32.asm:
extern _A_memcpy                       ; function entry
extern _memcpy386                      ; CPU specific function entry
extern _memcpySSE2                     ; CPU specific function entry
extern _memcpySSSE3                    ; CPU specific function entry
extern _memcpyU                        ; CPU specific function entry
extern _memcpyU256                     ; CPU specific function entry
extern _memcpyAVX512F                  ; CPU specific function entry
extern _memcpyAVX512BW                 ; CPU specific function entry

; Imported from instrset32.asm
extern _InstructionSet                 ; Instruction set for CPU dispatcher

; Imported from unalignedisfaster32.asm:
extern _UnalignedIsFaster              ; Tells if unaligned read is faster than PALIGNR
extern _Store256BitIsFaster            ; Tells if a 256 bit store is faster than two 128 bit stores

; Imported from memcpy32.asm
extern _GetMemcpyCacheLimit            ; Get the size limit for bypassing cache when copying with memcpy and memmove
extern _SetMemcpyCacheLimit1           ; Set the size limit for bypassing cache when copying with memcpy


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;      Prolog macro. Determine if we should move forwards or backwards
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Define prolog for this function
; Parameter 1 is forward function label
%MACRO  PROLOGM  1
        ; Check if dest overlaps src
        mov     eax, [esp+4]           ; dest
        sub     eax, [esp+8]           ; src
        cmp     eax, [esp+12]          ; count
        ; We can avoid testing for dest < src by using unsigned compare:
        ; (Assume that the memory block cannot span across address 0)
        ; Must move backwards if unsigned(dest-src) < count
        jae     %1                     ; Jump to memcpy if we can move forwards
        
        push    esi
        push    edi
        mov     edi, [esp+12]          ; dest
        mov     esi, [esp+16]          ; src
        mov     ecx, [esp+20]          ; count
%IFDEF  POSITIONINDEPENDENT
        push    ebx
        mov     ebx, edx               ; pointer to reference point RP
%ENDIF
        
%ENDM


; Define return from this function
%MACRO  EPILOGM 0
%IFDEF  POSITIONINDEPENDENT
        pop     ebx
%ENDIF
        pop     edi
        pop     esi
        mov     eax, [esp+4]           ; Return value = dest
        ret
%ENDMACRO


SECTION .text  align=16

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                          Common entry for dispatch
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; extern "C" void * A_memmove(void * dest, const void * src, size_t count);
; Function entry:
_A_memmove:
?OVR_memmove:

%IFNDEF POSITIONINDEPENDENT
        jmp     dword [memmoveDispatch] ; Go to appropriate version, depending on instruction set
RP      equ     0                      ; RP = 0 if not position-independent

%ELSE   ; Position-independent code

        call    get_thunk_edx          ; get reference point for position-independent code
RP:                                    ; reference point edx = offset RP

; Make the following instruction with address relative to RP:
        jmp     dword [edx+memmoveDispatch-RP]

%ENDIF

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; AVX512BW Version for processors with fast unaligned read and fast 512 bit write
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; edi = destination
; esi = source
; ecx = length

align 16 ; short versions
L000:   ; 41H - 80H bytes
        vmovdqu64 zmm0, [esi+ecx-40H]
        vmovdqu64 [edi+ecx-40H], zmm0
        sub     ecx, 40H

L010:   ; 0 - 40H bytes
        mov     eax, -1                ; if count = 1-31: |  if count = 32-63:
        bzhi    eax, eax, ecx          ; -----------------|--------------------
        kmovd   k1, eax                ;       count 1's  |  all 1's
        xor     eax, eax               ;                  |
        sub     ecx, 32                ;                  |
        cmovb   ecx, eax               ;               0  |  count-32
        dec     eax                    ;                  |
        bzhi    eax, eax, ecx          ;                  |
        kmovd   k2, eax                ;               0  |  count-32 1's
        kunpckdq k3, k2, k1            ; low 32 bits from k1, high 32 bits from k2. total = count 1's
        vmovdqu8 zmm0{k3}{z}, [esi]
        vmovdqu8 [edi]{k3}, zmm0
        vzeroupper
        EPILOGM

align 8
_memmoveAVX512BW:                      ; Version for processors with fast 512-bit read/write
memmoveAVX512BW@:                      ; local label
        PROLOGM _memcpyAVX512BW        ; jump to memcpyAVX512BW if copying forwards
        cmp     ecx, 40H
        jbe     L010                   ; Use simpler code if count <= 64
        cmp     ecx, 80H
        jbe     L000                   ; Use simpler code if count <= 128
        
L100:   ; count >= 80H                 ; Entry from memmoveAVX512F
        vmovdqu64 zmm1, [esi]         ; save first possibly unaligned block to after main loop
        vmovdqu64 zmm2, [esi+ecx-40H] ; save last  possibly unaligned block to after main loop

        ; Align destination by 40H
        mov     edx, edi               ; destination
        add     edi, 40H
        and     edi, -40H              ; first 40H boundary of destination
        lea     eax, [edx+ecx]         ; end of destination
        sub     edx, edi               ; -offset
        sub     esi, edx               ; add same offset to source
        add     ecx, edx               ; size minus first unaligned part
        and     ecx, -40H              ; round down size to multiple of 40H
        add     edx, edi               ; restore destination
        
        ; Check if count very big
        cmp     ecx, [_CacheBypassLimit]
        ja      L800                   ; Use non-temporal store if count > CacheBypassLimit

%define nop7  db 0FH, 1FH, 80H, 00H, 00H, 00H, 00H
        nop7

;align   16 
L200:   ; Main move loop
        sub      ecx, 40H
        vmovdqu64 zmm0, [esi+ecx]
        vmovdqa64 [edi+ecx], zmm0
        jnz      L200
        
L300:   ; insert remaining parts in beginning and end, possibly overwriting what has already been stored
        vmovdqu64 [eax-40H], zmm2
        vmovdqu64 [edx], zmm1
        vzeroupper
        EPILOGM

align 16
L800:  ; move loop, bypass cache
        sub      ecx, 40H
        vmovdqu64 zmm0, [esi+ecx]
        vmovntdq [edi+ecx], zmm0
        jnz      L800
        sfence
        jmp      L300

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; AVX512F Version for processors with fast unaligned read and fast 512 bit write
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; edi = destination
; esi = source
; ecx = length

align 16
_memmoveAVX512F:                       ; Version for processors with fast 512-bit read/write
memmoveAVX512F@:                       ; local label
        PROLOGM _memcpyAVX512F         ; jump to memcpyAVX512F if copying forwards
        cmp     ecx, 80H
        jnb     L100                   ; use same code as memmoveAVX512BW if count >= 80H
        cmp     ecx, 40H
        jb      A1000                  ; use code below if count < 40H
        ; count = 40H - 80H. Make two partially overlapping 40H blocks
        vmovdqu64 zmm0, [esi]
        vmovdqu64 zmm1, [esi+ecx-40H]
        vmovdqu64 [edi], zmm0
        vmovdqu64 [edi+ecx-40H], zmm1
        vzeroupper
        EPILOGM


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; AVX Version for processors with fast unaligned read and fast 256 bits write
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align 16
_memmoveU256:  ; Version for processors with fast 256-bit read/write
%IFDEF POSITIONINDEPENDENT
        call    get_thunk_edx
        add     edx, RP-$
%ENDIF
memmoveU256@:
        PROLOGM _memcpyU256
      
        cmp     ecx, 40H
        jb      A1000                    ; Use simpler code if count < 64
        
        ; count >= 64
        ; Note: this part will not always work if count < 64
        ; Calculate size of last block after last regular boundary of dest
        lea     edx, [edi+ecx]         ; end of dext
        and     edx, 1FH
        jz      B4300                   ; Skip if end of dest aligned by 32
        
        ; edx = size of last partial block, 1 - 32 bytes
        test    dl, 3
        jz      B4210
        test    dl, 1
        jz      B4201      ; B4200 if we haven't tested edx,3
        ; move 1 byte
        dec     ecx
        movzx   eax, byte [esi+ecx]
        mov     [edi+ecx], al        
B4200:  test    dl, 2
        jz      B4210
B4201:  ; move 2 bytes
        sub     ecx, 2
        movzx   eax, word [esi+ecx]
        mov     [edi+ecx], ax        
B4210:  test    dl, 4
        jz      B4220
        ; move 4 bytes
        sub     ecx, 4
        mov     eax, [esi+ecx]
        mov     [edi+ecx], eax
B4220:  test    dl, 8
        jz      B4230
        ; move 8 bytes
        sub     ecx, 8
        movq    xmm0, qword [esi+ecx]
        movq    qword [edi+ecx], xmm0        
B4230:  test    dl, 16
        jz      B4300
        ; move 16 bytes
        sub     ecx, 16
        movups  xmm0, [esi+ecx]
        movaps  [edi+ecx], xmm0
        
B4300:  ; Now end of dest is aligned by 32. Any partial block has been moved        
        mov     edx, ecx
        and     ecx, 1FH               ; remaining size after 32 bytes blocks moved
        and     edx, -20H              ; number of 32 bytes blocks
        jz      H4100
        add     esi, ecx
        add     edi, ecx
        
        ; Check if count very big
%IFNDEF POSITIONINDEPENDENT
        cmp     edx, [_CacheBypassLimit]
%ELSE
        cmp     edx, [ebx-RP+_CacheBypassLimit]
%ENDIF
        ja      H4800                   ; Use non-temporal store if count > _CacheBypassLimit
        
align 16
H4000:  ; 32 bytes move loop
        vmovups  ymm0, [esi+edx-20H]
        vmovaps  [edi+edx-20H], ymm0
        sub      edx, 20H
        jnz      H4000
        vzeroupper
        
H4090:  sub      esi, ecx
        sub      edi, ecx

H4100:  ; remaining 0-31 bytes
        test    ecx, ecx
        jz      H4600        
        test    cl, 10H
        jz      H4200
        ; move 16 bytes
        sub     ecx, 10H
        movups  xmm0, [esi+ecx]
        movaps  [edi+ecx], xmm0
        jz      H4600                     ; early out if count divisible by 16
H4200:  test    cl, 8
        jz      H4300
        ; move 8 bytes
        sub     ecx, 8
        movq    xmm0, qword [esi+ecx]
        movq    qword [edi+ecx], xmm0
H4300:  test    cl, 4
        jz      H4400
        ; move 4 bytes
        sub     ecx, 4
        mov     eax, [esi+ecx]
        mov     [edi+ecx], eax
        jz      H4600                     ; early out if count divisible by 4
H4400:  test    cl, 2
        jz      H4500
        ; move 2 bytes
        sub     ecx, 2
        movzx   eax, word [esi+ecx]
        mov     [edi+ecx], ax
H4500:  test    cl, 1
        jz      H4600
        ; move 1 byte
        movzx   eax, byte [esi]   ; ecx-1 = 0
        mov     [edi], al
H4600:  ; finished
        EPILOGM

align 16
H4800:  ; 32 bytes move loop, bypass cache
        vmovups  ymm0, [esi+edx-20H]
        vmovntps [edi+edx-20H], ymm0
        sub      edx, 20H
        jnz      H4800
        vzeroupper
        sfence
        jmp      H4090
        
        
        ; count < 64. Move 32-16-8-4-2-1 bytes
        ; multiple CPU versions, SSSE3 and later
A1000:  test    cl, 20H
        jz      A1100
        ; move 32 bytes
        ; movups is faster than 64-bit moves on processors with SSSE3
        sub     ecx, 20H
        movups  xmm0, [esi+ecx+10H]
        movups  xmm1, [esi+ecx]
        movups  [edi+ecx+10H], xmm0
        movups  [edi+ecx], xmm1
A1100:  test    cl, 10H
        jz      A1200
        ; move 16 bytes
        sub     ecx, 10H
        movups  xmm0, [esi+ecx]
        movups  [edi+ecx], xmm0
A1200:  test    cl, 8
        jz      A1300
        ; move 8 bytes
        sub     ecx, 8
        movq    xmm0, qword [esi+ecx]
        movq    qword [edi+ecx], xmm0
A1300:  test    cl, 4
        jz      A1400
        ; move 4 bytes
        sub     ecx, 4
        mov     eax, [esi+ecx]
        mov     [edi+ecx], eax
        jz      A1900                     ; early out if count divisible by 4
A1400:  test    cl, 2
        jz      A1500
        ; move 2 bytes
        sub     ecx, 2
        movzx   eax, word [esi+ecx]
        mov     [edi+ecx], ax
A1500:  test    cl, 1
        jz      A1900
        ; move 1 byte
        movzx   eax, byte [esi]    ; ecx-1 = 0
        mov     [edi], al
A1900:  ; finished
        EPILOGM
        
        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  Version for processors with fast unaligned read and fast 16 bytes write
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                
align 16
_memmoveU:  ; Version for processors with fast unaligned read
%IFDEF POSITIONINDEPENDENT
        call    get_thunk_edx
        add     edx, RP-$
%ENDIF
memmoveU@:
        PROLOGM _memcpyU
      
        cmp     ecx, 40H
        jb      A1000                    ; Use simpler code if count < 64
        
        ; count >= 64
        ; Note: this part will not always work if count < 64
        ; Calculate size of last block after last regular boundary of dest
        lea     edx, [edi+ecx]         ; end of dext
        and     edx, 0FH
        jz      B3300                   ; Skip if end of dest aligned by 16
        
        ; edx = size of last partial block, 1 - 15 bytes
        test    dl, 3
        jz      B3210
        test    dl, 1
        jz      B3201      ; B3200 if we haven't tested edx,3
        ; move 1 byte
        dec     ecx
        movzx   eax, byte [esi+ecx]
        mov     [edi+ecx], al        
B3200:  test    dl, 2
        jz      B3210
B3201:  ; move 2 bytes
        sub     ecx, 2
        movzx   eax, word [esi+ecx]
        mov     [edi+ecx], ax        
B3210:  test    dl, 4
        jz      B3220
        ; move 4 bytes
        sub     ecx, 4
        mov     eax, [esi+ecx]
        mov     [edi+ecx], eax
B3220:  test    dl, 8
        jz      B3300
        ; move 8 bytes
        sub     ecx, 8
        movq    xmm0, qword [esi+ecx]
        movq    qword [edi+ecx], xmm0        
        
B3300:  ; Now end of dest is aligned by 16. Any partial block has been moved        
        mov      edx, ecx
        and      ecx, 1FH              ; remaining size after 32 bytes blocks moved
        and      edx, -20H             ; number of 32 bytes blocks
        jz       H1100
        add      esi, ecx
        add      edi, ecx
        
        ; Check if count very big
%IFNDEF POSITIONINDEPENDENT
        cmp     edx, [_CacheBypassLimit]
%ELSE
        cmp     edx, [ebx+_CacheBypassLimit-RP]
%ENDIF
        ja      H1800                   ; Use non-temporal store if count > _CacheBypassLimit
        
align 16
H1000:  ; 32 bytes move loop
        movups   xmm1, [esi+edx-20H]
        movups   xmm0, [esi+edx-10H]
        movaps   [edi+edx-20H], xmm1
        movaps   [edi+edx-10H], xmm0
        sub      edx, 20H
        jnz      H1000
        
H1090:  sub      esi, ecx
        sub      edi, ecx

H1100:  ; remaining 0-31 bytes
        test    ecx, ecx
        jz      H1600        
        test    cl, 10H
        jz      H1200
        ; move 16 bytes
        sub     ecx, 10H
        movups  xmm0, [esi+ecx]
        movaps  [edi+ecx], xmm0
        jz      H1600                     ; early out if count divisible by 16
H1200:  test    cl, 8
        jz      H1300
        ; move 8 bytes
        sub     ecx, 8
        movq    xmm0, qword [esi+ecx]
        movq    qword [edi+ecx], xmm0
H1300:  test    cl, 4
        jz      H1400
        ; move 4 bytes
        sub     ecx, 4
        mov     eax, [esi+ecx]
        mov     [edi+ecx], eax
        jz      H1600                     ; early out if count divisible by 4
H1400:  test    cl, 2
        jz      H1500
        ; move 2 bytes
        sub     ecx, 2
        movzx   eax, word [esi+ecx]
        mov     [edi+ecx], ax
H1500:  test    cl, 1
        jz      H1600
        ; move 1 byte
        movzx   eax, byte [esi]   ; ecx-1 = 0
        mov     [edi], al
H1600:  ; finished
        EPILOGM

align 16
H1800:  ; 32 bytes move loop, bypass cache
        movups   xmm1, [esi+edx-20H]
        movups   xmm0, [esi+edx-10H]
        movntps  [edi+edx-20H], xmm1
        movntps  [edi+edx-10H], xmm0
        sub      edx, 20H
        jnz      H1800        
        sfence
        jmp      H1090
        
        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  Version for processors with SSSE3. Aligned read + shift + aligned write
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        
align 16
_memmoveSSSE3:  ; SSSE3 version begins here
%IFDEF POSITIONINDEPENDENT
        call    get_thunk_edx
        add     edx, RP-$
%ENDIF
memmoveSSSE3@:
        PROLOGM    _memcpySSSE3
      
        cmp     ecx, 40H
        jb      A1000                    ; Use simpler code if count < 64
        
        ; count >= 64
        ; Note: this part will not always work if count < 64
        ; Calculate size of last block after last regular boundary of dest
        lea     edx, [edi+ecx]         ; end of dext
        and     edx, 0FH
        jz      B1300                   ; Skip if end of dest aligned by 16
        
        ; edx = size of last partial block, 1 - 15 bytes
        test    dl, 3
        jz      B1210
        test    dl, 1
        jz      B1201      ; B1200 if we haven't tested edx,3
        ; move 1 byte
        dec     ecx
        movzx   eax, byte [esi+ecx]
        mov     [edi+ecx], al        
B1200:  test    dl, 2
        jz      B1210
B1201:  ; move 2 bytes
        sub     ecx, 2
        movzx   eax, word [esi+ecx]
        mov     [edi+ecx], ax        
B1210:  test    dl, 4
        jz      B1220
        ; move 4 bytes
        sub     ecx, 4
        mov     eax, [esi+ecx]
        mov     [edi+ecx], eax
B1220:  test    dl, 8
        jz      B1300
        ; move 8 bytes
        sub     ecx, 8
        movq    xmm0, qword [esi+ecx]
        movq    qword [edi+ecx], xmm0        
        
B1300:  ; Now end of dest is aligned by 16. Any partial block has been moved        
        ; Find alignment of end of src modulo 16 at this point:
        lea     eax, [esi+ecx]
        and     eax, 0FH
        
        ; Set up for loop moving 32 bytes per iteration:
        mov     edx, ecx               ; Save count
        and     ecx, -20H              ; Round down to nearest multiple of 32
        sub     edx, ecx               ; Remaining data after loop
        sub     esi, eax               ; Nearest preceding aligned block of src
        ; Add the same to esi and edi as we have subtracted from ecx
        add     esi, edx
        add     edi, edx
        
%IFNDEF POSITIONINDEPENDENT
        ; Check if count very big
        cmp     ecx, [_CacheBypassLimit]
        ja      B1400                   ; Use non-temporal store if count > _CacheBypassLimit
        
        ; Dispatch to different codes depending on src alignment
        jmp     [MAlignmentDispatchSSSE3+eax*4]

B1400:  ; Dispatch to different codes depending on src alignment
        jmp     [MAlignmentDispatchNT+eax*4]

%ELSE   ; Position-independent code

        ; Check if count very big
        ; Make the following instruction with address relative to RP:
        cmp     ecx, [ebx-RP+_CacheBypassLimit]
        ja      B1400                   ; Use non-temporal store if count > _CacheBypassLimit
        
        ; Dispatch to different codes depending on src alignment
        ; MAlignmentDispatch table contains addresses relative to RP
        ; Add table entry to ebx=RP to get jump address.

        ; Make the following instruction with address relative to RP:
        add     ebx,[ebx-RP+MAlignmentDispatchSSSE3+eax*4]
        jmp     ebx
        
B1400:  ; Same with MAlignmentDispatchNT:        
        add     ebx,[ebx-RP+MAlignmentDispatchNT+eax*4]
        jmp     ebx        
%ENDIF


align   16
C100:   ; Code for aligned src. SSE2 or later instruction set
        ; The nice case, src and dest have same alignment.

        ; Loop. ecx has positive index from the beginning, counting down to zero
        movaps  xmm0, [esi+ecx-10H]
        movaps  xmm1, [esi+ecx-20H]
        movaps  [edi+ecx-10H], xmm0
        movaps  [edi+ecx-20H], xmm1
        sub     ecx, 20H
        jnz     C100
        
        ; Move the remaining edx bytes (0 - 31):
        ; move 16-8-4-2-1 bytes, aligned
        test    edx, edx
        jz      C500                   ; Early out if no more data
        test    dl, 10H
        jz      C200
        ; move 16 bytes
        sub     ecx, 10H
        movaps  xmm0, [esi+ecx]
        movaps  [edi+ecx], xmm0
        
C200:   ; Other branches come in here, ecx may contain arbitrary offset
        test    edx, edx
        jz      C500                   ; Early out if no more data
        test    dl, 8
        jz      C210        
        ; move 8 bytes
        sub     ecx, 8 
        movq    xmm0, qword [esi+ecx]
        movq    qword [edi+ecx], xmm0
C210:   test    dl, 4
        jz      C220        
        ; move 4 bytes
        sub     ecx, 4        
        mov     eax, [esi+ecx]
        mov     [edi+ecx], eax
        jz      C500                   ; Early out if count divisible by 4
C220:   test    dl, 2
        jz      C230        
        ; move 2 bytes
        sub     ecx, 2
        movzx   eax, word [esi+ecx]
        mov     [edi+ecx], ax
C230:   test    dl, 1
        jz      C500        
        ; move 1 byte
        movzx   eax, byte [esi+ecx-1]    ; ecx-1 not always 0
        mov     [edi+ecx-1], al
C500:   ; finished     
        EPILOGM


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  Version for processors with SSE2. Aligned read + shift + aligned write
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        
align 16
_memmoveSSE2:   ; SSE2 version begins here
%IFDEF POSITIONINDEPENDENT
        call    get_thunk_edx
        add     edx, RP-$
%ENDIF
memmoveSSE2@:
        PROLOGM _memcpySSE2
      
        cmp     ecx, 40H
        jae     B0100                    ; Use simpler code if count < 64
        
        ; count < 64. Move 32-16-8-4-2-1 bytes
        test    cl, 20H
        jz      A100
        ; move 32 bytes
        ; movq is faster than movdqu on Intel Pentium M and Core 1
        ; movdqu is faster on later processors
        sub     ecx, 20H
        movq    xmm0, qword [esi+ecx+18H]
        movq    xmm1, qword [esi+ecx+10H]
        movq    xmm2, qword [esi+ecx+8]
        movq    xmm3, qword [esi+ecx]
        movq    qword [edi+ecx+18H], xmm0
        movq    qword [edi+ecx+10H], xmm1
        movq    qword [edi+ecx+8], xmm2
        movq    qword [edi+ecx], xmm3
A100:   test    cl, 10H
        jz      A200
        ; move 16 bytes
        sub     ecx, 10H
        movq    xmm0, qword [esi+ecx+8]
        movq    xmm1, qword [esi+ecx]
        movq    qword [edi+ecx+8], xmm0
        movq    qword [edi+ecx], xmm1
A200:   test    cl, 8
        jz      A300
        ; move 8 bytes
        sub     ecx, 8
        movq    xmm0, qword [esi+ecx]
        movq    qword [edi+ecx], xmm0
A300:   test    cl, 4
        jz      A400
        ; move 4 bytes
        sub     ecx, 4
        mov     eax, [esi+ecx]
        mov     [edi+ecx], eax
        jz      A900                     ; early out if count divisible by 4
A400:   test    cl, 2
        jz      A500
        ; move 2 bytes
        sub     ecx, 2
        movzx   eax, word [esi+ecx]
        mov     [edi+ecx], ax
A500:   test    cl, 1
        jz      A900
        ; move 1 byte
        movzx   eax, byte [esi]    ; ecx-1 = 0
        mov     [edi], al
A900:   ; finished
        EPILOGM
        
B0100:  ; count >= 64
        ; This part will not always work if count < 64
        ; Calculate size of last block after last regular boundary of dest
        lea     edx, [edi+ecx]         ; end of dest
        and     edx, 0FH
        jz      B0300                   ; Skip if end of dest aligned by 16
        
        ; edx = size of last partial block, 1 - 15 bytes
        test    dl, 3
        jz      B0210
        test    dl, 1
        jz      B0201      ; B0200 if we haven't tested edx,3
        ; move 1 byte
        dec     ecx
        movzx   eax, byte [esi+ecx]
        mov     [edi+ecx], al        
B0200:  test    dl, 2
        jz      B0210
B0201:  ; move 2 bytes
        sub     ecx, 2
        movzx   eax, word [esi+ecx]
        mov     [edi+ecx], ax        
B0210:  test    dl, 4
        jz      B0220
        ; move 4 bytes
        sub     ecx, 4
        mov     eax, [esi+ecx]
        mov     [edi+ecx], eax
B0220:  test    dl, 8
        jz      B0300
        ; move 8 bytes
        sub     ecx, 8
        movq    xmm0, qword [esi+ecx]
        movq    qword [edi+ecx], xmm0        
        
B0300:  ; Now end of dest is aligned by 16. Any partial block has been moved        
        ; Find alignment of end of src modulo 16 at this point:
        lea     eax, [esi+ecx]
        and     eax, 0FH
        
        ; Set up for loop moving 32 bytes per iteration:
        mov     edx, ecx               ; Save count
        and     ecx, -20H              ; Round down to nearest multiple of 32
        sub     edx, ecx               ; Remaining data after loop
        sub     esi, eax               ; Nearest preceding aligned block of src
        ; Add the same to esi and edi as we have subtracted from ecx
        add     esi, edx
        add     edi, edx
        
%IFNDEF POSITIONINDEPENDENT
        ; Check if count very big
        cmp     ecx, [_CacheBypassLimit]
        ja      B0400                   ; Use non-temporal store if count > _CacheBypassLimit
        
        ; Dispatch to different codes depending on src alignment
        jmp     [MAlignmentDispatchSSE2+eax*4]

B0400:   ; Dispatch to different codes depending on src alignment
        jmp     [MAlignmentDispatchNT+eax*4]

%ELSE   ; Position-independent code

        ; Check if count very big
        ; Make the following instruction with address relative to RP:
        cmp     ecx, [ebx-RP+_CacheBypassLimit]
        ja      B0400                   ; Use non-temporal store if count > _CacheBypassLimit
        
        ; Dispatch to different codes depending on src alignment
        ; MAlignmentDispatch table contains addresses relative to RP
        ; Add table entry to ebx=RP to get jump address.

        ; Make the following instruction with address relative to RP:
        add     ebx,[ebx-RP+MAlignmentDispatchSSE2+eax*4]
        jmp     ebx
        
B0400:   ; Same with MAlignmentDispatchNT:        
        add     ebx,[ebx-RP+MAlignmentDispatchNT+eax*4]
        jmp     ebx        
%ENDIF


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  Macros and alignment jump tables
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Macros for each src alignment, SSE2 instruction set:
; Make separate code for each alignment u because the shift instructions
; have the shift count as a constant:

%MACRO MOVE_REVERSE_UNALIGNED_SSE2 2
; Move ecx + edx bytes of data
; Source is misaligned. (src-dest) modulo 16 = %1
; %2 = 1 if non-temporal store desired
; eax = %1
; esi = src - %1 = nearest preceding 16-bytes boundary
; edi = dest (aligned)
; ecx = count rounded down to nearest divisible by 32
; edx = remaining bytes to move after loop
        movdqa  xmm0, [esi+ecx]        ; Read from nearest following 16B boundary        
%%L1:   ; Loop. ecx has positive index from the beginning, counting down to zero
        sub     ecx, 20H
        movdqa  xmm1, [esi+ecx+10H]    ; Read next two blocks aligned
        movdqa  xmm2, [esi+ecx]
        movdqa  xmm3, xmm1             ; Copy because used twice
        pslldq  xmm0, 16-%1             ; shift left
        psrldq  xmm1, %1                ; shift right
        por     xmm0, xmm1             ; combine blocks
        %IF %2 == 0
        movdqa  [edi+ecx+10H], xmm0    ; Save aligned
        %ELSE
        movntdq [edi+ecx+10H], xmm0    ; Save aligned
        %ENDIF
        movdqa  xmm0, xmm2             ; Save for next iteration
        pslldq  xmm3, 16-%1             ; shift left
        psrldq  xmm2, %1                ; shift right
        por     xmm3, xmm2             ; combine blocks
        %IF %2 == 0
        movdqa  [edi+ecx], xmm3        ; Save aligned
        %ELSE
        movntdq [edi+ecx], xmm3        ; Save aligned
        %ENDIF
        jnz     %%L1
                
        ; Move edx remaining bytes
        test    dl, 10H
        jz      %%L2
        ; One more 16-bytes block to move
        sub     ecx, 10H
        movdqa  xmm1, [esi+ecx]
        pslldq  xmm0, 16-%1             ; shift left
        psrldq  xmm1, %1                ; shift right
        por     xmm0, xmm1              ; combine blocks
        %IF %2 == 0
        movdqa  [edi+ecx], xmm0        ; Save aligned
        %ELSE
        movntdq [edi+ecx], xmm0        ; Save aligned
        %ENDIF        
%%L2:   ; Get src pointer back to misaligned state
        add     esi, eax
        %IF %2 == 1
        sfence
        %ENDIF        
        ; Move remaining 0 - 15 bytes, unaligned
        jmp     C200
%ENDMACRO

%MACRO MOVE_REVERSE_UNALIGNED_SSE2_4 1
; Special case: u = 4
        movaps  xmm0, [esi+ecx]        ; Read from nearest following 16B boundary
%%L1:   ; Loop. ecx has positive index from the beginning, counting down to zero
        sub     ecx, 20H
        movaps  xmm1, [esi+ecx+10H]    ; Read next two blocks aligned
        movaps  xmm2, [esi+ecx]
        movaps  xmm3, xmm0
        movaps  xmm0, xmm2        
        movss   xmm2, xmm1
        shufps  xmm2, xmm2, 00111001B  ; Rotate right
        movss   xmm1, xmm3
        shufps  xmm1, xmm1, 00111001B  ; Rotate right
        %IF %1 == 0
        movaps  [edi+ecx+10H], xmm1    ; Save aligned
        movaps  [edi+ecx], xmm2        ; Save aligned
        %ELSE
        movntps [edi+ecx+10H], xmm1    ; Non-temporal save
        movntps [edi+ecx], xmm2        ; Non-temporal save
        %ENDIF
        jnz     %%L1
                
        ; Move edx remaining bytes
        test    dl, 10H
        jz      %%L2
        ; One more 16-bytes block to move
        sub     ecx, 10H
        movaps  xmm1, [esi+ecx]
        movss   xmm1, xmm0
        shufps  xmm1, xmm1, 00111001B  ; Rotate right
        %IF %1 == 0
        movaps  [edi+ecx], xmm1        ; Save aligned
        %ELSE
        movntps [edi+ecx], xmm1        ; Non-temporal save
        %ENDIF        
%%L2:   ; Get src pointer back to misaligned state
        add     esi, eax
        %IF %1 == 1
        sfence
        %ENDIF        
        ; Move remaining 0 - 15 bytes, unaligned
        jmp     C200
%ENDMACRO

%MACRO  MOVE_REVERSE_UNALIGNED_SSE2_8  1
; Special case: u = 8
        movaps  xmm0, [esi+ecx]        ; Read from nearest following 16B boundary
        shufps  xmm0, xmm0, 01001110B  ; Rotate
%%L1:   ; Loop. ecx has positive index from the beginning, counting down to zero
        sub     ecx, 20H
        movaps  xmm1, [esi+ecx+10H]    ; Read next two blocks aligned
        shufps  xmm1, xmm1, 01001110B  ; Rotate
        movsd   xmm0, xmm1
        %IF %1 == 0
        movaps  [edi+ecx+10H], xmm0    ; Save aligned
        %ELSE
        movntps [edi+ecx+10H], xmm0    ; Non-temporal save
        %ENDIF
        movaps  xmm0, [esi+ecx]
        shufps  xmm0, xmm0, 01001110B  ; Rotate
        movsd   xmm1, xmm0
        %IF %1 == 0
        movaps  [edi+ecx], xmm1        ; Save aligned
        %ELSE
        movntps [edi+ecx], xmm1        ; Non-temporal save
        %ENDIF
        jnz     %%L1
                
        ; Move edx remaining bytes
        test    dl, 10H
        jz      %%L2
        ; One more 16-bytes block to move
        sub     ecx, 10H
        movaps  xmm1, [esi+ecx]
        shufps  xmm1, xmm1, 01001110B  ; Rotate 
        movsd   xmm0, xmm1
        %IF %1 == 0
        movaps  [edi+ecx], xmm0        ; Save aligned
        %ELSE
        movntps [edi+ecx], xmm0        ; Non-temporal save
        %ENDIF        
%%L2:   ; Get src pointer back to misaligned state
        add     esi, eax
        %IF %1 == 1
        sfence
        %ENDIF        
        ; Move remaining 0 - 15 bytes, unaligned
        jmp     C200
%ENDMACRO

%MACRO  MOVE_REVERSE_UNALIGNED_SSE2_12  1
; Special case: u = 12
        movaps  xmm0, [esi+ecx]        ; Read from nearest following 16B boundary
        shufps  xmm0, xmm0, 10010011B  ; Rotate right
%%L1:   ; Loop. ecx has positive index from the beginning, counting down to zero
        sub     ecx, 20H
        movaps  xmm1, [esi+ecx+10H]    ; Read next two blocks aligned
        shufps  xmm1, xmm1, 10010011B  ; Rotate left
        movss   xmm0, xmm1
        %IF %1 == 0
        movaps  [edi+ecx+10H], xmm0    ; Save aligned
        %ELSE
        movntps [edi+ecx+10H], xmm0    ; Non-temporal save
        %ENDIF
        movaps  xmm0, [esi+ecx]
        shufps  xmm0, xmm0, 10010011B  ; Rotate left
        movss   xmm1, xmm0
        %IF %1 == 0
        movaps  [edi+ecx], xmm1        ; Save aligned
        %ELSE
        movntps [edi+ecx], xmm1        ; Non-temporal save
        %ENDIF
        jnz     %%L1
                
        ; Move edx remaining bytes
        test    dl, 10H
        jz      %%L2
        ; One more 16-bytes block to move
        sub     ecx, 10H
        movaps  xmm1, [esi+ecx]
        shufps  xmm1, xmm1, 10010011B  ; Rotate left
        movss   xmm0, xmm1
        %IF %1 == 0
        movaps  [edi+ecx], xmm0        ; Save aligned
        %ELSE
        movntps [edi+ecx], xmm0        ; Non-temporal save
        %ENDIF        
%%L2:   ; Get src pointer back to misaligned state
        add     esi, eax
        %IF %1 == 1
        sfence
        %ENDIF        
        ; Move remaining 0 - 15 bytes, unaligned
        jmp     C200
%ENDMACRO

; Macros for each src alignment, Suppl.SSE3 instruction set:
; Code for unaligned src, Suppl.SSE3 instruction set.
; Make separate code for each alignment u because the palignr instruction
; has the shift count as a constant:

%MACRO MOVE_REVERSE_UNALIGNED_SSSE3  1
; Move ecx + edx bytes of data
; Source is misaligned. (src-dest) modulo 16 = %1
; eax = %1
; esi = src - %1 = nearest preceding 16-bytes boundary
; edi = dest (aligned)
; ecx = - (count rounded down to nearest divisible by 32)
; edx = remaining bytes to move after loop
        movdqa  xmm0, [esi+ecx]        ; Read from nearest following 16B boundary
        
%%L1:   ; Loop. ecx has positive index from the beginning, counting down to zero
        movdqa  xmm1, [esi+ecx-10H]    ; Read next two blocks        
        palignr xmm0, xmm1, %1         ; Combine parts into aligned block
        movdqa  [edi+ecx-10H], xmm0    ; Save aligned
        movdqa  xmm0, [esi+ecx-20H]
        palignr xmm1, xmm0, %1         ; Combine parts into aligned block
        movdqa  [edi+ecx-20H], xmm1    ; Save aligned
        sub     ecx, 20H
        jnz     %%L1
        
        ; Set up for edx remaining bytes
        test    dl, 10H
        jz      %%L2
        ; One more 16-bytes block to move
        sub     ecx, 10H
        movdqa  xmm1, [esi+ecx]        ; Read next two blocks        
        palignr xmm0, xmm1, %1         ; Combine parts into aligned block
        movdqa  [edi+ecx], xmm0        ; Save aligned
        
%%L2:   ; Get src pointer back to misaligned state
        add     esi, eax
        ; Move remaining 0 - 15 bytes
        jmp     C200
%ENDMACRO

; Make 15 instances of SSE2 macro for each value of the alignment u.
; These are pointed to by the jump table MAlignmentDispatchSSE2 below
; (aligns and fillers are inserted manually to minimize the 
;  number of 16-bytes boundaries inside loops)

align 16
D104:   MOVE_REVERSE_UNALIGNED_SSE2_4    0
D108:   MOVE_REVERSE_UNALIGNED_SSE2_8    0
D10C:   MOVE_REVERSE_UNALIGNED_SSE2_12   0
D101:   MOVE_REVERSE_UNALIGNED_SSE2 1,   0
D102:   MOVE_REVERSE_UNALIGNED_SSE2 2,   0
D103:   MOVE_REVERSE_UNALIGNED_SSE2 3,   0
D105:   MOVE_REVERSE_UNALIGNED_SSE2 5,   0
D106:   MOVE_REVERSE_UNALIGNED_SSE2 6,   0
D107:   MOVE_REVERSE_UNALIGNED_SSE2 7,   0
D109:   MOVE_REVERSE_UNALIGNED_SSE2 9,   0
D10A:   MOVE_REVERSE_UNALIGNED_SSE2 0AH, 0
D10B:   MOVE_REVERSE_UNALIGNED_SSE2 0BH, 0
D10D:   MOVE_REVERSE_UNALIGNED_SSE2 0DH, 0
D10E:   MOVE_REVERSE_UNALIGNED_SSE2 0EH, 0
D10F:   MOVE_REVERSE_UNALIGNED_SSE2 0FH, 0

; Make 15 instances of Sup.SSE3 macro for each value of the alignment u.
; These are pointed to by the jump table MAlignmentDispatchSupSSE3 below

align 16
E104:   MOVE_REVERSE_UNALIGNED_SSSE3 4
E108:   MOVE_REVERSE_UNALIGNED_SSSE3 8
E10C:   MOVE_REVERSE_UNALIGNED_SSSE3 0CH
E101:   MOVE_REVERSE_UNALIGNED_SSSE3 1
E102:   MOVE_REVERSE_UNALIGNED_SSSE3 2
E103:   MOVE_REVERSE_UNALIGNED_SSSE3 3
E105:   MOVE_REVERSE_UNALIGNED_SSSE3 5
E106:   MOVE_REVERSE_UNALIGNED_SSSE3 6
E107:   MOVE_REVERSE_UNALIGNED_SSSE3 7
E109:   MOVE_REVERSE_UNALIGNED_SSSE3 9
E10A:   MOVE_REVERSE_UNALIGNED_SSSE3 0AH
E10B:   MOVE_REVERSE_UNALIGNED_SSSE3 0BH
E10D:   MOVE_REVERSE_UNALIGNED_SSSE3 0DH
E10E:   MOVE_REVERSE_UNALIGNED_SSSE3 0EH
E10F:   MOVE_REVERSE_UNALIGNED_SSSE3 0FH

align 16        
F100:   ; Non-temporal move, src and dest have same alignment.
        ; Loop. ecx has positive index from the beginning, counting down to zero
        sub     ecx, 20H
        movaps  xmm0, [esi+ecx+10H]
        movaps  xmm1, [esi+ecx]
        movntps [edi+ecx+10H], xmm0
        movntps [edi+ecx], xmm1
        jnz     F100
        
        ; Move the remaining edx bytes (0 - 31):
        ; move 16-8-4-2-1 bytes, aligned
        test    dl, 10H
        jz      C200
        ; move 16 bytes
        sub     ecx, 10H
        movaps  xmm0, [esi+ecx]
        movntps  [edi+ecx], xmm0
        sfence
        ; move the remaining 0 - 15 bytes
        jmp     C200

; Non-temporal move, src and dest have different alignment.
; Make 15 instances of SSE2 macro for each value of the alignment u.
; These are pointed to by the jump table MAlignmentDispatchNT below

align 16
F104:   MOVE_REVERSE_UNALIGNED_SSE2_4    1
F108:   MOVE_REVERSE_UNALIGNED_SSE2_8    1
F10C:   MOVE_REVERSE_UNALIGNED_SSE2_12   1
F101:   MOVE_REVERSE_UNALIGNED_SSE2 1,   1
F102:   MOVE_REVERSE_UNALIGNED_SSE2 2,   1
F103:   MOVE_REVERSE_UNALIGNED_SSE2 3,   1
F105:   MOVE_REVERSE_UNALIGNED_SSE2 5,   1
F106:   MOVE_REVERSE_UNALIGNED_SSE2 6,   1
F107:   MOVE_REVERSE_UNALIGNED_SSE2 7,   1
F109:   MOVE_REVERSE_UNALIGNED_SSE2 9,   1
F10A:   MOVE_REVERSE_UNALIGNED_SSE2 0AH, 1
F10B:   MOVE_REVERSE_UNALIGNED_SSE2 0BH, 1
F10D:   MOVE_REVERSE_UNALIGNED_SSE2 0DH, 1
F10E:   MOVE_REVERSE_UNALIGNED_SSE2 0EH, 1
F10F:   MOVE_REVERSE_UNALIGNED_SSE2 0FH, 1

%IFDEF  POSITIONINDEPENDENT
get_thunk_edx: ; load caller address into edx for position-independent code
        mov edx, [esp]
        ret
%ENDIF


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  Version for old processors without SSE2
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align 8
; 80386 version used when SSE2 not supported:
_memmove386:
memmove386@:
        PROLOGM _memcpy386
; edi = dest
; esi = src
; ecx = count
        std                            ; Move backwards
        lea     edi, [edi+ecx-1]       ; Point to last byte of dest
        lea     esi, [esi+ecx-1]       ; Point to last byte of src
        cmp     ecx, 8
        jb      G500
G100:   test    edi, 3                 ; Test if unaligned
        jz      G200
        movsb
        dec     ecx
        jmp     G100                   ; Repeat while edi unaligned
        
G200:   ; edi is aligned now. Move 4 bytes at a time
        sub     edi, 3                 ; Point to last dword of dest
        sub     esi, 3                 ; Point to last dword of src
        mov     edx, ecx
        shr     ecx, 2
        rep     movsd                  ; move 4 bytes at a time
        mov     ecx, edx
        and     ecx, 3
        add     edi, 3                 ; Point to last byte of dest
        add     esi, 3                 ; Point to last byte of src        
        rep     movsb                  ; move remaining 0-3 bytes
        cld
        EPILOGM
        
G500:   ; count < 8. Move one byte at a time
        rep     movsb                  ; move count bytes
        cld
        EPILOGM


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;    CPU dispatcher
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; CPU dispatching for memmove. This is executed only once
memmoveCPUDispatch:
%IFNDEF POSITIONINDEPENDENT
        pushad
        ; set _CacheBypassLimit to half the size of the largest level cache
        push    0
        call    SetMemcpyCacheLimit@
        pop     ecx
        call    _InstructionSet
        mov     ebx, eax
        mov     esi, memmove386@       ; Point to generic version of memmove
        cmp     eax, 4                 ; check SSE2
        jb      Q100
        ; SSE2 supported
        mov     esi, memmoveSSE2@      ; Point to SSE2 version of memmove
        cmp     eax, 6                 ; check Suppl-SSE3
        jb      Q100
        ; Suppl-SSE3 supported
        mov     esi, memmoveSSSE3@     ; Point to SSSE3 version of memmove
        call    _UnalignedIsFaster
        test    eax, eax
        jz      Q100
        mov     esi, memmoveU@         ; Point to unaligned version of memmove
        call    _Store256BitIsFaster
        test    eax, eax
        jz      Q100        
        mov     esi, memmoveU256@      ; Point to 256 bit move version of memmove
        cmp     ebx, 15
        jb      Q100
        mov     esi, memmoveAVX512F@
        cmp     ebx, 16
        jb      Q100
        mov     esi, memmoveAVX512BW@
        
Q100:   mov     [memmoveDispatch], esi
        popad
        ; Continue in appropriate version of memmove
        jmp     [memmoveDispatch]

%ELSE   ; Position-independent version
        pushad
        mov     ebx, edx               ; reference point
        ; set _CacheBypassLimit to half the size of the largest level cache
        push    0
        call    SetMemcpyCacheLimit@
        pop     ecx
        call    _InstructionSet
        ; Point to generic version of memmove
        lea     esi, [ebx+memmove386@-RP]
        cmp     eax, 4                 ; check SSE2
        jb      Q100
        ; SSE2 supported
        ; Point to SSE2 version of memmove
        lea     esi, [ebx+memmoveSSE2@-RP]
        cmp     eax, 6                 ; check Suppl-SSE3
        jb      Q100
        ; Suppl-SSE3 supported
        ; Point to SSSE3 version of memmove
        lea     esi, [ebx+memmoveSSSE3@-RP]        
        call    _UnalignedIsFaster
        test    eax, eax
        jz      Q100
        ; Point to unaligned version of memmove
        lea     esi, [ebx+memmoveU@-RP]        
        call    _Store256BitIsFaster
        test    eax, eax
        jz      Q100
        ; Point to 256 bit move version of memmove
        lea     esi, [ebx+memmoveU256@-RP]        

        ; No position-independent versions of memcpyAVX512F, memcpyAVX512BW
        
Q100:   ; insert appropriate pointer
        mov     dword [ebx+memmoveDispatch-RP], esi
        popad
        ; Continue in appropriate version of memmove
        jmp     [edx+memmoveDispatch-RP]        
%ENDIF


; Note: Must call _SetMemcpyCacheLimit1 defined in memcpy32.asm
_SetMemcpyCacheLimit:
SetMemcpyCacheLimit@:  ; local label
        mov     eax, [esp+4]
        push    eax
        call    _SetMemcpyCacheLimit1
        pop     ecx
%ifdef  POSITIONINDEPENDENT
        call    get_thunk_edx
        mov     [edx + _CacheBypassLimit - $], eax
%else
        mov     [_CacheBypassLimit], eax
%endif
        ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;    data section. jump tables, dispatch function pointer, cache size
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Data segment must be included in function namespace
SECTION .data
align 16

; Jump tables for alignments 0 - 15:
; The CPU dispatcher replaces MAlignmentDispatchSSE2 with 
; MAlignmentDispatchSupSSE3 if Suppl-SSE3 is supported
; RP = reference point if position-independent code, otherwise RP = 0

MAlignmentDispatchSSE2:
DD C100-RP, D101-RP, D102-RP, D103-RP, D104-RP, D105-RP, D106-RP, D107-RP
DD D108-RP, D109-RP, D10A-RP, D10B-RP, D10C-RP, D10D-RP, D10E-RP, D10F-RP

MAlignmentDispatchSSSE3:
DD C100-RP, E101-RP, E102-RP, E103-RP, E104-RP, E105-RP, E106-RP, E107-RP
DD E108-RP, E109-RP, E10A-RP, E10B-RP, E10C-RP, E10D-RP, E10E-RP, E10F-RP

MAlignmentDispatchNT:
DD F100-RP, F101-RP, F102-RP, F103-RP, F104-RP, F105-RP, F106-RP, F107-RP
DD F108-RP, F109-RP, F10A-RP, F10B-RP, F10C-RP, F10D-RP, F10E-RP, F10F-RP

; Pointer to appropriate version.
; This initially points to memcpyCPUDispatch. memcpyCPUDispatch will
; change this to the appropriate version of memcpy, so that
; memcpyCPUDispatch is only executed once:
memmoveDispatch: DD memmoveCPUDispatch

; Bypass cache by using non-temporal moves if count > _CacheBypassLimit
; The optimal value of CacheBypassLimit is difficult to estimate, but
; a reasonable value is half the size of the largest cache:
_CacheBypassLimit: DD 0

%IFDEF POSITIONINDEPENDENT
; Fix potential problem in Mac linker
        DD      0, 0
%ENDIF
