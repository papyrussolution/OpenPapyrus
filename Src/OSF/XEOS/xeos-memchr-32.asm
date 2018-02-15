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

; We are in 32 bits mode
BITS    32

section .data

global __SSE2Status

; SSE2 status flag:
; 
;   -1: Unchecked
;    0: SSE2 not available
;    1: SSE2 available
__SSE2Status:   dd  -1

section .text

; Makes the entry point visible to the linker
global _xeos_memchr

;-------------------------------------------------------------------------------
; C99 - 32 bits memchr() function
; 
; void * memchr( const void * s, int c, size_t n );
; 
; Input registers:
;       
;       None - Arguments on stack
; 
; Return registers:
;       
;       - EAX:      A pointer to the first occurence of the character in the
;                   buffer, or 0 (NULL)
; 
; Killed registers:
;       
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;-------------------------------------------------------------------------------
_xeos_memchr:
    
    ; Checks the status of the SSE2 flag
    cmp DWORD [ ds:__SSE2Status ], 1
    
    ; SSE2 are available - Use the optimized version of memchr()
    je  _memchr32_sse2
    
    ; Checks the status of the SSE2 flag
    cmp DWORD [ ds:__SSE2Status ], 0
    
    ; SSE2 are not available - Use the less-optimized version of memchr()
    je  _memchr32
    
    ; SSE2 status needs to be checked
    .check:
        
        ; CPUID - Asks for CPU features (EAX=1)
        mov     eax,    1
        cpuid
        
        ; Checks the SSE2 bit (bit 26)
        test    edx,    0x4000000      
        jz      .fail
        
    ; SSE2 available
    .ok:
        
        ; Sets the SSE2 status flag for the next calls and process the buffer
        ; with the optimized version of memchr()
        mov DWORD [ ds:__SSE2Status ], 1
        jmp _memchr32_sse2
    
    ; SSE2 not available
    .fail:
        
        ; Sets the SSE2 status flag for the next calls and process the buffer
        ; with the less-optimized version of memchr()
        mov DWORD [ ds:__SSE2Status ], 0
        jmp _memchr32

