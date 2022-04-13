/*
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 */
#ifndef OPJ_CLOCK_H
#define OPJ_CLOCK_H
/**
@file opj_clock.h
@brief Internal function for timing

The functions in OPJ_CLOCK.C are internal utilities mainly used for timing.
*/

/** @defgroup MISC MISC - Miscellaneous internal functions */
/*@{*/

/** @name Exported functions */
/*@{*/
/**
Difference in successive opj_clock() calls tells you the elapsed time
@return Returns time in seconds
*/
double opj_clock(void);
/*@}*/

/*@}*/

#endif /* OPJ_CLOCK_H */

