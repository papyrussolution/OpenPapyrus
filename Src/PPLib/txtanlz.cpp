// TXTANLZ.CPP
// Copyright (c) A.Sobolev 2013, 2014, 2015, 2016, 2017, 2018
//
#include <pp.h>
#pragma hdrstop
#include <fann.h>
//
//
//
PPTextAnalyzer::Replacer::SrcItem::SrcItem() : Op(0), Flags(0), TargetIdx(0)
{
}

const SSzChunk * FASTCALL PPTextAnalyzer::Replacer::SrcItem::GetTermGroup(uint termIdx, uint * pGrpIdx) const
{
	const  int idx = (int)termIdx;
	for(uint i = 0; i < GL.getCount(); i++) {
		const SSzChunk & r_chunk = GL.at(i);
		if(idx >= r_chunk.Begin && idx <= r_chunk.GetEnd()) {
			ASSIGN_PTR(pGrpIdx, (i+1));
			return &r_chunk;
		}
	}
	return 0;
}

PPTextAnalyzer::Replacer::Chain::Chain() : TSVector <PPTextAnalyzer::Replacer::Term> () // @v9.8.4 TSArray-->TSVector
{
}

PPTextAnalyzer::Replacer::Chain & FASTCALL PPTextAnalyzer::Replacer::Chain::operator = (const PPTextAnalyzer::Replacer::Chain & rS)
{
	SVector::copy(rS); // @v9.8.4 SArray-->SVector
	return *this;
}

int PPTextAnalyzer::Replacer::Chain::Add(int type, int tok, uint32 id)
{
	assert(!Replacer::IsOp(type));
	Replacer::Term t;
	t.Type = (int16)type;
	t.Tok  = (int16)tok;
	t.Id = id;
	return insert(&t) ? 1 : PPSetErrorSLib();
}

int PPTextAnalyzer::Replacer::Chain::Add(int type, const PPTextAnalyzer::Replacer::Chain & rInner)
{
	int    ok = 1;
	assert(oneof3(type, stOpOr, stOpAnd, 0));
	const uint inner_c = rInner.getCount();
	assert(inner_c);
	if(type) {
		Replacer::Term t;
		t.Type = (int16)type;
		t.Tok  = 0;
		t.Id = inner_c;
		THROW_SL(insert(&t));
	}
	for(uint i = 0; i < inner_c; i++) {
		const Replacer::Term & r_item = rInner.at(i);
		THROW_SL(insert(&r_item));
	}
	CATCHZOK
	return ok;
}

//static
int FASTCALL PPTextAnalyzer::Replacer::IsOp(int termType) { return BIN(termType > stLastLex); }
//static
int FASTCALL PPTextAnalyzer::Replacer::IsLex(int termType) { return BIN(termType < stLastLex); }

PPTextAnalyzer::Replacer::Replacer() : P_Cluster(0)
{
	InitParsing(0);
}

PPTextAnalyzer::Replacer::~Replacer()
{
	delete P_Cluster;
}

void PPTextAnalyzer::Replacer::InitParsing(const char * pFileName)
{
	FileName = pFileName;
	LineNo = 0;
	State = 0;
	ZDELETE(P_Cluster);
}

void PPTextAnalyzer::Replacer::IncLineNo()
{
	LineNo++;
}

long PPTextAnalyzer::Replacer::SetState(long st, int set)
{
	long   prev_state = State;
	SETFLAG(State, st, set);
	return prev_state;
}

PPTextAnalyzer::Replacer::SrcItem * PPTextAnalyzer::Replacer::MakeSrcItem(
	PPTextAnalyzer::Replacer::SrcItem * pOuterSrcItem,
	int op, uint targetIdx, const PPTextAnalyzer::Replacer::Chain & rList, const TSVector <SSzChunk> & rGl) const // @v9.8.4 TSArray-->TSVector
{
	SETIFZ(pOuterSrcItem, new Replacer::SrcItem);
	if(pOuterSrcItem) {
		pOuterSrcItem->Op = op;
		pOuterSrcItem->TargetIdx = targetIdx;
		pOuterSrcItem->List = rList;
		pOuterSrcItem->GL = rGl;
	}
	else
		PPSetError(PPERR_NOMEM);
	return pOuterSrcItem;
}