;-------------------------------------------------------------------------------
; 32-bits SSE2 optimized memchr() function
; 
; void * _memchr32_sse2( const void * s, int c, size_t n );
; 
; Input registers:
;       
;       None - Arguments on stack
; 
; Return registers:
;       
;       - EAX:      A pointer to the first occurence of the character in the
;                   buffer, or 0 (NULL)
; 
; Killed registers:
;       
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;-------------------------------------------------------------------------------
_memchr32_sse2:
    
    ; Creates a stack frame, so we can save registers, making them available
    ; to use. Otherwise, only 3 registers are safe, which is not enough here
    push    ebp
    mov     ebp,        esp
    
    ; Saves EDI, ESI and EBX as we are going to use them
    push    edi
    push    esi
    push    ebx
    
    ; Gets the arguments from the stack
    mov     edi,        [ ebp +  8 ]
    mov     esi,        [ ebp + 12 ]
    mov     edx,        [ ebp + 16 ]
    
    ; Checks for a NULL buffer
    test        edi,    edi
    jz          .null
    
    ; Checks for a zero size
    test        edx,    edx
    jz          .null
    
    ; Ensures the character to search is 8 bits, and stores it in XMM0
    and         esi,    0xFF
    movd        xmm0,   esi
    
    ; Stores the original memory pointer in ESI
    mov         esi,    edi
    
    ; Aligns the memory pointer in EDI to a 16-byte boundary,
    ; so we can safelfy use the SSE instructions
    and         edi,    -16
    
    ; Prepares XMM0's value so we can search for the character using pcmpeqb
	punpcklbw   xmm0,   xmm0
	punpcklbw   xmm0,   xmm0
	pshufd      xmm0,   xmm0,   0
    
    ; Compares 16 bytes from EDI with the character to search (in XMM0)
    ; Equal bytes will be set to all 1s in XMM1, others to all 0s
	movdqa      xmm1,   [ edi ]
	pcmpeqb     xmm1,   xmm0
    
    ; Gets a mask in EBX with bits set to the most significant
    ; bits of each bytes from XMM1
	pmovmskb    ebx,    xmm1
    
    ; Gets the number of misaligned bytes in the original memory pointer (ESI)
    mov         ecx,    esi
    sub         ecx,    edi
    
    ; As we aligned the memory pointer in EDI to a 16-byte boundary,
    ; any preceding byte has to be ignored.
    ; So let's create a mask for those bytes in EAX, based on the number of
    ; misaligned bytes in the original memory pointer (ECX)
    xor         eax,    eax
    not         eax
    shl         eax,    cl
    
    ; As we may read more bytes, depending on the alignment, adjusts the buffer
    ; size in EDX
    add         edx,    ecx
    
    ; Masks the unwanted bytes in EBX, and checks if the character was found
    and         ebx,    eax
    jnz         .found
    
    .notfound:
        
        ; We've read 16 bytes - Advances the memory pointer and decrease
        ; the buffer size
        add         edi,    16
        sub         edx,    16
        
        ; Checks if we've reached the buffer size limit
        cmp         edx,    0
        jle         .null
        
        ; Checks if we can read 64 bytes at a time - if not, 16 bytes at a
        ; time will be read
        cmp         edx,    64
        jl .notfound_16
        
    .notfound_64:
        
        ; Reads the next 64 bytes from EDI into the XMM registers
        movdqa      xmm1,   [ edi ]
        movdqa      xmm2,   [ edi + 16 ]
        movdqa      xmm3,   [ edi + 32 ]
        movdqa      xmm4,   [ edi + 48 ]
        
        ; Compares the 64 bytes read with the character to search (in XMM0)
        ; Equal bytes will be set to all 1s in the XMM reisters, others to all 0s
        pcmpeqb     xmm1,   xmm0
        pcmpeqb     xmm2,   xmm0
        pcmpeqb     xmm3,   xmm0
        pcmpeqb     xmm4,   xmm0
        
        ; Gets a mask in EBX with bits set to the most significant
        ; bits of each bytes from XMM1
        pmovmskb    ebx,    xmm1
        
        ; Checks if a bit is set, meaning the character was found
        test        ebx,    ebx
        jnz         .found
        
        ; Checks the next 16 bytes - Advances the memory pointer and decrease
        ; the buffer size
        add         edi,    16
        sub         edx,    16
        
        ; Gets a mask in EBX with bits set to the most significant
        ; bits of each bytes from XMM2
        pmovmskb    ebx,    xmm2
        
        ; Checks if a bit is set, meaning the character was found
        test        ebx,    ebx
        jnz         .found
        
        ; Checks the next 16 bytes - Advances the memory pointer and decrease
        ; the buffer size
        add         edi,    16
        sub         edx,    16
        
        ; Gets a mask in EBX with bits set to the most significant
        ; bits of each bytes from XMM3
        pmovmskb    ebx,    xmm3
        
        ; Checks if a bit is set, meaning the character was found
        test        ebx,    ebx
        jnz         .found
        
        ; Checks the next 16 bytes - Advances the memory pointer and decrease
        ; the buffer size
        add         edi,    16
        sub         edx,    16
        
        ; Gets a mask in EBX with bits set to the most significant
        ; bits of each bytes from XMM4
        pmovmskb    ebx,    xmm4
        
        ; Checks if a bit is set, meaning the character was found
        test        ebx,    ebx
        jnz         .found
        
        ; Not found - Continues scanning
        jmp         .notfound
        
    .notfound_16:
        
        ; Compares 16 bytes from EDI with the character to search (in XMM0)
        ; Equal bytes will be set to all 1s in XMM1, others to all 0s
        movdqa      xmm1,   [ edi ]
        pcmpeqb     xmm1,   xmm0
        
        ; Gets a mask in EBX with bits set to the most significant
        ; bits of each bytes from XMM1
        pmovmskb    ebx,    xmm1
        
        ; Checks if a bit is set, meaning the character was found
        test        ebx,    ebx
        jnz         .found
        
        ; Not found - Continues scanning
        jmp         .notfound
        
    .found:
        
        ; Gets the index of the first bit set in EAX
        ; (index of the found character)
        bsf         eax,    ebx
        
        ; Substracts the character index from the remaining buffer size
        sub         edx,    eax
        
        ; Checks if we've reached the buffer size limit
        cmp         edx,    0
        jle         .null
        
        ; Adds the character index to the current value of the memory pointer,
        ; so we'll return the memory location to the found character
        add         eax,    edi
        
        ; Restores saved registers
        pop         ebx
        pop         esi
        pop         edi
        pop         ebp
        
        ret
        
    .null:
        
        ; Returns NULL
        xor         eax,    eax
        
        ; Restores saved registers
        pop         ebx
        pop         esi
        pop         edi
        pop         ebp
        
        ret

