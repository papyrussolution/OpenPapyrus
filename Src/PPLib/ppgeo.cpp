// PPGEO.CPP
// Copyright (c) A.Sobolev 2015, 2016, 2017, 2018, 2019, 2020, 2022, 2023, 2025
//
#include <pp.h>
#pragma hdrstop
#include <sartre.h>
//#include <berkeleydb.h>
//#include <berkeleydb-6232.h>
//
//
//
PPOsm::NodeCluster::Put__Param::Put__Param(const Node * pN, uint nodeCount) :
	P_N(pN), P_NrWayRefs(0), P_NrRelRefs(0), NCount(nodeCount), NrWayRefsCount(0), NrRelRefsCount(0)
{
}

PPOsm::NodeCluster::Put__Result::Put__Result() : ActualCount(0), ActualNrWayCount(0), ActualNrRelCount(0), NrWayShift(0), NrRelShift(0)
{
}

PPOsm::NodeCluster::NodeCluster() : SBuffer(32)
{
}

PPOsm::NodeCluster::~NodeCluster()
{
}

size_t PPOsm::NodeCluster::GetSize() const
{
	return GetAvailableSize();
}

/*static*/uint PPOsm::NodeCluster::GetPossiblePackCount(const Node * pN, size_t count, uint * pPossibleCountLogic)
{
	static const uint __row[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
	uint   possible_count = 0;
	uint   possible_count_logic = 0;
	if(!count)
		possible_count = 0;
	else {
		if(count == 1) {
			possible_count_logic = 1;
			possible_count = 1;
		}
		else {
			uint count_logic = 1; // Количество логических точек в отрезке
			uint count_hole = 0;  // Количество дырок в отрезке (разрывов между последовательными идентификаторами)
			{
				uint64 prev_id = pN[0].ID;
				for(size_t i = 1; i < __row[0] && i < count; i++) {
					const uint64 id = pN[i].ID;
					THROW(id > prev_id);
					const uint64 _diff = (id - prev_id);
					if((count_logic + _diff) > __row[0]) {
						break;
					}
					else {
						count_logic += (uint)_diff;
						count_hole += (uint)(_diff - 1);
						prev_id = id;
					}
				}
			}
			possible_count_logic = 1;
			possible_count = 1;
			if(count_logic > 1) {
				const  int64 head_id = pN->ID;
				for(size_t i = 0; i < SIZEOFARRAY(__row); i++) {
					const uint r = __row[i];
					if((head_id & (r-1)) == 0 && count_logic >= r) {
						if(r > 1) {
							uint   possible_count_logic_bounded = 1;
							uint   possible_count_bounded = 1;
							uint   prev_zvalue = pN[0].T.GetZValue();
							uint64 prev_id = pN[0].ID;
							for(uint j = 1, idx = 1; j < r; j++) {
								if(pN[idx].ID == (prev_id+1)) {
									const uint zval = pN[idx].T.GetZValue();
									if(zval == prev_zvalue) {
										possible_count_logic++;
										possible_count++;
										idx++;
									}
									else {
										break;
									}
								}
								else
									possible_count_logic++;
								//
								// Фиксируем допустимые значения, ограниченные решеткой __row[]
								//
								for(uint n = 0; n < SIZEOFARRAY(__row); n++) {
									if(possible_count_logic == __row[n]) {
										possible_count_logic_bounded = possible_count_logic;
										possible_count_bounded = possible_count;
										break;
									}
								}
								prev_id++;
							}
							possible_count_logic = possible_count_logic_bounded;
							possible_count = possible_count_bounded;
						}
						break;
					}
				}
			}
		}
		//
		// Специальный случай: нет смысла тратиться на логический кластер
		// если фактически в нем только 1 элемент - проще сохранить единичный элемент.
		//
		if(possible_count_logic > 1 && possible_count == 1) {
			possible_count_logic = 1;
		}
		assert(possible_count >= 1 && possible_count <= count);
		assert(possible_count_logic >= 1);
		assert(oneof8(possible_count_logic, 1, 2, 4, 8, 16, 32, 64, 128));
	}
	CATCH
		possible_count = 0;
		possible_count_logic = 0;
	ENDCATCH
	ASSIGN_PTR(pPossibleCountLogic, possible_count_logic);
	assert(possible_count <= possible_count_logic);
	return possible_count;
}

/*
	// Заголовок определяющий заголовочную точку и параметры всего кластера
	Indicator
	ID
	Tile
	Lat
	Lon
	// Последующие точки
	[
		{
			InferiorIndicator : byte
			Lat               : int8 || int16 || int32
			Lon               : int8 || int16 || int32
			TileLevel         : byte || 0
		}
	]
	// Связанные с точками объекты
	[

		{
			LinkIndicator : byte
			NodePos       : byte
			ID            : int32 || int64
		}
	]
*/

//int PPOsm::NodeCluster::Put__(const Node * pN, const NodeRefs * pNrList, size_t count, uint64 * pOuterID, Put__Result * pResult, uint forceLogicalCount)
int PPOsm::NodeCluster::Put__(const Put__Param & rP, uint64 * pOuterID, Put__Result * pResult, uint forceLogicalCount)
{
	assert(oneof9(forceLogicalCount, 0, 1, 2, 4, 8, 16, 32, 64, 128));
	int    ok = 1;
	Put__Result result;
	uint   possible_count_logic = 0;
	const  uint possible_count = forceLogicalCount ? rP.NCount : GetPossiblePackCount(rP.P_N, rP.NCount, &possible_count_logic);
	if(forceLogicalCount)
		possible_count_logic = forceLogicalCount;
	THROW(possible_count);
	const int64 last_possible_id = rP.P_N[possible_count-1].ID;
	Z();
    {
		Int64Array node_id_list;
		const Node & r_head = rP.P_N[0];
		const int32 head_lat = r_head.C.GetIntLat();
		const int32 head_lon = r_head.C.GetIntLon();
    	uint8 indicator = 0;
		switch(possible_count_logic) {
        	case 128: indicator = 7; break;
        	case 64: indicator = 6; break;
        	case 32: indicator = 5; break;
        	case 16: indicator = 4; break;
        	case 8: indicator = 3; break;
        	case 4: indicator = 2; break;
        	case 2: indicator = 1; break;
        	case 1: indicator = 0; break;
        	default:
				CALLEXCEPT(); // invalid count
        }
        if(pOuterID) {
            indicator |= indfOuterId;
        }
        else if(r_head.ID <= ULONG_MAX) {
			indicator |= indfId32;
        }
		LLAssocArray way_refs;
		LLAssocArray rel_refs;
		if(rP.P_NrWayRefs && rP.NrWayRefsCount) {
			uint last_node_pos = 0;
			result.NrWayShift = rP.NrWayRefsCount;
            for(uint nwi = 0; nwi < rP.NrWayRefsCount; nwi++) {
				const LLAssoc & r_assoc = rP.P_NrWayRefs[nwi];
				if(r_assoc.Key > last_possible_id) {
					result.NrWayShift = nwi;
					break;
				}
				else {
					for(uint i = last_node_pos; i < possible_count; i++) {
						if((int64)rP.P_N[i].ID == r_assoc.Key) {
							THROW(way_refs.Add(r_assoc.Key, r_assoc.Val, 0));
							last_node_pos = i;
							break;
						}
						else if((int64)rP.P_N[i].ID > r_assoc.Key) {
							last_node_pos = i;
							break;
						}
					}
				}
            }
			if(way_refs.getCount()) {
				way_refs.Sort();
				indicator |= indfHasRefs;
			}
		}
		if(rP.P_NrRelRefs && rP.NrRelRefsCount) {
			uint last_node_pos = 0;
			result.NrRelShift = rP.NrRelRefsCount;
            for(uint nwi = 0; nwi < rP.NrRelRefsCount; nwi++) {
				const LLAssoc & r_assoc = rP.P_NrRelRefs[nwi];
				if(r_assoc.Key > last_possible_id) {
					result.NrRelShift = nwi;
					break;
				}
				else {
					for(uint i = last_node_pos; i < possible_count; i++) {
						if((int64)rP.P_N[i].ID == r_assoc.Key) {
							THROW(rel_refs.Add(r_assoc.Key, r_assoc.Val, 0));
							last_node_pos = i;
							break;
						}
						else if((int64)rP.P_N[i].ID > r_assoc.Key) {
							last_node_pos = i;
							break;
						}
					}
				}
            }
			if(rel_refs.getCount()) {
				rel_refs.Sort();
				indicator |= indfHasRefs;
			}
		}
		THROW_SL(Write(indicator));
		if(!(indicator & indfOuterId)) {
			if(indicator & indfId32) {
				const uint32 id32 = (uint32)r_head.ID;
				THROW_SL(Write(id32));
			}
			else {
				THROW_SL(Write(r_head.ID));
			}
		}
		{
			//uint8 level = r_head.T.GetLevel();
			THROW_SL(Write(r_head.T.V));
		}
		THROW_SL(Write(head_lat));
		THROW_SL(Write(head_lon));
		if(indicator & indfHasRefs) {
			node_id_list.add((int64)r_head.ID);
		}
		result.ActualCount++;
		{
			int64  last_real_id = r_head.ID;
			if(possible_count_logic > 1) {
				const uint8 head_level = r_head.T.GetLevel();
				uint64 prev_id = r_head.ID;
				int32  prev_lat = head_lat;
				int32  prev_lon = head_lon;
				for(uint i = 1, idx = 1; i < possible_count_logic /*&& idx < possible_count*/; i++) {
					uint8 inf_indicator = 0;
					if(idx < possible_count) {
						assert(rP.P_N[idx].ID > prev_id);
						assert(rP.P_N[idx].T.GetZValue() == r_head.T.GetZValue());
						if(rP.P_N[idx].ID == (prev_id+1)) {
							last_real_id = (int64)rP.P_N[idx].ID;
							if(indicator & indfHasRefs) {
								node_id_list.add(last_real_id);
							}
							const uint8 _tile_level = rP.P_N[idx].T.GetLevel();
							const int32 _lat = rP.P_N[idx].C.GetIntLat();
							const int32 _lon = rP.P_N[idx].C.GetIntLon();
							const int32 _lat_diff = (_lat - prev_lat);
							const int32 _lon_diff = (_lon - prev_lon);
							if(_lat_diff >= SCHAR_MIN && _lat_diff <= SCHAR_MAX)
								inf_indicator |= infindfPrevLatIncr8;
							else if(_lat_diff >= SHRT_MIN && _lat_diff <= SHRT_MAX)
								inf_indicator |= infindfPrevLatIncr16;
							if(_lon_diff >= SCHAR_MIN && _lon_diff <= SCHAR_MAX)
								inf_indicator |= infindfPrevLonIncr8;
							else if(_lon_diff >= SHRT_MIN && _lon_diff <= SHRT_MAX)
								inf_indicator |= infindfPrevLonIncr16;
							if(_tile_level != head_level)
								inf_indicator |= infindfDiffTileLevel;
							THROW_SL(Write(inf_indicator));
							if(inf_indicator & infindfPrevLatIncr8) {
								const int8 _diff = (int8)_lat_diff;
								THROW_SL(Write(_diff));
							}
							else if(inf_indicator & infindfPrevLatIncr16) {
								const int16 _diff = (int16)_lat_diff;
								THROW_SL(Write(_diff));
							}
							else {
								THROW_SL(Write(_lat));
							}
							if(inf_indicator & infindfPrevLonIncr8) {
								const int8 _diff = (int8)_lon_diff;
								THROW_SL(Write(_diff));
							}
							else if(inf_indicator & infindfPrevLonIncr16) {
								const int16 _diff = (int16)_lon_diff;
								THROW_SL(Write(_diff));
							}
							else {
								THROW_SL(Write(_lon));
							}
							if(inf_indicator & infindfDiffTileLevel)
								THROW_SL(Write(_tile_level));

							prev_lat = _lat;
							prev_lon = _lon;
							idx++;
							result.ActualCount++;
						}
						else {
							inf_indicator |= infindfEmpty;
							THROW_SL(Write(inf_indicator));
						}
					}
					else {
						inf_indicator |= infindfEmpty;
						THROW_SL(Write(inf_indicator));
					}
					prev_id++;
				}
			}
			assert(last_real_id == last_possible_id);
			if(indicator & indfHasRefs) {
				assert(way_refs.getCount() || rel_refs.getCount()); // @paranoic
				uint8  _sid[16];
				{
					for(uint i = 0; i < way_refs.getCount(); i++) {
						const  LLAssoc & r_assoc = way_refs.at(i);
						uint   node_pos = 0;
						if(node_id_list.lsearch(r_assoc.Key, &node_pos)) {
							assert(node_pos >= 0 && node_pos < 256);
							const uint  rs = sshrinkuint64((uint64)r_assoc.Val, _sid);
							uint8 ref_indicator = 0;
							assert(rs >= 1 && rs <= 8);
							ref_indicator = (uint8)(rs - 1);
							ref_indicator |= refindfWay;
							THROW_SL(Write(ref_indicator));
							{
								uint8  node_pos8 = (uint8)node_pos;
								THROW_SL(Write(node_pos8));
							}
							THROW_SL(Write(_sid, rs));
							result.ActualNrWayCount++;
						}
						else if(r_assoc.Key > last_real_id) {
							break;
						}
					}
				}
				{
					for(uint i = 0; i < rel_refs.getCount(); i++) {
						const  LLAssoc & r_assoc = rel_refs.at(i);
						uint   node_pos = 0;
						if(node_id_list.lsearch(r_assoc.Key, &node_pos)) {
							assert(node_pos >= 0 && node_pos < 256);
							const uint  rs = sshrinkuint64((uint64)r_assoc.Val, _sid);
							uint8 ref_indicator = 0;
							assert(rs >= 1 && rs <= 8);
							ref_indicator = (uint8)(rs - 1);
							ref_indicator |= refindfRelation;
							THROW_SL(Write(ref_indicator));
							{
								uint8  node_pos8 = (uint8)node_pos;
								THROW_SL(Write(node_pos8));
							}
							THROW_SL(Write(_sid, rs));
							result.ActualNrRelCount++;
						}
						else if(r_assoc.Key > last_real_id) {
							break;
						}
					}
				}
				{
					//
					// Терминальный индикатор
					//
					uint8 ref_indicator = refindfTerminal;
					THROW_SL(Write(ref_indicator));
				}
			}
		}
		ASSIGN_PTR(pOuterID, r_head.ID);
    }
    CATCHZOK
	//ASSIGN_PTR(pActualCount, actual_count);
	ASSIGN_PTR(pResult, result);
	return ok;
}

int PPOsm::NodeCluster::Implement_Get(uint64 outerID, TSVector <Node> * pList, NodeRefs * pNrList, Node * pHead, uint * pCountLogic, uint * pCountActual)
{
	int    ok = 1;
	uint   count_logic = 0;
	uint   count_actual = 0;
	uint8  indicator = 0;
	Node   head;
	int32  head_lat = 0;
	int32  head_lon = 0;
	Int64Array node_id_list;
	const size_t preserve_pos = GetRdOffs();
	THROW_SL(Read(indicator));
	switch(indicator & indfCountMask) {
		case 0: count_logic =   1; break;
		case 1: count_logic =   2; break;
		case 2: count_logic =   4; break;
		case 3: count_logic =   8; break;
		case 4: count_logic =  16; break;
		case 5: count_logic =  32; break;
		case 6: count_logic =  64; break;
		case 7: count_logic = 128; break;
       	default: CALLEXCEPT(); // invalid count
	}
	if(indicator & indfOuterId) {
		head.ID = outerID;
	}
	else if(indicator & indfId32) {
		uint32   id32 = 0;
		THROW_SL(Read(id32));
		head.ID = id32;
	}
	else {
		THROW_SL(Read(head.ID));
	}
	THROW(head.ID && (head.ID % count_logic) == 0); // @todo errcode
	{
		//uint8 level = 0;
		THROW_SL(Read(head.T.V));
		//head.T.SetLevel(level);
	}
	THROW_SL(Read(head_lat));
	THROW_SL(Read(head_lon));
	head.C.SetInt(head_lat, head_lon);
	if(indicator & indfHasRefs && pNrList) {
		THROW_SL(node_id_list.add((int64)head.ID));
	}
	if(pList || pCountActual) {
		{
			if(pList)
				THROW_SL(pList->insert(&head));
			count_actual++;
		}
		if(count_logic > 1) {
			const uint8 head_level = head.T.GetLevel();
			uint64 prev_id = head.ID;
			int32  prev_lat = head_lat;
			int32  prev_lon = head_lon;
			for(uint i = 1; i < count_logic; i++) {
				uint8 inf_indicator = 0;
				THROW_SL(Read(inf_indicator));
				if(!(inf_indicator & infindfEmpty)) {
					uint8 _tile_level = 0;
					int32 _lat = 0;
					int32 _lon = 0;
					int32 _lat_diff = 0;
					int32 _lon_diff = 0;
					Node   node;
					if(inf_indicator & infindfPrevLatIncr8) {
						int8   _diff = 0;
						THROW_SL(Read(_diff));
						_lat = prev_lat + (int32)_diff;
					}
					else if(inf_indicator & infindfPrevLatIncr16) {
						int16  _diff = 0;
						THROW_SL(Read(_diff));
						_lat = prev_lat + (int32)_diff;
					}
					else {
						THROW_SL(Read(_lat));
					}
					if(inf_indicator & infindfPrevLonIncr8) {
						int8   _diff = 0;
						THROW_SL(Read(_diff));
						_lon = prev_lon + (int32)_diff;
					}
					else if(inf_indicator & infindfPrevLonIncr16) {
						int16  _diff = 0;
						THROW_SL(Read(_diff));
						_lon = prev_lon + (int32)_diff;
					}
					else {
						THROW_SL(Read(_lon));
					}
					if(inf_indicator & infindfDiffTileLevel) {
						THROW_SL(Read(_tile_level));
					}
					else
						_tile_level = head_level;
					node.C.SetInt(_lat, _lon);
					node.ID = (prev_id+1);
					node.T = head.T;
					node.T.SetLevel(_tile_level);
					if(pList)
						THROW_SL(pList->insert(&node));
					if(indicator & indfHasRefs && pNrList) {
						THROW_SL(node_id_list.add((int64)node.ID));
					}
					count_actual++;
					prev_lat = _lat;
					prev_lon = _lon;
				}
				prev_id++;
			}
		}
	}
	if(indicator & indfHasRefs && pNrList) {
		uint8 ref_indicator = 0;
		uint8  _sid[16];
		do {
			THROW_SL(Read(ref_indicator));
			if(!(ref_indicator & refindfTerminal)) {
				assert((ref_indicator&refindfWay)^(ref_indicator&refindfRelation));
				uint   node_pos = 0;
				uint8  node_pos8 = 0;
				THROW_SL(Read(node_pos8));
				node_pos = node_pos8;
				assert(node_pos >= 0 && node_pos < node_id_list.getCount());
				{
					const uint  rs = (ref_indicator & refindfIdSizeMask) + 1;
					assert(rs >= 1 && rs <= 8);
					memzero(_sid, sizeof(_sid));
					THROW_SL(Read(_sid, rs));
					const uint64 ref_id = sexpanduint64(_sid, rs);
					const uint64 node_id = node_id_list.get(node_pos);
					if(ref_indicator & refindfWay) {
						THROW_SL(pNrList->AddWayRef(node_id, ref_id));
					}
					else if(ref_indicator & refindfRelation) {
						THROW_SL(pNrList->AddRelRef(node_id, ref_id));
					}
				}
			}
		} while(!(ref_indicator & refindfTerminal));
	}
	ASSIGN_PTR(pHead, head);
	ASSIGN_PTR(pCountLogic, count_logic);
	ASSIGN_PTR(pCountActual, count_actual);
	CATCH
		ok = 0;
	ENDCATCH
	SetRdOffs(preserve_pos);
	return ok;
}

int PPOsm::NodeCluster::Get(uint64 outerID, TSVector <Node> & rList, NodeRefs * pNrList)
{
	return Implement_Get(outerID, &rList, pNrList, 0, 0, 0);
}

int PPOsm::NodeCluster::Get(uint64 outerID, TSVector <Node> & rList, NodeRefs * pNrList, Node * pHead, uint * pCountLogic, uint * pCountActual)
{
	return Implement_Get(outerID, &rList, pNrList, pHead, pCountLogic, pCountActual);
}

int PPOsm::NodeCluster::GetCount(uint64 outerID, uint * pLogicCount, uint * pActualCount)
{
	return Implement_Get(outerID, 0, 0, 0, pLogicCount, pActualCount);
}

int FASTCALL PPOsm::NodeCluster::GetHeaderID(uint64 * pID)
{
	Node   head;
	int    ok = Implement_Get(0, 0, 0, &head, 0, 0);
	if(ok) {
		ASSIGN_PTR(pID, head.ID);
	}
	return ok;
}

int FASTCALL PPOsm::NodeCluster::GetTile(uint64 outerID, Tile * pT)
{
	Node   head;
	int    ok = Implement_Get(outerID, 0, 0, &head, 0, 0);
	if(ok) {
		ASSIGN_PTR(pT, head.T);
	}
	return ok;
}

const void * FASTCALL PPOsm::NodeCluster::GetBuffer(size_t * pSize) const
{
    const void * ptr = SBuffer::GetBuf(0);
    ASSIGN_PTR(pSize, SBuffer::GetAvailableSize());
    return ptr;
}

int PPOsm::NodeCluster::SetBuffer(const void * pData, size_t size)
{
	int    ok = 1;
	SBuffer::Z();
	THROW_SL(SBuffer::Write(pData, size));
	CATCHZOK
	return ok;
}
//
//
//
PPOsm::WayBuffer::WayBuffer() : SBuffer()
{
}

int PPOsm::WayBuffer::Put(const Way * pW, uint64 * pOuterID)
{
	int    ok = 1;
	Z();
    {
		uint8 indicator = 0;
		const uint rc = pW->NodeRefs.getCount();
        if(pOuterID) {
            indicator |= indfOuterId;
        }
        else if(pW->ID <= ULONG_MAX) {
			indicator |= indfId32;
        }
		if(rc < 256)
			indicator |= indfCount8;
		if(rc) {
			uint8  sib[16];
			const int64 first_ref_id = pW->NodeRefs.at(0);
			const uint  frs = sshrinkuint64(first_ref_id, sib);
			if(frs <= 4)
				indicator |= indfFirstId32;
			if(rc > 1) {
				if(first_ref_id == pW->NodeRefs.at(rc-1)) {
                    indicator |= indfLoop;
                    if(rc == 5)
						indicator |= indfRectangle;
				}
                int64 max_id_diff = 0;
				for(uint i = 1; i < rc; i++) {
					if(indicator & indfLoop && i == (rc-1))
						break;
                    int64 id_diff = _abs64(pW->NodeRefs.at(i) - pW->NodeRefs.at(i-1));
                    SETMAX(max_id_diff, id_diff);
				}
				if(max_id_diff < (1<<7))
					indicator |= indfIncremental8;
				else if(max_id_diff < (1<<15))
					indicator |= indfIncremental16;
			}
		}
		//
		THROW_SL(Write(indicator));
		if(!(indicator & indfOuterId)) {
			if(indicator & indfId32) {
				const uint32 id32 = (uint32)pW->ID;
				THROW_SL(Write(id32));
			}
			else {
				THROW_SL(Write(pW->ID));
			}
		}
		{
			uint8 tile_level = pW->T.GetLevel();
			THROW_SL(Write(tile_level));
		}
		if(!(indicator & indfRectangle)) {
			if(indicator & indfCount8) {
                uint8 cb = (uint8)rc;
                THROW_SL(Write(cb));
			}
			else {
				uint32 cdw = rc;
				THROW_SL(Write(cdw));
			}
		}
		if(rc) {
			const int64 first_ref_id = pW->NodeRefs.at(0);
			if(indicator & indfFirstId32) {
				uint32 f32 = (uint32)first_ref_id;
				THROW_SL(Write(f32));
			}
			else {
				uint64 f64 = (uint64)first_ref_id;
				THROW_SL(Write(f64));
			}
			int64 prev_id = first_ref_id;
			for(uint i = 1; i < rc; i++) {
				if(indicator & indfLoop && i == (rc-1))
					break;
				else {
					const int64 cur_id = pW->NodeRefs.at(i);
					int64 id_diff = (cur_id - prev_id);
					if(indicator & indfIncremental8) {
						assert(id_diff > -(1<<7) && id_diff < (1<<7));
						int8 d8 = (int8)id_diff;
						THROW_SL(Write(d8));
					}
					else if(indicator & indfIncremental16) {
						assert(id_diff > -(1<<15) && id_diff < (1<<15));
						int16 d16 = (int16)id_diff;
						THROW_SL(Write(d16));
					}
					else {
						uint8 infind = 0;
						if(id_diff > -(1<<7) && id_diff < (1<<7)) {
                            infind = 1 | infindfIncremental;
							int8 d8 = (int8)id_diff;
							THROW_SL(Write(infind));
							THROW_SL(Write(d8));
						}
						else if(id_diff > -(1<<15) && id_diff < (1<<15)) {
                            infind = 2 | infindfIncremental;
							int16 d16 = (int16)id_diff;
							THROW_SL(Write(infind));
							THROW_SL(Write(d16));
						}
						else if(id_diff > -(1<<31) && id_diff < (1<<31)) {
                            infind = 4 | infindfIncremental;
							int32 d32 = (int32)id_diff;
							THROW_SL(Write(infind));
							THROW_SL(Write(d32));
						}
						else {
							uint8  sib[16];
							const uint frs = sshrinkuint64(cur_id, sib);
							assert(frs >= 1 && frs <= 8);
							infind = (uint8)frs;
							THROW_SL(Write(infind));
							THROW_SL(Write(sib, frs));
						}
					}
					prev_id = cur_id;
				}
			}
		}
		ASSIGN_PTR(pOuterID, pW->ID);
    }
    CATCHZOK
	return ok;
}

int PPOsm::WayBuffer::Get(uint64 outerID, Way * pW)
{
	// (IND) [ID] (TileLevel) [COUNT] ([INFIND] (POINT-ID))+

	pW->ID = 0;
	pW->T.V = 0;
	pW->NodeRefs.clear();

	int    ok = 1;
	uint8  indicator = 0;
	const size_t preserve_pos = GetRdOffs();
	THROW_SL(Read(indicator));
	if(!(indicator & indfOuterId)) {
		if(indicator & indfId32) {
			uint32 id32;
			THROW_SL(Read(id32));
			pW->ID = id32;
		}
		else {
			THROW_SL(Read(pW->ID));
		}
	}
	else
		pW->ID = outerID;
	{
		uint8 tile_level;
		THROW_SL(Read(tile_level));
		pW->T.SetLevel(tile_level);
	}
	{
		uint32 rc = 0;
		if(!(indicator & indfRectangle)) {
			if(indicator & indfCount8) {
				uint8 cb;
				THROW_SL(Read(cb));
				rc = cb;
			}
			else {
				uint32 cdw;
				THROW_SL(Read(cdw));
				rc = cdw;
			}
		}
		else
			rc = 5;
		if(rc) {
			int64 first_ref_id = 0;
			if(indicator & indfFirstId32) {
				uint32 f32;
				THROW_SL(Read(f32));
				first_ref_id = (int64)f32;
			}
			else {
				uint64 f64;
				THROW_SL(Read(f64));
				first_ref_id = (int64)f64;
			}
			pW->NodeRefs.add(first_ref_id);
			int64 prev_id = first_ref_id;
			for(uint i = 1; i < rc; i++) {
				if(indicator & indfLoop && i == (rc-1)) {
					pW->NodeRefs.add(first_ref_id); // Вставляем замыкающую точку контура равную первой
					break;
				}
				else {
					int64 cur_id = 0;
					if(indicator & indfIncremental8) {
						int8 d8;
						THROW_SL(Read(d8));
                        cur_id = prev_id + d8;
					}
					else if(indicator & indfIncremental16) {
						int16 d16;
						THROW_SL(Read(d16));
						cur_id = prev_id + d16;
					}
					else {
						uint8 infind = 0;
						THROW_SL(Read(infind));
						if(infind & infindfIncremental) {
							infind &= ~infindfIncremental;
							assert(oneof3(infind, 1, 2, 4));
							if(infind == 1) {
								int8 d8;
								THROW_SL(Read(d8));
								cur_id = prev_id + d8;
							}
							else if(infind == 2) {
								int16 d16;
								THROW_SL(Read(d16));
								cur_id = prev_id + d16;
							}
							else if(infind == 4) {
								int32 d32;
								THROW_SL(Read(d32));
								cur_id = prev_id + d32;
							}
						}
						else {
							uint8  sib[16];
							uint   frs = (uint)infind;
							assert(frs >= 1 && frs <= 8);
							THROW_SL(Read(sib, frs));
							cur_id = sexpanduint64(sib, frs);
						}
					}
					assert(cur_id > 0);
					pW->NodeRefs.add(cur_id);
					prev_id = cur_id;
				}
			}
		}
	}
	CATCHZOK
	SetRdOffs(preserve_pos);
	return ok;
}

const void * FASTCALL PPOsm::WayBuffer::GetBuffer(size_t * pSize) const
{
    const void * ptr = SBuffer::GetBuf(0);
    ASSIGN_PTR(pSize, SBuffer::GetAvailableSize());
    return ptr;
}

int PPOsm::WayBuffer::SetBuffer(const void * pData, size_t size)
{
	int    ok = 1;
	SBuffer::Z();
	THROW_SL(SBuffer::Write(pData, size));
	CATCHZOK
	return ok;
}

size_t PPOsm::WayBuffer::GetSize() const
{
	return SBuffer::GetAvailableSize();
}
//
//
//
PPOsm::Tile::Tile() : V(0)
{
}

PPOsm::Tile::Tile(const Tile & rS)
{
	V = rS.V;
}

PPOsm::Tile & FASTCALL PPOsm::Tile::operator = (const Tile & rS)
{
	V = rS.V;
	return *this;
}

void PPOsm::Tile::SetInvisible()
{
	V |= 0xff000000;
}

void FASTCALL PPOsm::Tile::SetLevel(uint8 level)
{
	V = ((V & 0x00ffffff) | (((uint32)level) << 24));
}

uint8 PPOsm::Tile::GetLevel() const
{
	return (uint8)((V & 0xff000000) >> 24);
}

uint32 PPOsm::Tile::GetZValue() const
{
	return (V & 0x00ffffff);
}
//
//
//
PPOsm::NPoint::NPoint() : ID(0)
{
}

bool FASTCALL PPOsm::NPoint::IsEq(const PPOsm::NPoint & rS) const { return (ID == rS.ID && C == rS.C); }
bool FASTCALL PPOsm::NPoint::operator == (const PPOsm::NPoint & rS) const { return IsEq(rS); }
bool FASTCALL PPOsm::NPoint::operator != (const PPOsm::NPoint & rS) const { return !IsEq(rS); }

PPOsm::Node::Node() : NPoint()
{
}

bool FASTCALL PPOsm::Node::IsEq(const Node & rS) const { return (NPoint::IsEq(rS) && T.V == rS.T.V); }
bool FASTCALL PPOsm::Node::operator == (const Node & rS) const { return IsEq(rS); }
bool FASTCALL PPOsm::Node::operator != (const Node & rS) const { return !IsEq(rS); }

PPOsm::NodeRefs::NodeRefs()
{
}

bool FASTCALL PPOsm::NodeRefs::IsEq(const NodeRefs & rS) const { return (WayRefs == rS.WayRefs && RelRefs == rS.RelRefs); }

int PPOsm::NodeRefs::AddWayRef(uint64 nodeID, uint64 wayID)
{
	if(!WayRefs.SearchPair(nodeID, wayID, 0))
		return WayRefs.Add(nodeID, wayID, 0) ? 1 : PPSetErrorSLib();
	else
		return -1;
}

int PPOsm::NodeRefs::AddRelRef(uint64 nodeID, uint64 relID)
{
	if(!RelRefs.SearchPair(nodeID, relID, 0))
		return RelRefs.Add(nodeID, relID, 0) ? 1 : PPSetErrorSLib();
	else
		return -1;
}

void PPOsm::NodeRefs::Clear()
{
	WayRefs.clear();
	RelRefs.clear();
}

void PPOsm::NodeRefs::Sort()
{
	WayRefs.Sort();
	RelRefs.Sort();
}

PPOsm::Way::Way() : ID(0)
{
}

void PPOsm::Way::Clear()
{
	ID = 0;
	T.V = 0;
	NodeRefs.clear();
}

bool FASTCALL PPOsm::Way::IsEq(const Way & rS) const
{
    bool   ok = true;
    THROW(ID == rS.ID);
    THROW(T.V == rS.T.V);
    THROW(NodeRefs == rS.NodeRefs);
    CATCHZOK
    return ok;
}

bool FASTCALL PPOsm::Way::operator == (const Way & rS) const { return IsEq(rS); }
bool FASTCALL PPOsm::Way::operator != (const Way & rS) const { return !IsEq(rS); }

PPOsm::RelMember::RelMember() : RefID(0), TypeSymbID(0), RoleSymbID(0)
{
}

PPOsm::Relation::Relation() : ID(0)
{
}

PPOsm::Tag::Tag() : KeySymbID(0), ValID(0)
{
}

/*static*/int FASTCALL PPOsm::SetProcessedNodeStat(uint logicalCount, uint qtty, TSVector <NodeClusterStatEntry> & rStat)
{
	int    ok = 1;
	int    found = 0;
	for(uint i = 0; !found && i < rStat.getCount(); i++) {
		NodeClusterStatEntry & r_entry = rStat.at(i);
		if(r_entry.LogicalCount == logicalCount) {
			r_entry.ProcessedCount += qtty;
			found = 1;
		}
	}
	if(!found) {
		NodeClusterStatEntry new_entry;
		MEMSZERO(new_entry);
		new_entry.LogicalCount = logicalCount;
		new_entry.ProcessedCount = qtty;
		THROW(rStat.insert(&new_entry));
	}
	CATCHZOK
	return ok;
}

/*static*/int FASTCALL PPOsm::SetProcessedWayStat(uint refCount, uint qtty, TSVector <WayStatEntry> & rStat)
{
	int    ok = 1;
	int    found = 0;
	for(uint i = 0; !found && i < rStat.getCount(); i++) {
		WayStatEntry & r_entry = rStat.at(i);
		if(r_entry.RefCount == refCount) {
			r_entry.ProcessedCount += qtty;
			found = 1;
		}
	}
	if(!found) {
		WayStatEntry new_entry;
		MEMSZERO(new_entry);
		new_entry.RefCount = refCount;
		new_entry.ProcessedCount = qtty;
		THROW(rStat.insert(&new_entry));
	}
	CATCHZOK
	return ok;
}

/*static*/int FASTCALL PPOsm::SetWayStat(WayBuffer & rWayBuf, TSVector <WayStatEntry> & rStat)
{
    int    ok = 1;
    Way    way;
    uint64 surr_outer_id = 1024; // Суррогатный outer id
    THROW(rWayBuf.Get(surr_outer_id, &way));
    {
    	const uint _wrc = way.NodeRefs.getCount();
    	int   _found = 0;
		for(uint i = 0; !_found && i < rStat.getCount(); i++) {
            if(rStat.at(i).RefCount == _wrc) {
				WayStatEntry & r_entry = rStat.at(i);
				r_entry.Size += rWayBuf.GetSize();
				r_entry.WayCount++;
				_found = 1;
            }
		}
		if(!_found) {
			WayStatEntry new_entry;
			MEMSZERO(new_entry);
			new_entry.RefCount = _wrc;
			new_entry.WayCount = 1;
			new_entry.Size = rWayBuf.GetSize();
			THROW_SL(rStat.insert(&new_entry));
		}
	}
    CATCHZOK
    return ok;
}

/*static*/int FASTCALL PPOsm::SetNodeClusterStat(NodeCluster & rCluster, TSVector <NodeClusterStatEntry> & rStat)
{
	int    ok = 1;
	uint   count_logic = 0;
	uint   count_actual = 0;
	uint64 surr_outer_id = 1024; // Мы не знаем здесь значения внешнего идентификатора, но он нам фактически не очень и нужен.
		// Что бы функция rCluster.GetCount отработала правильно в данном конкретном случае достаточно
		// подложить ей значение внешнего идентификатора кратное максимально возможному значению.
	THROW(rCluster.GetCount(surr_outer_id, &count_logic, &count_actual));
	{
		const  size_t sz = rCluster.GetSize();
		int    found = 0;
		for(uint i = 0; !found && i < rStat.getCount(); i++) {
			NodeClusterStatEntry & r_entry = rStat.at(i);
			if(r_entry.LogicalCount == count_logic) {
				r_entry.ActualCount += count_actual;
				r_entry.ClusterCount++;
				r_entry.Size += sz;
				found = 1;
			}
		}
		if(!found) {
			NodeClusterStatEntry new_entry;
			MEMSZERO(new_entry);
			new_entry.LogicalCount = count_logic;
			new_entry.ActualCount = count_actual;
			new_entry.ClusterCount = 1;
			new_entry.Size = sz;
			THROW(rStat.insert(&new_entry));
		}
	}
	CATCHZOK
	return ok;
}

PPOsm::PPOsm(const char * pDbPath) : Ht(1024*1024, 0), Grid(12), P_SrDb(0), LastSymbID(0), Status(0)
{
	if(!isempty(pDbPath))
		OpenDatabase(pDbPath);
}

PPOsm::~PPOsm()
{
	delete P_SrDb;
}

int PPOsm::OpenDatabase(const char * pDbPath)
{
	int    ok = 1;
	ZDELETE(P_SrDb);
	THROW_MEM(P_SrDb = new SrDatabase);
	THROW(P_SrDb->Open(pDbPath, SrDatabase::oWriteStatOnClose));
	CATCH
		ZDELETE(P_SrDb);
		ok = 0;
	ENDCATCH
	return ok;
}

SrDatabase * PPOsm::GetDb()
{
	return P_SrDb;
}

int PPOsm::LoadGeoGrid()
{
    int    ok = -1;
    if(!(Status & stGridLoaded)) {
		SString path, filename;
		PPGetPath(PPPATH_DD, path);
		path.SetLastSlash();
		(filename = path).Cat("planet-*-grid-12.txt");
		SDirEntry de;
		for(SDirec dir(filename, 0); dir.Next(&de) > 0;) {
			if(de.IsFile()) {
				de.GetNameA(path, filename);
				THROW_SL(Grid.Load(filename));
				Status |= stGridLoaded;
			}
		}
    }
    CATCHZOK
    return ok;
}

int PPOsm::BuildHashAssoc()
{
	return Ht.BuildAssoc();
}

long FASTCALL PPOsm::CheckStatus(long f) const
{
	return BIN((Status & f) == f);
}

uint FASTCALL PPOsm::SearchSymb(const char * pSymb) const
{
	uint   val = 0;
	uint   pos = 0;
	return Ht.Search(pSymb, &val, &pos) ? val : 0;
}

uint FASTCALL PPOsm::CreateSymb(const char * pSymb)
{
	uint   val = 0;
	uint   pos = 0;
	if(!Ht.Search(pSymb, &val, &pos)) {
        val = ++LastSymbID;
        if(!Ht.Add(pSymb, val))
			val = 0;
	}
	return val;
}

int PPOsm::GetSymbByID(uint id, SString & rSymb) const
{
	return Ht.GetByAssoc(id, rSymb);
}
//
//
//
PPGeoTrackItem::PPGeoTrackItem()
{
	THISZERO();
}

PPGeoTrackItem & FASTCALL PPGeoTrackItem::operator = (const PPGeoTrackItem & rS)
{
	memcpy(this, &rS, sizeof(*this));
	return *this;
}

PPGeoTrackItem & FASTCALL PPGeoTrackItem::operator = (const GeoTrackTbl::Rec & rS)
{
	Oid.Set(rS.ObjType, rS.ObjID);
	ExtOid.Set(rS.ExtObjType, rS.ExtObjID);
	Dtm.Set(GeoTrackCore::ConvertStorageDate(rS.Dts2010), rS.Tm);
	ExtEvent = rS.ExtEvent;
	Flags = rS.Flags;
	Latitude = rS.Latitude;
	Longitude = rS.Longitude;
	Altitude = rS.Altitude;
	Speed = rS.Speed * 10;
	return *this;
}

int FASTCALL PPGeoTrackItem::Get(GeoTrackTbl::Rec & rD) const
{
	MEMSZERO(rD);
	rD.ObjType = (int16)Oid.Obj;
	rD.ObjID = Oid.Id;
	rD.ExtObjType = (int16)ExtOid.Obj;
	rD.ExtObjID = ExtOid.Id;
	rD.Dts2010 = GeoTrackCore::GetStorageDate(Dtm.d);
	rD.Tm = Dtm.t;
	rD.ExtEvent = (int16)ExtEvent;
	rD.Flags = Flags;
	rD.Latitude = Latitude;
	rD.Longitude = Longitude;
	rD.Altitude = (int16)R0(Altitude);
	rD.Speed = (int16)R0(Speed / 10);
	return 1;
}

/*static*/LDATE FASTCALL GeoTrackCore::ConvertStorageDate(int16 sd)
{
	return (sd == 0) ? ZERODATE : plusdate(encodedate(1, 1, 2010), sd);
}

/*static*/int16 FASTCALL GeoTrackCore::GetStorageDate(LDATE dt)
{
	int16  result = 0;
	if(dt) {
		const LDATE _basedt = encodedate(1, 1, 2010);
		if(dt >= _basedt)
			result = (int16)diffdate(dt, _basedt);
	}
	return result;
}

GeoTrackCore::GeoTrackCore() : GeoTrackTbl()
{
}

int GeoTrackCore::Search(PPObjID oid, LDATETIME dtm, PPGeoTrackItem * pItem)
{
	int    ok = 1;
	GeoTrackTbl::Key0 k0;
	MEMSZERO(k0);
	k0.ObjType = (int16)oid.Obj;
	k0.ObjID = oid.Id;
	k0.Dts2010 = GetStorageDate(dtm.d);
	k0.Tm = dtm.t;
	if(search(0, &k0, spEq)) {
		ASSIGN_PTR(pItem, data);
	}
	else
		ok = PPDbSearchError();
	return ok;
}

int GeoTrackCore::PutItem(const PPGeoTrackItem & rItem, int use_ta)
{
	int    ok = 1;
	int    r;
	PPGeoTrackItem founded_item;
    GeoTrackTbl::Rec rec;
    THROW(rItem.Get(rec));
    {
    	PPTransaction tra(use_ta);
    	THROW(tra);
    	THROW(r = Search(rItem.Oid, rItem.Dtm, &founded_item));
    	if(r < 0) {
			THROW_DB(insertRecBuf(&rec));
    	}
		else {
			if(rItem.ExtOid.IsFullyDefined()) {
				if(founded_item.ExtOid.Id == 0) {
					THROW_DB(updateRecBuf(&rec));
				}
			}
			ok = -1;
		}
		THROW(tra.Commit());
    }
	CATCHZOK
	return ok;
}

int GeoTrackCore::PutChunk(const TSVector <PPGeoTrackItem> & rList, int use_ta)
{
	int    ok = -1;
	const  uint _c = rList.getCount();
	if(_c) {
    	PPTransaction tra(use_ta);
    	THROW(tra);
    	for(uint i = 0; i < _c; i++) {
			if(PutItem(rList.at(i), 0) > 0)
				ok = 1;
    	}
    	THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(GeoTracking); GeoTrackingFilt::GeoTrackingFilt() : PPBaseFilt(PPFILT_GEOTRACKING, 0, 0)
{
	SetFlatChunk(offsetof(GeoTrackingFilt, ReserveStart),
		offsetof(GeoTrackingFilt, Reserve)+sizeof(Reserve)-offsetof(GeoTrackingFilt, ReserveStart));
	Init(1, 0);
}

bool GeoTrackingFilt::IsEmpty() const
{
	return (Period.IsZero() && !Oi.Obj && !ExtOi.Obj && !Flags);
}
//
//
//
GeoTrackingTotal::GeoTrackingTotal() : Count(0), ObjCount(0)
{
}

PPViewGeoTracking::PPViewGeoTracking() : PPView(0, &Filt, PPVIEW_GEOTRACKING, 0, REPORT_GEOTRACKING)
{
}

PPViewGeoTracking::~PPViewGeoTracking()
{
}

/*virtual*/PPBaseFilt * PPViewGeoTracking::CreateFilt(const void * extraPtr) const
{
	GeoTrackingFilt * p_filt = new GeoTrackingFilt;
	return p_filt;
}

/*virtual*/int  PPViewGeoTracking::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class GeoTrackFiltDialog : public TDialog {
		DECL_DIALOG_DATA(GeoTrackingFilt);
	public:
		GeoTrackFiltDialog() : TDialog(DLG_GEOTRFILT)
		{
			SetupCalPeriod(CTLCAL_GEOTRFILT_PERIOD, CTL_GEOTRFILT_PERIOD);
			ObjTypeList.addzlist(PPOBJ_STYLOPALM, 0);
			ExtObjTypeList.addzlist(PPOBJ_BILL, PPOBJ_LOCATION, 0);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			SetPeriodInput(this, CTL_GEOTRFILT_PERIOD, &Data.Period);
			SetupObjListCombo(this, CTLSEL_GEOTRFILT_OBJT, Data.Oi.Obj, &ObjTypeList);
			SetupObjListCombo(this, CTLSEL_GEOTRFILT_EXTOBJT, Data.ExtOi.Obj, &ExtObjTypeList);
			SetupObjType();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			GetPeriodInput(this, CTL_GEOTRFILT_PERIOD, &Data.Period);
			Data.Oi.Obj = getCtrlLong(CTLSEL_GEOTRFILT_OBJT);
			Data.Oi.Id = Data.Oi.Obj ? getCtrlLong(CTLSEL_GEOTRFILT_OBJ) : 0;
			Data.ExtOi.Obj = getCtrlLong(CTLSEL_GEOTRFILT_EXTOBJT);
			Data.ExtOi.Id = 0;
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_GEOTRFILT_OBJT)) {
				PPID   preserved_obj_type = Data.Oi.Obj;
				Data.Oi.Obj = getCtrlLong(CTLSEL_GEOTRFILT_OBJT);
				if(Data.Oi.Obj != preserved_obj_type) {
					Data.Oi.Id = 0;
					SetupObjType();
				}
			}
			else
				return;
			clearEvent(event);
		}
		void   SetupObjType()
		{
			if(Data.Oi.Obj == PPOBJ_STYLOPALM) {
				disableCtrl(CTLSEL_GEOTRFILT_OBJ, false);
				SetupPPObjCombo(this, CTLSEL_GEOTRFILT_OBJ, Data.Oi.Obj, Data.Oi.Id, 0, 0);
			}
			else {
				disableCtrl(CTLSEL_GEOTRFILT_OBJ, true);
			}
			disableCtrl(CTLSEL_GEOTRFILT_EXTOBJ, true);
		}
		PPIDArray ObjTypeList;
		PPIDArray ExtObjTypeList;
	};
	if(!Filt.IsA(pBaseFilt))
		return 0;
	GeoTrackingFilt * p_filt = static_cast<GeoTrackingFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(GeoTrackFiltDialog, p_filt);
}

