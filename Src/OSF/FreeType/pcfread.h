/* pcfread.h

    FreeType font driver for pcf fonts

   Copyright 2003 by
   Francesco Zappa Nardelli

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
 */

#ifndef PCFREAD_H_
#define PCFREAD_H_

FT_BEGIN_HEADER
    FT_LOCAL(PCF_Property) pcf_find_property(PCF_Face face, const FT_String*  prop);
FT_END_HEADER

#endif /* PCFREAD_H_ */

/* END */
