;  leave.ash -- function leave assembler stuff
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


; /***********************************************************************
; //
; ************************************************************************/

%ifnidn FP,esp
  %include "error!"
%endif

%ifndef UCL_SAFE
%ifndef UCL_DEBUG
                mov     ebx, esp
  %undef FP
  %define FP ebx
%endif
%endif


;
; NOTE: on entry eax == 0
;


%ifdef UCL_DEBUG
                or      eax, eax
                jne     L_assert_fail
%endif


;
; check decompressed size
;

%ifdef UCL_SAFE
                cmp     edi, OUT_END
                ja      L_output_overrun
%endif


;
;  check compressed size
;

%ifdef UCL_SAFE
                cmp     esi, IN_END
%else
                mov     edx, IN
                add     edx, IN_LEN
                cmp     esi, edx
%endif

%ifdef UCL_SAFE
                jna     no_input_overrun
L_input_overrun:
                SET_REG eax, -201       ; UCL_E_INPUT_OVERRUN
                jmp     L_leave
no_input_overrun:
                je      .1
                ;;SET_REG eax, -205       ; UCL_E_INPUT_NOT_CONSUMED
                dec     eax             ; eax == -1
                mov     al, 0x33        ; -205
.1:
%else
                je      .1
                dec     eax             ; eax == -1
                mov     al, 0x37        ; -201
                ja      .1
                mov     al, 0x33        ; -205
.1:
%endif


;
;
;

%ifidn FP,esp   ; assert that we only can jump to L_leave if FP == esp
L_leave:
%endif
                sub     edi, OUT        ; write back the uncompressed size
                mov     edx, OUT_LEN
                mov     [edx], edi

%if locals_size > 0
                add     esp, byte locals_size
%endif
                pop_cregs
                ret



%ifdef UCL_SAFE
L_output_overrun:
                SET_REG eax, -202       ; UCL_E_OUTPUT_OVERRUN
                jmp     L_leave

L_lookbehind_overrun:
                SET_REG eax, -203       ; UCL_E_LOOKBEHIND_OVERRUN
                jmp     L_leave

L_error:
                SET_REG eax, -1         ; UCL_E_ERROR
                jmp     L_leave
%endif

%ifdef UCL_DEBUG
L_assert_fail:
                SET_REG eax, -99
                jmp     L_leave
%endif


; vi:ts=8:et