/*virtual*/int PPViewGeoTracking::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	// @v10.3.0 (never used) BExtInsert * p_bei = 0;
	Counter.Init();
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE);
	CATCHZOK
	return ok;
}

int PPViewGeoTracking::InitIteration()
{
	const  LDATE base_date = encodedate(1, 1, 2010);

	int    ok = 1;
	DBQ  * dbq = 0;
	GeoTrackTbl::Key0 k0, k0_;
	MEMSZERO(k0);
	BExtQuery::ZDelete(&P_IterQuery);
	THROW_MEM(P_IterQuery = new BExtQuery(&T, 0));
	P_IterQuery->selectAll();
	if(Filt.Oi.Obj) {
		k0.ObjType = (int16)Filt.Oi.Obj;
		k0.ObjID = Filt.Oi.Id;
		if(Filt.Period.low > base_date) {
			k0.Dts2010 = GeoTrackCore::GetStorageDate(Filt.Period.low);
		}
	}
	dbq = ppcheckfiltid(dbq, T.ObjType, Filt.Oi.Obj);
	dbq = ppcheckfiltid(dbq, T.ObjID, Filt.Oi.Id);
	dbq = ppcheckfiltid(dbq, T.ExtObjType, Filt.ExtOi.Obj);
	dbq = ppcheckfiltid(dbq, T.ExtObjID, Filt.ExtOi.Id);
	if(!Filt.Period.IsZero()) {
		if(Filt.Period.low > base_date) {
            dbq = &(*dbq && T.Dts2010 >= diffdate(Filt.Period.low, base_date));
		}
		if(Filt.Period.upp > base_date) {
			dbq = &(*dbq && T.Dts2010 <= diffdate(Filt.Period.upp, base_date));
		}
	}
	P_IterQuery->where(*dbq);
	k0_ = k0;
	Counter.Init(P_IterQuery->countIterations(0, &k0_, spGe));
	P_IterQuery->initIteration(false, &k0, spGe);
	CATCHZOK
	return ok;
}

