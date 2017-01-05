;  enter.ash -- function entry code assembler stuff
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

%ifndef UCL_BB
  %include "error!"
%endif

%define FP esp

%ifdef UCL_SAFE
  %if (UCL_BB == 8)
    %define locals_size     8
    %define OUT_END         dword [FP]
    %define IN_END          dword [FP + 4]
  %elif (UCL_BB == 16)
    %define locals_size     12
    %define OUT_END         dword [FP]
    %define IN_END          dword [FP + 4]
    %define IN_END2         dword [FP + 8]
  %elif (UCL_BB == 32)
    %define locals_size     12
    %define OUT_END         dword [FP]
    %define IN_END          dword [FP + 4]
    %define IN_END4         dword [FP + 8]
  %else
    %include "error!"
  %endif
%else
  %define locals_size       0
%endif


%define IN          dword [FP + locals_size + cregs_size + 4]
%define IN_LEN      dword [FP + locals_size + cregs_size + 8]
%define OUT         dword [FP + locals_size + cregs_size + 12]
%define OUT_LEN     dword [FP + locals_size + cregs_size + 16]



; /***********************************************************************
; //
; ************************************************************************/

                push_cregs
%if locals_size > 0
                sub     esp, byte locals_size
%endif
%ifdef UCL_SAFE
                mov     ebx, esp
  %undef FP
  %define FP ebx
%endif

                cld
                mov     esi, IN
                mov     edi, OUT

%ifdef UCL_SAFE
                mov     eax, edi
                mov     edx, OUT_LEN
                add     eax, [edx]
                jc      L_error
                mov     OUT_END, eax

                mov     eax, esi
                add     eax, IN_LEN
                jc      L_error
                mov     IN_END, eax

  %if (UCL_BB == 16)
                sub     eax, byte 2
                jc      L_input_overrun
                cmp     eax, esi
                jb      L_input_overrun
                mov     IN_END2, eax
  %elif (UCL_BB == 32)
                sub     eax, byte 4
                jc      L_input_overrun
                cmp     eax, esi
                jb      L_input_overrun
                mov     IN_END4, eax
  %endif
%endif ; ifdef UCL_SAFE


%undef FP
%define FP esp


; vi:ts=8:et