int PPTextAnalyzer::Replacer::AddClusterItem(Replacer::SrcItem * pItem)
{
	int    ok = -1;
	if(pItem) {
		SETIFZ(P_Cluster, new TSCollection <Replacer::SrcItem>);
		THROW_MEM(P_Cluster);
		THROW_SL(P_Cluster->insert(pItem));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPTextAnalyzer::Replacer::BuildSrcIndex()
{
	SrcListIndex.clear();
	{
		//
		// Операторы stOpTo, stOpSignal следуют с наивысшим приоритетом
		//
		for(uint i = 0; i < SrcList.getCount(); i++) {
			const SrcItem * p_src_item = SrcList.at(i);
			if(oneof2(p_src_item->Op, stOpTo, stOpSignal))
				SrcListIndex.add((long)i);
		}
	}
	{
		//
		// Операторы изменения регистра (stOpLower, stOpUpper, stOpCapital) следуют с меньшим приоритетом
		//
		for(uint i = 0; i < SrcList.getCount(); i++) {
			const SrcItem * p_src_item = SrcList.at(i);
			if(oneof3(p_src_item->Op, stOpLower, stOpUpper, stOpCapital))
				SrcListIndex.add((long)i);
		}
	}
	return 1;
}

uint PPTextAnalyzer::Replacer::SearchTarget(const Replacer::Chain & rChain) const
{
	uint   idx = 0;
	for(uint i = 0; !idx && i < TargetList.getCount(); i++) {
		const TargetItem * p_target = TargetList.at(i);
		if(p_target->List.IsEqual(rChain)) {
			idx = i+1;
		}
	}
	return idx;
}

PPTextAnalyzer::FindBlock::FindBlock(const PPTextAnalyzer::Replacer & rR) : TSVector <PPTextAnalyzer::FindItem> (), // @v9.8.6 TSArray-->TSVector
	R(rR), P_Item(0), IdxFirst(0), IdxLast(0), NextPos(0), State(0), P_Idx(0), P_Next(0)
{
}

PPTextAnalyzer::FindBlock::~FindBlock()
{
	if(!(State & stParentIndex))
		delete P_Idx;
	delete P_Next;
}

PPTextAnalyzer::FindBlock * PPTextAnalyzer::FindBlock::GetRecursiveBlock()
{
	if(SETIFZ(P_Next, new FindBlock(R))) {
		P_Next->P_Idx = P_Idx;
		if(P_Idx)
		   P_Next->State |= stParentIndex;
	}
	else
		PPSetErrorNoMem();
	return P_Next;
}

PPTextAnalyzer::FindBlock & PPTextAnalyzer::FindBlock::Init(const PPTextAnalyzer::Replacer::SrcItem * pItem, uint idxFirst, uint idxLast)
{
	P_Item = pItem;
	IdxFirst = idxFirst;
	IdxLast = idxLast;
	return Reset();
}

PPTextAnalyzer::FindBlock & PPTextAnalyzer::FindBlock::Reset()
{
	NextPos = 0;
	State &= ~(stStop|stTest);
	TempBuf.Z();
	clear();
	return *this;
}

const PPTextAnalyzer::FindItem * FASTCALL PPTextAnalyzer::FindBlock::GetGroupItem(uint grpIdx) const
{
	const uint c = getCount();
	for(uint i = 0; i < c; i++) {
		const PPTextAnalyzer::FindItem & r_item = at(i);
		if(r_item.GrpIdx == grpIdx)
			return &r_item;
	}
	return 0;
}

void PPTextAnalyzer::FindBlock::Sort()
{
	SVector::sort(CMPF_LONG); // @v9.8.6 SArray-->SVector
}

struct TrT {
	const  char * S;
	int16  Term;
	int16  Flags;
};

static const TrT TrList[] = {
	{ "%s", PPTextAnalyzer::Replacer::stSpace,       0 },
	{ "%z", PPTextAnalyzer::Replacer::stSpaceMul,    0 },
	{ "%_", PPTextAnalyzer::Replacer::stSpaceSingle, 0 },
	{ "%E", PPTextAnalyzer::Replacer::stEmpty,       0 },
	{ "%t", PPTextAnalyzer::Replacer::stTab,         0 },
	{ "%.", PPTextAnalyzer::Replacer::stDotOpt,      0 },
	{ "%-", PPTextAnalyzer::Replacer::stHyphenOpt,   0 },
	{ "%^", PPTextAnalyzer::Replacer::stBegin,       0 },
	{ "%$", PPTextAnalyzer::Replacer::stEnd,         0 },
	{ "%d", PPTextAnalyzer::Replacer::stNum,         0 },
	{ "%D", PPTextAnalyzer::Replacer::stNumL,        0 },
	{ "%f", PPTextAnalyzer::Replacer::stFloat,       0 },
	{ "%w", PPTextAnalyzer::Replacer::stWord,        0 },
	{ "%(", PPTextAnalyzer::Replacer::stLPar,        0 },
	{ "%)", PPTextAnalyzer::Replacer::stRPar,        0 },
	{ "%*", PPTextAnalyzer::Replacer::stAny,         0 },
	{ "%@", PPTextAnalyzer::Replacer::stCortege,     0 },
	{ "%%", PPTextAnalyzer::Replacer::stPercent,     0 },
	{ "%K", PPTextAnalyzer::Replacer::stOpCapital,   0 },
	{ "%a", PPTextAnalyzer::Replacer::stOpLower,     0 },
	{ "%A", PPTextAnalyzer::Replacer::stOpUpper,     0 },
	{ "%>", PPTextAnalyzer::Replacer::stOpTo,        0 },
	{ "%<", PPTextAnalyzer::Replacer::stOpFrom,      0 },
	{ "%|", PPTextAnalyzer::Replacer::stOpOr,        0 },
	{ "%&", PPTextAnalyzer::Replacer::stOpAnd,       0 },

	{ "%=", PPTextAnalyzer::Replacer::stOpCortege,   0 },
	{ "%!", PPTextAnalyzer::Replacer::stOpSignal,    0 },

	{ "%{", PPTextAnalyzer::Replacer::stOpClusterStart, 0 },
	{ "%}", PPTextAnalyzer::Replacer::stOpClusterEnd,   0 },

	{ "%--", PPTextAnalyzer::Replacer::stOpComment,    0 },
	{ "%#",  PPTextAnalyzer::Replacer::stOpPragma,     0 },

	{ "%/",  PPTextAnalyzer::Replacer::stOpContext,    0 }
};

/*

%{
%}%=brand

*/

int SLAPI PPTextAnalyzer::GetTrT(const PPTextAnalyzer::Replacer & rReplacer, SStrScan & rScan, SString & rExtBuf) const
{
	const char cfront = rScan[0];
	const char cnext = rScan[1];
	if(cfront == '%') {
		if(cnext >= '1' && cnext <= '9') {
			rScan.Incr(2);
			return (cnext-'0');
		}
		else {
			const uint16 s = *PTR16((const char *)rScan);
			for(uint i = 0; i < SIZEOFARRAY(TrList); i++) {
				if(*PTR16(TrList[i].S) == s) {
					rScan.Incr(2);
					int  term = TrList[i].Term;
					if(term == PPTextAnalyzer::Replacer::stHyphenOpt) {
						if(rScan[0] == '-') {
							rScan.Incr();
							term = PPTextAnalyzer::Replacer::stOpComment;
						}
					}
					else if(term == PPTextAnalyzer::Replacer::stCortege) {
						rExtBuf.Z();
						if(rScan.SearchChar('.')) {
							rScan.Get(rExtBuf);
							rScan.IncrLen(1); // (1): не забыть пропустить точку '.'
						}
					}
					else if(term == PPTextAnalyzer::Replacer::stOpPragma) {
						rScan.GetIdent(rExtBuf.Z());
					}
					else if(term == PPTextAnalyzer::Replacer::stOpCortege) {
						rScan.GetIdent(rExtBuf.Z());
					}
					else if(term == PPTextAnalyzer::Replacer::stNumL) {
						char c = rScan[0];
						rExtBuf.Z();
						if(c >= '0' && c <= '9') {
							rExtBuf.CatChar(c);
							rScan.Incr();
							c = rScan[0];
							if(c >= '0' && c <= '9') {
								rExtBuf.CatChar(c);
								rScan.Incr();
							}
							else {
								PPSetError(PPERR_TXT_TERMD_NEED2DIGIT);
								term = -1;
							}
						}
						else {
							PPSetError(PPERR_TXT_TERMD_NEED2DIGIT);
							term = -1;
						}
					}
					return term;
				}
			}
		}
	}
	else if(cfront == '&' && cnext == '&') {
		rScan.Incr(2);
		return PPTextAnalyzer::Replacer::stOpAnd;
	}
	else if(cfront == '|' && cnext == '|') {
		rScan.Incr(2);
		return PPTextAnalyzer::Replacer::stOpOr;
	}
	else if(cfront == '(' && rReplacer.GetState() & Replacer::psCluster) {
		rScan.Incr();
		return PPTextAnalyzer::Replacer::stLPar;
	}
	else if(cfront == ')' && rReplacer.GetState() & Replacer::psCluster) {
		rScan.Incr();
		return PPTextAnalyzer::Replacer::stRPar;
	}
	return 0;
}

static int FASTCALL GetTrText(int term, SString & rBuf)
{
	rBuf.Z();
	if(term >= 1 && term <= 9) {
		rBuf.CatChar('%').CatChar('0'+term);
		return 1;
	}
	else {
		for(uint i = 0; i < SIZEOFARRAY(TrList); i++) {
			if(TrList[i].Term == term) {
				rBuf = TrList[i].S;
				return 1;
			}
		}
	}
	return 0;
}

int SLAPI PPTextAnalyzer::Match(PPTextAnalyzer::FindBlock & rBlk, uint termIdx, const uint termLast, uint idxFirst, uint idxLast) const
{
	const uint first_term_idx = termIdx;
	rBlk.NextPos = 0;

	int    ok = 1;
	uint   idx = idxFirst;
	FindItem fi_common;
	FindItem fi_grp;
	// по отдельным полям инициализировать FindItem быстрее чем MEMSZERO(fi_common);
	fi_common.GrpIdx = 0;
	fi_common.IdxFirst = idxFirst;
	fi_common.IdxLast = 0;
	//
	// по отдельным полям инициализировать FindItem быстрее чем MEMSZERO(fi_grp);
	fi_grp.GrpIdx = 0;
	fi_grp.IdxFirst = 0;
	fi_grp.IdxLast = 0;
	//
	// const uint term_count = rBlk.P_Item->List.getCount();
	while(ok && termIdx <= termLast) {
		const uint preserv_term_idx = termIdx;
		const Replacer::Term & r_term = rBlk.P_Item->List.at(termIdx++);
		if(idx <= idxLast) {
			//
			// Вызов STokenizer::Get() достаточно дорогой (в контексте высоких требований к быстродействию данной функции).
			// По этому, хотя здесь вызов STokenizer::Get() наибоее логичен, мы будем вызывать или не вызывать Get (либо Get_WithoutText)
			// в каждом конкретном случае (switch case) отдельно.
			// Get(idx, rBlk.Item_);
			//
			if(!(rBlk.State & rBlk.stTest)) {
				uint   grp_idx = 0;
				const  SSzChunk * p_grp = rBlk.P_Item->GetTermGroup(preserv_term_idx, &grp_idx);
				if(p_grp) {
					if(p_grp->Begin == preserv_term_idx) {
						assert(fi_grp.GrpIdx == 0);
						fi_grp.GrpIdx = grp_idx;
						fi_grp.IdxFirst = idx;
					}
				}
				else {
					assert(fi_grp.GrpIdx == 0);
				}
			}
			int    idx_spc = 0;
			switch(r_term.Type) {
				case Replacer::stLiteral:
					if(rBlk.P_Idx)
						idx_spc = r_term.Id; // Processing after switch
					else {
						Get_WithoutText(idx, rBlk.Item_);
						if(r_term.Id == rBlk.Item_.TextId)
							idx++;
						else
							ok = 0;
					}
					break;
				case Replacer::stNum:
					if(rBlk.P_Idx)
						idx_spc = TextIndex::spcDigital; // Processing after switch
					else {
						Get(idx, rBlk.Item_);
						if(rBlk.Item_.Text.IsDigit())
							idx++;
						else
							ok = 0;
					}
					break;
				case Replacer::stNumL:
					{
						const uint tok_len = (uint)r_term.Tok;
						if(rBlk.P_Idx && tok_len > 0 && tok_len < 100)
							idx_spc = (TextIndex::spcDigL_First + 1) - tok_len; // Processing after switch
						else {
							Get(idx, rBlk.Item_);
							if(rBlk.Item_.Text.IsDigit() && rBlk.Item_.Text.Len() == tok_len)
								idx++;
							else
								ok = 0;
						}
					}
					break;
				case Replacer::stFloat:
					Get(idx, rBlk.Item_);
					if(rBlk.Item_.Text.IsDigit()) {
						if(idx < idxLast) {
							Get(++idx, rBlk.Item_);
							if(rBlk.Item_.Text == ".") {
								if(idx < idxLast) {
									Get(++idx, rBlk.Item_);
									if(rBlk.Item_.Text.IsDigit())
										idx++;
								}
							}
						}
					}
					else
						ok = 0;
					break;
				case Replacer::stSpace:
					Get(idx, rBlk.Item_);
					if(rBlk.Item_.Text == "\x20" || rBlk.Item_.Text == "\t") {
						do {
							if(idx < idxLast)
								Get(++idx, rBlk.Item_);
							else
								break;
						} while(rBlk.Item_.Text == "\x20" || rBlk.Item_.Text == "\t");
					}
					else
						ok = 0;
					break;
				case Replacer::stSpaceMul:
					Get(idx, rBlk.Item_);
					while(rBlk.Item_.Text == "\x20" || rBlk.Item_.Text == "\t") {
						if(idx < idxLast)
							Get(++idx, rBlk.Item_);
						else
							break;
					}
					break;
				case Replacer::stSpaceSingle:
					if(rBlk.P_Idx)
						idx_spc = TextIndex::spcSpaceOrTab; // Processing after switch
					else {
						Get(idx, rBlk.Item_);
						if(rBlk.Item_.Text == "\x20" || rBlk.Item_.Text == "\t")
							idx++;
						else
							ok = 0;
					}
					break;
				case Replacer::stEmpty:
					break;
				case Replacer::stTab:
					if(rBlk.P_Idx)
						idx_spc = TextIndex::spcTab; // Processing after switch
					else {
						Get(idx, rBlk.Item_);
						if(rBlk.Item_.Text == "\t")
							idx++;
						else
							ok = 0;
					}
					break;
				case Replacer::stDotOpt:
					Get(idx, rBlk.Item_);
					if(rBlk.Item_.Text == ".")
						idx++;
					break;
				case Replacer::stHyphenOpt:
					Get(idx, rBlk.Item_);
					if(rBlk.Item_.Text == "-")
						idx++;
					break;
				case Replacer::stWord:
					Get_WithoutText(idx, rBlk.Item_);
					if(rBlk.Item_.Token == tokWord)
						idx++;
					else
						ok = 0;
					break;
				case Replacer::stBegin:
					if(idx != rBlk.IdxFirst) {
						rBlk.State |= rBlk.stStop;
						ok = 0;
					}
					break;
				case Replacer::stEnd:
					ok = 0;
					break;
				case Replacer::stAny:
					rBlk.State |= rBlk.stTest;
					{
						int    local_ok = 0;
						//
						// Особый случай: %$ или конец образца после %* не будет правильно распознана локальным
						// циклом. Более того, специальной обработкой мы значительно ускорим процесс.
						//
						if(termIdx == (termLast+1)) {
							idx = idxLast+1;
							local_ok = 1;
						}
						else if(termIdx <= termLast && rBlk.P_Item->List.at(termIdx).Type == Replacer::stEnd) {
							idx = idxLast+1;
							termIdx++;
							local_ok = 1;
						}
						else if(Helper_FindReplacerSrcItem(rBlk, termIdx, termLast, idx+1, idxLast, &idx)) {
							local_ok = 1;
						}
						rBlk.NextPos = 0; // Нельзя вводить в заблуждение вызывающую функцию
						if(!local_ok)
							ok = 0;
					}
					rBlk.State &= ~rBlk.stTest;
					break;
				case Replacer::stCortege:
					{
						//
						// Рекурсивный блок поиска совпадения для одного из элементов кортежа.
						//
						const LongArray * p_clist = rBlk.R.SearchCortege(r_term.Id);
						assert(p_clist);
						int    local_ok = 0;
						FindBlock * p_local_fb = rBlk.GetRecursiveBlock();
						if(p_local_fb) {
							p_local_fb->State |= rBlk.stTest; // Нас не интересуют группы внутри элемента кортежа
							for(uint i = 0; !local_ok && i < p_clist->getCount(); i++) {
								const Replacer::SrcItem * p_si = rBlk.R.SrcList.at(p_clist->get(i));
								const uint si_c = p_si->List.getCount();
								if(si_c) {
									const uint local_term_last = si_c-1;
									p_local_fb->Init(p_si, idx, idxLast);
									if(Match(p_local_fb->Reset(), 0, local_term_last, idx, idxLast)) { // @recursion
										assert(p_local_fb->getCount());
										idx = p_local_fb->at(0).IdxLast+1;
										local_ok = 1;
									}
								}
							}
						}
						if(!local_ok)
							ok = 0;
					}
					break;
				case Replacer::stOpOr:
				case Replacer::stOpAnd:
					{
						assert(rBlk.P_Item->Flags & Replacer::SrcItem::fContext);

						int    local_ok = 0;
						FindBlock * p_local_fb = rBlk.GetRecursiveBlock();
						if(p_local_fb) {
							p_local_fb->State |= rBlk.stTest; // Нас не интересуют группы при поиске контекста

							const Replacer::SrcItem * p_si = rBlk.P_Item;
							const uint si_c = r_term.Id;
							if(si_c) {
								const  uint   local_term_last = termIdx+si_c-1;
								uint   local_ret_idx = 0;
								p_local_fb->Init(p_si, idx, idxLast);
								if(Helper_FindReplacerSrcItem(*p_local_fb, termIdx, local_term_last, idx, idxLast, &local_ret_idx)) {
									if(r_term.Type == Replacer::stOpAnd) {
										p_local_fb->Init(p_si, idx, idxLast);
										const uint local_term_first = local_term_last+1;
										if(Helper_FindReplacerSrcItem(*p_local_fb, local_term_first, termLast, idx, idxLast, &local_ret_idx))
											idx = local_ret_idx;
										else
											ok = 0;
									}
									else // Replacer::stOpOr
										idx = local_ret_idx;
								}
								else if(r_term.Type == Replacer::stOpAnd)
									ok = 0;
								else { // Replacer::stOpOr
									p_local_fb->Init(p_si, idx, idxLast);
									const uint local_term_first = local_term_last+1;
									if(Helper_FindReplacerSrcItem(*p_local_fb, local_term_first, termLast, idx, idxLast, &local_ret_idx))
										idx = local_ret_idx;
									else
										ok = 0;
								}
								//
								// Следующий оператор обеспечивает выход из цикла поиска. В общем случае, это - не правильно.
								// Теоретически, после сложного выражения может следовать цепочка поиска, которой должен
								// удовлетворять суффикс текста после выражения. Из-за выхода такое условие проверено не будет.
								// Для полной реализации необходимо знать точку окончания выражения, сейчас мы ее не знаем-
								// нам известна только длина левого терма, длина же правого - четко не определена.
								// @todo Необходимо при разборе выражения устанавливать описанную длину правого подвыражения.
								//
								termIdx = termLast+1; // Завершаем поиск.
							}
						}
					}
					break;
				default:
					assert(0); // invalid termType
					ok = 0;
			}
			if(idx_spc) {
				const LongArray * p_pos_list = rBlk.P_Idx->GetTextIndex(idx_spc);
				ok = 0;
				if(!p_pos_list)
					rBlk.State |= rBlk.stStop;
				else {
					const uint plc = p_pos_list->getCount();
					for(uint i = 0; i < plc; i++) {
						const uint p = p_pos_list->get(i);
						//
						// Позиции, где встречается токен r_term.Id в индексе должны быть упорядочены
						//
						assert(i == 0 || (long)p > p_pos_list->get(i-1));
						//
						if(p == idx) {
							idx++;
							ok = 1;
							break;
						}
						else if(p > idx) {
							if(preserv_term_idx == first_term_idx)
								rBlk.NextPos = p;
							break;
						}
					}
				}
			}
		}
		else {
			//
			// Текст кончился, однако, могут остаться опциональные компоненты сравнения,
			// которым окончание текста удовлетворяет.
			//
			if(!oneof4(r_term.Type, Replacer::stEnd, Replacer::stSpaceMul, Replacer::stDotOpt, Replacer::stHyphenOpt))
				ok = 0;
		}
		if(ok && fi_grp.GrpIdx && !(rBlk.State & rBlk.stTest)) {
			const SSzChunk & r_grp = rBlk.P_Item->GL.at(fi_grp.GrpIdx-1);
			if(r_grp.GetEnd() == preserv_term_idx) {
				fi_grp.IdxLast = idx-1;
				rBlk.insert(&fi_grp);
				MEMSZERO(fi_grp);
			}
		}
	}
	if(ok && !(rBlk.State & rBlk.stTest)) {
		fi_common.IdxLast = idx ? (idx-1) : idxLast;
		rBlk.insert(&fi_common);
		rBlk.Sort();
	}
	return ok;
}

int SLAPI PPTextAnalyzer::Helper_FindReplacerSrcItem(PPTextAnalyzer::FindBlock & rBlk, const uint termFirst, const uint termLast, uint idxFirst, const uint idxLast, uint * pFoundIdx) const
{
	int    ok = 0;
	if(rBlk.P_Item) {
		for(uint start = idxFirst; !ok && start <= idxLast;) {
			rBlk.Reset();
			if(pFoundIdx)
				rBlk.State |= rBlk.stTest;
			if(Match(rBlk, termFirst, termLast, start, idxLast)) {
				ASSIGN_PTR(pFoundIdx, start);
				ok = 1;
			}
			else if(rBlk.State & rBlk.stStop)
				break;
			else if(rBlk.NextPos)
				start = rBlk.NextPos;
			else
				start++;
		}
	}
	return ok;
}

PPTextAnalyzer::TextIndex::TextIndex()
{
}

void PPTextAnalyzer::TextIndex::Reset()
{
	for(uint i = 0; i < L.getCount(); i++) {
		Item * p_idx_item = L.at(i);
		p_idx_item->TextId = 0;
		p_idx_item->PosList.clear();
	}
}

int FASTCALL PPTextAnalyzer::TextIndex::Add_(uint position, int textId)
{
	int    ok = 1;
	if(textId) {
		uint p = 0;
		if(L.lsearch(&textId, &p, CMPF_LONG)) {
			Item * p_idx_item = L.at(p);
			assert(p_idx_item);
			assert(p_idx_item->TextId == textId);
			assert(p_idx_item->PosList.lsearch((long)position) == 0);
			THROW_SL(p_idx_item->PosList.add((long)position));
		}
		else {
			//
			// Дабы на тратить время на распределение и освобождение памяти
			// метод PPTextAnalyzer::TextIndex::Reset не разрушает L,
			// а лишь обнуляет индексы слов и соответствующие списки позиций.
			// По этому здесь сначала попробуем найти свободный элемент и использовать его.
			//
			long   zero_text_id = 0;
			if(L.lsearch(&zero_text_id, &p, CMPF_LONG)) {
				Item * p_idx_item = L.at(p);
				assert(p_idx_item);
				assert(p_idx_item->TextId == 0);
				assert(p_idx_item->PosList.lsearch((long)position) == 0);
				p_idx_item->TextId = textId;
				THROW_SL(p_idx_item->PosList.add((long)position));
			}
			else {
				Item * p_new_idx_item = new Item;
				THROW_MEM(p_new_idx_item);
				p_new_idx_item->TextId = textId;
				THROW_SL(p_new_idx_item->PosList.add((long)position));
				THROW_SL(L.insert(p_new_idx_item));
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

void PPTextAnalyzer::TextIndex::Finish()
{
	L.sort(CMPF_LONG);
}

int FASTCALL PPTextAnalyzer::TextIndex::HasTextId(int textId) const
{
	return L.bsearch(&textId, 0, CMPF_LONG);
}

const LongArray * FASTCALL PPTextAnalyzer::TextIndex::GetTextIndex(int textId) const
{
	uint   p = 0;
	return L.bsearch(&textId, &p, CMPF_LONG) ? &L.at(p)->PosList : 0;
}

int SLAPI PPTextAnalyzer::IndexText(PPTextAnalyzer::FindBlock & rBlk, uint idxFirst, uint idxLast) const
{
	int    ok = 1;
	THROW_MEM(SETIFZ(rBlk.P_Idx, new TextIndex));
	rBlk.P_Idx->Reset();
	for(uint i = idxFirst; i <= idxLast; i++) {
		Get(i, rBlk.Item_);
		THROW(rBlk.P_Idx->Add_(i, rBlk.Item_.TextId));
		if(rBlk.Item_.Text.IsDigit()) {
			THROW(rBlk.P_Idx->Add_(i, TextIndex::spcDigital));
			{
				const int len = rBlk.Item_.Text.Len();
				if(len > 0 && len < 100) {
					int    spc = (TextIndex::spcDigL_First + 1) - len;
					THROW(rBlk.P_Idx->Add_(i, spc));
				}
			}
		}
		else if(rBlk.Item_.Text == "\x20") {
			THROW(rBlk.P_Idx->Add_(i, TextIndex::spcSpace));
			THROW(rBlk.P_Idx->Add_(i, TextIndex::spcSpaceOrTab));
		}
		else if(rBlk.Item_.Text == "\t") {
			THROW(rBlk.P_Idx->Add_(i, TextIndex::spcTab));
			THROW(rBlk.P_Idx->Add_(i, TextIndex::spcSpaceOrTab));
		}
	}
	rBlk.P_Idx->Finish();
	CATCH
		ZDELETE(rBlk.P_Idx);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPTextAnalyzer::FindReplacerSrcItem(PPTextAnalyzer::FindBlock & rBlk) const
{
	const uint _c = rBlk.P_Item->List.getCount();
	return _c ? Helper_FindReplacerSrcItem(rBlk, 0, _c-1, rBlk.IdxFirst, rBlk.IdxLast, 0) : 0;
}

int SLAPI PPTextAnalyzer::DoReplacement(const PPTextAnalyzer::Replacer & rR, PPTextAnalyzer::FindBlock & rBlk, SString & rBuf) const
{
	rBuf.Z();

	int    ok = -1;
	const  Replacer::SrcItem * p_src_item = rBlk.P_Item;
	const  uint _c = p_src_item ? p_src_item->List.getCount() : 0;
	if(_c) {
		uint   i;
		const  int _op = p_src_item->Op;
		int    last_inserted_token = 0;
		uint   si = rBlk.IdxFirst;
		uint   fi = rBlk.IdxLast;
		uint   nexti = si;
		while(si <= fi && Helper_FindReplacerSrcItem(rBlk.Reset(), 0, _c-1, si, fi, 0)) {
			assert(rBlk.getCount());
			const FindItem & r_fbi = rBlk.at(0);
			assert(r_fbi.GrpIdx == 0);
			int    last_token = 0;
			for(i = nexti; i < r_fbi.IdxFirst; i++) {
				if(_op == Replacer::stOpSignal) {
					Get_WithoutText(i, rBlk.Item_);
					last_token = rBlk.Item_.Token;
				}
				else {
					Get(i, rBlk.Item_);
					rBuf.Cat(rBlk.Item_.Text);
					last_token = rBlk.Item_.Token;
				}
			}
			if(oneof2(_op, Replacer::stOpTo, Replacer::stOpSignal)) {
				if(rBlk.P_Item->TargetIdx) {
					const Replacer::TargetItem * p_target = rR.TargetList.at(rBlk.P_Item->TargetIdx-1);
					for(uint j = 0; j < p_target->List.getCount(); j++) {
						const Replacer::Term & r_term = p_target->List.at(j);
						switch(r_term.Type) {
							case Replacer::stLiteral:
								if(r_term.Id) {
									GetTextById(r_term.Id, rBlk.TempBuf);
									//
									// При первой вставке включаем форсированный пробел между предыдущим и вставляемым словами
									//
									if(last_token == tokWord) {
										if(r_term.Tok == tokWord)
											rBuf.Space();
										last_token = 0;
									}
									rBuf.Cat(rBlk.TempBuf);
									last_inserted_token = r_term.Tok;
								}
								break;
							case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
								{
									const FindItem * p_fbigi = rBlk.GetGroupItem(r_term.Type);
									THROW(p_fbigi); // @err
									for(uint k = p_fbigi->IdxFirst; k <= p_fbigi->IdxLast; k++) {
										Get(k, rBlk.Item_);
										//
										// При первой вставке включаем форсированный пробел между предыдущим и вставляемым словами
										//
										if(last_token == tokWord) {
											if(rBlk.Item_.Token == tokWord)
												rBuf.Space();
											last_token = 0;
										}
										rBuf.Cat(rBlk.Item_.Text);
										last_inserted_token = rBlk.Item_.Token;
									}
								}
								break;
							case Replacer::stSpaceSingle:
								rBuf.Space();
								last_inserted_token = tokDelim;
								break;
							case Replacer::stTab:
								rBuf.Tab();
								last_inserted_token = tokDelim;
								break;
							case Replacer::stEmpty:
								// ничего не вставляем
								break;
						}
					}
				}
			}
			else if(oneof3(_op, Replacer::stOpLower, Replacer::stOpUpper, Replacer::stOpCapital)) {
				int    ccas = 0;
				if(_op == Replacer::stOpLower)
					ccas = CCAS_LOWER;
				else if(_op == Replacer::stOpUpper)
					ccas = CCAS_UPPER;
				else if(_op == Replacer::stOpCapital)
					ccas = CCAS_CAPITAL;
				for(i = r_fbi.IdxFirst; i <= r_fbi.IdxLast; i++) {
					Get(i, rBlk.Item_);
					rBuf.Cat(rBlk.Item_.Text.SetCase(ccas));
				}
				last_inserted_token = 0; // Форсированный пробел после замены регистра точно не нужен
			}
			si = r_fbi.IdxLast+1; // r_fbi.IdxFirst+1
			nexti = r_fbi.IdxLast+1;
			ok = 1;
			if(p_src_item->Flags & Replacer::SrcItem::fContext) {
				//
				// Для контекста повторную обработку делать нельзя - бесконечный цикл
				//
				break;
			}
		}
		if(_op != Replacer::stOpSignal) {
			if(nexti <= fi) {
				for(i = nexti; i <= fi; i++) {
					Get(i, rBlk.Item_);
					//
					// После вставки включаем форсированный пробел между вставленным и следующим словами
					//
					if(last_inserted_token == tokWord) {
						if(rBlk.Item_.Token == tokWord)
							rBuf.Space();
						last_inserted_token = 0;
					}
					rBuf.Cat(rBlk.Item_.Text);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPTextAnalyzer::Helper_ReplacerSrcItemToStr(const PPTextAnalyzer::Replacer & rR,
	const PPTextAnalyzer::Replacer::SrcItem * pItem,
	const PPTextAnalyzer::Replacer::Chain & rC, uint start, uint count, SString & rBuf) const
{
	int    ok = 1;
	TSStack <uint> rpar_stack;
	assert(start < rC.getCount());
	assert((start + count) <= rC.getCount());
	SString temp_buf;
	for(uint i = start; i < (start+count); i++) {
		const  Replacer::Term & r_term = rC.at(i);
		const  SSzChunk * p_grp = pItem ? pItem->GetTermGroup(i, 0) : 0;
		if(p_grp && p_grp->Begin == i)
			rBuf.Cat("%(");
		if(oneof2(r_term.Type, Replacer::stOpOr, Replacer::stOpAnd)) {
			const uint _c = r_term.Id;
			rBuf.Cat("%(");
			i++;
			Helper_ReplacerSrcItemToStr(rR, pItem, rC, i, _c, rBuf); // @recursion
			if(GetTrText(r_term.Type, temp_buf))
				rBuf.Cat(temp_buf);
			else
				rBuf.Cat("%?");
			Helper_ReplacerSrcItemToStr(rR, pItem, rC, i+_c, count-(i-start+_c), rBuf); // @recursion
			rBuf.Cat("%)");
			break;
		}
		else if(r_term.Type == Replacer::stLiteral) {
			if(r_term.Id) {
				GetTextById(r_term.Id, temp_buf);
				rBuf.Cat(temp_buf);
			}
			else
				rBuf.Cat("#0");
		}
		else {
			if(GetTrText(r_term.Type, temp_buf))
				rBuf.Cat(temp_buf);
			else
				rBuf.Cat("%?");
			if(r_term.Type == Replacer::stCortege) {
				if(r_term.Id) {
					GetTextById(r_term.Id, temp_buf);
					rBuf.Cat(temp_buf);
				}
				else
					rBuf.Cat("#0");
				rBuf.CatChar('.');
			}
			else if(r_term.Type == Replacer::stNumL) {
				rBuf.CatLongZ(r_term.Tok, 2);
			}
		}
		if(p_grp && p_grp->GetEnd() == i)
			rBuf.Cat("%)");
	}
	return ok;
}

int SLAPI PPTextAnalyzer::ReplacerSrcItemToStr(const PPTextAnalyzer::Replacer & rR, const PPTextAnalyzer::Replacer::SrcItem * pItem, SString & rBuf) const
{
	int    ok = 1;
	rBuf.Z();
	if(pItem) {
		SString temp_buf;
		if(pItem->Flags & Replacer::SrcItem::fContext) {
			rBuf.Cat("%/");
		}
		Helper_ReplacerSrcItemToStr(rR, pItem, pItem->List, 0, pItem->List.getCount(), rBuf);
		if(pItem->Flags & Replacer::SrcItem::fContext) {
			rBuf.Cat("%/");
		}
		if(pItem->Op) {
			if(GetTrText(pItem->Op, temp_buf)) {
				rBuf.Cat(temp_buf);
			}
			else
				rBuf.Cat("%?");
		}
		if(pItem->TargetIdx) {
			if(pItem->Op == Replacer::stOpCortege) {
				GetTextById(pItem->TargetIdx, temp_buf);
				rBuf.Cat(temp_buf);
			}
			else if(pItem->TargetIdx <= rR.TargetList.getCount()) {
				const Replacer::TargetItem * p_target = rR.TargetList.at(pItem->TargetIdx-1);
				Helper_ReplacerSrcItemToStr(rR, 0, p_target->List, 0, p_target->List.getCount(), rBuf);
			}
			else {
				rBuf.Cat("@error");
			}
		}
		else if(pItem->Op == Replacer::stOpCortege) {
			rBuf.Cat("#0");
		}
	}
	return ok;
}

LongArray * PPTextAnalyzer::Replacer::SearchCortege(uint cortegeId) const
{
	LongArray * p_list = 0;
	uint   pos = 0;
	if(CrtgList.getFlags() & arySorted) {
		if(CrtgList.bsearch(&cortegeId, &pos, CMPF_LONG)) {
			p_list = &CrtgList.at(pos)->SrcListIdxList;
		}
	}
	else {
		if(CrtgList.lsearch(&cortegeId, &pos, CMPF_LONG)) {
			p_list = &CrtgList.at(pos)->SrcListIdxList;
		}
	}
	return p_list;
}

int PPTextAnalyzer::Replacer::AddCortegeItem(uint cortegeId, uint srcListIdx)
{
	int    ok = 1;
	assert(cortegeId != 0);
	assert(srcListIdx < SrcList.getCount());
	LongArray * p_list = SearchCortege(cortegeId);
	if(p_list) {
		THROW_SL(p_list->add(srcListIdx));
	}
	else {
		CortegeItem * p_new_item = new CortegeItem;
		THROW_MEM(p_new_item);
		p_new_item->CortegeId = cortegeId;
		THROW_SL(p_new_item->SrcListIdxList.add(srcListIdx));
		THROW_SL(CrtgList.insert(p_new_item));
		ok = 2;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPTextAnalyzer::ParseContext(const PPTextAnalyzer::Replacer & rReplacer, SStrScan & rScan, Replacer::Chain * pChain, int recur)
{
	int    ok = 1;
	int    term = 0;
	Item   item_;
	Replacer::Chain current_chain;
	SString temp_buf, term_ext_buf;
	do {
		term = GetTrT(rReplacer, rScan, term_ext_buf);
		if(term == 0) {
			temp_buf.CatChar(rScan[0]);
			rScan.Incr();
		}
		else {
			if(temp_buf.NotEmpty()) {
				uint   j;
				uint   idx_first = 0, idx_count = 0;
				THROW_SL(RunSString(0, 0, temp_buf, &idx_first, &idx_count));
				for(j = 0; j < idx_count; j++) {
					if(Get(idx_first+j, item_)) {
						current_chain.Add(Replacer::stLiteral, item_.Token, item_.TextId);
					}
				}
				temp_buf.Z();
			}
			switch(term) {
				case Replacer::stLPar:
					THROW_PP(current_chain.getCount() == 0, PPERR_TXA_COMPLEXSUBCTX);
					THROW(ParseContext(rReplacer, rScan, &current_chain, recur+1)); // @recursion
					break;
				case Replacer::stRPar:
					THROW_PP(recur > 0, PPERR_TXA_PARMISM);
					break;
				case Replacer::stOpOr:
					THROW_PP(current_chain.getCount(), PPERR_TXA_LEFTEMPTYOR);
					THROW(pChain->Add(Replacer::stOpOr, current_chain));
					current_chain.clear();
					break;
				case Replacer::stOpAnd:
					THROW_PP(current_chain.getCount(), PPERR_TXA_LEFTEMPTYAND);
					THROW(pChain->Add(Replacer::stOpAnd, current_chain));
					current_chain.clear();
					break;
				case Replacer::stOpContext:
					break;
				case Replacer::stSpace:
				case Replacer::stSpaceMul:
				case Replacer::stSpaceSingle:
				case Replacer::stEmpty:
				case Replacer::stTab:
				case Replacer::stDotOpt:
				case Replacer::stHyphenOpt:
				case Replacer::stWord:
				case Replacer::stNum:
				case Replacer::stNumL:
				case Replacer::stFloat:
				case Replacer::stBegin:
				case Replacer::stEnd:
				case Replacer::stAny:
					if(term == Replacer::stNumL) {
						int16  len = (int16)term_ext_buf.ToLong();
						THROW_PP(len > 0 && len < 100, PPERR_TXT_TERMD_NEED2DIGIT);
						THROW(current_chain.Add(term, len, 0));
					}
					else {
						THROW(current_chain.Add(term, 0, 0));
					}
					break;
			}
		}
	} while(!oneof2(term, Replacer::stOpContext, Replacer::stRPar));
	if(current_chain.getCount()) {
		THROW(pChain->Add(0, current_chain));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPTextAnalyzer::ParseReplacerLine(const SString & rLine, PPTextAnalyzer::Replacer & rReplacer)
{
	int    ok = 1;
	int    single_op = 0;
	int    do_close_cluster = 0; // В строке встретилась команда закрытия кластера. После обработки хвоста строки следует акцептировать содержимое кластера.
	int    cortege_id = 0;
	SString temp_buf, term_ext_buf;
	Item   item_;
	SStrScan scan(rLine);
	scan.Skip();

	Replacer::SrcItem * p_current_chain = 0;
	TSCollection <Replacer::SrcItem> or_list;
	Replacer::SrcItem * p_src = 0;
	Replacer::TargetItem * p_target = 0;
	TSStack <SSzChunk> grp_stack;
	int    term = 0, last_term = 0;
	for(; scan[0]; last_term = term) {
		term = GetTrT(rReplacer, scan, term_ext_buf);
		THROW(term >= 0);
		THROW_PP(last_term != Replacer::stOpClusterEnd || oneof6(term, Replacer::stOpTo, Replacer::stOpSignal, Replacer::stOpLower,
			Replacer::stOpUpper, Replacer::stOpCapital, Replacer::stOpCortege), PPERR_TXA_NOTVALIDOPAFTERCLUSTER);
		if(term == Replacer::stOpComment) {
			break;
		}
		else if(term == Replacer::stOpPragma) {

		}
		else if(term == Replacer::stPercent) {
			temp_buf.CatChar('%');
		}
		else if(term) {
			if(term == Replacer::stOpCortege) {
				uint   idx_first = 0, idx_count = 0;
				THROW_PP_S(term_ext_buf.NotEmpty(), PPERR_TXA_UNDEFCORTEGENAME, term_ext_buf);
				THROW_SL(RunSString(0, 0, term_ext_buf, &idx_first, &idx_count));
				THROW_PP_S(idx_count == 1, PPERR_TXA_CORTEGENAMENSINGLE, term_ext_buf);
				THROW_PP_S(Get(idx_first, item_), PPERR_TXA_UNDEFCORTEGENAME, term_ext_buf);
				cortege_id = item_.TextId;
				term_ext_buf = 0;
			}
			else {
				cortege_id = 0;
				if(temp_buf.NotEmpty()) {
					uint   j;
					uint   idx_first = 0, idx_count = 0;
					THROW_SL(RunSString(0, 0, temp_buf, &idx_first, &idx_count));
					for(j = 0; j < idx_count; j++) {
						if(Get(idx_first+j, item_)) {
							SETIFZ(p_current_chain, new Replacer::SrcItem);
							THROW_MEM(p_current_chain);
							p_current_chain->List.Add(Replacer::stLiteral, item_.Token, item_.TextId);
						}
					}
					temp_buf.Z();
				}
			}
			switch(term) {
				case Replacer::stOpContext:
					SETIFZ(p_current_chain, new Replacer::SrcItem);
					THROW_MEM(p_current_chain);
					rReplacer.SetState(Replacer::psContext, 1);
					THROW(ParseContext(rReplacer, scan, &p_current_chain->List, 0));
					rReplacer.SetState(Replacer::psContext, 0);
					p_current_chain->Flags |= Replacer::SrcItem::fContext;
					break;
				case Replacer::stOpClusterStart:
					THROW_PP(!(rReplacer.GetState() & Replacer::psCluster), PPERR_TXA_INNERCLUSTER);
					rReplacer.SetState(Replacer::psCluster, 1);
					single_op = term;
					scan.Skip();
					THROW_PP(scan[0] == 0, PPERR_TXA_CLUSTERSTARTNONSINGLE);
					break;
				case Replacer::stOpClusterEnd:
					THROW_PP(rReplacer.GetState() & Replacer::psCluster, PPERR_TXA_MISMCLUSTEREND);
					THROW_PP(rReplacer.P_Cluster && rReplacer.P_Cluster->getCount(), PPERR_TXA_EMPTYCLUSTER);
					rReplacer.SetState(Replacer::psCluster, 0);
					do_close_cluster = 1;
					break;
				case Replacer::stLPar:
					THROW_PP(!oneof2(single_op, Replacer::stOpTo, Replacer::stOpSignal), PPERR_TXA_GRPINTARGET);
					{
						SSzChunk g;
						g.Begin = p_current_chain ? p_current_chain->List.getCount() : 0;
						g.Len = 0;
						grp_stack.push(g);
					}
					break;
				case Replacer::stRPar:
					THROW_PP(!oneof2(single_op, Replacer::stOpTo, Replacer::stOpSignal), PPERR_TXA_GRPINTARGET);
					{
						SSzChunk g;
						THROW_PP(grp_stack.getPointer() > 0, PPERR_TXA_PARMISM);
						THROW_PP(p_current_chain, PPERR_TXA_PARMISM);
						grp_stack.pop(g);
						g.Len = p_current_chain->List.getCount() - g.Begin;
						THROW_SL(p_current_chain->GL.insert(&g));
					}
					break;
				case Replacer::stOpOr:
					THROW_PP(grp_stack.getPointer() == 0, PPERR_TXA_PARMISM);
					THROW_PP(p_current_chain, PPERR_TXA_LEFTEMPTYOR);
					or_list.insert(p_current_chain);
					p_current_chain = 0;
					break;
				case Replacer::stOpTo:
				case Replacer::stOpSignal:
				case Replacer::stOpLower:
				case Replacer::stOpUpper:
				case Replacer::stOpCapital:
				case Replacer::stOpCortege:
					{
						THROW_PP(!(rReplacer.GetState() & Replacer::psCluster), PPERR_TXA_INVOPINCLUSTER);
						THROW_PP(single_op == 0, PPERR_TXA_OPSINGLETONVIOL);
						THROW_PP(grp_stack.getPointer() == 0, PPERR_TXA_PARMISM);
						single_op = term;
						if(!do_close_cluster) {
							THROW_PP(p_current_chain, PPERR_TXA_LEFTEMPTYTO);
							THROW_PP(!(p_current_chain->Flags & Replacer::SrcItem::fContext) || term == Replacer::stOpSignal, PPERR_TXA_INVOPONCONTEXT);
							if(or_list.getCount()) {
								or_list.insert(p_current_chain);
								p_current_chain = 0;
							}
							else {
								SETIFZ(p_src, new Replacer::SrcItem);
								THROW_MEM(p_src);
								p_src->Op = term;
								assert(term != Replacer::stOpCortege || cortege_id != 0);
								p_src->TargetIdx = (term == Replacer::stOpCortege) ? cortege_id : 0;
								p_src->Flags = p_current_chain->Flags;
								p_src->List = p_current_chain->List;
								p_src->GL = p_current_chain->GL;
								ZDELETE(p_current_chain);
							}
						}
					}
					break;
				case Replacer::stOpFrom:
					THROW_PP(!(rReplacer.GetState() & Replacer::psCluster), PPERR_TXA_INVOPINCLUSTER);
					THROW_PP(single_op == 0, PPERR_TXA_OPSINGLETONVIOL);
					THROW_PP(grp_stack.getPointer() == 0, PPERR_TXA_PARMISM);
					THROW_PP(or_list.getCount() == 0, PPERR_TXA_LEFTFROMORLIST);
					THROW_PP(p_current_chain, PPERR_TXA_LEFTEMPTYFROM);
					single_op = term;
					SETIFZ(p_target, new Replacer::TargetItem);
					THROW_MEM(p_target);
					p_target->List = p_current_chain->List;
					ZDELETE(p_current_chain);
					break;
					//
				case Replacer::stSpace:
				case Replacer::stSpaceMul:
				case Replacer::stSpaceSingle:
				case Replacer::stEmpty:
				case Replacer::stTab:
				case Replacer::stDotOpt:
				case Replacer::stHyphenOpt:
				case Replacer::stWord:
				case Replacer::stNum:
				case Replacer::stNumL:
				case Replacer::stFloat:
				case Replacer::stBegin:
				case Replacer::stEnd:
				case Replacer::stAny:
					SETIFZ(p_current_chain, new Replacer::SrcItem);
					THROW_MEM(p_current_chain);
					if(term == Replacer::stNumL) {
						const int16 len = (int16)term_ext_buf.ToLong();
						THROW_PP(len > 0 && len < 100, PPERR_TXT_TERMD_NEED2DIGIT);
						THROW(p_current_chain->List.Add(term, len, 0));
					}
					else {
						THROW(p_current_chain->List.Add(term, 0, 0));
					}
					break;
				case Replacer::stCortege:
					{
						uint   idx_first = 0, idx_count = 0;
						THROW_PP_S(term_ext_buf.NotEmpty(), PPERR_TXA_UNDEFCORTEGENAME, term_ext_buf);
						THROW_SL(RunSString(0, 0, term_ext_buf, &idx_first, &idx_count));
						THROW_PP_S(idx_count == 1, PPERR_TXA_CORTEGENAMENSINGLE, term_ext_buf);
						THROW_PP_S(Get(idx_first, item_), PPERR_TXA_UNDEFCORTEGENAME, term_ext_buf);
						{
							uint   cid = item_.TextId;
							const LongArray * p_list = rReplacer.SearchCortege(cid);
							THROW_PP_S(p_list, PPERR_TXA_UNDEFCORTEGEID, term_ext_buf);
							SETIFZ(p_current_chain, new Replacer::SrcItem);
							THROW_MEM(p_current_chain);
							THROW(p_current_chain->List.Add(term, 0, cid));
						}
						term_ext_buf = 0;
					}
					break;
				default:
					if(term >= 1 && term <= 9) {
						THROW_PP(!(rReplacer.GetState() & Replacer::psCluster), PPERR_TXA_GRPSUBSTINCLUSTER);
						THROW_PP(single_op != Replacer::stOpFrom, PPERR_TXA_GRPSUBSTINSRC);
						SETIFZ(p_current_chain, new Replacer::SrcItem);
						THROW_MEM(p_current_chain);
						p_current_chain->List.Add(term, 0, 0);
					}
					break;
			}
		}
		else {
			temp_buf.CatChar(scan[0]);
			scan.Incr();
		}
	}
	THROW_PP(grp_stack.getPointer() == 0, PPERR_TXA_PARMISM);
	temp_buf.Strip(1); // хвостовые (1) пробелы убираем
	if(temp_buf.NotEmpty()) {
		uint   j;
		uint   idx_first = 0, idx_count = 0;
		THROW_SL(RunSString(0, 0, temp_buf, &idx_first, &idx_count));
		for(j = 0; j < idx_count; j++) {
			if(Get(idx_first+j, item_)) {
				SETIFZ(p_current_chain, new Replacer::SrcItem);
				THROW_MEM(p_current_chain);
				p_current_chain->List.Add(Replacer::stLiteral, item_.Token, item_.TextId);
			}
		}
		if(or_list.getCount()) {
			THROW_PP(p_current_chain, PPERR_TXA_LEFTEMPTYOR);
			or_list.insert(p_current_chain);
			p_current_chain = 0;
		}
		temp_buf.Z();
	}
	else if(p_current_chain) {
		if(or_list.getCount()) {
			or_list.insert(p_current_chain);
			p_current_chain = 0;
		}
	}
	{
		uint   target_idx = 0;
		switch(single_op) {
			case 0:
				THROW_PP(rReplacer.GetState() & Replacer::psCluster, PPERR_TXA_NOOPOUTOFCLUSTER);
				if(or_list.getCount()) {
					assert(or_list.getCount() > 1); // Случай единственного терма в or_list должен быть обработан как ошибка выше
					for(uint i = 0; i < or_list.getCount(); i++) {
						THROW(p_src = rReplacer.MakeSrcItem(p_src, single_op, target_idx, or_list.at(i)->List, or_list.at(i)->GL));
						THROW(rReplacer.AddClusterItem(p_src));
						p_src = 0;
					}
				}
				else if(p_current_chain) {
					THROW(p_src = rReplacer.MakeSrcItem(p_src, Replacer::stOpTo, target_idx, p_current_chain->List, p_current_chain->GL));
					THROW(rReplacer.AddClusterItem(p_src));
					p_src = 0;
				}
				break;
			case Replacer::stOpFrom:
				assert(p_target != 0);
				THROW_PP(!(rReplacer.GetState() & Replacer::psCluster), PPERR_TXA_INVOPINCLUSTER);
				target_idx = rReplacer.SearchTarget(p_target->List);
				if(!target_idx) {
					rReplacer.TargetList.insert(p_target);
					target_idx = rReplacer.TargetList.getCount();
					assert(rReplacer.SearchTarget(p_target->List) == target_idx); // Проверим инвариант
				}
				else {
					ZDELETE(p_target);
				}
				if(or_list.getCount()) {
					assert(or_list.getCount() > 1); // Случай единственного терма в or_list должен быть обработан как ошибка выше
					for(uint i = 0; i < or_list.getCount(); i++) {
						assert(p_src == 0);
						THROW(p_src = rReplacer.MakeSrcItem(p_src, Replacer::stOpTo, target_idx, or_list.at(i)->List, or_list.at(i)->GL));
						THROW_SL(rReplacer.SrcList.insert(p_src));
						p_src = 0;
					}
				}
				else {
					assert(p_src == 0);
					THROW_PP(p_current_chain, PPERR_TXA_LEFTEMPTYTO);
					THROW(p_src = rReplacer.MakeSrcItem(p_src, Replacer::stOpTo, target_idx, p_current_chain->List, p_current_chain->GL));
					THROW_SL(rReplacer.SrcList.insert(p_src));
					ZDELETE(p_current_chain);
					p_src = 0;
				}
				break;
			case Replacer::stOpTo:
			case Replacer::stOpSignal:
			case Replacer::stOpLower:
			case Replacer::stOpUpper:
			case Replacer::stOpCapital:
			case Replacer::stOpCortege:
				assert(p_target == 0);
				THROW_PP(!(rReplacer.GetState() & Replacer::psCluster), PPERR_TXA_INVOPINCLUSTER);
				if(oneof2(single_op, Replacer::stOpTo, Replacer::stOpSignal)) {
					THROW_PP(p_current_chain && p_current_chain->List.getCount(), PPERR_TXA_RIGHTEMPTYTO);
					target_idx = rReplacer.SearchTarget(p_current_chain->List);
					if(!target_idx) {
						THROW_SL(p_target = rReplacer.TargetList.CreateNewItem());
						p_target->List = p_current_chain->List;
						target_idx = rReplacer.TargetList.getCount();
						assert(rReplacer.SearchTarget(p_current_chain->List) == target_idx); // Проверим инвариант
						p_target = 0;
					}
				}
				else if(single_op == Replacer::stOpCortege) {
					assert(cortege_id != 0);
					target_idx = cortege_id;
				}
				if(do_close_cluster) {
					assert(rReplacer.P_Cluster && rReplacer.P_Cluster->getCount());
					for(uint i = 0; i < rReplacer.P_Cluster->getCount(); i++) {
						const Replacer::SrcItem * p_clu_item = rReplacer.P_Cluster->at(i);
						THROW(p_src = rReplacer.MakeSrcItem(p_src, single_op, target_idx, p_clu_item->List, p_clu_item->GL));
						THROW_SL(rReplacer.SrcList.insert(p_src));
						if(single_op == Replacer::stOpCortege) {
							THROW(rReplacer.AddCortegeItem(target_idx, rReplacer.SrcList.getCount()-1));
						}
						p_src = 0;
					}
					rReplacer.P_Cluster->freeAll();
				}
				else if(or_list.getCount()) {
					assert(or_list.getCount() > 1); // Случай единственного терма в or_list должен быть обработан как ошибка выше
					for(uint i = 0; i < or_list.getCount(); i++) {
						THROW(p_src = rReplacer.MakeSrcItem(p_src, single_op, target_idx, or_list.at(i)->List, or_list.at(i)->GL));
						THROW_SL(rReplacer.SrcList.insert(p_src));
						if(single_op == Replacer::stOpCortege) {
							THROW(rReplacer.AddCortegeItem(target_idx, rReplacer.SrcList.getCount()-1));
						}
						p_src = 0;
					}
				}
				else {
					assert(p_src != 0); // Все случаи, когда p_src == 0 && or_list.getCount() == 0 должны быть уже обработаны выше
					//
					// Необходимо только уточнить индекс цели и вставить объект в контейнер
					//
					p_src->TargetIdx = target_idx;
					THROW_SL(rReplacer.SrcList.insert(p_src));
					if(single_op == Replacer::stOpCortege) {
						THROW(rReplacer.AddCortegeItem(target_idx, rReplacer.SrcList.getCount()-1));
					}
					p_src = 0;
				}
				break;
		}
	}
	//
	// Отладочная проверка инвариантов
	//
	for(uint i = 0; i < rReplacer.SrcList.getCount(); i++) {
		const Replacer::SrcItem * p_item = rReplacer.SrcList.at(i);
		assert(p_item);
		if(p_item->Op == Replacer::stOpCortege) {
			assert(p_item->TargetIdx != 0);
			GetTextById(p_item->TargetIdx, temp_buf);
			assert(temp_buf.NotEmpty());
			assert(temp_buf.HasChr(' ') == 0);
			{
				const LongArray * p_list = rReplacer.SearchCortege(p_item->TargetIdx);
				assert(p_list);
				assert(p_list->lsearch((long)i));
			}
		}
		else if(p_item->TargetIdx) {
			assert(p_item->TargetIdx <= rReplacer.TargetList.getCount());
		}
		for(uint j = 0; j < p_item->GL.getCount(); j++) {
			const SSzChunk & r_grp = p_item->GL.at(j);
			assert(r_grp.Begin >= 0 && r_grp.Begin < (int)p_item->List.getCount());
			assert(r_grp.Len > 0 && r_grp.Len <= (int)p_item->List.getCount());
			assert((r_grp.Begin + r_grp.Len) <= (int)p_item->List.getCount());
		}
	}
	CATCHZOK
	delete p_current_chain;
	return ok;
}

int SLAPI PPTextAnalyzer::ParseReplacerFile(const char * pFileName, Replacer & rRpl)
{
	int    ok = 1;
	int    parsing_in_progress = 0;
	SString line_buf;
	SFile inf(pFileName, SFile::mRead);
	THROW_SL(inf.IsValid());
	rRpl.InitParsing(inf.GetName());
	parsing_in_progress = 1;
	while(inf.ReadLine(line_buf)) {
		line_buf.Chomp().Strip();
		rRpl.IncLineNo();
		if(line_buf.NotEmpty()) {
			THROW(ParseReplacerLine(line_buf, rRpl));
		}
	}
	THROW_PP(!(rRpl.GetState() & Replacer::psCluster), PPERR_TXA_CLUSTERSTARTUNCLOSED);
	parsing_in_progress = 0;
	THROW(rRpl.BuildSrcIndex());
	CATCH
		ok = 0;
		if(parsing_in_progress) {
			SString msg_buf, added_msg_buf;
			PPGetLastErrorMessage(DS.CheckExtFlag(ECF_SYSSERVICE), added_msg_buf);
			msg_buf.FormatFileParsingMessage(rRpl.FileName, (int)rRpl.LineNo, added_msg_buf);
			PPSetError(PPERR_TXA_PARSINGFAULT, msg_buf);
		}
	ENDCATCH
	return ok;
}

#if 0 // {

%z%>%_
д/%w%>для #1
д\%w%>для #1
лореаль%>l'oreal
помада губная%<помада для губ%|губная помада
сто рецептов красоты%K

#endif // } 0

SLAPI PPTextAnalyzer::PPTextAnalyzer() : STokenizer(), SignalProc(0), P_SignalProcExtra(0)
{
	Param p;
	p.Delim = " \t\n\r(){}[]<>,.:;-\\/&$#@!?*^\"+=%\xA0";
	p.Flags |= (fDivAlNum|fEachDelim);
	SetParam(&p);
}

SLAPI PPTextAnalyzer::~PPTextAnalyzer()
{
}

void SLAPI PPTextAnalyzer::SetSignalProc(TextAnalyzerSignalProc proc, void * pProcExtra)
{
	SignalProc = proc;
	P_SignalProcExtra = pProcExtra;
}

/*struct NnBlock {
	NnBlock();
};*/

int SLAPI PPTextAnalyzer::ProcessGoodsNN()
{
	int    ok = 1;
	float * p_result = 0;
	float * p_nn_input = 0;
	float * p_nn_output = 0;
	float * p_nn_test_output = 0;
	Fann * p_ann = 0;
	uint   max_inp_tokens = 0;
	LongArray group_list;
	LongArray brand_list;
	SString ident;
	SString text;
	SString norm_code; // Нормализованное представление штрихкода
	SString temp_buf;
	SString fmt_buf;
	StringSet code_list;
	STokenizer::Item token;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	Goods2Tbl::Rec parent_rec;
	GoodsCore * p_tbl = goods_obj.P_Tbl;
	long    iter_total = 0;
	StatBase sb_tokens; // Статистика по количеству токенов в наименованиях
	PPGetFilePath(PPPATH_LOG, "pptextanalyzer-nn.log", temp_buf);
	SFile   f_out(temp_buf, SFile::mWrite);
	PPWait(1);
	{
		PPLoadText(PPTXT_GETTINGFULLTEXTLIST, fmt_buf);
		PPWaitMsg(fmt_buf);
		BExtQuery q(p_tbl, 0, 24);
		q.select(p_tbl->ID, p_tbl->Name, p_tbl->ParentID, p_tbl->BrandID, 0L).where(p_tbl->Kind == PPGDSK_GOODS);
		Goods2Tbl::Key0 k0;
		for(q.initIteration(0, &k0, spFirst); q.nextIteration() > 0;) {
			p_tbl->copyBufTo(&goods_rec);
			int   unclssf = 0;
			if(goods_obj.Fetch(goods_rec.ParentID, &parent_rec) > 0 && parent_rec.Flags & GF_UNCLASSF) {
				unclssf = 1;
			}
			if(!unclssf) {
				iter_total++;
				(text = goods_rec.Name).ToLower().Transf(CTRANSF_INNER_TO_OUTER);
				if(text.NotEmptyS()) {
					Reset(0);
					(ident = "#OBJ").Dot().Cat(PPOBJ_GOODS).Dot().Cat(goods_rec.ID);
					uint   first_token_pos = 0;
					uint   token_count = 0;
					THROW_SL(RunSString(ident, 0, text, &first_token_pos, &token_count));
					{
						uint   real_tok_count = 0;
						for(uint i = 0; i < token_count; i++) {
							if(Get(first_token_pos+i, token)) {
								if(token.Token == STokenizer::tokWord && token.Text != ",")
									real_tok_count++;
							}
						}
						SETMAX(max_inp_tokens, real_tok_count);
						sb_tokens.Step((double)real_tok_count);
					}
					group_list.addnz(goods_rec.ParentID);
					brand_list.addnz(goods_rec.BrandID);
				}
			}
			if((iter_total % 1000) == 0) {
				(temp_buf = fmt_buf).Space().Cat(iter_total);
				PPWaitMsg(temp_buf);
			}
		}
		sb_tokens.Finish();
	}
	group_list.sortAndUndup();
	brand_list.sortAndUndup();
	{
		STokenizer::Reset(0 /* Внутренний символьный хэш не очищается */);
		//
		size_t inp_token_limit = 0; // Количество токенов, которые будут использоваться как входы для сети
		//inp_token_limit = max_inp_tokens;
		const double stddev = sb_tokens.GetStdDev();
		inp_token_limit = MIN((size_t)(sb_tokens.GetExp() + 3 * stddev), (size_t)sb_tokens.GetMax());
		//
		const size_t max_barcode_input_digits = 7;
		const size_t input_count = inp_token_limit + 1 + 1; // 1 - бренд или группа, 1 - часть штрихкода
		const size_t output_count = group_list.getCount();
		const float learning_rate = 0.06f;
		//uint   layers[] = { input_count, output_count * 4, output_count };
		LongArray _layers;
		_layers.addzlist(input_count, output_count * 4, output_count, 0);
		PPLoadText(PPTXT_NNBUILDING, fmt_buf);
		long   iter_no = 0;
		SString input_log_buf, output_log_buf;
		PPWaitMsg(fmt_buf);
		THROW_MEM(p_result = new float[output_count]);
		THROW_MEM(p_nn_input = new float[input_count]);
		THROW_MEM(p_nn_output = new float[output_count]);
		THROW_MEM(p_nn_test_output = new float[output_count]);
		THROW(p_ann = fann_create_standard_array(/*SIZEOFARRAY(layers), layers*/_layers));
		p_ann->SetTrainingAlgorithm(Fann::FANN_TRAIN_INCREMENTAL);
		p_ann->SetLearningRate(learning_rate);
		{
			(temp_buf = fmt_buf).Transf(CTRANSF_INNER_TO_OUTER);
            temp_buf.CatDiv(':', 2).Cat("dimension").CatDiv('{', 1);
			for(uint d = 0; d < _layers.getCount(); d++) {
				if(d)
					temp_buf.CatDiv(',', 2);
                temp_buf.Cat(_layers.get(d));
			}
			temp_buf.CatDiv('}', 1);
			temp_buf.CatEq("rate", learning_rate, MKSFMTD(0, 4, 0));
            f_out.WriteLine(temp_buf.CR());
		}
		// fann_set_activation_function_hidden(p_ann, FANN_SIGMOID);
		{
			BExtQuery q(p_tbl, 0, 24);
			q.select(p_tbl->ID, p_tbl->Name, p_tbl->ParentID, p_tbl->BrandID, 0L).where(p_tbl->Kind == PPGDSK_GOODS);
			Goods2Tbl::Key0 k0;
			BarcodeArray codes;
			BarcodeTbl::Rec bc_rec;
			RAssocArray temp_result_list;
			for(q.initIteration(0, &k0, spFirst); q.nextIteration() > 0;) {
				p_tbl->copyBufTo(&goods_rec);
				int   unclssf = 0;
				if(goods_obj.Fetch(goods_rec.ParentID, &parent_rec) > 0 && parent_rec.Flags & GF_UNCLASSF) {
					unclssf = 1;
				}
				if(!unclssf) {
					iter_no++;
					uint   group_pos = 0;
					if(goods_rec.Name[0] && group_list.bsearch(goods_rec.ParentID, &group_pos)) {
						assert(group_pos < output_count);
						(text = goods_rec.Name).ToLower().Transf(CTRANSF_INNER_TO_OUTER);
						if(text.NotEmptyS()) {
							Reset(0);
							p_tbl->ReadBarcodes(goods_rec.ID, codes);
							if(codes.getCount() == 0) {
								MEMSZERO(bc_rec);
								codes.insert(&bc_rec);
							}
							(ident = "#OBJ").Dot().Cat(PPOBJ_GOODS).Dot().Cat(goods_rec.ID);
							uint   first_token_pos = 0;
							uint   token_count = 0;
							memzero(p_nn_output, output_count * sizeof(*p_nn_output));
							p_nn_output[group_pos] = 1.0f;
							THROW_SL(RunSString(ident, 0, text, &first_token_pos, &token_count));
							code_list.clear();
							if(codes.getCount()) {
								for(uint j = 0; j < codes.getCount(); j++) {
									int    code_std = 0;
									int    code_diag = 0;
									int    dcr = PPObjGoods::DiagBarcode(codes.at(j).Code, &code_diag, &code_std, &norm_code);
									if(dcr > 0) {
										if(code_std == BARCSTD_UPCE) {
											PPBarcode::ConvertUpceToUpca(norm_code, temp_buf);
											temp_buf.Trim(7);
										}
										else if(code_std == BARCSTD_UPCA) {
											(temp_buf = norm_code).Trim(max_barcode_input_digits);
										}
										else if(code_std == BARCSTD_EAN13) {
											(temp_buf = norm_code).Trim(max_barcode_input_digits);
										}
										else if(code_std == BARCSTD_EAN8) {
											(temp_buf = norm_code).Trim(MIN(5, max_barcode_input_digits));
										}
										else {
											temp_buf.Z();
										}
										if(temp_buf.NotEmpty()) {
											code_list.add(temp_buf);
										}
									}
								}
							}
							if(code_list.getCount() == 0) {
								temp_buf.Z().CatChar('.');
								code_list.add(temp_buf);
							}
							for(uint j = 0; code_list.get(&j, norm_code);) {
								input_log_buf = 0;
								uint   slot_pos = 0;
								memzero(p_nn_input, input_count * sizeof(*p_nn_input));
								for(uint i = 0, inp_token_count = 0; inp_token_count < inp_token_limit && i < token_count; i++) {
									if(Get(first_token_pos+i, token)) {
										if(token.Token == STokenizer::tokWord && token.Text != ",") {
											p_nn_input[slot_pos++] = (float)token.TextId;
											input_log_buf.Cat(token.Text).Space();
											inp_token_count++;
										}
									}
								}
								{
									uint   brand_pos = 0;
									if(goods_rec.BrandID && brand_list.bsearch(goods_rec.BrandID, &brand_pos))
										p_nn_input[max_inp_tokens] = (float)(brand_pos+1);
									else
										p_nn_input[max_inp_tokens] = 0.0f;
									input_log_buf.CatDiv(',', 2).Cat(goods_rec.BrandID).Space();
								}
								{
									const long codev = norm_code.ToLong();
									p_nn_input[max_inp_tokens] = (codev != 0) ? (float)codev : 0.0f;
									input_log_buf.CatDiv(',', 2).Cat(p_nn_input[max_inp_tokens], MKSFMTD(0, 0, 0)).Space();
								}
								input_log_buf.CatDiv('=', 2).Cat(goods_rec.ParentID);
								input_log_buf.Space().Cat((temp_buf = parent_rec.Name).Transf(CTRANSF_INNER_TO_OUTER));
								f_out.WriteLine(input_log_buf.CR());
								p_ann->TrainWithOutput(p_nn_input, p_nn_output, p_nn_test_output);
								{
									temp_result_list.clear();
									//output_log_buf.Z().Tab();
									for(uint oi = 0; oi < output_count; oi++) {
										if(p_nn_test_output[oi] != 0.0f) {
											const PPID _rid = group_list.get(oi);
											const float _rv = p_nn_test_output[oi];
											temp_result_list.Add(_rid, _rv, 0);
											//output_log_buf.Cat(_rid).CatChar('=').Cat(p_nn_test_output[oi], MKSFMTD(0, 6, NMBF_NOTRAILZ)).Space();
										}
									}
									temp_result_list.SortByValRev();
									for(uint rli = 0; rli < temp_result_list.getCount(); rli++) {
										const RAssoc & r_a = temp_result_list.at(rli);
										output_log_buf.Z().Tab().Cat(r_a.Val, MKSFMTD(0, 6, NMBF_NOTRAILZ));
										GetGoodsName(r_a.Key, temp_buf);
										output_log_buf.Tab().Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
										f_out.WriteLine(output_log_buf.CR());
									}
									//f_out.WriteLine(output_log_buf.CR());
								}
							}
						}
					}
				}
				PPWaitPercent(iter_no, iter_total, fmt_buf);
			}
		}
	}
	CATCHZOK
	delete [] p_result;
	delete [] p_nn_input;
	delete [] p_nn_output;
	delete [] p_nn_test_output;
	fann_destroy(p_ann);
	PPWait(0);
	return ok;
}

int SLAPI PPTextAnalyzer::ProcessGoods()
{
	int    ok = 1;
	SString ident;
	SString text;
	PPObjGoods goods_obj;
	Goods2Tbl * p_tbl = goods_obj.P_Tbl;
	BExtQuery q(p_tbl, 0, 24);
	q.select(p_tbl->ID, p_tbl->Name, 0L).where(p_tbl->Kind == PPGDSK_GOODS);
	Goods2Tbl::Key0 k0;
	for(q.initIteration(0, &k0, spFirst); q.nextIteration() > 0;) {
		(text = p_tbl->data.Name).ToLower().Transf(CTRANSF_INNER_TO_OUTER);
		if(text.NotEmptyS()) {
			(ident = "#OBJ").Dot().Cat(PPOBJ_GOODS).Dot().Cat(p_tbl->data.ID);
			THROW_SL(RunSString(ident, 0, text, 0, 0));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPTextAnalyzer::ProcessPerson()
{
	int    ok = 1;
	SString ident;
	SString text;
	PPObjPerson psn_obj;
	PersonTbl * p_tbl = psn_obj.P_Tbl;
	BExtQuery q(p_tbl, 0, 24);
	q.select(p_tbl->ID, p_tbl->Name, 0L);
	PersonTbl::Key0 k0;
	for(q.initIteration(0, &k0, spFirst); q.nextIteration() > 0;) {
		(text = p_tbl->data.Name).ToLower().Transf(CTRANSF_INNER_TO_OUTER);
		if(text.NotEmptyS()) {
			(ident = "#OBJ").Dot().Cat(PPOBJ_PERSON).Dot().Cat(p_tbl->data.ID);
			THROW_SL(RunSString(ident, 0, text, 0, 0));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPTextAnalyzer::ProcessString(const PPTextAnalyzer::Replacer & rRpl, const char * pResource, const SString & rOrg, SString & rResult, PPTextAnalyzer::FindBlock * pOuterFb, SFile * pDebugFile)
{
	const char * p_resource = pResource;

	int    ok = -1;
	uint   i, j;
	FindBlock fb(rRpl);
	FindBlock * p_fb = NZOR(pOuterFb, &fb);
	SString temp_buf;
	uint   idx_first = 0, idx_count = 0;
	THROW_SL(RunSString(p_resource, 0, rOrg, &idx_first, &idx_count));
	if(idx_count) {
		THROW(IndexText(*p_fb, idx_first, idx_first+idx_count-1));
		if(pDebugFile) {
			temp_buf.Z();
			for(j = 0; j < idx_count; j++) {
				if(Get(idx_first+j, p_fb->Item_))
					temp_buf.Cat(p_fb->Item_.Text);
			}
			pDebugFile->WriteLine(temp_buf.CR());
			//
			for(i = 0; i < rRpl.SrcList.getCount(); i++) {
				const Replacer::SrcItem * p_src_item = rRpl.SrcList.at(i);
				const uint _c = p_src_item ? p_src_item->List.getCount() : 0;
				if(_c) {
					uint si = idx_first;
					uint fi = idx_first + idx_count - 1;
					p_fb->Init(p_src_item, si, fi);
					while(si <= fi && Helper_FindReplacerSrcItem(p_fb->Reset(), 0, _c-1, si, fi, 0)) {
						ReplacerSrcItemToStr(rRpl, p_src_item, temp_buf);
						temp_buf.Insert(0, "\t");
						temp_buf.CatDiv(':', 2);
						assert(p_fb->getCount());
						assert(p_fb->at(0).GrpIdx == 0);
						const PPTextAnalyzer::FindItem & r_fi = p_fb->at(0);
						for(j = r_fi.IdxFirst; j <= r_fi.IdxLast; j++) {
							Get(j, p_fb->Item_);
							temp_buf.Cat(p_fb->Item_.Text);
						}
						pDebugFile->WriteLine(temp_buf.CR());
						si = r_fi.IdxFirst+1;
					}
				}
			}
		}
		{
			//
			// Обработка ведется в последовательности, определенной индексом Replacer::SrcListIndex
			// Необходимость индексации обусловлена тем, что сначала следует сделать собственно замены, а уже
			// в последнюю очередь - изменение регистров символов.
			// Построение индекса осуществляется фукнцией Replacer::BuildSrcIndex, которая вызывается после
			// разбора файла правил в функции PPTextAnalyzer::ParseReplacerFile(const char *, Replacer &)
			//
			for(i = 0; i < rRpl.SrcListIndex.getCount(); i++) {
				const uint src_pos = (uint)rRpl.SrcListIndex.get(i);
				const Replacer::SrcItem * p_src_item = rRpl.SrcList.at(src_pos);
				PROFILE_START
				if(DoReplacement(rRpl, p_fb->Init(p_src_item, idx_first, idx_first + idx_count - 1), temp_buf) > 0) {
					if(p_src_item->Op == Replacer::stOpSignal) {
						if(SignalProc) {
							int spr = SignalProc(p_resource, 0, rOrg, temp_buf, P_SignalProcExtra);
							THROW(spr);
							if(spr < 0)
								break;
						}
						CALLPTRMEMB(pDebugFile, WriteLine(temp_buf.Insert(0, "\t signal:").CR()));
					}
					else {
						rResult = temp_buf;
						CALLPTRMEMB(pDebugFile, WriteLine(temp_buf.Insert(0, "\t").CR()));
						//
						// При повторной обработке текста не передаем p_resource дабы не возникала
						// путаница между оригинальым текстом и промежуточным результатом.
						//
						THROW_SL(RunSString(0, 0, rResult, &idx_first, &idx_count));
						THROW(IndexText(*p_fb, idx_first, idx_first+idx_count-1));
						ok = 1;
					}
				}
				PROFILE_END
			}
		}
	}
	CATCH
		rResult.Z();
		ok = 0;
	ENDCATCH
	return ok;
}

struct TestSignalProcExtra {
	SFile OutF;
	SString LineBuf;
};

static int TestSignalProc(const char * pResource, int64 orgOffs, const char * pOrgStr, const char * pSignalStr, void * pExtraPtr)
{
	TestSignalProcExtra * p_blk = (TestSignalProcExtra *)pExtraPtr;
	if(p_blk)
		p_blk->OutF.WriteLine(p_blk->LineBuf.Z().Cat(pResource).Tab().Cat(pOrgStr).Tab().Cat(pSignalStr).CR());
	return 1;
}

int SLAPI PPTextAnalyzer::Test()
{
	int    ok = 1;
	TestSignalProcExtra spe;
	PPWait(1);
	{
		SLS.SetCodepage(cp1251);

		Replacer rpl;
		SString line_buf, result_buf, file_name;
		SString input_file_name;
		{
			PPGetPath(PPPATH_TESTROOT, input_file_name);
			input_file_name.SetLastSlash().Cat("data\\replacer-rule.txt");
			THROW(ParseReplacerFile(input_file_name, rpl));
			{
				SPathStruc::ReplaceExt(file_name = input_file_name, "out", 1);
				SFile outf(file_name, SFile::mWrite);
				if(outf.IsValid()) {
					for(uint i = 0; i < rpl.SrcList.getCount(); i++) {
						const Replacer::SrcItem * p_src_item = rpl.SrcList.at(i);
						if(ReplacerSrcItemToStr(rpl, p_src_item, line_buf)) {
							outf.WriteLine(line_buf.CR());
						}
					}
				}
			}
		}
		{
			PPGetPath(PPPATH_TESTROOT, input_file_name);
			input_file_name.SetLastSlash().Cat("data\\replacer-input.txt");
			SFile inf(input_file_name, SFile::mRead);
			if(inf.IsValid()) {
				FindBlock fb(rpl);
				SPathStruc::ReplaceExt(file_name = inf.GetName(), "out", 1);
				SPathStruc::ReplaceExt(file_name = inf.GetName(), "csv", 1);
				SFile csvf(file_name, SFile::mWrite);
				{
					SPathStruc::ReplaceExt(file_name = inf.GetName(), "signal", 1);
					if(spe.OutF.Open(file_name, SFile::mWrite)) {
						SetSignalProc(TestSignalProc, &spe);
					}
				}
				long line_count = 0;
				while(inf.ReadLine(line_buf)) {
					line_count++;
				}
				inf.Seek(0);
				IterCounter cntr;
				cntr.Init(line_count);
				while(inf.ReadLine(line_buf)) {
					line_buf.Chomp().Strip().ToOem().ToLower().Transf(CTRANSF_INNER_TO_OUTER);
					Reset(0);
					THROW(ProcessString(rpl, 0, line_buf, result_buf, &fb, 0/*&outf*/));
					csvf.WriteLine(line_buf.Tab().Cat(result_buf).CR());
					PPWaitPercent(cntr.Increment());
				}
			}
		}
	}

	THROW(ProcessGoods());
	THROW(ProcessPerson());
	{
		SString out_file_name, line_buf;
		PPGetFilePath(PPPATH_IN, "pptextanalyzer.txt", out_file_name);
		SFile f_out(out_file_name, SFile::mWrite);
		Item   item;
		{
			for(uint i = 0; i < GetCommCount(); i++) {
				if(GetComm(i, item)) {
					line_buf.Z().Tab().Cat(item.Token).Tab().Cat(item.Text).CR();
					f_out.WriteLine(line_buf);
				}
			}
		}
		{
			for(uint i = 0; i < GetCount(); i++) {
				if(Get(i, item)) {
					line_buf.Z().Cat(item.Resource).Tab().Cat(item.Token).Tab().Cat(item.Text).CR();
					f_out.WriteLine(line_buf);
				}
			}
		}
	}
	PPWait(0);
	CATCHZOKPPERR
	return ok;
}

SLAPI PPTextAnalyzerWrapper::PPTextAnalyzerWrapper() : R(), Fb(R), Flags(0)
{
}

int SLAPI PPTextAnalyzerWrapper::Init(const char * pRuleFileName, long flags)
{
	int    ok = 1;
	SETFLAGBYSAMPLE(Flags, fEncInner, flags);
	if(A.ParseReplacerFile(pRuleFileName, R)) {
		Flags |= fInited;
	}
	else {
		Flags &= ~fInited;
		ok = 0;
	}
	return ok;
}

int SLAPI PPTextAnalyzerWrapper::ReplaceString(const char * pText, SString & rResult)
{
	rResult.Z();
	int    ok = -1;
	if(Flags & fInited) {
		(TempBuf = pText).Strip();
		if(Flags & fEncInner)
			TempBuf.Transf(CTRANSF_INNER_TO_OUTER);
		TempBuf.ToLower1251();
		A.Reset(0);
		ok = A.ProcessString(R, 0, TempBuf, rResult, &Fb, 0);
		rResult.SetIfEmpty(pText);
		if(Flags & fEncInner)
			rResult.Transf(CTRANSF_OUTER_TO_INNER);
	}
	else
		ok = 0;
	return ok;
}

IMPLEMENT_PPFILT_FACTORY(PrcssrObjText); SLAPI PrcssrObjTextFilt::PrcssrObjTextFilt() : 
	PPBaseFilt(PPFILT_PRCSSROBJTEXTPARAM, 0, 0), P_GoodsF(0), P_BrandF(0), P_PsnF(0)
{
	SetFlatChunk(offsetof(PrcssrObjTextFilt, ReserveStart),
		offsetof(PrcssrObjTextFilt, RuleFileName)-offsetof(PrcssrObjTextFilt, ReserveStart));
	SetBranchSString(offsetof(PrcssrObjTextFilt, RuleFileName));
	SetBranchBaseFiltPtr(PPFILT_GOODS, offsetof(PrcssrObjTextFilt, P_GoodsF));
	SetBranchBaseFiltPtr(PPFILT_BRAND, offsetof(PrcssrObjTextFilt, P_BrandF));
	SetBranchBaseFiltPtr(PPFILT_PERSON, offsetof(PrcssrObjTextFilt, P_PsnF));
	Init(1, 0);
}

PrcssrObjTextFilt & FASTCALL PrcssrObjTextFilt::operator = (const PrcssrObjTextFilt & rS)
{
	Copy(&rS, 0);
	return *this;
}

int SLAPI PrcssrObjTextFilt::IsEmpty() const
{
	if(ObjType)
		return 0;
	else if(ObjTextIdent)
		return 0;
	else if(Flags)
		return 0;
	else if(RuleFileName.NotEmpty())
		return 0;
	else if(P_GoodsF)
		return 0;
	else if(P_BrandF)
		return 0;
	else if(P_PsnF)
		return 0;
	else
		return 1;
}

SLAPI PrcssrObjText::PrcssrObjText() : P_Rpl(0)
{
}

SLAPI PrcssrObjText::~PrcssrObjText()
{
	delete P_Rpl;
}

//static
int SLAPI PrcssrObjText::VerifyRuleFile(const char * pFileName)
{
	int    ok = 1;
	THROW_SL(fileExists(pFileName));
	{
		PPTextAnalyzer ta;
		PPTextAnalyzer::Replacer rpl;
		THROW(ta.ParseReplacerFile(pFileName, rpl));
	}
	CATCHZOK
	return ok;
}

class ObjTextFiltDialog : public TDialog {
public:
	ObjTextFiltDialog() : TDialog(DLG_OBJTEXTFILT)
	{
	}
	int    setDTS(const PrcssrObjTextFilt * pData)
	{
		int    ok = 1;
		Data = *pData;
		SetupObjListCombo(this, CTLSEL_OBJTEXTFILT_OBJ, Data.ObjType);
		AddClusterAssoc(CTL_OBJTEXTFILT_FLAGS, 0, Data.fLog);
		AddClusterAssoc(CTL_OBJTEXTFILT_FLAGS, 1, Data.fReplace);
		AddClusterAssoc(CTL_OBJTEXTFILT_FLAGS, 2, Data.fSignal);
		AddClusterAssoc(CTL_OBJTEXTFILT_FLAGS, 3, Data.fNnClassifier); // @v9.2.7
		SetClusterData(CTL_OBJTEXTFILT_FLAGS, Data.Flags);

		FileBrowseCtrlGroup::Setup(this, CTLBRW_OBJTEXTFILT_RULEF, CTL_OBJTEXTFILT_RULEF, 1, 0,
			PPTXT_FILPAT_SARTRRULE, FileBrowseCtrlGroup::fbcgfFile);
		setCtrlString(CTL_OBJTEXTFILT_RULEF, Data.RuleFileName);
		enableCommand(cmEditFilt, oneof2(Data.ObjType, PPOBJ_GOODS, PPOBJ_PERSON));
		return ok;
	}
	int    getDTS(PrcssrObjTextFilt * pData)
	{
		int    ok = 1;
		uint   sel = 0;
		getCtrlData(CTLSEL_OBJTEXTFILT_OBJ, &Data.ObjType);
		GetClusterData(CTL_OBJTEXTFILT_FLAGS, &Data.Flags);
		getCtrlString(sel = CTL_OBJTEXTFILT_RULEF, Data.RuleFileName);
		if(!(Data.Flags & Data.fNnClassifier)) {
			THROW(PrcssrObjText::VerifyRuleFile(Data.RuleFileName));
		}
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = PPErrorByDialog(this, sel);
		ENDCATCH
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_OBJTEXTFILT_OBJ)) {
			getCtrlData(CTLSEL_OBJTEXTFILT_OBJ, &Data.ObjType);
			enableCommand(cmEditFilt, oneof2(Data.ObjType, PPOBJ_GOODS, PPOBJ_PERSON));
		}
		else if(event.isCmd(cmEditFilt)) {
			getCtrlData(CTLSEL_OBJTEXTFILT_OBJ, &Data.ObjType);
			if(Data.ObjType == PPOBJ_GOODS) {
				PPViewGoods view_goods;
				SETIFZ(Data.P_GoodsF, (GoodsFilt *)view_goods.CreateFilt(0));
				if(view_goods.EditBaseFilt(Data.P_GoodsF) > 0) {
					;
				}
			}
			else if(Data.ObjType == PPOBJ_PERSON) {
				PPViewPerson view_psn;
				SETIFZ(Data.P_PsnF, (PersonFilt *)view_psn.CreateFilt(0));
				if(view_psn.EditBaseFilt(Data.P_PsnF) > 0) {
					;
				}
			}
		}
		else
			return;
		clearEvent(event);
	}
	PrcssrObjTextFilt Data;
};

int SLAPI PrcssrObjText::InitParam(PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	if(P.IsA(pBaseFilt)) {
		PrcssrObjTextFilt * p_filt = (PrcssrObjTextFilt *)pBaseFilt;
		if(p_filt->IsEmpty()) {
			p_filt->ObjType = PPOBJ_GOODS;
			p_filt->ObjTextIdent = PrcssrObjTextFilt::otiName;
			p_filt->Flags |= PrcssrObjTextFilt::fLog;
		}
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PrcssrObjText::EditParam(PPBaseFilt * pBaseFilt)
{
	if(!P.IsA(pBaseFilt))
		return 0;
	PrcssrObjTextFilt * p_filt = (PrcssrObjTextFilt *)pBaseFilt;
	DIALOG_PROC_BODY(ObjTextFiltDialog, p_filt);
}

int SLAPI PrcssrObjText::Init(const PPBaseFilt * pBaseFilt)
{
	SLS.SetCodepage(cp1251);

	int    ok = 1;
	SString temp_buf, line_buf;
	THROW(P.IsA(pBaseFilt));
	P = *(PrcssrObjTextFilt *)pBaseFilt;
	Ta.Reset(STokenizer::coClearSymbTab);
	LogF.Close();
	ZDELETE(P_Rpl);
	if(P.Flags & P.fNnClassifier) {
	}
	else {
		THROW_SL(fileExists(P.RuleFileName));
		THROW_MEM(P_Rpl = new PPTextAnalyzer::Replacer);
		THROW(Ta.ParseReplacerFile(P.RuleFileName, *P_Rpl));
		if(P.Flags & P.fDebug) {
			SPathStruc::ReplaceExt(temp_buf = P.RuleFileName, "sr-debug", 1);
			SFile outf(temp_buf, SFile::mWrite);
			if(outf.IsValid()) {
				for(uint i = 0; i < P_Rpl->SrcList.getCount(); i++) {
					const PPTextAnalyzer::Replacer::SrcItem * p_src_item = P_Rpl->SrcList.at(i);
					if(Ta.ReplacerSrcItemToStr(*P_Rpl, p_src_item, line_buf))
						outf.WriteLine(line_buf.CR());
				}
			}
		}
		{
			SPathStruc::ReplaceExt(temp_buf = P.RuleFileName, "sr-log", 1);
			LogF.Close();
			LogF.Open(temp_buf, SFile::mWrite);
			if(LogF.IsValid()) {
				line_buf.Z().Cat(getcurdatetime_());
				//
				if(!SGetComputerName(temp_buf))
					temp_buf = "?COMP?";
				line_buf.Tab().Cat(temp_buf);
				//
				if(CurDict)
					CurDict->GetDbSymb(temp_buf);
				else
					temp_buf = "nologin";
				line_buf.Tab().Cat(temp_buf);
				//
				GetCurUserName(temp_buf);
				line_buf.Tab().Cat(temp_buf);
				//
				LogF.WriteLine(line_buf.CR());
			}
		}
	}
	CATCHZOK
	return ok;
}

//static
int PrcssrObjText::SignalProc(const char * pResource, int64 orgOffs, const char * pOrgStr, const char * pSignalStr, void * pExtraPtr)
{
	int    ok = 1;
	SignalProcBlock * p_blk = (SignalProcBlock *)pExtraPtr;
	PPObjID oi;
	oi.Set(0, 0);
	if(p_blk && oi.FromStr(pResource)) {
		if(oi.Obj == PPOBJ_GOODS) {
			if(!p_blk->GObj.SetupAttrByTextDescr(oi.Id, pSignalStr, !(p_blk->State & p_blk->stOuterTransaction))) {
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
			}
		}
	}
	return ok;
}

int SLAPI PrcssrObjText::Run()
{
	int    ok = 1, r;
	PPTransaction * p_tra = 0;
	if(P.Flags & P.fNnClassifier) {
		THROW(Ta.ProcessGoodsNN());
	}
	else {
		SString text_buf, result_buf, line_buf, temp_buf, resource_buf;
		THROW(P_Rpl);
		if(P.ObjType == PPOBJ_GOODS) {
			SignalProcBlock signal_blk;
			PPObjGoods goods_obj;
			PPTextAnalyzer::FindBlock fb(*P_Rpl);
			GoodsIterator goods_iter(P.P_GoodsF, 0, 0);
			Goods2Tbl::Rec goods_rec;
			PPObjID oi;
			uint   c = 0;
			PPWait(1);
			if(P.Flags & (P.fReplace|P.fSignal)) {
				THROW_MEM(p_tra = new PPTransaction(1));
				THROW(*p_tra);
				if(P.Flags & P.fSignal) {
					signal_blk.State |= signal_blk.stOuterTransaction;
					Ta.SetSignalProc(SignalProc, &signal_blk);
				}
			}
			while(goods_iter.Next(&goods_rec) > 0) {
				THROW(PPCheckUserBreak());
				c++;
				(text_buf = goods_rec.Name).ToLower().Transf(CTRANSF_INNER_TO_OUTER);
				Ta.Reset(0);
				oi.Set(PPOBJ_GOODS, goods_rec.ID).ToStr(resource_buf);
				THROW(r = Ta.ProcessString(*P_Rpl, resource_buf, text_buf, result_buf, &fb, 0/*&outf*/));
				if(LogF.IsValid()) {
					(temp_buf = goods_rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
					line_buf.Z().Cat("goods").Tab().CatChar('#').Cat(goods_rec.ID).Tab().Cat(temp_buf).Tab().Cat(result_buf).CR();
					LogF.WriteLine(line_buf);
				}
				if(r > 0 && P.Flags & P.fReplace) {
					if(result_buf != text_buf) {
						result_buf.ToOem();
						THROW(goods_obj.UpdateName(goods_rec.ID, result_buf, 0));
					}
				}
				if((c % 1000) == 0) {
					if(p_tra) {
						THROW(p_tra->Commit());
						ZDELETE(p_tra);
						THROW_MEM(p_tra = new PPTransaction(1));
						THROW(*p_tra);
					}
				}
				PPWaitPercent(goods_iter.GetIterCounter());
			}
			if(p_tra) {
				THROW(p_tra->Commit());
				ZDELETE(p_tra);
			}
		}
		else {
			if(P.ObjType == PPOBJ_PERSON){
				PPObjPerson psn_obj;
				PPViewPerson psn_view;
				PersonViewItem psn_item;
				PersonFilt psn_filt;
				PPPersonPacket psn_pack;
				PPTextAnalyzer::FindBlock fb(*P_Rpl);
				PPObjID oi;
				PPIDArray id_list;
				uint i;
				RVALUEPTR(psn_filt, P.P_PsnF);
				THROW(psn_view.Init_(&psn_filt));
				for(psn_view.InitIteration(); psn_view.NextIteration(&psn_item) > 0;) {
					id_list.add(psn_item.ID);
				}
				id_list.sortAndUndup();
				for(i = 0; i < id_list.getCount(); i++){
					PersonTbl::Rec psn_rec;
					PPID psn_id = id_list.get(i);
					if(psn_obj.Search(psn_id, &psn_rec) > 0){
						(text_buf = psn_rec.Name).ToLower().Transf(CTRANSF_INNER_TO_OUTER);
						oi.Set(PPOBJ_PERSON, psn_id).ToStr(resource_buf);
						THROW(r = Ta.ProcessString(*P_Rpl, resource_buf, text_buf, result_buf, &fb, 0));
						if(LogF.IsValid()) {
							(temp_buf = psn_item.Name).Transf(CTRANSF_INNER_TO_OUTER);
							line_buf.Z().Cat("person").Tab().CatChar('#').Cat(psn_id).Tab().Cat(temp_buf).Tab().Cat(result_buf).CR();
							LogF.WriteLine(line_buf);
						}
						if(r > 0 && P.Flags & P.fReplace) {
							if(result_buf != text_buf) {
								THROW(psn_obj.GetPacket(psn_id, &psn_pack, 0) > 0);
								result_buf.ToOem();
								STRNSCPY(psn_pack.Rec.Name, result_buf);
								THROW(psn_obj.PutPacket(&psn_id, &psn_pack, 1));
							}
						}
					}
				}

			}
			else {
				ok = -1;
			}
		}
	}
	CATCHZOK
	PPWait(0);
	delete p_tra;
	return ok;
}

int SLAPI DoProcessObjText(PrcssrObjTextFilt * pFilt)
{
	int    ok = -1;
	PrcssrObjText prcssr;
	if(pFilt) {
		if(prcssr.Init(pFilt) && prcssr.Run())
			ok = 1;
		else
			ok = PPErrorZ();
	}
	else {
		PrcssrObjTextFilt param;
		prcssr.InitParam(&param);
		if(prcssr.EditParam(&param) > 0)
			if(prcssr.Init(&param) && prcssr.Run())
				ok = 1;
			else
				ok = PPErrorZ();
	}
	return ok;
}
//
//
//
SLAPI PPObjectTokenizer::PPObjectTokenizer() : PPTextAnalyzer()
{
	Param tp;
	GetParam(&tp);
	tp.Flags &= ~fEachDelim;
	SetParam(&tp);
}

SLAPI PPObjectTokenizer::~PPObjectTokenizer()
{
}

SString & FASTCALL EncodeTokenizerResourceObj(PPObjID oi, SString & rBuf)
{
	return rBuf.Z().CatChar('#').Cat("OBJ").Dot().Cat(oi.Obj).Dot().Cat(oi.Id);
}

int FASTCALL DecodeTokenizerResourceObj(const SString & rBuf, PPObjID & rOi)
{
	int    ok = 0;
	SStrScan scan(rBuf);
	scan.Skip();
	const char * p_prefix = "#obj.";
	if(scan.Is(p_prefix)) {
		SString temp_buf;
		scan.Incr(sstrlen(p_prefix));
		if(scan.GetDigits(temp_buf)) {
			rOi.Obj = temp_buf.ToLong();
			if(scan[0] == '.') {
				scan.Incr();
				if(scan.GetDigits(temp_buf)) {
					rOi.Id = temp_buf.ToLong();
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int SLAPI PPObjectTokenizer::AddObject(PPObjID oi, const char * pName)
{
	int    ok = 1;
	(TextBuf = pName).Strip().ToLower().Transf(CTRANSF_INNER_TO_OUTER);
	if(TextBuf.NotEmpty()) {
		THROW_SL(RunSString(EncodeTokenizerResourceObj(oi, IdentBuf.Z()), 0, TextBuf, 0, 0));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPObjectTokenizer::ProcessSuprWare(PPID swType, PPID swCls)
{
	int    ok = 1;
	SuprWareFilt f_sw;
	PPViewSuprWare v_sw;
	SuprWareViewItem sw_item;
	f_sw.SuprWareType = swType;
	f_sw.SuprWareCat = swCls;
	THROW(v_sw.Init_(&f_sw));
	for(v_sw.InitIteration(); v_sw.NextIteration(&sw_item) > 0;) {
		PPObjID oi;
		oi.Set(PPOBJ_COMPGOODS, sw_item.ID);
		THROW(AddObject(oi, sw_item.Name));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjectTokenizer::ProcessGoods(const GoodsFilt * pFilt)
{
	int    ok = 1;
	Goods2Tbl::Rec goods_rec;
	for(GoodsIterator iter(pFilt, 0); iter.Next(&goods_rec) > 0;) {
		PPObjID oi;
		oi.Set(PPOBJ_GOODS, goods_rec.ID);
		THROW(AddObject(oi, goods_rec.Name));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjectTokenizer::SearchObjects(const char * pText, PPID objType, long flags, RAssocArray & rObjIdScoreList)
{
#ifdef NDEBUG
	const int debug_output = 0;
#else
	const int debug_output = 1;
#endif
	rObjIdScoreList.clear();
	int    ok = -1;
	SString text = pText;
	SString msg_buf, name_buf;
	if(text.NotEmptyS()) {
		SString temp_buf, src_word_buf;
		TSCollection <STokenizer::SearchBlockEntry> result;
		if(debug_output)
			PPLogMessage(PPFILNAM_DEBUG_LOG, (msg_buf = "SuprWareTokenizer").Tab().Cat(text), LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		text.ToLower().Transf(CTRANSF_INNER_TO_OUTER);
		ClearInput();
		Write(0, 0, text, text.Len()+1);
		if(Search(flags, result)) {
			uint   i;
			RAssocArray score_list;
			const double order_div = (double)result.getCount();
			for(i = 0; i < result.getCount(); i++) {
				const double order_coeff = (2.0 - ((i+1) / order_div));
				const SearchBlockEntry * p_entry = result.at(i);
				const int sw_r = GetTextById(p_entry->T, src_word_buf);
				const int _digital = BIN(src_word_buf.IsDigit());
				if(debug_output)
					if(sw_r > 0)
						PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf.Z().Tab().Cat((temp_buf = src_word_buf).ToOem()), LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
					else
						PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf.Z().Tab().CatChar('$').Cat(p_entry->T), LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
				for(uint j = 0; j < p_entry->RL.getCount(); j++) {
					if(GetTextById(p_entry->RL.at(j).RP, temp_buf) > 0) {
						PPObjID oi;
						if(DecodeTokenizerResourceObj(temp_buf, oi) && (!objType || objType == oi.Obj)) {
							if(debug_output) {
								if(objType && GetObjectName(objType, oi.Id, name_buf.Z()) > 0)
									PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf.Z().Tab().Tab().Cat(name_buf), LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
								else
									PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf.Z().Tab().Tab().Cat(temp_buf), LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
							}
							double score = (_digital ? 0.03 : 1.0) * order_coeff;
							rObjIdScoreList.Add(oi.Id, score);
							ok = 1;
						}
					}
				}
			}
			rObjIdScoreList.SortByVal();
			if(debug_output) {
				for(i = 0; i < rObjIdScoreList.getCount(); i++) {
					const RAssoc & r_rai = rObjIdScoreList.at(i);
					if(objType && GetObjectName(objType, r_rai.Key, name_buf.Z()) > 0) {
						msg_buf.Z().Tab().Cat(name_buf).Tab().Cat(r_rai.Val, SFMT_MONEY);
						PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
					}
				}
			}
		}
	}
	return ok;
}

struct _AnalogByComponentEntry {
	PPID   ComponentID;
	PPIDArray SuprWareList;
};

int SLAPI PPObjectTokenizer::SearchGoodsAnalogs(PPID goodsID, PPIDArray & rList, SString * pTransitComponentBuf)
{
	rList.clear();
	ASSIGN_PTR(pTransitComponentBuf, 0);

	int    ok = -1;
	uint   i;
	Goods2Tbl::Rec goods_rec;
	PPObjGoods goods_obj;
	if(goods_obj.Fetch(goodsID, &goods_rec) > 0) {
		RAssocArray sw_list;
		if(SearchObjects(goods_rec.Name, PPOBJ_COMPGOODS, sfFirstInTextOnly, sw_list) > 0) {
			const double epsilon = 1.0E-9;
			i = sw_list.getCount();
			PPIDArray id_list;
			if(i) {
				double prev_score = sw_list.at(i-1).Val;
				if(prev_score > 1.0) {
					do {
						const RAssoc & r_sw_item = sw_list.at(--i);
						if(fabs(r_sw_item.Val - prev_score) < epsilon) {
							prev_score = r_sw_item.Val;
							id_list.add(r_sw_item.Key);
						}
						else
							break;
					} while(i);
				}
				if(id_list.getCount()) {
					TSCollection <_AnalogByComponentEntry> sw_by_component_list;
					PPObjSuprWare sw_obj;
					PPSuprWarePacket sw_pack;
					PPIDArray component_list, by_component_list;
					LAssocArray target_to_component_list;
					RAssocArray final_by_component_list;
					for(i = 0; i < id_list.getCount(); i++) {
						if(sw_obj.Get(id_list.get(i), &sw_pack) > 0) {
							if(sw_pack.Rec.SuprWareType == SUPRWARETYPE_GOODS) {
								if(sw_pack.Items.getCount()) {
									for(uint j = 0; j < sw_pack.Items.getCount(); j++)
										component_list.add(sw_pack.Items.at(j).CompID);
								}
							}
							else if(sw_pack.Rec.SuprWareType == SUPRWARETYPE_COMPONENT) {
								component_list.add(sw_pack.Rec.ID);
							}
						}
					}
					component_list.sortAndUndup();
					for(i = 0; i < component_list.getCount(); i++) {
						const PPID c_id = component_list.get(i);
						by_component_list.clear();
						if(sw_obj.GetListByComponent(c_id, by_component_list) > 0 && by_component_list.getCount()) {
							_AnalogByComponentEntry * p_ace = sw_by_component_list.CreateNewItem();
							p_ace->ComponentID = c_id;
							p_ace->SuprWareList = by_component_list;
							for(uint j = 0; j < by_component_list.getCount(); j++) {
								const PPID by_component_id = by_component_list.get(j);
								final_by_component_list.Add(by_component_id, 1.0);
								target_to_component_list.Add(by_component_id, c_id, 0);
							}
						}
					}
					i = final_by_component_list.getCount();
					if(i) {
						PPIDArray transit_component_list;
						double prev_bc_score = final_by_component_list.at(i-1).Val;
						do {
							const RAssoc & r_a = final_by_component_list.at(--i);
							if(fabs(r_a.Val - prev_bc_score) < epsilon) {
								prev_bc_score = r_a.Val;
								Goods2Tbl::Rec sw_rec;
								if(sw_obj.Fetch(r_a.Key, &sw_rec) > 0) {
									RAssocArray analog_list;
									if(SearchObjects(sw_rec.Name, PPOBJ_GOODS, sfAllInPatternOnly, analog_list) > 0) {
										uint k = analog_list.getCount();
										if(k) {
											double prev_analog_score = analog_list.at(k-1).Val;
											do {
												const RAssoc & r_analog_item = analog_list.at(--k);
												if(fabs(r_analog_item.Val - prev_analog_score) < epsilon) {
													prev_analog_score = r_analog_item.Val;
													if(r_analog_item.Key != goodsID) {
														rList.add(r_analog_item.Key);
														{
															PPID   _c_id = 0;
															if(target_to_component_list.Search(r_a.Key, &_c_id, 0))
																transit_component_list.add(_c_id);
														}
														ok = 1;
													}
												}
												else
													break;
											} while(k);
										}
									}
								}
							}
						} while(i);
						if(pTransitComponentBuf) {
							(*pTransitComponentBuf) = 0;
							transit_component_list.sortAndUndup();
							for(i = 0; i < transit_component_list.getCount(); i++) {
								Goods2Tbl::Rec sw_rec;
								if(sw_obj.Fetch(transit_component_list.get(i), &sw_rec) > 0)
									pTransitComponentBuf->CatDivIfNotEmpty(',', 2).Cat(sw_rec.Name);
							}
						}
					}
				}
			}
		}
	}
	rList.sortAndUndup();
	rList.freeByKey(goodsID, 1);
	return ok;
}
//
//
//
class PPKeywordListGenerator : SStrGroup {
public:
	struct RunStatEntry { // @flat
		uint   WordP;
		uint   Count;     // Количество элементов, встретившихся в тексте
		uint   WordCount; // Количество слов в элементе
	};
	struct RunStat : public SStrGroup, public TSVector <PPKeywordListGenerator::RunStatEntry> { // @v9.8.4 TSArray-->TSVector
		RunStat();
		int    GetText(uint p, SString & rText) const;
		int    AddItem(const SString & rItem);

		uint   WordCount;
		uint   ItemCount;
		uint   LastRunWordCount;
		uint   LastRunItemCount;
		uint   RunCount;
	};

	SLAPI  PPKeywordListGenerator();
	SLAPI ~PPKeywordListGenerator();
	void   SLAPI Clear();
	const  SString & SLAPI GetDataFileName() const;
	int    SLAPI GetWord(uint wordP, SString & rBuf) const;
	int    SLAPI Run(const char * pContext, SString & rResult, RunStat * pStat);
private:
	int    SLAPI ReadData(const char * pFileName);
	int    SLAPI SearchGroup(const char * pContext, uint * pPos) const;
	int    SLAPI CreateGroup(const char * pContext, uint * pPos);
	int    SLAPI GetRandomWord(SString & rBuf);
	int    SLAPI GenerateByGroup(uint grpPos, const SString * pGrpText, const SString * pLoc, StringSet & rSs, LongArray & rSsPosList, RunStat * pStat);

	struct Item_ { // @flat
		uint   TextP;
		double Prob;
	};
	class Group : public TSVector <PPKeywordListGenerator::Item_> { // @v9.8.4 TSArray-->TSVector
	public:
		Group() : ContextP(0), ItemsLimit(0), WordsLimit(0)
		{
		}
		uint   ContextP;
        uint   ItemsLimit;
        uint   WordsLimit;
	};
	ulong  _Divider;
	TSCollection <PPKeywordListGenerator::Group> List;

	class RandWordBlock : public SStrGroup {
	public:
		int    SLAPI ReadData(const char * pFileName);
		void   SLAPI Clear();
        uint   SLAPI GetCount() const;
        int    FASTCALL GetWord(uint idx, SString & rBuf) const;

		LongArray Idx;
	};
	RandWordBlock Rwb;
	SString DataFileName;
	LDATETIME DataFileDtm;
	SCycleTimer DataFileChangeDetectTimer;
};

PPKeywordListGenerator::RunStat::RunStat() : WordCount(0), ItemCount(0), LastRunWordCount(0), LastRunItemCount(0), RunCount(0)
{
}

int PPKeywordListGenerator::RunStat::GetText(uint p, SString & rText) const
{
	return GetS(p, rText);
}

int PPKeywordListGenerator::RunStat::AddItem(const SString & rItem)
{
	uint   idx = 0;
	StringSet ss;
	rItem.Tokenize(" \t\n\r.,;", ss);
	SString temp_buf;
	uint   p = 0;
	uint   word_count = 0;
	for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
		word_count++;
		p = 0;
		PPKeywordListGenerator::RunStatEntry entry;
		temp_buf.Strip();
		if(Pool.search(temp_buf, &p, 0)) {
			if(lsearch(&p, &idx, CMPF_LONG)) {
				assert(at(idx).WordCount == 1);
				at(idx).Count++;
			}
			else {
				entry.WordP = p;
				entry.Count = 1;
				entry.WordCount = 1;
				insert(&entry);
			}
		}
		else {
			AddS(temp_buf, &entry.WordP);
			entry.Count = 1;
			entry.WordCount = 1;
			insert(&entry);
		}
		WordCount++;
	}
	if(word_count > 1) {
		PPKeywordListGenerator::RunStatEntry entry;
		(temp_buf = rItem).Strip();
		if(Pool.search(temp_buf, &p, 0)) {
			if(lsearch(&p, &idx, CMPF_LONG)) {
				assert(at(idx).WordCount == word_count);
				at(idx).Count++;
			}
			else {
				entry.WordP = p;
				entry.Count = 1;
				entry.WordCount = word_count;
				insert(&entry);
			}
		}
		else {
			AddS(temp_buf, &entry.WordP);
			entry.Count = 1;
			entry.WordCount = word_count;
			insert(&entry);
		}
	}
	WordCount += word_count;
	ItemCount++;
	LastRunWordCount += word_count;
	LastRunItemCount++;
	return 1;
}

SLAPI PPKeywordListGenerator::PPKeywordListGenerator() : SStrGroup(), DataFileChangeDetectTimer(60000), DataFileDtm(ZERODATETIME)
{
	_Divider = 9999;
	do {
		if(IsPrime(_Divider)) {
			break;
		}
	} while(--_Divider);
	SString file_name;
	PPIniFile ini_file;
	ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_KEYWORDDATAFILE, file_name);
	if(file_name.NotEmptyS() && fileExists(file_name)) {
		ReadData(file_name);
	}
}

SLAPI PPKeywordListGenerator::~PPKeywordListGenerator()
{
}

void SLAPI PPKeywordListGenerator::Clear()
{
	SStrGroup::ClearS();
	List.freeAll();
}

const SString & SLAPI PPKeywordListGenerator::GetDataFileName() const
{
	return DataFileName;
}

int SLAPI PPKeywordListGenerator::GetWord(uint wordP, SString & rBuf) const
{
	return GetS(wordP, rBuf);
}

int SLAPI PPKeywordListGenerator::RandWordBlock::ReadData(const char * pFileName)
{
	int    ok = 1;
	SFile in_f(pFileName, SFile::mRead);
    SString line_buf;
    if(in_f.IsValid()) {
		Clear();
		while(in_f.ReadLine(line_buf)) {
			line_buf.Chomp();
			uint pos = 0;
			AddS(line_buf, &pos);
			Idx.add((long)pos);
		}
		Idx.shuffle();
    }
    else
		ok = 0;
    return ok;
}

void SLAPI PPKeywordListGenerator::RandWordBlock::Clear()
{
	SStrGroup::ClearS();
	Idx.freeAll();
}

uint SLAPI PPKeywordListGenerator::RandWordBlock::GetCount() const
{
	return Idx.getCount();
}

int FASTCALL PPKeywordListGenerator::RandWordBlock::GetWord(uint idx, SString & rBuf) const
{
	rBuf.Z();
	if(idx < Idx.getCount()) {
		const uint pos = (uint)Idx.get(idx);
		GetS(pos, rBuf);
	}
	return BIN(rBuf.NotEmpty());
}

int SLAPI PPKeywordListGenerator::GetRandomWord(SString & rBuf)
{
	rBuf.Z();
	if(!Rwb.GetCount()) {
		if(DataFileName.NotEmpty()) {
			SString rw_file_name;
			SPathStruc ps(DataFileName);
            ps.Nam.CatChar('-').Cat("random");
            ps.Merge(rw_file_name);
			if(fileExists(rw_file_name)) {
				Rwb.ReadData(rw_file_name);
			}
		}
	}
	{
		const uint _c = Rwb.GetCount();
		if(_c) {
			do {
				const uint rn = SLS.GetTLA().Rg.GetUniformInt(_c);
				if(rn < _c) {
					Rwb.GetWord(rn, rBuf);
				}
			} while(rBuf.Empty());
		}
	}
	return rBuf.NotEmpty();
}

int SLAPI PPKeywordListGenerator::GenerateByGroup(uint grpPos, const SString * pGrpText, const SString * pLoc, StringSet & rSs, LongArray & rSsPosList, RunStat * pStat)
{
	static const double ztolerance = 1.0E-6;
	int    ok = 1;
	if(grpPos < List.getCount()) {
		SString temp_buf;
		uint   ss_pos = 0;
		const Group * p_group = List.at(grpPos);
		for(uint i = 0; i < p_group->getCount(); i++) {
			const Item_ & r_item = p_group->at(i);
			GetS(r_item.TextP, temp_buf);
			enum {
				spcSkip = -1,
				spcNone = 0,
				spcRandom,
				spcGroup,
				spcLoc
			};
			int    special = spcNone;
			if(temp_buf == "\\random")
				special = spcRandom;
			else if(temp_buf == "\\group") {
				special = spcGroup;
				if(!pGrpText || pGrpText->Empty())
					special = spcSkip;
			}
			else if(temp_buf == "\\loc") {
				special = spcLoc;
				if(!pLoc || pLoc->Empty())
					special = spcSkip;
			}
			if(special != spcSkip && temp_buf.NotEmpty()) {
				const  uint  pi_ = (uint)fint(r_item.Prob);
				const  double pf_ = ffrac(r_item.Prob);
				for(uint j = 0; j < pi_; j++) {
					if(special == spcRandom) {
						GetRandomWord(temp_buf);
					}
					else if(special == spcGroup) {
						assert(pGrpText); // Выше мы уже убедились, что pGrpText != 0
						temp_buf = pGrpText ? *pGrpText : (const char *)0;
					}
					else if(special == spcLoc) {
						assert(pLoc); // Выше мы уже убедились, что pLoc != 0
						temp_buf = pLoc ? *pLoc : (const char *)0;
					}
					if(temp_buf.NotEmpty()) {
						rSs.add(temp_buf, &(ss_pos = 0));
						rSsPosList.add(ss_pos);
						if(pStat)
							pStat->AddItem(temp_buf);
					}
				}
				if(pf_ > ztolerance) {
					const ulong rn = SLS.GetTLA().Rg.GetUniformInt(0x7ffffff);
					const double m = (double)(rn % _Divider);
					if((m / (double)_Divider) <= pf_) {
						if(special == spcRandom) {
							GetRandomWord(temp_buf);
						}
						else if(special == spcGroup) {
							assert(pGrpText); // Выше мы уже убедились, что pGrpText != 0
							temp_buf = pGrpText ? *pGrpText : (const char *)0;
						}
						else if(special == spcLoc) {
							assert(pLoc); // Выше мы уже убедились, что pLoc != 0
							temp_buf = pLoc ? *pLoc : (const char *)0;
						}
						if(temp_buf.NotEmpty()) {
							rSs.add(temp_buf, &(ss_pos = 0));
							rSsPosList.add(ss_pos);
							if(pStat)
								pStat->AddItem(temp_buf);
						}
					}
				}
			}
		}
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPKeywordListGenerator::Run(const char * pContext, SString & rResult, RunStat * pStat)
{
	rResult.Z();
	if(pStat)
		pStat->LastRunWordCount = 0;
	int    ok = 0;
	uint   grp_pos = 0;
	SString context, temp_buf, random_word;
	StringSet ss;
	uint   ss_pos;
	(context = pContext).Strip();
	LongArray ss_pos_list;
	{
		context.Strip();
		if(context[0] == '@') {
			//
			// @kwid=id;locn=loccode
			// @kws=symbol;locn=locname
			// locs=locsymb
			// @goods=id
			//
			int    ctx_typ = 0; // 1 - keyword, 2 - goods
			PPWorkbookPacket wb_pack;
			StringSet goods_text_list;
			StringSet goods_code_list;
			SString loctext;

			StringSet pss(';', context+1);
			SString temp_buf, left, right;
			for(uint p = 0; pss.get(&p, temp_buf);) {
				if(temp_buf.Divide('=', left, right) > 0) {
					if(left.CmpNC("kwid") == 0) {
						if(!wb_pack.Rec.ID) {
							SETIFZ(DS.GetTLA().P_WbObj, new PPObjWorkbook);
							PPObjWorkbook * p_wb_obj = DS.GetTLA().P_WbObj;
							if(p_wb_obj && p_wb_obj->GetPacket(right.ToLong(), &wb_pack) > 0) {
								ctx_typ = 1;
							}
						}
					}
					else if(left.CmpNC("kws") == 0) {
						if(!wb_pack.Rec.ID) {
							SETIFZ(DS.GetTLA().P_WbObj, new PPObjWorkbook);
							PPObjWorkbook * p_wb_obj = DS.GetTLA().P_WbObj;
							PPID   wb_id = 0;
							if(p_wb_obj && p_wb_obj->SearchBySymb(right.Strip(), &wb_id) > 0) {
								if(p_wb_obj->GetPacket(wb_id, &wb_pack) > 0) {
									ctx_typ = 1;
								}
							}
						}
					}
					else if(left.CmpNC("locn") == 0) {
						if(loctext.Empty() && right.NotEmptyS())
							loctext = right.Strip();
					}
					else if(left.CmpNC("locs") == 0) {
						if(loctext.Empty() && right.NotEmptyS()) {
							SETIFZ(DS.GetTLA().P_WObj, new PPObjWorld);
							PPObjWorld * p_w_obj = DS.GetTLA().P_WObj;
							WorldTbl::Rec w_rec;
							if(p_w_obj && p_w_obj->SearchByCode(right, &w_rec) > 0)
								(loctext = w_rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
						}
					}
					else if(left.CmpNC("goods") == 0) {
						const PPID goods_id = right.ToLong();
						PPObjGoods goods_obj;
						Goods2Tbl::Rec goods_rec;
						if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
							(temp_buf = goods_rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
							goods_text_list.add(temp_buf);

							BarcodeArray bc_list;
							goods_obj.ReadBarcodes(goods_id, bc_list);
							for(uint i = 0; i < bc_list.getCount(); i++) {
								temp_buf = bc_list.at(i).Code;
								if(temp_buf.NotEmptyS()) {
									goods_code_list.add(temp_buf);
									if(temp_buf.C(0) == 0 && temp_buf.Len() >= 8) {
										goods_code_list.add(temp_buf.ShiftLeft());
									}
								}
							}
							ctx_typ = 2;
						}
					}
				}
			}
			if(ctx_typ == 1) {
				const uint  _c = (wb_pack.Rec.KeywordCount > 0) ? (uint)wb_pack.Rec.KeywordCount : 1;
				loctext.Strip();
				(temp_buf = wb_pack.Rec.Name).ToLower().Transf(CTRANSF_INNER_TO_OUTER);
				if(DataFileChangeDetectTimer.Check(0)) {
					ReadData(DataFileName);
				}
				if(SearchGroup(temp_buf, &grp_pos) > 0) {
					for(uint i = 0; i < _c; i++) {
						GenerateByGroup(grp_pos, &temp_buf, &loctext, ss, ss_pos_list, pStat);
					}
				}
				else {
					const ObjTagItem * p_tag = wb_pack.TagL.GetItem(PPTAG_WORKBOOK_KWSYN);
					if(p_tag) {
						p_tag->GetStr(left);
						if(left.NotEmptyS()) {
							left.Transf(CTRANSF_INNER_TO_OUTER);
							StringSet synss(',', left);
							for(uint i = 0; i < _c; i++) {
								{
									//
									// Текст ключевого слова
									//
									(temp_buf = wb_pack.Rec.Name).Strip().Transf(CTRANSF_INNER_TO_OUTER);
									ss.add(temp_buf, &(ss_pos = 0));
									ss_pos_list.add(ss_pos);
								}
								//
								// Наименование локали (города)
								//
								if(loctext.NotEmpty()) {
									ss.add(loctext, &(ss_pos = 0));
									ss_pos_list.add(ss_pos);
								}
								//
								// Все синонимы один-за-другим
								//
								for(uint sp = 0; synss.get(&sp, temp_buf);) {
									ss.add(temp_buf, &(ss_pos = 0));
									ss_pos_list.add(ss_pos);
								}
								/*
								//
								// Снова наименование локали (города)
								//
								if(loctext.NotEmpty()) {
									ss.add(loctext, &(ss_pos = 0));
									ss_pos_list.add(ss_pos);
								}
								*/
							}
						}
					}
				}
			}
			else if(ctx_typ == 2) {
				const uint code_mult = 3;
				StringSet tok_list;
				uint   i;
				LongArray pos_list;
				for(i = 0; goods_text_list.get(&i, temp_buf);) {
					//Tokenize(const char * pDelimChrSet, StringSet & rResult) const;
					temp_buf.Tokenize(" \t\n\r,.;-=+:()[]{}", tok_list);
				}
				for(uint j = 0; j < code_mult; j++) {
					for(i = 0; goods_code_list.get(&i, temp_buf);) {
						tok_list.add(temp_buf);
					}
				}
				{
            		uint prev_pos = 0;
					for(i = 0; tok_list.get(&i, temp_buf);) {
						pos_list.add((long)prev_pos);
						prev_pos = i;
					}
					pos_list.shuffle();
					for(i = 0; i < pos_list.getCount(); i++) {
						if(tok_list.get(pos_list.get(i), temp_buf)) {
							if(rResult.NotEmpty())
								rResult.Space();
							rResult.Cat(temp_buf);
						}
					}
				}
			}
		}
		else {
			if(DataFileChangeDetectTimer.Check(0)) {
				ReadData(DataFileName);
			}
			if(SearchGroup(context, &grp_pos) > 0) {
				GenerateByGroup(grp_pos, &context, 0, ss, ss_pos_list, pStat);
			}
		}
		if(ss_pos_list.getCount()) {
			ss_pos_list.shuffle();
			for(uint i = 0; i < ss_pos_list.getCount(); i++) {
				ss.get(ss_pos_list.get(i), temp_buf);
				if(i)
					rResult.Space();
				rResult.Cat(temp_buf);
			}
			if(pStat)
				pStat->RunCount++;
		}
		ok = rResult.NotEmpty() ? 1 : -1;
	}
	return ok;
}

int SLAPI PPKeywordListGenerator::SearchGroup(const char * pContext, uint * pPos) const
{
	int    ok = 0;
	uint   pos = 0;
	if(!isempty(pContext)) {
		SString temp_buf;
		StringSet ss(',', 0);
		for(uint i = 0; !ok && i < List.getCount(); i++) {
			const Group * p_group = List.at(i);
			if(p_group) {
				GetS(p_group->ContextP, temp_buf);
				if(temp_buf == pContext) {
					pos = i;
					ok = 1;
				}
				else if(temp_buf.HasChr(',')) {
					ss.setBuf(temp_buf);
					for(uint ssp = 0; !ok && ss.get(&ssp, temp_buf);) {
						if(temp_buf == pContext) {
							pos = i;
							ok = 1;
						}
					}
				}
			}
		}
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int SLAPI PPKeywordListGenerator::CreateGroup(const char * pContext, uint * pPos)
{
	int    ok = 0;
	uint   pos = 0;
	if(!isempty(pContext)) {
		if(SearchGroup(pContext, &pos)) {
			ok = 2;
		}
		else {
			Group * p_new_group = List.CreateNewItem(&pos);
			AddS(pContext, &p_new_group->ContextP);
			ok = 1;
		}
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int SLAPI PPKeywordListGenerator::ReadData(const char * pFileName)
{
	int    ok = -1;
	uint   cur_grp_pos = 0;
	LDATETIME new_data_file_dtm;
	SFile  f_in(pFileName, SFile::mRead);
	THROW_SL(f_in.IsValid());
	SFile::GetTime(pFileName, 0, 0, &new_data_file_dtm);
	if(DataFileName.Empty() || !DataFileDtm || DataFileDtm != new_data_file_dtm) {
		SString line_buf, temp_buf, prob_buf;
		SString cur_context;
		Clear();
		DataFileDtm = new_data_file_dtm;
		DataFileName = pFileName;
		while(f_in.ReadLine(line_buf)) {
			if(line_buf.Chomp().Strip().NotEmptyS()) {
				if(line_buf[0] == '[') {
					uint   rb = 1;
					temp_buf.Z();
					while(line_buf[rb] != 0 && line_buf[rb] != ']') {
						temp_buf.CatChar(line_buf[rb++]);
					}
					if(line_buf[rb] == ']') {
						(cur_context = temp_buf).Strip().ToOem().ToLower().Transf(CTRANSF_INNER_TO_OUTER);
						THROW(CreateGroup(cur_context, &cur_grp_pos));
					}
					else {
						cur_context = 0;
						cur_grp_pos = 0;
					}
				}
				else if(cur_context.NotEmpty()) {
					Item_ new_item;
					new_item.TextP = 0;
					new_item.Prob = 1.0;
					if(line_buf.Divide(':', temp_buf, prob_buf) > 0) {
						new_item.Prob = prob_buf.ToReal();
						if(new_item.Prob <= 0.0)
							new_item.Prob = 1.0;
					}
					else
						temp_buf = line_buf;
					if(temp_buf.NotEmptyS()) {
						Group * p_group = List.at(cur_grp_pos);
						if(temp_buf.CmpNC("#wordlimit") == 0) {
							p_group->WordsLimit = (uint)new_item.Prob;
						}
						else if(temp_buf.CmpNC("#itemlimit") == 0) {
							p_group->ItemsLimit = (uint)new_item.Prob;
						}
						else {
							temp_buf.ToOem().ToLower().Transf(CTRANSF_INNER_TO_OUTER);
							AddS(temp_buf, &new_item.TextP);
							THROW_SL(p_group->insert(&new_item));
						}
					}
				}
			}
		}
		ok = 1;
	}
	DataFileChangeDetectTimer.Restart(60000);
	CATCHZOK
	return ok;
}

static PPKeywordListGenerator * _GetGlobalKeywordListGeneratorInstance()
{
	static const char * P_KeywordListGeneratorGlobalSymbol = "PPKEYWORDSEQGENERATOR";
	PPKeywordListGenerator * p_gen = 0;
	long   symbol_id = SLS.GetGlobalSymbol(P_KeywordListGeneratorGlobalSymbol, -1, 0);
	THROW_SL(symbol_id);
	if(symbol_id < 0) {
		TSClassWrapper <PPKeywordListGenerator> cls;
		THROW_SL(symbol_id = SLS.CreateGlobalObject(cls));
		THROW_SL(p_gen = (PPKeywordListGenerator *)SLS.GetGlobalObject(symbol_id));
		{
			long s = SLS.GetGlobalSymbol(P_KeywordListGeneratorGlobalSymbol, symbol_id, 0);
			assert(symbol_id == s);
		}
	}
	else if(symbol_id > 0) {
		THROW_SL(p_gen = (PPKeywordListGenerator *)SLS.GetGlobalObject(symbol_id));
	}
	CATCH
		p_gen = 0;
	ENDCATCH
	return p_gen;
}

static int GetKeywordListGeneratorInputFileName(SString & rResult)
{
	rResult.Z();
	int    ok = 1;
	ENTER_CRITICAL_SECTION
		PPKeywordListGenerator * p_gen = _GetGlobalKeywordListGeneratorInstance();
		if(p_gen)
			rResult = p_gen->GetDataFileName();
		else
			ok = 0;
	LEAVE_CRITICAL_SECTION
	return ok;
}

int PPGenerateKeywordSequence(const char * pContext, SString & rResult, void * pStat)
{
	rResult.Z();
	int    ok = 1;
	if(!isempty(pContext)) {
		SString context;
		context = pContext;
		ENTER_CRITICAL_SECTION
			PPKeywordListGenerator * p_gen = _GetGlobalKeywordListGeneratorInstance();
			ok = p_gen ? p_gen->Run(context, rResult, (PPKeywordListGenerator::RunStat *)pStat) : 0;
		LEAVE_CRITICAL_SECTION
	}
	return ok;
}

class RandomGenTest {
public:
	RandomGenTest(double step, double limit)
	{
		_Divider = 9999;
		do {
			if(IsPrime(_Divider)) {
				break;
			}
		} while(--_Divider);
		for(double v = 0.0; v <= limit; v += step) {
			SerItem * p_item = new SerItem(v);
			Series.insert(p_item);
		}
	}
	int Run(uint count)
	{
		int    ok = 1;
		uint   i;
		for(i = 0; i < Series.getCount(); i++) {
			SerItem * p_item = Series.at(i);
			p_item->S.Init(StatBase::fStoreVals);
		}
		for(uint j = 0; j < count; j++) {
			for(i = 0; i < Series.getCount(); i++) {
				SerItem * p_item = Series.at(i);
				const  uint  pi_ = (uint)fint(p_item->P);
				const  double pf_ = ffrac(p_item->P);
				{
					const ulong rn = SLS.GetTLA().Rg.GetUniformInt(0x7ffffff);
					const double m = (double)(rn % _Divider);
					double v = ((m / (double)_Divider) <= pf_) ? 1.0 : 0.0;
					p_item->S.Step(v);
				}
			}
		}
		for(i = 0; i < Series.getCount(); i++) {
			SerItem * p_item = Series.at(i);
			p_item->S.Finish();
		}
		return ok;
	}
	int  Output(SFile & rF)
	{
		int    ok = 1;
		SString line_buf;
		for(uint i = 0; i < Series.getCount(); i++) {
			const SerItem * p_item = Series.at(i);
			line_buf.Z().Cat(p_item->P, MKSFMTD(0, 5, 0)).CatDiv(';', 2).Cat(p_item->S.GetExp(), MKSFMTD(0, 6, 0)).CatDiv(':', 2);
			for(long j = 0; j < p_item->S.GetCount(); j++) {
				double _p;
				if(p_item->S.GetValue(j, &_p) > 0) {
					line_buf.CatChar((((long)_p) == 0) ? ' ' : '*');
				}
			}
			rF.WriteLine(line_buf.CR());
		}
		return ok;
	}
private:
	struct SerItem {
		SerItem(double p) : S(StatBase::fStoreVals), P(p)
		{
		}
		double P;
		StatBase S;
	};
	ulong _Divider;
	TSCollection <SerItem> Series;
};

int SLAPI Test_KeywordListGenerator()
{
	PPKeywordListGenerator::RunStat stat;
	SString temp_buf, word_buf, context_buf;
	SPathStruc ps;
	if(GetKeywordListGeneratorInputFileName(temp_buf)) {
		SPathStruc::ReplaceExt(temp_buf, "log", 1);
		SFile f_out(temp_buf, SFile::mWrite);
		{
			RandomGenTest rgt(0.01, 1.0);
			rgt.Run(1000);
			rgt.Output(f_out);
		}
		{
			context_buf = "org_addr";
			for(uint i = 0; i < 1000; i++) {
				if(PPGenerateKeywordSequence(context_buf, word_buf, &stat) > 0) {
					temp_buf.Z().Cat(stat.LastRunWordCount).Tab().Cat(word_buf).CR();
					f_out.WriteLine(temp_buf);
				}
				else
					break;
			}
		}
		/*
		{
			f_out.WriteLine((temp_buf = "===STATISTICS===").CR());
			temp_buf.Z().CatEq("RunCount", (long)stat.RunCount).Space().CatEq("WordCount", (long)stat.WordCount).CR();
			f_out.WriteLine(temp_buf);
			for(uint i = 0; i < stat.getCount(); i++) {
				const PPKeywordListGenerator::RunStatEntry & r_entry = stat.at(i);
				temp_buf.Z();
				g.GetWord(r_entry.WordP, word_buf.Z());
				temp_buf.Cat(word_buf).Tab().Cat(r_entry.Count).Tab().Cat((double)r_entry.Count / (double)stat.RunCount, MKSFMTD(0, 6, 0)).
					Tab().Cat((double)r_entry.Count / (double)stat.WordCount, MKSFMTD(0, 6, 0)).CR();
				f_out.WriteLine(temp_buf);
			}
		}
		*/
		{
			PPObjWorkbook wb_obj;
			WorkbookTbl::Rec wb_rec;
			for(SEnum en = wb_obj.P_Tbl->EnumByType(PPWBTYP_KEYWORD, 0); en.Next(&wb_rec) > 0;) {
				{
					context_buf.Z().CatChar('@').CatEq("kwid", wb_rec.ID);
					if(PPGenerateKeywordSequence(context_buf, word_buf, 0) > 0) {
						temp_buf.Z().Cat(context_buf).Tab().Cat(word_buf);
					}
					else {
						(temp_buf = "Error on context").CatDiv(':', 2).Cat(context_buf);
					}
					f_out.WriteLine(temp_buf.CR());
				}
				{
					context_buf.Z().CatChar('@').CatEq("kws", wb_rec.Symb);
					if(PPGenerateKeywordSequence(context_buf, word_buf, 0) > 0) {
						temp_buf.Z().Cat(context_buf).Tab().Cat(word_buf);
					}
					else {
						(temp_buf = "Error on context").CatDiv(':', 2).Cat(context_buf);
					}
					f_out.WriteLine(temp_buf.CR());
				}
				{
					context_buf.Z().CatChar('@').CatEq("kwid", wb_rec.ID).Semicol().CatEq("locs", "6300000100000");
					if(PPGenerateKeywordSequence(context_buf, word_buf, 0) > 0) {
						temp_buf.Z().Cat(context_buf).Tab().Cat(word_buf);
					}
					else {
						(temp_buf = "Error on context").CatDiv(':', 2).Cat(context_buf);
					}
					f_out.WriteLine(temp_buf.CR());
				}
				{
					context_buf.Z().CatChar('@').CatEq("kws", wb_rec.Symb).Semicol().CatEq("locn", "чебоксары");
					if(PPGenerateKeywordSequence(context_buf, word_buf, 0) > 0) {
						temp_buf.Z().Cat(context_buf).Tab().Cat(word_buf);
					}
					else {
						(temp_buf = "Error on context").CatDiv(':', 2).Cat(context_buf);
					}
					f_out.WriteLine(temp_buf.CR());
				}
			}
		}
	}
	return 1;
}
//
//
//
SLAPI PPAutoTranslSvc_Microsoft::PPAutoTranslSvc_Microsoft() : LastStatusCode(0), P_XpCtx(0), ExpirySec(0)
{
	AuthTime.SetZero();
	MEMSZERO(S);
}

SLAPI PPAutoTranslSvc_Microsoft::~PPAutoTranslSvc_Microsoft()
{
	xmlFreeParserCtxt(P_XpCtx);
	if(S.ReqCount) {
		SString log_msg_buf;
		log_msg_buf.CatEq("ReqCount", (uint32)S.ReqCount).Space().CatEq("InpChrCount", (uint32)S.InpChrCount).Space().
			CatEq("OutpChrCount", (uint32)S.OutpChrCount).Space().CatEq("TotalTiming", (int64)S.TotalTiming);
		PPLogMessage(PPFILNAM_AUTOTRANSL_LOG, log_msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
	}
}

int SLAPI PPAutoTranslSvc_Microsoft::Auth(const char * pIdent, const char * pSecret)
{
	Token.Z();
	ExpirySec = 0;
	AuthTime.SetZero();
	AuthName.Z();
	AuthSecret.Z();
	LastStatusCode = 0;
	LastStatusMessage.Z();

	int    ok = 1;
	SString temp_buf;
	// @v9.6.9 SString url = "https://datamarket.accesscontrol.windows.net/v2/OAuth2-13";
	SString url = "https://api.cognitive.microsoft.com/sts/v1.0/issueToken"; // @v9.6.9
	StrStrAssocArray post_fields;
	ScURL  curl;
	json_t * p_json_doc = 0;
	SString result_str;
	{
		SBuffer result_buf;
		SFile wr_stream(result_buf, SFile::mWrite);
		/* @v9.6.9
		post_fields.Add("grant_type", "client_credentials");
		post_fields.Add("client_id", pIdent);
		post_fields.Add("client_secret", pSecret);
		post_fields.Add("scope", "http://api.microsofttranslator.com");
		*/
		//post_fields.Add(/*"Ocp-Apim-Subscription-Key"*/"Subscription-Key", pSecret);
		(temp_buf = url).CatChar('?').CatEq("Subscription-Key", pSecret);
		THROW_SL(curl.HttpPost(/*url*/temp_buf, ScURL::mfDontVerifySslPeer, &post_fields, &wr_stream));
		{
			SBuffer * p_result_buf = (SBuffer *)wr_stream;
			size_t avl_size = p_result_buf ? p_result_buf->GetAvailableSize() : 0;
			THROW(avl_size);
			result_str.CopyFromN((const char *)p_result_buf->GetBuf(p_result_buf->GetRdOffs()), avl_size);
		}
	}
	{
		if(result_str.C(0) == '{') {
			json_t * p_next = 0;
			THROW(json_parse_document(&p_json_doc, result_str.cptr()) == JSON_OK);
			for(json_t * p_cur = p_json_doc; p_cur; p_cur = p_next) {
				p_next = p_cur->P_Next;
				switch(p_cur->Type) {
					case json_t::tARRAY:
						break;
					case json_t::tOBJECT:
						p_next = p_cur->P_Child;
						break;
					case json_t::tSTRING:
						if(p_cur->P_Child) {
							if(sstreqi_ascii(p_cur->Text, "statusCode")) {
								LastStatusCode = (temp_buf = p_cur->P_Child->Text).Unescape().ToLong();
							}
							else if(sstreqi_ascii(p_cur->Text, "message")) {
								LastStatusMessage = (temp_buf = p_cur->P_Child->Text).Unescape();
							}
						}
						break;
					default:
						break;
				}
			}
			ok = 0;
		}
		else {
			Token = result_str;
			AuthTime = getcurdatetime_();
			ExpirySec = 0;
		}
	}
	/* @v9.6.9
	{
		json_t * p_next = 0;
		THROW(json_parse_document(&p_json_doc, result_str.cptr()) == JSON_OK);
		for(json_t * p_cur = p_json_doc; p_cur; p_cur = p_next) {
			p_next = p_cur->next;
			switch(p_cur->type) {
				case JSON_ARRAY:
					break;
				case JSON_OBJECT:
					p_next = p_cur->child;
					break;
				case JSON_STRING:
					if(p_cur->text && p_cur->child) {
						if(sstreqi_ascii(p_cur->text, "access_token")) {
							(Token = p_cur->child->text).Unescape();
							AuthTime = getcurdatetime_();
							AuthName = pIdent;
							AuthSecret = pSecret;
						}
						else if(sstreqi_ascii(p_cur->text, "expires_in")) {
							ExpirySec = (temp_buf = p_cur->child->text).Unescape().ToLong();
						}
					}
					break;
				default:
					break;
			}
		}
	}
	*/
	CATCHZOK
	json_free_value(&p_json_doc);
	return ok;
}

int SLAPI PPAutoTranslSvc_Microsoft::Request(int srcLang, int destLang, const SString & rSrcText, SString & rResult)
{
	rResult.Z();
	int    ok = -1;
	xmlDoc * p_doc = 0;
	xmlNode * p_root = 0;
	SString temp_buf;
	SString result_str;
	StrStrAssocArray http_header;
	SString url = "http://api.microsofttranslator.com/v2/Http.svc/Translate?";
	SString log_buf;
	if(Token.NotEmpty() && !!AuthTime && ExpirySec > 0) {
		//
		// Проверка истечения срока действия токена.
		// Если время истекает, то повторяем авторизацию.
		//
		const LDATETIME ct = getcurdatetime_();
        const long sec = diffdatetimesec(ct, AuthTime);
        if(sec > (long)((double)ExpirySec * 0.9)) {
			//
			// Функция Auth затрет AuthName и AuthSecret на входе.
			// Потому необходимо скопировать эти значения во временные буферы _nam и _secr.
			//
			const SString _nam = AuthName;
			const SString _secr = AuthSecret;
			THROW(Auth(_nam, _secr));
        }
	}
	THROW(GetLinguaCode(srcLang, temp_buf));
	url.CatEq("from", temp_buf);
	log_buf.Cat(temp_buf).Tab();
	THROW(GetLinguaCode(destLang, temp_buf));
	url.CatChar('&').CatEq("to", temp_buf);
	log_buf.Cat(temp_buf).Tab();
	url.CatChar('&').CatEq("text", (temp_buf = rSrcText).ToUrl());
	log_buf.Cat((temp_buf = rSrcText).Transf(CTRANSF_UTF8_TO_INNER)).Tab();
	// $authHeader = "Authorization: Bearer ". $accessToken;
	http_header.Add("Authorization", temp_buf.Z().Cat("Bearer").Space().Cat(Token));
	http_header.Add("Content-Type", "text/xml");
	{
		const uint64 at_start = SLS.GetProfileTime();
		ScURL  curl;
		SBuffer result_buf;
		SFile wr_stream(result_buf, SFile::mWrite);
		THROW(curl.HttpGet(url, ScURL::mfDontVerifySslPeer, &http_header, &wr_stream));
		{
			const uint64 at_end = SLS.GetProfileTime();
			SBuffer * p_result_buf = (SBuffer *)wr_stream;
			const size_t avl_size = p_result_buf ? p_result_buf->GetAvailableSize() : 0;
			THROW(avl_size);
			result_str.CopyFromN((const char *)p_result_buf->GetBuf(p_result_buf->GetRdOffs()), avl_size);
			{
				THROW(SETIFZ(P_XpCtx, xmlNewParserCtxt()));
				THROW((p_doc = xmlCtxtReadMemory(P_XpCtx, (const char *)p_result_buf->GetBuf(), (int)avl_size, 0, 0, XML_PARSE_NOENT)));
				THROW(p_root = xmlDocGetRootElement(p_doc));
				if(SXml::GetContentByName(p_root, "string", temp_buf)) {
					rResult = temp_buf;
					{
						SStringU temp_buf_u;
						S.ReqCount++;
						S.TotalTiming += (at_end - at_start);
						temp_buf_u.CopyFromUtf8(rSrcText);
						S.InpChrCount += temp_buf_u.Len();
						temp_buf_u.CopyFromUtf8(rResult);
						S.OutpChrCount += temp_buf_u.Len();
					}
					{
						log_buf.Cat(rResult).Tab();
						log_buf.Cat(at_end - at_start);
						PPLogMessage(PPFILNAM_AUTOTRANSL_LOG, log_buf, 0);
					}
					ok = 1;
				}
			}
			if(ok <= 0) {
				log_buf.Cat(result_str);
				PPLogMessage(PPFILNAM_AUTOTRANSL_LOG, log_buf, 0);
			}
		}
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
	return ok;
}

int Helper_PPAutoTranslSvc_Microsoft_Auth(PPAutoTranslSvc_Microsoft & rAt)
{
	int   ok = 1;
	SString key_buf;
	PPVersionInfo vi = DS.GetVersionInfo();
	vi.GetMsftTranslAcc(key_buf);
	THROW_PP(key_buf.NotEmptyS(), PPERR_MSFTTRANSLKEY_INV);
	{
		SString ident, secret;
		THROW_PP(key_buf.Divide(':', ident, secret) > 0, PPERR_MSFTTRANSLKEY_INV);
		THROW(rAt.Auth(ident, secret));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPAutoTranslateText(int srcLang, int destLang, const SString & rSrcUtf8, SString & rResultUtf8)
{
	int    ok = 1;
	PPAutoTranslSvc_Microsoft at;
	THROW(Helper_PPAutoTranslSvc_Microsoft_Auth(at));
	THROW(at.Request(srcLang, destLang, rSrcUtf8, rResultUtf8));
	CATCHZOK
	return ok;
}
//
//
//
static int FASTCALL Helper_CollectLldFileStat(const char * pPath, SFile * pOutFile, SFile * pDetectTypeOutFile, SFile * pFileTypeToFreqOrderOutFile)
{
	int    ok = 1;
	SString wildcard;
	SString temp_buf, dest_path;
	SString item_buf;
	SString file_type_symb;
	SString src_path = pPath;
	SLldAlphabetAnalyzer aa;
	RAssocArray freq_list;
	SPathStruc ps;
	SPathStruc::NormalizePath(pPath, 0, src_path);
	(wildcard = src_path).SetLastSlash().CatChar('*').Dot().CatChar('*');
	SDirEntry de;
	for(SDirec sd(wildcard); sd.Next(&de) > 0;) {
		if(de.IsFolder()) {
			if(!de.IsSelf() && !de.IsUpFolder()) {
				(temp_buf = src_path).SetLastSlash().Cat(de.FileName);
				THROW(Helper_CollectLldFileStat(temp_buf, pOutFile, pDetectTypeOutFile, pFileTypeToFreqOrderOutFile)); // @recursion
			}
		}
		else {
			(temp_buf = src_path).SetLastSlash().Cat(de.FileName);
			SPathStruc::NormalizePath(temp_buf, 0, dest_path);
			SFileFormat ff;
			int ffr = 0;
			file_type_symb.Z();
			if(pDetectTypeOutFile) {
				ffr = ff.Identify(dest_path);
				if(ffr > 0) {
                    SFileFormat::GetExt(ff, file_type_symb);
				}
				temp_buf.Z().CatLongZ((long)ff, 3).Tab().CatLongZ((long)ffr, 3).Tab().Cat(file_type_symb).Tab().Cat(dest_path).CR();
				pDetectTypeOutFile->WriteLine(temp_buf);
			}
			aa.Clear();
			if(aa.CollectFileData(dest_path)) {
				const  uint64 count = aa.GetCount();
				aa.GetFreqListOrdered(freq_list);
				if(pOutFile) {
					const  char * p_psymb = ".,;:-_/\'`~!@#$%^&*()[]{}?<>=+\"";
					uint   zbits = 0;
					ps.Split(dest_path);
					if(count) {
						for(uint bi = 0; bi < 64; bi++) {
							if(!(count & ((uint64)1 << bi)))
								zbits++;
							else
								break;
						}
					}
					else
						zbits = 64;

					temp_buf.Z();

					(item_buf = ps.Ext).Align(10, ADJ_LEFT);
					temp_buf.Cat(item_buf);

					item_buf.Z().Cat(count);
					item_buf.Align(11, ADJ_RIGHT);
					temp_buf.Space().Cat(item_buf);
					temp_buf.Space().CatLongZ((long)zbits, 2);
					temp_buf.Space().Cat(dest_path);
					pOutFile->WriteLine(temp_buf.CR());

					temp_buf.Z();
					for(uint i = 0; i < freq_list.getCount(); i++) {
						const RAssoc item = freq_list.at(i);
                        uchar c = (uchar)item.Key;
						item_buf = 0;
						if(c == 0)
							item_buf.CatHex(c);
                        else if(isdec(c) || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || strchr(p_psymb, (int)c)) {
							item_buf.Space().CatChar(c);
                        }
                        else if(c == '\t')
							item_buf.CatChar('\\').CatChar('t');
						else
							item_buf.CatHex(c);
                        temp_buf.Cat(item_buf);
                        item_buf.Z().Cat(item.Val, MKSFMTD(8, 6, 0));
                        temp_buf.Space().Cat(item_buf);
                        item_buf.Z().Cat(aa.GetPeriodExp(c), MKSFMTD(8, 6, 0));
                        temp_buf.Space().Cat(item_buf);
                        item_buf.Z().Cat(aa.GetPeriodStdDev(c), MKSFMTD(8, 6, 0));
                        temp_buf.Space().Cat(item_buf);
                        temp_buf.Space();
					}
					pOutFile->WriteLine(temp_buf.CR());
				}
				if(ffr > 0 && pFileTypeToFreqOrderOutFile) {
					temp_buf.Z().CatLongZ((long)ff, 3).Tab().CatLongZ((long)ffr, 3).Tab().Cat(file_type_symb).Tab();
					for(uint i = 0; i < freq_list.getCount(); i++) {
						const RAssoc item = freq_list.at(i);
						uchar c = (uchar)item.Key;
                        temp_buf.CatHex(c);
					}
					temp_buf.CR();
					pFileTypeToFreqOrderOutFile->WriteLine(temp_buf);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI CollectLldFileStat()
{
    int    ok = 1;
#ifndef NDEBUG // {
    SString temp_buf, detect_file_name, freqorder_file_name;
    PPGetFilePath(PPPATH_OUT, "lldfilestat.log", temp_buf);
    PPGetFilePath(PPPATH_OUT, "lldfiledetect.log", detect_file_name);
    PPGetFilePath(PPPATH_OUT, "lldfreqorder.log", freqorder_file_name);
    SFile  f_out(temp_buf, SFile::mWrite);
    SFile  f_detect(detect_file_name, SFile::mWrite);
    SFile  f_freqorder(freqorder_file_name, SFile::mWrite);
    // "D:\\Papyrus\\Universe-HTT\\DATA"
	// "D:\\Papyrus\\Universe-HTT"
	ok = Helper_CollectLldFileStat("d:\\papyrus\\src", &f_out, &f_detect, &f_freqorder);
#endif // }
    return ok;
}
//
//
//
int SLAPI PPReadUnicodeBlockRawData(const char * pUnicodePath, const char * pCpPath, SUnicodeBlock & rBlk)
{
	int    ok = 1;
	xmlParserCtxt * p_ctx = 0;
	THROW(rBlk.Ut.ParseSource(pUnicodePath));
	//THROW(rBlk.Cpmp.ParseXml(pCpPath, &Ut));
	{
		SString base_path, path;
		SString temp_buf;
		//SString outp_lines[8];
		int    cp_sis = 0;
		SString cp_symb;
		SString cp_version;

		SFile  f_out((temp_buf = pCpPath).Strip().SetLastSlash().Cat("cp.log"), SFile::mWrite);
		(base_path = pCpPath).Strip();
		(temp_buf = base_path).SetLastSlash().Cat("*.xml");
		SDirEntry de;
		PPWait(1);
		for(SDirec sd(temp_buf, 0); sd.Next(&de) > 0;) {
			THROW(SETIFZ(p_ctx, xmlNewParserCtxt()));
			(temp_buf = base_path).SetLastSlash().Cat(de.FileName);
			THROW(rBlk.Cpmp.ParseXmlSingle(p_ctx, temp_buf, &rBlk.Ut));
			PPWaitMsg(temp_buf);
		}
	}
	CATCHZOK
	xmlFreeParserCtxt(p_ctx);
	return ok;
}

int SLAPI ParseCpEncodingTables(const char * pPath, SUnicodeTable * pUt)
{
	int    ok = 1;
	xmlParserCtxt * p_ctx = 0;
	SCodepageMapPool cpmp;
	//return cpmp.ParseXml(pPath, pUt);
	//int SLAPI SCodepageMapPool::ParseXml(const char * pPath, SUnicodeTable * pUt)
	{
		SString base_path, path;
		SString temp_buf;
		SString outp_lines[8];
		int    cp_sis = 0;
		SString cp_symb;
		SString cp_version;

		(temp_buf = pPath).Strip().SetLastSlash().Cat("cp.log");
		SFile  f_out(temp_buf, SFile::mWrite);

		(base_path = pPath).Strip();
		(temp_buf = base_path).SetLastSlash().Cat("*.xml");
		SDirEntry de;
		PPWait(1);
		for(SDirec sd(temp_buf, 0); sd.Next(&de) > 0;) {
			THROW(SETIFZ(p_ctx, xmlNewParserCtxt()));
			(temp_buf = base_path).SetLastSlash().Cat(de.FileName);
			THROW(cpmp.ParseXmlSingle(p_ctx, temp_buf, pUt));
			PPWaitMsg(temp_buf);
		}
	}
	CATCHZOK
	PPWait(0);
	xmlFreeParserCtxt(p_ctx);
	return ok;
}