int FASTCALL PPViewGeoTracking::NextIteration(GeoTrackingViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery) {
		while(ok < 0 && P_IterQuery->nextIteration() > 0) {
			Counter.Increment();
			T.copyBufTo(pItem);
			ok = 1;
		}
	}
	return ok;
}

int FASTCALL PPViewGeoTracking::CheckRecForFilt(const GeoTrackTbl::Rec * pRec)
{
	return 1;
}

/*virtual*/DBQuery * PPViewGeoTracking::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	int    add_dbe = 0;
	uint   brw_id = BROWSER_GEOTRACKING;
	DBQuery * q = 0;
	DBE    dbe_obj;
	DBE    dbe_extobj;
	DBE    dbe_dt;
	DBQ  * dbq = 0;
	GeoTrackTbl * t = new GeoTrackTbl();
	const  LDATE base_date = encodedate(1, 1, 2010);

	THROW(CheckTblPtr(t));
	{
		dbe_obj.init();
		dbe_obj.push(t->ObjType);
		dbe_obj.push(t->ObjID);
		dbe_obj.push(static_cast<DBFunc>(PPDbqFuncPool::IdOidText));
	}
	{
		dbe_extobj.init();
		dbe_extobj.push(t->ExtObjType);
		dbe_extobj.push(t->ExtObjID);
		dbe_extobj.push(static_cast<DBFunc>(PPDbqFuncPool::IdOidText));
	}
	{
		dbe_dt.init();
		dbe_dt.push(t->Dts2010);
		dbe_dt.push(dbconst(base_date));
		dbe_dt.push(static_cast<DBFunc>(PPDbqFuncPool::IdDateBase));
	}
	dbq = ppcheckfiltid(dbq, t->ObjType, Filt.Oi.Obj);
	dbq = ppcheckfiltid(dbq, t->ObjID, Filt.Oi.Id);
	dbq = ppcheckfiltid(dbq, t->ExtObjType, Filt.ExtOi.Obj);
	dbq = ppcheckfiltid(dbq, t->ExtObjID, Filt.ExtOi.Id);
	if(!Filt.Period.IsZero()) {
		if(Filt.Period.low > base_date) {
            dbq = &(*dbq && t->Dts2010 >= diffdate(Filt.Period.low, base_date));
		}
		if(Filt.Period.upp > base_date) {
			dbq = &(*dbq && t->Dts2010 <= diffdate(Filt.Period.upp, base_date));
		}
	}
	q = & select(
		t->ObjType,   // #0
		t->ObjID,     // #1
		t->Dts2010,   // #2
		t->Tm,        // #3
		dbe_dt,       // #4
		dbe_obj,      // #5
		dbe_extobj,   // #6
		t->Latitude,  // #7
		t->Longitude, // #8
		t->Altitude,  // #9
		t->Speed,     // #10
		0L).from(t, 0L);
	q->where(*dbq);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete t;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewGeoTracking::CalcTotal(GeoTrackingTotal * pTotal)
{
	int    ok = 1;
    GeoTrackingTotal total;
    GeoTrackingViewItem item;
    PPObjIDArray obj_list;
    for(InitIteration(); NextIteration(&item) > 0;) {
		total.Count++;
		PPObjID oi;
		oi.Set(item.ObjType, item.ObjID);
		if(!obj_list.Search(oi, 0)) {
			THROW(obj_list.Add(oi.Obj, oi.Id));
		}
    }
    total.ObjCount = obj_list.getCount();
    CATCHZOK
    ASSIGN_PTR(pTotal, total);
    return ok;
}

