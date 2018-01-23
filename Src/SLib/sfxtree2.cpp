// SFXTREE2.CPP
// Copyright (c) A.Sobolev 2016, 2018
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

DECL_CMPFUNC(SfxTreeChr);

class SSuffixTree {
public:
	struct Stat {
		uint   StringCount;
		uint   StringLen;
		uint   NodeCount;
		uint   EdgeCount;
		uint   EdgeHubCount;
		uint   NodeWideMax; // Максимальное количество ребер, исходящих из узла
		double NodeWideAvg; // Среднее количество ребер, исходящих из узла
		uint   NodeWithSfxLinkCount; // Количество узлов с ненулевым индекстом суффиксной связи
		size_t MemStr;
		size_t MemEdge;
		size_t MemNode;
		size_t MemEdgeHub;
	};

	SSuffixTree(uint32 itemSize);
	uint   GetItemSize() const
	{
		return ItemSize;
	}
	uint   FASTCALL CreateString(uint32 * pId);
	int    AddChunkToString(uint strP, const void * pItems, uint itemsCount);
	int    InsertString(uint stringP);
	//
	// Descr: Ищет символ алфавита pChr. Если символ найден, то возвращает
	//   номер позиции [1..], в противном случае возвращает 0.
	//
    uint   FASTCALL SearchChrInAlphabet(const void * pChr);
    //
    // Descr: Возвращает символ алфавита по номеру позиции [1..]
    //
    const  void * FASTCALL GetChrFromAlphabet(uint pos) const;
	uint   FASTCALL GetEdgeLen(uint edgeP) const;
	int    FASTCALL IsLeafNode(uint nodeP) const;
	uint   FASTCALL GetStrLen(uint strP) const;
	uint   FASTCALL GetParentNodeP(uint nodeP) const;
	//
	void   CalcStat(Stat * pStat) const;
	void   DebugOutput(long flags, SString & rBuf);
	void   DebugOutputStat(SString & rBuf) const;
private:
	class String : public SVector { // @v9.8.4 SArray-->SVector
	public:
		String(uint itemSize, uint32 id);

        uint32 ID;
        uint   PhasePosition;
	};

	class StringArray : public TSArray <SSuffixTree::String> { 
	public:
		StringArray();
		~StringArray();
		SSuffixTree::String * FASTCALL Get(uint pos) const;
		const  SSuffixTree::String * FASTCALL GetC(uint pos) const;
		int    SearchById(uint32 id, uint * pPos) const;
		int    CreateNewItem(const SSuffixTree * pT, uint32 id, uint * pPos);
	private:
		virtual void FASTCALL freeItem(void * pItem);
	};

	class IndexBase {
	public:
		IndexBase();
		IndexBase(uint strP, uint startIdx, uint endIdx);
		void   Init();
		IndexBase & FASTCALL operator = (const IndexBase & rS);

		uint   StrP;     // Позиция строки в SSuffixTree::StrList
		uint   StartIdx; // Start and end indices in the string
		uint   EndIdx;
		enum {
			fPhasePosRef = 0x0001 // EndIdx ссылается на позицю SSuffixTree::String.PhasePosion
		};
		uint   Flags;
	};

	struct Edge : public SSuffixTree::IndexBase {
		Edge();

		uint   SrcNodeP;
		uint   DestNodeP;
	};

	class EdgeHub {
	public:
		EdgeHub();
		~EdgeHub();
		void   Destroy();
		uint   GetCount() const;
		size_t SizeOf() const;
		uint   Get(uint p) const;
		int    Add(uint item);

		uint   Count;
		union {
			uint   I[4];
            uint * Ptr;
		};
	};

	class EdgeHubArray : public TSArray <EdgeHub> {
	public:
        EdgeHubArray();
        ~EdgeHubArray();
        uint   Add_();
	private:
		virtual void FASTCALL freeItem(void * pItem);
	};

	struct Node {
		Node();
		int    IsLeaf() const { return (EdgeHubP == 0); }
		uint   UpEdgeP;      // Индекс в массиве SSuffixArray::EdgeL входящего ребра
		uint   EdgeHubP;     // Позиция хаба исходящих ребер
		uint   SfxLinkNodeP; // Индекс в массиве SSuffixArray::NodeL суффиксной связи
	};
	//
	// A path in an implicit suffix tree can end at either a node, or
	// at some point in the label of an edge. We remember in each
	// extension where we ended using an LstPathEnd structure.
	//
	struct PathEnd {
		PathEnd();
		void   FASTCALL SetNode(uint nodeP);
		void   SetEdge(uint edgeP, uint offs);
		void   Advance(const SSuffixTree & rT, uint edgeP);

		uint   NodeP;
		uint   EdgeP;
		uint   Offset;
	};
	//
	//
	//
	uint   FASTCALL CreateNode(/*int nodeIndex*/);
	uint   CreateEdge(uint srcNodeP, uint destNodeP, uint strP, uint startIdx);
	uint   CreateEdge(uint srcNodeP, uint destNodeP, uint strP, uint startIdx, uint endIdx);
	uint   Helper_CreateEdge(uint srcNodeP, uint destNodeP, uint strP, uint startIdx, uint endIdx, uint flags);
	//
	// Descr: Создает листовой узел с индексом leafNodeIdx и ребро от srcNodeP к новому листовому узлу
	//
	uint   CreateLeaf(uint srcNodeP, /*int leafNodeIdx,*/ uint strP, uint startIdx);
	Edge * FASTCALL GetEdge(uint p) const;
	uint   FASTCALL GetIndexLen(const SSuffixTree::IndexBase & rI) const;
	Node * FASTCALL GetNode(uint p) const;
	EdgeHub * FASTCALL GetEdgeHub(uint p) const;
	SSuffixTree::String * FASTCALL GetStr(uint p) const;
	uint   FASTCALL GetParentNodeP(Node * pNode) const;
	uint   FASTCALL GetEndIdx(const SSuffixTree::IndexBase * pI) const;
	int    FASTCALL IncrementEndIdx(SSuffixTree::IndexBase * pI);
	int    FASTCALL UpdateEndIdx(SSuffixTree::IndexBase * pI, uint newPosition);
	void   OutputChr(const SSuffixTree::String * pStr, uint chrIdx, long flags, SString & rBuf) const;
	void   OutputChrChunk(const SSuffixTree::IndexBase & rRange, long flags, SString & rBuf) const;
	void   OutputEdge(uint edgeP, long flags, SString & rBuf) const;
	void   OutputNode(uint nodeP, uint tabs, long flags, SString & rBuf) const;
	//
	//
	//
	enum {
		fNeedVisitorUpdate = 0x0001
	};
	uint32 ItemSize;
	long   Flags;
	uint   RootNodeP;
	uint32 LastStrAutoId;
	StringArray StrList;
	String Alphabet; // Список символов, встречающихся во всех строках StrList. Каждый
		// символ представлен уникально, но массив не отсортирован (Edge ссылается на элементы
		// массива по индексу позиции [0..]).
	TSVector <Node> NodeL; // Нулевая позиция в массиве - эксклюзивная // @v9.8.4 TSArray-->TSVector
	TSVector <SSuffixTree::Edge> EdgeL; // Нулевая позиция в массиве - эксклюзивная // @v9.8.4 TSArray-->TSVector
	EdgeHubArray HubL; // Нулевая позиция в массиве - эксклюзивная
	//
	// Current phase of Ukkonen's algorithm. In order to implement the "Once a leaf, always a leaf"
	// Trick as explained by Gusfield, we make this a pointer to an integer.
	//
	// uint * P_Phase;

