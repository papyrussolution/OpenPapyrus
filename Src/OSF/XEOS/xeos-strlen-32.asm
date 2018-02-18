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
extern __SSE2Status

section .text align=16

; Makes the entry point visible to the linker
global _xeos_strlen

;-------------------------------------------------------------------------------
; C99 - 32 bits strlen() function
; size_t strlen( const char * s );
; Input registers:
;       None - Arguments on stack
; Return registers:
;       - EAX:      The length of the string
; Killed registers:
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;-------------------------------------------------------------------------------
_xeos_strlen:
    cmp DWORD [ ds:__SSE2Status ], 1 ; Checks the status of the SSE2 flag
    je  _strlen32_sse2 ; SSE2 are available - Use the optimized version of strlen()
    cmp DWORD [ ds:__SSE2Status ], 0 ; Checks the status of the SSE2 flag
    je  _strlen32 ; SSE2 are not available - Use the less-optimized version of strlen()
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
        ; Sets the SSE2 status flag for the next calls and process the string
        ; with the optimized version of strlen()
        mov DWORD [ ds:__SSE2Status ], 1
        jmp _strlen32_sse2
    ; SSE2 not available
    .fail:
        ; Sets the SSE2 status flag for the next calls and process the string
        ; with the less-optimized version of strlen()
        mov DWORD [ ds:__SSE2Status ], 0
        jmp _strlen32

;-------------------------------------------------------------------------------
; 32-bits SSE2 optimized strlen() function
; size_t _strlen32_sse2( const char * s );
; Input registers:
;       None - Arguments on stack
; Return registers:
;       - EAX:      The length of the string
; Killed registers:
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;-------------------------------------------------------------------------------
_strlen32_sse2:
    mov         edx,    [ esp + 4 ] ; Gets the string pointer in EDX from the stack
    ; Checks for a NULL string
    test        edx,    edx
    jz          .null
    mov         eax,    edx ; Stores the original string pointer in EAX
    ; Aligns the string pointer in EDX to a 16-byte boundary,
    ; so we can safelfy use the SSE instructions
    and         edx,    -16
    pxor        xmm0,   xmm0 ; Resets XMM0
    ; Compares 16 bytes from EDX with 0 (in XMM0)
    ; Equal bytes will be set to all 1s in XMM0, others to all 0s
    pcmpeqb     xmm0,   [ edx ]
    ; Gets the number of misaligned bytes in the original string pointer (EAX)
    mov         ecx,    eax
    sub         ecx,    edx
    pmovmskb    edx,    xmm0 ; Gets a mask in RDX with bits set to the most significant bits of each bytes from XMM0
    ; As we aligned the string pointer to a 16-byte boundary,
    ; any preceding byte has to be ignored.
    ; So let's create a mask for those bytes in EAX, based on the number of
    ; misaligned bytes in the original string pointer (ECX)
    xor         eax,    eax
    not         eax
    shl         eax,    cl
    mov         ecx,    [ esp + 4 ] ; Gets the string pointer in ECX from the stack
    ; Aligns the string pointer in ECX to a 16-byte boundary,
    ; so we can safelfy use the SSE instructions
    and         ecx,    -16
    ; Masks the unwanted bytes in EDX, and checks if a 0 byte was found
    and         edx,    eax
    jnz         .found
    .notfound:
        add         ecx,    16 ; Next 16 bytes from ECX will be checked
        pxor        xmm0,   xmm0 ; Resets XMM0
        ; Compares 16 bytes from ECX with 0 (in XMM0)
        ; Equal bytes will be set to all 1s in XMM0, others to all 0s
        pcmpeqb     xmm0,   [ ecx ]
        pmovmskb    edx,    xmm0 ; Gets a mask in EDX with bits set to the most significant bits of each bytes from XMM0
        ; Checks if a bit is set, meaning a zero byte was found
        test        edx,    edx
        jz          .notfound
    .found:
        bsf         eax,    edx ; Gets the index of the first bit set in EAX (index of the found 0-byte)
        mov         edx,    [ esp + 4 ] ; Gets the string pointer in EDX from the stack
        ; Computes the difference between the original string pointer
        ; and the current value of RCX, and adjusts EAX so it now
        ; contains the string length
        sub         edx,    ecx
        sub         eax,    edx
        ret
    .null:
        xor         eax,    eax ; NULL string - Returns 0
        ret

