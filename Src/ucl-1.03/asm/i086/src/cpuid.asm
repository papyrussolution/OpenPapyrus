;  cpuid.asm -- CPUID
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
; // int __far ucl_cpuid_asm(ucl_uint32 __far info[16])
; ************************************************************************/

UCL_PUBLIC ucl_cpuid_asm
  CPU 8086
        push    bp
        mov     bp, sp
        and     sp, byte -4             ; align sp for "pushfd"
        push    cx
        push    es
        push    di
        cld

        ; es:di = info
        les     di, [bp+6]

        mov     cx, 0xf000

        ; check for 286 or better (bits 12..15 are always set on an 8086)
        pushf
        pushf
        pop     ax
        and     ax, 0x0fff
        push    ax
        popf
        pushf
        pop     ax
        popf
        and     ax, cx
        cmp     ax, cx
        jz      no_386

        ; check for 386 or better (bits 12..15 are always clear on an 286)
        pushf
        pushf
        pop     ax
        or      ax, cx
        push    ax
        popf
        pushf
        pop     ax
        popf
        and     ax, cx
        jz      no_386

  CPU 386
        push    eax
        push    ebx
        push    ecx
        push    edx

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
        xor     eax, eax
        call    do_cpuid

        ; standard level 0000_0001h
        ;   get processor type/family/model/stepping and feature flags
        xor     eax, eax
        inc     ax
        call    do_cpuid

        ; extended level 8000_0000h
        ;   get maximum supported extended level and vendor ID string
        mov     eax, 0x80000000
        call    do_cpuid

        ; extended level 8000_0001h
        ;   get processor family/model/stepping and features flags
        mov     eax, 0x80000001
        call    do_cpuid


        pop     edx
        pop     ecx
        pop     ebx
        pop     eax

        xor     ax, ax          ; 0: success

return:
  CPU 8086
        pop     di
        pop     es
        pop     cx
        mov     sp, bp
        pop     bp
        retf


no_cpuid:
  CPU 386
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
no_386:
  CPU 8086
        xor     ax, ax
        mov     cx, 32
        rep stosw
        dec     ax              ; -1: failure
        jmp     return


toggle_eflags:
  CPU 386
        pushfd
        pushfd
        pop     eax
        mov     ecx, eax
        xor     eax, edx
        push    eax
        popfd
        pushfd
        pop     eax
        popfd
        xor     eax, ecx
        and     eax, edx
        ret


do_cpuid:
  CPU 386
        xor     ebx, ebx
        xor     ecx, ecx
        xor     edx, edx
  CPU 586
        cpuid
  CPU 386
        stosd
        mov     [es:di], ebx
        mov     [es:di+4], ecx
        mov     [es:di+8], edx
        add     di, 12
        ret


UCL_PUBLIC_END ucl_cpuid_asm


; vi:ts=8:et

