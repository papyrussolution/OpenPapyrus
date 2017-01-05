;  n2b_d_n6.asm -- ucl_nrv2b_decompress_asm_safe_le16
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

%define UCL_BB 16
%define UCL_SAFE

%include "n2b_asm.ash"

    .text

    UCL_PUBLIC ucl_nrv2b_decompress_asm_safe_le16

%include "enter.ash"
%include "n2b_d.ash"
%include "leave.ash"

    UCL_PUBLIC_END ucl_nrv2b_decompress_asm_safe_le16


; vi:ts=8:et

