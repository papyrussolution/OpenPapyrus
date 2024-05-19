// WSCTL.CPP
// Copyright (c) A.Sobolev 2023, 2024
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <wsctl.h>
//
// 
// 
WsCtl_LoginBlock::WsCtl_LoginBlock()
{
	Z();
}
	
WsCtl_LoginBlock::~WsCtl_LoginBlock()
{
	Z();
}
	
WsCtl_LoginBlock & WsCtl_LoginBlock::Z()
{
	memzero(LoginText, sizeof(LoginText));
	memzero(PwText, sizeof(PwText));
	return *this;
}

WsCtl_RegistrationBlock::WsCtl_RegistrationBlock()
{
	Z();
}
	
WsCtl_RegistrationBlock::~WsCtl_RegistrationBlock()
{
	Z();
}
	
WsCtl_RegistrationBlock & WsCtl_RegistrationBlock::Z()
{
	memzero(Name, sizeof(Name));
	memzero(Phone, sizeof(Phone));
	memzero(PwText, sizeof(PwText));
	memzero(PwRepeatText, sizeof(PwRepeatText));
	return *this;
}
//
//
//
WsCtl_SelfIdentityBlock::WsCtl_SelfIdentityBlock() : PrcID(0)
{
}
	
int WsCtl_SelfIdentityBlock::GetOwnIdentifiers()
{
	int    ok = 1;
	bool   found = false;
	S_GUID _uuid;
	GetMACAddrList(&MacAdrList);
	{
		WinRegKey reg_key(HKEY_LOCAL_MACHINE, PPConst::WrKey_WsCtl, 1);
		if(reg_key.GetBinary(PPConst::WrParam_WsCtl_MachineUUID, &_uuid, sizeof(_uuid)) > 0) {
			Uuid = _uuid;
			found = true;
		}
	}
	if(!found) {
		WinRegKey reg_key(HKEY_LOCAL_MACHINE, PPConst::WrKey_WsCtl, 0);
		_uuid.Generate();
		if(reg_key.PutBinary(PPConst::WrParam_WsCtl_MachineUUID, &_uuid, sizeof(_uuid))) {
			Uuid = _uuid;
			ok = 2;
		}
		else
			ok = 0;
	}
	if(ok > 0) {
		SString msg_buf;
		msg_buf.Z().Cat("WSCTL own-uuid").CatDiv(':', 2).Cat(Uuid, S_GUID::fmtIDL);
		PPLogMessage("wsctl-debug.log", msg_buf, LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_SLSSESSGUID);
	}
	return ok;
}
//
//
//
WsCtl_Config::WsCtl_Config() : Port(0), Timeout(0)
{
}
	
WsCtl_Config & WsCtl_Config::Z()
{
	Server.Z();
	Port = 0;
	Timeout = 0;
	DbSymb.Z();
	User.Z();
	Password.Z();
	return *this;
}
//
// Descr: Считывает конфигурацию из win-реестра
//
int WsCtl_Config::Read()
{
	int    ok = 1;
	SJson * p_js = 0;
	SSecretTagPool pool;
	{
		SBuffer sbuf;
		WinRegKey reg_key(HKEY_LOCAL_MACHINE, PPConst::WrKey_WsCtl, 1);
		int    r = reg_key.GetBinary(PPConst::WrParam_WsCtl_Config, sbuf);
		THROW_SL(r);
		if(r > 0) {
			SString temp_buf;
			SSecretTagPool pool;
			THROW_SL(pool.Serialize(-1, sbuf, 0));
			p_js = pool.GetJson(SSecretTagPool::tagRawData);
			THROW(FromJsonObj(p_js));
		}
		else
			ok = -1;
	}
	CATCHZOK
	delete p_js;
	return ok;
}
//
// Descr: Записывает конфигурацию в win-реестр
//
int WsCtl_Config::Write()
{
	int    ok = 1;
	SString temp_buf;
	SSecretTagPool pool;
	SBuffer sbuf;
	SJson * p_js = ToJsonObj();
	THROW(p_js);
	p_js->ToStr(temp_buf);
	THROW_SL(pool.Put(SSecretTagPool::tagRawData, temp_buf, temp_buf.Len(), 0));
	THROW_SL(pool.Serialize(+1, sbuf, 0));
	{
		WinRegKey reg_key(HKEY_LOCAL_MACHINE, PPConst::WrKey_WsCtl, 0);
		THROW_SL(reg_key.PutBinary(PPConst::WrParam_WsCtl_Config, sbuf.GetBufC(), sbuf.GetAvailableSize()));
		ok = 1;
	}
	CATCHZOK
	delete p_js;
	return ok;
}

SJson * WsCtl_Config::ToJsonObj() const
{
	SJson * p_result = SJson::CreateObj();
	SString temp_buf;
	p_result->InsertStringNe("server", (temp_buf = Server).Escape());
	if(Port > 0) {
		p_result->InsertInt("port", Port);
	}
	if(Timeout > 0) {
		p_result->InsertInt("timeout", Timeout);
	}
	p_result->InsertStringNe("dbsymb", (temp_buf = DbSymb).Escape());
	p_result->InsertStringNe("user", (temp_buf = User).Escape());
	p_result->InsertStringNe("password", (temp_buf = Password).Escape());
	return p_result;
}

int WsCtl_Config::FromJsonObj(const SJson * pJsObj)
{
	int    ok = 0;
	if(SJson::IsObject(pJsObj)) {
		Z();
		const SJson * p_c = pJsObj->FindChildByKey("server");
		if(SJson::IsString(p_c))
			(Server = p_c->Text).Unescape();
		p_c = pJsObj->FindChildByKey("port");
		if(SJson::IsNumber(p_c))
			Port = p_c->Text.ToLong();
		p_c = pJsObj->FindChildByKey("timeout");
		if(SJson::IsNumber(p_c))
			Timeout = p_c->Text.ToLong();
		p_c = pJsObj->FindChildByKey("dbsymb");
		if(SJson::IsString(p_c))
			(DbSymb = p_c->Text).Unescape();
		p_c = pJsObj->FindChildByKey("user");
		if(SJson::IsString(p_c))
			(User = p_c->Text).Unescape();
		p_c = pJsObj->FindChildByKey("password");
		if(SJson::IsString(p_c))
			(Password = p_c->Text).Unescape();
		ok = 1;
	}
	return ok;
}
//
//
//
WsCtl_ClientPolicy::WsCtl_ClientPolicy()
{
}

WsCtl_ClientPolicy::WsCtl_ClientPolicy(const WsCtl_ClientPolicy & rS)
{
	Copy(rS);
}

WsCtl_ClientPolicy & FASTCALL WsCtl_ClientPolicy::Copy(const WsCtl_ClientPolicy & rS)
{
	SysUser = rS.SysUser;
	SysPassword = rS.SysPassword;
	SsAppEnabled = rS.SsAppEnabled;
	SsAppDisabled = rS.SsAppDisabled;
	SsAppPaths = rS.SsAppPaths; // @v11.8.6
	TSCollection_Copy(AllowedPathList, rS.AllowedPathList);
	TSCollection_Copy(AllowedRegList, rS.AllowedRegList);
	return *this;
}
	
WsCtl_ClientPolicy & WsCtl_ClientPolicy::Z()
{
	SysUser.Z();
	SysPassword.Z();
	SsAppEnabled.Z();
	SsAppDisabled.Z();
	SsAppPaths.Z(); // @v11.8.1
	AllowedPathList.freeAll(); // @v11.8.2
	AllowedRegList.freeAll(); // @v11.8.2
	return *this;
}

