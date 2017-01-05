;  crcs_asm.asm -- ucl_crc32_asm_small checksum in assembly
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


; /***********************************************************************
; // ucl_crc32_asm_small(unsigned crc, const void *buf, unsigned len)
; ************************************************************************/

UCL_PUBLIC ucl_crc32_asm_small
        push    ecx

        xor     eax, eax
        mov     ecx, [esp + 4 + 8]     ; ecx = buf
        jecxz   return

        push    edi
        push    ebx

        ; edi = ucl_crc32_table_small
        call    init_tab
ucl_crc32_table_small:
%if (ucl_crc32_table_small - ucl_crc32_asm_small) != 16
  %include "alignment error"
%endif
        dd      0x00000000, 0x1db71060+4, 0x3b6e20c0+8, 0x26d930a0+12
        dd      0x76dc4190, 0x6b6b51f0+4, 0x4db26150+8, 0x50057130+12
        dd      0xedb88320, 0xf00f9340+4, 0xd6d6a3e0+8, 0xcb61b380+12
        dd      0x9b64c2b0, 0x86d3d2d0+4, 0xa00ae270+8, 0xbdbdf210+12
init_tab:
        pop     edi

        push    edx

        mov     edx, [esp + 16 + 12]   ; edx = len
        add     edx, ecx               ; edx = end ptr
        mov     eax, [esp + 16 + 4]    ; eax = crc
        not     eax
        jmp     .enter_loop

.loop:
%if (.loop - ucl_crc32_asm_small) != 96
  %include "alignment error"
%endif
        xor     al, [ecx]
        inc     ecx
  %rep 2
    %if 0
        mov     ebx, eax
        shr     eax, 4
        and     ebx, byte 15
    %else
        mov     ebx, 15
        and     ebx, eax
        shr     eax, 4
    %endif
        xor     eax, [edi + 4*ebx]
  %endrep
.enter_loop:
        cmp     ecx, edx
        jne     .loop

        not     eax

        pop     edx
        pop     ebx
        pop     edi
return:
        pop     ecx
        ret

UCL_PUBLIC_END ucl_crc32_asm_small


; vi:ts=8:et