/*virtual*/void PPViewGeoTracking::ViewTotal()
{
	GeoTrackingTotal total;
	CalcTotal(&total);
	TDialog * dlg = new TDialog(DLG_GEOTRTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setCtrlLong(CTL_GEOTRTOTAL_COUNT, total.Count);
		dlg->setCtrlLong(CTL_GEOTRTOTAL_OBJCOUNT, total.ObjCount);
		ExecViewAndDestroy(dlg);
	}
}

/*virtual*/int PPViewGeoTracking::Detail(const void *, PPViewBrowser * pBrw)
{
	return -1;
}

int PPViewGeoTracking::Export()
{
	const  int gpx_ver_major = 1;
	const  int gpx_ver_minor = 1;
	const  char * p_schema_url = "http://www.topografix.com/GPX/";
	int    ok = -1;
	SString out_file_name, path;
	xmlTextWriter * p_writer = 0;
	SXml::WNode * p_n_trk = 0;
	SXml::WNode * p_n_trkseg = 0;
	PPWaitStart();
	THROW(InitIteration());
	if(GetCounter().GetTotal()) {
		SString fn_suffix;
        if(Filt.Oi.IsFullyDefined()) {
			GetObjectName(Filt.Oi.Obj, Filt.Oi.Id, fn_suffix);
        }
		out_file_name.Z().Cat("geo-tracking");
		if(fn_suffix.NotEmptyS()) {
			out_file_name.CatChar('-').Cat(fn_suffix.Transf(CTRANSF_INNER_TO_OUTER));
		}
		out_file_name.DotCat("gpx");
		PPGetFilePath(PPPATH_OUT, out_file_name, path);
		THROW(p_writer = xmlNewTextWriterFilename(path, 0));
		{
			//xmlTextWriterSetIndent(writer, 1);
			//xmlTextWriterSetIndentString(writer, (const xmlChar *)" ");
			xmlTextWriterStartDocument(p_writer, 0, "utf-8", "yes");
			{
				SString out_buf, temp_buf;
				SString schema_loc;
				(schema_loc = p_schema_url).Cat(gpx_ver_major).Slash().Cat(gpx_ver_minor);
				SXml::WNode n_gpx(p_writer, "gpx");
					n_gpx.PutAttrib("xmlns", schema_loc);
					temp_buf.Z().Cat(gpx_ver_major).Dot().Cat(gpx_ver_minor);
					n_gpx.PutAttrib("version", temp_buf);
					{
						PPVersionInfo vi = DS.GetVersionInfo();
						SVerT ver = vi.GetVersion();
						//vi.GetProductName(temp_buf);
						vi.GetTextAttrib(vi.taiProductName, temp_buf);
						out_buf.Z().Cat(temp_buf);
						out_buf.Space().Cat(ver.ToStr(temp_buf));
					}
					n_gpx.PutAttrib("creator", out_buf);
					n_gpx.PutAttrib(SXml::nst_xmlns("xsi"), InetUrl::MkHttp("www.w3.org", "2001/XMLSchema-instance"));
					(temp_buf = schema_loc).Space().Cat(p_schema_url).Cat(gpx_ver_major).Slash().Cat(gpx_ver_minor).Slash().Cat("gpx.xsd");
					n_gpx.PutAttrib("xsi:schemaLocation", temp_buf);
				GeoTrackingViewItem item;
				PPObjID last_oid;
				while(NextIteration(&item) > 0) {
					PPObjID oid(item.ObjType, item.ObjID);
					LDATETIME dtm;
					dtm.Set(GeoTrackCore::ConvertStorageDate(item.Dts2010), item.Tm);
					if(!p_n_trk || oid != last_oid) {
						ZDELETE(p_n_trk);
						THROW_MEM(p_n_trk = new SXml::WNode(p_writer, "trk"));
						{
							GetObjectTitle(oid.Obj, out_buf.Z());
							GetObjectName(oid.Obj, oid.Id, temp_buf);
							out_buf.Space().CatBrackStr(temp_buf).Transf(CTRANSF_INNER_TO_UTF8);
							p_n_trk->PutInner("name", out_buf);
						}
						ZDELETE(p_n_trkseg);
						THROW_MEM(p_n_trkseg = new SXml::WNode(p_writer, "trkseg"));
					}
					assert(p_n_trkseg != 0);
                    {
                    	SXml::WNode n_trkpt(p_writer, "trkpt");
							n_trkpt.PutAttrib("lat", out_buf.Z().Cat(item.Latitude, MKSFMTD(0, 12, NMBF_NOTRAILZ)));
							n_trkpt.PutAttrib("lon", out_buf.Z().Cat(item.Longitude, MKSFMTD(0, 12, NMBF_NOTRAILZ)));
							n_trkpt.PutInner("ele", out_buf.Z().Cat((double)item.Altitude, MKSFMTD(0, 12, NMBF_NOTRAILZ)));
							out_buf.Z().Cat(dtm, DATF_ISO8601CENT, 0);
							n_trkpt.PutInner("time", out_buf);
                    }
					last_oid = oid;
					PPWaitPercent(Counter);
				}
				ZDELETE(p_n_trkseg);
				ZDELETE(p_n_trk);
			}
			xmlTextWriterEndDocument(p_writer);
		}
	}
	CATCHZOKPPERR
	xmlFreeTextWriter(p_writer);
	PPWaitStop();
	return ok;
}