;-------------------------------------------------------------------------------
; 32-bits optimized strlen() function
; size_t _strlen64( const char * s );
; Input registers:
;       None - Arguments on stack
; Return registers:
;       - EAX:      The length of the string
; Killed registers:
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;-------------------------------------------------------------------------------   
_strlen32:
    ; Creates a stack frame, so we can save registers, making them available
    ; to use. Otherwise, only 3 registers are safe, which is not enough here
    push    ebp
    mov     ebp,        esp
    ; Saves EDI, ESI and EBX as we are going to use them
    push    edi
    push    esi
    push    ebx
    mov         ecx,    [ ebp + 8 ] ; Gets the original string pointer in ECX from the stack
    ; Checks for a NULL string
    test        ecx,    ecx
    jz          .null
    mov         eax,    ecx ; Stores the original string pointer in EAX
    and         ecx,    -4 ; Aligns the string pointer in ECX to a 4-byte boundary
    sub         eax,    ecx ; Gets the number of misaligned bytes in the original string pointer
    ; Multiples by eight to get the number of misaligned bits (in EAX)
    mov         edx,    4
    mul         edx
    xchg        edx,    ecx ; Saves the string pointer in EDX, as we are going to use ECX
    ; As we aligned the string pointer in EDX to a 4-byte boundary,
    ; any preceding byte has to be ignored.
    ; So let's create a mask for those bytes in EAX, based on the number of
    ; misaligned bytes in the original string pointer (ECX)
    xor         ecx,    ecx
    not         ecx
    xchg        eax,    ecx
    shl         eax,    cl
    not         eax
    mov         ecx,    [ edx ] ; Reads 4 bytes from the string
    or          ecx,    eax ; Masks the unwanted bytes in ECX
    xor         eax,    eax ; Resets EAX
    ; Loop that scans an aligned quad-word for 0
    ; This could have been implemented with an assembly loop, to avoid
    ; repeating the test for each byte, but it is much faster this way.
    .scan:
        ; Checks if a byte from ECX is zero - Thanks to Sean Eron Anderson:
        ; http://graphics.stanford.edu/~seander/bithacks.html
        mov         edi,    0x01010101
        mov         ebx,    ecx
        sub         ebx,    edi
        mov         edi,    0x80808080
        mov         esi,    ecx
        not         esi
        and         esi,    edi
        and         ebx,    esi
        test        ebx,    ebx
        jnz         .test
        add         eax,    4 ; Increase EAX
        ; Reads the next 4 bytes from the string
        add         edx,    4
        mov         ecx,    [ edx ]
        jmp         .scan ; Scans the next bytes
    .test:
        ; Test byte 1 for 0
        test        ecx,    0xFF
        jz          .found
        ; Increase EAX and prepare next byte
        inc         eax
        shr         ecx,    8
        ; Test byte 2 for 0
        test        ecx,    0xFF
        jz          .found
        ; Increase EAX and prepare next byte
        inc         eax
        shr         ecx,    8
        ; Test byte 3 for 0
        test        ecx,    0xFF
        jz          .found
        ; Increase EAX and prepare next byte
        inc         eax
        shr         ecx,    8
        ; Test byte 4 for 0
        test        ecx,    0xFF
        jz          .found
        inc         eax ; Increase EAX
        ; Reads the next 4 bytes from the string
        add         edx,    4
        mov         ecx,    [ edx ]
        jmp         .scan ; Scans the bytes that have been read
    .found:
        mov         ecx,    [ esp + 4 ] ; Gets the original string pointer in ECX from the stack
        mov         edx,    ecx ; Stores the original string pointer in EDX
        and         ecx,    -4  ; Aligns the string pointer in ECX to a 4-byte boundary
        sub         edx,    ecx ; Gets the number of misaligned bytes in the original string pointer
        sub         eax,    edx ; Substract the number of preceding bytes needed to align the string pointer to a 4-byte boundary
        ; Restores saved registers
        pop     ebx
        pop     esi
        pop     edi
        pop     ebp
        
        ret
    .null:
        xor         eax,    eax ; NULL string - Returns 0
        ret
