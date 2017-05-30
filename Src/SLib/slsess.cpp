// SLSESS.CPP
// Copyright (c) A.Sobolev 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2014, 2015, 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <locale.h> // setlocale()
#include <openssl/ssl.h>
// htmlhelp.h {
#define HH_INITIALIZE            0x001C  // Initializes the help system.
#define HH_UNINITIALIZE          0x001D  // Uninitializes the help system.
// } htmlhelp.h
#define USE_OPENSSL_STATIC 
//
//
//
SlExtraProcBlock::SlExtraProcBlock()
{
	Reset();
}

void SlExtraProcBlock::Reset()
{
	F_LoadString = 0;
	F_ExpandString = 0;
	F_CallHelp = 0;
	F_CallCalc = 0;
	F_CallCalendar = 0;
	F_GetGlobalSecureConfig = 0;
	F_GetDefaultEncrKey = 0;
}

void SlExtraProcBlock::Set(const SlExtraProcBlock * pS)
{
	if(pS) {
		F_LoadString = pS->F_LoadString;
		F_ExpandString = pS->F_ExpandString;
		F_CallHelp = pS->F_CallHelp;
		F_CallCalc = pS->F_CallCalc;
		F_CallCalendar = pS->F_CallCalendar;
		F_GetGlobalSecureConfig = pS->F_GetGlobalSecureConfig;
		F_GetDefaultEncrKey = pS->F_GetDefaultEncrKey;
	}
	else
		Reset();
}
//
//
//
SLAPI SlThreadLocalArea::SlThreadLocalArea()
{
	Id = 0;
	LastErr = 0;
	LastOsErr = 0;
	LastSockErr = 0;
	LastCurlErr = 0;
	BinDateFmt_ = DEFAULT_BIN_DATE_FORMAT;
	TxtDateFmt_ = DEFAULT_TXT_DATE_FORMAT;
	{
		LDATE c = getcurdate_();
		DefaultYear_  = c.year();
		DefaultMonth_ = c.month();
	}
	CurrentCp = cpUndef;
	UiFlags = 0;
	UiLanguageId = -1; // @v8.9.10
	// @v9.6.5 memzero(OneCStrBuf, sizeof(OneCStrBuf));
	SAry_OrgFCMP = 0;
	SAry_PtrContainer = 0;
	SAry_SortExtraData = 0;
	FontDc = 0;
	P_Rez = 0;
	Rg.Set(getcurdate_().v ^ getcurtime_().v);
	NextDialogLuPos.Set(-1, -1);
}

SLAPI SlThreadLocalArea::~SlThreadLocalArea()
{
	Destroy();
}

void SLAPI SlThreadLocalArea::Destroy()
{
	ZDELETE(P_Rez);
	if(FontDc)
		::DeleteDC(FontDc);
	RemoveTempFiles();
}

TVRez * SLAPI SlThreadLocalArea::GetRez()
{
	if(!P_Rez) {
		SString name;
		long   PP  = 0x00005050L; // "PP"
		long   EXT = 0x00534552L; // "RES"
		makeExecPathFileName((const char*)&PP, (const char*)&EXT, name);
		if(fileExists(name))
			P_Rez = new TVRez(name, 1);
	}
	return P_Rez;
}

void SLAPI SlThreadLocalArea::SetNextDialogLuPos(int left, int top)
{
	NextDialogLuPos.Set(left, top);
}

TPoint SLAPI SlThreadLocalArea::GetNextDialogLuPos()
{
	TPoint result = NextDialogLuPos;
	NextDialogLuPos.Set(-1, -1);
	return result;
}

HDC SLAPI SlThreadLocalArea::GetFontDC()
{
	if(!FontDc) {
		FontDc = ::CreateCompatibleDC(0);
		if(FontDc && !::SetGraphicsMode(FontDc, GM_ADVANCED)) {
			::DeleteDC(FontDc);
			FontDc = 0;
		}
	}
	return FontDc;
}

int SLAPI SlThreadLocalArea::RegisterTempFileName(const char * pFileName)
{
	return isempty(pFileName) ? -1 : TempFileList.add(pFileName);
}

int SLAPI SlThreadLocalArea::RemoveTempFiles()
{
	SString file_name;
	StringSet temp_list;
	for(uint i = 0; TempFileList.get(&i, file_name);)
		if(!SFile::Remove(file_name))
			temp_list.add(file_name);
	TempFileList = temp_list;
	return 1;
}