bool FASTCALL WsCtl_ClientPolicy::IsEq(const WsCtl_ClientPolicy & rS) const
{
	bool   eq = true;
	if(SysUser != rS.SysUser)
		eq = false;
	else if(SysPassword != rS.SysPassword)
		eq = false;
	else {
		if(!SsAppEnabled.IsEqPermutation(rS.SsAppEnabled))
			eq = false;
		else if(!SsAppDisabled.IsEqPermutation(rS.SsAppDisabled))
			eq = false;
		else if(!SsAppPaths.IsEqPermutation(rS.SsAppPaths)) // @v11.8.6
			eq = false;
		else if(!TSCollection_IsEq(&AllowedPathList, &rS.AllowedPathList))
			eq = false;
		else if(!TSCollection_IsEq(&AllowedRegList, &rS.AllowedRegList))
			eq = false;
	}
	return eq;
}
	
SJson * WsCtl_ClientPolicy::ToJsonObj() const
{
	SJson * p_js = SJson::CreateObj();
	SString temp_buf;
	if(SysUser.NotEmpty() || SysPassword.NotEmpty()) {
		SJson * p_js_acc = SJson::CreateObj();
		if(SysUser.NotEmpty()) {
			(temp_buf = SysUser).Escape();
			p_js_acc->InsertString("login", temp_buf);
		}
		if(SysPassword.NotEmpty()) {
			(temp_buf = SysPassword).Escape();
			p_js_acc->InsertString("password", temp_buf);
		}
		p_js->Insert("account", p_js_acc);
	}
	if(SsAppEnabled.getCount() || SsAppDisabled.getCount()) {
		SJson * p_js_app = SJson::CreateObj();
		if(SsAppEnabled.getCount()) {
			SJson * p_js_arr = SJson::CreateArr();
			for(uint ssp = 0; SsAppEnabled.get(&ssp, temp_buf);) {
				if(temp_buf.NotEmptyS())
					p_js_arr->InsertChild(SJson::CreateString(temp_buf));
			}
			p_js_app->Insert("enable", p_js_arr);
		}
		if(SsAppDisabled.getCount()) {
			SJson * p_js_arr = SJson::CreateArr();
			for(uint ssp = 0; SsAppDisabled.get(&ssp, temp_buf);) {
				if(temp_buf.NotEmptyS())
					p_js_arr->InsertChild(SJson::CreateString(temp_buf));
			}
			p_js_app->Insert("disable", p_js_arr);
		}
		// @v11.8.1 {
		if(SsAppPaths.getCount()) {
			SJson * p_js_arr = SJson::CreateArr();
			for(uint ssp = 0; SsAppPaths.get(&ssp, temp_buf);) {
				if(temp_buf.NotEmptyS())
					p_js_arr->InsertChild(SJson::CreateString(temp_buf));
			}
			p_js_app->Insert("paths", p_js_arr);
		}
		// } @v11.8.1 
		if(AllowedPathList.getCount()) {
			SJson * p_js_arr = SJson::CreateArr();
			for(uint i = 0; i < AllowedPathList.getCount(); i++) {
				const AllowedPath * p_item = AllowedPathList.at(i);
				if(p_item && p_item->Path.NotEmpty()) {
					//"path"
					//"access"
					SJson * p_js_item = SJson::CreateObj();
					p_js_item->InsertString("path", (temp_buf = p_item->Path).Escape());
					if(p_item->Flags) {
						// @todo
					}
				}
			}
		}
		if(AllowedRegList.getCount()) {
			SJson * p_js_arr = SJson::CreateArr();
			for(uint i = 0; i < AllowedRegList.getCount(); i++) {
				const AllowedRegistryEntry * p_item = AllowedRegList.at(i);
				if(p_item && p_item->Branch.NotEmpty()) {
					SJson * p_js_item = SJson::CreateObj();
					p_js_item->InsertString("key", (temp_buf = p_item->Branch).Escape());
					temp_buf.Z();
					if(p_item->RegKeyType == WinRegKey::regkeytypGeneral)
						temp_buf = "general";
					else if(p_item->RegKeyType == WinRegKey::regkeytypWow64_64)
						temp_buf = "wow64_64";
					else if(p_item->RegKeyType == WinRegKey::regkeytypWow64_32)
						temp_buf = "wow64_32";
					if(temp_buf.NotEmpty()) {
						p_js_item->InsertString("type", temp_buf);
					}
					if(p_item->Flags) {
						// @todo
					}
				}
			}
		}
		p_js->Insert("app", p_js_app);
	}
	return p_js;
}
	
int WsCtl_ClientPolicy::FromJsonObj(const SJson * pJsObj)
{
	int    ok = 1;
	if(SJson::IsObject(pJsObj)) {
		Z();
		SString temp_buf;
		const SJson * p_c = pJsObj->FindChildByKey("account");
		if(SJson::IsObject(p_c)) {
			//(Server = p_c->Text).Unescape();
			const SJson * p_ac = p_c->FindChildByKey("login");
			if(SJson::IsString(p_ac)) {
				(SysUser = p_ac->Text).Unescape();
			}
			p_ac = p_c->FindChildByKey("password");
			if(SJson::IsString(p_ac)) {
				(SysPassword = p_ac->Text).Unescape();
			}
		}
		p_c = pJsObj->FindChildByKey("app");
		if(SJson::IsObject(p_c)) {
			const SJson * p_ac = p_c->FindChildByKey("enable");
			if(SJson::IsArray(p_ac)) {
				for(const SJson * p_js_item = p_ac->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
					if(SJson::IsString(p_js_item) && (temp_buf = p_js_item->Text).NotEmptyS())
						SsAppEnabled.add(temp_buf.Unescape());
				}
			}
			p_ac = p_c->FindChildByKey("disable");
			if(SJson::IsArray(p_ac)) {
				for(const SJson * p_js_item = p_ac->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
					if(SJson::IsString(p_js_item) && (temp_buf = p_js_item->Text).NotEmptyS())
						SsAppDisabled.add(temp_buf.Unescape());
				}
			}
			// @v11.8.1 {
			p_ac = p_c->FindChildByKey("paths");
			if(SJson::IsArray(p_ac)) {
				for(const SJson * p_js_item = p_ac->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
					if(SJson::IsString(p_js_item) && (temp_buf = p_js_item->Text).NotEmptyS())
						SsAppPaths.add(temp_buf.Unescape());
				}
			}
			// } @v11.8.1 
			p_ac = p_c->FindChildByKey("allowance");
			if(SJson::IsArray(p_ac)) {
				for(const SJson * p_js_item = p_ac->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
					if(SJson::IsObject(p_js_item)) {
						const SJson * p_js_f = p_js_item->FindChildByKey("path");
						if(SJson::IsString(p_js_f)) {
							AllowedPath * p_new_item = AllowedPathList.CreateNewItem();
							THROW(p_new_item);
							(p_new_item->Path = p_js_f->Text).Unescape();
							p_js_f = p_js_item->FindChildByKey("accs");
							if(SJson::IsString(p_js_f)) {
								// parse accsess rights
							}
						}
						else {
							p_js_f = p_c->FindChildByKey("registry");
							if(SJson::IsString(p_js_f)) {
								AllowedRegistryEntry * p_new_item = AllowedRegList.CreateNewItem();
								(p_new_item->Branch = p_js_f->Text).Unescape();

								p_js_f = p_c->FindChildByKey("type");
								WinRegKey wrk;
								if(SJson::IsString(p_js_f)) {
									(temp_buf = p_js_f->Text).Unescape().Strip();
									if(temp_buf.IsEqiAscii("general"))
										p_new_item->RegKeyType = WinRegKey::regkeytypGeneral;
									else if(temp_buf.IsEqiAscii("wow64_32"))
										p_new_item->RegKeyType = WinRegKey::regkeytypWow64_32;
									else if(temp_buf.IsEqiAscii("wow64_64"))
										p_new_item->RegKeyType = WinRegKey::regkeytypWow64_64;
									else
										p_new_item->RegKeyType = WinRegKey::regkeytypWow64_64; // @default (я не уверен, что это - правильно)
								}
								p_js_f = p_js_item->FindChildByKey("accs");
								if(SJson::IsString(p_js_f)) {
									// parse accsess rights
								}
							}
						}
					}
				}
			}
			// @v11.8.2 {
			p_ac = p_c->FindChildByKey("allowedpaths");
			if(SJson::IsArray(p_ac)) {
				for(const SJson * p_js_item = p_ac->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
					if(SJson::IsObject(p_js_item)) {
						const SJson * p_js_item = p_c->FindChildByKey("path");
						if(SJson::IsString(p_js_item)) {
							AllowedPath * p_new_item = AllowedPathList.CreateNewItem();
							THROW(p_new_item);
							(p_new_item->Path = p_js_item->Text).Unescape();
							//
							p_js_item = p_c->FindChildByKey("access");
							if(SJson::IsString(p_js_item)) {
								p_new_item->Flags = SFile::accsfAll; // @stub
							}
							else {
								p_new_item->Flags = SFile::accsfAll;
							}
						}
					}
				}
			}
			p_ac = p_c->FindChildByKey("allowedregisters");
			if(SJson::IsArray(p_ac)) {
				for(const SJson * p_js_item = p_ac->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
					if(SJson::IsObject(p_js_item)) {
						/*
							struct AllowedRegistryEntry {
								int   RegKeyType; // WinRegKey::regkeytypXXX general || wow64_32 || wow64_64
								uint  Flags;       
								SString Branch;
							};
						*/
						const SJson * p_js_item = p_c->FindChildByKey("key");
						if(SJson::IsString(p_js_item)) {
							AllowedRegistryEntry * p_new_item = AllowedRegList.CreateNewItem();
							(p_new_item->Branch = p_js_item->Text).Unescape();
							p_js_item = p_c->FindChildByKey("type");
							WinRegKey wrk;
							if(SJson::IsString(p_js_item)) {
								(temp_buf = p_js_item->Text).Unescape().Strip();
								if(temp_buf.IsEqiAscii("general"))
									p_new_item->RegKeyType = WinRegKey::regkeytypGeneral;
								else if(temp_buf.IsEqiAscii("wow64_32"))
									p_new_item->RegKeyType = WinRegKey::regkeytypWow64_32;
								else if(temp_buf.IsEqiAscii("wow64_64"))
									p_new_item->RegKeyType = WinRegKey::regkeytypWow64_64;
								else
									p_new_item->RegKeyType = WinRegKey::regkeytypWow64_64; // @default (я не уверен, что это - правильно)
							}
							p_js_item = p_c->FindChildByKey("access");
							if(SJson::IsString(p_js_item)) {
								p_new_item->Flags = SFile::accsfAll; // @stub
							}
							else
								p_new_item->Flags = SFile::accsfAll;
						}
					}
				}
			}
			// } @v11.8.2
		}
		ok = 1;			
	}
	CATCHZOK
	return ok;
}

