;  n2_d.ash -- UCL assembler decompressors
;
;  This file is part of the UCL data compression library.
;
;  Copyright (C) 2004 Markus Franz Xaver Johannes Oberhumer
;  Copyright (C) 2003 Markus Franz Xaver Johannes Oberhumer
;  Copyright (C) 2002 Markus Franz Xaver Johannes Oberhumer
;  Copyright (C) 2001 Markus Franz Xaver Johannes Oberhumer
;  Copyright (C) 2000 Markus Franz Xaver Johannes Oberhumer
;  Copyright (C) 1999 Markus Franz Xaver Johannes Oberhumer
;  Copyright (C) 1998 Markus Franz Xaver Johannes Oberhumer
;  Copyright (C) 1997 Markus Franz Xaver Johannes Oberhumer
;  Copyright (C) 1996 Markus Franz Xaver Johannes Oberhumer
;  All Rights Reserved.
;
;  The UCL library is free software; you can redistribute it and/or
;  modify it under the terms of the GNU General Public License as
;  published by the Free Software Foundation; either version 2 of
;  the License, or (at your option) any later version.
;
;  The UCL library is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with the UCL library; see the file COPYING.
;  If not, write to the Free Software Foundation, Inc.,
;  59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
;
;  Markus F.X.J. Oberhumer
;  <markus@oberhumer.com>
;  http://www.oberhumer.com/opensource/ucl/
;


; /***********************************************************************
; // util
; ************************************************************************/

; "inline" macros passed to the prefix-decode macros
%macro decode_check_off 1
  %ifdef UCL_SAFE
                js      L_lookbehind_overrun   ; error if >= 0x80000000
  %endif
%endmacro

%macro decode_check_len 1
  %ifdef UCL_SAFE
                js      L_output_overrun       ; error if >= 0x80000000
  %endif
%endmacro


; /***********************************************************************
; // copy match:
; //   copy ecx bytes from edi+%1 to edi
; //
; //   input:       edi, %1 (typically eax or ebp), ecx
; //   output:      edi := edi + ecx
; //   output:      ecx := 0
; //   preserves:   ebx, esi, ebp
; //   trashes:     eax, ecx, edx
; ************************************************************************/

; local macro for copy_match - set %1 to [edi+%2]
%macro copy_match.init 2
  %ifdef UCL_SAFE
                ; error if [edi+ecx] is > OUT_END
                mov     %1, edi
                add     %1, ecx
                jc      L_output_overrun
                cmp     %1, OUT_END
                ja      L_output_overrun
                ; error if [edi+%2] is < OUT
                mov     %1, edi
                add     %1, %2
                jnc     L_lookbehind_overrun
                cmp     %1, OUT
                jb      L_lookbehind_overrun
  %else
                lea     %1, [edi+%2]
  %endif
%endmacro


%macro copy_match 1
  %ifdef UCL_SMALL
    %ifdef UCL_SAFE
                mov     edx, esi                ; save esi
                copy_match.init esi, %1
    %else
                push    esi
                lea     esi, [edi+%1]           ; (inlined copy_match.init)
    %endif
                rep
                movsb
    %ifdef UCL_SAFE
                mov     esi, edx                ; restore esi
    %else
                pop     esi
    %endif
                jmp     decompr_loop

  %elifndef UCL_FAST
                copy_match.init edx, %1
%%copy1:        mov     al, [edx]
                inc     edx
                mov     [edi], al
                inc     edi
                dec     ecx
                jnz     %%copy1
                jmp     decompr_loop
  %else
                copy_match.init edx, %1
                cmp     %1, byte -4
                jbe     %%copy4
%%copy1:        mov     al, [edx]
                inc     edx
                mov     [edi], al
                inc     edi
                dec     ecx
                jnz     %%copy1
                jmp     decompr_loop
                align   4
%%copy4:        mov     eax, [edx]
                add     edx, byte 4
                mov     [edi], eax
                add     edi, byte 4
                sub     ecx, byte 4
                ja      %%copy4
                add     edi, ecx
                xor     ecx, ecx
                jmp     decompr_loop
  %endif
%endmacro


; /***********************************************************************
; //
; ************************************************************************/

                xor     ecx, ecx

;
; init bit buffer for FILLBITS/FILLBYTES
;

%if (UCL_BB == 8)
  %ifndef UCL_SAFE
                ; set carry flag - see FILLBYTES_8
                stc
  %endif
%elif (UCL_BB == 16)
                ; no need to init ebx or set the carry flag
%elif (UCL_BB == 32)
                ; no need to init ebx or set the carry flag
%endif
                jmp     decompr_start


;
; copy a literal
;


%ifndef UCL_SMALL
                align   16
%endif
decompr_literal:
                SAFE_MOVSB


;
; get flag bit - literal or match
;