SLAPI SlSession::SlSession() : SSys(1), GlobSymbList(256, 0)
{
	Construct();
}

int SLAPI SlSession::Construct()
{
	int    ok = -1;
	//
	// Так как SlSession инстациируется только как глобальный объект, закладываемся на то,
	// что Id изначально равен 0.
	//
	if(Id == 0) {
		SessUuid.Generate(); // @v8.0.2 Генерируем абсолютно уникальный id сессии.
		{
#if (USE_ASMLIB > 0)
			//
			// Перед началом исполнения программы сделаем вызовы функций из библиотеки ASMLIB для того,
			// чтобы они сразу инициализировали внутренние таблицы, зависящие от процессора.
			// Таким образом, мы избежим риска конфликтов при многопоточном исполнении.
			//
			const  size_t S = 128;
			char   temp_buf1[S], temp_buf2[S];
			A_memset(temp_buf1, 0, S);
			A_memset(temp_buf2, 0, S);
			A_memmove(temp_buf1, temp_buf2, S);
			A_memcpy(temp_buf2, temp_buf1, S);
			A_memset(temp_buf2, '1', S/4);
			/* @v9.0.6
			A_strlen(temp_buf2);
			A_strcpy(temp_buf1, temp_buf2);
			A_strcmp(temp_buf1, temp_buf2);
			A_stricmp(temp_buf1, temp_buf2);
			A_strstr(temp_buf1, "11");
			*/
#endif
		}
		Id = 1;
		TlsIdx = -1L;
		LastThread = 0;
		WsaInitCounter = 0;
		StopFlag = 0;
		DragndropObjIdx = 0;
		ExtraProcBlk.Reset();
		/* @v9.1.2 replaced by ExtraProcBlk
		F_LoadString = 0;
		F_ExpandString = 0; // @v9.0.11
		F_CallHelp = 0;
		F_GetGlobalSecureConfig = 0; // @v7.6.7
		*/
		P_StopEvnt = 0;
		HelpCookie = 0;
		UiLanguageId = 0; // @v8.9.10
#ifdef _MT
		TlsIdx = TlsAlloc();
#endif
		InitThread();
		ok = 1;
	}
	return ok;
}

SLAPI SlSession::~SlSession()
{
	if(ExtraProcBlk.F_CallHelp && HelpCookie) {
		ExtraProcBlk.F_CallHelp(0, HH_UNINITIALIZE, HelpCookie);
	}
	ReleaseThread();
#ifdef _MT
	TlsFree(TlsIdx);
	delete P_StopEvnt;
#endif
	for(int i = 0; i < WsaInitCounter; i++)
		WSACleanup();
#ifndef __GENERIC_MAIN_CONDUIT__
	ShutdownGdiplus();
#endif
}

int SLAPI SlSession::CheckStopFlag() const
{
	return BIN(StopFlag);
}

int SLAPI SlSession::Stop()
{
	int    ok = -1;
	if(!StopFlag) {
		ENTER_CRITICAL_SECTION
		if(!StopFlag) {
			StopFlag = 1;
			CALLPTRMEMB(P_StopEvnt, Signal());
			ok = 1;
		}
		LEAVE_CRITICAL_SECTION
	}
	return ok;
}

int SLAPI SlSession::ResetStopState()
{
	int    ok = -1;
	if(StopFlag) {
		ENTER_CRITICAL_SECTION
		if(StopFlag) {
			StopFlag = 0;
			if(P_StopEvnt) {
				P_StopEvnt->Reset();
				ZDELETE(P_StopEvnt);
			}
			ok = 1;
		}
		LEAVE_CRITICAL_SECTION
	}
	return ok;
}

