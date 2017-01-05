// SFXTREE.CPP
// Copyright (c) A.Sobolev 2006
//
#include <slib.h>

struct SfxTreeNode {
	struct Item {
		uint8  C;        // Символ, связанный с этим элементом
		uint32 OutEdge;  // Дуга, выходящая из этого элемента узла
	};
	static size_t GetSize(size_t chrCount, size_t tagCount)
	{
		return (sizeof(SfxTreeNode) + chrCount * sizeof(SfxNodeItem) + (tagCount * sizeof(uint32)));
	}
	void * operator new (size_t sz, size_t chrSpace, size_t tagSpace)
	{
		if(chrSpace < 2)
			chrSpace = 2;
		else if(chrSpace > 256)
			chrSpace = 256;
		size_t s = GetSize(chrSpace, tagSpace);
		SfxTreeNode * p = calloc(s, 1);
		if(p) {
			memzero(p, s);
			p->ChrSpace = chrSpace;
			p->TagSpace = tagSpace;
		}
		return p;
	}
	int    Copy(const SfxTreeNode & s)
	{
		if((ChrSpace * sizeof(SfxNodeItem) + TagSpace * sizeof(uint32)) >= s.GetSize()) {
			Flags = s.Flags;
			MaxChar = s.MaxChar;
			ChrCount = s.ChrCount;
			TagCount = s.TagCount;
			SfxLink = s.SfxLink;
			InEdge = s.InEdge;
			memcpy(this+1, (&s)+1, s.GetSize());
			return 1;
		}
		else
			return 0;
	}
	//
	// Descr: Возвращает полный размер узла в байтах
	//
	size_t GetSize() const
	{
		return SfxTreeNode::GetSize(ChrCount, TagCount);
	}
	uint   GetCount() const
	{
		return (ChrCount + TagCount);
	}
	//
	// Descr: Ищет в узле элемент, помеченный символом c.
	// Returns:
	//   0  - искомый элемент не найден
	//   !0 - искомый элемент найден.
	//
	int    Search(uint8 c, uint32 * pEdgeIdx) const;
	enum {
		fHash   = 0x01,   // Узел построен как хэш-таблица. Максимальный символ равен MaxChar.
		fSorted = 0x02    // Узел построен как сортированный массив
	};
	uint8  Flags;
	uint8  MaxChar;   //
	uint8  ChrCount;  // Количество действительных элементов в узле
	uint8  ChrSpace;  // Количество символьных элементов, для которых распределено пространство
	uint16 TagCount;  // Количество элементов-тэгов
	uint16 TagSpace;  // Количество тэгов, для которых распределено пространство
	uint32 SfxLink;   // Суффиксная связь узла. Если (Flags & fFree) то этот член
		// указывает на следующий свободный узел в пуле.
	uint32 InEdge;    // Дуга, из которой выходит узел
};

struct SfxTreeTag {
	uint32 Sub;
	uint32 Sfx;
};
//
// Позиция на дуге
//
struct SfxTreeEdgePos {
	uint32 Idx;
	uint32 Pos;
};

struct SfxTreeEdge {
	enum {
		fLeaf = 0x0001, // Листовая дуга. В этом случае OutNode указывает на позицию
			// тэга суффикса (SfxTreeTag).
		fTag  = 0x0002  // Дуга является меткой суффикса. То есть ассоциирована с виртуальным
			// терминальным символом. Соответственно, ей не сопоставлено ни одного реального
			// символа строки.В этом случае: Start - позиция символа в подстроке Len.
	};
	uint32 Start;   // Первый символ последовательности
	uint32 End;     // Последний символ последовательности
	uint32 InNode;  // Узел, из которого дуга выходит
	uint32 OutNode; // Исходящий узел
	uint16 Flags;
};

class SfxTreeEdgeList {
public:
	SfxTreeEdgeList();
	~SfxTreeEdgeList();
	int    Add(uint32 * pPos, const SfxTreeEdge &);
private:
	//
	// Первая дуга является листом наибольшего суффикса (tag=1,1).
	//
	TSArray <SfxTreeEdge> EdgeList;
	TSArray <SfxTreeTag> TagList;
};

class SfxTreeNodeList {
public:
	SfxTreeNodeList();
	~SfxTreeNodeList();
	//
	// Descr: Возвращает указатель на структуру узла дерева.
	//   Если вызывающая функция, изменяет объект по этому указателю,
	//   то она должна после завершения действий по изменению вызвать
	//   SfxTreeNodeList::Commit(uint32) с тем же индексом узла.
	//
	SfxTreeNode * Get(uint32);
	int    Commit(uint32 pos, SfxTreeNode * pNode);
	int    GetHead(uint32 * pPos);
	int    Add(uint32 * pPos);
	int    Expand(uint32 pos, uint8 c, uint32 outEdgePos);

