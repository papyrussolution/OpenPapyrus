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
global _xeos_strchr

;-------------------------------------------------------------------------------
; C99 - 64 bits strchr() function
; 
; char * strchr( const char * s, int c );
; 
; Input registers:
;       
;       - RDI:      The string pointer
;       - RSI:      The character to search
; 
; Return registers:
;       
;       - RAX:      A pointer to the first occurence of the character in the
;                   string, or 0 (NULL)
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_xeos_strchr:
    
    ; Checks the status of the SSE2 flag
    cmp QWORD [ rel __SSE2Status ], 1
    
    ; SSE2 are available - Use the optimized version of strchr()
    je  _strchr64_sse2
    
    ; Checks the status of the SSE2 flag
    cmp QWORD [ rel __SSE2Status ], 0
    
    ; SSE2 are not available - Use the less-optimized version of strchr()
    je  _strchr64
    
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
        ; with the optimized version of strchr()
        mov QWORD [ rel __SSE2Status ], 1
        jmp _strchr64_sse2
    
    ; SSE2 not available
    .fail:
        
        ; Restores the values of RCX and RDX
        mov     rcx,    r8
        mov     rdx,    r9
        
        ; Sets the SSE2 status flag for the next calls and fills the buffer
        ; with the less-optimized version of strchr()
        mov QWORD [ rel __SSE2Status ], 0
        jmp _strchr64

;-------------------------------------------------------------------------------
; 64-bits SSE2 optimized strchr() function
; 
; char * _strchr64_sse2( const char * s, int c );
; 
; Input registers:
;       
;       - RDI:      The string pointer
;       - RSI:      The character to search
; 
; Return registers:
;       
;       - RAX:      A pointer to the first occurence of the character in the
;                   string, or 0 (NULL)
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_strchr64_sse2:
    
    ret

;-------------------------------------------------------------------------------
; 64-bits optimized strchr() function
; 
; char * _strchr64( const char * s, int c );
; 
; Input registers:
;       
;       - RDI:      The string pointer
;       - RSI:      The character to search
; 
; Return registers:
;       
;       - RAX:      A pointer to the first occurence of the character in the
;                   string, or 0 (NULL)
; 
; Killed registers:
;       
;       None - System V AMD64 ABI (RBP, RBX, R12-R15 must be preserved)
;-------------------------------------------------------------------------------
_strchr64:
    
    ret
