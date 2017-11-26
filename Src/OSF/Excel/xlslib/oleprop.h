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

#ifndef OLEPROP_H
#define OLEPROP_H

namespace xlslib_core
{
	typedef std::vector <xlslib_core::COleProp * XLSLIB_DFLT_ALLOCATOR> Tree_Level_Vect_t;
	typedef Tree_Level_Vect_t::iterator Tree_Level_Itor_t;

	typedef std::vector<xlslib_core::COleProp* XLSLIB_DFLT_ALLOCATOR> NodeList_t;
	typedef NodeList_t::iterator NodeList_Itor_t;

	/*
	 ******************************
	 * COleProp class declaration
	 ******************************
	 */
	class COleProp {
	protected:
		std::string m_sName;
		uint16 m_nNameSize;
		uint8 m_nPropType;
		uint8 m_nNodeColor;

		int32 m_nPrevPropIndex;
		int32 m_nNextPropIndex;
		int32 m_nChildPropIndex;

		int32 m_nTSCreatedSeconds;
		int32 m_nTSCreatedDays;
		int32 m_nTSModifiedSeconds;
		int32 m_nTSModifiedDays;

		int32 m_nStartBlock;
		size_t m_nSize;

		// The following set of attributes are not part of the definition of
		// an OleDoc's property:
		int32 m_nIndex;
		CDataStorage* m_pData;

	private:
		COleProp(const COleProp& that);
		COleProp& operator=(const COleProp& right);

	public:
		Tree_Level_Vect_t m_Child_List;

		COleProp();
		COleProp(int32 node_index, const std::string& name, CDataStorage* data = NULL);
		COleProp(int32 node_index, const char *name, CDataStorage* data = NULL);
		~COleProp();

		int Init(int32 node_index, const std::string& name, CDataStorage* data = NULL);

		int SetName(const std::string& name);
		const std::string& GetName(void) const;

		int SetIndex(int32 newindex);
		inline int32 GetIndex(void) const {return m_nIndex; }

		int SetSize(size_t size);
		size_t GetSize(void) const;

		int SetType(uint8 newtype);
		uint8 GetType(void) const;

		int SetColor(uint8 newcolor);
		uint8 GetColor(void) const;

		int SetPreviousIndex(int32 prev);
		int32 GetPreviousIndex(void) const;

		int SetNextIndex(int32 next);
		int32 GetNextIndex(void) const;

		int SetChildIndex(int32 child);
		int32 GetChildIndex(void) const;

		int SetStartBlock(int32 sb);
		int32 GetStartBlock(void) const;

		void SetDataPointer(CDataStorage* pdata);
		CDataStorage* GetDataPointer(void) const;

		void SetCreatedSecs(int32 sec1);
		int32 GetCreatedSecs(void) const;

		void SetCreatedDays(int32 day1);
		int32 GetCreatedDays(void) const;

		void SetModifiedSecs(int32 sec2);
		int32 GetModifiedSecs(void) const;

		void SetModifiedDays(int32 day2);
		int32 GetModifiedDays(void) const;
	};
}

#endif
