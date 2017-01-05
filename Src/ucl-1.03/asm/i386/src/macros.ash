;  macros.ash -- generic NASM macros
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
; // misc
; ************************************************************************/

%macro dummy_macro 0+
%endmacro



; /***********************************************************************
; // sections
; ************************************************************************/

%ifndef __OUTPUT_FORMAT__
  %error "OUTPUT_FORMAT missing"
%endif


%macro .text 0
  %ifidni __OUTPUT_FORMAT__,elf
                ;SECTION .text align=16
                SECTION .text
  %elifidni __OUTPUT_FORMAT__,obj
                SEGMENT _TEXT class=CODE public use32 flat align=16
  %elifidni __OUTPUT_FORMAT__,win32
                ;SECTION .text align=16
                SECTION .text
  %else
                SECTION .text
  %endif
%endmacro

%macro .data 0
  %ifidni __OUTPUT_FORMAT__,obj
                SEGMENT _DATA class=DATA public use32 flat
  %else
                SECTION .data
  %endif
%endmacro

%macro .bss 0
  %ifidni __OUTPUT_FORMAT__,obj
                SEGMENT _BSS class=BSS public use32 flat
  %else
                SECTION .bss
  %endif
%endmacro


; declare a C global function

%macro CGLOBALF 1
  %ifidni __OUTPUT_FORMAT__,elf
                GLOBAL %1:function
  %elifidni __OUTPUT_FORMAT__,win32
                GLOBAL _%1
    %define %1 _%1
  %else
                GLOBAL _%1
    %define %1 _%1
  %endif
%endmacro


; /***********************************************************************
; // align_code
; ************************************************************************/

%macro nop_code_ia16 1
  %push nop_code_ia16
  %assign %$n (%1)
  %if %$n == 0
  %elif %$n == 1
        db      0x90                            ; nop
  %elif %$n == 2
        db      0x89,0xf6                       ; mov si,si
  %elif %$n == 3
        db      0x8d,0x74,0                     ; lea si,[si+0x0]
  %elif %$n == 4
        db      0x8d,0xb4,0,0                   ; lea si,[si+0x0]
  %elif %$n == 5
        db      0x90                            ; nop
        db      0x8d,0xb4,0,0                   ; lea si,[si+0x0]
  %elif %$n == 6
        db      0x89,0xf6                       ; mov si,si
        db      0x8d,0xbd,0,0                   ; lea di,[di+0x0]
  %elif %$n == 7
        db      0x8d,0x74,0                     ; lea si,[si+0x0]
        db      0x8d,0xbd,0,0                   ; lea di,[di+0x0]
  %elif %$n == 8
        db      0x8d,0xb4,0,0                   ; lea si,[si+0x0]
        db      0x8d,0xbd,0,0                   ; lea di,[di+0x0]
  %elif %$n >= 9 && %$n < 128
        db      0xeb,%$n-2                      ; jmp short
        times %$n-2 db 0x90                     ; nop
  %else
        %error "invalid nop_code_ia16"
        %include "invalid_nop_code_ia16"
  %endif
  %pop
%endmacro


%macro nop_code_ia32 1
  %push nop_code_ia32
  %assign %$n (%1)
  %if %$n == 0
  %elif %$n == 1
        db      0x90                            ; nop
  %elif %$n == 2
        db      0x89,0xf6                       ; mov esi,esi
  %elif %$n == 3
        db      0x8d,0x76,0                     ; lea esi,[esi+0x0]
  %elif %$n == 4
        db      0x8d,0x74,0x26,0                ; lea esi,[esi+0x0]
  %elif %$n == 5
        db      0x90                            ; nop
        db      0x8d,0x74,0x26,0                ; lea esi,[esi+0x0]
  %elif %$n == 6
        db      0x8d,0xb6,0,0,0,0               ; lea esi,[esi+0x0]
  %elif %$n == 7
        db      0x8d,0xb4,0x26,0,0,0,0          ; lea esi,[esi+0x0]
  %elif %$n == 8
        db      0x90                            ; nop
        db      0x8d,0xb4,0x26,0,0,0,0          ; lea esi,[esi+0x0]
  %elif %$n == 9
        db      0x89,0xf6                       ; mov esi,esi
        db      0x8d,0xbc,0x27,0,0,0,0          ; lea edi,[edi+0x0]
  %elif %$n == 10
        db      0x8d,0x76,0                     ; lea esi,[esi+0x0]
        db      0x8d,0xbc,0x27,0,0,0,0          ; lea edi,[edi+0x0]
  %elif %$n == 11
        db      0x8d,0x74,0x26,0                ; lea esi,[esi+0x0]
        db      0x8d,0xbc,0x27,0,0,0,0          ; lea edi,[edi+0x0]
  %elif %$n == 12
        db      0x8d,0xb6,0,0,0,0               ; lea esi,[esi+0x0]
        db      0x8d,0xbf,0,0,0,0               ; lea edi,[edi+0x0]
  %elif %$n == 13
        db      0x8d,0xb6,0,0,0,0               ; lea esi,[esi+0x0]
        db      0x8d,0xbc,0x27,0,0,0,0          ; lea edi,[edi+0x0]
  %elif %$n == 14
        db      0x8d,0xb4,0x26,0,0,0,0          ; lea esi,[esi+0x0]
        db      0x8d,0xbc,0x27,0,0,0,0          ; lea edi,[edi+0x0]
  %elif %$n >= 15 && %$n < 128
        db      0xeb,%$n-2                      ; jmp short
        times %$n-2 db 0x90                     ; nop
  %else
        %error "invalid nop_code_ia32"
        %include "invalid_nop_code_ia32"
  %endif
  %pop
