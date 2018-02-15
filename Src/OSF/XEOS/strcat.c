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
#include <string.h>

char * xeos_strcat_c( char * restrict s1, const char * restrict s2 );
char * xeos_strcat_c( char * restrict s1, const char * restrict s2 )
{
    if( s1 == NULL || s2 == NULL )
    {
        return s1;
    }
    
    /* Note: a scope is used in order to comply with the 'restrict' keyword */
    {
        unsigned char         c;
        unsigned long         l;
        unsigned char       * cp1;
        const unsigned char * cp2;
        unsigned long       * lp1;
        const unsigned long * lp2;
        
        cp1 = ( unsigned char * )s1;
        cp2 = ( const unsigned char * )s2;
        
        cp1 += strlen( ( const char * )cp1 );
        
        /* Copy one byte at a time until the source is aligned on a long */
        while( ( ( ( uintptr_t )cp2 & ( uintptr_t )-sizeof( unsigned long ) ) < ( uintptr_t )cp2 ) )
        {
            c          = *( cp2++ );
            *( cp1++ ) = c;
            
            if( c == 0 )
            {
                return s1;
            }
        }
            
        lp1 = ( void * )cp1;
        lp2 = ( void * )cp2;
        
        /* Checks if the destination is also aligned */
        if( ( ( uintptr_t )lp1 & ( uintptr_t )-sizeof( unsigned long ) ) == ( uintptr_t )lp1 )
        {
            /* Copy one long at a time */
            while( 1 )
            {
                l = *( lp2++ );
                
                /*
                 * Checks if a byte from "l" is zero - Thanks to Sean Eron Anderson:
                 * http://graphics.stanford.edu/~seander/bithacks.html
                 */
                #ifdef __LP64__
                if( ( ( l - 0x0101010101010101 ) & ( ~l & 0x8080808080808080 ) ) )
                #else
                if( ( ( l - 0x01010101 ) & ( ~l & 0x80808080 ) ) )
                #endif
                {
                    lp2--;
                    
                    break;
                }
                
                *( lp1++ ) = l;
            }
            
            cp1 = ( void * )lp1;
            cp2 = ( void * )lp2;
            
            /* Copy remaining bytes one by one */
            while( 1 )
            {
                c          = *( cp2++ );
                *( cp1++ ) = c;
                
                if( c == 0 )
                {
                    return s1;
                }
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
                    c = ( unsigned char )( ( l1[ 0 ] >> ( i * 8 ) ) & 0xFF );
                    
                    *( cp1++ ) = c;
                    
                    if( c == 0 )
                    {
                        return s1;
                    }
                }
                
                lp1 = ( void * )cp1;
            }
            
            *( cp1 ) = 0;
        }
    }
    
    return s1;
}