;-------------------------------------------------------------------------------
; 32-bits optimized memchr() function
; 
; void * _memchr64( const void * s, int c, size_t n );
; 
; Input registers:
;       
;       None - Arguments on stack
; 
; Return registers:
;       
;       - EAX:      A pointer to the first occurence of the character in the
;                   buffer, or 0 (NULL)
; 
; Killed registers:
;       
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;-------------------------------------------------------------------------------   
_memchr32:
    
    ; Creates a stack frame, so we can save registers, making them available
    ; to use. Otherwise, only 3 registers are safe, which is not enough here
    push    ebp
    mov     ebp,        esp
    
    ; Saves EDI, ESI and EBX as we are going to use them
    push    edi
    push    esi
    push    ebx
    
    ; Gets the arguments from the stack
    mov     edi,        [ ebp +  8 ]
    mov     esi,        [ ebp + 12 ]
    mov     edx,        [ ebp + 16 ]
    
    ; Checks for a NULL buffer
    test        edi,    edi
    jz          .null
    
    ; Ensures the character to search is 8 bits
    and         esi,    0xFF
    
    ; Stores the original memory pointer in ECX and EAX
    mov         ecx,    edi
    mov         eax,    edi
    
    ; Aligns the pointer in EAX to a 4-byte boundary
    and         eax,    -4
    
    ; Gets the number of misaligned bytes in the original memory pointer (ECX)
    sub         ecx,    eax
    
    ; Checks if we're aligned
    test        ecx,    ecx
    jnz         .notaligned
    
    .aligned:
        
        ; Saves EDX as we are going to alter it
        push        edx
        
        ; Creates the magic number allowing to test if a QWORD contains a
        ; byte - Thanks to Sean Eron Anderson:
        ; http://graphics.stanford.edu/~seander/bithacks.html
        xor         edx,    edx
        xor         eax,    eax
        not         eax
        mov         ecx,    255
        div         ecx
        mul         esi
        mov         ebx,    eax
        
        ; Restores EDX
        pop         edx
        
        ; Fill all 4 parts of ESI with the 8 bits to search
        mov         ecx,    esi
        shl         esi,    8
        or          esi,    ecx
        mov         ecx,    esi
        shl         esi,    16
        or          esi,    ecx
        
    .aligned_loop:
        
        ; Checks if we've reached the buffer size limit
        test        edx,    edx
        jz          .null
        
        ; Checks if we can read 8 bytes (otherwise, we'll read one byte at a time)
        cmp         edx,    4
        jb          .aligned_end
        
        ; Reads 4 bytes from RDI
        mov         eax,    [ edi ]
        
        ; Saves EDI and ESI as we are going to alter them
        push        edi
        push        esi
        
        ; Checks if the current QWORD contains the byte to search (magic number
        ; is in EBX) - Thanks to Sean Eron Anderson:
        ; http://graphics.stanford.edu/~seander/bithacks.html
        mov         edi,    eax
        xor         edi,    ebx
        mov         esi,    edi
        not         esi
        mov         ecx,    0x01010101
        sub         edi,    ecx
        mov         ecx,    0x80808080
        and         esi,    ecx
        test        edi,    esi
        jnz         .aligned_found
        
        ; Restores registers
        pop         esi
        pop         edi
        
        ; Not found, process next 4 bytes
        add         edi,    4
        sub         edx,    4
        jmp         .aligned_loop
        
    .aligned_found:
        
        ; Restores registers
        pop         esi
        pop         edi
        
        ; XOR the bytes read with ESI, so matching bytes will be zero
        xor         eax,    esi
        
        ; Checks if the character was found
        test        eax,    0x000000FF
        jz          .found
        
        ; Advances the memory pointer and decreases the buffer size
        add         edi,    1
        sub         edx,    1
        
        ; Checks if the character was found in the next byte
        test        eax,    0x0000FF00
        jz          .found
        
        ; Advances the memory pointer and decreases the buffer size
        add         edi,    1
        sub         edx,    1
        
        ; Checks if the character was found in the next byte
        test        eax,    0x00FF0000
        jz          .found
        
        ; Advances the memory pointer and decreases the buffer size
        add         edi,    1
        sub         edx,    1
        
        ; Checks if the character was found in the next byte
        test        eax,    0xFF000000
        jz          .found
        
        ; Not found - Continues scanning
        jmp         .aligned_loop
        
    .aligned_end:
        
        ; Ensures the character to search is 8 bits
        and         esi,    0xFF
        
        ; Number of remaining bytes
        mov         ecx,    edx
        
    .notaligned:
        
        ; Checks if we've reached the buffer size limit
        test        edx,    edx
        jz          .null
        
        ; Reads a byte from EDI
        xor         eax,    eax
        mov         al,     [ edi ]
        
        ; Checks if the character is found
        cmp         eax,    esi
        je          .found
        
        ; Advances the memory pointer and decreases the buffer size
        add         edi,    1
        sub         edx,    1
        
        ; Decreases the number of time we need to loop until we're aligned
        sub         ecx,    1
        
        ; Checks if we're aligned
        test        ecx,    ecx
        jz          .aligned
        
        ; Not aligned - Continues scanning
        jmp         .notaligned
        
    .found:
        
        ; Address of the found character
        mov         eax,    edi
        
        ; Restores saved registers
        pop         ebx
        pop         esi
        pop         edi
        pop         ebp
        
        ret
        
    .null:
        
        ; Returns NULL
        xor         eax,    eax
        
        ; Restores saved registers
        pop         ebx
        pop         esi
        pop         edi
        pop         ebp
        
        ret
