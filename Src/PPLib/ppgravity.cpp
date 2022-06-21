// PPGRAVITY.CPP
// Copyright (c) A.Sobolev 2020
// @codepage UTF-8
// @experimental
// Модуль интерфейса с языком gravity
//
#include <pp.h>
#pragma hdrstop
#if _MSC_VER >= 1600 // {
#include <..\osf\gravity\include\gravity_.h>

#if 1 // {

struct PPGravityUnitTestData {
	PPGravityUnitTestData();
	PPGravityUnitTestData & Z();

	bool   Processed;
	bool   IsFuzzy;
	uint8  Reserve[2]; // @alignment
	uint32 NCount;
	uint32 NSuccess;
	uint32 NFailure;
	GravityErrorType ExpectedErr;
	GravityValue ExpectedValue;
	int32 ExpectedRow;
	int32 ExpectedCol;
};

class GravityTestCls01 : public GravityClassImplementation {
	int    IVal;
	double RVal;
	SString StrVal;
	//
	// Interface ITest implementation
	//
	static bool __cdecl _Callee_Get_ValInt(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex)
	{

	}
	static bool __cdecl _Callee_Set_ValInt(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex);
	static bool __cdecl _Callee_Get_ValReal(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex);
	static bool __cdecl _Callee_Set_ValReal(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex);
	static bool __cdecl _Callee_Get_ValString(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex);
	static bool __cdecl _Callee_Set_ValString(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex);
	static bool __cdecl _Callee_CalcSomething(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex);
	//
	// Interface ITest implementation
	//
	int32 Get_ValInt();
	int32 Set_ValInt(int32 val);
	double Get_ValReal();
	int32 Set_ValReal(double val);
	SString & Get_ValString();
	int32 Set_ValString(SString & val_);
	double CalcSomething(double a, double b, double c);
public:
	GravityTestCls01() : GravityClassImplementation("TestCls01", 0), IVal(0), RVal(0.0)
	{
	}
};

PPGravityUnitTestData::PPGravityUnitTestData() : Processed(false), IsFuzzy(false), NCount(0), NSuccess(0), NFailure(0), ExpectedErr(GRAVITY_ERROR_NONE),
	ExpectedRow(0), ExpectedCol(0)
{
}

PPGravityUnitTestData & PPGravityUnitTestData::Z()
{
	Processed = false;
	IsFuzzy = false;
	NCount = 0;
	NSuccess = 0;
	NFailure = 0;
	ExpectedErr = GRAVITY_ERROR_NONE;
	ExpectedValue.from_null();
	ExpectedRow = 0;
	ExpectedCol = 0;
	return *this;
}

PPGravityModule::PPGravityModule() : P_OuterLogger(0), P_Utd(0)
{
}

PPGravityModule::~PPGravityModule()
{
	if(P_Utd) {
		delete static_cast<PPGravityUnitTestData *>(P_Utd);
	}
}

//typedef void (* gravity_log_callback)(gravity_vm * vm, const char * message, void * xdata);

/*static*/void PPGravityModule::CbLog(void * pVm, const char * pMsg, void * pExtra)
{
	PPGravityModule * p_this = static_cast<PPGravityModule *>(pExtra);
	SString msg_buf;
	if(!isempty(pMsg))
		msg_buf.CatDiv(':', 2).Cat(pMsg);
	if(p_this->P_OuterLogger)
		p_this->P_OuterLogger->Log(msg_buf);
	PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO);
}

/*static*/void PPGravityModule::CbLogClear(void * pVm, void * pExtra)
{
	PPGravityModule * p_this = static_cast<PPGravityModule *>(pExtra);
	if(p_this->P_OuterLogger)
		p_this->P_OuterLogger->Clear();
}

