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

/*
 ***********************************
 *  COleProp class Implementation
 ***********************************
 */

using namespace xlslib_core;

COleProp::COleProp() :
	m_sName(),
	m_nNameSize(0),
	m_nPropType(0),
	m_nNodeColor(0),

	m_nPrevPropIndex(0),
	m_nNextPropIndex(0),
	m_nChildPropIndex(0),

	m_nTSCreatedSeconds(0),
	m_nTSCreatedDays(0),
	m_nTSModifiedSeconds(0),
	m_nTSModifiedDays(0),

	m_nStartBlock(0),
	m_nSize(0),
	m_nIndex(0),
	m_pData(NULL),
	m_Child_List()
{
}

COleProp::COleProp(int32 node_index, const std::string& name, CDataStorage* data) :
	m_sName(name),
	m_nNameSize(0),
	m_nPropType(0),
	m_nNodeColor(0),

	m_nPrevPropIndex(0),
	m_nNextPropIndex(0),
	m_nChildPropIndex(0),

	m_nTSCreatedSeconds(0),
	m_nTSCreatedDays(0),
	m_nTSModifiedSeconds(0),
	m_nTSModifiedDays(0),

	m_nStartBlock(0),
	m_nSize(0),
	m_nIndex(node_index),
	m_pData(data),
	m_Child_List()
{
}

COleProp::COleProp(int32 node_index, const char *name, CDataStorage* data) :
	m_sName(name),
	m_nNameSize(0),
	m_nPropType(0),
	m_nNodeColor(0),

	m_nPrevPropIndex(0),
	m_nNextPropIndex(0),
	m_nChildPropIndex(0),

	m_nTSCreatedSeconds(0),
	m_nTSCreatedDays(0),
	m_nTSModifiedSeconds(0),
	m_nTSModifiedDays(0),

	m_nStartBlock(0),
	m_nSize(0),
	m_nIndex(node_index),
	m_pData(data),
	m_Child_List()
{
}

COleProp::~COleProp()
{
	for(Tree_Level_Itor_t chld = m_Child_List.begin(); chld != m_Child_List.end(); chld++) {
		delete *chld;
	}
}

int COleProp::Init(int32 node_index, const std::string& name, CDataStorage* data)
{
	int errcode = NO_ERRORS;
	SetName(name);
	m_pData = data;
	m_nIndex = node_index;
	return errcode;
}

int COleProp::SetName(const std::string& name)
{
	int errcode = NO_ERRORS;
	m_nNameSize = static_cast<uint16>(name.size() + 1);
	m_sName = name;
	return errcode;
}
const std::string& COleProp::GetName(void) const
{
	return m_sName;
}

int COleProp::SetIndex(int32 newindex)
{
	int errcode = NO_ERRORS;
	m_nIndex = newindex;
	return errcode;
}

int COleProp::SetType(uint8 newtype)
{
	int errcode = NO_ERRORS;
	m_nPropType = newtype;
	return errcode;
}
uint8 COleProp::GetType(void) const
{
	return m_nPropType;
}

int COleProp::SetColor(uint8 newcolor)
{
	int errcode = NO_ERRORS;
	m_nNodeColor = newcolor;
	return errcode;
}
uint8 COleProp::GetColor(void) const
{
	return m_nNodeColor;
}

int COleProp::SetPreviousIndex(int32 prev)
{
	int errcode = NO_ERRORS;
	m_nPrevPropIndex = prev;
	return errcode;
}
int32 COleProp::GetPreviousIndex(void) const
{
	return m_nPrevPropIndex;
}

int COleProp::SetNextIndex(int32 next)
{
	int errcode = NO_ERRORS;
	m_nNextPropIndex = next;
	return errcode;
}
int32 COleProp::GetNextIndex(void) const
{
	return m_nNextPropIndex;
}

int COleProp::SetChildIndex(int32 child)
{
	int errcode = NO_ERRORS;
	m_nChildPropIndex = child;
	return errcode;
}
int32 COleProp::GetChildIndex(void) const
{
	return m_nChildPropIndex;
}

int COleProp::SetStartBlock(int32 sb)
{
	int errcode = NO_ERRORS;
	m_nStartBlock = sb;
	return errcode;
}
int32 COleProp::GetStartBlock(void) const
{
	return m_nStartBlock;
}

int COleProp::SetSize(size_t size)
{
	int errcode = NO_ERRORS;
	m_nSize = size;
	return errcode;
}
size_t COleProp::GetSize(void) const
{
	return m_nSize;
}

void COleProp::SetDataPointer(CDataStorage* pdata)
{
	m_pData = pdata;
}
CDataStorage* COleProp::GetDataPointer(void) const
{
	return m_pData;
}

void COleProp::SetCreatedSecs(int32 secs1)
{
	m_nTSCreatedSeconds = secs1;
}
int32 COleProp::GetCreatedSecs(void) const
{
	return m_nTSCreatedSeconds;
}

void COleProp::SetCreatedDays(int32 days1)
{
	m_nTSCreatedDays = days1;
}
int32 COleProp::GetCreatedDays(void) const
{
	return m_nTSCreatedDays;
}

void COleProp::SetModifiedSecs(int32 secs2)
{
	m_nTSModifiedSeconds = secs2;
}
int32 COleProp::GetModifiedSecs(void) const
{
	return m_nTSModifiedSeconds;
}

void COleProp::SetModifiedDays(int32 days2)
{
	m_nTSModifiedDays = days2;
}
int32 COleProp::GetModifiedDays(void) const
{
	return m_nTSModifiedDays;
}
