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
global _xeos_strcat

; External symbols
extern _strlen

;-------------------------------------------------------------------------------
; C99 - 32 bits strcat() function
; 
; char * strcat( char * restrict s1, const char * restrict s2 );
; 
; Input registers:
;       
;       None - Arguments on stack
; 
; Return registers:
;       
;       - EAX:      The destination string pointer
; 
; Killed registers:
;       
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;-------------------------------------------------------------------------------
_xeos_strcat:
    
    ; Checks the status of the SSE2 flag
    cmp DWORD [ ds:__SSE2Status ], 1
    
    ; SSE2 are available - Use the optimized version of strcat()
    je  _strcat32_sse2
    
    ; Checks the status of the SSE2 flag
    cmp DWORD [ ds:__SSE2Status ], 0
    
    ; SSE2 are not available - Use the less-optimized version of strcat()
    je  _strcat32
    
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
        ; with the optimized version of strcat()
        mov DWORD [ ds:__SSE2Status ], 1
        jmp _strcat32_sse2
    
    ; SSE2 not available
    .fail:
        
        ; Sets the SSE2 status flag for the next calls and process the buffer
        ; with the less-optimized version of strcat()
        mov DWORD [ ds:__SSE2Status ], 0
        jmp _strcat32

;-------------------------------------------------------------------------------
; 32-bits SSE2 optimized strcat() function
; 
; char * _strcat64_sse2( char * restrict s1, const char * restrict s2 );
; 
; Input registers:
;       
;       None - Arguments on stack
; 
; Return registers:
;       
;       - EAX:      The destination string pointer
; 
; Killed registers:
;       
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;-------------------------------------------------------------------------------
_strcat32_sse2:
    
    ; Creates a stack frame, so we can save registers, making them available
    ; to use. Otherwise, only 3 registers are safe, which is not enough here
    push    ebp
    mov     ebp,        esp
    
    ; Saves EDI and ESI as we are going to use them
    push    edi
    push    esi
    
    ; Gets the arguments from the stack
    mov     edi,        [ ebp +  8 ]
    mov     esi,        [ ebp + 12 ]
    
    ; Checks for a NULL destination string pointer
    test        edi,    edi
    jz          .ret
    
    ; Checks for a NULL source string pointer
    test        esi,    esi
    jz          .ret
    
    ; Ensures the stack is aligned on a 16-byte boundary as we'll call
    ; an external function
    and         esp,    -16
    
    ; Arguments for strlen()
    sub         esp,    12
    push        edi
    
    ; Gets the length of the destination string
    call        _strlen
    
    ; Resets the stack
    mov         esp,    ebp
    sub         esp,    8
    
    ; Advances the destination string pointer after its last character
    add         edi,    eax
    
    ; Gets the number of misaligned bytes in the original source pointer
    mov         eax,        esi
    mov         ecx,        esi
    and         ecx,        -16
    sub         eax,        ecx
    mov         ecx,        16
    sub         ecx,        eax
    
    ; Checks if the source pointer is already aligned
    cmp         ecx,        16
    jne         .source_notaligned
    
    .source_aligned:
        
        ; Gets the number of misaligned bytes in the original destination pointer
        mov         eax,        edi
        mov         ecx,        edi
        and         ecx,        -16
        sub         eax,        ecx
        mov         ecx,        16
        sub         ecx,        eax
        
        ; Checks if the destination pointer is also aligned
        cmp         ecx,        16
        jne         .dest_notaligned
        
        .source_dest_aligned:
            
            ; Reads 16 bytes from the source string
            movdqa      xmm1,       [ esi ]
            
            ; Resets XMM0
            pxor        xmm0,       xmm0
            
            ; Compares 16 bytes from XMM1 with 0 (in XMM0)
            ; Equal bytes will be set to all 1s in XMM0, others to all 0s
            pcmpeqb     xmm0,       xmm1
            
            ; Gets a mask in EDX with bits set to the most significant
            ; bits of each bytes from XMM0
            pmovmskb    edx,        xmm0
            
            ; Checks if a bit is set, meaning a zero byte was found
            test        edx,        edx
            jnz         .copy_end
            
            ; Writes 16 bytes into the destination string
            movdqa      [ edi ],    xmm1
            
            ; Advances the source and sestination string pointers
            add         edi,        16
            add         esi,        16
            
            ; Continues copying
            jmp         .source_dest_aligned
            
        .dest_notaligned:
            
            ; Reads 16 bytes from the source string
            movdqa      xmm1,       [ esi ]
            
            ; Resets XMM0
            pxor        xmm0,       xmm0
            
            ; Compares 16 bytes from XMM1 with 0 (in XMM0)
            ; Equal bytes will be set to all 1s in XMM0, others to all 0s
            pcmpeqb     xmm0,       xmm1
            
            ; Gets a mask in EDX with bits set to the most significant
            ; bits of each bytes from XMM0
            pmovmskb    edx,        xmm0
            
            ; Checks if a bit is set, meaning a zero byte was found
            test        edx,        edx
            jnz         .copy_end
            
            ; Writes 16 bytes into the destination string
            movdqu      [ edi ],    xmm1
            
            ; Advances the source and sestination string pointers
            add         edi,        16
            add         esi,        16
                
            jmp         .dest_notaligned
                    
    .copy_end:
        
        ; Reads a byte from the source buffer
        mov         al,         [ esi ]
        
        ; Writes a byte into the destination buffer
        mov         [ edi ],    al
        
        ; If zero, we reached the end of the destination string
        test        al,         al
        jz          .ret
        
        ; Advances the source and destination pointers and decreases
        ; the number of bytes to write
        add         edi,        1
        add         esi,        1
        sub         ecx,        1
        
        ; Not aligned - Continues writing single bytes
        jmp         .copy_end
        
    .source_notaligned:
        
        ; Reads a byte from the source buffer
        mov         al,         [ esi ]
        
        ; Writes a byte into the destination buffer
        mov         [ edi ],    al
        
        ; If zero, we reached the end of the destination string
        test        al,         al
        jz          .ret
        
        ; Advances the source and destination pointers and decreases
        ; the number of bytes to write
        add         edi,        1
        add         esi,        1
        sub         ecx,        1
        
        ; Checks if we're aligned
        test        ecx,        ecx
        jz          .source_aligned
        
        ; Not aligned - Continues writing single bytes
        jmp         .source_notaligned
    
    .ret:
        
        ; Returns the destination string pointer
        mov         eax,    [ ebp + 8 ]
        
        ; Restores saved registers
        pop         esi
        pop         edi
        pop         ebp
        
        ret

