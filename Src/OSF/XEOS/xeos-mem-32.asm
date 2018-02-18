; XEOS-MEM-32.ASM
;
; We are in 32 bits mode
; @sobolev BITS    32

global _xeos_memchr ; Makes the entry point visible to the linker
global _xeos_strlen ; Makes the entry point visible to the linker
;extern _strlen ; External symbols

SECTION .text  align=16
;
; C99 - 32 bits memchr() function
; void * memchr( const void * s, int c, size_t n );
; Input registers:
;       None - Arguments on stack
; Return registers:
;       - EAX:      A pointer to the first occurence of the character in the buffer, or 0 (NULL)
; Killed registers:
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;
_xeos_memchr:
	jmp _memchr32_sse2 ; @debug
    cmp DWORD [__SSE2Status], 1 ; Checks the status of the SSE2 flag
    je  _memchr32_sse2 ; SSE2 are available - Use the optimized version of memchr()
    cmp DWORD [__SSE2Status], 0 ; Checks the status of the SSE2 flag
    je  _memchr32 ; SSE2 are not available - Use the less-optimized version of memchr()
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
        mov DWORD [__SSE2Status], 1
        jmp _memchr32_sse2
    ; SSE2 not available
    .fail:
        ; Sets the SSE2 status flag for the next calls and process the buffer
        ; with the less-optimized version of memchr()
        mov DWORD [__SSE2Status], 0
        jmp _memchr32
;-------------------------------------------------------------------------------
; 32-bits SSE2 optimized memchr() function
; void * _memchr32_sse2( const void * s, int c, size_t n );
; Input registers:
;       None - Arguments on stack
; Return registers:
;       - EAX:      A pointer to the first occurence of the character in the buffer, or 0 (NULL)
; Killed registers:
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;-------------------------------------------------------------------------------
align 16
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
    ; Gets a mask in EBX with bits set to the most significant bits of each bytes from XMM1
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
    add         edx,    ecx ; As we may read more bytes, depending on the alignment, adjusts the buffer size in EDX
    ; Masks the unwanted bytes in EBX, and checks if the character was found
    and         ebx,    eax
    jnz         .found
    .notfound:
        ; We've read 16 bytes - Advances the memory pointer and decrease the buffer size
        add         edi,    16
        sub         edx,    16
        ; Checks if we've reached the buffer size limit
        cmp         edx,    0
        jle         .null
        ; Checks if we can read 64 bytes at a time - if not, 16 bytes at a time will be read
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
        ; Checks the next 16 bytes - Advances the memory pointer and decrease the buffer size
        add         edi,    16
        sub         edx,    16
        ; Gets a mask in EBX with bits set to the most significant bits of each bytes from XMM2
        pmovmskb    ebx,    xmm2
        ; Checks if a bit is set, meaning the character was found
        test        ebx,    ebx
        jnz         .found
        ; Checks the next 16 bytes - Advances the memory pointer and decrease the buffer size
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
        jmp         .notfound ; Not found - Continues scanning
    .notfound_16:
        ; Compares 16 bytes from EDI with the character to search (in XMM0)
        ; Equal bytes will be set to all 1s in XMM1, others to all 0s
        movdqa      xmm1,   [ edi ]
        pcmpeqb     xmm1,   xmm0
        ; Gets a mask in EBX with bits set to the most significant bits of each bytes from XMM1
        pmovmskb    ebx,    xmm1
        ; Checks if a bit is set, meaning the character was found
        test        ebx,    ebx
        jnz         .found
        jmp         .notfound ; Not found - Continues scanning
    .found:
        ; Gets the index of the first bit set in EAX (index of the found character)
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
        xor         eax,    eax ; Returns NULL
        ; Restores saved registers
        pop         ebx
        pop         esi
        pop         edi
        pop         ebp
        ret
;
; 32-bits optimized memchr() function
; void * _memchr64( const void * s, int c, size_t n );
; Input registers:
;       None - Arguments on stack
; Return registers:
;       - EAX: A pointer to the first occurence of the character in the buffer, or 0 (NULL)
; Killed registers:
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;
align 16
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
    and         esi,    0xFF ; Ensures the character to search is 8 bits
    ; Stores the original memory pointer in ECX and EAX
    mov         ecx,    edi
    mov         eax,    edi
    and         eax,    -4  ; Aligns the pointer in EAX to a 4-byte boundary
    sub         ecx,    eax ; Gets the number of misaligned bytes in the original memory pointer (ECX)
    ; Checks if we're aligned
    test        ecx,    ecx
    jnz         .notaligned
    .aligned:
        push        edx ; Saves EDX as we are going to alter it
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
        pop         edx ; Restores EDX
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
        mov         eax,    [ edi ] ; Reads 4 bytes from RDI
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
        xor         eax,    esi ; XOR the bytes read with ESI, so matching bytes will be zero
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
        jmp         .aligned_loop ; Not found - Continues scanning
    .aligned_end:
        and         esi,    0xFF ; Ensures the character to search is 8 bits
        mov         ecx,    edx ; Number of remaining bytes
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
        sub         ecx,    1 ; Decreases the number of time we need to loop until we're aligned
        ; Checks if we're aligned
        test        ecx,    ecx
        jz          .aligned

        jmp         .notaligned ; Not aligned - Continues scanning
        
    .found:
        mov         eax,    edi ; Address of the found character
        ; Restores saved registers
        pop         ebx
        pop         esi
        pop         edi
        pop         ebp
        ret
    .null:
        xor         eax,    eax ; Returns NULL
        ; Restores saved registers
        pop         ebx
        pop         esi
        pop         edi
        pop         ebp
        ret
;
; C99 - 32 bits strlen() function
; size_t strlen( const char * s );
; Input registers:
;       None - Arguments on stack
; Return registers:
;       - EAX:      The length of the string
; Killed registers:
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;
_xeos_strlen:
	jmp _strlen32_sse2 ; @debug
    ;cmp DWORD [ds:__SSE2Status], 1 ; Checks the status of the SSE2 flag
	cmp DWORD [__SSE2Status], 1 ; Checks the status of the SSE2 flag
    je  _strlen32_sse2 ; SSE2 are available - Use the optimized version of strlen()
    cmp DWORD [__SSE2Status], 0 ; Checks the status of the SSE2 flag
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
        mov DWORD [__SSE2Status], 1
        jmp _strlen32_sse2
    ; SSE2 not available
    .fail:
        ; Sets the SSE2 status flag for the next calls and process the string
        ; with the less-optimized version of strlen()
        mov DWORD [__SSE2Status], 0
        jmp _strlen32
;
; 32-bits SSE2 optimized strlen() function
; size_t _strlen32_sse2( const char * s );
; Input registers:
;       None - Arguments on stack
; Return registers:
;       - EAX:      The length of the string
; Killed registers:
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;
align 16
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
;
; 32-bits optimized strlen() function
; size_t _strlen64( const char * s );
; Input registers:
;       None - Arguments on stack
; Return registers:
;       - EAX:      The length of the string
; Killed registers:
;       None - __cdecl (all except EAX, ECX, EDX must be preserved)
;
align 16
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

SECTION .data
align 16
;global __SSE2Status
; SSE2 status flag:
; 
;   -1: Unchecked
;    0: SSE2 not available
;    1: SSE2 available
__SSE2Status:   dd  -1
SECTION .text