%endmacro


%macro nop_code_amd64 1
  %push nop_code_amd64
  %assign %$n (%1)
  %if %$n == 0
  %elif %$n == 1
        db      0x90                            ; nop
  %elif %$n == 2
        db      0x66,0x90                       ; nop
  %elif %$n == 3
        db      0x66,0x66,0x90                  ; nop
  %elif %$n == 4
        db      0x66,0x66,0x66,0x90             ; nop
  %elif %$n == 5
        db      0x66,0x66,0x90                  ; nop
        db      0x66,0x90                       ; nop
  %elif %$n == 6
        db      0x66,0x66,0x90                  ; nop
        db      0x66,0x66,0x90                  ; nop
  %elif %$n == 7
        db      0x66,0x66,0x66,0x90             ; nop
        db      0x66,0x66,0x90                  ; nop
  %elif %$n == 8
        db      0x66,0x66,0x66,0x90             ; nop
        db      0x66,0x66,0x66,0x90             ; nop
  %elif %$n == 9
        db      0x66,0x66,0x90                  ; nop
        db      0x66,0x66,0x90                  ; nop
        db      0x66,0x66,0x90                  ; nop
  %elif %$n == 10
        db      0x66,0x66,0x66,0x90             ; nop
        db      0x66,0x66,0x90                  ; nop
        db      0x66,0x66,0x90                  ; nop
  %elif %$n == 11
        db      0x66,0x66,0x66,0x90             ; nop
        db      0x66,0x66,0x66,0x90             ; nop
        db      0x66,0x66,0x90                  ; nop
  %elif %$n == 12
        db      0x66,0x66,0x66,0x90             ; nop
        db      0x66,0x66,0x66,0x90             ; nop
        db      0x66,0x66,0x66,0x90             ; nop
  %elif %$n == 13
        db      0x66,0x66,0x66,0x90             ; nop
        db      0x66,0x66,0x90                  ; nop
        db      0x66,0x66,0x90                  ; nop
        db      0x66,0x66,0x90                  ; nop
  %elif %$n == 14
        db      0x66,0x66,0x66,0x90             ; nop
        db      0x66,0x66,0x66,0x90             ; nop
        db      0x66,0x66,0x90                  ; nop
        db      0x66,0x66,0x90                  ; nop
  %elif %$n == 15
        db      0x66,0x66,0x66,0x90             ; nop
        db      0x66,0x66,0x66,0x90             ; nop
        db      0x66,0x66,0x66,0x90             ; nop
        db      0x66,0x66,0x90                  ; nop
  %elif %$n >= 16 && %$n < 128
        db      0xeb,%$n-2                      ; jmp short
        times %$n-2 db 0x90                     ; nop
  %else
        %error "invalid nop_code_amd64"
        %include "invalid_nop_code_amd64"
  %endif
  %pop
%endmacro


%macro align_code 1.nolist
        nop_code_ia32   ($$-$) & ((%1)-1)
%endmacro


; /***********************************************************************
; //
; ************************************************************************/

; push/pop registers as required by the C compiler
;
;   FIXME: must check the i386 ABI - probably we don't have
;          to preserve ecx and edx

%define cregs_size      24      ; 6 registers a 4 bytes

%macro push_cregs 0
                push    ebp
                push    edi
                push    esi
                push    ebx
                push    ecx
                push    edx
%endmacro

%macro pop_cregs 0
                pop     edx
                pop     ecx
                pop     ebx
                pop     esi
                pop     edi
                pop     ebp
%endmacro



; vi:ts=8:et

