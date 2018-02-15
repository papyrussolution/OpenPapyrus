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
global _xeos_memcpy

;-------------------------------------------------------------------------------
; C99 - 64 bits memcpy() function
; 
; void * memcpy( void * restrict s1, const void * restrict s2, size_t n );
; 
; Input registers:
;       
;       - RDI:      The destination pointer
;       - RSI:      The source pointer
;       - RDX:      The number of bytes to copy
; 
; Return registers:
;       
;       - RAX:      The destination pointer
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_xeos_memcpy:

.start:
    
    ; Checks the status of the SSE2 flag
    cmp QWORD [ rel __SSE2Status ], 1
    
    ; SSE2 are available - Use the optimized version of memcpy()
    je  _memcpy64_sse2
    
    ; Checks the status of the SSE2 flag
    cmp QWORD [ rel __SSE2Status ], 0
    
    ; SSE2 are not available - Use the less-optimized version of memcpy()
    je  _memcpy64
    
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
        
        ; Sets the SSE2 status flag for the next calls and process the buffer
        ; with the optimized version of memcpy()
        mov QWORD [ rel __SSE2Status ], 1
        jmp _memcpy64_sse2
    
    ; SSE2 not available
    .fail:
        
        ; Restores the values of RCX and RDX
        mov     rcx,    r8
        mov     rdx,    r9
        
        ; Sets the SSE2 status flag for the next calls and process the buffer
        ; with the less-optimized version of memcpy()
        mov QWORD [ rel __SSE2Status ], 0
        jmp _memcpy64
            
