// TEST-PROCESS.CPP
// Copyright (c) A.Sobolev 2023, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <wsctl.h>
#include <aclapi.h>

SLTEST_R(SecurityAttributes) // @v12.4.4
{
	class LocalBlock {
	public:
		static int Helper_CreateDir(const SString & rDir, bool useSA)
		{
			int    ok = -1;
			SECURITY_DESCRIPTOR * p_sd = 0;
			ACL * p_new_dacl = 0;
			SECURITY_ATTRIBUTES sa;
			SECURITY_ATTRIBUTES * p_sa = 0;
			if(useSA) {
				S_WinSID current_user_sid;
				if(SlProcess::GetCurrentUserSid(current_user_sid)) {	
					EXPLICIT_ACCESS_W ea;
					MEMSZERO(ea);
					ea.grfAccessPermissions = GENERIC_ALL|STANDARD_RIGHTS_ALL|SPECIFIC_RIGHTS_ALL;
					ea.grfAccessMode = SET_ACCESS;
					ea.grfInheritance = CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE;
					ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
					ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
					//ea.Trustee.ptstrName = (LPWSTR)pUserSid;
					ea.Trustee.ptstrName = static_cast<LPWSTR>(current_user_sid.GetPtr());
					{
						// Создаем новый DACL
						const DWORD seia_r = ::SetEntriesInAclW(1, &ea, 0/*p_old_dacl*/, &p_new_dacl);
						THROW(seia_r == ERROR_SUCCESS); // std::wcerr << L"SetEntriesInAcl failed: " << seia_r << std::endl;
						//
						p_sd = (SECURITY_DESCRIPTOR *)::LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
						THROW(::InitializeSecurityDescriptor(p_sd, SECURITY_DESCRIPTOR_REVISION));
						THROW(::SetSecurityDescriptorDacl(p_sd, TRUE/*bDaclPresent flag*/, p_new_dacl, FALSE/*not a default DACL*/));
						//
						{
							MEMSZERO(sa);
							sa.nLength = sizeof(sa);
							sa.lpSecurityDescriptor = p_sd;
							p_sa = &sa;
						}
					}
				}
			}
			{
				int    cdr = 0;
				if(p_sa) {
					cdr = SFile::CreateDirSA(rDir, p_sa);
				}
				else {
					cdr = SFile::CreateDir(rDir);
				}
				if(cdr) {
					ok = 1;
				}
				else {
					ok = 0;
				}
			}
			CATCHZOK
			if(p_sd) {
				::LocalFree(p_sd);
			}
			if(p_new_dacl) {
				::LocalFree(p_new_dacl);
			}
			return ok;
		}
	};
	SString temp_buf;
	SString base_directory;
	SString work_dir;
	uint   arg_no = 0;
	PSECURITY_DESCRIPTOR p_sd = 0;
	S_WinSID current_user_sid;
	if(EnumArg(&arg_no, temp_buf))
		base_directory = temp_buf;
	else {
		(base_directory = GetSuiteEntry()->OutPath).SetLastSlash().Cat("TEST-DIRECTORY");
	}
	if(!SFile::IsDir(base_directory)) {
		THROW(SFile::CreateDir(base_directory));
	}
	{
		for(long i = 1; i < 10000; i++) {
			(work_dir = base_directory).SetLastSlash().CatLongZ(i, 5);
			if(!SFile::IsDir(work_dir))
				break;
			else
				work_dir.Z();
		}
		THROW(work_dir.NotEmpty());
		THROW(SlProcess::GetCurrentUserSid(current_user_sid));
		{
			PSID   p_owner = 0;
			PSID   p_group = 0;
			ACL  * p_dacl = 0;
			ACL  * p_sacl = 0;
			SStringU temp_buf_u;
			THROW(LocalBlock::Helper_CreateDir(work_dir, true));
			temp_buf_u.CopyFromUtf8(work_dir);
			const DWORD gnsi_r = ::GetNamedSecurityInfoW(temp_buf_u.ucptr(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, &p_owner, &p_group, &p_dacl, &p_sacl, &p_sd);
			THROW(gnsi_r == ERROR_SUCCESS);
			{
				S_WinSID ws(p_dacl);
				TSCollection <WinSecurityAttrs::AceEntry> ace_list;
				WinSecurityAttrs::GetAclInfo(p_dacl, ace_list);
				{
					(temp_buf = GetSuiteEntry()->OutPath).SetLastSlash().Cat("SecurityAttributes_debug_info.txt");
					SFile f_out(temp_buf, SFile::mWrite);
					if(f_out.IsValid()) {
						SString _trustee_raw;
						SString _trustee;
						current_user_sid.ToStr(S_WinSID::fmtSystem, _trustee_raw);
						current_user_sid.ToStr(S_WinSID::fmtFriendly, _trustee);
						temp_buf.Z().Cat("current_user_sid").CatDiv(':', 2).Cat(_trustee_raw).CatChar('(').Cat(_trustee).CatChar(')');
						f_out.WriteLine(temp_buf.CR());
						if(ace_list.getCount()) {
							for(uint i = 0; i < ace_list.getCount(); i++) {
								const WinSecurityAttrs::AceEntry * p_entry = ace_list.at(i);
								if(p_entry) {
									temp_buf.Z().Cat(p_entry->TrusteeRaw).CatChar('(').Cat(p_entry->Trustee).CatChar(')');
									temp_buf.Space().Cat("AccessMode ").CatDiv(':', 2).CatHex(p_entry->AccessMode);
									temp_buf.Space().Cat("Inheritance").CatDiv(':', 2).CatHex(p_entry->Inheritance);
									temp_buf.Space().Cat("Permissions").CatDiv(':', 2).CatHex(p_entry->Permissions);
									f_out.WriteLine(temp_buf.CR());
								}
							}
						}
						else {
							temp_buf.Z().Cat("ace_list is empty");
							f_out.WriteLine(temp_buf.CR());
						}
					}
				}
			}
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	if(p_sd) 
		::LocalFree(p_sd);
	SFile::RemoveDir(work_dir);
	return CurrentStatus;
}

SLTEST_R(Evnt)
{
	class EvntThread : public SlThread {
	public:
		struct InitBlock {
			uint   Id;
			const  char * P_EvntNameStart;
			const  char * P_EvntNameFinish;
			volatile int * P_SignalVar;
			volatile int * P_Result;
		};
		EvntThread(InitBlock * pBlk) : SlThread(pBlk)
		{
			Id = pBlk->Id;
			EvntNameStart  = pBlk->P_EvntNameStart;
			EvntNameFinish = pBlk->P_EvntNameFinish;
			P_SignalVar = pBlk->P_SignalVar;
			P_Result = pBlk->P_Result;
		}
		virtual void Run()
		{
			assert(SLS.GetConstTLA().Id == GetThreadID());
			*P_Result = 1;
			Evnt evnt_start(EvntNameStart, Evnt::modeOpen);
			Evnt evnt_finish(EvntNameFinish, Evnt::modeCreate);
#ifndef NDEBUG
			assert((HANDLE)evnt_start);
			assert((HANDLE)evnt_finish);
			assert(evnt_start.Wait(-1) > 0);
#else
			if(!(HANDLE)evnt_start)
				*P_Result = 0;
			if(!(HANDLE)evnt_finish)
				*P_Result = 0;
			if(evnt_start.Wait(-1) < 0)
				*P_Result = 0;
#endif
			*P_SignalVar = Id;
			SDelay(500);
			*P_SignalVar = -(int)Id;
			evnt_finish.Signal();
		}
	private:
		uint   Id;
		volatile int * P_SignalVar;
		volatile int * P_Result;
		SString EvntNameStart;
		SString EvntNameFinish;
	};

	int    ok = 1;
	static volatile int signal_var = 0;
	static volatile int result = 0;
	SString evnt_name_start("TestEvnt_Start");
	SString evnt_name_finish("TestEvnt_Finish");
	Evnt ev_start(evnt_name_start, Evnt::modeCreate);
	SLCHECK_NZ((HANDLE)ev_start);
	{
		int    ok = 1;
		for(uint i = 1; i <= 10; i++) {
			const  uint tid = i;
			EvntThread::InitBlock blk;
			blk.Id = tid;
			blk.P_EvntNameStart  = evnt_name_start;
			blk.P_EvntNameFinish = evnt_name_finish;
			blk.P_SignalVar = &signal_var;
			blk.P_Result = &result;
			EvntThread * p_thread = new EvntThread(&blk);
			p_thread->Start();
			SDelay(500);
			{
				Evnt ev_finish(evnt_name_finish, Evnt::modeOpen);
				SLCHECK_NZ((HANDLE)ev_finish);
#ifndef NDEBUG
				assert(signal_var == 0);
#else
				SLCHECK_NZ(signal_var == 0);
#endif
				ev_start.Signal();
				SDelay(10);
#ifndef NDEBUG
				assert(signal_var == tid);
				assert(ev_finish.Wait(-1) > 0);
				assert(signal_var == -(int)tid);
#else
				SLCHECK_NZ(signal_var == tid);
				SLCHECK_NZ(ev_finish.Wait(-1) > 0);
				SLCHECK_NZ(signal_var == -(int)tid);
#endif
			}
			WaitForSingleObject((HANDLE)*p_thread, INFINITE);
			signal_var = 0;
			ev_start.Reset();
		}
	}
	return CurrentStatus;
}

SLTEST_R(ReadWriteLock)
{
	//
	// Тест ReadWrite-блокировки (ReadWriteLock)
	// Идея теста в том, чтобы заставить очень много потоков (128)
	// толкаться вокруг небольшого буфера RwlCommonDataList с целью записывать
	// в него предсказуемые значения, зависящие от номера потока (RwlThread::Id)
	// и параллельно считывать эти значения с одновременной проверкой корректности.
	//
	// Корректность записанных/считанных значений верифицируется следующим образом:
	// -- предварительно формируется буфер RwlDataPatternList допустимых для каждого
	//   потока значений (по RwlPattersPerThread на каждый поток).
	// -- поток за одну итерацию записывает в RwlCommonDataList какое-то одно из этих допустимых
	//   значений вместе с собственным идентификатором.
	// -- поток, читающий RwlCommonDataList проверяет весь массив на предмет того,
	//   чтобы каждый элемент (RwlEntry) массива содержал либо {0; 0} (еще не записанные
	//   ячейки), либо {Id; одно из допустимых для потока Id значение }.
	//
	// Так как количество ячеек в RwlCommonDataList мало по сравнению с количеством
	// потоков, они (потоки), будучи запущенными одновременно, станут толкаться вокруг
	// этого буфера и нарушение синхронизации моментально приведет к возникновению исключения (assert).
	// Последнее утверждение легко проверяется отключением вызовов WriteLock()/Unlock() или
	// ReadLock()/Unlock().
	//
	// Каждый поток при очередной итерации может быть либо "писателем" либо "читателем" в
	// зависимости от четности случайного числа, полученного на этой итерации. Кроме того,
	// после каждой итерации осуществляется небольшая случайная задержка SDelay(rn%7)
	//
	struct RwlEntry {
		uint   Id;
		int64  Val;
	};

	static const uint RwlThreadCount = 128;
	static const uint RwlPattersPerThread = 16;
	static RwlEntry RwlCommonDataList[37];
	static int64 RwlDataPatternList[RwlThreadCount*RwlPattersPerThread];
	static const char * P_StartEvntName = "RwlTest_Start";
	static volatile int ResultList[RwlThreadCount];

	class RwlThread : public SlThread {
	public:
		struct InitBlock {
			uint   Id;
			ReadWriteLock * P_Lock;
			volatile int  * P_Result;
		};
		RwlThread(InitBlock * pBlk) : SlThread(pBlk), Id(pBlk->Id), P_Lock(pBlk->P_Lock), P_Result(pBlk->P_Result)
		{
		}
		virtual void Run()
		{
			assert(SLS.GetConstTLA().Id == GetThreadID());
			*P_Result = 1;
			//int    lck = 0;
			assert(Id > 0 && Id <= RwlThreadCount);
			Evnt start_evnt(P_StartEvntName, Evnt::modeOpen);
			start_evnt.Wait(-1);
			for(uint i = 0; i < 10000; i++) {
				const  uint rn = SLS.GetTLA().Rg.GetUniformInt(200000+Id*7+i);
				const  int  writer = BIN((rn % 2) == 0);
				if(writer) {
					uint j = (rn % RwlPattersPerThread);
					int64 pattern = RwlDataPatternList[((Id-1) * RwlPattersPerThread)+j];
					{
						//P_Lock->WriteLock(); 
						SRWLOCKER(*P_Lock, SReadWriteLocker::Write);
						//lck = 1;
						const uint pos = rn%SIZEOFARRAY(RwlCommonDataList);
						RwlCommonDataList[pos].Id = Id;
						RwlCommonDataList[pos].Val = pattern;
						//P_Lock->Unlock(); 
						//lck = 0;
					}
				}
				else {
					//P_Lock->ReadLock(); 
					SRWLOCKER(*P_Lock, SReadWriteLocker::Read);
					//lck = 1;
					for(uint j = 0; j < SIZEOFARRAY(RwlCommonDataList); j++) {
						const RwlEntry & r_entry = RwlCommonDataList[j];
#ifndef NDEBUG
						assert(r_entry.Id != 0 || r_entry.Val == 0);
						assert(r_entry.Id >= 0 && r_entry.Id <= RwlThreadCount);
#else
						THROW(r_entry.Id != 0 || r_entry.Val == 0);
						THROW(r_entry.Id >= 0 && r_entry.Id <= RwlThreadCount);
#endif
						if(r_entry.Id) {
							int    found = 0;
							uint   k;
							for(k = 0; !found && k < RwlPattersPerThread; k++) {
								int64 pattern = RwlDataPatternList[((r_entry.Id-1) * RwlPattersPerThread)+k];
								if(r_entry.Val == pattern)
									found = 1;
							}
							uint   real_id = 0;
							for(k = 0; !real_id && k < SIZEOFARRAY(RwlDataPatternList); k++) {
								if(RwlDataPatternList[k] == r_entry.Val) {
									real_id = (k % RwlPattersPerThread) + 1;
								}
							}
#ifndef NDEBUG
							assert(found);
#else
							THROW(found);
#endif
						}
					}
					//P_Lock->Unlock(); 
					//lck = 0;
				}
				SDelay(rn%7);
			}
			//CATCH
				//if(lck)
					//P_Lock->Unlock();
				//*P_Result = 0;
			//ENDCATCH
		}
		uint   Id;
		ReadWriteLock * P_Lock;
		volatile int * P_Result;
	};
	int    ok = 1;
	{
		int    r1 = 0;
		int    r2 = 0;
		int    r3 = 0;
		int    ur = 0;
		ReadWriteLock lck;
		r1 = lck.WriteLockT_(1000);
		assert(r1 > 0);
		r2 = lck.ReadLockT_(1000);
		assert(r2 < 0);
		ur = lck.Unlock_();
		assert(ur);
		r2 = lck.ReadLockT_(1000);
		assert(r2 > 0);
		r3 = lck.ReadLockT_(1000);
		assert(r3 > 0);
		r1 = lck.WriteLockT_(1000);
		assert(r1 < 0);
		ur = lck.Unlock_();
		assert(ur);
		ur = lck.Unlock_();
		assert(ur);
		r1 = lck.WriteLockT_(1000);
		assert(r1 > 0);
		ur = lck.Unlock_();
		assert(ur);
		r2 = lck.ReadLockT_(1000);
		assert(r2 > 0);
		ur = lck.Unlock_();
		assert(ur);
	}
	{
		static ReadWriteLock _RwLck;
		class AbstractWriter {
			void * P;
		public:
			AbstractWriter(long timeout) : P(0)
			{
				P = (_RwLck.WriteLockT_(timeout) > 0) ? reinterpret_cast<void *>(1) : 0;
			}
			~AbstractWriter()
			{
				if(P) {
					_RwLck.Unlock_();
					P = 0;
				}
				assert(P == 0);
			}
			bool    IsValid() const { return (P != 0); }
		};
		class AbstractReader {
			void * P;
		public:
			AbstractReader(long timeout) : P(0)
			{
				P = (_RwLck.ReadLockT_(timeout) > 0) ? reinterpret_cast<void *>(1) : 0;
			}
			~AbstractReader()
			{
				if(P) {
					_RwLck.Unlock_();
					P = 0;
				}
				assert(P == 0);
			}
			bool   IsValid() const { return (P != 0); }
		};
		class Thread_W : public SlThread_WithStartupSignal {
			long    Timeout;
		public:
			Thread_W(long timeout) : SlThread_WithStartupSignal(0), Timeout(timeout)
			{
				assert(Timeout > 0);
			}
			void Run()
			{
				AbstractWriter o(Timeout*3);
				assert(o.IsValid());
				SDelay(Timeout / 2);
			}
		};
		class Thread_R : public SlThread_WithStartupSignal {
			long    Timeout;
		public:
			Thread_R(long timeout) : SlThread_WithStartupSignal(0), Timeout(timeout)
			{
				assert(Timeout > 0);
			}
			void Run()
			{
				AbstractReader o(Timeout*3);
				assert(o.IsValid());
				SDelay(Timeout / 2);
			}
		};
		//
		int    r1 = 0;
		int    r2 = 0;
		uint   tc = 0;
		HANDLE tl[128];
		MEMSZERO(tl);
		{
			Thread_W * p_w = new Thread_W(8000);
			p_w->Start(true);
			tl[tc++] = *p_w;
		}
		{
			Thread_R * p_r = new Thread_R(8000);
			p_r->Start(true);
			tl[tc++] = *p_r;
		}
		{
			Thread_W * p_w = new Thread_W(8000);
			p_w->Start(true);
			tl[tc++] = *p_w;
		}
		{
			Thread_R * p_r = new Thread_R(8000);
			p_r->Start(true);
			tl[tc++] = *p_r;
		}
		{
			Thread_R * p_r = new Thread_R(8000);
			p_r->Start(true);
			tl[tc++] = *p_r;
		}
		{
			Thread_W * p_w = new Thread_W(8000);
			p_w->Start(true);
			tl[tc++] = *p_w;
		}
		{
			Thread_R * p_r = new Thread_R(8000);
			p_r->Start(true);
			tl[tc++] = *p_r;
		}
		{
			Thread_R * p_r = new Thread_R(8000);
			p_r->Start(true);
			tl[tc++] = *p_r;
		}
		{
			Thread_W * p_w = new Thread_W(8000);
			p_w->Start(true);
			tl[tc++] = *p_w;
		}
		{
			Thread_R * p_r = new Thread_R(8000);
			p_r->Start(true);
			tl[tc++] = *p_r;
		}
		{
			Thread_R * p_r = new Thread_R(8000);
			p_r->Start(true);
			tl[tc++] = *p_r;
		}
		{
			Thread_W * p_w = new Thread_W(8000);
			p_w->Start(true);
			tl[tc++] = *p_w;
		}
		//
		::WaitForMultipleObjects(tc, tl, TRUE, INFINITE);
		r1 = _RwLck.WriteLock_();
		assert(r1 > 0);
		r1 = _RwLck.Unlock_();
		assert(r1 > 0);
		r2 = _RwLck.ReadLock_();
		assert(r2 > 0);
		r2 = _RwLck.Unlock_();
		assert(r2 > 0);
	}
	uint   i;
	ReadWriteLock * p_lock = new ReadWriteLock();
	memzero(RwlCommonDataList, sizeof(RwlCommonDataList));
	for(i = 0; i < RwlThreadCount; i++) {
		for(uint j = 0; j < RwlPattersPerThread; j++) {
			int64 pattern = (int64)SLS.GetTLA().Rg.GetUniformInt(2000000000L);
			RwlDataPatternList[(i * RwlPattersPerThread)+j] = pattern;
		}
	}
	{
		size_t h_chunk_count = 0;
		size_t h_count[6];
		HANDLE h_list[6][MAXIMUM_WAIT_OBJECTS];
		memzero(h_count, sizeof(h_count));
		memzero(h_list, sizeof(h_list));
		Evnt start_evnt(P_StartEvntName, Evnt::modeCreate);
		for(i = 0; i < RwlThreadCount; i++) {
			RwlThread::InitBlock blk;
			blk.Id = i+1;
			blk.P_Lock = p_lock;
			blk.P_Result = ResultList+i;
			RwlThread * p_thread = new RwlThread(&blk);
			p_thread->Start(false);
			//
			// WaitForMultipleObjects не может обработать более
			// MAXIMUM_WAIT_OBJECTS (64) объектов за один вызов
			//
			if(h_count[h_chunk_count] == MAXIMUM_WAIT_OBJECTS) {
				h_chunk_count++;
				assert(h_chunk_count < SIZEOFARRAY(h_count));
			}
			h_list[h_chunk_count][h_count[h_chunk_count]++] = (HANDLE)*p_thread;
		}
		start_evnt.Signal();
		int    r;
		for(i = 0; i <= h_chunk_count; i++) {
			r = WaitForMultipleObjects(h_count[i], h_list[i], 1, INFINITE);
		}
		for(i = 0; ok && i < RwlThreadCount; i++) {
			if(ResultList[i] == 0)
				ok = 0;
		}
	}
	return ok;
}

/*
	HKEY_LOCAL_MACHINE --> MACHINE
	HKEY_CURRENT_USER  --> CURRENT_USER
*/

SLTEST_R(SlProcess)
{
	bool   use_appcontainer = false;
	SString temp_buf;
	SString path;
	SString working_dir;
	SString policy_path;
	SlProcess p;
	SlProcess::Result result;
	SlProcess::AppContainer ac;
	WsCtl_ClientPolicy policy;
	SString path_in(GetSuiteEntry()->InPath);
	PPGetPath(PPPATH_BIN, path);
	path.SetLastSlash().Cat("..").SetLastSlash().Cat(PPLoadStringS("testapp_path", temp_buf).Transf(CTRANSF_INNER_TO_UTF8));
	working_dir = path;
	path.SetLastSlash().Cat("SlTestApp.exe");
	temp_buf = path;
	SFsPath::NormalizePath(temp_buf, SFsPath::npfCompensateDotDot, path);
	SLCHECK_NZ(p.SetPath(path));
	p.SetFlags(SlProcess::fNewConsole);

	p.SetWorkingDir(working_dir);

	p.AddArg("param1");
	p.AddArg(temp_buf.Z().CatQStr("param2 with spaces"));
	p.AddArg(temp_buf.Z().CatQStr("параметр3 с пробелами и русскими буквами"));
	if(pathValid(path_in, 1)) {
		p.AddArg(temp_buf.Z().Cat("policypath"));
		p.AddArg(temp_buf.Z().CatQStr(path_in));
	}
	//
	{
		bool policy_is_ok = false;
		(temp_buf = path_in).SetLastSlash().Cat("wsctl-policy.json");
		SJson * p_js = SJson::ParseFile(temp_buf);
		if(p_js) {
			if(policy.FromJsonObj(p_js)) {
				policy.Resolve();
				policy_is_ok = true;
			}
			ZDELETE(p_js);
		}
	}
	//
	if(use_appcontainer) {
		SLCHECK_NZ(ac.Create("Test-App-Container-2"));
		SLCHECK_NZ(ac.AllowPath(path_in, 0));
		SLCHECK_NZ(ac.AllowPath(working_dir, 0));
		{
			/*
				MACHINE\Software\Papyrus
				CURRENT_USER\Software\Papyrus
			*/
			// "CLASSES_ROOT", "CURRENT_USER", "MACHINE", and "USERS
			//WinRegKey test_key(HKEY_CURRENT_USER, PPConst::WrKey_SlTestApp, 0);
		
			temp_buf.Z().Cat("CURRENT_USER").SetLastSlash().Cat(/*PPConst::WrKey_SlTestApp*/"Software\\Papyrus");
			SLCHECK_NZ(ac.AllowRegistry(temp_buf, WinRegKey::regkeytypWow64_32, 0));

			temp_buf.Z().Cat("MACHINE").SetLastSlash().Cat(/*PPConst::WrKey_SlTestApp*/"Software\\Papyrus");
			SLCHECK_NZ(ac.AllowRegistry(temp_buf, WinRegKey::regkeytypWow64_32, 0));
		}
		p.SetAppContainer(&ac);
	}
	if(policy.SysUser.NotEmpty()) {
		p.SetImpersUser(policy.SysUser, policy.SysPassword);
		p.SetFlags(SlProcess::fLogonWithProfile);
	}
	{
		p.SetFlags(SlProcess::fCaptureStdOut|SlProcess::fCaptureStdErr);
	}
	const int run_result = p.Run(&result);
	SLCHECK_NZ(run_result);
	if(run_result) {
		{
			const wchar_t * p_named_pipe_name = L"\\\\.\\pipe\\sltestapp-pipe";
			SIntHandle h_pipe;
			bool do_exit = false;
			do {
				h_pipe = ::CreateFileW(p_named_pipe_name, GENERIC_READ|GENERIC_WRITE, 0/*no sharing*/,
					NULL/*default security attributes*/, OPEN_EXISTING/*opens existing pipe*/, 0/*default attributes*/, NULL/*no template file*/);
				if(!h_pipe) {
					if(GetLastError() == ERROR_PIPE_BUSY) {
						boolint w_ok = WaitNamedPipeW(p_named_pipe_name, 20000);
						if(!w_ok) {
							do_exit = true;
						}
					}
					else {
						do_exit = true;
					}
				}
			} while(!h_pipe && !do_exit);
			if(!!h_pipe) {
				DWORD pipe_mode = PIPE_READMODE_MESSAGE; 
				boolint _ok = ::SetNamedPipeHandleState(h_pipe, &pipe_mode/*new pipe mode*/, NULL/*don't set maximum bytes*/, NULL/*don't set maximum time*/);
				if(_ok) {
					STempBuffer wr_buf(1024);
					STempBuffer rd_buf(1024);
					SString message("Hello, Named Pipe!");
					DWORD wr_size = 0;
					boolint wr_ok = WriteFile(h_pipe, message.cptr(), message.Len()+1, &wr_size, NULL/*not overlapped*/);
					if(wr_ok) {
						do {
							DWORD rd_size = 0;
							boolint rd_ok = ReadFile(h_pipe, rd_buf, rd_buf.GetSize(), &rd_size, NULL/*not overlapped*/);
							if(rd_ok) {
								;
							}
							else {
								if(GetLastError() == ERROR_MORE_DATA) {
									
								}
							}
						} while(false);
					}
				}
			}
		}
		/*if(result.F_StdOut) {
			char out_line[1024];
			while(fgets(out_line, sizeof(out_line)-1, result.F_StdOut)) {
				(temp_buf = out_line).Strip().Chomp();
				SetInfo(temp_buf);
			}
		}*/
		/*if(result.F_StdErr) {
			char out_line[1024];
			while(fgets(out_line, sizeof(out_line)-1, result.F_StdErr)) {
				(temp_buf = out_line).Strip().Chomp();
				SetInfo(temp_buf);
			}
		}*/
	}
	//
	if(use_appcontainer) {
		SLCHECK_NZ(ac.Delete());
	}
	// @v12.3.11 {
	{
		wchar_t prc_name[512];
		SlProcess::ProcessPool pp;
		const uint ppc = SlProcess::GetProcessList(pp);
		SLCHECK_NZ(ppc);
		DWORD prc_name_buf_len = SIZEOFARRAY(prc_name);
		if(::QueryFullProcessImageNameW(::GetCurrentProcess(), 0, prc_name, &prc_name_buf_len)) {
			temp_buf.CopyUtf8FromUnicode(prc_name, sstrlen(prc_name), 1);
			SFsPath ps(temp_buf);
			ps.Merge(SFsPath::fNam|SFsPath::fExt, temp_buf);
			const SlProcess::ProcessPool::Entry * p_pp_entry = pp.SearchByExeFileName(temp_buf, 0);
			SLCHECK_NZ(p_pp_entry);
			if(p_pp_entry) {
				SLCHECK_EQ(p_pp_entry->ProcessId, static_cast<uint>(::GetCurrentProcessId()));
			}
		}
	}
	// } @v12.3.11 
	//CATCH
		//CurrentStatus = 0;
	//ENDCATCH
	return CurrentStatus;
}
