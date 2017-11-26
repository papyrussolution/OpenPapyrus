/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2004 Yeico S. A. de C. V. All Rights Reserved.
 * Copyright 2008-2011 David Hoerl All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY David Hoerl ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL David Hoerl OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef XLSLIB_TIMESPAN_H
#define XLSLIB_TIMESPAN_H

/* #include "xlsys.h" */

#include <time.h>
#include <assert.h>	

class CCpuClockTimespan
{
  enum
    { INVALID_VALUE = -1, };
 
 public:
  CCpuClockTimespan()
    : m_clockStart(static_cast<clock_t>(INVALID_VALUE)),
    m_nUsedClockTicks(static_cast<clock_t>(INVALID_VALUE))
    {}
  ~CCpuClockTimespan()
    {}

  void StartClock()
    {
      // the clock is already started !!!
      // stop it first !!!
      assert( m_clockStart == static_cast<clock_t>(INVALID_VALUE));
      m_nUsedClockTicks = static_cast<clock_t>(INVALID_VALUE);
      m_clockStart = clock();
    }

  void StopClock()
    {
      const clock_t clockStop = clock();

      // start the clock first !!!
      assert( m_clockStart != (clock_t)INVALID_VALUE);
      m_nUsedClockTicks = clockStop - m_clockStart;

      // after this, we can start it again !!!
      m_clockStart = static_cast<clock_t>(INVALID_VALUE);
    }

  unsigned long GetUsedMilliseconds() const
    {
      const int MILLISECONDS_PER_SECOND = 1000;

      // the clock was never started,
      // or it's started, but it has not been stopped yet
      assert( m_nUsedClockTicks != (clock_t)INVALID_VALUE);

      double nSeconds =	( ( double)m_nUsedClockTicks) / CLOCKS_PER_SEC;

      const unsigned long nMilliseconds = (unsigned long)(nSeconds * MILLISECONDS_PER_SECOND);

      return nMilliseconds;
    }

 private:

  // when did we Start to measure
  // clock time?
  clock_t m_clockStart;

  // the used clock ticks, from Start to Stop
  clock_t m_nUsedClockTicks;
};

#endif
