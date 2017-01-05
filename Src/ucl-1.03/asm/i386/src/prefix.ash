;  prefix.ash -- prefix decoding assembler stuff
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


; notes:
;   arg1 must be a register
;   arg2 must be a macro that accepts one argument


; /***********************************************************************
; // note: must init %1 with 1
; ************************************************************************/

%macro Decode_SS11 2
%%loop:         GETBIT
                adc     %1, %1
                %2      %1
  %ifdef UCL_SMALL
                GETBIT
                jnc     %%loop
  %else
                ADDBITS
                jnc     %%loop
                jnz     %%break
                FILLBITS
                jnc     %%loop
%%break:
  %endif
%endmacro


; /***********************************************************************
; // note: must init %1 with 1
; ************************************************************************/

%macro Decode_SS12 2
%%loop:         GETBIT
                adc     %1, %1
                %2      %1
  %ifdef UCL_SMALL
                GETBIT
                jc      %%break
  %else
                ADDBITS
                jnc     %%nobreak
                jnz     %%break
                FILLBITS
                jc      %%break
%%nobreak:
  %endif
                dec     %1
                GETBIT
                adc     %1, %1
                %2      %1
                jmp     %%loop
%%break:
%endmacro


; /***********************************************************************
; // note: must init %1 with 0
; ************************************************************************/

%macro Decode_P3_SS11 2
                GETBIT
                adc     %1, %1
                GETBIT
                adc     %1, %1
                jnz     %%done
                inc     %1
                Decode_SS11 %1, %2
                add     %1, byte 2
%%done:
%endmacro


; vi:ts=8:et