decompr_loop:

%ifdef UCL_SMALL
                ADDBITS
                jnz     d_gotbit
decompr_start:  FILLBITS
d_gotbit:       jc      decompr_literal
%else
                ADDBITS
                jnc     decompr_match
                jnz     decompr_literal
decompr_start:  FILLBITS
                jc      decompr_literal
%endif

decompr_match:


; /***********************************************************************
; // NRV2A
; ************************************************************************/

%ifdef NRV2A

;
; decode offset (eax)
;

                SET_REG   eax, 1
                Decode_SS11 eax, decode_check_off
%ifdef UCL_SAFE
                cmp     eax, 0xffffff + 2
                ja      L_lookbehind_overrun
%endif
                sub     eax, byte 2
                shl     eax, 8
                SAFE_LODSB
                xor     eax, byte -1
                jz      decompr_end

;
; decode length (ecx)
;

                ;;xor     ecx, ecx
                Decode_P3_SS11 ecx, decode_check_len

                cmp     eax, -0xe00
                adc     ecx, byte 1

;
; copy match
;
                copy_match eax

%endif ; NRV2A


; /***********************************************************************
; // NRV2B
; ************************************************************************/

%ifdef NRV2B

;
; decode offset (eax -> ebp)
;

                SET_REG   eax, 1
                Decode_SS11 eax, decode_check_off
%ifdef UCL_SAFE
                cmp     eax, 0xffffff + 3
                ja      L_lookbehind_overrun
%endif
                sub     eax, byte 3
                jc      .decompr_same_off
                shl     eax, 8
                SAFE_LODSB
                xor     eax, byte -1
                jz      decompr_end
                mov     ebp, eax
.decompr_same_off:

;
; decode length (ecx)
;

                ;;xor     ecx, ecx
                Decode_P3_SS11 ecx, decode_check_len

                cmp     ebp, -0xd00
                adc     ecx, byte 1

;
; copy match
;
                copy_match ebp


%endif ; NRV2B


; /***********************************************************************
; // NRV2D
; ************************************************************************/

%ifdef NRV2D

;
; decode offset (eax -> ebp) and first bit of length (ecx)
;
                ;;xor     ecx, ecx

                SET_REG   eax, 1
                Decode_SS12 eax, decode_check_off
%ifdef UCL_SAFE
                cmp     eax, 0xffffff + 3
                ja      L_lookbehind_overrun
%endif
                sub     eax, byte 3
                jc      .decompr_same_off
                shl     eax, 8
                SAFE_LODSB
                xor     eax, byte -1
                jz      decompr_end
%ifdef UCL_SAFE
                ; must be negative
                jns     L_lookbehind_overrun
%endif
                sar     eax, 1                  ; shift low-bit into carry
                mov     ebp, eax
                jmp     .decompr_got_off

.decompr_same_off:
                GETBIT
.decompr_got_off:
                adc     ecx, ecx

;
; decode length (ecx)
;

                GETBIT
                adc     ecx, ecx
                jnz     .decompr_got_len
                inc     ecx
                Decode_SS11 ecx, decode_check_len
                add     ecx, byte 2
.decompr_got_len:

                cmp     ebp, -0x500
                adc     ecx, byte 1

;
; copy match
;
                copy_match ebp


%endif ; NRV2D


; /***********************************************************************
; // NRV2E
; ************************************************************************/

%ifdef NRV2E

;
; decode offset (eax -> ebp) and first bit of length into carry
;
                ;;xor     ecx, ecx

                SET_REG   eax, 1
                Decode_SS12 eax, decode_check_off
%ifdef UCL_SAFE
                cmp     eax, 0xffffff + 3
                ja      L_lookbehind_overrun
%endif
                sub     eax, byte 3
                jc      .decompr_same_off
                shl     eax, 8
                SAFE_LODSB
                xor     eax, byte -1
                jz      decompr_end
%ifdef UCL_SAFE
                ; must be negative
                jns     L_lookbehind_overrun
%endif
                sar     eax, 1                  ; shift low-bit into carry
                mov     ebp, eax
                jnc     .decompr_got_off

.decompr_mlen1:
                GETBIT
                adc     ecx, ecx
                jmp     .decompr_got_len

.decompr_same_off:
                GETBIT

;
; decode length (ecx)
;

                jc      .decompr_mlen1
.decompr_got_off:
                inc     ecx
                GETBIT
                jc      .decompr_mlen1
                Decode_SS11 ecx, decode_check_len
                add     ecx, byte 2
.decompr_got_len:
                cmp     ebp, -0x500
                adc     ecx, byte 2

;
; copy match
;
                copy_match ebp


%endif ; NRV2E


; /***********************************************************************
; // that's all, folks
; ************************************************************************/

decompr_end:


; vi:ts=8:et:nowrap