;-------------------------------------------------------------------------------
; 64-bits SSE2 optimized memcpy() function
; 
; void * _memcpy64_sse2( void * restrict s1, const void * restrict s2, size_t n );
; 
; Input registers:
;       
;       - RDI:      The destination pointer
;       - RSI:      The source pointer
;       - RDX:      The number of bytes to copy
; 
; Return registers:
;       
;       - RAX:      The destination pointer
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_memcpy64_sse2:
    
    ; Saves the original memory pointer, as we'll need to return it
    mov         r8,         rdi
    
    ; Checks for a NULL destination pointer
    test        rdi,        rdi
    jz          .ret
    
    ; Checks for a NULL source pointer
    test        rsi,        rsi
    jz          .ret
    
    ; Gets the number of misaligned bytes in the original source pointer
    mov         rax,        rsi
    mov         rcx,        rsi
    and         rcx,        -16
    sub         rax,        rcx
    mov         rcx,        16
    sub         rcx,        rax
    
    ; Checks if the source pointer is already aligned
    cmp         rcx,        16
    jne         .source_notaligned
        
    .source_aligned:
        
        ; Gets the number of misaligned bytes in the original destination pointer
        mov         rax,        rdi
        mov         rcx,        rdi
        and         rcx,        -16
        sub         rax,        rcx
        mov         rcx,        16
        sub         rcx,        rax
        
        ; Checks if the destination pointer is also aligned
        cmp         rcx,        16
        jne         .dest_notaligned
        
        .source_dest_aligned:
            
            ; Writes 128 bytes at a time, if possible
            cmp         rdx,        128
            jb          .source_dest_aligned_64
            
            .source_dest_aligned_128:
                
                ; Reads 128 bytes from the source buffer
                movdqa      xmm0,           [ rsi ]
                movdqa      xmm1,           [ rsi + 16 ]
                movdqa      xmm2,           [ rsi + 32 ]
                movdqa      xmm3,           [ rsi + 48 ]
                movdqa      xmm4,           [ rsi + 64 ]
                movdqa      xmm5,           [ rsi + 80 ]
                movdqa      xmm6,           [ rsi + 96 ]
                movdqa      xmm7,           [ rsi + 112 ]
                
                ; Writes 128 bytes into the destination buffer
                movdqa      [ rdi ],        xmm0
                movdqa      [ rdi +  16 ],  xmm1
                movdqa      [ rdi +  32 ],  xmm2
                movdqa      [ rdi +  48 ],  xmm3
                movdqa      [ rdi +  64 ],  xmm4
                movdqa      [ rdi +  80 ],  xmm5
                movdqa      [ rdi +  96 ],  xmm6
                movdqa      [ rdi + 112 ],  xmm7
                
                ; Advances the source and destination pointers and decreases
                ; the number of bytes to write
                add         rdi,        128
                add         rsi,        128
                sub         rdx,        128
                
                ; Writes 128 bytes at a time, if possible
                cmp         rdx,        128
                jge         .source_dest_aligned_128
            
            .source_dest_aligned_64:
                
                ; Writes 64 bytes at a time, if possible
                cmp         rdx,        64
                jb          .source_dest_aligned_16
                
                ; Reads 64 bytes from the source buffer
                movdqa      xmm0,           [ rsi ]
                movdqa      xmm1,           [ rsi + 16 ]
                movdqa      xmm2,           [ rsi + 32 ]
                movdqa      xmm3,           [ rsi + 48 ]
                
                ; Writes 64 bytes into the destination buffer
                movdqa      [ rdi ],        xmm0
                movdqa      [ rdi +  16 ],  xmm1
                movdqa      [ rdi +  32 ],  xmm2
                movdqa      [ rdi +  48 ],  xmm3
                
                ; Advances the source and destination pointers and decreases
                ; the number of bytes to write
                add         rdi,        64
                add         rsi,        64
                sub         rdx,        64
                
                ; Continues writing
                jmp         .source_dest_aligned_64
                
            .source_dest_aligned_16:
                
                ; Writes 16 bytes at a time, if possible
                cmp         rdx,        16
                jb          .copy_end
                
                ; Reads 16 bytes from the source buffer
                movdqa      xmm0,       [ rsi ]
                
                ; Writes 16 bytes into the destination buffer
                movdqa      [ rdi ],    xmm0
                
                ; Advances the source and destination pointers and decreases
                ; the number of bytes to write
                add         rdi,        16
                add         rsi,        16
                sub         rdx,        16
                
                ; Continues writing
                jmp         .source_dest_aligned_16
                
                ; Writes remaining bytes one by one
                jmp         .copy_end
                
        .dest_notaligned:
            
            ; Writes 128 bytes at a time, if possible
            cmp         rdx,        128
            jb          .dest_notaligned_64
            
            .dest_notaligned_128:
                
                ; Reads 128 bytes from the source buffer
                movdqa      xmm0,           [ rsi ]
                movdqa      xmm1,           [ rsi + 16 ]
                movdqa      xmm2,           [ rsi + 32 ]
                movdqa      xmm3,           [ rsi + 48 ]
                movdqa      xmm4,           [ rsi + 64 ]
                movdqa      xmm5,           [ rsi + 80 ]
                movdqa      xmm6,           [ rsi + 96 ]
                movdqa      xmm7,           [ rsi + 112 ]
                
                ; Writes 128 bytes into the destination buffer
                movdqu      [ rdi ],        xmm0
                movdqu      [ rdi +  16 ],  xmm1
                movdqu      [ rdi +  32 ],  xmm2
                movdqu      [ rdi +  48 ],  xmm3
                movdqu      [ rdi +  64 ],  xmm4
                movdqu      [ rdi +  80 ],  xmm5
                movdqu      [ rdi +  96 ],  xmm6
                movdqu      [ rdi + 112 ],  xmm7
                
                ; Advances the source and destination pointers and decreases
                ; the number of bytes to write
                add         rdi,        128
                add         rsi,        128
                sub         rdx,        128
                
                ; Writes 128 bytes at a time, if possible
                cmp         rdx,        128
                jge         .dest_notaligned_128
            
            .dest_notaligned_64:
                
                ; Writes 64 bytes at a time, if possible
                cmp         rdx,        64
                jb          .dest_notaligned_16
                
                ; Reads 64 bytes from the source buffer
                movdqa      xmm0,           [ rsi ]
                movdqa      xmm1,           [ rsi + 16 ]
                movdqa      xmm2,           [ rsi + 32 ]
                movdqa      xmm3,           [ rsi + 48 ]
                
                ; Writes 64 bytes into the destination buffer
                movdqu      [ rdi ],        xmm0
                movdqu      [ rdi +  16 ],  xmm1
                movdqu      [ rdi +  32 ],  xmm2
                movdqu      [ rdi +  48 ],  xmm3
                
                ; Advances the source and destination pointers and decreases
                ; the number of bytes to write
                add         rdi,        64
                add         rsi,        64
                sub         rdx,        64
                
                ; Continues writing
                jmp         .dest_notaligned_64
                
            .dest_notaligned_16:
                
                ; Writes 16 bytes at a time, if possible
                cmp         rdx,        16
                jb          .copy_end
                
                ; Reads 16 bytes from the source buffer
                movdqa      xmm0,       [ rsi ]
                
                ; Writes 16 bytes into the destination buffer
                movdqu      [ rdi ],    xmm0
                
                ; Advances the source and destination pointers and decreases
                ; the number of bytes to write
                add         rdi,        16
                add         rsi,        16
                sub         rdx,        16
                
                ; Continues writing
                jmp         .dest_notaligned_16
                
    .copy_end:
        
        ; Checks if we have bytes to write
        test        rdx,        rdx
        jz          .ret
        
        ; Reads a byte from the source buffer
        mov         al,         [ rsi ]
        
        ; Writes a byte into the destination buffer
        mov         [ rdi ],    al
        
        ; Advances the source and destination pointers and decreases
        ; the number of bytes to write
        add         rdi,        1
        add         rsi,        1
        sub         rdx,        1
        sub         rcx,        1
        
        ; Not aligned - Continues writing single bytes
        jmp         .copy_end
        
    .source_notaligned:
        
        ; Checks if we have bytes to write
        test        rdx,        rdx
        jz          .ret
        
        ; Reads a byte from the source buffer
        mov         al,         [ rsi ]
        
        ; Writes a byte into the destination buffer
        mov         [ rdi ],    al
        
        ; Advances the source and destination pointers and decreases
        ; the number of bytes to write
        add         rdi,        1
        add         rsi,        1
        sub         rdx,        1
        sub         rcx,        1
        
        ; Checks if we're aligned
        test        rcx,        rcx
        jz          .source_aligned
        
        ; Not aligned - Continues writing single bytes
        jmp         .source_notaligned
        
    .ret:
        
        ; Returns the original destination pointer
        mov         rax,    r8
        
        ret
        