int WsCtl_ClientPolicy::Resolve()
{
	int    ok = 1;
	{
		const SrUedContainer_Rt * p_uedc = DS.GetUedContainer();
		SString temp_buf;
		SString template_buf;
		SString tail_buf;
		for(uint i = 0; i < AllowedPathList.getCount(); i++) {
			AllowedPath * p_item = AllowedPathList.at(i);
			p_item->ResolvedPath.Z();
			if(p_item->Path.NotEmpty()) {
				uint end_pos = 0;
				if(p_item->Path.HasPrefix("${") && p_item->Path.SearchCharPos(2, '}', &end_pos)) {
					temp_buf = p_item->Path;
					temp_buf.Sub(0, end_pos+1, template_buf); // длина отрезка = end_pos+1
					(tail_buf = temp_buf).ShiftLeft(end_pos+1); // Убираем шаблон из пути. Если там еще что-то есть, то 
					template_buf.ShiftLeft(2).TrimRight(); // убираем служебные символы ${}
					if(p_uedc) {
						uint64 ued_path = p_uedc->SearchSymb(template_buf, UED_META_FSKNOWNFOLDER);
						if(ued_path) {
							if(GetKnownFolderPath(ued_path, temp_buf)) {
								if(tail_buf.NotEmptyS()) {
									tail_buf.ShiftLeftChr('\\').ShiftLeftChr('/');
									if(tail_buf.NotEmptyS()) {
										temp_buf.SetLastSlash().Cat(tail_buf);
									}
								}
								p_item->ResolvedPath = temp_buf;
							}
						}
					}
				}
				else {
					p_item->ResolvedPath = p_item->Path;
				}
			}
			else
				p_item->ResolvedPath.Z();
		}
	}
	return ok;
}

int WsCtl_ClientPolicy::Apply()
{
	int    ok = 1;
	if(SsAppDisabled.getCount()) {
		WinRegKey reg_key(HKEY_CURRENT_USER, SlConst::P_WrKey_MsWin_DisallowRun, 0);
		THROW(reg_key.PutEnumeratedStrings(SsAppDisabled, 0));
	}
	if(SsAppEnabled.getCount()) {
		WinRegKey reg_key(HKEY_CURRENT_USER, SlConst::P_WrKey_MsWin_RestrictRun, 0);
		THROW(reg_key.PutEnumeratedStrings(SsAppEnabled, 0));
	}
	CATCHZOK
	return ok;
}

SString & WsCtl_ClientPolicy::MakeBaseSystemImagePath(SString & rBuf) const
{
	rBuf.Z();
	PPGetPath(PPPATH_WORKSPACE, rBuf);
	rBuf.SetLastSlash().Cat("wsctl").SetLastSlash().Cat("wsbui");
	return rBuf;
}

bool WsCtl_ClientPolicy::IsThereSystemImage() const
{
	SString bu_base_path;
	MakeBaseSystemImagePath(bu_base_path);
	SSystemBackup sb;
	uint32 bu_id = sb.GetLastBackupId(bu_base_path);
	return (bu_id > 0);
}

int WsCtl_ClientPolicy::CreateSystemImage()
{
	int    ok = 1;
	SSystemBackup::Param param;
	MakeBaseSystemImagePath(param.BackupPath);
	if(SysUser.NotEmpty()) {
		param.AddProfileEntry(SysUser, SysPassword);
	}
	if(AllowedPathList.getCount()) {
		for(uint i = 0; i < AllowedPathList.getCount(); i++) {
			const AllowedPath * p_ap = AllowedPathList.at(i);
			if(p_ap && ((p_ap->Flags & SFile::accsfAll) || (p_ap->Flags & SFile::accsfGenericWrite) ||
				(p_ap->Flags & (SFile::accsfDataWrite|SFile::accsfDataAppend|
				SFile::accsfEaWrite|SFile::accsfDirAddFile|SFile::accsfDirAddSub|SFile::accsfDirDelete)))) {
				if(p_ap->ResolvedPath.NotEmpty())
					param.AddPathEntry(p_ap->ResolvedPath);
			}
		}
	}
	if(AllowedRegList.getCount()) {
		for(uint i = 0; i < AllowedRegList.getCount(); i++) {
			const AllowedRegistryEntry * p_ar = AllowedRegList.at(i);
			if(p_ar && ((p_ar->Flags & WinRegKey::accsfAll) || (p_ar->Flags & WinRegKey::accsfKeyWrite) ||
				(p_ar->Flags & (WinRegKey::accsfSetValue|WinRegKey::accsfCreateSub)))) {
				if(p_ar->Branch.NotEmpty())
					param.AddRegEntry(p_ar->Branch);
			}
		}
	}
	{
		param.Flags |= SSystemBackup::Param::fDumpAfterBackup;
		SSystemBackup sb(param);
		if(sb.Backup()) {
			;
		}
		else {
			ok = 0;
		}
	}
	return ok;
}