int SLAPI SlSession::Init(const char * pAppName, HINSTANCE hInst)
{
	/*
	{
		//
		// Перед началом исполнения программы сделаем вызовы функций из библиотеки ASMLIB для того,
		// чтобы они сразу инициализировали внутренние таблицы, зависящие от процессора.
		// Таким образом, мы избежим риска конфликтов при многопоточном исполнении.
		//
		const  size_t S = 128;
		char   temp_buf1[S], temp_buf2[S];
		A_memset(temp_buf1, 0, S);
		A_memset(temp_buf2, 0, S);
		A_memmove(temp_buf1, temp_buf2, S);
		A_memcpy(temp_buf2, temp_buf1, S);
		A_memset(temp_buf2, '1', S/4);
		A_strlen(temp_buf2);
		A_strcpy(temp_buf1, temp_buf2);
		A_strcmp(temp_buf1, temp_buf2);
		A_stricmp(temp_buf1, temp_buf2);
		A_strstr(temp_buf1, "11");
	}
	*/
	H_Inst = NZOR(hInst, GetModuleHandle(0));
	char   exe_path[MAXPATH];
	GetModuleFileName(H_Inst, exe_path, sizeof(exe_path));
	ExePath = exe_path;
	AppName = pAppName;
#ifdef _MT
	if(AppName.NotEmpty()) {
		SString n;
		P_StopEvnt = new Evnt(GetStopEventName(n), Evnt::modeCreate);
	}
#endif
	RegisterBIST();
	SFileFormat::Register();
	return 1;
}

void SLAPI SlSession::SetAppName(const char * pAppName)
{
	AppName = pAppName;
#ifdef _MT
	if(!P_StopEvnt && AppName.NotEmpty()) {
		SString n;
		P_StopEvnt = new Evnt(GetStopEventName(n), Evnt::modeCreate);
	}
#endif
}