	//
	// To avoid the O(m) cost of setting that value in stone once a string insertion is over, we make phase
	// point into the following list. A new element is created for every string insertion.
	//
	// TSCollection <uint> Phases;
	//uint   Ext; // Current extension of Ukkonen's algorithm.

	int    IsEqChr(const void * p1, const void * p2) const;
	int    StrEq(const void * pChr, uint strP, uint itemIdx) const;
	int    StrEq(uint str1P, uint item1, uint str2P, uint item2) const;
	int    StrEq(const SSuffixTree::String * pS1, uint item1, const SSuffixTree::String * pS2, uint item2) const;
	uint   StrItemsCommon(uint str1P, uint off1, uint str2P, uint off2, uint maxLen) const;

	uint   FindEdgeWithStartitem(uint nodeP, uint strP, uint index) const;
	uint   FindEdgeWithStartitem(uint nodeP, const void * pChr) const;
	//
	// Descr: follows an arbitrary string through the tree.
	// @tree: tree to query.
	// @node: node to start at.
	// @string: the string to follow.
	// @end: result argument, see below.
	//
	// The function follows the @skipstring in @tree starting from @node,
	// reporting where in the tree the string ends through the @end
	// result pointer.
	//
	// Returns: number of string items successfully matched.
	//
	uint   FollowStringSlow(uint nodeP, uint strP, SSuffixTree::PathEnd & rEnd);
	//
	// Descr: follows an existing string in the tree, using skip/count.
	// @tree: tree to query.
	// @node: node to start at.
	// @skipstring: the string to follow.
	// @end: result argument, see below.
	//
	// The function follows the @skipstring in @tree starting from @node,
	// reporting where in the tree the string ends through the @end
	// result pointer.
	//
	void   FollowString(uint nodeP, const SSuffixTree::IndexBase & rSkipString, SSuffixTree::PathEnd & rEnd);
};

IMPL_CMPFUNC(SfxTreeChr, p1, p2)
{
	int    result = 0;
    const SSuffixTree * p_st = (const SSuffixTree *)pExtraData;
    if(p_st) {
		const uint item_size = p_st->GetItemSize();
        switch(item_size) {
			case 1: result = CMPSIGN(*PTR8(p1), *PTR8(p2)); break;
			case 2: result = CMPSIGN(*PTR16(p1), *PTR16(p2)); break;
			case 4: result = CMPSIGN(*PTR32(p1), *PTR32(p2)); break;
			case 8: result = CMPSIGN(*PTR64(p1), *PTR64(p2)); break;
			default: result = memcmp(p1, p2, item_size); break;
        }
    }
    return result;
}

SSuffixTree::String::String(uint itemSize, uint32 id) : SVector(itemSize, O_ARRAY), ID(id), PhasePosition(0) // @v9.8.4 SArray-->SVector
{
}

SSuffixTree::StringArray::StringArray() : TSArray <SSuffixTree::String> (aryDataOwner|aryEachItem)
{
}

SSuffixTree::StringArray::~StringArray()
{
	freeAll();
}

SSuffixTree::String * FASTCALL SSuffixTree::StringArray::Get(uint pos) const
{
	return (pos < getCount()) ? &at(pos) : 0;
}

const SSuffixTree::String * FASTCALL SSuffixTree::StringArray::GetC(uint pos) const
{
	return (pos < getCount()) ? &at(pos) : 0;
}

int SSuffixTree::StringArray::SearchById(uint32 id, uint * pPos) const
{
	return lsearch(&id, pPos, CMPF_LONG, offsetof(SSuffixTree::String, ID));
}

int SSuffixTree::StringArray::CreateNewItem(const SSuffixTree * pT, uint32 id, uint * pPos)
{
	int    ok = 1;
	uint   ex_pos = 0;
	uint   new_pos = 0;
	THROW(id);
	THROW(SearchById(id, &ex_pos) == 0); // Дублирование идентификатора строки
	{
		SSuffixTree::String new_item(pT->GetItemSize(), id);
		new_pos = getCount();
		THROW(insert(&new_item));
		new_item.setFlag(aryDataOwner, 0); // Снимаем флаг владения данными что бы
			// не разрушить переданный массиву объект при вызове деструктора new_item
	}
	ASSIGN_PTR(pPos, new_pos);
	CATCHZOK
	return ok;
}

//virtual
void FASTCALL SSuffixTree::StringArray::freeItem(void * pItem) { ((SSuffixTree::String *)pItem)->freeAll(); }
//
//
//
SSuffixTree::IndexBase::IndexBase()
{
	Init();
}

SSuffixTree::IndexBase::IndexBase(uint strP, uint startIdx, uint endIdx/*, uint extraIdx*/)
{
	Init();
	StrP = strP;
	StartIdx = startIdx;
	EndIdx = endIdx;
	Flags = 0;
	//P_EndIdx[0] = endIdx;
	//ExtraIdx = extraIdx;
}

void SSuffixTree::IndexBase::Init()
{
	THISZERO();
	//P_EndIdx = &EndIdxLocal;
}

SSuffixTree::IndexBase & FASTCALL SSuffixTree::IndexBase::operator = (const SSuffixTree::IndexBase & rS)
{
	StrP = rS.StrP;
	StartIdx = rS.StartIdx;
	EndIdx = rS.EndIdx;
	Flags = rS.Flags;
	//SETIFZ(P_EndIdx, &EndIdxLocal);
	//*P_EndIdx = *rS.P_EndIdx;
	//ExtraIdx = rS.ExtraIdx;
	return *this;
}
//
//
//
SSuffixTree::PathEnd::PathEnd() : NodeP(0), EdgeP(0), Offset(0)
{
}

void FASTCALL SSuffixTree::PathEnd::SetNode(uint nodeP)
{
	NodeP = nodeP;
	EdgeP = 0;
	Offset = 0;
}

void SSuffixTree::PathEnd::SetEdge(uint edgeP, uint offs)
{
	NodeP = 0;
	EdgeP = edgeP;
	Offset = offs;
}

void SSuffixTree::PathEnd::Advance(const SSuffixTree & rT, uint edgeP)
{
	if(NodeP) {
		if(rT.GetEdgeLen(edgeP) == 1) {
			const SSuffixTree::Edge * p_edge = rT.GetEdge(edgeP);
			assert(p_edge);
			SetNode(p_edge->DestNodeP);
		}
		else
			SetEdge(edgeP, 1);
	}
	else {
		Offset++;
		if(EdgeP && rT.GetEdgeLen(EdgeP) == Offset) {
			const SSuffixTree::Edge * p_edge = rT.GetEdge(EdgeP);
			assert(p_edge);
			SetNode(p_edge->DestNodeP);
		}
	}
}
//
//
//
SSuffixTree::Edge::Edge() : SSuffixTree::IndexBase(), SrcNodeP(0), DestNodeP(0)
{
}
//
//
//
SSuffixTree::Node::Node() : UpEdgeP(0), SfxLinkNodeP(0), EdgeHubP(0)
{
}
//
//
//
SSuffixTree::EdgeHub::EdgeHub() : Count(0)
{
	assert(sizeof(I) >= sizeof(Ptr));
	memzero(I, sizeof(I));
}

SSuffixTree::EdgeHub::~EdgeHub()
{
	Destroy();
}

