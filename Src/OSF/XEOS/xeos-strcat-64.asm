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
global _xeos_strcat

; External symbols
extern _strlen

;-------------------------------------------------------------------------------
; C99 - 64 bits strcat() function
; 
; char * strcat( char * restrict s1, const char * restrict s2 );
; 
; Input registers:
;       
;       - RDI:      The destination string pointer
;       - RSI:      The source string pointer
; 
; Return registers:
;       
;       - RAX:      The destination string pointer
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_xeos_strcat:

.start:
    
    ; Checks the status of the SSE2 flag
    cmp QWORD [ rel __SSE2Status ], 1
    
    ; SSE2 are available - Use the optimized version of strcat()
    je  _strcat64_sse2
    
    ; Checks the status of the SSE2 flag
    cmp QWORD [ rel __SSE2Status ], 0
    
    ; SSE2 are not available - Use the less-optimized version of strcat()
    je  _strcat64
    
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
        ; with the optimized version of strcat()
        mov QWORD [ rel __SSE2Status ], 1
        jmp _strcat64_sse2
    
    ; SSE2 not available
    .fail:
        
        ; Restores the values of RCX and RDX
        mov     rcx,    r8
        mov     rdx,    r9
        
        ; Sets the SSE2 status flag for the next calls and process the buffer
        ; with the less-optimized version of strcat()
        mov QWORD [ rel __SSE2Status ], 0
        jmp _strcat64
            
;-------------------------------------------------------------------------------
; 64-bits SSE2 optimized strcat() function
; 
; char * _strcat64_sse2( char * restrict s1, const char * restrict s2 );
; 
; Input registers:
;       
;       - RDI:      The destination string pointer
;       - RSI:      The source string pointer
; 
; Return registers:
;       
;       - RAX:      The destination string pointer
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_strcat64_sse2:
    
    ; Creates a stack frame, so we can safely call strlen().
    push        rbp
    mov         rbp,    rsp
    
    ; Checks for a NULL destination string pointer
    test        rdi,    rdi
    jz          .ret
    
    ; Checks for a NULL source string pointer
    test        rsi,    rsi
    jz          .ret
    
    ; Stores the original destination string pointer in R8
    mov         r8,     rdi
    
    ; Saves input registers
    push        rdi
    push        rsi
    
    ; Gets the length of the destination string
    call        _strlen
    
    ; Restores inputs registers
    pop         rsi
    pop         rdi
    
    ; Advances the destination string pointer after its last character
    add         rdi,    rax
    
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
            
            ; Reads 16 bytes from the source string
            movdqa      xmm1,       [ rsi ]
            
            ; Resets XMM0
            pxor        xmm0,       xmm0
            
            ; Compares 16 bytes from XMM1 with 0 (in XMM0)
            ; Equal bytes will be set to all 1s in XMM0, others to all 0s
            pcmpeqb     xmm0,       xmm1
            
            ; Gets a mask in RDX with bits set to the most significant
            ; bits of each bytes from XMM0
            pmovmskb    rdx,        xmm0
            
            ; Checks if a bit is set, meaning a zero byte was found
            test        rdx,        rdx
            jnz         .copy_end
            
            ; Writes 16 bytes into the destination string
            movdqa      [ rdi ],    xmm1
            
            ; Advances the source and sestination string pointers
            add         rdi,        16
            add         rsi,        16
            
            ; Continues copying
            jmp         .source_dest_aligned
            
        .dest_notaligned:
            
            ; Reads 16 bytes from the source string
            movdqa      xmm1,       [ rsi ]
            
            ; Resets XMM0
            pxor        xmm0,       xmm0
            
            ; Compares 16 bytes from XMM1 with 0 (in XMM0)
            ; Equal bytes will be set to all 1s in XMM0, others to all 0s
            pcmpeqb     xmm0,       xmm1
            
            ; Gets a mask in RDX with bits set to the most significant
            ; bits of each bytes from XMM0
            pmovmskb    rdx,        xmm0
            
            ; Checks if a bit is set, meaning a zero byte was found
            test        rdx,        rdx
            jnz         .copy_end
            
            ; Writes 16 bytes into the destination string
            movdqu      [ rdi ],    xmm1
            
            ; Advances the source and sestination string pointers
            add         rdi,        16
            add         rsi,        16
                
            jmp         .dest_notaligned
                    
    .copy_end:
        
        ; Reads a byte from the source buffer
        mov         al,         [ rsi ]
        
        ; Writes a byte into the destination buffer
        mov         [ rdi ],    al
        
        ; If zero, we reached the end of the destination string
        test        al,         al
        jz          .ret
        
        ; Advances the source and destination pointers and decreases
        ; the number of bytes to write
        add         rdi,        1
        add         rsi,        1
        sub         rcx,        1
        
        ; Not aligned - Continues writing single bytes
        jmp         .copy_end
        
    .source_notaligned:
        
        ; Reads a byte from the source buffer
        mov         al,         [ rsi ]
        
        ; Writes a byte into the destination buffer
        mov         [ rdi ],    al
        
        ; If zero, we reached the end of the destination string
        test        al,         al
        jz          .ret
        
        ; Advances the source and destination pointers and decreases
        ; the number of bytes to write
        add         rdi,        1
        add         rsi,        1
        sub         rcx,        1
        
        ; Checks if we're aligned
        test        rcx,        rcx
        jz          .source_aligned
        
        ; Not aligned - Continues writing single bytes
        jmp         .source_notaligned
    
    .ret:
        
        ; Returns the destination string pointer
        mov         rax,    r8
        
        ; Restores saved registers
        pop         rbp
        
        ret
        
