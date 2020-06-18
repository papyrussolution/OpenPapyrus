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

#if 0 // {

class PPGravityModule {
public:
	SLAPI  PPGravityModule();
	int    SLAPI Compile(const char * pPgm);
	int    SLAPI Run();
protected:
	static void CbError(void * pVm, int errorType, const char * pMsg, GravityErrorDescription * pErrorDesc, void * xdata);
	//typedef const char * (* gravity_loadfile_callback)(const char * file, size_t * size, uint32 * fileid, void * xdata, bool * is_static);
	static const char * CbLoadFile(const char * pFileName, size_t * pSize, uint32 * pFileId, void * pExtra, bool * pIsStatic);
	//typedef void (* gravity_unittest_callback)(gravity_vm * vm, GravityErrorType error_type, const char * desc, const char * note, gravity_value_t value, int32 row, int32 col, void * xdata);
	static void CbUnitTest(gravity_vm * pVm, GravityErrorType error_type, const char * pDescr, const char * pNote, gravity_value_t value, int32 row, int32 col, void * pExtra);
	void * SLAPI CreateCompiler();
	void * P_Compiler;
};

SLAPI PPGravityModule::PPGravityModule() : P_Compiler(0)
{
}

/*static*/void PPGravityModule::CbError(void * pVm, int errorType, const char * pMsg, GravityErrorDescription * pErrorDesc, void * xdata)
{
}

/*static*/const char * PPGravityModule::CbLoadFile(const char * pFileName, size_t * pSize, uint32 * pFileId, void * pExtra, bool * pIsStatic)
{
	const char * p_ret = 0;
	return p_ret;
}

/*static*/void PPGravityModule::CbUnitTest(gravity_vm * pVm, GravityErrorType error_type, const char * pDescr, const char * pNote, gravity_value_t value, int32 row, int32 col, void * pExtra)
{
}

void * SLAPI PPGravityModule::CreateCompiler()
{
	gravity_delegate_t delegate(this);
	delegate.error_callback = reinterpret_cast<gravity_error_callback>(PPGravityModule::CbError)/*unittest_error*/;
	delegate.unittest_callback = PPGravityModule::CbUnitTest/*unittest_callback*/;
	delegate.loadfile_callback = PPGravityModule::CbLoadFile/*unittest_read*/;
	gravity_compiler_t * p_compiler = gravity_compiler_create(&delegate);
	return p_compiler;
}
#endif // } 0
#endif // } _MSC_VER >= 1600