void SSuffixTree::EdgeHub::Destroy()
{
	if(Count > SIZEOFARRAY(I)) {
		ZDELETE(Ptr);
	}
	Count = 0;
	memzero(I, sizeof(I));
	// Edge_PosList.freeAll();
}

uint SSuffixTree::EdgeHub::GetCount() const
{
	return Count;
}

size_t SSuffixTree::EdgeHub::SizeOf() const
{
	size_t sz = sizeof(*this);
	if(Count > SIZEOFARRAY(I))
		sz += Count * sizeof(*Ptr);
	return sz;
}

uint SSuffixTree::EdgeHub::Get(uint p) const
{
	assert(p < Count);
	if(Count > SIZEOFARRAY(I)) {
		assert(Ptr);
		return Ptr[p];
	}
	else {
		return I[p];
	}
}

int SSuffixTree::EdgeHub::Add(uint item)
{
	int    ok = 1;
	if(Count < SIZEOFARRAY(I)) {
		I[Count++] = item;
	}
	else if(Count == SIZEOFARRAY(I)) {
		uint * _ptr = (uint *)SAlloc::M((Count+1) * sizeof(uint));
		THROW(_ptr);
		memcpy(_ptr, I, sizeof(I));
		_ptr[Count++] = item;
		Ptr = _ptr;
	}
	else {
		uint * _ptr = (uint *)SAlloc::R(Ptr, (Count+1) * sizeof(uint));
		THROW(_ptr);
		_ptr[Count++] = item;
		Ptr = _ptr;
	}
	CATCHZOK
	return ok;
}
//
//
//
SSuffixTree::EdgeHubArray::EdgeHubArray() : TSArray <EdgeHub> (aryDataOwner|aryEachItem)
{
}

SSuffixTree::EdgeHubArray::~EdgeHubArray()
{
	freeAll();
}

uint SSuffixTree::EdgeHubArray::Add_()
{
	uint   new_pos = getCount();
	EdgeHub new_hub;
	return insert(&new_hub) ? new_pos : 0;
}

//virtual
void FASTCALL SSuffixTree::EdgeHubArray::freeItem(void * pItem)
{
	EdgeHub * p_hub = (EdgeHub *)pItem;
	p_hub->Destroy();
}
//
//
//
SSuffixTree::SSuffixTree(uint32 itemSize) : Alphabet(itemSize, 0), ItemSize(itemSize), Flags(0), RootNodeP(0), LastStrAutoId(0)
{
	assert(oneof4(itemSize, 1, 2, 4, 8));
	//P_Phase = 0;
	//Ext = 0;
	{
		// Нулевая позиция списка строк - эксклюзивная
		CreateString(0);
	}
	{
		// Нулевая позиция списка узлов - эксклюзивная
		Node zero_node;
		NodeL.insert(&zero_node);
	}
	{
		// Нулевая позиция списка ребер - эксклюзивная
		Edge zero_edge;
		EdgeL.insert(&zero_edge);
	}
	{
		HubL.Add_();
	}
}

uint FASTCALL SSuffixTree::CreateString(uint32 * pId)
{
	uint   new_pos = 0;
	uint32 new_id = DEREFPTRORZ(pId);
	if(new_id == 0) {
		do {
			new_id = ++LastStrAutoId;
		} while(StrList.SearchById(new_id, 0));
	}
	THROW(StrList.CreateNewItem(this, new_id, &new_pos));
	ASSIGN_PTR(pId, new_id);
	CATCH
		new_pos = 0;
	ENDCATCH
	return new_pos;
}
//
// Descr: Ищет символ алфавита pChr. Если символ найден, то возвращает
//   номер позиции [1..], в противном случае возвращает 0.
//
uint FASTCALL SSuffixTree::SearchChrInAlphabet(const void * pChr)
{
	uint   pos = 0;
	return Alphabet.lsearch(pChr, &pos, PTR_CMPFUNC(SfxTreeChr), 0, this) ? (pos+1) : 0;
}
//
// Descr: Возвращает символ алфавита по номеру позиции [1..]
//
const void * FASTCALL SSuffixTree::GetChrFromAlphabet(uint pos) const
{
	return (pos > 0 && pos <= Alphabet.getCount()) ? Alphabet.at(pos-1) : 0;
}

