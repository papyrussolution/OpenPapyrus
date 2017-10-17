// PPSERVER.CPP
// Copyright (c) A.Sobolev 2005, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <crpe.h> // @v8.6.8
#include <StyloConduit.h>
#include <ppds.h>
//
// Пустая функция, используемая для насильственной линковки данного модуля.
//
int dummy_ppserver()
{
	return 0;
}
//
//
//
//static
int FASTCALL PPThread::GetKindText(int kind, SString & rBuf)
{
	return PPGetSubStr(PPTXT_PPTHREADKINDTITLES, kind, rBuf);
}

SLAPI PPThread::PPThread(int kind, const char * pText, void * pInitData) : SlThread(pInitData)
{
	Kind = kind;
	JobID = 0;
	Text = pText;
	StartMoment = ZERODATETIME;
}

int SLAPI PPThread::GetKind() const
{
	return Kind;
}

int32  SLAPI PPThread::GetUniqueSessID() const
{
	return UniqueSessID;
}

void SLAPI PPThread::Startup()
{
	SlThread::Startup();
	DBS.InitThread();
	DS.InitThread(this);
	UniqueSessID = DS.GetTLA().GetId();
	StartMoment = getcurdatetime_();
}

void SLAPI PPThread::Shutdown()
{
	DS.Logout();
	//
	// @v8.0.6 Изменен порядок вызова. Было: SlThread::Shutdown(); DBS.ReleaseThread(); DS.ReleaseThread();
	//
	DS.ReleaseThread();
	DBS.ReleaseThread();
	SlThread::Shutdown();
}

void FASTCALL PPThread::SetJobID(PPID jobID)
{
	if(Kind == kJob)
		JobID = jobID;
}

void FASTCALL PPThread::SetText(const char * pTxt)
{
	Text = pTxt;
}

void FASTCALL PPThread::SetMessage(const char * pMsg)
{
	LastMsg_ = pMsg;
}

void FASTCALL PPThread::GetInfo(PPThread::Info & rInfo) const
{
	rInfo.Id = GetThreadID();
	rInfo.Kind = Kind;
	rInfo.JobID = (Kind == kJob) ? JobID : 0;
	rInfo.Status = 0;
	if(IsStopping())
		rInfo.Status |= Info::stLocalStop;
	rInfo.StartMoment = StartMoment;
	rInfo.Text = Text;
	rInfo.LastMsg = LastMsg_;
	rInfo.UniqueSessID = UniqueSessID;
}

void FASTCALL PPThread::LockStackToStr(SString & rBuf) const
{
	const SlThreadLocalArea * p_sl_tla = (const SlThreadLocalArea *)SlThread::P_Tla;
	if(p_sl_tla) {
		p_sl_tla->LckStk.ToStr(rBuf);
	}
	else {
		rBuf.Cat("TLA inaccessible");
	}
}

int SLAPI PPThread::Info::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, Id, rBuf));
	THROW_SL(pCtx->Serialize(dir, Kind, rBuf));
	THROW_SL(pCtx->Serialize(dir, Status, rBuf));
	THROW_SL(pCtx->Serialize(dir, JobID, rBuf));
	THROW_SL(pCtx->Serialize(dir, StartMoment, rBuf));
	THROW_SL(pCtx->Serialize(dir, Text, rBuf));
	THROW_SL(pCtx->Serialize(dir, LastMsg, rBuf));
	CATCHZOK
	return ok;
}
//
//
//
SLAPI PPServerCmd::PPServerCmd() : PPJobSrvCmd()
{
	P_SoBlk = 0;
	P_ShT = PPGetStringHash(PPSTR_HASHTOKEN);
	Init();
}

SLAPI PPServerCmd::~PPServerCmd()
{
	ZDELETE(P_SoBlk);
	P_ShT = 0;
}

void SLAPI PPServerCmd::Init()
{
	SessID = 0;
	//Params.Z();
	ClearParams();
	ZDELETE(P_SoBlk);
}

void SLAPI PPServerCmd::ClearParams()
{
	ParamL.Clear();
}

int FASTCALL PPServerCmd::PutParam(int parid, const char * pVal)
{
	int    ok = 1;
	if(parid > 0) {
		if(isempty(pVal)) {
			ParamL.Remove(parid);
			ok = 4;
		}
		else {
			ok = ParamL.Add(parid, pVal, 1/*replaceDup*/);
		}
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL PPServerCmd::GetParam(int parid, SString & rVal) const
{
	return ParamL.Get(parid, rVal);
}

int SLAPI PPServerCmd::GetWord(const char * pBuf, size_t * pPos)
{
	int    ok = 1;
	size_t p = pPos ? *pPos : 0;
	Term.Z();
	while(oneof2(pBuf[p], ' ', '\t'))
		p++;
	if(pBuf[p]) {
		int    q = 0;
		if(pBuf[p] == '\"') {
			p++;
			q = 1;
		}
		/*char*/int c = pBuf[p];
		while(c != 0) {
			if(q && c == '\"') {
				p++;
				break;
			}
			else if(!q && oneof2(c, ' ', '\t')) {
				break;
			}
			else {
				if(q && c == '\\' && pBuf[p+1] == '\"') {
					c = '\"';
					p++;
				}
				Term.CatChar(c);
				c = pBuf[++p];
			}
		}
		while(pBuf[p] == ' ' || pBuf[p] == '\t')
			p++;
	}
	else
		ok = 0;
	ASSIGN_PTR(pPos, p);
	return ok;
}

int SLAPI PPServerCmd::ParseLine(const SString & rLine, long flags)
{
	// PPJobSrvProtocol
	//
	// GETIMAGE GOODS 2017
	// GETTDDO test_tddo
	// GTACHECKIN GTAOP() GLOBALUSER.ID(1006) OBJTYPE(GOODS) OBJID(37980) COUNT(1) DURATION(0:2:14)
	// GETOBJECTTAG TestStringTag 1093
	// SETOBJECTTAG TestStringTag 54320
	//
	enum {
		tNone = 0,
		tLogin = 1,
		tSuspend,
		tResume,
		tStyloBHTII,
		tGetBizScores,
		tCheckGlobalCredential,
		tGetTDDO,
		tGetImage,
		tGetNextFilePart,
		tAckFile,
		tCancelFile,
		tExecViewNF,
		tPreparePalmOutData,
		tPreparePalmInData,
		tGetFile,
		tPutFile,
		tPutNextFilePart,
		tProcessPalmXmlData,
		tSetGlobalUser,

		tSetImageMime,
		tPing,

		tExpTariffTA,
		tSendSMS,
		tResetCache,

		tCPosProcessorCommand,
		tGetDisplayInfo,
		tGetWorkbookContent,
		tSetTxtCmdTerm,
		tGetKeywordSeq,  // @v8.2.9
		tRegisterStylo, // @v8.7.1
		tSoBlock,
		tNoArg,
		tSetWorkbookContent, // @v8.7.7
		tQueryNaturalToken, // @v8.8.12
		tGetArticleByPerson,
		tGetPersonByArticle,
		tLogLockStack // @v9.8.1
	};
	enum {
		cmdfNeedAuth = 0x0001, // Команда требует авторизованного сеанса
		cmdfNoArg    = 0x0002  // Команда без аргументов
	};
	//
	// @v9.6.4 Начиная с этого релиза переводим идентификацию команд на новую технику,
	// использующую хэш-таблицу, определенную в файле определения строк.
	// Пока реализация не стабилизируется будем параллельно поддерживать поле P_Symb в
	// нижеследующей структуре (см assert @1).
	// В дальнейшем от этого рудимента надо будет избавиться.
	//
	struct CmdSymb {
		uint   H;
		const  char * P_Symb;
		int    Tok;
		int    CmdType;
		long   Flags;
	};
	static const CmdSymb symb_list[] = {
		{ PPHS_HELLO                    , "HELLO",                     tNoArg,                   PPSCMD_HELLO,                 cmdfNoArg },
		{ PPHS_HSH                      , "HSH",                       tNoArg,                   PPSCMD_HSH,                   cmdfNoArg },
		{ PPHS_GETLASTERR               , "GETLASTERR",                tNoArg,                   PPSCMD_GETLASTERRMSG,         cmdfNoArg },
		{ PPHS_SETGLOBALUSER            , "SETGLOBALUSER",             tSetGlobalUser,           PPSCMD_SETGLOBALUSER,         cmdfNeedAuth },
		{ PPHS_GETGLOBALUSER            , "GETGLOBALUSER",             tNoArg,                   PPSCMD_GETGLOBALUSER,         cmdfNeedAuth|cmdfNoArg },
		{ PPHS_GETSERVERSTAT            , "GetServerStat",             tNoArg,                   PPSCMD_GETSERVERSTAT,         cmdfNoArg },
		{ PPHS_QUIT                     , "QUIT",                      tNoArg,                   PPSCMD_QUIT,                  cmdfNoArg },
		{ PPHS_LOGOUT                   , "LOGOUT",                    tNoArg,                   PPSCMD_LOGOUT,                cmdfNeedAuth|cmdfNoArg },
		{ PPHS_SPII                     , "SPII",                      tNoArg,                   PPSCMD_SPII,                  cmdfNoArg },
		{ PPHS_STYLOBHT                 , "STYLOBHT",                  tNoArg,                   PPSCMD_STYLOBHT,              cmdfNoArg },
		{ PPHS_LOGIN                    , "LOGIN",                     tLogin,                   PPSCMD_LOGIN,                 0 },
		{ PPHS_SUSPEND                  , "SUSPEND",                   tSuspend,                 PPSCMD_SUSPEND,               0 },
		{ PPHS_RESTORESESS              , "RESTORESESS",               tResume,                  PPSCMD_RESUME,                0 },
		{ PPHS_RESUME                   , "RESUME",                    tResume,                  PPSCMD_RESUME,                0 },
		{ PPHS_STYLOBHTII               , "STYLOBHTII",                tStyloBHTII,              PPSCMD_STYLOBHTII,            0 },
		{ PPHS_GETBIZSCORES             , "GETBIZSCORES",              tGetBizScores,            PPSCMD_GETBIZSCORES,          cmdfNeedAuth },
		{ PPHS_CHECKGLOBALCREDENTIAL    , "CheckGlobalCredential",     tCheckGlobalCredential,   PPSCMD_CHECKGLOBALCREDENTIAL, cmdfNeedAuth },
		{ PPHS_GETTDDO                  , "GETTDDO",                   tGetTDDO,                 PPSCMD_GETTDDO,               0 },
		{ PPHS_GETIMAGE                 , "GETIMAGE",                  tGetImage,                PPSCMD_GETIMAGE,              cmdfNeedAuth },
		{ PPHS_GETNEXTFILEPART          , "GETNEXTFILEPART",           tGetNextFilePart,         PPSCMD_GETNEXTFILEPART,       0 },
		{ PPHS_ACKFILE                  , "ACKFILE",                   tAckFile,                 PPSCMD_ACKFILE,               0 },
		{ PPHS_CANCELFILE               , "CANCELFILE",                tCancelFile,              PPSCMD_CANCELFILE,            0 },
		{ PPHS_EXECVIEW                 , "EXECVIEW",                  tExecViewNF,              PPSCMD_EXECVIEWNF,            cmdfNeedAuth },
		{ PPHS_TESTPAPYRUSSERVER        , "TESTPAPYRUSSERVER",         tNoArg,                   PPSCMD_TEST,                  cmdfNoArg },
		{ PPHS_PREPAREPALMOUTDATA       , "PREPAREPALMOUTDATA",        tPreparePalmOutData,      PPSCMD_PREPAREPALMOUTDATA,    0 },
		{ PPHS_PREPAREPALMINDATA        , "PREPAREPALMINDATA",         tPreparePalmInData,       PPSCMD_PREPAREPALMINDATA,     0 },
		{ PPHS_GETFILE                  , "GETFILE",                   tGetFile,                 PPSCMD_GETFILE,               0 },
		{ PPHS_PUTFILE                  , "PUTFILE",                   tPutFile,                 PPSCMD_PUTFILE,               0 },
		{ PPHS_PUTNEXTFILEPART          , "PUTNEXTFILEPART",           tPutNextFilePart,         PPSCMD_PUTNEXTFILEPART,       0 },
		{ PPHS_PROCESSPALMXMLDATA       , "PROCESSPALMXMLDATA",        tProcessPalmXmlData,      PPSCMD_PROCESSPALMXMLDATA,    0 },
		{ PPHS_SETIMAGEMIME             , "SETIMAGEMIME",              tSetImageMime,            PPSCMD_SETIMAGEMIME,          cmdfNeedAuth },
		{ PPHS_PING                     , "PING",                      tPing,                    PPSCMD_PING,                  0 },
		{ PPHS_SELECT                   , "SELECT",                    tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_SET                      , "SET",                       tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_CREATE                   , "CREATE",                    tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_CCHECKCREATE             , "CCHECKCREATE",              tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_CCHECKADDLINE            , "CCHECKADDLINE",             tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_CCHECKFINISH             , "CCHECKFINISH",              tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_SCARDREST                , "SCARDREST",                 tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_SCARDDEPOSIT             , "SCARDDEPOSIT",              tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_SCARDWITHDRAW            , "SCARDWITHDRAW",             tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_GTACHECKIN               , "GTACHECKIN",                tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_BILLCREATE               , "BILLCREATE",                tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_BILLADDLINE              , "BILLADDLINE",               tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_BILLFINISH               , "BILLFINISH",                tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_SETPERSONREL             , "SETPERSONREL",              tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_GETPERSONREL             , "GETPERSONREL",              tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_SETOBJECTTAG             , "SETOBJECTTAG",              tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_INCOBJECTTAG             , "INCOBJECTTAG",              tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_DECOBJECTTAG             , "DECOBJECTTAG",              tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_GETOBJECTTAG             , "GETOBJECTTAG",              tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_DRAFTTRANSITGOODSREST    , "DRAFTTRANSITGOODSREST",     tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_DRAFTTRANSITGOODSRESTLIST, "DRAFTTRANSITGOODSRESTLIST", tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_GETGOODSMATRIX           , "GETGOODSMATRIX",            tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth },
		{ PPHS_GETOBJATTACHMENTINFO     , "GETOBJATTACHMENTINFO",      tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth }, // @v8.4.1
		{ PPHS_GETTSESSPLACESTATUS      , "GETTSESSPLACESTATUS",       tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth }, // @v8.7.8
		{ PPHS_TSESSCIPCHECKIN          , "TSESSCIPCHECKIN",           tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth }, // @v8.8.2
		{ PPHS_TSESSCIPCANCEL           , "TSESSCIPCANCEL",            tSoBlock,                 PPSCMD_SOBLK,                 cmdfNeedAuth }, // @v8.8.2
		{ PPHS_EXPTARIFFTA              , "EXPTARIFFTA",               tExpTariffTA,             PPSCMD_EXPTARIFFTA,           cmdfNeedAuth },
		{ PPHS_SENDSMS                  , "SENDSMS",                   tSendSMS,                 PPSCMD_SENDSMS,               cmdfNeedAuth },
		{ PPHS_RESETCACHE               , "RESETCACHE",                tResetCache,              PPSCMD_RESETCACHE,            cmdfNeedAuth },
		{ PPHS_LOGLOCKSTACK             , "LOGLOCKSTACK",              tLogLockStack,            PPSCMD_LOGLOCKSTACK,          0            }, // @v9.8.1
		{ PPHS_CPOSINIT                 , "CPOSINIT",                  tCPosProcessorCommand,    PPSCMD_POS_INIT,              cmdfNeedAuth },
		{ PPHS_CPOSRELEASE              , "CPOSRELEASE",               tCPosProcessorCommand,    PPSCMD_POS_RELEASE,           cmdfNeedAuth },
		{ PPHS_CPOSGETCTABLELIST        , "CPOSGETCTABLELIST",         tCPosProcessorCommand,    PPSCMD_POS_GETCTABLELIST,     cmdfNeedAuth },
		{ PPHS_CPOSGETCCHECKLIST        , "CPOSGETCCHECKLIST",         tCPosProcessorCommand,    PPSCMD_POS_GETCCHECKLIST,     cmdfNeedAuth },
		{ PPHS_CPOSGETCCHECKLNCOUNT     , "CPOSGETCCHECKLNCOUNT",      tCPosProcessorCommand,    PPSCMD_POS_GETCCHECKLNCOUNT,  cmdfNeedAuth },
		{ PPHS_CPOSADDCCHECKLINE        , "CPOSADDCCHECKLINE",         tCPosProcessorCommand,    PPSCMD_POS_ADDCCHECKLINE,     cmdfNeedAuth },
		{ PPHS_CPOSRMVCCHECKLINE        , "CPOSRMVCCHECKLINE",         tCPosProcessorCommand,    PPSCMD_POS_RMVCCHECKLINE,     cmdfNeedAuth },
		{ PPHS_CPOSCLEARCCHECK          , "CPOSCLEARCCHECK",           tCPosProcessorCommand,    PPSCMD_POS_CLEARCCHECK,       cmdfNeedAuth },
		{ PPHS_CPOSRMVCCHECK            , "CPOSRMVCCHECK",             tCPosProcessorCommand,    PPSCMD_POS_RMVCCHECK,         cmdfNeedAuth },
		{ PPHS_CPOSPRINTCCHECK          , "CPOSPRINTCCHECK",           tCPosProcessorCommand,    PPSCMD_POS_PRINTCCHECK,       cmdfNeedAuth },
		{ PPHS_CPOSPRINTCCHECKLOCAL     , "CPOSPRINTCCHECKLOCAL",      tCPosProcessorCommand,    PPSCMD_POS_PRINTCCHECKLOCAL,  cmdfNeedAuth },
		{ PPHS_CPOSGETCFG               , "CPOSGETCFG",                tCPosProcessorCommand,    PPSCMD_POS_GETCONFIG,         cmdfNeedAuth },
		{ PPHS_CPOSSETCFG               , "CPOSSETCFG",                tCPosProcessorCommand,    PPSCMD_POS_SETCONFIG,         cmdfNeedAuth },
		{ PPHS_CPOSGETSTATE             , "CPOSGETSTATE",              tCPosProcessorCommand,    PPSCMD_POS_GETSTATE,          cmdfNeedAuth },
		{ PPHS_CPOSSELECTCCHECK         , "CPOSSELECTCCHECK",          tCPosProcessorCommand,    PPSCMD_POS_SELECTCCHECK,      cmdfNeedAuth },
		{ PPHS_CPOSSUSPENDCCHECK        , "CPOSSUSPENDCCHECK",         tCPosProcessorCommand,    PPSCMD_POS_SUSPENDCCHECK,     cmdfNeedAuth },
		{ PPHS_CPOSSELECTTBL            , "CPOSSELECTTBL",             tCPosProcessorCommand,    PPSCMD_POS_SELECTCTABLE,      cmdfNeedAuth },
		{ PPHS_CPOSGETRIGHTS            , "CPOSGETRIGHTS",             tCPosProcessorCommand,    PPSCMD_POS_GETCPOSRIGHTS,     cmdfNeedAuth },
		{ PPHS_CPOSDECRYPTAUTHDATA      , "CPOSDECRYPTAUTHDATA",       tCPosProcessorCommand,    PPSCMD_POS_DECRYPTAUTHDATA,   cmdfNeedAuth },
		{ PPHS_CPOSGETGOODSPRICE        , "CPOSGETGOODSPRICE",         tCPosProcessorCommand,    PPSCMD_POS_GETGOODSPRICE,     cmdfNeedAuth },    // @v8.6.8
		{ PPHS_CPOSGETCURRENTSTATE      , "CPOSGETCURRENTSTATE",       tCPosProcessorCommand,    PPSCMD_POS_GETCURRENTSTATE,   cmdfNeedAuth },  // @v8.6.10
		{ PPHS_CPOSRESETCURRENTLINE     , "CPOSRESETCURRENTLINE",      tCPosProcessorCommand,    PPSCMD_POS_RESETCURRENTLINE,  cmdfNeedAuth }, // @v8.6.10
		{ PPHS_CPOSPROCESSBARCODE       , "CPOSPROCESSBARCODE",        tCPosProcessorCommand,    PPSCMD_POS_PROCESSBARCODE,    cmdfNeedAuth },   // @v8.6.10
		{ PPHS_CPOSSETCCROWQUEUE        , "CPOSSETCCROWQUEUE",         tCPosProcessorCommand,    PPSCMD_POS_CPOSSETCCROWQUEUE, cmdfNeedAuth }, // @v8.6.12
		{ PPHS_CPOSGETMODIFLIST         , "CPOSGETMODIFLIST",          tCPosProcessorCommand,    PPSCMD_POS_GETMODIFLIST,      cmdfNeedAuth },      // @v8.8.10
		{ PPHS_GETDISPLAYINFO           , "GETDISPLAYINFO",            tGetDisplayInfo,          PPSCMD_GETDISPLAYINFO,        cmdfNeedAuth },
		{ PPHS_GETWORKBOOKCONTENT       , "GETWORKBOOKCONTENT",        tGetWorkbookContent,      PPSCMD_GETWORKBOOKCONTENT,    cmdfNeedAuth },
		{ PPHS_SETTXTCMDTERM            , "SETTXTCMDTERM",             tSetTxtCmdTerm,           PPSCMD_SETTXTCMDTERM,                    0 },
		{ PPHS_GETKEYWORDSEQ            , "GETKEYWORDSEQ",             tGetKeywordSeq,           PPSCMD_GETKEYWORDSEQ,         cmdfNeedAuth }, // @v8.2.9
		{ PPHS_REGISTERSTYLO            , "REGISTERSTYLO",             tRegisterStylo,           PPSCMD_REGISTERSTYLO,         cmdfNeedAuth }, // @v8.7.1
		{ PPHS_SETWORKBOOKCOTENT        , "SETWORKBOOKCONTENT",        tSetWorkbookContent,      PPSCMD_SETWORKBOOKCONTENT,    cmdfNeedAuth }, // @v8.7.2
		{ PPHS_QUERYNATURALTOKEN        , "QUERYNATURALTOKEN",         tQueryNaturalToken,       PPSCMD_QUERYNATURALTOKEN,     cmdfNeedAuth }, // @v8.8.12
		{ PPHS_GETARTICLEBYPERSON       , "GETARTICLEBYPERSON",        tGetArticleByPerson,      PPSCMD_GETARTICLEBYPERSON,    cmdfNeedAuth }, // @v8.9.0
		{ PPHS_GETPERSONBYARTICLE       , "GETPERSONBYARTICLE",        tGetPersonByArticle,      PPSCMD_GETPERSONBYARTICLE,    cmdfNeedAuth } // @v8.9.0
	};
	int    ok = 1;
	size_t p = 0;
	SString temp_buf, name, pw, db_symb;
	int    err = 0;
	H.Type = 0;
	int    tok = 0;
	uint   i;
	uint   _u_hs = 0; // @v9.6.4
	long   cmd_flags = 0;
	{
		// @v9.8.0 Params.Z();
		ClearParams(); // @v9.8.0
	}
    THROW(GetWord(rLine, &p));
	Term.ToLower(); // @v9.6.4
	CALLPTRMEMB(P_ShT, Search(Term, &_u_hs, 0)); // @v9.6.4
	for(i = 0; i < SIZEOFARRAY(symb_list); i++) {
		// @v9.6.4 if(Term.CmpNC(symb_list[i].P_Symb) == 0) {
		if(symb_list[i].H == _u_hs) { // @v9.6.4
			assert(sstreqi_ascii(symb_list[i].P_Symb, Term)); // @1
			tok = symb_list[i].Tok;
			H.Type = symb_list[i].CmdType;
			cmd_flags = symb_list[i].Flags;
			break;
		}
	}
	// @v8.7.4 (Не уверен, но похоже эта строка генерирует неприятные побочные эффекты) THROW(!(cmd_flags & cmdfNeedAuth) || DS.GetConstTLA().IsAuth());
	// @v9.0.10 THROW_PP(!(cmd_flags & cmdfNeedAuth) || (flags & plfLoggedIn), PPERR_NOTLOGGEDIN); // @v9.0.9
	switch(tok) {
		case tNoArg:
			break;
		case tPing:
			for(i = 1; GetWord(rLine, &p); i++) {
				PutParam(i, Term); // PPPutExtStrData(i, Params, Term);
			}
			break;
		case tSetTxtCmdTerm:
			if(GetWord(rLine, &p)) {
				PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
			}
			break;
		case tLogin:
			if(GetWord(rLine, &p)) {
				db_symb = Term;
				if(GetWord(rLine, &p)) {
					name = Term;
					if(GetWord(rLine, &p))
						pw = Term;
				}
				else
					ok = 0;
			}
			else
				ok = 0;
			PutParam(1, db_symb); // PPPutExtStrData(1, Params, db_symb);
			PutParam(2, name); // PPPutExtStrData(2, Params, name);
			PutParam(3, pw); // PPPutExtStrData(3, Params, pw);
			break;
		case tSuspend:
			if(GetWord(rLine, &p)) {
				name = Term;
				PutParam(1, name); // PPPutExtStrData(1, Params, name);
			}
			break;
		case tResume:
			if(GetWord(rLine, &p))
				name = Term;
			else {
				name = 0;
				ok = 0;
			}
			PutParam(1, name); // PPPutExtStrData(1, Params, name);
			break;
		case tResetCache:
			ok = 0;
			name = 0;
			temp_buf.Z();
			if(GetWord(rLine, &p)) {
				name = Term;
				if(GetWord(rLine, &p))
					temp_buf = Term;
				ok = 1;
			}
			PutParam(1, name); // PPPutExtStrData(1, Params, name);
			PutParam(2, temp_buf); // PPPutExtStrData(2, Params, temp_buf);
			break;
		case tLogLockStack:
			break;
		case tStyloBHTII:
			ok = 0;
			if(GetWord(rLine, &p)) {
				db_symb = Term;
				if(GetWord(rLine, &p)) {
					name = Term;
					if(GetWord(rLine, &p)) {
						pw = Term;
						ok = 1;
					}
				}
			}
			PutParam(1, db_symb); // PPPutExtStrData(1, Params, db_symb);
			PutParam(2, name); // PPPutExtStrData(2, Params, name);
			PutParam(3, pw); // PPPutExtStrData(3, Params, pw);
			break;
		case tGetBizScores:
			if(GetWord(rLine, &p)) {
				name = Term;
				if(GetWord(rLine, &p))
					pw = Term;
				else
					ok = 0;
			}
			else
				ok = 0;
			PutParam(1, name); // PPPutExtStrData(1, Params, name);
			PutParam(2, pw); // PPPutExtStrData(2, Params, pw);
			break;
		case tCheckGlobalCredential:
			if(GetWord(rLine, &p)) {
				name = Term;
				if(GetWord(rLine, &p))
					pw = Term;
				else
					ok = 0;
			}
			else
				ok = 0;
			PutParam(1, name); // PPPutExtStrData(1, Params, name);
			PutParam(2, pw); // PPPutExtStrData(2, Params, pw);
			break;
		case tGetTDDO:
			if(GetWord(rLine, &p)) {
				int    fld_n = 0;
				PutParam(++fld_n, Term); // PPPutExtStrData(++fld_n, Params, Term);
				if(Term.CmpNC("INLINE") == 0) {
					// inline код
					(temp_buf = rLine).ShiftLeft(p);
					PutParam(++fld_n, temp_buf); // PPPutExtStrData(++fld_n, Params, temp_buf);
				}
				else {
					//
					while(GetWord(rLine, &p)) {
						PutParam(++fld_n, Term); // PPPutExtStrData(++fld_n, Params, Term);
					}
				}
			}
			else
				ok = 0;
			break;
		case tGetImage:
			THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_OBJTYPE, rLine);
			PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
			THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_OBJID, rLine);
			PutParam(2, Term); // PPPutExtStrData(2, Params, Term);
			break;
		case tSetImageMime:
			// SETIMAGEMIME goods 52103 updateFlags ContentType ContentMime64
			{
				THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_OBJTYPE, rLine); // ObjTypeSymb
				PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
				THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_OBJID, rLine); // ObjID
				PutParam(2, Term); // PPPutExtStrData(2, Params, Term);
				THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_UPDATEFLAGS, rLine); // UpdateFlags
				PutParam(3, Term); // PPPutExtStrData(3, Params, Term);
				THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_CONTENTTYPE, rLine); // ContentType
				PutParam(4, Term); // PPPutExtStrData(4, Params, Term);
				THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_CONTENTMIME, rLine); // ContentMime64
				PutParam(5, Term); // PPPutExtStrData(5, Params, Term);
			}
			break;
		case tGetFile:
		case tPutFile:
		case tProcessPalmXmlData:
			if(GetWord(rLine, &p)) {
				//
				// Для путей имеющих символ ' ' (пробел)
				//
				SString path;
				path = Term;
				while(GetWord(rLine, &p))
					path.Space().Cat(Term);
				PutParam(1, path); // PPPutExtStrData(1, Params, path);
				if(GetWord(rLine, &p)) {
					PutParam(2, temp_buf = Term); // PPPutExtStrData(2, Params, temp_buf = Term);
					if(GetWord(rLine, &p)) {
						PutParam(3, temp_buf = Term); // PPPutExtStrData(3, Params, temp_buf = Term);
					}
				}
			}
			else
				err = PPERR_JOBSRV_ARG_FILEPATH;
			break;
		case tPutNextFilePart:
		case tGetNextFilePart:
		case tAckFile:
		case tCancelFile:
			THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_FILECOOKIE, rLine);
			PutParam(1, Term); // PPPutExtStrData(1, Params, Term); // cookie
			break;
		case tExecViewNF:
			THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_NAMEDFILT, rLine); // символ именованного фильтра
			PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
			if(GetWord(rLine, &p)) // [наименование структуры DL600 для экспорта]
				PutParam(2, Term); // PPPutExtStrData(2, Params, Term);
			break;
		case tSoBlock:
			SETIFZ(P_SoBlk, new SelectObjectBlock);
			ok = P_SoBlk ? P_SoBlk->Parse(rLine) : PPSetErrorNoMem();
			break;
		case tCPosProcessorCommand:
			PutParam(1, rLine); // PPPutExtStrData(1, Params, rLine);
			break;
		case tPreparePalmInData:
			THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_STYLONAME, rLine); // device name
			PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
			if(GetWord(rLine, &p)) // UUID устройства
				PutParam(2, Term); // PPPutExtStrData(2, Params, Term);
			break;
		case tPreparePalmOutData:
			THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_STYLONAME, rLine); // device name
			PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
			if(GetWord(rLine, &p)) { // UUID устройства
				PutParam(2, Term); // PPPutExtStrData(2, Params, Term);
				if(GetWord(rLine, &p)) { // Дата последнего обмена
					PutParam(3, Term); // PPPutExtStrData(3, Params, Term);
					if(GetWord(rLine, &p)) { // Время последнего обмена
						PutParam(4, Term); // PPPutExtStrData(4, Params, Term);
					}
				}
			}
			break;
		case tSetGlobalUser:
			// SETGLOBALUSER name
			THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_GLOBALUSERNAME, rLine); // Имя глобальной учетной записи
			PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
			break;
		case tExpTariffTA:
			break;
		case tSendSMS:
			if(GetWord(rLine, &p)) { // Номер телефона
				PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
				if(GetWord(rLine, &p)) { // От кого
					PutParam(2, Term); // PPPutExtStrData(2, Params, Term);
					if(GetWord(rLine, &p)) // Сообщение (зашифрованное в Mime64)
						PutParam(3, Term); // PPPutExtStrData(3, Params, Term);
				}
			}
			break;
		case tGetDisplayInfo:
			if(GetWord(rLine, &p)) // ид устройства из DisplayList
				PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
			break;
		case tGetWorkbookContent:
			if(GetWord(rLine, &p)) // ид контента
				PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
			break;
		case tSetWorkbookContent:
			break;
		case tGetKeywordSeq:
			if(GetWord(rLine, &p)) // контекст
				PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
			break;
		case tRegisterStylo:
			THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_STYLONAME, rLine); // device name
			PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
			THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_MAGICSYMBOL, rLine); // magic symbol
			PutParam(2, Term); // PPPutExtStrData(2, Params, Term);
			break;
		case tQueryNaturalToken:
			THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_NATURALTOKEN, rLine); // Необходимый параметр - собственно запрашиваемый токен
			PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
			{
				uint   arg_n = 1;
				while(GetWord(rLine, &p)) {
					PutParam(++arg_n, Term); // PPPutExtStrData(++arg_n, Params, Term);
				}
			}
			break;
		case tGetArticleByPerson:
			THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_ARBYPSNID, rLine); // Необходимый параметр - идентификатор персоналии
			PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
			if(GetWord(rLine, &p)) {
				PutParam(2, Term); // PPPutExtStrData(2, Params, Term); // Идентификация таблицы статей
			}
			break;
		case tGetPersonByArticle:
			THROW_PP_S(GetWord(rLine, &p), PPERR_JOBSRV_ARG_PSNBYARID, rLine); // Необходимый параметр - идентификатор статьи
			PutParam(1, Term); // PPPutExtStrData(1, Params, Term);
			break;
		default:
			err = PPERR_INVSERVERCMD;
			break;
	}
	if(err)
		ok = PPSetError(err, rLine);
	CATCHZOK
	return ok;
}
//
//
//
class PPJobSession : public PPThread {
public:
	SLAPI  PPJobSession(PPJob * p, const PPJobPool & rPool) : PPThread(PPThread::kJob, p->Name, p)
	{
		SetJobID(p->ID);
		P_Pool = &rPool;
		PPGetFilePath(PPPATH_LOG, PPFILNAM_JOB_LOG, _LogFileName); // @v8.6.1 PPFILNAM_SERVER_LOG-->PPFILNAM_JOB_LOG
		InitStartupSignal();
	}
private:
	virtual void SLAPI Startup();
	virtual void SLAPI Run();
	int    SLAPI DoJob(PPJobMngr * pMngr, PPJob * pJob);
	int    SLAPI MailNotify(const char * pTmpLogFileName);