/*static*/void PPGravityModule::CbError(void * pVm, int errorType, const char * pMsg, GravityErrorDescription * pErrorDesc, void * pExtra)
{
	PPGravityModule * p_this = static_cast<PPGravityModule *>(pExtra);
	SString msg_buf;
	const char * p_type = "N/A";
	switch(errorType) {
		case GRAVITY_ERROR_NONE: p_type = "NONE"; break;
		case GRAVITY_ERROR_SYNTAX: p_type = "SYNTAX"; break;
		case GRAVITY_ERROR_SEMANTIC: p_type = "SEMANTIC"; break;
		case GRAVITY_ERROR_RUNTIME: p_type = "RUNTIME"; break;
		case GRAVITY_WARNING: p_type = "WARNING"; break;
		case GRAVITY_ERROR_IO: p_type = "I/O"; break;
	}
	if(errorType == GRAVITY_ERROR_RUNTIME) 
		(msg_buf = "RUNTIME ERROR");
	else { 
		(msg_buf = p_type).Space().Cat("ERROR on").Space().Cat(pErrorDesc->fileid).Space().CatChar('(').Cat(pErrorDesc->lineno).Comma().Cat(pErrorDesc->colno).CatChar(')');
		//printf("%s ERROR on %d (%d,%d): ", p_type, pErrorDesc->fileid, pErrorDesc->lineno, pErrorDesc->colno);
	}
	if(!isempty(pMsg))
		msg_buf.CatDiv(':', 2).Cat(pMsg);
	if(p_this->P_OuterLogger)
		p_this->P_OuterLogger->Log(msg_buf);
	PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO);
}

int PPGravityModule::LoadFile(const char * pFileName, SBuffer & rBuffer)
{
	int    ok = 1;
	SString file_name;
	rBuffer.Z();
	SPathStruc ps(pFileName);
	THROW(ps.Nam.NotEmpty() && ps.Ext.NotEmpty());
	ps.Merge(file_name);
	THROW_SL(fileExists(file_name));
	{
		int64 fs = 0;
		SFile f(file_name, SFile::mRead);
		THROW_SL(f.IsValid());
		if(f.CalcSize(&fs) && fs > 0) {
			THROW(fs <= SMEGABYTE(8)); // @error file too big
			{
				STempBuffer temp_buffer(SMEGABYTE(1));
				size_t actual_size = 0;
				THROW_SL(temp_buffer.IsValid());
				do {
					THROW_SL(f.Read(temp_buffer, temp_buffer.GetSize(), &actual_size));
					THROW_SL(rBuffer.Write(temp_buffer, actual_size));
				} while(actual_size > 0);
			}
		}
	}
	CATCHZOK
	return ok;
}

/*static*/const char * PPGravityModule::CbLoadFile(const char * pFileName, size_t * pSize, uint32 * pFileId, void * pExtra, bool * pIsStatic)
{
	PPGravityModule * p_this = static_cast<PPGravityModule *>(pExtra);
	char * p_ret = 0;
	size_t size = 0;
	SBuffer buffer;
	if(p_this && p_this->LoadFile(pFileName, buffer)) {
		p_ret = static_cast<char *>(SAlloc::M(buffer.GetAvailableSize() + 64)); // 64 insurance
		if(p_ret) {
			size = buffer.GetAvailableSize();
			if(buffer.Read(p_ret, size) != size) {
				size = 0;
				ZFREE(p_ret);
			}
		}
	}
	ASSIGN_PTR(pSize, size);
	ASSIGN_PTR(pIsStatic, false);
	return p_ret; 
}

static void CbUnitTest(void * pVm, GravityErrorType errorType, const char * pDescr, const char * pNote, GravityValue value, int32 row, int32 col, void * pExtra)
{
	PPGravityModule * p_this = static_cast<PPGravityModule *>(pExtra);
	if(p_this) {
		if(!p_this->P_Utd)
			p_this->P_Utd = new PPGravityUnitTestData;
		static_cast<PPGravityUnitTestData *>(p_this->P_Utd)->ExpectedErr = errorType;
		static_cast<PPGravityUnitTestData *>(p_this->P_Utd)->ExpectedValue = value;
		static_cast<PPGravityUnitTestData *>(p_this->P_Utd)->ExpectedRow = row;
		static_cast<PPGravityUnitTestData *>(p_this->P_Utd)->ExpectedCol = col;
		SString msg_buf;
		if(!isempty(pNote)) {
			msg_buf.Tab().Cat("NOTE").CatDiv(':', 2).Cat(pNote);
			p_this->LogToOuterLogger(msg_buf);
			PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO);
		}
		msg_buf.Z().Tab().Cat(pDescr);
		p_this->LogToOuterLogger(msg_buf);
		PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO);
	}
}

