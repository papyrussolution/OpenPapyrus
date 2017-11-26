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

#include "xlslib-internal.h"
#pragma hdrstop

#define OLE_DEBUG	0

using namespace std;
using namespace xlslib_core;

/*
 ***********************************
 *  COleDoc class implementation
 ***********************************
 */
const uint8 COleDoc::OLE_FILETYPE[] = { 0xd0, 0xcf, 0x11, 0xe0, 0xa1, 0xb1, 0x1a, 0xe1};

COleDoc::COleDoc()
{
}

#if 0
COleDoc::COleDoc(const string& file_name)
{
	Open(file_name);
}
#endif
 

COleDoc::~COleDoc()
{
}

int COleDoc::DumpHeader(blocks bks, size_t total_data_size)
{
	size_t i;
	size_t total_data_blocks;
	size_t sectorID;
	size_t msatID;
	int	errcode = NO_ERRORS;

	total_data_blocks = total_data_size/BIG_BLOCK_SIZE;

#if OLE_DEBUG
	std::cerr << "dataBlocks=" << total_data_blocks << std::endl;
#endif
	// [00]FILETYPE
	WriteByteArray(COleDoc::OLE_FILETYPE, sizeof(COleDoc::OLE_FILETYPE));
	// [08]UK1
	WriteSigned32(HEADVAL_DFLT_UK1);
	// [0c]UK2
	WriteSigned32(HEADVAL_DFLT_UK2);
	// [10]UK2b
	WriteSigned32(HEADVAL_DFLT_UK2b);
	// [14]UK3
	WriteSigned32(HEADVAL_DFLT_UK3);
	// [18]UK4
	WriteSigned16(HEADVAL_DFLT_UK4);
	// [1a]UK5
	WriteSigned16(HEADVAL_DFLT_UK5);
	// [1c]UK6
	WriteSigned16(HEADVAL_DFLT_UK6);
	// [1e]LOG_2_BIG_BLOCK
	WriteSigned16(HEADVAL_DFLT_LOG2_BIGBLOCK);
	// [20]LOG_2_SMALL_BLOCK
	WriteSigned32(HEADVAL_DFLT_LOG2_SMALLBLOCK);
	// [24]UK7
	WriteSigned32(HEADVAL_DFLT_UK7);
	// [28]UK8
	WriteSigned32(HEADVAL_DFLT_UK8);

	// [2c] BAT_COUNT (BBDEPOT NUM BLOCKS)
	WriteUnsigned32((uint32)bks.bat_count);

	//[30] PROPERTIES_START_BLOCK
	// Since the big block depot will go immediately after the data, I need
	// to know the size of the data and the size of the BAT in blocks (prev)
	WriteUnsigned32((uint32)(bks.msat_count+total_data_blocks+bks.bat_count));
#if OLE_DEBUG
	std::cerr << "HEADER says directory at " << (bks.msat_count+total_data_blocks+bks.bat_count) << std::endl;
#endif
	// [34] UK9
	WriteSigned32(HEADVAL_DFLT_UK9);
	// [38] UK10
	WriteSigned32(HEADVAL_DFLT_UK10);

	// [3c] SBAT_START
	// No small blocks will be used, so this is set to the default empty value
	WriteSigned32(HEADVAL_DFLT_SBAT_START);

	// [40] SBAT_BLOCKCOUNT_NUMBER
	// Use the default value
	WriteSigned32(HEADVAL_DFLT_SBAT_COUNT);

	// [44] XBAT_START
	// we will use first and possibly additional blocks for large files
	WriteSigned32(bks.msat_count ? 0 : HEADVAL_DFLT_XBAT_START);
#if OLE_DEBUG
	std::cerr <<  "xbatStart=" << (bks.msat_count ? 0 : HEADVAL_DFLT_XBAT_START) << std::endl;
#endif

	// [48] XBAT_COUNT
	WriteUnsigned32((uint32)bks.msat_count);  // was HEADVAL_DFLT_XBAT_COUNT (0)
#if OLE_DEBUG
	std::cerr << "msat_count=" << bks.msat_count << std::endl;
#endif

	// [4C] BAT_ARRAY
	// The BAT_ARRAY shall be calculated from the number of BAT blocks and their position

	// The additional blocks, if needed, are directly below the header block, so we can write
	// them out contiguously. The special conditions are:
	//  * for each MSAT block, the last entry needs to be a pointer to the next block
	//  * the fill is -1 for all unused entries
	//  * if there are MSAT blocks, the very last entry in the last block is a special marker
	// first sector ID
	sectorID = bks.msat_count + total_data_blocks;
	for(i=0; i<bks.header_bat_count; i++) {
#if OLE_DEBUG
		std::cerr << "sectorID=" << sectorID << std::endl;
#endif
		WriteUnsigned32((uint32)sectorID++);
	}
#if OLE_DEBUG
	std::cerr << std::hex << Position() << std::dec << std::endl;
	std::cerr << "SEC_ID[" << i << "]=" << (total_data_size/(BIG_BLOCK_SIZE) + i) << std::endl;
#endif

	// if we don't fill the header table, zero out unused entries
	for(i = 0; i<bks.header_fill; i++) {
		WriteSigned32(BAT_NOT_USED);
	}

#if OLE_DEBUG
	std::cerr << "Position=0x" << std::hex << Position() << std::dec << std::endl;
#endif

	// plow ahead, adding up to 127 entries per extra MSAT block
	msatID = 1; // sector 0 is the first MSAT, if used
	for(i=1; i<=bks.extra_bat_count; i++) {
		WriteUnsigned32((uint32)sectorID++);
		if((i % BAT_BLOCKS_PER_MSAT_BLOCK) == 0) {
			if(i == bks.extra_bat_count) {
				WriteSigned32(BAT_END_CHAIN); // pointer to next MSAT
			} else {
				WriteUnsigned32((uint32)msatID++);  // pointer to next MSAT
			}
		}
	}
	if(bks.extra_fill) {
		for(i = 0; i<bks.extra_fill; i++) {
			WriteSigned32(BAT_NOT_USED);
		}
		WriteSigned32(BAT_END_CHAIN);   // pointer to next MSAT
	}

#if OLE_DEBUG
	std::cerr << "Position=0x" << std::hex << Position() << std::dec << std::endl;
#endif

	XL_ASSERT(Position() == (HEAD_SIZE + (bks.msat_count*BIG_BLOCK_SIZE)));

	return errcode;
}

