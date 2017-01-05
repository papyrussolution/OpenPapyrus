;  util.ash -- misc utils
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
; //
; ************************************************************************/

%macro SET_REG_SMALL 2
  %if %2 == 0
                xor     %1, %1
  %elif %2 == -1
                or      %1, byte -1
  %elif %2 == 1
                xor     %1, %1
                inc     %1
  %elif %2 == 2
                xor     %1, %1
                inc     %1
                inc     %1
  %elif %2 == -2
                or      %1, byte -1
                dec     %1
  %else
                mov     %1, %2
  %endif
%endmacro


%macro SET_REG 2
  %if %2 == 0
                xor     %1, %1
  %elif %2 == -1
                or      %1, byte -1
  %elifdef UCL_SMALL
                SET_REG_SMALL %1,%2
  %else
                mov     %1, %2
  %endif
%endmacro


; /***********************************************************************
; //
; ************************************************************************/

%macro SAFE_LODSB 0
  %ifdef UCL_SAFE
                ; error if esi >= IN_END
                cmp     esi, IN_END
                jae     L_input_overrun
  %endif
  %ifdef UCL_SMALL
                lodsb
  %else
                mov     al, [esi]
                inc     esi
  %endif
%endmacro


%macro SAFE_MOVSB 0
  %ifdef UCL_SAFE
                ; error if esi >= IN_END
                cmp     esi, IN_END
                jae     L_input_overrun
                ; error if edi >= OUT_END
                cmp     edi, OUT_END
                jae     L_output_overrun
  %endif
  %ifdef UCL_SMALL
                movsb
  %else
                mov     al, [esi]
                inc     esi
                mov     [edi], al
                inc     edi
  %endif
%endmacro


; vi:ts=8:et