int SLAPI SlSession::InitWSA()
{
	int    ok = 1;
	ENTER_CRITICAL_SECTION
	WSADATA wsa_data;
	if(WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		ok = (SLibError = SLERR_SOCK_WINSOCK, 0);
	else
		WsaInitCounter++;
	LEAVE_CRITICAL_SECTION
	return ok;
}

typedef int  (*SSL_LIBRARY_INIT_PROC)();
typedef void (*SSL_LOAD_ERROR_STRINGS_PROC)();

/* linker
..\OSF\OPENSSL\lib\libeay32.lib
..\OSF\OPENSSL\lib\ssleay32.lib
*/

int SLAPI SlSession::InitSSL()
{
	int    ok = -1;
	if(SslInitCounter == 0) {
		ENTER_CRITICAL_SECTION
#ifdef USE_OPENSSL_STATIC
		{
			if(SSL_library_init()) {
				SSL_load_error_strings();
				SslInitCounter.Incr();
				ok = 1;
			}
			else
				ok = 0;
		}
#else
		{
			SDynLibrary ssl_lib("ssleay32.dll");
			if(ssl_lib.IsValid()) {
				SSL_LIBRARY_INIT_PROC ssl_init_proc = (SSL_LIBRARY_INIT_PROC)ssl_lib.GetProcAddr("SSL_library_init");
				SSL_LOAD_ERROR_STRINGS_PROC ssl_les_proc = (SSL_LOAD_ERROR_STRINGS_PROC)ssl_lib.GetProcAddr("SSL_load_error_strings");
				if(ssl_init_proc && ssl_les_proc && ssl_init_proc()) {
					ssl_les_proc();
					SslInitCounter.Incr();
					ok = 1;
				}
				else
					ok = 0;
			}
			else
				ok = 0;
		}
#endif // USE_OPENSSL_STATIC
		LEAVE_CRITICAL_SECTION
	}
	return ok;
}

int SLAPI SlSession::InitThread()
{
	SlThreadLocalArea * p_tla = 0;
#ifdef _MT
	p_tla = new SlThreadLocalArea;
	TlsSetValue(TlsIdx, p_tla);
#else
	p_tla = &Tla;
#endif
	p_tla->Id = LastThread.Incr();
	return 1;
}

void SLAPI SlSession::ReleaseThread()
{
#ifdef _MT
	SlThreadLocalArea * p_tla = (SlThreadLocalArea *)TlsGetValue(TlsIdx);
	if(p_tla) {
		TlsSetValue(TlsIdx, 0);
		delete p_tla;
	}
#else
	Tla.Destroy();
#endif
}

void * FASTCALL SGetTls(const long idx)
{
#ifdef NDEBUG
	return TlsGetValue(idx);
#else
	void * p = TlsGetValue(idx);
	if(p)
		return p;
	else {
		assert(0);
		return 0;
	}
#endif
}

SlThreadLocalArea & SLAPI SlSession::GetTLA()
{
#ifdef _MT
	return *(SlThreadLocalArea *)SGetTls(TlsIdx);
#else
	return Tla;
#endif
}

const SlThreadLocalArea & SLAPI SlSession::GetConstTLA() const
{
#ifdef _MT
	return *(SlThreadLocalArea *)SGetTls(TlsIdx);
#else
	return Tla;
#endif
}

int FASTCALL SlSession::SetError(int errCode, const char * pAddedMsg)
{
	const int sock_err = (errCode == SLERR_SOCK_WINSOCK) ? WSAGetLastError() : 0;
	SlThreadLocalArea & r_tla = GetTLA();
	if(&r_tla) {
		//
		// @1 Если глобальный объект SLS разрушается раньше иных глобальных объектов,
		// которые могут вызвать SlSession::SetError, то при завершении процесса может возникнуть исключение
		// обращения к нулевому адресу. Во избежании этого проверяем &r_tla на 0.
		//
		r_tla.LastErr = errCode;
		r_tla.AddedMsgString = pAddedMsg;
		r_tla.LastSockErr = sock_err;
	}
	return 0; // @v8.7.8 1-->0
}

int FASTCALL SlSession::SetError(int errCode)
{
	const int sock_err = (errCode == SLERR_SOCK_WINSOCK) ? WSAGetLastError() : 0;
	SlThreadLocalArea & r_tla = GetTLA();
	if(&r_tla) {
		//
		// see @1 above
		//
		r_tla.LastErr = errCode;
		r_tla.AddedMsgString = 0;
		r_tla.LastSockErr = sock_err;
	}
	return 0; // @v8.7.8 1-->0
}

int FASTCALL SlSession::SetOsError(const char * pAddedMsg)
{
	const int last_err = ::GetLastError();
	SlThreadLocalArea & r_tla = GetTLA();
	if(&r_tla) {
		//
		// see @1 above
		//
		r_tla.LastErr = SLERR_WINDOWS;
		r_tla.LastOsErr = last_err;
		r_tla.AddedMsgString = pAddedMsg;
	}
	return 0; // @v8.7.8 1-->0
}

int SLAPI SlSession::GetOsError() const
	{ return GetConstTLA().LastOsErr; }
const SString & SlSession::GetAddedMsgString() const
	{ return GetConstTLA().AddedMsgString; }
const SString & SLAPI SlSession::GetExePath() const
	{ return ExePath; }
const SString & SLAPI SlSession::GetAppName() const
	{ return AppName; }

int SLAPI SlSession::SetUiLanguageId(int languageId, int currentThreadOnly)
{
	if(currentThreadOnly) {
		GetTLA().UiLanguageId = languageId;
	}
	else {
		ENTER_CRITICAL_SECTION
		UiLanguageId = languageId;
		LEAVE_CRITICAL_SECTION
	}
	return 1;
}

int  SLAPI SlSession::GetUiLanguageId() const
{
	int    lid = GetConstTLA().UiLanguageId;
	return (lid < 0) ? UiLanguageId : lid;
}

void FASTCALL SlSession::SetAddedMsgString(const char * pStr)
	{ GetTLA().AddedMsgString = pStr; }

SString & SlSession::GetStopEventName(SString & rBuf) const
{
	// @v8.1.11 return (rBuf = 0).Cat(AppName).CatChar('_').Cat("Stop").CatChar('_').Cat((long)H_Inst);
	// @v8.1.11 {
	SString temp_buf;
	SessUuid.ToStr(S_GUID::fmtIDL, temp_buf);
	return (rBuf = 0).Cat(AppName).CatChar('_').Cat("Stop").CatChar('_').Cat(temp_buf);
	// } @v8.1.11
}

int SLAPI SlSession::RegisterTempFileName(const char * pFileName)
	{ return GetTLA().RegisterTempFileName(pFileName); }
int SLAPI SlSession::RemoveTempFiles()
	{ return GetTLA().RemoveTempFiles(); }

int SLAPI SlSession::SetLogPath(const char * pPath)
{
	GetTLA().LogPath = pPath;
	return 1;
}

SString & SLAPI SlSession::GetLogPath(SString & rPath) const
{
	rPath = GetConstTLA().LogPath;
	return rPath;
}
//
//
//
struct GlobalObjectEntry {
	void   operator = (SClassWrapper & rCls)
	{
		VT = *(void **)&rCls;
	}
	int    Create()
	{
		if(VT) {
			uint8  stub[32];
			SClassWrapper * p_cls = (SClassWrapper *)stub;
			(*(void **)p_cls) = VT;
			Ptr = p_cls->Create();
		}
		return (Ptr != 0);
	}
	void   Destroy()
	{
		if(VT && Ptr) {
			uint8  stub[32];
			SClassWrapper * p_cls = (SClassWrapper *)stub;
			(*(void **)p_cls) = VT;
			p_cls->Destroy(Ptr);
		}
		Ptr = 0;
	}
	void * VT;
	void * Ptr;
};

SlSession::GlobalObjectArray::GlobalObjectArray() : SArray(sizeof(GlobalObjectEntry))
{
	//
	// Дабы не использовать нулевой индекс вставляем фиктивный первый элемент.
	//
	TSClassWrapper <int> zero_cls;
	GlobalObjectEntry zero_entry;
	zero_entry.VT = *(void **)&zero_cls;
	zero_entry.Ptr = 0;
	insert(&zero_entry);
}

SlSession::GlobalObjectArray::~GlobalObjectArray()
{
	Cs.Enter();
	for(uint i = 1; i < count; i++)
		((GlobalObjectEntry *)at(i))->Destroy();
	Cs.Leave();
}

uint SlSession::GlobalObjectArray::CreateObject(SClassWrapper & rCls)
{
	uint   new_idx = 0;
	assert(count > 0);
	Cs.Enter();
	for(uint i = 1; !new_idx && i < count; i++) {
		GlobalObjectEntry * p_entry = (GlobalObjectEntry *)at(i);
		if(p_entry->Ptr == 0) {
			*p_entry = rCls;
			THROW_S(p_entry->Create(), SLERR_NOMEM);
			new_idx = i;
		}
	}
	if(!new_idx) {
		GlobalObjectEntry new_entry;
		new_entry = rCls;
		THROW_S(new_entry.Create(), SLERR_NOMEM);
		THROW(insert(&new_entry));
		new_idx = getCount()-1;
	}
	CATCH
		new_idx = 0;
	ENDCATCH
	Cs.Leave();
	return new_idx;
}

int SlSession::GlobalObjectArray::DestroyObject(uint idx)
{
	int    ok = 1;
	Cs.Enter();
	if(idx && idx < count)
		((GlobalObjectEntry *)at(idx))->Destroy();
	Cs.Leave();
	return ok;
}

void * FASTCALL SlSession::GlobalObjectArray::GetObject(uint idx)
{
	void * ptr = 0;
	Cs.Enter();
	if(idx && idx < getCount())
		ptr = ((GlobalObjectEntry *)at(idx))->Ptr;
	Cs.Leave();
	if(!ptr) {
		SString temp_buf;
		SLS.SetError(SLERR_GLOBOBJIDXNFOUNT, temp_buf.Cat(idx));
	}
	return ptr;
}

uint SLAPI SlSession::CreateGlobalObject(SClassWrapper & rCls)
	{ return GlobObjList.CreateObject(rCls); }
int SLAPI SlSession::DestroyGlobalObject(uint idx)
	{ return GlobObjList.DestroyObject(idx); }
void * FASTCALL SlSession::GetGlobalObject(uint idx)
	{ return GlobObjList.GetObject(idx); }
int64 SLAPI SlSession::GetSequenceValue()
	{ return SeqValue.Incr(); }

long SLAPI SlSession::GetGlobalSymbolId(const char * pSymb, long ident)
{
	long   _i = 0;
	ENTER_CRITICAL_SECTION
	uint   val = 0;
	if(GlobSymbList.Search(pSymb, &val, 0)) {
		_i = (long)val;
		assert(ident <= 0 || _i == ident);
		if(ident > 0 && _i != ident) {
			_i = 0;
		}
	}
	else if(ident >= 0) {
		val = (uint)NZOR(ident, LastGlobSymbId.Incr());
		if(GlobSymbList.Add(pSymb, val, 0)) {
			_i = (long)val;
		}
	}
	else
		_i = -1;
	LEAVE_CRITICAL_SECTION
	return _i;
}

long  SLAPI SlSession::SetUiFlag(long f, int set)
{
    SlThreadLocalArea & r_tla = GetTLA();
    const long prev_ui_flags = r_tla.UiFlags;
    SETFLAG(r_tla.UiFlags, f, set);
    return prev_ui_flags;
}

int   FASTCALL SlSession::CheckUiFlag(long f) const
{
	return BIN((GetConstTLA().UiFlags & f) == f);
}

struct DdoEntry {
	DdoEntry()
	{
		Type = 0;
		P_Obj = 0;
	}
	int    Type;
	void * P_Obj;
};

int SLAPI SlSession::SetupDragndropObj(int ddoType, void * pObj)
{
	int    ok = 1;
	ENTER_CRITICAL_SECTION
	if(DragndropObjIdx) {
		DestroyGlobalObject(DragndropObjIdx);
		DragndropObjIdx = 0;
	}
	if(ddoType) {
		TSClassWrapper <DdoEntry> ptr_cls;
		DragndropObjIdx = CreateGlobalObject(ptr_cls);
		if(DragndropObjIdx) {
			 DdoEntry * p_item = (DdoEntry *)SLS.GetGlobalObject(DragndropObjIdx);
			 p_item->Type = ddoType;
			 p_item->P_Obj = pObj;
		}
		else
			ok = 0;
	}
	LEAVE_CRITICAL_SECTION
	return ok;
}

int SLAPI SlSession::IsThereDragndropObj(void ** ppObj)
{
	int    type = 0;
	ENTER_CRITICAL_SECTION
	if(DragndropObjIdx) {
		DdoEntry * p_item = (DdoEntry *)SLS.GetGlobalObject(DragndropObjIdx);
		if(p_item) {
			ASSIGN_PTR(ppObj, p_item->P_Obj);
			type = p_item->Type;
		}
	}
	LEAVE_CRITICAL_SECTION
	return type;
}

const SGlobalSecureConfig & SLAPI SlSession::GetGlobalSecureConfig()
{
	SlThreadLocalArea & r_tla = GetTLA();
	SGlobalSecureConfig & r_cfg = r_tla.Gsc;
	if(r_cfg.IsEmpty()) {
		if(ExtraProcBlk.F_GetGlobalSecureConfig) {
			ExtraProcBlk.F_GetGlobalSecureConfig(&r_cfg);
		}
		if(r_cfg.IsEmpty()) {
			SString temp_buf;
			getExecPath(temp_buf).SetLastSlash();
			r_cfg.CaPath = temp_buf;
			r_cfg.CaFile = temp_buf.Cat("cacerts.pem");
		}
	}
	return r_cfg;
}
//
//
//
int SLAPI SlSession::LogMessage(const char * pFileName, const char * pStr, ulong maxFileSize)
{
	int    ok = 1;
	long   current_size = 0;
	FILE * f = 0;
	SString file_name = pFileName;
	SString msg_buf;
	(msg_buf = pStr).CR();
	{
		SPathStruc ps;
		if(file_name.NotEmptyS())
			ps.Split(file_name);
		if(ps.Nam.Empty())
			ps.Nam = "slib";
		if(ps.Ext.Empty())
			ps.Ext = "log";
		if(ps.Drv.Empty() && ps.Dir.Empty()) {
			GetLogPath(file_name);
			if(!file_name.NotEmptyS()) {
				getExecPath(file_name);
			}
			file_name.SetLastSlash().Cat(ps.Nam);
			if(ps.Ext.C(0) != '.')
				file_name.Dot();
			file_name.Cat(ps.Ext);
		}
		else
			ps.Merge(file_name);
	}
	f = fopen(file_name, "r");
	if(f) {
		fseek(f, 0, SEEK_END);
		current_size = ftell(f);
		fclose(f);
		f = 0;
	}
	else {
		f = fopen(file_name, "w");
		if(f)
			fclose(f);
		else
			ok = 0;
	}
	if(ok) {
		ENTER_CRITICAL_SECTION
			int    counter;
			if(maxFileSize && (current_size + msg_buf.Len()) > maxFileSize*1024) {
				counter = 0;
				SString ext, b = file_name;
				do {
					SPathStruc::ReplaceExt(b, (ext = 0).CatLongZ(++counter, 3), 1);
				} while(fileExists(b));
				copyFileByName(file_name, b);
				SFile::Remove(file_name);
			}
			counter = 30;
			do {
				if(!(f = fopen(file_name, "a+"))) {
					if(CheckStopFlag()) {
						ok = 0;
						break;
					}
					Sleep(10);
				}
			} while(!f && --counter);
			if(f) {
				fputs(msg_buf, f);
				fclose(f);
			}
		LEAVE_CRITICAL_SECTION
	}
	return ok;
}

int SLAPI SlSession::LoadString(const char * pSignature, SString & rBuf) const
{
	LoadStringFunc f_ls = ExtraProcBlk.F_LoadString;
	return f_ls ? f_ls(pSignature, rBuf) : 0;
}

int SLAPI SlSession::ExpandString(SString & rBuf, int ctransf) const
{
	ExpandStringFunc f_es = ExtraProcBlk.F_ExpandString;
	return f_es ? f_es(rBuf, ctransf) : 0;
}

void SLAPI SlSession::GetExtraProcBlock(SlExtraProcBlock * pBlk) const
{
	ASSIGN_PTR(pBlk, ExtraProcBlk);
}

void SLAPI SlSession::SetExtraProcBlock(const SlExtraProcBlock * pBlk)
{
	ENTER_CRITICAL_SECTION
	ExtraProcBlk.Set(pBlk);
	LEAVE_CRITICAL_SECTION
}

#if 0 // @v9.1.2 replaced by SetExtraProcBlock() {
int SLAPI SlSession::SetGlobalSecureConfigFunc(GetGlobalSecureConfigFunc fProc)
{
	F_GetGlobalSecureConfig = fProc;
	return 1;
}

int SLAPI SlSession::SetLoadStringFunc(LoadStringFunc fProc)
{
	ENTER_CRITICAL_SECTION
	F_LoadString = fProc;
	LEAVE_CRITICAL_SECTION
	return 1;
}

int SLAPI SlSession::SetExpandStringFunc(ExpandStringFunc fProc)
{
	ENTER_CRITICAL_SECTION
	F_ExpandString = fProc;
	LEAVE_CRITICAL_SECTION
	return 1;
}

int SLAPI SlSession::SetCallHelpFunc(CallHelpFunc fProc)
{
	ENTER_CRITICAL_SECTION
	F_CallHelp = fProc;
	if(F_CallHelp && !HelpCookie) {
		F_CallHelp(0, HH_INITIALIZE, (uint)&HelpCookie);
	}
	LEAVE_CRITICAL_SECTION
	return 1;
}

#endif // } 0 @v9.1.2 replaced by SetExtraProcBlock()

int SLAPI SlSession::CallHelp(uint32 wnd, uint cmd, uint ctx)
{
	return ExtraProcBlk.F_CallHelp ? ExtraProcBlk.F_CallHelp(wnd, cmd, ctx) : 0;
}

int SLAPI SlSession::SubstString(const char * pSrcStr, int ansiCoding, SString & rBuf)
{
	int    ok = -1;
	if(pSrcStr && pSrcStr[0] == '@' && !strchr(pSrcStr, ' ')) {
		SString _text = pSrcStr;
		if(LoadString(_text.ShiftLeft(1), rBuf) > 0) {
			if(ansiCoding)
				rBuf.Transf(CTRANSF_INNER_TO_OUTER);
			ok = 1;
		}
	}
	return ok;
}

int FASTCALL SlSession::SetCodepage(SCodepage cp)
{
	int    ok = 1;
	SlThreadLocalArea & r_tla = GetTLA();
	r_tla.CurrentCp = cp;
	SString cp_text;
	if(r_tla.CurrentCp.ToStr(SCodepageIdent::fmtCLibLocale, cp_text)) {
		setlocale(LC_CTYPE, cp_text);
		setlocale(LC_COLLATE, cp_text);
	}
	return ok;
}

SCodepage SLAPI SlSession::GetCodepage() const
{
	const SlThreadLocalArea & r_tla = GetConstTLA();
	return r_tla.CurrentCp;
}


#pragma warning(disable:4073)
#pragma init_seg(lib)
SlSession SLS; // @global
//
//
//
uint32 FASTCALL RSHash(const void * pData, uint len); // @prototype

void __LinkFile_HASHFUNC()
{
	RSHash("", 0);
}
