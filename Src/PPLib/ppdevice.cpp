// PPDEVICE.CPP
// Copyright (c) A.Sobolev 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
//
#include <pp.h>
#pragma hdrstop
//
//
//
SLAPI PPGenericDevice::PPGenericDevice()
{
	THISZERO();
	Tag = PPOBJ_GENERICDEVICE;
}

SLAPI PPGenericDevicePacket::PPGenericDevicePacket()
{
}

int SLAPI PPGenericDevicePacket::GetExtStrData(int fldID, SString & rBuf) const
{
	return PPGetExtStrData(fldID, ExtString, rBuf);
}

int SLAPI PPGenericDevicePacket::PutExtStrData(int fldID, const char * pBuf)
{
	return PPPutExtStrData(fldID, ExtString, pBuf);
}

SLAPI PPObjGenericDevice::PPObjGenericDevice(void * extraPtr) : PPObjReference(PPOBJ_GENERICDEVICE, extraPtr)
{
}

StrAssocArray * SLAPI PPObjGenericDevice::MakeStrAssocList(void * extraPtr)
{
	const long dvc_cls = (long)extraPtr;
	StrAssocArray * p_list = new StrAssocArray();
	THROW_MEM(p_list);
	{
		PPGenericDevice rec;
		for(SEnum en = ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
			if(!dvc_cls || rec.DeviceClass == dvc_cls) {
				if(*strip(rec.Name) == 0)
					ideqvalstr(rec.ID, rec.Name, sizeof(rec.Name));
				THROW_SL(p_list->Add(rec.ID, 0, rec.Name));
			}
		}
	}
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int SLAPI PPObjGenericDevice::PutPacket(PPID * pID, PPGenericDevicePacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		int    action = 0;
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			PPGenericDevice org_rec;
			THROW(ref->GetItem(Obj, *pID, &org_rec) > 0);
			if(pPack) {
				THROW(ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
				THROW(ref->PutPropVlrString(Obj, *pID, GENDVCPRP_EXTSTRDATA, pPack->ExtString));
				action = PPACN_OBJUPD;
			}
			else {
				THROW(ref->RemoveItem(Obj, *pID, 0));
				action = PPACN_OBJRMV;
			}
			Dirty(*pID);
		}
		else {
			THROW(ref->AddItem(Obj, pID, &pPack->Rec, 0));
			THROW(ref->PutPropVlrString(Obj, *pID, GENDVCPRP_EXTSTRDATA, pPack->ExtString));
			action = PPACN_OBJADD;
		}
		if(action)
			DS.LogAction(action, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGenericDevice::GetPacket(PPID id, PPGenericDevicePacket * pPack)
{
	int    ok = 0;
	if(pPack) {
		ok = Search(id, &pPack->Rec);
		if(ok > 0) {
			THROW(ref->GetPropVlrString(Obj, id, GENDVCPRP_EXTSTRDATA, pPack->ExtString));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGenericDevice::Edit(PPID * pID, void * extraPtr)
{
	class GenDvcDialog : public TDialog {
	public:
		GenDvcDialog() : TDialog(DLG_ADEVICE)
		{
		}
		int    setDTS(const PPGenericDevicePacket * pData)
		{
			Data = *pData;
			SString temp_buf;
			setCtrlLong(CTL_ADEVICE_ID, Data.Rec.ID);
			setCtrlString(CTL_ADEVICE_NAME, Data.Rec.Name);
			setCtrlString(CTL_ADEVICE_SYMB, Data.Rec.Symb);
			SetupStringCombo(this, CTLSEL_ADEVICE_CLS, PPTXT_ABSTRACTDEVICETYPENAMES, Data.Rec.DeviceClass);
			AddClusterAssoc(CTL_ADEVICE_FLAGS, 0, PPCommObjEntry::fPassive);
			SetClusterData(CTL_ADEVICE_FLAGS, Data.Rec.Flags);

			Data.GetExtStrData(GENDVCEXSTR_ENTRY, temp_buf);
			setCtrlString(CTL_ADEVICE_DVCSYMB, temp_buf);
			Data.GetExtStrData(GENDVCEXSTR_INITSTR, temp_buf); // @v7.9.6
			setCtrlString(CTL_ADEVICE_INITSTR, temp_buf);      // @v7.9.6

			return 1;
		}
		int    getDTS(PPGenericDevicePacket * pData)
		{
			int    ok = 1;
			SString temp_buf;
			getCtrlData(CTL_ADEVICE_NAME, Data.Rec.Name);
			getCtrlData(CTL_ADEVICE_SYMB, Data.Rec.Symb);
			getCtrlData(CTLSEL_ADEVICE_CLS, &Data.Rec.DeviceClass);
			GetClusterData(CTL_ADEVICE_FLAGS, &Data.Rec.Flags);
			getCtrlString(CTL_ADEVICE_DVCSYMB, temp_buf);
			Data.PutExtStrData(GENDVCEXSTR_ENTRY, temp_buf);
			getCtrlString(CTL_ADEVICE_INITSTR, temp_buf);       // @v7.9.6
			Data.PutExtStrData(GENDVCEXSTR_INITSTR, temp_buf);  // @v7.9.6
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	public:
		PPGenericDevicePacket Data;
	};
	int    ok = -1, is_new = 0;
	GenDvcDialog * dlg = 0;
	PPGenericDevicePacket pack;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	{
		THROW(CheckDialogPtr(&(dlg = new GenDvcDialog)));
		dlg->setDTS(&pack);
		while(ok <= 0 && ExecView(dlg) == cmOK) {
			if(dlg->getDTS(&pack)) {
				if(!CheckName(*pID, pack.Rec.Name, 0))
					dlg->selectCtrl(CTL_ADEVICE_NAME);
				else if(!PutPacket(pID, &pack, 1))
					ok = PPErrorZ();
				else
					ok = cmOK;
			}
			else
				ok = PPErrorZ();
		}
	}
	CATCHZOKPPERR
	return ok;
}
//
// @ModuleDef(PPAbstractDevice)
//
PPAbstractDevice::ConnectionParam::ConnectionParam() : DeviceNo(0), NotOwned(0), ReleCount(0), P_Conn(0), GoodsID(0)
{
	MEMSZERO(Cpp);
}

int PPAbstractDevice::ConnectionParam::IsEqualAddr(const ConnectionParam & rParam) const
{
	return BIN(rParam.Address.Cmp(Address, 0) == 0);
}
//
//
//
PPAbstractDevice::IdentBlock::IdentBlock() : P_Lib(0), IsLibOwner(0), Func(0)
{
}

PPAbstractDevice::IdentBlock::~IdentBlock()
{
	Release();
}

int PPAbstractDevice::IdentBlock::operator !() const
{
	return !Func;
}

void PPAbstractDevice::IdentBlock::Release()
{
	Func = 0;
	if(IsLibOwner)
		delete P_Lib;
	P_Lib = 0;
}

int PPAbstractDevice::IdentBlock::Set(SDynLibrary * pLib, int setOwnership)
{
	int    ok = -1;
	Release();
	if(pLib) {
		P_Lib = pLib;
		IsLibOwner = BIN(setOwnership);
		Func = (ProcDevDll)P_Lib->GetProcAddr("RunCommand");
		if(Func) {
			ok = 1;
		}
		else {
			PPSetError(PPERR_DVC_DLLHASNTENTRYPOINT);
			ok = 0;
		}
	}
	return ok;
}
//
//
//
//static
int SLAPI PPAbstractDevice::GetDrvIniSectByDvcClass(int dvcClass, int * pReserveTypesStrId, SString * pSectName)
{
	int    sect_id = 0;
	int    rts_id = 0;
	switch(dvcClass) {
		case DVCCLS_TOGGLE:
			sect_id = PPINISECT_DRV_TOGGLE;
			break;
		case DVCCLS_SCALES:
			sect_id = PPINISECT_DRV_SCALE;
			rts_id = PPTXT_SCLT;
			break;
		case DVCCLS_SYNCPOS:
			sect_id = PPINISECT_DRV_SYNCPOS;
			rts_id = PPTXT_CMT;
			break;
		case DVCCLS_DISPLAY:
			sect_id = PPINISECT_DRV_CUSTDISP;
			rts_id = PPTXT_CUSTDISP;
			break;
		case DVCCLS_BNKTERM:
			sect_id = PPINISECT_DRV_BNKTERM;
			break;
		case DVCCLS_READER:
			sect_id = PPINISECT_DRV_READER;
			break;
	}
	if(pSectName) {
		(*pSectName) = 0;
		if(sect_id)
			PPIniFile::GetSectSymb(sect_id, *pSectName);
	}
	ASSIGN_PTR(pReserveTypesStrId, rts_id);
	return sect_id;
}

PPAbstractDevice::PPAbstractDevice(const char * pDvcName) : RetBuf(4096), State(0)
{
	GetCapability(&PCpb);
	if(pDvcName)
		DvcName = pDvcName;
	else {
		DvcName.Space();
		DvcName.Z(); // Гарантия не пустого буфера
	}
}

// @vmiller {
int SLAPI GetStrFromDrvIni(int iniSectID, long devTypeId, int numOfOldDev, SString & rBuf)
{
	int    ok = 1;
	uint   pos = 0, sync_count = 0, localDevId = 0;
	SString path;
	StringSet set;
	PPGetFilePath(PPPATH_BIN, "ppdrv.ini", path);
	PPIniFile ini_file(path);
	localDevId = devTypeId - numOfOldDev;
	if(oneof2(iniSectID, PPINISECT_DRV_SYNCPOS, PPINISECT_DRV_ASYNCPOS)) {
		THROW(ini_file.GetEntryList(PPINISECT_DRV_SYNCPOS, &set, 1));
		THROW(ini_file.GetEntryList(PPINISECT_DRV_ASYNCPOS, &set, 1));
	}
	else {
		THROW(ini_file.GetEntryList(iniSectID, &set, 1));
	}
	if(localDevId <= set.getCount()) {
		for(uint i = 1; i <= set.getCount(); i++) {
			set.get(&pos, rBuf);
			if(localDevId == i)
				break;
		}
	}
	else
		ok = -1;
	CATCHZOK;
	return ok;
}

//static
int SLAPI PPAbstractDevice::ParseRegEntry(const char * pLine, SString & rSymbol, SString & rDrvName, SString & rDrvPath, int * pDrvImpl)
{
	// Shtrih-DPD201=DRV:Shtrih-DPD201 (Петроглиф),dll,ppdrv-cd-Shtrih-DPD201.dll

	rSymbol.Z();
	rDrvName.Z();
	rDrvPath.Z();

	int    ok = 0, r = 2;
	int    drv_impl = 0;
	SString temp_buf;
	const char * p_eq = strchr(pLine, '=');

	size_t eq_pos = 0;
	temp_buf = pLine;
	if(temp_buf.StrChr('=', &eq_pos)) {
		rSymbol = temp_buf.Trim(eq_pos).Strip();
		r = 1;
		eq_pos++; // Переступаем за '='
	}
	StringSet ss(',', pLine+eq_pos);
	uint   pos = 0;
	if(ss.get(&pos, temp_buf)) {
		rDrvName = temp_buf.Strip();
		if(ss.get(&pos, temp_buf)) {
			if(temp_buf.Strip().CmpNC("dll") == 0) {
				drv_impl = DVCDRVIMPL_DLL;
			}
			if(ss.get(&pos, temp_buf)) {
				rDrvPath = temp_buf.Strip();
				ok = r;
			}
		}
	}
	if(!ok) {
		PPSetError(PPERR_DVC_INVREGENTRY, pLine);
	}
	ASSIGN_PTR(pDrvImpl, drv_impl);
	return ok;
}

int PPAbstractDevice::GetDllName(int dvcClass, long devTypeId, SString & rDllPath)
{
	rDllPath.Z();

	int    ok = 1, idx = 0;
	SString line_buf = ""; // А то ошибка может быть // @vmiller
	int    rts_id = 0;
	int    sect_id = GetDrvIniSectByDvcClass(dvcClass, &rts_id, 0);
	if(rts_id) {
		while(PPGetSubStr(rts_id, idx, line_buf) > 0)
			idx++;
	}
	THROW(GetStrFromDrvIni(sect_id, devTypeId, idx, line_buf));
	{
		SString symbol, drv_name, drv_path;
		int    drv_impl = 0;
		SPathStruc ps;
		THROW(ParseRegEntry(line_buf, symbol, drv_name, drv_path, &drv_impl));
		ps.Split(drv_path);
		if(ps.Drv.Empty() && ps.Dir.Empty()) {
			PPGetPath(PPPATH_BIN, rDllPath);
			rDllPath.SetLastSlash().Cat("DRV").SetLastSlash().Cat(drv_path);
		}
		else
			rDllPath = drv_path;
	}
	CATCHZOK;
	return ok;
}
// }@vmiller

int PPAbstractDevice::GetDllName(int dvcClass, const char * pName, SString & rDllPath)
{
	rDllPath.Z();

	int    ok = 1;
	SString line_buf;
	int    rts_id = 0;
	int    sect_id = GetDrvIniSectByDvcClass(dvcClass, &rts_id, 0);
	THROW(sect_id);
	{
		SString symbol, drv_name, drv_path;
		int    drv_impl = 0;
		SPathStruc ps;

		PPGetFilePath(PPPATH_BIN, "ppdrv.ini", line_buf);
		PPIniFile ini_file(line_buf);
		THROW_PP_S(ini_file.Get(sect_id, pName, line_buf) > 0, PPERR_DVC_UNDEFDRVSYMB, pName);

		THROW(ParseRegEntry(line_buf, symbol, drv_name, drv_path, &drv_impl));
		ps.Split(drv_path);
		if(ps.Drv.Empty() && ps.Dir.Empty()) {
			PPGetPath(PPPATH_BIN, rDllPath);
			rDllPath.SetLastSlash().Cat("DRV").SetLastSlash().Cat(drv_path);
		}
		else
			rDllPath = drv_path;
	}
	CATCHZOK;
	return ok;
}

int PPAbstractDevice::IdentifyDevice(int dvcClass, const char * pName)
{
	int    ok = 1;
	THROW_PP(!isempty(pName), PPERR_UNDEFADVCDESCR);
	{
		SString dll_symb, dll_file_name;
		(dll_symb = "PPDRV").CatChar('&').Cat(dvcClass).CatChar('&').Cat(pName);
		SDynLibrary * p_dll = 0;
		ENTER_CRITICAL_SECTION
			long   symbol_id = SLS.GetGlobalSymbol(dll_symb, -1, 0);
			THROW_SL(symbol_id);
			if(symbol_id < 0) {
				TSClassWrapper <SDynLibrary> cls;
				THROW_SL(symbol_id = SLS.CreateGlobalObject(cls));
				THROW_SL(p_dll = (SDynLibrary *)SLS.GetGlobalObject(symbol_id));
				if(GetDllName(dvcClass, pName, dll_file_name)) {
					if(!p_dll->Load(dll_file_name)) {
						SLS.DestroyGlobalObject(symbol_id);
						CALLEXCEPT_PP(PPERR_SLIB);
					}
					else {
						long s = SLS.GetGlobalSymbol(dll_symb, symbol_id, 0);
						assert(symbol_id == s);
					}
				}
				else {
					SLS.DestroyGlobalObject(symbol_id);
					CALLEXCEPT();
				}
			}
			else if(symbol_id > 0) {
				THROW_SL(p_dll = (SDynLibrary *)SLS.GetGlobalObject(symbol_id));
				THROW_SL(p_dll->IsValid());
			}
			THROW(Ib.Set(p_dll, 0));
		LEAVE_CRITICAL_SECTION
	}
	CATCHZOK
	return ok;
}

// @vmiller
int PPAbstractDevice::IdentifyDevice(const char * pSymb)
{
	int    ok = 1;
	SDynLibrary * p_lib = new SDynLibrary(pSymb);
	THROW_MEM(p_lib)
	THROW_SL(p_lib->IsValid());
	THROW(Ib.Set(p_lib, 1));
	CATCHZOK
	return ok;
}

PPAbstractDevice::~PPAbstractDevice()
{
}

int PPAbstractDevice::GetCapability(Capability * pCpb)
{
	if(pCpb) {
		pCpb->Cls = DVCCLS_UNKN;
		pCpb->Flags = 0;
	}
	return 1;
}

int PPAbstractDevice::OpenConnection(const ConnectionParam &)
{
	return -1;
}

int PPAbstractDevice::CloseConnection()
{
	return -1;
}

int PPAbstractDevice::RunCmd(int cmdId, const StrAssocArray & rIn, StrAssocArray & rOut)
{
	return -1;
}
//
// Преобразует форматированную строку в массив
// @vmiller
//
int StrToArr(SString & s_arr, StrAssocArray & strArr)
{
	SString pair;
	if(s_arr.NotEmpty()) {
		StringSet sstr(';', s_arr);
		strArr.Clear();
		for(uint i = 0, j = 0; sstr.get(&i, pair) > 0; j++)
			strArr.Add(j, pair, 1);
	}
	return 1;
}

int PPAbstractDevice::Helper_RunCmd(SString & rCmd, SString & rArg, StrAssocArray & rOut)
{
	int    ok = 1, r = 0;
	SString s_arr;
	const   size_t buf_size_quant = 4096; // @v9.7.6 1024-->4096
	//STempBuffer output(buf_size_quant);
	//THROW_SL(output.IsValid());
	if(RetBuf.GetSize() < buf_size_quant)
		RetBuf.Alloc(buf_size_quant);
	THROW_SL(RetBuf.IsValid());
	//THROW_SL(Ib);
	if(!Ib) {
		SLS.SetOsError("RunCommand");
		THROW(0);
	}
	do {
		//memzero(output, output.GetSize());
		memzero(RetBuf, RetBuf.GetSize());
		{
			const char * p_cmd = rCmd.cptr();
			const char * p_arg = rArg.cptr();
			//r = Ib.Func(p_cmd, p_arg, output, output.GetSize());
			r = Ib.Func(p_cmd, p_arg, RetBuf, RetBuf.GetSize());
		}
		if(r == 0) {
			//if(output.IsValid()) {
			if(RetBuf.IsValid()) {
				//s_arr.Z().Cat(output);
				s_arr.Z().Cat(RetBuf);
				THROW(StrToArr(s_arr, rOut));
			}
		}
		else if(r == 1) {
			//THROW(rOut.Add(0, output, 1));
			THROW(rOut.Add(0, RetBuf, 1));
			ok = -1;
			r = 0;
		}
		else if(r == 2) {
			//size_t new_size = output.GetSize() + 1024;
			//THROW_SL(output.Alloc(new_size));
			size_t new_size = RetBuf.GetSize() + 1024;
			THROW_SL(RetBuf.Alloc(new_size));
		}
	} while(r > 0);
	CATCHZOK;
	return ok;
}

// @vmiller
int PPAbstractDevice::RunCmd__(int cmdID, const StrAssocArray & rIn, StrAssocArray & rOut)
{
	int    ok = 1;
	SString s_cmd, input, str;
	THROW(PPLoadString(PPSTR_ABDVCCMD, cmdID, s_cmd)); // @todo @err
	for(uint i = 0; i < rIn.getCount(); i++) {
		StrAssocArray::Item item = rIn.Get(i);
		THROW(PPLoadString(PPSTR_ABDVCCMD, item.Id, str)); // @todo @err
		if(input.NotEmpty())
			input.Semicol();
		input.CatEq(str, item.Txt);
	}
	//THROW(Helper_RunCmd(s_cmd, input, rOut)); // @vmiller comment
	ok = Helper_RunCmd(s_cmd, input, rOut);
	CATCHZOK;
	return ok;
}

int PPAbstractDevice::RunCmd(const char * pCmd, StrAssocArray & rOut)
{
	int    ok = 1;
	SString s_cmd, input;
	{
		(s_cmd = pCmd).Strip();
		size_t p = 0;
		if(s_cmd.StrChr(' ', &p)) {
			(input = s_cmd.cptr() + p + 1).Strip();
			s_cmd.Trim(p);
		}
	}
	THROW(Helper_RunCmd(s_cmd, input, rOut));
	CATCHZOK;
	return ok;
}

// static
int SLAPI PPAbstractDevice::CreateInstance(const char * pSymb, PPAbstractDevice ** ppDvc)
{
	int    ok = 0;
	PPAbstractDevice * p_dvc = 0;
	SString ffn;
	ffn.Cat("ADF").CatChar('_').Cat(pSymb);
	FN_PPDEVICE_FACTORY f = (FN_PPDEVICE_FACTORY)GetProcAddress(SLS.GetHInst(), ffn);
	if(f) {
		p_dvc = f();
		if(p_dvc)
			ok = 1;
	}
	else
		PPSetError(PPERR_PPDEVICEUNIMPL, pSymb);
	ASSIGN_PTR(ppDvc, p_dvc);
	return ok;
}

int PPAbstractDevice::GetConnParam(ConnectionParam *)
{
	return -1;
}

int PPAbstractDevice::GetSessionPrice(PPObjGoods * pGObj, double * pPrice)
{
	return -1;
}
//
//
//
class PPDevice_Leader : public PPAbstractDevice {
public:
	PPDevice_Leader();
	~PPDevice_Leader();

	virtual int GetCapability(Capability * pCpb);
	virtual int OpenConnection(const ConnectionParam &);
	virtual int CloseConnection();
	virtual int RunCmd(int innerId, const StrAssocArray & rIn, StrAssocArray & rOut);
	virtual int GetConnParam(ConnectionParam *);
	virtual int GetSessionPrice(PPObjGoods *, double *);

	int    CardCodeToString(const uint8 * pCardCode, SString & rBuf) const;
	int    StringToCardCode(const char * pStr, uint8 * pCardCode) const;
private:
	static const size_t DispLineSize;
	int Helper_RunCmd(int innerId, const StrAssocArray & rIn, StrAssocArray & rOut);

	uint8   CalcCheckCode(const uint8 * pBuf, size_t dataLen) const;
	ConnectionParam ConnP;
	SCommPort * P_Cp;
	MIME64  M64;
};

IMPLEMENT_PPDEVICE_FACTORY(PPDevice_Leader);

//static
const size_t PPDevice_Leader::DispLineSize = 16;

PPDevice_Leader::PPDevice_Leader() : PPAbstractDevice("Leader Controller"), P_Cp(0)
{
}

PPDevice_Leader::~PPDevice_Leader()
{
	CloseConnection();
}

uint8 PPDevice_Leader::CalcCheckCode(const uint8 * pBuf, size_t dataLen) const
{
	uint8 c = 0;
	for(size_t i = 1; i < dataLen; i++) {
		c ^= pBuf[i];
	}
	return c;
}

int PPDevice_Leader::CardCodeToString(const uint8 * pCardCode, SString & rBuf) const
{
	rBuf.Z().CatLongZ((long)pCardCode[2], 3).CatChar(',').Cat(swapw(*(uint16 *)(pCardCode+3)));
	return 1;
}

int PPDevice_Leader::StringToCardCode(const char * pStr, uint8 * pCardCode) const
{
	return -1;
}

int PPDevice_Leader::GetCapability(Capability * pCpb)
{
	if(pCpb) {
		pCpb->Cls = DVCCLS_TOGGLE;
		pCpb->Flags = 0;
		pCpb->DispLineSize = DispLineSize;
		pCpb->DispLinesCount = 2;
	}
	return 1;
}

int PPDevice_Leader::OpenConnection(const ConnectionParam & rConnParam)
{
	int    ok = 1;
	if(!(State & stConnected)) {
		int    port_no = 0;
		CommPortTimeouts cpt;
		if(ConnP.NotOwned)
			P_Cp = 0;
		else
			ZDELETE(P_Cp);
		ConnP = rConnParam;
		if(!ConnP.NotOwned) {
			THROW(IsComDvcSymb(ConnP.Address, &port_no) == comdvcsCom && port_no > 0);
			port_no--;
			THROW(ConnP.DeviceNo > 0 && ConnP.DeviceNo <= 254);
			THROW_MEM(P_Cp = new SCommPort);
			P_Cp->SetParams(&ConnP.Cpp);
			P_Cp->SetReadCyclingParams(10, 10);

			cpt.Get_NumTries = 0;
			cpt.Get_Delay    = 20;
			cpt.Put_NumTries = 0;
			cpt.Put_Delay    = 0;
			cpt.W_Get_Delay  = 0;
			P_Cp->SetTimeouts(&cpt);
			THROW_PP_S(P_Cp->InitPort(port_no), PPERR_SLIB, ConnP.Address);
			State |= stConnected;
			ConnP.P_Conn = (long)P_Cp;
		}
		else {
			P_Cp = (SCommPort*)ConnP.P_Conn;
			THROW(P_Cp);
			State |= stConnected;
		}
	}
	else
		ok = -1;
	CATCH
		ZDELETE(P_Cp);
		State |= stError;
		ok = 0;
	ENDCATCH
	return ok;
}

int PPDevice_Leader::CloseConnection()
{
	int    ok = 1;
	if(State & stConnected) {
		State &= ~stConnected;
		if(!ConnP.NotOwned)
			ZDELETE(P_Cp);
		else
			P_Cp = 0;
	}
	else
		ok = -1;
	return ok;
}

// virtual
int PPDevice_Leader::RunCmd(int cmdId, const StrAssocArray & rIn, StrAssocArray & rOut)
{
	int ok = 0;
	if(State & stConnected) {
		uint16 put_tries = (ConnP.Cpt.Put_NumTries) ? ConnP.Cpt.Put_NumTries : 1;
		int    put_ok = 0;
		for(uint16 num_tries = 0; !ok && num_tries < put_tries; num_tries++)
			ok = Helper_RunCmd(cmdId, rIn, rOut);
	}
	return ok;
}

int PPDevice_Leader::Helper_RunCmd(int cmdId, const StrAssocArray & rIn, StrAssocArray & rOut)
{
	int    ok = 1;
	size_t i;
	SString temp_buf, head_buf, tail_buf;
	uint8  cs = 0;
	uint8  data_buf[512];
	size_t data_size = 0;
	rOut.Clear();
	if(State & stConnected && P_Cp) {
		uint16 get_num_tries = (ConnP.Cpt.Get_NumTries) ? ConnP.Cpt.Get_NumTries : 1, num_tries = 0;
		switch(cmdId) {
			case DVCCMD_PING:
				{
					data_size = 0;
					data_buf[data_size++] = 0xAA;
					data_buf[data_size++] = (uint8)ConnP.DeviceNo;
					data_buf[data_size++] = 0x01;
					cs = CalcCheckCode(data_buf, data_size);
					data_buf[data_size++] = cs;
					for(i = 0; i < data_size; i++) {
						THROW_SL(P_Cp->PutChr((int)data_buf[i]));
					}
					{
						data_size = 0;
						int    chr;
						for(num_tries = 0; !data_size && num_tries < get_num_tries; num_tries++)
							while(P_Cp->GetChr(&chr))
								data_buf[data_size++] = (uint8)chr;
						THROW_PP(data_size, PPERR_DVC_NOREPLY);
						cs = CalcCheckCode(data_buf, data_size-1);
						THROW_PP(cs == data_buf[data_size-1], PPERR_DVC_INVREPLY);
						THROW_PP(data_buf[0] == 0xAA, PPERR_DVC_INVREPLY);
						THROW_PP(data_buf[1] == (int8)ConnP.DeviceNo, PPERR_DVC_INVREPLY);
						if(data_buf[2] == 0x01) {
							temp_buf.Z();
							CardCodeToString(data_buf+3, temp_buf);
							rOut.Add(DVCCMDPAR_CARD, temp_buf);
						}
						else if(data_buf[2] == 0x02) {
						}
						else {
							THROW_PP(0, PPERR_DVC_INVREPLY);
						}
					}
				}
				break;
			case DVCCMD_SETTEXT:
				{
					if(rIn.Get(DVCCMDPAR_TEXT, temp_buf) > 0) {
						// Header
						data_size = 0;
						data_buf[data_size++] = 0xAA;
						data_buf[data_size++] = (uint8)ConnP.DeviceNo;
						data_buf[data_size++] = 0x06;
						// Line 1
						temp_buf.Wrap(DispLineSize, head_buf, tail_buf);
						for(i = 0; i < head_buf.Len(); i++)
							data_buf[data_size++] = head_buf.C(i);
						for(; i < DispLineSize; i++)
							data_buf[data_size++] = ' ';
						// Line 2
						temp_buf = tail_buf;
						temp_buf.Wrap(DispLineSize, head_buf, tail_buf);
						for(i = 0; i < head_buf.Len(); i++)
							data_buf[data_size++] = head_buf.C(i);
						for(; i < DispLineSize; i++)
							data_buf[data_size++] = ' ';
						//
						cs = CalcCheckCode(data_buf, data_size);
						data_buf[data_size++] = cs;
						for(i = 0; i < data_size; i++) {
							THROW_SL(P_Cp->PutChr((int)data_buf[i]));
						}
						{
							data_size = 0;
							int    chr;
							for(num_tries = 0; !data_size && num_tries < get_num_tries; num_tries++)
								while(P_Cp->GetChr(&chr)) {
									data_buf[data_size++] = (uint8)chr;
								}
							THROW_PP(data_size, PPERR_DVC_NOREPLY);
							cs = CalcCheckCode(data_buf, data_size-1);
							THROW_PP(cs == data_buf[data_size-1], PPERR_DVC_INVREPLY);
							THROW_PP(data_buf[0] == 0xAA, PPERR_DVC_INVREPLY);
							THROW_PP(data_buf[1] == (int8)ConnP.DeviceNo, PPERR_DVC_INVREPLY);
							THROW_PP(data_buf[2] == 0x02, PPERR_DVC_INVREPLY);
						}
					}
				}
				break;
			case DVCCMD_TOGGLE:
				{
					long   rele = 0;
					data_size = 0;
					data_buf[data_size++] = 0xAA;
					data_buf[data_size++] = (uint8)ConnP.DeviceNo;
					data_buf[data_size++] = 0x03;
					temp_buf.Z();
					rIn.Get(DVCCMDPAR_TEXT, temp_buf);
					// Line 1
					temp_buf.Wrap(DispLineSize, head_buf, tail_buf);
					for(i = 0; i < head_buf.Len(); i++)
						data_buf[data_size++] = head_buf.C(i);
					for(; i < DispLineSize; i++)
						data_buf[data_size++] = ' ';
					// Line 2
					temp_buf = tail_buf;
					temp_buf.Wrap(DispLineSize, head_buf, tail_buf);
					for(i = 0; i < head_buf.Len(); i++)
						data_buf[data_size++] = head_buf.C(i);
					for(; i < DispLineSize; i++)
						data_buf[data_size++] = ' ';
					//
					if(rIn.Get(DVCCMDPAR_COUNT, temp_buf)) {
						rele = temp_buf.ToLong();
						if(rele < 0 || rele > 255)
							rele = 0;
					}
					data_buf[data_size++] = (uint8)rele;
					cs = CalcCheckCode(data_buf, data_size);
					data_buf[data_size++] = cs;
					for(i = 0; i < data_size; i++) {
						THROW_SL(P_Cp->PutChr((int)data_buf[i]));
					}
					{
						data_size = 0;
						int    chr;
						for(num_tries = 0; !data_size && num_tries < get_num_tries; num_tries++)
							while(P_Cp->GetChr(&chr)) {
								data_buf[data_size++] = (uint8)chr;
							}
						THROW_PP(data_size, PPERR_DVC_NOREPLY);
						cs = CalcCheckCode(data_buf, data_size-1);
						THROW_PP(cs == data_buf[data_size-1], PPERR_DVC_INVREPLY);
						THROW_PP(data_buf[0] == 0xAA, PPERR_DVC_INVREPLY);
						THROW_PP(data_buf[1] == (int8)ConnP.DeviceNo, PPERR_DVC_INVREPLY);
						THROW_PP(data_buf[2] == 0x03, PPERR_DVC_INVREPLY);
					}
				}
				break;
			default:
				ok = -1;
				break;
		}
	}
	CATCHZOK
	return ok;
}

int PPDevice_Leader::GetConnParam(ConnectionParam * pParam)
{
	ASSIGN_PTR(pParam, ConnP);
	return 1;
}

int PPDevice_Leader::GetSessionPrice(PPObjGoods * pGObj, double * pPrice)
{
	int    ok = 0;
	if(pGObj) {
		QuotIdent qi(0, PPQUOTK_BASE);
		ok = pGObj->GetQuot(ConnP.GoodsID, qi, 0, 0, pPrice, 1);
	}
	return ok;
}

// @vmiller
#if SLTEST_RUNNING // {

#define EXPORT	extern "C" __declspec (dllexport)

class DvcDriver {
public:
	DvcDriver();
	~DvcDriver();
	int Test(SString sentence, SString & answer);
	int RunOneCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize);
private:
	SString Text;
};

DvcDriver * P_DvcDrv = 0;

DvcDriver::DvcDriver()
{
}

DvcDriver::~DvcDriver()
{
}

int Init()
{
	if(!P_DvcDrv)
		P_DvcDrv = new DvcDriver;
	return 1;
}

int Release()
{
	if(P_DvcDrv)
		ZDELETE(P_DvcDrv);
	return 1;
}

//EXPORT int RunCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize)
//{
//	int ok = 0;
//	THROW((pCmd != NULL) && (pOutputData != NULL) && (outSize != NULL));
//	if(stricmp(pCmd, "INIT") == 0)
//		THROW(Init())
//	else if(stricmp(pCmd, "RELEASE") == 0)
//		THROW(Release());
//	if(P_DvcDrv)
//		ok = P_DvcDrv->RunOneCommand(pCmd, pInputData, pOutputData, outSize);
//	CATCH
//		ok = 1;
//	ENDCATCH;
//	return ok;
//}

int DvcDriver::RunOneCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize)
{
	int    ok = 0;
	size_t new_size = 0;
	SString str, param_name, param_val;
	StringSet set(';', pInputData);
	for(uint i = 0; set.get(&i, str) > 0;) {
		str.Divide('=', param_name, param_val);
		if(param_name.CmpNC("TEXT") == 0)
			Text = param_val;
	}
	if(sstreqi_ascii(pCmd,"TEST")) {
		SString answer, new_size;
		THROW(!Test(Text, answer));
		if(answer.BufSize() > outSize) {
			new_size.Z().Cat(answer.BufSize());
			memcpy(pOutputData, new_size, outSize);
			ok = 2;
		}
		else
			memcpy(pOutputData, answer, answer.BufSize());
	}
	else if(!sstreqi_ascii(pCmd, "INIT")) {
		memcpy(pOutputData, "2", sizeof("2"));
        ok = 1;
	}
	CATCH
		memcpy(pOutputData, "1", sizeof("1"));
		ok = 1;
	ENDCATCH;
	return ok;
}

int DvcDriver::Test(SString sentence, SString & answer)
{
	int    ok = 0;
	size_t pos;
	if(sentence.Search("hello", 0, 1, &pos))
		answer = "Hello from driver";
	else if(sentence.Search("error", 0, 1, &pos))
		ok = 1;
	else
		answer.Z().Cat(sentence).CatCharN('-',1024);
	return ok;
}

SLTEST_R(PPAbstractDevice)
{
	int    ok = 1;
	int    r = 0;
	SString answer;
	StrAssocArray in, out;
	PPAbstractDevice dvc("");

	THROW(dvc.IdentifyDevice("ppw.exe"));
	in.Clear();
	THROW(dvc.RunCmd__(DVCCMD_INIT, in, out));
	in.Clear();
	in.Add(DVCPARAM_TEXT, "Driver, hello!!! Answer, answer!!", 1);
	THROW(r = dvc.RunCmd__(DVCCMD_TEST, in, out) == 1); // Без ошибок
	in.Clear();
	in.Add(DVCPARAM_TEXT, "It`s me", 1);
	THROW(r = dvc.RunCmd__(DVCCMD_TEST, in, out) == 1); // Ошибка переполнения выходного буфера. Резрешается в SendCmd
	in.Clear();
	in.Add(DVCPARAM_TEXT, "Get error", 1);
	THROW((r = dvc.RunCmd__(DVCCMD_TEST, in, out)) == -1); // Ошибка выполнения команды (код ошибки записывается в out). На этапе тестирования код равен 1
	out.Get(0, answer);
	THROW(answer == "1");
	THROW((r = dvc.RunCmd__(DVCCMD_SOMETHING, in, out)) == -1); // Неизвестная команда (код ошибки - 2)
	out.Get(0, answer);
	THROW(answer == "2");
	in.Clear();
	THROW(dvc.RunCmd__(DVCCMD_RELEASE, in, out));

	CATCH
		ok = 0;
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