int COleDoc::DumpData(void)
{
	int errcode = NO_ERRORS;

	NodeList_t node_list;
	GetAllNodes(node_list);
	for(NodeList_Itor_t i = node_list.begin(); i != node_list.end(); i++) {
		if((*i)->GetType() == PTYPE_FILE) {
			for(StoreList_Itor_t j = (*i)->GetDataPointer()->begin();
				j != (*i)->GetDataPointer()->end(); j++) {
				//unsigned short *val = (unsigned short *)(j->GetBuffer());
				//printf("POS=%u wrote=%lu HEX=0x%4.4x\n", Position(), j->GetDataSize(), *val);
				XL_ASSERT(j->GetBuffer() != NULL);
				//XL_ASSERT(j->GetDataSize() > 0);
				errcode = WriteByteArray(j->GetBuffer(), j->GetDataSize());
				if (errcode != NO_ERRORS) {
					break;
				}
			}
		}
	}

	return errcode;
}

int COleDoc::DumpDepots(blocks bks)
{
	int errcode = NO_ERRORS;

	NodeList_t node_list;
	GetAllNodes(node_list);
	int32 bat_index;

	bat_index = 0;

	// tells Excel that these are used by the MSAT
	for(size_t i=0; i<bks.msat_count; ++i) {
		WriteSigned32(BAT_MSAT_PLACE);
		++bat_index;
		++bks._bat_entries;
	}

#if OLE_DEBUG
	int foo=0;
#endif
	for(NodeList_Itor_t node = node_list.begin(); node != node_list.end(); node++) {
		size_t chain_len;
		size_t data_size;

		// The start block is set in the node.
		(*node)->SetStartBlock(bat_index);
		// Write the chain for this node element
		data_size	= (*node)->GetDataPointer()->GetDataSize();
		chain_len	= data_size/BIG_BLOCK_SIZE - 1;
#if OLE_DEBUG
		std::cerr	<< "NODE[" << foo++ << "]: start_block=" << bat_index
					<< " dataSize=" << data_size
					<< " Sectors=" << chain_len + 1 /* directory_terminator */ << std::endl;
#endif
		for(size_t i = 0; i < chain_len; i++) {
			WriteSigned32(++bat_index);
			++bks._bat_entries;
		}

		// Set the terminator number
		WriteSigned32(BAT_END_CHAIN);
		++bat_index;
		++bks._bat_entries;
	}

#if OLE_DEBUG
	std::cerr	<< "BAT_SELF_PLACE=" << (bat_index+1)
				<< " -> " << (bat_index+1+bks.bat_count+1)
				<< " TOTAL=" << bks.bat_count << std::endl;
#endif
	// Write the -3 number for every index in the BAT that references to some BAT block (uh!?)
	for(size_t i=0; i<bks.bat_count; i++) {
		WriteSigned32(BAT_SELF_PLACE);
		++bat_index;
		++bks._bat_entries;
	}
#if OLE_DEBUG
	std::cerr << "last write: left = " << (Position() % HEAD_SIZE) << std::endl;
#endif

	// This is the entry for the directory chain, the very last block, saying directory is just one sector
	WriteSigned32(BAT_END_CHAIN);
	++bat_index;
	++bks._bat_entries;

	// Fill the rest of the _LAST_ BAT block, code appears to handle the 0 case (IT DID NOT - SEE addition of final % BIG_BLOCK_SIZE BELOW)
	uint32 num_indexes = (uint32)bat_index;
	uint32 to_fill_size = (BIG_BLOCK_SIZE - ((4*num_indexes) % BIG_BLOCK_SIZE)) % BIG_BLOCK_SIZE;

#if OLE_DEBUG
	std::cerr << "num_indexes=" << num_indexes << " to_fill_size=" << to_fill_size << std::endl;
#endif

	SerializeFixedArray(BAT_NOT_USED_BYTE, to_fill_size);

#if OLE_DEBUG
	std::cerr << "last write: left = " << (Position() % HEAD_SIZE) << std::endl;

	std::cerr << "Position=0x" << std::hex << Position() << std::dec << std::endl;
	std::cerr << "bat_entries=" << bks.bat_entries << " actual=" << bks._bat_entries << std::endl;
#endif

	XL_ASSERT(bks.bat_entries == bks._bat_entries);

	return errcode;
}