/*virtual*/int PPViewGeoTracking::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	int    update = 0;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_EXPORT:
				ok = -1;
				Export();
				break;
		}
	}
	if(ok > 0 && update > 0) {
		CALLPTRMEMB(pBrw, Update());
	}
	return (update > 0) ? ok : ((ok <= 0) ? ok : -1);
}
//
// Implementation of PPALDD_GeoTracking
//
PPALDD_CONSTRUCTOR(GeoTracking)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(GeoTracking) { Destroy(); }

int PPALDD_GeoTracking::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(GeoTracking, rsrv);
	H.FltBeg = p_filt->Period.low;
	H.FltEnd = p_filt->Period.upp;
	H.FltBegTm = p_filt->BegTm;
	H.FltObjType = p_filt->Oi.Obj;
	H.FltObjID = p_filt->Oi.Id;
	H.FltExtObjType = p_filt->ExtOi.Obj;
	H.FltExtObjID = p_filt->ExtOi.Id;
	//double LuLat; // Географическая широта левого верхнего угла
	//double LuLon; // Географическая долгота левого верхнего угла
	//double RbLat; // Географическая широта правого нижнего угла
	//double RbLon; // Географическая долгота правого нижнего угла
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GeoTracking::InitIteration(long iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(GeoTracking);
}

int PPALDD_GeoTracking::NextIteration(long iterId)
{
	START_PPVIEW_ALDD_ITER(GeoTracking);
	I.ObjType = item.ObjType;
	I.ObjID = item.ObjID;
	I.Dt = GeoTrackCore::ConvertStorageDate(item.Dts2010);
	I.Tm = item.Tm;
	I.ExtEvent = item.ExtEvent;
	I.ExtObjType = item.ExtObjType;
	I.ExtObjID = item.ExtObjID;
	I.Flags = item.Flags;
	I.Latitude = item.Latitude;
	I.Longitude = item.Longitude;
	I.Altitude = item.Altitude;
	I.Speed = item.Speed;
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_GeoTracking::Destroy() { DESTROY_PPVIEW_ALDD(GeoTracking); }
//
//
//
#if SLTEST_RUNNING // {

SLTEST_R(PPGeo)
{
	int    ok = 1;
	/*
	{
		PPOsm::Node node_list_solid = {

		};
		//uint FASTCALL PPOsm::NodeCluster::GetPossiblePackCount(const Node * pN, size_t count)
	}
	*/
	//CATCHZOK
	return ok;
}

#endif // } SLTEST_RUNNING

