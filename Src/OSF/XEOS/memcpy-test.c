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

#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static double __gettime()
{
    struct timeval  t;
    struct timezone tzp;
    
    gettimeofday( &t, &tzp );
    
    return t.tv_sec + t.tv_usec * 1e-6;
}

void * xeos_memcpy( void * restrict s1, const void * restrict s2, size_t n );
void * xeos_memcpy_c( void * restrict s1, const void * restrict s2, size_t n );

extern int _SSE2Status;

int main( void )
{
    size_t c;
    size_t i;
    char * s1;
    char * s2;
    double t1;
    double t2;
    double t3;
    double t4;
    
    start:
    
    xeos_memcpy( NULL, NULL, 0 );
    
    #ifdef __LP64__
    printf( "---------- Testing on x86_64 | SSE2 = %i ----------\n", _SSE2Status );
    #else
    printf( "---------- Testing on i386 | SSE2 = %i ----------\n", _SSE2Status );
    #endif
    
    s1 = calloc( 100, 1 );
    s2 = malloc( 100 );
    
    memcpy( s1, "ABCDEFGH ABCDEFGH ABCDEFGH ABCDEFGH ABCDEFGH ABCDEFGH ABCDEFGH ABCDEFGH ABCDEFGH ABCDEFGH ABCDEFGH ", 99 );
    
    memset( s2, ' ', 99 );
    memcpy( s2, s1, 100 );
    printf( "    memcpy:                                                   %s\n", s2 );
    
    memset( s2, ' ', 99 );
    xeos_memcpy( s2, s1, 100 );
    printf( "    xeos_memcpy:                                              %s\n", s2 );
    
    memset( s2, ' ', 99 );
    xeos_memcpy_c( s2, s1, 100 );
    printf( "    xeos_memcpy_c:                                            %s\n", s2 );
    
    s1 += 1;
    s2 += 2;
    
    memset( s2, ' ', 98 );
    memcpy( s2, s1, 98 );
    printf( "    memcpy (misaligned):                                      %s\n", s2 );
    
    memset( s2, ' ', 98 );
    xeos_memcpy( s2, s1, 98 );
    printf( "    xeos_memcpy (misaligned):                                 %s\n", s2 );
    
    memset( s2, ' ', 98 );
    xeos_memcpy_c( s2, s1, 98 );
    printf( "    xeos_memcpy_c (misaligned):                               %s\n", s2 );
    
    free( s1 - 1 );
    free( s2 - 2 );
    
    s1 = malloc( 4096 );
    s2 = malloc( 4096 );
    
    c  = 10000000;
    t1 = __gettime();
    
    for( i = 0; i < c; i++ )
    {
        memcpy( s2, s1, 4096 );
    }
    
    t2 = __gettime();
    
    for( i = 0; i < c; i++ )
    {
        xeos_memcpy( s2, s1, 4096 );
    }
    
    t3 = __gettime();
    
    for( i = 0; i < c; i++ )
    {
        xeos_memcpy_c( s2, s1, 4096 );
    }
    
    t4 = __gettime();
    
    printf( "    %zi iterations - time of memcpy:                     %f\n", c, t2 - t1 );
    printf( "    %zi iterations - time of xeos_memcpy:                %f\n", c, t3 - t2 );
    printf( "    %zi iterations - time of xeos_memcpy_c:              %f\n", c, t4 - t3 );
    
    free( s1 );
    free( s2 );
    
    s1 = malloc( 4096 + 1 );
    s2 = malloc( 4096 + 2 );
    
    s1 += 1;
    s2 += 2;
    
    c  = 10000000;
    t1 = __gettime();
    
    for( i = 0; i < c; i++ )
    {
        memcpy( s2, s1, 4096 );
    }
    
    t2 = __gettime();
    
    for( i = 0; i < c; i++ )
    {
        xeos_memcpy( s2, s1, 4096 );
    }
    
    t3 = __gettime();
    
    for( i = 0; i < c; i++ )
    {
        xeos_memcpy_c( s2, s1, 4096 );
    }
    
    t4 = __gettime();
    
    printf( "    %zi iterations - time of memcpy (misaligned):        %f\n", c, t2 - t1 );
    printf( "    %zi iterations - time of xeos_memcpy (misaligned):   %f\n", c, t3 - t2 );
    printf( "    %zi iterations - time of xeos_memcpy_c (misaligned): %f\n", c, t4 - t3 );
    
    free( s1 - 1 );
    free( s2 - 2 );
    
    if( _SSE2Status == 1 )
    {
        _SSE2Status = 0;
        
        goto start;
    }
    
    return 0;
}