	SString _LogFileName;
	PPJob  Job;
	const PPJobPool * P_Pool;
};

void SLAPI PPJobSession::Startup()
{
	PPThread::Startup();
	Job = *(PPJob *)P_InitData;
	SignalStartup();
}

void SLAPI PPJobSession::Run()
{
	int    ok = 1;
	int    debug_r = 0;
	int    is_logged_in = 0, heap_corrupted = 0;
	char   secret[64];
	PPJobMngr mngr;
	//
		const PPThreadLocalArea & r_tla = DS.GetConstTLA();
		assert((&r_tla) != 0);
	//
	if(!(Job.Descr.Flags & PPJobDescr::fNoLogin)) {
		PPVersionInfo vi = DS.GetVersionInfo();
		debug_r = 1;
		THROW(vi.GetSecret(secret, sizeof(secret)));
		debug_r = 2;
		THROW(DS.Login(Job.DbSymb, PPSession::P_JobLogin, secret));
		memzero(secret, sizeof(secret));
		is_logged_in = 1;
	}
	debug_r = 3;
	THROW(DoJob(&mngr, &Job));
	CATCH
		PPError();
	ENDCATCH
	memzero(secret, sizeof(secret));
	if(is_logged_in)
		DS.Logout();
}
//
// @v5.8 ANDREW
// @used PPJobSession::DoJob()
static void SendMailCallback(const IterCounter & bytesCounter, const IterCounter & msgCounter)
{
	SString msg;
	PPLoadText(PPTXT_SENDMAILWAITMSG, msg);
	if(msgCounter.GetTotal() > 1)
		msg.Space().Cat(msgCounter).CatChar('/').Cat(msgCounter.GetTotal());
	PPWaitPercent(bytesCounter, msg);
}

int SLAPI PPJobSession::MailNotify(const char * pTmpLogFileName)
{
	int    ok = 1;
	PPDeclStruc	* p_ds = 0;
	SString temp_buf, msg_buf;
	PPInternetAccount inet_acc;
	if(Job.EmailAccID && DS.GetConstTLA().State & PPThreadLocalArea::stAuth) {
		PPObjInternetAccount ia_obj;
		if(ia_obj.Get(Job.EmailAccID, &inet_acc) <= 0)
			inet_acc.ID = 0; // @paranoic
	}
	if(!inet_acc.ID) {
		//
		// определить email аккаунт с помощью charry
		//
		PPDeclStrucProcessor ds_prc;
		PPGetPath(PPPATH_BIN, temp_buf);
		temp_buf.SetLastSlash().Cat("mailacc.txt");
		if(ds_prc.InitDataParsing(temp_buf) > 0 && ds_prc.NextDecl(&p_ds, 1)) {
			inet_acc = ((PPDS_CrrMailAccount*)p_ds)->Data;
			inet_acc.ID = -1; // Главное, что бы не 0
		}
	}
	if(inet_acc.ID) {
		SMailMessage	mail_msg;
		IterCounter	mail_counter;
		uint   line_count = 0; // Количество строк считанных из временного файла журналов
		//
		// сгенерировать текст сообщения //
		//
		inet_acc.GetExtField(MAEXSTR_FROMADDRESS, temp_buf);
		mail_msg.SetField(SMailMessage::fldFrom, temp_buf);

		Job.GetExtStrData(Job.extssEMailSubj, temp_buf);
		if(!temp_buf.NotEmptyS())
			PPLoadText(PPTXT_JOBSRV_MAILNOT_JOB, temp_buf);
		mail_msg.SetField(SMailMessage::fldSubj, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
		if(!isempty(pTmpLogFileName)) {
			//
			// тело сообщения взять из временного файла
			//
			SFile tmp_log_file(pTmpLogFileName, SFile::mRead);
			while(tmp_log_file.ReadLine(temp_buf)) {
				msg_buf.Cat(temp_buf);
				line_count++;
			}
			msg_buf.ToUtf8(); // @v7.6.4
			mail_msg.SetField(SMailMessage::fldText, msg_buf);
		}
		if(!(Job.Flags & Job.fSkipEmptyNotification) || line_count) {
			SString subscribed_list_buff, addr_line;
			Job.GetExtStrData(Job.extssEMailAddrList, subscribed_list_buff);
			if(!subscribed_list_buff.NotEmptyS()) {
				//
				// получить список рассылки из "pp.ini"
				//
				PPIniFile pp_ini_file;
				THROW_PP(pp_ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_JOBSRV_NOTIFY, subscribed_list_buff), PPERR_JOBSRV_MAILLISTMISS);
			}
			{
				StringSet ss(';', subscribed_list_buff);
				if(ss.getCount()) {
					for(uint p = 0; ss.get(&p, temp_buf);) {
						//
						// @todo validate email address
						//
						if(temp_buf.Strip().Len() > 1) {
							if(addr_line.NotEmpty())
								addr_line.CatDiv(',', 2);
							addr_line.Cat(temp_buf);
						}
					}
					mail_msg.SetField(SMailMessage::fldTo, addr_line);
					mail_counter.Init(1);
					if(!PPMailSmtp::Send(inet_acc, mail_msg, SendMailCallback, mail_counter))
						PPError();
				}
			}
		}
	}
	CATCHZOK
	delete p_ds;
	return ok;
}

