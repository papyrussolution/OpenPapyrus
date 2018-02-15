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
global _xeos_memchr

;-------------------------------------------------------------------------------
; C99 - 64 bits memchr() function
; 
; void * memchr( const void * s, int c, size_t n );
; 
; Input registers:
;       
;       - RDI:      The memory pointer
;       - RSI:      The character to search
;       - RDX:      The size of the memory buffer
; 
; Return registers:
;       
;       - RAX:      A pointer to the first occurence of the character in the
;                   buffer, or 0 (NULL)
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_xeos_memchr:

.start:
    
    ; Checks the status of the SSE2 flag
    cmp QWORD [ rel __SSE2Status ], 1
    
    ; SSE2 are available - Use the optimized version of memchr()
    je  _memchr64_sse2
    
    ; Checks the status of the SSE2 flag
    cmp QWORD [ rel __SSE2Status ], 0
    
    ; SSE2 are not available - Use the less-optimized version of memchr()
    je  _memchr64
    
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
        ; with the optimized version of memchr()
        mov QWORD [ rel __SSE2Status ], 1
        jmp _memchr64_sse2
    
    ; SSE2 not available
    .fail:
        
        ; Restores the values of RCX and RDX
        mov     rcx,    r8
        mov     rdx,    r9
        
        ; Sets the SSE2 status flag for the next calls and process the buffer
        ; with the less-optimized version of memchr()
        mov QWORD [ rel __SSE2Status ], 0
        jmp _memchr64
            