void PPGravityModule::LogToOuterLogger(const char * pMsg) // @temporary
{
	CALLPTRMEMB(P_OuterLogger, Log(pMsg));
}

int PPGravityModule::RunBuffer(const char * pText, PPLogger * pOuterLogger, GravityValue * pResult)
{
	int    ok = 1;
	SString msg_buf;
	SBuffer buffer;
	P_OuterLogger = pOuterLogger;
	gravity_delegate_t delegate(this);
	delegate.log_callback = reinterpret_cast<gravity_log_callback>(PPGravityModule::CbLog);
	//typedef void (* gravity_log_clear)(gravity_vm * vm, void * xdata);
	delegate.log_clear = reinterpret_cast<gravity_log_clear>(PPGravityModule::CbLogClear);
	delegate.error_callback = reinterpret_cast<gravity_error_callback>(PPGravityModule::CbError);
	delegate.unittest_callback = reinterpret_cast<gravity_unittest_callback>(CbUnitTest);
	delegate.loadfile_callback = PPGravityModule::CbLoadFile/*unittest_read*/;
	gravity_vm * p_vm = 0;
	gravity_closure_t * p_closure = 0;
	gravity_compiler_t * p_compiler = gravity_compiler_create(&delegate);
	THROW(p_compiler);
	p_closure = gravity_compiler_run(p_compiler, pText, sstrlen(pText), 0/*fileId*/, true/*isStatic*/, false/*addDebug*/);
	THROW(p_closure);
	THROW(p_vm = gravity_vm_new(&delegate));
	gravity_compiler_transfer(p_compiler, p_vm);
	gravity_compiler_free(p_compiler);
	p_compiler = 0;
	THROW(gravity_vm_runmain(p_vm, p_closure));
	{
		GravityValue result = gravity_vm_result(p_vm);
		ASSIGN_PTR(pResult, result);
		if(P_Utd) {
			static_cast<PPGravityUnitTestData *>(P_Utd)->Processed = true;
			if(static_cast<PPGravityUnitTestData *>(P_Utd)->IsFuzzy || gravity_value_equals(result, static_cast<PPGravityUnitTestData *>(P_Utd)->ExpectedValue)) {
				static_cast<PPGravityUnitTestData *>(P_Utd)->NSuccess++;
				msg_buf.Cat("SUCCESS");
				LogToOuterLogger(msg_buf);
				PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
			}
			else {
				static_cast<PPGravityUnitTestData *>(P_Utd)->NFailure++;
				msg_buf.Cat("FAILURE");
				LogToOuterLogger(msg_buf);
				PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
			}
			gravity_value_free(0, static_cast<PPGravityUnitTestData *>(P_Utd)->ExpectedValue);
			// case for empty files or simple declarations test
			if(!static_cast<PPGravityUnitTestData *>(P_Utd)->Processed) {
				static_cast<PPGravityUnitTestData *>(P_Utd)->NSuccess++;
				msg_buf.Cat("SUCCESS");
				LogToOuterLogger(msg_buf);
				PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
			}
		}
	}
	CATCHZOK
	gravity_vm_free(p_vm);
	gravity_compiler_free(p_compiler);
	P_OuterLogger = 0;
	return ok;
}

