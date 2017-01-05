;  bits.ash -- assembler stuff
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


; bit-buffer size
%ifndef UCL_BB
  %error UCL_BB
%endif


; /***********************************************************************
; // bit-buffer essentials
; ************************************************************************/

; ------------- ADDBITS -------------

%macro ADDBITS 0
  %if (UCL_BB == 8)
                add     bl, bl
  %elif (UCL_BB == 16)
                add     bx, bx
  %elif (UCL_BB == 32)
                add     ebx, ebx
  %endif
%endmacro


%macro ADDXBITS 0
  %if (UCL_BB == 8)
                adc     bl, bl
  %elif (UCL_BB == 16)
                adc     bx, bx
  %elif (UCL_BB == 32)
                adc     ebx, ebx
  %endif
%endmacro


; ------------- FILLBYTES_xx -------------

; get 8 bits; then get 1 bit into C
; Note: we shift the C flag through -> must have carry set on entry !
%macro FILLBYTES_8 0
  %ifdef UCL_SAFE
                ; error if esi >= IN_END
                cmp     esi, IN_END
                jae     L_input_overrun
                ; as a nice side effect of the `cmp' the carry flag
                ; is now still set
  %endif
                mov     bl, [esi]
                inc     esi
                adc     bl, bl
%endmacro


; get 16 bits in little endian format; then get 1 bit into C
%macro FILLBYTES_LE16 0
  %ifdef UCL_SAFE
                ; error if esi > IN_END2
                cmp     esi, IN_END2
                ja      L_input_overrun
  %endif
                mov     bx,  [esi]
                sub     esi, byte -2            ; sets carry flag !
                adc     bx, bx
%endmacro


; get 32 bits in little endian format; then get 1 bit into C
%macro FILLBYTES_LE32 0
  %ifdef UCL_SAFE
                ; error if esi > IN_END4
                cmp     esi, IN_END4
                ja      L_input_overrun
  %endif
                mov     ebx, [esi]
                sub     esi, byte -4            ; sets carry flag !
                adc     ebx, ebx
%endmacro


; ------------- FILLBITS -------------

%macro FILLBYTES 0
  %if (UCL_BB == 8)
                FILLBYTES_8
  %elif (UCL_BB == 16)
                FILLBYTES_LE16
  %elif (UCL_BB == 32)
                FILLBYTES_LE32
  %endif
%endmacro


%macro FILLBITS 0
                FILLBYTES
%endmacro


; ------------- GETBIT -------------

; get one bit into the Carry flag
%macro GETBIT 0
                ADDBITS
                jnz     %%gotbit
                FILLBITS
%%gotbit:
%endmacro


; vi:ts=8:et