int COleDoc::DumpFileSystem(void)
{
	int errcode = NO_ERRORS;


	NodeList_t node_list;
	GetAllNodes(node_list);

#if OLE_DEBUG
	std::cerr	<< "FILESYSTEM directory at SecID=" << (Position() - HEAD_SIZE)/BIG_BLOCK_SIZE
				<< " (remain=" << (Position() % BIG_BLOCK_SIZE) << ")" << std::endl;
#endif
	DumpNode(GetRootEntry());

	for(NodeList_Itor_t node  = node_list.begin(); node != node_list.end(); node++) {
		DumpNode(*(*node));
	}

	return errcode;
}

int COleDoc::DumpOleFile(void)
{
	blocks bks;
	int errcode = NO_ERRORS;
	bks = GetBATCount();
	size_t total_data_size = GetTotalDataSize();
	//printf("TOTAL %lu blocks=%lu mod=%lu\n", total_data_size, total_data_size/512, total_data_size % 512);
	errcode |= DumpHeader(bks, total_data_size);
	XL_ASSERT((Position() % 512) == 0 /*1*/);
	errcode |= DumpData();
	XL_ASSERT((Position() % 512) == 0 /*2*/);
	errcode |= DumpDepots(bks);
	XL_ASSERT((Position() % 512) == 0 /*3*/);
	errcode |= DumpFileSystem();
	XL_ASSERT((Position() % 512) == 0 /*3*/);
	return errcode;
}

