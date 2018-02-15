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

void * xeos_memcpy_c( void * restrict s1, const void * restrict s2, size_t n );
void * xeos_memcpy_c( void * restrict s1, const void * restrict s2, size_t n )
{
    if( s1 == NULL || s2 == NULL || n == 0 )
    {
        return s1;
    }
    
    /* Note: a scope is used in order to comply with the 'restrict' keyword */
    {
        unsigned char       * cp1;
        const unsigned char * cp2;
        unsigned long       * lp1;
        const unsigned long * lp2;
        
        cp1 = s1;
        cp2 = s2;
        
        /* Copy one byte at a time until the source is aligned on a long */
        while( n > 0 && ( ( ( uintptr_t )cp2 & ( uintptr_t )-sizeof( unsigned long ) ) < ( uintptr_t )cp2 ) )
        {
            *( cp1++ ) = *( cp2++ );
            
            n--;
        }
            
        lp1 = ( void * )cp1;
        lp2 = ( void * )cp2;
        
        /* Checks if the destination is also aligned */
        if( ( ( uintptr_t )lp1 & ( uintptr_t )-sizeof( unsigned long ) ) == ( uintptr_t )lp1 )
        {
            /* Loop unroll - Copy 16 longs at a time */
            while( n >= sizeof( unsigned long ) * 16 )
            {
                lp1[  0 ] = lp2[  0 ];
                lp1[  1 ] = lp2[  1 ];
                lp1[  2 ] = lp2[  2 ];
                lp1[  3 ] = lp2[  3 ];
                lp1[  4 ] = lp2[  4 ];
                lp1[  5 ] = lp2[  5 ];
                lp1[  6 ] = lp2[  6 ];
                lp1[  7 ] = lp2[  7 ];
                lp1[  8 ] = lp2[  8 ];
                lp1[  9 ] = lp2[  9 ];
                lp1[ 10 ] = lp2[ 10 ];
                lp1[ 11 ] = lp2[ 11 ];
                lp1[ 12 ] = lp2[ 12 ];
                lp1[ 13 ] = lp2[ 13 ];
                lp1[ 14 ] = lp2[ 14 ];
                lp1[ 15 ] = lp2[ 15 ];
                n        -= sizeof( unsigned long ) * 16;
                lp1      += 16;
                lp2      += 16;
            }
            
            /* Loop unroll - Copy 8 longs at a time */
            while( n >= sizeof( unsigned long ) * 8 )
            {
                lp1[ 0 ] = lp2[ 0 ];
                lp1[ 1 ] = lp2[ 1 ];
                lp1[ 2 ] = lp2[ 2 ];
                lp1[ 3 ] = lp2[ 3 ];
                lp1[ 4 ] = lp2[ 4 ];
                lp1[ 5 ] = lp2[ 5 ];
                lp1[ 6 ] = lp2[ 6 ];
                lp1[ 7 ] = lp2[ 7 ];
                n       -= sizeof( unsigned long ) * 8;
                lp1     += 8;
                lp2     += 8;
            }
            
            /* Copy one long at a time */
            while( n >= sizeof( unsigned long ) )
            {
                *( lp1++ ) = *( lp2++ );
                n         -= sizeof( unsigned long );
            }
            
            cp1 = ( void * )lp1;
            cp2 = ( void * )lp2;
            
            /* Copy remaining bytes one by one */
            while( n-- )
            {
                *( cp1++ ) = *( cp2++ );
            }
        }
        else
        {
            {
                unsigned long l1[ 8 ];
                unsigned long l2[ 8 ];
                size_t        diff;
                size_t        i;
                size_t        ls;
                size_t        rs;
                
                diff = 0;
                
                if( n >= sizeof( long ) * 9 )
                {
                    /* Number of bytes to write one by one until the destination is aligned on a long */
                    diff = sizeof( unsigned long ) - ( ( uintptr_t )cp1 - ( ( uintptr_t )cp1 & ( uintptr_t )-sizeof( unsigned long ) ) );
                    
                    /* Computes left and right shifts now, to save processing time */
                    ls = 8 * ( sizeof( long ) - diff );
                    rs = 8 * diff;
                    
                    /* Reads a long, and saves the remaining bytes that we'll have to write */
                    l1[ 0 ] = *( lp2++ );
                    l2[ 0 ] = l1[ 0 ] >> ( diff * 8 );
                    
                    /* Writes bytes one by one until the destination is aligned on a long */
                    for( i = 0; i < diff; i++ )
                    {
                        *( cp1++ ) = ( unsigned char )( ( l1[ 0 ] >> ( i * 8 ) ) & 0xFF );
                    }
                    
                    n  -= sizeof( long );
                    lp1 = ( void * )cp1;
                    
                    /* Writes 8 longs into the aligned destination, saving the remaining bytes */
                    while( n > sizeof( long ) * 8 )
                    {
                        l1[ 0 ]  = lp2[ 0 ];
                        l1[ 1 ]  = lp2[ 1 ];
                        l1[ 2 ]  = lp2[ 2 ];
                        l1[ 3 ]  = lp2[ 3 ];
                        l1[ 4 ]  = lp2[ 4 ];
                        l1[ 5 ]  = lp2[ 5 ];
                        l1[ 6 ]  = lp2[ 6 ];
                        l1[ 7 ]  = lp2[ 7 ];
                        
                        l2[ 1 ]  = l1[ 0 ] >> rs;
                        l2[ 2 ]  = l1[ 1 ] >> rs;
                        l2[ 3 ]  = l1[ 2 ] >> rs;
                        l2[ 4 ]  = l1[ 3 ] >> rs;
                        l2[ 5 ]  = l1[ 4 ] >> rs;
                        l2[ 6 ]  = l1[ 5 ] >> rs;
                        l2[ 7 ]  = l1[ 6 ] >> rs;
                        l2[ 0 ]  = l1[ 7 ] >> rs;
                        
                        lp1[ 0 ] = ( l1[ 0 ] << ls ) | l2[ 0 ];
                        lp1[ 1 ] = ( l1[ 1 ] << ls ) | l2[ 1 ];
                        lp1[ 2 ] = ( l1[ 2 ] << ls ) | l2[ 2 ];
                        lp1[ 3 ] = ( l1[ 3 ] << ls ) | l2[ 3 ];
                        lp1[ 4 ] = ( l1[ 4 ] << ls ) | l2[ 4 ];
                        lp1[ 5 ] = ( l1[ 5 ] << ls ) | l2[ 5 ];
                        lp1[ 6 ] = ( l1[ 6 ] << ls ) | l2[ 6 ];
                        lp1[ 7 ] = ( l1[ 7 ] << ls ) | l2[ 7 ];
                        
                        lp1 += 8;
                        lp2 += 8;
                        
                        n -= sizeof( long ) * 8;
                    }
                }
                
                cp1 = ( void * )lp1;
                cp2 = ( void * )lp2;
                
                /* Writes the remaining bytes, due to the alignment */
                if( diff != 0 )
                {
                    diff = sizeof( long ) - diff;
                    
                    while( diff-- )
                    {
                        *( cp1++ ) = ( unsigned char )( l2[ 0 ] & 0xFF );
                        l2[ 0 ]    = l2[ 0 ] >> 8;
                    }
                }
                
                /* Writes the remaining bytes */
                while( n-- )
                {
                    *( cp1++ ) = *( cp2++ );
                }
            }
        }
    }
    
    return s1;
}
