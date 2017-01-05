;  cpuid.asm -- i386 CPUID
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
; // int ucl_cpuid_asm(ucl_uint32 info[16])
; ************************************************************************/

UCL_PUBLIC ucl_cpuid_asm

        push_cregs
        ; save and align esp
        mov     ebp, esp
        and     esp, byte -4

        cld
        mov     edi, [ebp + cregs_size + 4]     ; edi = info

        ; check for 486 or better
        mov     edx, 0x00040000         ; 1 << 18
        call    toggle_eflags
        jz      no_cpuid

        ; check for cpuid support
;;        mov     edx, 0x00200000         ; 1 << 21
        shl     edx, 3                  ; 1 << 21
        call    toggle_eflags
        jz      no_cpuid


        ; standard level 0000_0000h
        ;   get maximum supported standard level and vendor ID string
        SET_REG eax, 0
        call    do_cpuid

        ; standard level 0000_0001h
        ;   get processor type/family/model/stepping and feature flags
        SET_REG eax, 1
        call    do_cpuid

        ; extended level 8000_0000h
        ;   get maximum supported extended level and vendor ID string
        SET_REG eax, 0x80000000
        call    do_cpuid

        ; extended level 8000_0001h
        ;   get processor family/model/stepping and features flags
        SET_REG eax, 0x80000001
        call    do_cpuid

        xor     eax, eax        ; 0: success


return:
        mov     esp, ebp
        pop_cregs
        ret


no_cpuid:
;;        xor     eax, eax
        lea     ecx, [eax + 16] ; SET_REG ecx, 16
        rep stosd
        dec     eax             ; -1: failure
        jmp     return


toggle_eflags:
        pushf
        pushf
        pop     eax
        mov     ecx, eax
        xor     eax, edx
        push    eax
        popf
        pushf
        pop     eax
        popf
        xor     eax, ecx
        and     eax, edx
        ret


do_cpuid:
        xor     ebx, ebx
        xor     ecx, ecx
        xor     edx, edx
  CPU 586
        cpuid
  CPU 386
%if 0
        mov     [edi], eax
        mov     [edi+4], ebx
        mov     [edi+8], ecx
        mov     [edi+12], edx
        add     edi, 16
%else
        stosd
        mov     [edi], ebx
        mov     [edi+4], ecx
        mov     [edi+8], edx
        add     edi, 12
%endif
        ret



UCL_PUBLIC_END ucl_cpuid_asm


; vi:ts=8:et