blocks COleDoc::GetBATCount()
{
	blocks bks;
	size_t bat_num_entries;
	size_t data_bat_entries;
	size_t bat_num_blocks, dir_bat_entries, bat_blocks_needed, bat_block_capacity;
	size_t extra_bats, msat_blocks, msat_bats, last_block_extras;
	memset(&bks, 0, sizeof(bks));
	data_bat_entries = GetTotalDataSize()/BIG_BLOCK_SIZE; //  + GetNumDataFiles(); //terminator???
	XL_ASSERT(GetTotalDataSize() == (data_bat_entries * BIG_BLOCK_SIZE) );

	// Block allocation strategy. Within the OLE header are 109 slots for BAT Sectors.
	// But, when the file gets big, you run out (127 sectors in each BAT Sector). So,
	// the 110th BAT has to go into a special block dedicated to hold these. One additional
	// block gets you 127 more BAT entries, and so forth.

	dir_bat_entries		= 1;    // Last block is directory, this is the terminator that says dir uses only one sector
	bat_num_blocks		= 1;    // seed to enter loop below
	bat_block_capacity	= 0;
	msat_blocks			= 0;
	//msat_bats			= 0;	// no affect
	bat_num_entries		= 0;    // kch
	bat_blocks_needed	= data_bat_entries/BAT_ENTRIES_PER_BLOCK;       // minimum
	bat_blocks_needed	+= bat_blocks_needed/BAT_ENTRIES_PER_BLOCK;     // for the BAT itself

	// first loop assumes no MSATs involved
	while(bat_num_blocks > bat_block_capacity || bat_num_blocks != bat_blocks_needed) {
		bat_num_blocks = bat_blocks_needed;
		if(bat_num_blocks > HEADER_SAT_SIZE) {
			msat_bats = bat_num_blocks - HEADER_SAT_SIZE;
			msat_blocks = msat_bats/BAT_BLOCKS_PER_MSAT_BLOCK;
			if(msat_bats % BAT_BLOCKS_PER_MSAT_BLOCK) {++msat_blocks; }
		}

		bat_num_entries = msat_blocks + data_bat_entries + bat_num_blocks + dir_bat_entries;    // bat_bat_entries

		// based on what we know now, this is what we need
		bat_blocks_needed	= bat_num_entries/BAT_ENTRIES_PER_BLOCK;
		if(bat_num_entries % BAT_ENTRIES_PER_BLOCK) {++bat_blocks_needed; }

		// number of slots available
		bat_block_capacity = HEADER_SAT_SIZE + BAT_BLOCKS_PER_MSAT_BLOCK*msat_blocks;
#if OLE_DEBUG
		std::cerr	<< "bat_blocks=" << bat_num_blocks
					<< " capacity=" << bat_block_capacity
					<< " needed=" << bat_blocks_needed << std::endl;
#endif
	}

	if(bat_num_blocks > HEADER_SAT_SIZE) {
		extra_bats				= bat_num_blocks - HEADER_SAT_SIZE;

		bks.msat_count			= msat_blocks;
		bks.header_bat_count	= HEADER_SAT_SIZE;
		bks.extra_bat_count		= extra_bats;

		last_block_extras = extra_bats % BAT_BLOCKS_PER_MSAT_BLOCK;
		if(last_block_extras) {
			bks.extra_fill	= BAT_BLOCKS_PER_MSAT_BLOCK - last_block_extras;
		}
	} else {
		bks.header_bat_count	= bat_num_blocks;
		bks.header_fill			= HEADER_SAT_SIZE - bat_num_blocks;
	}
	bks.bat_entries			= bat_num_entries;
	bks.bat_count			= bat_num_blocks;

#if OLE_DEBUG
	std::cerr	<< "entries=" << bks.bat_entries << " bats=" << bks.bat_count
				<< " msats=" << bks.msat_count << " headerBats=" << bks.header_bat_count
				<< " extraBats=" << bks.extra_bat_count << " headFill=" << bks.header_fill
				<< " extraFill=" << bks.extra_fill << std::endl;
#endif

	return bks;
}

