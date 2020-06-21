// PPGRAVITY.CPP
// Copyright (c) A.Sobolev 2020
// @codepage UTF-8
// @experimental
// Модуль интерфейса с языком gravity
//
#include <pp.h>
#pragma hdrstop
#if _MSC_VER >= 1600 // {
#include <..\OSF\gravity\include\gravity_.h>

#if 1 // {

class PPGravityModule {
public:
	SLAPI  PPGravityModule();
	int    SLAPI RunFile(const char * pFileName, GravityValue * pResult);

	static void CbError(void * pVm, int errorType, const char * pMsg, GravityErrorDescription * pErrorDesc, void * xdata);
	//typedef const char * (* gravity_loadfile_callback)(const char * file, size_t * size, uint32 * fileid, void * xdata, bool * is_static);
	static const char * CbLoadFile(const char * pFileName, size_t * pSize, uint32 * pFileId, void * pExtra, bool * pIsStatic);
	//typedef void (* gravity_unittest_callback)(gravity_vm * vm, GravityErrorType error_type, const char * desc, const char * note, GravityValue value, int32 row, int32 col, void * xdata);
	static void CbUnitTest(gravity_vm * pVm, GravityErrorType error_type, const char * pDescr, const char * pNote, GravityValue value, int32 row, int32 col, void * pExtra);
protected:
	int    SLAPI LoadFile(const char * pFileName, SBuffer & rBuffer);

	struct UnitTestData {
		UnitTestData() : Processed(false), IsFuzzy(false), NCount(0), NSuccess(0), NFailure(0), ExpectedErr(GRAVITY_ERROR_NONE),
			ExpectedRow(0), ExpectedCol(0)
		{
		}
		UnitTestData & Z()
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
	UnitTestData Utd;
};

SLAPI PPGravityModule::PPGravityModule()
{
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
	PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO);
	//printf("%s\n", pMsg);
}

int SLAPI PPGravityModule::LoadFile(const char * pFileName, SBuffer & rBuffer)
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

/*static*/void PPGravityModule::CbUnitTest(gravity_vm * pVm, GravityErrorType errorType, const char * pDescr, const char * pNote, GravityValue value, int32 row, int32 col, void * pExtra)
{
	PPGravityModule * p_this = static_cast<PPGravityModule *>(pExtra);
	if(p_this) {
		p_this->Utd.ExpectedErr = errorType;
		p_this->Utd.ExpectedValue = value;
		p_this->Utd.ExpectedRow = row;
		p_this->Utd.ExpectedCol = col;
		SString msg_buf;
		if(!isempty(pNote)) {
			msg_buf.Tab().Cat("NOTE").CatDiv(':', 2).Cat(pNote);
			PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO);
		}
		msg_buf.Z().Tab().Cat(pDescr);
		PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO);
	}
}

int SLAPI PPGravityModule::RunFile(const char * pFileName, GravityValue * pResult)
{
	int    ok = 1;
	SString msg_buf;
	SBuffer buffer;
	gravity_delegate_t delegate(this);
	delegate.error_callback = reinterpret_cast<gravity_error_callback>(PPGravityModule::CbError)/*unittest_error*/;
	delegate.unittest_callback = PPGravityModule::CbUnitTest/*unittest_callback*/;
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
	Utd.Processed = true;
	{
		GravityValue result = gravity_vm_result(p_vm);
		ASSIGN_PTR(pResult, result);
		if(Utd.IsFuzzy || gravity_value_equals(result, Utd.ExpectedValue)) {
			Utd.NSuccess++;
			msg_buf.Cat("SUCCESS");
			PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
		}
		else {
			Utd.NFailure++;
			msg_buf.Cat("FAILURE");
			PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
		}
		gravity_value_free(0, Utd.ExpectedValue);
	}
	// case for empty files or simple declarations test
	if(!Utd.Processed) {
		Utd.NSuccess++;
		msg_buf.Cat("SUCCESS");
		PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
	}
	CATCHZOK
	gravity_vm_free(p_vm);
	gravity_compiler_free(p_compiler);
	return ok;
}

void SLAPI TestGravity()
{
	int    ok = 1;
	PPGravityModule gm;
	SString temp_buf;
	const char * p_path = "D:/Papyrus/Src/OSF/gravity/test/unittest";
	(temp_buf = p_path).SetLastDSlash().Cat("*.gravity");
	SDirEntry de;
	for(SDirec direc(temp_buf); direc.Next(&de) > 0;) {
		if(de.IsFile()) {
			(temp_buf = p_path).SetLastDSlash().Cat(de.FileName);
			GravityValue result;
			gm.RunFile(temp_buf, &result);
		}
	}
	CATCHZOK
}

#endif // } 0
#endif // } _MSC_VER >= 1600
