;-------------------------------------------------------------------------------
; XEOS - X86 Experimental Operating System
; 
; Copyright (c) 2010-2013, Jean-David Gadina - www.xs-labs.com
; All rights reserved.
; 
; XEOS Software License - Version 1.0 - December 21, 2012
; 
; Permission is hereby granted, free of charge, to any person or organisation
; obtaining a copy of the software and accompanying documentation covered by
; this license (the "Software") to deal in the Software, with or without
; modification, without restriction, including without limitation the rights
; to use, execute, display, copy, reproduce, transmit, publish, distribute,
; modify, merge, prepare derivative works of the Software, and to permit
; third-parties to whom the Software is furnished to do so, all subject to the
; following conditions:
; 
;       1.  Redistributions of source code, in whole or in part, must retain the
;           above copyright notice and this entire statement, including the
;           above license grant, this restriction and the following disclaimer.
; 
;       2.  Redistributions in binary form must reproduce the above copyright
;           notice and this entire statement, including the above license grant,
;           this restriction and the following disclaimer in the documentation
;           and/or other materials provided with the distribution, unless the
;           Software is distributed by the copyright owner as a library.
;           A "library" means a collection of software functions and/or data
;           prepared so as to be conveniently linked with application programs
;           (which use some of those functions and data) to form executables.
; 
;       3.  The Software, or any substancial portion of the Software shall not
;           be combined, included, derived, or linked (statically or
;           dynamically) with software or libraries licensed under the terms
;           of any GNU software license, including, but not limited to, the GNU
;           General Public License (GNU/GPL) or the GNU Lesser General Public
;           License (GNU/LGPL).
; 
;       4.  All advertising materials mentioning features or use of this
;           software must display an acknowledgement stating that the product
;           includes software developed by the copyright owner.
; 
;       5.  Neither the name of the copyright owner nor the names of its
;           contributors may be used to endorse or promote products derived from
;           this software without specific prior written permission.
; 
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT OWNER AND CONTRIBUTORS "AS IS"
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
; THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
; PURPOSE, TITLE AND NON-INFRINGEMENT ARE DISCLAIMED.
; 
; IN NO EVENT SHALL THE COPYRIGHT OWNER, CONTRIBUTORS OR ANYONE DISTRIBUTING
; THE SOFTWARE BE LIABLE FOR ANY CLAIM, DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
; EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN ACTION OF CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
; NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF OR IN CONNECTION WITH
; THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE, EVEN IF ADVISED
; OF THE POSSIBILITY OF SUCH DAMAGE.
;-------------------------------------------------------------------------------

; $Id$

; We are in 64 bits mode
BITS    64

section .data

global __SSE2Status

; SSE2 status flag:
; 
;   -1: Unchecked
;    0: SSE2 not available
;    1: SSE2 available
__SSE2Status:   dq  -1

section .text

; Makes the entry point visible to the linker
global _xeos_memset

;-------------------------------------------------------------------------------
; C99 - 64 bits memset() function
; 
; void * memset( void * p, int c, size_t n );
; 
; Input registers:
;       
;       - RDI:      The memory pointer
;       - RSI:      The character to repeat
;       - RDX:      The number of characters to set
; 
; Return registers:
;       
;       - RAX:      The memory pointer
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_xeos_memset:
    
    ; Checks the status of the SSE2 flag
    cmp QWORD [ rel __SSE2Status ], 1
    
    ; SSE2 are available - Use the optimized version of memset()
    je  _memset64_sse2
    
    ; Checks the status of the SSE2 flag
    cmp QWORD [ rel __SSE2Status ], 0
    
    ; SSE2 are not available - Use the less-optimized version of memset()
    je  _memset64
    
    ; SSE2 status needs to be checked
    .check:
        
        ; Saves the values of RCX and RDX, as they will be altered by CPUID
        mov     r8,     rcx
        mov     r9,     rdx
        
        ; CPUID - Asks for CPU features (RAX=1)
        mov     rax,    1
        cpuid
        
        ; Checks the SSE2 bit (bit 26)
        test    edx,    0x4000000      
        jz      .fail
        
    ; SSE2 available
    .ok:
        
        ; Restores the values of RCX and RDX
        mov     rcx,    r8
        mov     rdx,    r9
        
        ; Sets the SSE2 status flag for the next calls and fills the buffer
        ; with the optimized version of memset()
        mov QWORD [ rel __SSE2Status ], 1
        jmp _memset64_sse2
    
    ; SSE2 not available
    .fail:
        
        ; Restores the values of RCX and RDX
        mov     rcx,    r8
        mov     rdx,    r9
        
        ; Sets the SSE2 status flag for the next calls and fills the buffer
        ; with the less-optimized version of memset()
        mov QWORD [ rel __SSE2Status ], 0
        jmp _memset64