// NOTE: name_unicode has to be deleted after this function finishes.
// Ideally, this function should be implemented as part of a std::string
// derived class, so the array would be deleted automatically


size_t COleDoc::GetUnicodeName(const char* name, char** ppname_unicode)
{
	size_t name_size = strlen(name);
	if(name_size > PROPERTY_MAX_NAME_LENGTH) {
		name_size = PROPERTY_MAX_NAME_LENGTH;
	}

	size_t size_unicode = (name_size+1)*2;

	if(*ppname_unicode != NULL) { delete[] *ppname_unicode; }
	*ppname_unicode = (char*)new uint8[size_unicode];
	memset(*ppname_unicode, 0x00, size_unicode);

	for(size_t i=0; i<(size_unicode/2-1); i++) {
		(*ppname_unicode)[2*i] = name[i];
	}

	return size_unicode;
}

int COleDoc::DumpNode(COleProp& node)
{
	int errcode = NO_ERRORS;
	char* name_unicode = NULL;

	// Get the unicode name and its size
	size_t size_name = GetUnicodeName(node.GetName().c_str(), &name_unicode);

	// [00] PROPERTY_NAME
	WriteByteArray((const uint8*)name_unicode, size_name);

	// Fill the rest of the name field with 0x00
	XL_ASSERT(PPTPOS_NAMELENGTH > size_name);
	SerializeFixedArray(PROPERTY_DFLT_NOTUSED, PPTPOS_NAMELENGTH - size_name);

	// [40] NAME_SIZE
	WriteSigned16((int16)size_name);

	// [42] PROPERTY_TYPE
	WriteByte(node.GetType());

	// [43] NODE_COLOR
	WriteByte(node.GetColor());

	// [44] PREVIOUS_PROP
	WriteSigned32(node.GetPreviousIndex());
	// [48] NEXT_PROP
	WriteSigned32(node.GetNextIndex());
	// [4c] CHILD_PROP
	WriteSigned32(node.GetChildIndex());

	// printf("NAME: %s TYPE=%d Color=%d Indexes: %4.4x %4.4x %4.4x\n", node.GetName().c_str(), node.GetType(), node.GetColor(), node.GetPreviousIndex(), node.GetNextIndex(), node.GetChildIndex() );

	// Fill empty block
	SerializeFixedArray(PROPERTY_DFLT_NOTUSED, (PPTPOS_SECS1 - PPTPOS_UNUSED_EMPTY0));

	//[64]...[70]
	// SECONDS_1, DAYS_2, SECONDS_2, DAYS_2
	WriteSigned32(node.GetCreatedSecs());
	WriteSigned32(node.GetCreatedDays());
	WriteSigned32(node.GetModifiedDays());
	WriteSigned32(node.GetModifiedSecs());

	// [74] START_BLOCK
#if OLE_DEBUG
	std::cerr << "START_BLOCK_1=" << node.GetStartBlock() << std::endl;
#endif
	WriteSigned32(node.GetStartBlock());
	// [78] SIZE
	if(node.GetType() == PTYPE_FILE) {
		WriteSigned32((int32)node.GetSize());
	} else {
		WriteSigned32(0);
	}
	// A unused space:
	WriteSigned32(PROPERTY_DFLT_NOTUSED);
	delete[] name_unicode;
	name_unicode = NULL;
	return errcode;
}
