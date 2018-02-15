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
global _xeos_strlen

;-------------------------------------------------------------------------------
; C99 - 64 bits strlen() function
; 
; size_t strlen( const char * s );
; 
; Input registers:
;       
;       - RDI:      The string pointer
; 
; Return registers:
;       
;       - RAX:      The length of the string pointed by RDI
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_xeos_strlen:
    
    ; Checks the status of the SSE2 flag
    cmp QWORD [ rel __SSE2Status ], 1
    
    ; SSE2 are available - Use the optimized version of strlen()
    je  _strlen64_sse2
    
    ; Checks the status of the SSE2 flag
    cmp QWORD [ rel __SSE2Status ], 0
    
    ; SSE2 are not available - Use the less-optimized version of strlen()
    je  _strlen64
    
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
        
        ; Sets the SSE2 status flag for the next calls and process the string
        ; with the optimized version of strlen()
        mov QWORD [ rel __SSE2Status ], 1
        jmp _strlen64_sse2
    
    ; SSE2 not available
    .fail:
        
        ; Restores the values of RCX and RDX
        mov     rcx,    r8
        mov     rdx,    r9
        
        ; Sets the SSE2 status flag for the next calls and process the string
        ; with the less-optimized version of strlen()
        mov QWORD [ rel __SSE2Status ], 0
        jmp _strlen64
            
;-------------------------------------------------------------------------------
; 64-bits SSE2 optimized strlen() function
; 
; size_t _strlen64_sse2( const char * s );
; 
; Input registers:
;       
;       - RDI:      The string pointer
; 
; Return registers:
;       
;       - RAX:      The length of the string pointed by RDI
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_strlen64_sse2:
    
    ; Checks for a NULL string
    test        rdi,    rdi
    jz          .null
    
    ; Stores the original string pointer in RSI
    mov         rsi,    rdi
    
    ; Aligns the string pointer in RDI to a 16-byte boundary,
    ; so we can safelfy use the SSE instructions
    and         rdi,    -16
    
    ; Resets XMM0
    pxor        xmm0,   xmm0
    
    ; Compares 16 bytes from RDI with 0 (in XMM0)
    ; Equal bytes will be set to all 1s in XMM0, others to all 0s
    pcmpeqb     xmm0,   [ rdi ]
    
    ; Gets a mask in RDX with bits set to the most significant
    ; bits of each bytes from XMM0
    pmovmskb    rdx,    xmm0
    
    ; Gets the number of misaligned bytes in the original string pointer (RSI)
    mov         rcx,    rsi
    sub         rcx,    rdi
    
    ; As we aligned the string pointer in RDI to a 16-byte boundary,
    ; any preceding byte has to be ignored.
    ; So let's create a mask for those bytes in RAX, based on the number of
    ; misaligned bytes in the original string pointer (RCX)
    xor         rax,    rax
    not         rax
    shl         rax,    cl
    
    ; Masks the unwanted bytes in RDX, and checks if a 0 byte was found
    and         rdx,    rax
    jnz         .found
    
    .notfound:
        
        ; Next 16 bytes from RDI will be checked
        add         rdi,    16
        
        ; Resets XMM0
        pxor        xmm0,   xmm0
        
        ; Compares 16 bytes from RDI with 0 (in XMM0)
        ; Equal bytes will be set to all 1s in XMM0, others to all 0s
        pcmpeqb     xmm0,   [ rdi ]
        
        ; Gets a mask in RDX with bits set to the most significant
        ; bits of each bytes from XMM0
        pmovmskb    rdx,    xmm0
        
        ; Checks if a bit is set, meaning a zero byte was found
        test        rdx,    rdx
        jz          .notfound
        
    .found:
        
        ; Gets the index of the first bit set in RAX
        ; (index of the found 0-byte)
        bsf         rax,    rdx
        
        ; Computes the difference between the original string pointer
        ; and the current value of RDI, and adjusts RAX so it now
        ; contains the string length
        sub         rsi,    rdi
        sub         rax,    rsi
        
        ret
    
    .null:
        
        ; NULL string - Returns 0
        xor         rax,    rax
        
        ret
        
