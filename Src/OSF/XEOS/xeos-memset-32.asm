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
global _xeos_memset

;-------------------------------------------------------------------------------
; C99 - 32 bits memset() function
; 
; void * memset( void * p, int c, size_t n );
; 
; Input registers:
;       
;       None - Arguments on stack
; 
; Return registers:
;       
;       - EAX:      The memory pointer
; 
; Killed registers:
;       
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;-------------------------------------------------------------------------------
_xeos_memset:
    
    ; Checks the status of the SSE2 flag
    cmp DWORD [ ds:__SSE2Status ], 1
    
    ; SSE2 are available - Use the optimized version of memset()
    je  _memset32_sse2
    
    ; Checks the status of the SSE2 flag
    cmp DWORD [ ds:__SSE2Status ], 0
    
    ; SSE2 are not available - Use the less-optimized version of memset()
    je  _memset32
    
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
        
        ; Sets the SSE2 status flag for the next calls and fills the buffer
        ; with the optimized version of memset()
        mov DWORD [ ds:__SSE2Status ], 1
        jmp _memset32_sse2
    
    ; SSE2 not available
    .fail:
        
        ; Sets the SSE2 status flag for the next calls and fills the buffer
        ; with the less-optimized version of memset()
        mov DWORD [ ds:__SSE2Status ], 0
        jmp _memset32

