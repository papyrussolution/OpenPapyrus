/*******************************************************************************
 * XEOS - X86 Experimental Operating System
 * 
 * Copyright (c) 2010-2013, Jean-David Gadina - www.xs-labs.com
 * All rights reserved.
 * 
 * XEOS Software License - Version 1.0 - December 21, 2012
 * 
 * Permission is hereby granted, free of charge, to any person or organisation
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to deal in the Software, with or without
 * modification, without restriction, including without limitation the rights
 * to use, execute, display, copy, reproduce, transmit, publish, distribute,
 * modify, merge, prepare derivative works of the Software, and to permit
 * third-parties to whom the Software is furnished to do so, all subject to the
 * following conditions:
 * 
 *      1.  Redistributions of source code, in whole or in part, must retain the
 *          above copyright notice and this entire statement, including the
 *          above license grant, this restriction and the following disclaimer.
 * 
 *      2.  Redistributions in binary form must reproduce the above copyright
 *          notice and this entire statement, including the above license grant,
 *          this restriction and the following disclaimer in the documentation
 *          and/or other materials provided with the distribution, unless the
 *          Software is distributed by the copyright owner as a library.
 *          A "library" means a collection of software functions and/or data
 *          prepared so as to be conveniently linked with application programs
 *          (which use some of those functions and data) to form executables.
 * 
 *      3.  The Software, or any substancial portion of the Software shall not
 *          be combined, included, derived, or linked (statically or
 *          dynamically) with software or libraries licensed under the terms
 *          of any GNU software license, including, but not limited to, the GNU
 *          General Public License (GNU/GPL) or the GNU Lesser General Public
 *          License (GNU/LGPL).
 * 
 *      4.  All advertising materials mentioning features or use of this
 *          software must display an acknowledgement stating that the product
 *          includes software developed by the copyright owner.
 * 
 *      5.  Neither the name of the copyright owner nor the names of its
 *          contributors may be used to endorse or promote products derived from
 *          this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT OWNER AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, TITLE AND NON-INFRINGEMENT ARE DISCLAIMED.
 * 
 * IN NO EVENT SHALL THE COPYRIGHT OWNER, CONTRIBUTORS OR ANYONE DISTRIBUTING
 * THE SOFTWARE BE LIABLE FOR ANY CLAIM, DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN ACTION OF CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF OR IN CONNECTION WITH
 * THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/* $Id$ */

#include <stdlib.h>
#include <stdint.h>

void * xeos_memchr_c( const void * p, int c, size_t n );
void * xeos_memchr_c( const void * p, int c, size_t n )
{
    const unsigned char * cp;
    const unsigned long * lp;
    unsigned long         l;
    unsigned long         mask;
    unsigned long         magic;
    
    if( p == NULL || n == 0 )
    {
        return NULL;
    }
    
    cp = p;
    c &= 0xFF;
    
    /* Reads one byte at a time until the pointer is aligned to a long */
    while( n > 0 && ( ( ( uintptr_t )cp & ( uintptr_t )-sizeof( unsigned long ) ) < ( uintptr_t )cp ) )
    {
        if( *( cp++ ) == ( unsigned char )c )
        {
            return ( void * )( cp - 1 );
        }
        
        n--;
    }
    
    magic = ( ( ~0UL / 255 ) * ( unsigned char )c );
    lp    = ( const unsigned long * )( ( void * )cp );
    mask  = ( unsigned long )c << 24
          | ( unsigned long )c << 16
          | ( unsigned long )c << 8
          | ( unsigned long )c;
    
    #ifdef __LP64__
    mask = mask << 32 | mask;
    #endif
    
    /* Reads one long at a time */
    while( n > sizeof( unsigned long ) )
    {
        l  = *( lp++ );
        n -= sizeof( unsigned long );
        
        /*
         * Checks if a byte from "l" contains the character - Thanks to Sean
         * Eron Anderson:
         * http://graphics.stanford.edu/~seander/bithacks.html
         */
        {
            unsigned long l2;
            
            l2 = l ^ magic;
            
            #ifdef __LP64__
            if( !( ( l2 - 0x0101010101010101 ) & ( ~l2 & 0x8080808080808080 ) ) )
            #else
            if( !( ( l2 - 0x01010101 ) & ( ~l2 & 0x80808080 ) ) )
            #endif
            {
                continue;
            }
        }
        
        l = l ^ mask;
        
        if( ( l & 0x000000FF ) == 0 ) { return ( void * )( ( const char * )lp -   sizeof( unsigned long ) ); }
        if( ( l & 0x0000FF00 ) == 0 ) { return ( void * )( ( const char * )lp - ( sizeof( unsigned long ) - 1 ) ); }
        if( ( l & 0x00FF0000 ) == 0 ) { return ( void * )( ( const char * )lp - ( sizeof( unsigned long ) - 2 ) ); }
        if( ( l & 0xFF000000 ) == 0 ) { return ( void * )( ( const char * )lp - ( sizeof( unsigned long ) - 3 ) ); }
        
        #ifdef __LP64__
        
        if( ( l & 0x000000FF00000000 ) == 0 ) { return ( void * )( ( const char * )lp - ( sizeof( unsigned long ) - 4 ) ); }
        if( ( l & 0x0000FF0000000000 ) == 0 ) { return ( void * )( ( const char * )lp - ( sizeof( unsigned long ) - 5 ) ); }
        if( ( l & 0x00FF000000000000 ) == 0 ) { return ( void * )( ( const char * )lp - ( sizeof( unsigned long ) - 6 ) ); }
        if( ( l & 0xFF00000000000000 ) == 0 ) { return ( void * )( ( const char * )lp - ( sizeof( unsigned long ) - 7 ) ); }
        
        #endif
    }
    
    cp = ( unsigned char * )( ( void * )lp );
    
    /* Reads remaining bytes one by one */
    while( n-- )
    {
        if( *( cp++ ) == ( unsigned char )c )
        {
            return ( void * )( cp - 1 );
        }
    }
    
    return NULL;
}