;-------------------------------------------------------------------------------
; 64-bits optimized memcpy() function
; 
; void * _memcpy64( void * restrict s1, const void * restrict s2, size_t n );
; 
; Input registers:
;       
;       - RDI:      The destination pointer
;       - RSI:      The source pointer
;       - RDX:      The number of bytes to copy
; 
; Return registers:
;       
;       - RAX:      The destination pointer
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_memcpy64:
    
    ; Saves the original memory pointer, as we'll need to return it
    mov         r8,         rdi
    
    ; Checks for a NULL destination pointer
    test        rdi,        rdi
    jz          .ret
    
    ; Checks for a NULL source pointer
    test        rsi,        rsi
    jz          .ret
    
    ; Gets the number of misaligned bytes in the original source pointer
    mov         rax,        rsi
    mov         rcx,        rsi
    and         rcx,        -8
    sub         rax,        rcx
    mov         rcx,        8
    sub         rcx,        rax
    
    ; Checks if the source pointer is already aligned
    cmp         rcx,        8
    jne         .source_notaligned
        
    .source_aligned:
        
        ; IMPORTANT NOTE
        ; 
        ; At this point, the source pointer is aligned to a 8-byte boundary.
        ; The destination pointer might not be.
        ; As the x86/x86-64 architecture allows unaligned memory access,
        ; let's pretend we don't care about this.
        ; Of course, unaligned memory access might be slower, but the
        ; destination pointer should be aligned most of the time.
        ; Moreover, there's almost no performance penalty with recent CPUs
        ; from Intel (Sandy Bridge, Nehalem).
        
        .source_dest_aligned:
            
            ; Writes 128 bytes at a time, if possible
            cmp         rdx,        128
            jb          .source_dest_aligned_64
            
            .source_dest_aligned_128:
                
                ; Reads and writes 128 bytes from the source buffer into
                ; the destination buffer
                mov         r8,             [ rsi ]
                mov         r9,             [ rsi + 8 ]
                mov         r10,            [ rsi + 16 ]
                mov         r11,            [ rsi + 24 ]
                mov         [ rdi ],        r8
                mov         [ rdi +  8 ],   r9
                mov         [ rdi + 16 ],   r10
                mov         [ rdi + 24 ],   r11
                mov         r8,             [ rsi + 32 ]
                mov         r9,             [ rsi + 40 ]
                mov         r10,            [ rsi + 48 ]
                mov         r11,            [ rsi + 56 ]
                mov         [ rdi + 32 ],   r8
                mov         [ rdi + 40 ],   r9
                mov         [ rdi + 48 ],   r10
                mov         [ rdi + 56 ],   r11
                mov         r8,             [ rsi + 64 ]
                mov         r9,             [ rsi + 72 ]
                mov         r10,            [ rsi + 80 ]
                mov         r11,            [ rsi + 88 ]
                mov         [ rdi + 64 ],   r8
                mov         [ rdi + 72 ],   r9
                mov         [ rdi + 80 ],   r10
                mov         [ rdi + 88 ],   r11
                mov         r8,             [ rsi + 96 ]
                mov         r9,             [ rsi + 104 ]
                mov         r10,            [ rsi + 112 ]
                mov         r11,            [ rsi + 120 ]
                mov         [ rdi +  96 ],  r8
                mov         [ rdi + 104 ],  r9
                mov         [ rdi + 112 ],  r10
                mov         [ rdi + 120 ],  r11
                
                ; Advances the source and destination pointers and decreases
                ; the number of bytes to write
                add         rdi,        128
                add         rsi,        128
                sub         rdx,        128
                
                ; Writes 128 bytes at a time, if possible
                cmp         rdx,        128
                jge         .source_dest_aligned_128
            
            .source_dest_aligned_64:
                
                ; Writes 64 bytes at a time, if possible
                cmp         rdx,        64
                jb          .source_dest_aligned_8
                
                ; Reads and writes 64 bytes from the source buffer into
                ; the destination buffer
                mov         r8,             [ rsi ]
                mov         r9,             [ rsi + 8 ]
                mov         r10,            [ rsi + 16 ]
                mov         r11,            [ rsi + 24 ]
                mov         [ rdi ],        r8
                mov         [ rdi +  8 ],   r9
                mov         [ rdi + 16 ],   r10
                mov         [ rdi + 24 ],   r11
                mov         r8,             [ rsi + 32 ]
                mov         r9,             [ rsi + 40 ]
                mov         r10,            [ rsi + 48 ]
                mov         r11,            [ rsi + 56 ]
                mov         [ rdi + 32 ],   r8
                mov         [ rdi + 40 ],   r9
                mov         [ rdi + 48 ],   r10
                mov         [ rdi + 56 ],   r11
                
                ; Advances the source and destination pointers and decreases
                ; the number of bytes to write
                add         rdi,        64
                add         rsi,        64
                sub         rdx,        64
                
                ; Continues writing
                jmp         .source_dest_aligned_64
                
            .source_dest_aligned_8:
                
                ; Writes 8 bytes at a time, if possible
                cmp         rdx,        8
                jb          .copy_end
                
                ; Reads and writes 8 bytes from the source buffer into
                ; the destination buffer
                mov         r8,             [ rsi ]
                mov         [ rdi ],        r8
                
                ; Advances the source and destination pointers and decreases
                ; the number of bytes to write
                add         rdi,        8
                add         rsi,        8
                sub         rdx,        8
                
                ; Continues writing
                jmp         .source_dest_aligned_8
                
    .copy_end:
        
        ; Checks if we have bytes to write
        test        rdx,        rdx
        jz          .ret
        
        ; Reads a byte from the source buffer
        mov         al,         [ rsi ]
        
        ; Writes a byte into the destination buffer
        mov         [ rdi ],    al
        
        ; Advances the source and destination pointers and decreases
        ; the number of bytes to write
        add         rdi,        1
        add         rsi,        1
        sub         rdx,        1
        sub         rcx,        1
        
        ; Not aligned - Continues writing single bytes
        jmp         .copy_end
        
    .source_notaligned:
        
        ; Checks if we have bytes to write
        test        rdx,        rdx
        jz          .ret
        
        ; Reads a byte from the source buffer
        mov         al,         [ rsi ]
        
        ; Writes a byte into the destination buffer
        mov         [ rdi ],    al
        
        ; Advances the source and destination pointers and decreases
        ; the number of bytes to write
        add         rdi,        1
        add         rsi,        1
        sub         rdx,        1
        sub         rcx,        1
        
        ; Checks if we're aligned
        test        rcx,        rcx
        jz          .source_aligned
        
        ; Not aligned - Continues writing single bytes
        jmp         .source_notaligned
        
    .ret:
        
        ; Returns the original destination pointer
        mov         rax,    r8
        
        ret