int PPGravityModule::RunFile(const char * pFileName, PPLogger * pOuterLogger, GravityValue * pResult)
{
	int    ok = 1;
	SString msg_buf;
	SBuffer buffer;
	P_OuterLogger = pOuterLogger;
	gravity_delegate_t delegate(this);
	delegate.error_callback = reinterpret_cast<gravity_error_callback>(PPGravityModule::CbError)/*unittest_error*/;
	delegate.unittest_callback = reinterpret_cast<gravity_unittest_callback>(CbUnitTest);
	delegate.loadfile_callback = PPGravityModule::CbLoadFile/*unittest_read*/;
	gravity_vm * p_vm = 0;
	gravity_closure_t * p_closure = 0;
	gravity_compiler_t * p_compiler = gravity_compiler_create(&delegate);
	msg_buf.Z().Cat("Gravity-unit-test").CatDiv('-', 1).Cat(pFileName).CatDiv(':', 2);
	THROW(p_compiler);
	THROW(PPGravityModule::LoadFile(pFileName, buffer));
	p_closure = gravity_compiler_run(p_compiler, buffer.GetBufC(buffer.GetRdOffs()), buffer.GetAvailableSize(), 0/*fileId*/, true/*isStatic*/, false/*addDebug*/);
	THROW(p_closure);
	THROW(p_vm = gravity_vm_new(&delegate));
	gravity_compiler_transfer(p_compiler, p_vm);
	gravity_compiler_free(p_compiler);
	p_compiler = 0;
	THROW(gravity_vm_runmain(p_vm, p_closure));
	{
		GravityValue result = gravity_vm_result(p_vm);
		ASSIGN_PTR(pResult, result);
		if(P_Utd) {
			static_cast<PPGravityUnitTestData *>(P_Utd)->Processed = true;
			if(static_cast<PPGravityUnitTestData *>(P_Utd)->IsFuzzy || gravity_value_equals(result, static_cast<PPGravityUnitTestData *>(P_Utd)->ExpectedValue)) {
				static_cast<PPGravityUnitTestData *>(P_Utd)->NSuccess++;
				msg_buf.Cat("SUCCESS");
				LogToOuterLogger(msg_buf);
				PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
			}
			else {
				static_cast<PPGravityUnitTestData *>(P_Utd)->NFailure++;
				msg_buf.Cat("FAILURE");
				LogToOuterLogger(msg_buf);
				PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
			}
			gravity_value_free(0, static_cast<PPGravityUnitTestData *>(P_Utd)->ExpectedValue);
			// case for empty files or simple declarations test
			if(!static_cast<PPGravityUnitTestData *>(P_Utd)->Processed) {
				static_cast<PPGravityUnitTestData *>(P_Utd)->NSuccess++;
				msg_buf.Cat("SUCCESS");
				LogToOuterLogger(msg_buf);
				PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
			}
		}
	}
	CATCHZOK
	gravity_vm_free(p_vm);
	gravity_compiler_free(p_compiler);
	return ok;
}

/*
class PapyrusGravityTestIfc {
	double Foo(int a, int b);
	prop X;
	prop Y;
	prop Z;
};
*/

static bool GravityProc_PapyrusGravityTestIfc_Foo(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
}

static bool GravityProc_PapyrusGravityTestIfc_GetProp(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
}

static bool GravityProc_PapyrusGravityTestIfc_SetProp(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
}

void TestGravity()
{
	int    ok = 1;
	PPLogger logger;
	PPGravityModule gm;
	SString temp_buf;
	const char * p_path = "D:/Papyrus/Src/OSF/gravity/test/unittest";
	(temp_buf = p_path).SetLastDSlash().Cat("*.gravity");
	SDirEntry de;
	for(SDirec direc(temp_buf); direc.Next(&de) > 0;) {
		if(de.IsFile()) {
			(temp_buf = p_path).SetLastDSlash().Cat(de.FileName);
			GravityValue result;
			gm.RunFile(temp_buf, &logger, &result);
		}
	}
	//CATCHZOK
}

#endif // } 0
#else
void TestGravity()
{
}
#endif // } _MSC_VER >= 1600