;-------------------------------------------------------------------------------
; 32-bits SSE2 optimized memset() function
; 
; void * _memset32_sse2( void * p, int c, size_t n );
; 
; Input registers:
;       
;       None - Arguments on stack
; 
; Return registers:
;       
;       - EAX:      The memory pointer
; 
; Killed registers:
;       
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;-------------------------------------------------------------------------------
_memset32_sse2:
    
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
    
    ; Saves the original memory pointer, as we'll need to return it
    mov     ebx,        edi
    
    ; Checks for a NULL memory pointer
    test    edi,        edi
    jz      .end
    
    ; Checks for a zero size
    test    edx,        edx
    jz      .end
    
    ; memset() is often called to set a buffer to zero, so prepare xmm0
    ; for such a case
    pxor    xmm0,       xmm0
    
    ; Checks if bytes needs to be set to zero
    test    esi,        esi
    jz      .padded
    
    .pad:
        
        ; A character other than zero will be used. Fills the ESI registers
        ; with four time the character value, so it will be easier to fill
        ; a XMM register with the character to set
        mov     ecx,        esi
        shl     esi,        8
        or      esi,        ecx
        mov     ecx,        esi
        shl     esi,        16
        or      esi,        ecx
        
        ; Fills the XMM0 register with the character to write, so we'll be
        ; able to write 16 bytes at a time
        movd    xmm0,       esi
        shufps  xmm0,       xmm0,   0x00
        
    .padded:
        
        ; Aligns the memory pointer in EDI to a 16-byte boundary,
        ; so we can safelfy use the SSE instructions
        and     edi,        -16
        
        ; Gets the number of misaligned bytes in the original memory pointer (EBX)
        mov     eax,        ebx
        sub     eax,        edi
        
        ; Checks if the pointer is already aligned
        test     eax,        eax
        jz      .aligned
        
        ; If not, computes the number of bytes to be written until we are
        ; aligned on the next 16-byte boundary
        mov     ecx,        16
        sub     ecx,        eax
        
    .notaligned:
        
        ; Restores the original pointer in EDI
        mov     edi,        ebx
        
        ; Stores the character to write in EAX, so we can access it as a byte
        ; using AL
        mov     eax,        esi
        
    .notaligned_loop:
        
        ; Checks if we have bytes to write
        test     edx,        edx
        jz      .end
        
        ; Writes a byte into the memory buffer
        mov     [ edi ],    al
        
        ; Advances the memory pointer
        inc     edi
        
        ; Decreases the number of bytes to write (EDX is the total, ECX is the
        ; number of bytes to write until we're aligned to a 16-byte boundary)
        dec     ecx
        dec     edx
        
        ; Checks if we are aligned to a 16-byte boundary. If so, the SSE
        ; instructions can be safely used
        test     ecx,        ecx
        jz      .aligned
        jmp     .notaligned_loop
        
    .aligned:
        
        ; Writes 128 bytes at a time, if possible
        cmp     edx,        128
        jge     .aligned_128
        
        ; Writes 64 bytes at a time, if possible
        cmp     edx,        64
        jge     .aligned_64
        
        ; Writes 32 bytes at a time, if possible
        cmp     edx,        32
        jge     .aligned_32
    
        ; Writes 16 bytes at a time, if possible
        cmp     edx,        16
        jge     .aligned_16
        
    .aligned_end:
        
        ; We're aligned on a 16-byte boundary, but there's not enough bytes
        ; to write in order to use the SSE instructions, so writes the
        ; remaining bytes one by one
        mov     ecx,            edx
        mov     eax,            esi
        jmp     .notaligned_loop
        
    .aligned_128:
        
        ; Writes 128 bytes into the memory buffer
        movdqa  [ edi       ],  xmm0
        movdqa  [ edi +  16 ],  xmm0
        movdqa  [ edi +  32 ],  xmm0
        movdqa  [ edi +  48 ],  xmm0
        movdqa  [ edi +  64 ],  xmm0
        movdqa  [ edi +  80 ],  xmm0
        movdqa  [ edi +  96 ],  xmm0
        movdqa  [ edi + 112 ],  xmm0
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     edi,            128
        sub     edx,            128
        
        ; Next bytes - Writes 128 bytes at a time, if possible
        cmp     edx,            128
        jge     .aligned_128
        
        ; Next bytes - Writes 64 bytes at a time, if possible
        cmp     edx,            64
        jge     .aligned_64
        
        ; Next bytes - Writes 32 bytes at a time, if possible
        cmp     edx,            32
        jge     .aligned_32
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     edx,            16
        jge     .aligned_16
        
        ; Checks if we're done writing bytes
        test    edx,            edx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
        
    .aligned_64:
        
        ; Writes 64 bytes into the memory buffer
        movdqa  [ edi      ],   xmm0
        movdqa  [ edi + 16 ],   xmm0
        movdqa  [ edi + 32 ],   xmm0
        movdqa  [ edi + 48 ],   xmm0
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     edi,            64
        sub     edx,            64
        
        ; Next bytes - Writes 64 bytes at a time, if possible
        cmp     edx,            64
        jge     .aligned_64
        
        ; Next bytes - Writes 32 bytes at a time, if possible
        cmp     edx,            32
        jge     .aligned_32
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     edx,            16
        jge     .aligned_16
        
        ; Checks if we're done writing bytes
        test    edx,            edx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
        
    .aligned_32:
        
        ; Writes 32 bytes into the memory buffer
        movdqa  [ edi      ],   xmm0
        movdqa  [ edi + 16 ],   xmm0
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     edi,            32
        sub     edx,            32
        
        ; Next bytes - Writes 32 bytes at a time, if possible
        cmp     edx,            32
        jge     .aligned_32
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     edx,            16
        jge     .aligned_16
        
        ; Checks if we're done writing bytes
        test    edx,            edx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
        
    .aligned_16:
        
        ; Writes 16 bytes into the memory buffer
        movdqa  [ edi ],        xmm0
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     edi,            16
        sub     edx,            16
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     edx,            16
        jge     .aligned
        
        ; Checks if we're done writing bytes
        test    edx,            edx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
    
    .end:
        
        ; Return value - Gets the original pointer
        mov     eax,            ebx
        
        ; Restores saved registers
        pop     ebx
        pop     esi
        pop     edi
        pop     ebp
        
        ret

;-------------------------------------------------------------------------------
; 32-bits optimized memset() function
; 
; void * _memset32( void * p, int c, size_t n );
; 
; Input registers:
;       
;       None - Arguments on stack
; 
; Return registers:
;       
;       - EAX:      The memory pointer
; 
; Killed registers:
;       
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;-------------------------------------------------------------------------------
_memset32:
    
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
    
    ; Saves the original memory pointer, as we'll need to return it
    mov     ebx,        edi
    
    ; Checks for a NULL memory pointer
    test    edi,        edi
    jz      .end
    
    ; Checks for a zero size
    test    edx,        edx
    jz      .end
    
    ; Checks if bytes needs to be set to zero
    test    esi,        esi
    jz      .padded
    
    .pad:
        
        ; A character other than zero will be used. Fills the ESI registers
        ; with four time the character value, so will be able to write 4 bytes
        ; at a time
        mov     ecx,        esi
        shl     esi,        8
        or      esi,        ecx
        mov     ecx,        esi
        shl     esi,        16
        or      esi,        ecx
        
    .padded:
        
        ; Aligns the memory pointer in EDI to a 4-byte boundary
        and     edi,        -4
        
        ; Gets the number of misaligned bytes in the original memory pointer (EBX)
        mov     eax,        ebx
        sub     eax,        edi
        
        ; Checks if the pointer is already aligned
        test    eax,        eax
        jz      .aligned
        
        ; If not, computes the number of bytes to be written until we are
        ; aligned on the next 4-byte boundary
        mov     ecx,        4
        sub     ecx,        eax
        
    .notaligned:
        
        ; Restores the original pointer in EDI
        mov     edi,        ebx
        
        ; Stores the character to write in EAX, so we can access it as a byte
        ; using AL
        mov     eax,        esi
        
    .notaligned_loop:
        
        ; Checks if we have bytes to write
        test    edx,        edx
        jz      .end
        
        ; Writes a byte into the memory buffer
        mov     [ edi ],    al
        
        ; Advances the memory pointer
        inc     edi
        
        ; Decreases the number of bytes to write (EDX is the total, ECX is the
        ; number of bytes to write until we're aligned to a 4-byte boundary)
        dec     ecx
        dec     edx
        
        ; Checks if we are aligned to a 4-byte boundary. If so, we'll be able
        ; to write more than one byte at a time
        test    ecx,        ecx
        jz      .aligned
        jmp     .notaligned_loop
        
    .aligned:
        
        ; Writes 128 bytes at a time, if possible
        cmp     edx,        128
        jge     .aligned_128
        
        ; Writes 54 bytes at a time, if possible
        cmp     edx,        64
        jge     .aligned_64
        
        ; Writes 32 bytes at a time, if possible
        cmp     edx,        32
        jge     .aligned_32
    
        ; Writes 16 bytes at a time, if possible
        cmp     edx,        16
        jge     .aligned_16
        
    .aligned_end:
        
        ; We're aligned on a 4-byte boundary, but there's not enough bytes
        ; to write, so writes the remaining bytes one by one
        mov     ecx,            edx
        mov     eax,            esi
        jmp     .notaligned_loop
        
    .aligned_128:
        
        ; Writes 128 bytes into the memory buffer
        mov  [ edi       ],  esi
        mov  [ edi +   4 ],  esi
        mov  [ edi +   8 ],  esi
        mov  [ edi +  12 ],  esi
        mov  [ edi +  16 ],  esi
        mov  [ edi +  20 ],  esi
        mov  [ edi +  24 ],  esi
        mov  [ edi +  28 ],  esi
        mov  [ edi +  32 ],  esi
        mov  [ edi +  36 ],  esi
        mov  [ edi +  40 ],  esi
        mov  [ edi +  44 ],  esi
        mov  [ edi +  48 ],  esi
        mov  [ edi +  52 ],  esi
        mov  [ edi +  56 ],  esi
        mov  [ edi +  60 ],  esi
        mov  [ edi +  64 ],  esi
        mov  [ edi +  68 ],  esi
        mov  [ edi +  72 ],  esi
        mov  [ edi +  76 ],  esi
        mov  [ edi +  80 ],  esi
        mov  [ edi +  84 ],  esi
        mov  [ edi +  88 ],  esi
        mov  [ edi +  92 ],  esi
        mov  [ edi +  96 ],  esi
        mov  [ edi + 100 ],  esi
        mov  [ edi + 104 ],  esi
        mov  [ edi + 108 ],  esi
        mov  [ edi + 112 ],  esi
        mov  [ edi + 116 ],  esi
        mov  [ edi + 120 ],  esi
        mov  [ edi + 124 ],  esi
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     edi,            128
        sub     edx,            128
        
        ; Next bytes - Writes 128 bytes at a time, if possible
        cmp     edx,            128
        jge     .aligned_128
        
        ; Next bytes - Writes 64 bytes at a time, if possible
        cmp     edx,            64
        jge     .aligned_64
        
        ; Next bytes - Writes 32 bytes at a time, if possible
        cmp     edx,            32
        jge     .aligned_32
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     edx,            16
        jge     .aligned_16
        
        ; Checks if we're done writing bytes
        test    edx,            edx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
        
    .aligned_64:
        
        ; Writes 64 bytes into the memory buffer
        mov  [ edi       ],  esi
        mov  [ edi +   4 ],  esi
        mov  [ edi +   8 ],  esi
        mov  [ edi +  12 ],  esi
        mov  [ edi +  16 ],  esi
        mov  [ edi +  20 ],  esi
        mov  [ edi +  24 ],  esi
        mov  [ edi +  28 ],  esi
        mov  [ edi +  32 ],  esi
        mov  [ edi +  36 ],  esi
        mov  [ edi +  40 ],  esi
        mov  [ edi +  44 ],  esi
        mov  [ edi +  48 ],  esi
        mov  [ edi +  52 ],  esi
        mov  [ edi +  56 ],  esi
        mov  [ edi +  60 ],  esi
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     edi,            64
        sub     edx,            64
        
        ; Next bytes - Writes 64 bytes at a time, if possible
        cmp     edx,            64
        jge     .aligned_64
        
        ; Next bytes - Writes 32 bytes at a time, if possible
        cmp     edx,            32
        jge     .aligned_32
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     edx,            16
        jge     .aligned_16
        
        ; Checks if we're done writing bytes
        test    edx,            edx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
        
    .aligned_32:
        
        ; Writes 32 bytes into the memory buffer
        mov  [ edi       ],  esi
        mov  [ edi +   4 ],  esi
        mov  [ edi +   8 ],  esi
        mov  [ edi +  12 ],  esi
        mov  [ edi +  16 ],  esi
        mov  [ edi +  20 ],  esi
        mov  [ edi +  24 ],  esi
        mov  [ edi +  28 ],  esi
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     edi,            32
        sub     edx,            32
        
        ; Next bytes - Writes 32 bytes at a time, if possible
        cmp     edx,            32
        jge     .aligned_32
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     edx,            16
        jge     .aligned_16
        
        ; Checks if we're done writing bytes
        test    edx,            edx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
        
    .aligned_16:
        
        ; Writes 16 bytes into the memory buffer
        mov  [ edi       ],  esi
        mov  [ edi +   4 ],  esi
        mov  [ edi +   8 ],  esi
        mov  [ edi +  12 ],  esi
        
        ; Advances the memory pointer and decreases the number of bytes to write
        add     edi,            16
        sub     edx,            16
        
        ; Next bytes - Writes 16 bytes at a time, if possible
        cmp     edx,            16
        jge     .aligned
        
        ; Checks if we're done writing bytes
        test    edx,            edx
        jz      .end
        
        ; If we still have byte to write, writes them one by one
        jmp     .aligned_end
    
    .end:
        
        ; Return value - Gets the original pointer
        mov     eax,            ebx
        
        ; Restores saved registers
        pop     ebx
        pop     esi
        pop     edi
        pop     ebp
        
        ret