int SLAPI PPJobSession::DoJob(PPJobMngr * pMngr, PPJob * pJob)
{
	int    ok = 1;
	SString tmp_log_fpath, fmt_buf, msg_buf;
	PPJob  inner_job;
	THROW_INVARG(pMngr && pJob);
	if(pJob->NextJobID) {
		//
		// Тестируем цепочку задач на отсутствие рекурсии
		// @note: Вероятно, рекурсия в цепочке задач может быть полезным инструментом, но пока запретим ее.
		//
		LongArray recur_list;
		for(PPJob * p_job = pJob; p_job != 0;) {
			if(recur_list.addUnique(p_job->ID) > 0) {
				const PPID next_job_id = p_job->NextJobID;
				if(next_job_id && P_Pool) {
					const PPJob * p_temp_job = P_Pool->GetJob(next_job_id);
					if(p_temp_job) {
						inner_job = *p_temp_job;
						p_job = &inner_job;
					}
					else {
						PPSetError(PPERR_JOBNEXTHANGED, p_job->Name);
						PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_LASTERR);
						p_job = 0;
					}
				}
				else
					p_job = 0;
			}
			else {
				PPSetError(PPERR_JOBRECURDETECTED, p_job->Name);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_LASTERR);
				p_job = 0; // @loopexit
			}
		}
	}
	{
		PPJob * p_job = pJob;
		if(p_job->Flags & PPJob::fDisable) {
			msg_buf.Printf(PPLoadTextS(PPTXT_JOBSKIPPED, fmt_buf), p_job->Descr.Text.cptr(), p_job->Name.cptr(), p_job->DbSymb.cptr());
			PPLogMessage(_LogFileName, msg_buf, LOGMSGF_TIME|LOGMSGF_THREADID);
		}
		else {
			long   job_info_id = 0;
			long   logmsg_flags = LOGMSGF_TIME|LOGMSGF_THREADID|LOGMSGF_THREADINFO;
			if(p_job->Flags & PPJob::fNotifyByMail) {
				PPMakeTempFileName("JSL", "TMP", 0, tmp_log_fpath);
				DS.SetTempLogFileName(tmp_log_fpath);
				if(p_job->Flags & PPJob::fSkipEmptyNotification)
					logmsg_flags |= LOGMSGF_NODUPFORJOB;
			}
			msg_buf.Printf(PPLoadTextS(PPTXT_JOBRUNNED, fmt_buf), p_job->Descr.Text.cptr(), p_job->Name.cptr(), p_job->DbSymb.cptr());
			PPLogMessage(_LogFileName, msg_buf, logmsg_flags);
			//
			{
				const uint64 tm_start = SLS.GetProfileTime();
				ok = pMngr->DoJob(p_job->Descr.CmdID, &p_job->Param);
				const uint64 tm_finish = SLS.GetProfileTime();
				msg_buf.Printf(PPLoadTextS(PPTXT_JOBFINISHED, fmt_buf), p_job->Descr.Text.cptr(), p_job->Name.cptr(),
					p_job->DbSymb.cptr(), (int64)(tm_finish-tm_start));
				PPLogMessage(_LogFileName, msg_buf, logmsg_flags);
			}
			if(p_job->Flags & PPJob::fNotifyByMail) {
				MailNotify(tmp_log_fpath);
				DS.SetTempLogFileName(0);
				SFile::Remove(tmp_log_fpath);
			}
			// @v8.6.1 THROW(ok);
			{
				const PPID next_job_id = p_job->NextJobID;
				if(next_job_id) {
					if(ok) {
						if(P_Pool) {
							const PPJob * p_temp_job = P_Pool->GetJob(next_job_id);
							THROW_PP_S(p_temp_job, PPERR_JOBNEXTHANGED, p_job->Name);
							{
								msg_buf.Printf(PPLoadTextS(PPTXT_JOBNEXTLAUNCH, fmt_buf), p_temp_job->Name.cptr());
								PPLogMessage(_LogFileName, msg_buf, LOGMSGF_TIME|LOGMSGF_THREADID);
								//
								PPJob job = *p_temp_job;
								PPJobSession * p_sess = new PPJobSession(&job, *P_Pool);
								p_sess->Start(1);
							}
						}
					}
					else {
						msg_buf.Printf(PPLoadTextS(PPTXT_JOBNEXTNLAUNCHPREVNOK, fmt_buf), ok);
						PPLogMessage(_LogFileName, msg_buf, LOGMSGF_TIME|LOGMSGF_THREADID);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
#define JOB_MAX_STAT_ENTRIES 5

class PPJobServer : public PPThread {
public:
	SLAPI  PPJobServer();
	SLAPI ~PPJobServer();
private:
	struct StatItem {
		long   JobID;
		LDATETIME LastRunningTime;
		long   Count;       // Общее количество запусков
		long   AvgDuration; // Средняя продолжительность (ms)
	};

	virtual void SLAPI Run();

	int    SLAPI GetLastStat(PPID jobID, uint * pPos, StatItem * pItem, uint * pCount) const;
	int    SLAPI RemoveOldStat(PPID jobID, uint countToRemove);
	int    SLAPI AddStat(PPID jobID, const LDATETIME & lastTime, long duration);
	int    SLAPI SaveStat();
	int    SLAPI LoadStat();
	int    SLAPI Arrange(PPJobPool * pPool, LAssocArray * pPlan, PPIDArray * pOnStartUpList, LDATE * pPlanDate);

	PPJobMngr Mngr;
	SString StatFilePath;
	SString LogFileName;
	SArray * P_Stat;
};

SLAPI PPJobServer::PPJobServer() : PPThread(PPThread::kJobServer, 0, 0)
{
	P_Stat = 0;
	PPGetFilePath(PPPATH_LOCAL, PPFILNAM_JOBSTAT,  StatFilePath);
	PPGetFilePath(PPPATH_LOG,   PPFILNAM_SERVER_LOG, LogFileName);
}

SLAPI PPJobServer::~PPJobServer()
{
	delete P_Stat;
}

int SLAPI PPJobServer::GetLastStat(PPID jobID, uint * pPos, StatItem * pItem, uint * pCount) const
{
	int    ok = -1;
	uint   count = 0;
	if(P_Stat) {
		LDATETIME dtm;
		dtm.SetZero();
		uint   idx = 0;
		StatItem * p_item;
		for(uint i = 0; P_Stat->enumItems(&i, (void **)&p_item);) {
			if(p_item->JobID == jobID) {
				count++;
				if(cmp(dtm, p_item->LastRunningTime) < 0) {
					dtm = p_item->LastRunningTime;
					idx = i;
				}
			}
		}
		if(idx > 0) {
			ASSIGN_PTR(pPos, idx-1);
			ASSIGN_PTR(pItem, *(StatItem *)P_Stat->at(idx-1));
			ok = 1;
		}
	}
	ASSIGN_PTR(pCount, count);
	return ok;
}

int SLAPI PPJobServer::RemoveOldStat(PPID jobID, uint countToRemove)
{
	int    ok = -1;
	uint   count = 0;
	if(P_Stat) {
		StatItem * p_item;
		for(uint i = 0; count < countToRemove && P_Stat->enumItems(&i, (void **)&p_item);)
			if(p_item->JobID == jobID) {
				P_Stat->atFree(--i);
				count++;
				ok = 1;
			}
	}
	return ok;
}

int SLAPI PPJobServer::AddStat(PPID jobID, const LDATETIME & lastTime, long duration)
{
	int    ok = 1;
	StatItem last_stat;
	uint   last_stat_pos = 0;
	uint   count = 0;
	StatItem new_item;
	MEMSZERO(new_item);
	new_item.JobID = jobID;
	new_item.LastRunningTime = lastTime;
	if(GetLastStat(jobID, &last_stat_pos, &last_stat, &count) > 0) {
		new_item.Count = last_stat.Count + 1;
		new_item.AvgDuration = (long)(((double)last_stat.AvgDuration * last_stat.Count + (double)duration) / new_item.Count);
		if((count + 1) > JOB_MAX_STAT_ENTRIES)
			RemoveOldStat(jobID, (count + 1) - JOB_MAX_STAT_ENTRIES);
	}
	else {
		new_item.Count = 1;
		new_item.AvgDuration = duration;
	}
	if(!P_Stat) {
		THROW_MEM(P_Stat = new SArray(sizeof(StatItem)));
	}
	THROW_SL(P_Stat->insert(&new_item));
	CATCHZOK
	return ok;
}

int SLAPI PPJobServer::SaveStat()
{
	int    ok = -1;
	if(P_Stat && P_Stat->getCount()) {
		SFile  f;
		SString buf;
		StatItem * p_item;
		THROW_SL(f.Open(StatFilePath, SFile::mWrite));
		for(uint i = 0; P_Stat->enumItems(&i, (void **)&p_item);) {
			buf = 0;
			buf.Cat(p_item->JobID).Semicol().Cat(p_item->LastRunningTime).Semicol().
				Cat(p_item->Count).Semicol().Cat(p_item->AvgDuration).CR();
			THROW_SL(f.WriteLine(buf));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPJobServer::LoadStat()
{
	int    ok = -1;
	SFile  f;
	SString buf, field;
	ZDELETE(P_Stat);
	THROW_SL(f.Open(StatFilePath, SFile::mRead));
	while(f.ReadLine(buf)) {
		StatItem item;
		MEMSZERO(item);
		StringSet ss(',', buf);
		for(uint pos = 0, c = 0; ss.get(&pos, field); c++) {
			if(c == 0)
				item.JobID = field.ToLong();
			else if(c == 1)
				strtodatetime(field, &item.LastRunningTime, DATF_DMY, 0);
			else if(c == 2)
				item.Count = field.ToLong();
			else if(c == 3)
				item.AvgDuration = field.ToLong();
		}
		if(!P_Stat)
			THROW_MEM(P_Stat = new SArray(sizeof(StatItem)));
		THROW_SL(P_Stat->insert(&item));
	}
	ok = 1;
	CATCHZOK
	return ok;
}

int SLAPI PPJobServer::Arrange(PPJobPool * pPool, LAssocArray * pPlan, PPIDArray * pOnStartUpList, LDATE * pPlanDate)
{
	int    ok = -1, r;
	LDATETIME curdtm = getcurdatetime_();
	pPlan->freeAll();
	if((r = Mngr.LoadPool(0, pPool, 1)) > 0) {
		PPJob job;
		for(PPID id = 0; pPool->Enum(&id, &job) > 0;) {
			if(!(job.Flags & PPJob::fDisable)) { // @v8.2.5 |PPJob::fUnSheduled
				if(!(job.Flags & PPJob::fUnSheduled)) {
					LDATETIME dtm;
					StatItem si;
					MEMSZERO(si);
					if(GetLastStat(job.ID, 0, &si, 0) > 0) {
						dtm = si.LastRunningTime;
						while(job.Dtr.Next_(dtm, &dtm) > 0 && dtm.d <= curdtm.d)
							if(dtm.d == curdtm.d) {
								if(!job.ScheduleBeforeTime || dtm.t < job.ScheduleBeforeTime) // @v9.2.11
									pPlan->Add(dtm.t.totalsec(), job.ID, 0);
							}
					}
					else {
						if(job.Dtr.Prd == PRD_DAY && job.Dtr.Dtl.D.QuantSec > 0)
							dtm.d = plusdate(curdtm.d, -1);
						else
							encodedate(1, 1, curdtm.d.year()-1, &dtm.d);
						while(job.Dtr.Next_(dtm, &dtm) > 0 && dtm.d <= curdtm.d)
							if(cmp(dtm, curdtm) > 0) {
								if(!job.ScheduleBeforeTime || dtm.t < job.ScheduleBeforeTime) // @v9.2.11
									pPlan->Add(dtm.t.totalsec(), job.ID, 0);
							}
					}
				}
				if(pOnStartUpList && job.Flags & PPJob::fOnStartUp) {
					pOnStartUpList->add(job.ID);
				}
				ok = 1;
			}
		}
		pPlan->Sort();
		{
			SString fmt_buf, msg_buf, dt_buf;
			PPLoadText(PPTXT_JOBPLANARRANGED, fmt_buf);
			dt_buf.Cat(curdtm.d);
			msg_buf.Printf(fmt_buf, dt_buf.cptr(), pPlan->getCount());
			PPLogMessage(PPFILNAM_SERVER_LOG, msg_buf, LOGMSGF_TIME);
		}
	}
	else {
		PPLogMessage(PPFILNAM_SERVER_LOG, PPSTR_TEXT, PPTXT_JOBPOOLLOADFAULT, LOGMSGF_TIME);
		if(r == 0)
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_LASTERR);
	}
	ASSIGN_PTR(pPlanDate, curdtm.d);
	return ok;
}

void SLAPI PPJobServer::Run()
{
	SString temp_buf, msg_buf;
	STimer timer;
	Evnt   stop_event(SLS.GetStopEventName(temp_buf), Evnt::modeOpen);
	PPJobPool pool(&Mngr, 0, 1);
	LAssocArray plan;
	PPIDArray on_startup_list;
	LDATE plandate;
	Arrange(&pool, &plan, &on_startup_list, &plandate);
	uint   cur_plan_pos = 0;
	DirChangeNotification * p_dcn = Mngr.CreateDcn();
	LoadStat();
	//
	// Запуск задач, у которых установлен признак PPJob::fOnStartUp
	//
	if(on_startup_list.getCount()) {
		for(uint i = 0; i < on_startup_list.getCount(); i++) {
			const  PPID   job_id = on_startup_list.get(i);
			const  PPJob * p_job = pool.GetJob(job_id);
			if(p_job) {
				if(!(p_job->Flags & PPJob::fDisable)) { // @paranoic
					PPJob job = *p_job;
					PPJobSession * p_sess = new PPJobSession(&job, pool);
					p_sess->Start(1);
				}
			}
			else {
				// error
			}
		}
	}
	for(int stop = 0; !stop;) {
		int    arrange_job = 0;
		LDATETIME dtm;
		if(cur_plan_pos < plan.getCount()) {
			const LAssoc & r_item = plan.at(cur_plan_pos);
			dtm.d = plandate;
			dtm.settotalsec(r_item.Key);
		}
		else {
			arrange_job = 1;
			dtm.d = plusdate(plandate, 1);
			dtm.settotalsec(0);
		}
		timer.Set(dtm, 0);
		uint   h_count = 0;
		HANDLE h_list[32];
		h_list[h_count++] = timer;
		h_list[h_count++] = *p_dcn;
		h_list[h_count++] = stop_event;
		uint   r = WaitForMultipleObjects(h_count, h_list, 0, INFINITE);
		if(r == WAIT_OBJECT_0 + 0) { // timer
			if(arrange_job) {
 				Arrange(&pool, &plan, 0, &plandate);
 				cur_plan_pos = 0;
			}
			else {
				do {
					PPID   job_id = plan.at(cur_plan_pos++).Val;
					const  PPJob * p_job = pool.GetJob(job_id);
					if(p_job) {
						if(!(p_job->Flags & PPJob::fDisable)) { // @paranoic
							int    skip = 0;
							if(p_job->Flags & PPJob::fPermanent) {
								TSCollection <PPThread::Info> thread_info_list;
								DS.GetThreadInfoList(0, thread_info_list);
								for(uint i = 0; !skip && i < thread_info_list.getCount(); i++) {
									const PPThread::Info * p_item = thread_info_list.at(i);
									if(p_item && p_item->Kind == PPThread::kJob && p_item->JobID == p_job->ID) {
										PPLoadText(PPTXT_JOBPERMANENTALRRUNNED, temp_buf);
										msg_buf.Printf(temp_buf, p_job->Descr.Text.cptr(), p_job->DbSymb.cptr());
										PPLogMessage(PPFILNAM_SERVER_LOG, msg_buf, LOGMSGF_TIME);
										skip = 1;
									}
								}
							}
							if(!skip) {
								PPJob job = *p_job;
								PPJobSession * p_sess = new PPJobSession(&job, pool);
								p_sess->Start(1);
							}
						}
					}
					else {
						// error
					}
				} while(cur_plan_pos < plan.getCount() && plan.at(cur_plan_pos).Key == plan.at(cur_plan_pos-1).Key);
			}
		}
		else if(r == WAIT_OBJECT_0 + 1) { // Каталог, содержащий файл пула изменился //
			if(Mngr.IsPoolChanged()) {
 				Arrange(&pool, &plan, 0, &plandate);
 				cur_plan_pos = 0;
			}
			p_dcn->Next();
		}
		else if(r == WAIT_OBJECT_0 + 2) { // stop event
			PPLogMessage(PPFILNAM_SERVER_LOG, PPSTR_TEXT, PPTXT_JOBSERVERSTOPPED, LOGMSGF_TIME);
			stop = 1; // quit loop
		}
		else if(r == WAIT_FAILED) {
			// error
		}
	}
	SaveStat();
	delete p_dcn;
}
//
//
//
class CPosNodeBlock {
public:
	friend CPosProcessor;
	enum {
		cPosNode = 1,
		cAgent,
		cGoods,
		cMode,
		cCode,
		cQueue,
		cUUID,
		cCCheck,
		cCTable,
		cModif // список модификаторов к выбранному товару. Передается в виде: '(' GoodsID[,Qtty[,Price]];GoodsID ')'
			// modif_list := '(' modif_items ')'
			// modif_items := modif_item | modif_item ';' modif_items
			// modif_item := integer | integer ',' real | integer ',' real ',' real
	};
	enum {
		scID = 1,
		scName,
		scCode,
		scCheck,
		scTableNum,
		scLastCheck,
		scGuestCount,
		scSCardCode,
		scGoodsId,
		scQtty,
		scLast,
		scRowNum
	};

	SLAPI  CPosNodeBlock();
	SLAPI ~CPosNodeBlock();
	int    SLAPI Execute(uint cmd, const char * pParams, PPJobSrvReply & rReply);
private:
	static Sdr_CPosCheckRow & SLAPI MakeCPosCheckRow(long idx, PPID ccID, const CCheckItem & rCcItem, Sdr_CPosCheckRow & rRow);
	int    SLAPI Parse(uint cmd, int crit, int subcriterion, const SString & rArg);
	int    SLAPI ResolveCrit_PosNode(int subcriterion, const SString & rArg, PPID * pID);
	int    SLAPI ResolveCrit_ArByPerson(int subcriterion, const SString & rArg, PPID accSheetID, PPID * pID);
	int    SLAPI GetCheckList(long tblId, TSArray <Sdr_CPosCheck> * pList);
	int    SLAPI OpenSession(PPID cnId, S_GUID * pUuid);

	struct CashNodeBlock {
		PPID   ID;
		PPID   AgentID;
		S_GUID Uuid;
	};
	struct SelectTableBlock {
		long   TblId;
		long   GuestCount;
		char   SCardCode[32];
	};
	struct AddLnBlock {
		PPID   GoodsId;
		double Qtty;
	};
	struct LnQueueBlock {
		long   RowNo;
		long   Queue;
	};
	struct ProcessCodeBlock {
		int    Mode;
		char   Code[128];
	};
	struct CommandBlock {
		SLAPI  CommandBlock();
		void   SLAPI Clear();

		union {
			CashNodeBlock    CN;
			SelectTableBlock ST;
			AddLnBlock       AL;
			ProcessCodeBlock PC;
			LnQueueBlock     LQ;
			long   TblId;
			long   CheckId;
			long   RowNum;
			PPID   GoodsID;
		} U;
		SaModif ModifList; // Список модификаторов как приложение к субблоку AL
	};
	CommandBlock CmdBlk;
	TSArray <Sdr_CPosTable> CTblList;
	PPSyncCashNode CashNode;
	PPObjGoods GObj;
	PPObjArticle ArObj;
	PPObjPerson  PsnObj;
	PPObjCashNode ObjCashN;
	CCheckCore CCheckTbl;
	CPosProcessor * P_Prcssr;
	S_GUID DeviceUuid;
};

SLAPI CPosNodeBlock::CommandBlock::CommandBlock()
{
	Clear();
}

void SLAPI CPosNodeBlock::CommandBlock::Clear()
{
	MEMSZERO(U);
	ModifList.clear();
}

SLAPI CPosNodeBlock::CPosNodeBlock()
{
	P_Prcssr = 0;
	DeviceUuid.SetZero();
}

SLAPI CPosNodeBlock::~CPosNodeBlock()
{
	ZDELETE(P_Prcssr);
}

//static
Sdr_CPosCheckRow & SLAPI CPosNodeBlock::MakeCPosCheckRow(long idx, PPID ccID, const CCheckItem & rCcItem, Sdr_CPosCheckRow & rRow)
{
	rRow._id     = idx;
	rRow.CheckId = ccID;
	rRow.GoodsId = rCcItem.GoodsID;
	rRow.Flags   = rCcItem.Flags;
	rRow.Qtty    = rCcItem.Quantity;
	rRow.Price   = rCcItem.Price;
	rRow.Discount = rCcItem.Discount;
	return rRow;
}

int SLAPI CPosNodeBlock::Parse(uint cmd, int crit, int subcriterion, const SString & rArg)
{
	int    ok = -1;
	switch(cmd) {
		case PPSCMD_POS_INIT:
			{
				switch(crit) {
					case cPosNode:
						THROW((ok = ResolveCrit_PosNode(subcriterion, rArg, &CmdBlk.U.CN.ID)) > 0);
						break;
					case cAgent:
						{
							const PPID agent_acs_id = GetAgentAccSheet();
							THROW(ok = ResolveCrit_ArByPerson(subcriterion, rArg, agent_acs_id, &CmdBlk.U.CN.AgentID));
						}
						break;
					case cUUID:
						{
							S_GUID uuid;
							THROW_SL(uuid.FromStr(rArg));
							CmdBlk.U.CN.Uuid = uuid;
						}
						break;
				}
			}
			break;
		case PPSCMD_POS_GETCCHECKLIST:
			if(crit == cCTable) {
				if(subcriterion == scTableNum)
					CmdBlk.U.TblId = rArg.ToLong();
			}
			break;
		case PPSCMD_POS_SUSPENDCCHECK:
			break;
		case PPSCMD_POS_SELECTCCHECK:
			if(crit == cCCheck) {
				if(subcriterion == scID)
					CmdBlk.U.CheckId = rArg.ToLong();
			}
			break;
		case PPSCMD_POS_SELECTCTABLE:
			if(crit == cCTable) {
				switch(subcriterion) {
					case scTableNum: CmdBlk.U.ST.TblId = rArg.ToLong(); break;
					case scGuestCount: CmdBlk.U.ST.GuestCount = rArg.ToLong(); break;
					case scSCardCode: STRNSCPY(CmdBlk.U.ST.SCardCode, rArg); break;
				}
			}
			break;
		case PPSCMD_POS_ADDCCHECKLINE:
			{
				if(crit == cGoods) {
					switch(subcriterion) {
						case scID: CmdBlk.U.AL.GoodsId = rArg.ToLong(); break;
						case scQtty: CmdBlk.U.AL.Qtty = rArg.ToReal(); break;
					}
				}
				else if(crit == cModif) {
                    SString temp_buf, item_buf, fld_buf;
					StringSet ss(';', rArg);
					SStrScan scan;
					for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
						SaModifEntry entry;
						MEMSZERO(entry);
						scan.Set(temp_buf, 0);
						THROW_PP_S(scan.GetNumber(fld_buf), PPERR_CPOS_INVMODIFFORMAT, rArg);
						entry.GoodsID = fld_buf.ToLong();
						{
							Goods2Tbl::Rec goods_rec;
							THROW_PP_S(GObj.Fetch(entry.GoodsID, &goods_rec) > 0, PPERR_CPOS_INVMODIFGOODS, rArg);
						}
						scan.Skip().IncrChr(',');
						if(scan.GetNumber(fld_buf)) {
							entry.Qtty = fld_buf.ToReal();
							THROW_PP_S(entry.Qtty > 0.0, PPERR_CPOS_INVMODIFFORMAT, rArg);
							scan.Skip().IncrChr(',');
							if(scan.GetNumber(fld_buf)) {
								entry.Price = fld_buf.ToReal();
								//THROW_PP_S(entry.Price > 0.0, PPERR_CPOS_INVMODIFFORMAT, rArg);
							}
						}
						CmdBlk.ModifList.insert(&entry);
					}
				}
			}
			break;
		case PPSCMD_POS_CLEARCCHECK:
			if(crit == cCCheck) {
				if(subcriterion == scRowNum) {
					if(rArg.Cmp("ALL", 1) == 0)
						CmdBlk.U.RowNum = -1;
					else
						CmdBlk.U.RowNum = rArg.ToLong();
				}
			}
			break;
		case PPSCMD_POS_CPOSSETCCROWQUEUE:
			if(crit == cQueue) {
				CmdBlk.U.LQ.Queue = rArg.ToLong();
			}
			else if(crit == cCCheck) {
				if(subcriterion == scRowNum) {
					CmdBlk.U.LQ.RowNo = rArg.ToLong();
				}
			}
			break;
		case PPSCMD_POS_GETGOODSPRICE:
			CmdBlk.U.GoodsID = rArg.ToLong();
			break;
		case PPSCMD_POS_GETMODIFLIST:
			CmdBlk.U.GoodsID = rArg.ToLong();
			break;
		case PPSCMD_POS_PROCESSBARCODE:
			{
				if(crit == cMode)
					CmdBlk.U.PC.Mode = rArg.ToLong();
				else if(crit == cCode)
					STRNSCPY(CmdBlk.U.PC.Code, rArg);
			}
			break;
	}
	CATCHZOK
	return ok;
}

int SLAPI CPosNodeBlock::ResolveCrit_PosNode(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		case scCode:
			{
				PPObjCashNode cn_obj;
				if(cn_obj.SearchBySymb(rArg, &id) > 0) {
					;
				}
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int SLAPI CPosNodeBlock::ResolveCrit_ArByPerson(int subcriterion, const SString & rArg, PPID accSheetID, PPID * pID)
{
	int    ok = 1;
	const PPID acs_id = NZOR(accSheetID, GetSupplAccSheet());
	PPID   temp_id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			temp_id = rArg.ToLong();
			break;
		case scCode:
			if(acs_id) {
				PPObjAccSheet acs_obj;
				PPAccSheet acs_rec;
				if(acs_obj.Fetch(acs_id, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup) {
					PPObjPersonKind pk_obj;
					PPPersonKind pk_rec;
					if(pk_obj.Fetch(acs_rec.ObjGroup, &pk_rec) > 0 && pk_rec.CodeRegTypeID) {
						PPIDArray list;
						SString temp_buf = rArg;
						if(temp_buf.CmpPrefix("SAL", 1) == 0)
							temp_buf.ShiftLeft(3);
						if(PsnObj.GetListByRegNumber(pk_rec.CodeRegTypeID, pk_rec.ID, temp_buf, list) > 0)
							temp_id = list.get(0);
					}
				}
				THROW_PP_S(temp_id, PPERR_ARCODENFOUND, rArg);
			}
			break;
		case scName:
			PsnObj.P_Tbl->SearchByName(rArg, &temp_id, 0);
			break;
		default:
			CALLEXCEPT_PP(PPERR_CMDSEL_INVSUBCRITERION);
			break;
	}
	if(temp_id && acs_id) {
		PPID   ar_id = 0;
		ArObj.P_Tbl->PersonToArticle(temp_id, acs_id, &ar_id);
		temp_id = ar_id;
	}
	else
		temp_id = 0;
	ok = (temp_id ? 1 : -1);
	CATCH
		temp_id = 0;
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pID, temp_id);
	return ok;
}

int SLAPI CPosNodeBlock::GetCheckList(long tblId, TSArray <Sdr_CPosCheck> * pList)
{
	int    ok = 1;
	CCheckViewItem item;
	CCheckFilt     f;
	PPViewCCheck   v;
	THROW_INVARG(pList);
	f.Flags = CCheckFilt::fShowSuspended|CCheckFilt::fSuspendedOnly|CCheckFilt::fCTableStatus;
	f.TableCode = tblId;
	f.NodeList.Add(P_Prcssr->GetPosNodeID());
	f.Period.upp = CashNode.CurDate;
	f.Period.low = plusdate(f.Period.upp, -CashNode.Scf.DaysPeriod);
	v.Init_(&f);
	for(v.InitIteration(0); v.NextIteration(&item) > 0;) {
		if(item.TableCode > 0) {
			Sdr_CPosCheck sdr_rec;
			sdr_rec._id        = item.ID;
			sdr_rec.TableId    = item.TableCode;
			sdr_rec.GuestCount = item.GuestCount;
			sdr_rec.Number     = item.Code;
			sdr_rec.SCardId    = item.SCardID;
			sdr_rec.Amount     = MONEYTOLDBL(item.Amount);
			THROW(pList->insert(&sdr_rec));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI CPosNodeBlock::OpenSession(PPID cnId, S_GUID * pUuid)
{
	int    ok = 1;
	LDATE  cur_dt = getcurdate_();
	THROW(P_Prcssr->OpenSession(&cur_dt, 1));
	if(cnId != 0)
		THROW(ObjCashN.Search(cnId, &CashNode) > 0);
	RVALUEPTR(DeviceUuid, pUuid);
	CashNode.CurDate = cur_dt;
	CATCHZOK
	return ok;
}

int SLAPI CPosNodeBlock::Execute(uint cmd, const char * pParams, PPJobSrvReply & rReply)
{
	struct SymbItem {
		long   ID;
		const  char * P_Text;
	};
	static const SymbItem crit_titles[] = {
		{ cPosNode, "POSNODE" },
		{ cAgent,   "AGENT" },
		{ cGoods,   "GOODS" },
		{ cMode,    "MODE" },
		{ cCode,    "CODE" },
		{ cQueue,   "QUEUE" },
		{ cUUID,    "UUID"  },
		{ cModif,   "MODIF" },
		{ cCCheck,  "CCHECK" },
		{ cCTable,  "CTABLE" }
	};
	static const SymbItem subcrit_titles[] = {
		{ scID,            "ID"   },
		{ scCode,          "CODE" },
		{ scCheck,         "CHECKID" },
		{ scTableNum,      "CTABLENUM" },
		{ scLastCheck,     "LASTCHECK" },
		{ scGuestCount,    "GUESTCOUNT" },
		{ scSCardCode,     "SCARDCODE" },
		{ scGoodsId,       "GOODSID" },
		{ scQtty,          "QTTY" },
		{ scName,          "NAME" },
		{ scLast,          "LAST" },
		{ scRowNum,        "ROWNUM" }
	};
	CmdBlk.Clear();

	int    ok = 1;
	size_t pos = 0;
	uint   i;
	SStrScan scan(pParams);
	SString temp_buf, added_msg, obj, crit, sub_crit;
	pos = scan.Offs;
	THROW_PP(scan.Skip().GetIdent(temp_buf), PPERR_CMDSEL_EXP_SELECT);
	pos = scan.Offs;
	if(scan.Skip().GetIdent(temp_buf)) {
		SString arg_buf;
		do {
			int    criterion = 0;
			int    sub_criterion = 0;
			arg_buf = 0;
			for(i = 0; !criterion && i < SIZEOFARRAY(crit_titles); i++) {
				if(temp_buf.CmpNC(crit_titles[i].P_Text) == 0) {
					crit = temp_buf;
					criterion = crit_titles[i].ID;
				}
			}
			if(criterion == scLast) {
				//
				// Одиночные критерии (не требующие параметров):
				// 'ACTUAL' 'LAST'
				//
				sub_criterion = 0;
				arg_buf = 0;
			}
			else {
				if(scan[0] == '.') {
					scan.Incr();
					THROW_PP(scan.Skip().GetIdent(temp_buf), PPERR_CMDSEL_EXP_SUBCRITERION);
					for(i = 0; !sub_criterion && i < SIZEOFARRAY(subcrit_titles); i++) {
						if(temp_buf.CmpNC(subcrit_titles[i].P_Text) == 0) {
							sub_crit = temp_buf;
							sub_criterion = subcrit_titles[i].ID;
							(added_msg = obj).Space().Cat("BY").Space().Cat(crit).Dot().Cat(sub_crit);
						}
					}
				}
				THROW_PP(scan.Skip()[0] == '(', PPERR_CMDSEL_EXP_LEFTPAR);
				{
					char   c;
					int    par_count = 0;
					do {
						scan.Incr();
						c = scan[0];
						THROW_PP(c, PPERR_CMDSEL_EXP_RIGHTPAR);
						if(c == ')') {
							if(par_count)
								par_count--;
							else {
								scan.Incr();
								break;
							}
						}
						else if(c == '(')
							par_count++;
						arg_buf.CatChar(c);
					} while(c);
				}
			}
			THROW(Parse(cmd, criterion, sub_criterion, arg_buf));
		} while(scan.Skip().GetIdent(temp_buf));
	}
	{
		long   start = 0;
		const char * p_iter = "iter";
		const char * p_head = "head";
		PPImpExpParam param;
		if(cmd != PPSCMD_POS_INIT) {
			THROW_PP(P_Prcssr, PPERR_POSNODEPRCNDEF);
			THROW(OpenSession(0, 0));
		}
		switch(cmd) {
			case PPSCMD_POS_INIT:
				{
					const PPID agent_id = CmdBlk.U.CN.AgentID;
					ArticleTbl::Rec ar_rec;
					THROW_PP_S(agent_id && ArObj.Search(agent_id, &ar_rec) > 0, PPERR_POSAGENTNFOUND, agent_id);
					ZDELETE(P_Prcssr);
					THROW_PP(CConfig.Flags & CCFLG_USECCHECKEXT, PPERR_CPOS_MUSTUSECCHECKEXT);
					THROW_MEM(P_Prcssr = new CPosProcessor(CmdBlk.U.CN.ID, 0, 0, 0));
					THROW(OpenSession(CmdBlk.U.CN.ID, &CmdBlk.U.CN.Uuid));
					THROW(P_Prcssr->SetupAgent(agent_id, 1));
					P_Prcssr->SetupSessUuid(CmdBlk.U.CN.Uuid);
					CTblList.freeAll();
					for(i = 0; i < P_Prcssr->CTblList.getCount(); i++) {
						Sdr_CPosTable sdr_ctbl;
						MEMSZERO(sdr_ctbl);
						sdr_ctbl._id = P_Prcssr->CTblList.at(i);
						PPLoadString("cafetable", temp_buf);
						STRNSCPY(sdr_ctbl.Name, temp_buf.Space().Cat(sdr_ctbl._id));
						CTblList.insert(&sdr_ctbl);
					}
					rReply.SetAck();
					ok = 1;
				}
				break;
			case PPSCMD_POS_RELEASE:
				ok = 1;
				if(P_Prcssr) {
					if(!P_Prcssr->Backend_Release())
						ok = 0;
				}
				ZDELETE(P_Prcssr);
				if(ok)
					rReply.SetAck();
				else
					rReply.SetError();
				break;
			case PPSCMD_POS_PROCESSBARCODE:
				{
					THROW(P_Prcssr->RecognizeCode(CmdBlk.U.PC.Mode, CmdBlk.U.PC.Code, 1));
					rReply.SetAck();
					ok = 1;
				}
				break;
			case PPSCMD_POS_GETCTABLELIST:
				{
					P_Prcssr->ExportCTblList(temp_buf);
                	rReply.WriteString(temp_buf);
				}
				ok = 1;
				break;
			case PPSCMD_POS_GETCCHECKLIST:
				{
					THROW(P_Prcssr->GetAuthAgentID());
					P_Prcssr->ExportCCheckList(CmdBlk.U.TblId, temp_buf);
					rReply.WriteString(temp_buf);
				}
				ok = 1;
				break;
			case PPSCMD_POS_SELECTCCHECK:
				{
					THROW(P_Prcssr->GetAuthAgentID());
					THROW(P_Prcssr->RestoreSuspendedCheck(CmdBlk.U.CheckId));
					THROW(P_Prcssr->SetupAgent(P_Prcssr->GetAuthAgentID(), 0));
					(temp_buf = "Check selected. ID=[").Cat(CmdBlk.U.CheckId).Cat("]");
					PPLogMessage(PPFILNAM_STYLOWAITER_LOG, temp_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_THREADID);
				}
				rReply.SetAck();
				ok = 1;
				break;
			case PPSCMD_POS_SUSPENDCCHECK:
				{
					THROW(P_Prcssr->GetAuthAgentID());
					THROW(P_Prcssr->AcceptCheck(0, 0, 0, CPosProcessor::accmSuspended) > 0);
					THROW(P_Prcssr->SetupAgent(P_Prcssr->GetAuthAgentID(), 0));
					PPLogMessage(PPFILNAM_STYLOWAITER_LOG, "Check suspended", LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_THREADID);
				}
				rReply.SetAck();
				ok = 1;
				break;
			case PPSCMD_POS_SELECTCTABLE:
				THROW(P_Prcssr->GetAuthAgentID());
				THROW(P_Prcssr->SetupCTable(CmdBlk.U.ST.TblId, CmdBlk.U.ST.GuestCount));
				// P_Prcssr->SetSCard(ST.SCardCode); @todo CCheckPaneDialog->CPosProcessor
				rReply.SetAck();
				ok = 1;
				break;
			case PPSCMD_POS_ADDCCHECKLINE:
				{
					PPID   chk_id = MAXLONG;
					CPosProcessor::PgsBlock pgsb(fabs(CmdBlk.U.AL.Qtty));
					THROW(P_Prcssr->GetAuthAgentID());
					THROW(P_Prcssr->SetupNewRow(CmdBlk.U.AL.GoodsId, /*fabs(CmdBlk.U.AL.Qtty), 0, 0*/pgsb) > 0);
					THROW(P_Prcssr->Backend_SetModifList(CmdBlk.ModifList));
					THROW(P_Prcssr->AcceptRow() > 0);
					rReply.SetAck();
					ok = 1;
				}
				break;
			case PPSCMD_POS_CLEARCCHECK:
				{
					PPID   chk_id = MAXLONG;
					THROW(P_Prcssr->GetAuthAgentID());
					if(CmdBlk.U.RowNum == -1) {
						if(P_Prcssr->CheckRights(CPosProcessor::orfEscCheck))
							P_Prcssr->ClearCheck();
					}
					else { // (Права доступа проверяются в Backend_RemoveRow) if(P_Prcssr->CheckRights(CPosProcessor::orfEscChkLine)) {
						THROW(P_Prcssr->Backend_RemoveRow(CmdBlk.U.RowNum));
					}
					rReply.SetAck();
					ok = 1;
				}
				break;
			case PPSCMD_POS_CPOSSETCCROWQUEUE:
				{
					THROW(P_Prcssr->GetAuthAgentID());
					THROW(P_Prcssr->Backend_SetRowQueue(CmdBlk.U.LQ.RowNo, CmdBlk.U.LQ.Queue));
					rReply.SetAck();
					ok = 1;
				}
				break;
			case PPSCMD_POS_GETCPOSRIGHTS:
				break;
			case PPSCMD_POS_PRINTCCHECK:
				{
					PPID   chk_id = MAXLONG;
					THROW(P_Prcssr->GetAuthAgentID());
					PEOpenEngine();
					P_Prcssr->Print(1, 0, 0);
					PECloseEngine();
					rReply.SetAck();
					ok = 1;
				}
				break;
			case PPSCMD_POS_PRINTCCHECKLOCAL:
				{
					PPID   chk_id = MAXLONG;
					THROW(P_Prcssr->GetAuthAgentID());
					PEOpenEngine();
					P_Prcssr->PrintToLocalPrinters(-1);
					PECloseEngine();
					rReply.SetAck();
					ok = 1;
				}
				break;
			case PPSCMD_POS_GETGOODSPRICE:
				{
                	RetailGoodsInfo rgi;
                	P_Prcssr->GetRgi(CmdBlk.U.GoodsID, 0.0, PPObjGoods::rgifPriceOnly, rgi);
                	rReply.WriteString(temp_buf.Z().Cat(rgi.Price));
				}
				break;
			case PPSCMD_POS_GETCURRENTSTATE:
				{
					P_Prcssr->ExportCurrentState(temp_buf);
                	rReply.WriteString(temp_buf);
				}
				break;
			case PPSCMD_POS_RESETCURRENTLINE:
				{
					THROW(P_Prcssr->GetAuthAgentID());
					P_Prcssr->ResetCurrentLine();
					rReply.SetAck();
					ok = 1;
				}
				break;
			case PPSCMD_POS_GETMODIFLIST:
				{
					P_Prcssr->ExportModifList(CmdBlk.U.GoodsID, temp_buf);
                	rReply.WriteString(temp_buf);
				}
				break;
		}
	}
	CATCH
		rReply.SetError();
		ok = 0;
	ENDCATCH
	return ok;
}

#define MAGIC_FILETRANSMIT "$#FILETRANSMITMAGIC#$"

class PPServerSession : public /*PPThread*/PPWorkerSession {
public:
	static int TestingClient(TcpSocket & rSo, StrAssocArray & rStrList);

	struct InitBlock {
		SLAPI  InitBlock();
		enum {
			fDebugMode = 0x0001
		};
		uint   ClosedSockTimeout;
		uint   SuspTimeout;
		uint   SleepTimeout;
		int    TxtCmdTerminalCode;
		long   Flags;
	};

	SLAPI  PPServerSession(TcpSocket & rSock, const InitBlock & rBlk, InetAddr & rAddr);
	SLAPI ~PPServerSession();
	int    FASTCALL SendReply(PPJobSrvReply & rReply);
	virtual int SLAPI SubstituteSock(TcpSocket & rSo, PPJobSrvReply * pReply);
	virtual void SLAPI Shutdown();
private:
	static int TestSend(TcpSocket & rSo, const void * pBuf, size_t sz, size_t * pActualSize);
	static int TestRecv(TcpSocket & rSo, void * pBuf, size_t sz, size_t * pActualSize);
	static int TestRecvBlock(TcpSocket & rSo, void * pBuf, size_t sz, size_t * pActualSize);

	virtual void SLAPI Run();
	virtual CmdRet SLAPI ProcessCommand(PPServerCmd * pEv, PPJobSrvReply & rReply);
	CmdRet SLAPI Testing();
	CmdRet SLAPI ReceiveFile(int verb, const char * pParam, PPJobSrvReply & rReply);
	size_t SLAPI Helper_ReceiveFilePart(PPJobSrvReply::TransmitFileBlock & rBlk, SFile * pF);

	uint32 SuspendTimeout;     // Таймаут (ms) ожидания восстановления приостановленной сессии         //
	uint32 CloseSocketTimeout; // Таймаут (ms) ожидания восстановления сессии после разрыва соединения //
	uint32 SleepTimeout;       // Таймаут (ms) ожидания хоть какого-то события в активной сессии.

	TcpSocket So;
	StyloBhtIIExchanger * P_SbiiBlk;
	Evnt   EvSubstSockStart;
	Evnt   EvSubstSockReady;
	Evnt   EvSubstSockFinish;
	InetAddr Addr; // Адрес инициатора соединения
};

SLAPI PPWorkerSession::FTB::FTB()
{
	THISZERO();
}

SLAPI PPWorkerSession::FTB::~FTB()
{
	delete P_F;
}

SLAPI PPWorkerSession::PPWorkerSession(int threadKind) : PPThread(/*PPThread::kNetSession*/threadKind, 0, 0)
{
	P_CPosBlk = 0;
	State = 0;
	Counter = 0;
	P_TxtCmdTerminal = 0;
}

SLAPI PPWorkerSession::~PPWorkerSession()
{
	ZDELETE(P_CPosBlk);
}

// virtual
void SLAPI PPWorkerSession::Shutdown()
{
	ZDELETE(P_CPosBlk);
	FtbList.freeAll();
	PPThread::Shutdown();
}

SString & SLAPI PPWorkerSession::GetTxtCmdTermMnemonic(SString & rBuf) const
{
	rBuf.Z();
	if(P_TxtCmdTerminal == 0) {
		rBuf = "none";
	}
	else {
		const size_t len = strlen(P_TxtCmdTerminal);
		if(len == 0) {
			rBuf.CatHex((uint8)0);
		}
		else {
			for(size_t i = 0; i < len; i++) {
				rBuf.CatHex((uint8)P_TxtCmdTerminal[i]);
			}
		}
	}
	return rBuf;
}

int SLAPI PPWorkerSession::SetupTxtCmdTerm(int code)
{
	int    ok = 1;
	switch(code) {
		case -1: P_TxtCmdTerminal = 0; break;
		case 0: P_TxtCmdTerminal = 0; break;
		case 1: P_TxtCmdTerminal = "\xD\xA"; break;
		case 2: P_TxtCmdTerminal = "\xD\xA\xD\xA"; break;
		case 3: P_TxtCmdTerminal = "\xD"; break;
		case 4: P_TxtCmdTerminal = "\xA"; break;
		case 5: P_TxtCmdTerminal = "\x0"; break;
		default: ok = 0; break;
	}
	return ok;
}

PPWorkerSession::CmdRet SLAPI PPWorkerSession::Helper_QueryNaturalToken(PPServerCmd * pEv, PPJobSrvReply & rReply)
{
	CmdRet ret = cmdretOK;
	xmlTextWriter * p_writer = 0;
	xmlBuffer * p_xml_buf = 0;
	SString token, temp_buf;
	PPObjPerson * p_psn_obj = 0;
	pEv->GetParam(1, token); //PPGetExtStrData(1, pEv->Params, token);
	if(token.NotEmptyS()) {
		uint   i;
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		LAssocArray rel_obj_list;
		tr.Run(token.ucptr(), -1, nta, 0);
		if(nta.Has(SNTOK_EMAIL) > 0.0f) {
            PPIDArray psn_list;
            PPIDArray loc_list;
            THROW_MEM(SETIFZ(p_psn_obj, new PPObjPerson));
            if(p_psn_obj->SearchEmail(token, 0, &psn_list, &loc_list) > 0) {
				psn_list.sortAndUndup();
				loc_list.sortAndUndup();
				for(i = 0; i < psn_list.getCount(); i++) {
					rel_obj_list.Add(PPOBJ_PERSON, psn_list.get(i), 0);
				}
				for(i = 0; i < loc_list.getCount(); i++) {
					rel_obj_list.Add(PPOBJ_LOCATION, loc_list.get(i), 0);
				}
            }
		}
		THROW(p_xml_buf = xmlBufferCreate());
		THROW(p_writer = xmlNewTextWriterMemory(p_xml_buf, 0));
		{
			SXml::WDoc _doc(p_writer, cpUTF8);
			{
				SXml::WNode n_root(p_writer, "NaturalToken");
				{
					SXml::WNode(p_writer, "Token", (temp_buf = token).ToUtf8());
					{
						SXml::WNode n_probidlist(p_writer, "TokenTypeList");
						for(i = 0; i < nta.getCount(); i++) {
							const SNaturalToken & r_nt = nta.at(i);
							SXml::WNode n_item(p_writer, "TokenType");
							{
								n_item.PutInner("TokenTypeId", temp_buf.Z().Cat(r_nt.ID));
								PPGetSubStrById(PPTXT_NATURALTOKENID, r_nt.ID, temp_buf.Z());
								n_item.PutInner("TokenTypeSymb", temp_buf);
								n_item.PutInner("TokenTypeProb", temp_buf.Z().Cat((double)r_nt.Prob, MKSFMTD(0, 12, NMBF_NOTRAILZ)));
							}
						}
					}
					if(rel_obj_list.getCount()) {
						PersonTbl::Rec psn_rec;
						LocationTbl::Rec loc_rec;
						THROW_MEM(SETIFZ(p_psn_obj, new PPObjPerson));
						SXml::WNode n_relobjlist(p_writer, "RelObjList");
						for(i = 0; i < rel_obj_list.getCount(); i++) {
							SXml::WNode n_obj(p_writer, "RelObj");
							const LAssoc item = rel_obj_list.at(i);
							const PPID obj_type = item.Key;
							n_obj.PutInner("Obj", temp_buf.Z().Cat(obj_type));
							n_obj.PutInner("Id", temp_buf.Z().Cat(item.Val));
							if(obj_type == PPOBJ_PERSON) {
								if(p_psn_obj->Fetch(item.Val, &psn_rec) > 0) {
									n_obj.PutInner("Name", (temp_buf = psn_rec.Name).Transf(CTRANSF_INNER_TO_UTF8));
								}
							}
							else if(obj_type == PPOBJ_LOCATION) {
								if(p_psn_obj->LocObj.Fetch(item.Val, &loc_rec) > 0) {
									temp_buf.Z();
									LocationCore::GetExField(&loc_rec, LOCEXSTR_CONTACT, temp_buf);
									if(!temp_buf.NotEmptyS())
										temp_buf = loc_rec.Name;
									if(!temp_buf.NotEmptyS())
										LocationCore::GetExField(&loc_rec, LOCEXSTR_FULLADDR, temp_buf);
									if(!temp_buf.NotEmptyS())
										LocationCore::GetExField(&loc_rec, LOCEXSTR_SHORTADDR, temp_buf);
									if(temp_buf.NotEmptyS()) {
										n_obj.PutInner("Name", (temp_buf = psn_rec.Name).Transf(CTRANSF_INNER_TO_UTF8));
									}
								}
							}
						}
					}
				}
			}
			xmlTextWriterFlush(p_writer);
			temp_buf.CopyFromN((char *)p_xml_buf->content, p_xml_buf->use);
			temp_buf.ReplaceCR();
			temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
			rReply.SetString(temp_buf);
		}
	}
	CATCH
		ret = cmdretError;
	ENDCATCH
	xmlFreeTextWriter(p_writer);
	xmlBufferFree(p_xml_buf);
	delete p_psn_obj;
    return ret;
}

void FASTCALL PPWorkerSession::RealeasFtbEntry(uint pos)
{
	if(pos < FtbList.getCount()) {
		FTB * p_ftb = FtbList.at(pos);
		if(p_ftb) {
			ZDELETE(p_ftb->P_F);
			memzero(p_ftb, sizeof(*p_ftb));
		}
	}
}

PPWorkerSession::CmdRet SLAPI PPWorkerSession::TransmitFile(int verb, const char * pParam, PPJobSrvReply & rReply)
{
	const uint32 DefFileChunkSize = 4 * 1024 * 1024; // 4Mb

	CmdRet ret = cmdretOK;
	int    set_more_flag = 0;
	int32  cookie = 0;
	SString temp_buf;
	SFile * p_f = 0;
	PPJobSrvReply::TransmitFileBlock blk;
	const uint32 chunk_size = DefFileChunkSize;
	if(verb == tfvStart) {
		rReply.SetDataType(rReply.htFile, "FILE");
		THROW_SL(fileExists(pParam));
		{
			SPathStruc ps;
			SFileUtil::Stat fs;
			SFileFormat ff;
			ps.Split(pParam);
			ps.Merge(SPathStruc::fNam|SPathStruc::fExt, temp_buf);
			temp_buf.CopyTo(blk.Name, sizeof(blk.Name));
			THROW(SFileUtil::GetStat(pParam, &fs));
			blk.CrtTime = fs.CrtTime;
			blk.AccsTime = fs.AccsTime;
			blk.ModTime = fs.ModTime;
			blk.Size = fs.Size;
			ff.Identify(pParam);
			blk.Format = ff;
			if(fs.Size <= (int64)chunk_size) {
				blk.PartSize = (uint32)fs.Size;
			}
			else {
				blk.PartSize = chunk_size;
				set_more_flag = 1;
			}
			{
				int64  offs = 0;
				STempBuffer buf(blk.PartSize);
				if(buf.GetSize()) {
					THROW_MEM(p_f = new SFile(pParam, SFile::mRead|SFile::mBinary));
					THROW_SL(p_f->IsValid());
					THROW_SL(p_f->ReadV(buf, buf.GetSize()));
					offs = p_f->Tell64();
				}
				if(set_more_flag) {
					FTB * p_ftb = new FTB;
					THROW_MEM(p_ftb);
					memzero(p_ftb, sizeof(*p_ftb));
					cookie = ++Counter;
					blk.Cookie = cookie;
					p_ftb->Cookie = blk.Cookie;
					p_ftb->Offs = offs;
					p_ftb->Tfb = blk;
					p_ftb->P_F = p_f; // Передаем указатель на открытый файл в структуру FTB
					p_f = 0;   // Освобождать память под p_f уже после этого не следует
					FtbList.insert(p_ftb);
				}
				THROW_SL(rReply.Write(&blk, sizeof(blk)));
				THROW_SL(rReply.Write(buf, buf.GetSize()));
				THROW(rReply.FinishWriting(set_more_flag ? PPJobSrvProtocol::hfMore : 0));
			}
		}
	}
	else if(verb == tfvNext) {
		temp_buf = pParam;
		int32  cookie = temp_buf.ToLong();
		uint   pos = 0;
		THROW_PP_S(FtbList.lsearch(&cookie, &pos, CMPF_LONG), PPERR_JOBSRV_FTCOOKIENFOUND, pParam);
		FTB * p_ftb = FtbList.at(pos);
		THROW_PP_S(p_ftb, PPERR_JOBSRV_FTCOOKIENFOUND, pParam);
		if(p_ftb->P_F) {
			blk = p_ftb->Tfb;
			if(blk.Size <= (int64)chunk_size) {
				blk.PartSize = (uint32)blk.Size;
			}
			else {
				blk.PartSize = chunk_size;
				set_more_flag = 1;
			}
			{
				int64  offs = 0;
				STempBuffer buf(blk.PartSize);
				if(buf.GetSize()) {
					THROW_SL(p_ftb->P_F->IsValid());
					THROW_SL(p_ftb->P_F->Seek64(p_ftb->Offs));
					// THROW_SL(p_ftb->P_F->ReadV(buf, buf.GetSize())); // надо считать оставшееся кол-во байтов
					THROW_SL(p_ftb->P_F->Read(buf, buf.GetSize(), (size_t*)&blk.PartSize));
					offs = p_ftb->P_F->Tell64();
				}
				buf.Alloc(blk.PartSize);
				if(set_more_flag) {
					p_ftb->Offs = offs;
					p_ftb->Tfb = blk;
				}
				THROW_SL(rReply.Write(&blk, sizeof(blk)));
				THROW_SL(rReply.Write(buf, buf.GetSize()));
				THROW(rReply.FinishWriting(set_more_flag ? PPJobSrvProtocol::hfMore : 0));
			}
		}
	}
	else if(verb == tfvFinish) {
		temp_buf = pParam;
		int32  cookie = temp_buf.ToLong();
		uint   pos = 0;
		THROW_PP_S(FtbList.lsearch(&cookie, &pos, CMPF_LONG), PPERR_JOBSRV_FTCOOKIENFOUND, pParam);
		RealeasFtbEntry(pos);
		rReply.SetAck();
	}
	else if(verb == tfvCancel) {
		temp_buf = pParam;
		int32  cookie = temp_buf.ToLong();
		uint   ftb_pos = 0;
		THROW_PP_S(FtbList.lsearch(&cookie, &ftb_pos, CMPF_LONG), PPERR_JOBSRV_FTCOOKIENFOUND, pParam);
		//
		// Для принимаемых файлов
		//
		{
			SString fname;
			FTB * p_ftb = FtbList.at(ftb_pos);
			if(p_ftb && p_ftb->P_F) {
				if(p_ftb->P_F->GetMode() & (mWrite|mAppend)) {
					fname = p_ftb->P_F->GetName();
					p_ftb->P_F->Close();
					p_ftb->P_F = 0;
					if(fname.NotEmptyS())
						SFile::Remove(fname);
				}
			}
		}
		RealeasFtbEntry(ftb_pos);
		rReply.SetAck();
	}
	CATCH
		rReply.SetError();
	ENDCATCH
	ZDELETE(p_f);
	return ret;
}

int SLAPI PPWorkerSession::FinishReceivingFile(PPJobSrvReply::TransmitFileBlock & rBlk, const SString & rFilePath, PPJobSrvReply & rReply)
{
	int    ok = 1;
	ObjLinkFiles lf;
	if(rBlk.TransmType == rBlk.ttObjImage) {
		if(rBlk.ObjType == PPOBJ_GOODS) {
			PPObjGoods goods_obj;
			Goods2Tbl::Rec goods_rec;
			THROW(goods_obj.Search(rBlk.ObjID, &goods_rec) > 0);
			THROW(lf.SetupZeroPositionFile(rBlk.ObjType, rBlk.ObjID, rFilePath));
			THROW(goods_obj.UpdateFlags(rBlk.ObjID, GF_HASIMAGES, 0, 1));
		}
		else if(rBlk.ObjType == PPOBJ_PERSON) {
			PPObjPerson psn_obj;
			PersonTbl::Rec psn_rec;
			THROW(psn_obj.Search(rBlk.ObjID, &psn_rec) > 0);
			THROW(lf.SetupZeroPositionFile(rBlk.ObjType, rBlk.ObjID, rFilePath));
			THROW(psn_obj.P_Tbl->UpdateFlags(rBlk.ObjID, PSNF_HASIMAGES, 0, 1));
		}
		else if(rBlk.ObjType == PPOBJ_TSESSION) {
			PPObjTSession tses_obj;
			TSessionTbl::Rec tses_rec;
			THROW(tses_obj.Search(rBlk.ObjID, &tses_rec) > 0);
			THROW(lf.SetupZeroPositionFile(rBlk.ObjType, rBlk.ObjID, rFilePath));
			THROW(tses_obj.P_Tbl->UpdateFlags(rBlk.ObjID, TSESF_HASIMAGES, 0, 1));
		}
	}
	else if(rBlk.TransmType == rBlk.ttWorkbookContent) {
        if(rBlk.ObjType == PPOBJ_WORKBOOK) {
            PPObjWorkbook wb_obj;
            WorkbookTbl::Rec wb_rec;
            THROW(wb_obj.Search(rBlk.ObjID, &wb_rec) > 0);
            THROW(lf.SetupZeroPositionFile(rBlk.ObjType, rBlk.ObjID, rFilePath));
        }
	}
	rReply.SetAck();
	CATCHZOK
	return ok;
}

PPWorkerSession::CmdRet SLAPI PPWorkerSession::ProcessCommand(PPServerCmd * pEv, PPJobSrvReply & rReply)
{
	CmdRet ok = cmdretOK;
	int    disable_err_reply = 0;
	int    r = 0;
	SString reply_buf, temp_buf, name, db_symb;
	THROW(rReply.StartWriting());
	switch(pEv->GetH().Type) {
		//
		// Эти команды обрабатываются порожденными классами {
		//
		case PPSCMD_SPII:
		case PPSCMD_GETBIZSCORES:
		case PPSCMD_STYLOBHT:
		case PPSCMD_SUSPEND:
		case PPSCMD_RESUME:
		case PPSCMD_SETIMAGE:
		case PPSCMD_PUTFILE:
		case PPSCMD_PUTNEXTFILEPART:
		case PPSCMD_TEST:
		case PPSCMD_GETDISPLAYINFO:
			ok = cmdretUnprocessed;
			break;
		// }
		case PPSCMD_STYLOBHTII:
			//
			// Сложный процесс: вызывающая процедура получит cmdretStyloBhtIIMode и переведет
			// сеанс в режим обмена с устройством StyloBHT II в результате чего управление
			// попадет в StyloBHTExch посредством вызова PPServerSession::ProcessCommand (see below)
			//
			disable_err_reply = 1;
			pEv->GetParam(1, db_symb);  // PPGetExtStrData(1, pEv->Params, db_symb);
			pEv->GetParam(2, name);     // PPGetExtStrData(2, pEv->Params, name);
			pEv->GetParam(3, temp_buf); // PPGetExtStrData(3, pEv->Params, temp_buf);
			//
			{
				SString pwd;
				Reference::Decrypt(Reference::crymRef2, temp_buf, temp_buf.Len(), pwd);
				int   lr = DS.Login(db_symb, name, pwd);
				pwd.Obfuscate();
				if(!lr) {
					rReply.SetString(temp_buf.Z().CatChar('0'));
					ok = cmdretQuit;
				}
				else {
					rReply.SetString(temp_buf.Z().CatChar('1'));
					ok = cmdretStyloBhtIIMode;
				}
			}
			//
			// StyloBhtIIExchange сама ответит клиенту. Поэтому reply здесь не формируется.
			//
			break;
		//
		// }
		//
		case PPSCMD_HELLO:
			{
				if(HelloReplyText.Empty()) {
					PPVersionInfo vi = DS.GetVersionInfo();
					vi.GetProductName(HelloReplyText);
				}
				rReply.SetString(HelloReplyText);
			}
			break;
		case PPSCMD_HSH:
			rReply.SetString(temp_buf.Z().Cat(DS.GetTLA().GetId()));
			break;
		case PPSCMD_GETLASTERRMSG:
			PPGetLastErrorMessage(1, temp_buf);
			rReply.SetString(temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
			break;
		case PPSCMD_SETTXTCMDTERM:
			{
				pEv->GetParam(1, temp_buf); // PPGetExtStrData(1, pEv->Params, temp_buf);
				if(temp_buf.IsDigit() || temp_buf == "-1") {
					long   terminal_code = temp_buf.ToLong();
					SetupTxtCmdTerm(temp_buf.ToLong());
				}
				rReply.SetString(GetTxtCmdTermMnemonic(temp_buf));
			}
			break;
		case PPSCMD_GETKEYWORDSEQ:
			{
				pEv->GetParam(1, name); // PPGetExtStrData(1, pEv->Params, name);
				if(PPGenerateKeywordSequence(name, temp_buf, 0))
					rReply.SetString(temp_buf);
				else
					rReply.SetError();
			}
			break;
		case PPSCMD_LOGIN:
			pEv->GetParam(1, db_symb); // PPGetExtStrData(1, pEv->Params, db_symb);
			pEv->GetParam(2, name); // PPGetExtStrData(2, pEv->Params, name);
			pEv->GetParam(3, temp_buf); // PPGetExtStrData(3, pEv->Params, temp_buf);
			THROW(DS.Login(db_symb, name, temp_buf) > 0);
			State |= stLoggedIn;
			rReply.SetString(temp_buf.Z().Cat(LConfig.SessionID));
			{
				(temp_buf = db_symb).CatChar(':').Cat(name);
				DS.SetThreadNotification(PPSession::stntText, temp_buf);
			}
			break;
		case PPSCMD_LOGOUT:
			State &= ~stLoggedIn;
			rReply.SetAck();
			ok = cmdretQuit;
			break;
		case PPSCMD_LOGLOCKSTACK: // @v9.8.1
			DS.LogLocStk();
			break;
		case PPSCMD_RESETCACHE:
			THROW_PP(State & stLoggedIn, PPERR_NOTLOGGEDIN);
			{
				PPID   obj_type = 0;
				long   obj_type_ext = 0;
				pEv->GetParam(1, temp_buf); // PPGetExtStrData(1, pEv->Params, temp_buf);
				THROW(obj_type = GetObjectTypeBySymb(temp_buf, &obj_type_ext));
				if(obj_type == PPOBJ_GOODS) {
					pEv->GetParam(2, temp_buf); // PPGetExtStrData(2, pEv->Params, temp_buf);
					if(temp_buf.CmpNC("NAMEPOOL") == 0) {
						PPObjGoods goods_obj;
						goods_obj.P_Tbl->ResetFullList();
						ok = cmdretOK;
					}
					else {
						CALLEXCEPT_PP(PPERR_RESETCACHE_NOTSUPP);
					}
				}
				else {
					CALLEXCEPT_PP_S(PPERR_RESETCACHE_OBJ_NOTSUPP, temp_buf);
				}
			}
			break;
		case PPSCMD_CHECKGLOBALCREDENTIAL:
			{
				pEv->GetParam(1, name); // PPGetExtStrData(1, pEv->Params, name);
				temp_buf.Space() = 0; // Гарантируем ненулевой буфер temp_buf.P_Buf
				pEv->GetParam(2, temp_buf); // PPGetExtStrData(2, pEv->Params, temp_buf);
				PPObjGlobalUserAcc gua_obj;
				PPGlobalUserAcc gua_rec;
				THROW(gua_obj.CheckPassword(name, temp_buf, &gua_rec) > 0);
				rReply.SetAck();
			}
			break;
		case PPSCMD_SOBLK:
			THROW_PP(State & stLoggedIn, PPERR_NOTLOGGEDIN);
			THROW_INVARG(pEv->P_SoBlk);
			THROW(r = pEv->P_SoBlk->Execute(rReply));
			break;
		case PPSCMD_POS_INIT:
			ZDELETEFAST(P_CPosBlk);
			P_CPosBlk = new CPosNodeBlock();
			// @fallthrough
		case PPSCMD_POS_RELEASE:
		case PPSCMD_POS_GETCTABLELIST:
		case PPSCMD_POS_GETCCHECKLIST:
		case PPSCMD_POS_GETCCHECKLNCOUNT:
		case PPSCMD_POS_ADDCCHECKLINE:
		case PPSCMD_POS_RMVCCHECKLINE:
		case PPSCMD_POS_CLEARCCHECK:
		case PPSCMD_POS_RMVCCHECK:
		case PPSCMD_POS_PRINTCCHECK:
		case PPSCMD_POS_PRINTCCHECKLOCAL:
		case PPSCMD_POS_GETCONFIG:
		case PPSCMD_POS_SETCONFIG:
		case PPSCMD_POS_GETSTATE:
		case PPSCMD_POS_SELECTCCHECK:
		case PPSCMD_POS_SUSPENDCCHECK:
		case PPSCMD_POS_SELECTCTABLE:
		case PPSCMD_POS_GETCPOSRIGHTS:
		case PPSCMD_POS_GETGOODSPRICE:
		case PPSCMD_POS_GETCURRENTSTATE:
		case PPSCMD_POS_RESETCURRENTLINE:
		case PPSCMD_POS_PROCESSBARCODE:
		case PPSCMD_POS_CPOSSETCCROWQUEUE:
		case PPSCMD_POS_GETMODIFLIST:
			{
				pEv->GetParam(1, temp_buf); // PPGetExtStrData(1, pEv->Params, temp_buf);
				THROW_PP(P_CPosBlk, PPERR_POSNODEPRCNDEF);
				THROW(r = P_CPosBlk->Execute(pEv->GetH().Type, temp_buf, rReply));
			}
			break;
		case PPSCMD_POS_DECRYPTAUTHDATA:
			{
				SString left, right;
				Sdr_CPosAuth rec;
				MEMSZERO(rec);
				pEv->GetParam(1, temp_buf); // PPGetExtStrData(1, pEv->Params, temp_buf);
				temp_buf.Divide(' ', left, right);
				memcpy(&rec, (const char*)right, sizeof(rec));
				decrypt(&rec, sizeof(rec));
				rReply.Write((const char *)&rec, sizeof(rec));
				rReply.SetDataType(PPJobSrvReply::htFile, 0);
			}
			break;
		case PPSCMD_GETTDDO:
			{
				// GETTDDO filename par1 par2 ar "sdgjlsgj  sfjsfjl;"
				// GETTDDO INLINE ${@(Goods, 35324).Name}
				Tddo t;
				StringSet ext_param_list;
				int    fld_n = 0;
				//
				// Первый аргумент - имя tddo-файла или "INLINE"
				//
				pEv->GetParam(++fld_n, name); // PPGetExtStrData(++fld_n, pEv->Params, name);
				if(name.Cmp("INLINE", 1) == 0) {
					pEv->GetParam(++fld_n, temp_buf); // PPGetExtStrData(++fld_n, pEv->Params, temp_buf);
				}
				else {
					//
					// Все последующие аргументы - дополнительные параметры, используемые при разборе tddo-файла
					//
					// while(PPGetExtStrData(++fld_n, pEv->Params, temp_buf) > 0) {
					while(pEv->GetParam(++fld_n, temp_buf) > 0) { // @v9.8.0
						ext_param_list.add(temp_buf);
					}
					THROW(Tddo::LoadFile(name, temp_buf));
					t.SetInputFileName(name);
				}
				DlRtm::ExportParam ep;
				THROW(t.Process(0 /*data_name*/, temp_buf, /*0, 0*/ep, &ext_param_list, rReply));
				rReply.SetDataType(PPJobSrvReply::htGenericText, 0);
			}
			break;
		case PPSCMD_EXECVIEWNF:
			// EXECVIEW named_filt_name [dl600_data_name]
			THROW_PP(State & stLoggedIn, PPERR_NOTLOGGEDIN);
			{
				SString nf_symb, dl600_name, file_name;
				pEv->GetParam(1, nf_symb); // PPGetExtStrData(1, pEv->Params, nf_symb);
				pEv->GetParam(2, dl600_name); // PPGetExtStrData(2, pEv->Params, dl600_name);
				THROW(PPView::ExecuteNF(nf_symb, dl600_name, file_name));
				ok = TransmitFile(tfvStart, file_name, rReply);
			}
			break;
		case PPSCMD_SETIMAGEMIME:
			// SETIMAGEMIME goods 52103 updateFlags ContentType ContentMime64
			THROW_PP(State & stLoggedIn, PPERR_NOTLOGGEDIN);
			{
				SString content_type, img_mime, file_name, file_ext;
				pEv->GetParam(1, name); // PPGetExtStrData(1, pEv->Params, name);
				long   upd_flags = 0;
				size_t bin_size = 0;
				long   obj_type_ext = 0;
				PPID   obj_type = GetObjectTypeBySymb(name, &obj_type_ext);
				pEv->GetParam(2, temp_buf); // PPGetExtStrData(2, pEv->Params, temp_buf);
				PPID   obj_id = temp_buf.ToLong();
				ObjLinkFiles lf;
				pEv->GetParam(3, temp_buf); // PPGetExtStrData(3, pEv->Params, temp_buf);
				upd_flags = temp_buf.ToLong();
				pEv->GetParam(4, content_type); // PPGetExtStrData(4, pEv->Params, content_type);
				pEv->GetParam(5, img_mime); // PPGetExtStrData(5, pEv->Params, img_mime);
				SFileFormat ff;
				ff.IdentifyMime(content_type);
				switch(ff) {
					case ff.Jpeg: file_ext = "jpg"; break;
					case ff.Png:  file_ext = "png"; break;
					case ff.Tiff: file_ext = "tiff"; break;
					case ff.Gif:  file_ext = "gif"; break;
					case ff.Bmp:  file_ext = "bmp"; break;
				}
				PPMakeTempFileName("objimg", file_ext, 0, file_name);
				{
					STempBuffer tbuf(img_mime.Len() * 2);
					THROW_SL(tbuf.IsValid());
					THROW_SL(img_mime.DecodeMime64(tbuf, tbuf.GetSize(), &bin_size));
					{
						SFile img_file(file_name, SFile::mWrite|SFile::mBinary);
						THROW_SL(img_file.IsValid());
						THROW_SL(img_file.Write(tbuf, bin_size));
					}
				}
				if(obj_type == PPOBJ_GOODS) {
					PPObjGoods goods_obj;
					Goods2Tbl::Rec goods_rec;
					THROW(goods_obj.Search(obj_id, &goods_rec) > 0);
					THROW(lf.SetupZeroPositionFile(obj_type, obj_id, file_name));
					THROW(goods_obj.UpdateFlags(obj_id, GF_HASIMAGES, 0, 1));
				}
				else if(obj_type == PPOBJ_PERSON) {
					PPObjPerson psn_obj;
					PersonTbl::Rec psn_rec;
					THROW(psn_obj.Search(obj_id, &psn_rec) > 0);
					THROW(lf.SetupZeroPositionFile(obj_type, obj_id, file_name));
					THROW(psn_obj.P_Tbl->UpdateFlags(obj_id, PSNF_HASIMAGES, 0, 1));
				}
			}
			break;
		case PPSCMD_GETIMAGE:
			// GETIMAGE goods 52103
			THROW_PP(State & stLoggedIn, PPERR_NOTLOGGEDIN);
			{
				pEv->GetParam(1, name); // PPGetExtStrData(1, pEv->Params, name);
				long   obj_type_ext = 0;
				PPID   obj_type = GetObjectTypeBySymb(name, &obj_type_ext);
				pEv->GetParam(2, temp_buf); // PPGetExtStrData(2, pEv->Params, temp_buf);
				PPID   obj_id = temp_buf.ToLong();
				SString img_path, obj_name;
				ObjLinkFiles lf;
				if(obj_type == PPOBJ_GOODS) {
					PPObjGoods goods_obj;
					Goods2Tbl::Rec goods_rec;
					THROW(goods_obj.Fetch(obj_id, &goods_rec) > 0);
					obj_name = goods_rec.Name;
					THROW_PP_S(goods_rec.Flags & GF_HASIMAGES, PPERR_OBJHASNTIMG, obj_name);
				}
				else if(obj_type == PPOBJ_BRAND) {
					PPObjBrand brand_obj;
					PPBrand    brand_rec;
					THROW(brand_obj.Fetch(obj_id, &brand_rec) > 0);
					obj_name = brand_rec.Name;
					THROW_PP_S(brand_rec.Flags & BRNDF_HASIMAGES, PPERR_OBJHASNTIMG, obj_name);
				}
				else if(obj_type == PPOBJ_PERSON) {
					PPObjPerson psn_obj;
					PersonTbl::Rec psn_rec;
					THROW(psn_obj.Fetch(obj_id, &psn_rec) > 0);
					obj_name = psn_rec.Name;
					THROW_PP_S(psn_rec.Flags & PSNF_HASIMAGES, PPERR_OBJHASNTIMG, obj_name);
				}
				else if(obj_type == PPOBJ_TSESSION) {
					PPObjTSession tses_obj;
					TSessionTbl::Rec tses_rec;
					THROW(tses_obj.Search(obj_id, &tses_rec) > 0);
					tses_obj.MakeName(&tses_rec, obj_name);
					THROW_PP_S(tses_rec.Flags & TSESF_HASIMAGES, PPERR_OBJHASNTIMG, obj_name);
				}
				else {
					CALLEXCEPT_PP_S(PPERR_JOBSRV_GETIMG_INVOBJTYPE, name);
				}
				THROW(lf.GetZeroPositionFile(obj_type, obj_id, img_path));
				THROW_PP_S(img_path.NotEmptyS(), PPERR_OBJHASNTIMG, obj_name);
				ok = TransmitFile(tfvStart, img_path, rReply);
			}
			break;
		case PPSCMD_GETFILE:
			pEv->GetParam(1, name); // PPGetExtStrData(1, pEv->Params, name);
			if(name.CmpPrefix(MAGIC_FILETRANSMIT, 0) == 0) {
				name.ShiftLeft(strlen(MAGIC_FILETRANSMIT));
				ok = TransmitFile(tfvStart, name, rReply);
			}
			else {
				CALLEXCEPT_PP_S(PPERR_JOBSRV_FILETRANSM_INVMAGIC, name);
			}
			break;
		case PPSCMD_GETNEXTFILEPART:
			pEv->GetParam(1, name); // PPGetExtStrData(1, pEv->Params, name);
			ok = TransmitFile(tfvNext, name, rReply);
			break;
		case PPSCMD_ACKFILE:
			pEv->GetParam(1, name); // PPGetExtStrData(1, pEv->Params, name);
			ok = TransmitFile(tfvFinish, name, rReply);
			break;
		case PPSCMD_CANCELFILE:
			pEv->GetParam(1, name); // PPGetExtStrData(1, pEv->Params, name);
			ok = TransmitFile(tfvCancel, name, rReply);
			break;
		case PPSCMD_GETSERVERSTAT:
			{
				const ThreadID this_thread_id = DS.GetConstTLA().GetThreadID();

				TSCollection <PPThread::Info> list;
				DS.GetThreadInfoList(0, list);
				SSerializeContext ctx;
				int32 c = list.getCount();
				SBuffer _temp_sbuf;

				ctx.Init(0, getcurdate_());
				THROW_SL(ctx.Serialize(+1, c, _temp_sbuf));
				for(int i = 0; i < c; i++) {
					PPThread::Info * p_item = list.at(i);
					if(p_item->Id == this_thread_id && p_item->LastMsg.Empty()) {
						p_item->LastMsg = "GetServerStat";
					}
					THROW(p_item->Serialize(+1, _temp_sbuf, &ctx));
				}
				THROW_SL(ctx.SerializeState(+1, rReply));
				THROW_SL(rReply.Write(_temp_sbuf.GetBuf(_temp_sbuf.GetRdOffs()), _temp_sbuf.GetAvailableSize()));
			}
			break;
		case PPSCMD_STOPTHREAD:
			{
				PPJobSrvProtocol::StopThreadBlock blk;
				PPThread::Info tinfo;
				SSerializeContext ctx;
				ctx.Init(0, getcurdate_());
				THROW_SL(blk.Serialize(-1, *pEv, &ctx));
				DS.GetThreadInfo(blk.TId, tinfo);
				THROW(DS.StopThread(blk.TId));
				rReply.SetAck();
				{
					SString fmt_buf, msg, kind_buf;
					PPThread::GetKindText(tinfo.Kind, kind_buf);
					PPFormatT(PPTXT_SERVERTHREADSTOPPED, &msg, kind_buf.cptr(), tinfo.Text.cptr(), blk.HostName.cptr(), blk.UserName.cptr());
					PPLogMessage(PPFILNAM_SERVER_LOG, msg, LOGMSGF_TIME);
				}
			}
			break;
		case PPSCMD_CREATEVIEW:
			ok = PPView::ExecuteServer(*pEv, rReply) ? cmdretOK : cmdretError;
			break;
		case PPSCMD_DESTROYVIEW:
			ok = PPView::Destroy(*pEv, rReply) ? cmdretOK : cmdretError;
			break;
		case PPSCMD_REFRESHVIEW:
			ok = PPView::Refresh(*pEv, rReply) ? cmdretOK : cmdretError;
			break;
		case PPSCMD_SETGLOBALUSER:
			{
				pEv->GetParam(1, name); // PPGetExtStrData(1, pEv->Params, name);
				PPID   gua_id = name.ToLong();
				PPThreadLocalArea & r_tla = DS.GetTLA();
				// @ Muxa {
				r_tla.GlobAccID = 0;
				r_tla.GlobAccName = 0;
				r_tla.State &= ~PPThreadLocalArea::stExpTariffTa;
				if(name == "0" || gua_id == PPGUAID_UHTT_CORE) {
					r_tla.GlobAccID = gua_id;
					ok = cmdretOK;
				}
				// } @Muxa
				else {
					PPObjGlobalUserAcc gua_obj;
					PPGlobalUserAcc gua_rec;
					if(gua_id && gua_obj.Fetch(gua_id, &gua_rec) > 0) {
						r_tla.GlobAccID = gua_rec.ID;
						r_tla.GlobAccName = gua_rec.Name;
						ok = cmdretOK;
					}
					else if(gua_obj.SearchByName(name, &(gua_id = 0), &gua_rec) > 0) {
						r_tla.GlobAccID = gua_rec.ID;
						r_tla.GlobAccName = gua_rec.Name;
						ok = cmdretOK;
					}
					else
						ok = cmdretError;
				}
			}
			break;
		case PPSCMD_GETGLOBALUSER:
			{
				temp_buf.Cat(DS.GetConstTLA().GlobAccID);
				rReply.SetString(temp_buf);
				ok = cmdretOK;
			}
			break;
		case PPSCMD_EXPTARIFFTA:
			{
				PPThreadLocalArea & r_tla = DS.GetTLA();
				if(!(r_tla.State & PPThreadLocalArea::stExpTariffTa)) {
					r_tla.State |= PPThreadLocalArea::stExpTariffTa;
					temp_buf.Cat(1);   // on
				}
				else {
					r_tla.State &= ~PPThreadLocalArea::stExpTariffTa;
					temp_buf.Cat(0);   // off
				}
				rReply.SetString(temp_buf);
				ok = cmdretOK;
			}
			break;
		case PPSCMD_PING:
			{
				pEv->GetParam(1, temp_buf); // PPGetExtStrData(1, pEv->Params, temp_buf);
				if(temp_buf.NotEmptyS()) {
					if(temp_buf.CmpNC("TERM") == 0) {
						pEv->GetParam(2, temp_buf); // PPGetExtStrData(2, pEv->Params, temp_buf);
						SString terminal;
						if(temp_buf.NotEmptyS())
							terminal = temp_buf;
						else
							terminal.Z().CRB().CRB();
						ulong c = SLS.GetTLA().Rg.GetUniformInt(100);
						temp_buf.Z();
						for(ulong i = 0; i < c; i++) {
							temp_buf.Cat((long)SLS.GetTLA().Rg.GetUniformInt(10));
						}
						temp_buf.Cat(terminal);
						rReply.SetString(temp_buf);
					}
					else
						rReply.SetString(temp_buf.CRB());
				}
				else
					rReply.SetString("PONG");
				break;
			}
		case PPSCMD_QUIT:
			rReply.SetAck();
			ok = cmdretQuit;
			break;
		case PPSCMD_REGISTERSTYLO:
			{
				PPID   device_id = 0;
				PPObjStyloPalm sp_obj;
				PPObjStyloPalm::RegisterDeviceBlock rdb;
				SString dev_name, magic_buf;
				pEv->GetParam(1, dev_name); // PPGetExtStrData(1, pEv->Params, dev_name);
				pEv->GetParam(2, magic_buf); // PPGetExtStrData(2, pEv->Params, magic_buf);
				dev_name.Strip();
                int    sr = sp_obj.SearchBySymb(dev_name, &device_id, 0);
                if(sr < 0)
					sr = sp_obj.SearchByName(dev_name, &device_id, 0);
				THROW(sr > 0);
				THROW(sp_obj.RegisterDevice(device_id, &rdb, 1));
				ok = cmdretOK;
			}
            break;
		case PPSCMD_PREPAREPALMINDATA:
			{
				long   start = 0;
				SString dev_uuid, dev_name, path, dir;
				pEv->GetParam(1, dev_name); // PPGetExtStrData(1, pEv->Params, dev_name);
				pEv->GetParam(2, dev_uuid); // PPGetExtStrData(2, pEv->Params, dev_uuid);
				dev_uuid = dev_name; // @todo
				PPGetPath(PPPATH_SPII, dir);
				dir.SetLastSlash().Cat(dev_uuid).SetLastSlash();
				MakeTempFileName(dir, "tmpsti", "xml", 0, path);
				/* @Удаляется в ReceiveFile
				if(fileExists(path))
					SFile::Remove(path);
				*/
				rReply.SetString(path);
				{
					(path = dir).Cat("stylo.log");
					SyncTable::LogMessage(path, "\nEXCHANGE STARTED");
					SyncTable::LogMessage(path, "IMPORT");
				}
			}
			break;
		case PPSCMD_PREPAREPALMOUTDATA:
			{
				SString dev_uuid, dev_name, path, temp_path, palm_path, log_path, msg_buf;
				SString last_dt, last_tm;
				pEv->GetParam(1, dev_name); // PPGetExtStrData(1, pEv->Params, dev_name);
				pEv->GetParam(2, dev_uuid); // PPGetExtStrData(2, pEv->Params, dev_uuid);
				pEv->GetParam(3, last_dt); // PPGetExtStrData(3, pEv->Params, last_dt);
				pEv->GetParam(4, last_tm); // PPGetExtStrData(4, pEv->Params, last_tm);
				dev_uuid = dev_name; // @todo
				PPGetPath(PPPATH_SPII, palm_path);
				{
					(log_path = palm_path).SetLastSlash().Cat(dev_uuid).SetLastSlash().Cat("stylo.log");
					SyncTable::LogMessage(log_path, "EXPORT");
				}
				path = palm_path;
				path.SetLastSlash().Cat(dev_uuid).SetLastSlash();
				(temp_path = path).Cat("temp_in.zip");
				path.Cat("in.xml");
				if(fileExists(temp_path))
					SFile::Remove(temp_path);
				if(fileExists(path)) {
					const int wr = SFile::WaitForWriteSharingRelease(path, 60000); // @v8.5.11 // @v8.5.12 30000-->60000
					// @v9.0.0 THROW_SL(wr);
					THROW_PP(wr, PPERR_STYLODATALOCKED); // @v9.0.0
					if(wr > 0) {
						//PPTXT_WAITFORWRSHRFILENOTIFY "Ожидание завершения записи в файл '%zstr' продлилось %int ms"
						PPFormatT(PPTXT_WAITFORWRSHRFILENOTIFY, &msg_buf, path.cptr(), wr);
						PPLogMessage(PPFILNAM_SERVER_LOG, msg_buf, LOGMSGF_TIME);
					}
					LDATE dt = ZERODATE;
					LTIME tm = ZEROTIME;
					// SCopyFile(path, temp_path, 0, FILE_SHARE_READ, 0);
					strtodate(last_dt, DATF_DMY, &dt);
					strtotime(last_tm, TIMF_HMS, &tm);
					if(PPObjStyloPalm::XmlCmpDtm(dt, tm, path) < 0) {
						SString traf_log_path;
						PPGetPath(PPPATH_LOG, traf_log_path);
						rReply.SetString(temp_path);
						PKZip(path, temp_path, palm_path);
						PalmTcpExchange::LogTrafficInfo(temp_path, traf_log_path, dev_name, 1);
					}
					else {
						rReply.SetAck();
						SyncTable::LogMessage(log_path, "SPII OK: Tables on palm is recently that host");
					}
				}
				else {
					PPSetErrorSLib();
					SLS.SetError(SLERR_FILENOTFOUND, path);
					{
						PPGetLastErrorMessage(DS.CheckExtFlag(ECF_SYSSERVICE), temp_buf.Z());
						SyncTable::LogMessage(log_path, (msg_buf = "SPII FAIL: ").Cat(temp_buf));
					}
					CALLEXCEPT();
				}
				ok = cmdretOK;
			}
			break;
		case PPSCMD_PROCESSPALMXMLDATA:
			{
				SString temp_path, log_path, dev_name, dev_uuid;
				pEv->GetParam(1, temp_path); // PPGetExtStrData(1, pEv->Params, temp_path);
				pEv->GetParam(2, dev_name); // PPGetExtStrData(2, pEv->Params, dev_name);
				pEv->GetParam(3, dev_uuid); // PPGetExtStrData(3, pEv->Params, dev_uuid);
				{
					dev_uuid = dev_name; // @todo
					if(dev_uuid.Empty()) {
						SPathStruc sp;
						sp.Split(temp_path);
						sp.Merge(SPathStruc::fDrv|SPathStruc::fDir, log_path);
					}
					else {
						PPGetPath(PPPATH_SPII, log_path);
                        log_path.SetLastSlash().Cat(dev_uuid);
					}
					log_path.SetLastSlash().Cat("stylo.log");
				}
				if(fileExists(temp_path) > 0) {
					long start = 1;
					SString dir, path;
					SPathStruc sp;
					sp.Split(temp_path);
					sp.Merge(SPathStruc::fDrv|SPathStruc::fDir, dir);
					path = MakeTempFileName(dir, "out", "xml", &start, path);
					SCopyFile(temp_path, path, 0, FILE_SHARE_READ, 0);
					{
						(path = dir).SetLastSlash().Cat("sp_ready");
						SFile file(path, mWrite);
						file.Close();
					}
					if(dev_name.Empty()) {
						SString right;
						StringSet ss("\\");
						SPathStruc sp;

						sp.Split(temp_path);
						ss.setBuf(sp.Dir);
						sp.Dir.RmvLastSlash();
						sp.Dir.Reverse();
						sp.Dir.Divide('\\', dev_name, right);
					}
					{
						SString traf_log_path;
						PPGetPath(PPPATH_LOG, traf_log_path);
						PalmTcpExchange::LogTrafficInfo(temp_path, traf_log_path, dev_name, 0);
					}
					SFile::Remove(temp_path);
					SyncTable::LogMessage(log_path, "SPII OK: IMPORT");
				}
				else {
					rReply.SetError();
					SyncTable::LogMessage(log_path, "SPII FAIL: IMPORT");
				}
				ok = cmdretOK;
			}
			break;
		case PPSCMD_SENDSMS:
			{
				const  PPThreadLocalArea & r_tla_c = DS.GetConstTLA();
				PPGta  gta_blk;
				PPObjBill * p_bobj = BillObj;
				if(r_tla_c.GlobAccID && r_tla_c.State & PPThreadLocalArea::stExpTariffTa) {
					gta_blk.GlobalUserID = r_tla_c.GlobAccID;
					gta_blk.Op = GTAOP_SMSSEND;
					if(p_bobj) {
						p_bobj->InitGta(gta_blk);
						if(gta_blk.Quot != 0.0) {
							// Для рассылки SMS кредит не применяется!
							THROW_PP((gta_blk.SCardRest/*+ gta_blk.SCardMaxCredit*/) > 0.0, PPERR_GTAOVERDRAFT);
						}
					}
				}
				{
					SString result;
					SString old_phone;
					SString new_phone;
					SString err_msg;
					SString from;
					SString message;
					PPLogger logger;
					PPAlbatrosConfig  albtr_cfg;
					SmsClient client(&logger);

					pEv->GetParam(1, old_phone); // PPGetExtStrData(1, pEv->Params, old_phone);
					pEv->GetParam(2, from); // PPGetExtStrData(2, pEv->Params, from);
					pEv->GetParam(3, message); // PPGetExtStrData(3, pEv->Params, message);
					// @todo Error message
					THROW(!old_phone.Empty());
					THROW(!message.Empty());
					THROW(!from.Empty());
					THROW(PPAlbatrosCfgMngr::Get(&albtr_cfg));
					{
						size_t bin_size = 0;
						STempBuffer buf(message.Len() * 2);
						THROW(buf.IsValid());
						THROW(message.DecodeMime64(buf, buf.GetSize(), &bin_size));
						buf[bin_size] = 0;
						message.Z().Cat((const char *)buf).Transf(CTRANSF_UTF8_TO_INNER);
					}
					THROW(client.SmsInit_(albtr_cfg.Hdr.SmsAccID, from));
					// @v8.5.4 client.SetRecvTimeout(0);
					if(FormatPhone(old_phone, new_phone, err_msg)) {
						THROW(client.SendSms(new_phone, message, result.Z()));
						temp_buf.Z().Cat(new_phone).Space().Cat(result);
						logger.Log(temp_buf);
					}
					client.SmsRelease_();
					rReply.SetAck();
					ok = cmdretOK;
				}
				if(gta_blk.GlobalUserID) {
					gta_blk.Count = 1;
					gta_blk.Duration = ZEROTIME;
					gta_blk.Dtm = getcurdatetime_();
					GtaJournalCore * p_gtaj = DS.GetGtaJ();
					if(p_gtaj)
						THROW(p_gtaj->CheckInOp(gta_blk, 1));
				}
			}
			break;
		case PPSCMD_QUERYNATURALTOKEN:
			ok = Helper_QueryNaturalToken(pEv, rReply);
			break;
		case PPSCMD_GETPERSONBYARTICLE:
			{
				pEv->GetParam(1, temp_buf); // PPGetExtStrData(1, pEv->Params, temp_buf);
				const PPID ar_id = temp_buf.ToLong();
				const PPID psn_id = ar_id ? ObjectToPerson(ar_id, 0) : 0;
				rReply.SetString(temp_buf.Z().Cat(psn_id));
				ok = cmdretOK;
			}
			break;
		case PPSCMD_GETARTICLEBYPERSON:
			{
				pEv->GetParam(1, temp_buf); // PPGetExtStrData(1, pEv->Params, temp_buf);
				const PPID psn_id = temp_buf.ToLong();
				PPID  ar_id = 0;
				if(psn_id) {
					PPID  acs_id = 0;
					//if(PPGetExtStrData(2, pEv->Params, temp_buf) > 0 && temp_buf.NotEmptyS()) {
					if(pEv->GetParam(2, temp_buf) > 0 && temp_buf.NotEmptyS()) {
						acs_id = temp_buf.ToLong();
						if(!acs_id) {
							PPObjAccSheet acs_obj;
							acs_obj.SearchBySymb(temp_buf, &acs_id, 0);
						}
					}
					{
						PPObjArticle ar_obj;
						ar_obj.P_Tbl->PersonToArticle(psn_id, acs_id, &ar_id);
					}
				}
				rReply.SetString(temp_buf.Z().Cat(ar_id));
				ok = cmdretOK;
			}
			break;
		case PPSCMD_GETWORKBOOKCONTENT:
			THROW_PP(State & stLoggedIn, PPERR_NOTLOGGEDIN);
			{
				// @v8.3.5 MemLeakTracer mlt;
				pEv->GetParam(1, temp_buf); // PPGetExtStrData(1, pEv->Params, temp_buf);
				PPID    obj_id = temp_buf.ToLong();
				SString path;
				PPObjWorkbook wb_obj;
				PPWorkbookPacket pack;
				THROW(wb_obj.GetPacket(obj_id, &pack) > 0);
				pack.F.GetZeroPositionFile(PPOBJ_WORKBOOK, obj_id, path);
				THROW_PP_S(path.NotEmptyS(), PPERR_WORKBOOKHASNTCONTENT, pack.Rec.Name);
				ok = TransmitFile(tfvStart, path, rReply);
			}
			break;
		/*
		case PPSCMD_GETTSESSPLACESTATUS:
			THROW_PP(State & stLoggedIn, PPERR_NOTLOGGEDIN);
			{
				PPID   qk_id = 0;
				PPGetExtStrData(1, pEv->Params, temp_buf);
				PPID   tsess_id = temp_buf.ToLong();
				PPGetExtStrData(2, pEv->Params, place_code);
				if(tsess_id) {
					PPObjTSession::PlaceStatus status;
					PPObjTSession tsess_obj;
					int    r = tsess_obj.GetPlaceStatus(tsess_id, place_code, qk_id, 0, status);
					THROW(r);
				}
			}
			break;
		*/
		default:
			CALLEXCEPT_PP(PPERR_INVSERVERCMD);
			break;
	}
	CATCH
		if(!disable_err_reply)
			rReply.SetError();
		PPErrorZ();
		ok = cmdretError;
	ENDCATCH
	if(ok != cmdretResume)
		rReply.FinishWriting();
	return ok;
}
//
//
//
SLAPI PPServerSession::InitBlock::InitBlock()
{
	THISZERO();
}

SLAPI PPServerSession::PPServerSession(TcpSocket & rSock, const InitBlock & rBlk, InetAddr & rAddr) :
	PPWorkerSession(PPThread::kNetSession)/*PPThread(PPThread::kNetSession, 0, 0)*/,
	EvSubstSockFinish(Evnt::modeCreate),
	EvSubstSockStart(Evnt::modeCreate),
	EvSubstSockReady(Evnt::modeCreate)
{
	SuspendTimeout = rBlk.SuspTimeout; // 3600000;
	CloseSocketTimeout = rBlk.ClosedSockTimeout; // 60000;
	SleepTimeout = rBlk.SleepTimeout; // INFINITE;
	if(rBlk.Flags & rBlk.fDebugMode)
		State |= stDebugMode;
	SetupTxtCmdTerm(rBlk.TxtCmdTerminalCode);
	rSock.MoveToS(So);
	P_SbiiBlk = 0;
	Addr = rAddr;
}

SLAPI PPServerSession::~PPServerSession()
{
	ZDELETE(P_SbiiBlk);
}

size_t SLAPI PPServerSession::Helper_ReceiveFilePart(PPJobSrvReply::TransmitFileBlock & rBlk, SFile * pF)
{
	size_t rd_size = 0;
	uint   try_no = 0;
	THROW_PP(rBlk.PartSize, PPERR_JOBSRV_RCVFZEROPARTSIZE);
	{
		STempBuffer wr_buf(rBlk.PartSize);
		THROW_SL(wr_buf.IsValid());
		do {
			if(try_no)
				SDelay(50);
			size_t _cs = wr_buf.GetSize() - (size_t)rd_size;
			size_t recv_size = 0;
			THROW_SL(So.RecvBlock(wr_buf, _cs, &recv_size) > 0);
			rd_size += recv_size;
			THROW_SL(pF->Write(wr_buf, recv_size));
		} while(rd_size < rBlk.PartSize && ++try_no < 3);
		THROW_PP_S(rd_size == rBlk.PartSize, PPERR_JOBSRV_RCVFFAULT, rBlk.Name);
	}
	CATCH
		rd_size = 0;
	ENDCATCH
	return rd_size;
}

PPServerSession::CmdRet SLAPI PPServerSession::ReceiveFile(int verb, const char * pParam, PPJobSrvReply & rReply)
{
	char * p_buf = 0;
	long   ftb_pos_todel = -1;
	CmdRet ret = cmdretOK;
	int    set_more_flag = 0;
	size_t recv_size = 0;
	SFile * p_f = 0;
	SString temp_buf, file_path;
	PPJobSrvReply::TransmitFileBlock blk;
	if(verb == tfvStart) {
		long   m = SFile::mWrite;
		SString file_ext;
		THROW_SL(So.RecvBlock(&blk, sizeof(blk), &recv_size) > 0 && recv_size == sizeof(blk));
		file_path = pParam;
		SFileFormat::GetExt(blk.Format, file_ext);
		if(file_ext.Empty())
			file_ext = ".";
		if(blk.TransmType == blk.ttObjImage) {
			THROW_PP_S(oneof5(blk.ObjType, PPOBJ_GOODS, PPOBJ_BRAND, PPOBJ_PERSON, PPOBJ_TSESSION, PPOBJ_WORKBOOK), PPERR_JOBSRV_OBJTYPENOTSUPP, temp_buf.Z().Cat(blk.ObjType));
			m |= SFile::mBinary;
			PPMakeTempFileName("oimg", file_ext, 0, file_path);
		}
		else if(blk.TransmType == blk.ttWorkbookContent) {
			THROW_PP_S(blk.ObjType = PPOBJ_WORKBOOK, PPERR_JOBSRV_OBJTYPENOTSUPP, temp_buf.Z().Cat(blk.ObjType));
			m |= SFile::mBinary;
			PPMakeTempFileName("wbc", file_ext, 0, file_path);
		}
		else {
			file_path = pParam;
			if(!file_path.Len())
				file_path = blk.Name;
			if(fileExists(file_path)) {
				if(SFile::Remove(file_path) <= 0) {
					file_path = 0;
					THROW_SL(0);
				}
			}
		}
		{
			THROW_MEM(p_f = new SFile(file_path, m));
			THROW_SL(p_f->IsValid());
			{
				int64  rd_offs = 0;
				size_t rd_size = Helper_ReceiveFilePart(blk, p_f);
				THROW(rd_size);
				rd_offs += rd_size;
				if(rd_offs < blk.Size) {
					FTB * p_ftb = new FTB;
					THROW_MEM(p_ftb);
					blk.Cookie = ++Counter;
					p_ftb->Cookie = blk.Cookie;
					p_ftb->Offs = rd_offs;
					p_ftb->Tfb = blk;
					p_ftb->P_F = p_f; // Передаем указатель на открытый файл в структуру FTB
					p_f = 0;   // Освобождать память под p_f уже после этого не следует
					FtbList.insert(p_ftb);

					ftb_pos_todel = (long)FtbList.getCount() - 1;
					rReply.WriteString(temp_buf.Z().Cat(blk.Cookie));
				}
				else {
					file_path = p_f->GetName();
					ZDELETE(p_f);
					THROW(FinishReceivingFile(blk, file_path, rReply));
				}
			}
		}
	}
	else if(verb == tfvNext) {
		THROW_SL(So.RecvBlock(&blk, sizeof(blk), &recv_size) > 0 && recv_size == sizeof(blk));
		{
			const  int32 cookie = blk.Cookie;
			uint   ftb_pos = 0;
			THROW_PP_S(FtbList.lsearch(&cookie, &ftb_pos, CMPF_LONG), PPERR_JOBSRV_FTCOOKIENFOUND, pParam);
			FTB * p_ftb = FtbList.at(ftb_pos);
			ftb_pos_todel = (long)ftb_pos;
			THROW_PP_S(p_ftb, PPERR_JOBSRV_FTCOOKIENFOUND, pParam);
			THROW_PP_S(p_ftb->P_F && p_ftb->P_F->IsValid(), PPERR_JOBSRV_FTCOOKIEFINVAL, p_ftb->Tfb.Name);
			{
				int64  rd_offs = p_ftb->Offs;
				int64  rd_size = Helper_ReceiveFilePart(blk, p_ftb->P_F);
				THROW(rd_size);
				rd_offs += rd_size;
				if(rd_offs < blk.Size) {
					p_ftb->Cookie = blk.Cookie;
					p_ftb->Offs = rd_offs;
					p_ftb->Tfb = blk;
					rReply.WriteString(temp_buf.Z().Cat(cookie));
				}
				else {
					THROW(FinishReceivingFile(blk, p_ftb->P_F->GetName(), rReply));
					RealeasFtbEntry(ftb_pos);
				}
			}
		}
	}
	// ftvCancel обрабатывается функцией PPServerSession::TransmitFile(
	CATCH
		rReply.SetError();
		ret = cmdretError;
		if(file_path.Len())
			SFile::Remove(file_path);
	ENDCATCH
	ZDELETE(p_f);
	return ret;
}
/*
	Сценарий теста:

	Client                         Server
	------                         ------
	"TESTPAPYRUSSERVER\xD\xA" >>
								   for i = 1; i <= 16*1024; i++
								   {
								   <<    i
								   <<    buf[i]
	CRC32 of buf[i] >>
								   }
								   << "END OF RANDOM SERIES\xD\xA"
	uint16 number_of_lines >>
	for i = 0; i < number_of_lines; i++
	{
		"line\n" >>
	}
								   << All received lines
	"QUIT\xD\xA" >>
*/

static const char * P_TestEndOfSeries = "END OF RANDOM SERIES";

//static
int PPServerSession::TestingClient(TcpSocket & rSo, StrAssocArray & rStrList)
{
	int    ok = 1;
	size_t actual_size;
	CRC32  crc;
	ulong  c, clen;
	uint   max_size = 16*1024;
	char * p_buf = (char *)SAlloc::M(max_size+sizeof(ulong));
	SString msg_buf, line_buf;
	(line_buf = "TESTPAPYRUSSERVER").CRB();
	THROW(TestSend(rSo, line_buf.cptr(), line_buf.Len(), &actual_size) > 0);
	//
	// Принимаем от сервера случайные последовательности байт.
	//
	do {
		THROW(TestRecvBlock(rSo, &clen, sizeof(clen), &actual_size) > 0);
		if(clen == 0) {
			THROW(TestRecv(rSo, p_buf, max_size, &actual_size));
			if(strncmp(p_buf, P_TestEndOfSeries, strlen(P_TestEndOfSeries)) != 0) {
				msg_buf.Printf("Expected string '%s'", P_TestEndOfSeries);
				PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_COMP);
			}
			break;
		}
		else {
			THROW(TestRecvBlock(rSo, p_buf, clen, &actual_size) > 0);
			c = crc.Calc(0, (uint8 *)p_buf, clen);
			THROW(TestSend(rSo, &c, sizeof(c), &actual_size) > 0);
		}
	} while(1);
	{
		uint   i;
		uint16 num_lines = (uint16)rStrList.getCount();
		THROW(TestSend(rSo, &num_lines, sizeof(num_lines), &actual_size) > 0);
		for(i = 0; i < num_lines; i++) {
			line_buf = rStrList.at(i).Txt;
			line_buf.CR();
			THROW(TestSend(rSo, line_buf.cptr(), line_buf.Len(), &actual_size) > 0);
		}
		{
			SBuffer buf;
			int r = 1;
			while(r > 0) {
				if(rSo.Select(TcpSocket::mRead, 10000)) {
					if(!rSo.RecvBuf(buf, 0, &actual_size)) {
						PPSetErrorSLib();
						PPLogMessage(PPFILNAM_DEBUG_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_COMP|LOGMSGF_LASTERR);
						ok = 0;
					}
					else if(actual_size == 0)
						r = -1;
				}
				else
					r = -1;
			}
			if(r) {
				i = 0;
				while((actual_size = buf.ReadTerm("\n", p_buf, max_size)) != 0) {
					line_buf.CopyFromN(p_buf, actual_size).Chomp();
					if(line_buf.Cmp(rStrList.at(i).Txt, 0) != 0) {
						msg_buf.Printf("Returned line not equal to sended line '%s'", line_buf.cptr());
						PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_COMP);
					}
					i++;
				}
			}
		}
	}
	(line_buf = "QUIT").CRB();
	THROW(TestSend(rSo, line_buf, line_buf.Len(), &actual_size) > 0);
	CATCHZOK
	SAlloc::F(p_buf);
	return ok;
}

//static
int PPServerSession::TestSend(TcpSocket & rSo, const void * pBuf, size_t sz, size_t * pActualSize)
{
	int    ok = 1;
	size_t actual_size;
	SString msg_buf;
	if(!rSo.Send(pBuf, sz, &actual_size)) {
		PPSetErrorSLib();
		PPLogMessage(PPFILNAM_DEBUG_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_COMP|LOGMSGF_LASTERR);
		ok = 0;
	}
	else if(actual_size != sz) {
		msg_buf.Printf("It's sended invlid count of bytes (%u vs %u)", actual_size, sz);
		PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_COMP);
		ok = -1;
	}
	ASSIGN_PTR(pActualSize, actual_size);
	return ok;
}

//static
int PPServerSession::TestRecv(TcpSocket & rSo, void * pBuf, size_t sz, size_t * pActualSize)
{
	int    ok = 1;
	size_t actual_size;
	SString msg_buf;
	if(!rSo.Recv(pBuf, sz, &actual_size)) {
		PPSetErrorSLib();
		PPLogMessage(PPFILNAM_DEBUG_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_COMP|LOGMSGF_LASTERR);
		ok = 0;
	}
	else if(actual_size != sz) {
		msg_buf.Printf("It's received inavlid count of bytes (%u vs %u)", actual_size, sz);
		PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_COMP);
		ok = -1;
	}
	ASSIGN_PTR(pActualSize, actual_size);
	return ok;
}

//static
int PPServerSession::TestRecvBlock(TcpSocket & rSo, void * pBuf, size_t sz, size_t * pActualSize)
{
	int    ok = 1;
	size_t actual_size;
	SString msg_buf;
	if(!rSo.RecvBlock(pBuf, sz, &actual_size)) {
		PPSetErrorSLib();
		PPLogMessage(PPFILNAM_DEBUG_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_COMP|LOGMSGF_LASTERR);
		ok = 0;
	}
	else if(actual_size != sz) {
		msg_buf.Printf("It's received invalid count of bytes (%u vs %u)", actual_size, sz);
		PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_COMP);
		ok = -1;
	}
	ASSIGN_PTR(pActualSize, actual_size);
	return ok;
}

PPServerSession::CmdRet SLAPI PPServerSession::Testing()
{
	CmdRet ok = cmdretQuit;
	uint   max_size = 16*1024;
	SString msg_buf, line_buf;
	//
	// Посылаем клиенту буферы длинами от 1 до max_size байт
	// со случайным содержимым.
	// В ответ должны получить CRC32 этих последовательностей.
	//
	char * p_buf = (char *)SAlloc::M(max_size+sizeof(ulong));
	SRandGenerator & r_rg = SLS.GetTLA().Rg;
	CRC32  crc;
	ulong  c = 0;
	size_t actual_size;
	for(uint i = 1; i <= 16*1024; i++) {
		for(uint j = 0; j <= i/sizeof(ulong); j += sizeof(ulong)) {
			PTR32(p_buf)[j] = r_rg.Get();
		}
		c = i;
		//
		// Передаем длину буфера
		//
		THROW(TestSend(So, &c, sizeof(c), &actual_size) > 0);
		c = crc.Calc(0, (uint8 *)p_buf, i);
		THROW(TestSend(So, p_buf, i, &actual_size) > 0);
		ulong rc = 0;
		THROW(TestRecvBlock(So, &rc, sizeof(rc), &actual_size) > 0);
		if(rc != c)
			PPLogMessage(PPFILNAM_DEBUG_LOG, "Received CRC not equal to sended series", LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_COMP);
	}
	//
	// Посылаем клиенту сигнал о том, что все серии переданы (32-разрядный 0 и строка).
	//
	THROW(TestSend(So, &(c = 0), sizeof(c), &actual_size) > 0);
	(msg_buf = P_TestEndOfSeries).CRB();
	THROW(TestSend(So, msg_buf.cptr(), msg_buf.Len(), &actual_size) > 0);
	//
	// Теперь принимаем от клиента количество строк, которые он нам отошлет (2-x байтовое число)
	//
	uint16 num_lines = 0;
	THROW(TestRecvBlock(So, &num_lines, sizeof(num_lines), &actual_size) > 0);
	{
		//
		// Принимаем строки в общий пул. Каждая строка должна завершаться символом '\n'.
		//
		SBuffer str_pool_buf;
		int r = 1;
		while(r > 0) {
			if(So.Select(TcpSocket::mRead, 10000)) {
				if(!So.RecvBuf(str_pool_buf, 0, &actual_size)) {
					PPSetErrorSLib();
					PPLogMessage(PPFILNAM_DEBUG_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_COMP|LOGMSGF_LASTERR);
					r = 0;
				}
				else if(actual_size == 0)
					r = -1;
			}
			else
				r = -1;
		}
		if(r) {
			StringSet ss;
			while((actual_size = str_pool_buf.ReadTerm("\n", p_buf, max_size)) != 0) {
				msg_buf.CopyFromN(p_buf, actual_size);
				ss.add(msg_buf.Chomp());
			}
			uint ssc = ss.getCount();
			if(ssc != num_lines) {
				msg_buf.Printf("Number of accepted lines not equal to declared count (%u vs %u)", ssc, (uint)num_lines);
				PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_COMP);
			}
			//
			// Передаем обратно клиенту строки, которые от него получили (дополняем завершающим '\n')
			//
			for(uint j = 0; ss.get(&j, line_buf);) {
				line_buf.CR();
				THROW(TestSend(So, line_buf.cptr(), line_buf.Len(), &actual_size) > 0);
			}
		}
	}
	CATCH
		ok = cmdretError;
	ENDCATCH
	SAlloc::F(p_buf);
	return ok;
}
/*

Протокол обмена данными при обработке команд:

	Command:

		Если первые два байта команды нулевые, то далее следует бинарный пакет команды,
		в противном случае - команда символьная.

	Reply:

	{
	}

*/
PPServerSession::CmdRet SLAPI PPServerSession::ProcessCommand(PPServerCmd * pEv, PPJobSrvReply & rReply)
{
	CmdRet ok = PPWorkerSession::ProcessCommand(pEv, rReply);
	if(ok == cmdretUnprocessed) {
		int    disable_err_reply = 0;
		int    r = 0;
		SString reply_buf, temp_buf, name, db_symb;
		// (StartWriting уже был выполнен вызовом PPWorkerSession::ProcessCommand) THROW(rReply.StartWriting());
		switch(pEv->GetH().Type) {
			//
			// Эти команды обмениваются по собственному протоколу {
			//
			case PPSCMD_SPII:
				{
					disable_err_reply = 1;
					SString log_path, path, devl_path, install_path;
					PPGetPath(PPPATH_SPII, path);
					PPGetPath(PPPATH_LOG, log_path);
					PPGetFilePath(PPPATH_BIN, PPFILNAM_TCPALLOWEDDEVICELIST, devl_path);
					{
						PPGetPath(PPPATH_BIN, install_path);
						install_path.RmvLastSlash();
						install_path.TrimToDiv(install_path.Len() - 1, "\\").SetLastSlash().Cat("install");
					}
					PalmTcpExchange p_e(&So, path, log_path, devl_path, install_path);
					// Если выполнение обмена закончилось с ошибкой, то все равно обрываем соединение. (обрывается по ok = 100)
					//
					//
					if(SpiiExchange(&p_e, 0, 0))
						rReply.SetString(temp_buf.Z().Cat(100));
					else if(!disable_err_reply)
						rReply.SetError();
					ok = cmdretQuit;
				}
				break;
			case PPSCMD_GETBIZSCORES:
				disable_err_reply = 1;
				//
				// Login не требуется (GetBizScoresVals сама выполняет DS.Login)
				//
				pEv->GetParam(1, name); // PPGetExtStrData(1, pEv->Params, name);
				pEv->GetParam(2, temp_buf); // PPGetExtStrData(2, pEv->Params, temp_buf);
				THROW(GetBizScoresVals(name, temp_buf, &So));
				ok = cmdretQuit;
				//
				// GetBizScoresVals сама ответит клиенту. Поэтому reply здесь не формируется.
				//
				break;
			case PPSCMD_STYLOBHT:
				disable_err_reply = 1;
				THROW(StyloBHTExch(&So));
				ok = cmdretQuit;
				break;
			//
			// }
			//
			case PPSCMD_SUSPEND:
				if(pEv->GetParam(1, name) > 0) { //if(PPGetExtStrData(1, pEv->Params, name) > 0) {
					long timeout = name.ToLong();
					if(timeout > 0 && timeout <= 1000000)
						SuspendTimeout = timeout * 1000;
				}
				rReply.SetString(temp_buf.Z().Cat(DS.GetTLA().GetId()));
				ok = cmdretSuspend;
				break;
			case PPSCMD_RESUME:
				{
					PPThread::Info thread_info;
					PPJobSrvReply reply;
					reply = rReply;
					pEv->GetParam(1, temp_buf); // PPGetExtStrData(1, pEv->Params, temp_buf);
					long id = temp_buf.ToLong();
					reply.SetAck();
					reply.FinishWriting();
					THROW(r = DS.SetThreadSock(id, So, &reply));
					if(r < 0) {
						//
						// Поток, на который мы хотим переключиться занят (не ответил в течении заданного времени).
						//
						reply.Clear();
						reply.SetString("BUSY");
						ok = cmdretOK;
					}
					else {
						ok = cmdretResume;
					}
					rReply = reply;
				}
				break;
			case PPSCMD_SETIMAGE:
				THROW_PP(State & stLoggedIn, PPERR_NOTLOGGEDIN);
				THROW(ReceiveFile(tfvStart, temp_buf.Z(), rReply));
				break;
			case PPSCMD_PUTFILE:
				pEv->GetParam(1, name); // PPGetExtStrData(1, pEv->Params, name);
				if(name.CmpPrefix(MAGIC_FILETRANSMIT, 0) == 0)
					name.ShiftLeft(strlen(MAGIC_FILETRANSMIT));
				else
					name = 0;
				ok = ReceiveFile(tfvStart, name, rReply);
				break;
			case PPSCMD_PUTNEXTFILEPART:
				ok = ReceiveFile(tfvNext, name, rReply);
				break;
			case PPSCMD_TEST:
				ok = Testing();
				break;
			case PPSCMD_GETDISPLAYINFO:
				{
					long   id = 0;
					SString str_id, str_buf;
					pEv->GetParam(1, str_id); // PPGetExtStrData(1, pEv->Params, str_id);
					if((id = str_id.ToLong()) > 0) {
						int r = 0;
						PalmDisplayBlock blk;
						THROW(PPObjStyloPalm::LockDisplayQueue(id));
						THROW(r = PPObjStyloPalm::PeekDisplayBlock(id, blk, 0));
						if(blk.DirectMsg.Len()) {
							(str_buf = blk.DirectMsg).Transf(CTRANSF_INNER_TO_OUTER);
							rReply.Write(str_buf.cptr(), str_buf.Len());
							rReply.SetDataType(PPJobSrvReply::htFile, 0);
							rReply.FinishWriting();
							THROW(SendReply(rReply));
							{
								size_t rcv_bytes = 0;
								char recv_buf[256];
								THROW(So.Recv(recv_buf, sizeof(recv_buf), &rcv_bytes));
								if(r > 0 && rcv_bytes > 0) {
									THROW(PPObjStyloPalm::PopDisplayBlock(id, &blk, 0));
									str_buf = "1";
								}
							}
						}
						THROW(PPObjStyloPalm::UnlockDisplayQueue(id));
					}
					rReply.Clear();
					rReply.SetAck();
					rReply.SetDataType(PPJobSrvReply::htFile, 0);
					ok = cmdretQuit;
				}
				break;
			default:
				CALLEXCEPT_PP(PPERR_INVSERVERCMD);
				break;
		}
		CATCH
			if(!disable_err_reply)
				rReply.SetError();
			PPErrorZ();
			ok = cmdretError;
		ENDCATCH
		if(ok != cmdretResume)
			rReply.FinishWriting();
	} // Если родительский класс вернул что-то отличное от cmdretUnprocessed, то - немедленный выход с этим результатом
	return ok;
}
//
//
//
//static
const int16 PPJobSrvProtocol::CurrentProtocolVer = 1;

SString & FASTCALL PPJobSrvProtocol::Header::ToStr(SString & rBuf) const
{
	rBuf.Z();
	if(Zero == 0) {
		rBuf.CatEq("ProtocolVer", (long)ProtocolVer).CatDiv(';', 2).
			CatEq("DataLen", DataLen).CatDiv(';', 2).
			CatEq("Type", Type).CatDiv(';', 2).
			Cat("Flags").Eq().CatHex(Flags);
	}
	else {
		rBuf.CatChar(((char *)&Zero)[0]);
		rBuf.CatChar(((char *)&Zero)[1]);
	}
	return rBuf;
}

SLAPI PPJobSrvProtocol::PPJobSrvProtocol() : SBuffer()
{
	State = 0;
	P_TokAck = "ACK";
	P_TokErr = "ERROR";
}

int SLAPI PPJobSrvProtocol::TestSpecToken(const char * pTok)
{
	int    yes = 0;
	size_t spec_sz = strlen(pTok);
	if(GetAvailableSize() == (spec_sz+2)) {
		STempBuffer temp_buf(spec_sz+2+1);
		THROW_SL(ReadStatic(temp_buf, temp_buf.GetSize()-1));
		temp_buf[temp_buf.GetSize()-1] = 0;
		if(strncmp(temp_buf, pTok, spec_sz) == 0)
			yes = 1;
	}
	CATCH
		yes = 0;
	ENDCATCH
	return yes;
}

int SLAPI PPJobSrvProtocol::StartReading(SString * pRepString)
{
	int    ok = 1;
	ASSIGN_PTR(pRepString, 0);
	ErrText = 0;
	State |= stReading;
	size_t avl_sz = GetAvailableSize();
	THROW_PP(avl_sz >= 2, PPERR_JOBSRV_INVREPLYSIZE);
	THROW_SL(ReadStatic(&H, 2));
	if(H.Zero == 0) {
		THROW_SL(Read(&H, sizeof(H)));
		THROW_PP(H.DataLen == avl_sz, PPERR_JOBSRV_MISSMATCHREPLYSIZE);
		if(H.Flags & hfRepError) {
			if(H.Type == htGenericText)
				ErrText.CatN((const char *)Ptr(GetRdOffs()), GetAvailableSize());
		}
	}
	else {
		MEMSZERO(H);
		H.Flags  |= hfSlString;
		H.DataLen = (int32)avl_sz;
		if(TestSpecToken(P_TokErr))
			H.Flags |= hfRepError;
		else if(TestSpecToken(P_TokAck))
			H.Flags |= hfAck;
		CALLPTRMEMB(pRepString, CatN((const char *)Ptr(GetRdOffs()), avl_sz));
		ok = 2;
	}
	CATCHZOK
	return ok;
}

SString & FASTCALL PPJobSrvProtocol::ToStr(SString & rBuf) const
{
	if(State & stStructured) {
		H.ToStr(rBuf);
	}
	else {
		size_t avl_sz = MIN(GetWrOffs(), 255);
		rBuf.Z().CatN((const char *)GetBuf(), avl_sz);
		rBuf.Chomp();
	}
	return rBuf;
}

int SLAPI PPJobSrvProtocol::CheckRepError()
{
	if(H.Flags & hfRepError) {
		PPSetError(PPERR_JOBSRVCLI_ERR, ErrText);
		return 0;
	}
	else
		return 1;
}

SLAPI PPJobSrvProtocol::StopThreadBlock::StopThreadBlock()
{
	TId = 0;
	MAddr.Init();
}

int SLAPI PPJobSrvProtocol::StopThreadBlock::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->SerializeBlock(dir, sizeof(TId)+sizeof(MAddr), &TId, rBuf, 0));
	THROW_SL(pCtx->Serialize(dir, HostName, rBuf));
	THROW_SL(pCtx->Serialize(dir, UserName, rBuf));
	CATCHZOK
	return ok;
}

SLAPI PPJobSrvProtocol::TransmitFileBlock::TransmitFileBlock()
{
	CrtTime.SetZero();
	AccsTime.SetZero();
	ModTime.SetZero();
	Size = 0;
	Format = SFileFormat::Unkn;
	Flags = 0;
	Cookie = 0;
	PartSize = 0;
	TransmType = ttGeneric;
	Reserve = 0;
	ObjType = 0;
	ObjID = 0;
	memzero(Hash, sizeof(Hash));
	memzero(Reserve2, sizeof(Reserve));
	memzero(Name, sizeof(Name));
}

int SLAPI PPJobSrvProtocol::Helper_Recv(TcpSocket & rSo, const char * pTerminal, size_t * pActualSize)
{
	int    ok = 1;
	const  size_t zs = sizeof(H.Zero);
	size_t actual_size = 0;
	MEMSZERO(H);
	Clear();
	THROW_SL(rSo.RecvBlock(&H.Zero, zs, &actual_size));
	if(H.Zero == 0) {
		THROW_SL(rSo.RecvBlock(PTR8(&H) + zs, sizeof(H) - zs, &actual_size));
		THROW_SL(Write(&H, sizeof(H)));
		if(H.DataLen > sizeof(H))
			THROW_SL(rSo.RecvBuf(*this, H.DataLen - sizeof(H), &actual_size));
		State |= stStructured; // @v8.7.4
	}
	else {
		WriteByte(((char *)&H)[0]);
		WriteByte(((char *)&H)[1]);
		if(pTerminal != 0) {
			THROW_SL(rSo.RecvUntil(*this, pTerminal, &actual_size));
		}
		else {
			THROW_SL(rSo.RecvBuf(*this, 0, &actual_size));
		}
		actual_size += zs;
		State &= ~stStructured; // @v8.7.4
	}
	CATCHZOK
	ASSIGN_PTR(pActualSize, actual_size);
	return ok;
}
//
//
//
SLAPI PPJobSrvCmd::PPJobSrvCmd() : PPJobSrvProtocol()
{
}

int SLAPI PPJobSrvCmd::StartWriting(int cmdId)
{
	int    ok = 1;
	MEMSZERO(H);
	H.ProtocolVer = CurrentProtocolVer;
	H.Type = cmdId;
	H.DataLen = sizeof(H);
	Clear();
	THROW_SL(Write(&H, sizeof(H)));
	State |= stStructured;
	State &= ~stReading;
	CATCHZOK
	return ok;
}

int SLAPI PPJobSrvCmd::StartWriting(const char * pStr)
{
	int    ok = 1;
	Clear();
	if(pStr)
		Write(pStr, strlen(pStr));
	State &= ~stStructured;
	State &= ~stReading;
	return ok;
}

int SLAPI PPJobSrvCmd::FinishWriting()
{
	int    ok = 1;
	if(State & stStructured) {
		((Header *)Ptr(GetRdOffs()))->DataLen = (int32)GetAvailableSize();
	}
	else {
		Write("\xD\xA", 2);
	}
	return ok;
}
//
//
//
SLAPI PPJobSrvReply::PPJobSrvReply(PPServerSession * pSess) : PPJobSrvProtocol()
{
	P_Sess = pSess;
}

int SLAPI PPJobSrvReply::StartWriting()
{
	int    ok = 1;
	SetDataType(htGeneric, 0);
	MEMSZERO(H);
	H.ProtocolVer = CurrentProtocolVer;
	H.Type = DataType;
	Clear();
	THROW_SL(Write(&H, sizeof(H)));
	State |= stStructured;
	State &= ~stReading;
	CATCHZOK
	return ok;
}

int SLAPI PPJobSrvReply::SetDataType(int dataType, const char * pDataTypeText)
{
	DataType = dataType;
	DataTypeText = pDataTypeText;
	return 1;
}

int FASTCALL PPJobSrvReply::WriteString(const SString & rStr)
{
	return Write(rStr.cptr(), rStr.Len()) ? 1 : PPSetErrorSLib();
}

int FASTCALL PPJobSrvReply::FinishWriting(int hdrFlags)
{
	int    ok = 1;
	if(State & stStructured) {
		((Header *)Ptr(GetRdOffs()))->DataLen = (int32)GetAvailableSize();
		((Header *)Ptr(GetRdOffs()))->Type = DataType;
		if(hdrFlags)
			((Header *)Ptr(GetRdOffs()))->Flags |= hdrFlags;
	}
	else {
		Write("\xD\xA", 2);
	}
	return ok;
}

int FASTCALL PPJobSrvReply::SetString(const char * pStr)
{
	int    ok = 1;
	Clear();
	if(pStr)
		Write(pStr, strlen(pStr));
	State &= ~stStructured;
	State &= ~stReading;
	return ok;
}

int FASTCALL PPJobSrvReply::SetString(const SString & rStr)
{
	int    ok = 1;
	Clear();
	Write(rStr, rStr.Len());
	State &= ~stStructured;
	State &= ~stReading;
	return ok;
}

int FASTCALL PPJobSrvReply::SetInformer(const char * pMsg)
{
	int    ok = 1;
	THROW(StartWriting());
	if(pMsg) {
		SetDataType(htGenericText, 0);
		THROW_SL(Write(pMsg, strlen(pMsg)));
	}
	THROW(FinishWriting(hfInformer));
	State &= ~stReading;
	CATCHZOK
	return ok;
}

int SLAPI PPJobSrvReply::SetAck()
{
	return SetString(P_TokAck);
}

int SLAPI PPJobSrvReply::SetError()
{
	int    ok = 1;
	SString text;
	SetDataType(htGenericText, 0);
	MEMSZERO(H);
	H.ProtocolVer = CurrentProtocolVer;
	H.Type = DataType;
	H.Flags |= hfRepError;
	Clear();
	THROW_SL(Write(&H, sizeof(H)));
	PPGetLastErrorMessage(1, text);
	THROW_SL(Write(text, text.Len()));
	State |= stStructured;
	State &= ~stReading;
	CATCHZOK
	return ok;
}

int FASTCALL PPJobSrvReply::SendInformer(const char * pMsg)
{
	int    ok = 1;
	if(P_Sess && P_Sess->IsConsistent()) {
		PPJobSrvReply reply;
		THROW(reply.SetInformer(pMsg));
		THROW(P_Sess->SendReply(reply));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
//
//
int FASTCALL PPServerSession::SendReply(PPJobSrvReply & rReply)
{
	return So.SendBuf(rReply, 0) ? 1 : PPSetErrorSLib();
}

// virtual
int SLAPI PPServerSession::SubstituteSock(TcpSocket & rS, PPJobSrvReply * pReply)
{
	//
	// Note: Функция выполняется вне потока PPServerSession
	//
	int    ok = -1;
	//
	// Сигнализируем потоку о необходимости переключить сокет
	//
	EvSubstSockStart.Signal();
	//
	// ждем ответного сигнала от потока
	//
	int    r = EvSubstSockReady.Wait(30000);
	if(r > 0) {
		//
		// Поток готов к переключению сокета
		//
		if(So.CopyS(rS)) {
			if(pReply)
				SendReply(*pReply);
			ok = 1;
		}
		else
			ok = 0;
		EvSubstSockFinish.Signal();
	}
	else {
		//
		// В течении заданного таймаута поток не ответил на запрос о подмене сокета
		//
		EvSubstSockStart.Reset();
		ok = -1;
	}
	return ok;
}

// virtual
void SLAPI PPServerSession::Shutdown()
{
	ZDELETE(P_SbiiBlk);
	PPWorkerSession::Shutdown();
}

void SLAPI PPServerSession::Run()
{
	#define INTERNAL_ERR_INVALID_WAITING_MODE 0

	enum WaitingMode {
		wmodActiveSession = 0,            // Активная сессия //
		wmodActiveSession_TransportError, // Активная сессия с ошибкой приема-передачи. В этом режиме сессия готова восстановить соединение.
		wmodActiveSession_AfterReconnection, // @construct Активная сессия несопсредственно после восстановления соединения //
		wmodSuspended,                    // Приостановленная сессия //
		wmodDisconnected,                 // Сессия с разорванным соединением
		wmodStyloBhtII                    // Сессия находится в режиме обмена с терминалом StyloBHT II
	};
	WaitingMode waiting_mode = wmodActiveSession;
	//
	SString s, fmt_buf, log_buf, temp_buf;
	PPJobSrvReply reply(this);
	PPServerCmd cmd;
	SString _s, log_file_name, debug_log_file_name;
	Evnt   stop_event(SLS.GetStopEventName(_s), Evnt::modeOpen);
	WSAEVENT sock_event = WSACreateEvent();
	LDATETIME tm_start = getcurdatetime_();
	PPGetFilePath(PPPATH_LOG, PPFILNAM_SERVER_LOG, log_file_name);
	PPGetFilePath(PPPATH_LOG, PPFILNAM_SERVERDEBUG_LOG, debug_log_file_name);
	while(1) {
		uint32 wait_obj_substsock = INFINITE;
		uint32 wait_obj_sock = INFINITE;
		uint32 wait_obj_localstop = INFINITE;
		uint32 wait_obj_stop = INFINITE;
		uint   h_count = 0;
		HANDLE h_list[32];
		uint32 timeout = CloseSocketTimeout;
		switch(waiting_mode) {
			case wmodActiveSession:
			case wmodActiveSession_AfterReconnection: // @v8.0.8
			case wmodStyloBhtII: // @v8.3.6
				timeout = SleepTimeout;
				WSAEventSelect(So, sock_event, ((waiting_mode == wmodActiveSession) ? (FD_READ|FD_CLOSE) : FD_READ));
				h_list[h_count++] = sock_event;
				wait_obj_sock = WAIT_OBJECT_0+h_count-1;
				//
				// Пока мы здесь оставим реакцию на восстановление сессии, но
				// в дальнейшем ее следует убрать.
				//
				h_list[h_count++] = EvSubstSockStart;
				wait_obj_substsock = WAIT_OBJECT_0+h_count-1;
				if(waiting_mode != wmodStyloBhtII)
					waiting_mode = wmodActiveSession; // @v8.0.8
				break;
			case wmodActiveSession_TransportError:
				timeout = SleepTimeout;
				WSAEventSelect(So, sock_event, FD_READ|FD_CLOSE);
				h_list[h_count++] = sock_event;
				wait_obj_sock = WAIT_OBJECT_0+h_count-1;
				h_list[h_count++] = EvSubstSockStart;
				wait_obj_substsock = WAIT_OBJECT_0+h_count-1;
				break;
			case wmodSuspended:
				timeout = SuspendTimeout;
				// @v8.0.0 WSAEventSelect(So, sock_event, 0);
				h_list[h_count++] = EvSubstSockStart;
				wait_obj_substsock = WAIT_OBJECT_0+h_count-1;
				break;
			case wmodDisconnected:
				timeout = CloseSocketTimeout;
				// @v8.0.0 WSAEventSelect(So, sock_event, 0);
				h_list[h_count++] = EvSubstSockStart;
				wait_obj_substsock = WAIT_OBJECT_0+h_count-1;
				break;
			default:
				assert(INTERNAL_ERR_INVALID_WAITING_MODE);
				break;
		}
		h_list[h_count++] = EvLocalStop;
		wait_obj_localstop = WAIT_OBJECT_0+h_count-1;
		h_list[h_count++] = stop_event;
		wait_obj_stop = WAIT_OBJECT_0+h_count-1;
		//
		uint   r = WaitForMultipleObjects(h_count, h_list, 0, timeout);
		if(r == WAIT_TIMEOUT) {
			int    msg_id;
			switch(waiting_mode) {
				case wmodActiveSession:
				case wmodActiveSession_AfterReconnection: 
				case wmodStyloBhtII: msg_id = PPTXT_LOG_SRVSESSINTRBYTIMEOUT_ACTV; break; // @v8.3.6
				case wmodActiveSession_TransportError: msg_id = PPTXT_LOG_SRVSESSINTRBYTIMEOUT_ACTV; break;
				case wmodSuspended: msg_id = PPTXT_LOG_SRVSESSINTRBYTIMEOUT_SUSP; break;
				case wmodDisconnected: msg_id = PPTXT_LOG_SRVSESSINTRBYTIMEOUT_DSCN; break;
				default: 
					msg_id = PPTXT_LOG_SRVSESSINTRBYTIMEOUT_ACTV;
					assert(INTERNAL_ERR_INVALID_WAITING_MODE);
					break;
			}
			s.Printf(PPLoadTextS(msg_id, fmt_buf), GetUniqueSessID());
			PPLogMessage(PPFILNAM_SERVER_LOG, s, LOGMSGF_TIME|LOGMSGF_COMP);
			break;
		}
		else if(r == wait_obj_sock) {
			WSANETWORKEVENTS nwev;
			int ene_ret = WSAEnumNetworkEvents(So, 0, &nwev);
			if(ene_ret == 0) {
				if(nwev.lNetworkEvents & FD_READ) {
					tm_start = getcurdatetime_();
					if(waiting_mode == wmodActiveSession_TransportError) {
						//
						// Перед этим была встречена ошибка сетевого транспорта.
						// Переключаем снова режим на обычную активную сессию (возможно проблемы исчезли).
						//
						waiting_mode = wmodActiveSession;
					}
					if(waiting_mode == wmodStyloBhtII) {
						int  sbii_r = P_SbiiBlk ? P_SbiiBlk->ProcessSocketInput(So) : 0;
						if(sbii_r <= 0) {
							waiting_mode = wmodActiveSession;
							break;
						}
					}
					else if(cmd.Helper_Recv(So, P_TxtCmdTerminal, 0)) {
						reply.Clear();
						CmdRet cmdret = cmdretError;
						switch(cmd.StartReading(&s)) {
							case 2: // Текстовая команда
								{
									const uint64 tm_start = SLS.GetProfileTime();
									log_buf = 0;
									{
										//
										// Убираем терминальную последовательность из считанной строки
										//
										if(!isempty(P_TxtCmdTerminal)) {
											size_t tl = 0;
											while(P_TxtCmdTerminal[tl] != 0)
												tl++;
											while(tl && s.Last() == P_TxtCmdTerminal[tl-1]) {
												s.TrimRight();
												tl--;
											}
										}
										else
											s.Chomp();
									}
									if(cmd.ParseLine(s, (State & stLoggedIn) ? cmd.plfLoggedIn : 0)) {
										int    log_level = 1;
										const  int is_login_cmd = BIN(cmd.GetH().Type == PPSCMD_LOGIN);
										if(cmd.GetH().Type == PPSCMD_HELLO)
											log_level = 0;
										else if(is_login_cmd)
											log_level = 2;
										// @v8.3.4 {
										if(State & stDebugMode) {
											Addr.ToStr(0, log_buf);
											log_buf.Space().Cat("SERVER REQ").CatDiv(':', 2);
											if(is_login_cmd) {
												cmd.GetParam(1, fmt_buf); // PPGetExtStrData(1, cmd.Params, fmt_buf);
												log_buf.Cat("LOGIN").Space().Cat(fmt_buf);
											}
											else {
												log_buf.Cat(s);
											}
											PPLogMessage(debug_log_file_name, log_buf, LOGMSGF_TIME|LOGMSGF_THREADINFO);
										}
										// } @v8.3.4
										cmdret = ProcessCommand(&cmd, reply);
										if(log_level) {
											const uint64 tm_finish = SLS.GetProfileTime();
											log_buf.Z().Cat("CMD").CatDiv(':', 2);
											if(log_level == 2) { // LOGIN вносим в журнал без пароля //
												cmd.GetParam(1, fmt_buf); // PPGetExtStrData(1, cmd.Params, fmt_buf);
												log_buf.Cat("LOGIN").Space().Cat(fmt_buf);
											}
											else
												log_buf.Cat(s);
											log_buf.Space().Cat("RET").CatDiv(':', 2).Cat(cmdret).Space().Cat("TIME").CatDiv(':', 2).Cat((int64)(tm_finish-tm_start));
											PPLogMessage(log_file_name, log_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER|LOGMSGF_THREADINFO);
										}
									}
									else {
										PPLogMessage(log_file_name, 0, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER|LOGMSGF_LASTERR|LOGMSGF_THREADINFO);
										reply.SetError();
										reply.FinishWriting();
									}
									// @v8.3.4 {
									if(State & stDebugMode) {
										Addr.ToStr(0, log_buf);
										log_buf.Space().Cat("SERVER REP").CatDiv(':', 2).Cat(reply.ToStr(temp_buf));
										PPLogMessage(debug_log_file_name, log_buf, LOGMSGF_TIME|LOGMSGF_THREADINFO);
									}
									// } @v8.3.4
								}
								break;
							case 1: // Бинарная команда
								{
									// @v8.3.4 {
									if(State & stDebugMode) {
										Addr.ToStr(0, log_buf);
										log_buf.Space().Cat("SERVER REQ").CatDiv(':', 2).Cat(cmd.ToStr(temp_buf));
										PPLogMessage(debug_log_file_name, log_buf, LOGMSGF_TIME|LOGMSGF_THREADINFO);
									}
									// } @v8.3.4
									cmdret = ProcessCommand(&cmd, reply);
									// @v8.3.4 {
									if(State & stDebugMode) {
										Addr.ToStr(0, log_buf);
										log_buf.Space().Cat("SERVER REP").CatDiv(':', 2).Cat(reply.ToStr(temp_buf));
										PPLogMessage(debug_log_file_name, log_buf, LOGMSGF_TIME|LOGMSGF_THREADINFO);
									}
									// } @v8.3.4
								}
								break;
							default:
								reply.SetError();
								reply.FinishWriting();
								break;
						}
						if(cmdret != cmdretResume && So.IsValid()) {
							if(!SendReply(reply)) {
								waiting_mode = wmodActiveSession_TransportError;
								PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_LASTERR|LOGMSGF_THREADINFO);
							}
						}
						if(cmdret == cmdretSuspend) {
							waiting_mode = wmodSuspended;
							if(1/*CConfig.Flags & CCFLG_DEBUG*/) {
								s.Printf(PPLoadTextS(PPTXT_LOG_SRVSESSSUSPENDED, fmt_buf), GetUniqueSessID(), (long)(SuspendTimeout / 1000));
								PPLogMessage(PPFILNAM_SERVER_LOG, s, LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_THREADINFO);
							}
						}
						else if(cmdret == cmdretStyloBhtIIMode) {
							SETIFZ(P_SbiiBlk, new StyloBhtIIExchanger);
							if(P_SbiiBlk) {
								waiting_mode = wmodStyloBhtII;
							}
							else {
								break;
							}
						}
						else if(oneof2(cmdret, cmdretQuit, cmdretResume))
							break;
					}
					else {
						waiting_mode = wmodActiveSession_TransportError;
						PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_LASTERR|LOGMSGF_THREADINFO);
					}
				}
				else if(nwev.lNetworkEvents & FD_CLOSE) {
					if(waiting_mode != wmodSuspended) {
						waiting_mode = wmodDisconnected;
						if(/*CConfig.Flags & CCFLG_DEBUG*/1) {
							s.Printf(PPLoadTextS(PPTXT_LOG_SRVSESSCONNCLOSED, fmt_buf), GetUniqueSessID(), (long)(CloseSocketTimeout / 1000));
							PPLogMessage(PPFILNAM_SERVER_LOG, s, LOGMSGF_TIME|LOGMSGF_THREADINFO);
						}
					}
					else {
						; // В suspended-режиме отключение сеанса - нормальное событие.
					}
				}
			}
			else {
				int    wsa_err = WSAGetLastError();
				SLS.SetError(SLERR_SOCK_WINSOCK, 0);
				PPSetErrorSLib();
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_LASTERR|LOGMSGF_THREADINFO);
			}
			ResetEvent(sock_event);
		}
		else if(r == wait_obj_substsock) {
			EvSubstSockStart.Reset(); // @v8.0.6 Если не сбросить событие, то возникнет бесконечный цикл
			EvSubstSockReady.Signal();
			EvSubstSockFinish.Wait();
			waiting_mode = wmodActiveSession_AfterReconnection; // @v8.0.8
			if(/*CConfig.Flags & CCFLG_DEBUG*/1) {
				s.Printf(PPLoadTextS(PPTXT_LOG_SRVSESSRECONNECT, fmt_buf), GetUniqueSessID());
				PPLogMessage(PPFILNAM_SERVER_LOG, s, LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_THREADINFO);
			}
		}
		else if(r == wait_obj_localstop) {
			s.Printf(PPLoadTextS(PPTXT_LOG_SRVSESSINTRBYLOCALSTOP, fmt_buf), GetUniqueSessID());
			PPLogMessage(PPFILNAM_SERVER_LOG, s, LOGMSGF_TIME|LOGMSGF_COMP);
			break;
		}
		else if(r == wait_obj_stop) {
			s.Printf(PPLoadTextS(PPTXT_LOG_SRVSESSINTRBYSTOPF, fmt_buf), GetUniqueSessID());
			PPLogMessage(PPFILNAM_SERVER_LOG, s, LOGMSGF_TIME|LOGMSGF_COMP);
			break;
		}
	}
	WSACloseEvent(sock_event);
}
//
//
//
int SLAPI CheckVersion()
{
	int    ok = 1;
	SVerT ppws_ver = DS.GetVersion();
	SVerT ppw_ver;
	{
		const char * p_ppw_fname = "ppw.exe";
		uint   info_size = 0;
		SString path;
		DWORD set_to_zero = 0;

		DS.GetPath(PPPATH_BIN, path);
		path.SetLastSlash().Cat(p_ppw_fname);
		info_size = GetFileVersionInfoSize(path, &set_to_zero); // @unicodeproblem
		if(info_size) {
			char * p_buf = new char[info_size];
			if(GetFileVersionInfo(path, 0, info_size, p_buf)) { // @unicodeproblem
				uint   value_size = 0;
				char * p_ver_buf = 0;
				if(VerQueryValue(p_buf, _T("\\"), (LPVOID *)&p_ver_buf, &value_size)) { // @unicodeproblem
					VS_FIXEDFILEINFO * p_file_info = (VS_FIXEDFILEINFO *)p_ver_buf;
					ppw_ver.Set(HIWORD(p_file_info->dwProductVersionMS), LOWORD(p_file_info->dwProductVersionMS), HIWORD(p_file_info->dwProductVersionLS));
				}
			}
			ZDELETE(p_buf);
		}
	}
	if(ppws_ver.Cmp(&ppw_ver) != 0) {
		PPLogMessage(PPFILNAM_SERVER_LOG, PPSTR_TEXT, PPTXT_PPYSERVERSTOPPED, LOGMSGF_TIME);
		ok = (PPError(PPERR_INVPPWSVER), 0);
	}
	return ok;
}
//
//
//
#define DEFAULT_SERVER_PORT 28015

int SLAPI RunNginxServer(); // @prototype(ppngx.cpp)

int SLAPI run_server()
{
	DS.SetExtFlag(ECF_SYSSERVICE, 1);
	DS.SetExtFlag(ECF_FULLGOODSCACHE, 1);
	if(CheckVersion()) {
		PPIniFile ini_file;
		{
			PPJobServer * p_job_srv = new PPJobServer;
			p_job_srv->Start();
		}
#if defined(_PPSERVER) // {
		{
			int    ival = 0;
			if(ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_NGINX, &ival) > 0 && ival == 1) {
				class NginxServer : public PPThread {
				public:
					NginxServer() : PPThread(PPThread::kNginxServer, "nginx", 0)
					{
					}
					virtual void Run()
					{
						RunNginxServer();
					}
				};
				NginxServer * p_ngx_srv = new NginxServer;
				p_ngx_srv->Start();
			}
		}
#endif // } _PPSERVER
		{
			class PPServer : public TcpServer {
			public:
				SLAPI  PPServer(const InetAddr & rAddr, int clientTimeout, const PPServerSession::InitBlock & rSib) :
					TcpServer(rAddr), ClientTimeout(clientTimeout), Sib(rSib)
				{
				}
				virtual int SLAPI ExecSession(::TcpSocket & rSock, InetAddr & rAddr)
				{
					if(ClientTimeout > 0)
						rSock.SetTimeout(ClientTimeout);
					PPServerSession * p_sess = new PPServerSession(rSock, Sib, rAddr);
					p_sess->Start(0/* @v8.5.10 1-->0 */);
					//
					// Объект p_sess будет удален функцией p_sess->Start
					//
					return 1;
				}
				const int ClientTimeout;
				const PPServerSession::InitBlock Sib;
			};
			PPServerSession::InitBlock sib;
			InetAddr addr;
			int    port = 0;
			int    client_timeout = -1;
			sib.ClosedSockTimeout = 60000;
			sib.SuspTimeout = 3600000;
			sib.SleepTimeout = INFINITE;
			sib.TxtCmdTerminalCode = -1;
			{
				int    ival = 0;
				if(ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_SERVER_PORT, &port) <= 0 || port == 0)
					port = DEFAULT_SERVER_PORT;
				if(ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_SERVER_SOCKETTIMEOUT, &client_timeout) <= 0 || client_timeout <= 0)
					client_timeout = -1;
				if(ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_SERVER_CLOSEDSOCKTIMEOUT, &(ival = 0)) > 0) {
					if(ival >= 0)
						sib.ClosedSockTimeout = (uint)ival;
				}
				if(ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_SERVER_SUSPSOCKTIMEOUT, &(ival = 0)) > 0) {
					if(ival >= 0)
						sib.SuspTimeout = (uint)ival;
				}
				if(ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_SERVER_SLEEPSOCKTIMEOUT, &(ival = 0)) > 0) {
					if(ival >= 0)
						sib.SleepTimeout = (uint)ival;
					else
						sib.SleepTimeout = INFINITE;
				}
				// @v8.1.1 {
				if(ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_SERVER_TXTCMDTERM, &(ival = 0)) > 0)
					sib.TxtCmdTerminalCode = ival;
				// } @v8.1.1
				// @v8.3.4 {
				if(ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_DEBUG, &(ival = 0)) > 0)
					sib.Flags |= sib.fDebugMode;
				// } @v8.3.4
			}
			addr.Set((ulong)0, port);
			PPServer server(addr, client_timeout, sib);
			if(server.Run())
				PPLogMessage(PPFILNAM_SERVER_LOG, PPSTR_TEXT, PPTXT_PPYSERVERSTOPPED, LOGMSGF_TIME);
		}
	}
	return 1;
}
//
//
//
SLAPI PPJobSrvClient::PPJobSrvClient() : So(120000), SyncTimer(120000) // @v7.8.10 So(600000)-->So(60000) // @v8.7.4 60000-->120000
{
	State = 0;
	SessId = 0;
	InformerCallbackProc = 0;
	P_InformerCallbackParam = 0;
	InitPort = 0;
}