;-------------------------------------------------------------------------------
; 64-bits SSE2 optimized memchr() function
; 
; void * _memchr64_sse2( const void * s, int c, size_t n );
; 
; Input registers:
;       
;       - RDI:      The memory pointer
;       - RSI:      The character to search
;       - RDX:      The size of the memory buffer
; 
; Return registers:
;       
;       - RAX:      A pointer to the first occurence of the character in the
;                   buffer, or 0 (NULL)
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_memchr64_sse2:
    
    ; Checks for a NULL buffer
    test        rdi,    rdi
    jz          .null
    
    ; Checks for a zero size
    test        rdx,    rdx
    jz          .null
    
    ; Ensures the character to search is 8 bits, and stores it in XMM0
    and         rsi,    0xFF
    movq        xmm0,   rsi
    
    ; Stores the original memory pointer in RSI
    mov         rsi,    rdi
    
    ; Aligns the memory pointer in RDI to a 16-byte boundary,
    ; so we can safelfy use the SSE instructions
    and         rdi,    -16
    
    ; Prepares XMM0's value so we can search for the character using pcmpeqb
	punpcklbw   xmm0,   xmm0
	punpcklbw   xmm0,   xmm0
	pshufd      xmm0,   xmm0,   0
    
    ; Compares 16 bytes from RDI with the character to search (in XMM0)
    ; Equal bytes will be set to all 1s in XMM1, others to all 0s
	movdqa      xmm1,   [ rdi ]
	pcmpeqb     xmm1,   xmm0
    
    ; Gets a mask in R8 with bits set to the most significant
    ; bits of each bytes from XMM1
	pmovmskb    r8,     xmm1
    
    ; Gets the number of misaligned bytes in the original memory pointer (RSI)
    mov         rcx,    rsi
    sub         rcx,    rdi
    
    ; As we aligned the memory pointer in RDI to a 16-byte boundary,
    ; any preceding byte has to be ignored.
    ; So let's create a mask for those bytes in RAX, based on the number of
    ; misaligned bytes in the original memory pointer (RCX)
    xor         rax,    rax
    not         rax
    shl         rax,    cl
    
    ; As we may read more bytes, depending on the alignment, adjusts the buffer
    ; size in RDX
    add         rdx,    rcx
    
    ; Masks the unwanted bytes in R8, and checks if the character was found
    and         r8,     rax
    jnz         .found
    
    .notfound:
        
        ; We've read 16 bytes - Advances the memory pointer and decrease
        ; the buffer size
        add         rdi,    16
        sub         rdx,    16
        
        ; Checks if we've reached the buffer size limit
        cmp         rdx,    0
        jle         .null
        
        ; Checks if we can read 64 bytes at a time - if not, 16 bytes at a
        ; time will be read
        cmp         rdx,    64
        jl          .notfound_16
        
    .notfound_64:
        
        ; Reads the next 64 bytes from RDI into the XMM registers
        movdqa      xmm1,   [ rdi ]
        movdqa      xmm2,   [ rdi + 16 ]
        movdqa      xmm3,   [ rdi + 32 ]
        movdqa      xmm4,   [ rdi + 48 ]
        
        ; Compares the 64 bytes read with the character to search (in XMM0)
        ; Equal bytes will be set to all 1s in the XMM reisters, others to all 0s
        pcmpeqb     xmm1,   xmm0
        pcmpeqb     xmm2,   xmm0
        pcmpeqb     xmm3,   xmm0
        pcmpeqb     xmm4,   xmm0
        
        ; Gets a mask in R8 with bits set to the most significant
        ; bits of each bytes from XMM1
        pmovmskb    r8,     xmm1
        
        ; Checks if a bit is set, meaning the character was found
        test        r8,     r8
        jnz         .found
        
        ; Checks the next 16 bytes - Advances the memory pointer and decrease
        ; the buffer size
        add         rdi,    16
        sub         rdx,    16
        
        ; Gets a mask in R8 with bits set to the most significant
        ; bits of each bytes from XMM2
        pmovmskb    r8,     xmm2
        
        ; Checks if a bit is set, meaning the character was found
        test        r8,     r8
        jnz         .found
        
        ; Checks the next 16 bytes - Advances the memory pointer and decrease
        ; the buffer size
        add         rdi,    16
        sub         rdx,    16
        
        ; Gets a mask in R8 with bits set to the most significant
        ; bits of each bytes from XMM3
        pmovmskb    r8,     xmm3
        
        ; Checks if a bit is set, meaning the character was found
        test        r8,     r8
        jnz         .found
        
        ; Checks the next 16 bytes - Advances the memory pointer and decrease
        ; the buffer size
        add         rdi,    16
        sub         rdx,    16
        
        ; Gets a mask in R8 with bits set to the most significant
        ; bits of each bytes from XMM4
        pmovmskb    r8,     xmm4
        
        ; Checks if a bit is set, meaning the character was found
        test        r8,     r8
        jnz         .found
        
        ; Not found - Continues scanning
        jmp         .notfound
        
    .notfound_16:
        
        ; Compares 16 bytes from RDI with the character to search (in XMM0)
        ; Equal bytes will be set to all 1s in XMM1, others to all 0s
        movdqa      xmm1,   [ rdi ]
        pcmpeqb     xmm1,   xmm0
        
        ; Gets a mask in R8 with bits set to the most significant
        ; bits of each bytes from XMM1
        pmovmskb    r8,     xmm1
        
        ; Checks if a bit is set, meaning the character was found
        test        r8,     r8
        jnz         .found
        
        ; Not found - Continues scanning
        jmp         .notfound
        
    .found:
        
        ; Gets the index of the first bit set in RAX
        ; (index of the found character)
        bsf         rax,    r8
        
        ; Substracts the character index from the remaining buffer size
        sub         rdx,    rax
        
        ; Checks if we've reached the buffer size limit
        cmp         rdx,    0
        jle         .null
        
        ; Adds the character index to the current value of the memory pointer,
        ; so we'll return the memory location to the found character
        add         rax,    rdi
        
        ret
        
    .null:
        
        ; Returns NULL
        xor         rax,    rax
        
        ret
        
