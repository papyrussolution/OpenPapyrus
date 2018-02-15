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

void * xeos_memset_c( void * p, int c, size_t n );
void * xeos_memset_c( void * p, int c, size_t n )
{
    unsigned char * cp;
    unsigned long * lp;
    unsigned long   l;
    
    if( p == NULL || n == 0 )
    {
        return p;
    }
    
    cp = p;
    c &= 0xFF;
    
    /* Copies one byte at a time until the pointer is aligned to a long */
    while( n > 0 && ( ( ( uintptr_t )cp & ( uintptr_t )-sizeof( unsigned long ) ) < ( uintptr_t )cp ) )
    {
        *( cp++ ) = ( unsigned char )c;
        
        n--;
    }
    
    lp = ( unsigned long * )( ( void * )cp );
    l  = ( unsigned long )c << 24
       | ( unsigned long )c << 16
       | ( unsigned long )c << 8
       | ( unsigned long )c;
    
    #ifdef __LP64__
    l = l << 32 | l;
    #endif
    
    /* Loop unroll - writes 16 longs at a time */
    while( n > sizeof( unsigned long ) * 16 )
    {
        lp[  0 ] = l;
        lp[  1 ] = l;
        lp[  2 ] = l;
        lp[  3 ] = l;
        lp[  4 ] = l;
        lp[  5 ] = l;
        lp[  6 ] = l;
        lp[  7 ] = l;
        lp[  8 ] = l;
        lp[  9 ] = l;
        lp[ 10 ] = l;
        lp[ 11 ] = l;
        lp[ 12 ] = l;
        lp[ 13 ] = l;
        lp[ 14 ] = l;
        lp[ 15 ] = l;
        n        -= sizeof( unsigned long ) * 16;
        lp       += 16;
    }
    
    /* Loop unroll - writes 8 longs at a time */
    while( n > sizeof( unsigned long ) * 8 )
    {
        lp[ 0 ] = l;
        lp[ 1 ] = l;
        lp[ 2 ] = l;
        lp[ 3 ] = l;
        lp[ 4 ] = l;
        lp[ 5 ] = l;
        lp[ 6 ] = l;
        lp[ 7 ] = l;
        n        -= sizeof( unsigned long ) * 8;
        lp       += 8;
    }
    
    /* Writes a long at a time */
    while( n > sizeof( unsigned long ) )
    {
        *( lp++ ) = l;
        n        -= sizeof( unsigned long );
    }
    
    cp = ( unsigned char * )( ( void * )lp );
    
    /* Writes remaining bytes one by one */
    while( n-- )
    {
        *( cp++ ) = ( unsigned char )c;
    }
    
    return p;
}