;-------------------------------------------------------------------------------
; 64-bits SSE2 optimized memset() function
; 
; void * _memset64_sse2( void * p, int c, size_t n );
; 
; Input registers:
;       
;       - RDI:      The memory pointer
;       - RSI:      The character to repeat
;       - RDX:      The number of characters to set
; 
; Return registers:
;       
;       - RAX:      The memory pointer
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_memset64_sse2:
    
    ; Saves the original memory pointer, as we'll need to return it
    mov     r8,         rdi
    
    ; Checks for a NULL memory pointer
    test    rdi,        rdi
    jz      .end
    
    ; Checks for a zero size
    test    rdx,        rdx
    jz      .end
    
    ; memset() is often called to set a buffer to zero, so prepare xmm0
    ; for such a case
    pxor    xmm0,       xmm0
    
    ; Checks if bytes needs to be set to zero
    test    rsi,        rsi
    jz      .padded
    
    .pad:
        
        ; A character other than zero will be used. Fills the RSI registers
        ; with four time the character value, so it will be easier to fill
        ; a XMM register with the character to set
        mov     rcx,        rsi
        shl     rsi,        8
        or      rsi,        rcx
        mov     rcx,        rsi
        shl     rsi,        16
        or      rsi,        rcx
        
        ; Fills the XMM0 register with the character to write, so we'll be
        ; able to write 16 bytes at a time
        movd    xmm0,       rsi
        shufps  xmm0,       xmm0,   0x00
        
    .padded:
        
        ; Aligns the memory pointer in RDI to a 16-byte boundary,
        ; so we can safelfy use the SSE instructions
        and     rdi,        -16
        
        ; Gets the number of misaligned bytes in the original memory pointer (R8)
        mov     rax,        r8
        sub     rax,        rdi
        
        ; Checks if the pointer is already aligned
        test    rax,        rax
        jz      .aligned
        
        ; If not, computes the number of bytes to be written until we are
        ; aligned on the next 16-byte boundary
        mov     rcx,        16
        sub     rcx,        rax
        
    .notaligned:
        
        ; Restores the original pointer in RDI
        mov     rdi,        r8
        
        ; Stores the character to write in RAX, so we can access it as a byte
        ; using AL
        mov     rax,        rsi
        
    .notaligned_loop:
        
        ; Checks if we have bytes to write
        test    rdx,        rdx
        jz      .end
        
        ; Writes a byte into the memory buffer
        mov     [ rdi ],    al
        
        ; Advances the memory pointer
        inc     rdi
        
        ; Decreases the number of bytes to write (RDX is the total, RCX is the
        ; number of bytes to write until we're aligned to a 16-byte boundary)
        dec     rcx
        dec     rdx
        
        ; Checks if we are aligned to a 16-byte boundary. If so, the SSE
        ; instructions can be safely used
        test    rcx,        rcx
        jz      .aligned
        jmp     .notaligned_loop
        
    .aligned:
        
        ; Writes 128 bytes at a time, if possible
        cmp     rdx,        128
        jge     .aligned_128
        
        ; Writes 64 bytes at a time, if possible
        cmp     rdx,        64
        jge     .aligned_64
        
        ; Writes 32 bytes at a time, if possible
        cmp     rdx,        32
        jge     .aligned_32
    
        ; Writes 16 bytes at a time, if possible
        cmp     rdx,        16
        jge     .aligned_16
        
    .aligned_end:
        
        ; We're aligned on a 16-byte boundary, but there's not enough bytes
        ; to write in order to use the SSE instructions, so writes the
        ; remaining bytes one by one
        mov     rcx,            rdx
        mov     rax,            rsi
        jmp     .notaligned_loop
        
    .aligned_128:
        
        ; Writes 128 bytes into the memory buffer
        movdqa  [ rdi       ],  xmm0
        movdqa  [ rdi +  16 ],  xmm0
        movdqa  [ rdi +  32 ],  xmm0
        movdqa  [ rdi +  48 ],  xmm0
        movdqa  [ rdi +  64 ],  xmm0
        movdqa  [ rdi +  80 ],  xmm0
        movdqa  [ rdi +  96 ],  xmm0
        movdqa  [ rdi + 112 ],  xmm0
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     rdi,            128
        sub     rdx,            128
        
        ; Next bytes - Writes 128 bytes at a time, if possible
        cmp     rdx,            128
        jge     .aligned_128
        
        ; Next bytes - Writes 64 bytes at a time, if possible
        cmp     rdx,            64
        jge     .aligned_64
        
        ; Next bytes - Writes 32 bytes at a time, if possible
        cmp     rdx,            32
        jge     .aligned_32
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     rdx,            16
        jge     .aligned_16
        
        ; Checks if we're done writing bytes
        test    rdx,            rdx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
        
    .aligned_64:
        
        ; Writes 64 bytes into the memory buffer
        movdqa  [ rdi      ],   xmm0
        movdqa  [ rdi + 16 ],   xmm0
        movdqa  [ rdi + 32 ],   xmm0
        movdqa  [ rdi + 48 ],   xmm0
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     rdi,            64
        sub     rdx,            64
        
        ; Next bytes - Writes 64 bytes at a time, if possible
        cmp     rdx,            64
        jge     .aligned_64
        
        ; Next bytes - Writes 32 bytes at a time, if possible
        cmp     rdx,            32
        jge     .aligned_32
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     rdx,            16
        jge     .aligned_16
        
        ; Checks if we're done writing bytes
        test    rdx,            rdx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
        
    .aligned_32:
        
        ; Writes 32 bytes into the memory buffer
        movdqa  [ rdi      ],   xmm0
        movdqa  [ rdi + 16 ],   xmm0
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     rdi,            32
        sub     rdx,            32
        
        ; Next bytes - Writes 32 bytes at a time, if possible
        cmp     rdx,            32
        jge     .aligned_32
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     rdx,            16
        jge     .aligned_16
        
        ; Checks if we're done writing bytes
        test    rdx,            rdx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
        
    .aligned_16:
        
        ; Writes 16 bytes into the memory buffer
        movdqa  [ rdi ],        xmm0
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     rdi,            16
        sub     rdx,            16
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     rdx,            16
        jge     .aligned
        
        ; Checks if we're done writing bytes
        test    rdx,            rdx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
    
    .end:
        
        ; Return value - Gets the original pointer
        mov     rax,            r8
        
        ret

;-------------------------------------------------------------------------------
; 64-bits optimized memset() function
; 
; void * _memset64( void * p, int c, size_t n );
; 
; Input registers:
;       
;       - RDI:      The memory pointer
;       - RSI:      The character to repeat
;       - RDX:      The number of characters to set
; 
; Return registers:
;       
;       - RAX:      The memory pointer
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_memset64:
    
    ; Saves the original memory pointer, as we'll need to return it
    mov     r8,         rdi
    
    ; Checks for a NULL memory pointer
    test    rdi,        rdi
    jz      .end
    
    ; Checks for a zero size
    test    rdx,        rdx
    jz      .end
    
    ; Checks if bytes needs to be set to zero
    test    rsi,        rsi
    jz      .padded
    
    .pad:
        
        ; A character other than zero will be used. Fills the RSI registers
        ; with eight time the character value, so will be able to write 8 bytes
        ; at a time
        mov     rcx,        rsi
        shl     rsi,        8
        or      rsi,        rcx
        mov     rcx,        rsi
        shl     rsi,        16
        or      rsi,        rcx
        mov     rcx,        rsi
        shl     rsi,        32
        or      rsi,        rcx
        
    .padded:
        
        ; Aligns the memory pointer in RDI to a 8-byte boundary
        and     rdi,        -8
        
        ; Gets the number of misaligned bytes in the original memory pointer (R8)
        mov     rax,        r8
        sub     rax,        rdi
        
        ; Checks if the pointer is already aligned
        test    rax,        rax
        jz      .aligned
        
        ; If not, computes the number of bytes to be written until we are
        ; aligned on the next 8-byte boundary
        mov     rcx,        8
        sub     rcx,        rax
        
    .notaligned:
        
        ; Restores the original pointer in RDI
        mov     rdi,        r8
        
        ; Stores the character to write in RAX, so we can access it as a byte
        ; using AL
        mov     rax,        rsi
        
    .notaligned_loop:
        
        ; Checks if we have bytes to write
        test    rdx,        rdx
        jz      .end
        
        ; Writes a byte into the memory buffer
        mov     [ rdi ],    al
        
        ; Advances the memory pointer
        inc     rdi
        
        ; Decreases the number of bytes to write (RDX is the total, RCX is the
        ; number of bytes to write until we're aligned to a 8-byte boundary)
        dec     rcx
        dec     rdx
        
        ; Checks if we are aligned to a 8-byte boundary. If so, we'll be able
        ; to write more than one byte at a time
        test    rcx,        rcx
        jz      .aligned
        jmp     .notaligned_loop
        
    .aligned:
        
        ; Writes 128 bytes at a time, if possible
        cmp     rdx,        128
        jge     .aligned_128
        
        ; Writes 64 bytes at a time, if possible
        cmp     rdx,        64
        jge     .aligned_64
        
        ; Writes 32 bytes at a time, if possible
        cmp     rdx,        32
        jge     .aligned_32
    
        ; Writes 16 bytes at a time, if possible
        cmp     rdx,        16
        jge     .aligned_16
        
    .aligned_end:
        
        ; We're aligned on a 8-byte boundary, but there's not enough bytes
        ; to write, so writes the remaining bytes one by one
        mov     rcx,            rdx
        mov     rax,            rsi
        jmp     .notaligned_loop
        
    .aligned_128:
        
        ; Writes 128 bytes into the memory buffer
        mov  [ rdi       ],  rsi
        mov  [ rdi +   8 ],  rsi
        mov  [ rdi +  16 ],  rsi
        mov  [ rdi +  24 ],  rsi
        mov  [ rdi +  32 ],  rsi
        mov  [ rdi +  40 ],  rsi
        mov  [ rdi +  48 ],  rsi
        mov  [ rdi +  56 ],  rsi
        mov  [ rdi +  64 ],  rsi
        mov  [ rdi +  72 ],  rsi
        mov  [ rdi +  80 ],  rsi
        mov  [ rdi +  88 ],  rsi
        mov  [ rdi +  96 ],  rsi
        mov  [ rdi + 104 ],  rsi
        mov  [ rdi + 112 ],  rsi
        mov  [ rdi + 120 ],  rsi
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     rdi,            128
        sub     rdx,            128
        
        ; Next bytes - Writes 128 bytes at a time, if possible
        cmp     rdx,            128
        jge     .aligned_128
        
        ; Next bytes - Writes 64 bytes at a time, if possible
        cmp     rdx,            64
        jge     .aligned_64
        
        ; Next bytes - Writes 32 bytes at a time, if possible
        cmp     rdx,            32
        jge     .aligned_32
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     rdx,            16
        jge     .aligned_16
        
        ; Checks if we're done writing bytes
        test    rdx,            rdx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
        
    .aligned_64:
        
        ; Writes 64 bytes into the memory buffer
        mov  [ rdi       ],  rsi
        mov  [ rdi +   8 ],  rsi
        mov  [ rdi +  16 ],  rsi
        mov  [ rdi +  24 ],  rsi
        mov  [ rdi +  32 ],  rsi
        mov  [ rdi +  40 ],  rsi
        mov  [ rdi +  48 ],  rsi
        mov  [ rdi +  56 ],  rsi
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     rdi,            64
        sub     rdx,            64
        
        ; Next bytes - Writes 64 bytes at a time, if possible
        cmp     rdx,            64
        jge     .aligned_64
        
        ; Next bytes - Writes 32 bytes at a time, if possible
        cmp     rdx,            32
        jge     .aligned_32
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     rdx,            16
        jge     .aligned_16
        
        ; Checks if we're done writing bytes
        test    rdx,            rdx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
        
    .aligned_32:
        
        ; Writes 32 bytes into the memory buffer
        mov  [ rdi       ],  rsi
        mov  [ rdi +   8 ],  rsi
        mov  [ rdi +  16 ],  rsi
        mov  [ rdi +  24 ],  rsi
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     rdi,            32
        sub     rdx,            32
        
        ; Next bytes - Writes 32 bytes at a time, if possible
        cmp     rdx,            32
        jge     .aligned_32
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     rdx,            16
        jge     .aligned_16
        
        ; Checks if we're done writing bytes
        test    rdx,            rdx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
        
    .aligned_16:
        
        ; Writes 16 bytes into the memory buffer
        mov  [ rdi       ],  rsi
        mov  [ rdi +   8 ],  rsi
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     rdi,            16
        sub     rdx,            16
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     rdx,            16
        jge     .aligned
        
        ; Checks if we're done writing bytes
        test    rdx,            rdx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
    
    .end:
        
        ; Return value - Gets the original pointer
        mov     rax,            r8
        
        ret