	int    SetInEdge(uint32 nodePos, uint32 inEdgePos);
private:
	SfxTreeNode * AllocNode(SfxTreeNode * pNode, uint numItems);
	TSCollection <SfxTreeNode> List;
};

SfxTreeNode * SfxTreeNodeList::AllocNode(SfxTreeNode * pNode, uint numItems)
{
	if(pNode == 0)
		pNode = new SfxTreeNode(numItems, 0);
	if(numItems <= pNode->ChrSpace)
		return pNode;
	else {
		SfxTreeNode * p_temp = new SfxTreeNode(numItems, 0);
		p_temp->Copy(pNode);
		delete pNode;
		pNode = p_temp;
	}
	return pNode;
}

SfxTreeNode * SfxTreeNodeList::Get(uint32 pos)
{
	if(pos < List.getCount())
		return List.at(pos);
	else
		return 0;
}

int SfxTreeNodeList::Commit(uint32 pos, SfxTreeNode * pNode)
{
	return 1;
}

int SfxTreeNodeList::Add(uint32 * pPos)
{
	int    ok = 0;
	SfxTreeNode * p_node = AllocNode(0, 2);
	if(p_node) {
		if(List.insert(p_node)) {
			ASSIGN_PTR(pPos, List.getCount());
			ok = 1;
		}
	}
	else
		SLibError = SLERR_NOMEM;
	return ok;
}

int SfxTreeNodeList::Expand(uint32 pos, uint8 c, uint32 outEdgePos)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	SfxTreeNode * p_node = Get(pos);
	THROW(p_node);
	THROW_V(p_node->Search(c) == 0, SLERR_SFXTRE_NODEHASCHR); // @debug
	THROW(p_node = AllocNode(p_node, p_node->GetCount() + 1));
	THROW(p_node->AddChrItem(c, outEdgePos));
	THROW(Commit(pos, p_node));
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

class SfxTree {
public:
	SfxTree();
	~SfxTree();
	int    Add(uint32 tag, const uint8 * pData, size_t len);
	uint8  GetChar(uint32 pos);
private:
	int    SplitEdge(SfxTreeEdge * pEdge, uint32 pos, uint8 c);
	int    CreateLeafNode(uint32 inEdgePos, uint32 cPos, uint8 c);
	uint32 PosE;
	SBuffer Data;
	SfxTreeEdgeList EL;
	SfxTreeNodeList NL;
};

#define SFXTRE_MAIN_EDGE_IDX 0
#define SFXTRE_ROOT_IDX      0

SfxTree::SfxTree()
{
	HeadNode = 0;
}

SfxTree::~SfxTree()
{
}

