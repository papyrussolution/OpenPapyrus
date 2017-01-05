;  crc_asm.asm -- ucl_crc32_asm checksum in assembly
;
;  This file is part of the UCL data compression library.
;
;  Copyright (C) 1996-2004 Markus Franz Xaver Johannes Oberhumer
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


%include "conf.ash"

        .text

%define USE_MOVZX

%macro CRC_CORE 0
  %ifdef USE_MOVZX
        movzx   ebx, al
  %else
        mov     bl, al
  %endif
        shr     eax, 8
        xor     eax, [edi + 4*ebx]
%endmacro


%macro CRC 0
        xor     al, [esi]
        inc     esi
        CRC_CORE
%endmacro


; /***********************************************************************
; // ucl_crc32_asm(unsigned crc, const void *buf, unsigned len, const unsigned *tab)
; ************************************************************************/

UCL_PUBLIC ucl_crc32_asm

        push_cregs
        xor     eax, eax
%ifndef USE_MOVZX
        xor     ebx, ebx
%endif
        mov     esi, [esp + cregs_size + 8]     ; esi = buf
        mov     ecx, [esp + cregs_size + 12]    ; ecx = len
        mov     edi, [esp + cregs_size + 16]    ; edi = crc32_table
        test    esi, esi
        jz      return
        mov     eax, [esp + cregs_size + 4]     ; eax = crc
        not     eax

%if 1
        test    ecx, ecx
        jz      done
; the first 0..3 bytes (until esi is dword aligned)
align_loop:
        test     esi, 3
        jz      .break
        CRC
        dec     ecx
        jnz     align_loop
.break:
%endif


; the unrolled loop - 8 bytes per loop
        mov     edx, ecx
        shr     edx, 3
        jz      remainder

        align_code 16
unrolled_loop:
    %rep 2
        xor     eax, [esi]
        add     esi, byte 4
        CRC_CORE
        CRC_CORE
        CRC_CORE
        CRC_CORE
    %endrep
        dec     edx
        jnz     unrolled_loop


; the remaining 0..7 bytes
remainder:
        and     ecx, byte 7
        jz      done
small_loop:
        CRC
        dec     ecx
        jnz     small_loop

done:
        not     eax
return:
        pop_cregs
        ret

UCL_PUBLIC_END ucl_crc32_asm


; vi:ts=8:et

