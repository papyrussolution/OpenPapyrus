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

#ifndef OLEDOC_H
#define OLEDOC_H

namespace xlslib_core
{
#define HEADPOS_ID                  (0x00)
#define HEADPOS_UK1                 (0x08)
#define HEADPOS_UK2                 (0x0c)
#define HEADPOS_UK2b                (0x10) /* According to the Excel sample... but undocummented */
#define HEADPOS_UK3                 (0x14)
#define HEADPOS_UK4                 (0x18)
#define HEADPOS_UK5                 (0x1a)
#define HEADPOS_UK6                 (0x1c)
#define HEADPOS_LOG2_BIGBLOCK       (0x1e)
#define HEADPOS_LOG2_SMALLBLOCK     (0x20)
#define HEADPOS_UK7                 (0x24)
#define HEADPOS_UK8                 (0x28)
#define HEADPOS_BAT_COUNT           (0x2c)
#define HEADPOS_PROPERTIES_SB       (0x30)
#define HEADPOS_UK9                 (0x34)
#define HEADPOS_UK10                (0x38)
#define HEADPOS_SBAT_COUNT          (0x40)
#define HEADPOS_SBAT_START          (0x3c)
#define HEADPOS_XBAT_START          (0x44)
#define HEADPOS_XBAT_COUNT          (0x48)
#define HEADPOS_BAT_ARRAY           (0x4c)


#define HEADVAL_DFLT_NOTUSED        (0xff)
#define HEADVAL_DFLT_UK1            (0x00)
#define HEADVAL_DFLT_UK2            (0x00)
/* According to the Excel sample... but undocummented */
#define HEADVAL_DFLT_UK2b           (0x00)
#define HEADVAL_DFLT_UK3            (0x00)
/* POIFS documentation says 0x3b... let's stick with the Excel sample ...*/
#define HEADVAL_DFLT_UK4            (0x3e)
#define HEADVAL_DFLT_UK5            (0x03)
#define HEADVAL_DFLT_UK6              (-2)
#define HEADVAL_DFLT_UK7            (0x00)
#define HEADVAL_DFLT_UK8            (0x00)
#define HEADVAL_DFLT_UK9            (0x00)
#define HEADVAL_DFLT_UK10         (0x1000)


#define HEADVAL_DFLT_LOG2_BIGBLOCK      (9)
#define HEADVAL_DFLT_LOG2_SMALLBLOCK    (6)
#define HEADVAL_DFLT_BATCOUNT			(0)
#define HEADVAL_DFLT_PROPERTIES_SB     (-2)
#define HEADVAL_DFLT_SBAT_START        (-2)
//POIFS says it should be 1 ... let's stick to M$
#define HEADVAL_DFLT_SBAT_COUNT         (0)
#define HEADVAL_DFLT_XBAT_START        (-2)
#define HEADVAL_DFLT_XBAT_COUNT         (0)
// #define HEADVAL_DFLT_BAT_ARRAY    /* Cannot have a default value */

#define HEAD_SIZE                BIG_BLOCK_SIZE
#define HEAD_ID_SZ               (0x08)

// could be char is not signed
#define BAT_NOT_USED_BYTE	(0xff)
#define BAT_NOT_USED		(-1)
#define BAT_END_CHAIN		(-2)
#define BAT_SELF_PLACE		(-3)
#define BAT_MSAT_PLACE		(-4)

// BAT blocks are filled - no pointers
#define BAT_ENTRIES_PER_BLOCK		(BIG_BLOCK_SIZE/4)
// pointer to next, or final terminator
#define BAT_BLOCKS_PER_MSAT_BLOCK	(BAT_ENTRIES_PER_BLOCK - 1)
#define HEADER_SAT_SIZE				109

/*
 ******************************
 ******************************COleFile class declaration
 ******************************
 */
	// Block allocation strategy. Within the OLE header are 109 slots for BAT Sectors.
	// But, when the file gets big, you run out (127 sectors in each BAT Sector). So,
	// the 110th BAT has to go into a special block dedicated to hold these. One additional
	// block gets you 127 more BAT entries, and so forth.
	//
	typedef struct {
		size_t bat_entries;         // total number of entries
		size_t _bat_entries;        // debug - count'm
		size_t bat_count;           // total number of sectors used for real data
		size_t _bat_count;          // debug - count'm
		size_t msat_count;          // total number of additional Master Sector Allocations Blocks (each hold 127)
		size_t header_bat_count;    // first 109 used
		size_t extra_bat_count;     // in addition to first 109
		size_t header_fill;         // padding in main header only!
		size_t extra_fill;          // padding in last MSAT!
	} blocks, *blocksP;

	class COleDoc : public CBinFile, public COleFileSystem {
	private:
		int  DumpHeader(blocks bks, size_t total_data_size);
		int  DumpData(void);
		int  DumpDepots(blocks bks);
		int  DumpFileSystem(void);
		size_t GetUnicodeName(const char* name, char** ppname_unicode);
		int DumpNode(COleProp& node);
		blocks GetBATCount();
		static const uint8 OLE_FILETYPE[];
	public:
		COleDoc();
		//COleDoc(const string& file_name);
		virtual ~COleDoc();
		int DumpOleFile();
	};
}

#endif