int WsCtl_ClientPolicy::RestoreSystemImage()
{
	int    ok = -1;
	SString bu_base_path;
	MakeBaseSystemImagePath(bu_base_path);
	SSystemBackup sb;
	if(sb.Restore(bu_base_path, _FFFF32, SSystemBackup::rfRemoveNewEntries|SSystemBackup::rfRestoreRemovedEntries)) {
		ok = 1;
	}
	else {
		ok = 0;
	}
	return ok;
}
//
//
//
WsCtlSrvBlock::RegistrationBlock::RegistrationBlock() : Status(0), SCardID(0), PsnID(0)
{
	Z();
}
		
WsCtlSrvBlock::RegistrationBlock & WsCtlSrvBlock::RegistrationBlock::Z()
{
	WsCtlUuid.Z();
	Name.Obfuscate();
	Name.Z();
	Phone.Obfuscate();
	Phone.Z();
	PwText.Obfuscate();
	PwText.Z();
	Status = 0;
	SCardID = 0;
	PsnID = 0;
	return *this;
}
		
bool WsCtlSrvBlock::RegistrationBlock::IsValid() const
{
	bool result = true;
	if(Name.IsEmpty())
		result = false;
	else {
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		tr.Run(Phone, nta, 0);
		if(!nta.Has(SNTOK_PHONE))
			result = false;
	}
	return result;
}

bool WsCtlSrvBlock::RegistrationBlock::FromJsonObj(const SJson * pJs)
{
	bool   result = false;
	Z();
	if(pJs) {
		const SJson * p_c = pJs->FindChildByKey("nm"); // @v12.0.1 "name"-->"nm"
		if(SJson::IsString(p_c))
			Name = p_c->Text;
		p_c = pJs->FindChildByKey("phone");
		if(SJson::IsString(p_c))
			Phone = p_c->Text;
		p_c = pJs->FindChildByKey("pw");
		if(SJson::IsString(p_c))
			PwText = p_c->Text;
		p_c = pJs->FindChildByKey("wsctluuid");
		if(SJson::IsString(p_c)) {
			WsCtlUuid.FromStr(p_c->Text);
		}
	}
	result = IsValid();
	return result;
}

WsCtlSrvBlock::AuthBlock::AuthBlock()
{
}
		
WsCtlSrvBlock::AuthBlock & WsCtlSrvBlock::AuthBlock::Z()
{
	WsCtlUuid.Z();
	LoginText.Z();
	Pw.Z();
	return *this;
}
//
// Descr: Возвращает true если входящие параметры инициализированы в достаточной степени для запуска исполнения команды
//
bool WsCtlSrvBlock::AuthBlock::IsEmpty() const
{
	return LoginText.IsEmpty();//(Phone.NotEmpty() || Name.NotEmpty() || ScCode.NotEmpty());
}

bool WsCtlSrvBlock::AuthBlock::FromJsonObj(const SJson * pJs)
{
	Z();
	if(pJs) {
		const SJson * p_c = pJs->FindChildByKey("login");
		if(SJson::IsString(p_c))
			LoginText = p_c->Text;
		p_c = pJs->FindChildByKey("pw");
		if(SJson::IsString(p_c))
			Pw = p_c->Text;
		p_c = pJs->FindChildByKey("wsctluuid");
		if(SJson::IsString(p_c)) {
			WsCtlUuid.FromStr(p_c->Text);
		}
	}
	return !IsEmpty();
}

WsCtlSrvBlock::StartSessBlock::StartSessBlock() : SCardID(0), GoodsID(0), TSessID(0), TechID(0), Amount(0.0), RetTechID(0), RetSCardID(0), RetGoodsID(0), ScOpDtm(ZERODATETIME), WrOffAmount(0.0)
{
}
		
WsCtlSrvBlock::StartSessBlock & WsCtlSrvBlock::StartSessBlock::Z()
{
	WsCtlUuid.Z();
	SCardID = 0;
	GoodsID = 0;
	TSessID = 0;
	TechID = 0;
	Amount = 0.0;
	RetTechID = 0;
	RetSCardID = 0;
	RetGoodsID = 0;
	ScOpDtm.Z();
	WrOffAmount = 0.0;
	SessTimeRange.Z();
	return *this;
}

bool WsCtlSrvBlock::StartSessBlock::FromJsonObj(const SJson * pJs)
{
	Z();
	if(pJs) {
		const SJson * p_c = pJs->FindChildByKey("scardid");
		if(SJson::IsNumber(p_c))
			SCardID = p_c->Text.ToLong();
		p_c = pJs->FindChildByKey("goodsid");
		if(SJson::IsNumber(p_c))
			GoodsID = p_c->Text.ToLong();
		p_c = pJs->FindChildByKey("techid");
		if(SJson::IsNumber(p_c))
			TechID = p_c->Text.ToLong();
		p_c = pJs->FindChildByKey("amt");
		if(SJson::IsNumber(p_c))
			Amount = p_c->Text.ToReal();
		p_c = pJs->FindChildByKey("wsctluuid");
		if(SJson::IsString(p_c)) {
			WsCtlUuid.FromStr(p_c->Text);
		}
	}
	return !IsEmpty();
}

WsCtlSrvBlock::WsCtlSrvBlock() : PrcID(0), ScSerID(0), PsnKindID(0)
{
	ScObj.FetchConfig(&ScCfg);
	if(ScCfg.DefCreditSerID) {
		PPObjSCardSeries scs_obj;
		PPSCardSeries scs_rec;
		if(scs_obj.Fetch(ScCfg.DefCreditSerID, &scs_rec) > 0) {
			ScSerID = ScCfg.DefCreditSerID;
			PsnKindID = scs_rec.PersonKindID;
		}
	}
}
	