;-------------------------------------------------------------------------------
; 64-bits optimized memchr() function
; 
; void * _memchr64( const void * s, int c, size_t n );
; 
; Input registers:
;       
;       - RDI:      The memory pointer
;       - RSI:      The character to search
;       - RDX:      The size of the memory buffer
; 
; Return registers:
;       
;       - RAX:      A pointer to the first occurence of the character in the
;                   buffer, or 0 (NULL)
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_memchr64:
    
    ; Checks for a NULL buffer
    test        rdi,    rdi
    jz          .null
    
    ; Ensures the character to search is 8 bits
    and         rsi,    0xFF
    
    ; Stores the original memory pointer in RCX and RAX
    mov         rcx,    rdi
    mov         rax,    rdi
    
    ; Aligns the pointer in RAX to a 8-byte boundary
    and         rax,    -8
    
    ; Gets the number of misaligned bytes in the original memory pointer (RCX)
    sub         rcx,    rax
    
    ; Checks if we're aligned
    test        rcx,    rcx
    jnz         .notaligned
    
    .aligned:
        
        ; Saves RDX as we are going to alter it
        xchg        rdx,    r8
        
        ; Creates the magic number allowing to test if a QWORD contains a
        ; byte - Thanks to Sean Eron Anderson:
        ; http://graphics.stanford.edu/~seander/bithacks.html
        xor         rdx,    rdx
        xor         rax,    rax
        not         rax
        mov         rcx,    255
        div         rcx
        mul         rsi
        mov         r10,    rax
        
        ; Restores RDX
        xchg        rdx,    r8
        
        ; Fill all 8 parts of RSI with the 8 bits to search
        mov         rcx,    rsi
        shl         rsi,    8
        or          rsi,    rcx
        mov         rcx,    rsi
        shl         rsi,    16
        or          rsi,    rcx
        mov         rcx,    rsi
        shl         rsi,    32
        or          rsi,    rcx
        
    .aligned_loop:
        
        ; Checks if we've reached the buffer size limit
        test        rdx,    rdx
        jz          .null
        
        ; Checks if we can read 8 bytes (otherwise, we'll read one byte at a time)
        cmp         rdx,    8
        jb          .aligned_end
        
        ; Reads 8 bytes from RDI
        mov         rax,    [ rdi ]
        
        ; Checks if the current QWORD contains the byte to search (magic number
        ; is in R10) - Thanks to Sean Eron Anderson:
        ; http://graphics.stanford.edu/~seander/bithacks.html
        mov         r8,     rax
        xor         r8,     r10
        mov         r9,     r8
        not         r9
        mov         rcx,    0x0101010101010101
        sub         r8,     rcx
        mov         rcx,    0x8080808080808080
        and         r9,     rcx
        test        r8,     r9
        jnz         .aligned_found
        
        ; Not found, process next 8 bytes
        add         rdi,    8
        sub         rdx,    8
        jmp         .aligned_loop
        
    .aligned_found:
        
        ; XOR the bytes read with RSI, so matching bytes will be zero
        xor         rax,    rsi
        
        ; Checks if the character was found
        test        rax,    0x000000FF
        jz          .found
        
        ; Advances the memory pointer and decreases the buffer size
        add         rdi,    1
        sub         rdx,    1
        
        ; Checks if the character was found in the next byte
        test        rax,    0x0000FF00
        jz          .found
        
        ; Advances the memory pointer and decreases the buffer size
        add         rdi,    1
        sub         rdx,    1
        
        ; Checks if the character was found in the next byte
        test        rax,    0x00FF0000
        jz          .found
        
        ; Advances the memory pointer and decreases the buffer size
        add         rdi,    1
        sub         rdx,    1
        
        ; Checks if the character was found in the next byte
        test        rax,    0xFF000000
        jz          .found
        
        ; Process next 4 bytes
        shr         rax,    32
        
        ; Advances the memory pointer and decreases the buffer size
        add         rdi,    1
        sub         rdx,    1
        
        ; Checks if the character was found in the next byte
        test        rax,    0x000000FF
        jz          .found
        
        ; Advances the memory pointer and decreases the buffer size
        add         rdi,    1
        sub         rdx,    1
        
        ; Checks if the character was found in the next byte
        test        rax,    0x0000FF00
        jz          .found
        
        ; Advances the memory pointer and decreases the buffer size
        add         rdi,    1
        sub         rdx,    1
        
        ; Checks if the character was found in the next byte
        test        rax,    0x00FF0000
        jz          .found
        
        ; Advances the memory pointer and decreases the buffer size
        add         rdi,    1
        sub         rdx,    1
        
        ; Checks if the character was found in the next byte
        test        rax,    0xFF000000
        jz          .found
        
        ; Advances the memory pointer and decreases the buffer size
        add         rdi,    1
        sub         rdx,    1
        
        ; Not found - Continues scanning
        jmp         .aligned_loop
    
    .aligned_end:
        
        ; Ensures the character to search is 8 bits
        and         rsi,    0xFF
        
        ; Number of remaining bytes
        mov         rcx,    rdx
        
    .notaligned:
        
        ; Checks if we've reached the buffer size limit
        test        rdx,    rdx
        jz          .null
        
        ; Reads a byte from RDI
        xor         rax,    rax
        mov         al,     [ rdi ]
        
        ; Checks if the character is found
        cmp         rax,    rsi
        je          .found
        
        ; Advances the memory pointer and decreases the buffer size
        add         rdi,    1
        sub         rdx,    1
        
        ; Decreases the number of time we need to loop until we're aligned
        sub         rcx,    1
        
        ; Checks if we're aligned
        test        rcx,    rcx
        jz          .aligned
        
        ; Not aligned - Continues scanning
        jmp         .notaligned
        
    .found:
        
        ; Address of the found character
        mov         rax,    rdi
        
        ret
        
    .null:
        
        ; Returns NULL
        xor         rax,    rax
        
        ret
