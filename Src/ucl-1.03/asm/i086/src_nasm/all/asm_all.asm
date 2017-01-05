;  asm_all.asm --
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

; /***** DO NOT EDIT - GENERATED AUTOMATICALLY *****/

%define F(name) name
%define globalf(x) global x
bits 16
%ifidni __OUTPUT_FORMAT__,obj
segment _TEXT class=CODE public use16 align=1
%else
section .text
%endif

globalf(_ucl_cpuid_asm)
globalf(F(ucl_cpuid_asm))
_ucl_cpuid_asm:
F(ucl_cpuid_asm):
db 85,137,229,131,228,252,81,6,87,252,196,126,6,185,0,240
db 156,156,88,37,255,15,80,157,156,88,157,33,200,57,200,116
db 98,156,156,88,9,200,80,157,156,88,157,33,200,116,84,102
db 80,102,83,102,81,102,82,102,186,0,0,4,0,232,77,0
db 116,57,102,193,226,3,232,68,0,116,48,102,49,192,232,89
db 0,102,49,192,64,232,82,0,102,184,0,0,0,128,232,73
db 0,102,184,1,0,0,128,232,64,0,102,90,102,89,102,91
db 102,88,49,192,95,7,89,137,236,93,203,102,90,102,89,102
db 91,102,88,49,192,185,32,0,243,171,72,235,231,102,156,102
db 156,102,88,102,137,193,102,49,208,102,80,102,157,102,156,102
db 88,102,157,102,49,200,102,33,208,195,102,49,219,102,49,201
db 102,49,210,15,162,102,171,38,102,137,29,38,102,137,77,4
db 38,102,137,85,8,131,199,12,195

globalf(_ucl_rdtsc_asm)
globalf(F(ucl_rdtsc_asm))
_ucl_rdtsc_asm:
F(ucl_rdtsc_asm):
db 85,137,229,102,80,102,82,6,87,196,126,6,252,15,49,102
db 171,38,102,137,21,137,70,252,95,7,102,90,102,88,201,203

globalf(_ucl_rdtsc_add_asm)
globalf(F(ucl_rdtsc_add_asm))
_ucl_rdtsc_add_asm:
F(ucl_rdtsc_add_asm):
db 85,137,229,102,80,102,82,30,87,197,126,6,248,15,49,102
db 1,5,102,131,210,0,102,1,85,4,137,70,252,95,31,102
db 90,102,88,201,203