SLAPI PPJobSrvClient::~PPJobSrvClient()
{
	Logout();
	Disconnect();
}

int FASTCALL PPJobSrvClient::Sync(int force)
{
	int    ok = -1;
	if((force || SyncTimer.Check(0)) && !(State & stLockExec)) {
		PPJobSrvReply reply;
		if(Exec("HELLO", reply) && reply.StartReading(0) && reply.CheckRepError())
			ok = 1;
		else
			ok = 0;
	}
	return ok;
}

int SLAPI PPJobSrvClient::SetInformerProc(int (*proc)(const char * pMsg, void * pParam), void * pParam)
{
	InformerCallbackProc = proc;
	P_InformerCallbackParam = pParam;
	return 1;
}

int SLAPI PPJobSrvClient::Connect(const char * pAddr, int port)
{
	int    ok = 1;
	if(!(State & stConnected)) {
		int    timeout = -1;
		InetAddr addr;
		SString addr_buf;
		if(pAddr) {
			const char * p = strchr(pAddr, ':');
			if(p) {
				addr_buf.CatN(pAddr, (size_t)(p - pAddr));
				port = atoi(p+1);
			}
		}
		{
			PPIniFile ini_file;
			if(pAddr == 0 || port <= 0) {
				if(port <= 0 && (ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_SERVER_PORT, &port) <= 0 || port <= 0))
					port = DEFAULT_SERVER_PORT;
				if(ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_CLIENTSOCKETTIMEOUT, &timeout) <= 0 || timeout <= 0) // @v8.7.4 PPINIPARAM_SERVER_SOCKETTIMEOUT-->PPINIPARAM_CLIENTSOCKETTIMEOUT
					timeout = -1;
				if(!pAddr) {
					if(ini_file.Get(PPINISECT_SERVER, PPINIPARAM_SERVER_NAME, addr_buf) <= 0)
						addr_buf = "localhost";
				}
				else
					addr_buf = pAddr;
			}
			// @v8.3.4 {
			{
				int   iv = 0;
				if(ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_DEBUG, &iv) > 0)
					State |= stDebugMode;
				else
					State &= ~stDebugMode;
			}
			PPGetFilePath(PPPATH_LOG, PPFILNAM_SERVERDEBUG_LOG, DebugLogFileName);
			// } @v8.3.4
		}
		InitAddr = addr_buf;
		InitPort = port;
		addr.Set(addr_buf, port);
		// @v7.8.12 {
		if(timeout > 0)
			So.SetTimeout(timeout);
		// } @v7.8.12
		THROW_SL(So.Connect(addr));
		State |= stConnected;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPJobSrvClient::Disconnect()
{
	int    ok = 1;
	if(State & stConnected) {
		So.Disconnect();
		State &= ~stConnected;
		State &= ~stDebugMode; // @v8.3.4
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPJobSrvClient::Reconnect(const char * pAddr, int port)
{
	int    ok = -1;
	Disconnect();
	if(pAddr == 0 && InitAddr.NotEmpty())
		pAddr = InitAddr;
	if(port <= 0 && InitPort > 0)
		port = InitPort;
	if(Connect(pAddr, port)) {
		if(AuthCookie.NotEmpty()) {
			PPJobSrvReply reply;
			SString temp_buf;
			(temp_buf = "RESUME").Space().Cat(AuthCookie);
			if(Exec(temp_buf, reply) && reply.StartReading(&temp_buf.Z()) && reply.CheckRepError()) {
				if(reply.GetH().Flags & PPJobSrvReply::hfAck)
					ok = 2;
			}
		}
		if(ok < 0) {
			ok = 1;
		}
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPJobSrvClient::TestBreakConnection()
{
	So.Disconnect();
	return 1;
}

int SLAPI PPJobSrvClient::Exec(PPJobSrvCmd & rCmd, const char * pTerminal, PPJobSrvReply & rReply)
{
	int    ok = 1;
	int    do_log_error = 0;
	const  int preserve_so_timeout = So.GetTimeout(); // @v9.1.8
	SString log_buf, temp_buf;
	ExecLock.Lock(); // @v8.3.4 ENTER_CRITICAL_SECTION-->ExecLock.Lock()
	State |= stLockExec;
	rReply.Clear();
	THROW_PP(State & stConnected, PPERR_JOBSRVCLI_NOTCONN);
	// @v8.3.4 {
	if(State & stDebugMode) {
		log_buf.Z().Cat("CLIENT REQ").CatDiv(':', 2);
		log_buf.Cat(rCmd.ToStr(temp_buf));
		PPLogMessage(DebugLogFileName, log_buf, LOGMSGF_TIME|LOGMSGF_THREADINFO);
	}
	// } @v8.3.4
	THROW_SL(So.SendBuf(rCmd, 0));
	So.SetTimeout(1000 * 3600 * 24); // @v9.1.8 Временно устанавливаем очень большой таймаут так как процесс может выполнятся сервером очень долго
	do {
		size_t actual_size = 0;
		do_log_error = 1;
		const int rr = rReply.Helper_Recv(So, pTerminal, &actual_size);
		THROW(rr);
		if(rReply.GetH().Flags & PPJobSrvReply::hfInformer) {
			TempBuf = 0;
			if(rReply.GetH().Type == PPJobSrvReply::htGenericText) {
				PPJobSrvReply::Header temp_hdr;
				rReply.Read(&temp_hdr, sizeof(temp_hdr));
				TempBuf.Cat(rReply);
			}
			if(InformerCallbackProc) {
				InformerCallbackProc(TempBuf, P_InformerCallbackParam);
			}
			//
			// Для информеров в журнал отладки ничего не выводим (слишком большой поток мало значащих сообщений)
			//
		}
		else if(State & stDebugMode) {
			log_buf.Z().Cat("CLIENT REP").CatDiv(':', 2);
			log_buf.Cat(rReply.ToStr(temp_buf));
			PPLogMessage(DebugLogFileName, log_buf, LOGMSGF_TIME|LOGMSGF_THREADINFO);
		}
	} while(rReply.GetH().Flags & PPJobSrvReply::hfInformer);
	CATCH
		if(do_log_error)
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		ok = 0;
	ENDCATCH
	So.SetTimeout(preserve_so_timeout); // @v9.1.8
	State &= ~stLockExec;
	ExecLock.Unlock(); // @v8.3.4 LEAVE_CRITICAL_SECTION-->ExecLock.Unlock()
	return ok;
}

int SLAPI PPJobSrvClient::Exec(PPJobSrvCmd & rCmd, PPJobSrvReply & rReply)
{
	return Exec(rCmd, 0, rReply);
}

int SLAPI PPJobSrvClient::Exec(const char * pCmd, const char * pTerminal, PPJobSrvReply & rReply)
{
	PPJobSrvCmd cmd;
	cmd.StartWriting(pCmd);
	cmd.FinishWriting();
	return Exec(cmd, pTerminal, rReply);
}

int SLAPI PPJobSrvClient::Exec(const char * pCmd, PPJobSrvReply & rReply)
{
	PPJobSrvCmd cmd;
	cmd.StartWriting(pCmd);
	cmd.FinishWriting();
	return Exec(cmd, 0, rReply);
}

int SLAPI PPJobSrvClient::GetLastErr(SString & rBuf)
{
	int    ok = 1;
	PPJobSrvReply reply;
	THROW(Exec("GETLASTERR", reply));
	THROW(reply.StartReading(&rBuf));
	CATCHZOK
	rBuf.Chomp();
	return ok;
}

int SLAPI PPJobSrvClient::Login(const char * pDbSymb, const char * pUserName, const char * pPassword)
{
	int    ok = 1;
	SString cmd, reply_str;
	PPJobSrvReply reply;
	if(State & stLoggedIn) {
		THROW(Logout());
	}
	cmd.Cat("LOGIN").Space().Cat(pDbSymb).Space().Cat(pUserName).Space().Cat(pPassword);
	THROW(Exec(cmd, reply));
	THROW(reply.StartReading(0));
	THROW(reply.CheckRepError());
	{
		THROW(Exec("HSH", reply));
		THROW(reply.StartReading(&reply_str));
		THROW(reply.CheckRepError());
		AuthCookie = reply_str.Chomp();
	}
	State |= stLoggedIn;
	CATCHZOK
	return ok;
}

int SLAPI PPJobSrvClient::Logout()
{
	int    ok = 1;
	if(State & stLoggedIn) {
		SString reply_str;
		PPJobSrvReply reply;
		THROW(Exec("LOGOUT", reply));
		THROW(reply.StartReading(0));
		if(reply.GetH().Flags & PPJobSrvReply::hfRepError) {
			GetLastErr(reply_str);
			PPSetError(PPERR_JOBSRVCLI_ERR, reply_str);
			CALLEXCEPT();
		}
		AuthCookie = 0;
		State &= ~stLoggedIn;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
//
//
//#ifndef NDEBUG // {

static int informer_proc(const char * pMsg, void * pParam)
{
	if(pMsg)
		printf("%s\n", pMsg);
	return 1;
}

int SLAPI run_client()
{
	int    err = 0;
	SString reply_str, msg_buf;
	PPJobSrvClient cli;
	cli.SetInformerProc(informer_proc, 0);
	if(cli.Connect(0, 0)) {
		PPJobSrvReply reply;
		while(1) {
			char cmd[1024];
			memzero(cmd, sizeof(cmd));
#if (_MSC_VER >= 1900)
			gets_s(cmd, sizeof(cmd));
#else
			gets(cmd);
#endif
			if(*strip(cmd)) {
				if(sstreqi_ascii(cmd, "exit"))
					break;
				// @v7.3.2 @debug {
				else if(sstreqi_ascii(cmd, "pingtest")) {
					if(cli.Exec("ping term 0A", "0A", reply)) {
						int r = reply.StartReading(&reply_str);
						if(r == 2) {
							reply_str.ToOem();
						}
						else if(r == 1) {
							const PPJobSrvProtocol::Header & hdr = reply.GetH();
							reply_str.Z().Cat("Binary reply").CatDiv(':', 2).
								CatEq("Protocol Ver", (long)hdr.ProtocolVer).CatDiv(';', 2).
								CatEq("Data Len", hdr.DataLen).CatDiv(';', 2).
								CatEq("Data Type", hdr.Type).CatDiv(';', 2).
								CatEq("Flags", hdr.Flags).CRB();
							if(hdr.Type == PPJobSrvReply::htGenericText) {
								reply_str.Cat(reply).ToOem().CRB();
							}
						}
						else {
							PPGetLastErrorMessage(1, msg_buf);
							reply_str.Z().Cat("Error").CatDiv(':', 2).Cat(msg_buf);
						}
						printf(reply_str.cptr());
					}
				}
				// } @v7.3.2 @debug
				else if(sstreqi_ascii(cmd, "connect")) {
					if(cli.GetState() & PPJobSrvClient::stConnected) {
						printf("Allready connected\n");
					}
					else if(cli.Connect(0, 0)) {
						printf("Connection OK\n");
					}
					else
						printf("Connection failed\n");
				}
				else if(cli.GetState() & PPJobSrvClient::stConnected) {
					int is_quit = (strnicmp(cmd, "quit", 4) == 0) ? 1 : 0;
					if(cli.Exec(cmd, reply)) {
						int r = reply.StartReading(&reply_str);
						if(r == 2) {
							reply_str.ToOem();
						}
						else if(r == 1) {
							const PPJobSrvProtocol::Header & hdr = reply.GetH();
							reply_str.Z().Cat("Binary reply").CatDiv(':', 2).
								CatEq("Protocol Ver", (long)hdr.ProtocolVer).CatDiv(';', 2).
								CatEq("Data Len", hdr.DataLen).CatDiv(';', 2).
								CatEq("Data Type", hdr.Type).CatDiv(';', 2).
								CatEq("Flags", hdr.Flags).CRB();
							if(hdr.Type == PPJobSrvReply::htGenericText) {
								reply_str.Cat(reply).ToOem().CRB();
							}
						}
						else {
							PPGetLastErrorMessage(1, msg_buf);
							reply_str.Z().Cat("Error").CatDiv(':', 2).Cat(msg_buf);
						}
						printf(reply_str.cptr());
					}
					if(is_quit)
						cli.Disconnect();
				}
			}
		}
	}
	else {
		PPGetLastErrorMessage(1, msg_buf);
		reply_str.Z().Cat("Error").CatDiv(':', 2).Cat(msg_buf).CR();
		printf(reply_str.cptr());
		err = -1;
	}
	return err;
}

//#endif // } !NDEBUG
//
//
//
#define SERVICE_NAME      "PAPYRUS_SERVICE"
#define SERVER_STOP_WAIT  4000
#define SERVER_START_WAIT 8000

int SLAPI run_service()
{
	static SERVICE_STATUS ServiceStatus;
	static SERVICE_STATUS_HANDLE ServiceStatusHandle = NULL;

	class ServiceProcWrapper {
	private:
		static BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
		{
			static DWORD check_point = 1;
			BOOL   result = TRUE;
			ServiceStatus.dwControlsAccepted = (dwCurrentState == SERVICE_START_PENDING) ? 0 : (SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN);
			ServiceStatus.dwCurrentState = dwCurrentState;
			ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
			ServiceStatus.dwWaitHint = dwWaitHint;
			ServiceStatus.dwCheckPoint = oneof2(dwCurrentState, SERVICE_RUNNING, SERVICE_STOPPED) ? 0 : check_point++;
			result = SetServiceStatus(ServiceStatusHandle, &ServiceStatus);
			if(!result)
				SLS.LogMessage(0, "SetServiceStatus error");
			return result;
		}
		static VOID WINAPI ServiceCtrl(DWORD dwCtrlCode)
		{
			switch(dwCtrlCode) {
				case SERVICE_CONTROL_SHUTDOWN:
				case SERVICE_CONTROL_STOP:
					ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, SERVER_STOP_WAIT);
					SLS.Stop();
					Sleep(SERVER_STOP_WAIT);
					ReportStatusToSCMgr(SERVICE_STOPPED, 0, 0);
					break;
				default:
					ReportStatusToSCMgr(ServiceStatus.dwCurrentState, NO_ERROR, 0);
			}
		}
	public:
		static void ServiceMain(DWORD argc, LPTSTR * argv)
		{
			ServiceStatusHandle = RegisterServiceCtrlHandler(_T(SERVICE_NAME), ServiceProcWrapper::ServiceCtrl);
			if(ServiceStatusHandle) {
				MEMSZERO(ServiceStatus);
				ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
				ServiceStatus.dwServiceSpecificExitCode = 0;
				if(ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, SERVER_START_WAIT)) {
					ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 0);

					SLS.InitThread();
					DBS.InitThread();
					DS.InitThread(0);
					DS.SetExtFlag(ECF_SYSSERVICE, 1);
					DS.SetExtFlag(ECF_FULLGOODSCACHE, 1);

					run_server();

					DS.ReleaseThread();
					DBS.ReleaseThread();
					SLS.ReleaseThread();
				}
				ReportStatusToSCMgr(SERVICE_STOPPED, NO_ERROR, 0);
			}
		}
	};
	SERVICE_TABLE_ENTRY dispatch_table[] = {
		{ _T(SERVICE_NAME), (LPSERVICE_MAIN_FUNCTION)ServiceProcWrapper::ServiceMain }, { NULL, NULL }
	};
	return StartServiceCtrlDispatcher(dispatch_table) ? 1 : 0; // @unicodeproblem
}

int SLAPI RFIDPrcssr()
{
#if 0 // {
	SString file_name;
	GetComDvcSymb(comdvcsCom, 6, 1, file_name);
	/*
	if(sio_open(6)!=SIO_OK) {
	}
	*/
	HANDLE h_port = ::CreateFile(file_name, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
	if(h_port != INVALID_HANDLE_VALUE) {
		int r = 0;
		size_t sended = 0;
		long crc = 0;
		char crc_buf[16];
		SString query_str;

		//crc = 1^1;
		//memzero(crc_buf, sizeof(crc_buf));
		//ltoa(crc, crc_buf, 16);
		// query_str.Cat(crc_buf);
		{
			DCB    dcb;
			r = GetCommState(h_port, &dcb);
			dcb.BaudRate = CBR_57600;
			dcb.Parity   = NOPARITY;
			dcb.ByteSize = 8;
			dcb.StopBits = 2;//ONESTOPBIT;
			r = BIN(SetCommState(h_port, &dcb));

			COMMTIMEOUTS cto;
			cto.ReadIntervalTimeout = MAXDWORD;
			cto.ReadTotalTimeoutMultiplier = MAXDWORD;
			cto.ReadTotalTimeoutConstant = 5000;
			cto.WriteTotalTimeoutConstant = 0;
			cto.WriteTotalTimeoutMultiplier = 0;
			r = BIN(SetCommTimeouts(h_port, &cto));
		}
		while(1) {
			char data[32];
			DWORD sz = 0;
			// query_str.Z().CatChar((char)170).CatChar((char)1).CatChar((char)1).CatChar((char)1^1);
			query_str.Z().CatChar((char)170).CatChar((char)1).CatChar((char)6);
			STRNSCPY(data, "1234567890123456789012345678901");
			crc = 1^6;
			for(uint i = 0; i < strlen(data); i++)
				crc = crc ^ data[i];
			query_str.CatChar((char)crc);
			memzero(data, sizeof(data));
			sz = query_str.Len();
			WriteFile(h_port, query_str, sz, &sz, 0);

			char ret_buf[256];
			memzero(ret_buf, sizeof(ret_buf));
			// ReadFile(h_port, ret_buf, 1, &sz, 0);
			if(sz != 0) {
				/*
				if(WriteFile(h_port, (const char*)query_str, sz, &sz, 0) > 0)
					r = ReadFile(h_port, ret_buf, 1, &(sz = sizeof(ret_buf)), 0);
				*/
				int a;
				a = 10;
			}
		}
		/*
		DCB    dcb;
		GetCommState(H_Port, &dcb);
		dcb.BaudRate = CBR_9600;
		dcb.fParity  = FALSE;
		dcb.Parity   = 0;
		dcb.ByteSize = 8;
		dcb.StopBits = 0;
		ok = BIN(SetCommState(H_Port, &dcb));

		COMMTIMEOUTS cto;
		cto.ReadIntervalTimeout = MAXDWORD;
		cto.ReadTotalTimeoutMultiplier = MAXDWORD;
		cto.ReadTotalTimeoutConstant = Data.Get_Delay;
		cto.WriteTotalTimeoutConstant = 0;
		cto.WriteTotalTimeoutMultiplier = 0;
		ok = BIN(SetCommTimeouts(H_Port, &cto));
		ok = 1;                                  // ???
		*/
	}
	CloseHandle(h_port);
	/*
	WSADATA wsa_data;
	if(WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0) {
		while(1) {
			InetAddr  addr;
			TcpSocket sock;

			addr.Set("192.168.0.250");
			if(sock.Connect(addr) > 0) {
				size_t sended = 0;
				long crc = 0;
				char crc_buf[16];
				SString query_str;

				query_str = "AAh,1h,01h,";
				crc = 1^1;
				memzero(crc_buf, sizeof(crc_buf));
				ltoa(crc, crc_buf, 16);
				query_str.Cat(crc_buf);
				if(sock.Send((const char*)query_str, query_str.Len(), &sended) > 0) {
					char recv_buf[256];
					size_t recv = 0;
					memzero(recv_buf, sizeof(recv_buf));
					sock.Recv(recv_buf, sizeof(recv_buf), &recv);
					if(query_str.Cmp(recv_buf, 0) != 0) {
						char data_buf[32];
						memzero(data_buf, sizeof(data_buf));
						query_str = "AAh,1h,06h,";
						STRNSCPY(data_buf, "This is a test");
						crc = 1^6;
						for(uint i = 0; i < sizeof(data_buf); i++) {
							query_str.CatChar(data_buf[i]).CatChar(',');
							crc = crc ^ data_buf[i];
						}
						memzero(crc_buf, sizeof(crc_buf));
						ltoa(crc, crc_buf, 16);
						query_str.Cat(crc_buf);
						sock.Send((const char*)query_str, query_str.Len(), &sended);
					}
				}
				sock.Disconnect();
			}
		}
	}
	WSACleanup();
	*/
#endif // } 0
	return 1;
}
//
//
int SLAPI install_service(int inst, const char * pLoginName, const char * pLoginPassword)
{
	int    ok = 0;
	if(inst) {
		SString descr, cmd_line;
		PPLoadText(PPTXT_SERVICEDESCR, descr);
		cmd_line = SLS.GetExePath();
		cmd_line.Space().Cat("service");
		ok = WinService::Install(SERVICE_NAME, descr, cmd_line, pLoginName, pLoginPassword);
	}
	else {
		WinService::Uninstall("ppws"); // Удаление службы со старым (и неверным) именем
		ok = WinService::Uninstall(SERVICE_NAME);
	}
	return ok;
}

int SLAPI start_service(int start)
{
	return start ? WinService::Start(SERVICE_NAME, 0) : WinService::Start(SERVICE_NAME, 1);
}
//
//
//
int SLAPI TestReconnect()
{
	int    ok = 1;
	PPJobSrvClient * p_cli = DS.GetClientSession(0);
	if(p_cli) {
		p_cli->TestBreakConnection();
		SDelay(1000);
		p_cli = DS.GetClientSession(0);
	}
	return ok;
}
//
//
//
#if SLTEST_RUNNING // {

SLTEST_R(PapyrusTcpClient)
{
	int    ok = 1;
	int    is_connected = 0;
	InetAddr addr;
	SString srv_addr_line, str_file_name;
	int    port = 0;
	int    timeout = 0;
	uint   arg_no = 0;
	SString arg, temp_buf;
	StrAssocArray str_list;
	/*
	;
	; Аргументы:
	;   0 - имя удаленного компьютера с установленным JobServer'ом
	;   1 - порт
	;   2 - Таймаут сокета (в миллисекундах)
	;   3 - Путь к файлу, содержащему набор строк
	;
	*/
	if(EnumArg(&arg_no, arg)) {
		srv_addr_line = arg;
		if(EnumArg(&arg_no, arg)) {
			port = arg.ToLong();
			if(EnumArg(&arg_no, arg)) {
				timeout = arg.ToLong();
				if(EnumArg(&arg_no, arg)) {
					str_file_name = arg;
				}
			}
		}
	}
	if(srv_addr_line.Empty())
		srv_addr_line = "localhost";
	if(port <= 0)
		port = DEFAULT_SERVER_PORT;
	if(timeout < 0 || timeout > 100000)
		timeout = 30000;
	if(fileExists(str_file_name)) {
		SFile f(str_file_name, SFile::mRead);
		long i = 0;
		while(f.ReadLine(temp_buf) && i < 32000) {
			str_list.Add(++i, temp_buf.Chomp());
		}
	}
	else {
		str_list.Add(1, "Test 01");
		str_list.Add(2, "Test 02");
		str_list.Add(3, "Test 03");
		str_list.Add(4, "");
	}
	if(addr.Set(srv_addr_line, port)) {
		TcpSocket so(timeout);
		if(so.Connect(addr)) {
			ok = PPServerSession::TestingClient(so, str_list);
		}
		else {
			addr.ToStr(InetAddr::fmtHost | InetAddr::fmtPort, srv_addr_line);
			SetInfo(temp_buf.Printf("Unable connect to %s", srv_addr_line.cptr()), 0);
		}
	}
	else
		SetInfo(temp_buf.Printf("Error setting address %s:%d\n", srv_addr_line.cptr(), port), 0);
	return ok;
}

#if 0 // Реализовать для UhttSoap конфигурацию ServerDebug
// @prototype
int TestUhttSoapClient(const char * pAddress, const char * pLoginName, const char * pPassword);

SLTEST_R(UhttSoap)
{
	TestUhttSoapClient("http://192.168.0.21/Universe-HTT/soap/univere_htt.php", "master", "123");
	return 1;
}
#endif

SLTEST_R(PapyrusRestoreSess)
{
	int    ok = 1;
	int    is_connected = 0;
	InetAddr addr;
	int    port = 0;
	int    timeout = 0;
	uint   arg_no = 0;
	const uint sess_count = 300;
	const size_t recv_size = 1024;
	char   recv_buf[recv_size];
	SString arg, temp_buf, db_symb, user, pwd, srv_addr_line;
	LongArray sess_list;
	/*
	;
	; Аргументы:
	;   0 - имя удаленного компьютера с установленным JobServer'ом
	;   1 - порт
	;   2 - символ базы данных
	;   3 - имя пользователя //
	;   4 - пароль
	*/
	if(EnumArg(&arg_no, arg)) {
		srv_addr_line = arg;
		if(EnumArg(&arg_no, arg)) {
			port = arg.ToLong();
			if(EnumArg(&arg_no, arg)) {
				db_symb = arg;
				if(EnumArg(&arg_no, arg)) {
					user = arg;
					if(EnumArg(&arg_no, arg))
						pwd = arg;
				}
			}
		}
	}
	if(srv_addr_line.Empty())
		srv_addr_line = "localhost";
	if(port <= 0)
		port = DEFAULT_SERVER_PORT;
	timeout = 30000;
	if(addr.Set(srv_addr_line, port)) {
		TcpSocket so(timeout);
		for(uint i = 0; ok && i < sess_count; i++) {
			if(so.Connect(addr)) {
				SString buf;
				(temp_buf = "LOGIN").Space().Cat(db_symb).Space().Cat(user).Space().Cat(pwd);
				if(so.Send(temp_buf, temp_buf.Len(), 0) > 0) {
					memzero(recv_buf, sizeof(recv_buf));
					if(so.Recv(recv_buf, recv_size, 0) > 0 && atol(recv_buf) > 0) {
						memzero(recv_buf, sizeof(recv_buf));
						if(so.Send(temp_buf = "HSH", temp_buf.Len(), 0) > 0 && so.Recv(recv_buf, recv_size, 0) > 0) {
							int32 sess_id = atol(recv_buf);
							if(sess_id)
								sess_list.add(sess_id);
						}
						else {
							SetInfo(temp_buf.Printf("Can't handshake with server\n"), 0);
							ok = 0;
						}
					}
					else {
						SetInfo(temp_buf.Printf("Invalid user name or password db_symb=%s\n", db_symb.cptr()), 0);
						ok = 0;
					}
				}
			}
			else {
				addr.ToStr(InetAddr::fmtHost | InetAddr::fmtPort, srv_addr_line);
				SetInfo(temp_buf.Printf("Unable connect to %s\n", srv_addr_line.cptr()), 0);
				ok = 0;
			}
			so.BreakSocket();
		}
	}
	else {
		SetInfo(temp_buf.Printf("Error setting address %s:%d\n", srv_addr_line.cptr(), port), 0);
		ok = 0;
	}
	if(ok > 0 && sess_list.getCount()) {
		SRandGenerator srand;
		TcpSocket so(timeout);
		for(uint i = 0; i < 100; i++) {
			int32 sess_id = sess_list.at(SLS.GetTLA().Rg.GetUniformInt(sess_count));
			SetInfo(0, 1);
			so.BreakSocket();
			if(so.Connect(addr)) {
				(temp_buf = "RESTORESESS").Space().Cat(sess_id);
				memzero(recv_buf, sizeof(recv_buf));
				if(so.Send(temp_buf, temp_buf.Len(), 0) <= 0 || so.Recv(recv_buf, recv_size, 0) <= 0 || strnicmp(recv_buf, "ACK", 3) != 0) {
					SetInfo(0, 0);
					SetInfo(temp_buf.Printf("Can't restore session %ld\n", sess_id), 1);
				}
			}
			else {
				SetInfo(0, 0);
				addr.ToStr(InetAddr::fmtHost|InetAddr::fmtPort, srv_addr_line);
				SetInfo(temp_buf.Printf("Unable connect to %s\n", srv_addr_line.cptr()), 0);
			}
		}
	}
	return ok;
}

/*
SLTEST_R(PapyrusTransmitFile)
{
	int    ok = 1;
	return ok;
}
*/

#endif // } SLTEST_RUNNING