;-------------------------------------------------------------------------------
; 64-bits optimized strlen() function
; 
; size_t _strlen64( const char * s );
; 
; Input registers:
;       
;       - RDI:      The string pointer
; 
; Return registers:
;       
;       - RAX:      The length of the string pointed by RDI
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_strlen64:
    
    ; Checks for a NULL string
    test        rdi,    rdi
    jz          .null
    
    ; Stores the original string pointer in RAX
    mov         rax,    rdi
    
    ; Aligns the string pointer in RDI to a 8-byte boundary
    and         rdi,    -8
    
    ; Gets the number of misaligned bytes in the original string pointer,
    ; and keeps a copy in RSI
    sub         rax,    rdi
    mov         rsi,    rax
    
    ; Multiples by eight to get the number of misaligned bits (in RAX)
    mov         rcx,    8
    mul         rcx
    
    ; As we aligned the string pointer in RDI to a 8-byte boundary,
    ; any preceding byte has to be ignored.
    ; So let's create a mask for those bytes in RAX, based on the number of
    ; misaligned bytes in the original string pointer (RCX)
    xor         rcx,    rcx
    not         rcx
    xchg        rax,    rcx
    shl         rax,    cl
    not         rax
    
    ; Reads 8 bytes from the string
    mov         rdx,    [ rdi ]
    
    ; Masks the unwanted bytes in RDX
    or          rdx,    rax
    
    ; Resets RAX
    xor         rax,    rax
    
    ; Loop that scans an aligned quad-word for 0
    ; This could have been implemented with an assembly loop, to avoid
    ; repeating the test for each byte, but it is much faster this way.
    .scan:
        
        ; Checks if a byte from RDX is zero - Thanks to Sean Eron Anderson:
        ; http://graphics.stanford.edu/~seander/bithacks.html
        mov         r10,    0x0101010101010101
        mov         r11,    0x8080808080808080
        mov         r8,     rdx
        mov         r9,     rdx
        sub         r8,     r10
        not         r9
        and         r9,     r11
        and         r8,     r9
        test        r8,     r8
        jnz         .test
        
        ; Increase RAX
        add         rax,    8
        
        ; Reads the next 8 bytes from the string
        add         rdi,    8
        mov         rdx,    [ rdi ]
        
        ; Scans the next bytes
        jmp         .scan
        
    .test:
        
        ; Test byte 1 for 0
        test        rdx,    0xFF
        jz          .found
        
        ; Increase RAX and prepare next byte
        inc         rax
        shr         rdx,    8
        
        ; Test byte 2 for 0
        test        rdx,    0xFF
        jz          .found
        
        ; Increase RAX and prepare next byte
        inc         rax
        shr         rdx,    8
        
        ; Test byte 3 for 0
        test        rdx,    0xFF
        jz          .found
        
        ; Increase RAX and prepare next byte
        inc         rax
        shr         rdx,    8
        
        ; Test byte 4 for 0
        test        rdx,    0xFF
        jz          .found
        
        ; Increase RAX and prepare next byte
        inc         rax
        shr         rdx,    8
        
        ; Test byte 5 for 0
        test        rdx,    0xFF
        jz          .found
        
        ; Increase RAX and prepare next byte
        inc         rax
        shr         rdx,    8
        
        ; Test byte 6 for 0
        test        rdx,    0xFF
        jz          .found
        
        ; Increase RAX and prepare next byte
        inc         rax
        shr         rdx,    8
        
        ; Test byte 7 for 0
        test        rdx,    0xFF
        jz          .found
        
        ; Increase RAX and prepare next byte
        inc         rax
        shr         rdx,    8
        
        ; Test byte 8 for 0
        test        rdx,    0xFF
        jz          .found
        
        ; Increase RAX
        inc         rax
        
        ; Reads the next 8 bytes from the string
        add         rdi,    8
        mov         rdx,    [ rdi ]
        
        ; Scans the next bytes
        jmp         .scan
    
    .found:
        
        ; Substract the number of preceding bytes needed to align the string
        ; pointer to a 8-byte boundary
        sub         rax,    rsi
        
        ret
    
    .null:
        
        ; NULL string - Returns 0
        xor         rax,    rax
        
        ret