int SfxTree::CreateLeafNode(uint32 inEdgePos, uint32 cPos, uint8 c)
{
	int    ok;
	uint32 node_pos = 0, new_edge_pos = 0;
	SfxTreeNode * p_node = 0;
	SfxTreeEdge new_edge;
	THROW(NL.Add(&node_pos));
	new_edge.Start   = cPos;
	new_edge.End     = cPos;
	new_edge.InNode  = node_pos;
	new_edge.OutNode = 0;
	new_edge.Flags   = SfxTreeEdge::fLeaf;
	THROW(EL.Add(&new_edge_pos, &new_edge));
	THROW(NL.Expand(node_pos, c, c_edge_pos));
	p_node = NL.Get(node_pos);
	p_node->InEdge = inEdgePos;
	THROW(NL.Commit(node_pos, p_node));
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}
//
// Descr: Разбивает дугу edgePos на две дуги (newEdge, edge) в точке между индексами splitPos и splitPos+1.
//  Затем создает еще одну дугу (cEdge), исходящую из вновь созданного узла по символу c (позиция cPos).
//  Дуга edge становится исходящей дугой нового узла по символу Data[splitPos+1];
//  Дуга newEdge становится входящей дугой нового узла.
//  Дуга cEdge становится листовой исходящей дугой нового узла по символу Data[cPos].
//
int SfxTree::SplitEdge(const SfxTreeEdgePos & edgePos, uint32 cPos, uint8 c)
{
	int    ok = 1;
	uint32 node_idx = 0, new_edge_idx = 0, c_edge_idx = 0;
	SfxTreeEdge new_edge, c_edge;
	SfxTreeNode * p_node = 0;
	SfxTreeEdge * p_edge = EL.Get(edgePos.Idx);
	THROW(p_edge);
	if(edgePos.Pos < p_edge->End) {
		THROW(NL.Add(&node_idx));

		new_edge.Start   = p_edge->Start;
		new_edge.End     = edgePos.Pos;
		new_edge.InNode  = p_edge->InNode;
		new_edge.OutNode = node_idx;
		new_edge.Flags   = 0;

		c_edge.Start   = cPos;
		c_edge.End     = cPos;
		c_edge.InNode  = node_idx;
		c_edge.OutNode = 0;
		c_edge.Flags   = SfxTreeEdge::fLeaf;

		p_edge->Start  = edgePos.Pos+1;
		p_edge->InNode = node_idx;
		THROW(EL.Commit(edgePos));

		THROW(EL.Add(&new_edge_idx, &new_edge));
		THROW(EL.Add(&c_edge_idx, &c_edge));

		THROW(NL.Expand(node_idx, GetChar(edgePos.Pos+1), edgePos.Idx));
		THROW(NL.Expand(node_idx, c, c_edge_idx));
		p_node = NL.Get(node_idx);
		p_node->InEdge = new_edge_idx;
		THROW(NL.Commit(node_idx, p_node));
	}
	else {
		THROW_V(edgePos.Pos == p_edge->End, SLERR_SFXTRE_INVEDGEEXT); // Внутренняя ошибка:
			// неверное позиционирование при продолжении дуги.
		if(p_edge->Flags & SfxTreeEdge::fLeaf) {
			p_edge->End++;
		}
		else {
			THROW(CreateLeafNode(edgePos.Idx, cPos, c));
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int SfxTree::SearchOnEdge(uint32 * pStart, uint32 end, SfxTreeEdgePos * pEdgePos)
{
	int    ok = -1, found = 1;
	SfxTreeEdge * p_edge = EL.Get(pEdgePos->Idx);
	pEdgePos->Pos = 0;
	if(p_edge == 0)
		ok = 0;
	else {
		uint32 i;
		uint32 start = *pStart;
		uint32 edge_start = p_edge->Start;
		uint32 edge_len = (p_edge->End - edge_start);
		for(i = 0; i < edge_len; i++) {
			if(GetChr(start+i) != GetChr(edge_start+i)) {
				found = 0;
				break;
			}
		}
		*pStart = start+i-1;
		if(found) {
			if(i == edge_len)
				ok = 2;
			else
				ok = 1;
		}
	}
	return ok;
}

int SfxTree::Search(uint32 nodeIdx, uint32 start, uint32 end, SfxTreeEdgePos * pEdgePos)
{
	int    ok = -1;
	SfxTreeEdgePos edge_pos;
	SfxTreeNode * p_node = NL.Get(nodeIdx);
	THROW(p_node);
	if(p_node->Search(GetChar(start), &edge_pos.Idx)) {
		int r = SearchOnEdge(start, end, &edge_pos);
		if(r > 0) {
			ASSIGN_PTR(pEdgePos, edge_pos);
			ok = 1;
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int SfxTree::SearchForBuild(uint32 start, uint32 end, SfxTreeEdgePos * pEdgePos)
{
	int    ok = 1;
	SfxTreeEdgePos new_pos;
	uint32 node_idx;
	SfxTreeEdge * p_edge = 0;
	if(start == 0) {
		new_pos.Idx = SFXTRE_MAIN_EDGE_IDX;
		THROW(p_edge = EL.Get(edge_idx));
		new_pos.Pos = p_edge->End;
	}
	else {
		SfxTreeEdgePos prev_edge_pos = *pEdgePos;
		SfxTreeNode * p_node;
		if(prev_edge_pos.Idx == SFXTRE_MAIN_EDGE_IDX) {
			node_idx = SFXTRE_ROOT_IDX;
			THROW(Search(node_idx, start, end, &new_pos));
		}
		else {
			THROW(p_edge = EL.Get(prev_edge_pos.Idx));
			node_idx = p_edge->InNode;
			THROW(p_node = NL.Get(node_idx));
			node_idx = p_node->SfxLink;
			THROW(p_node = NL.Get(node_idx));
			THROW(Search(node_idx, end - prev_pos, end, &new_pos));
		}
	}
	ASSIGN_PTR(pEdgePos, new_pos);
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int SfxTree::Add(uint32 tag, const uint8 * pData, size_t len)
{
	int    ok = 1;
	Data.Write(pData, len);
	THROW(CreateLeafNode(0, 0, GetChar(0));
	for(uint i = 1; i < len; i++) {
		uint8  c_end = GetChar(i);
		SfxTreeEdgePos edge_pos;
		for(uint j = 0; j < i; j++) {
			THROW(SearchForBuild(j, i-1, &edge_pos));
			THROW(SplitEdge(edge_pos, i, c_end));
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}
