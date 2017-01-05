;  rdtsca.asm -- RDTSC
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
; // unsigned __far ucl_rdtsc_add_asm(ucl_uint32 __far ticks[2])
; ************************************************************************/

UCL_PUBLIC ucl_rdtsc_add_asm
  CPU 386
        push    bp
        mov     bp, sp
        push    eax
        push    edx
        push    ds
        push    di

        ; ds:di = ticks
        lds     di, [bp+6]

        clc                     ; serializing instruction
  CPU 586
        rdtsc
  CPU 386
        add     [di], eax       ; ticks[0] = low 32 bits
        adc     edx, byte 0
        add     [di+4], edx     ; ticks[1] = high 32 bits

        ; store low 16 bits as return code - see "pop eax"
        mov     [bp-4], ax

        pop     di
        pop     ds
        pop     edx
        pop     eax
        leave
        retf                    ; return low bits


UCL_PUBLIC_END ucl_rdtsc_add_asm


; vi:ts=8:et