;-------------------------------------------------------------------------------
; 32-bits optimized strcat() function
; 
; char * _strcat64( char * restrict s1, const char * restrict s2 );
; 
; Input registers:
;       
;       None - Arguments on stack
; 
; Return registers:
;       
;       - EAX:      The destination string pointer
; 
; Killed registers:
;       
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;-------------------------------------------------------------------------------   
_strcat32:
    
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
    
    ; Checks for a NULL destination string pointer
    test        edi,    edi
    jz          .ret
    
    ; Checks for a NULL source string pointer
    test        esi,    esi
    jz          .ret
    
    ; Ensures the stack is aligned on a 16-byte boundary as we'll call
    ; an external function
    and         esp,    -16
    
    ; Arguments for strlen()
    sub         esp,    12
    push        edi
    
    ; Gets the length of the destination string
    call        _strlen
    
    ; Resets the stack
    mov         esp,    ebp
    sub         esp,    12
    
    ; Advances the destination string pointer after its last character
    add         edi,    eax
    
    ; Gets the number of misaligned bytes in the original source pointer
    mov         eax,        esi
    mov         ecx,        esi
    and         ecx,        -4
    sub         eax,        ecx
    mov         ecx,        4
    sub         ecx,        eax
    
    ; Checks if the source pointer is already aligned
    cmp         ecx,        4
    jne         .source_notaligned
    
    .source_aligned:
        
        ; IMPORTANT NOTE
        ; 
        ; At this point, the source pointer is aligned to a 4-byte boundary.
        ; The destination pointer might not be.
        ; As the x86/x86-64 architecture allows unaligned memory access,
        ; let's pretend we don't care about this.
        ; Of course, unaligned memory access might be slower, but the
        ; destination pointer should be aligned most of the time.
        ; Moreover, there's almost no performance penalty with recent CPUs
        ; from Intel (Sandy Bridge, Nehalem).
        
        ; Reads 4 bytes from the source string
        mov         eax,        [ esi ]
        
        ; Checks if a byte from ECX is zero - Thanks to Sean Eron Anderson:
        ; http://graphics.stanford.edu/~seander/bithacks.html
        mov         edx,    0x01010101
        mov         ebx,    eax
        sub         ebx,    edx
        
        mov         edx,    0x80808080
        mov         ecx,    eax
        not         ecx
        and         ecx,    edx
        
        and         ebx,    ecx
        test        ebx,    ebx
        jnz         .copy_end
        
        ; Writes 4 bytes into the destination string
        mov         [ edi ],    eax
        
        ; Advances the source and sestination string pointers
        add         edi,        4
        add         esi,        4
        
        ; Continues copying
        jmp         .source_aligned
                    
    .copy_end:
        
        ; Reads a byte from the source buffer
        mov         al,         [ esi ]
        
        ; Writes a byte into the destination buffer
        mov         [ edi ],    al
        
        ; If zero, we reached the end of the destination string
        test        al,         al
        jz          .ret
        
        ; Advances the source and destination pointers and decreases
        ; the number of bytes to write
        add         edi,        1
        add         esi,        1
        sub         ecx,        1
        
        ; Not aligned - Continues writing single bytes
        jmp         .copy_end
        
    .source_notaligned:
        
        ; Reads a byte from the source buffer
        mov         al,         [ esi ]
        
        ; Writes a byte into the destination buffer
        mov         [ edi ],    al
        
        ; If zero, we reached the end of the destination string
        test        al,         al
        jz          .ret
        
        ; Advances the source and destination pointers and decreases
        ; the number of bytes to write
        add         edi,        1
        add         esi,        1
        sub         ecx,        1
        
        ; Checks if we're aligned
        test        ecx,        ecx
        jz          .source_aligned
        
        ; Not aligned - Continues writing single bytes
        jmp         .source_notaligned
    
    .ret:
        
        ; Returns the destination string pointer
        mov         eax,    [ ebp + 8 ]
        
        ; Restores saved registers
        pop         ebx
        pop         esi
        pop         edi
        pop         ebp
        
        ret