;-------------------------------------------------------------------------------
; 64-bits optimized strcat() function
; 
; char * _strcat64( char * restrict s1, const char * restrict s2 );
; 
; Input registers:
;       
;       - RDI:      The destination string pointer
;       - RSI:      The source string pointer
; 
; Return registers:
;       
;       - RAX:      The destination string pointer
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_strcat64:
    
    ; Creates a stack frame, so we can safely call strlen().
    push        rbp
    mov         rbp,    rsp
    
    ; Checks for a NULL destination string pointer
    test        rdi,    rdi
    jz          .ret
    
    ; Checks for a NULL source string pointer
    test        rsi,    rsi
    jz          .ret
    
    ; Stores the original destination string pointer in R8
    mov         r8,     rdi
    
    ; Saves input registers
    push        rdi
    push        rsi
    
    ; Gets the length of the destination string
    call        _strlen
    
    ; Restores inputs registers
    pop         rsi
    pop         rdi
    
    ; Advances the destination string pointer after its last character
    add         rdi,    rax
    
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
        
        ; Reads 8 bytes from the source string
        mov         rax,        [ rsi ]
        
        ; Checks if a byte from RDX is zero - Thanks to Sean Eron Anderson:
        ; http://graphics.stanford.edu/~seander/bithacks.html
        mov         r10,        0x0101010101010101
        mov         r11,        0x8080808080808080
        mov         rcx,        rax
        mov         r9,         rax
        sub         rcx,        r10
        not         r9
        and         r9,         r11
        and         rcx,        r9
        test        rcx,        rcx
        jnz         .copy_end
        
        ; Writes 8 bytes into the destination string
        mov         [ rdi ],    rax
        
        ; Advances the source and sestination string pointers
        add         rdi,        8
        add         rsi,        8
        
        ; Continues copying
        jmp         .source_aligned
        
    .copy_end:
        
        ; Reads a byte from the source buffer
        mov         al,         [ rsi ]
        
        ; Writes a byte into the destination buffer
        mov         [ rdi ],    al
        
        ; If zero, we reached the end of the destination string
        test        al,         al
        jz          .ret
        
        ; Advances the source and destination pointers and decreases
        ; the number of bytes to write
        add         rdi,        1
        add         rsi,        1
        sub         rcx,        1
        
        ; Not aligned - Continues writing single bytes
        jmp         .copy_end
        
    .source_notaligned:
        
        ; Reads a byte from the source buffer
        mov         al,         [ rsi ]
        
        ; Writes a byte into the destination buffer
        mov         [ rdi ],    al
        
        ; If zero, we reached the end of the destination string
        test        al,         al
        jz          .ret
        
        ; Advances the source and destination pointers and decreases
        ; the number of bytes to write
        add         rdi,        1
        add         rsi,        1
        sub         rcx,        1
        
        ; Checks if we're aligned
        test        rcx,        rcx
        jz          .source_aligned
        
        ; Not aligned - Continues writing single bytes
        jmp         .source_notaligned
    
    .ret:
        
        ; Returns the destination string pointer
        mov         rax,    r8
        
        ; Restores saved registers
        pop         rbp
        
        ret
