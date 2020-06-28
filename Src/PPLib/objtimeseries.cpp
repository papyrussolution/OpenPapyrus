// OBJTIMESERIES.CPP
// Copyright (c) A.Sobolev 2018, 2019, 2020
// @codepage UTF-8
// Модуль, управляющий объектом данных PPObjTimeSeries
//
#include <pp.h>
#pragma hdrstop
#include <fann2.h>

static SString & OutputLongArrayValues(const LongArray & rList, const char * pTitle, SString & rBuf)
{
	rBuf.Cat(pTitle).CatChar('=');
	SForEachVectorItem(rList, ii) { rBuf.CatDivConditionally(',', 0, LOGIC(ii)).Cat(rList.get(ii)); }
	return rBuf;
}

const uint16 PPTssModel::Default_OptRangeStep = 50;
const uint16 PPTssModel::Default_OptRangeStepCount = 1; // @v10.7.9 6-->1
const uint16 PPTssModel::Default_OptRangeMultiLimit = 250;
const uint16 PPTssModel::Default_CqaMatchPromille = 1; // @20200408 9-->18 // @20200415 18-->1

SLAPI PPTssModel::PPTssModel()
{
	THISZERO();
}

int FASTCALL PPTssModel::IsEqual(const PPTssModel & rS) const
{
	int    eq = 1;
#define CMPF(f) if(f!=rS.f) eq = 0;
	if(!sstreq(Name, rS.Name))
		eq = 0;
	else if(!sstreq(Symb, rS.Symb))
		eq = 0;
	else CMPF(BestSubsetDimention)
	else CMPF(BestSubsetMaxPhonyIters)
	else CMPF(BestSubsetOptChunk)
	else CMPF(MainFrameRangeCount)
	else CMPF(DefTargetQuant)
	else CMPF(OptRangeStep_)
	//else CMPF(OptRangeStepMkPart)
	else CMPF(OptRangeMultiLimit)
	else CMPF(OptRangeMaxExtProbe)
	else CMPF(OptTargetCriterion)
	else CMPF(OptRangeStepCount) // @v10.7.5
	// @v10.7.10 else CMPF(ChaosFactor)
	else CMPF(UseDataSince)
	else CMPF(InitTrendErrLimit_)
	else CMPF(InitMainTrendErrLimit)
	else CMPF(MinWinRate)
	else CMPF(OverallWinRateLimit)
	else CMPF(Flags)
	else CMPF(OptRangeStep_Measure) // @v10.7.6
	else CMPF(StrategyPoolSortOrder) // @v10.7.7
	else CMPF(CqaMatchPromille) // @v10.7.7
	else CMPF(MinSimilItems) // @v10.7.7
#undef CMPF
	return eq;
}

PPTssModelPacket::Extension::Extension() : AdoptSlShiftDn(0), AdoptSlShiftUp(0), AdoptTpFinishShiftUp(0)
{
	Ver = DS.GetVersion();
	memzero(Reserve, sizeof(Reserve));
}

int FASTCALL PPTssModelPacket::Extension::IsEqual(const Extension & rS) const
{
	int    yes = 1;
	if(AdoptSlShiftDn != rS.AdoptSlShiftDn)
		yes = 0;
	else if(AdoptSlShiftUp != rS.AdoptSlShiftUp)
		yes = 0;
	else if(AdoptTpFinishShiftUp != rS.AdoptTpFinishShiftUp)
		yes = 0;
	return yes;
}

SLAPI PPTssModelPacket::PPTssModelPacket()
{
}

int FASTCALL PPTssModelPacket::IsEqual(const PPTssModelPacket & rS) const
{
	int    eq = 1;
	if(!Rec.IsEqual(rS.Rec))
		eq = 0;
	else if(!MainFrameSizeList.IsEqual(&rS.MainFrameSizeList))
		eq = 0;
	else if(!InputFrameSizeList.IsEqual(&rS.InputFrameSizeList))
		eq = 0;
	else if(!MaxDuckQuantList.IsEqual(&rS.MaxDuckQuantList))
		eq = 0;
	else if(!TargetQuantList.IsEqual(&rS.TargetQuantList))
		eq = 0;
	else if(!StakeBoundList.IsEqual(rS.StakeBoundList))
		eq = 0;
	else if(!E.IsEqual(rS.E))
		eq = 0;
	return eq;
}

int SLAPI PPTssModelPacket::SerializeTail(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	if(dir > 0)
		E.Ver = DS.GetVersion();
	THROW_SL(pSCtx->SerializeBlock(dir, sizeof(E), &E, rBuf, 1));
	THROW_SL(pSCtx->Serialize(dir, &MainFrameSizeList, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &InputFrameSizeList, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &MaxDuckQuantList, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &TargetQuantList, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &StakeBoundList, rBuf));
	CATCHZOK
	return ok;
}

int SLAPI PPTssModelPacket::Output(SString & rBuf) const
{
	int    ok = 1;
	SString temp_buf;
	rBuf.Z();
	//rBuf.Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, 0);
	(temp_buf = Rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
	rBuf.CR().CatChar('#').Cat(Rec.ID).Space().Cat(temp_buf);
	if(Rec.Flags & PPTssModel::fTrendErrLimitAsMedianPart)
		rBuf.CR().CatEq("partitial_trend_err_limit", Rec.InitTrendErrLimit_, MKSFMTD(0, 3, 0)); // @v10.7.4
	else
		rBuf.CR().CatEq("init_trend_err_limit", Rec.InitTrendErrLimit_, MKSFMTD(0, 3, 0));
	rBuf.CR().CatEq("init_main_trend_err_limit", Rec.InitMainTrendErrLimit, MKSFMTD(0, 3, 0)); // @v10.7.0
	rBuf.CR();
	OutputLongArrayValues(InputFrameSizeList, "input_frame_size", rBuf).CR();
	OutputLongArrayValues(MainFrameSizeList, "main_frame_size", rBuf).CR();
	rBuf.CatEq("main_frame_range_count", static_cast<uint>(Rec.MainFrameRangeCount));
	rBuf.CR();
	if(StakeBoundList.getCount()) {
		rBuf.Cat("stake_bound_list").CatChar('=');
		for(uint sblidx = 0; sblidx < StakeBoundList.getCount(); sblidx++) {
			const LAssoc & r_sbl_item = StakeBoundList.at(sblidx);
			if(sblidx)
				rBuf.Space();
			rBuf.CatChar('{').Cat(r_sbl_item.Key).CatDiv(':', 0).Cat(r_sbl_item.Val).CatChar('}');
		}
		rBuf.CR();
	}
	else {
		OutputLongArrayValues(MaxDuckQuantList, "max_duck_quant", rBuf).CR();
		OutputLongArrayValues(TargetQuantList, "target_quant", rBuf).CR();
	}
	rBuf.CatEq("def_target_quant", Rec.DefTargetQuant);
	rBuf.CR();
	{
		rBuf.Cat("opt_range_target").CatChar('=');
		if(Rec.OptTargetCriterion == PPTssModel::tcWinRatio)
			rBuf.Cat("winratio");
		else if(Rec.OptTargetCriterion == PPTssModel::tcVelocity)
			rBuf.Cat("velocity");
		else if(Rec.OptTargetCriterion == PPTssModel::tcAngleRatio) // @v10.7.9
			rBuf.Cat("angleratio");
		else
			rBuf.Cat("amount");
	}
	{
		rBuf.CR().CatEq("opt_range_multi", STextConst::GetBool(Rec.Flags & PPTssModel::fOptRangeMulti));
		rBuf.CR().CatEq("opt_range_multi_limit", Rec.OptRangeMultiLimit);
		{
			rBuf.CR().CatEq("opt_range_step", Rec.OptRangeStep_);
			switch(Rec.OptRangeStep_Measure) {
				case PPTssModel::orsAbsolute: rBuf.Space().Cat("absolute"); break;
				case PPTssModel::orsMkPart:   rBuf.Space().Cat("mk-part"); break;
				case PPTssModel::orsLog:      rBuf.Space().Cat("logarighm-part"); break;
				case PPTssModel::orsRadialPart: rBuf.Space().Cat("radial-part"); break;
				default:
					if(Rec.Flags & PPTssModel::fOptRangeStepAsMkPart_)
						rBuf.Space().Cat("mk-part");
					else
						rBuf.Space().Cat("absolute"); 

			}
			/*if(Rec.Flags & PPTssModel::fOptRangeStepAsMkPart)
				rBuf.CR().CatEq("opt_range_stepmkpart", Rec.OptRangeStep_);
			else
				rBuf.CR().CatEq("opt_range_step", Rec.OptRangeStep_);*/
		}
		rBuf.CR().CatEq("opt_range_step_count", Rec.OptRangeStepCount); // @v10.7.5
		rBuf.CR().CatEq("opt_range_max_ext_probe", Rec.OptRangeMaxExtProbe);
	}
	rBuf.CR().CatEq("min_win_rate", Rec.MinWinRate, MKSFMTD(0, 2, 0));
	rBuf.CR().CatEq("overall_win_rate_limit", Rec.OverallWinRateLimit, MKSFMTD(0, 2, 0)); // @v10.7.0
	rBuf.CR().CatEq("strategy_pool_sort_order", static_cast<uint>(Rec.StrategyPoolSortOrder)); // @v10.7.7
	rBuf.CR().CatEq("cqa_math_promille", static_cast<uint>(Rec.CqaMatchPromille)); // @v10.7.7
	rBuf.CR().CatEq("best_subset_dimension", Rec.BestSubsetDimention);
	rBuf.CR().CatEq("best_subset_trendfollowing", STextConst::GetBool(Rec.Flags & PPTssModel::fBestSubsetTrendFollowing));
	rBuf.CR().CatEq("best_subset_opt_chunk", static_cast<uint>(Rec.BestSubsetOptChunk));
	rBuf.CR().CatEq("best_subset_max_phony_iters", Rec.BestSubsetMaxPhonyIters);
	if(checkdate(Rec.UseDataSince))
		rBuf.CR().CatEq("use_data_since", Rec.UseDataSince, DATF_DMY|DATF_CENTURY);
	/*if(checkdate(Rec.UseDataTill))
		rBuf.CR().CatEq("use_data_till", Rec.UseDataTill, DATF_DMY|DATF_CENTURY);
	if(checkdate(P.UseDataForStrategiesTill))
		rBuf.CR().CatEq("p.use_data_for_strategies_till", P.UseDataForStrategiesTill, DATF_DMY|DATF_CENTURY);*/
	// @v10.7.10 rBuf.CR().CatEq("chaos_factor", static_cast<uint>(Rec.ChaosFactor));
	//rBuf.CR();
	//f_out.WriteLine(msg_buf);
	return ok;
}


SLAPI PPObjTssModel::PPObjTssModel(void * extraPtr) : PPObjReference(PPOBJ_TSSMODEL, extraPtr)
{
}

/*virtual*/int  SLAPI PPObjTssModel::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjTssModel, PPTssModelPacket>(this, p, id, stream, pCtx); }

/*virtual*/int  SLAPI PPObjTssModel::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPTssModelPacket * p_pack = static_cast<PPTssModelPacket *>(p->Data);
		if(stream == 0) {
			if(*pID == 0) {
				PPID   same_id = 0;
				if(ref->SearchSymb(Obj, &same_id, p_pack->Rec.Name, offsetof(PPTssModel, Name)) > 0) {
					PPTssModel same_rec;
					if(Search(same_id, &same_rec) > 0) {
						ASSIGN_PTR(pID, same_id);
					}
					else
						same_id = 0;
				}
				if(same_id == 0) {
					p_pack->Rec.ID = 0;
					THROW(PutPacket(pID, p_pack, 1));
				}
			}
			else {
				p_pack->Rec.ID = *pID;
				THROW(PutPacket(pID, p_pack, 1));
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPObjTssModel::Browse(void * extraPtr)
{
	class ObjTssModelDialog : public ObjViewDialog {
	public:
		ObjTssModelDialog(PPObjTssModel * pObj, void * extraPtr) : ObjViewDialog(DLG_TSSMODELVIEW, pObj, extraPtr)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			ObjViewDialog::handleEvent(event);
			if(event.isCmd(cmAddBySample) && P_Obj) {
				PPID   sample_id = getCurrID();
				if(sample_id) {
					PPID   new_id = 0;
					if(static_cast<PPObjTssModel *>(P_Obj)->AddBySample(&new_id, sample_id) > 0)
						updateList(-1);
				}
			}
			else if(event.isKeyDown(kbCtrlX)) {
				PPID   new_id = 0;
				if(static_cast<PPObjTssModel *>(P_Obj)->ImportFromIni(&new_id) > 0)
					updateList(-1);				
			}
			else
				return;
			clearEvent(event);
		}
	};
	int    ok = cmCancel;
	THROW(CheckRights(PPR_READ));
	{
		ObjTssModelDialog * p_dlg = new ObjTssModelDialog(this, extraPtr);
		THROW(CheckDialogPtr(&p_dlg));
		ok = ExecViewAndDestroy(p_dlg);
	}
	CATCHZOK
	return ok;
}

class TssModelDialog : public TDialog {
	DECL_DIALOG_DATA(PPTssModelPacket);
public:
	TssModelDialog() : TDialog(DLG_TSSMODEL)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SString temp_buf;
		setCtrlData(CTL_TSSMODEL_NAME, Data.Rec.Name);
		setCtrlData(CTL_TSSMODEL_SYMB, Data.Rec.Symb);
		setCtrlData(CTL_TSSMODEL_ID, &Data.Rec.ID);
		setLongListValues(CTL_TSSMODEL_MFSL, Data.MainFrameSizeList);
		setLongListValues(CTL_TSSMODEL_IFSL, Data.InputFrameSizeList);
		setLongListValues(CTL_TSSMODEL_MDQL, Data.MaxDuckQuantList);
		setLongListValues(CTL_TSSMODEL_TQL, Data.TargetQuantList);
		{
			temp_buf.Z();
			for(uint sblidx = 0; sblidx < Data.StakeBoundList.getCount(); sblidx++) {
				const LAssoc & r_sbl_item = Data.StakeBoundList.at(sblidx);
				if(sblidx)
					temp_buf.CatDiv(';', 2);
				temp_buf.Cat(r_sbl_item.Key).CatDiv(',', 0).Cat(r_sbl_item.Val);
			}
			setCtrlString(CTL_TSSMODEL_SBL, temp_buf);
		}
		setCtrlData(CTL_TSSMODEL_MFRC, &Data.Rec.MainFrameRangeCount);
		setCtrlData(CTL_TSSMODEL_MFERRLIM, &Data.Rec.InitMainTrendErrLimit);
		setCtrlData(CTL_TSSMODEL_IFERRLIM, &Data.Rec.InitTrendErrLimit_);
		AddClusterAssoc(CTL_TSSMODEL_TRTIFERRLIM, 0, PPTssModel::fTrendErrLimitAsMedianPart);
		SetClusterData(CTL_TSSMODEL_TRTIFERRLIM, Data.Rec.Flags);
		setCtrlData(CTL_TSSMODEL_OPTRSTEP, &Data.Rec.OptRangeStep_);
		if(Data.Rec.OptRangeStep_Measure == PPTssModel::orsUndef) {
			if(Data.Rec.Flags & PPTssModel::fOptRangeStepAsMkPart_)
				Data.Rec.OptRangeStep_Measure = PPTssModel::orsMkPart;
			else
				Data.Rec.OptRangeStep_Measure = PPTssModel::orsAbsolute;
		}
		AddClusterAssocDef(CTL_TSSMODEL_TRTOPTRSTEP, 0, PPTssModel::orsAbsolute);
		AddClusterAssoc(CTL_TSSMODEL_TRTOPTRSTEP, 1, PPTssModel::orsMkPart);
		AddClusterAssoc(CTL_TSSMODEL_TRTOPTRSTEP, 2, PPTssModel::orsLog);
		AddClusterAssoc(CTL_TSSMODEL_TRTOPTRSTEP, 3, PPTssModel::orsRadialPart);
		SetClusterData(CTL_TSSMODEL_TRTOPTRSTEP, Data.Rec.OptRangeStep_Measure);
		//setCtrlData(CTL_TSSMODEL_OPTRSTEPMP, &Data.Rec.OptRangeStepMkPart);
		setCtrlData(CTL_TSSMODEL_OPTRSTEPCT, &Data.Rec.OptRangeStepCount); // @v10.7.5
		setCtrlData(CTL_TSSMODEL_OPTRMLIM, &Data.Rec.OptRangeMultiLimit);
		setCtrlData(CTL_TSSMODEL_OPTRMEXTP, &Data.Rec.OptRangeMaxExtProbe);
		setCtrlData(CTL_TSSMODEL_BSUBSDIM, &Data.Rec.BestSubsetDimention);
		setCtrlData(CTL_TSSMODEL_BSUBSPHIT, &Data.Rec.BestSubsetMaxPhonyIters);
		setCtrlData(CTL_TSSMODEL_MINWINRATE, &Data.Rec.MinWinRate);
		setCtrlData(CTL_TSSMODEL_OMINWINRATE, &Data.Rec.OverallWinRateLimit);
		setCtrlData(CTL_TSSMODEL_DATASINCE, &Data.Rec.UseDataSince);
		AddClusterAssoc(CTL_TSSMODEL_FLAGS, 0, PPTssModel::fOptRangeMulti);
		AddClusterAssoc(CTL_TSSMODEL_FLAGS, 1, PPTssModel::fBestSubsetTrendFollowing);
		SetClusterData(CTL_TSSMODEL_FLAGS, Data.Rec.Flags);
		AddClusterAssocDef(CTL_TSSMODEL_OPTCRIT, 0, PPTssModel::tcAmount);
		AddClusterAssoc(CTL_TSSMODEL_OPTCRIT, 1, PPTssModel::tcVelocity);
		AddClusterAssoc(CTL_TSSMODEL_OPTCRIT, 2, PPTssModel::tcWinRatio);
		AddClusterAssoc(CTL_TSSMODEL_OPTCRIT, 3, PPTssModel::tcAngleRatio); // @v10.7.9
		SetClusterData(CTL_TSSMODEL_OPTCRIT, Data.Rec.OptTargetCriterion);
		AddClusterAssoc(CTL_TSSMODEL_BSUBSCHNK,    0, 0); // @v10.7.10
		AddClusterAssoc(CTL_TSSMODEL_BSUBSCHNK,    1, 1);
		AddClusterAssocDef(CTL_TSSMODEL_BSUBSCHNK, 2, 3);
		AddClusterAssoc(CTL_TSSMODEL_BSUBSCHNK,    3, 7);
		AddClusterAssoc(CTL_TSSMODEL_BSUBSCHNK,    4, 15);
		SetClusterData(CTL_TSSMODEL_BSUBSCHNK, Data.Rec.BestSubsetOptChunk);
		// @v10.7.7 {
		AddClusterAssocDef(CTL_TSSMODEL_SPPS, 0, PPTssModel::sppsResult);
		AddClusterAssoc(CTL_TSSMODEL_SPPS, 1, PPTssModel::sppsProb);
		AddClusterAssoc(CTL_TSSMODEL_SPPS, 2, PPTssModel::sppsVelocity);
		AddClusterAssoc(CTL_TSSMODEL_SPPS, 3, PPTssModel::sppsCqaFactor);
		AddClusterAssoc(CTL_TSSMODEL_SPPS, 4, PPTssModel::sppsShuffle);
		AddClusterAssoc(CTL_TSSMODEL_SPPS, 5, PPTssModel::sppsStakeCount); // @v10.7.9
		SetClusterData(CTL_TSSMODEL_SPPS, Data.Rec.StrategyPoolSortOrder);
		setCtrlData(CTL_TSSMODEL_CQAMATCH, &Data.Rec.CqaMatchPromille);
		setCtrlData(CTL_TSSMODEL_MINSIMIL, &Data.Rec.MinSimilItems);
		// } @v10.7.7 
		// @v10.7.10 {
		{
			temp_buf.Z();
			if(Data.E.AdoptSlShiftDn || Data.E.AdoptSlShiftUp) {
				temp_buf.Cat(Data.E.AdoptSlShiftDn).CatDiv(',', 2).Cat(Data.E.AdoptSlShiftUp);
				if(Data.E.AdoptTpFinishShiftUp)
					temp_buf.CatDiv(',', 2).Cat(Data.E.AdoptTpFinishShiftUp);
			}
			setCtrlString(CTL_TSSMODEL_ADOPTSLTS, temp_buf);
		}
		// } @v10.7.10 
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		SString temp_buf;
		getCtrlData(CTL_TSSMODEL_NAME, Data.Rec.Name);
		getCtrlData(CTL_TSSMODEL_SYMB, Data.Rec.Symb);
		//setCtrlData(CTL_TSSMODEL_ID, &Data.Rec.ID);
		getLongListValues(CTL_TSSMODEL_MFSL, Data.MainFrameSizeList);
		getLongListValues(CTL_TSSMODEL_IFSL, Data.InputFrameSizeList);
		getLongListValues(CTL_TSSMODEL_MDQL, Data.MaxDuckQuantList);
		getLongListValues(CTL_TSSMODEL_TQL, Data.TargetQuantList);
		{
			LAssocArray temp_sbl;
			getCtrlString(CTL_TSSMODEL_SBL, temp_buf);
			StringSet ss(';', temp_buf);
			SString left_buf, right_buf;
			for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
				if(temp_buf.Divide(',', left_buf, right_buf)) {
					long tp = left_buf.ToLong();
					long sl = right_buf.ToLong();
					if(tp > 0 && sl > 0) {
						temp_sbl.Add(tp, sl);
					}
				}
			}
			Data.StakeBoundList = temp_sbl;
		}
		getCtrlData(CTL_TSSMODEL_MFRC, &Data.Rec.MainFrameRangeCount);
		getCtrlData(CTL_TSSMODEL_MFERRLIM, &Data.Rec.InitMainTrendErrLimit);
		getCtrlData(CTL_TSSMODEL_IFERRLIM, &Data.Rec.InitTrendErrLimit_);
		GetClusterData(CTL_TSSMODEL_TRTIFERRLIM, &Data.Rec.Flags);
		getCtrlData(CTL_TSSMODEL_OPTRSTEP, &Data.Rec.OptRangeStep_);
		//GetClusterData(CTL_TSSMODEL_TRTOPTRSTEP, &Data.Rec.Flags);
		Data.Rec.OptRangeStep_Measure = static_cast<uint8>(GetClusterData(CTL_TSSMODEL_TRTOPTRSTEP));
		//getCtrlData(CTL_TSSMODEL_OPTRSTEPMP, &Data.Rec.OptRangeStepMkPart);
		getCtrlData(CTL_TSSMODEL_OPTRSTEPCT, &Data.Rec.OptRangeStepCount); // @v10.7.5
		getCtrlData(CTL_TSSMODEL_OPTRMLIM, &Data.Rec.OptRangeMultiLimit);
		getCtrlData(CTL_TSSMODEL_OPTRMEXTP, &Data.Rec.OptRangeMaxExtProbe);
		getCtrlData(CTL_TSSMODEL_BSUBSDIM, &Data.Rec.BestSubsetDimention);
		getCtrlData(CTL_TSSMODEL_BSUBSPHIT, &Data.Rec.BestSubsetMaxPhonyIters);
		getCtrlData(CTL_TSSMODEL_MINWINRATE, &Data.Rec.MinWinRate);
		getCtrlData(CTL_TSSMODEL_OMINWINRATE, &Data.Rec.OverallWinRateLimit);
		getCtrlData(CTL_TSSMODEL_DATASINCE, &Data.Rec.UseDataSince);
		GetClusterData(CTL_TSSMODEL_FLAGS, &Data.Rec.Flags);
		GetClusterData(CTL_TSSMODEL_OPTCRIT, &Data.Rec.OptTargetCriterion);
		Data.Rec.BestSubsetOptChunk = static_cast<uint8>(GetClusterData(CTL_TSSMODEL_BSUBSCHNK));
		// @v10.7.7 {
		Data.Rec.StrategyPoolSortOrder = static_cast<uint8>(GetClusterData(CTL_TSSMODEL_SPPS));
		getCtrlData(CTL_TSSMODEL_CQAMATCH, &Data.Rec.CqaMatchPromille);
		getCtrlData(CTL_TSSMODEL_MINSIMIL, &Data.Rec.MinSimilItems);
		// } @v10.7.7 
		// @v10.7.10 {
		{
			getCtrlString(CTL_TSSMODEL_ADOPTSLTS, temp_buf);
			Data.E.AdoptSlShiftDn = 0;
			Data.E.AdoptSlShiftUp = 0;
			Data.E.AdoptTpFinishShiftUp = 0;
			if(temp_buf.NotEmptyS()) {
				StringSet ss;
				temp_buf.Tokenize(",;", ss);
				uint fn = 0;
				for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
					fn++;
					const long n = temp_buf.ToLong();
					if(fn == 1) {
						if(n > 0 && n < 256)
							Data.E.AdoptSlShiftDn = static_cast<uint8>(n);
					}
					else if(fn == 2) {
						if(n > 0 && n < 256)
							Data.E.AdoptSlShiftUp = static_cast<uint8>(n);
					}
					else if(fn == 3) {
						if(n > 0 && n < 256)
							Data.E.AdoptTpFinishShiftUp = static_cast<uint8>(n);
					}
				}
			}
		}
		// } @v10.7.10
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	int    setLongListValues(uint ctlId, const LongArray & rList)
	{
		int    ok = 1;
		SString temp_buf;
		LongArray temp_list = rList;
		temp_list.sortAndUndup();
		SForEachVectorItem(temp_list, i) { temp_buf.CatDivConditionally(',', 0, LOGIC(i)).Cat(temp_list.get(i)); }
		setCtrlString(ctlId, temp_buf);
		return ok;
	}
	int    getLongListValues(uint ctlId, LongArray & rList)
	{
		rList.clear();
		int    ok = 1;
		SString temp_buf;
		getCtrlString(ctlId, temp_buf);
		StringSet ss(',', temp_buf);
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			rList.addnz(inrangeordefault(temp_buf.ToLong(), 1, (1440*180), 0));
		}
		rList.sortAndUndup();
		return ok;
	}
};
	
int SLAPI PPObjTssModel::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel, is_new = 0;
	PPTssModelPacket pack;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	ok = PPDialogProcBody <TssModelDialog, PPTssModelPacket> (&pack);
	THROW(ok);
	if(ok > 0) {
		THROW(PutPacket(pID, &pack, 1));
		ok = cmOK;
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPObjTssModel::ImportFromIni(PPID * pID)
{
	int    ok = -1;
	SString temp_buf;
	TssModelDialog * dlg = 0;
	PrcssrTsStrategyAnalyze::ModelParam model_param;
	if(PrcssrTsStrategyAnalyze::ReadModelParam(model_param)) {
		PPTssModelPacket pack;
		temp_buf = "Model imported from pp.ini";
		STRNSCPY(pack.Rec.Name, temp_buf);
		if(!CheckDupName(0, temp_buf)) {
			for(long i = 1; i < 999; i++) {
				(temp_buf = pack.Rec.Name).Space().CatChar('#').CatLongZ(i, 3);
				if(CheckDupName(0, temp_buf)) {
					temp_buf.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
					break;
				}
			}
		}
		pack.Rec.OptTargetCriterion = static_cast<int16>(model_param.OptTargetCriterion);
		// @v10.7.10 pack.Rec.ChaosFactor = static_cast<int16>(model_param.ChaosFactor);
		pack.Rec.BestSubsetDimention = model_param.BestSubsetDimention;
		pack.Rec.BestSubsetMaxPhonyIters = model_param.BestSubsetMaxPhonyIters;
		pack.Rec.BestSubsetOptChunk = model_param.BestSubsetOptChunk;
		pack.Rec.InitMainTrendErrLimit = model_param.InitMainTrendErrLimit;
		if(model_param.PartitialTrendErrLimit > 0.0) {
			pack.Rec.InitTrendErrLimit_ = model_param.PartitialTrendErrLimit;
			pack.Rec.Flags |= PPTssModel::fTrendErrLimitAsMedianPart;
		}
		else
			pack.Rec.InitTrendErrLimit_ = model_param.InitTrendErrLimit;
		pack.Rec.MainFrameRangeCount = model_param.MainFrameRangeCount;
		pack.Rec.OptRangeMultiLimit = model_param.OptRangeMultiLimit;
		pack.Rec.OptRangeMaxExtProbe = model_param.OptRangeMaxExtProbe;
		if(model_param.OptRangeStepMkPart > 0) {
			pack.Rec.OptRangeStep_ = model_param.OptRangeStepMkPart;
			//pack.Rec.Flags |= PPTssModel::fOptRangeStepAsMkPart;
			pack.Rec.OptRangeStep_Measure = PPTssModel::orsMkPart;
		}
		else {
			pack.Rec.OptRangeStep_ = model_param.OptRangeStep;
			pack.Rec.OptRangeStep_Measure = PPTssModel::orsAbsolute;
		}
		pack.Rec.UseDataSince = model_param.UseDataSince;
		pack.Rec.MinWinRate = model_param.MinWinRate;
		pack.Rec.OverallWinRateLimit = model_param.OverallWinRateLimit;
		pack.Rec.DefTargetQuant = model_param.DefTargetQuant;
		SETFLAG(pack.Rec.Flags, PPTssModel::fBestSubsetTrendFollowing, BIN(model_param.Flags & model_param.fBestSubsetTrendFollowing));
		SETFLAG(pack.Rec.Flags, PPTssModel::fOptRangeMulti, BIN(model_param.Flags & model_param.fOptRangeMulti));
		pack.MainFrameSizeList = model_param.MainFrameSizeList;
		pack.InputFrameSizeList = model_param.InputFrameSizeList;
		pack.MaxDuckQuantList = model_param.MaxDuckQuantList;
		pack.TargetQuantList = model_param.TargetQuantList;
		pack.StakeBoundList = model_param.StakeBoundList;
		//
		if(CheckDialogPtrErr(&(dlg = new TssModelDialog()))) {
			dlg->setDTS(&pack);
			for(int valid_data = 0; !valid_data && (ok = ExecView(dlg)) == cmOK;) {
				if(dlg->getDTS(&pack) > 0) {
					THROW(CheckRightsModByID(pID));
					THROW(PutPacket(pID, &pack, 1));
					valid_data = 1;
				}
				else
					PPError();
			}
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

int SLAPI PPObjTssModel::AddBySample(PPID * pID, PPID sampleID)
{
	int    ok = cmCancel;
	SString temp_buf;
	PPTssModelPacket pack;
	TssModelDialog * dlg = 0;
	THROW(CheckRights(PPR_INS));
	if(GetPacket(sampleID, &pack) > 0) {
		pack.Rec.ID = 0;
		//
		// Подстановка уникального имени
		//
		for(long i = 1; i < 999; i++) {
			(temp_buf = pack.Rec.Name).Space().CatChar('#').CatLongZ(i, 3);
			if(CheckDupName(0, temp_buf)) {
				temp_buf.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
				break;
			}
		}
		if(CheckDialogPtrErr(&(dlg = new TssModelDialog()))) {
			dlg->setDTS(&pack);
			for(int valid_data = 0; !valid_data && (ok = ExecView(dlg)) == cmOK;) {
				if(dlg->getDTS(&pack) > 0) {
					THROW(CheckRightsModByID(pID));
					THROW(PutPacket(pID, &pack, 1));
					valid_data = 1;
				}
				else
					PPError();
			}
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

int SLAPI PPObjTssModel::SerializePacket(int dir, PPTssModelPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW(pPack->SerializeTail(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int SLAPI PPObjTssModel::PutPacket(PPID * pID, PPTssModelPacket * pPack, int use_ta)
{
	int    ok = 1;
	PPID   _id = DEREFPTRORZ(pID);
	const  int is_new = (_id == 0);
	const  int is_removing = BIN(*pID != 0 && pPack == 0);
	PPID   hid = 0;
	PPTssModelPacket org_pack;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(_id) {
			THROW(GetPacket(_id, &org_pack) > 0);
		}
		if(pPack == 0) {
			if(*pID) {
				THROW(CheckRights(PPR_DEL));
				THROW(ref->RemoveItem(Obj, _id, 0));
				THROW(ref->RemoveProperty(Obj, _id, 0, 0));
				//THROW(ref->Ot.PutList(Obj, _id, 0, 0));
				//THROW(RemoveSync(_id));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, hid, 0);
			}
		}
		else {
			SBuffer sbuf;
			SSerializeContext sctx;
			THROW(CheckRightsModByID(pID));
			if(_id) {
				if(pPack->IsEqual(org_pack))
					ok = -1;
				else {
					THROW(ref->UpdateItem(Obj, _id, &pPack->Rec, 1, 0));
					{
						THROW(pPack->SerializeTail(+1, sbuf, &sctx));
						THROW(ref->PutPropSBuffer(Obj, _id, TSSMODELPRP_EXTENSION, sbuf, 0));
					}
					//THROW(ref->Ot.PutList(Obj, _id, &pPack->TagL, 0));
					DS.LogAction(PPACN_OBJUPD, Obj, _id, 0, 0);
				}
			}
			else {
				THROW(ref->AddItem(Obj, &_id, &pPack->Rec, 0));
				pPack->Rec.ID = _id;
				{
					THROW(pPack->SerializeTail(+1, sbuf, &sctx));
					THROW(ref->PutPropSBuffer(Obj, _id, TSSMODELPRP_EXTENSION, sbuf, 0));
				}
				//THROW(ref->Ot.PutList(Obj, _id, &pPack->TagL, 0));
				DS.LogAction(PPACN_OBJADD, Obj, _id, 0, 0);
				ASSIGN_PTR(pID, _id);
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		if(is_new) {
			*pID = 0;
			if(pPack)
				pPack->Rec.ID = 0;
		}
		ok = 0;
	ENDCATCH
	return ok;
}
	
int SLAPI PPObjTssModel::GetPacket(PPID id, PPTssModelPacket * pPack)
{
	int    ok = Search(id, &pPack->Rec);
	if(ok > 0) {
		SBuffer sbuf;
		SSerializeContext sctx;
		THROW(ref->GetPropSBuffer(Obj, id, TSSMODELPRP_EXTENSION, sbuf));
		THROW(pPack->SerializeTail(-1, sbuf, &sctx));
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI PPTimeSeries::PPTimeSeries() : Tag(PPOBJ_TIMESERIES), ID(0), Flags(0), Type(tUnkn), BuyMarg(0.0), SellMarg(0.0),
	SpikeQuant_t(0.0), Prec(0), AvgSpread(0.0), OptMaxDuck(0), OptMaxDuck_S(0), PeakAvgQuant(0), PeakAvgQuant_S(0), TargetQuant(0), TssModelID(0)
{
	PTR32(Name)[0] = 0;
	PTR32(Symb)[0] = 0;
	PTR32(CurrencySymb)[0] = 0;
}

int FASTCALL PPTimeSeries::IsEqual(const PPTimeSeries & rS) const
{
	int    eq = 1;
	if(!sstreq(Name, rS.Name))
		eq = 0;
	else if(!sstreq(Symb, rS.Symb))
		eq = 0;
	else if(Type != rS.Type) // @v10.5.6
		eq = 0;
	else if(Flags != rS.Flags)
		eq = 0;
	else if(BuyMarg != rS.BuyMarg)
		eq = 0;
	else if(SellMarg != rS.SellMarg)
		eq = 0;
	else if(Prec != rS.Prec)
		eq = 0;
	else if(SpikeQuant_t != rS.SpikeQuant_t)
		eq = 0;
	else if(AvgSpread != rS.AvgSpread)
		eq = 0;
	else if(OptMaxDuck != rS.OptMaxDuck)
		eq = 0;
	else if(OptMaxDuck_S != rS.OptMaxDuck_S)
		eq = 0;
	else if(PeakAvgQuant != rS.PeakAvgQuant)
		eq = 0;
	else if(PeakAvgQuant_S != rS.PeakAvgQuant_S)
		eq = 0;
	else if(TargetQuant != rS.TargetQuant) // @v10.4.2
		eq = 0;
	else if(TssModelID != rS.TssModelID) // @v10.7.5
		eq = 0;
	else if(!sstreq(CurrencySymb, rS.CurrencySymb))
		eq = 0;
	return eq;
}

SLAPI PPTimeSeriesPacket::Extension::Extension() : MarginManual(0.0), FixedStakeVolume(0.0), AvgLocalDeviation(0.0), UseDataForStrategiesSince(ZERODATE)
{
}

int SLAPI PPTimeSeriesPacket::Extension::IsEmpty() const
{
	return (MarginManual == 0.0 && FixedStakeVolume == 0.0 && AvgLocalDeviation == 0.0 && !checkdate(UseDataForStrategiesSince));
}

int FASTCALL PPTimeSeriesPacket::Extension::IsEqual(const PPTimeSeriesPacket::Extension & rS) const
{
	int    eq = 1;
	if(MarginManual != rS.MarginManual)
		eq = 0;
	else if(FixedStakeVolume != rS.FixedStakeVolume) // @v10.6.3
		eq = 0;
	else if(AvgLocalDeviation != rS.AvgLocalDeviation) // @v10.7.1
		eq = 0;
	else if(UseDataForStrategiesSince != rS.UseDataForStrategiesSince) // @v10.7.2
		eq = 0;
	return eq;
}

SLAPI PPTimeSeriesPacket::PPTimeSeriesPacket()
{
}

int FASTCALL PPTimeSeriesPacket::IsEqual(const PPTimeSeriesPacket & rS) const
{
	int    eq = 1;
	if(!Rec.IsEqual(rS.Rec))
		eq = 0;
	else if(!E.IsEqual(rS.E))
		eq = 0;
	return eq;
}

double SLAPI PPTimeSeriesPacket::GetMargin(int sell) const
	{ return inrangeordefault(E.MarginManual, 1E-8, 1.0, (sell ? Rec.SellMarg : Rec.BuyMarg)); }
const char * SLAPI PPTimeSeriesPacket::GetSymb() const
	{ return Rec.Symb; }
SLAPI PPObjTimeSeries::Config::Entry::Entry() : TsID(0), Flags(0)
	{ memzero(Reserve, sizeof(Reserve)); }

PPObjTimeSeries::Config::ExtBlock::ExtBlock() : MaxAvgTimeSec(0), TsFlashTimer(0), MinLossQuantForReverse(0), MinAgeSecondsForReverse(0),
	TerminalTimeAdjustment(0), LocalDevPtCount(0), LDMT_Factor(0), MainTrendMaxErrRel(0.0f), TestCount(0), Reserve2(0),
	// @v10.7.10 ChaosFactor(0), 
	Reserve3(0) // @v10.7.10
{
	memzero(Reserve, sizeof(Reserve));
}

SLAPI PPObjTimeSeries::Config::Config() : /*Tag(PPOBJ_CONFIG), ID(PPCFG_MAIN), Prop(PPPRP_TSSTAKECFG),*/Ver(DS.GetVersion()), Flags(0), MaxStakeCount(0),
	AvailableLimitPart(0.0), AvailableLimitAbs(0.0), MinPerDayPotential(0.0)
{
}

int FASTCALL PPObjTimeSeries::Config::IsEqual(const PPObjTimeSeries::Config & rS) const
{
	int    eq = 1;
	if(Ver != rS.Ver)
		eq = 0;
	else if(Flags != rS.Flags)
		eq = 0;
	else if(MaxStakeCount != rS.MaxStakeCount)
		eq = 0;
	else if(AvailableLimitPart != rS.AvailableLimitPart)
		eq = 0;
	else if(AvailableLimitAbs != rS.AvailableLimitAbs)
		eq = 0;
	else if(MinPerDayPotential != rS.MinPerDayPotential)
		eq = 0;
	else if(E.MaxAvgTimeSec != rS.E.MaxAvgTimeSec)
		eq = 0;
	else if(E.TsFlashTimer != rS.E.TsFlashTimer)
		eq = 0;
	else if(E.MinLossQuantForReverse != rS.E.MinLossQuantForReverse) // @v10.4.2
		eq = 0;
	else if(E.MinAgeSecondsForReverse != rS.E.MinAgeSecondsForReverse) // @v10.4.2
		eq = 0;
	else if(E.TerminalTimeAdjustment != rS.E.TerminalTimeAdjustment) // @v10.5.5
		eq = 0;
	else if(E.LocalDevPtCount != rS.E.LocalDevPtCount) // @v10.7.1
		eq = 0;
	else if(E.LDMT_Factor != rS.E.LDMT_Factor) // @v10.7.1
		eq = 0;
	else if(E.MainTrendMaxErrRel != rS.E.MainTrendMaxErrRel) // @v10.7.2
		eq = 0;
	else if(E.TestCount != rS.E.TestCount) // @v10.7.7
		eq = 0;
	else if(!List.IsEqual(rS.List))
		eq = 0;
	return eq;
}

PPObjTimeSeries::Config & SLAPI PPObjTimeSeries::Config::Z()
{
	//Tag = PPOBJ_CONFIG;
	//ID = PPCFG_MAIN;
	//Prop = PPPRP_TSSTAKECFG;
	Ver = DS.GetVersion();
	Flags = 0;
	MaxStakeCount = 0;
	AvailableLimitPart = 0.0;
	AvailableLimitAbs = 0.0;
	MinPerDayPotential = 0.0;
	E.TsFlashTimer = 0; // @v10.4.2
	E.MinLossQuantForReverse  = 0; // @v10.4.2
	E.MinAgeSecondsForReverse = 0; // @v10.4.2
	E.LocalDevPtCount = 0; // @v10.7.1
	E.LDMT_Factor = 0; // @v10.7.1
	E.TestCount = 0; // @v10.7.7
	memzero(E.Reserve, sizeof(E.Reserve));
	List.clear();
	return *this;
}

const PPObjTimeSeries::Config::Entry * FASTCALL PPObjTimeSeries::Config::SearchEntry(PPID tsID) const
{
	uint    idx = 0;
	return List.lsearch(&tsID, &idx, CMPF_LONG) ? &List.at(idx) : 0;
}

int SLAPI PPObjTimeSeries::Config::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	if(dir > 0) {
		Ver = DS.GetVersion();
	}
	THROW_SL(Ver.Serialize(dir, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, Flags, rBuf));
	THROW_SL(pSCtx->Serialize(dir, MaxStakeCount, rBuf));
	THROW_SL(pSCtx->Serialize(dir, AvailableLimitPart, rBuf));
	THROW_SL(pSCtx->Serialize(dir, AvailableLimitAbs, rBuf));
	THROW_SL(pSCtx->Serialize(dir, MinPerDayPotential, rBuf));
	THROW_SL(pSCtx->SerializeBlock(dir, sizeof(E), &E, rBuf, 0));
	THROW_SL(pSCtx->Serialize(dir, &List, rBuf));
	if(dir > 0) {
		/*if(!Ver.IsLt(10, 3, 1)) {
		}*/
	}
	else if(dir < 0) {
		/*if(Ver.IsLt(10, 3, 1)) {
		}
		else {
		}*/
	}
	CATCHZOK
	return ok;
}

class TimSerCfgDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPObjTimeSeries::Config);
public:
	TimSerCfgDialog() : PPListDialog(DLG_TIMSERCFG, CTL_TIMSERCFG_LIST)
	{
		updateList(-1);
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_TIMSERCFG_MAXSTAKE, &Data.MaxStakeCount);
		setCtrlReal(CTL_TIMSERCFG_ALIMPART, Data.AvailableLimitPart);
		setCtrlReal(CTL_TIMSERCFG_ALIMABS, Data.AvailableLimitAbs);
		setCtrlData(CTL_TIMSERCFG_MINPOTENT, &Data.MinPerDayPotential);
		setCtrlData(CTL_TIMSERCFG_RVRMLQ,    &Data.E.MinLossQuantForReverse); // @v10.4.2
		setCtrlData(CTL_TIMSERCFG_RVRMAS,    &Data.E.MinAgeSecondsForReverse); // @v10.4.2
		setCtrlData(CTL_TIMSERCFG_TERMTMADJ, &Data.E.TerminalTimeAdjustment); // @v10.5.5
		setCtrlData(CTL_TIMSERCFG_TSFLSHTMR, &Data.E.TsFlashTimer); // @v10.3.3
		setCtrlData(CTL_TIMSERCFG_LOCDEVPT,  &Data.E.LocalDevPtCount); // @v10.7.1
		setCtrlData(CTL_TIMSERCFG_LDMTFACTOR,  &Data.E.LDMT_Factor); // @v10.7.1
		setCtrlData(CTL_TIMSERCFG_MTMAXERR,   &Data.E.MainTrendMaxErrRel); // @v10.7.2
		setCtrlData(CTL_TIMSERCFG_TESTCOUNT,  &Data.E.TestCount); // @v10.7.7
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 0, PPObjTimeSeries::Config::fTestMode);
		// @v10.8.0 AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 1, PPObjTimeSeries::Config::fUseStakeMode2); // @v10.3.3
		DisableClusterItem(CTL_TIMSERCFG_FLAGS, 1, 1); // @v10.8.0
		// @v10.8.0 AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 2, PPObjTimeSeries::Config::fUseStakeMode3); // @v10.3.3
		DisableClusterItem(CTL_TIMSERCFG_FLAGS, 2, 1); // @v10.8.0
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 3, PPObjTimeSeries::Config::fAllowReverse); // @v10.4.2
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 4, PPObjTimeSeries::Config::fVerifMode); // @v10.4.7
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 5, PPObjTimeSeries::Config::fIgnoreStrangeStakes); // @v10.6.3
		AddClusterAssoc(CTL_TIMSERCFG_FLAGS, 6, PPObjTimeSeries::Config::fLogStakeEvaluation); // @v10.6.8
		SetClusterData(CTL_TIMSERCFG_FLAGS, Data.Flags);
		updateList(-1);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTL_TIMSERCFG_MAXSTAKE, &Data.MaxStakeCount);
		getCtrlData(CTL_TIMSERCFG_ALIMPART, &Data.AvailableLimitPart);
		getCtrlData(CTL_TIMSERCFG_ALIMABS,  &Data.AvailableLimitAbs);
		getCtrlData(CTL_TIMSERCFG_MINPOTENT, &Data.MinPerDayPotential);
		getCtrlData(CTL_TIMSERCFG_RVRMLQ,    &Data.E.MinLossQuantForReverse); // @v10.4.2
		getCtrlData(CTL_TIMSERCFG_RVRMAS,    &Data.E.MinAgeSecondsForReverse); // @v10.4.2
		getCtrlData(CTL_TIMSERCFG_TERMTMADJ, &Data.E.TerminalTimeAdjustment); // @v10.5.5
		getCtrlData(CTL_TIMSERCFG_TSFLSHTMR, &Data.E.TsFlashTimer); // @v10.3.3
		getCtrlData(CTL_TIMSERCFG_LOCDEVPT,  &Data.E.LocalDevPtCount); // @v10.7.1
		getCtrlData(CTL_TIMSERCFG_LDMTFACTOR, &Data.E.LDMT_Factor); // @v10.7.1
		getCtrlData(CTL_TIMSERCFG_MTMAXERR,   &Data.E.MainTrendMaxErrRel); // @v10.7.2
		getCtrlData(CTL_TIMSERCFG_TESTCOUNT,  &Data.E.TestCount); // @v10.7.7
		GetClusterData(CTL_TIMSERCFG_FLAGS, &Data.Flags);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	virtual int setupList()
	{
		int    ok = 1;
		SString temp_buf;
		PPTimeSeries rec;
		StringSet ss(SLBColumnDelim);
		for(uint i = 0; i < Data.List.getCount(); i++) {
			PPObjTimeSeries::Config::Entry & r_entry = Data.List.at(i);
			ss.clear();
			temp_buf.Z();
			if(TsObj.Search(r_entry.TsID, &rec) > 0) {
				temp_buf.Cat(rec.Symb);
			}
			else {
				PTR32(rec.Name)[0] = 0;
				ideqvalstr(r_entry.TsID, temp_buf);
			}
			ss.add(temp_buf);
			//
			temp_buf.Z();
			if(r_entry.Flags & Data.efDisableStake)
				temp_buf.CatChar('D');
			if(r_entry.Flags & Data.efLong)
				temp_buf.CatChar('L');
			if(r_entry.Flags & Data.efShort)
				temp_buf.CatChar('S');
			ss.add(temp_buf);
			ss.add(rec.Name);
			addStringToList(i+1, ss.getBuf());
		}
		return ok;
	}
	virtual int addItem(long * pPos, long * pID)
	{
		int    ok = -1;
		long   pos = 0;
		long   id = 0;
		PPObjTimeSeries::Config::Entry entry;
		if(EditItem(&entry) > 0) {
			for(uint i = 0; i < Data.List.getCount(); i++) {
				PPObjTimeSeries::Config::Entry & r_temp_entry = Data.List.at(i);
				if(r_temp_entry.TsID == entry.TsID) {
					Data.List.at(i) = entry;
					id = i+1;
					pos = i;
					ok = 1;
					break;
				}
			}
			if(!id) {
				Data.List.insert(&entry);
				id = Data.List.getCount();
				pos = id-1;
				ok = 1;
			}
		}
		ASSIGN_PTR(pPos, pos);
		ASSIGN_PTR(pID, id);
		return ok;
	}
	virtual int editItem(long pos, long id)
	{
		int    ok = -1;
		if(pos >= 0 && pos < Data.List.getCountI()) {
			PPObjTimeSeries::Config::Entry entry = Data.List.at(pos);
			if(EditItem(&entry) > 0) {
				Data.List.at(pos) = entry;
				ok = 1;
			}
		}
		return ok;
	}
	virtual int delItem(long pos, long id)
	{
		int    ok = -1;
		if(pos >= 0 && pos < Data.List.getCountI()) {
			Data.List.atFree(pos);
			ok = 1;
		}
		return ok;
	}
	int EditItem(PPObjTimeSeries::Config::Entry * pData)
	{
		int    ok = -1;
		TDialog * dlg = new TDialog(DLG_TIMSERCFGITEM);
		if(CheckDialogPtrErr(&dlg)) {
			SetupPPObjCombo(dlg, CTLSEL_TIMSERCFGITEM_TS, PPOBJ_TIMESERIES, pData->TsID, 0, 0);
			dlg->disableCtrl(CTLSEL_TIMSERCFGITEM_TS, pData->TsID != 0);
			dlg->AddClusterAssoc(CTL_TIMSERCFGITEM_FLAGS, 0, PPObjTimeSeries::Config::efLong);
			dlg->AddClusterAssoc(CTL_TIMSERCFGITEM_FLAGS, 1, PPObjTimeSeries::Config::efShort);
			dlg->AddClusterAssoc(CTL_TIMSERCFGITEM_FLAGS, 2, PPObjTimeSeries::Config::efDisableStake);
			dlg->SetClusterData(CTL_TIMSERCFGITEM_FLAGS, pData->Flags);
			while(ok < 0 && ExecView(dlg) == cmOK) {
				dlg->getCtrlData(CTLSEL_TIMSERCFGITEM_TS, &pData->TsID);
				dlg->GetClusterData(CTL_TIMSERCFGITEM_FLAGS, &pData->Flags);
				if(pData->TsID) {
					ok = 1;
				}
				else {
					PPErrorByDialog(dlg, CTLSEL_TIMSERCFGITEM_TS, PPERR_TIMSERNEEDED);
				}
			}
		}
		delete dlg;
		return ok;
	}
	PPObjTimeSeries TsObj;
};

//static
int SLAPI PPObjTimeSeries::EditConfig(const PPObjTimeSeries::Config * pCfg)
{
	int    ok = -1;
	PPObjTimeSeries::Config config;
	if(!RVALUEPTR(config, pCfg)) {
		THROW(PPObjTimeSeries::ReadConfig(&config));
	}
	ok = PPDialogProcBody <TimSerCfgDialog, PPObjTimeSeries::Config> (&config);
	if(ok > 0 && !pCfg) {
		THROW(PPObjTimeSeries::WriteConfig(&config, 1));
	}
	CATCH
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

//static
int SLAPI PPObjTimeSeries::WriteConfig(PPObjTimeSeries::Config * pCfg, int use_ta)
{
	int    ok = 1;
	int    is_new = 0;
	int    is_upd = 1;
	PPObjTimeSeries::Config ex_cfg;
	if(ReadConfig(&ex_cfg) > 0) {
		if(pCfg && pCfg->IsEqual(ex_cfg))
			is_upd = 0;
	}
	else
		is_new = 1;
	if(is_upd) {
		SBuffer buffer;
		if(pCfg) {
			SSerializeContext sctx;
			THROW(pCfg->Serialize(1, buffer, &sctx));
		}
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(PPRef->PutPropSBuffer(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_TSSTAKECFG, buffer, 0));
		DS.LogAction(is_new ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, PPCFGOBJ_TIMESERIES, 0, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

//static
int SLAPI PPObjTimeSeries::ReadConfig(PPObjTimeSeries::Config * pCfg)
{
	int    ok = -1;
	SBuffer buffer;
	pCfg->Z();
	SSerializeContext sctx;
	if(PPRef->GetPropSBuffer(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_TSSTAKECFG, buffer) > 0) {
		if(pCfg->Serialize(-1, buffer, &sctx))
			ok = 1;
		else {
			pCfg->Z();
			ok = 0;
		}
	}
	return ok;
}

SLAPI PPObjTimeSeries::PPObjTimeSeries(void * extraPtr) : PPObjReference(PPOBJ_TIMESERIES, extraPtr)
{
}

int SLAPI PPObjTimeSeries::EditDialog(PPTimeSeriesPacket * pEntry)
{
	class TimeSeriesDialog : public TDialog {
		DECL_DIALOG_DATA(PPTimeSeriesPacket);
	public:
		TimeSeriesDialog() : TDialog(DLG_TIMSER)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			int    ok = 1;
			SString temp_buf;
			setCtrlLong(CTL_TIMSER_ID, Data.Rec.ID);
			setCtrlString(CTL_TIMSER_NAME, temp_buf = Data.Rec.Name);
			setCtrlString(CTL_TIMSER_SYMB, temp_buf = Data.Rec.Symb);
			AddClusterAssoc(CTL_TIMSER_FLAGS, 0, PPCommObjEntry::fPassive);
			SetClusterData(CTL_TIMSER_FLAGS, Data.Rec.Flags);
			disableCtrl(CTL_TIMSER_ID, (!PPMaster || Data.Rec.ID));
			// @v10.5.6 {
			AddClusterAssocDef(CTL_TIMSER_TYPE, 0, PPTimeSeries::tUnkn);
			AddClusterAssoc(CTL_TIMSER_TYPE, 1, PPTimeSeries::tForex);
			AddClusterAssoc(CTL_TIMSER_TYPE, 2, PPTimeSeries::tStocks);
			SetClusterData(CTL_TIMSER_TYPE, Data.Rec.Type);
			// } @v10.5.6
			setCtrlData(CTL_TIMSER_PREC, &Data.Rec.Prec);
			setCtrlReal(CTL_TIMSER_BUYMARG, Data.Rec.BuyMarg);
			setCtrlReal(CTL_TIMSER_SELLMARG, Data.Rec.SellMarg);
			setCtrlReal(CTL_TIMSER_MANMARG, Data.E.MarginManual); // @v10.5.5
			setCtrlReal(CTL_TIMSER_FXSTAKE, Data.E.FixedStakeVolume); // @v10.6.3
			setCtrlReal(CTL_TIMSER_QUANT, Data.Rec.SpikeQuant_t);
			setCtrlReal(CTL_TIMSER_AVGSPRD, Data.Rec.AvgSpread);
			setCtrlData(CTL_TIMSER_OPTMD, &Data.Rec.OptMaxDuck);
			setCtrlData(CTL_TIMSER_OPTMDS, &Data.Rec.OptMaxDuck_S);
			setCtrlData(CTL_TIMSER_TGTQUANT, &Data.Rec.TargetQuant); // @v10.4.2
			setCtrlData(CTL_TIMSER_SBIDT, &Data.E.UseDataForStrategiesSince); // @v10.7.2
			SetupPPObjCombo(this, CTLSEL_TIMSER_TSSMODEL, PPOBJ_TSSMODEL, Data.Rec.TssModelID, 0); // @v10.7.5
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			PPID   _id = Data.Rec.ID;
			SString temp_buf;
			getCtrlData(CTL_TIMSER_ID, &_id);
			getCtrlString(sel = CTL_TIMSER_NAME, temp_buf);
			THROW(Obj.CheckName(_id, temp_buf, 1));
			STRNSCPY(Data.Rec.Name, temp_buf);
			getCtrlString(sel = CTL_TIMSER_SYMB, temp_buf);
			THROW(Obj.ref->CheckUniqueSymb(Obj.Obj, _id, temp_buf, offsetof(ReferenceTbl::Rec, Symb)));
			STRNSCPY(Data.Rec.Symb, temp_buf);
			GetClusterData(CTL_TIMSER_TYPE,  &Data.Rec.Type); // @v10.5.6
			GetClusterData(CTL_TIMSER_FLAGS, &Data.Rec.Flags);
			getCtrlData(CTL_TIMSER_TGTQUANT, &Data.Rec.TargetQuant); // @v10.4.2
			getCtrlData(sel = CTL_TIMSER_MANMARG, &Data.E.MarginManual); // @v10.5.5
			THROW_PP(Data.E.MarginManual >= 0.0 && Data.E.MarginManual <= 2.0, PPERR_USERINPUT); // @v10.5.5
			getCtrlData(CTL_TIMSER_FXSTAKE, &Data.E.FixedStakeVolume); // @v10.6.3
			getCtrlData(CTL_TIMSER_SBIDT, &Data.E.UseDataForStrategiesSince); // @v10.7.2
			getCtrlData(CTLSEL_TIMSER_TSSMODEL, &Data.Rec.TssModelID); // @v10.7.5
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmTimSerStat)) {
				SString temp_buf;
				SString out_buf;
				if(Data.Rec.ID) {
					STimeSeries ts;
					if(Obj.GetTimeSeries(Data.Rec.ID, ts) > 0) {
						const uint tsc = ts.GetCount();
						if(tsc) {
							PPObjTimeSeries::Config cfg; // @v10.7.1
							Obj.ReadConfig(&cfg); // @v10.7.1
							STimeSeries::Stat stat(0);
							stat.LocalDevPtCount = cfg.E.LocalDevPtCount; // @v10.7.1
							//ts.Sort(); // @debug
							ts.Analyze("close", stat);
							out_buf.CatEq("count", stat.GetCount()).Space().CRB();
							out_buf.CatEq("min", stat.GetMin(), MKSFMTD(0, 5, 0)).Space().CRB();
							out_buf.CatEq("max", stat.GetMax(), MKSFMTD(0, 5, 0)).Space().CRB();
							out_buf.CatEq("avg", stat.GetExp(), MKSFMTD(0, 5, 0)).Space().CRB();
							out_buf.CatEq("delta-avg", stat.DeltaAvg, MKSFMTD(0, 8, 0)).Space().CRB();
							out_buf.CatEq("local-dev-avg", stat.LocalDevAvg, MKSFMTD(0, 8, 0)).Space().CRB(); // @v10.7.1
							temp_buf.Z().Cat((stat.State & stat.stSorted) ? "sorted" : "unsorted").Space().
								Cat((stat.State & stat.stHasTmDup) ? "has-dup" : "no-dup");
							out_buf.Cat(temp_buf).Space().CRB();
							out_buf.CatEq("spread-avg", Data.Rec.AvgSpread, MKSFMTD(0, 1, 0)).Space().CRB();
							out_buf.CatEq("spike-quant", Data.Rec.SpikeQuant_t, MKSFMTD(0, 9, 0)).Space().CRB();
							out_buf.CatEq("buy-margin", Data.Rec.BuyMarg, MKSFMTD(0, 6, 0)).Space().CRB();
							SUniTime utm;
							LDATETIME dtm;
							ts.GetTime(0, &utm);
							utm.Get(dtm);
							out_buf.CatEq("first_time", temp_buf.Z().Cat(dtm, DATF_DMY|DATF_CENTURY, TIMF_HMS)).Space().CRB();
							ts.GetTime(tsc-1, &utm);
							utm.Get(dtm);
							out_buf.CatEq("last_time", temp_buf.Z().Cat(dtm, DATF_DMY|DATF_CENTURY, TIMF_HMS)).Space().CRB();
							if(!(stat.State & stat.stSorted) || (stat.State & stat.stHasTmDup)) {
								int rr = ts.Repair("close");
								if(rr > 0) {
									if(!Obj.SetTimeSeries(Data.Rec.ID, &ts, 1))
										PPError();
								}
							}
						}
						else
							out_buf = "empty series";
					}
					else
						out_buf = "no series";
				}
				else
					out_buf = "no series";
				setCtrlString(CTL_TIMSER_STAT, out_buf);
			}
			else
				return;
			clearEvent(event);
		}
		PPObjTimeSeries Obj;
	};
	int    ok = -1;
	TimeSeriesDialog * p_dlg = 0;
	if(pEntry) {
		SString obj_title;
		THROW(CheckDialogPtr(&(p_dlg = new TimeSeriesDialog())));
		THROW(EditPrereq(&pEntry->Rec.ID, p_dlg, 0));
		p_dlg->setTitle(GetObjectTitle(Obj, obj_title));
		p_dlg->setDTS(pEntry);
		while(ok < 0 && ExecView(p_dlg) == cmOK) {
			if(p_dlg->getDTS(pEntry)) {
				ok = 1;
			}
		}
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

/*virtual*/int SLAPI PPObjTimeSeries::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	int    is_new = 0;
	PPTimeSeriesPacket pack;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	{
		THROW(ok = EditDialog(&pack));
		if(ok > 0) {
			THROW(is_new || CheckRights(PPR_MOD));
			if(*pID)
				*pID = pack.Rec.ID;
			THROW(PutPacket(pID, &pack, 1));
			ok = cmOK;
		}
	}
	CATCHZOKPPERR
	return ok;
}

int  SLAPI PPObjTimeSeries::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options/* = rmv_default*/, void * pExtraParam)
{
	int    r = -1, conf = 1;
	THROW(CheckRights(PPR_DEL));
	if(options & PPObject::user_request) {
		conf = CONFIRM(PPCFM_DELETE);
	}
	if(conf) {
		PPWait(1);
		THROW(PutPacket(&id, 0, BIN(options & PPObject::use_transaction)));
		r = 1;
	}
	CATCH
		r = 0;
		if(options & PPObject::user_request)
			PPError();
	ENDCATCH
	PPWait(0);
	return r;
}

struct TimeSeriesExtention {
	PPID   Tag;               // Const=PPOBJ_TIMESERIES
	PPID   ID;                // @id
	PPID   Prop;              // Const=TIMSERPRP_EXTENSION
	SVerT  Ver;               // Версия системы, создавшей запись
	uint8  Reserve[32];       // @v10.6.3 [52]-->[44] // @v10.7.1 [44]-->[36] // @v10.7.2 [36]-->[32]
	LDATE  UseDataForStrategiesSince; // @v10.7.2
	double AvgLocalDeviation; // @v10.7.1
	double FixedStakeVolume;  // @v10.6.3
	double MarginManual;      //
	long   Reserve1;          //
	long   Reserve2;          //
};

int SLAPI PPObjTimeSeries::PutPacket(PPID * pID, PPTimeSeriesPacket * pPack, int use_ta)
{
	int    ok = 1;
	PPID   _id = DEREFPTRORZ(pID);
	const  int is_new = (_id == 0);
	const  int is_removing = BIN(*pID != 0 && pPack == 0);
	PPID   hid = 0;
	PPTimeSeriesPacket org_pack;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(_id) {
			THROW(GetPacket(_id, &org_pack) > 0);
		}
		if(pPack == 0) {
			if(*pID) {
				TextRefIdent tri(Obj, _id, PPTRPROP_TIMESERIES);
				THROW(CheckRights(PPR_DEL));
				THROW(ref->RemoveItem(Obj, _id, 0));
				THROW(ref->RemoveProperty(Obj, _id, 0, 0));
				//THROW(ref->Ot.PutList(Obj, _id, 0, 0));
				THROW(ref->UtrC.SetTimeSeries(tri, 0, 0));
				//THROW(RemoveSync(_id));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, hid, 0);
			}
		}
		else {
			const TimeSeriesExtention * p_ext = 0;
			TimeSeriesExtention ext_blk;
			if(!pPack->E.IsEmpty()) {
				MEMSZERO(ext_blk);
				ext_blk.Ver = DS.GetVersion();
				ext_blk.MarginManual = pPack->E.MarginManual;
				ext_blk.FixedStakeVolume = pPack->E.FixedStakeVolume; // @v10.6.3
				ext_blk.AvgLocalDeviation = pPack->E.AvgLocalDeviation; // @v10.7.1
				ext_blk.UseDataForStrategiesSince = pPack->E.UseDataForStrategiesSince; // @v10.7.2
				p_ext = &ext_blk;
			}
			THROW(CheckRightsModByID(pID));
			if(_id) {
				if(pPack->IsEqual(org_pack))
					ok = -1;
				else {
					THROW(ref->UpdateItem(Obj, _id, &pPack->Rec, 1, 0)); // @v10.7.4 @fix pPack-->&pPack->Rec
					THROW(ref->PutProp(Obj, _id, TIMSERPRP_EXTENSION, p_ext, sizeof(*p_ext), 0)); // @v10.5.5
					//THROW(ref->Ot.PutList(Obj, _id, &pPack->TagL, 0));
					DS.LogAction(PPACN_OBJUPD, Obj, _id, 0, 0);
				}
			}
			else {
				THROW(ref->AddItem(Obj, &_id, &pPack->Rec, 0)); // @v10.7.4 @fix pPack-->&pPack->Rec
				pPack->Rec.ID = _id;
				THROW(ref->PutProp(Obj, _id, TIMSERPRP_EXTENSION, p_ext, sizeof(*p_ext), 0)); // @v10.5.5
				//THROW(ref->Ot.PutList(Obj, _id, &pPack->TagL, 0));
				DS.LogAction(PPACN_OBJADD, Obj, _id, 0, 0);
				ASSIGN_PTR(pID, _id);
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		if(is_new) {
			*pID = 0;
			if(pPack)
				pPack->Rec.ID = 0;
		}
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPObjTimeSeries::GetPacket(PPID id, PPTimeSeriesPacket * pPack)
{
	int    ok = Search(id, &pPack->Rec);
	if(ok > 0) {
		// @v10.5.5 {
		TimeSeriesExtention ext_blk;
		if(ref->GetProperty(Obj, id, TIMSERPRP_EXTENSION, &ext_blk, sizeof(ext_blk)) > 0) {
			if(ext_blk.MarginManual > 0.0 && ext_blk.MarginManual <= 2.0) {
				pPack->E.MarginManual = ext_blk.MarginManual;
			}
			pPack->E.FixedStakeVolume = ext_blk.FixedStakeVolume; // @v10.6.3
			pPack->E.AvgLocalDeviation = ext_blk.AvgLocalDeviation; // @v10.7.1
			pPack->E.UseDataForStrategiesSince = ext_blk.UseDataForStrategiesSince; // @v10.7.2
		}
		// } @v10.5.5
	}
	return ok;
}

int SLAPI PPObjTimeSeries::SerializePacket(int dir, PPTimeSeriesPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->SerializeBlock(dir, sizeof(pPack->E), &pPack->E, rBuf, 0));
	CATCHZOK
	return ok;
}

/*virtual*/int SLAPI PPObjTimeSeries::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE && _obj == PPOBJ_TSSMODEL) {
		PPTimeSeries rec;
		for(SEnum en = ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
			if(rec.TssModelID == _id) {
				ok = RetRefsExistsErr(Obj, rec.ID);
				break;
			}
		}
	}
	return ok;
}

/*virtual*/int SLAPI PPObjTimeSeries::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	PPTimeSeriesPacket * p_pack = new PPTimeSeriesPacket;
	p->Data = p_pack;
	THROW_MEM(p->Data);
	if(stream == 0) {
		THROW(GetPacket(id, p_pack) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile(static_cast<FILE *>(stream), 0))
		THROW(SerializePacket(-1, p_pack, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SLAPI PPObjTimeSeries::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPTimeSeriesPacket * p_pack = static_cast<PPTimeSeriesPacket *>(p->Data);
		if(stream == 0) {
			if(*pID == 0) {
				PPID   same_id = 0;
				PPTimeSeries same_rec;
				if(ref->SearchSymb(Obj, &same_id, p_pack->Rec.Name, offsetof(PPTimeSeries, Name)) > 0) {
					if(Search(same_id, &same_rec) > 0) {
						ASSIGN_PTR(pID, same_id);
					}
					else
						same_id = 0;
				}
				else if(ref->SearchSymb(Obj, &same_id, p_pack->Rec.Symb, offsetof(PPTimeSeries, Symb)) > 0) {
					if(Search(same_id, &same_rec) > 0) {
						ASSIGN_PTR(pID, same_id);
					}
					else
						same_id = 0;
				}
				if(same_id == 0) {
					p_pack->Rec.ID = 0;
					THROW(PutPacket(pID, p_pack, 1));
				}
			}
			else {
				p_pack->Rec.ID = *pID;
				THROW(PutPacket(pID, p_pack, 1));
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

/*virtual*/int SLAPI PPObjTimeSeries::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPTimeSeriesPacket * p_pack = static_cast<PPTimeSeriesPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_TSSMODEL, &p_pack->Rec.TssModelID, ary, replace));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPObjTimeSeries::SetTimeSeries(PPID id, STimeSeries * pTs, int use_ta)
{
	int    ok = 1;
	PPTimeSeries ts_rec;
	{
		long   stored_ts_count = 0;
		PPUserFuncProfiler ufp(PPUPRF_TIMSERWRITE); // @v10.3.3
		{
			TextRefIdent tri(Obj, id, PPTRPROP_TIMESERIES);
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(Search(id, &ts_rec) > 0);
			THROW(ref->UtrC.SetTimeSeries(tri, pTs, 0));
			stored_ts_count = pTs ? static_cast<long>(pTs->GetCount()) : 0L;
			DS.LogAction(PPACN_TSSERIESUPD, Obj, id, stored_ts_count, 0); // @v10.3.3
			THROW(tra.Commit());
		}
		ufp.SetFactor(0, static_cast<double>(stored_ts_count)); // @v10.3.3
		ufp.Commit(); // @v10.3.3
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTimeSeries::GetTimeSeries(PPID id, STimeSeries & rTs)
{
	TextRefIdent tri(Obj, id, PPTRPROP_TIMESERIES);
	return ref->UtrC.Search(tri, rTs);
}
//
//
//
struct StrategyOptEntry {
	SLAPI  StrategyOptEntry(double factor, double result) : Factor(factor), Result1(result)
	{
	}
	SLAPI  StrategyOptEntry(double factor, double result, double result2) : Factor(factor), Result1(result), Result2(result2)
	{
	}
	double Factor;
	double Result1;
	double Result2;
};

struct TimeSeries_OptEntryList_Graph_Param {
	TimeSeries_OptEntryList_Graph_Param() : Func(funcUndef), GroupingPt(0), SegmentCount(0), SegmentN(0)
	{
		AbsSegment.Z();
	}
	enum {
		funcUndef   = 0,
		funcResult1 = 1,
		funcResult2 = 2
	};
	SString FileName;
	long  Func;
	long  GroupingPt;
	long  SegmentCount; // Количество сегментов, на которые надо разделить все множество для отображение одног из них
	long  SegmentN;     // Номер сегмента для отображения (segm_n/segm_count)
	RealRange AbsSegment; // Абсолютное значение сегмента отображения (value_low..value_upp)
};

int SLAPI TimeSeries_OptEntryList_Graph(TimeSeries_OptEntryList_Graph_Param * pParam)
{
	int    ok = -1;
	TDialog * dlg = 0;
	SString path;
	SString temp_buf;
	SString selected_file_name;
	long    selected_file_id = 0;
	TSVector <StrategyOptEntry> soe_list;
	StrAssocArray file_list;
	TimeSeries_OptEntryList_Graph_Param param_;
	RVALUEPTR(param_, pParam);
	PPGetPath(PPPATH_OUT, path);
	//optentrylist-gbpjpy.c-7200-1680-40-40-short.csv 
	(temp_buf = path).SetLastSlash().Cat("optentrylist-*.csv");
	if(param_.FileName.NotEmpty()) {
		SPathStruc ps(param_.FileName);
		ps.Merge(SPathStruc::fNam|SPathStruc::fExt, selected_file_name);
	}
	SDirEntry sde;
	long fid = 0;
	for(SDirec sd(temp_buf); sd.Next(&sde) > 0;) {
		(temp_buf = path).SetLastSlash().Cat(sde.FileName);
		if(fileExists(temp_buf)) {
			file_list.Add(++fid, sde.FileName, 0);
			if(selected_file_name.IsEqiAscii(sde.FileName))
				selected_file_id = fid;
		}
	}
	if(file_list.getCount()) {
		dlg = new TDialog(DLG_TSOELGRAPH);
		if(CheckDialogPtr(&dlg)) {
			fid = selected_file_id;
			SetupStrAssocCombo(dlg, CTLSEL_TSOELGRAPH_FILE, &file_list, fid, 0, 0, 0);
			dlg->AddClusterAssocDef(CTL_TSOELGRAPH_FUNC, 0, param_.funcResult1);
			dlg->AddClusterAssoc(CTL_TSOELGRAPH_FUNC, 1, param_.funcResult2);
			dlg->SetClusterData(CTL_TSOELGRAPH_FUNC, param_.Func);
			dlg->setCtrlLong(CTL_TSOELGRAPH_GRPPT, param_.GroupingPt);
			temp_buf.Z();
			if(param_.SegmentCount > 0 && param_.SegmentN > 0)
				temp_buf.Cat(param_.SegmentN).CatChar('/').Cat(param_.SegmentCount);
			else if(!param_.AbsSegment.IsZero()) {
				temp_buf.Cat(param_.AbsSegment.low, MKSFMTD(0, 10, NMBF_NOTRAILZ)).Dot().Dot().
					Cat(param_.AbsSegment.upp, MKSFMTD(0, 10, NMBF_NOTRAILZ));
			}
			dlg->setCtrlString(CTL_TSOELGRAPH_SEGM, temp_buf);
			if(ExecView(dlg) == cmOK) {
				dlg->getCtrlData(CTLSEL_TSOELGRAPH_FILE, &fid);
				dlg->GetClusterData(CTL_TSOELGRAPH_FUNC, &param_.Func);
				dlg->getCtrlData(CTL_TSOELGRAPH_GRPPT, &param_.GroupingPt);
				dlg->getCtrlString(CTL_TSOELGRAPH_SEGM, temp_buf);
				param_.SegmentN = 0;
				param_.SegmentCount = 0;
				param_.AbsSegment.Z();
				if(temp_buf.NotEmptyS()) {
					SString left_buf, right_buf;
					if(temp_buf.Divide('/', left_buf, right_buf) > 0) {
						param_.SegmentN = left_buf.ToLong();
						param_.SegmentCount = right_buf.ToLong();
						if(param_.SegmentN <= 0 || param_.SegmentCount <= 0) {
							param_.SegmentN = 0;
							param_.SegmentCount = 0;
						}
					}
					else {
						if(temp_buf.Search("..", 0, 0, 0)) {
							strtorrng(temp_buf, param_.AbsSegment);
						}
					}
				}
				SString file_name;
				if(file_list.GetText(fid, file_name)) {
					(temp_buf = path).SetLastSlash().Cat(file_name);
					if(fileExists(temp_buf)) {
						param_.FileName = file_name;
						ASSIGN_PTR(pParam, param_);
						SString line_buf;
						StringSet ss("\t");
						SFile f_in(temp_buf, SFile::mRead);
						uint line_no = 0;
						const uint grouping = (param_.GroupingPt > 0) ? param_.GroupingPt : 1;

						Generator_GnuPlot plot(0);
						Generator_GnuPlot::PlotParam pparam;
						pparam.Flags |= Generator_GnuPlot::PlotParam::fDots;

						plot.Preamble();
						PPGpPlotItem item(plot.GetDataFileName(), file_name, PPGpPlotItem::sDots);
						item.Style.SetPoint(GetColorRef(SClrBlue), PPGpStyle::ptDot, MAX(static_cast<float>(grouping) / 20.0f, 1.0f));
						item.AddDataIndex(1);
						item.AddDataIndex(2);
						plot.AddPlotItem(item);
						plot.SetGrid();
						plot.Plot(&pparam);
						plot.StartData(1);
						
						soe_list.clear();
						while(f_in.ReadLine(line_buf)) {
							line_no++;
							if(line_no > 1) {
								ss.setBuf(line_buf.Chomp().Strip());
								StrategyOptEntry soe(0.0, 0.0, 0.0);
								uint fld_n = 0;
								for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
									fld_n++;
									switch(fld_n) {
										case 1: soe.Factor = temp_buf.ToReal(); break;
										case 2: soe.Result1 = temp_buf.ToReal(); break;
										case 3: soe.Result2 = temp_buf.ToReal(); break;
									}
								}
								soe_list.insert(&soe);
							}
						}
						{
							uint _first_idx = 0;
							uint _last_idx = soe_list.getCount()-1;
							if(param_.SegmentCount > 0 && param_.SegmentN > 0) {
								const double first_factor = soe_list.at(0).Factor;
								const double last_factor = soe_list.at(soe_list.getCount()-1).Factor;
								const double part = (last_factor - first_factor) / static_cast<double>(param_.SegmentCount);
								const double bound_low = first_factor + (part * static_cast<double>(param_.SegmentN-1));
								const double bound_upp = first_factor + (part * static_cast<double>(param_.SegmentN));
								{
									for(uint soeidx = 0; soeidx < soe_list.getCount(); soeidx++) {
										const StrategyOptEntry & r_soe = soe_list.at(soeidx);
										if(r_soe.Factor >= bound_low) {
											_first_idx = soeidx;
											break;
										}
									}
								}
								{
									for(uint soeidx = _first_idx+1; soeidx < soe_list.getCount(); soeidx++) {
										const StrategyOptEntry & r_soe = soe_list.at(soeidx);
										if(r_soe.Factor > bound_upp) {
											_last_idx = soeidx;
											break;
										}
									}
								}
								plot.SetAxisRange(Generator_GnuPlot::axY, bound_low-(part/2.0), bound_upp+(part/2.0));
							}
							else if(!param_.AbsSegment.IsZero()) {
								{
									for(uint soeidx = 0; soeidx < soe_list.getCount(); soeidx++) {
										const StrategyOptEntry & r_soe = soe_list.at(soeidx);
										if(r_soe.Factor >= param_.AbsSegment.low) {
											_first_idx = soeidx;
											break;
										}
									}
								}
								{
									for(uint soeidx = _first_idx+1; soeidx < soe_list.getCount(); soeidx++) {
										const StrategyOptEntry & r_soe = soe_list.at(soeidx);
										if(r_soe.Factor > param_.AbsSegment.upp) {
											_last_idx = soeidx;
											break;
										}
									}
								}
								{
									double ax_x_lo = soe_list.at(_first_idx).Factor;
									double ax_x_up = soe_list.at(_last_idx).Factor;
									plot.SetAxisRange(Generator_GnuPlot::axY, ax_x_lo, ax_x_up);
								}
							}
							{
								uint grp_count = 0;
								double x = 0.0;
								double y = 0.0;
								for(uint soeidx = _first_idx; soeidx <= _last_idx; soeidx++) {
									const StrategyOptEntry & r_soe = soe_list.at(soeidx);
									if(param_.Func == param_.funcResult1)
										y += r_soe.Result1;
									else if(param_.Func == param_.funcResult2)
										y += r_soe.Result2;
									else
										y += r_soe.Result1;
									grp_count++;
									if(grp_count == grouping) {
										plot.PutData(x);  // #1
										plot.PutData(y);  // #2
										plot.PutEOR();
										grp_count = 0;
										x = r_soe.Factor;
										y = 0.0;
									}
								}
								if(grp_count) {
									plot.PutData(x);  // #1
									plot.PutData(y);  // #2
									plot.PutEOR();
								}
							}
						}
						plot.PutEndOfData();
						ok = plot.Run();
					}
				}
			}
		}
	}
	delete dlg;
	return ok;
}
//
//
//
#if 0 // @construction {

IMPLEMENT_PPFILT_FACTORY(TimSerStrategyContainer); SLAPI TimSerStrategyContainerFilt::TimSerStrategyContainerFilt() : PPBaseFilt(PPFILT_TIMSERSTRATEGYCONTAINER, 0, 0)
{
	SetFlatChunk(offsetof(TimSerStrategyContainerFilt, ReserveStart),
		offsetof(TimSerStrategyContainerFilt, Reserve)-offsetof(TimSerStrategyContainerFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

SLAPI PPViewTimSerStrategyContainer::PPViewTimSerStrategyContainer() : PPView(0, &Filt, PPVIEW_TIMSERSTRATEGYCONTAINER), P_DsList(0)
{
	ImplementFlags |= (implBrowseArray|implOnAddSetupPos|implDontEditNullFilter);
}

SLAPI PPViewTimSerStrategyContainer::~PPViewTimSerStrategyContainer()
{
}

int SLAPI PPViewTimSerStrategyContainer::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	Sc.clear();
	if(Filt.TsID) {
		PPTimeSeriesPacket pack;
		if(TsObj.GetPacket(Filt.TsID, &pack) > 0) {
			PPObjTimeSeries::Strategy s;
			TsObj.GetStrategies(Filt.TsID, PPObjTimeSeries::sstSelection, Sc);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewTimSerStrategyContainer::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	return ok;
}

int SLAPI PPViewTimSerStrategyContainer::InitIteration()
{
	return 0;
}

int FASTCALL PPViewTimSerStrategyContainer::NextIteration(PPObjTimeSeries::Strategy *)
{
	return 0;
}

//static 
int FASTCALL PPViewTimSerStrategyContainer::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int   ok = 0;
	return ok;
}

/*virtual*/SArray * SLAPI PPViewTimSerStrategyContainer::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_result = 0;
	return p_result;
}

/*virtual*/void SLAPI PPViewTimSerStrategyContainer::PreprocessBrowser(PPViewBrowser * pBrw)
{
}

/*virtual*/int  SLAPI PPViewTimSerStrategyContainer::OnExecBrowser(PPViewBrowser *)
{
	int   ok = 0;
	return ok;
}

/*virtual*/int  SLAPI PPViewTimSerStrategyContainer::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int   ok = 0;
	return ok;
}

int SLAPI PPViewTimSerStrategyContainer::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int   ok = 0;
	return ok;
}

int SLAPI PPViewTimSerStrategyContainer::MakeList(PPViewBrowser * pBrw)
{
	int   ok = 0;
	return ok;
}
#endif // } 0 @construction
//
//
//
IMPLEMENT_PPFILT_FACTORY(TimeSeries); SLAPI TimeSeriesFilt::TimeSeriesFilt() : PPBaseFilt(PPFILT_TIMESERIES, 0, 0)
{
	SetFlatChunk(offsetof(TimeSeriesFilt, ReserveStart),
		offsetof(TimeSeriesFilt, Reserve)-offsetof(TimeSeriesFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

SLAPI PPViewTimeSeries::PPViewTimeSeries() : PPView(&Obj, &Filt, PPVIEW_TIMESERIES), P_DsList(0), P_OelgParam(0)
{
	ImplementFlags |= (implBrowseArray|implOnAddSetupPos|implDontEditNullFilter);
}

SLAPI PPViewTimeSeries::~PPViewTimeSeries()
{
	ZDELETE(P_DsList);
	ZDELETE(P_OelgParam);
}

int SLAPI PPViewTimeSeries::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Obj.ReadConfig(&Cfg);
	Counter.Init();
	CATCHZOK
	return ok;
}

int SLAPI PPViewTimeSeries::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	return -1;
}

int SLAPI PPViewTimeSeries::InitIteration()
{
	int    ok = 1;
	if(!P_DsList) {
		THROW(MakeList(0));
	}
	CALLPTRMEMB(P_DsList, setPointer(0));
	CATCHZOK
	return ok;
}

int FASTCALL PPViewTimeSeries::NextIteration(TimeSeriesViewItem * pItem)
{
	int    ok = -1;
	while(ok < 0 && P_DsList && P_DsList->getPointer() < P_DsList->getCount()) {
		const BrwItem * p_item = static_cast<const BrwItem *>(P_DsList->at(P_DsList->getPointer()));
		if(p_item && Obj.Search(p_item->ID, pItem) > 0)
			ok = 1;
		P_DsList->incPointer();
	}
	return ok;
}

//static
int FASTCALL PPViewTimeSeries::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewTimeSeries * p_v = static_cast<PPViewTimeSeries *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

PPViewTimeSeries::BrwItem::BrwItem(const PPTimeSeriesPacket * pTs)
{
	if(pTs) {
		ID = pTs->Rec.ID;
		STRNSCPY(Name, pTs->Rec.Name);
		STRNSCPY(Symb, pTs->Rec.Symb);
		STRNSCPY(CurrencySymb, pTs->Rec.CurrencySymb);
		BuyMarg = pTs->Rec.BuyMarg;
		SellMarg = pTs->Rec.SellMarg;
		ManualMarg = pTs->E.MarginManual; // @v10.5.6
		Type = static_cast<int16>(pTs->Rec.Type); // @v10.5.6
		Prec = pTs->Rec.Prec;
		SpikeQuant_t = pTs->Rec.SpikeQuant_t;
		AvgSpread = pTs->Rec.AvgSpread;
		OptMaxDuck = pTs->Rec.OptMaxDuck;
		OptMaxDuck_S = pTs->Rec.OptMaxDuck_S;
		PeakAvgQuant = pTs->Rec.PeakAvgQuant;
		PeakAvgQuant_S = pTs->Rec.PeakAvgQuant_S;
		TargetQuant = pTs->Rec.TargetQuant;
		Flags = pTs->Rec.Flags;
		CfgFlags = 0;
	}
	else {
		THISZERO();
	}
}

int SLAPI PPViewTimeSeries::CmpSortIndexItems(PPViewBrowser * pBrw, const PPViewTimeSeries::BrwItem * pItem1, const PPViewTimeSeries::BrwItem * pItem2)
{
	int    sn = 0;
	AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
	if(p_def) {
		for(uint i = 0; !sn && i < pBrw->GetSettledOrderList().getCount(); i++) {
			int    col = pBrw->GetSettledOrderList().get(i);
			TYPEID typ1 = 0;
			TYPEID typ2 = 0;
			uint8  dest_data1[512];
			uint8  dest_data2[512];
			if(p_def->GetCellData(pItem1, labs(col)-1, &typ1, &dest_data1, sizeof(dest_data1)) && p_def->GetCellData(pItem2, labs(col)-1, &typ2, &dest_data2, sizeof(dest_data2))) {
				assert(typ1 == typ2);
				if(typ1 == typ2) {
					sn = stcomp(typ1, dest_data1, dest_data2);
					if(sn && col < 0)
						sn = -sn;
				}
			}
		}
	}
	return sn;
}

static IMPL_CMPFUNC(PPViewTimeSeriesBrwItem, i1, i2)
{
	int    si = 0;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(pExtraData);
	if(p_brw) {
		PPViewTimeSeries * p_view = static_cast<PPViewTimeSeries *>(p_brw->P_View);
		if(p_view) {
			const PPViewTimeSeries::BrwItem * p_item1 = static_cast<const PPViewTimeSeries::BrwItem *>(i1);
			const PPViewTimeSeries::BrwItem * p_item2 = static_cast<const PPViewTimeSeries::BrwItem *>(i2);
			si = p_view->CmpSortIndexItems(p_brw, p_item1, p_item2);
		}
	}
	return si;
}

int SLAPI PPViewTimeSeries::MakeList(PPViewBrowser * pBrw)
{
	int    ok = 1;
	PPTimeSeries item;
	const  int is_sorting_needed = BIN(pBrw && pBrw->GetSettledOrderList().getCount()); // @v10.6.4
	if(P_DsList)
		P_DsList->clear();
	else
		P_DsList = new SArray(sizeof(BrwItem));
	for(SEnum en = Obj.Enum(0); en.Next(&item) > 0;) {
		PPTimeSeriesPacket pack;
		if(Obj.GetPacket(item.ID, &pack) > 0) {
			BrwItem new_item(&pack);
			const PPObjTimeSeries::Config::Entry * p_ce = Cfg.SearchEntry(item.ID);
			if(p_ce)
				new_item.CfgFlags = p_ce->Flags;
			THROW_SL(P_DsList->insert(&new_item));
		}
	}
	// @v10.6.4 {
	if(pBrw) {
		BrowserDef * p_def = pBrw->getDef();
		for(uint cidx = 0; cidx < p_def->getCount(); cidx++) {
			p_def->at(cidx).Options |= BCO_SORTABLE;
		}
		if(is_sorting_needed)
			P_DsList->sort(PTR_CMPFUNC(PPViewTimeSeriesBrwItem), pBrw);
	}
	// } @v10.6.4 
	CATCHZOK
	return ok;
}

int SLAPI PPViewTimeSeries::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		const  BrwItem * p_item = static_cast<const BrwItem *>(pBlk->P_SrcData);
		int    r = 0;
		switch(pBlk->ColumnN) {
			case 0: pBlk->Set(p_item->ID); break; // @id
			case 1: pBlk->Set(p_item->Symb); break; // @symb
			case 2: pBlk->Set(p_item->Name); break; // @name
			case 3: pBlk->Set(p_item->CurrencySymb); break;
			case 4: pBlk->Set(static_cast<long>(p_item->Prec)); break;
			case 5: pBlk->Set(p_item->BuyMarg); break;
			case 6: pBlk->Set(p_item->SellMarg); break;
			case 7: pBlk->Set(p_item->SpikeQuant_t); break;
			case 8: pBlk->Set(p_item->AvgSpread); break;
			case 9: pBlk->Set(p_item->ManualMarg); break;
		}
	}
	return ok;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewTimeSeries * p_view = static_cast<PPViewTimeSeries *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int SLAPI PPViewTimeSeries::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pCellStyle && col >= 0) {
		const BrowserDef * p_def = pBrw->getDef();
		if(col < static_cast<long>(p_def->getCount())) {
			const BroColumn & r_col = p_def->at(col);
			if(col == 0) { // id
				const long cfg_flags = static_cast<const BrwItem *>(pData)->CfgFlags;
				if(cfg_flags & PPObjTimeSeries::Config::efDisableStake)
					ok = pCellStyle->SetLeftTopCornerColor(GetColorRef(SClrGrey));
				if(cfg_flags & PPObjTimeSeries::Config::efLong && cfg_flags & PPObjTimeSeries::Config::efShort)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrOrange));
				else if(cfg_flags & PPObjTimeSeries::Config::efLong)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrGreen));
				else if(cfg_flags & PPObjTimeSeries::Config::efShort)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrRed));
			}
			else if(col == 2) { // name
				const int16 type = static_cast<const BrwItem *>(pData)->Type;
				if(type == PPTimeSeries::tForex)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrPink));
				else if(type == PPTimeSeries::tStocks)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrLightblue));
			}
		}
	}
	return ok;
}

void SLAPI PPViewTimeSeries::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewTimeSeries::GetDataForBrowser, this);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
		// @v10.6.4 {
		BrowserDef * p_def = pBrw->getDef();
		if(p_def) {
			for(uint cidx = 0; cidx < p_def->getCount(); cidx++) {
				p_def->at(cidx).Options |= BCO_SORTABLE;
			}
		}
		// } @v10.6.4 
	}
}

SArray * SLAPI PPViewTimeSeries::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_TIMESERIES;
	SArray * p_array = 0;
	PPTimeSeries ds_item;
	THROW(MakeList(0));
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int SLAPI PPViewTimeSeries::OnExecBrowser(PPViewBrowser *)
{
	return -1;
}

int SLAPI PPViewTimeSeries::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	if(id) {
		TimSerDetailFilt filt;
		filt.TsID = id;
		PPView::Execute(PPVIEW_TIMSERDETAIL, &filt, /*PPView::exefModeless*/0, 0);
	}
	return ok;
}

enum {
	removetimeseriesStrategies = 0x0001,
	removetimeseriesTimSer     = 0x0002,
	removetimeseriesCompletely = 0x0004,
};

static int CallRemoveTimeSeriesDialog(const char * pWarnTextSign, long * pData)
{
	class RemoveTimeSeriesDialog : public TDialog {
		DECL_DIALOG_DATA(long);
	public:
		RemoveTimeSeriesDialog(const char * pWarnTextSign) : TDialog(DLG_RMVTIMSER), Data(0)
		{
			setStaticText(CTL_RMVTIMSER_WARN, PPLoadStringS(pWarnTextSign, SLS.AcquireRvlStr()));
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			AddClusterAssoc(CTL_RMVTIMSER_WHAT, 0, removetimeseriesStrategies);
			AddClusterAssoc(CTL_RMVTIMSER_WHAT, 1, removetimeseriesTimSer);
			AddClusterAssoc(CTL_RMVTIMSER_WHAT, 2, removetimeseriesCompletely);
			SetClusterData(CTL_RMVTIMSER_WHAT, Data);
			DisableClusterItem(CTL_RMVTIMSER_WHAT, 0, Data & removetimeseriesCompletely);
			DisableClusterItem(CTL_RMVTIMSER_WHAT, 1, Data & removetimeseriesCompletely);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			GetClusterData(CTL_RMVTIMSER_WHAT, &Data);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_RMVTIMSER_WHAT)) {
				const long preserve_data = Data;
				GetClusterData(CTL_RMVTIMSER_WHAT, &Data);
				if(preserve_data != Data) {
					DisableClusterItem(CTL_RMVTIMSER_WHAT, 0, Data & removetimeseriesCompletely);
					DisableClusterItem(CTL_RMVTIMSER_WHAT, 1, Data & removetimeseriesCompletely);
					if(Data & removetimeseriesCompletely) {
						Data |= (removetimeseriesStrategies|removetimeseriesTimSer);
					}
				}
				clearEvent(event);
			}
		}
	};
	DIALOG_PROC_BODY_P1(RemoveTimeSeriesDialog, pWarnTextSign, pData);
}

int SLAPI PPViewTimeSeries::DeleteItem(PPID id)
{
	int    ok = -1;
	long   flags = 0;
	PPTimeSeries rec;
	if(id && Obj.Search(id, &rec) > 0) {
		if(CallRemoveTimeSeriesDialog(0, &flags) > 0) {
			PPTransaction tra(1);
			THROW(tra);
			if(flags & removetimeseriesCompletely) {
				THROW(Obj.PutPacket(&id, 0, 0));
			}
			else {
				if(flags & removetimeseriesStrategies) {
					THROW(Obj.PutStrategies(id, PPObjTimeSeries::sstAll, 0, 0));
					THROW(Obj.PutStrategies(id, PPObjTimeSeries::sstSelection, 0, 0));
				}
				if(flags & removetimeseriesTimSer) {
					THROW(Obj.SetTimeSeries(id, 0, 0));
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewTimeSeries::DeleteAll()
{
	int    ok = -1;
	long   flags = 0;
	PPTimeSeries rec;
	if(CallRemoveTimeSeriesDialog("removetimeseries_warn", &flags) > 0) {
		TimeSeriesViewItem item;
		PPTransaction tra(1);
		THROW(tra);
		for(InitIteration(); NextIteration(&item) > 0;) {
			if(flags & removetimeseriesCompletely) {
				THROW(Obj.PutPacket(&item.ID, 0, 0));
			}
			else {
				if(flags & removetimeseriesStrategies) {
					THROW(Obj.PutStrategies(item.ID, PPObjTimeSeries::sstAll, 0, 0));
					THROW(Obj.PutStrategies(item.ID, PPObjTimeSeries::sstSelection, 0, 0));
				}
				if(flags & removetimeseriesTimSer) {
					THROW(Obj.SetTimeSeries(item.ID, 0, 0));
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewTimeSeries::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -2;
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	const  PPID preserve_id = id;
	if(ppvCmd == PPVCMD_DELETEITEM) { // Перехват обработки команды до PPView::ProcessCommand
		ok = DeleteItem(id);
	}
	else {
		ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	}
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_USERSORT:
				ok = 1; // The rest will be done below
				break;
			case PPVCMD_DELETEALL:
				ok = DeleteAll();
				break;
			case PPVCMD_CONFIG:
				ok = -1;
				if(PPObjTimeSeries::EditConfig(0) > 0) {
					PPObjTimeSeries::ReadConfig(&Cfg);
					ok = 1;
				}
				break;
			case PPVCMD_EXPORT:
				ok = -1;
				Obj.Export(id);
				break;
			//case PPVCMD_TRYSTRATEGIES:
			case PPVCMD_TEST:
				ok = -1;
				if(id) {
					PPObjTimeSeries::TryStrategies(id);
				}
				break;
			case PPVCMD_OPTENTRYGRAPH:
				ok = -1;
				{
					SETIFZ(P_OelgParam, new TimeSeries_OptEntryList_Graph_Param);
					TimeSeries_OptEntryList_Graph(P_OelgParam);
				}
				break;
			/* @v10.7.0 case PPVCMD_EVALOPTMAXDUCK:
				ok = -1;
				if(id) {
					if(PPObjTimeSeries::EvaluateOptimalMaxDuck(id) > 0)
						ok = 1;
				}
				break;*/
			case PPVCMD_REFRESH:
				ok = 1;
				break;
		}
	}
	if(ok > 0) {
		MakeList(pBrw);
		if(pBrw) {
			AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
			if(p_def) {
				SArray * p_array = new SArray(*P_DsList);
				p_def->setArray(p_array, 0, 1);
				pBrw->setRange(p_array->getCount());
				uint   temp_pos = 0;
				long   update_pos = -1;
				if(preserve_id > 0 && P_DsList->lsearch(&preserve_id, &temp_pos, CMPF_LONG))
					update_pos = temp_pos;
				if(update_pos >= 0)
					pBrw->go(update_pos);
				else if(update_pos == MAXLONG)
					pBrw->go(p_array->getCount()-1);
			}
			pBrw->Update();
		}
	}
	return ok;
}

int SLAPI PPObjTimeSeries::Export(PPID id)
{
	int   ok = 1;
	PPTimeSeries ts_rec;
	if(Search(id, &ts_rec) > 0) {
		SString temp_buf;
		SString file_name;
		SString line_buf;
		temp_buf.Z().Cat("ts-raw").CatChar('-').Cat(ts_rec.Symb).Dot().Cat("txt").ToLower();
		PPGetFilePath(PPPATH_OUT, temp_buf, file_name);
		SFile f_out(file_name, SFile::mWrite);
		if(f_out.IsValid()) {
			STimeSeries ts;
			StrategyContainer scontainer;
			if(GetTimeSeries(id, ts) > 0) {
				const uint tsc = ts.GetCount();
				uint vec_idx = 0;
				if(ts.GetValueVecIndex("close", &vec_idx)) {
					LDATETIME t_prev = ZERODATETIME;
					for(uint j = 0; j < tsc; j++) {
						LDATETIME t = ZERODATETIME;
						double v = 0;
						SUniTime ut;
						ts.GetTime(j, &ut);
						ut.Get(t);
						ts.GetValue(j, vec_idx, &v);
						long td = j ? diffdatetimesec(t, t_prev) : 0;
						line_buf.Z().Cat(t, DATF_ISO8601|DATF_CENTURY, 0).Tab().Cat(v, MKSFMTD(10, 5, 0)).Tab().Cat(td);
						f_out.WriteLine(line_buf.CR());
						t_prev = t;
					}
				}
				const PPObjTimeSeries::StrategySetType sst_list[] = { PPObjTimeSeries::sstAll, PPObjTimeSeries::sstSelection };
				for(uint sst_idx = 0; sst_idx < SIZEOFARRAY(sst_list); sst_idx++) {
					if(GetStrategies(id, sst_list[sst_idx], scontainer) > 0) {
						PPObjTimeSeries::GetStrategySetTypeName(sst_list[sst_idx], line_buf);
						f_out.WriteLine(line_buf.CR());
						for(uint i = 0; i < scontainer.getCount(); i++) {
							const Strategy & r_s = scontainer.at(i);
							line_buf.Z().
								CatEq("ID", r_s.ID).Space().
								CatEq("InputFrameSize", r_s.InputFrameSize).Space().
								CatEq("Prec", static_cast<long>(r_s.Prec)).Space().
								//CatEq("TargetQuant", r_s.TargetQuant).Space().
								CatEq("MaxDuckQuant", r_s.MaxDuckQuant).Space().
								CatEq("PeakAvgQuant", static_cast<long>(r_s.PeakAvgQuant)).Space().
								CatEq("PeakMaxQuant", static_cast<long>(r_s.PeakMaxQuant)).Space().
								CatEq("BottomAvgQuant", static_cast<long>(r_s.BottomAvgQuant)).Space().
								CatEq("StakeMode", static_cast<long>(r_s.StakeMode)).Space().
								CatEq("BaseFlags", r_s.BaseFlags).Space().
								CatEq("Margin", r_s.Margin).Space().
								CatEq("SpikeQuant", r_s.SpikeQuant_s).Space().
								CatEq("SpreadAvg", r_s.SpreadAvg).Space().
								CatEq("TrendErrAvg", r_s.TrendErrAvg, MKSFMTD(0, 6, 0)).Space(). // @v10.3.12
								CatEq("TrendErrLim", r_s.TrendErrLim, MKSFMTD(0, 1, 0)).Space() // @v10.3.12
								// @v10.3.12 CatEq("StakeThreshold", r_s.StakeThreshold).Space()
								;
							temp_buf.Z().Cat(r_s.OptDeltaRange.low, MKSFMTD(0, 9, 0)).Dot().Dot().Cat(r_s.OptDeltaRange.upp, MKSFMTD(0, 9, 0));
							line_buf.CatEq("OptDeltaRange", temp_buf).Space().
								CatEq("OptDeltaCount", r_s.OptDeltaRange.Count).Space();
							line_buf.CatEq("OptDelta2Stride", r_s.OptDelta2Stride).Space();
							temp_buf.Z().Cat(r_s.OptDelta2Range.low, MKSFMTD(0, 9, 0)).Dot().Dot().Cat(r_s.OptDelta2Range.upp, MKSFMTD(0, 9, 0));
							line_buf.CatEq("OptDelta2Range", temp_buf).Space().
								CatEq("OptDelta2Count", r_s.OptDelta2Range.Count).Space().
								CatEq("StakeCloseMode", r_s.StakeCloseMode).Space().
								CatEq("StakeCount", r_s.StakeCount).Space().
								CatEq("V.Result", r_s.V.Result, MKSFMTD(0, 8, 0)).Space().
								CatEq("V.TmCount", r_s.V.TmCount).Space().
								CatEq("V.TmSec", r_s.V.TmSec);
							f_out.WriteLine(line_buf.CR());
						}
					}
				}
			}
		}
	}
	return ok;
}

int SLAPI PPObjTimeSeries::Browse(void * extraPtr)
{
	return PPView::Execute(PPVIEW_TIMESERIES, 0, 1, 0);
}

int SLAPI PPObjTimeSeries::Test() // @experimental
{
	int    ok = 1;
	SString temp_buf;
	SString src_file_name;
	SString test_file_name;
	SLS.QueryPath("testroot", src_file_name);
	src_file_name.SetLastSlash().Cat("data").SetLastSlash().Cat("ts").SetLastSlash().Cat("test-symb-export-eurusd.csv");
	SFile f_in(src_file_name, SFile::mRead);
	if(f_in.IsValid()) {
		test_file_name = src_file_name;
		SPathStruc::ReplaceExt(test_file_name, "out", 1);
		SString line_buf;
		StringSet ss_in(",");
		STimeSeries ts;
		LDATETIME dtm;
		double open = 0.0;
		double close = 0.0;
		long   tick_vol = 0;
		long   real_vol = 0;
		long   spread = 0;
		//uint   vecidx_open = 0;
		uint   vecidx_close = 0;
		//uint   vecidx_ticvol = 0;
		uint   vecidx_realvol = 0;
		//uint   vecidx_spread = 0;
		//THROW_SL(ts.AddValueVec("open", T_DOUBLE, 0, &vecidx_open));
		//THROW_SL(ts.AddValueVec("open", T_INT32, 5, &vecidx_open));
		//THROW_SL(ts.AddValueVec("close", T_DOUBLE, 0, &vecidx_close));
		THROW_SL(ts.AddValueVec("close", T_INT32, 5, &vecidx_close));
		//THROW_SL(ts.AddValueVec("tick_volume", T_INT32, 0, &vecidx_ticvol));
		THROW_SL(ts.AddValueVec("real_volume", T_INT32, 0, &vecidx_realvol));
		//THROW_SL(ts.AddValueVec("spread", T_INT32, 0, &vecidx_spread));
		{
			uint8 sign[8];
			size_t actual_size = 0;
			if(f_in.Read(sign, 4, &actual_size) && actual_size == 4) {
				if(sign[0] == 0xEF && sign[1] == 0xBB && sign[2] == 0xBF)
					f_in.Seek(3);
				else
					f_in.Seek(0);
			}
		}
		while(f_in.ReadLine(line_buf)) {
			line_buf.Chomp().Strip();
			if(line_buf.NotEmpty()) {
				ss_in.setBuf(line_buf);
				dtm.Z();
				open = 0.0;
				close = 0.0;
				tick_vol = 0;
				real_vol = 0;
				spread = 0;
				for(uint ssp = 0, fldn = 0; ss_in.get(&ssp, temp_buf); fldn++) {
					switch(fldn) {
						case 0: strtodate(temp_buf, DATF_YMD, &dtm.d); break;
						case 1: strtotime(temp_buf, TIMF_HMS, &dtm.t); break;
						case 2: open = temp_buf.ToReal(); break;
						case 3: close = temp_buf.ToReal(); break;
						case 4: tick_vol = temp_buf.ToLong(); break;
						case 5: real_vol = temp_buf.ToLong(); break;
						case 6: spread = temp_buf.ToLong(); break;
					}
				}
				if(checkdate(&dtm) && close > 0.0) {
					SUniTime ut;
					ut.Set(dtm, SUniTime::indMin);
					uint   item_idx = 0;
					THROW_SL(ts.AddItem(ut, &item_idx));
					//THROW_SL(ts.SetValue(item_idx, vecidx_open, open));
					THROW_SL(ts.SetValue(item_idx, vecidx_close, close));
					//THROW_SL(ts.SetValue(item_idx, vecidx_ticvol, tick_vol));
					THROW_SL(ts.SetValue(item_idx, vecidx_realvol, real_vol));
					//THROW_SL(ts.SetValue(item_idx, vecidx_spread, spread));
				}
			}
		}
		{
			//SFile f_out(test_file_name, SFile::mWrite);
			//
			{
				STimeSeries dts;
				SBuffer sbuf; // serialize buf
				SBuffer cbuf; // compress buf
				SBuffer dbuf; // decompress buf
				SSerializeContext sctx;
				THROW_SL(ts.Serialize(+1, sbuf, &sctx));
				{
					{
						SCompressor c(SCompressor::tZLib);
						THROW_SL(c.CompressBlock(sbuf.GetBuf(sbuf.GetRdOffs()), sbuf.GetAvailableSize(), cbuf, 0, 0));
					}
					{
						SCompressor c(SCompressor::tZLib);
						THROW_SL(c.DecompressBlock(cbuf.GetBuf(cbuf.GetRdOffs()), cbuf.GetAvailableSize(), dbuf));
					}
					THROW_SL(dts.Serialize(-1, dbuf, &sctx));
				}
				dts.GetCount();
				THROW(dts.Sort());
				{
					SFile f_out(test_file_name, SFile::mWrite);
					THROW(f_out.IsValid());
					for(uint i = 0; i < dts.GetCount(); i++) {
						SUniTime ut;
						dts.GetTime(i, &ut);
						ut.Get(dtm);
						//THROW(dts.GetValue(i, vecidx_open, &open));
						THROW(dts.GetValue(i, vecidx_close, &close));
						//THROW(dts.GetValue(i, vecidx_ticvol, &tick_vol));
						THROW(dts.GetValue(i, vecidx_realvol, &real_vol));
						//THROW(dts.GetValue(i, vecidx_spread, &spread));
						line_buf.Z().Cat(dtm.d, DATF_ANSI|DATF_CENTURY).Comma().Cat(dtm.t, TIMF_HM).Comma().
							/*Cat(open, MKSFMTD(0, 5, 0)).Comma().*/Cat(close, MKSFMTD(0, 5, 0)).Comma()./*Cat(tick_vol).Comma().*/Cat(real_vol)/*.Comma().Cat(spread)*/.CR();
						THROW(f_out.WriteLine(line_buf));
					}
				}
			}
			{
				PPID    id = 0;
				PPTimeSeriesPacket ts_pack;
				STimeSeries ex_ts;
				int gts = 0;
				if(SearchBySymb("eurusd", &id, &ts_pack.Rec) > 0) {
					gts = GetTimeSeries(id, ex_ts);
				}
				else {
					STRNSCPY(ts_pack.Rec.Name, "eurusd");
					STRNSCPY(ts_pack.Rec.Symb, "eurusd");
					THROW(PutPacket(&id, &ts_pack, 1));
				}
				THROW(SetTimeSeries(id, &ts, 1));
			}
		}
	}
	CATCHZOK
	return ok;
}

static int SLAPI AnalyzeTsTradeFrame(const STimeSeries & rTs, uint frameSize, double target, double * pFreq, double * pMaxDuckAvg)
{
	int    ok = -1;
	const  uint tsc = rTs.GetCount();
	RealArray frame;
	uint   frame_count = 0;
	uint   target_count = 0;
	StatBase max_duck_stat(0);
	const double zero = 0.0;
	for(uint fs = 0; fs < (tsc-frameSize); fs++) {
		rTs.GetFrame("close", fs, frameSize, STimeSeries::nfOne|STimeSeries::nfBaseStart, frame);
		int   target_reached = 0;
		double max_duck = 1.0;
		if(frame.getCount() == frameSize) {
			for(uint i = 0; !target_reached && i < frame.getCount(); i++) {
				const double fv = frame.at(i);
				SETMIN(max_duck, fv);
				if(fv > target)
					target_reached = 1;
			}
			frame_count++;
			ok = 1;
		}
		if(target_reached) {
			max_duck_stat.Step(max_duck);
			target_count++;
		}
	}
	max_duck_stat.Finish();
	ASSIGN_PTR(pFreq, fdivui(target_count, frame_count));
	ASSIGN_PTR(pMaxDuckAvg, max_duck_stat.GetExp());
	return ok;
}

struct TestStrategyRawResult {
	struct StepHistogramItem {
		StepHistogramItem() : StepDot(0), StepAngle(0.0), SuccCount(0), Mean(0.0), StdDev(0.0), Result(0.0), ResultQ(0.0), Prob(0.0)
		{
		}
		uint   StepDot;   // Размер сегмента в точках
		double StepAngle; // Размер сегмента в радианах
		uint   SuccCount; // Количество успешных сегментов (дающих позитивный результат при вероятности выгрыша, превышающей фиксированный порог)
		double Mean;      // Среднее количество точек для шага
		double StdDev;    // Стандартное отклонение количества точек для шага
		double Result;    // Суммарный результат ставок (суммируются только успешные сегменты)
		double ResultQ;   // Суммарный результат взвешенный по q-фактору
		double Prob;      // Суммарная вероятность выигрышей (только для успешных сегментов)
	};
	TestStrategyRawResult() : TotalResult(0.0), TotalProb(0.0), BaseFlags(0), InputFrameSize(0), MainFrameSize(0), MainFrameRangeIdx(0),
		TargetQuant(0), MaxDuckQuant(0), StakeMode(0), Reserve(0),
		BestStep(0), BestStepBarCount(0), BestStepResult(0.0), BestStepProb(0.0), DoCalcResonanceMeasure(false), ResonanceMeasure(0.0)
	{
		FactorR.Z();
	}
	const char * GetStakeDirText() const { return (BaseFlags & PPObjTimeSeries::Strategy::bfShort) ? "SHORT" : "LONG"; }
	TSVector <StrategyOptEntry> SoList;
	TSVector <StepHistogramItem> SahList; // @v10.7.8 Гистограмма результатов по угловым шагам
	TSVector <StepHistogramItem> SdhList; // @v10.7.8 Гистограмма результатов по шагам в точках
	RealRange FactorR;       // @v10.7.8 Диапазон изменения тангенса наклона регрессии для всего множества значений
	double TotalResult;
	double TotalProb;
	uint   BestStep;
	uint   BestStepBarCount;
	double BestStepResult;
	double BestStepProb;
	double ResonanceMeasure;
	//
	// Параметры стратегии
	//
	uint32 BaseFlags;        // @flags
	uint32 InputFrameSize;   // Количество периодов с отсчетом назад, на основании которых принимается прогноз
	uint32 MainFrameSize;    // Длина периода магистрального тренда
	uint32 MainFrameRangeIdx; // @v10.7.9
	uint16 TargetQuant;      // Максимальный рост в квантах SpikeQuant
	uint16 MaxDuckQuant;     // Максимальная величина "проседания" в квантах SpikeQuant
	int16  StakeMode;        // Режим покупки: 0 - сплошной (случайный); 1 - по значению тренда // @v10.8.9 unused: 2 - по изменению тренда, 3 - по значению и изменению тренда
	bool   DoCalcResonanceMeasure; // @v10.8.0 Вычислить только фактор резонанса для набора параметров стратегий по фиксированному зерну (гистограммы не вычислять)
	uint8  Reserve;          // @alignment
};

IMPL_CMPFUNC(PPObjTimeSeries_OptimalFactorRange_Result, p1, p2)
{
	const PPObjTimeSeries::OptimalFactorRange * p_i1 = static_cast<const PPObjTimeSeries::OptimalFactorRange *>(p1);
	const PPObjTimeSeries::OptimalFactorRange * p_i2 = static_cast<const PPObjTimeSeries::OptimalFactorRange *>(p2);
	return CMPSIGN(p_i1->Result, p_i2->Result);
}

static double CalcFactorRangeResultSum(const TSVector <StrategyOptEntry> & rList, uint firstIdx, uint lastIdx) // CalcFactorRangeResult_Func
{
	return rList.sumDouble(offsetof(StrategyOptEntry, Result1), firstIdx, lastIdx);
}

static double CalcFactorRangeResult_R1DivR2(const TSVector <StrategyOptEntry> & rList, uint firstIdx, uint lastIdx) // CalcFactorRangeResult_Func
{
	double s1 = rList.sumDouble(offsetof(StrategyOptEntry, Result1), firstIdx, lastIdx);
	double s2 = rList.sumDouble(offsetof(StrategyOptEntry, Result2), firstIdx, lastIdx);
	return fdivnz(s1, s2) /* * (lastIdx - firstIdx + 1) */;
}

static double CalcFactorRangeResult_R2DivC(const TSVector <StrategyOptEntry> & rList, uint firstIdx, uint lastIdx) // CalcFactorRangeResult_Func
{
	double s2 = rList.sumDouble(offsetof(StrategyOptEntry, Result2), firstIdx, lastIdx);
	double c = static_cast<double>(lastIdx - firstIdx + 1);
	return fdivnz(s2, c) /* * (lastIdx - firstIdx + 1) */;
}

static double CalcFactorRangeResult2Sum(const TSVector <StrategyOptEntry> & rList, const IntRange & rPosRange) // CalcFactorRangeResult2_Func
{
	return rList.sumDouble(offsetof(StrategyOptEntry, Result1), rPosRange.low, rPosRange.upp);
}

static double CalcFactorRangeResult2Sum_SpecMult(const TSVector <StrategyOptEntry> & rList, const IntRange & rPosRange) // CalcFactorRangeResult2_Func
{
	double result = rList.sumDouble(offsetof(StrategyOptEntry, Result1), rPosRange.low, rPosRange.upp);
	double c = static_cast<double>(rPosRange.upp - rPosRange.low + 1);
	return result * pow(c, 1.0/3.0);
}

static double CalcFactorRangeResult2_R1DivR2(const TSVector <StrategyOptEntry> & rList, const IntRange & rPosRange) // CalcFactorRangeResult2_Func
{
	double s1 = rList.sumDouble(offsetof(StrategyOptEntry, Result1), rPosRange.low, rPosRange.upp);
	double s2 = rList.sumDouble(offsetof(StrategyOptEntry, Result2), rPosRange.low, rPosRange.upp);
	return fdivnz(s1, s2) /* * (lastIdx - firstIdx + 1) */;
}

static double CalcFactorRangeResult2_R2DivC_Simple(const TSVector <StrategyOptEntry> & rList, const IntRange & rPosRange) // CalcFactorRangeResult2_Func
{
	double s2 = rList.sumDouble(offsetof(StrategyOptEntry, Result2), rPosRange.low, rPosRange.upp);
	double c = static_cast<double>(rPosRange.upp - rPosRange.low + 1);
	return fdivnz(s2, c);
}

static double CalcFactorRangeResult2_R2DivC(const TSVector <StrategyOptEntry> & rList, const IntRange & rPosRange) // CalcFactorRangeResult2_Func
{
	double s2 = rList.sumDouble(offsetof(StrategyOptEntry, Result2), rPosRange.low, rPosRange.upp);
	double result = 0.0;
	const double c = static_cast<double>(rPosRange.upp - rPosRange.low + 1);
	result = fdivnz(s2, c);
	//result *= log(static_cast<double>(rPosRange.upp - rPosRange.low + 1)); // @20200603 experimental
	if(result == 1.0) {
		double s1 = rList.sumDouble(offsetof(StrategyOptEntry, Result1), rPosRange.low, rPosRange.upp);
		result *= s1;
	}
	return result;
}

static double CalcFactorRangeResult2_R2DivAngle(const TSVector <StrategyOptEntry> & rList, const IntRange & rPosRange) // CalcFactorRangeResult2_Func
{
	double s2 = rList.sumDouble(offsetof(StrategyOptEntry, Result2), rPosRange.low, rPosRange.upp);
	double result = 0.0;
	// @20200526 @experimental (проверка угловой вероятности против линейной) {
	// Вывод: достойный кандидат на рассмотрение, но пока не будем использовать. Зерно нужно делать меньше чем при абсолютной
	// вероятности, стратегии получаются более равномерные по количеству ставок.
	const double start_angle = atan(rList.at(rPosRange.low).Factor);
	const double end_angle = atan(rList.at(rPosRange.upp).Factor);
	const double delta_angle = end_angle - start_angle;
	assert(delta_angle > 0.0);
	result = fdivnz(s2, delta_angle);
	// } @20200526 @experimental
	return result;
}

typedef double (* CalcFactorRangeResult_Func)(const TSVector <StrategyOptEntry> & rList, uint firstIdx, uint lastIdx);
typedef double (* CalcFactorRangeResult2_Func)(const TSVector <StrategyOptEntry> & rList, const IntRange & rPosRange);

static int FASTCALL IsThereIntRangeIntersection(const TSVector <IntRange> & rList, const IntRange & rR/*, int32 * pFirstIsUpperBount*/)
{
	for(uint i = 0; i < rList.getCount(); i++) {
		if(rR.Intersect(rList.at(i))) {
			//ASSIGN_PTR(pFirstIsUpperBount, rList.at(i).upp);
			return 1;
		}
	}
	return 0;
}

static int FASTCALL IsThereIntRangeIntersection_OnSortedList(const TSVector <IntRange> & rList, const IntRange & rR/*, int32 * pFirstIsUpperBount*/)
{
	for(uint i = 0; i < rList.getCount(); i++) {
		const IntRange & r_item = rList.at(i);
		if(rR.Intersect(r_item)) {
			//ASSIGN_PTR(pFirstIsUpperBount, rList.at(i).upp);
			return 1;
		}
		else if(rR.upp < r_item.low)
			break;
		assert(r_item != rR); // @v10.7.7
	}
	return 0;
}

struct FindOptimalFactorRangeExtrEntry {
	FindOptimalFactorRangeExtrEntry() : MaxResult(0.0), MaxSfIdx(0), MaxSfIdxUp(0)
	{
	}
	double MaxResult;
	uint   MaxSfIdx;
	uint   MaxSfIdxUp;
};

static int SLAPI FindOptimalFactorRange_InitialSplitting(const TSVector <StrategyOptEntry> & rList, int useInitialSplitting, uint * pFirstIdx, uint * pLastIdx)
{
	int    result = 0;
	assert(oneof3(useInitialSplitting, 4, 5, 6));
	const  uint _c = rList.getCount();
	if(_c > 1 && oneof3(useInitialSplitting, 4, 5, 6)) {
		double total_max_result = 0.0;
		TSVector <IntRange> pos_range_list;
		IntRange work_range;
		work_range.Set(0, _c-1);
		uint   _first_idx = 0;
		uint   _last_idx = _c-1;
		int    do_break = 0; // Сигнал для выхода из функции из-за того, что приемлемых значений больше нет
		if(oneof2(useInitialSplitting, 5, 6)) {
			if(rList.at(0).Factor >= 0.0 && useInitialSplitting == 6) {
				_last_idx = 0;
				do_break = 1; // nothing to do
			}
			else if(rList.at(_c-1).Factor <= 0.0 && useInitialSplitting == 5) {
				_last_idx = 0;
				do_break = 1; // nothing to do
			}
			else {
				if(useInitialSplitting == 5) {
					for(uint i = 0; i < _c; i++) {
						if(rList.at(i).Factor > 0.0) {
							_first_idx = i;
							break;
						}
					}
				}
				else if(useInitialSplitting == 6) {
					for(uint i = 0; i < _c; i++) {
						if(rList.at(i).Factor >= 0.0) {
							assert(i > 0);
							_last_idx = i-1;
							break;
						}
					}
				}
			}
		}
		ASSIGN_PTR(pFirstIdx, _first_idx);
		ASSIGN_PTR(pLastIdx, _last_idx);
		if(!do_break)
			result = 1;
	}
	return result;
}

static IMPL_CMPFUNC(OptimalFactorRange_WithPositions_ByPos, i1, i2)
{
	const PPObjTimeSeries::OptimalFactorRange_WithPositions * p1 = static_cast<const PPObjTimeSeries::OptimalFactorRange_WithPositions *>(i1);
	const PPObjTimeSeries::OptimalFactorRange_WithPositions * p2 = static_cast<const PPObjTimeSeries::OptimalFactorRange_WithPositions *>(i2);
	int    si = 0;
	CMPCASCADE2(si, p1, p2, LoPos, UpPos);
	return si;
}

static int SLAPI FindOptimalFactorRange_PostProcess(const TSVector <StrategyOptEntry> & rList, CalcFactorRangeResult2_Func cfrrFunc, 
	TSVector<PPObjTimeSeries::OptimalFactorRange_WithPositions> & rRc, int singleScanAlgorithm)
{
	int    ok = 1;
	if(rRc.getCount() > 1) {
		IntRange work_range;
		rRc.sort(PTR_CMPFUNC(OptimalFactorRange_WithPositions_ByPos));
		int is_there_adjacent_chunks = 0;
		if(singleScanAlgorithm) {
			/* @construction uint i = rRc.getCount();
			if(i) do {
				const PPObjTimeSeries::OptimalFactorRange_WithPositions & r_this = rRc.at(--i);
				if(i) {
					uint prev_idx = i-1;
					int  do_next = 0;
					do {
						do_next = 0;
						const PPObjTimeSeries::OptimalFactorRange_WithPositions & r_prev = rRc.at(prev_idx);
						if(r_prev.LoPos == r_this.LoPos) {
							assert(r_this.UpPos >= r_prev.UpPos);
							rRc.atFree(prev_idx);
							prev_idx--;
							do_next = 1;
						}
						else {
							if(i < (rRc.getCount()-1)) {
								uint next_idx = i+1;
								const PPObjTimeSeries::OptimalFactorRange_WithPositions & r_next = rRc.at(next_idx);
								if(r_next.UpPos == r_this.UpPos) {
									assert(r_next.LoPos >= r_this.LoPos);
								}
							}
							do_next = 0;
							break; // @paranoic (контрольный в голову)
						}
					} while(do_next);
				}
			} while(i);
			*/
		}
		{
			for(uint i = 1; i < rRc.getCount(); i++) {
				const PPObjTimeSeries::OptimalFactorRange_WithPositions & r_prev = rRc.at(i-1);
				const PPObjTimeSeries::OptimalFactorRange_WithPositions & r_this = rRc.at(i);
				if(!singleScanAlgorithm) {
					assert(r_this.LoPos > r_prev.LoPos);
					assert(r_this.UpPos > r_prev.UpPos);
					assert(r_this.LoPos > r_prev.UpPos);
				}
				const int adjacent_diff = (r_this.LoPos - r_prev.UpPos);
				if(adjacent_diff == 1) {
					is_there_adjacent_chunks = 1;
				}
			}
		}
		if(is_there_adjacent_chunks) {
			TSVector<PPObjTimeSeries::OptimalFactorRange_WithPositions> new_rc;
			new_rc.insert(&rRc.at(0));
			for(uint i = 1; i < rRc.getCount(); i++) {
				const PPObjTimeSeries::OptimalFactorRange_WithPositions & r_prev = new_rc.at(new_rc.getCount()-1); // @nb not rRc but new_rc
				const PPObjTimeSeries::OptimalFactorRange_WithPositions & r_this = rRc.at(i);
				const int adjacent_diff = (r_this.LoPos - r_prev.UpPos);
				if(adjacent_diff == 1) {
					PPObjTimeSeries::OptimalFactorRange_WithPositions new_range;
					new_range.LoPos = r_prev.LoPos;
					new_range.UpPos = r_this.UpPos;
					new_range.Set(rList.at(new_range.LoPos).Factor, rList.at(new_range.UpPos).Factor);
					assert(new_range.low == r_prev.low && new_range.upp == r_this.upp);
					new_range.GenSeq = MIN(r_prev.GenSeq, r_this.GenSeq); // @v10.7.9
#ifndef NDEBUG
					const double result_prev = r_prev.Result;
					const double result_this = r_this.Result;
					work_range.Set(r_prev.LoPos, r_prev.UpPos);
					const double result_prev_c = cfrrFunc(rList, work_range);
					assert(feqeps(result_prev_c, result_prev, 1E-6));
					work_range.Set(r_this.LoPos, r_this.UpPos);
					const double result_this_c = cfrrFunc(rList, work_range);
					assert(feqeps(result_this_c, result_this, 1E-6));
#endif
					work_range.Set(new_range.LoPos, new_range.UpPos);
					const double temp_result = cfrrFunc(rList, work_range);
					new_range.Result = temp_result;
					//
					new_range.Count = (new_range.UpPos - new_range.LoPos + 1);
					new_rc.atFree(new_rc.getCount()-1);
					new_rc.insert(&new_range);
				}
				else {
					new_rc.insert(&r_this);
				}
			}
			rRc = new_rc;
		}
	}
	return ok;
}

SLAPI PPObjTimeSeries::StrategyResultValue::StrategyResultValue() : Result(0.0), TmCount(0), TmSec(0)
{
}

void FASTCALL PPObjTimeSeries::StrategyResultValue::Append(const StrategyResultValue & rV)
{
	Result += rV.Result;
	TmCount += rV.TmCount;
	TmSec += rV.TmSec;
}

void FASTCALL PPObjTimeSeries::StrategyResultEntry::SetValue(const StrategyResultValue & rV)
{
	V.Append(rV);
	StakeCount++;
	if(rV.Result > 0.0)
		WinCount++;
}

void FASTCALL PPObjTimeSeries::StrategyResultEntry::SetValue(const StrategyResultValueEx & rV)
{
	SetValue(static_cast<const StrategyResultValue &>(rV));
	SumPeak += rV.Peak;
	SumBottom += rV.Bottom;
	SETMAX(MaxPeak, rV.Peak);
}

void FASTCALL PPObjTimeSeries::StrategyResultEntry::SetOuter(const StrategyResultEntry & rS)
{
	ENTER_CRITICAL_SECTION
	V.Append(rS.V);
	StakeCount += rS.StakeCount;
	WinCount += rS.WinCount;
	SumPeak += rS.SumPeak;
	SumBottom += rS.SumBottom;
	SETMAX(MaxPeak, rS.MaxPeak);
	LEAVE_CRITICAL_SECTION
}

PPObjTimeSeries::StrategyResultValue & SLAPI PPObjTimeSeries::StrategyResultValue::Z()
{
	Result = 0.0;
	TmCount = 0;
	TmSec = 0;
	return *this;
}

double SLAPI PPObjTimeSeries::StrategyResultValue::GetResultPerSec() const { return fdivnz(Result, static_cast<double>(TmSec)); }
double SLAPI PPObjTimeSeries::StrategyResultValue::GetResultPerDay() const { return fdivnz(3600.0 * 24.0 * Result, static_cast<double>(TmSec)); }

SLAPI PPObjTimeSeries::StrategyResultValueEx::StrategyResultValueEx() : StrategyResultValue(), Peak(0.0), Bottom(0.0), StartPoint(0), LastPoint(0), StrategyIdx(-1),
	OptFactor(0.0), OptFactor2(0.0), TrendErr(0.0), TrendErrRel(0.0), MainTrendErr(0.0), MainTrendErrRel(0.0), LocalDeviation(0.0), LocalDeviation2(0.0)
{
}

PPObjTimeSeries::StrategyResultValueEx & SLAPI PPObjTimeSeries::StrategyResultValueEx::Z()
{
	StrategyResultValue::Z();
	Peak = 0.0;
	Bottom = 0.0;
	StartPoint = 0;
	LastPoint = 0;
	TmR.Z();
	StrategyIdx = -1;
	OptFactor = 0.0;
	OptFactor2 = 0.0;
	TrendErr = 0.0;
	TrendErrRel = 0.0;
	MainTrendErr = 0.0;
	MainTrendErrRel = 0.0;
	LocalDeviation = 0.0;
	LocalDeviation2 = 0.0;
	return *this;
}

SLAPI PPObjTimeSeries::StrategyResultEntry::StrategyResultEntry() : Strategy(),
	LastResultIdx(0), SumPeak(0.0), SumBottom(0.0), MaxPeak(0.0), TotalSec(0)
{
	PTR32(Symb)[0] = 0;
}

SLAPI PPObjTimeSeries::StrategyResultEntry::StrategyResultEntry(const PPObjTimeSeries::Strategy & rS, int stakeMode) : Strategy(),
	SumPeak(0.0), SumBottom(0.0), MaxPeak(0.0), TotalSec(0), LastResultIdx(0)
{
	PTR32(Symb)[0] = 0;
	InputFrameSize = rS.InputFrameSize;
	Prec = rS.Prec;
	TargetQuant = rS.TargetQuant;
	MaxDuckQuant = rS.MaxDuckQuant;
	OptDelta2Stride = rS.OptDelta2Stride;
	StakeMode = stakeMode;
	BaseFlags = rS.BaseFlags;
	Margin = rS.Margin;
	SpikeQuant_s = rS.SpikeQuant_s;
	SpreadAvg = rS.SpreadAvg;
	OptDeltaRange = rS.OptDeltaRange;
	OptDelta2Range = rS.OptDelta2Range;
	PeakAvgQuant = rS.PeakAvgQuant;
	BottomAvgQuant = rS.BottomAvgQuant;
	PeakMaxQuant = rS.PeakMaxQuant;
	TrendErrAvg = rS.TrendErrAvg;
	TrendErrLim = rS.TrendErrLim;
	MainFrameSize = rS.MainFrameSize;
	MainTrendErrAvg = rS.MainTrendErrAvg;
	MainTrendErrLim = rS.MainTrendErrLim;
	GenSeq = rS.GenSeq;
	GenPtCount = rS.GenPtCount;
}

SLAPI PPObjTimeSeries::StrategyResultEntry::StrategyResultEntry(const PPObjTimeSeries::TrainNnParam & rTnnp, int stakeMode) : Strategy(),
	SumPeak(0.0), SumBottom(0.0), MaxPeak(0.0), TotalSec(0), LastResultIdx(0)
{
	STRNSCPY(Symb, rTnnp.Symb);
	InputFrameSize = rTnnp.InputFrameSize;
	Prec = rTnnp.Prec;
	TargetQuant = rTnnp.TargetQuant;
	MaxDuckQuant = rTnnp.MaxDuckQuant;
	OptDelta2Stride = rTnnp.OptDelta2Stride;
	StakeMode = stakeMode;
	BaseFlags = rTnnp.BaseFlags;
	Margin = rTnnp.Margin;
	SpikeQuant_s = rTnnp.SpikeQuant_s;
	SpreadAvg = rTnnp.SpreadAvg;
	OptDeltaRange = rTnnp.OptDeltaRange;
	OptDelta2Range = rTnnp.OptDelta2Range;
	PeakAvgQuant = rTnnp.PeakAvgQuant;
	BottomAvgQuant = rTnnp.BottomAvgQuant;
	PeakMaxQuant = rTnnp.PeakMaxQuant;
	TrendErrAvg = rTnnp.TrendErrAvg;
	TrendErrLim = rTnnp.TrendErrLim;
	MainFrameSize = rTnnp.MainFrameSize;
	MainTrendErrAvg = rTnnp.MainTrendErrAvg;
	MainTrendErrLim = rTnnp.MainTrendErrLim;
	GenPtCount = rTnnp.GenPtCount; // @v10.7.11
}

SLAPI PPObjTimeSeries::TrendEntry::TrendEntry(uint stride, uint nominalCount) : Stride(stride), NominalCount(nominalCount), SpikeQuant_r(0.0), ErrAvg(0.0), ErrLimitByPel(0.0)
{
	assert(stride > 0 && stride <= 100000);
	//assert(NominalCount > 0 && NominalCount <= 100);
}

//static
const PPObjTimeSeries::TrendEntry * FASTCALL PPObjTimeSeries::SearchTrendEntry(const TSCollection <PPObjTimeSeries::TrendEntry> & rTrendList, uint stride)
{
	for(uint i = 0; i < rTrendList.getCount(); i++) {
		const PPObjTimeSeries::TrendEntry * p_te = rTrendList.at(i);
		if(p_te && p_te->Stride == stride)
			return p_te;
	}
	return 0;
}

SLAPI PPObjTimeSeries::BestStrategyBlock::BestStrategyBlock() : MaxResult(0.0), MaxResultIdx(-1), TvForMaxResult(0.0), Tv2ForMaxResult(0.0),
	TrendErr(0.0), TrendErrRel(0.0), MainTrendErr(0.0), MainTrendErrRel(0.0), LocalDeviation(0.0), LocalDeviation2(0.0)
{
}

PPObjTimeSeries::BestStrategyBlock & SLAPI PPObjTimeSeries::BestStrategyBlock::Z()
{
	MaxResult = 0.0;
	MaxResultIdx = -1;
	TvForMaxResult = 0.0;
	Tv2ForMaxResult = 0.0;
	TrendErr = 0.0;
	TrendErrRel = 0.0;
	MainTrendErr = 0.0;
	MainTrendErrRel = 0.0;
	LocalDeviation = 0.0;
	LocalDeviation2 = 0.0;
	return *this;
}

void SLAPI PPObjTimeSeries::BestStrategyBlock::SetResult(double localResult, uint strategyIdx, double tv, double tv2)
{
	if(MaxResult < localResult) {
		MaxResult = localResult;
		MaxResultIdx = static_cast<int>(strategyIdx);
		TvForMaxResult = tv;
		Tv2ForMaxResult = tv2;
	}
}

SLAPI  PPObjTimeSeries::FactorToResultRelation::FactorToResultRelation() : FactorQuant(0), PeakAvg(0), Result(0.0)
{
}

SLAPI  PPObjTimeSeries::FactorToResultRelation::FactorToResultRelation(uint factorQuant, double result) : FactorQuant(factorQuant), PeakAvg(0), Result(result)
{
}

static FORCEINLINE double Implement_CalcSL(int sell, double mdv, int prec, double peak)
	{ return sell ? round(peak * (1.0 + mdv), prec) : round(peak * (1.0 - mdv), prec); }
static FORCEINLINE double Implement_CalcSL_Short(double mdv, int prec, double peak)
	{ return round(peak * (1.0 + mdv), prec); }
static FORCEINLINE double Implement_CalcSL_Long(double mdv, int prec, double peak)
	{ return round(peak * (1.0 - mdv), prec); }

int SLAPI PPObjTimeSeries::Strategy::CalcResult2(const DateTimeArray & rTmList, const RealArray & rValList, uint valueIdx, PPObjTimeSeries::StrategyResultValueEx & rV) const
{
	rV.Z();
	int    ok = 1;
	const  uint tsc = rTmList.getCount();
	assert(tsc == rValList.getCount());
	if(tsc && tsc == rValList.getCount() && valueIdx < tsc) {
		const LDATETIME start_tm = rTmList.get(valueIdx);
		const double stake_value = rValList.at(valueIdx);
		double peak = stake_value;
		double bottom = stake_value;
		const  uint org_max_duck_quant = MaxDuckQuant;
		const  uint max_duck_quant = org_max_duck_quant; // Возможно, будет меняться в течении жизни ставки (пока нет)
		const  double mdv = (MaxDuckQuant * SpikeQuant_s);
		const  int is_short = BIN(BaseFlags & bfShort);
		const  int prec = Prec;
		const  int adjust_sl = BIN(StakeCloseMode == clsmodAdjustLoss); // @v10.4.8
		{
			static const double spread_adjustment = 1.1; // Поправка запаса прочности для размера спреда
			const uint _target_quant = TargetQuant;
			const double spread = (Prec > 0 && SpreadAvg > 0.0) ? (SpreadAvg * fpow10i(-Prec) * spread_adjustment) : 0.0;
			const double margin = Margin;
			uint k = valueIdx+1;
			if(is_short) {
				const double target_value = _target_quant ? round(stake_value * (1.0 - (_target_quant * SpikeQuant_s)), prec) : 0.0;
				const double org_sl = Implement_CalcSL_Short(mdv, prec, peak);
				double sl = org_sl;
				while(k < tsc) {
					const double value = rValList.at(k);
					const double value_with_spread = (value + spread);
					const double stake_delta = -((value - stake_value) / stake_value);
					rV.Result = -((value_with_spread - stake_value) / stake_value) / margin;
					SETMAX(bottom, value);
					if(value >= sl) {
						ok = 2;
						break;
					}
					else if(_target_quant && value <= target_value) {
						ok = 2;
						break;
					}
					else {
						assert(bottom <= org_sl);
						if(peak > value) {
							peak = value;
							if(adjust_sl)
								sl = Implement_CalcSL_Short(mdv, prec, peak);
						}
						k++;
					}
				}
			}
			else {
				const double target_value = _target_quant ? round(stake_value * (1.0 + (_target_quant * SpikeQuant_s)), prec) : 0.0;
				const double org_sl = Implement_CalcSL_Long(mdv, prec, peak);
				double sl = org_sl;
				while(k < tsc) {
					const double value = rValList.at(k);
					const double value_with_spread = (value - spread);
					const double stake_delta = ((value - stake_value) / stake_value);
					rV.Result = ((value_with_spread - stake_value) / stake_value) / margin;
					SETMIN(bottom, value);
					if(value <= sl) {
						ok = 2;
						break;
					}
					else if(_target_quant && value >= target_value) {
						ok = 2;
						break;
					}
					else {
						assert(bottom >= org_sl);
						if(peak < value) {
							peak = value;
							if(adjust_sl)
								sl = Implement_CalcSL_Long(mdv, prec, peak);
						}
						k++;
					}
				}
			}
			rV.StartPoint = valueIdx; // @v10.7.9
			rV.TmCount = k - valueIdx;
			if(k == tsc) {
				const LDATETIME local_tm = rTmList.get(k-1);
				rV.TmSec = diffdatetimesec(local_tm, start_tm);
				rV.LastPoint = k-1;
				ok = 3;
			}
			else {
				const LDATETIME local_tm = rTmList.get(k);
				rV.TmSec = diffdatetimesec(local_tm, start_tm);
				rV.LastPoint = k;
				if(is_short) {
					rV.Peak = (stake_value - peak) / stake_value;
					rV.Bottom = (bottom - stake_value) / stake_value;
				}
				else {
					rV.Peak = (peak - stake_value) / stake_value;
					rV.Bottom = (stake_value - bottom) / stake_value;
				}
				rV.TmR.Start = start_tm; // @v10.4.11
				rV.TmR.Finish = local_tm; // @v10.4.11
				// @v10.7.8 {
				//if(rV.Result < 0.0) rV.Result *= 1.2; // Дополнительный штраф за проигрыш
				// } @v10.7.8
			}
		}
	}
	return ok;
}

void SLAPI PPObjTimeSeries::StrategyContainer::Simulate(const DateTimeArray & rTmList, const RealArray & rValList, const TSCollection <TrendEntry> & rTrendList,
	const Config & rCfg, StrategyResultEntry & rSre, TSVector <StrategyResultValueEx> * pDetailsList) const
{
	const uint tsc = rTmList.getCount();
	assert(tsc == rValList.getCount());
	if(tsc && tsc == rValList.getCount()) {
		uint max_ifs = 0;
		uint max_delta2_stride = 0;
		PPObjTimeSeries::StrategyContainer::Index1 sc_index1;
		CreateIndex1(sc_index1);
		for(uint tidx = 0; tidx < rTrendList.getCount(); tidx++) {
			const PPObjTimeSeries::TrendEntry * p_te = rTrendList.at(tidx);
			assert(p_te);
			if(p_te) {
				assert(p_te->TL.getCount() == tsc);
				SETMAX(max_ifs, p_te->Stride);
			}
		}
		const uint _start_offset = max_ifs+max_delta2_stride;
		const uint _max_test_count = NZOR(rCfg.E.TestCount, 1);
		const uint _offs_inc_list[] = { 173, 269, 313, 401, 439 };
		SelectBlock scsb(rTrendList, sc_index1);
		scsb.P_VList = &rValList;
		scsb.DevPtCount = rCfg.E.LocalDevPtCount;
		scsb.LDMT_Factor = rCfg.E.LDMT_Factor;
		scsb.MainTrendMaxErrRel = rCfg.E.MainTrendMaxErrRel;
		scsb.Criterion = PPObjTimeSeries::StrategyContainer::selcritResult | PPObjTimeSeries::StrategyContainer::selcritfSkipAmbiguous;
		for(uint init_offset = _start_offset, test_no = 0; test_no < _max_test_count; (init_offset += _offs_inc_list[test_no % SIZEOFARRAY(_offs_inc_list)]), (test_no++)) {
			const LDATETIME start_tm = rTmList.at(init_offset);
			LDATETIME finish_tm = ZERODATETIME;
			for(uint i = init_offset; i < tsc; i++) {
				if(SelectS2(scsb.Init(static_cast<int>(i))) > 0) {
					assert(scsb.MaxResultIdx >= 0 && scsb.MaxResultIdx < getCountI());
					const PPObjTimeSeries::Strategy & r_s = at(scsb.MaxResultIdx);
					PPObjTimeSeries::StrategyResultValueEx rv_ex;
					const int csr = r_s.CalcResult2(rTmList, rValList, i, rv_ex);
					if(csr == 2) {
						rSre.SetValue(rv_ex);
						const LDATETIME local_finish_tm = rTmList.at(rv_ex.LastPoint);
						assert(local_finish_tm == rv_ex.TmR.Finish);
						if(cmp(finish_tm, local_finish_tm) < 0)
							finish_tm = local_finish_tm;
						rv_ex.StrategyIdx = static_cast<int>(scsb.MaxResultIdx);
						rv_ex.OptFactor = scsb.TvForMaxResult;
						rv_ex.OptFactor2 = scsb.Tv2ForMaxResult;
						rv_ex.TrendErr = scsb.TrendErr;
						rv_ex.TrendErrRel = scsb.TrendErrRel;
						rv_ex.MainTrendErr = scsb.MainTrendErr;
						rv_ex.MainTrendErrRel = scsb.MainTrendErrRel;
						rv_ex.LocalDeviation = scsb.LocalDeviation;
						rv_ex.LocalDeviation2 = scsb.LocalDeviation2;
						CALLPTRMEMB(pDetailsList, insert(&rv_ex));
						i = rv_ex.LastPoint; // Дальше продолжим со следующей точки // @v10.5.1 (rv_ex.LastPoint+1)-->(rv_ex.LastPoint) цикл for сделает инкремент
					}
					else
						break; // Ряд оборвался - дальше анализировать нельзя: некоторые стратегии не имеют достаточно данных.
				}
			}
			if(!!finish_tm)
				rSre.TotalSec += diffdatetimesec(finish_tm, start_tm);
		}
	}
}

static SString & FASTCALL _TsFineOptFactorMakeFinishEvntName(const char * pUniq, SString & rBuf)
{
    size_t len = sstrlen(pUniq);
    uint32 hash = SlHash::BobJenc(pUniq, len);
	(rBuf = "TSFINDOPTFACTORFINISHEVNT").CatChar('-').Cat(hash);
	return rBuf;
}

//static
int SLAPI Ts_Helper_FindOptimalFactor(const DateTimeArray & rTmList, const RealArray & rValList,
	const PPObjTimeSeries::TrainNnParam & rS, double & rResult, uint & rPeakQuant)
{
	class CalcResultThread : public SlThread_WithStartupSignal {
	public:
		struct InitBlock {
			InitBlock(const DateTimeArray & rTmList, const RealArray & rValList, const PPObjTimeSeries::TrainNnParam & rS,
				uint firstIdx, uint lastIdx, PPObjTimeSeries::StrategyResultEntry * pResult, ACount * pCounter, const SString & rUniq) :
				R_TmList(rTmList), R_ValList(rValList), R_S(rS), FirstIdx(firstIdx), LastIdx(lastIdx), P_Result(pResult), P_Counter(pCounter),
				Uniq(rUniq)
			{
			}
			const DateTimeArray & R_TmList;
			const RealArray & R_ValList;
			const PPObjTimeSeries::TrainNnParam & R_S;
			const uint FirstIdx;
			const uint LastIdx;
			const SString Uniq;
			PPObjTimeSeries::StrategyResultEntry * P_Result;
			ACount * P_Counter;
		};
		CalcResultThread(InitBlock * pBlk) : SlThread_WithStartupSignal(0), B(*pBlk)
		{
		}
		virtual void Run()
		{
			assert(SLS.GetConstTLA().Id == GetThreadID());
			B.P_Counter->Incr();
			const uint tsc = B.R_TmList.getCount();
			assert(B.FirstIdx >= 0 && B.FirstIdx < tsc && B.LastIdx >= B.FirstIdx && B.LastIdx < tsc);
			PPObjTimeSeries::StrategyResultEntry sre(B.R_S, 0);
			sre.LastResultIdx = 0;
			for(uint i = B.FirstIdx; i <= B.LastIdx; i++) {
				PPObjTimeSeries::StrategyResultValueEx rv_ex;
				const  int csr = B.R_S.CalcResult2(B.R_TmList, B.R_ValList, i, rv_ex);
				assert(csr);
				if(csr == 2) {
					sre.SetValue(rv_ex);
					sre.LastResultIdx = i;
				}
			}
			B.P_Result->SetOuter(sre);
            long c = B.P_Counter->Decr();
            if(c <= 0) {
				SString temp_buf;
				Evnt evnt_finish(_TsFineOptFactorMakeFinishEvntName(B.Uniq, temp_buf), Evnt::modeOpen);
				evnt_finish.Signal();
            }
		}
	private:
		InitBlock B;
	};
	int    ok = 1;
	const  uint max_thread = 4;
	const  uint tsc = rTmList.getCount();
	Evnt * p_ev_finish = 0;
	SString temp_buf;
	PPObjTimeSeries::StrategyResultEntry sre(rS, 0);
	sre.LastResultIdx = 0;
	HANDLE objs_to_wait[16];
	size_t objs_to_wait_count = 0;
	MEMSZERO(objs_to_wait);
	assert(max_thread <= SIZEOFARRAY(objs_to_wait));
	if(max_thread > 1 && tsc >= 10000) {
		const uint chunk_size = tsc / max_thread;
		uint next_chunk_idx = 0;
		ACount thread_counter;
		S_GUID uuid;
		uuid.Generate();
		SString uniq;
		uuid.ToStr(S_GUID::fmtPlain, uniq);
		THROW_S(p_ev_finish = new Evnt(_TsFineOptFactorMakeFinishEvntName(uniq, temp_buf), Evnt::modeCreateAutoReset), SLERR_NOMEM);
		thread_counter.Incr();
		for(uint tidx = 0; tidx < max_thread; tidx++) {
			const uint last_chunk_idx = (tidx == (max_thread-1)) ? (tsc-1) : (next_chunk_idx+chunk_size-1);
			CalcResultThread::InitBlock tb(rTmList, rValList, rS, next_chunk_idx, last_chunk_idx, &sre, &thread_counter, uniq);
			CalcResultThread * p_thread = new CalcResultThread(&tb);
			THROW_S(p_thread, SLERR_NOMEM);
			p_thread->Start(1/*0*/);
			objs_to_wait[objs_to_wait_count++] = *p_thread;
			next_chunk_idx += chunk_size;
		}
		WaitForMultipleObjects(objs_to_wait_count, objs_to_wait, TRUE, INFINITE);
	}
	else {
		for(uint i = 0; i < tsc; i++) {
			PPObjTimeSeries::StrategyResultValueEx rv_ex;
			const  int csr = rS.CalcResult2(rTmList, rValList, i, rv_ex);
			THROW(csr);
			if(csr == 2) {
				sre.SetValue(rv_ex);
				sre.LastResultIdx = i;
			}
		}
	}
	rResult = sre.V.GetResultPerDay();
	const double peak_avg = sre.GetPeakAverage();
	rPeakQuant = static_cast<uint>(R0i(peak_avg / sre.SpikeQuant_s));
	CATCHZOK
	ZDELETE(p_ev_finish);
	return ok;
}

int SLAPI PPObjTimeSeries::FindOptimalFactor(const DateTimeArray & rTmList, const RealArray & rValList, const TrainNnParam & rS, int what, const IntRange & rMdRange, int mdStep,
	int entireRange, TSVector <FactorToResultRelation> & rSet, FactorToResultRelation & rResult)
{
	int    ok = -1;
	assert(oneof2(what, 0, 1));
	assert(rMdRange.low > 0 && rMdRange.upp > rMdRange.low && mdStep > 0);
	SString log_buf;
	SString temp_buf;
	SString log_file_name;
	SString symb_buf;
	rResult.FactorQuant = 0;
	rResult.PeakAvg = 0;
	rResult.Result = 0.0;
	PPGetFilePath(PPPATH_LOG, "Ts-FindOptimalLevel.log", log_file_name);
	TrainNnParam local_s(rS);
	const bool the_first_call = (rSet.getCount() == 0);
	const uint tsc = rTmList.getCount();
	TSVector <FactorToResultRelation> mdr_list;
	assert(rValList.getCount() == tsc);
	for(uint md = rMdRange.low; (int)md <= rMdRange.upp; md += mdStep) {
		int    done = 0;
		double local_result = 0.0;
		uint   local_peak_quant = 0;
		for(uint sidx = 0; !done && sidx < rSet.getCount(); sidx++) {
			const FactorToResultRelation & r_set_item = rSet.at(sidx);
			if(r_set_item.FactorQuant == md) {
				local_result = r_set_item.Result;
				done = 1;
			}
		}
		if(!done) {
			if(what == 0)
				local_s.MaxDuckQuant = md;
			else
				local_s.TargetQuant = md;
			THROW(Ts_Helper_FindOptimalFactor(rTmList, rValList, local_s, local_result, local_peak_quant));
			{
				FactorToResultRelation new_set_entry(md, local_result);
				new_set_entry.PeakAvg = local_peak_quant;
				THROW_SL(rSet.insert(&new_set_entry));
			}
		}
		{
			symb_buf = rS.Symb;
			if(rS.BaseFlags & rS.bfShort)
				symb_buf.CatChar('-').Cat("REV");
			if(what == 0)
				log_buf.Z().Cat("FindOptimalMaxDuck");
			else
				log_buf.Z().CatChar('[').CatEq("FindOptimalMaxDuck", rS.MaxDuckQuant).CatChar(']').Space().Cat("FindOptimalPeak");
			log_buf.CatDiv(':', 2).Cat(symb_buf).Space().CatEq((what == 0) ? "MaxDuckQuant" : "PeakQuant", md).Space().
				CatEq("Result", local_result, MKSFMTD(15, 5, 0));
			PPLogMessage(log_file_name, log_buf, LOGMSGF_TIME);
		}
		{
			FactorToResultRelation mdr_entry(md, local_result);
			mdr_entry.PeakAvg = local_peak_quant;
			const uint last_mdr_pos = mdr_list.getCount();
			THROW_SL(mdr_list.insert(&mdr_entry));
			if(!entireRange && (the_first_call && what == 0) && mdr_list.getCount() > 1) {
				const double prev_result = mdr_list.at(last_mdr_pos-1).Result;
				if(local_result < prev_result)
					break;
			}
		}
	}
	{
		for(uint sidx = 0; sidx < rSet.getCount(); sidx++) {
			const FactorToResultRelation & r_outer_item = rSet.at(sidx);
			uint pos = 0;
			if(!mdr_list.lsearch(&r_outer_item.FactorQuant, &pos, PTR_CMPFUNC(uint)))
				mdr_list.insert(&r_outer_item);
			else
				assert(mdr_list.at(pos).FactorQuant == r_outer_item.FactorQuant);
		}
		mdr_list.sort(PTR_CMPFUNC(uint));
		const uint mdrc = mdr_list.getCount();
		if(mdrc > 1) {
			IntRange local_md_range;
			double max_result = -MAXDOUBLE;
			int    best_idx = -1;
			for(uint j = 0; j < mdrc; j++) {
				const double local_mdr_result = mdr_list.at(j).Result;
				if(max_result < local_mdr_result) {
					max_result = local_mdr_result;
					best_idx = static_cast<int>(j);
				}
			}
			assert(best_idx >= 0);
			const uint low_idx = static_cast<uint>((best_idx > 0) ? (best_idx-1) : best_idx);
			const uint upp_idx = static_cast<uint>(((best_idx+1) < (int)mdrc) ? (best_idx+1) : best_idx);
			local_md_range.Set(mdr_list.at(low_idx).FactorQuant, mdr_list.at(upp_idx).FactorQuant);
			assert(local_md_range.upp > local_md_range.low);
			THROW(local_md_range.upp > local_md_range.low);
			const  int range_divider = (local_md_range == rMdRange) ? 5 : 4;
			int    local_md_step = (local_md_range.upp - local_md_range.low) / range_divider;
			if(local_md_step > 0) {
				int   rr = FindOptimalFactor(rTmList, rValList, rS, what, local_md_range, local_md_step, 0, rSet, rResult); // @recursion
				THROW(rr);
				ok = rr;
			}
			else {
				rResult = mdr_list.at((best_idx >= 0 && best_idx < (int)mdrc) ? best_idx : 0);
				ok = 1;
			}
		}
		else if(mdrc) {
			rResult = mdr_list.at(0);
			ok = 1;
		}
		if(ok > 0 && the_first_call) {
			if(what == 0)
				log_buf.Z().Cat("!OptimalMaxDuck");
			else
				log_buf.Z().Cat("!OptimalPeak").Space().CatChar('[').CatEq("MaxDuck", rS.MaxDuckQuant).CatChar(']');
			log_buf.CatDiv(':', 2).Cat(rS.Symb).Space().CatEq((what == 0) ? "MaxDuckQuant" : "PeakQuant", rResult.FactorQuant).Space().
				CatEq("Result", rResult.Result, MKSFMTD(15, 5, 0));
			PPLogMessage(log_file_name, log_buf, LOGMSGF_TIME);
		}
	}
	CATCHZOK
	return ok;
}

PPObjTimeSeries::OptimalFactorRange::OptimalFactorRange() : Count(0), Result(0.0), Opt2Stride(0)
{
	RealRange::Z();
}

PPObjTimeSeries::OptimalFactorRange & PPObjTimeSeries::OptimalFactorRange::Z()
{
	RealRange::Z();
	Opt2Stride = 0;
	Count = 0;
	Result = 0.0;
	return *this;
}

PPObjTimeSeries::OptimalFactorRange_WithPositions::OptimalFactorRange_WithPositions() : OptimalFactorRange(), LoPos(0), UpPos(0), GenSeq(0), GenPtCount(0)
{
}

PPObjTimeSeries::OptimalFactorRange_WithPositions & PPObjTimeSeries::OptimalFactorRange_WithPositions::Z()
{
	OptimalFactorRange::Z();
	LoPos = 0;
	UpPos = 0;
	GenSeq = 0;
	GenPtCount = 0;
	return *this;
}

SLAPI PPObjTimeSeries::Strategy::Strategy() : InputFrameSize(0), Prec(0), TargetQuant(0), MaxDuckQuant(0), OptDelta2Stride(0), StakeMode(0),
	BaseFlags(0), Margin(0.0), SpikeQuant_s(0.0), StakeDistMedian(0), SpreadAvg(0.0), StakeCount(0), WinCount(0),
	StakeCloseMode(0), PeakAvgQuant(0), BottomAvgQuant(0), PeakMaxQuant(0), ID(0), TrendErrAvg(0.0), TrendErrLim(0.0), MainFrameSize(0), MainTrendErrAvg(0.0),
	MainTrendErrLim(0.0), GenSeq(0), StakeCountAtFinalSimulation(0), GenPtCount(0)
{
}

void SLAPI PPObjTimeSeries::Strategy::Reset()
{
	SpikeQuant_s = 0.0;
	MaxDuckQuant = 0;
}

SLAPI PPObjTimeSeries::StrategyContainer::FlatBlock::FlatBlock() : MainTrendErrAvg(0.0), AvgLocalDeviation(0.0), UseDataForStrategiesTill(ZERODATE), 
	MinSimilItems(0)
{
	memzero(Reserve, sizeof(Reserve));
}

SLAPI PPObjTimeSeries::StrategyContainer::StrategyContainer() : TSVector <Strategy>(), Ver(4), StorageTm(ZERODATETIME), LastValTm(ZERODATETIME)
{
}

SLAPI PPObjTimeSeries::StrategyContainer::StrategyContainer(const PPObjTimeSeries::StrategyContainer & rS) : TSVector <Strategy>(rS),
	Ver(rS.Ver), StorageTm(rS.StorageTm), LastValTm(rS.LastValTm), Fb(rS.Fb)
{
}

PPObjTimeSeries::StrategyContainer & FASTCALL PPObjTimeSeries::StrategyContainer::operator = (const PPObjTimeSeries::StrategyContainer & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL PPObjTimeSeries::StrategyContainer::Copy(const PPObjTimeSeries::StrategyContainer & rS)
{
	int    ok = TSVector <PPObjTimeSeries::Strategy>::copy(rS);
	Ver = rS.Ver;
	StorageTm = rS.StorageTm;
	LastValTm = rS.LastValTm;
	Fb = rS.Fb;
	return ok;
}

struct Ts_Strategy_Before10612 { // @flat @persistent
	SLAPI  Ts_Strategy_Before10612()
	{
		THISZERO();
	}
	void CopyTo(PPObjTimeSeries::Strategy & rD) const
	{
		#define F(f) rD.f = f
			F(InputFrameSize);
			F(Prec);
			F(TargetQuant);
			F(MaxDuckQuant);
			F(OptDelta2Stride);
			F(StakeMode);
			F(BaseFlags);
			F(Margin);
			F(SpikeQuant_s);
			F(SpreadAvg);
			F(OptDeltaRange);
			F(OptDelta2Range);
			F(StakeCount);
			F(WinCount);
			F(V);
			F(StakeCloseMode);
			F(PeakAvgQuant);
			F(BottomAvgQuant);
			F(PeakMaxQuant);
			F(ID);
			F(TrendErrAvg);
			F(TrendErrLim);
			F(MainFrameSize);
		#undef F
	}

	enum {
		bfShort      = 0x0001, // Стратегия для short-торговли
	};
	enum {
		clsmodFullMaxDuck = 0, //
		clsmodAdjustLoss  = 1  //
	};
	uint32 InputFrameSize;
	int16  Prec;
	uint16 TargetQuant;
	uint16 MaxDuckQuant;
	uint16 OptDelta2Stride;
	int16  StakeMode;
	uint32 BaseFlags;
	double Margin;
	double SpikeQuant_s;
	double SpreadAvg;
	double Reserve3; // @reserve
	PPObjTimeSeries::OptimalFactorRange OptDeltaRange;
	PPObjTimeSeries::OptimalFactorRange OptDelta2Range;
	uint32 StakeCount;
	uint32 WinCount;
	PPObjTimeSeries::StrategyResultValue V;
	uint16 StakeCloseMode;
	uint16 PeakAvgQuant;
	uint16 BottomAvgQuant;
	uint16 PeakMaxQuant;
	uint32 ID;
	double TrendErrAvg;
	double TrendErrLim;
	uint32 MainFrameSize;
};

int SLAPI PPObjTimeSeries::StrategyContainer::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	uint32 ver = Ver;
	if(!StorageTm.d)
		StorageTm = getcurdatetime_();
	THROW_SL(pSCtx->Serialize(dir, ver, rBuf));
	if(dir < 0 && ver < Ver)
		ok = 2;
	THROW_SL(pSCtx->Serialize(dir, StorageTm, rBuf));
	THROW_SL(pSCtx->Serialize(dir, LastValTm, rBuf));
	if(dir >= 0 || ver >= 4)
		THROW_SL(pSCtx->SerializeBlock(dir, sizeof(Fb), &Fb, rBuf, 1)); 
	if(dir < 0 && ver < 4) {
		SVector::clear();
		TSVector <Ts_Strategy_Before10612> old_ver_vec;
		THROW_SL(pSCtx->Serialize(dir, &old_ver_vec, rBuf));
		for(uint i = 0; i < old_ver_vec.getCount(); i++) {
			Strategy new_item;
			old_ver_vec.at(i).CopyTo(new_item);
			THROW_SL(SVector::insert(&new_item));
		}
	}
	else {
		THROW_SL(pSCtx->Serialize(dir, static_cast<SVector *>(this), rBuf));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTimeSeries::StrategyContainer::SetupByTsPacket(const PPTimeSeriesPacket & rTsPack, const PPTssModelPacket * pModel)
{
	int    ok = 1;
	Fb.AvgLocalDeviation = rTsPack.E.AvgLocalDeviation;
	Fb.MinSimilItems = (pModel && pModel->Rec.MinSimilItems > 1) ? pModel->Rec.MinSimilItems : 0;
	return ok;
}

void SLAPI PPObjTimeSeries::StrategyContainer::SetUseDataForStrategiesTill(LDATE dt)
{
	Fb.UseDataForStrategiesTill = checkdate(dt) ? dt : ZERODATE;
}

void PPObjTimeSeries::StrategyContainer::SetLastValTm(LDATETIME dtm)
{
	LastValTm = dtm;
}

void SLAPI PPObjTimeSeries::StrategyContainer::TrimByVariance(double bound)
{
	int   do_next = 0;
	do {
		do_next = 0;
		StatBase sb;
		uint stcnt_min = UINT_MAX;
		uint stcnt_min_idx = 0;
		for(uint i = 0; i < getCount(); i++) {
			const PPObjTimeSeries::Strategy & r_s = at(i);
			const uint stcnt = r_s.StakeCount;
			if(stcnt_min > stcnt) {
				stcnt_min = stcnt;
				stcnt_min_idx = i+1;
			}
			sb.Step(static_cast<double>(stcnt));
		}
		sb.Finish();
		double vc = sb.GetStdDev() / sb.GetExp();
		if(vc > bound && stcnt_min_idx) {
			atFree(stcnt_min_idx-1);
			do_next = 1;
		}
	} while(do_next);
}

PPObjTimeSeries::StrategyContainer::IndexEntry1::Range::Range(const RealRange & rR, uint idx) : Idx(idx), RealRange(rR)
{
}

PPObjTimeSeries::StrategyContainer::IndexEntry1::IndexEntry1() : Stride(0)
{
}

int SLAPI PPObjTimeSeries::StrategyContainer::CreateIndex1(PPObjTimeSeries::StrategyContainer::Index1 & rIndex) const
{
	int    ok = 1;
	rIndex.freeAll();
	for(uint si = 0; si < getCount(); si++) {
		const PPObjTimeSeries::Strategy & r_s = at(si);
		if(r_s.StakeMode == 1 && r_s.V.TmSec > 0) {
			IndexEntry1 * p_entry = 0;
			for(uint i = 0; !p_entry && i < rIndex.getCount(); i++) {
				IndexEntry1 * p_temp_entry = rIndex.at(i);
				if(p_temp_entry->Stride == r_s.InputFrameSize)
					p_entry = p_temp_entry;
			}
			if(!p_entry) {
				THROW_SL(p_entry = rIndex.CreateNewItem());
				p_entry->Stride = r_s.InputFrameSize;
			}
			{
				IndexEntry1::Range rng(r_s.OptDeltaRange, si);
				p_entry->RangeList.insert(&rng);
			}
		}
	}
	if(rIndex.getCount()) {
		rIndex.sort(PTR_CMPFUNC(uint));
		for(uint i = 0; i < rIndex.getCount(); i++) {
			IndexEntry1 * p_temp_entry = rIndex.at(i);
			p_temp_entry->RangeList.sort(PTR_CMPFUNC(double));
		}
	}
	CATCHZOK
	return ok;
}

SLAPI  PPObjTimeSeries::StrategyContainer::SelectBlock::SelectBlock(const TSCollection <TrendEntry> & rTrendList, const Index1 & rIndex) : 
	R_TrendList(rTrendList), LastTrendIdx(0), Criterion(0),
	R_Index(rIndex), P_Ts(0), P_VList(0), DevPtCount(0), LDMT_Factor(0), MainTrendMaxErrRel(0.0f), Reserve(0)
{
}

PPObjTimeSeries::StrategyContainer::SelectBlock & FASTCALL PPObjTimeSeries::StrategyContainer::SelectBlock::Init(int lastTrendIdx)
{
	BestStrategyBlock::Z();
	LastTrendIdx = lastTrendIdx;
	return *this;
}

int SLAPI PPObjTimeSeries::StrategyContainer::IsThereSimilStrategy(uint thisIdx, const LongArray & rSelectedIdxList, LongArray & rSimilIdxList) const
{
	int    yes = 0;
	rSimilIdxList.clear();
	if(thisIdx < getCount()) {
		const Strategy & r_s = at(thisIdx);
		for(uint i = 0; i < rSelectedIdxList.getCount(); i++) {
			const uint idx = static_cast<uint>(rSelectedIdxList.get(i));
			if(idx != thisIdx && idx < getCount()) {
				const Strategy & r_other_s = at(idx);
				if((r_s.BaseFlags & Strategy::bfShort) == (r_other_s.BaseFlags & Strategy::bfShort)) {
					if(r_s.InputFrameSize != r_other_s.InputFrameSize) {
						rSimilIdxList.add(static_cast<long>(idx));
						yes = 1;
					}
				}
			}
		}
	}
	return yes;
}

int SLAPI PPObjTimeSeries::StrategyContainer::SelectS2(SelectBlock & rBlk) const
{
	int    ok = -1;
	uint   potential_long_count = 0;
	uint   potential_short_count = 0;
	rBlk.AllSuitedPosList.clear();
	const  int last_trend_idx = rBlk.LastTrendIdx;
	{
		const TrendEntry * p_main_te_last = 0; // Импровизированное кэширование последнего найденного магистрального тренда
		const int is_single = BIN(getCount() == 1); //(rBlk.R_Index.getCount() == 1);
		for(uint tidx = 0; tidx < rBlk.R_TrendList.getCount(); tidx++) {
			const TrendEntry * p_te = rBlk.R_TrendList.at(tidx);
			const IndexEntry1 * p_ie = 0;
			if(rBlk.R_Index.getCount()) { // @v10.7.7
				if(is_single && rBlk.R_Index.at(0)->Stride == p_te->Stride)
					p_ie = rBlk.R_Index.at(0);
				else {
					uint  ip = 0;
					if(rBlk.R_Index.bsearch(&p_te->Stride, &ip, CMPF_LONG)) 
						p_ie = rBlk.R_Index.at(ip);
				}
			}
			if(p_ie) {
				assert(p_ie->Stride == p_te->Stride);
				const uint tlc = p_te->TL.getCount();
				assert((last_trend_idx < 0 && (-last_trend_idx) <= static_cast<int>(tlc)) || (last_trend_idx >= 0 && last_trend_idx < static_cast<int>(tlc)));
				const uint trend_idx = (last_trend_idx < 0) ? (tlc+last_trend_idx) : static_cast<uint>(last_trend_idx);
				const double tv = p_te->TL.at(trend_idx);
				const uint range_list_count = p_ie->RangeList.getCount();
				assert(!is_single || range_list_count == 1); // @v10.7.3
				for(uint ridx = 0; ridx < range_list_count; ridx++) {
					const IndexEntry1::Range & r_ri = p_ie->RangeList.at(ridx);
					if(r_ri.low <= tv && r_ri.upp >= tv) {
						const Strategy & r_s = at(r_ri.Idx);
						assert(r_s.InputFrameSize == p_ie->Stride);
						assert(r_s.OptDeltaRange.Check(tv));
						const double trend_err = p_te->ErrL.at(trend_idx);
						const double trend_err_limit = (r_s.TrendErrLim * r_s.TrendErrAvg);
						if(trend_err_limit <= 0.0 || (trend_err > 0.0 && trend_err <= trend_err_limit)) {
							bool do_skip = false;
							if(r_s.BaseFlags & r_s.bfShort) {
								potential_short_count++;
								if(rBlk.Criterion & selcritfSkipShort)
									do_skip = true;
							}
							else {
								potential_long_count++;
								if(rBlk.Criterion & selcritfSkipLong)
									do_skip = true;
							}
							if(!do_skip) {
								if(r_s.MainFrameSize) {
									const TrendEntry * p_main_te = 0;
									if(p_main_te_last && p_main_te_last->Stride == r_s.MainFrameSize)
										p_main_te = p_main_te_last;
									else
										p_main_te = SearchTrendEntry(rBlk.R_TrendList, r_s.MainFrameSize);
									if(p_main_te) {
										p_main_te_last = p_main_te;
										double main_trend_val = p_main_te->TL.at(trend_idx);
										if(!r_s.OptDelta2Range.Check(main_trend_val))
											do_skip = true;
										else {
											const double main_trend_err = p_main_te->ErrL.at(trend_idx); 
											const double main_trend_err_limit = (r_s.MainTrendErrLim * r_s.MainTrendErrAvg);
											if(main_trend_err_limit > 0.0 && (main_trend_err > 0.0 && main_trend_err > main_trend_err_limit))
												do_skip = true;
										}
									}
									else
										do_skip = true;
								}
								if(!do_skip) {
									long lpos = static_cast<long>(r_ri.Idx);
									rBlk.AllSuitedPosList.insert(&lpos);
								}
							}
						}
					}
					else if(r_ri.low > tv) { // Диапазоны отсортированы - дальше искать нечего
						break;
					}
				}
				// @v10.7.3 больше нечего делать в этом цикле раз для единственной стратегии тренд рассмотрен {
				if(is_single) 
					break;
				// } @v10.7.3
			}
		}
	}
	const uint rc = rBlk.AllSuitedPosList.getCount();
	const uint min_sim_items = (Fb.MinSimilItems > 0 && !(rBlk.Criterion & selcritfSingle)) ? Fb.MinSimilItems : 1;
	if(rc >= min_sim_items) {
		bool skip_short = false;
		bool skip_long = false;
		if(potential_long_count && potential_short_count) {
			if(rBlk.Criterion & selcritfSkipAmbiguous) {
				skip_short = true;
				skip_long = true;
				ok = -2;
			}
			else if(rBlk.Criterion & selcritfWeightAmbiguous) {
				if(potential_long_count > potential_short_count)
					skip_short = true;
				else if(potential_short_count > potential_long_count)
					skip_long = true;
				else {
					skip_short = true;
					skip_long = true;
					ok = -2;
				}
			}
		}
		if(!skip_long || !skip_short) {
			LongArray simil_idx_list;
			for(uint clidx = 0; clidx < rc; clidx++) {
				const uint sidx = static_cast<uint>(rBlk.AllSuitedPosList.get(clidx));
				const Strategy & r_s = at(sidx);
				if((r_s.BaseFlags & r_s.bfShort && !skip_short) || (!(r_s.BaseFlags & r_s.bfShort) && !skip_long)) {
					if(min_sim_items <= 1 || IsThereSimilStrategy(sidx, rBlk.AllSuitedPosList, simil_idx_list)) { // @v10.7.7
						double local_result = 0.0;
						switch(rBlk.Criterion & 0xffL) {
							case selcritVelocity: local_result = r_s.V.GetResultPerDay(); break;
							case selcritWinRatio: local_result = r_s.GetWinCountRate(); break;
							case selcritResult: local_result = r_s.V.Result; break;
						}
						if(local_result > 0.0) {
							{
								const TrendEntry * p_te = SearchTrendEntry(rBlk.R_TrendList, r_s.InputFrameSize);
								const TrendEntry * p_main_te = r_s.MainFrameSize ? SearchTrendEntry(rBlk.R_TrendList, r_s.MainFrameSize) : 0;
								const uint tlc = p_te ? p_te->TL.getCount() : 0;
								const uint trend_idx = (last_trend_idx < 0) ? (tlc-1) : static_cast<uint>(last_trend_idx);
								const double trend_err = p_te->ErrL.at(trend_idx);
								const double tv = p_te ? p_te->TL.at(trend_idx) : 0.0;
								const double tv2 = p_main_te ? p_main_te->TL.at(trend_idx) : 0.0; // @v10.6.12

								// @v10.7.1 {
								const double main_trend_err = p_main_te ? p_main_te->ErrL.at(trend_idx) : 0.0;
								const double main_trend_err_rel = fdivnz(main_trend_err, r_s.MainTrendErrAvg);
								const double trend_err_rel = fdivnz(trend_err, r_s.TrendErrAvg);
								double local_std_dev = 0.0;
								double local_deviation = 0.0;
								double local_deviation2 = 0.0;
								if(rBlk.MainTrendMaxErrRel <= 0.0f || main_trend_err_rel <= static_cast<double>(rBlk.MainTrendMaxErrRel)) { // @v10.7.1
									if(rBlk.DevPtCount > 0 && tlc) {
										if(rBlk.P_VList) {
											if(rBlk.P_VList->getCount() == tlc) {
												if(static_cast<long>(trend_idx) >= (rBlk.DevPtCount + 1)) {
													StatBase sb(0);
													for(uint vi = (trend_idx - rBlk.DevPtCount + 1); vi <= trend_idx; vi++) {
														sb.Step(rBlk.P_VList->at(vi));
													}
													sb.Finish();
													{
														local_std_dev = sb.GetStdDev();
														local_deviation = (local_std_dev / sb.GetExp()) * 1000.0;
														if(Fb.AvgLocalDeviation > 0.0)
															local_deviation2 = local_std_dev / Fb.AvgLocalDeviation;
													}
												}
											}
										}
										else if(rBlk.P_Ts) {
											//
											// Особый случай, обрабатываемый в run-time: размерность вектора тренда очень короткая,
											// индекс позиции в векторе исходных значений можно отсчитать только с конца (last_trend_idx < 0).
											//
											const uint tsc = rBlk.P_Ts->GetCount();
											if(tsc >= tlc && (last_trend_idx < 0)) {
												const uint ts_idx = (tsc-1);
												if(static_cast<long>(ts_idx) >= (rBlk.DevPtCount + 1)) {
													uint vec_idx = 0;
													if(rBlk.P_Ts->GetValueVecIndex("close", &vec_idx)) {
														RealArray vl;
														if(rBlk.P_Ts->GetRealArray(vec_idx, (ts_idx - rBlk.DevPtCount + 1), rBlk.DevPtCount, vl)) {
															StatBase sb(0);
															sb.Step(vl);
															sb.Finish();
															{
																local_std_dev = sb.GetStdDev();
																local_deviation = (local_std_dev / sb.GetExp()) * 1000.0;
																if(Fb.AvgLocalDeviation > 0.0)
																	local_deviation2 = local_std_dev / Fb.AvgLocalDeviation;
															}
														}
													}
												}
											}
										}
									}
									const double ldmt_limit = (rBlk.LDMT_Factor > 0) ? static_cast<double>(rBlk.LDMT_Factor) / 1000.0 : 3000.0 /*large unreachable value*/;
									// if((main_trend_err_rel * local_deviation) <= ldmt_limit/*0.055*/) {
									// 
									// Проверка на условие непревышения локальной девиацией предельного значения, заданного в конфигурации.
									// Кроме того, если количество точек для замера локальной девиации задано, но она тем не менее 0, то 
									// в этом случае ставку делать нельзя (плоский участок без движения - есть основания полагать что это очень
									// рискованная позиция для ставки).
									// 
									//if(local_deviation2 <= ldmt_limit && ((rBlk.DevPtCount <= 0 || local_deviation2 > 0.0))) {
									if((main_trend_err_rel * local_deviation) <= ldmt_limit/*0.055*/) {
										// } @v10.7.1 
										rBlk.SetResult(local_result, sidx, tv, tv2);
										rBlk.TrendErr = trend_err;
										rBlk.TrendErrRel = trend_err_rel;
										rBlk.MainTrendErr = main_trend_err;
										rBlk.MainTrendErrRel = main_trend_err_rel;
										rBlk.LocalDeviation = local_deviation;
										rBlk.LocalDeviation2 = local_deviation2;
									}
								}
							}
						}
					}
				}
			}
			if(rBlk.MaxResult > 0.0)
				ok = 1;
		}
	}
	return ok;
}

SLAPI PPObjTimeSeries::StrategyContainer::CritEntry::CritEntry(uint idx) : Idx(idx), Crit1(0.0), Crit2(0.0), Crit3(0.0), StakeCount(0)
{
}

static IMPL_CMPFUNC(StrategyCritEntry, i1, i2)
{
	const PPObjTimeSeries::StrategyContainer::CritEntry * p1 = static_cast<const PPObjTimeSeries::StrategyContainer::CritEntry *>(i1);
	const PPObjTimeSeries::StrategyContainer::CritEntry * p2 = static_cast<const PPObjTimeSeries::StrategyContainer::CritEntry *>(i2);
	int   si = 0;
	CMPCASCADE3(si, p2, p1, Crit1, Crit2, Crit3); // Нам нужно чтобы большие значения были вверху (потому (p2, p1) вместо (p1, p2))
	return si;
}

static IMPL_CMPFUNC(StrategyCritEntry_StakeCount, i1, i2)
{
	const PPObjTimeSeries::StrategyContainer::CritEntry * p1 = static_cast<const PPObjTimeSeries::StrategyContainer::CritEntry *>(i1);
	const PPObjTimeSeries::StrategyContainer::CritEntry * p2 = static_cast<const PPObjTimeSeries::StrategyContainer::CritEntry *>(i2);
	int   si = CMPSIGN(p2->StakeCount, p1->StakeCount); // обратный порядок: больше ставок - меньше индекс
	return si;
}

int SLAPI PPObjTimeSeries::StrategyContainer::MakeOrderIndex(long flags, TSArray <PPObjTimeSeries::StrategyContainer::CritEntry> & rIndex) const
{
	int    ok = 1;
	rIndex.clear();
	for(uint i = 0; i < getCount(); i++) {
		THROW(AddStrategyToOrderIndex(i, flags, rIndex));
	}
	rIndex.sort(PTR_CMPFUNC(StrategyCritEntry));
	CATCHZOK
	return ok;
}

int SLAPI PPObjTimeSeries::StrategyContainer::AddStrategyToOrderIndex(uint pos, long flags, TSArray <PPObjTimeSeries::StrategyContainer::CritEntry> & rIndex) const
{
	int    ok = 1;
	if(pos >= 0 && pos < getCount()) {
		const Strategy & r_item = at(pos);
		const double result_per_day = r_item.V.GetResultPerDay();
		const double result_total = r_item.V.Result;
		CritEntry crit_entry(pos+1);
		crit_entry.StakeCount = r_item.StakeCount; // @v10.7.8
		if(flags & gbsfCritProb) {
			crit_entry.Crit1 = r_item.GetWinCountRate();
			crit_entry.Crit2 = result_total; // @v10.7.9 result_per_day-->result_total
		}
		else if(flags & gbsfCritProfitMultProb)
			crit_entry.Crit1 = result_per_day * r_item.GetWinCountRate();
		else if(flags & gbsfCritTotalResult) {
			crit_entry.Crit1 = result_total;
			crit_entry.Crit2 = r_item.GetWinCountRate();
		}
		else if(flags & gbsfOptDeltaRangeCQA) {
			crit_entry.Crit1 = r_item.OptDelta2Range.GetMiddle();
			crit_entry.Crit2 = r_item.GetOptDeltaRangeCQA();
			crit_entry.Crit3 = result_total;
		}
		else if(flags & gbsfCritStakeCount) { // @v10.7.9
			crit_entry.Crit1 = static_cast<double>(r_item.StakeCount);
			crit_entry.Crit2 = result_total;
		}
		else
			crit_entry.Crit1 = result_total; // @v10.7.9 result_per_day-->result_total
		THROW_SL(rIndex.insert(&crit_entry));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPObjTimeSeries::StrategyContainer::GetBestSubset2(const PPTssModelPacket & rTssPacket, 
	StrategyContainer & rScDest, StrategyContainer * pScSkipDueDup, LongArray * pToRemovePosList, LongArray * pHaveAnalogPosList) const
{
	rScDest.clear();
	int    ok = 1;
	long _ssflags = gbsfLong|gbsfShort|gbsfStakeMode1; // @v10.8.0 @unused |gbsfStakeMode3|gbsfStakeMode2;
	const uint _max_count = rTssPacket.Rec.BestSubsetDimention;
	const double _min_win_rate = rTssPacket.Rec.MinWinRate;
	const uint _cqa_match_promille = (rTssPacket.Rec.CqaMatchPromille > 0) ? rTssPacket.Rec.CqaMatchPromille : /*PPTssModel::Default_CqaMatchPromille*/0;
	{
		//
		// Если (tss_model.Rec.MinSimilItems > 1) то удалять схожие по CQA-фактору стратегии нельзя - они должны работать вместе
		//
		if((rTssPacket.Rec.MinSimilItems <= 1) && _cqa_match_promille) // @v10.7.7 
			_ssflags |= gbsfEliminateCQADups; // @v10.7.4
		if(rTssPacket.Rec.Flags & PPTssModel::fBestSubsetTrendFollowing)
			_ssflags |= gbsfTrendFollowing;
		switch(rTssPacket.Rec.StrategyPoolSortOrder) {
			case PPTssModel::sppsResult:     _ssflags |= gbsfCritTotalResult; break;
			case PPTssModel::sppsProb:       _ssflags |= gbsfCritProb; break;
			case PPTssModel::sppsVelocity:   _ssflags |= gbsfCritProfitMultProb; break; // !Не соответствует выбор флагу
			case PPTssModel::sppsCqaFactor:  _ssflags |= gbsfOptDeltaRangeCQA; break;
			case PPTssModel::sppsShuffle:    _ssflags |= gbsfShuffle; break;
			case PPTssModel::sppsStakeCount: _ssflags |= gbsfCritStakeCount; break; // @v10.7.9
		}
		_ssflags |= gbsfEliminate100prob; // @v10.7.9
	}
	TSArray <PPObjTimeSeries::StrategyContainer::CritEntry> range_list_;
	TSArray <PPObjTimeSeries::StrategyContainer::CritEntry> range_list_cqa;
	const uint _c = getCount();
	LongArray pos_to_remove;
	LongArray have_analog_pos_list;
	// @v10.7.8 {
	// Еще одно экспериментальное соображение: уберем из списка стратегий ту половину, которая имеет (наименьшее количество ставок) наименьшую результативность.
	// Подоплека такова: малое количество ставок имеет меньшую статистическую значимость и вероятность "заваливания" таких стратегий высока.
	///* Вердикт: (лучше не стало (скорее, хуже)) - а вот при сортировке по результату, кажется, стало лучше!
#if 0 // {
	{
		TSArray <PPObjTimeSeries::StrategyContainer::CritEntry> range_list_by_stakecount;
		for(uint i = 0; i < _c; i++) {
			const Strategy & r_item = at(i);
			THROW(AddStrategyToOrderIndex(i, /*gbsfOptDeltaRangeCQA*/gbsfCritTotalResult, range_list_by_stakecount));
		}
		range_list_by_stakecount.sort(PTR_CMPFUNC(StrategyCritEntry_StakeCount));
		for(uint j = 0; j < range_list_by_stakecount.getCount(); j++) {
			const uint _pos = range_list_by_stakecount.at(j).Idx-1;
			if(_pos > static_cast<uint>(static_cast<double>(_c) * 0.7))
				pos_to_remove.add(static_cast<long>(_pos));
		}
	}
#endif // } 0
	//*/
	// } @v10.7.8 
	// @v10.7.9 {
	if(_ssflags & gbsfEliminate100prob) {
		// @20200528 {
		// Будем вырезать стратегии, у которых количество ставок меньше, чем среднее по всей выборке, попавшей 
		// в изначальный набор. Это, в общем, улучшает результат по сравнению с удалением только 100%-ных стратегий.
		uint i;
		if(_c <= 3) {
			// Если стратегий 3 или меньше, то оставляем только те, у которых максимальное количество ставок 
			uint max_stake_count = 0;
			for(i = 1; i < _c; i++) {
				const Strategy & r_item = at(i);
				SETMAX(max_stake_count, r_item.StakeCount);
				//pos_to_remove.add(static_cast<long>(i));
			}
			if(max_stake_count) {
				for(i = 1; i < _c; i++) {
					const Strategy & r_item = at(i);
					if(r_item.StakeCount < max_stake_count)
						pos_to_remove.add(static_cast<long>(i));
				}
			}
		}
		else {
			const double max_vc = 0.12;
			// @v10.7.11 Экспериментальная попытка улучшить результат посредством ограничения фактора доверия CalcConfidenceFactor(). 
			// Вердикт: успеха не принесла.
			const double min_confidence_factor = 0.0/*500.0*/; 
			if(max_vc > 0.0) {
				// @v10.7.11 {
				if(min_confidence_factor > 0.0) {
					for(i = 0; i < _c; i++) {
						const Strategy & r_item = at(i);
						if(r_item.CalcConfidenceFactor() < min_confidence_factor)
							pos_to_remove.add(static_cast<long>(i));
					}
				}
				// } @v10.7.11 
				double vc = EvaluateScore(scoreEvStakeCountVarCoeff, &pos_to_remove);
				while(vc > max_vc) {
					uint min_stake_count = UINT_MAX;
					uint min_stake_count_pos = 0;
					for(i = 0; i < _c; i++) {
						if(!pos_to_remove.lsearch(i)) {
							const Strategy & r_item = at(i);
							if(min_stake_count > r_item.StakeCount) {
								min_stake_count = r_item.StakeCount;
								min_stake_count_pos = i;
							}
						}
					}
					if(min_stake_count < UINT_MAX) {
						pos_to_remove.add(static_cast<long>(min_stake_count_pos));
						vc = EvaluateScore(scoreEvStakeCountVarCoeff, &pos_to_remove);
					}
					else
						break;
				}
			}
			else {
				StatBase sb;
				for(i = 0; i < _c; i++) {
					sb.Step(static_cast<double>(at(i).StakeCount));
				}
				sb.Finish();
				const double avg_stk_cnt = sb.GetExp();
				for(i = 0; i < _c; i++) {
					const Strategy & r_item = at(i);
					if(static_cast<double>(r_item.StakeCount) < (avg_stk_cnt * 2)) {
						pos_to_remove.add(static_cast<long>(i));
					}
				}
			}
		}
		// } @20200528 
	}
	// } @v10.7.9 
	if(_ssflags & gbsfEliminateCQADups) {
		for(uint i = 0; i < _c; i++) {
			if(!pos_to_remove.lsearch(i)) { // @v10.7.8
				const Strategy & r_item = at(i);
				if(r_item.V.Result > 0.0 && r_item.GetWinCountRate() >= _min_win_rate) {
					int    skip = 0;
					if(_ssflags & gbsfTrendFollowing && r_item.StakeMode == 1) {
						if(r_item.BaseFlags & r_item.bfShort) {
							if(!r_item.OptDeltaRange.LessThan(0.0))
								skip = 1;
						}
						else {
							if(!r_item.OptDeltaRange.GreaterThan(0.0))
								skip = 1;
						}
					}
					if(!skip) {
						if(((r_item.BaseFlags & r_item.bfShort) && (_ssflags & gbsfShort)) || (!(r_item.BaseFlags & r_item.bfShort) && (_ssflags & gbsfLong))) {
							if(r_item.StakeMode == 1 && _ssflags & gbsfStakeMode1) {
								THROW(AddStrategyToOrderIndex(i, gbsfOptDeltaRangeCQA, range_list_cqa));
							}
						}
					}
				}
			}
		}
		range_list_cqa.sort(PTR_CMPFUNC(StrategyCritEntry));
		// @v10.7.5 {
		{
			for(uint ridx = 0; ridx < range_list_cqa.getCount(); ridx++) {
				const CritEntry & r_range_item_ = range_list_cqa.at(ridx);
				const uint pos = static_cast<uint>(r_range_item_.Idx-1);
				assert(pos < _c);
				if(!pos_to_remove.lsearch(static_cast<long>(pos)) && !have_analog_pos_list.lsearch(static_cast<long>(pos))) { // @v10.7.8 !pos_to_remove.lsearch(static_cast<long>(pos))
					Strategy & r_item = at(pos);
					const double cqa = r_item.GetOptDeltaRangeCQA();
					int    has_analog = 0; // !0 если позиция pos имеет аналог по CQ
					for(uint ridx2 = ridx+1; ridx2 < range_list_cqa.getCount(); ridx2++) {
						const CritEntry & r_range_item_next = range_list_cqa.at(ridx2);
						const uint pos_next = static_cast<uint>(r_range_item_next.Idx-1);
						Strategy & r_item_next = at(pos_next);
						if(r_item_next.OptDelta2Range.RealRange::IsEqual(r_item.OptDelta2Range)) {
							if(r_item.InputFrameSize != r_item_next.InputFrameSize) {
								const double cqa_next = r_item_next.GetOptDeltaRangeCQA();
								const double cqam = (cqa_next + cqa) / 2.0;
								const uint diff = static_cast<uint>(R0(fabs((cqa_next - cqa) / cqam) * 1000.0));
								if(diff <= _cqa_match_promille) {
									has_analog = 1;
									have_analog_pos_list.add(static_cast<long>(pos_next));
								}
							}
						}
					}
					if(has_analog)
						have_analog_pos_list.add(static_cast<long>(pos));
				}
			}
		}
		// } @v10.7.5 
		{
			for(uint ridx = 0; ridx < range_list_cqa.getCount(); ridx++) {
				const CritEntry & r_range_item_ = range_list_cqa.at(ridx);
				const uint pos = static_cast<uint>(r_range_item_.Idx-1);
				assert(pos < _c);
				if(!pos_to_remove.lsearch(pos)) {
					Strategy & r_item = at(pos);
					const double cqa = r_item.GetOptDeltaRangeCQA();
					double best_crit = r_item.V.Result;
					uint   best_crit_pos = pos;
					for(uint ridx2 = ridx+1; ridx2 < range_list_cqa.getCount(); ridx2++) {
						const CritEntry & r_range_item_next = range_list_cqa.at(ridx2);
						const uint pos_next = static_cast<uint>(r_range_item_next.Idx-1);
						Strategy & r_item_next = at(pos_next);
						if(r_item_next.OptDelta2Range.RealRange::IsEqual(r_item.OptDelta2Range)) {
							const double cqa_next = r_item_next.GetOptDeltaRangeCQA();
							const double cqam = (cqa_next + cqa) / 2.0;
							const uint diff = static_cast<uint>(R0(fabs((cqa_next - cqa) / cqam) * 1000.0));
							if(diff <= _cqa_match_promille) {
								double result = r_item_next.V.Result;
								if(best_crit < result) {
									best_crit = result;
									pos_to_remove.add(static_cast<long>(best_crit_pos));
									best_crit_pos = pos_next;
								}
								else
									pos_to_remove.add(static_cast<long>(pos_next));
							}
						}
					}
				}
			}
		}
		pos_to_remove.sortAndUndup();
		have_analog_pos_list.sortAndUndup(); // @v10.7.5
	}
	{
		for(uint i = 0; i < _c; i++) {
			const Strategy & r_item = at(i);
			if(!pos_to_remove.lsearch(static_cast<long>(i)) && r_item.V.Result > 0.0 && r_item.GetWinCountRate() >= _min_win_rate) {
				// @v10.7.5 Специальное (экспериментальное) соображение из-за которого введен have_analog_pos_list:
				// если мы имеем более одной стратегии с идентичным углом наклона но разными дистанциями регрессии, то
				// это дает основание полагать, что такой угол обеспечивает более надежную стратегию. Следовательно, стратегии с углами,
				// не имеющими аналогов, будет разумным убрать из набора стратегий.
				// Это соображение базируется на предположении, что реальную вероятность выигрыша обеспечивает не сам угол 
				// наклона регрессионной прямой, а кривая (парабола, вероятно) касательной к которой является найденная регрессия.
				/*if(have_analog_pos_list.lsearch(static_cast<long>(i)))*/{
					int    skip = 0;
					if(_ssflags & gbsfTrendFollowing && r_item.StakeMode == 1) {
						if(r_item.BaseFlags & r_item.bfShort) {
							if(!r_item.OptDeltaRange.LessThan(0.0))
								skip = 1;
						}
						else {
							if(!r_item.OptDeltaRange.GreaterThan(0.0))
								skip = 1;
						}
					}
					if(!skip) {
						if(((r_item.BaseFlags & r_item.bfShort) && (_ssflags & gbsfShort)) || (!(r_item.BaseFlags & r_item.bfShort) && (_ssflags & gbsfLong))) {
							if(r_item.StakeMode == 1 && _ssflags & gbsfStakeMode1) {
								THROW(AddStrategyToOrderIndex(i, _ssflags, range_list_));
							}
						}
					}
				}
			}
		}
		if(_ssflags & gbsfShuffle)
			range_list_.shuffle();
		else
			range_list_.sort(PTR_CMPFUNC(StrategyCritEntry));
		for(uint ridx = 0; ridx < range_list_.getCount() && rScDest.getCount() < _max_count; ridx++) {
			const CritEntry & r_range_item_ = range_list_.at(ridx);
			const uint pos = static_cast<uint>(r_range_item_.Idx-1);
			assert(pos < _c);
			const Strategy & r_item = at(pos);
			int   do_skip = 0;
			const double _eps = 1E-11;
			if(_ssflags & gbsfEliminateDups && r_item.StakeMode == 1) {
				for(uint j = 0; !do_skip && j < rScDest.getCount(); j++) {
					const Strategy & r_j_item = rScDest.at(j);
					if(r_j_item.StakeMode == r_item.StakeMode && r_j_item.InputFrameSize == r_item.InputFrameSize) {
						if(feqeps(r_j_item.OptDeltaRange.low, r_item.OptDeltaRange.low, _eps) && feqeps(r_j_item.OptDeltaRange.upp, r_item.OptDeltaRange.upp, _eps)) {
							if(r_j_item.MainFrameSize == r_item.MainFrameSize) { // @v10.6.9
								if(feqeps(r_j_item.OptDelta2Range.low, r_item.OptDelta2Range.low, _eps) && feqeps(r_j_item.OptDelta2Range.upp, r_item.OptDelta2Range.upp, _eps)) { // @v10.6.9
									if(pScSkipDueDup) {
										THROW_SL(pScSkipDueDup->insert(&r_item));
									}
									do_skip = 1;
								}
							}
						}
					}
				}
			}
			if(!do_skip) {
				THROW_SL(rScDest.insert(&r_item));
			}
		}
	}
	rScDest.Fb.AvgLocalDeviation = Fb.AvgLocalDeviation;
	rScDest.Fb.UseDataForStrategiesTill = Fb.UseDataForStrategiesTill;
	ASSIGN_PTR(pToRemovePosList, pos_to_remove);
	ASSIGN_PTR(pHaveAnalogPosList, have_analog_pos_list);
	CATCHZOK
	return ok;
}

#if 0 // @v10.7.10 (replaced with GetBestSubset2()) {
int SLAPI PPObjTimeSeries::StrategyContainer::GetBestSubset(long flags, uint maxCount, double minWinRate, uint cqaMatchPromille, StrategyContainer & rScDest, 
	StrategyContainer * pScSkipDueDup, LongArray * pToRemovePosList, LongArray * pHaveAnalogPosList) const
{
	rScDest.clear();
	int    ok = 1;
	TSArray <PPObjTimeSeries::StrategyContainer::CritEntry> range_list_;
	TSArray <PPObjTimeSeries::StrategyContainer::CritEntry> range_list_cqa;
	const uint _c = getCount();
	LongArray pos_to_remove;
	LongArray have_analog_pos_list;
	// @v10.7.8 {
	// Еще одно экспериментальное соображение: уберем из списка стратегий ту половину, которая имеет (наименьшее количество ставок) наименьшую результативность.
	// Подоплека такова: малое количество ставок имеет меньшую статистическую значимость и вероятность "заваливания" таких стратегий высока.
	///* Вердикт: (лучше не стало (скорее, хуже)) - а вот при сортировке по результату, кажется, стало лучше!
#if 0 // {
	{
		TSArray <PPObjTimeSeries::StrategyContainer::CritEntry> range_list_by_stakecount;
		for(uint i = 0; i < _c; i++) {
			const Strategy & r_item = at(i);
			THROW(AddStrategyToOrderIndex(i, /*gbsfOptDeltaRangeCQA*/gbsfCritTotalResult, range_list_by_stakecount));
		}
		range_list_by_stakecount.sort(PTR_CMPFUNC(StrategyCritEntry_StakeCount));
		for(uint j = 0; j < range_list_by_stakecount.getCount(); j++) {
			const uint _pos = range_list_by_stakecount.at(j).Idx-1;
			if(_pos > static_cast<uint>(static_cast<double>(_c) * 0.7))
				pos_to_remove.add(static_cast<long>(_pos));
		}
	}
#endif // } 0
	//*/
	// } @v10.7.8 
	// @v10.7.9 {
	if(flags & gbsfEliminate100prob) {
		// @20200528 {
		// Будем вырезать стратегии, у которых количество ставок меньше, чем среднее по всей выборке, попавшей 
		// в изначальный набор. Это, в общем, улучшает результат по сравнению с удалением только 100%-ных стратегий.
		{
			StatBase sb;
			uint i;
			for(i = 0; i < _c; i++) {
				sb.Step(static_cast<double>(at(i).StakeCount));
			}
			sb.Finish();
			const double avg_stk_cnt = sb.GetExp();
			for(i = 0; i < _c; i++) {
				const Strategy & r_item = at(i);
				if(static_cast<double>(r_item.StakeCount) < (avg_stk_cnt)) {
					pos_to_remove.add(static_cast<long>(i));
				}
			}
		}
		// } @20200528 
		/* @20200528 {
			for(uint i = 0; i < _c; i++) {
				const Strategy & r_item = at(i);
				if(r_item.GetWinCountRate() >= 1.0) {
					pos_to_remove.add(static_cast<long>(i));
				}
			}
		}*/
	}
	// } @v10.7.9 
	if(flags & gbsfEliminateCQADups) {
		if(cqaMatchPromille <= 0)
			cqaMatchPromille = PPTssModel::Default_CqaMatchPromille;
		for(uint i = 0; i < _c; i++) {
			if(!pos_to_remove.lsearch(i)) { // @v10.7.8
				const Strategy & r_item = at(i);
				if(r_item.V.Result > 0.0 && r_item.GetWinCountRate() >= minWinRate) {
					int    skip = 0;
					if(flags & gbsfTrendFollowing && r_item.StakeMode == 1) {
						if(r_item.BaseFlags & r_item.bfShort) {
							if(!r_item.OptDeltaRange.LessThan(0.0))
								skip = 1;
						}
						else {
							if(!r_item.OptDeltaRange.GreaterThan(0.0))
								skip = 1;
						}
					}
					if(!skip) {
						if(((r_item.BaseFlags & r_item.bfShort) && (flags & gbsfShort)) || (!(r_item.BaseFlags & r_item.bfShort) && (flags & gbsfLong))) {
							if((r_item.StakeMode == 1 && flags & gbsfStakeMode1) || (r_item.StakeMode == 2 && flags & gbsfStakeMode2) || (r_item.StakeMode == 3 && flags & gbsfStakeMode3)) {
								THROW(AddStrategyToOrderIndex(i, gbsfOptDeltaRangeCQA, range_list_cqa));
							}
						}
					}
				}
			}
		}
		range_list_cqa.sort(PTR_CMPFUNC(StrategyCritEntry));
		// @v10.7.5 {
		{
			for(uint ridx = 0; ridx < range_list_cqa.getCount(); ridx++) {
				const CritEntry & r_range_item_ = range_list_cqa.at(ridx);
				const uint pos = static_cast<uint>(r_range_item_.Idx-1);
				assert(pos < _c);
				if(!pos_to_remove.lsearch(static_cast<long>(pos)) && !have_analog_pos_list.lsearch(static_cast<long>(pos))) { // @v10.7.8 !pos_to_remove.lsearch(static_cast<long>(pos))
					Strategy & r_item = at(pos);
					const double cqa = r_item.GetOptDeltaRangeCQA();
					int    has_analog = 0; // !0 если позиция pos имеет аналог по CQ
					for(uint ridx2 = ridx+1; ridx2 < range_list_cqa.getCount(); ridx2++) {
						const CritEntry & r_range_item_next = range_list_cqa.at(ridx2);
						const uint pos_next = static_cast<uint>(r_range_item_next.Idx-1);
						Strategy & r_item_next = at(pos_next);
						if(r_item_next.OptDelta2Range.RealRange::IsEqual(r_item.OptDelta2Range)) {
							if(r_item.InputFrameSize != r_item_next.InputFrameSize) {
								const double cqa_next = r_item_next.GetOptDeltaRangeCQA();
								const double cqam = (cqa_next + cqa) / 2.0;
								const uint diff = static_cast<uint>(R0(fabs((cqa_next - cqa) / cqam) * 1000.0));
								if(diff <= cqaMatchPromille) {
									has_analog = 1;
									have_analog_pos_list.add(static_cast<long>(pos_next));
								}
							}
						}
					}
					if(has_analog)
						have_analog_pos_list.add(static_cast<long>(pos));
				}
			}
		}
		// } @v10.7.5 
		{
			for(uint ridx = 0; ridx < range_list_cqa.getCount(); ridx++) {
				const CritEntry & r_range_item_ = range_list_cqa.at(ridx);
				const uint pos = static_cast<uint>(r_range_item_.Idx-1);
				assert(pos < _c);
				if(!pos_to_remove.lsearch(pos)) {
					Strategy & r_item = at(pos);
					const double cqa = r_item.GetOptDeltaRangeCQA();
					double best_crit = r_item.V.Result;
					uint   best_crit_pos = pos;
					for(uint ridx2 = ridx+1; ridx2 < range_list_cqa.getCount(); ridx2++) {
						const CritEntry & r_range_item_next = range_list_cqa.at(ridx2);
						const uint pos_next = static_cast<uint>(r_range_item_next.Idx-1);
						Strategy & r_item_next = at(pos_next);
						if(r_item_next.OptDelta2Range.RealRange::IsEqual(r_item.OptDelta2Range)) {
							const double cqa_next = r_item_next.GetOptDeltaRangeCQA();
							const double cqam = (cqa_next + cqa) / 2.0;
							const uint diff = static_cast<uint>(R0(fabs((cqa_next - cqa) / cqam) * 1000.0));
							if(diff <= cqaMatchPromille) {
								double result = r_item_next.V.Result;
								if(best_crit < result) {
									best_crit = result;
									pos_to_remove.add(static_cast<long>(best_crit_pos));
									best_crit_pos = pos_next;
								}
								else
									pos_to_remove.add(static_cast<long>(pos_next));
							}
						}
					}
				}
			}
		}
		pos_to_remove.sortAndUndup();
		have_analog_pos_list.sortAndUndup(); // @v10.7.5
	}
	{
		for(uint i = 0; i < _c; i++) {
			const Strategy & r_item = at(i);
			if(!pos_to_remove.lsearch(static_cast<long>(i)) && r_item.V.Result > 0.0 && r_item.GetWinCountRate() >= minWinRate) {
				// @v10.7.5 Специальное (экспериментальное) соображение из-за которого введен have_analog_pos_list:
				// если мы имеем более одной стратегии с идентичным углом наклона но разными дистанциями регрессии, то
				// это дает основание полагать, что такой угол обеспечивает более надежную стратегию. Следовательно, стратегии с углами,
				// не имеющими аналогов, будет разумным убрать из набора стратегий.
				// Это соображение базируется на предположении, что реальную вероятность выигрыша обеспечивает не сам угол 
				// наклона регрессионной прямой, а кривая (парабола, вероятно) касательной к которой является найденная регрессия.
				/*if(have_analog_pos_list.lsearch(static_cast<long>(i)))*/{
					int    skip = 0;
					if(flags & gbsfTrendFollowing && r_item.StakeMode == 1) {
						if(r_item.BaseFlags & r_item.bfShort) {
							if(!r_item.OptDeltaRange.LessThan(0.0))
								skip = 1;
						}
						else {
							if(!r_item.OptDeltaRange.GreaterThan(0.0))
								skip = 1;
						}
					}
					if(!skip) {
						if(((r_item.BaseFlags & r_item.bfShort) && (flags & gbsfShort)) || (!(r_item.BaseFlags & r_item.bfShort) && (flags & gbsfLong))) {
							if((r_item.StakeMode == 1 && flags & gbsfStakeMode1) || (r_item.StakeMode == 2 && flags & gbsfStakeMode2) || (r_item.StakeMode == 3 && flags & gbsfStakeMode3)) {
								THROW(AddStrategyToOrderIndex(i, flags, range_list_));
							}
						}
					}
				}
			}
		}
		if(flags & gbsfShuffle)
			range_list_.shuffle();
		else
			range_list_.sort(PTR_CMPFUNC(StrategyCritEntry));
		for(uint ridx = 0; ridx < range_list_.getCount() && rScDest.getCount() < maxCount; ridx++) {
			const CritEntry & r_range_item_ = range_list_.at(ridx);
			const uint pos = static_cast<uint>(r_range_item_.Idx-1);
			assert(pos < _c);
			const Strategy & r_item = at(pos);
			int   do_skip = 0;
			const double _eps = 1E-11;
			if(flags & gbsfEliminateDups && r_item.StakeMode == 1) {
				for(uint j = 0; !do_skip && j < rScDest.getCount(); j++) {
					const Strategy & r_j_item = rScDest.at(j);
					if(r_j_item.StakeMode == r_item.StakeMode && r_j_item.InputFrameSize == r_item.InputFrameSize) {
						if(feqeps(r_j_item.OptDeltaRange.low, r_item.OptDeltaRange.low, _eps) && feqeps(r_j_item.OptDeltaRange.upp, r_item.OptDeltaRange.upp, _eps)) {
							if(r_j_item.MainFrameSize == r_item.MainFrameSize) { // @v10.6.9
								if(feqeps(r_j_item.OptDelta2Range.low, r_item.OptDelta2Range.low, _eps) && feqeps(r_j_item.OptDelta2Range.upp, r_item.OptDelta2Range.upp, _eps)) { // @v10.6.9
									// @v10.6.9 {
									if(pScSkipDueDup) {
										THROW_SL(pScSkipDueDup->insert(&r_item));
									}
									// } @v10.6.9 
									do_skip = 1;
								}
							}
						}
					}
				}
			}
			if(!do_skip) {
				THROW_SL(rScDest.insert(&r_item));
			}
		}
	}
	rScDest.Fb.AvgLocalDeviation = Fb.AvgLocalDeviation; // @v10.7.1
	rScDest.Fb.UseDataForStrategiesTill = Fb.UseDataForStrategiesTill; // @v10.7.3
	ASSIGN_PTR(pToRemovePosList, pos_to_remove); // @v10.7.5
	ASSIGN_PTR(pHaveAnalogPosList, have_analog_pos_list); // @v10.7.5
	CATCHZOK
	return ok;
}
#endif // } 0 @v10.7.10 (replaced with GetBestSubset2())

const PPObjTimeSeries::Strategy * FASTCALL PPObjTimeSeries::StrategyContainer::SearchByID(uint32 id) const
{
	const PPObjTimeSeries::Strategy * p_result = 0;
	if(id) {
		for(uint i = 0; !p_result && i < getCount(); i++)
			if(at(i).ID == id)
				p_result = &at(i);
	}
	return p_result;
}

double SLAPI PPObjTimeSeries::StrategyContainer::EvaluateScore(int scoreId, const LongArray * pExceptIndices) const
{
	double score = 0.0;
	StatBase sb;
	for(uint i = 0; i < getCount(); i++) {
		if(!pExceptIndices || !pExceptIndices->lsearch(i)) {
			const Strategy & r_s = at(i);
			// @20200306 score += (r_s.GetWinCountRate() * r_s.V.GetResultPerDay());
			if(scoreId == scoreResult) {
				score += r_s.V.Result; // @20200306 
			}
			else if(oneof3(scoreId, scoreEvStakeCountMean, scoreEvStakeCountStdDev, scoreEvStakeCountVarCoeff)) {
				sb.Step(r_s.StakeCount);
			}
			else if(oneof3(scoreId, scoreSeStakeCountMean, scoreSeStakeCountStdDev, scoreSeStakeCountVarCoeff)) {
				sb.Step(r_s.StakeCountAtFinalSimulation);
			}
		}
	}
	sb.Finish();
	if(scoreId == scoreEvStakeCountMean)
		score = sb.GetExp();
	else if(scoreId == scoreEvStakeCountStdDev)
		score = sb.GetStdDev();
	else if(scoreId == scoreEvStakeCountVarCoeff)
		score = fdivnz(sb.GetStdDev(), sb.GetExp());
	else if(scoreId == scoreSeStakeCountMean)
		score = sb.GetExp();
	else if(scoreId == scoreSeStakeCountStdDev) 
		score = sb.GetStdDev();
	else if(scoreId == scoreSeStakeCountVarCoeff)
		score = fdivnz(sb.GetStdDev(), sb.GetExp());
	return score;
}

int SLAPI PPObjTimeSeries::StrategyContainer::GetInputFramSizeList(LongArray & rList, uint * pMaxOptDelta2Stride) const
{
	int    ok = -1;
	uint   max_opt_delta2_stride = 0;
	rList.clear();
	for(uint i = 0; i < getCount(); i++) {
		const Strategy & r_item = at(i);
		rList.addnz(static_cast<long>(r_item.InputFrameSize));
		rList.addnz(static_cast<long>(r_item.MainFrameSize)); // @v10.4.10
		SETMAX(max_opt_delta2_stride, r_item.OptDelta2Stride);
	}
	ASSIGN_PTR(pMaxOptDelta2Stride, max_opt_delta2_stride);
	if(rList.getCount()) {
		rList.sortAndUndup();
		ok = 1;
	}
	return ok;
}

//static
SString & FASTCALL PPObjTimeSeries::GetStrategySetTypeName(StrategySetType sst, SString & rBuf)
{
	rBuf.Z();
	switch(sst) {
		case sstAll: rBuf = "StrategySetAll"; break;
		case sstSelection: rBuf = "StrategySetSelection"; break;
		//case sstSelectionTrendFollowing: rBuf = "StrategySetSelectionTrendFollowing"; break;
		default: rBuf = "StrategySetUnkn"; break;
	}
	return rBuf;
}

int SLAPI PPObjTimeSeries::PutStrategies(PPID id, StrategySetType sst, StrategyContainer * pL, int use_ta)
{
	int    ok = 1;
	assert(oneof2(sst, sstAll, sstSelection));
	PPTimeSeries ts_rec;
	SBuffer buffer;
	const uint _c = SVectorBase::GetCount(pL);
	PPID   prop = 0;
	THROW(oneof2(sst, sstAll, sstSelection));
	switch(sst) {
		case sstAll: prop = TIMSERPRP_STAKEMODEL_ALL; break;
		case sstSelection: prop = TIMSERPRP_STAKEMODEL_SEL; break;
		//case sstSelectionTrendFollowing: prop = TIMSERPRP_STAKEMODEL_SELTF; break;
		default: assert(0); break;
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(id, &ts_rec) > 0);
		if(_c) {
			SSerializeContext sctx;
			THROW(pL->Serialize(+1, buffer, &sctx));
		}
		THROW(ref->PutPropSBuffer(Obj, id, prop, buffer, 0));
		DS.LogAction(PPACN_TSSTRATEGYUPD, Obj, id, static_cast<long>(_c), 0); // @v10.3.3
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjTimeSeries::GetStrategies(PPID id, StrategySetType sst, StrategyContainer & rL)
{
	int    ok = -1;
	assert(oneof2(sst, sstAll, sstSelection));
	SBuffer buffer;
	PPID   prop = 0;
	THROW(oneof2(sst, sstAll, sstSelection));
	switch(sst) {
		case sstAll: prop = TIMSERPRP_STAKEMODEL_ALL; break;
		case sstSelection: prop = TIMSERPRP_STAKEMODEL_SEL; break;
		//case sstSelectionTrendFollowing: prop = TIMSERPRP_STAKEMODEL_SELTF; break;
		default: assert(0); break;
	}
	rL.clear();
	THROW(ref->GetPropSBuffer(Obj, id, prop, buffer));
	{
		size_t sd_size = buffer.GetAvailableSize();
		if(sd_size) {
			STempBuffer temp_buf(sd_size);
			SSerializeContext sctx;
			THROW(rL.Serialize(-1, buffer, &sctx));
			if(rL.getCount())
				ok = 1;
		}
	}
	CATCH
		rL.clear();
		ok = 0;
	ENDCATCH
	return ok;
}

SLAPI PPObjTimeSeries::TrainNnParam::TrainNnParam(const char * pSymb, long flags) : Strategy(), Symb(pSymb),
	ActionFlags(flags) /* @v10.7.4 ForwardFrameSize(0), HiddenLayerDim(0), EpochCount(1), LearningRate(0.1f)*/
{
}

SLAPI PPObjTimeSeries::TrainNnParam::TrainNnParam(const PPTimeSeriesPacket & rTsPack, long flags) : Strategy(), Symb(rTsPack.Rec.Symb),
	ActionFlags(flags) /* @v10.7.4 ForwardFrameSize(0), HiddenLayerDim(0), EpochCount(1), LearningRate(0.1f)*/
{
	Prec = rTsPack.Rec.Prec;
	Margin = (rTsPack.E.MarginManual > 0.0) ? rTsPack.E.MarginManual : rTsPack.Rec.BuyMarg; // @v10.5.6 rTsPack.E.MarginManual
	SpreadAvg = rTsPack.Rec.AvgSpread;
}

void SLAPI PPObjTimeSeries::TrainNnParam::Reset()
{
	Strategy::Reset();
}

SString & SLAPI PPObjTimeSeries::TrainNnParam::MakeFileName(SString & rBuf) const
{
	return rBuf.Z().Cat(Symb).CatChar('-').Cat(InputFrameSize).CatChar('-').Cat(MaxDuckQuant).Dot().Cat("fann").ToLower();
}

double SLAPI PPObjTimeSeries::Strategy::GetWinCountRate() const { return fdivnz((double)WinCount, (double)StakeCount); }
double SLAPI PPObjTimeSeries::Strategy::CalcConfidenceFactor() const
{
	return (GenPtCount > 0) ? (100.0 * static_cast<double>(StakeCount) / log(static_cast<double>(GenPtCount))) : 0.0;
}

// Descr: Возвращает средний катет (OptDeltaRange*InputFrameSize) / SpikeQuant
//
double SLAPI PPObjTimeSeries::Strategy::GetOptDeltaRangeCQA() const
{
	return (InputFrameSize > 0 && SpikeQuant_s > 0.0) ? ((InputFrameSize * OptDeltaRange.GetMiddle()) / SpikeQuant_s) : 0.0;
}

double SLAPI PPObjTimeSeries::StrategyResultEntry::GetPeakAverage() const { return fdivnz(SumPeak, (double)StakeCount); }
double SLAPI PPObjTimeSeries::StrategyResultEntry::GetBottomAverage() const { return fdivnz(SumBottom, (double)StakeCount); }

//static
double SLAPI PPObjTimeSeries::Strategy::CalcSlTpAdjustment(int prec, double averageSpreadForAdjustment)
{
	const double __spread = (prec > 0 && averageSpreadForAdjustment > 0.0) ? (averageSpreadForAdjustment * fpow10i(-prec)) : 0.0;
	return __spread;
}

double SLAPI PPObjTimeSeries::Strategy::CalcSL(double peak, double averageSpreadForAdjustment) const
{
	const  double mdv = (MaxDuckQuant * SpikeQuant_s);
	const  double __spread = CalcSlTpAdjustment(Prec, averageSpreadForAdjustment);
	const  double _r = (BaseFlags & bfShort) ? (peak * (1.0 + mdv) + __spread) : (peak * (1.0 - mdv) - __spread);
	return round(_r, Prec);
}

double SLAPI PPObjTimeSeries::Strategy::CalcSL(double peak, double externalSpikeQuant, double averageSpreadForAdjustment) const
{
	const  double mdv = (MaxDuckQuant * ((externalSpikeQuant > 0.0) ? externalSpikeQuant : SpikeQuant_s));
	const  double __spread = CalcSlTpAdjustment(Prec, averageSpreadForAdjustment);
	const  double _r = (BaseFlags & bfShort) ? (peak * (1.0 + mdv) + __spread) : (peak * (1.0 - mdv) - __spread);
	return round(_r, Prec);
}

double SLAPI PPObjTimeSeries::Strategy::CalcTP(double stakeBase, double averageSpreadForAdjustment) const
{
	if(TargetQuant) {
		const  double tv = (TargetQuant * SpikeQuant_s);
		const  double __spread = CalcSlTpAdjustment(Prec, averageSpreadForAdjustment);
		const  double _r = (BaseFlags & bfShort) ? (stakeBase * (1.0 - tv) + __spread) : (stakeBase * (1.0 + tv) - __spread);
		return round(_r, Prec);
	}
	else
		return 0.0;
}

double SLAPI PPObjTimeSeries::Strategy::CalcTP(double stakeBase, double externalSpikeQuant, double averageSpreadForAdjustment) const
{
	if(TargetQuant) {
		const  double tv = (TargetQuant * ((externalSpikeQuant > 0.0) ? externalSpikeQuant : SpikeQuant_s));
		const  double __spread = CalcSlTpAdjustment(Prec, averageSpreadForAdjustment);
		const  double _r = (BaseFlags & bfShort) ? (stakeBase * (1.0 - tv) + __spread) : (stakeBase * (1.0 + tv) - __spread);
		return round(_r, Prec);
	}
	else
		return 0.0;
}

//static
double SLAPI PPObjTimeSeries::Strategy::CalcSL_withExternalFactors(double peak, bool isShort, int prec, uint maxDuckQuant, double spikeQuant, double averageSpreadForAdjustment)
{
	const  double mdv = (maxDuckQuant * spikeQuant);
	const  double __spread = CalcSlTpAdjustment(prec, averageSpreadForAdjustment);
	if(isShort) {
		return round(peak * (1.0 + mdv) + __spread, prec);
	}
	else {
		return round(peak * (1.0 - mdv) - __spread, prec);
	}
}

//static
double SLAPI PPObjTimeSeries::Strategy::CalcTP_withExternalFactors(double stakeBase, bool isShort, int prec, uint targetQuant, double spikeQuant, double averageSpreadForAdjustment)
{
	if(targetQuant) {
		const  double tv = (targetQuant * spikeQuant);
		const  double __spread = CalcSlTpAdjustment(prec, averageSpreadForAdjustment);
		if(isShort) {
			return round(stakeBase * (1.0 - tv) + __spread, prec);
		}
		else {
			return round(stakeBase * (1.0 + tv) - __spread, prec);
		}
	}
	else
		return 0.0;
}
/*
time-series-precision: точность представления значений
time-series-margin-buy:  маржина при покупке инструмента, представленного символом
time-series-margin-sell: маржина при продаже инструмента, представленного символом
time-series-currency-base: основная валюта
time-series-currency-profit: валюта прибыли
time-series-currency-margin: валюта маржины
time-series-volume-min: минимальный объем сделки
time-series-volume-max: максимальный объем сделки
time-series-volume-step: Минимальный шаг изменения объема для заключения сделки
*/
IMPLEMENT_PPFILT_FACTORY(PrcssrTsStrategyAnalyze); SLAPI PrcssrTsStrategyAnalyzeFilt::PrcssrTsStrategyAnalyzeFilt() : PPBaseFilt(PPFILT_PRCSSRTSSTRATEGYANALYZE, 0, 0)
{
	SetFlatChunk(offsetof(PrcssrTsStrategyAnalyzeFilt, ReserveStart),
		offsetof(PrcssrTsStrategyAnalyzeFilt, TsList)-offsetof(PrcssrTsStrategyAnalyzeFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(PrcssrTsStrategyAnalyzeFilt, TsList));
	Init(1, 0);
}

PrcssrTsStrategyAnalyzeFilt & FASTCALL PrcssrTsStrategyAnalyzeFilt::operator = (const PrcssrTsStrategyAnalyzeFilt & rS)
{
	Copy(&rS, 0);
	return *this;
}

SLAPI PrcssrTsStrategyAnalyze::PrcssrTsStrategyAnalyze() : LastStrategyId()
{
}

SLAPI PrcssrTsStrategyAnalyze::~PrcssrTsStrategyAnalyze()
{
}

long SLAPI PrcssrTsStrategyAnalyze::GetNewStrategyId() const
{
	return LastStrategyId.Incr();
}

int SLAPI PrcssrTsStrategyAnalyze::InitParam(PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	if(P.IsA(pBaseFilt)) {
		PrcssrTsStrategyAnalyzeFilt * p_filt = static_cast<PrcssrTsStrategyAnalyzeFilt *>(pBaseFilt);
		if(p_filt->Flags == 0)
			p_filt->Flags = (PrcssrTsStrategyAnalyzeFilt::fFindOptMaxDuck|PrcssrTsStrategyAnalyzeFilt::fFindStrategies|
				PrcssrTsStrategyAnalyzeFilt::fProcessLong|PrcssrTsStrategyAnalyzeFilt::fProcessShort);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::EditParam(PPBaseFilt * pBaseFilt)
{
	class PrcssrTssaDialog : public TDialog {
		DECL_DIALOG_DATA(PrcssrTsStrategyAnalyzeFilt);
	public:
		PrcssrTssaDialog() : TDialog(DLG_TSSA)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_TSSA_TS, PPOBJ_TIMESERIES, Data.TsList.GetSingle(), 0/*OLW_LOADDEFONOPEN*/, 0);
			AddClusterAssoc(CTL_TSSA_FLAGS, 0, Data.fFindOptMaxDuck);
			AddClusterAssoc(CTL_TSSA_FLAGS, 1, Data.fFindStrategies);
			AddClusterAssoc(CTL_TSSA_FLAGS, 2, Data.fForce);
			AddClusterAssoc(CTL_TSSA_FLAGS, 3, Data.fProcessLong);
			AddClusterAssoc(CTL_TSSA_FLAGS, 4, Data.fProcessShort);
			AddClusterAssoc(CTL_TSSA_FLAGS, 5, Data.fAutodetectTargets);
			AddClusterAssoc(CTL_TSSA_FLAGS, 6, Data.fSimulateAfter); // @v10.7.0
			AddClusterAssoc(CTL_TSSA_FLAGS, 7, Data.fAnalyzeModels); // @v10.7.7
			SetClusterData(CTL_TSSA_FLAGS, Data.Flags);
			AddClusterAssocDef(CTL_TSSA_CLOSEMODE, 0, PPObjTimeSeries::Strategy::clsmodFullMaxDuck);
			AddClusterAssoc(CTL_TSSA_CLOSEMODE, 1, PPObjTimeSeries::Strategy::clsmodAdjustLoss);
			SetClusterData(CTL_TSSA_CLOSEMODE, Data.CloseMode);
			setCtrlDate(CTL_TSSA_SBTDT, Data.UseDataForStrategiesTill); // @v10.7.2
			Setup();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			PPID   id = getCtrlLong(CTLSEL_TSSA_TS);
			Data.TsList.Add(id);
			GetClusterData(CTL_TSSA_FLAGS, &Data.Flags);
			GetClusterData(CTL_TSSA_CLOSEMODE, &Data.CloseMode);
			getCtrlData(CTL_TSSA_SBTDT, &Data.UseDataForStrategiesTill); // @v10.7.2
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmTsList)) {
				PPID   id = 0;
				PPIDArray list;
				Data.TsList.Get(list);
				getCtrlData(CTLSEL_TSSA_TS, &id);
				if(id)
					list.addUnique(id);
				ListToListData data(PPOBJ_TIMESERIES, 0, &list);
				data.TitleStrID = PPTXT_SELTIMSERLIST;
				if(ListToListDialog(&data) > 0) {
					Data.TsList.Set(&list);
					Setup();
				}
				clearEvent(event);
			}
		}
		void   Setup()
		{
			PPID   id = Data.TsList.GetSingle();
			PPID   prev_id = getCtrlLong(CTLSEL_TSSA_TS);
			if(id != prev_id)
				setCtrlData(CTLSEL_TSSA_TS, &id);
			if(Data.TsList.GetCount() > 1)
				SetComboBoxListText(this, CTLSEL_TSSA_TS);
			disableCtrl(CTLSEL_TSSA_TS, BIN(Data.TsList.GetCount() > 1));
		}
	};
	if(!P.IsA(pBaseFilt))
		return 0;
	PrcssrTsStrategyAnalyzeFilt * p_filt = static_cast<PrcssrTsStrategyAnalyzeFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(PrcssrTssaDialog, p_filt);
}

int SLAPI PrcssrTsStrategyAnalyze::Init(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(P.IsA(pBaseFilt));
	P = *static_cast<const PrcssrTsStrategyAnalyzeFilt *>(pBaseFilt);
	CATCHZOK
	return ok;
}

//static
SString & SLAPI PPObjTimeSeries::StrategyToString(const PPObjTimeSeries::Strategy & rS, const double * pOptFactor, const double * pOptFactor2, SString & rBuf)
{
	rBuf.Z();
	const long trange_fmt = MKSFMTD(0, 10, NMBF_NOTRAILZ|NMBF_FORCEPOS); // @v10.6.8 NMBF_FORCEPOS
	rBuf.Cat("Strategy").CatChar('(');
	rBuf.CatLongZ(rS.ID, 5);
	if(rS.GenSeq > 0)
		rBuf.Dot().CatLongZ(rS.GenSeq, 3);
	rBuf.CatChar(')');
	rBuf.CatDiv(':', 2).Cat((rS.BaseFlags & rS.bfShort) ? "S" : "B").CatChar('/');
	rBuf.CatLongZ(rS.InputFrameSize, 3).CatChar('/').Cat(rS.MaxDuckQuant).CatChar(':').Cat(rS.TargetQuant).CatChar('/').Cat(rS.StakeMode);
	rBuf.Space().Cat("Potential").CatDiv(':', 2).
		Cat(rS.V.GetResultPerDay(), MKSFMTD(0, 2, 0)).CatChar('/'). // @v10.7.3 MKSFMTD(0, 4, 0)-->MKSFMTD(0, 2, 0)
		Cat(rS.V.Result, MKSFMTD(5, 1, 0)).CatChar('/'); // @v10.7.3
	{
		const double wcr = rS.GetWinCountRate();
		assert(wcr >= 0.0 && wcr <= 1.0);
		if(wcr == 1.0)
			rBuf.Cat("WONE");
		else
			rBuf.CatChar('W').Cat(fmul1000i(wcr));
	}
	rBuf.CatChar('/').CatChar('#').Cat(rS.StakeCount);
	if(rS.StakeCountAtFinalSimulation > 0)
		rBuf.CatChar(':').Cat(rS.StakeCountAtFinalSimulation);
	if(rS.GenPtCount > 0) // @v10.7.11
		rBuf.CatChar('^').CatLongZ(R0i(rS.CalcConfidenceFactor()), 3);
	// @v10.7.9 {
	/* (не сработало) if(rS.StakeDistMedian > 0)
		rBuf.CatChar('*').Cat(log(static_cast<double>(rS.StakeDistMedian * (rS.WinCount-1)))*100.0, MKSFMTD(0, 0, 0));*/
	// } @v10.7.9 
	rBuf.CatChar('/').
		CatChar('T').CatLongZ(static_cast<long>(rS.StakeCount ? (rS.V.TmSec / rS.StakeCount) : 0), 6);
	if(rS.MainFrameSize) {
		rBuf.Space().Cat("MF").Cat(rS.MainFrameSize).CatChar('[');
		// @v10.6.12 {
		if(pOptFactor2) {
			const double op2r_dist = rS.OptDelta2Range.GetDistance();
			if(op2r_dist > 0.0 && rS.OptDelta2Range.Check(*pOptFactor2)) {
				const double tv2_part = (*pOptFactor2 - rS.OptDelta2Range.low) / op2r_dist;
				rBuf.Cat(tv2_part, MKSFMTD(0, 3, 0)).CatChar('|');
			}
			else
				rBuf.CatChar('?').CatChar('|');
		}
		// } @v10.6.12 
		rBuf.Cat(rS.OptDelta2Range, trange_fmt).CatChar(']');
	}
	if(oneof3(rS.StakeMode, 1, 2, 3)) {
		rBuf.Space().CatChar('[');
		if(pOptFactor) {
			if(rS.StakeMode == 1) {
				const double opr_dist = rS.OptDeltaRange.GetDistance();
				if(opr_dist > 0.0 && rS.OptDeltaRange.Check(*pOptFactor)) {
					const double tv_part = (*pOptFactor - rS.OptDeltaRange.low) / opr_dist;
					rBuf.Cat(tv_part, MKSFMTD(0, 3, 0)).CatChar('|');
				}
				else
					rBuf.CatChar('?').CatChar('|');
				rBuf.Cat(rS.OptDeltaRange, trange_fmt);
				rBuf.CatChar('|').Cat(rS.GetOptDeltaRangeCQA(), MKSFMTD(0, 2, 0));
			}
			else if(rS.StakeMode == 2)
				rBuf.Cat(*pOptFactor, trange_fmt).CatChar('|').Cat(rS.OptDelta2Stride).CatChar('/').Cat(rS.OptDelta2Range, trange_fmt);
			else if(rS.StakeMode == 3) {
				rBuf.Cat(*pOptFactor, trange_fmt).CatChar('|').Cat(rS.OptDeltaRange, trange_fmt).CatChar(']').Space();
				rBuf.CatChar('[');
				rBuf.Cat(*pOptFactor, trange_fmt).CatChar('|').Cat(rS.OptDelta2Stride).CatChar('/').Cat(rS.OptDelta2Range, trange_fmt);
			}
		}
		else {
			if(rS.StakeMode == 1)
				rBuf.Cat(rS.OptDeltaRange, trange_fmt).CatChar('|').Cat(rS.GetOptDeltaRangeCQA(), MKSFMTD(0, 2, 0));
			else if(rS.StakeMode == 2)
				rBuf.Cat(rS.OptDelta2Stride).CatChar('/').Cat(rS.OptDelta2Range, trange_fmt);
			else if(rS.StakeMode == 3) {
				rBuf.Cat(rS.OptDeltaRange, trange_fmt).CatChar(']').Space();
				rBuf.CatChar('[');
				rBuf.Cat(rS.OptDelta2Stride).CatChar('/').Cat(rS.OptDelta2Range, trange_fmt);
			}
		}
		rBuf.CatChar(']').Space();
	}
	return rBuf;
}

//static
SString & SLAPI PPObjTimeSeries::StrategyOutput(const char * pSymb, const PPObjTimeSeries::Strategy * pSre, SString & rBuf)
{
	rBuf.Z();
	if(pSre == 0) {
		rBuf.
		Cat("symbol").Space().
		Cat("id").Space().
		Cat("spike-quant").Space().
		Cat("max-duck:target").Space().
		Cat("input-frame-size").Space().
		Cat("result").Space().
		Cat("stake-count").Space().
		Cat("win-count-rel").Space().
		//Cat("total-tm-count").Space().
		Cat("total-tm-hr").Space().
		Cat("result-per-day").Space().
		Cat("peak-avg-quant:peak-max-quant:bottom-avg-quant").Space().
		Cat("opt-delta-freq").Space().
		Cat("opt-delta-result").Space().
		Cat("opt-delta2-freq").Space().
		Cat("opt-delta2-result").Space();
	}
	else {
		SString temp_buf;
		if(!isempty(pSymb))
			rBuf.Cat(pSymb);
		if(pSre->BaseFlags & pSre->bfShort)
			rBuf.CatChar('-').Cat("REV");
		if(pSre->StakeMode)
			rBuf.CatChar('/').Cat(pSre->StakeMode);
		else
			rBuf.Space().Space();
		rBuf.Space().
			CatLongZ(pSre->ID, 4).Space().
			Cat(pSre->SpikeQuant_s, MKSFMTD(15, 8, 0)).Space().
			Cat(pSre->MaxDuckQuant).CatChar(':').Cat(pSre->TargetQuant).Space().
			Cat(pSre->InputFrameSize).Space();
		if(pSre->MainFrameSize) {
			rBuf.Cat("MF").Cat(pSre->MainFrameSize).CatChar('[').Cat(pSre->OptDelta2Range, MKSFMTD(0, 12, 0)).CatChar(']').Space();
		}
		rBuf.Cat(pSre->V.Result, MKSFMTD(15, 5, 0)).Space();
		rBuf.Cat(temp_buf.Z().Cat(pSre->StakeCount).Align(7, ADJ_RIGHT)).Space();
		{
			const double wcr = pSre->GetWinCountRate();
			rBuf.Cat(wcr, MKSFMTD(15, 5, 0)).Space();
		}
			//Cat(pSre->V.TmCount).Space().
		rBuf.Cat(pSre->V.TmSec / 3600.0, MKSFMTD(9, 1, 0)).Space().
			Cat(pSre->V.GetResultPerDay(), MKSFMTD(9, 6, 0)).Space().
			CatLongZ(pSre->PeakAvgQuant, 2).CatChar(':').CatLongZ(pSre->PeakMaxQuant, 2).CatChar(':').CatLongZ(pSre->BottomAvgQuant, 2).Space();
		{
			rBuf.CatChar('[').Cat(pSre->OptDeltaRange, MKSFMTD(0, 12, NMBF_FORCEPOS)).
				CatChar('/').Cat(pSre->OptDeltaRange.Count).
				CatChar('|').Cat(pSre->GetOptDeltaRangeCQA(), MKSFMTD(0, 2, 0)).CatChar(']');// @v10.7.3 .CatChar('/').Cat(pSre->LastResultIdx-pSre->InputFrameSize+1);
			rBuf.Space();
			if(pSre->StakeMode == 0)
				rBuf.Cat(pSre->OptDeltaRange.Result, MKSFMTD(15, 5, 0));
			rBuf.Space();
			if(pSre->StakeMode != 1) {
				rBuf.Cat(pSre->OptDelta2Range, MKSFMTD(0, 12, 0)).Space().
					Cat(pSre->OptDelta2Stride).CatChar('/').Cat(pSre->OptDelta2Range.Count);// @v10.7.3 .CatChar('/').Cat(pSre->LastResultIdx - pSre->InputFrameSize + 1);
			}
			rBuf.Space();
			if(pSre->StakeMode == 0)
				rBuf.Cat(pSre->OptDelta2Range.Result, MKSFMTD(15, 5, 0));
			rBuf.Space();
		}
	}
	return rBuf;
}

SLAPI PrcssrTsStrategyAnalyze::ModelParam::ModelParam() : Flags(0), OptTargetCriterion(tcWinRatio), InitTrendErrLimit(0.0), PartitialTrendErrLimit(0.0), 
	InitMainTrendErrLimit(0.0), BestSubsetDimention(0), BestSubsetMaxPhonyIters(0), BestSubsetOptChunk(0), UseDataSince(ZERODATE), UseDataTill(ZERODATE), 
	DefTargetQuant(0), MinWinRate(0.0), OptRangeStep(0), OptRangeStepMkPart(0), OptRangeMultiLimit(0), OptRangeMaxExtProbe(0), MainFrameRangeCount(0), 
	OverallWinRateLimit(0.0), 
	Reserve(0) // @v10.7.10
	// @v10.7.10 , ChaosFactor(0)
{
}

PrcssrTsStrategyAnalyze::ModelParam & SLAPI PrcssrTsStrategyAnalyze::ModelParam::Z()
{
	Flags = 0;
	InitTrendErrLimit = 0.0;
	PartitialTrendErrLimit = 0.0; // @v10.7.4
	InitMainTrendErrLimit = 0.0; // @v10.7.0
	BestSubsetDimention = 0;
	BestSubsetMaxPhonyIters = 0;
	BestSubsetOptChunk = 0;
	MaxDuckQuantList.clear();
	TargetQuantList.clear(); // @v10.7.3
	StakeBoundList.clear(); // @v10.7.3
	InputFrameSizeList.clear();
	UseDataSince = ZERODATE;
	UseDataTill = ZERODATE; // @v10.7.0
	OverallWinRateLimit = 0.0; // @v10.7.0
	DefTargetQuant = 0;
	// @v10.5.0 MainFrameSize = 0; // @v10.5.0
	MainFrameRangeCount = 0; // @v10.5.0
	MinWinRate = 0.0;
	OptRangeStep = 0; // @v10.4.7
	OptRangeStepMkPart = 0; // @v10.4.7
	OptRangeMultiLimit = 0; // @v10.4.7
	OptRangeMaxExtProbe = 0; // @v10.7.3
	// @v10.7.10 ChaosFactor = 0; // @v10.7.1
	Reserve = 0; // @v10.7.10
	return *this;
}

//static
int SLAPI PrcssrTsStrategyAnalyze::ReadModelParam(ModelParam & rMp)
{
	int    ok = 1;
	SString temp_buf;
	PPIniFile ini_file;
	rMp.Z();
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_INITTRENDERRLIMIT, temp_buf) > 0)
		rMp.InitTrendErrLimit = temp_buf.ToReal();
	rMp.InitTrendErrLimit = inrangeordefault(rMp.InitTrendErrLimit, 1E-6, 10.0, 0.0);
	// @v10.7.4 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_PARTTRENDERRLIMIT, temp_buf) > 0)
		rMp.PartitialTrendErrLimit = temp_buf.ToReal();
	rMp.PartitialTrendErrLimit = inrangeordefault(rMp.PartitialTrendErrLimit, 0.1, 1.0, 0.0);
	// } @v10.7.4 
	// @v10.7.0 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_INITMAINTRENDERRLIMIT, temp_buf) > 0)
		rMp.InitMainTrendErrLimit = temp_buf.ToReal();
	rMp.InitMainTrendErrLimit = inrangeordefault(rMp.InitMainTrendErrLimit, 1E-6, 10.0, 0.0);
	// } @v10.7.0 
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_BESTSUBSETDIMESION, temp_buf) > 0)
		rMp.BestSubsetDimention = static_cast<uint>(temp_buf.ToLong());
	rMp.BestSubsetDimention = inrangeordefault(rMp.BestSubsetDimention, 1, 3000, 100);
	// @v10.4.3 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_BESTSUBSETTF, temp_buf) > 0) {
		if(temp_buf.IsEqiAscii("true") || temp_buf.IsEqiAscii("yes") || temp_buf.IsEqual("1"))
			rMp.Flags |= rMp.fBestSubsetTrendFollowing;
	}
	// } @v10.4.3
	// @v10.4.5
	{
		if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_OPTRANGETARGET, temp_buf) > 0) {
			if(temp_buf.IsEqiAscii("velocity"))
				rMp.OptTargetCriterion = rMp.tcVelocity;
			else if(temp_buf.IsEqiAscii("winratio")) // @v10.4.6
				rMp.OptTargetCriterion = rMp.tcWinRatio;
			else if(temp_buf.IsEqiAscii("angleratio")) // @v10.7.9
				rMp.OptTargetCriterion = rMp.tcAngleRatio;
			else if(temp_buf.IsEqiAscii("amount")) // @v10.7.3
				rMp.OptTargetCriterion = rMp.tcAmount;
			else
				rMp.OptTargetCriterion = rMp.tcAmount; // @default
		}
		if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_OPTRANGEMULTI, temp_buf) > 0) {
			if(temp_buf.IsEqiAscii("true") || temp_buf.IsEqiAscii("yes") || temp_buf.IsEqual("1"))
				rMp.Flags |= rMp.fOptRangeMulti;
		}
	}
	// @v10.4.5
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_BESTSUBSETMAXPHONY, temp_buf) > 0)
		rMp.BestSubsetMaxPhonyIters = static_cast<uint>(temp_buf.ToLong());
	if(rMp.BestSubsetMaxPhonyIters == 0 || rMp.BestSubsetMaxPhonyIters > 1000)
		rMp.BestSubsetMaxPhonyIters = 7;
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_BESTSUBSETOPTCHUNK, temp_buf) > 0)
		rMp.BestSubsetOptChunk = static_cast<uint>(temp_buf.ToLong());
	if(!oneof4(rMp.BestSubsetOptChunk, 1, 3, 7, 15)) // @v10.6.9 (15) // @v10.7.8 (1)
		rMp.BestSubsetOptChunk = 0;
	// @v10.4.2 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_DEFTARGETQUANT, temp_buf) > 0)
		rMp.DefTargetQuant = static_cast<uint>(temp_buf.ToLong());
	rMp.DefTargetQuant = inrangeordefault(rMp.DefTargetQuant, 1, 200, 18);
	// } @v10.4.2
	// @v10.4.7 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_OPTRANGE_STEP, temp_buf) > 0) {
		rMp.OptRangeStep = static_cast<uint>(temp_buf.ToLong());
	}
	if(rMp.OptRangeStep > 1000000)
		rMp.OptRangeStep = 0;
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_OPTRANGE_STEPMKPART, temp_buf) > 0) {
		rMp.OptRangeStepMkPart = static_cast<uint>(temp_buf.ToLong());
	}
	if(rMp.OptRangeStepMkPart > 100000) {
		rMp.OptRangeStepMkPart = 0;
	}
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_OPTRANGEMULTI_LIMIT, temp_buf) > 0) {
		rMp.OptRangeMultiLimit = static_cast<uint>(temp_buf.ToLong());
	}
	if(rMp.OptRangeMultiLimit > 100)
		rMp.OptRangeMultiLimit = 0;
	// } @v10.4.7
	// @v10.7.3 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_OPTRANGE_MAXEXTPROBE, temp_buf) > 0) {
		rMp.OptRangeMaxExtProbe = static_cast<uint>(temp_buf.ToLong());
	}
	if(rMp.OptRangeMaxExtProbe > 1000)
		rMp.OptRangeMaxExtProbe = 0;
	// } @v10.7.3 

	// @v10.4.1 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_USEDATASINCE, temp_buf) > 0)
		strtodate(temp_buf.Strip(), DATF_DMY, &rMp.UseDataSince);
	if(!checkdate(rMp.UseDataSince))
		rMp.UseDataSince = ZERODATE;
	// } @v10.4.1
	// @v10.7.0 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_USEDATATILL, temp_buf) > 0)
		strtodate(temp_buf.Strip(), DATF_DMY, &rMp.UseDataTill);
	if(!checkdate(rMp.UseDataTill))
		rMp.UseDataTill = ZERODATE;
	// } @v10.7.0
	// @v10.4.2 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_MINWINRATE, temp_buf) > 0) {
		rMp.MinWinRate = temp_buf.ToReal();
	}
	rMp.MinWinRate = inrangeordefault(rMp.MinWinRate, 0.0, 1.0, 0.0);
	// } @v10.4.2
	// @v10.7.0 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_OVERALLWINRATELIMIT, temp_buf) > 0) {
		rMp.OverallWinRateLimit = temp_buf.ToReal();
	}
	rMp.OverallWinRateLimit = inrangeordefault(rMp.OverallWinRateLimit, 0.0, 1.0, 0.0);
	// } @v10.7.0
	// @v10.4.9 {
	{
		if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_MAINFRAMESIZE, temp_buf) > 0) {
			StringSet ss(',', temp_buf);
			for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
				const long md = temp_buf.ToLong();
				if(md > 0 && md <= (1440*180))
					rMp.MainFrameSizeList.add(md);
			}
			rMp.MainFrameSizeList.sortAndUndup();
			//rMp.MainFrameSize = static_cast<uint>(temp_buf.ToLong());
		}
		/*if(rMp.MainFrameSize > 10000) {
			rMp.MainFrameSize = 0;
		}*/
		if(rMp.MainFrameSizeList.getCount() && ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_MAINFRAMERANGECOUNT, temp_buf) > 0) {
			rMp.MainFrameRangeCount = static_cast<uint>(temp_buf.ToLong());
			if(rMp.MainFrameRangeCount > 12)
				rMp.MainFrameRangeCount = 3;
		}
		else
			rMp.MainFrameRangeCount = 0;
	}
	// } @v10.4.9
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_MAXDUCKQUANT, temp_buf) > 0) {
		StringSet ss(',', temp_buf);
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			const long md = temp_buf.ToLong();
			if(md > 0 && md <= 300)
				rMp.MaxDuckQuantList.add(md);
		}
		rMp.MaxDuckQuantList.sortAndUndup();
	}
	if(rMp.MaxDuckQuantList.getCount() == 0) {
		// @v10.4.3 rMp.MaxDuckQuantList.add(28);
		// @v10.4.3 rMp.MaxDuckQuantList.add(30);
		// @v10.4.3 rMp.MaxDuckQuantList.add(32);
		// @v10.4.3 rMp.MaxDuckQuantList.add(34);
		// @v10.4.3 rMp.MaxDuckQuantList.add(36);
		// @v10.4.3 rMp.MaxDuckQuantList.add(38);
		// @v10.4.3 rMp.MaxDuckQuantList.add(40);
		// @v10.4.3 rMp.MaxDuckQuantList.add(42);
		rMp.MaxDuckQuantList.add(57); // @v10.4.3 // @v10.7.0 30-->57
	}
	// @v10.4.3 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_TARGETQUANT, temp_buf) > 0) {
		StringSet ss(',', temp_buf);
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			const long md = temp_buf.ToLong();
			if(md > 0 && md <= 300)
				rMp.TargetQuantList.add(md);
		}
		rMp.TargetQuantList.sortAndUndup();
	}
	// } @v10.4.3
	// @v10.7.3 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_STAKEBOUNDS, temp_buf) > 0) {
		StringSet ss(';', temp_buf);
		SString left_buf, right_buf;
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			if(temp_buf.Divide(',', left_buf, right_buf)) {
				long tp = left_buf.ToLong();
				long sl = right_buf.ToLong();
				if(tp > 0 && sl > 0) {
					rMp.StakeBoundList.Add(tp, sl);
				}
			}
		}
	}
	// } @v10.7.3 
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_INPUTFRAMESIZE, temp_buf) > 0) {
		StringSet ss(',', temp_buf);
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			const long ifs = temp_buf.ToLong();
			if(ifs > 0 && ifs <= 10080)
				rMp.InputFrameSizeList.add(ifs);
		}
		rMp.InputFrameSizeList.sortAndUndup();
	}
	if(rMp.InputFrameSizeList.getCount() == 0) {
		rMp.InputFrameSizeList.add(15);
		rMp.InputFrameSizeList.add(30);
		rMp.InputFrameSizeList.add(60);
		rMp.InputFrameSizeList.add(90);
		rMp.InputFrameSizeList.add(120);
		rMp.InputFrameSizeList.add(150);
		rMp.InputFrameSizeList.add(280);
		rMp.InputFrameSizeList.add(440);
		rMp.InputFrameSizeList.add(600);
	}
	/* @v10.7.10 
	// @v10.7.1 {
	if(ini_file.Get(PPINISECT_TSSTAKE, PPINIPARAM_TSSTAKE_CHAOSFACTOR, temp_buf) > 0)
		rMp.ChaosFactor = temp_buf.ToLong();
	if(rMp.ChaosFactor < 0 || rMp.ChaosFactor > 1000)
		rMp.ChaosFactor = 0;
	// } @v10.7.1
	*/
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::GetTimeSeries(PPID tsID, LDATE dateSince, LDATE dateTill, STimeSeries & rTs)
{
	int gtsr = TsObj.GetTimeSeries(tsID, rTs);
	if(gtsr > 0) {
		if(checkdate(dateSince)) {
			STimeSeries temp_ts;
			SUniTime ut_since;
			SUniTime ut_till;
			SUniTime * p_ut_till = 0;
			ut_since.Set(dateSince);
			if(checkdate(dateTill)) {
				ut_till.Set(dateTill);
				p_ut_till = &ut_till;
			}
			if(rTs.GetChunkRecentSince(ut_since, p_ut_till, temp_ts) > 0)
				rTs = temp_ts;
			else
				gtsr = 0;
		}
	}
	return gtsr;
}

#if 0 // @v10.7.2 {
int SLAPI PrcssrTsStrategyAnalyze::GetTimeSeries(PPID tsID, ModelParam & rMp, STimeSeries & rTs)
{
	int gtsr = TsObj.GetTimeSeries(tsID, rTs);
	if(gtsr > 0) {
		if(checkdate(rMp.UseDataSince)) {
			STimeSeries temp_ts;
			SUniTime ut_since;
			SUniTime ut_till;
			SUniTime * p_ut_till = 0;
			ut_since.Set(rMp.UseDataSince);
			// @v10.7.0 {
			if(checkdate(rMp.UseDataTill)) {
				ut_till.Set(rMp.UseDataTill);
				p_ut_till = &ut_till;
			}
			// } @v10.7.0 
			if(rTs.GetChunkRecentSince(ut_since, p_ut_till, temp_ts) > 0)
				rTs = temp_ts;
			else
				gtsr = 0;
		}
	}
	return gtsr;
}
#endif // } 0 @v10.7.2

static SString & OutputStategyResultEntry(const PPObjTimeSeries::StrategyResultEntry & rEntry, SString & rBuf)
{
	rBuf.Z().
		CatEq("Result", rEntry.V.Result, MKSFMTD(15, 5, 0)).Tab().
		CatEq("StakeCount", rEntry.StakeCount).Tab().
		CatEq("WinCount", rEntry.WinCount).Tab(). // @v10.7.1
		CatEq("LossCount", rEntry.StakeCount-rEntry.WinCount).Tab(). // @v10.7.1
		CatEq("WinCountRate", rEntry.GetWinCountRate(), MKSFMTD(15, 5, 0)).Tab().
		CatEq("TmCount", rEntry.V.TmCount).Tab().
		CatEq("TmSec", rEntry.V.TmSec).Tab().
		CatEq("TotalSec", rEntry.TotalSec).Tab().
		CatEq("ResultPerTotalDay", fdivnz(3600.0 * 24.0 * rEntry.V.Result, static_cast<double>(rEntry.TotalSec)), MKSFMTD(0, 12, 0));
	return rBuf;
}

// @v10.6.5 @construction
int SLAPI PrcssrTsStrategyAnalyze::FindOptimalMaxDuck(const PPTimeSeriesPacket & rTsPack, const DateTimeArray & rTsTmList, const RealArray & rTsValList, uint flags, uint * pResult)
{
	int    ok = 1;
	const  int is_short = BIN(flags & fomdfShort);
	const  uint32 org_opt_max_duck_val = is_short ? rTsPack.Rec.OptMaxDuck_S : rTsPack.Rec.OptMaxDuck;
	uint32 cur_opt_max_duck_val = org_opt_max_duck_val;
	PPObjTimeSeries::TrainNnParam tnnp(rTsPack, PPObjTimeSeries::TrainNnParam::afAnalyzeFrame);
	tnnp.SpikeQuant_s = rTsPack.Rec.SpikeQuant_t;
	// @v10.7.4 tnnp.EpochCount = 1;
	tnnp.InputFrameSize = 0;
	tnnp.MaxDuckQuant = 0;
	// @v10.7.4 tnnp.StakeThreshold = 0.05;
	//assert(oneof2(P.CloseMode, tnnp.clsmodFullMaxDuck, tnnp.clsmodAdjustLoss));
	//tnnp.StakeCloseMode = static_cast<uint16>(P.CloseMode);
	tnnp.StakeCloseMode = tnnp.clsmodAdjustLoss;
	SETFLAG(tnnp.BaseFlags, tnnp.bfShort, is_short);
	{
		PPObjTimeSeries::FactorToResultRelation opt_max_duck;
		PPObjTimeSeries::FactorToResultRelation opt_peak;
		{
			IntRange md_range;
			int    md_step = 0;
			if(flags & fomdfEntireRange) {
				md_range.Set(20, 1000);
				md_step = 20;
			}
			else {
				md_range.Set(50, 500);
				md_step = 50;
			}
			TSVector <PPObjTimeSeries::FactorToResultRelation> opt_max_duck_set;
			THROW(TsObj.FindOptimalFactor(rTsTmList, rTsValList, tnnp, 0/*what*/, md_range, md_step, BIN(flags & fomdfEntireRange), opt_max_duck_set, opt_max_duck));
			cur_opt_max_duck_val = opt_max_duck.FactorQuant;
		}
	}
	if(flags & fomdfStoreResult) {
		PPID   temp_id = rTsPack.Rec.ID;
		PPTimeSeriesPacket ts_pack_to_upd;
		THROW(TsObj.GetPacket(temp_id, &ts_pack_to_upd) > 0);
		if(is_short)
			ts_pack_to_upd.Rec.OptMaxDuck_S = cur_opt_max_duck_val;
		else
			ts_pack_to_upd.Rec.OptMaxDuck = cur_opt_max_duck_val;
		THROW(TsObj.PutPacket(&temp_id, &ts_pack_to_upd, 1));
	}
	CATCHZOK
	ASSIGN_PTR(pResult, cur_opt_max_duck_val);
	return ok;
}

void PPObjTimeSeries::TrendEntry::SqrtErrList(StatBase * pS)
{
	for(uint trlidx = 0; trlidx < ErrL.getCount(); trlidx++) {
		double err = ErrL.at(trlidx);
		assert(err >= 0.0);
		if(err > 0.0) {
			double sqr_err = sqrt(err);
			CALLPTRMEMB(pS, Step(sqr_err));
			ErrL.at(trlidx) = sqr_err;
		}
	}
}

int SLAPI PrcssrTsStrategyAnalyze::MakeArVectors(const STimeSeries & rTs, const LongArray & rFrameSizeList, uint flags, double partitialTrendErrLimit, TSCollection <PPObjTimeSeries::TrendEntry> & rTrendListSet)
{
	class MakeArVectorTask : public SlThread_WithStartupSignal { // @v10.7.3 (SlThread-->SlThread_WithStartupSignal)
	public:
		struct InitBlock {
			InitBlock(const STimeSeries * pTs, uint inputFrameSize, long flags, double partitialTrendErrLimit, PPObjTimeSeries::TrendEntry * pResult) :
				P_Ts(pTs), P_ValueList(0), InputFrameSize(inputFrameSize), Flags(flags), PartitialTrendErrLimit(partitialTrendErrLimit), P_Result(pResult)
			{
			}
			InitBlock(const RealArray * pValueList, uint inputFrameSize, long flags, double partitialTrendErrLimit, PPObjTimeSeries::TrendEntry * pResult) :
				P_Ts(0), P_ValueList(pValueList), InputFrameSize(inputFrameSize), Flags(flags), PartitialTrendErrLimit(partitialTrendErrLimit), P_Result(pResult)
			{
			}
			const STimeSeries * P_Ts;
			const RealArray * P_ValueList;
			const uint InputFrameSize;
			const long Flags;
			const double PartitialTrendErrLimit;
			PPObjTimeSeries::TrendEntry * P_Result;
		};
		MakeArVectorTask(InitBlock * pBlk) : SlThread_WithStartupSignal(0), B(*pBlk)
		{
			// @v10.7.3 (SlThread-->SlThread_WithStartupSignal) InitStartupSignal();
		}
		virtual void Run()
		{
			assert(SLS.GetConstTLA().Id == GetThreadID());
			const  uint tsc = B.P_Ts ? B.P_Ts->GetCount() : (B.P_ValueList ? B.P_ValueList->getCount() : 0);
			RealArray error_list;
			STimeSeries::AnalyzeFitParam afp(B.InputFrameSize, 0, 0);
			int afr = 0;
			if(B.P_Ts)
				afr = B.P_Ts->AnalyzeFit("close", afp, &B.P_Result->TL, 0, &error_list, 0, 0);
			else if(B.P_ValueList) 
				afr = STimeSeries::AnalyzeFit(*B.P_ValueList, afp, &B.P_Result->TL, 0, &error_list, 0, 0);
			if(afr) {
				assert(B.P_Result->TL.getCount() == tsc);
				assert(error_list.getCount() == tsc);
				{
					StatBase trls(0);
					B.P_Result->ErrL = error_list;
					// @v10.7.3 {
					error_list.freeAll();
					B.P_Result->TL.Shrink();
					B.P_Result->ErrL.Shrink();
					// } @v10.7.3 
					if(B.Flags & mavfDontSqrtErrList) {
						PROFILE_START
						for(uint trlidx = 0; trlidx < B.P_Result->ErrL.getCount(); trlidx++) {
							const double err = B.P_Result->ErrL.at(trlidx);
							assert(err >= 0.0);
							if(err > 0.0) {
								const double sqr_err = sqrt(err);
								trls.Step(sqr_err);
							}
						}
						PROFILE_END
					}
					else {
						PROFILE_START
						B.P_Result->SqrtErrList(&trls);
						PROFILE_END
					}
					trls.Finish();
					B.P_Result->ErrAvg = trls.GetExp();
					// @v10.7.4 {
					if(B.PartitialTrendErrLimit > 0.0 && B.PartitialTrendErrLimit <= 1.0) {
						error_list = B.P_Result->ErrL;
						error_list.Sort();
						uint lim_pos = static_cast<uint>(R0i(B.PartitialTrendErrLimit * static_cast<double>(error_list.getCount())));
						if(lim_pos >= 0 && lim_pos < error_list.getCount()) {
							B.P_Result->ErrLimitByPel = error_list.at(lim_pos) / trls.GetExp();
						}
					}
					// } @v10.7.4 
				}
			}
			else {
				// @todo @error
			}
		}
	private:
		// @v10.7.3 (SlThread-->SlThread_WithStartupSignal) virtual void SLAPI Startup() { SlThread::Startup(); SignalStartup(); }
		InitBlock B;
	};
	int    ok = 1;
	const  uint tsc = rTs.GetCount();
	SString msg_buf;
	RealArray temp_real_list;
	RealArray value_list;
	rTrendListSet.freeAll();

	const size_t thread_limit = 32;
	PPObjTimeSeries::TrendEntry * thread_result_list[thread_limit];
	uint   thread_inpfrmsz_list[thread_limit];
	const  uint max_threads = 16; // @20200318 4-->16
	uint   thr_idx = 0;

	uint   vec_idx = 0;
	rTs.GetValueVecIndex("close", &vec_idx);
	const int gvr = rTs.GetRealArray(vec_idx, 0, tsc, value_list);
	THROW(gvr);
	memzero(thread_result_list, sizeof(thread_result_list));
	memzero(thread_inpfrmsz_list, sizeof(thread_inpfrmsz_list));
	for(uint ifsidx = 0; ifsidx < rFrameSizeList.getCount(); ifsidx++) {
		const uint input_frame_size = static_cast<uint>(rFrameSizeList.get(ifsidx));
		assert(PPObjTimeSeries::SearchTrendEntry(rTrendListSet, input_frame_size) == 0);
		PPObjTimeSeries::TrendEntry * p_new_trend_entry = new PPObjTimeSeries::TrendEntry(input_frame_size, tsc);
		THROW_SL(p_new_trend_entry);
		if(max_threads == 0) {
			PROFILE_START
			{
				{
					msg_buf.Z().Cat("AnalyzeFit").Space().Cat(rTs.GetSymb()).Space().Cat(input_frame_size);
					PPWaitMsg(msg_buf);
				}
				STimeSeries::AnalyzeFitParam afp(input_frame_size, 0, 0);
				//THROW(rTs.AnalyzeFit("close", afp, &p_new_trend_entry->TL, 0, &temp_real_list, 0, 0));
				THROW(STimeSeries::AnalyzeFit(value_list, afp, &p_new_trend_entry->TL, 0, &temp_real_list, 0, 0));
				assert(p_new_trend_entry->TL.getCount() == tsc);
				assert(temp_real_list.getCount() == tsc);
				{
					StatBase trls(0);
					p_new_trend_entry->ErrL = temp_real_list;
					if(flags & mavfDontSqrtErrList) {
						for(uint trlidx = 0; trlidx < p_new_trend_entry->ErrL.getCount(); trlidx++) {
							double err = p_new_trend_entry->ErrL.at(trlidx);
							assert(err >= 0.0);
							if(err > 0.0) {
								double sqr_err = sqrt(err);
								trls.Step(sqr_err);
							}
						}
					}
					else {
						p_new_trend_entry->SqrtErrList(&trls);
					}
					trls.Finish();
					p_new_trend_entry->ErrAvg = trls.GetExp();
				}
				{
					PPWait(0);
				}
			}
			PROFILE_END
			rTrendListSet.insert(p_new_trend_entry);
		}
		else {
			assert(thr_idx <= max_threads);
			if(thr_idx >= max_threads) {
				HANDLE objs_to_wait[thread_limit];
				size_t objs_to_wait_count = 0;
				{
					for(uint i = 0; i < thr_idx; i++) {
						MakeArVectorTask::InitBlock tb(/*&rTs*/&value_list, thread_inpfrmsz_list[i], flags, partitialTrendErrLimit, thread_result_list[i]);
						MakeArVectorTask * p_thread = new MakeArVectorTask(&tb);
						THROW_S(p_thread, SLERR_NOMEM);
						p_thread->Start(1/*0*/);
						objs_to_wait[objs_to_wait_count++] = *p_thread;
					}
				}
				::WaitForMultipleObjects(objs_to_wait_count, objs_to_wait, TRUE, INFINITE);
				{
					for(uint i = 0; i < thr_idx; i++) {
						rTrendListSet.insert(thread_result_list[i]);
					}
				}
				thr_idx = 0;
				memzero(thread_result_list, sizeof(thread_result_list));
				memzero(thread_inpfrmsz_list, sizeof(thread_inpfrmsz_list));
			}
			thread_result_list[thr_idx] = p_new_trend_entry;
			thread_inpfrmsz_list[thr_idx] = input_frame_size;
			thr_idx++;
		}
	}
	if(thr_idx) {
		assert(max_threads > 0);
		HANDLE objs_to_wait[thread_limit];
		size_t objs_to_wait_count = 0;
		{
			for(uint i = 0; i < thr_idx; i++) {
				MakeArVectorTask::InitBlock tb(&rTs, thread_inpfrmsz_list[i], flags, partitialTrendErrLimit, thread_result_list[i]);
				MakeArVectorTask * p_thread = new MakeArVectorTask(&tb);
				THROW_S(p_thread, SLERR_NOMEM);
				p_thread->Start(1/*0*/);
				objs_to_wait[objs_to_wait_count++] = *p_thread;
			}
		}
		::WaitForMultipleObjects(objs_to_wait_count, objs_to_wait, TRUE, INFINITE);
		{
			for(uint i = 0; i < thr_idx; i++) {
				rTrendListSet.insert(thread_result_list[i]);
			}
		}
		thr_idx = 0;
		memzero(thread_result_list, sizeof(thread_result_list));
		memzero(thread_inpfrmsz_list, sizeof(thread_inpfrmsz_list));
	}
	CATCHZOK
	return ok;
}

struct TsFindStrategiesLoopBlock {
	TsFindStrategiesLoopBlock(const PPTssModelPacket & rTssModel, const PPObjTimeSeries::Config & rCfg, const PPTimeSeriesPacket & rTsPack, 
		const STimeSeries & rTs, const DateTimeArray & rTsTmList, const RealArray & rTsValList, 
		TSCollection <PPObjTimeSeries::TrendEntry> & rTrendList, PPObjTimeSeries::StrategyContainer & rSContainer, TSCollection <TestStrategyRawResult> * pTsrrList) :
		R_TssModel(rTssModel), R_Cfg(rCfg), R_TsPack(rTsPack), R_Ts(rTs), R_TsTmList(rTsTmList), R_TsValList(rTsValList), 
		R_TrendList(rTrendList), R_SContainer(rSContainer), P_TsrrList(pTsrrList), P_FOut(0)
	{
	}
	const PPTssModelPacket & R_TssModel;
	const PPTimeSeriesPacket & R_TsPack;
	const STimeSeries & R_Ts;
	const DateTimeArray & R_TsTmList;
	const RealArray & R_TsValList;
	TSCollection <PPObjTimeSeries::TrendEntry> & R_TrendList; // @v10.7.3
	const PPObjTimeSeries::Config & R_Cfg; // @v10.7.3
	PPObjTimeSeries::StrategyContainer & R_SContainer;
	TSCollection <TestStrategyRawResult> * P_TsrrList; // @v10.7.7
	SFile * P_FOut;
};

struct TsFindStrategiesBlock {
	TsFindStrategiesBlock(
		const PPTssModelPacket & rTssModel,
		const PPObjTimeSeries::Config & rCfg,
		const PPTimeSeriesPacket & rTsPack, 
		const DateTimeArray & rTsTmList, 
		const RealArray & rTsValList, 
		const TSCollection <PPObjTimeSeries::TrendEntry> & rTrendList,
		const PPObjTimeSeries::TrainNnParam & rTnnp2, int optFactorSide, int stakeSide, 
		PPObjTimeSeries::StrategyContainer * pSContainer, TestStrategyRawResult * pTsrr) :
		R_TssModel(rTssModel),
		R_Cfg(rCfg), R_TsPack(rTsPack), R_TsTmList(rTsTmList), R_TsValList(rTsValList), 
		R_TrendList(rTrendList),
		R_Tnnp2(rTnnp2), OptFactorSide(optFactorSide), StakeSide(stakeSide), P_SContainer(pSContainer), P_Tsrr(pTsrr), /*P_FDump(0),*/ P_FOut(0)
	{
	}
	enum TsTestStrategyOfrOption {
		// @v10.8.0 tstsofroMode1        = 0x0001,
		tstsofroPositive                 = 0x1000,
		tstsofroNegative                 = 0x2000,
		tstsofroTotalOnly                = 0x4000, // @v10.7.7
		tstsofroEvaluateResonanceMeasure = 0x8000  // @v10.8.0
	};
	uint RadialIncrement(const TSVector <StrategyOptEntry> & rList, uint curIdx, uint lastIdx, double stepAngle) const
	{
		const double _start_factor_angle = atan(rList.at(curIdx).Factor);
		for(uint k = curIdx+1; k <= lastIdx; k++) {
			const double _cur_factor_angle = atan(rList.at(k).Factor);
			if((_cur_factor_angle - _start_factor_angle) >= stepAngle)
				return k;
		}
		return lastIdx;
	}
	//
	// useInitialSplitting:
	//   0 - первая итерация, как и последующие, делит список на две равные части
	//   1 - первая итерация делит список на положительную и отрицательную части
	//   2 - первая итерация запускает обработку только положительных значений списка
	//   3 - первая итерация запускает обработку только отрицательных значений списка
	//   4 - последовательное сканирование всего диапазона фреймами для нахождения максимального
	//   5 - последовательное сканирование всего диапазона фреймами для нахождения максимального (только положительные значения)
	//   6 - последовательное сканирование всего диапазона фреймами для нахождения максимального (только отрицательные значения)
	//
	int SLAPI FindOptimalFactorRange(const TSVector <StrategyOptEntry> & rList, int useInitialSplitting, 
		TSVector<PPObjTimeSeries::OptimalFactorRange_WithPositions> * pRc, TestStrategyRawResult * pRawResult) const
	{
		int    ok = -1;
		const  uint _c = rList.getCount();
		CalcFactorRangeResult2_Func cfrrFunc = 0; //(rMp.Flags & rMp.fOptRangeTarget_Velocity) ? CalcFactorRangeResult2_R1DivR2 : CalcFactorRangeResult2Sum;
		const PPTssModel & r_tssm = R_TssModel.Rec;
		if(r_tssm.OptTargetCriterion == PPTssModel::tcAmount) {
			//cfrrFunc = CalcFactorRangeResult2Sum_SpecMult;
			cfrrFunc = CalcFactorRangeResult2Sum;
		}
		else if(r_tssm.OptTargetCriterion == PPTssModel::tcVelocity)
			cfrrFunc = CalcFactorRangeResult2_R1DivR2;
		else if(r_tssm.OptTargetCriterion == PPTssModel::tcWinRatio)
			cfrrFunc = CalcFactorRangeResult2_R2DivC;
		else if(r_tssm.OptTargetCriterion == PPTssModel::tcAngleRatio) // @v10.7.9
			cfrrFunc = CalcFactorRangeResult2_R2DivAngle;
		assert(cfrrFunc != 0);
		CALLPTRMEMB(pRc, clear());
		// interval_exp_direction: 0 - не раздвигать интервалы, 1 - только расширять, 2 - только сжимать, 3 - расширять и сжимать
		const  int  interval_exp_direction = 1; // Как показали эксперименты сжимать интервал - плохая затея: при сжатии результат может улучшаться, но добротность
			// стратегий сильно страдает из-за снижения репрезентативности.
		const  uint maxprobe = (r_tssm.OptRangeMaxExtProbe > 0) ? r_tssm.OptRangeMaxExtProbe : 1;
		uint   _first_idx = 0;
		uint   _last_idx = 0;
		if(FindOptimalFactorRange_InitialSplitting(rList, useInitialSplitting, &_first_idx, &_last_idx)) {
			double total_max_result = 0.0;
			TSVector <IntRange> pos_range_list;
			IntRange work_range;
			const  double _real_item_count = static_cast<double>(_last_idx - _first_idx + 1);
			int    use_radial_partitioning = 0;
			uint   sf_step = 0;
			uint   sf_step_increment = 1;
			double sf_step_angle = 0.0; // for (rTssModel.Rec.OptRangeStep_Measure == PPTssModel::orsMkRadian)
			const double _start_factor_angle = atan(rList.at(_first_idx).Factor);
			const double _end_factor_angle = atan(rList.at(_last_idx).Factor);
			switch(r_tssm.OptRangeStep_Measure) {
				case PPTssModel::orsUndef:
					if(r_tssm.Flags & PPTssModel::fOptRangeStepAsMkPart_) {
						sf_step = ffloori(_real_item_count * static_cast<double>(r_tssm.OptRangeStep_) / 1000000.0);
						sf_step_increment = 10;
					}
					else if(r_tssm.OptRangeStep_Measure == PPTssModel::orsAbsolute) {
						sf_step = (r_tssm.OptRangeStep_ > 0) ? r_tssm.OptRangeStep_ : PPTssModel::Default_OptRangeStep;
						sf_step_increment = (r_tssm.OptRangeStep_/10); // @20200611 1-->(r_tssm.OptRangeStep_/10)
					}
					break;
				case PPTssModel::orsMkPart:
					sf_step = ffloori(_real_item_count * static_cast<double>(r_tssm.OptRangeStep_) / 1000000.0);
					sf_step_increment = 10;
					break;
				case PPTssModel::orsAbsolute:
					sf_step = (r_tssm.OptRangeStep_ > 0) ? r_tssm.OptRangeStep_ : PPTssModel::Default_OptRangeStep;
					sf_step_increment = 1;
					break;
				case PPTssModel::orsLog:
					{
						//sf_step = ffloori(log(_real_item_count) * static_cast<double>(rTssModel.Rec.OptRangeStep_));
						//
						const double angle_step_count = (100.0 * _real_item_count) / (log(_real_item_count) * static_cast<double>(r_tssm.OptRangeStep_));
						sf_step_angle = ((_end_factor_angle - _start_factor_angle) / angle_step_count);
						sf_step = 1;
						use_radial_partitioning = 1;
					}
					break;
				case PPTssModel::orsRadialPart:
					sf_step_angle = ((_end_factor_angle - _start_factor_angle) * static_cast<double>(r_tssm.OptRangeStep_)) / 10000000.0; // @202005002 1000000.0-->10000000.0
					sf_step = 1; // calculated during the loop
					use_radial_partitioning = 1;
					break;
				default:
					const int undefined_opt_range_step_measure = 0;
					assert(undefined_opt_range_step_measure);
					break;
			}
			if(pRawResult) {
				pRawResult->SoList.clear();
				if(_last_idx >= _first_idx) {
					pRawResult->FactorR.low = _start_factor_angle;
					pRawResult->FactorR.upp = _end_factor_angle;
					pRawResult->SoList.insertChunk(_last_idx - _first_idx + 1, &rList.at(_first_idx));
					if(pRawResult->SoList.getCount()) {
						work_range.Set(0, pRawResult->SoList.getCount()-1);
						pRawResult->TotalProb = CalcFactorRangeResult2_R2DivC_Simple(pRawResult->SoList, work_range);
						pRawResult->TotalResult = CalcFactorRangeResult2Sum(pRawResult->SoList, work_range);
						if(pRawResult->DoCalcResonanceMeasure) {
							uint   lo_idx = 0;
							uint   up_idx = _c-1;
							const uint sfidx_increment = 1;
							double total_win = 0.0;
							double total_pt = 0;
							for(uint sfidx = _first_idx; sfidx < _last_idx; sfidx += sfidx_increment) { 
								uint sfidx_up = 0;
								if(use_radial_partitioning) {
									assert(sf_step_angle > 0.0);
									sfidx_up = RadialIncrement(rList, sfidx, _last_idx, sf_step_angle);
									assert(sfidx_up <= _last_idx);
								}
								else {
									sfidx_up = smin(sfidx+sf_step-1, _last_idx);
								}
								if(sfidx_up > sfidx) {
									work_range.Set(sfidx, sfidx_up);
									const double _result = CalcFactorRangeResult2Sum(rList, work_range);
									if(_result > 0.0) {
										double s2 = rList.sumDouble(offsetof(StrategyOptEntry, Result2), work_range.low, work_range.upp);
										double c = static_cast<double>(work_range.upp - work_range.low + 1);
										const double _prob = fdivnz(s2, c);
										total_win += s2;
										total_pt += c;
									}
								}
								else
									break;
							}
							pRawResult->ResonanceMeasure = fdivnz(total_win, total_pt);
						}
						else {
							const double win_prob_threshould = 0.8;
							pRawResult->BestStep = 0;
							pRawResult->BestStepBarCount = 0;
							pRawResult->BestStepResult = 0.0;
							double best_step_effective_result = 0.0;
							double _effective_result_sum = 0.0;
							double _result_sum = 0.0;
							double _resultq_sum = 0.0; // @20200515 Результат, взвешенный по q-фактору
							TSVector <IntRange> work_range_list;
							{
								for(uint angle_step = 10; angle_step <= 3000; angle_step += 10) {
									const double local_sf_step_angle = ((_end_factor_angle - _start_factor_angle) * static_cast<double>(angle_step)) / 10000000.0;
									TestStrategyRawResult::StepHistogramItem shi;
									StatBase shi_stat;
									shi.StepDot = 0;
									shi.StepAngle = angle_step;
									_effective_result_sum = 0.0;
									_result_sum = 0.0;
									work_range_list.clear();
									const double _spc_mult = 1.0;
									for(uint sfidx = _first_idx; sfidx < _last_idx;) { 
										assert(local_sf_step_angle > 0.0);
										uint sfidx_up = 0;
										sfidx_up = RadialIncrement(rList, sfidx, _last_idx, local_sf_step_angle);
										assert(sfidx_up <= _last_idx);
										if(sfidx_up > sfidx) {
											const uint pt_count = (sfidx_up-sfidx+1);
											shi_stat.Step(pt_count);
											work_range.Set(sfidx, sfidx_up);
											const double _result = CalcFactorRangeResult2Sum(rList, work_range);
											const double _prob = CalcFactorRangeResult2_R2DivC_Simple(rList, work_range);
											if(_result > 0.0 && _prob >= win_prob_threshould) {
												_result_sum += _result;
												_resultq_sum += _result * 2.0 * log(static_cast<double>(pt_count)); // @20200515
												_effective_result_sum += (_result * _spc_mult);
												work_range_list.insert(&work_range);
												shi.SuccCount++;
											}
										}
										sfidx = sfidx_up+1;
									}
									shi_stat.Finish();
									shi.Mean = shi_stat.GetExp();
									shi.StdDev = shi_stat.GetStdDev();
									shi.Result = _result_sum;
									shi.ResultQ = _resultq_sum; // @20200515
									shi.Prob = 0.0;
									{
										double _s2 = 0.0;
										double _c2 = 0.0;
										for(uint wrlidx = 0; wrlidx < work_range_list.getCount(); wrlidx++) {
											const IntRange & r_item = work_range_list.at(wrlidx);
											double s2 = rList.sumDouble(offsetof(StrategyOptEntry, Result2), r_item.low, r_item.upp);
											double c = static_cast<double>(r_item.upp - r_item.low + 1);
											_s2 += s2;
											_c2 += c;
										}
										shi.Prob = fdivnz(_s2, _c2);
										assert(shi.SuccCount == work_range_list.getCount());
									}
									pRawResult->SahList.insert(&shi);
								}
							}
							{
								for(uint step = 1; step <= 3000; step++) {
									TestStrategyRawResult::StepHistogramItem shi;
									StatBase shi_stat;
									shi.StepDot = step;
									shi.StepAngle = 0.0;
									_effective_result_sum = 0.0;
									_result_sum = 0.0;
									_resultq_sum = 0.0;
									work_range_list.clear();
									//const double _spc_mult = log(static_cast<double>(step));
									const double _spc_mult = exp(static_cast<double>(step) / _real_item_count);
									for(uint sfidx = _first_idx; sfidx < (_last_idx-step); sfidx += step) { 
										if((sfidx+step-1) <= _last_idx) {
											uint sfidx_up = MIN(sfidx+step-1, _last_idx);
											double step_angle = atan(rList.at(sfidx_up).Factor) - atan(rList.at(sfidx).Factor);
											shi_stat.Step(step_angle);
											work_range.Set(sfidx, sfidx_up);
											const double _result = CalcFactorRangeResult2Sum(rList, work_range);
											const double _prob = CalcFactorRangeResult2_R2DivC_Simple(rList, work_range);
											if(_result > 0.0 && _prob >= win_prob_threshould) {
												const uint pt_count = (sfidx_up-sfidx+1);
												_result_sum += _result;
												_resultq_sum += _result * 2.0 * log(static_cast<double>(pt_count)); // @20200515
												_effective_result_sum += (_result * _spc_mult);
												work_range_list.insert(&work_range);
												shi.SuccCount++;
											}
										}
										else
											break;
									}
									shi_stat.Finish();
									shi.Mean = shi_stat.GetExp();
									shi.StdDev = shi_stat.GetStdDev();
									shi.Result = _result_sum;
									shi.ResultQ = _resultq_sum; // @20200515
									shi.Prob = 0.0;
									{
										double _s2 = 0.0;
										double _c2 = 0.0;
										for(uint wrlidx = 0; wrlidx < work_range_list.getCount(); wrlidx++) {
											const IntRange & r_item = work_range_list.at(wrlidx);
											double s2 = rList.sumDouble(offsetof(StrategyOptEntry, Result2), r_item.low, r_item.upp);
											double c = static_cast<double>(r_item.upp - r_item.low + 1);
											_s2 += s2;
											_c2 += c;
										}
										shi.Prob = fdivnz(_s2, _c2);
										assert(shi.SuccCount == work_range_list.getCount());
									}
									pRawResult->SdhList.insert(&shi);
									if(_effective_result_sum > best_step_effective_result) {
										best_step_effective_result = _effective_result_sum;
										pRawResult->BestStepResult = _result_sum;
										pRawResult->BestStep = step;
										{
											double _s2 = 0.0;
											double _c2 = 0.0;
											for(uint wrlidx = 0; wrlidx < work_range_list.getCount(); wrlidx++) {
												const IntRange & r_item = work_range_list.at(wrlidx);
												double s2 = rList.sumDouble(offsetof(StrategyOptEntry, Result2), r_item.low, r_item.upp);
												double c = static_cast<double>(r_item.upp - r_item.low + 1);
												_s2 += s2;
												_c2 += c;
											}
											pRawResult->BestStepProb = fdivnz(_s2, _c2);
											pRawResult->BestStepBarCount = work_range_list.getCount();
										}
									}
								}
							}
						}
					}
				}
			}
			if(pRc) {
				const  int single_scan_algorithm = 0; // @v10.7.8 
				// (must be done by caller) rList.sort(PTR_CMPFUNC(double));
				const uint sf_step_count = (r_tssm.OptRangeStepCount > 0) ? r_tssm.OptRangeStepCount : PPTssModel::Default_OptRangeStepCount;
				const uint max_range_count = (r_tssm.OptRangeMultiLimit > 0) ? r_tssm.OptRangeMultiLimit : PPTssModel::Default_OptRangeMultiLimit;
				for(int do_next_iter = 1; do_next_iter;) {
					do_next_iter = 0;
					FindOptimalFactorRangeExtrEntry _sfd_extr_entry;
					for(uint sfdelta = 1; sfdelta <= sf_step_count; sfdelta++) {
						double prev_result = 0.0; // @v20200405 total_sum-->-1000000000.0-->0.0
						uint   lo_idx = 0;
						uint   up_idx = _c-1;
						uint   iter_no = 0;
						FindOptimalFactorRangeExtrEntry _extr_entry;
						const uint sfidx_increment = 1;
						for(uint sfidx = _first_idx; sfidx < _last_idx; sfidx += sfidx_increment) { 
							uint sfidx_up = 0;
							// Пока результат начальных точек отрицательный можно смело двигаться вперед 
							while(sfidx < _last_idx && rList.at(sfidx).Result1 < 0.0)
								sfidx++;
							if(use_radial_partitioning) {
								assert(sf_step_angle > 0.0);
								sfidx_up = RadialIncrement(rList, sfidx, _last_idx, sf_step_angle*sfdelta);
								assert(sfidx_up <= _last_idx);
							}
							else {
								sfidx_up = smin(sfidx+(sf_step+(sf_step_increment*(sfdelta-1)))-1, _last_idx);
							}
							// Пытаемся расширить номинальных диапазон за счет сдвига вправо до тех пор пока по точкам результат положительный
							while(sfidx_up < _last_idx && rList.at(sfidx_up).Result1 > 0.0)
								sfidx_up++;
							if(sfidx_up > sfidx) {
								if(!IsThereIntRangeIntersection_OnSortedList(pos_range_list, work_range.Set(sfidx, sfidx_up))) {
									const double _result = cfrrFunc(rList, work_range);
									if(_result > _extr_entry.MaxResult) {
										_extr_entry.MaxResult = _result;
										_extr_entry.MaxSfIdx = sfidx;
										_extr_entry.MaxSfIdxUp = sfidx_up;
									}
								}
							}
							else
								break;
						}
						if(_extr_entry.MaxResult > 0.0) {
							lo_idx = _extr_entry.MaxSfIdx;
							up_idx = _extr_entry.MaxSfIdxUp;
						}
						if(interval_exp_direction & 1) { // расширять интервал
							while(lo_idx > 0) {
								int done = 0;
								for(uint probedeep = 1; !done && probedeep <= maxprobe && probedeep <= lo_idx; probedeep++) { // @v10.3.12 @fix (probedeep > lo_idx)-->(probedeep <= lo_idx)
									if(!IsThereIntRangeIntersection_OnSortedList(pos_range_list, work_range.Set(lo_idx-probedeep, up_idx)/*, 0*/)) {
										const double temp_result = cfrrFunc(rList, work_range);
										if(prev_result < temp_result) {
											prev_result = temp_result;
											lo_idx -= probedeep;
											done = 1;
										}
									}
								}
								if(!done)
									break;
							}
							while((up_idx+1) < _c) {
								int done = 0;
								for(uint probedeep = 1; !done && probedeep <= maxprobe && (up_idx+probedeep) < _c; probedeep++) {
									if(!IsThereIntRangeIntersection_OnSortedList(pos_range_list, work_range.Set(lo_idx, up_idx+probedeep)/*, 0*/)) {
										const double temp_result = cfrrFunc(rList, work_range);
										if(prev_result < temp_result) {
											prev_result = temp_result;
											up_idx += probedeep;
											done = 1;
										}
									}
								}
								if(!done)
									break;
							}
						}
						if(interval_exp_direction & 2) { // сжимать интервал
							while(lo_idx < up_idx) { // Сдвигаем нижнюю границу вверх
								int done = 0;
								for(uint probedeep = 1; !done && probedeep <= maxprobe && (lo_idx+probedeep) <= up_idx; probedeep++) {
									if(!IsThereIntRangeIntersection_OnSortedList(pos_range_list, work_range.Set(lo_idx+probedeep, up_idx)/*, 0*/)) {
										const double temp_result = cfrrFunc(rList, work_range);
										if(prev_result < temp_result) {
											prev_result = temp_result;
											lo_idx += probedeep;
											done = 1;
										}
									}
								}
								if(!done)
									break;
							}
							while(lo_idx < up_idx) { // Сдвигаем верхнюю границу вниз
								int done = 0;
								for(uint probedeep = 1; !done && probedeep <= maxprobe && (up_idx) >= (lo_idx+probedeep); probedeep++) {
									if(!IsThereIntRangeIntersection_OnSortedList(pos_range_list, work_range.Set(lo_idx, up_idx-probedeep)/*, 0*/)) {
										const double temp_result = cfrrFunc(rList, work_range);
										if(prev_result < temp_result) {
											prev_result = temp_result;
											up_idx -= probedeep;
											done = 1;
										}
									}
								}
								if(!done)
									break;
							}
						}
						if(_sfd_extr_entry.MaxResult < prev_result) {
							_sfd_extr_entry.MaxResult = prev_result;
							_sfd_extr_entry.MaxSfIdx = lo_idx;
							_sfd_extr_entry.MaxSfIdxUp = up_idx;
						}
					}
					if(_sfd_extr_entry.MaxResult > 0.0 && (_sfd_extr_entry.MaxSfIdx > 0 || _sfd_extr_entry.MaxSfIdxUp < (_c-1))) {
						int   do_reckon = 0;
						if(total_max_result == 0.0) {
							total_max_result = _sfd_extr_entry.MaxResult;
							do_reckon = 1;
						}
						else {
							if(max_range_count)
								do_reckon = 1;
							else if(r_tssm.OptTargetCriterion == PPTssModel::tcVelocity) {
								if((_sfd_extr_entry.MaxResult / total_max_result) >= 0.5) {
									// total_max_result не обновляем: нам нужен самый первый результат в качестве такового, ибо он - максимальный (следующие меньше)
									do_reckon = 1;
								}
							}
							else if(oneof2(r_tssm.OptTargetCriterion, PPTssModel::tcWinRatio, PPTssModel::tcAngleRatio)) { // @v10.7.9 PPTssModel::tcAngleRatio
								total_max_result = _sfd_extr_entry.MaxResult;
								do_reckon = 1;
							}
							else if(_sfd_extr_entry.MaxResult > total_max_result) {
								total_max_result = _sfd_extr_entry.MaxResult;
								do_reckon = 1;
							}
						}
						if(do_reckon) {
							work_range.Set(_sfd_extr_entry.MaxSfIdx, _sfd_extr_entry.MaxSfIdxUp);
							PPObjTimeSeries::OptimalFactorRange_WithPositions new_range;
							new_range.Set(rList.at(_sfd_extr_entry.MaxSfIdx).Factor, rList.at(_sfd_extr_entry.MaxSfIdxUp).Factor);
							new_range.LoPos = _sfd_extr_entry.MaxSfIdx;
							new_range.UpPos = _sfd_extr_entry.MaxSfIdxUp;
							new_range.Result = _sfd_extr_entry.MaxResult;
							new_range.Count = (_sfd_extr_entry.MaxSfIdxUp - _sfd_extr_entry.MaxSfIdx + 1);
		#ifndef NDEBUG
							const double temp_result = cfrrFunc(rList, work_range);
							assert(feqeps(temp_result, new_range.Result, 1E-6));
		#endif
							assert(work_range.low == static_cast<int32>(new_range.LoPos));
							assert(work_range.upp == static_cast<int32>(new_range.UpPos));
							pos_range_list.ordInsert(&work_range, 0, PTR_CMPFUNC(_2long)); // @v10.7.7 insert-->ordInsert
							new_range.GenSeq = pRc->getCount()+1; // @v10.7.9
							new_range.GenPtCount = (_last_idx - _first_idx + 1); // @v10.7.11
							pRc->insert(&new_range);
							if(!max_range_count || pRc->getCount() < max_range_count)
								do_next_iter = 1;
							ok = 1;
						}
					}
				}
				FindOptimalFactorRange_PostProcess(rList, cfrrFunc, *pRc, single_scan_algorithm);
			}
		}
		return ok;
	}
	int SLAPI TestStrategy(const PPObjTimeSeries::TrendEntry & rTe, const PPObjTimeSeries::TrendEntry * pMainTrendEntry,
		const PPObjTimeSeries::Strategy & rS, int options, PPObjTimeSeries::StrategyResultEntry & rSre, 
		TSVector <PPObjTimeSeries::StrategyResultEntry> * pOptResultList, TestStrategyRawResult * pRawResult)
	{
		int    ok = 1;
		const  uint tsc = R_TsTmList.getCount();
		const  uint ifs = rS.InputFrameSize;
		const  uint main_ifs = (pMainTrendEntry && rS.MainFrameSize) ? rS.MainFrameSize : 0;
		const  double trend_err_limit = (rS.TrendErrLim * rS.TrendErrAvg); // @v10.3.12
		const  double main_trend_err_limit = main_ifs ? (rS.MainTrendErrLim * rS.MainTrendErrAvg) : 0.0; // @v10.7.0
		assert(tsc == R_TsValList.getCount());
		CALLPTRMEMB(pOptResultList, clear());
		if(tsc && tsc == R_TsValList.getCount()) {
			class LocalResultBlock {
			public:
				void FASTCALL Add(const PPObjTimeSeries::StrategyResultValueEx & rRvEx, long criterion)
				{
					ResultList.add(rRvEx.Result); // @optvector
					if(criterion == PPTssModel::tcVelocity)
						ResultAddendumList.add(static_cast<double>(rRvEx.TmSec));
					else // @v10.7.6 if(oneof2(criterion, PPTssModel::tcWinRatio, PPTssModel::tcAngleRatio))
						ResultAddendumList.add((rRvEx.Result > 0.0) ? 1.0 : 0.0);
				}
				void FASTCALL AddZero()
				{
					ResultList.add(0.0);
					ResultAddendumList.add(0.0);
				}
				RealArray ResultList;
				RealArray ResultAddendumList;
			};
			LocalResultBlock result_block;
			uint   signal_count = 0;
			rSre.LastResultIdx = 0;
			assert(tsc == rTe.TL.getCount());
			THROW(rTe.TL.getCount() == tsc);
			PROFILE_START
			if(rSre.StakeMode == 0) {
				for(uint i = 0; i < tsc; i++) {
					int    is_signal = 0; // @v10.7.6 @fix 1-->0
					PPObjTimeSeries::StrategyResultValueEx rv_ex;
					const double main_trend_val = pMainTrendEntry ? pMainTrendEntry->TL.at(i) : 0.0;
					if(!main_ifs || (i >= (main_ifs+1) && rS.OptDelta2Range.Check(main_trend_val))) {
						const  int csr = rS.CalcResult2(R_TsTmList, R_TsValList, i, rv_ex);
						THROW(csr);
						is_signal = BIN(csr == 2);
					}
					if(is_signal) {
						signal_count++;
						result_block.Add(rv_ex, R_TssModel.Rec.OptTargetCriterion);
						rSre.SetValue(rv_ex);
						rSre.LastResultIdx = i;
					}
					else
						result_block.AddZero();
				}
			}
			else if(rSre.StakeMode == 1) {
				const uint ifs_plus_one_max = MAX((ifs+1), (main_ifs+1));
				for(uint i = 0; i < tsc; i++) {
					PPObjTimeSeries::StrategyResultValueEx rv_ex;
					int    is_signal = 0;
					if(i >= ifs_plus_one_max) {
						int main_ifs_is_ok = 0;
						if(main_ifs) {
							if(rS.OptDelta2Range.Check(pMainTrendEntry->TL.at(i))) {
								const  double main_trend_err = pMainTrendEntry->ErrL.at(i); 
								if(main_trend_err_limit <= 0 || (main_trend_err > 0.0 && main_trend_err <= main_trend_err_limit))
									main_ifs_is_ok = 1;
							}
						}
						else
							main_ifs_is_ok = 1;
						if(main_ifs_is_ok) {
							const  double trend_err = rTe.ErrL.at(i);
							if((trend_err_limit <= 0.0 || (trend_err > 0.0 && trend_err <= trend_err_limit)) && rS.OptDeltaRange.Check(rTe.TL.at(i))) {
								const  int csr = rS.CalcResult2(R_TsTmList, R_TsValList, i, rv_ex);
								THROW(csr);
								if(csr == 2)
									rSre.LastResultIdx = i;
								is_signal = BIN(csr == 2);
							}
						}
					}
					if(is_signal) {
						signal_count++;
						result_block.Add(rv_ex, R_TssModel.Rec.OptTargetCriterion);
						rSre.SetValue(rv_ex);
					}
					else
						result_block.AddZero();
				}
			}
			if(signal_count) {
				assert(signal_count == rSre.StakeCount);
				const double peak_avg = rSre.GetPeakAverage();
				const double bottom_avg = rSre.GetBottomAverage();
				assert(peak_avg >= 0.0);
				assert(bottom_avg >= 0.0);
				if(rSre.StakeMode == 0) {
					rSre.PeakAvgQuant = static_cast<uint16>(R0i(peak_avg / rSre.SpikeQuant_s));
					rSre.BottomAvgQuant = static_cast<uint16>(R0i(bottom_avg / rSre.SpikeQuant_s));
				}
				rSre.PeakMaxQuant = static_cast<uint16>(R0i(rSre.MaxPeak / rSre.SpikeQuant_s)); // Для каждого из режимов
			}
			assert(result_block.ResultList.getCount() == tsc);
			assert(result_block.ResultAddendumList.getCount() == tsc);
			PROFILE_END
			if(pOptResultList) {
				const uint  first_correl_idx = ifs;
				TSVector <StrategyOptEntry> so_list;
				if(rSre.StakeMode == 0) {
					PROFILE_START
					so_list.clear();
					for(uint i = 0; i <= rSre.LastResultIdx; i++) {
						if(i >= (first_correl_idx+1)) {
							const double local_result = result_block.ResultList.at(i);
							if(local_result != 0.0) {
								const double trend_err = rTe.ErrL.at(i);
								if(trend_err_limit <= 0.0 || (trend_err > 0.0 && trend_err <= trend_err_limit)) {
									const double local_factor = rTe.TL.at(i);
									if(oneof3(R_TssModel.Rec.OptTargetCriterion, PPTssModel::tcVelocity, PPTssModel::tcWinRatio, PPTssModel::tcAngleRatio)) {
										const double local_addendum = result_block.ResultAddendumList.at(i);
										if(oneof2(R_TssModel.Rec.OptTargetCriterion, PPTssModel::tcWinRatio, PPTssModel::tcAngleRatio) || local_addendum > 0.0) {
											StrategyOptEntry entry(local_factor, local_result, local_addendum);
											THROW_SL(so_list.insert(&entry));
										}
									}
									else {
										StrategyOptEntry entry(local_factor, local_result);
										THROW_SL(so_list.insert(&entry));
									}
								}
							}
						}
					}
					so_list.sort(PTR_CMPFUNC(double));
					int initial_splitting = 4;
					if(options & tstsofroPositive)
						initial_splitting = 5;
					else if(options & tstsofroNegative)
						initial_splitting = 6;
					else
						initial_splitting = 4;
					{
						TSVector <PPObjTimeSeries::OptimalFactorRange_WithPositions> ofr_list;
						FindOptimalFactorRange(so_list, initial_splitting, 
							(options & tstsofroTotalOnly || (pRawResult && pRawResult->DoCalcResonanceMeasure)) ? 0 : &ofr_list, pRawResult);
						for(uint ofridx = 0; ofridx < ofr_list.getCount(); ofridx++) {
							PPObjTimeSeries::OptimalFactorRange & r_ofr = ofr_list.at(ofridx);
							PPObjTimeSeries::StrategyResultEntry sre_temp(rS, rS.StakeMode);
							sre_temp.OptDeltaRange = r_ofr;
							sre_temp.GenSeq = ofr_list.at(ofridx).GenSeq; // @v10.7.9
							sre_temp.GenPtCount = ofr_list.at(ofridx).GenPtCount; // @v10.7.11
							if(main_ifs) {
								sre_temp.OptDelta2Range = rS.OptDelta2Range;
								sre_temp.MainFrameSize = main_ifs;
							}
							else
								sre_temp.OptDelta2Range.Z();
							sre_temp.PeakAvgQuant = rSre.PeakAvgQuant;
							sre_temp.BottomAvgQuant = rSre.BottomAvgQuant;
							sre_temp.PeakMaxQuant = rSre.PeakMaxQuant;
							pOptResultList->insert(&sre_temp);
						}
					}
					PROFILE_END
				}
			}
		}
		CATCHZOK
		return ok;
	}
	const PPTssModelPacket & R_TssModel;
	const PPTimeSeriesPacket & R_TsPack;
	const DateTimeArray & R_TsTmList;
	const RealArray & R_TsValList;
	const PPObjTimeSeries::TrainNnParam & R_Tnnp2;
	const TSCollection <PPObjTimeSeries::TrendEntry> & R_TrendList;
	const PPObjTimeSeries::Config & R_Cfg; // @v10.7.3
	// OptFactorSide:
	// 0 - оптимизировать только в положительной области
	// 1 - оптимизировать только в отрицательной области
	// -1 - оптимизировать по всему множеству значений
	const    int OptFactorSide;
	const    int StakeSide; // 0 - long, 1 - short
	PPObjTimeSeries::StrategyContainer * P_SContainer;
	TestStrategyRawResult * P_Tsrr;
	SFile * P_FOut;
};

static SString & CatStrategyPrefix(int optFactorSide, SString & rBuf)
{
	return rBuf.Cat("M1").CatChar((optFactorSide == 0) ? '+' : ((optFactorSide == 1) ? '-' : '*'));
}

int SLAPI PrcssrTsStrategyAnalyze::FindStrategies(void * pBlk) const
{
	int    ok = 1;
	SString temp_buf;
	SString msg_buf;
	TsFindStrategiesBlock * p_blk = static_cast<TsFindStrategiesBlock *>(pBlk);
	PPObjTimeSeries::StrategyResultEntry sre(p_blk->R_Tnnp2, 0);
	TSVector <PPObjTimeSeries::StrategyResultEntry> sr_raw_list;
	PPObjTimeSeries::StrategyContainer temp_sc;
	TSVector <PPObjTimeSeries::StrategyResultValueEx> sr_detail_list; // @v10.7.9
	const  int ofs = p_blk->OptFactorSide;
	int tstso = (ofs == 0) ? (TsFindStrategiesBlock::tstsofroPositive) : ((ofs == 1) ? TsFindStrategiesBlock::tstsofroNegative : 0);
	// @v10.8.0 tstso |= TsFindStrategiesBlock::tstsofroMode1;
	if(!p_blk->P_SContainer)
		tstso |= TsFindStrategiesBlock::tstsofroTotalOnly;
	uint    lt_min_winrate_count = 0; // @v10.7.2 Количество стратегий, надежность которых меньше лимита. При превышении этим
		// значением max_lt_min_winrate_count прекращаем тестирование - скорее всего все остальные хуже.
	const uint max_lt_min_winrate_count = 100000; // @v10.7.3 2-->20 // @v10.7.8 20-->100000 (very varge in order to avoid limit)
	const PPObjTimeSeries::TrendEntry * p_trend_entry = PPObjTimeSeries::SearchTrendEntry(p_blk->R_TrendList, p_blk->R_Tnnp2.InputFrameSize);
	const PPObjTimeSeries::TrendEntry * p_main_trend_entry = p_blk->R_Tnnp2.MainFrameSize ? PPObjTimeSeries::SearchTrendEntry(p_blk->R_TrendList, p_blk->R_Tnnp2.MainFrameSize) : 0;
	THROW(p_blk->TestStrategy(*p_trend_entry, p_main_trend_entry, p_blk->R_Tnnp2, tstso, sre, &sr_raw_list, p_blk->P_Tsrr));
	if(p_blk->P_SContainer) {
		const int adopt_sltp = BIN(p_blk->R_TssModel.E.AdoptSlShiftDn || p_blk->R_TssModel.E.AdoptSlShiftUp);
		for(uint srridx = 0; srridx < sr_raw_list.getCount(); srridx++) {
			PPObjTimeSeries::StrategyResultEntry & r_sr_raw = sr_raw_list.at(srridx);
			PPObjTimeSeries::StrategyResultEntry sre_test(r_sr_raw, 1/*stake_mode*/);
			STRNSCPY(sre_test.Symb, p_blk->R_TsPack.GetSymb());
			THROW(p_blk->TestStrategy(*p_trend_entry, p_main_trend_entry, r_sr_raw, tstso, sre_test, 0, 0 /*so_list*/));
			if(adopt_sltp) {
				const uint   maxduck_lo = sre_test.MaxDuckQuant - p_blk->R_TssModel.E.AdoptSlShiftDn;
				const uint   maxduck_up = sre_test.MaxDuckQuant + p_blk->R_TssModel.E.AdoptSlShiftUp;
				const double tsmd_rel = fdivui(sre_test.TargetQuant, sre_test.MaxDuckQuant);
				uint  best_md = 0;
				uint  best_t = 0;
				PPObjTimeSeries::StrategyResultEntry sre_best2;
				for(uint md = maxduck_lo; md <= maxduck_up; md++) {
					const uint t = R0i(static_cast<double>(md) * tsmd_rel);
					PPObjTimeSeries::StrategyResultEntry sre_sim(r_sr_raw, 1/*stake_mode*/); // Результат симуляции стратегии методом TsSimulateStrategyContainer
					STRNSCPY(sre_sim.Symb, p_blk->R_TsPack.GetSymb());
					PPObjTimeSeries::StrategyResultEntry sre_test2 = sre_test;
					sre_test2.MaxDuckQuant = md;
					sre_test2.TargetQuant = t;
					temp_sc.clear(); // !
					temp_sc.insert(&sre_test2);
					sr_detail_list.clear();
					temp_sc.Simulate(p_blk->R_TsTmList, p_blk->R_TsValList, p_blk->R_TrendList, p_blk->R_Cfg, sre_sim, &sr_detail_list);
					if(best_md == 0) {
						assert(best_t == 0);
						sre_test2.V = sre_sim.V;
						sre_test2.StakeCount = sre_sim.StakeCount;
						sre_test2.WinCount = sre_sim.WinCount;
						sre_best2 = sre_test2;
						best_md = md;
						best_t = t;
					}
					else {
						//if(sre_sim.StakeCount > sre_best2.StakeCount && sre_sim.GetWinCountRate() >= sre_best2.GetWinCountRate()) {
						if(sre_sim.GetWinCountRate() >= sre_best2.GetWinCountRate()) {
							sre_test2.V = sre_sim.V;
							sre_test2.StakeCount = sre_sim.StakeCount;
							sre_test2.WinCount = sre_sim.WinCount;
							sre_best2 = sre_test2;
							best_md = md;
							best_t = t;
						}
					}
				}
				sre_test = sre_best2;
			}
			else {
				PPObjTimeSeries::StrategyResultEntry sre_sim(r_sr_raw, 1/*stake_mode*/); // Результат симуляции стратегии методом TsSimulateStrategyContainer
				STRNSCPY(sre_sim.Symb, p_blk->R_TsPack.GetSymb());
				temp_sc.clear(); // !
				temp_sc.insert(&sre_test);
				sr_detail_list.clear();
				temp_sc.Simulate(p_blk->R_TsTmList, p_blk->R_TsValList, p_blk->R_TrendList, p_blk->R_Cfg, sre_sim, &sr_detail_list);
				sre_test.V = sre_sim.V;
				sre_test.StakeCount = sre_sim.StakeCount;
				sre_test.WinCount = sre_sim.WinCount;
				// @20200528 sre_test.StakeDistMedian = CalcStakeDistanceMedian(sr_detail_list, 0); // @v10.7.9
				//sre_test.StakeDistMedian = CalcFailDistribution(sr_detail_list, 0, p_blk->R_TsTmList, 100) * 1000.0; // @20200528 
			}
			sre_test.ID = GetNewStrategyId();
			/* @v10.7.8 if(p_blk->P_FDump) {
				sre_test.ToString(0, p_blk->StakeSide, ofs, msg_buf);
				p_blk->P_FDump->WriteLine(msg_buf.CR());
			}*/
			{
				CatStrategyPrefix(ofs, msg_buf.Z()).Space().Cat(PPObjTimeSeries::StrategyOutput(sre_test.Symb, &sre_test, temp_buf)).CR();
				CALLPTRMEMB(p_blk->P_FOut, WriteLine(msg_buf));
			}
			if(sre_test.StakeCount > 0 && sre_test.GetWinCountRate() > p_blk->R_TssModel.Rec.MinWinRate) { // @v10.7.3 (sre_test.GetWinCountRate() > p_blk->R_ModelParam.MinWinRate)
				THROW_SL(p_blk->P_SContainer->insert(dynamic_cast <PPObjTimeSeries::Strategy *>(&sre_test)));
			}
			CALLPTRMEMB(p_blk->P_FOut, Flush());
			if(p_blk->R_TssModel.Rec.OptTargetCriterion == PPTssModel::tcAmount && sre_test.V.Result < 0.0) {
				break;
			}
			else if(p_blk->R_TssModel.Rec.MinWinRate > 0.0 && sre_test.GetWinCountRate() <= p_blk->R_TssModel.Rec.MinWinRate) {
				//
				// Нет смысла тестировать последующие стратегии в этом наборе поскольку уровень надежности 
				// для каждого следующего кандидата снижается (как правило, но бывают и исключения).
				//
				lt_min_winrate_count++;
				if(lt_min_winrate_count >= max_lt_min_winrate_count)
					break;
			}
		}
	}
	CATCHZOK
	return ok;
}

void  SLAPI PPObjTimeSeries::StrategyResultEntry::ToString(long flags, int stakeSide, int optFactorSide, SString & rBuf)
{
	rBuf.Z();
	if(flags & 1) {
		rBuf.Cat("Symb").Semicol().
			Cat("Dir").Semicol().
			Cat("OFS").Semicol().
			Cat("Frame").Semicol().
			Cat("MainFrame").Semicol().
			Cat("Duck").Semicol().
			Cat("Target").Semicol().
			Cat("RPD").Semicol().
			Cat("WinRt").Semicol().
			Cat("StkCnt").Semicol().
			Cat("AvgTm").Semicol().
			Cat("RngLo").Semicol().
			Cat("RngUp").Semicol().
			Cat("TrendErrLim").Semicol().
			Cat("TrendErrAvg").Semicol().
			Cat("MainTrendErrLim").Semicol().
			Cat("MainTrendErrAvg");
	}
	else {
		// "Symb" "Dir" "OFS" "Frame" "MainFrame" "Duck" "Target" "RPD" "WinRt" "StkCnt" "AvgTm" "RngLo" "RngUp"
		rBuf.Cat(Symb).Semicol();
			rBuf.Cat((stakeSide == 1) ? "sale" : "buy").Semicol();
			CatStrategyPrefix(optFactorSide, rBuf).Semicol();
			rBuf.Cat(InputFrameSize).Semicol();
			rBuf.Cat(MainFrameSize).Semicol();
			rBuf.Cat(MaxDuckQuant).Semicol();
			rBuf.Cat(TargetQuant).Semicol();
			rBuf.Cat(V.GetResultPerDay(), MKSFMTD(9, 6, 0)).Semicol();
			rBuf.Cat(GetWinCountRate(), MKSFMTD(15, 5, 0)).Semicol();
			rBuf.Cat(StakeCount).Semicol();
			rBuf.Cat(V.TmSec).Semicol();
			rBuf.Cat(OptDeltaRange.low, MKSFMTD(0, 12, 0)).Semicol();
			rBuf.Cat(OptDeltaRange.upp, MKSFMTD(0, 12, 0)).Semicol();
			if(MainFrameSize) {
				rBuf.Cat(OptDelta2Range.low, MKSFMTD(0, 12, 0)).Semicol();
				rBuf.Cat(OptDelta2Range.upp, MKSFMTD(0, 12, 0)).Semicol();
			}
			rBuf.Cat(TrendErrLim, MKSFMTD(0,  3, 0)).Semicol();
			rBuf.Cat(TrendErrAvg, MKSFMTD(0, 12, 0)).Semicol();
			rBuf.Cat(MainTrendErrLim, MKSFMTD(0,  3, 0)).Semicol();
			rBuf.Cat(MainTrendErrAvg, MKSFMTD(0, 12, 0)).Semicol();
	}
}

// @construction
int SLAPI PrcssrTsStrategyAnalyze::FindResonanceCombination(void * pBlk, const LongArray & rFrameSizeList, const LAssocArray & rStakeBoundList, 
	uint mainFrameRangeIdx, const TsMainFrameRange * pMainFrameRange, int stakeSide, ResonanceCombination & rResult)
{
	int    ok = -1;
	TsFindStrategiesLoopBlock * p_blk = static_cast<TsFindStrategiesLoopBlock *>(pBlk);
	//TSCollection <PPObjTimeSeries::TrendEntry> trend_list;
	const PPTssModelPacket & r_tss_model = p_blk->R_TssModel;
	//THROW(MakeArVectors(p_blk->R_Ts, rFrameSizeList, 0, 
		//((r_tss_model.Rec.Flags & PPTssModel::fTrendErrLimitAsMedianPart) ? r_tss_model.Rec.InitTrendErrLimit_ : 0.0), trend_list));
	const bool is_short = (stakeSide == 1);
	const int opt_factor_side = (r_tss_model.Rec.Flags & PPTssModel::fBestSubsetTrendFollowing) ? (is_short ? 1 : 0) : -1;
	const PPObjTimeSeries::TrendEntry * p_main_trend_entry = pMainFrameRange ? PPObjTimeSeries::SearchTrendEntry(p_blk->R_TrendList, pMainFrameRange->MainFrameSize) : 0;
	int tstso = (opt_factor_side == 0) ? TsFindStrategiesBlock::tstsofroPositive : ((opt_factor_side == 1) ? (TsFindStrategiesBlock::tstsofroNegative) : 0);
	tstso |= TsFindStrategiesBlock::tstsofroEvaluateResonanceMeasure;
	// @v10.8.0 tstso |= TsFindStrategiesBlock::tstsofroMode1;
	/*if(!p_blk->P_SContainer)
		tstso |= TsFindStrategiesBlock::tstsofroTotalOnly;*/
	TSCollection <TestStrategyRawResult> local_tsrr_list;
	SString out_buf;
	if(p_blk->P_FOut) {
		out_buf.Z().Cat("FindResonanceCombination").Space().Cat(is_short ? "SHORT" : "LONG").CatDiv('{', 1).CR();
		p_blk->P_FOut->WriteLine(out_buf);
	}
	for(uint ifsidx = 0; ifsidx < rFrameSizeList.getCount(); ifsidx++) {
		const uint input_frame_size = static_cast<uint>(rFrameSizeList.get(ifsidx));
		const PPObjTimeSeries::TrendEntry * p_trend_entry = PPObjTimeSeries::SearchTrendEntry(p_blk->R_TrendList, input_frame_size);
		assert(p_trend_entry);
		assert(p_trend_entry->Stride == input_frame_size); // @paranoic
		for(uint bpidx = 0; bpidx < rStakeBoundList.getCount(); bpidx++) {
			const long local_target_quant = rStakeBoundList.at(bpidx).Key;
			const long local_maxduck_quant = rStakeBoundList.at(bpidx).Val;
			PPObjTimeSeries::TrainNnParam tnnp2(p_blk->R_TsPack, PPObjTimeSeries::TrainNnParam::afAnalyzeFrame);
			tnnp2.SpikeQuant_s = p_blk->R_TsPack.Rec.SpikeQuant_t;
			tnnp2.InputFrameSize = input_frame_size;
			tnnp2.MaxDuckQuant = static_cast<uint16>(local_maxduck_quant);
			assert(oneof2(P.CloseMode, PPObjTimeSeries::TrainNnParam::clsmodFullMaxDuck, PPObjTimeSeries::TrainNnParam::clsmodAdjustLoss));
			tnnp2.StakeCloseMode = static_cast<uint16>(P.CloseMode);
			SETFLAG(tnnp2.BaseFlags, PPObjTimeSeries::TrainNnParam::bfShort, stakeSide == 1);
			if(pMainFrameRange) {
				tnnp2.MainFrameSize = pMainFrameRange->MainFrameSize;
				tnnp2.OptDelta2Range.low = pMainFrameRange->R.low;
				tnnp2.OptDelta2Range.upp = pMainFrameRange->R.upp;
			}
			else {
				tnnp2.MainFrameSize = 0;
				tnnp2.OptDelta2Range.Z();
			}
			tnnp2.TrendErrAvg = p_trend_entry->ErrAvg;
			if(p_trend_entry->ErrLimitByPel > 0.0) 
				tnnp2.TrendErrLim = p_trend_entry->ErrLimitByPel;
			else if(!(r_tss_model.Rec.Flags & PPTssModel::fTrendErrLimitAsMedianPart))
				tnnp2.TrendErrLim = r_tss_model.Rec.InitTrendErrLimit_;
			tnnp2.MainTrendErrAvg = p_main_trend_entry ? p_main_trend_entry->ErrAvg : 0.0;
			tnnp2.MainTrendErrLim = r_tss_model.Rec.InitMainTrendErrLimit;
			tnnp2.TargetQuant = static_cast<uint16>(local_target_quant);
			TsFindStrategiesBlock fsblk(p_blk->R_TssModel, p_blk->R_Cfg, p_blk->R_TsPack, p_blk->R_TsTmList, p_blk->R_TsValList, p_blk->R_TrendList, tnnp2, opt_factor_side, stakeSide, 
				0/*PPObjTimeSeries::StrategyContainer* */, 0/*TestStrategyRawResult* */);
			const PPObjTimeSeries::TrendEntry * p_trend_entry = PPObjTimeSeries::SearchTrendEntry(fsblk.R_TrendList, fsblk.R_Tnnp2.InputFrameSize);
			const PPObjTimeSeries::TrendEntry * p_main_trend_entry = fsblk.R_Tnnp2.MainFrameSize ? PPObjTimeSeries::SearchTrendEntry(fsblk.R_TrendList, fsblk.R_Tnnp2.MainFrameSize) : 0;
			TSVector <PPObjTimeSeries::StrategyResultEntry> sr_raw_list;
			TestStrategyRawResult * p_tsrr = local_tsrr_list.CreateNewItem();
			p_tsrr->MainFrameRangeIdx = mainFrameRangeIdx+1;
			p_tsrr->BaseFlags = tnnp2.BaseFlags;
			p_tsrr->InputFrameSize = tnnp2.InputFrameSize;
			p_tsrr->MainFrameSize = tnnp2.MainFrameSize;
			p_tsrr->TargetQuant = tnnp2.TargetQuant;
			p_tsrr->MaxDuckQuant = tnnp2.MaxDuckQuant;
			p_tsrr->StakeMode = tnnp2.StakeMode;
			p_tsrr->DoCalcResonanceMeasure = true; // !
			PPObjTimeSeries::StrategyResultEntry sre(tnnp2, 0);
			THROW(fsblk.TestStrategy(*p_trend_entry, p_main_trend_entry, fsblk.R_Tnnp2, tstso, sre, &sr_raw_list, p_tsrr));
			if(p_blk->P_FOut) {
				out_buf.Z().Tab().Cat(p_tsrr->InputFrameSize).Space().Cat(p_tsrr->TargetQuant).CatChar('/').Cat(p_tsrr->MaxDuckQuant).
					Space().CatEq("ResonanceMeasure", p_tsrr->ResonanceMeasure).CR();
				p_blk->P_FOut->WriteLine(out_buf);
			}
		}
	}
	{
		double best_result = 0.0;
		uint   best_idx = 0;
		for(uint i = 0; i < local_tsrr_list.getCount(); i++) {
			const TestStrategyRawResult * p_tsrr = local_tsrr_list.at(i);
			if(p_tsrr) {
				if(p_tsrr->ResonanceMeasure > best_result) {
					best_result = p_tsrr->ResonanceMeasure;
					best_idx = i;
				}
			}
		}
		if(best_result > 0.0) {
			const TestStrategyRawResult * p_tsrr = local_tsrr_list.at(best_idx);
			rResult.FrameSize = p_tsrr->InputFrameSize;
			rResult.MaxDuckQuant = p_tsrr->MaxDuckQuant;
			rResult.TargetQuant = p_tsrr->TargetQuant;
			if(p_blk->P_FOut) {
				out_buf.Z().Tab().Cat("The best").CatDiv(':', 2).Cat(p_tsrr->InputFrameSize).Space().Cat(p_tsrr->TargetQuant).CatChar('/').Cat(p_tsrr->MaxDuckQuant).
					Space().CatEq("ResonanceMeasure", p_tsrr->ResonanceMeasure).CR();
				p_blk->P_FOut->WriteLine(out_buf);
			}
			ok = 1;
		}
		if(p_blk->P_FOut) {
			p_blk->P_FOut->WriteLine(out_buf.Z().CatDiv('}', 1).Cat("FindResonanceCombination").CR());
			p_blk->P_FOut->Flush();
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::FindStrategiesLoop(void * pBlk)
{
	int    ok = 1;
	int    do_find_resonance = 1; // @construction
	TsFindStrategiesLoopBlock * p_blk = static_cast<TsFindStrategiesLoopBlock *>(pBlk);
	TSVector <TsMainFrameRange> main_frame_range_list;
	SString temp_buf;
	SString msg_buf;
	uint   max_chunk_count = 0; // @v10.7.3 Максимальный количество допустимых стратегий в одной итерации
	const PPTssModelPacket & r_tss_model = p_blk->R_TssModel;
	const bool use_main_frame = LOGIC(r_tss_model.MainFrameSizeList.getCount() && r_tss_model.Rec.MainFrameRangeCount);
	const int  force_fixed_maxduck_values = 1;
	LAssocArray stake_bound_list = r_tss_model.StakeBoundList;
	if(stake_bound_list.getCount() == 0)
		stake_bound_list.Add(40, 60);
	RealArray temp_real_list;
	if(P.Flags & (P.fFindStrategies|P.fAnalyzeModels)) { // @v10.7.7 fAnalyzeModels
		p_blk->R_TrendList.freeAll();
		LongArray frame_size_list = r_tss_model.InputFrameSizeList;
		if(use_main_frame) {
			for(uint mfsidx = 0; mfsidx < r_tss_model.MainFrameSizeList.getCount(); mfsidx++)
				frame_size_list.addUnique(r_tss_model.MainFrameSizeList.get(mfsidx));
		}
		{
			THROW(MakeArVectors(p_blk->R_Ts, frame_size_list, 0, 
				((r_tss_model.Rec.Flags & PPTssModel::fTrendErrLimitAsMedianPart) ? r_tss_model.Rec.InitTrendErrLimit_ : 0.0), p_blk->R_TrendList));
			if(p_blk->P_FOut) {
				for(uint tlsidx = 0; tlsidx < p_blk->R_TrendList.getCount(); tlsidx++) {
					const PPObjTimeSeries::TrendEntry * p_trend_entry = p_blk->R_TrendList.at(tlsidx);
					if(p_trend_entry) {
						p_trend_entry->Analyze(r_tss_model, msg_buf);
						p_blk->P_FOut->WriteLine(msg_buf.CR());
					}
				}
				p_blk->P_FOut->Flush();
			}
			if(use_main_frame) {
				for(uint tlsidx = 0; tlsidx < p_blk->R_TrendList.getCount(); tlsidx++) {
					const PPObjTimeSeries::TrendEntry * p_trend_entry = p_blk->R_TrendList.at(tlsidx);
					const uint input_frame_size = p_trend_entry->Stride;
					if(r_tss_model.MainFrameSizeList.lsearch(static_cast<long>(input_frame_size))) {
						temp_real_list = p_trend_entry->TL;
						temp_real_list.Sort();
						uint trlidx = temp_real_list.getCount();
						if(trlidx) do {
							if(temp_real_list.at(--trlidx) == 0.0)
								temp_real_list.atFree(trlidx);
						} while(trlidx);
						uint part_count = temp_real_list.getCount() / r_tss_model.Rec.MainFrameRangeCount;
						for(uint partidx = 0; partidx < r_tss_model.Rec.MainFrameRangeCount; partidx++) {
							TsMainFrameRange main_frame_range;
							main_frame_range.MainFrameSize = input_frame_size;
							main_frame_range.R.low = temp_real_list.at(partidx * part_count);
							const uint upp_idx = ((partidx == (r_tss_model.Rec.MainFrameRangeCount-1)) ? temp_real_list.getCount() : ((partidx + 1) * part_count)) - 1;
							main_frame_range.R.upp = temp_real_list.at(upp_idx);
							main_frame_range_list.insert(&main_frame_range);
						}
					}
				}
			}
			if(main_frame_range_list.getCount() == 0) {
				RealRange fake_main_frame_range;
				main_frame_range_list.insert(&fake_main_frame_range.Z());
			}
		}
	}
	{
		class FindStrategiesTask : public SlThread_WithStartupSignal { // @v10.7.3 (SlThread-->SlThread_WithStartupSignal)
		public:
			FindStrategiesTask(const PrcssrTsStrategyAnalyze * pSelf, TsFindStrategiesBlock & rBlk) : SlThread_WithStartupSignal(0), P_Self(pSelf), R_B(rBlk)
			{
			}
			virtual void Run()
			{
				assert(SLS.GetConstTLA().Id == GetThreadID());
				P_Self->FindStrategies(&R_B);
			}
		private:
			TsFindStrategiesBlock & R_B;
			const PrcssrTsStrategyAnalyze * P_Self;
		};
		//
		const size_t thread_limit = 32;
		const  uint max_threads = (P.Flags & P.fAnalyzeModels) ? 2 : 16; // @20200318 8-->16
		TSCollection <PPObjTimeSeries::StrategyContainer> sc_list;
		TSCollection <TestStrategyRawResult> local_tsrr_list; // @v10.7.7
		TSCollection <PPObjTimeSeries::TrainNnParam> tnnp_list;
		TSCollection <TsFindStrategiesBlock> thread_blk_list;
		SETIFZ(p_blk->P_TsrrList, &local_tsrr_list);
		HANDLE objs_to_wait[thread_limit];
		size_t objs_to_wait_count = 0;
		//
		memzero(objs_to_wait, sizeof(objs_to_wait));
		//
		// stake_side: 0 - long, 1 - short
		//
		for(int stake_side = 0; stake_side < 2; stake_side++) {
			if((stake_side == 0 && P.Flags & P.fProcessLong) || (stake_side == 1 && P.Flags & P.fProcessShort)) {
				for(uint mfrlidx = 0; mfrlidx < main_frame_range_list.getCount(); mfrlidx++) {
					const TsMainFrameRange * p_main_frame_range = use_main_frame ? &main_frame_range_list.at(mfrlidx) : 0;
					const PPObjTimeSeries::TrendEntry * p_main_trend_entry = use_main_frame ? PPObjTimeSeries::SearchTrendEntry(p_blk->R_TrendList, p_main_frame_range->MainFrameSize) : 0;
					const bool is_short = (stake_side == 1);
					const uint32 org_opt_max_duck_val = is_short ? p_blk->R_TsPack.Rec.OptMaxDuck_S : p_blk->R_TsPack.Rec.OptMaxDuck;
					// opt_factor_side:
					// 0 - оптимизировать только в положительной области
					// 1 - оптимизировать только в отрицательной области
					// -1 - оптимизировать по всему множеству значений
					const int opt_factor_side = (r_tss_model.Rec.Flags & PPTssModel::fBestSubsetTrendFollowing) ? (is_short ? 1 : 0) : -1;
					uint  cur_opt_max_duck_val = org_opt_max_duck_val;
					LongArray effective_input_frame_size_list;
					LAssocArray effective_stake_bound_list;
					if(!force_fixed_maxduck_values && (P.Flags & P.fFindOptMaxDuck && (org_opt_max_duck_val <= 0 || (P.Flags & P.fForce)))) {
						const uint fomdflags = (is_short ? fomdfShort : 0) | fomdfStoreResult;
						THROW(FindOptimalMaxDuck(p_blk->R_TsPack, p_blk->R_TsTmList, p_blk->R_TsValList, fomdflags, &cur_opt_max_duck_val));
					}
					if(do_find_resonance) {
						ResonanceCombination resonance_result;
						if(FindResonanceCombination(p_blk, r_tss_model.InputFrameSizeList, stake_bound_list, 
							mfrlidx, p_main_frame_range, stake_side, resonance_result) > 0) {
							effective_input_frame_size_list.add(resonance_result.FrameSize);
							effective_stake_bound_list.Add(resonance_result.TargetQuant, resonance_result.MaxDuckQuant);
						}
					}
					else {
						effective_input_frame_size_list = r_tss_model.InputFrameSizeList;
						effective_stake_bound_list = stake_bound_list;
					}
					if(P.Flags & (P.fFindStrategies|P.fAnalyzeModels)) { // @v10.7.7 P.fAnalyzeModels
						for(uint ifsidx = 0; ifsidx < effective_input_frame_size_list.getCount(); ifsidx++) {
							const uint input_frame_size = static_cast<uint>(effective_input_frame_size_list.get(ifsidx));
							const PPObjTimeSeries::TrendEntry * p_trend_entry = PPObjTimeSeries::SearchTrendEntry(p_blk->R_TrendList, input_frame_size);
							assert(p_trend_entry);
							assert(p_trend_entry->Stride == input_frame_size); // @paranoic
							for(uint bpidx = 0; bpidx < effective_stake_bound_list.getCount(); bpidx++) {
								const long local_target_quant = effective_stake_bound_list.at(bpidx).Key;
								const long local_maxduck_quant = effective_stake_bound_list.at(bpidx).Val;
								if(objs_to_wait_count >= max_threads) {
									::WaitForMultipleObjects(objs_to_wait_count, objs_to_wait, TRUE, INFINITE);
									{
										for(uint thrbidx = 0; thrbidx < thread_blk_list.getCount(); thrbidx++) {
											const TsFindStrategiesBlock * p_fsblk = thread_blk_list.at(thrbidx);
											if(p_fsblk && p_fsblk->P_SContainer) {
												const PPObjTimeSeries::StrategyContainer & r_sc = *p_fsblk->P_SContainer;
												for(uint sidx = 0; sidx < r_sc.getCount(); sidx++) {
													const PPObjTimeSeries::Strategy & r_s = r_sc.at(sidx);
													p_blk->R_SContainer.insert(&r_s);
													if(p_blk->P_FOut) {
														CatStrategyPrefix(opt_factor_side, msg_buf.Z()).Space().
															Cat(PPObjTimeSeries::StrategyOutput(p_blk->R_TsPack.GetSymb(), &r_s, temp_buf)).CR();
														p_blk->P_FOut->WriteLine(msg_buf);
													}
												}
												SETMAX(max_chunk_count, r_sc.getCount());
											}
										}
										CALLPTRMEMB(p_blk->P_FOut, Flush());
									}
									objs_to_wait_count = 0;
									memzero(objs_to_wait, sizeof(objs_to_wait));
									thread_blk_list.freeAll();
								}
								PPObjTimeSeries::TrainNnParam * p_tnnp2 = new PPObjTimeSeries::TrainNnParam(p_blk->R_TsPack, PPObjTimeSeries::TrainNnParam::afAnalyzeFrame);
								p_tnnp2->SpikeQuant_s = p_blk->R_TsPack.Rec.SpikeQuant_t;
								p_tnnp2->InputFrameSize = input_frame_size;
								p_tnnp2->MaxDuckQuant = static_cast<uint16>(local_maxduck_quant);
								assert(oneof2(P.CloseMode, PPObjTimeSeries::TrainNnParam::clsmodFullMaxDuck, PPObjTimeSeries::TrainNnParam::clsmodAdjustLoss));
								p_tnnp2->StakeCloseMode = static_cast<uint16>(P.CloseMode);
								SETFLAG(p_tnnp2->BaseFlags, PPObjTimeSeries::TrainNnParam::bfShort, stake_side == 1);
								if(p_main_frame_range) {
									p_tnnp2->MainFrameSize = p_main_frame_range->MainFrameSize;
									p_tnnp2->OptDelta2Range.low = p_main_frame_range->R.low;
									p_tnnp2->OptDelta2Range.upp = p_main_frame_range->R.upp;
								}
								else {
									p_tnnp2->MainFrameSize = 0;
									p_tnnp2->OptDelta2Range.Z();
								}
								p_tnnp2->TrendErrAvg = p_trend_entry->ErrAvg;
								if(p_trend_entry->ErrLimitByPel > 0.0) 
									p_tnnp2->TrendErrLim = p_trend_entry->ErrLimitByPel;
								else if(!(r_tss_model.Rec.Flags & PPTssModel::fTrendErrLimitAsMedianPart))
									p_tnnp2->TrendErrLim = r_tss_model.Rec.InitTrendErrLimit_;
								p_tnnp2->MainTrendErrAvg = p_main_trend_entry ? p_main_trend_entry->ErrAvg : 0.0;
								p_tnnp2->MainTrendErrLim = r_tss_model.Rec.InitMainTrendErrLimit;
								p_tnnp2->TargetQuant = static_cast<uint16>(local_target_quant);
								tnnp_list.insert(p_tnnp2);
								PPObjTimeSeries::StrategyContainer * p_sc = 0;
								TestStrategyRawResult * p_tsrr = 0;
								if(P.Flags & P.fFindStrategies) {
									p_sc = sc_list.CreateNewItem();
								}
								if(P.Flags & P.fAnalyzeModels) { 
									p_tsrr = p_blk->P_TsrrList->CreateNewItem();
									//
									p_tsrr->MainFrameRangeIdx = mfrlidx+1;
									p_tsrr->BaseFlags = p_tnnp2->BaseFlags;
									p_tsrr->InputFrameSize = p_tnnp2->InputFrameSize;
									p_tsrr->MainFrameSize = p_tnnp2->MainFrameSize;
									p_tsrr->TargetQuant = p_tnnp2->TargetQuant;
									p_tsrr->MaxDuckQuant = p_tnnp2->MaxDuckQuant;
									p_tsrr->StakeMode = p_tnnp2->StakeMode;
								}
								TsFindStrategiesBlock * p_fsblk = new TsFindStrategiesBlock(r_tss_model, p_blk->R_Cfg, p_blk->R_TsPack, p_blk->R_TsTmList, p_blk->R_TsValList, 
									p_blk->R_TrendList, *p_tnnp2, opt_factor_side, stake_side, p_sc, p_tsrr);
								p_fsblk->P_FOut = p_blk->P_FOut; // @v10.8.0 0-->p_blk->P_FOut
								thread_blk_list.insert(p_fsblk);
								{
									FindStrategiesTask * p_thread = new FindStrategiesTask(this, *p_fsblk);
									THROW_S(p_thread, SLERR_NOMEM);
									p_thread->Start(1);
									objs_to_wait[objs_to_wait_count++] = *p_thread;
								}
							}
						}
					}
				}
			}
		}
		if(objs_to_wait_count) {
			::WaitForMultipleObjects(objs_to_wait_count, objs_to_wait, TRUE, INFINITE);
			{
				for(uint thrbidx = 0; thrbidx < thread_blk_list.getCount(); thrbidx++) {
					const TsFindStrategiesBlock * p_fsblk = thread_blk_list.at(thrbidx);
					if(p_fsblk) {
						if(p_fsblk->P_SContainer) {
							const PPObjTimeSeries::StrategyContainer & r_sc = *p_fsblk->P_SContainer;
							for(uint sidx = 0; sidx < r_sc.getCount(); sidx++) {
								const PPObjTimeSeries::Strategy & r_s = r_sc.at(sidx);
								p_blk->R_SContainer.insert(&r_s);
								if(p_blk->P_FOut) {
									const bool is_short = LOGIC(r_s.BaseFlags & r_s.bfShort);
									const int opt_factor_side = (r_tss_model.Rec.Flags & PPTssModel::fBestSubsetTrendFollowing) ? (is_short ? 1 : 0) : -1;
									CatStrategyPrefix(opt_factor_side, msg_buf.Z()).Space().Cat(PPObjTimeSeries::StrategyOutput(p_blk->R_TsPack.GetSymb(), &r_s, temp_buf)).CR();
									p_blk->P_FOut->WriteLine(msg_buf);
								}
							}
							SETMAX(max_chunk_count, r_sc.getCount());
						}
					}
				}
			}
			objs_to_wait_count = 0;
			memzero(objs_to_wait, sizeof(objs_to_wait));
			thread_blk_list.freeAll();
		}
		// @v10.7.3 {
		if(p_blk->P_FOut) {
			if(max_chunk_count) {
				msg_buf.Z().CatCharN('-', 8).CR().CatEq("max-chunk-count", max_chunk_count).CR();
				p_blk->P_FOut->WriteLine(msg_buf);
			}
			if(SVectorBase::GetCount(p_blk->P_TsrrList)) {
				//msg_buf.Z().Cat("test-strategy-raw-result");
				//p_blk->P_FOut->WriteLine(msg_buf.CR());
				for(uint i = 0; i < p_blk->P_TsrrList->getCount(); i++) {
					TestStrategyRawResult * p_tsrr = p_blk->P_TsrrList->at(i);
					if(p_tsrr) {
						{
							SString opt_entry_stat_list_fn;
							opt_entry_stat_list_fn.Cat("optentrystatlist").CatChar('-').Cat(p_blk->R_TsPack.Rec.Symb).CatChar('-');
							if(p_tsrr->MainFrameSize)
								opt_entry_stat_list_fn.Cat(p_tsrr->MainFrameSize).CatChar('_').Cat(p_tsrr->MainFrameRangeIdx).CatChar('-');
							opt_entry_stat_list_fn.Cat(p_tsrr->InputFrameSize).CatChar('-').Cat(p_tsrr->MaxDuckQuant).CatChar('-').Cat(p_tsrr->TargetQuant).CatChar('-').
								Cat(p_tsrr->GetStakeDirText());
							opt_entry_stat_list_fn.ToLower().Dot().Cat("txt");
							PPGetFilePath(PPPATH_OUT, opt_entry_stat_list_fn, temp_buf);
							SFile f_ol(temp_buf, SFile::mWrite);
							if(f_ol.IsValid()) {
								msg_buf.Z().CatCharN(' ', 4);
									msg_buf.Cat(p_tsrr->GetStakeDirText()).Space().
									//Cat(p_tsrr->StakeMode).Space().
									Cat(p_tsrr->MainFrameSize, MKSFMTD(4, 0, ALIGN_RIGHT)).Space().
									Cat(p_tsrr->InputFrameSize, MKSFMTD(4, 0, ALIGN_RIGHT)).Space().
									Cat(p_tsrr->MaxDuckQuant, MKSFMTD(3, 0, ALIGN_RIGHT)).CatChar(':').Cat(p_tsrr->TargetQuant, MKSFMTD(3, 0, ALIGN_LEFT)).CatDiv(':', 2);
								msg_buf.Cat(p_tsrr->FactorR.low, MKSFMTD(0, 12, 0)).Dot().Dot().Cat(p_tsrr->FactorR.upp, MKSFMTD(0, 12, 0)).Space();
								msg_buf.Cat(p_tsrr->SoList.getCount(), MKSFMTD(7, 0, ALIGN_RIGHT)).Space().
									Cat(p_tsrr->TotalResult, MKSFMTD(15, 6, ALIGN_RIGHT)).Space().
									Cat(p_tsrr->TotalProb, MKSFMTD(8, 4, ALIGN_RIGHT)).Space();
								msg_buf.Cat(p_tsrr->BestStep, MKSFMTD(3, 0, ALIGN_RIGHT)).CatChar(':').
									Cat(p_tsrr->BestStepResult, MKSFMTD(15, 6, ALIGN_RIGHT)).CatChar(':').
									Cat(p_tsrr->BestStepProb, MKSFMTD(8, 4, ALIGN_RIGHT)).CatChar(':').
									Cat(p_tsrr->BestStepBarCount, MKSFMTD(3, 0, ALIGN_RIGHT));
								f_ol.WriteLine(msg_buf.CR());
								{
									msg_buf.Z().CatCharN('-', 8).Cat("Step-Angle-Histogram");
									f_ol.WriteLine(msg_buf.CR());
									msg_buf.Z().Cat("StepDot").Tab().Cat("StepAngle").Tab().Cat("Mean").Tab().Cat("StdDev").Tab().Cat("VarCoeff").Tab().
										Cat("SuccCount").Tab().Cat("Result").Tab().Cat("ResultQ").Tab().Cat("Prob").Tab().Cat("AvgResult");
									f_ol.WriteLine(msg_buf.CR());
									for(uint si = 0; si < p_tsrr->SahList.getCount(); si++) {
										const TestStrategyRawResult::StepHistogramItem & r_hi = p_tsrr->SahList.at(si);
										msg_buf.Z().Cat(r_hi.StepDot).Tab().Cat(r_hi.StepAngle, MKSFMTD(0, 12, NMBF_NOTRAILZ)).Tab().
											Cat(r_hi.Mean, MKSFMTD(0, 3, 0)).Tab().
											Cat(r_hi.StdDev, MKSFMTD(0, 3, 0)).Tab().
											Cat(r_hi.StdDev / r_hi.Mean, MKSFMTD(0, 3, 0)).Tab().
											Cat(r_hi.SuccCount).Tab().
											Cat(r_hi.Result, MKSFMTD(0, 1, 0)).Tab().
											Cat(r_hi.ResultQ, MKSFMTD(0, 1, 0)).Tab().
											Cat(r_hi.Prob, MKSFMTD(0, 3, 0)).Tab().
											Cat(r_hi.Result / r_hi.SuccCount, MKSFMTD(0, 3, 0));
										f_ol.WriteLine(msg_buf.CR());
									}
								}
								{
									msg_buf.Z().CatCharN('-', 8).Cat("Step-Dot-Histogram");
									f_ol.WriteLine(msg_buf.CR());
									for(uint si = 0; si < p_tsrr->SdhList.getCount(); si++) {
										const TestStrategyRawResult::StepHistogramItem & r_hi = p_tsrr->SdhList.at(si);
										if(r_hi.SuccCount > 0) {
											msg_buf.Z().Cat(r_hi.StepDot).Tab().Cat(r_hi.StepAngle, MKSFMTD(0, 12, NMBF_NOTRAILZ)).Tab().
												Cat(r_hi.Mean * 1E6, MKSFMTD(0, 5, 0)).Tab().
												Cat(r_hi.StdDev * 1E6, MKSFMTD(0, 5, 0)).Tab().
												Cat(r_hi.StdDev / r_hi.Mean, MKSFMTD(0, 5, 0)).Tab().
												Cat(r_hi.SuccCount).Tab().
												Cat(r_hi.Result, MKSFMTD(0, 1, 0)).Tab().
												Cat(r_hi.ResultQ, MKSFMTD(0, 1, 0)).Tab().
												Cat(r_hi.Prob, MKSFMTD(0, 3, 0)).Tab().
												Cat(r_hi.Result / r_hi.SuccCount, MKSFMTD(0, 3, 0));
											f_ol.WriteLine(msg_buf.CR());
										}
									}
								}
							}
						}
						if(p_tsrr->SoList.getCount()) {
							SString opt_entry_list_fn;
							opt_entry_list_fn.Cat("optentrylist").CatChar('-').Cat(p_blk->R_TsPack.Rec.Symb).CatChar('-');
							if(p_tsrr->MainFrameSize)
								opt_entry_list_fn.Cat(p_tsrr->MainFrameSize).CatChar('_').Cat(p_tsrr->MainFrameRangeIdx).CatChar('-');
							opt_entry_list_fn.Cat(p_tsrr->InputFrameSize).CatChar('-').Cat(p_tsrr->MaxDuckQuant).CatChar('-').Cat(p_tsrr->TargetQuant).CatChar('-').
								Cat(p_tsrr->GetStakeDirText());
							opt_entry_list_fn.ToLower().Dot().Cat("csv");
							PPGetFilePath(PPPATH_OUT, opt_entry_list_fn, temp_buf);
							SFile f_ol(temp_buf, SFile::mWrite);
							if(f_ol.IsValid()) {
								msg_buf.Z().Cat("factor").Tab().Cat("result1").Tab().Cat("result2");
								f_ol.WriteLine(msg_buf.CR());
								for(uint j = 0; j < p_tsrr->SoList.getCount(); j++) {
									const StrategyOptEntry & r_entry = p_tsrr->SoList.at(j);
									msg_buf.Z().Cat(r_entry.Factor, MKSFMTD(0, 10, 0)).Tab().Cat(r_entry.Result1, MKSFMTD(0, 10, 0)).Tab().Cat(r_entry.Result2, MKSFMTD(0, 10, 0));
									f_ol.WriteLine(msg_buf.CR());
								}
							}
							p_tsrr->SoList.freeAll();
						}
					}
				}
			}
			/*
			PPObjTimeSeries::StrategyContainer scollection_full(p_blk->R_SContainer);
			if(scollection_full.getCount()) {
				msg_buf.Z().CatCharN('-', 8).Space().Cat("strategies-ordered-by-cqa").Space().CatEq("count", scollection_full.getCount()).CR();
				p_blk->P_FOut->WriteLine(msg_buf);
				TSArray <PPObjTimeSeries::StrategyContainer::CritEntry> sc_order_index;
				scollection_full.MakeOrderIndex(PPObjTimeSeries::StrategyContainer::gbsfOptDeltaRangeCQA, sc_order_index);
				for(uint i = 0; i < sc_order_index.getCount(); i++) {
					const PPObjTimeSeries::StrategyContainer::CritEntry & r_ie = sc_order_index.at(i);
					assert(r_ie.Idx > 0 && r_ie.Idx <= scollection_full.getCount());
					if(r_ie.Idx > 0 && r_ie.Idx <= scollection_full.getCount()) {
						const PPObjTimeSeries::Strategy & r_s = scollection_full.at(r_ie.Idx-1);
						msg_buf.Z().Cat(PPObjTimeSeries::StrategyOutput(p_blk->R_TsPack.GetSymb(), &r_s, temp_buf)).CR();
						p_blk->P_FOut->WriteLine(msg_buf);
					}
				}
			}
			*/
			p_blk->P_FOut->Flush(); // @v10.7.6
		}
		// } @v10.7.3 
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::GetTssModel(const PPTimeSeries * pTsRec, PPTssModelPacket * pTssModel)
{
	int    ok = -1;
	PPID   tss_model_id = pTsRec ? pTsRec->TssModelID : 0;
	PPObjTssModel tss_model_obj;
	if(!tss_model_id) {
		PPID   temp_id = 0;
		if(tss_model_obj.SearchBySymb("default", &temp_id, 0) > 0) {
			tss_model_id = temp_id;
			ok = 2;
		}
	}
	if(tss_model_id) {
		if(tss_model_obj.GetPacket(tss_model_id, pTssModel) > 0) {
			if(ok < 0)
				ok = 1;
		}
		else
			ok = -1;
	}
	return ok;
}

uint SLAPI PrcssrTsStrategyAnalyze::CalcStakeCountAtFinalSimulation(const TSVector <PPObjTimeSeries::StrategyResultValueEx> & rSreEx, uint scIdx) const
{
	uint   result = 0;
	SForEachVectorItem(rSreEx, reidx) { if(rSreEx.at(reidx).StrategyIdx == scIdx) result++; }
	return result;
}

double SLAPI PrcssrTsStrategyAnalyze::CalcStakeResult(const TSVector <PPObjTimeSeries::StrategyResultValueEx> & rSreEx, uint scIdx) const
{
	double result = 0.0;
	SForEachVectorItem(rSreEx, reidx) { if(rSreEx.at(reidx).StrategyIdx == scIdx) result += rSreEx.at(reidx).Result; }
	return result;
}

double SLAPI PrcssrTsStrategyAnalyze::CalcFailDistribution(const TSVector <PPObjTimeSeries::StrategyResultValueEx> & rSreEx, uint scIdx, 
	const DateTimeArray & rTsTmList, uint partitionCount) const
{
	double result = 0.0;
	uint   pc = (partitionCount > 0) ? partitionCount : 100;
	const uint rc = rTsTmList.getCount();
	LAssocArray histogram;
	SForEachVectorItem(rSreEx, reidx) { 
		const PPObjTimeSeries::StrategyResultValueEx & r_item = rSreEx.at(reidx);
		if(r_item.StrategyIdx == scIdx && r_item.Result > 0.0) {
			long key = (r_item.StartPoint / (rc / pc)) + 1;
			histogram.IncrementValueByKey(key);
			//start_pt_list.add(static_cast<long>(r_item.StartPoint));
		}
	}
	{
		StatBase sb;
		SForEachVectorItem(histogram, i) { sb.Step(static_cast<double>(histogram.at(i).Val)); }
		sb.Finish();
		result = sb.GetStdDev();
	}
	return result;
}

uint SLAPI PrcssrTsStrategyAnalyze::CalcStakeDistanceMedian(const TSVector <PPObjTimeSeries::StrategyResultValueEx> & rSreEx, uint scIdx) const
{
	uint   result = 0;
	if(rSreEx.getCount()) {
		LongArray start_pt_list;
		LongArray distance_list;
		SForEachVectorItem(rSreEx, reidx) { 
			const PPObjTimeSeries::StrategyResultValueEx & r_item = rSreEx.at(reidx);
			if(r_item.StrategyIdx == scIdx && r_item.Result > 0.0)
				start_pt_list.add(static_cast<long>(r_item.StartPoint));
		}
		start_pt_list.sort();
		long prev_pt = 0;
		StatBase sb;
		for(uint i = 0; i < start_pt_list.getCount(); i++) {
			const long pt = start_pt_list.get(i);
			long distance = (pt - prev_pt);
			distance_list.add(distance);
			sb.Step(static_cast<double>(distance));
			prev_pt = pt;
		}
		sb.Finish();
		if(distance_list.getCount()) {
			distance_list.sort();
			result = distance_list.get(distance_list.getCount()/2);
			//result = R0i(sb.GetExp());
		}
	}
	return result;
}

int SLAPI PrcssrTsStrategyAnalyze::StrategyContainerDressing(const PPObjTimeSeries::Config & rConfig, 
	const PPTimeSeriesPacket & rTsPack, const PPTssModelPacket & rTssModel, const DateTimeArray & rTsTmList, const RealArray & rTsValList,
	const TSCollection <PPObjTimeSeries::TrendEntry> & rTrendListSet, 
	const PPObjTimeSeries::StrategyContainer & rSrcSc,
	PPObjTimeSeries::StrategyContainer & rScSelection, SFile * pFOut)
{
	int    ok = 1;
	//
	// shuffle_selection_count позволяет реализовать идею итеративного выявления множества наиболее адекватных
	// стратегий путем многократного перемешивания всего множества исходных стратегий и последующего выбора 
	// набора самых живучих из них. Но, увы, похоже это не работает.
	//
	const  uint shuffle_selection_count = 0;
	if(shuffle_selection_count) {
		SString msg_buf;
		SString temp_buf;
		LAssocArray survivor_s_id_list;
		for(uint i = 0; i < shuffle_selection_count; i++) {
			PPObjTimeSeries::StrategyContainer temp_sc = rSrcSc;
			PPObjTimeSeries::StrategyContainer temp_sc_selection;
			PPObjTimeSeries::StrategyResultEntry sre;
			temp_sc.shuffle();
			int scdr = Helper_StrategyContainerDressing(rConfig, rTsPack, rTssModel, rTsTmList, rTsValList, rTrendListSet, temp_sc, temp_sc_selection, &sre, /*pFOut*/0);
			THROW(scdr);
			if(scdr > 0) {
				SForEachVectorItem(temp_sc_selection, j) { survivor_s_id_list.IncrementValueByKey(temp_sc_selection.at(j).ID); }
				survivor_s_id_list.SortByVal();
				if(pFOut) {
					msg_buf.Z().Cat("Shuffle selection result").CatChar('[').Cat(i+1).CatChar('/').Cat(shuffle_selection_count).CatChar(']');
					pFOut->WriteLine(msg_buf.CR());
					SForEachVectorItem(rScSelection, si) { pFOut->WriteLine(PPObjTimeSeries::StrategyToString(temp_sc_selection.at(si), 0, 0, msg_buf.Z()).CR()); }
					OutputStategyResultEntry(sre, msg_buf);
					pFOut->WriteLine(msg_buf.CR());
					msg_buf.Z().Cat("Survivor list").CatChar(':');
					pFOut->WriteLine(msg_buf.CR());
					for(uint k = 0; k < survivor_s_id_list.getCount(); k++) {
						LAssoc s = survivor_s_id_list.at(k);
						const PPObjTimeSeries::Strategy * p_s = rSrcSc.SearchByID(s.Key);
						assert(p_s);
						//assert(temp_sc_selection.SearchByID(s.Key));
						assert(temp_sc.SearchByID(s.Key));
						if(p_s) {
							PPObjTimeSeries::StrategyToString(*p_s, 0, 0, temp_buf);
							msg_buf.Z().Cat(s.Val).Space().Cat(temp_buf);
							pFOut->WriteLine(msg_buf.CR());
						}
					}
					pFOut->Flush();
				}
			}
			else
				break;
		}
		{
			const LDATE date_till = checkdate(P.UseDataForStrategiesTill) ? P.UseDataForStrategiesTill : ZERODATE;
			rScSelection.clear();
			rScSelection.SetupByTsPacket(rTsPack, &rTssModel);
			rScSelection.SetUseDataForStrategiesTill(date_till);
			uint k = survivor_s_id_list.getCount();
			if(k) {
				uint p = static_cast<uint>(R0i(0.85 * static_cast<double>(survivor_s_id_list.getCount())));
				long bound_n = survivor_s_id_list.at(p).Val + 1;
				//long bound_n2 = (shuffle_selection_count/2);
				do {
					LAssoc s = survivor_s_id_list.at(--k);
					if(s.Val >= bound_n) {
						const PPObjTimeSeries::Strategy * p_s = rSrcSc.SearchByID(s.Key);
						assert(p_s);
						rScSelection.insert(p_s);
					}
				} while(k);
			}
		}
	}
	else {
		THROW(Helper_StrategyContainerDressing(rConfig, rTsPack, rTssModel, rTsTmList, rTsValList, rTrendListSet, rSrcSc, rScSelection, 0, pFOut));
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::Helper_StrategyContainerDressing(const PPObjTimeSeries::Config & rConfig, 
	const PPTimeSeriesPacket & rTsPack, const PPTssModelPacket & rTssModel, const DateTimeArray & rTsTmList, const RealArray & rTsValList,
	const TSCollection <PPObjTimeSeries::TrendEntry> & rTrendListSet, 
	const PPObjTimeSeries::StrategyContainer & rSrcSc,
	PPObjTimeSeries::StrategyContainer & rScSelection, PPObjTimeSeries::StrategyResultEntry * pSre, SFile * pFOut)
{
	int    ok = -1;
	SString msg_buf;
	PPObjTimeSeries::StrategyContainer sc_process = rSrcSc;
	const uint8 best_subset_opt_chunk = rTssModel.Rec.BestSubsetOptChunk;
	const LDATE date_till = checkdate(P.UseDataForStrategiesTill) ? P.UseDataForStrategiesTill : ZERODATE;
	RealRange strategy_result_range;
	{
		strategy_result_range.Set(SMathConst::Max, SMathConst::Min);
		SForEachVectorItem(sc_process, scidx) { strategy_result_range.SetupMinMax(sc_process.at(scidx).V.Result); }
	}
	//
	// Если (tss_model.Rec.MinSimilItems > 1) то селекцию стратегий делать нельзя - она вычистит парные стратегии
	//
	if(rTssModel.Rec.MinSimilItems > 1 || !oneof4(best_subset_opt_chunk, 1, 3, 7, 15)) { // @v10.7.7 (tss_model.Rec.MinSimilItems > 1) // @v10.7.8 (1)
		rScSelection = sc_process;
		rScSelection.SetupByTsPacket(rTsPack, &rTssModel); // @v10.7.1
		rScSelection.SetUseDataForStrategiesTill(date_till); // @v10.7.3
	}
	else {
		// @v10.7.7 const uint test_count_on_selection = 1; // @20200413 10-->20
		//
		// Многопоточный подбор оптимальной комбинации стратегий
		//
		assert(oneof4(best_subset_opt_chunk, 1, 3, 7, 15)); // @v10.6.9 (15) // @v10.7.8 (1)
		class StrategySetSimulationTask : public SlThread_WithStartupSignal { // @v10.7.3 (SlThread-->SlThread_WithStartupSignal)
		public:
			struct InitBlock {
				InitBlock(const DateTimeArray & rTmList, const RealArray & rValList,
					const TSCollection <PPObjTimeSeries::TrendEntry> & rTrendList,
					const PPObjTimeSeries::StrategyContainer & rSc,
					const PPObjTimeSeries::Config & rCfg,
					PPObjTimeSeries::StrategyResultEntry * pResult,
					TSVector <PPObjTimeSeries::StrategyResultValueEx> * pResultEx) :
					R_TmList(rTmList), R_ValList(rValList), R_TrendList(rTrendList), R_Sc(rSc), R_Cfg(rCfg), 
					P_Result(pResult), P_ResultEx(pResultEx)
				{
				}
				const DateTimeArray & R_TmList;
				const RealArray & R_ValList;
				const TSCollection <PPObjTimeSeries::TrendEntry> & R_TrendList;
				const PPObjTimeSeries::StrategyContainer & R_Sc;
				const PPObjTimeSeries::Config & R_Cfg;
				PPObjTimeSeries::StrategyResultEntry * P_Result;
				TSVector <PPObjTimeSeries::StrategyResultValueEx> * P_ResultEx;
			};
			StrategySetSimulationTask(InitBlock * pBlk) : SlThread_WithStartupSignal(0), B(*pBlk)
			{
			}
			virtual void Run()
			{
				assert(SLS.GetConstTLA().Id == GetThreadID());
				B.R_Sc.Simulate(B.R_TmList, B.R_ValList, B.R_TrendList, B.R_Cfg, *B.P_Result, B.P_ResultEx);
			}
		private:
			InitBlock B;
		};
		const double overall_win_rate_limit = (rTssModel.Rec.OverallWinRateLimit > 0.0) ? rTssModel.Rec.OverallWinRateLimit : 0.0; // @v10.7.0
		const  uint scp_inc = (best_subset_opt_chunk == 1) ? 1 : ((best_subset_opt_chunk == 3) ? 2 : ((best_subset_opt_chunk == 7) ? 3 : ((best_subset_opt_chunk == 15) ? 4 : 0)));
		double best_result = 0.0;
		double best_result_spcmult = 0.0; // @v10.7.8 Лучший результат взвешенный по специальному множителю
		double best_winrate = 0.0; // @20200517 Лучшая вероятность выигрыша
		uint   best_loss_count = 0; // @v10.7.1
		uint   phony_iter_no = 0;
		rScSelection.clear(); // @20200421
		rScSelection.SetupByTsPacket(rTsPack, &rTssModel); // @v10.7.1
		rScSelection.SetUseDataForStrategiesTill(date_till); // @v10.7.3
		for(uint scpidx = 0; scpidx < sc_process.getCount() && phony_iter_no < rTssModel.Rec.BestSubsetMaxPhonyIters; scpidx += scp_inc) {
			PPObjTimeSeries::StrategyResultEntry sre[15];
			PPObjTimeSeries::StrategyContainer sc[15]; // 15 - max of model_param.BestSubsetOptChunk
			TSVector <PPObjTimeSeries::StrategyResultValueEx> sre_ex[15]; // @v10.7.6
			HANDLE objs_to_wait[32];
			size_t objs_to_wait_count = 0;
			MEMSZERO(objs_to_wait);
			uint thr_idx;
			if(best_subset_opt_chunk == 1) {
				thr_idx = 0;
				sc[thr_idx] = rScSelection;
				sc[thr_idx].insert(&sc_process.at(scpidx)); // 0001
				sc[thr_idx].Simulate(rTsTmList, rTsValList, rTrendListSet, rConfig, sre[thr_idx], &sre_ex[thr_idx]);
			}
			else {
				if(best_subset_opt_chunk == 15) {
					for(thr_idx = 0; thr_idx < best_subset_opt_chunk; thr_idx++) {
						sc[thr_idx] = rScSelection;
						// 0001, 0010, 0011, 0100, 0101, 0110, 0111, 1000, 1001, 1010, 1011, 1100, 1101, 1110, 1111
						if(thr_idx == 0) {
							sc[thr_idx].insert(&sc_process.at(scpidx)); // 0001
						}
						else if(scpidx < (sc_process.getCount()-1)) {
							if(thr_idx == 1) { // 0010
								sc[thr_idx].insert(&sc_process.at(scpidx+1));
							}
							else if(thr_idx == 2) { // 0011
								sc[thr_idx].insert(&sc_process.at(scpidx));
								sc[thr_idx].insert(&sc_process.at(scpidx+1));
							}
							else if(scpidx < (sc_process.getCount()-2)) {
								if(thr_idx == 3) { // 0100
									sc[thr_idx].insert(&sc_process.at(scpidx+2));
								}
								else if(thr_idx == 4) { // 0101
									sc[thr_idx].insert(&sc_process.at(scpidx));
									sc[thr_idx].insert(&sc_process.at(scpidx+2));
								}
								else if(thr_idx == 5) { // 0110
									sc[thr_idx].insert(&sc_process.at(scpidx+1));
									sc[thr_idx].insert(&sc_process.at(scpidx+2));
								}
								else if(thr_idx == 6) { // 0111
									sc[thr_idx].insert(&sc_process.at(scpidx));
									sc[thr_idx].insert(&sc_process.at(scpidx+1));
									sc[thr_idx].insert(&sc_process.at(scpidx+2));
								}
								else if(scpidx < (sc_process.getCount()-3)) {
									if(thr_idx == 7) { // 1000
										sc[thr_idx].insert(&sc_process.at(scpidx+3));
									}
									else if(thr_idx == 8) { // 1001
										sc[thr_idx].insert(&sc_process.at(scpidx));
										sc[thr_idx].insert(&sc_process.at(scpidx+3));
									}
									else if(thr_idx == 9) { // 1010
										sc[thr_idx].insert(&sc_process.at(scpidx+1));
										sc[thr_idx].insert(&sc_process.at(scpidx+3));
									}
									else if(thr_idx == 10) { // 1011
										sc[thr_idx].insert(&sc_process.at(scpidx));
										sc[thr_idx].insert(&sc_process.at(scpidx+1));
										sc[thr_idx].insert(&sc_process.at(scpidx+3));
									}

									else if(thr_idx == 11) { // 1100
										sc[thr_idx].insert(&sc_process.at(scpidx+2));
										sc[thr_idx].insert(&sc_process.at(scpidx+3));
									}
									else if(thr_idx == 12) { // 1101
										sc[thr_idx].insert(&sc_process.at(scpidx));
										sc[thr_idx].insert(&sc_process.at(scpidx+2));
										sc[thr_idx].insert(&sc_process.at(scpidx+3));
									}
									else if(thr_idx == 13) { // 1110
										sc[thr_idx].insert(&sc_process.at(scpidx+1));
										sc[thr_idx].insert(&sc_process.at(scpidx+2));
										sc[thr_idx].insert(&sc_process.at(scpidx+3));
									}
									else if(thr_idx == 14) { // 1111
										sc[thr_idx].insert(&sc_process.at(scpidx));
										sc[thr_idx].insert(&sc_process.at(scpidx+1));
										sc[thr_idx].insert(&sc_process.at(scpidx+2));
										sc[thr_idx].insert(&sc_process.at(scpidx+3));
									}
								}
							}
						}
						StrategySetSimulationTask::InitBlock tb(rTsTmList, rTsValList, rTrendListSet, sc[thr_idx], rConfig, &sre[thr_idx], &sre_ex[thr_idx]);
						StrategySetSimulationTask * p_thread = new StrategySetSimulationTask(&tb);
						THROW_S(p_thread, SLERR_NOMEM);
						p_thread->Start(1/*0*/);
						objs_to_wait[objs_to_wait_count++] = *p_thread;
					}
				}
				else if(best_subset_opt_chunk == 7) {
					for(thr_idx = 0; thr_idx < best_subset_opt_chunk; thr_idx++) {
						sc[thr_idx] = rScSelection;
						// 001, 010, 011, 100, 101, 110, 111
						if(thr_idx == 0) {
							sc[thr_idx].insert(&sc_process.at(scpidx));
						}
						else if(scpidx < (sc_process.getCount()-1)) {
							if(thr_idx == 1) {
								sc[thr_idx].insert(&sc_process.at(scpidx+1));
							}
							else if(thr_idx == 2) {
								sc[thr_idx].insert(&sc_process.at(scpidx));
								sc[thr_idx].insert(&sc_process.at(scpidx+1));
							}
							else if(scpidx < (sc_process.getCount()-2)) {
								if(thr_idx == 3) {
									sc[thr_idx].insert(&sc_process.at(scpidx+2));
								}
								else if(thr_idx == 4) {
									sc[thr_idx].insert(&sc_process.at(scpidx));
									sc[thr_idx].insert(&sc_process.at(scpidx+2));
								}
								else if(thr_idx == 5) {
									sc[thr_idx].insert(&sc_process.at(scpidx+1));
									sc[thr_idx].insert(&sc_process.at(scpidx+2));
								}
								else if(thr_idx == 6) {
									sc[thr_idx].insert(&sc_process.at(scpidx));
									sc[thr_idx].insert(&sc_process.at(scpidx+1));
									sc[thr_idx].insert(&sc_process.at(scpidx+2));
								}
							}
						}
						StrategySetSimulationTask::InitBlock tb(rTsTmList, rTsValList, rTrendListSet, sc[thr_idx], rConfig, &sre[thr_idx], &sre_ex[thr_idx]);
						StrategySetSimulationTask * p_thread = new StrategySetSimulationTask(&tb);
						THROW_S(p_thread, SLERR_NOMEM);
						p_thread->Start(1/*0*/);
						objs_to_wait[objs_to_wait_count++] = *p_thread;
					}
				}
				else if(best_subset_opt_chunk == 3) {
					for(thr_idx = 0; thr_idx < best_subset_opt_chunk; thr_idx++) {
						sc[thr_idx] = rScSelection;
						// 01, 10, 11
						if(thr_idx == 0) {
							sc[thr_idx].insert(&sc_process.at(scpidx));
						}
						else if(scpidx < (sc_process.getCount()-1)) {
							if(thr_idx == 1) {
								sc[thr_idx].insert(&sc_process.at(scpidx+1));
							}
							else if(thr_idx == 2) {
								sc[thr_idx].insert(&sc_process.at(scpidx));
								sc[thr_idx].insert(&sc_process.at(scpidx+1));
							}
						}
						StrategySetSimulationTask::InitBlock tb(rTsTmList, rTsValList, rTrendListSet, sc[thr_idx], rConfig, &sre[thr_idx], &sre_ex[thr_idx]);
						StrategySetSimulationTask * p_thread = new StrategySetSimulationTask(&tb);
						THROW_S(p_thread, SLERR_NOMEM);
						p_thread->Start(1/*0*/);
						objs_to_wait[objs_to_wait_count++] = *p_thread;
					}
				}
				::WaitForMultipleObjects(objs_to_wait_count, objs_to_wait, TRUE, INFINITE);
			}
			int    best_thr_idx = -1;
			enum {
				setcritResult = 1,
				setcritResultSpcMult,
				setcritWinRate
			};
			//
			// @20200529
			// Применил ограничение, предписывающее выбирать только такие комбинации стратегий, которые не снижают 
			// количество фактических ставок для предыдущих комбинаций. 
			//
			const  int  setcrit = setcritResultSpcMult;
			const  uint min_actual_stake_count = 0; // @v10.7.8
			double local_best_result = best_result;
			double local_best_result_spcmult = best_result_spcmult; // @v10.7.8
			double local_best_winrate = best_winrate; // @20200517
			double best_qfactor = 0.0; // @v10.7.8
			for(thr_idx = 0; thr_idx < best_subset_opt_chunk; thr_idx++) {
				const double win_rate = sre[thr_idx].GetWinCountRate();
				if(win_rate >= overall_win_rate_limit) {
					// @v10.7.8 const double local_thr_result = sre[thr_idx].V.Result * win_rate; // @20200413 (* win_rate)
					const double local_thr_result = sre[thr_idx].V.Result; /* / sqrt(static_cast<double>(sc[thr_idx].getCount()))*/; // @v10.7.8
					double local_thr_result_spcmult = 0.0;
					double local_thr_winrate = win_rate;
					double test_sre_ex_result = 0.0; // Итоговый результат просуммированный по всем стратегиям для проверки
					double qfactor = 0.0;
					{
						for(uint ssi = 0; ssi < sc[thr_idx].getCount(); ssi++) {
							uint stake_count_at_final_simulation = CalcStakeCountAtFinalSimulation(sre_ex[thr_idx], ssi);
							PPObjTimeSeries::Strategy & r_s = sc[thr_idx].at(ssi);
							r_s.StakeCountAtFinalSimulation = stake_count_at_final_simulation;
							const double sre_ex_result = CalcStakeResult(sre_ex[thr_idx], ssi);
							test_sre_ex_result += sre_ex_result;
							//local_thr_result_spcmult += (sre_ex_result * pow(static_cast<double>(r_s.StakeCountAtFinalSimulation), 1.0/3.0));
							qfactor += r_s.StakeCountAtFinalSimulation;
							//double local_factor = ((r_s.V.Result) * sqrt(static_cast<double>(r_s.StakeCountAtFinalSimulation) / static_cast<double>(r_s.StakeCount))); // @20200518
							// @20200519 double local_factor = ((r_s.V.Result) * static_cast<double>(r_s.StakeCountAtFinalSimulation)); // @20200518
							// @20200519 local_thr_result_spcmult += sre_ex_result * (local_factor);
							// SQ better than exp
							double mult3 = SQ(sre_ex_result / strategy_result_range.low); // @20200519
							//double mult3 = fpow2(win_rate * (log(r_s.StakeDistMedian * (r_s.WinCount-1)))); // 20200524
							local_thr_result_spcmult += mult3 * static_cast<double>(r_s.StakeCountAtFinalSimulation); // @20200519
						}
						// @20200518 const double fa = (qfactor / static_cast<double>(sc[thr_idx].getCount()));
						// @20200518 qfactor = 2.0 * log(fa);
						// @20200518 local_thr_result_spcmult = local_thr_result * qfactor;
						assert(feqeps(test_sre_ex_result, local_thr_result, 1.0E-7)); // test_sre_ex_result must be equal to local_thr_result
					}
					int   skip_chunk = 1;
					switch(setcrit) {
						case setcritResultSpcMult: skip_chunk = BIN(local_best_result_spcmult >= local_thr_result_spcmult); break;
						case setcritWinRate: skip_chunk = BIN(local_best_winrate >= local_thr_winrate); break;
						case setcritResult: skip_chunk = BIN(local_best_result >= local_thr_result); break;
						default: skip_chunk = 1; break;
					}
					if(!skip_chunk) {
						if(min_actual_stake_count > 0) {
							for(uint ssi = 0; ssi < sc[thr_idx].getCount(); ssi++) {
								uint stake_count_at_final_simulation = CalcStakeCountAtFinalSimulation(sre_ex[thr_idx], ssi);
								if(stake_count_at_final_simulation < min_actual_stake_count)
									skip_chunk = 1;
							}
						}
						// @20200513 {
						/*if(!skip_chunk && setcrit == setcritResult) {
							for(uint ssi = 0; ssi < sc[thr_idx].getCount(); ssi++) {
								uint stake_count_at_final_simulation = CalcStakeCountAtFinalSimulation(sre_ex[thr_idx], ssi);
								if((stake_count_at_final_simulation+3) < sc[thr_idx].at(ssi).StakeCount)
									skip_chunk = 1;
							}
						}*/
						// } @20200513 
						if(!skip_chunk /*&& setcrit == setcritResult*/) {
							for(uint ssi = 0; !skip_chunk && ssi < rScSelection.getCount(); ssi++) {
								const uint new_stake_count_at_final_simulation = CalcStakeCountAtFinalSimulation(sre_ex[thr_idx], ssi);
								const uint prev_stake_count_at_final_simulation = rScSelection.at(ssi).StakeCountAtFinalSimulation;
								if(new_stake_count_at_final_simulation < prev_stake_count_at_final_simulation)
									skip_chunk = 1;
							}
						}
						if(!skip_chunk) {
							local_best_result = local_thr_result;
							local_best_result_spcmult = local_thr_result_spcmult;
							local_best_winrate = local_thr_winrate;
							best_qfactor = qfactor;
							best_thr_idx = static_cast<int>(thr_idx);
						}
					}
				}
			}
			if(best_thr_idx >= 0) {
				phony_iter_no = 0;
				rScSelection = sc[best_thr_idx];
				SForEachVectorItem(rScSelection, si) { rScSelection.at(si).StakeCountAtFinalSimulation = CalcStakeCountAtFinalSimulation(sre_ex[best_thr_idx], si); } // @v10.7.7
				best_loss_count = sre[best_thr_idx].StakeCount - sre[best_thr_idx].WinCount; // @v10.7.1
				best_result = local_best_result;
				best_result_spcmult = local_best_result_spcmult;
				best_winrate = local_best_winrate;
				ASSIGN_PTR(pSre, sre[best_thr_idx]);
				if(pFOut) {
					msg_buf.Z().Cat("Local Good Subset").CatDiv(':', 2).Cat("BC-LS").CatChar('-').Cat(scpidx+1).Space().
						CatEq("Q-Factor", best_qfactor, MKSFMTD(0, 6, NMBF_NOTRAILZ));
					pFOut->WriteLine(msg_buf.CR());
					SForEachVectorItem(rScSelection, si) { pFOut->WriteLine(PPObjTimeSeries::StrategyToString(rScSelection.at(si), 0, 0, msg_buf.Z()).CR()); }
					OutputStategyResultEntry(sre[best_thr_idx], msg_buf);
					pFOut->WriteLine(msg_buf.CR());
					pFOut->Flush();
				}
				ok = 1;
			}
			else
				phony_iter_no++;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::Run()
{
	const int force_fixed_maxduck_values = 1;
	int    ok = 1;
	const  LDATETIME now = getcurdatetime_();
	SString temp_buf;
	SString save_file_name;
	SString msg_buf;
	PPIDArray id_pre_list;
	RealArray temp_real_list;
	PPTimeSeriesPacket ts_pack;
	TSVector <PPObjTimeSeries::QuoteReqEntry> quote_req_list;
	PPObjTssModel tss_model_obj;
	if(P.TsList.GetCount()) {
		P.TsList.Get(id_pre_list);
	}
	else {
		PPTimeSeries ts_rec;
		for(SEnum en = TsObj.Enum(0); en.Next(&ts_rec) > 0;)
			id_pre_list.add(ts_rec.ID);
	}
	if(id_pre_list.getCount()) {
		PPObjTimeSeries::Config config;
		THROW(PPObjTimeSeries::ReadConfig(&config));
		id_pre_list.sortAndUndup();
		PPIDArray id_list;
		if(P.Flags & P.fAutodetectTargets) {
			//
			// При автодетекте серий, требующих пересчета формируем список в порядке убывания времени
			// последнего изменения модели. Те серии, у которых нет модели вообще окажутся в начале списка
			// (будут пересчитаны в первую очередь)
			//
			struct TempEntry {
				TempEntry(LDATETIME dtm, PPID id) : StorageDtm(dtm), ID(id)
				{
				}
				const LDATETIME StorageDtm;
				const PPID   ID;
			};
			SVector temp_list(sizeof(TempEntry));
			for(uint i1 = 0; i1 < id_pre_list.getCount(); i1++) {
				const PPID id = id_pre_list.get(i1);
				PPTimeSeries ts_rec;
				if(TsObj.Search(id, &ts_rec) > 0) {
					PPObjTimeSeries::StrategyContainer sc;
					TempEntry new_entry((TsObj.GetStrategies(id, PPObjTimeSeries::sstSelection, sc) > 0) ? sc.GetStorageTm() : ZERODATETIME, id);
					THROW_SL(temp_list.insert(&new_entry));
				}
			}
			temp_list.sort(PTR_CMPFUNC(LDATETIME));
			SForEachVectorItem(temp_list, i2) { THROW_SL(id_list.add(static_cast<const TempEntry *>(temp_list.at(i2))->ID)); }
		}
		else
			id_list = id_pre_list;
		//
		PPWait(1);
		TSCollection <PPObjTimeSeries::TrendEntry> trend_list_set;
		SString out_file_name;
		SString out_total_file_name;
		PPGetFilePath(PPPATH_OUT, "AnalyzeTsStrategy2.txt", out_file_name);
		PPGetFilePath(PPPATH_OUT, "AnalyzeTsStrategy2-total.txt", out_total_file_name);
		const int is_out_file_exists = fileExists(out_file_name);
		SFile f_out(out_file_name, SFile::mAppend);
		SFile f_out_total(out_total_file_name, SFile::mAppend);
		f_out.WriteLine(msg_buf.Z().CatCharN('-', 20).CR());
		{
			msg_buf.Z().CatCharN('=', 8).Space().Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, 0).Space().CatCharN('=', 8);
			if(checkdate(P.UseDataForStrategiesTill))
				msg_buf.CR().CatEq("p.use_data_for_strategies_till", P.UseDataForStrategiesTill, DATF_DMY|DATF_CENTURY);
			msg_buf.CR();
			f_out.WriteLine(msg_buf);
			for(uint j = 0; j < id_list.getCount(); j++) {
				const PPID id = id_list.get(j);
				if(TsObj.GetPacket(id, &ts_pack) > 0) {
					PPObjTimeSeries::StrategyContainer sc;
					LDATETIME strg_dtm = ZERODATETIME;
					LDATETIME lastval_dtm = ZERODATETIME;
					uint   sver = 0;
					if(TsObj.GetStrategies(id, PPObjTimeSeries::sstSelection, sc) > 0) {
						strg_dtm = sc.GetStorageTm();
						lastval_dtm = sc.GetLastValTm();
						sver = sc.GetVersion();
					}
					msg_buf.Z().Cat(ts_pack.Rec.Symb).Space().Cat(sver).Space().Cat(strg_dtm, DATF_ISO8601|DATF_CENTURY, 0).Space().
						Cat(lastval_dtm, DATF_ISO8601|DATF_CENTURY);
					if(checkdate(ts_pack.E.UseDataForStrategiesSince))
						msg_buf.Space().CatEq("UseDataForStrategiesSince", ts_pack.E.UseDataForStrategiesSince, DATF_DMY|DATF_CENTURY);
					f_out.WriteLine(msg_buf.CR());
				}
			}
		}
		f_out.Flush();
		for(uint i = 0; i < id_list.getCount(); i++) {
			const PPID id = id_list.get(i);
			if(TsObj.GetPacket(id, &ts_pack) > 0) {
				PPTssModelPacket tss_model;
				if(GetTssModel(&ts_pack.Rec, &tss_model) > 0) {
					STimeSeries ts;
					const LDATE date_since = checkdate(ts_pack.E.UseDataForStrategiesSince) ? ts_pack.E.UseDataForStrategiesSince : tss_model.Rec.UseDataSince;
					const LDATE date_till = checkdate(P.UseDataForStrategiesTill) ? P.UseDataForStrategiesTill : ZERODATE; //(checkdate(model_param.UseDataTill) ? model_param.UseDataTill : ZERODATE);
					const int gtsr = GetTimeSeries(id, date_since, date_till, ts);
					tss_model.Output(msg_buf);
					f_out.WriteLine(msg_buf.CR());
					f_out.Flush();
					if(gtsr > 0) {
						{
							f_out.WriteLine(msg_buf.Z().CR());
							f_out.WriteLine(PPObjTimeSeries::StrategyOutput(0, 0, msg_buf).CR());
						}
						const uint tsc = ts.GetCount();
						const bool use_main_frame = LOGIC(tss_model.MainFrameSizeList.getCount() && tss_model.Rec.MainFrameRangeCount);
						STimeSeries::Stat st(0);
						st.LocalDevPtCount = config.E.LocalDevPtCount;
						uint   vec_idx = 0;
						THROW_SL(ts.GetValueVecIndex("close", &vec_idx));
						ts.Analyze("close", st);
						if(ts_pack.Rec.SpikeQuant_t <= 0.0 || (ts_pack.E.AvgLocalDeviation <= 0 && st.LocalDevAvg > 0.0) || (P.Flags & P.fForce)) {
							ts_pack.Rec.SpikeQuant_t = st.DeltaAvg / 2.0;
							ts_pack.E.AvgLocalDeviation = st.LocalDevAvg; // @v10.7.1
							PPID   temp_id = id;
							THROW(TsObj.PutPacket(&temp_id, &ts_pack, 1));
						}
						{
							DateTimeArray ts_tm_list;
							RealArray ts_val_list;
							PPObjTimeSeries::StrategyContainer scontainer;
							TSVector <PPObjTimeSeries::StrategyResultEntry> sr_raw_list;
							THROW_SL(ts.GetRealArray(vec_idx, 0, tsc, ts_val_list));
							THROW_SL(ts.GetTimeArray(0, tsc, ts_tm_list));
							assert(ts_tm_list.getCount() == tsc);
							assert(ts_val_list.getCount() == tsc);
							const LDATETIME last_val_dtm = ts_tm_list.get(tsc-1);
							if(P.Flags & P.fAnalyzeModels) {
								scontainer.SetLastValTm(last_val_dtm);
								scontainer.SetUseDataForStrategiesTill(date_till); // @v10.7.3
								scontainer.SetupByTsPacket(ts_pack, &tss_model); // @v10.7.1
								{
									PPTssModelPacket analyze_tss_model = tss_model;
									TSCollection <TestStrategyRawResult> tsrr_list;
									TsFindStrategiesLoopBlock fslblk(analyze_tss_model, config, ts_pack, ts, ts_tm_list, ts_val_list, trend_list_set, scontainer, &tsrr_list);
									fslblk.P_FOut = &f_out;
									THROW(FindStrategiesLoop(&fslblk));
								}
							}
							if(P.Flags & P.fFindStrategies) {
								scontainer.SetLastValTm(last_val_dtm);
								scontainer.SetUseDataForStrategiesTill(date_till); // @v10.7.3
								scontainer.SetupByTsPacket(ts_pack, &tss_model); // @v10.7.1
								{
									TsFindStrategiesLoopBlock fslblk(tss_model, config, ts_pack, ts, ts_tm_list, ts_val_list, trend_list_set, scontainer, 0);
									fslblk.P_FOut = &f_out;
									// @v10.7.8 fslblk.P_FDump = &f_dump;
									THROW(FindStrategiesLoop(&fslblk));
								}
								double best_sc_result = 0.0;
								uint   best_sc_idx = 0;
								double prev_result = 0.0;
								LongArray to_remove_pos_list;
								LongArray have_analog_pos_list;
								PPObjTimeSeries::StrategyContainer sc_selection;
								PPObjTimeSeries::StrategyContainer sc_process;
								PPObjTimeSeries::StrategyContainer sc_skip_due_dup; // @v10.6.9 стратегии, вынесенные из рассмотрения, поскольку для них есть более удачливые дубликаты
								/* @v10.7.10
								const  uint cqa_match_promille = (tss_model.Rec.CqaMatchPromille > 0) ? tss_model.Rec.CqaMatchPromille : 0;
								long   ssflags = PPObjTimeSeries::StrategyContainer::gbsfLong|PPObjTimeSeries::StrategyContainer::gbsfShort|
									PPObjTimeSeries::StrategyContainer::gbsfStakeMode3|PPObjTimeSeries::StrategyContainer::gbsfStakeMode2|PPObjTimeSeries::StrategyContainer::gbsfStakeMode1;
								//
								// Если (tss_model.Rec.MinSimilItems > 1) то удалять схожие по CQA-фактору стратегии нельзя - они должны работать вместе
								//
								if((tss_model.Rec.MinSimilItems <= 1) && cqa_match_promille) // @v10.7.7 
									ssflags |= scontainer.gbsfEliminateCQADups; // @v10.7.4
								if(tss_model.Rec.Flags & PPTssModel::fBestSubsetTrendFollowing)
									ssflags |= scontainer.gbsfTrendFollowing;
								switch(tss_model.Rec.StrategyPoolSortOrder) {
									case PPTssModel::sppsResult:     ssflags |= scontainer.gbsfCritTotalResult; break;
									case PPTssModel::sppsProb:       ssflags |= scontainer.gbsfCritProb; break;
									case PPTssModel::sppsVelocity:   ssflags |= scontainer.gbsfCritProfitMultProb; break; // !Не соответствует выбор флагу
									case PPTssModel::sppsCqaFactor:  ssflags |= scontainer.gbsfOptDeltaRangeCQA; break;
									case PPTssModel::sppsShuffle:    ssflags |= scontainer.gbsfShuffle; break;
									case PPTssModel::sppsStakeCount: ssflags |= scontainer.gbsfCritStakeCount; break; // @v10.7.9
								}
								ssflags |= scontainer.gbsfEliminate100prob; // @v10.7.9
								if(scontainer.GetBestSubset(ssflags, tss_model.Rec.BestSubsetDimention, tss_model.Rec.MinWinRate, 
									cqa_match_promille, sc_process, &sc_skip_due_dup, &to_remove_pos_list, &have_analog_pos_list) > 0) { 
								*/
								if(scontainer.GetBestSubset2(tss_model, sc_process, &sc_skip_due_dup, &to_remove_pos_list, &have_analog_pos_list) > 0) { // @v10.7.10
									{
										PPObjTimeSeries::StrategyContainer scollection_full(scontainer);
										if(scollection_full.getCount()) {
											msg_buf.Z().CatCharN('-', 8).Space().Cat("strategies-ordered-by-cqa").Space().CatEq("count", scollection_full.getCount()).CR();
											f_out.WriteLine(msg_buf);
											TSArray <PPObjTimeSeries::StrategyContainer::CritEntry> sc_order_index;
											scollection_full.MakeOrderIndex(PPObjTimeSeries::StrategyContainer::gbsfOptDeltaRangeCQA, sc_order_index);
											for(uint scidx = 0; scidx < sc_order_index.getCount(); scidx++) {
												const PPObjTimeSeries::StrategyContainer::CritEntry & r_ie = sc_order_index.at(scidx);
												assert(r_ie.Idx > 0 && r_ie.Idx <= scollection_full.getCount());
												if(r_ie.Idx > 0 && r_ie.Idx <= scollection_full.getCount()) {
													const uint ___spos = r_ie.Idx-1;
													const PPObjTimeSeries::Strategy & r_s = scollection_full.at(___spos);
													msg_buf.Z().Cat(PPObjTimeSeries::StrategyOutput(ts_pack.GetSymb(), &r_s, temp_buf)).Space();
													if(to_remove_pos_list.lsearch(___spos))
														msg_buf.Cat("R");
													if(have_analog_pos_list.lsearch(___spos))
														msg_buf.Cat("A");
													f_out.WriteLine(msg_buf.CR());
												}
											}
										}
									}
									//sc_process.TrimByVariance(0.2); // @20200520
									{
										f_out.WriteLine(msg_buf.Z().CR());
										f_out.WriteLine((msg_buf = "--- TsSimulateStrategyContainer").Space().Cat(ts_pack.GetSymb()).CR());
										msg_buf.Z().Cat("Full Subset").CatDiv(':', 2).Cat(sc_process.getCount());
										f_out.WriteLine(msg_buf.CR());
										SForEachVectorItem(sc_process, si) { f_out.WriteLine(PPObjTimeSeries::StrategyToString(sc_process.at(si), 0, 0, msg_buf).CR()); }
									}
									{
										f_out.WriteLine(msg_buf.Z().CR());
										f_out.WriteLine((msg_buf = "--- TsSkippedDueDupStrategyContainer").Space().Cat(ts_pack.GetSymb()).CR());
										msg_buf.Z().Cat("Full Subset").CatDiv(':', 2).Cat(sc_skip_due_dup.getCount());
										f_out.WriteLine(msg_buf.CR());
										SForEachVectorItem(sc_skip_due_dup, si) { f_out.WriteLine(PPObjTimeSeries::StrategyToString(sc_skip_due_dup.at(si), 0, 0, msg_buf).CR()); }
									}
									sc_skip_due_dup.freeAll();
									THROW(StrategyContainerDressing(config, ts_pack, tss_model, ts_tm_list, ts_val_list, trend_list_set, sc_process, sc_selection, &f_out));
								}
								if(sc_selection.getCount()) {
									//
									// Контрольный тест для отборного контейнера sc_selection
									//
									sc_selection.SetLastValTm(last_val_dtm);
									msg_buf.Z().Cat("Selected-Subset").CatDiv(':', 2).Cat("BC-LS").CatChar('-').Cat(sc_selection.getCount());
									f_out.WriteLine(msg_buf.CR());
									SForEachVectorItem(sc_selection, si) { f_out.WriteLine(PPObjTimeSeries::StrategyToString(sc_selection.at(si), 0, 0, msg_buf).CR()); }
									PPObjTimeSeries::StrategyResultEntry sre;
									TSVector <PPObjTimeSeries::StrategyResultValueEx> sr_detail_list; // @v10.7.10
									sc_selection.Simulate(ts_tm_list, ts_val_list, trend_list_set, config, sre, &sr_detail_list);
									// @v10.7.10 {
									{
										for(uint ssi = 0; ssi < sc_selection.getCount(); ssi++) {
											uint stake_count_at_final_simulation = CalcStakeCountAtFinalSimulation(sr_detail_list, ssi);
											PPObjTimeSeries::Strategy & r_s = sc_selection.at(ssi);
											r_s.StakeCountAtFinalSimulation = stake_count_at_final_simulation;
										}
									}
									// } @v10.7.10 
									THROW(TsObj.PutStrategies(id, PPObjTimeSeries::sstSelection, &sc_selection, 1)); // Сохранение отобранных стратегий в базе данных
									OutputStategyResultEntry(sre, msg_buf);
									f_out.WriteLine(msg_buf.CR());
									f_out.Flush();
									if(f_out_total.IsValid()) {
										struct MainFrameCountEntry {
											MainFrameCountEntry(const RealRange & rR) : R(rR), Count(1)
											{
											}
											RealRange R;
											long   Count;
										};
										LAssocArray ifs_count_list;
										LAssocArray md_count_list;
										LAssocArray target_count_list;
										SVector main_frame_count_list(sizeof(MainFrameCountEntry));
										for(uint scidx = 0; scidx < sc_selection.getCount(); scidx++) {
											const PPObjTimeSeries::Strategy & r_sc = sc_selection.at(scidx);
											ifs_count_list.IncrementValueByKey(static_cast<long>(r_sc.InputFrameSize));
											md_count_list.IncrementValueByKey(static_cast<long>(r_sc.MaxDuckQuant));
											target_count_list.IncrementValueByKey(static_cast<long>(r_sc.TargetQuant));
											if(r_sc.MainFrameSize) {
												int   mf_found = 0;
												for(uint i = 0; i < main_frame_count_list.getCount(); i++) {
													MainFrameCountEntry * p_mfce = static_cast<MainFrameCountEntry *>(main_frame_count_list.at(i));
													if(p_mfce->R.IsEqual(r_sc.OptDelta2Range)) {
														p_mfce->Count++;
														mf_found = 1;
													}
												}
												if(!mf_found) {
													MainFrameCountEntry new_entry(r_sc.OptDelta2Range);
													main_frame_count_list.insert(&new_entry);
												}
											}
										}
										msg_buf.Z().Cat(getcurdatetime_(), DATF_YMD|DATF_CENTURY, TIMF_HMS).Space().Cat(ts_pack.GetSymb());
										f_out_total.WriteLine(msg_buf.CR());
										SForEachVectorItem(sc_selection, si) { f_out_total.WriteLine(PPObjTimeSeries::StrategyToString(sc_selection.at(si), 0, 0, msg_buf).CR()); }
										OutputStategyResultEntry(sre, msg_buf);
										f_out_total.WriteLine(msg_buf.CR());
										{
											double vc = sc_selection.EvaluateScore(sc_selection.scoreEvStakeCountVarCoeff, 0);
											msg_buf.Z().Cat("scoreEvStakeCountVarCoeff").CatDiv(':', 2).Cat(vc, MKSFMTD(0, 5, 0));
											f_out_total.WriteLine(msg_buf.CR());
										}
										{
											double vc = sc_selection.EvaluateScore(sc_selection.scoreSeStakeCountVarCoeff, 0);
											msg_buf.Z().Cat("scoreSeStakeCountVarCoeff").CatDiv(':', 2).Cat(vc, MKSFMTD(0, 5, 0));
											f_out_total.WriteLine(msg_buf.CR());
										}
										{
											msg_buf.Z().Cat("InputFrameSize").CatDiv(':', 2);
											ifs_count_list.Sort();
											for(uint clidx = 0; clidx < ifs_count_list.getCount(); clidx++) {
												const LAssoc & r_assoc = ifs_count_list.at(clidx);
												msg_buf.CatDivConditionally(' ', 0, LOGIC(clidx)).Cat(r_assoc.Key).CatChar('/').Cat(r_assoc.Val);
											}
											f_out_total.WriteLine(msg_buf.CR());
										}
										{
											msg_buf.Z().Cat("MaxDuckQuant").CatDiv(':', 2);
											md_count_list.Sort();
											for(uint clidx = 0; clidx < md_count_list.getCount(); clidx++) {
												const LAssoc & r_assoc = md_count_list.at(clidx);
												msg_buf.CatDivConditionally(' ', 0, LOGIC(clidx)).Cat(r_assoc.Key).CatChar('/').Cat(r_assoc.Val);
											}
											f_out_total.WriteLine(msg_buf.CR());
										}
										{
											msg_buf.Z().Cat("TargetQuant").CatDiv(':', 2);
											target_count_list.Sort();
											for(uint clidx = 0; clidx < target_count_list.getCount(); clidx++) {
												const LAssoc & r_assoc = target_count_list.at(clidx);
												msg_buf.CatDivConditionally(' ', 0, LOGIC(clidx)).Cat(r_assoc.Key).CatChar('/').Cat(r_assoc.Val);
											}
											f_out_total.WriteLine(msg_buf.CR());
										}
										// @v10.6.8 {
										{
											const long trange_fmt = MKSFMTD(0, 10, NMBF_NOTRAILZ);
											msg_buf.Z().Cat("MainFrameRange").CatDiv(':', 2);
											for(uint clidx = 0; clidx < main_frame_count_list.getCount(); clidx++) {
												const MainFrameCountEntry * p_entry = static_cast<const MainFrameCountEntry *>(main_frame_count_list.at(clidx));
												msg_buf.CatDivConditionally(' ', 0, LOGIC(clidx)).CatChar('[').Cat(p_entry->R, trange_fmt).CatChar(']').CatChar('/').Cat(p_entry->Count);
											}
											f_out_total.WriteLine(msg_buf.CR());
										}
										// } @v10.6.8 
										f_out_total.Flush();
									}
								}
							}
						}
					}
				}
			}
		}
		// @v10.7.0 {
		if(P.Flags & P.fSimulateAfter) {
			THROW(TryStrategyContainers(id_list));
		}
		// } @v10.7.0 
	}
	CATCH
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
		ok = 0;
	ENDCATCH
	PPWait(0);
	return ok;
}

int PPObjTimeSeries::TrendEntry::Analyze(const PPTssModelPacket & rTssModel, SString & rOutBuf) const
{
	int    ok = 1;
	rOutBuf.Z();

	const uint input_frame_size = Stride;
	rOutBuf.Tab().CatEq("stride", input_frame_size).CR();
	//f_out.WriteLine(msg_buf.Z().Tab().CatEq("stride", input_frame_size).CR());
	
	//PPObjTimeSeries::TrendEntry trend_entry(input_frame_size, tsc);
	//StatBase cov00_stat(0);
	//StatBase cov00sqr_stat(0);
	//STimeSeries::AnalyzeFitParam afp(input_frame_size, 0, 0);
	//THROW(ts.AnalyzeFit("close", afp, &trend_entry.TL, 0, &cov00_list, &cov01_list, &cov11_list));
	//assert(trend_entry.TL.getCount() == tsc);
	//assert(cov00_list.getCount() == tsc);
	//assert(cov01_list.getCount() == tsc);
	//assert(cov11_list.getCount() == tsc);
	{
		const double model_err_limit_ = rTssModel.Rec.InitTrendErrLimit_;
		StatBase errl_stat;
		RealArray temp_real_list = ErrL;
		errl_stat.Step(temp_real_list);
		errl_stat.Finish();
		temp_real_list.Sort();
		const double median = temp_real_list.at(temp_real_list.getCount() / 2);
		const double median_2 = temp_real_list.at(temp_real_list.getCount() / 4);
		const double median_3 = temp_real_list.at(3 * temp_real_list.getCount() / 4);
		rOutBuf.Tab(2).Cat("error-list").CatDiv(':', 2).CR().Tab(3).
			CatEq("count", errl_stat.GetCount()).Space().
			CatEq("min", errl_stat.GetMin(), MKSFMTD(0, 6, 0)).Space().
			CatEq("max", errl_stat.GetMax(), MKSFMTD(0, 6, 0)).Space().
			CatEq("avg", errl_stat.GetExp(), MKSFMTD(0, 6, 0)).Space().
			CatEq("variance", errl_stat.GetVariance(), MKSFMTD(0, 6, 0)).Space().
			CatEq("stddev", errl_stat.GetStdDev(), MKSFMTD(0, 6, 0)).Space().
			CatEq("median", median, MKSFMTD(0, 6, 0)).Space().
			CatEq("median (1/4)", median_2, MKSFMTD(0, 6, 0)).Space().
			CatEq("median (3/4)", median_3, MKSFMTD(0, 6, 0)).Space().
			CatEq("median/avg", median/errl_stat.GetExp(), MKSFMTD(0, 3, 0)).Space().
			CatEq("median (1/4)/avg", median_2/errl_stat.GetExp(), MKSFMTD(0, 3, 0)).Space().
			CatEq("median (3/4)/avg", median_3/errl_stat.GetExp(), MKSFMTD(0, 3, 0));
		if(!(rTssModel.Rec.Flags & PPTssModel::fTrendErrLimitAsMedianPart) && model_err_limit_ > 0.0) {
			rOutBuf.CR().Tab(3).CatEq("init_trend_err_limit", model_err_limit_, MKSFMTD(0, 3, 0)).Space();
			const double b = errl_stat.GetExp() * model_err_limit_;
			uint  err_lim_pos = 0;
			uint _cp = temp_real_list.getCount();
			if(_cp) do {
				double ev = temp_real_list.at(--_cp);
				if(ev < b) {
					err_lim_pos = _cp;
					break;
				}
			} while(_cp);
			rOutBuf.CatEq("err_lim_part", fdivui(err_lim_pos, temp_real_list.getCount()), MKSFMTD(0, 3, 0));
		}
		if((rTssModel.Rec.Flags & PPTssModel::fTrendErrLimitAsMedianPart) && model_err_limit_ > 0.0 && model_err_limit_ <= 1.0) {
			rOutBuf.CR().Tab(3).CatEq("partitial_trend_err_limit", model_err_limit_, MKSFMTD(0, 3, 0)).Space();
			uint lim_pos = static_cast<uint>(R0i(model_err_limit_ * static_cast<double>(temp_real_list.getCount())));
			if(lim_pos >= 0 && lim_pos < temp_real_list.getCount()) {
				double err_limit_by_pel = temp_real_list.at(lim_pos) / errl_stat.GetExp();
				rOutBuf.CatEq("err_limit_by_pel", err_limit_by_pel, MKSFMTD(0, 3, 0));
			}
		}
		//f_out.WriteLine(msg_buf.CR());
	}
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::AnalyzeRegression(PPID tsID)
{
	int    ok = -1;
	SString temp_buf;
	SString msg_buf;
	PPTimeSeries ts_rec;
	PPWait(1);
	if(TsObj.Search(tsID, &ts_rec) > 0) {
		STimeSeries ts;
		STimeSeries ts_last_chunk;
		TSCollection <PPObjTimeSeries::TrendEntry> trend_list_set;
		int gtsr = TsObj.GetTimeSeries(tsID, ts);
		if(gtsr > 0) {
			PPTssModelPacket tss_model;
			if(GetTssModel(&ts_rec, &tss_model) > 0) {
				const uint tsc = ts.GetCount();
				SString out_file_name;
				(temp_buf = "tsregression").CatChar('-').Cat(ts_rec.Symb);
				PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
				SFile f_out(out_file_name, SFile::mAppend);	
				THROW_SL(f_out.IsValid());
				const bool use_main_frame = LOGIC(tss_model.MainFrameSizeList.getCount() && tss_model.Rec.MainFrameRangeCount);
				{
					LongArray frame_size_list = tss_model.InputFrameSizeList;
					if(use_main_frame) {
						for(uint mfsidx = 0; mfsidx < tss_model.MainFrameSizeList.getCount(); mfsidx++) {
							frame_size_list.addUnique(tss_model.MainFrameSizeList.get(mfsidx));
						}
					}
					THROW(MakeArVectors(ts, frame_size_list, 0, 
						((tss_model.Rec.Flags & PPTssModel::fTrendErrLimitAsMedianPart) ? tss_model.Rec.InitTrendErrLimit_ : 0.0), trend_list_set));
				}
				{
					uint   vec_idx = 0;
					RealArray cov00_list;
					RealArray cov01_list;
					RealArray cov11_list;
					RealArray temp_real_list;
					THROW_SL(ts.GetValueVecIndex("close", &vec_idx));
					f_out.WriteLine(msg_buf.Z().CatCharN('-', 8).Space().Cat(getcurdatetime_(), DATF_ISO8601, 0).CR());
					for(uint ifsidx = 0; ifsidx < trend_list_set.getCount(); ifsidx++) {
						const PPObjTimeSeries::TrendEntry * p_te = trend_list_set.at(ifsidx);
						if(p_te) {
							p_te->Analyze(tss_model, msg_buf);
							f_out.WriteLine(msg_buf.CR());
							/*for(uint trlidx = 0; trlidx < tsc; trlidx++) {
								const double cov00 = cov00_list.at(trlidx);
								const double cov00sqr = sqrt(cov00);
								assert(cov00 >= 0.0);
								if(cov00 > 0.0) { // strictly 'greater than'
									cov00_stat.Step(cov00);
									cov00sqr_stat.Step(cov00sqr);
								}
							}
							cov00_stat.Finish();
							cov00sqr_stat.Finish();
							const double cov00_avg = cov00_stat.GetExp();
							const double cov00sqr_avg = cov00sqr_stat.GetExp();
							for(uint trlidx = 0; trlidx < tsc; trlidx++) {
								const double cov00 = cov00_list.at(trlidx);
								const double cov00sqr = sqrt(cov00);
								assert(cov00 >= 0.0);
								double ts_value = 0.0;
								ts.GetValue(trlidx, vec_idx, &ts_value);
								msg_buf.Z().Cat(ts_value, MKSFMTD(0, 5, 0));
								msg_buf.Space().Cat(cov00 * 10e9, MKSFMTD(0, 3, 0));
								if(cov00_avg > 0.0)
									msg_buf.Space().Cat(cov00 / cov00_avg, MKSFMTD(0, 3, 0));
								else
									msg_buf.Space().Cat("---");
								msg_buf.Space().Cat(cov00sqr * 10e9, MKSFMTD(0, 3, 0));
								if(cov00sqr_avg > 0.0)
									msg_buf.Space().Cat(cov00sqr / cov00sqr_avg, MKSFMTD(0, 3, 0));
								else
									msg_buf.Space().Cat("---");
								msg_buf.Space().Cat(cov01_list.at(trlidx) * 10e9, MKSFMTD(0, 3, 0));
								msg_buf.Space().Cat(cov11_list.at(trlidx) * 10e9, MKSFMTD(0, 3, 0));
								f_out.WriteLine(msg_buf.CR());
							}
							msg_buf.Z().CatEq("count", tsc).Space().CatEq("frame", input_frame_size).Space().
								CatEq("cov00-avg", cov00_avg * 10e9, MKSFMTD(0, 3, 0)).Space().
								CatEq("cov00sqr-avg", cov00sqr_avg * 10e9, MKSFMTD(0, 3, 0));
							f_out.WriteLine(msg_buf.CR());
							*/
						}
					}
				}
			}
		}
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

class TryStrategyContainerResult {
public:
	SLAPI  TryStrategyContainerResult() : TsID(0)
	{
	}
	PPID   TsID;
	PPObjTimeSeries::StrategyContainer Sc;
	TSVector <PPObjTimeSeries::StrategyResultValueEx> DetailsList;
};

class TryStrategyContainerResultCollection : public TSCollection <TryStrategyContainerResult> {
public:
	struct IndexEntry {
		STimeChunk TmR;
		uint   EntryIdx;
		uint   DlIdx;
	};
	SLAPI  TryStrategyContainerResultCollection()
	{
	}
	void   SLAPI GetTsList(PPIDArray & rTsList) const
	{
		rTsList.clear();
		for(uint i = 0; i < getCount(); i++) {
			const TryStrategyContainerResult * p_item = at(i);
			assert(p_item);
			if(p_item)
				rTsList.add(p_item->TsID);
		}
		rTsList.sortAndUndup();
	}
	int    SLAPI MakeIndex();
	uint   SLAPI GetIndexCount() const { return Index.getCount(); }
	const  TryStrategyContainerResult * SLAPI GetEntryByIndex(uint idx, uint * pDetailEntryIdx) const
	{
		const  TryStrategyContainerResult * p_ret = 0;
		if(idx < Index.getCount()) {
			const IndexEntry & r_ie = Index.at(idx);
			assert(r_ie.EntryIdx < getCount());
			if(r_ie.EntryIdx < getCount()) {
				const TryStrategyContainerResult * p_item = at(r_ie.EntryIdx);
				assert(p_item);
				if(p_item) {
					assert(r_ie.DlIdx < p_item->DetailsList.getCount());
					if(r_ie.DlIdx < p_item->DetailsList.getCount()) {
						ASSIGN_PTR(pDetailEntryIdx, r_ie.DlIdx);
						p_ret = p_item;
					}
				}
			}
		}
		return p_ret;
	}
private:
	TSVector <IndexEntry> Index;
};

static IMPL_CMPFUNC(TryStrategyContainerResultCollectionIndexEntry, i1, i2)
{
	const TryStrategyContainerResultCollection::IndexEntry * p1 = static_cast<const TryStrategyContainerResultCollection::IndexEntry *>(i1);
	const TryStrategyContainerResultCollection::IndexEntry * p2 = static_cast<const TryStrategyContainerResultCollection::IndexEntry *>(i2);
	return p1->TmR.cmp(p2->TmR);
}

int SLAPI TryStrategyContainerResultCollection::MakeIndex()
{
	Index.clear();
	int    ok = 1;
	for(uint i = 0; i < getCount(); i++) {
		const TryStrategyContainerResult * p_item = at(i);
		assert(p_item);
		if(p_item) {
			for(uint j = 0; j < p_item->DetailsList.getCount(); j++) {
				PPObjTimeSeries::StrategyResultValueEx & r_dentry = p_item->DetailsList.at(j);
				IndexEntry new_entry;
				new_entry.TmR = r_dentry.TmR;
				new_entry.EntryIdx = i;
				new_entry.DlIdx = j;
				THROW_SL(Index.insert(&new_entry));
			}
		}
	}
	Index.sort(PTR_CMPFUNC(TryStrategyContainerResultCollectionIndexEntry));
	CATCHZOK
	return ok;
}

int SLAPI PrcssrTsStrategyAnalyze::TryStrategyContainers(const PPIDArray & rTsList)
{
	int    ok = -1;
	SString temp_buf;
	SString msg_buf;
	STimeSeries ts;
	STimeSeries ts_last_chunk;
	PPObjTimeSeries::Config config; // @v10.7.1
	TryStrategyContainerResultCollection result_collection;
	PPWait(1);
	THROW(PPObjTimeSeries::ReadConfig(&config)); // @v10.7.1
	for(uint ts_idx = 0; ts_idx < rTsList.getCount(); ts_idx++) {
		const PPID ts_id = rTsList.get(ts_idx);
		PPTimeSeriesPacket ts_pack;
		if(TsObj.GetPacket(ts_id, &ts_pack) > 0) {
			PPTssModelPacket tss_model;
			if(GetTssModel(&ts_pack.Rec, &tss_model) > 0) {
				// @v10.7.10 config.E.ChaosFactor = tss_model.Rec.ChaosFactor; // @v10.7.1
				TSCollection <PPObjTimeSeries::TrendEntry> trend_list_set;
				const LDATE date_since = checkdate(ts_pack.E.UseDataForStrategiesSince) ? ts_pack.E.UseDataForStrategiesSince : tss_model.Rec.UseDataSince;
				const int gtsr = GetTimeSeries(ts_id, date_since, ZERODATE/*dateTill*/, ts);
				if(gtsr > 0) {
					const uint tsc = ts.GetCount();
					TryStrategyContainerResult * p_result_entry = 0;
					THROW_SL(p_result_entry = result_collection.CreateNewItem());
					p_result_entry->TsID = ts_id;
					TsObj.GetStrategies(ts_id, PPObjTimeSeries::sstSelection, p_result_entry->Sc);
					{
						PROFILE_START
						uint  max_opt_delta2_stride = 0;
						DateTimeArray ts_tm_list;
						RealArray ts_val_list;
						LongArray ifs_list;
						RealArray temp_real_list;
						uint   vec_idx = 0;
						THROW_SL(ts.GetValueVecIndex("close", &vec_idx));
						if(tsc > 500 && p_result_entry->Sc.GetInputFramSizeList(ifs_list, &max_opt_delta2_stride) > 0) {
							const uint trend_nominal_count = max_opt_delta2_stride + 3;
							ifs_list.sortAndUndup(); // @paranoic
							ts_last_chunk.Z();
							if(tsc > 10000) {
								ts.GetChunkRecentCount(10000, ts_last_chunk);
							}
							const uint tsc_test_chunk = ts_last_chunk.GetCount();
							{
	#ifndef NDEBUG
								THROW(MakeArVectors(ts, ifs_list, mavfDontSqrtErrList, 
									((tss_model.Rec.Flags & PPTssModel::fTrendErrLimitAsMedianPart) ? tss_model.Rec.InitTrendErrLimit_ : 0.0), trend_list_set));
								if(tsc_test_chunk) {
									for(uint tlsidx = 0; tlsidx < trend_list_set.getCount(); tlsidx++) {
										PPObjTimeSeries::TrendEntry * p_trend_entry = trend_list_set.at(tlsidx);
										assert(p_trend_entry);
										if(p_trend_entry) {
											const uint input_frame_size = p_trend_entry->Stride;
											//
											// Тестовый блок призванный проверить эквивалентность расчета трендов для построения стратегии и
											// при использовании рядов для реального трейдинга.
											//
											STimeSeries::AnalyzeFitParam afp2(input_frame_size, tsc_test_chunk-trend_nominal_count, trend_nominal_count);
											RealArray test_trend_list;
											RealArray test_err_list;
											THROW(ts_last_chunk.AnalyzeFit("close", afp2, &test_trend_list, 0, &test_err_list, 0, 0));
											uint tcidx = test_trend_list.getCount();
											uint wcidx = p_trend_entry->TL.getCount();
											assert(wcidx >= tcidx);
											if(tcidx) do {
												const double tv = test_trend_list.at(--tcidx);
												const double wv = p_trend_entry->TL.at(--wcidx);
												assert(tv == wv);
											} while(tcidx);
											//
											uint teidx = test_err_list.getCount();
											uint weidx = p_trend_entry->ErrL.getCount();
											assert(weidx >= teidx);
											if(teidx) do {
												const double tv = test_err_list.at(--teidx);
												const double wv = p_trend_entry->ErrL.at(--weidx);
												assert(tv == wv);
											} while(teidx);
											p_trend_entry->SqrtErrList(0); // see flag mavfDontSqrtErrList in MakeArVectors
										}
									}
								}
	#else
								THROW(MakeArVectors(ts, ifs_list, 0, 
									((tss_model.Rec.Flags & PPTssModel::fTrendErrLimitAsMedianPart) ? tss_model.Rec.InitTrendErrLimit_ : 0.0), trend_list_set));
	#endif
							}
							{
								TSVector <PPObjTimeSeries::StrategyResultEntry> sr_raw_list;
								TSVector <RealRange> main_frame_range_list;
								THROW_SL(ts.GetRealArray(vec_idx, 0, tsc, ts_val_list));
								THROW_SL(ts.GetTimeArray(0, tsc, ts_tm_list));
								assert(ts_tm_list.getCount() == tsc);
								assert(ts_val_list.getCount() == tsc);
							}
							// @v10.6.12 {
							{
								int do_store_sc = 0;
								for(uint sidx = 0; sidx < p_result_entry->Sc.getCount(); sidx++) {
									PPObjTimeSeries::Strategy & r_s = p_result_entry->Sc.at(sidx);
									if(r_s.MainFrameSize > 0 && r_s.MainTrendErrAvg == 0.0) {
										for(uint tlsidx = 0; tlsidx < trend_list_set.getCount(); tlsidx++) {
											PPObjTimeSeries::TrendEntry * p_trend_entry = trend_list_set.at(tlsidx);
											assert(p_trend_entry);
											if(p_trend_entry && p_trend_entry->Stride == r_s.MainFrameSize) {
												r_s.MainTrendErrAvg = p_trend_entry->ErrAvg;
												do_store_sc = 1;
											}
										}
									}
								}
								if(do_store_sc)
									THROW(TsObj.PutStrategies(ts_id, PPObjTimeSeries::sstSelection, &p_result_entry->Sc, 1));
							}
							// } @v10.6.12 
							{
								SString out_file_name;
								(temp_buf = "tsscsim").CatChar('-').Cat(ts_pack.Rec.Symb);
								PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
								SFile f_out(out_file_name, SFile::mAppend);

								msg_buf.Z().CatCharN('=', 8).Space().Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, 0).Space().CatCharN('=', 8);
								f_out.WriteLine(msg_buf.CR());

								tss_model.Output(msg_buf);
								f_out.WriteLine(msg_buf.CR().CR());

								msg_buf.Z().Cat("Selected-Subset").CatDiv(':', 2).Cat("BC-LS").CatChar('-').Cat(p_result_entry->Sc.getCount());
								LDATE till_date = p_result_entry->Sc.GetUseDataForStrategiesTill();
								if(checkdate(till_date))
									msg_buf.Space().CatEq("UseDataForStrategiesTill", till_date, DATF_ISO8601|DATF_CENTURY);
								else 
									till_date = ZERODATE;
								f_out.WriteLine(msg_buf.CR());
								for(uint si = 0; si < p_result_entry->Sc.getCount(); si++) {
									PPObjTimeSeries::StrategyToString(p_result_entry->Sc.at(si), 0, 0, msg_buf.Z());
									f_out.WriteLine(msg_buf.CR());
								}
								f_out.WriteLine(msg_buf.Z().CR());
								{
									uint after_till_stake_count = 0;
									uint after_till_win_count = 0;
									uint after_till_loss_count = 0;
									PPObjTimeSeries::StrategyResultEntry sre;
									PROFILE_START
									p_result_entry->Sc.Simulate(ts_tm_list, ts_val_list, trend_list_set, config, sre, &p_result_entry->DetailsList);
									PROFILE_END
									OutputStategyResultEntry(sre, msg_buf);
									f_out.WriteLine(msg_buf.CR());
									for(uint dlidx = 0; dlidx < p_result_entry->DetailsList.getCount(); dlidx++) {
										PPObjTimeSeries::StrategyResultValueEx & r_dli = p_result_entry->DetailsList.at(dlidx);
										if(till_date && r_dli.TmR.Start.d > till_date) {
											after_till_stake_count++;
											if(r_dli.Result > 0.0)
												after_till_win_count++;
											else
												after_till_loss_count++;
										}
										PPObjTimeSeries::StrategyToString(p_result_entry->Sc.at(r_dli.StrategyIdx), &r_dli.OptFactor, &r_dli.OptFactor2, temp_buf);
										msg_buf.Z().
											Cat(r_dli.TmR.Start, DATF_ISO8601|DATF_CENTURY, 0).
											Tab().Cat(r_dli.TmR.Finish, DATF_ISO8601|DATF_CENTURY, 0).
											Tab().Cat(r_dli.Result, MKSFMTD(0, 5, 0)).
											Tab().Cat(r_dli.TrendErrRel, MKSFMTD(0, 4, 0)).
											Tab().Cat(r_dli.MainTrendErrRel, MKSFMTD(0, 4, 0)). // @v10.6.12
											Tab().Cat(r_dli.LocalDeviation, MKSFMTD(0, 4, 0)). // @v10.7.1
											Tab().Cat(r_dli.LocalDeviation2, MKSFMTD(0, 4, 0)). // @v10.7.1
											Tab().Cat(temp_buf);
										f_out.WriteLine(msg_buf.CR());
									}
									if(after_till_stake_count) {
										msg_buf.Z().CatCharN('-', 8).Space().Cat("result-after-till-date").Space().
											CatEq("StakeCount", after_till_stake_count).Space().CatEq("WinCount", after_till_win_count).Space().CatEq("LossCount", after_till_loss_count);
										msg_buf.Space().CatEq("WinCountRate", fdivui(after_till_win_count, after_till_stake_count), MKSFMTD(0, 5, 0));
										f_out.WriteLine(msg_buf.CR());
									}
									f_out.Flush();
									ok = 1;
								}
							}
						}
						PROFILE_END
					}
				}
			}
		}
	}
	{
		THROW(result_collection.MakeIndex());
		{
			PPIDArray result_ts_list;
			SString out_file_name;
			SString symb;
			PPTimeSeries ts_rec;
			(temp_buf = "tsscsim").CatChar('-').Cat("collection").CatChar('-').Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, TIMF_HMS|TIMF_NODIV);
			PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
			SFile f_out(out_file_name, SFile::mWrite);

			result_collection.GetTsList(result_ts_list);
			msg_buf.Z();
			for(uint tsidx = 0; tsidx < result_ts_list.getCount(); tsidx++) {
				const PPID result_ts_id = result_ts_list.get(tsidx);
				if(TsObj.Search(result_ts_id, &ts_rec) > 0) {
					msg_buf.Cat(ts_rec.ID).Space().Cat(ts_rec.Symb).CR();
				}
			}
			f_out.WriteLine(msg_buf.CR());
			uint   after_till_stake_count = 0;
			uint   after_till_win_count = 0;
			uint   after_till_loss_count = 0;
			LDATE  prev_date = ZERODATE;
			uint   win_per_day = 0;
			uint   lose_per_day = 0;
			uint   stake_per_day = 0;
			for(uint rcidx = 0; rcidx < result_collection.GetIndexCount(); rcidx++) {
				uint   dlidx = 0;
				const  TryStrategyContainerResult * p_ri = result_collection.GetEntryByIndex(rcidx, &dlidx);
				if(p_ri) {
					PPObjTimeSeries::StrategyResultValueEx & r_dli = p_ri->DetailsList.at(dlidx);
					const LDATE till_date = p_ri->Sc.GetUseDataForStrategiesTill();
					if(checkdate(till_date) && r_dli.TmR.Start.d > till_date) {
						after_till_stake_count++;
						if(r_dli.Result > 0.0)
							after_till_win_count++;
						else
							after_till_loss_count++;
					}
					{
						if(prev_date && prev_date != r_dli.TmR.Start.d) {
							msg_buf.Z().Cat("--------").
							Tab().Cat(prev_date, DATF_ISO8601|DATF_CENTURY).
							Tab().Cat(stake_per_day).
							Tab().Cat(win_per_day).
							Tab().Cat(lose_per_day);
							f_out.WriteLine(msg_buf.CR());
							win_per_day = 0;
							lose_per_day = 0;
							stake_per_day = 0;
						}
						prev_date = r_dli.TmR.Start.d;
						stake_per_day++;
						if(r_dli.Result > 0.0)
							win_per_day++;
						else if(r_dli.Result < 0.0)
							lose_per_day++;
					}
					PPObjTimeSeries::StrategyToString(p_ri->Sc.at(r_dli.StrategyIdx), &r_dli.OptFactor, &r_dli.OptFactor2, temp_buf);
					symb.Z();
					if(TsObj.Search(p_ri->TsID, &ts_rec) > 0)
						symb = ts_rec.Symb;
					if(symb.Empty())
						ideqvalstr(p_ri->TsID, symb);
					msg_buf.Z().
						Cat(symb).
						Tab().Cat(r_dli.TmR.Start, DATF_ISO8601|DATF_CENTURY, 0).
						Tab().Cat(r_dli.TmR.Finish, DATF_ISO8601|DATF_CENTURY, 0).
						Tab().Cat(r_dli.Result, MKSFMTD(0, 5, NMBF_FORCEPOS)).
						Tab().Cat(r_dli.TrendErrRel, MKSFMTD(0, 4, 0)).
						Tab().Cat(r_dli.MainTrendErrRel, MKSFMTD(0, 4, 0)). // @v10.6.12
						Tab().Cat(r_dli.LocalDeviation, MKSFMTD(0, 4, 0)). // @v10.7.1
						Tab().Cat(r_dli.LocalDeviation2, MKSFMTD(0, 4, 0)). // @v10.7.1
						Tab().Cat(temp_buf);
					f_out.WriteLine(msg_buf.CR());
				}
			}
			if(prev_date) {
				msg_buf.Z().Cat("--------").
				Tab().Cat(prev_date, DATF_ISO8601|DATF_CENTURY).
				Tab().Cat(stake_per_day).
				Tab().Cat(win_per_day).
				Tab().Cat(lose_per_day);
				f_out.WriteLine(msg_buf.CR());
				win_per_day = 0;
				lose_per_day = 0;
				stake_per_day = 0;
			}
			if(after_till_stake_count) {
				msg_buf.Z().CatCharN('-', 8).Space().Cat("result-after-till-date").Space().
					CatEq("StakeCount", after_till_stake_count).Space().CatEq("WinCount", after_till_win_count).Space().CatEq("LossCount", after_till_loss_count);
				msg_buf.Space().CatEq("WinCountRate", fdivui(after_till_win_count, after_till_stake_count), MKSFMTD(0, 5, 0));
				f_out.WriteLine(msg_buf.CR());
			}
		}
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

struct TimeSeriesTestParam : public PPExtStrContainer {
	SLAPI  TimeSeriesTestParam() : PPExtStrContainer(), Action(acnNone)
	{
	}
	enum {
		extssParam = 1,
		extssInfo  = 2,
	};
	enum {
		acnNone              = 0,
		acnAnalyzeRegression = 1,
		acnTestStrategies    = 2,
		acnEvalOptMaxDuck    = 3 
	};
	long   Action;
	ObjIdListFilt TsList; // @anchor
};

int SLAPI EditTimeSeriesTestParam(TimeSeriesTestParam * pData)
{
	class TimeSeriesTestParamDialog : public TDialog {
		DECL_DIALOG_DATA(TimeSeriesTestParam);
	public:
		TimeSeriesTestParamDialog() : TDialog(DLG_TIMSERTEST)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_TIMSERTEST_TS, PPOBJ_TIMESERIES, Data.TsList.GetSingle(), 0, 0);
			AddClusterAssocDef(CTL_TIMSERTEST_WHAT, 0, Data.acnAnalyzeRegression);
			AddClusterAssoc(CTL_TIMSERTEST_WHAT, 1, Data.acnTestStrategies);
			AddClusterAssoc(CTL_TIMSERTEST_WHAT, 2, Data.acnEvalOptMaxDuck);
			SetClusterData(CTL_TIMSERTEST_WHAT, Data.Action);
			Data.GetExtStrData(Data.extssParam, temp_buf);
			setCtrlString(CTL_TIMSERTEST_PARAM, temp_buf);
			Data.GetExtStrData(Data.extssInfo, temp_buf);
			setCtrlString(CTL_TIMSERTEST_INFO, temp_buf);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			SString temp_buf;
			//getCtrlData(sel = CTLSEL_TIMSERTEST_TS, &Data.TsID);
			PPID   id = getCtrlLong(sel = CTLSEL_TIMSERTEST_TS);
			Data.TsList.Add(id);
			THROW_PP(Data.TsList.GetCount(), PPERR_TIMSERNEEDED);
			GetClusterData(CTL_TIMSERTEST_WHAT, &Data.Action);
			getCtrlString(CTL_TIMSERTEST_PARAM, temp_buf);
			Data.PutExtStrData(Data.extssParam, temp_buf);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmTsList)) {
				PPID   id = 0;
				PPIDArray list;
				Data.TsList.Get(list);
				getCtrlData(CTLSEL_TIMSERTEST_TS, &id);
				if(id)
					list.addUnique(id);
				ListToListData data(PPOBJ_TIMESERIES, 0, &list);
				data.TitleStrID = PPTXT_SELTIMSERLIST;
				if(ListToListDialog(&data) > 0) {
					Data.TsList.Set(&list);
					Setup();
				}
				clearEvent(event);
			}
		}
		void   Setup()
		{
			PPID   id = Data.TsList.GetSingle();
			PPID   prev_id = getCtrlLong(CTLSEL_TIMSERTEST_TS);
			if(id != prev_id)
				setCtrlData(CTLSEL_TIMSERTEST_TS, &id);
			if(Data.TsList.GetCount() > 1)
				SetComboBoxListText(this, CTLSEL_TIMSERTEST_TS);
			disableCtrl(CTLSEL_TIMSERTEST_TS, BIN(Data.TsList.GetCount() > 1));
		}
	};
	DIALOG_PROC_BODY(TimeSeriesTestParamDialog, pData);
}

//static
int SLAPI PPObjTimeSeries::TryStrategies(PPID id)
{
	int    ok = -1;
	TimeSeriesTestParam param;
	param.TsList.Add(id);
	if(EditTimeSeriesTestParam(&param) > 0) {
		if(param.Action == TimeSeriesTestParam::acnTestStrategies) {
			PrcssrTsStrategyAnalyze prc;
			if(param.TsList.GetCount()) {
				THROW(prc.TryStrategyContainers(param.TsList.Get()));
			}
		}
		else if(param.Action == TimeSeriesTestParam::acnAnalyzeRegression) {
			PrcssrTsStrategyAnalyze prc;
			for(uint i = 0; i < param.TsList.GetCount(); i++) {
				const PPID ts_id = param.TsList.Get(i);
				THROW(prc.AnalyzeRegression(ts_id));
			}
		}
		else if(param.Action == TimeSeriesTestParam::acnEvalOptMaxDuck) {
			for(uint i = 0; i < param.TsList.GetCount(); i++) {
				const PPID ts_id = param.TsList.Get(i);
				THROW(PPObjTimeSeries::EvaluateOptimalMaxDuck(ts_id));
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

//static 
int SLAPI PPObjTimeSeries::EvaluateOptimalMaxDuck(PPID id)
{
	int    ok = -1;
	int    do_update_packet = 0;
	PrcssrTsStrategyAnalyze prc;
	PPTimeSeriesPacket ts_pack;
	PPObjTimeSeries ts_obj;
	uint    result_long = 0;
	uint    result_short = 0;
	if(ts_obj.GetPacket(id, &ts_pack) > 0) {
		STimeSeries ts;
		const int gtsr = ts_obj.GetTimeSeries(id, ts);
		if(gtsr > 0) {
			const uint tsc = ts.GetCount();
			STimeSeries::Stat st(0);
			uint   vec_idx = 0;
			THROW_SL(ts.GetValueVecIndex("close", &vec_idx));
			ts.Analyze("close", st);
			if(ts_pack.Rec.SpikeQuant_t <= 0.0) {
				ts_pack.Rec.SpikeQuant_t = st.DeltaAvg / 2.0;
				do_update_packet = 1;
			}
			{
				DateTimeArray ts_tm_list;
				RealArray ts_val_list;
				uint    flags = 0;
				THROW_SL(ts.GetRealArray(vec_idx, 0, tsc, ts_val_list));
				THROW_SL(ts.GetTimeArray(0, tsc, ts_tm_list));
				assert(ts_tm_list.getCount() == tsc);
				assert(ts_val_list.getCount() == tsc);
				THROW(prc.FindOptimalMaxDuck(ts_pack, ts_tm_list, ts_val_list, PrcssrTsStrategyAnalyze::fomdfEntireRange, &result_long));
				THROW(prc.FindOptimalMaxDuck(ts_pack, ts_tm_list, ts_val_list, PrcssrTsStrategyAnalyze::fomdfEntireRange|PrcssrTsStrategyAnalyze::fomdfShort, &result_short));
				if(result_long != ts_pack.Rec.OptMaxDuck) {
					ts_pack.Rec.OptMaxDuck = result_long;
					do_update_packet = 1;
				}
				if(result_short != ts_pack.Rec.OptMaxDuck_S) {
					ts_pack.Rec.OptMaxDuck_S = result_short;
					do_update_packet = 1;
				}
			}
			if(0/*do_update_packet*/) {
				PPID   temp_id = id;
				THROW(ts_obj.PutPacket(&temp_id, &ts_pack, 1));
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
TimSerDetailViewItem::TimSerDetailViewItem() : ItemIdx(0)
{
}

IMPLEMENT_PPFILT_FACTORY(TimSerDetail); SLAPI TimSerDetailFilt::TimSerDetailFilt() : PPBaseFilt(PPFILT_TIMSERDETAIL, 0, 0)
{
	SetFlatChunk(offsetof(TimSerDetailFilt, ReserveStart),
		offsetof(TimSerDetailFilt, Reserve)-offsetof(TimSerDetailFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

SLAPI PPViewTimSerDetail::PPViewTimSerDetail() : PPView(0, &Filt, PPVIEW_TIMSERDETAIL), P_DsList(0)
{
	ImplementFlags |= (implBrowseArray);
}

SLAPI PPViewTimSerDetail::~PPViewTimSerDetail()
{
	ZDELETE(P_DsList);
}

int SLAPI PPViewTimSerDetail::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	Ts.Z();
	CALLPTRMEMB(P_DsList, clear());
	if(Filt.TsID) {
		Obj.GetTimeSeries(Filt.TsID, Ts);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewTimSerDetail::MakeList(PPViewBrowser * pBrw)
{
	int    ok = 1;
	if(P_DsList)
		P_DsList->clear();
	else {
		THROW_SL(P_DsList = new SArray(sizeof(uint)));
	}
	for(uint i = 0; i < Ts.GetCount(); i++) {
		uint idx = i+1;
		THROW_SL(P_DsList->insert(&idx));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewTimSerDetail::ViewGraph(const PPViewBrowser * pBrw)
{
	int    ok = 1;
	PPTimeSeries ts_rec;
	const  uint tsc = Ts.GetCount();
	if(Obj.Search(Filt.TsID, &ts_rec) > 0 && tsc) {
		SString temp_buf;
		uint vec_idx = 0;
		if(Ts.GetValueVecIndex("close", &vec_idx)) {
			Generator_GnuPlot plot(0);
			Generator_GnuPlot::PlotParam param;
			plot.Preamble();
			PPGpPlotItem item(plot.GetDataFileName(), ts_rec.Symb, PPGpPlotItem::sLines);
			item.Style.SetLine(GetColorRef(SClrBlack), 1);
			item.AddDataIndex(1);
			item.AddDataIndex(3);
			plot.AddPlotItem(item);
			plot.SetDateTimeFormat(Generator_GnuPlot::axX, 1);
			plot.SetGrid();
			plot.Plot(&param);
			plot.StartData(1);
			const uint dot_count = MIN(tsc, 10000);
			for(uint i = tsc-dot_count; i < tsc; i++) {
				SUniTime ut;
				LDATETIME dtm;
				double value = 0.0;
				Ts.GetTime(i, &ut);
				ut.Get(dtm);
				Ts.GetValue(i, vec_idx, &value);
				plot.PutData(dtm);        // #1 #2
				plot.PutData(value);      // #3
				plot.PutEOR();
			}
			plot.PutEndOfData();
			ok = plot.Run();
		}
	}
	return ok;
}

int SLAPI PPViewTimSerDetail::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 1;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		const  uint idx = *static_cast<const uint *>(pBlk->P_SrcData) - 1;
		int    r = 0;
		if(pBlk->ColumnN == 0)
			pBlk->Set(static_cast<int32>(idx));
		else {
			if(pBlk->ColumnN == 1) {
				SUniTime ut;
				LDATETIME dtm;
				Ts.GetTime(idx, &ut);
				ut.Get(dtm);
				SString & r_temp_buf = SLS.AcquireRvlStr();
				r_temp_buf.Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0);
				pBlk->Set(r_temp_buf);
			}
			else if(pBlk->ColumnN == 2) {
				int32 diffsec = 0;
				if(idx > 0) {
					SUniTime ut;
					LDATETIME dtm;
					LDATETIME dtm2;
					Ts.GetTime(idx, &ut);
					ut.Get(dtm);
					Ts.GetTime(idx-1, &ut);
					ut.Get(dtm2);
					diffsec = diffdatetimesec(dtm, dtm2);
				}
				pBlk->Set(diffsec);
			}
			else if(pBlk->ColumnN > 2 && pBlk->ColumnN < static_cast<int>(Ts.GetValueVecCount()+3)) {
				double value = 0.0;
				Ts.GetValue(idx, pBlk->ColumnN-3, &value);
				pBlk->Set(value);
			}
		}
	}
	return ok;
}

//static
int FASTCALL PPViewTimSerDetail::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewTimSerDetail * p_v = static_cast<PPViewTimSerDetail *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

void SLAPI PPViewTimSerDetail::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		STimeSeries d;
		const uint tsvc = Ts.GetValueVecCount();
		if(tsvc) {
			SString symb_buf;
			uint   fld_no = 2;
			for(uint i = 0; i < tsvc; i++) {
				int    fxprec = 0;
				TYPEID typ = 0;
				if(Ts.GetValueVecParam(i, &typ, &symb_buf, &fxprec, 0)) {
					const int  prec = (GETSTYPE(typ) == S_FLOAT) ? 5 : fxprec;
					long fmt = MKSFMTD(0, prec, 0);
					fld_no++;
					pBrw->InsColumn(-1, symb_buf, fld_no, T_DOUBLE, fmt, BCO_USERPROC);
				}
			}
		}
		pBrw->SetDefUserProc(PPViewTimSerDetail::GetDataForBrowser, this);
		//pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
}

int SLAPI PPViewTimSerDetail::OnExecBrowser(PPViewBrowser * pBrw)
{
	return -1;
}

int SLAPI PPViewTimSerDetail::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	const  PPID preserve_id = id;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_EXPORT:
				ok = -1;
				//Obj.Export(id);
				break;
			case PPVCMD_GRAPH:
				ok = ViewGraph(pBrw);
				break;
			case PPVCMD_REFRESH:
				ok = 1;
				break;
		}
	}
	if(ok > 0) {
		MakeList(pBrw);
		if(pBrw) {
			AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
			if(p_def) {
				SArray * p_array = new SArray(*P_DsList);
				p_def->setArray(p_array, 0, 1);
				pBrw->setRange(p_array->getCount());
				uint   temp_pos = 0;
				long   update_pos = -1;
				if(preserve_id > 0 && P_DsList->lsearch(&preserve_id, &temp_pos, CMPF_LONG))
					update_pos = temp_pos;
				if(update_pos >= 0)
					pBrw->go(update_pos);
				else if(update_pos == MAXLONG)
					pBrw->go(p_array->getCount()-1);
			}
			pBrw->Update();
		}
	}
	return ok;
}

SArray * SLAPI PPViewTimSerDetail::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_TIMSERDETAIL;
	SArray * p_array = 0;
	THROW(MakeList(0));
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int SLAPI PPViewTimSerDetail::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	return ok;
}

int SLAPI PPViewTimSerDetail::InitIteration()
{
	return -1;
}

int FASTCALL PPViewTimSerDetail::NextIteration(TimSerDetailViewItem * pItem)
{
	return -1;
}