int SSuffixTree::AddChunkToString(uint strP, const void * pItems, uint itemsCount)
{
	int    ok = 1;
	if(pItems && itemsCount) {
		SSuffixTree::String * p_str = GetStr(strP);
		THROW(p_str);
		THROW(p_str->insertChunk(itemsCount, pItems));
		{
			const uint32 item_size = ItemSize;
			SSuffixTree::String temp_alphabet(item_size, 0);
			THROW(temp_alphabet.insertChunk(itemsCount, pItems));
			temp_alphabet.sort(PTR_CMPFUNC(SfxTreeChr), this);
			{
				const void * p_prev_item = 0;
				uint   prev_item_count = 0;
				for(uint i = 0; i < temp_alphabet.getCount(); i++) {
					const void * p_this_item = temp_alphabet.at(i);
					if(p_prev_item && IsEqChr(p_prev_item, p_this_item))
						prev_item_count++;
					else {
						if(SearchChrInAlphabet(p_this_item)) {
							;
						}
						else {
							THROW(Alphabet.insert(p_this_item));
						}
						p_prev_item = p_this_item;
						prev_item_count = 1;
					}
				}
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

uint FASTCALL SSuffixTree::GetEndIdx(const SSuffixTree::IndexBase * pI) const
{
	if(pI) {
		if(pI->Flags & pI->fPhasePosRef) {
			const SSuffixTree::String * p_str = GetStr(pI->StrP);
			return p_str ? p_str->PhasePosition : 0;
		}
		else {
			return pI->EndIdx;
		}
	}
	else
		return 0;
}

int FASTCALL SSuffixTree::IncrementEndIdx(SSuffixTree::IndexBase * pI)
{
	int    ok = 1;
	if(pI) {
		if(pI->Flags & pI->fPhasePosRef) {
			SSuffixTree::String * p_str = GetStr(pI->StrP);
			if(p_str)
				p_str->PhasePosition++;
			else
				ok = 0;
		}
		else
			pI->EndIdx++;
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL SSuffixTree::UpdateEndIdx(SSuffixTree::IndexBase * pI, uint newPosition)
{
	int    ok = 1;
	if(pI) {
		if(pI->Flags & pI->fPhasePosRef) {
			SSuffixTree::String * p_str = GetStr(pI->StrP);
			if(p_str)
				p_str->PhasePosition = newPosition;
			else
				ok = 0;
		}
		else
			pI->EndIdx = newPosition;
	}
	else
		ok = 0;
	return ok;
}

uint FASTCALL SSuffixTree::CreateNode(/*int nodeIndex*/)
{
	uint   new_node_p = NodeL.getCount();
	assert(new_node_p > 0);
	Node new_node;
	THROW(NodeL.insert(&new_node));
	CATCH
		new_node_p = 0;
	ENDCATCH
	return new_node_p;
}

uint SSuffixTree::CreateEdge(uint srcNodeP, uint destNodeP, uint strP, uint startIdx)
{
	return Helper_CreateEdge(srcNodeP, destNodeP, strP, startIdx, 0, SSuffixTree::IndexBase::fPhasePosRef);
}

uint SSuffixTree::CreateEdge(uint srcNodeP, uint destNodeP, uint strP, uint startIdx, uint endIdx)
{
	return Helper_CreateEdge(srcNodeP, destNodeP, strP, startIdx, endIdx, 0);
}

uint SSuffixTree::Helper_CreateEdge(uint srcNodeP, uint destNodeP, uint strP, uint startIdx, uint endIdx, uint flags)
{
	uint   new_edge_p = EdgeL.getCount();
	assert(new_edge_p > 0);
	SSuffixTree::Edge new_edge;
	new_edge.SrcNodeP = srcNodeP;
	new_edge.DestNodeP = destNodeP;
	new_edge.StrP = strP;
	new_edge.StartIdx = startIdx;
	//
	// For a leaf edge, we make the end index point to the current phase pointer in the tree structure.
	//
	new_edge.EndIdx = endIdx; // P_Phase;
	if(flags & IndexBase::fPhasePosRef)
		new_edge.Flags |= IndexBase::fPhasePosRef;
	THROW(EdgeL.insert(&new_edge));
	{
		Node * p_src_node = GetNode(srcNodeP);
		assert(p_src_node);
		THROW(p_src_node);
		{
            if(!p_src_node->EdgeHubP) {
				THROW(p_src_node->EdgeHubP = HubL.Add_());
            }
			EdgeHub * p_hub = GetEdgeHub(p_src_node->EdgeHubP);
			assert(p_hub);
			THROW(p_hub);
			THROW(p_hub->Add(new_edge_p));
		}
	}
	{
		Node * p_dest_node = GetNode(destNodeP);
		if(p_dest_node)
			p_dest_node->UpEdgeP = new_edge_p;
	}
	CATCH
		new_edge_p = 0;
	ENDCATCH
	return new_edge_p;
}
//
// Descr: Создает листовой узел с индексом leafNodeIdx и ребро от srcNodeP к новому листовому узлу
//
uint SSuffixTree::CreateLeaf(uint srcNodeP, /*int leafNodeIdx,*/ uint strP, uint startIdx)
{
	uint   new_edge_p = 0;
	uint   leaf_node_p = CreateNode(/*leafNodeIdx*/);
	THROW(leaf_node_p);
	THROW(new_edge_p = Helper_CreateEdge(srcNodeP, leaf_node_p, strP, startIdx, 0, IndexBase::fPhasePosRef));
	CATCH
		new_edge_p = 0;
	ENDCATCH
	return new_edge_p;
}

SSuffixTree::Edge * FASTCALL SSuffixTree::GetEdge(uint p) const
{
	return (p && p < EdgeL.getCount()) ? &EdgeL.at(p) : 0;
}

uint FASTCALL SSuffixTree::GetIndexLen(const SSuffixTree::IndexBase & rI) const
{
	uint   len = (GetEndIdx(&rI)-rI.StartIdx+1);
	return len;
}

uint FASTCALL SSuffixTree::GetEdgeLen(uint p) const
{
	return (p && p < EdgeL.getCount()) ? GetIndexLen(EdgeL.at(p)) : 0;
}

SSuffixTree::Node * FASTCALL SSuffixTree::GetNode(uint p) const
{
	return (p && p < NodeL.getCount()) ? &NodeL.at(p) : 0;
}

SSuffixTree::EdgeHub * FASTCALL SSuffixTree::GetEdgeHub(uint p) const
{
	return (p && p < HubL.getCount()) ? &HubL.at(p) : 0;
}

int FASTCALL SSuffixTree::IsLeafNode(uint nodeP) const
{
	const Node * p_node = GetNode(nodeP);
	return p_node ? p_node->IsLeaf() : 0;
}

SSuffixTree::String * FASTCALL SSuffixTree::GetStr(uint p) const
{
	return (p < StrList.getCount()) ? &StrList.at(p) : 0;
}

uint FASTCALL SSuffixTree::GetStrLen(uint p) const
{
	return (p < StrList.getCount()) ? StrList.at(p).getCount() : 0;
}

uint FASTCALL SSuffixTree::GetParentNodeP(Node * pNode) const
{
	uint   node_p = 0;
	if(pNode && pNode->UpEdgeP) {
		const Edge * p_up_edge = GetEdge(pNode->UpEdgeP);
		if(p_up_edge)
			node_p = p_up_edge->SrcNodeP;
	}
	return node_p;
}

uint FASTCALL SSuffixTree::GetParentNodeP(uint nodeP) const
{
	return GetParentNodeP(GetNode(nodeP));
}

int SSuffixTree::IsEqChr(const void * p1, const void * p2) const
{
	int    yes = 0;
	switch(GetItemSize()) {
		case 1:
			yes = BIN(*PTR8(p1) == *PTR8(p2));
			break;
		case 2:
			yes = BIN(*PTR16(p1) == *PTR16(p2));
			break;
		case 4:
			yes = BIN(*PTR32(p1) == *PTR32(p2));
			break;
		case 8:
			yes = BIN(PTR32(p1)[0] == PTR32(p2)[0] && PTR32(p1)[1] == PTR32(p2)[1]);
			break;
		default:
			yes = BIN(memcmp(p1, p2, GetItemSize()) == 0);
			break;
	}
	return yes;
}

int SSuffixTree::StrEq(const SSuffixTree::String * pS1, uint item1, const SSuffixTree::String * pS2, uint item2) const
{
	int    yes = 0;
	//const  uint len1 = pS1 ? pS1->getCount() : 0;
	const  uint len1 = SVectorBase::GetCount(pS1);
	//const  uint len2 = pS2 ? pS2->getCount() : 0;
	const  uint len2 = SVectorBase::GetCount(pS2);
	if(item1 <= len1 && item2 <= len2) {
		// Treat the end-of-string markers separately:
		if(item1 == len1 || item2 == len2) {
			if(item1 == len1 && item2 == len2)
				yes = (pS1 == pS2) ?  1/* Comparing end of identical strings */ : 0/* Comparing end of different strings */;
			else
				yes = 0 /*Comparing end and non-end*/;
		}
		else
			yes = IsEqChr(pS1->at(item1), pS2->at(item2));
	}
	return yes;
}

int SSuffixTree::StrEq(uint str1P, uint item1, uint str2P, uint item2) const
{
	int    yes = 0;
	const SSuffixTree::String * p_str1 = GetStr(str1P);
	const SSuffixTree::String * p_str2 = (str1P == str2P) ? p_str1 : GetStr(str2P);
	return StrEq(p_str1, item1, p_str2, item2);
}

int SSuffixTree::StrEq(const void * pChr, uint strP, uint itemIdx) const
{
	const SSuffixTree::String * p_str = GetStr(strP);
	//const  uint len = p_str ? p_str->getCount() : 0;
	const  uint len = SVectorBase::GetCount(p_str);
	return (itemIdx < len) ? IsEqChr(pChr, p_str->at(itemIdx)) : 0;
}

uint SSuffixTree::StrItemsCommon(uint str1P, uint off1, uint str2P, uint off2, uint maxLen) const
{
	uint   result = 0;
	const SSuffixTree::String * p_str1 = GetStr(str1P);
	const SSuffixTree::String * p_str2 = (str1P == str2P) ? p_str1 : GetStr(str2P);
	//const  uint len1 = p_str1 ? p_str1->getCount() : 0;
	const  uint len1 = SVectorBase::GetCount(p_str1);
	//const  uint len2 = p_str2 ? p_str2->getCount() : 0;
	const  uint len2 = SVectorBase::GetCount(p_str2);
	if(off1 <= len1 && off2 <= len2) {
		const uint len = MIN(MIN((len1+1) - off1, (len2+1) - off2), maxLen);
		while(result < len && StrEq(p_str1, off1 + result, p_str2, off2 + result))
			result++;
	}
	return result;
}

uint SSuffixTree::FindEdgeWithStartitem(uint nodeP, uint strP, uint index) const
{
	uint   result_edge_p = 0;
	const SSuffixTree::Node * p_node = GetNode(nodeP);
	if(p_node && index <= GetStrLen(strP)) { //D0("Invalid input\n");
		const EdgeHub * p_hub = GetEdgeHub(p_node->EdgeHubP);
		const uint _c = p_hub ? p_hub->GetCount() : 0;
		for(uint i = 0; !result_edge_p && i < _c; i++) {
			const uint edge_p = p_hub->Get(i);
			const SSuffixTree::Edge * p_item = GetEdge(edge_p);
			if(p_item && StrEq(p_item->StrP, p_item->StartIdx, strP, index))
				result_edge_p = edge_p;
		}
	}
	return result_edge_p;
}

uint SSuffixTree::FindEdgeWithStartitem(uint nodeP, const void * pChr) const
{
	uint   result_edge_p = 0;
	const SSuffixTree::Node * p_node = GetNode(nodeP);
	if(p_node && pChr) { //D0("Invalid input\n");
		const EdgeHub * p_hub = GetEdgeHub(p_node->EdgeHubP);
		const uint _c = p_hub ? p_hub->GetCount() : 0;
		for(uint i = 0; !result_edge_p && i < _c; i++) {
			const uint edge_p = p_hub->Get(i);
			const SSuffixTree::Edge * p_item = GetEdge(edge_p);
			if(p_item && StrEq(pChr, p_item->StrP, p_item->StartIdx))
				result_edge_p = edge_p;
		}
	}
	return result_edge_p;
}
//
// Descr: follows an arbitrary string through the tree.
// @tree: tree to query.
// @node: node to start at.
// @string: the string to follow.
// @end: result argument, see below.
//
// The function follows the @skipstring in @tree starting from @node,
// reporting where in the tree the string ends through the @end
// result pointer.
//
// Returns: number of string items successfully matched.
//
uint SSuffixTree::FollowStringSlow(uint nodeP, uint strP, SSuffixTree::PathEnd & rEnd)
{
	uint   items_done = 0;
	if(!nodeP) {
		memzero(&rEnd, sizeof(PathEnd)); //D0("Invalid input.\n");
	}
	else {
		uint   items_todo = GetStrLen(strP)+1; // (rString.getCount()+1);
		//
		// Find p_edge where our string starts, making use of the fact
		// that no two out-edges from a pNode can start with the same character.
		//
		uint edge_p = 0;
		while(items_todo > 0 && (edge_p = FindEdgeWithStartitem(nodeP, strP, items_done)) != 0) {
			const Edge * p_edge = GetEdge(edge_p);
			assert(p_edge);
			const uint common = StrItemsCommon(p_edge->StrP, p_edge->StartIdx, strP, items_done, items_todo);
			const uint edge_len = GetEdgeLen(edge_p);
			if(common < edge_len) {
				//D1("Mismatch in p_edge at %s\n", (const char *)lst_debug_print_substring(rString, 0, items_done + common, 0, temp_buf));
				rEnd.SetEdge(edge_p, common);
				return items_done + common;
			}
			else {
				nodeP = p_edge->DestNodeP;
				items_done += edge_len;
				items_todo -= edge_len;
			}
		}
		rEnd.SetNode(nodeP);
	}
	return items_done;
}
//
// Descr: follows an existing string in the tree, using skip/count.
// @tree: tree to query.
// @node: node to start at.
// @skipstring: the string to follow.
// @end: result argument, see below.
//
// The function follows the @skipstring in @tree starting from @node,
// reporting where in the tree the string ends through the @end
// result pointer.
//
void SSuffixTree::FollowString(uint nodeP, const SSuffixTree::IndexBase & rSkipString, SSuffixTree::PathEnd & rEnd)
{
	uint items_done = 0;
	SString temp_buf, temp_buf2;
	if(nodeP) { // D0("Invalid input.\n");
		//D3("Overlaying p_string %s at pNode %u, empty: %i\n", (const char *)pSkipString->Print(temp_buf), pNode->id, (pSkipString->StartIndex == LST_EMPTY_STRING));
		//
		// We need to figure out how many p_string items we need to walk down in
		// the tree so that we can then extend by the next item.
		//
		if(rSkipString.StartIdx == UINT_MAX/*LST_EMPTY_STRING*/) {
			rEnd.SetNode(nodeP); //D0("Empty p_string -- nothing to follow.\n");
		}
		else {
			const uint str_p = rSkipString.StrP;
			uint items_todo = GetIndexLen(rSkipString);
			if(items_todo == 0) {
				rEnd.SetNode(nodeP);
			}
			else {
				//
				// Find p_edge where our p_string starts, making use of the fact
				// that no two out-edges from a pNode can start with the same character.
				//
				uint   edge_p = 0;
				while(items_todo > 0) {
					edge_p = FindEdgeWithStartitem(nodeP, str_p, rSkipString.StartIdx + items_done);
					if(!edge_p) {
						rEnd.SetNode(nodeP);
						return;
					}
					else {
						const uint edge_len = GetEdgeLen(edge_p);
						//
						// Follow edges in tree, emplying the Skip/Count Trick as per Gusfield.
						// When the p_string we're looking up is longer than the p_edge's label,
						// we can just skip this p_edge:
						//
						if(items_todo >= edge_len) {
							items_todo -= edge_len;
							items_done += edge_len;
							const Edge * p_edge = GetEdge(edge_p);
							if(items_todo == 0) {
								rEnd.SetNode(p_edge->DestNodeP); //D2("Skipped to pNode %u, last internal %u\n", p_edge->P_DestNode->id, pNode->id);
								return;
							}
							else {
								nodeP = p_edge->DestNodeP; //D1("Skipping to pNode %u.\n", pNode->id);
								continue;
							}
						}
						else {
							//
							// When the p_string is shorter than the p_edge, we need to compare
							// the strings and figure out where we stop within the p_edge.
							// This will need a new p_edge as per extension Rule 2.
							//
							break;
						}
					}
				}
				{
					const Edge * p_edge = GetEdge(edge_p);
					assert(p_edge);
					const uint common = StrItemsCommon(p_edge->StrP, p_edge->StartIdx, str_p, rSkipString.StartIdx + items_done, items_todo);
					//p_edge->Print(temp_buf);
					rEnd.SetEdge(edge_p, common);
				}
			}
		}
	}
}

int SSuffixTree::InsertString(uint stringP)
{
	int    ok = 1;

	PathEnd end;
	Node * p_start_leaf = 0;
	uint   inner_node_p = 0;
	uint   prev_new_node_p = 0;
	uint   new_inner_node_p = 0;

	uint   j;
	uint   stop_extensions = 0;
	uint   last_extension = 0;
	uint   find_skipstring = 1;
	SString outp_buf;
	//D1("Inserting pString %s\n", (const char *)rString.ToStr(outp_buf));
	{
		//
		// stree_next_phase - Trick 3 management.
		// @tree: tree to advance phase pointer in.
		//
		// This function takes care of allowing Trick 3 to work correctly
		// when multiple strings are inserted into the tree, and helps avoiding
		// the O(m) overhead to create a true suffix tree from the intermediate
		// one Gusfield mentions in section 6.1.6. We don't exchange any labels
		// when we consider edge label ends "final", we rather start using
		// different pointers to end offsets from now on, not touching the old
		// ones any more in the future.
		//
		/*
		uint * p_phase = 0;
		THROW(p_phase = new uint);
		Phases.insert(p_phase);
		P_Phase = p_phase;
		*/
		//D0("New tree extension pointer.\n");
	}
	//
	// First and subsequent pString insertions are handled slightly
	// differently -- first one is built from scratch, the others
	// are built after matching them onto the current tree as much
	// as possible, and then proceeding as usual.
	//
	SSuffixTree::String * p_bstring = GetStr(stringP);
	assert(p_bstring);
	p_bstring->PhasePosition = 0;
	if(RootNodeP == 0) { // Первая добавленная строка
		//D0("No strings in tree yet.\n");
		RootNodeP = CreateNode(/*-1*/);
		THROW(RootNodeP);
		{
			uint   edge_p = CreateLeaf(RootNodeP, /*0,*/ stringP, 0);
			THROW(edge_p);
			{
				Edge * p_edge = GetEdge(edge_p);
				THROW(p_edge);
				p_start_leaf = GetNode(p_edge->DestNodeP);
				//
				// Construct first tree -- simply enter a single edge with a single-item label.
				//
				//D0("Phase 0 started.\n");
				end.SetEdge(p_start_leaf->UpEdgeP, 0);
				last_extension = 0;
				//D_PRINT_TREE(tree);
				// Skip to phase 1:
				p_bstring->PhasePosition = 1;
			}
		}
	}
	else {
		//
		// Follow new pString through tree, hence finding the number of phases we can skip:
		//
		p_bstring->PhasePosition = FollowStringSlow(RootNodeP, stringP, end);
		last_extension = -1;
		find_skipstring = 0;
		//D1("Starting new pString at phase %u\n", p_bstring->PhasePosition);
	}
	//
	// Following Ukkonen's algorithm, we insert the new pString
	// of length |m| in m "phases", constructing tree p_bstring->PhasePosition+1 from
	// tree p_bstring->PhasePosition. Each phase p_bstring->PhasePosition+1 consists of p_bstring->PhasePosition+1 "extensions", one
	// for each suffix in the pString s[1,p_bstring->PhasePosition+1].
	//
	// By including the last pString item (our end-of-pString marker),
	// we automatically avoid the issues of having suffixes that are prefixes of other suffixes.
	//
	//assert(p_bstring->PhasePosition != 0);
	const uint string_length = GetStrLen(stringP);
	for(; p_bstring->PhasePosition < string_length; p_bstring->PhasePosition++) {
		//D2("Phase %i started ------------------------- %s\n", p_bstring->PhasePosition, (const char *)lst_debug_print_substring(rString, 0, p_bstring->PhasePosition-1, p_bstring->PhasePosition, outp_buf));
		//*P_Phase = i;
		uint   use_end = stop_extensions ? 1 : 0;
		//
		// Now do the remaining extensions. We don't start at index 0
		// with extensions but rather implement speedup Trick 3 as per Gusfield.
		// j - extension of Ukkonen algorithm
		//
		for((j = (last_extension+1)), stop_extensions = 0; j < (p_bstring->PhasePosition+1) && !stop_extensions; j++) {
			//Ext = j;
			//D2("Phase %u, extension %i started.\n", p_bstring->PhasePosition, j);
			//
			// Get the node from which we start to walk down,
			// either found via suffix links or because it's the root:
			//
			class _IndexTemp : public IndexBase {
			public:
				_IndexTemp(uint strP, uint startIdx, uint endIdx, uint extraIdx) : IndexBase(strP, startIdx, endIdx)
				{
					ExtraIdx = extraIdx;
				}
				//
				// For the appended single string items, it's convenient to have the index of that single item handy:
				//
				uint   ExtraIdx;
			};
			_IndexTemp skipstring(stringP, 0, 0, p_bstring->PhasePosition);
			if(use_end) {
				use_end = 0; //D0("Re-using last phase's pString end\n");
			}
			else if(find_skipstring) {
				//
				// Finds the string range we need to cross up to previous node.
				//
				// The function finds the string range we need to jump over until
				// we get back up to a node that has a suffix link. If that node is
				// the root, the upstring will be empty.
				//
				// Result: The node we arrive at. Also, the string range is returned @skipstring.
				//
				uint   node_p = 0;
				{
					const Node * p_node = GetNode(end.NodeP);
					if(p_node) {
						if(p_node->SfxLinkNodeP) {
							//
							// The node we ended at already has a suffix link,
							// so we need to do nothing. Mark the pSkipString as empty:
							//
							skipstring.StartIdx = UINT_MAX; // LST_EMPTY_STRING
							node_p = p_node->SfxLinkNodeP; //D0("Suffix link at start node\n");
						}
						//
						// If the node doesn't have a suffix link directly, we must
						// hop up over the complete edge's string. If we pEnd up at
						// the root, the caller must descend as in the naive algorithm,
						// otherwise the pSkipString is just whatever the string attached
						// to the edge is:
						//
						else if(end.NodeP == RootNodeP) {
							skipstring.StartIdx = UINT_MAX/* LST_EMPTY_STRING */;
							node_p = RootNodeP; //D0("End node is root\n");
						}
						else {
							const uint parent_node_p = GetParentNodeP(end.NodeP);
							if(parent_node_p == RootNodeP)
								node_p = RootNodeP; //D0("Parent is root\n");
							else {
								const Edge * p_edge = GetEdge(p_node->UpEdgeP);
								assert(p_edge);
								if(p_edge)
									skipstring.IndexBase::operator = (*p_edge);
								//
								// Follow the node up the edge, and cross over across
								// the suffix link. Then return the node we arrive at.
								//
								//D0("Suffix link up from node\n");
								const Node * p_parent_node = GetNode(parent_node_p);
								if(p_parent_node)
									node_p = p_parent_node->SfxLinkNodeP;
							}
						}
					}
					else {
						// Okay -- the funkier case: we start in the middle of an edge.
						const Edge * p_edge = GetEdge(end.EdgeP);
						assert(p_edge);
						Node * p_parent_node = GetNode(p_edge->SrcNodeP);
						if(p_edge->SrcNodeP == RootNodeP)
							node_p = RootNodeP; //D0("Edge src is root\n");
						else {
							skipstring.StrP = p_edge->StrP;
							skipstring.StartIdx = p_edge->StartIdx;
							UpdateEndIdx(&skipstring, skipstring.StartIdx + end.Offset - 1);
							node_p = p_parent_node->SfxLinkNodeP;
						}
					}
				}
				if(node_p == RootNodeP) {
					if(skipstring.StartIdx != UINT_MAX/*LST_EMPTY_STRING*/) {
						//D0("Starting at root\n");
						//
						// It's the root node -- just follow the path down in the tree as in the naive algorithm.
						//
						skipstring.StrP = stringP;
						skipstring.StartIdx = j;
						UpdateEndIdx(&skipstring, p_bstring->PhasePosition-1);
						if((p_bstring->PhasePosition-1) < j)
							skipstring.StartIdx = UINT_MAX; // LST_EMPTY_STRING;
					}
				}
				else {
					//D0("Using suffix link\n");
					//
					// It's not the root node -- exploit suffix
					// links as much as possible. stree_get_skipstring()
					// has already filled in stuff for us.
					//
				}
				FollowString(node_p, skipstring, end);
			}
			find_skipstring = 1;
			new_inner_node_p = 0;
			inner_node_p = 0;
			//
			// Now extend the found path in the tree by the new character in this phase:
			//
			if(end.NodeP) {
				//
				// We followed the path up to a node. If that node is a leaf,
				// we're done per Extension Rule 1. Otherwise we need to find
				// out if our new character is somewhere on the out-edges
				// of that node, and if not, hook in a new edge.
				//
				inner_node_p = end.NodeP;
				{
					//
					// The block below extends the tree by a single string item, down from
					// a node within the tree. The point of insertion is given through @end.
					//
					// Returns: 1 in @stop_extensions if Rule 3 (see Gusfield) applied and we
					// can hence stop extensions for the current phase.
					//
					if(IsLeafNode(end.NodeP)) {
						//D0("Rule 1 -- extending label.\n");
						const Node * p_node = GetNode(end.NodeP);
						Edge * p_up_edge = p_node ? GetEdge(p_node->UpEdgeP) : 0;
						THROW(p_up_edge);
						THROW(IncrementEndIdx(p_up_edge));
						end.SetEdge(p_node->UpEdgeP, GetEdgeLen(p_node->UpEdgeP) - 2);
					}
					else {
						uint   edge_p = FindEdgeWithStartitem(end.NodeP, stringP, skipstring.ExtraIdx);
						if(!edge_p) {
							//
							// Extension Rule 2:
							//
							THROW(edge_p = CreateLeaf(end.NodeP, /*Ext,*/ stringP, skipstring.ExtraIdx)); //D0("Rule 2 -- adding edge.\n");
						}
						else {
							//
							// Otherwise it's Extension Rule 3, so we do nothing,
							// but only mark that we've applied that rule so we can speed things up.
							//
							end.Advance(*this, edge_p); //D0("Rule 3 -- marked for stop.\n");
							stop_extensions = 1;
						}
					}
				}
			}
			else {
				//
				// We followed the path down to somewhere within an edge label.
				// Now we need to check if the item following the common part in the label is what we want to add;
				// that's a case of Extension Rule 3. Otherwise we need to split
				// the edge and hook in a new edge with the new character.
				//
				assert(end.EdgeP);
				{
					//
					// The block below extends the tree by a single string item, down from within
					// an edge in the tree. The point of insertion is given through @end.
					//
					// Returns: 1 in @stop_extensions if Rule 3 (see Gusfield) applied and we
					// can hence stop extensions for the current phase.
					//
					//uint   new_inner_node_p = 0;
					uint   new_edge_p = 0;
					Edge * p_end_edge = GetEdge(end.EdgeP);
					assert(p_end_edge);
					THROW(p_end_edge);
					if(StrEq(stringP, skipstring.ExtraIdx, p_end_edge->StrP, p_end_edge->StartIdx + end.Offset)) {
						//D2("Rule 3 within edge %u-%u -- marked for stop.\n", r_end_edge.P_SrcNode->id, r_end_edge.P_DestNode->id);
						end.Advance(*this, 0);
						stop_extensions = 1;
					}
					else {
						THROW(new_inner_node_p = CreateNode(/*-1*/));
						uint   old_node_p = p_end_edge->DestNodeP;
						//
						// Carefully carefully carefully -- when we split a leaf edge,
						// we need to figure out what kind of pEnd index to use (the edge-local or global one).
						// It's not enough to check whether it's a leaf or not -- it could be a leaf created for a previous pString.
						// So only make the pEnd index point back into the pTree if it was pointing there
						// originally anyway. However, pTree->phase changes over time, so we must not use that.
						//
						if(IsLeafNode(old_node_p)) {
							if(p_end_edge->Flags & IndexBase::fPhasePosRef) {
								THROW(new_edge_p = CreateEdge(new_inner_node_p, old_node_p, p_end_edge->StrP, p_end_edge->StartIdx + end.Offset));
							}
							else {
								THROW(new_edge_p = CreateEdge(new_inner_node_p, old_node_p, p_end_edge->StrP, p_end_edge->StartIdx + end.Offset, p_end_edge->EndIdx));
							}
						}
						else {
							THROW(new_edge_p = CreateEdge(new_inner_node_p, old_node_p, p_end_edge->StrP, p_end_edge->StartIdx + end.Offset, GetEndIdx(p_end_edge)));
						}
						{
							//
							// После создания новых ребер объект по указателю p_end_edge мог измениться - необходимо инициализировать снова
							//
							p_end_edge = GetEdge(end.EdgeP);
							//
							//p_end_edge->P_EndIdx = &p_end_edge->EndIdxLocal;
							//p_end_edge->P_EndIdx[0] = p_end_edge->StartIdx + end.Offset - 1;
							p_end_edge->Flags &= ~Edge::fPhasePosRef;
							p_end_edge->EndIdx = p_end_edge->StartIdx + end.Offset - 1;
							p_end_edge->DestNodeP = new_inner_node_p;
							{
								Node * p_new_inner_node = GetNode(new_inner_node_p);
								THROW(p_new_inner_node);
								p_new_inner_node->UpEdgeP = end.EdgeP;
							}
							//
							// Now add another edge to the new node inserted, and label it with the remainder of the pString.
							//
							THROW(new_edge_p = CreateLeaf(new_inner_node_p, /*Ext,*/ stringP, skipstring.ExtraIdx));
						}
					}
				}
			}
			//
			// Now take care of suffix links: if we've created a new inner
			// node in the last extension, create a suffix link to either
			// the last inner node we've encountered above, or a new inner node, if we've created one.
			//
			if(prev_new_node_p) {
				Node * p_prev_new_node = GetNode(prev_new_node_p);
				p_prev_new_node->SfxLinkNodeP = NZOR(new_inner_node_p, inner_node_p);
			}
			prev_new_node_p = new_inner_node_p;
			//D_PRINT_TREE(tree);
		}
		//
		// Remember how far we got for the next phase. Also,
		// repeat the last extension if we aborted due to Rule 3.
		//
		last_extension = j - 1 - stop_extensions;
	}
	Flags |= fNeedVisitorUpdate;
	CATCHZOK
	return ok;
}

void SSuffixTree::OutputChr(const SSuffixTree::String * pStr, uint chrIdx, long flags, SString & rBuf) const
{
	if(pStr) {
		if(chrIdx == pStr->getCount()) {
			rBuf.CatChar('{').Cat("E").CatChar('}');
		}
		else if(chrIdx > pStr->getCount()) {
			rBuf.CatChar('{').Cat("O").CatChar('}');
		}
		else {
			const void * p_chr = pStr->at(chrIdx);
			switch(ItemSize) {
				case 1:
					rBuf.CatChar(*(char *)p_chr);
					break;
				default:
					{
						for(uint i = 0; i < ItemSize; i++) {
							rBuf.CatHex(PTR8(p_chr)[0]);
						}
					}
					break;
			}
		}
	}
	else {
		rBuf.CatChar('{').Cat("Z").CatChar('}');
	}
}

void SSuffixTree::OutputChrChunk(const SSuffixTree::IndexBase & rRange, long flags, SString & rBuf) const
{
	const String * p_str = GetStr(rRange.StrP);
	const uint end_idx = GetEndIdx(&rRange);
	if(p_str) {
		rBuf.CatEq("str", (ulong)rRange.StrP).CatChar('[').Cat(rRange.StartIdx).CatChar('-').Cat(end_idx).CatChar(']');
		rBuf.Space();
		for(uint i = rRange.StartIdx; i <= end_idx; i++) {
			OutputChr(p_str, i, flags, rBuf);
		}
	}
	else {
		rBuf.CatEq("zerostr", (ulong)rRange.StrP).CatChar('[').Cat(rRange.StartIdx).CatChar('-').Cat(end_idx).CatChar(']');
	}
}

void SSuffixTree::OutputEdge(uint edgeP, long flags, SString & rBuf) const
{
	const Edge * p_edge = GetEdge(edgeP);
	if(p_edge) {
		rBuf.CatEq("edge", (ulong)edgeP).CatDiv(':', 2);
		OutputChrChunk(*p_edge, flags, rBuf);
	}
	else {
		rBuf.CatEq("zeroedge", (ulong)edgeP);
	}
}

void SSuffixTree::OutputNode(uint nodeP, uint tabs, long flags, SString & rBuf) const
{
	const Node * p_node = GetNode(nodeP);
	if(tabs)
		rBuf.Tab(tabs);
	if(p_node) {
		rBuf.CatEq("node", (ulong)nodeP).CatDiv(':', 2);
		{
            const EdgeHub * p_hub = GetEdgeHub(p_node->EdgeHubP);
            if(p_hub) {
				const uint ec = p_hub->GetCount();
				if(ec) {
					rBuf.CR();
					for(uint i = 0; i < ec; i++) {
						const uint edge_p = p_hub->Get(i);
						rBuf.Tab(tabs+1);
						OutputEdge(edge_p, flags, rBuf);
						const Edge * p_edge = GetEdge(edge_p);
						if(p_edge) {
							rBuf.CR();
							OutputNode(p_edge->DestNodeP, tabs+2, flags, rBuf); // @recursion
						}
						rBuf.CR();
					}
				}
				else {
					rBuf.Cat("empty hub");
				}
            }
            else {
				rBuf.Cat("leaf");
            }
		}
	}
	else {
		rBuf.CatEq("zeronode", (ulong)nodeP);
	}
}

void SSuffixTree::CalcStat(Stat * pStat) const
{
	Stat   stat;
	uint   i;
	MEMSZERO(stat);
	stat.StringCount = StrList.getCount();
	for(i = 0; i < stat.StringCount; i++) {
		const String & r_item = StrList.at(i);
		const uint sl = r_item.getCount();
		stat.StringLen += sl;
		stat.MemStr += sl * r_item.getItemSize();
		stat.MemStr += StrList.getItemSize();
	}
	//
	stat.NodeCount = NodeL.getCount();
	double node_wide_sum = 0;
	for(i = 0; i < stat.NodeCount; i++) {
		const Node & r_item = NodeL.at(i);
		const EdgeHub * p_hub = GetEdgeHub(r_item.EdgeHubP);
		if(p_hub) {
			const uint hc = p_hub->GetCount();
			node_wide_sum += (double)hc;
			SETMAX(stat.NodeWideMax, hc);
		}
		if(r_item.SfxLinkNodeP)
			stat.NodeWithSfxLinkCount++;
	}
	stat.MemNode = NodeL.getItemSize() * stat.NodeCount;
	stat.NodeWideAvg = fdivnz(node_wide_sum, stat.NodeCount);
	//
	stat.EdgeCount = EdgeL.getCount();
	stat.MemEdge = stat.EdgeCount * EdgeL.getItemSize();
	//
	stat.EdgeHubCount = HubL.getCount();
	for(i = 0; i < stat.EdgeHubCount; i++) {
		const EdgeHub & r_item = HubL.at(i);
		stat.MemEdgeHub += r_item.SizeOf();
	}
	ASSIGN_PTR(pStat, stat);
}

void SSuffixTree::DebugOutputStat(SString & rBuf) const
{
	Stat s;
	CalcStat(&s);
	rBuf.CatEq("StringCount", s.StringCount).Space();
	rBuf.CatEq("StringLen", s.StringLen).Space();
	rBuf.CatEq("NodeCount", s.NodeCount).Space();
	rBuf.CatEq("EdgeCount", s.EdgeCount).Space();
	rBuf.CatEq("EdgeHubCount", s.EdgeHubCount).Space();
	rBuf.CatEq("NodeWideMax", s.NodeWideMax).Space();
	rBuf.CatEq("NodeWideAvg", s.NodeWideAvg, MKSFMTD(0, 6, 0)).Space();
	rBuf.CatEq("NodeWithSfxLinkCount", s.NodeWithSfxLinkCount).Space();
	rBuf.CatEq("MemStr", s.MemStr).Space();
	rBuf.CatEq("MemEdge", s.MemEdge).Space();
	rBuf.CatEq("MemNode", s.MemNode).Space();
	rBuf.CatEq("MemEdgeHub", s.MemEdgeHub);
}

void SSuffixTree::DebugOutput(long flags, SString & rBuf)
{
	OutputNode(RootNodeP, 0, flags, rBuf);
}
//
//
//
int SLAPI TestSuffixTree()
{
	int    ok = 1;
	{
		MemLeakTracer mlt;
		{
			SSuffixTree st(1);
			{
				//const char * p_string = "mississippi";
				//const char * p_string = "писатель, ещё при жизни признанный главой русской литературы. творчество льва толстого ознаменовало новый этап в русском и мировом реализме, выступив мостом между классическим романом xix века и литературой xx века. лев толстой оказал сильное влияние на эволюцию европейского гуманизма, а также на развитие реалистических традиций в мировой литературе. произведения льва толстого многократно экранизировались и инсценировались в ссср и за рубежом; его пьесы ставились на сценах всего мира.";
#if	0 // {
				{
					SFile f_in("D:\\Papyrus\\Src\\PPTEST\\DATA\\rustext.txt", SFile::mRead|SFile::mBinary);
					THROW(f_in.IsValid());
					{
						uint32 str_id = 0;
						uint str_p = st.CreateString(&str_id);

						STempBuffer in_buf(8*1024);
						size_t actual_size = 0;
						THROW(in_buf.IsValid());
						THROW(str_p);
                        while(f_in.Read(in_buf, in_buf.GetSize(), &actual_size) > 0) {
							THROW(st.AddChunkToString(str_p, in_buf, actual_size));
                        }
                        THROW(st.InsertString(str_p));
					}
					{
						SFile f_out("TestSuffixTree_large", SFile::mWrite);
						SString outp_buf;
						/*
						st.DebugOutput(0, outp_buf);
						f_out.WriteLine(outp_buf);
						*/
						st.DebugOutputStat(outp_buf.Z().CR());
						f_out.WriteLine(outp_buf);
					}
				}
#else // }{
				{
					{
						uint32 str_id = 0;
						uint str_p = st.CreateString(&str_id);
						const char * p_string = "mississippi";
						const size_t len = strlen(p_string);
						THROW(str_p);
						THROW(st.AddChunkToString(str_p, p_string, len));
						THROW(st.InsertString(str_p));
					}
					{
						uint32 str_id = 0;
						uint str_p = st.CreateString(&str_id);
						const char * p_string = "abc";
						const size_t len = strlen(p_string);
						THROW(str_p);
						THROW(st.AddChunkToString(str_p, p_string, len));
						THROW(st.InsertString(str_p));
					}
					{
						SFile f_out("TestSuffixTree_short", SFile::mWrite);
						SString outp_buf;
						st.DebugOutput(0, outp_buf);
						f_out.WriteLine(outp_buf);
						st.DebugOutputStat(outp_buf.Z().CR());
						f_out.WriteLine(outp_buf);
					}
				}
#endif // }
			}
			CATCHZOK
		}
	}
	return ok;
}