int WsCtlSrvBlock::SearchPrcByWsCtlUuid(const S_GUID & rWsCtlUuid, PPID * pPrcID, SString * pPrcNameUtf8)
{
	int    ok = -1;
	ASSIGN_PTR(pPrcID, 0);
	CALLPTRMEMB(pPrcNameUtf8, Z());
	if(!!rWsCtlUuid) {
		Reference * p_ref = PPRef;
		PPIDArray prc_list;
		p_ref->Ot.SearchObjectsByGuid(PPOBJ_PROCESSOR, PPTAG_PRC_UUID, rWsCtlUuid, &prc_list);
		THROW_PP_S(prc_list.getCount(), PPERR_WSCTL_PRCBYUUIDNFOUND, SLS.AcquireRvlStr().Cat(rWsCtlUuid, S_GUID::fmtIDL));
		THROW_PP_S(prc_list.getCount() == 1, PPERR_WSCTL_DUPPRCUUID, SLS.AcquireRvlStr().Cat(rWsCtlUuid, S_GUID::fmtIDL));
		{
			const  PPID prc_id = prc_list.get(0);
			ProcessorTbl::Rec prc_rec;
			THROW(TSesObj.PrcObj.Fetch(prc_id, &prc_rec) > 0);
			ASSIGN_PTR(pPrcID, prc_id);
			//WsUUID = rUuid;
			if(pPrcNameUtf8)
				(*pPrcNameUtf8 = prc_rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}
	
int WsCtlSrvBlock::Init(const S_GUID & rUuid)
{
	int    ok = SearchPrcByWsCtlUuid(rUuid, &PrcID, &PrcNameUtf8);
	if(ok > 0)
		WsUUID = rUuid;
	return ok;
}
//
// Descr: Возвращает ранжированный список видов котировок, которые могут быть применены для установки цен для технологических сессий.
//
int WsCtlSrvBlock::GetRawQuotKindList(PPIDArray & rList) 
{
	rList.Z();
	int    ok = 1;
	StrAssocArray qk_sa_list;
	QuotKindFilt qk_filt;
	qk_filt.MaxItems = -1;
	qk_filt.Flags |= (QuotKindFilt::fIgnoreRights|QuotKindFilt::fAddBase|QuotKindFilt::fSortByRankName);
	QkObj.MakeList(&qk_filt, &qk_sa_list);
	{
		for(uint i = 0; i < qk_sa_list.getCount(); i++) {
			rList.add(qk_sa_list.at_WithoutParent(i).Id);
		}
		QkObj.ArrangeList(getcurdatetime_(), rList, RTLPF_USEQUOTWTIME);
	}
	return ok;
}

int WsCtlSrvBlock::GetQuotList(PPID goodsID, PPID locID, const PPIDArray & rRawQkList, PPQuotArray & rQList)
{
	rQList.clear();
	int    ok = -1;
	Goods2Tbl::Rec goods_rec;
	if(GObj.Fetch(goodsID, &goods_rec) > 0) {
		PPQuotKind qk_rec;
		PPIDArray qk_list_intersection;
		GObj.GetQuotList(goodsID, locID, rQList);
		rQList.GetQuotKindIdList(qk_list_intersection);
		qk_list_intersection.intersect(&rRawQkList, 0);
		if(qk_list_intersection.getCount()) {
			uint i = rQList.getCount();
			assert(i);
			if(i) {
				do {
					const PPQuot & r_q = rQList.at(--i);
					if(qk_list_intersection.lsearch(r_q.Kind)) {
						const int qkfr = QkObj.Fetch(r_q.Kind, &qk_rec);
						assert(qkfr > 0);
						if(qkfr <= 0) {
							rQList.atFree(i);
						}
					}
					else {
						rQList.atFree(i);
					}
				} while(i);
				if(rQList.getCount())
					ok = 1;
			}
		}
	}
	return ok;
}
	
int WsCtlSrvBlock::StartSess(StartSessBlock & rBlk)
{
	int    ok = 1;
	const LDATETIME now_dtm = getcurdatetime_();
	PPID   tses_id = 0;
	double screst = 0.0; // Остаток на счете клиента на момент начала сессии
	SCardTbl::Rec sc_rec;
	Goods2Tbl::Rec goods_rec;
	THROW(!rBlk.IsEmpty());
	THROW(rBlk.WsCtlUuid);
	THROW(ScSerID); // @todo @err (не удалось определить серию карт, с которыми ассоциированы аккаунты клиентов)
	THROW(ScObj.Search(rBlk.SCardID, &sc_rec) > 0);
	THROW(ScObj.P_Tbl->GetRest(rBlk.SCardID, ZERODATE, &screst));
	THROW(GObj.Search(rBlk.GoodsID, &goods_rec) > 0);
	{
		PPID   prc_id = 0;
		PPID   tec_id = 0;
		double amount_to_wroff = 0.0;
		PPIDArray tec_id_list;
		SString prc_name_utf8;
		ProcessorTbl::Rec prc_rec;
		TechTbl::Rec tec_rec;
		THROW(SearchPrcByWsCtlUuid(rBlk.WsCtlUuid, &prc_id, &prc_name_utf8) > 0);
		THROW(TSesObj.GetPrc(prc_id, &prc_rec, 1, 1) > 0);
		TSesObj.TecObj.GetListByPrcGoods(prc_id, rBlk.GoodsID, &tec_id_list);
		THROW(tec_id_list.getCount());
		tec_id = tec_id_list.get(0);
		THROW(TSesObj.TecObj.Fetch(tec_id, &tec_rec) > 0);
		{
			double price = 0.0;
			PPIDArray raw_qk_id_list;
			PPQuotArray qlist;
			GetRawQuotKindList(raw_qk_id_list);
			if(GetQuotList(rBlk.GoodsID, prc_rec.LocID, raw_qk_id_list, qlist) > 0) {
				assert(qlist.getCount());
				price = qlist.at(0).Quot;
			}
			if(price > 0.0) {
				if(rBlk.Amount > 0) {
					if(feqeps(rBlk.Amount, price, 1.0E-6)) {
						amount_to_wroff = price; // ok
					}
					else {
						; // @todo @err
					}
				}
			}
			else {
				; // @todo @err
			}
		}
		if(amount_to_wroff > 0.0) {
			THROW_PP_S(amount_to_wroff <= screst, PPERR_SCARDRESTNOTENOUGH, sc_rec.Code);
		}
		{
			TSessionPacket pack;
			ProcessorTbl::Rec inh_prc_rec;
			TSesObj.InitPacket(&pack, TSESK_SESSION, prc_id, 0, TSESST_INPROCESS);
			pack.OuterTimingPrice = amount_to_wroff;
			pack.Rec.TechID = tec_id;
			pack.Rec.SCardID = rBlk.SCardID; 
			pack.Rec.StDt = now_dtm.d;
			pack.Rec.StTm = now_dtm.t;
			pack.Rec.PlannedQtty = (tec_rec.InitQtty > 0.0f) ? tec_rec.InitQtty : 1.0;
			if(tec_rec.Capacity > 0.0 && pack.Rec.PlannedQtty > 0.0) {
				long duration = R0i(pack.Rec.PlannedQtty / tec_rec.Capacity);
				if(duration > 0) {
					pack.Rec.PlannedTiming = duration;
					LDATETIME finish_dtm;
					(finish_dtm = now_dtm).addsec(duration);
					pack.Rec.FinDt = finish_dtm.d;
					pack.Rec.FinTm = finish_dtm.t;
				}
			}
			THROW(TSesObj.CheckSessionTime(pack.Rec));
			THROW(TSesObj.GetPrc(prc_id, &inh_prc_rec, 1, 1) > 0);
			if(inh_prc_rec.WrOffOpID) {
				PPOprKind op_rec;
				if(GetOpData(inh_prc_rec.WrOffOpID, &op_rec) > 0 && op_rec.AccSheetID && sc_rec.PersonID) {
					PPID   ar_id = 0;
					if(ArObj.P_Tbl->PersonToArticle(sc_rec.PersonID, op_rec.AccSheetID, &ar_id) > 0) {
						pack.Rec.ArID = ar_id;
					}
				}
			}
			{
				PPTransaction tra(1);
				THROW(tra);
				THROW(TSesObj.PutPacket(&tses_id, &pack, 0));
				pack.GetTimeRange(rBlk.SessTimeRange);
				if(amount_to_wroff > 0.0) {
					SCardCore::OpBlock op;
					op.LinkOi.Set(PPOBJ_TSESSION, tses_id);
					op.SCardID = rBlk.SCardID;
					op.Amount = -amount_to_wroff;
					op.Dtm = getcurdatetime_();
					THROW(ScObj.P_Tbl->PutOpBlk(op, 0, 0));
					rBlk.RetSCardID = op.SCardID;
					rBlk.ScOpDtm = op.Dtm;
					rBlk.WrOffAmount = op.Amount;
				}
				THROW(tra.Commit());
			}
			{
				/*
					PPID   TSessID;
					PPID   TechID;
					PPID   RetSCardID;
					PPID   RetGoodsID;
					LDATETIME ScOpDtm; // Время операции списания денег //
					double WrOffAmount; // Списанная сумма //
					STimeChunk SessTimeRange;
				*/
				rBlk.TSessID = tses_id;
				rBlk.RetTechID = pack.Rec.TechID;
				rBlk.RetSCardID = rBlk.SCardID;
				rBlk.RetGoodsID = rBlk.GoodsID;
				// @todo rBlk.ScOpDtm
				// @todo rBlk.WrOffAmount
				rBlk.SessTimeRange.Start.Set(pack.Rec.StDt, pack.Rec.StTm);
				rBlk.SessTimeRange.Finish.Set(pack.Rec.FinDt, pack.Rec.FinTm);
			}
		}
	}
	CATCHZOK
	return ok;
}

int WsCtlSrvBlock::RegisterComputer(ComputerRegistrationBlock & rBlk)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	PPComputerPacket ex_comp_pack;
	THROW(rBlk.MacAdrList.getCount());
	{
		PPID   ex_id = 0;
		PPIDArray comp_id_list_by_uuid;
		PPIDArray comp_id_list_by_macadr;
		if(!!rBlk.WsCtlUuid) {
			p_ref->Ot.SearchObjectsByGuid(PPOBJ_COMPUTER, PPTAG_COMPUTER_GUID, rBlk.WsCtlUuid, &comp_id_list_by_uuid);
		}
		{
			PPIDArray temp_list;
			for(uint i = 0; i < rBlk.MacAdrList.getCount(); i++) {
				const MACAddr & r_macadr = rBlk.MacAdrList.at(i);
				PPID c_id = 0;
				int r = CompObj.SearchByMacAddr(r_macadr, &c_id, &ex_comp_pack);
				CompObj.GetListByMacAddr(r_macadr, temp_list);
				if(temp_list.getCount())
					comp_id_list_by_macadr.add(&temp_list);
				/*THROW(r);
				if(r > 0) {
					rBlk.Status = 2;
					rBlk.Name = c_pack.Rec.Name;
					rBlk.ComputerID = c_pack.Rec.ID;
					rBlk.WsCtlUuid = c_pack.Rec.Uuid;
					found_by_macadr = true;
				}*/
			}
		}
		if(comp_id_list_by_uuid.getCount()) {
			if(comp_id_list_by_macadr.getCount()) {
				PPIDArray temp_list(comp_id_list_by_uuid);
				temp_list.intersect(&comp_id_list_by_macadr);
				if(!temp_list.getCount()) {
					// Неоднозначность в идентификации по GUID и MAC-адресу
					CALLEXCEPT_PP(PPERR_COMPIDENTAMBIG_GUIDMACADR);
				}
				else if(temp_list.getCount() > 1) {
					// Неоднозначность в идентификации по GUID и MAC-адресу
					CALLEXCEPT_PP(PPERR_COMPIDENTAMBIG_GUIDMACADR);
				}
				else {
					assert(temp_list.getCount() == 1);
					ex_id = temp_list.get(0);
				}
			}
			else if(comp_id_list_by_uuid.getCount() > 1) {
				// Неоднозначность в идентификации по GUID
				CALLEXCEPT_PP(PPERR_COMPIDENTAMBIG_GUID);
			}
			else {
				assert(comp_id_list_by_uuid.getCount() == 1);
				ex_id = comp_id_list_by_uuid.get(0);
			}
		}
		else if(comp_id_list_by_macadr.getCount() == 1)
			ex_id = comp_id_list_by_macadr.get(0);
		else if(comp_id_list_by_macadr.getCount()) {
			// Неоднозначность в идентификации по MAC-адресу
			CALLEXCEPT_PP(PPERR_COMPIDENTAMBIG_MACADR);
		}
		//const bool found_by_macadr = comp_id_list_by_macadr.getCount() ? true : false;
		//const bool found_by_uuid = comp_id_list_by_uuid.getCount() ? true : false;
		if(ex_id) {
			THROW(CompObj.Get(ex_id, &ex_comp_pack) > 0);
			{
				bool do_update = false;
				if(ex_comp_pack.Rec.MacAdr.IsZero() && rBlk.MacAdrList.getCount()) {
					ex_comp_pack.Rec.MacAdr = rBlk.MacAdrList.at(0);
					do_update = true;
				}
				if(ex_comp_pack.Rec.Uuid.IsZero() && !rBlk.WsCtlUuid.IsZero()) {
					ex_comp_pack.Rec.Uuid = rBlk.WsCtlUuid;
					do_update = true;
				}
				if(do_update) {
					THROW(CompObj.Put(&ex_id, &ex_comp_pack, 1));
				}
			}
			rBlk.Status = 2;
			rBlk.Name = ex_comp_pack.Rec.Name;
			rBlk.ComputerID = ex_comp_pack.Rec.ID;
			rBlk.WsCtlUuid = ex_comp_pack.Rec.Uuid;
		}
		else {
			PPID   new_computer_id = 0;
			PPComputerPacket new_c_pack;
			if(rBlk.MacAdrList.getCount())
				new_c_pack.Rec.MacAdr = rBlk.MacAdrList.at(0);
			if(rBlk.Name.NotEmptyS()) {
				STRNSCPY(new_c_pack.Rec.Name, rBlk.Name);
			}
			else {
				CompObj.GenerateName(0, rBlk.Name);
				STRNSCPY(new_c_pack.Rec.Name, rBlk.Name);
			}
			if(!!rBlk.WsCtlUuid) {
				new_c_pack.Rec.Uuid = rBlk.WsCtlUuid;
			}
			else
				new_c_pack.Rec.Uuid.Generate();
			THROW(CompObj.Put(&new_computer_id, &new_c_pack, 1));
			{
				rBlk.Status = 1;
				rBlk.Name = new_c_pack.Rec.Name;
				rBlk.ComputerID = new_c_pack.Rec.ID;
				rBlk.WsCtlUuid = new_c_pack.Rec.Uuid;
			}
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int WsCtlSrvBlock::Registration(RegistrationBlock & rBlk)
{
	int    ok = 1;
	PPID   sc_id = 0;
	PPID   psn_id = 0;
	SString temp_buf;
	PPObjSCardSeries scs_obj;
	PPSCardSeries2 scs_rec;
	THROW(rBlk.IsValid());
	THROW(ScSerID); // @todo @err (не удалось определить серию карт, с которыми ассоциированы аккаунты клиентов)
	THROW(scs_obj.Search(ScSerID, &scs_rec) > 0);
	THROW(scs_rec.PersonKindID); // @todo @err Это поле необходимо для идентификации вида, которому будет принадлежать новая персоналия //
	{
		PPObjIDArray oid_list_by_phone;
		PPEAddr::Phone::NormalizeStr(rBlk.Phone, 0, temp_buf);
		PsnObj.LocObj.P_Tbl->SearchPhoneObjList(temp_buf, 0, oid_list_by_phone);
		if(oid_list_by_phone.getCount()) {
			; // @todo Надо что-то сделать если телефон уже есть в базе данных
		}
		else {
			PPPersonPacket psn_pack;
			PPSCardPacket sc_pack;
			PPObjPerson::EditBlock _eb;
			PsnObj.InitEditBlock(scs_rec.PersonKindID, _eb);
			psn_pack.Rec.Status = NZOR(_eb.InitStatusID, PPPRS_PRIVATE);
			STRNSCPY(psn_pack.Rec.Name, rBlk.Name);
			psn_pack.Kinds.add(_eb.InitKindID);
			psn_pack.ELA.AddItem(PPELK_MOBILE, rBlk.Phone);
			//
			//sc_pack
			{
				assert(psn_id == 0);
				assert(sc_id == 0);
				SString dummy_number;
				PPTransaction tra(1);
				THROW(tra);
				THROW(PsnObj.PutPacket(&psn_id, &psn_pack, 0));
				THROW(ScObj.Create_(&sc_id, ScSerID, psn_id, /*pPatternRec*/0, dummy_number, rBlk.PwText, /*flags*/PPObjSCard::cdfCreditCard, /*use_ta*/0));
				THROW(tra.Commit());
				rBlk.PsnID = psn_id;
				rBlk.SCardID = sc_id;
				rBlk.Status = 1;
			}

		}
	}
	CATCH
		rBlk.Phone = 0;
		rBlk.SCardID = 0;
		rBlk.SCardID = 0;
		ok = 0;
	ENDCATCH
	return ok;
}
	
int WsCtlSrvBlock::Auth(AuthBlock & rBlk)
{
	int    ok = 0;
	SString temp_buf;
	SCardTbl::Rec sc_rec;
	PersonTbl::Rec psn_rec;
	THROW(!rBlk.IsEmpty()); // @todo @err (не определены параметры для авторизации)
	THROW(ScSerID); // @todo @err (не удалось определить серию карт, с которыми ассоциированы аккаунты клиентов)
	assert(rBlk.LoginText.NotEmpty()); // Вызов rBlk.IsEmpty() выше должен гарантировать этот инвариант
	{
		bool    text_identification_done = false;
		SNaturalTokenArray nta;
		STokenRecognizer tr;
		tr.Run(rBlk.LoginText, nta, 0);
		if(nta.Has(SNTOK_PHONE)) {
			PPObjIDArray oid_list_by_phone;
			PPEAddr::Phone::NormalizeStr(rBlk.LoginText, 0, temp_buf);
			PsnObj.LocObj.P_Tbl->SearchPhoneObjList(temp_buf, 0, oid_list_by_phone);
			if(oid_list_by_phone.getCount()) {
				PPID   single_sc_id = 0;
				PPID   single_psn_id = 0;
				bool   mult_sc_id_by_phone = false;
				bool   mult_psn_id_by_phone = false;
				for(uint i = 0; i < oid_list_by_phone.getCount(); i++) {
					const PPObjID oid = oid_list_by_phone.at(i);
					if(oid.Obj == PPOBJ_SCARD && ScObj.Fetch(oid.Id, &sc_rec) > 0 && sc_rec.SeriesID == ScSerID) {
						if(!single_sc_id) {
							if(!mult_sc_id_by_phone)
								single_sc_id = oid.Id;
						}
						else {
							single_sc_id = 0;
							mult_sc_id_by_phone = true;
						}
					}
					else if(oid.Obj == PPOBJ_PERSON && PsnObj.Fetch(oid.Id, &psn_rec) > 0) {
						if(!single_psn_id) {
							if(!mult_psn_id_by_phone)
								single_psn_id = oid.Id;
						}
						else {
							single_psn_id = 0;
							mult_psn_id_by_phone = true;
						}
					}
				}
				if(single_sc_id) {
					THROW(ScObj.Search(single_sc_id, &sc_rec) > 0); // @todo @err // Выше мы проверили single_sc_id на "невисячесть"
					THROW(sc_rec.PersonID && PsnObj.Search(sc_rec.PersonID, &psn_rec) > 0); // @todo @err
					rBlk.SCardID = single_sc_id;
					rBlk.PsnID = psn_rec.ID;
					// (еще пароль проверить надо) ok = 1;
				}
				else if(single_psn_id) {
					PPIDArray potential_sc_list;
					THROW(PsnObj.Search(single_psn_id, &psn_rec) > 0); // @todo @err // Выше мы проверили single_psn_id на "невисячесть"
					ScObj.P_Tbl->GetListByPerson(single_psn_id, ScSerID, &potential_sc_list);
					THROW(potential_sc_list.getCount()); // @todo @err
					rBlk.SCardID = potential_sc_list.get(0);
					rBlk.PsnID = single_psn_id;
					text_identification_done = true;
					// (еще пароль проверить надо) ok = 1;
				}
			}
		}
		if(!text_identification_done) {
			if(ScObj.SearchCode(ScSerID, rBlk.LoginText, &sc_rec) > 0) {
				THROW(sc_rec.SeriesID == ScSerID); // @todo @err
				THROW(sc_rec.PersonID && PsnObj.Search(sc_rec.PersonID, &psn_rec) > 0); // @todo @err
				rBlk.SCardID = sc_rec.ID;
				rBlk.PsnID = sc_rec.PersonID;
				text_identification_done = true;
			}
		}
		if(!text_identification_done) {
			PPIDArray kind_list;
			PPIDArray potential_sc_list;
			THROW_PP(PsnKindID, PPERR_WSCTL_ACCPSNKINDUNDEF); // не удалось определить вид персоналий-клиентов для идентификации по имени
			kind_list.add(PsnKindID);
			(temp_buf = rBlk.LoginText).Transf(CTRANSF_UTF8_TO_INNER); // Строки в rBlk в кодировке utf-8
			THROW_PP(PsnObj.SearchFirstByName(temp_buf, &kind_list, 0, &psn_rec) > 0, PPERR_WSCTL_UNABLERECOGNLOGIN);
			ScObj.P_Tbl->GetListByPerson(psn_rec.ID, ScSerID, &potential_sc_list);
			THROW_PP(potential_sc_list.getCount(), PPERR_WSCTL_NAMENOTLINKEDWITHSC);
			rBlk.SCardID = potential_sc_list.get(0);
			rBlk.PsnID = psn_rec.ID;
			text_identification_done = true;
		}
	}
	if(rBlk.SCardID) {
		PPSCardPacket sc_pack;
		SString pw_buf;
		THROW(ScObj.GetPacket(rBlk.SCardID, &sc_pack) > 0); // @todo @err // Выше мы проверили rBlk.SCardID на "невисячесть"
		sc_pack.GetExtStrData(PPSCardPacket::extssPassword, pw_buf);
		if(rBlk.Pw.NotEmpty()) {
			(temp_buf = rBlk.Pw).Transf(CTRANSF_UTF8_TO_INNER);
			THROW_PP(temp_buf == pw_buf, PPERR_INVUSERORPASSW);
		}
		else {
			THROW_PP(pw_buf.IsEmpty(), PPERR_INVUSERORPASSW);
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}
	
int WsCtlSrvBlock::SendClientPolicy(SString & rResult)
{
	rResult.Z();
	int    ok = 1;
	SJson * p_js = 0;
	SString temp_buf;
	PPGetPath(PPPATH_WORKSPACE, temp_buf);
	temp_buf.SetLastSlash().Cat("wsctl").SetLastSlash().Cat("wsctl-policy.json");
	p_js = SJson::ParseFile(temp_buf);
	THROW(p_js);
	{
		WsCtl_ClientPolicy policy;
		THROW(policy.FromJsonObj(p_js));
		p_js->ToStr(rResult);
	}
	CATCHZOK
	return ok;
}

int WsCtlSrvBlock::SendProgramList(SString & rResult)
{
	rResult.Z();
	int    ok = 1;
	SJson * p_js = 0;
	SString temp_buf;
	PPGetPath(PPPATH_WORKSPACE, temp_buf);
	temp_buf.SetLastSlash().Cat("wsctl").SetLastSlash().Cat("wsctl-program.json");
	p_js = SJson::ParseFile(temp_buf);
	THROW(p_js);
	{
		WsCtl_ProgramCollection pgm_list;
		THROW(pgm_list.FromJsonObj(p_js));
		p_js->ToStr(rResult);
	}
	CATCHZOK
	return ok;	
}
//
//
//
WsCtl_ProgramEntry::WsCtl_ProgramEntry()
{
}

WsCtl_ProgramEntry & WsCtl_ProgramEntry::Z()
{
	Title.Z();
	ExeFileName.Z();
	FullResolvedPath.Z();
	PicSymb.Z();
	return *this;
}

bool FASTCALL WsCtl_ProgramEntry::IsEq(const WsCtl_ProgramEntry & rS) const
{
	bool   eq = true;
	if(Category != rS.Category)
		eq = false;
	else if(Title != rS.Title)
		eq = false;
	else if(ExeFileName != rS.ExeFileName)
		eq = false;
	else if(PicSymb != rS.PicSymb)
		eq = false;
	return eq;
}

SJson * WsCtl_ProgramEntry::ToJsonObj(bool withResolvance) const
{
	SJson * p_result = SJson::CreateObj();
	SString temp_buf;
	p_result->InsertString("category", (temp_buf = Category).Escape());
	p_result->InsertString("title", (temp_buf = Title).Escape());
	p_result->InsertString("exefile", (temp_buf = ExeFileName).Escape());
	p_result->InsertString("picsymb", (temp_buf = PicSymb).Escape());
	if(withResolvance) {
		if(FullResolvedPath.NotEmpty()) {
			p_result->InsertString("resolvedpath", (temp_buf = FullResolvedPath).Escape());
		}
	}
	return p_result;
}

int WsCtl_ProgramEntry::FromJsonObj(const SJson * pJsObj)
{
	int    ok = 1;
	THROW(SJson::IsObject(pJsObj));
	{
		const SJson * p_c = pJsObj->FindChildByKey("title");
		if(SJson::IsString(p_c))
			(Title = p_c->Text).Unescape();
		p_c = pJsObj->FindChildByKey("category");
		if(SJson::IsString(p_c))
			(Category = p_c->Text).Unescape();
		p_c = pJsObj->FindChildByKey("exefile");
		if(SJson::IsString(p_c))
			(ExeFileName = p_c->Text).Unescape();
		p_c = pJsObj->FindChildByKey("picsymb");
		if(SJson::IsString(p_c))
			(PicSymb = p_c->Text).Unescape();
		p_c = pJsObj->FindChildByKey("resolvedpath");
		if(SJson::IsString(p_c))
			(FullResolvedPath = p_c->Text).Unescape();
	}		
	CATCHZOK
	return ok;
}

WsCtl_ProgramCollection::WsCtl_ProgramCollection() : TSCollection <WsCtl_ProgramEntry>(), SelectedCatSurrogateId(0), Resolved(false)
{
}

WsCtl_ProgramCollection::WsCtl_ProgramCollection(const WsCtl_ProgramCollection & rS) : TSCollection <WsCtl_ProgramEntry>(), SelectedCatSurrogateId(0), Resolved(false)
{
	Copy(rS);
}

WsCtl_ProgramCollection & FASTCALL WsCtl_ProgramCollection::Copy(const WsCtl_ProgramCollection & rS)
{
	TSCollection_Copy(*this, rS);
	SelectedCatSurrogateId = rS.SelectedCatSurrogateId;
	CatList = rS.CatList;
	Resolved = rS.Resolved;
	return *this;
}

bool FASTCALL WsCtl_ProgramCollection::IsEq(const WsCtl_ProgramCollection & rS) const
{
	bool   eq = true;
	if(!TSCollection_IsEq(this, &rS))
		eq = false;
	return eq;
}
	
SJson * WsCtl_ProgramCollection::ToJsonObj(bool withResolvance) const
{
	SJson * p_result = SJson::CreateObj();
	SJson * p_js_list = SJson::CreateArr();
	if(withResolvance) {
		p_result->InsertBool("resolved", Resolved);
	}
	for(uint i = 0; i < getCount(); i++) {
		const WsCtl_ProgramEntry * p_entry = at(i);
		if(p_entry) {
			SJson * p_js_entry = p_entry->ToJsonObj(withResolvance);
			THROW(p_js_entry);
			p_js_list->InsertChild(p_js_entry);
		}
	}
	p_result->Insert("list", p_js_list);
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

int WsCtl_ProgramCollection::FromJsonObj(const SJson * pJsObj)
{
	int    ok = 1;
	//SString temp_buf;
	THROW(SJson::IsObject(pJsObj));
	{
		const SJson * p_c = pJsObj->FindChildByKey("list");
		if(p_c) {
			THROW(SJson::IsArray(p_c)); // @todo @err
			for(const SJson * p_js_entry = p_c->P_Child; p_js_entry; p_js_entry = p_js_entry->P_Next) {
				if(SJson::IsObject(p_js_entry)) {
					WsCtl_ProgramEntry * p_new_entry = CreateNewItem();
					THROW(p_new_entry->FromJsonObj(p_js_entry));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int WsCtl_ProgramCollection::MakeCatList()
{
	int    ok = -1;
	long   surrogate_id = 0;
	CatList.Z();
	SString temp_buf;
	CatList.Add(catsurrogateidAll, "All"); // @v11.9.1
	for(uint i = 0; i < getCount(); i++) {
		const WsCtl_ProgramEntry * p_entry = at(i);
		if(p_entry) {
			temp_buf = p_entry->Category;
			if(temp_buf.NotEmptyS()) {
				if(!CatList.SearchByTextNcUtf8(temp_buf, 0)) {
					CatList.Add(++surrogate_id, temp_buf);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int WsCtl_ProgramCollection::Resolve(const WsCtl_ClientPolicy & rPolicy)
{
	int    ok = -1;
	if(!IsResolved()) {
		SString temp_buf;
		const bool is_there_app_paths = (rPolicy.SsAppPaths.getCount() > 0);
		if(is_there_app_paths) {
			SFileEntryPool fep;
			SFileEntryPool::Entry fe;
			SString path;
			for(uint i = 0; i < getCount(); i++) {
				WsCtl_ProgramEntry * p_pe = at(i);
				if(p_pe && p_pe->ExeFileName.NotEmpty() && p_pe->FullResolvedPath.IsEmpty()) {
					if(!p_pe->ExeFileName.HasPrefixIAscii("prog")) { // С префиксом prog - фейковые отладочные наименования программ
						bool found = false;
						for(uint ssp = 0; !found && rPolicy.SsAppPaths.get(&ssp, temp_buf);) {
							SFindFileParam ffp(temp_buf, p_pe->ExeFileName);
							SFindFile2(ffp, fep);
							if(fep.GetCount()) {
								for(uint fepidx = 0; !found && fep.Get(fepidx, &fe, &path) > 0; fepidx++) {
									p_pe->FullResolvedPath = path;
									found = true;
								}
							}
						}
					}
				}
			}
		}
		Resolved = true;
		ok = 1;
	}
	return ok;
}

int WsCtl_SessionFrame::Start()
{
	int    ok = 1;
	if(State & stRunning)
		ok = -1;
	else {
		SString sys_user_name;
		THROW(Policy.SysUser.NotEmpty()); // @todo @err
		SSystem::GetUserName_(sys_user_name); // @debug
		//SSystem::UserProfileInfo profile_info;
		THROW(Policy.IsThereSystemImage()); // @todo @err
		{
			SSystem::UserProfileInfo profile_info;
			SPtrHandle h_token = SSystem::Logon(0, Policy.SysUser, Policy.SysPassword, SSystem::logontypeInteractive, &profile_info);
			THROW_SL(h_token);
			H_UserToken = h_token;
			SSystem::GetUserName_(sys_user_name); // @debug
		}
		State |= stRunning;
	}
	CATCHZOK
	return ok;
}

int WsCtl_SessionFrame::Finish()
{
	int    ok = 1;
	if(State & stRunning) {
		for(uint i = 0; i < RunningProcessList.getCount(); i++) {
			Process * p_process = RunningProcessList.at(i);
			if(p_process) {
				::TerminateProcess(p_process->HProcess, _FFFFU);
			}
		}
		RunningProcessList.freeAll();
		{
			if(H_UserToken) {
				::CloseHandle(H_UserToken);
				H_UserToken = 0;
			}
		}
		Policy.RestoreSystemImage(); // @v11.8.12
		State &= ~stRunning;
	}
	else
		ok = -1;
	return ok;
}

int WsCtl_SessionFrame::LaunchProcess(const WsCtl_ProgramEntry * pPe)
{
	int    ok = 0;
	if(Start()) { // В случае, если сессия не запущена - запускаем
		SString temp_buf;
		SlProcess::Result result;
		SlProcess proc;
		proc.SetPath(pPe->FullResolvedPath);
		SFsPath ps(pPe->FullResolvedPath);
		ps.Merge(SFsPath::fDrv|SFsPath::fDir, temp_buf);
		proc.SetWorkingDir(temp_buf);
		proc.SetFlags(SlProcess::fLogonWithProfile);
		if(/*H_UserToken*/false) {
			proc.SetImpersUserToken(H_UserToken);
		}
		else if(Policy.SysUser.NotEmpty()) {
			proc.SetImpersUser(Policy.SysUser, Policy.SysPassword);
		}
		if(proc.Run(&result)) {
			Process * p_rpl_entry = RunningProcessList.CreateNewItem();
			if(p_rpl_entry) {
				*static_cast<SlProcess::Result *>(p_rpl_entry) = result;
				p_rpl_entry->Name = pPe->Title;
				p_rpl_entry->Path = pPe->FullResolvedPath;
			}
			ok = 1;
		}
	}
	return ok;
}
