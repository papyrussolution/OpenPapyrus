// STHREAD.CPP
// Copyright (c) A.Sobolev 2003, 2005, 2007, 2008, 2010, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
// @v10.9.3 #include <process.h>
//
// Контрольный таймаут ожидания для заданных бесконечных таймаутов.
// Если в течении этого таймаута ожидание на завершилось, то предпринимаются
// информирующие действия (например, вывод в журнал).
// @v10.7.7 (moved to SlConstParam::WaitableObjCheckTimeout) static const long _CheckTimeout = 1 * 60 * 1000; 
//
void FASTCALL SDelay(uint msec)
{
	::Sleep(msec);
}
//
//
//
SWaitableObject::SWaitableObject() : H(0)
{
}

SWaitableObject::SWaitableObject(HANDLE h) : H(h)
{
}

SWaitableObject::~SWaitableObject()
{
	if(H)
		CloseHandle(H);
}

bool FASTCALL SWaitableObject::operator == (const SWaitableObject & s) const { return (H == s.H); }
bool SWaitableObject::IsValid() const { return LOGIC(H); }

int FASTCALL SWaitableObject::Wait(long timeout)
{
	int    ok = 0;
	if(timeout >= 0) {
		ok = ::WaitForSingleObject(H, timeout);
	}
	else {
		int    r = ::WaitForSingleObject(H, SlConst::WaitableObjCheckTimeout); // @v10.7.7 _CheckTimeout-->SlConst::WaitableObjCheckTimeout
		if(r < 0) {
			// @v10.4.0 {
			{
				SString temp_buf;
				temp_buf.CatCurDateTime(DATF_DMY, TIMF_HMS).Space().Cat("SWaitableObject::Wait _CheckTimeout overflow");
				SLS.LogMessage(0, temp_buf, 0); 
			}
			// } @v10.4.0
			ok = ::WaitForSingleObject(H, INFINITE);
		}
		else
			ok = r;
	}
	if(ok == WAIT_OBJECT_0)
		ok = 1;
	else if(ok == WAIT_TIMEOUT)
		ok = -1;
	else
		ok = 0;
	return ok;
}
//
//
//
ACount::ACount() : C(0)
{
}
//
// Descr: Этот конструктор используется тогда, когда не следует явно инициализировать переменную в ноль.
//
ACount::ACount(CtrOption option)
{
}

ACount::operator long() const
{
	return C;
}

long FASTCALL ACount::Add(long add)
{
	::InterlockedExchangeAdd(&C, add);
	return C;
}

long FASTCALL ACount::Assign(long val)
{
	InterlockedExchange(&C, val);
	return C;
}

long ACount::Incr()
{
	return ::InterlockedIncrement(&C);
}

long ACount::Decr()
{
	return ::InterlockedDecrement(&C);
}
//
//
//
SCriticalSection::Data::Data(int dontDestroy)  : DontDestroyOnDestruction(dontDestroy)
{
	InitializeCriticalSection(&C);
}

SCriticalSection::Data::~Data()
{
	if(!DontDestroyOnDestruction)
		DeleteCriticalSection(&C);
}

void SCriticalSection::Data::Enter() { EnterCriticalSection(&C); }
int  SCriticalSection::Data::TryEnter() { return TryEnterCriticalSection(&C); }
void SCriticalSection::Data::Leave() { LeaveCriticalSection(&C); }
//
//
//
// A BlockingCounter instance may be initialized
// only with a non-negative value
BlockingCounter::BlockingCounter() : ExclusiveAccess(0, 0), Count(0)
{
}

int BlockingCounter::Value() const { return Count; }
int BlockingCounter::IsClear() const { return (Count == 0); }
// Blocks until the counter is clear
int BlockingCounter::WaitUntilClear() { return ClearEvent.Wait(-1); }
// Blocks until the counter is dirty
int BlockingCounter::WaitUntilDirty() { return DirtyEvent.Wait(-1); }

// Non-blocking increment
int BlockingCounter::Increment()
{
	int    ok = ExclusiveAccess.Wait();
	if(ok) {
		int mustSignal = IsClear();
		// This is the actual increment
		Count++;
		// If the counter was clear before the increment,
		// reset Clear event and signal Dirty event
		// The Dirty event may be consumed many times.
		if(mustSignal) {
			ClearEvent.Reset();
			DirtyEvent.Signal();
		}
		ExclusiveAccess.Release();
	}
	return ok;
}

// Blocking decrement
int BlockingCounter::BlockingDecrement()
{
	int    ok = 1;
	int    is_raced = 1;
	while(ok && is_raced) {
		// Make sure the counter is dirty.
		ok = WaitUntilDirty();
		// The Dirty event has been signaled permanently.
		// It is possible that we've been raced.
		if(ok) {
			ok = ExclusiveAccess.Wait();
			if(ok) {
				// If we've been raced, we'll have to go back to
				// wait on the Dirty event.
				is_raced = IsClear();
				if(!is_raced) {
					// This is the actual decrement
					Count--;
					// If the counter is clear after the decrement,
					// reset Dirty event and signal Clear event.
					// The Clear event may be consumed many times.
					if(IsClear()) {
						ClearEvent.Signal();
						DirtyEvent.Reset();
					}
				}
				ExclusiveAccess.Release();
			}
		}
	}
	return ok;
}

BlockingCounter & BlockingCounter::operator++()
{
	Increment();
	return *this;
}

BlockingCounter & BlockingCounter::operator--()
{
	BlockingDecrement();
	return *this;
}
//
//
//
SemiMutex::SemiMutex() : ExclusiveAccess(0, 0)
{
}

// Share execution with other "readers".
int SemiMutex::ReadLock()
{
	// Lock exclusively for a moment
	// to make sure there is no "writer" on the way.
	int ok = ExclusiveAccess.Wait();
	if(ok) {
		// If we are here, then there is no "writer"
		// in the critical section.
		ok = ReadAccess.Increment();
		ExclusiveAccess.Release();
	}
	return ok;
}

// Terminate a shared execution.
int SemiMutex::ReadUnlock()
{
	// Decrement the blocking counter.
	// That may signal an event for a blocked "writer".
	return ReadAccess.BlockingDecrement();
}

// Exclusive lock
int SemiMutex::Lock()
{
	// Lock exclusively to stop all following threads -
	// "readers" and "writers". There still might be some
	// "readers" ahead.
	int ok = ExclusiveAccess.Wait();
	if(ok)
		ok = ReadAccess.WaitUntilClear();
	// Now there is no one either ahead or behind us.
	// The critical section remains locked exclusively.
	return ok;
}

int SemiMutex::Unlock()
{
	return ExclusiveAccess.Release();
}
//
//
//
Evnt::Evnt(const char * pName, int mode) : SWaitableObject()
{
	assert(oneof3(mode, modeCreate, modeCreateAutoReset, modeOpen));
	assert(!isempty(pName));
	if(oneof2(mode, modeCreate, modeCreateAutoReset)) {
		H = CreateEvent(0, (mode == modeCreateAutoReset) ? 0 : 1, 0, SUcSwitch(pName)); // @unicodeproblem
	}
	else {
		H = OpenEvent(EVENT_ALL_ACCESS, 0, SUcSwitch(pName)); // @unicodeproblem
	}
}

Evnt::Evnt(int mode) : SWaitableObject()
{
	assert(oneof2(mode, modeCreate, modeCreateAutoReset));
	H = CreateEvent(0, (mode == modeCreateAutoReset) ? 0 : 1, 0, 0);
}

int Evnt::Signal() { return BIN(SetEvent(H)); }
int Evnt::Reset() { return BIN(ResetEvent(H)); }
//
//
//
Sem::Sem(const char * pName, int mode, int initVal)
{
	assert(!isempty(pName));
	const TCHAR * p_name = SUcSwitch(pName); // @unicodeproblem
	H = (mode == modeCreate) ? ::CreateSemaphore(0, initVal, MAXLONG, p_name) : ::OpenSemaphore(EVENT_ALL_ACCESS, 0, p_name);
}

Sem::Sem(int initVal) : SWaitableObject(::CreateSemaphore(0, initVal, MAXLONG, 0))
{
}

int Sem::Release(int count)
{
	return BIN(ReleaseSemaphore(H, count, 0));
}
//
//
//
SMutex::SMutex(int initialValue, const char * pName) : SWaitableObject()
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = 0;
	sa.bInheritHandle = TRUE;
	H = ::CreateMutex(&sa, initialValue ? TRUE : FALSE, SUcSwitch(pName)); // @unicodeproblem
}

int SMutex::Release()
{
	return BIN(::ReleaseMutex(H));
}
//
//
//
STimer::STimer(const char * pName) : SWaitableObject(CreateWaitableTimer(0, 1, SUcSwitch(pName))) // @unicodeproblem
{
}

int STimer::Set(const LDATETIME & rDtm, long period)
{
	int    ok = 0;
	if(IsValid()) {
		LARGE_INTEGER due_time;
		SYSTEMTIME st;
		rDtm.Get(st);
		FILETIME ft_local, ft_utc;
		SystemTimeToFileTime(&st, &ft_local);
		LocalFileTimeToFileTime(&ft_local, &ft_utc);
		due_time.LowPart = ft_utc.dwLowDateTime;
		due_time.HighPart = ft_utc.dwHighDateTime;
		ok = BIN(SetWaitableTimer(H, &due_time, period, 0, 0, 0));
	}
	return ok;
}

int STimer::Cancel()
{
	return BIN(H && CancelWaitableTimer(H));
}
//
//
//
DirChangeNotification::DirChangeNotification(const char * pName, int watchSubtree, long filtFlags) : 
	SWaitableObject(::FindFirstChangeNotification(SUcSwitch(pName), watchSubtree, filtFlags)) // @unicodeproblem
{
}

DirChangeNotification::~DirChangeNotification()
{
	FindCloseChangeNotification(H);
	H = 0;
}

int DirChangeNotification::Next()
{
	return BIN(FindNextChangeNotification(H));
}
//
//
//
ReadWriteLock::ReadWriteLock() : Dr(0), Dw(0), ActiveCount(0)
{
}

ReadWriteLock::~ReadWriteLock()
{
	//assert(ActiveCount == 0);
}

int FASTCALL ReadWriteLock::Helper_ReadLock(long timeout)
{
	int    ok = 1;
	int    write_pending = 0;
	{
		SCriticalSection cs(Cs);
		write_pending = (Dw || ActiveCount < 0);
		if(write_pending) {
			Dr++;
			assert(ActiveCount >= -1);
		}
		else {
			ActiveCount++;
			assert(ActiveCount > 0);
		}
		assert(Dr >= 0);
		assert(Dw >= 0);
	}
	if(write_pending) {
		ok = Sr.Wait(timeout);
	}
	if(ok > 0) {
		assert(ActiveCount > 0);
	}
	// @v11.3.4 {
	else {
		assert(write_pending);
		SCriticalSection cs(Cs);
		Dr--;
		assert(Dr >= 0);
		assert(Dw >= 0);
		assert(ActiveCount >= -1);
	}
	// } @v11.3.4 
	return ok;
}

int FASTCALL ReadWriteLock::ReadLockT_(long timeout)
{
	return Helper_ReadLock(timeout);
}

int ReadWriteLock::ReadLock_()
{
	return Helper_ReadLock(-1);
}

int FASTCALL ReadWriteLock::Helper_WriteLock(long timeout)
{
	int    ok = 1;
	int    busy = 0;
	{
		SCriticalSection cs(Cs);
		busy = ActiveCount;
		if(busy)
			Dw++;
		else
			ActiveCount--;
		assert(Dr >= 0);
		assert(Dw >= 0);
		assert(ActiveCount >= -1);
	}
	if(busy) {
		ok = Sw.Wait(timeout);
	}
	if(ok > 0) {
		assert(ActiveCount == -1);
	}
	// @v11.3.4 {
	else {
		assert(busy);
		SCriticalSection cs(Cs);
		Dw--;
		assert(Dr >= 0);
		assert(Dw >= 0);
		assert(ActiveCount >= -1);
	}
	// } @v11.3.4 
	return ok;
}

int FASTCALL ReadWriteLock::WriteLockT_(long timeout)
{
	return Helper_WriteLock(timeout);
}

int ReadWriteLock::WriteLock_()
{
	return Helper_WriteLock(-1);
}

int ReadWriteLock::Unlock_()
{
	int    ok = 1;
	Sem  * p_sem = 0;
	int    c = 1;
	{
		SCriticalSection cs(Cs);
		assert(ActiveCount >= -1);
		if(ActiveCount > 0)
			ActiveCount--;
		else
			ActiveCount++;
		if(ActiveCount == 0) {
			if(Dw > 0) {
				ActiveCount--;
				Dw--;
				p_sem = &Sw;
			}
			else if(Dr > 0) {
				ActiveCount = Dr;
				Dr = 0;
				p_sem = &Sr;
				c = ActiveCount;
			}
		}
		assert(Dr >= 0);
		assert(Dw >= 0);
		assert(ActiveCount >= -1);
	}
	CALLPTRMEMB(p_sem, Release(c));
	return ok;
}
//
//
//
void FASTCALL SReadWriteLocker::InitInstance(Type t)
{
	assert(oneof3(t, Read, Write, None));
	State = 0;
	int    r = 0;
	if(t == Read) {
		r = R_L.Helper_ReadLock(Timeout);
		if(r > 0)
			State |= stRLocked;
	}
	else if(t == Write) {
		r = R_L.Helper_WriteLock(Timeout);
		if(r > 0)
			State |= stWLocked;
	}
	else if(t == None) {
		r = 1;
	}
	if(r <= 0) {
		State = stError;
		if(r < 0)
			State |= stTimeout;
	}
}

SReadWriteLocker::SReadWriteLocker(ReadWriteLock & rL, Type t, long timeout) : R_L(rL), Timeout(timeout)
{
	InitInstance(t);
}

SReadWriteLocker::SReadWriteLocker(ReadWriteLock & rL, Type t, long timeout, const char * pSrcFileName, uint srcLineNo) : R_L(rL), Timeout(timeout)
{
	int    s = 0;
	if(t != None) {
		SLS.LockPush(SLockStack::WRLT_to_LSLT(t), pSrcFileName, srcLineNo);
		s = 1;
	}
	InitInstance(t);
	if(s)
		State |= stTraced;
}

int SReadWriteLocker::operator !() const
{
	return BIN(State & stError);
}

int SReadWriteLocker::GetState() const
{
	return State;
}

SReadWriteLocker::~SReadWriteLocker()
{
	Unlock();
}

int SReadWriteLocker::Unlock()
{
	int    ok = 1;
	if(State & (stRLocked|stWLocked)) {
		R_L.Unlock_();
		State &= ~(stRLocked|stWLocked);
	}
	else
		ok = -1;
	if(State & stTraced) {
		SLS.LockPop();
		State &= ~stTraced;
	}
	return ok;
}

int FASTCALL SReadWriteLocker::Toggle(Type t) { return Toggle(t, 0, 0); }

int SReadWriteLocker::Toggle(Type t, const char * pSrcFileName, uint srcLineNo)
{
	assert(oneof3(t, Read, Write, None));
	int    ok = 0;
	int    s = 0;
	if(t == Read) {
		if(State & stRLocked)
			ok = 1;
		else if(State & stWLocked) {
			Unlock();
			if(pSrcFileName) {
				SLS.LockPush(SLockStack::ltRW_R, pSrcFileName, srcLineNo);
				s = 1;
			}
			ok = R_L.Helper_ReadLock(Timeout);
			if(ok > 0) 
				State = stRLocked;
			if(s)
				State |= stTraced;
		}
	}
	else if(t == Write) {
		if(State & stWLocked)
			ok = 1;
		else if(State & stRLocked) {
			Unlock();
			if(pSrcFileName) {
				SLS.LockPush(SLockStack::ltRW_W, pSrcFileName, srcLineNo);
				s = 1;
			}
			ok = R_L.Helper_WriteLock(Timeout);
			if(ok > 0)
				State = stWLocked;
			if(s)
				State |= stTraced;
		}
	}
	else if(t == None) {
		Unlock();
		ok = 1;
	}
	if(ok <= 0) {
		State = stError;
		if(ok < 0)
			State |= stTimeout;
	}
	return ok;
}
//
//
//
// @v11.8.2 (replaced with Signature_SlThread) #define SIGN_SLTHREAD 0x09970199UL

SlThread::SlThread(void * pInitData, long stopTimeout) : Sign(SlConst::Signature_SlThread), P_StartupSignal(0), P_Creation(0), P_Tla(0)
{
	Reset(pInitData, 1, stopTimeout);
}

SlThread::~SlThread()
{
	Stop(0);
	delete P_Creation;
	delete P_StartupSignal;
	Sign = 0;
}

int SlThread::InitStartupSignal()
{
	if(P_StartupSignal)
		return -1;
	else {
		P_StartupSignal = new Evnt;
		return P_StartupSignal ? 1 : (SLibError = SLERR_NOMEM, 0);
	}
}

bool SlThread::IsConsistent() const { return (Sign == SlConst::Signature_SlThread); }
int  SlThread::SignalStartup() { return P_StartupSignal ? P_StartupSignal->Signal() : 0; }
void SlThread::SetIdleState() { State_Slt |= stIdle; }
void SlThread::ResetIdleState() { State_Slt &= ~stIdle; }

void SlThread::Reset(void * pInitData, int withForce, long stopTimeout)
{
	State_Slt = 0;
	Handle = 0;
	ID     = 0;
	StopTimeout	= stopTimeout;
	// Reset init data only if force is applied
	if(withForce)
		P_InitData = pInitData;
}
//
// Create an execution thread
//
int FASTCALL SlThread::Start(int waitOnStartup)
{
	if(!(State_Slt & stRunning)) {
		P_Creation = new Evnt;
		uint   tmp_id;
		Handle = reinterpret_cast<ThreadHandle>(_beginthreadex(0, 0, _Exec, this, 0, &tmp_id));
		ID = static_cast<ThreadID>(tmp_id);
		SETFLAG(State_Slt, stRunning, Handle);
		// Now the new thread may run
		if(State_Slt & stRunning) {
			CALLPTRMEMB(P_Creation, Signal());
		}
	}
	if(waitOnStartup)
		assert(P_StartupSignal);
	if(P_StartupSignal) {
		P_StartupSignal->Wait();
		ZDELETE(P_StartupSignal);
	}
	return BIN(State_Slt & stRunning);
}

void SlThread::Stop(long timeout)
{
	if(State_Slt & stRunning) {
		if(WaitForSingleObject(Handle, NZOR(timeout, StopTimeout)) == WAIT_TIMEOUT)
			Terminate();
	}
}
//
// Terminate the execution thread brutally
//
int SlThread::Terminate()
{
	int    ok = BIN(TerminateThread(Handle, static_cast<DWORD>(-1)));
	State_Slt &= ~stRunning;
	return ok;
}

int SlThread::WaitUntilFinished()
{
	int    ok = (State_Slt & stRunning) ? 0 : 1;
	if(!ok)
		ok = (WaitForSingleObject(Handle, INFINITE) == WAIT_OBJECT_0);
	return ok;
}
//
// This implementation is guaranteed to be cancelable.
//
void FASTCALL SlThread::Sleep(uint milliseconds)
{
	SleepEx(milliseconds, TRUE);
}

void SlThread::SetStopState()
{
	State_Slt |= stLocalStop;
	EvLocalStop.Signal();
}

bool SlThread::IsRunning() const { return LOGIC(State_Slt & stRunning); }
bool SlThread::IsIdle() const { return LOGIC(State_Slt & stIdle); }
bool SlThread::IsStopping() const { return LOGIC(State_Slt & stLocalStop); }
//
// This method is invoked on behalf of the new thread before Run()
//
void SlThread::Startup()
{
	P_Tla = SLS.InitThread();
}
//
// This method is invoked on behalf of the dying thread after Run()
//
void SlThread::Shutdown()
{
	SLS.ReleaseThread();
}

void SlThread::Run()
{
}
//
// This is each thread's thread proc
/*static*/ThreadProcReturn THREADPROCCALL SlThread::_Exec(void * pThis)
{
	SlThread * p_this = static_cast<SlThread *>(pThis);
	if(p_this->P_Creation) {
		p_this->P_Creation->Wait();
		ZDELETE(p_this->P_Creation);
	}
	p_this->Startup();
	p_this->Run();
	p_this->Shutdown();
	p_this->Reset();
	delete p_this;
	return 0;
}
//
//
//
SlThread_WithStartupSignal::SlThread_WithStartupSignal(void * pInitData, long stopTimeout /*= SLTHREAD_DEFAULT_STOP_TIMEOUT*/) :
	SlThread(pInitData, stopTimeout)
{
	InitStartupSignal();
}

/*virtual*/void SlThread_WithStartupSignal::Startup()
{
	SlThread::Startup();
	SignalStartup();
}
//
//
//
/*static*/uint FASTCALL SLockStack::WRLT_to_LSLT(SReadWriteLocker::Type wrlt)
{
	switch(wrlt) {
		case SReadWriteLocker::Read: return ltRW_R;
		case SReadWriteLocker::Write: return ltRW_W;
		case SReadWriteLocker::None: return ltNone;
		default: return 1000; // UNKNOWN
	}
}

SLockStack::SLockStack()
{
}

void SLockStack::Push(uint lockType, const char * pSrcFileName, uint lineNo)
{
	Entry new_entry;
	MEMSZERO(new_entry);
	if(!isempty(pSrcFileName)) {
		// SPathStruc::NormalizePath(pSrcFileName, SPathStruc::npfSlash, TempBuf);
		//if(TempBuf.NotEmpty()) {
		new_entry.SrcFileSymbId = SLS.GetGlobalSymbol(pSrcFileName, 0, 0);
		//}
	}
	new_entry.SrcLineNo = lineNo;
	new_entry.LockType = lockType;
    new_entry.TimeCount = SLS.GetProfileTime();
    S.push(new_entry);
}

void SLockStack::Pop()
{
	Entry entry;
	int    r = S.pop(entry);
	assert(r);
}

void SLockStack::ToStr(SString & rBuf) const
{
	SString temp_buf;
	for(uint i = 0; i < S.getPointer(); i++) {
		const Entry & r_entry = *static_cast<const Entry *>(S.at(i));
		switch(r_entry.LockType) {
			case ltNone: rBuf.Cat("NONE"); break;
			case ltCS: rBuf.Cat("CS"); break;
			case ltRW_R: rBuf.Cat("RW-R"); break;
			case ltRW_W: rBuf.Cat("RW-W"); break;
			default: rBuf.Cat("UNKN"); break;
		}
		rBuf.Space().Cat(r_entry.TimeCount);
		rBuf.Space();
        if(r_entry.SrcFileSymbId > 0) {
			if(SLS.GetGlobalSymbol(0, r_entry.SrcFileSymbId, &temp_buf) > 0)
				rBuf.Cat(temp_buf);
			else
				rBuf.CatEq("unkn-file", r_entry.SrcFileSymbId);
        }
        else
			rBuf.CatEq("unkn-file", r_entry.SrcFileSymbId);
		if(r_entry.SrcLineNo >= 0) {
			rBuf.Space().Cat(r_entry.SrcLineNo);
		}
		rBuf.CR();
	}
}
//
// 
//
SlProcess::SlProcess() : Flags(0)
{
}

SlProcess::~SlProcess()
{
}

bool SlProcess::SetFlags(uint flags)
{
	bool  ok = true;
	// @todo Необходимо проверить переданные с аргументов флаги на непротиворечивость.
	// Например, internal-флаги не должны передаваться (их надо игнорировать).
	Flags |= flags;
	return ok;
}

static void foo()
{
	/*
	const char * p_app_name = 0;
	const char * p_cmd_line = 0;
		BOOL CreateProcessW(
		  [in, optional]      LPCWSTR               lpApplicationName,
		  [in, out, optional] LPWSTR                lpCommandLine,
		  [in, optional]      LPSECURITY_ATTRIBUTES lpProcessAttributes,
		  [in, optional]      LPSECURITY_ATTRIBUTES lpThreadAttributes,
		  [in]                BOOL                  bInheritHandles,
		  [in]                DWORD                 dwCreationFlags,
		  [in, optional]      LPVOID                lpEnvironment,
		  [in, optional]      LPCWSTR               lpCurrentDirectory,
		  [in]                LPSTARTUPINFOW        lpStartupInfo,
		  [out]               LPPROCESS_INFORMATION lpProcessInformation
		);
	*/
	/*

	fBreakawayFromJob           CREATE_BREAKAWAY_FROM_JOB
	fDefaultErrorMode           CREATE_DEFAULT_ERROR_MODE
	fNewConsole                 CREATE_NEW_CONSOLE
	fNewProcessGroup            CREATE_NEW_PROCESS_GROUP
	fNoWindow                   CREATE_NO_WINDOW
	fProtectedProcess           CREATE_PROTECTED_PROCESS
	fPreserveCodeAuthzLevel     CREATE_PRESERVE_CODE_AUTHZ_LEVEL
	fSecureProcess              CREATE_SECURE_PROCESS
	fSeparateWowVdm             CREATE_SEPARATE_WOW_VDM
	fSharedWowVdm               CREATE_SHARED_WOW_VDM
	fSuspended                  CREATE_SUSPENDED
	fUnicodeEnvironment         CREATE_UNICODE_ENVIRONMENT
	fDebugOnlyThisProcess       DEBUG_ONLY_THIS_PROCESS
	fDebugProcess               DEBUG_PROCESS
	fDetachedProcess            DETACHED_PROCESS
	fExtendedStartupinfoPresent EXTENDED_STARTUPINFO_PRESENT
	fInheritParentAffinity      INHERIT_PARENT_AFFINITY

	Creation Flags: 
		Constant/value 	Description
		CREATE_BREAKAWAY_FROM_JOB 0x01000000
			The child processes of a process associated with a job are not associated with the job.
			If the calling process is not associated with a job, this constant has no effect. 
			If the calling process is associated with a job, the job must set the JOB_OBJECT_LIMIT_BREAKAWAY_OK limit.
		CREATE_DEFAULT_ERROR_MODE 0x04000000
			The new process does not inherit the error mode of the calling process. Instead, the new process gets the default error mode.
			This feature is particularly useful for multithreaded shell applications that run with hard errors disabled.
			The default behavior is for the new process to inherit the error mode of the caller. Setting this flag changes that default behavior.
		CREATE_NEW_CONSOLE 0x00000010
			The new process has a new console, instead of inheriting its parent's console (the default). For more information, see Creation of a Console.
			This flag cannot be used with DETACHED_PROCESS.
		CREATE_NEW_PROCESS_GROUP 0x00000200
			The new process is the root process of a new process group. The process group includes all processes that are descendants of this root process. The process identifier of the new process group is the same as the process identifier, which is returned in the lpProcessInformation parameter. Process groups are used by the GenerateConsoleCtrlEvent function to enable sending a CTRL+BREAK signal to a group of console processes.
			If this flag is specified, CTRL+C signals will be disabled for all processes within the new process group.
			This flag is ignored if specified with CREATE_NEW_CONSOLE.
		CREATE_NO_WINDOW 0x08000000
			The process is a console application that is being run without a console window. Therefore, the console handle for the application is not set.
			This flag is ignored if the application is not a console application, or if it is used with either CREATE_NEW_CONSOLE or DETACHED_PROCESS.
		CREATE_PROTECTED_PROCESS 0x00040000
			The process is to be run as a protected process. The system restricts access to protected processes and the threads of protected processes. For more information on how processes can interact with protected processes, see Process Security and Access Rights.
			To activate a protected process, the binary must have a special signature. This signature is provided by Microsoft but not currently available for non-Microsoft binaries. There are currently four protected processes: media foundation, audio engine, Windows error reporting, and system. Components that load into these binaries must also be signed. Multimedia companies can leverage the first two protected processes. For more information, see Overview of the Protected Media Path.
			Windows Server 2003 and Windows XP: This value is not supported.
		CREATE_PRESERVE_CODE_AUTHZ_LEVEL 0x02000000
			Allows the caller to execute a child process that bypasses the process restrictions that would normally be applied automatically to the process.
		CREATE_SECURE_PROCESS 0x00400000
			This flag allows secure processes, that run in the Virtualization-Based Security environment, to launch.
		CREATE_SEPARATE_WOW_VDM 0x00000800
			This flag is valid only when starting a 16-bit Windows-based application. If set, the new process runs in a private Virtual DOS Machine (VDM). By default, all 16-bit Windows-based applications run as threads in a single, shared VDM. The advantage of running separately is that a crash only terminates the single VDM; any other programs running in distinct VDMs continue to function normally. Also, 16-bit Windows-based applications that are run in separate VDMs have separate input queues. That means that if one application stops responding momentarily, applications in separate VDMs continue to receive input. The disadvantage of running separately is that it takes significantly more memory to do so. You should use this flag only if the user requests that 16-bit applications should run in their own VDM.
		CREATE_SHARED_WOW_VDM 0x00001000
			The flag is valid only when starting a 16-bit Windows-based application. If the DefaultSeparateVDM switch in the Windows section of WIN.INI is TRUE, 
			this flag overrides the switch. The new process is run in the shared Virtual DOS Machine.
		CREATE_SUSPENDED 0x00000004
			The primary thread of the new process is created in a suspended state, and does not run until the ResumeThread function is called.
		CREATE_UNICODE_ENVIRONMENT 0x00000400
			If this flag is set, the environment block pointed to by lpEnvironment uses Unicode characters. Otherwise, the environment block uses ANSI characters.
		DEBUG_ONLY_THIS_PROCESS 0x00000002
			The calling thread starts and debugs the new process. It can receive all related debug events using the WaitForDebugEvent function.
		DEBUG_PROCESS 0x00000001
			The calling thread starts and debugs the new process and all child processes created by the new process. 
			It can receive all related debug events using the WaitForDebugEvent function.
			A process that uses DEBUG_PROCESS becomes the root of a debugging chain. 
			This continues until another process in the chain is created with DEBUG_PROCESS.
			If this flag is combined with DEBUG_ONLY_THIS_PROCESS, the caller debugs only the new process, not any child processes.
		DETACHED_PROCESS 0x00000008
			For console processes, the new process does not inherit its parent's console (the default). The new process can call the AllocConsole function at a later time to create a console. For more information, see Creation of a Console.
			This value cannot be used with CREATE_NEW_CONSOLE.
		EXTENDED_STARTUPINFO_PRESENT 0x00080000
			The process is created with extended startup information; the lpStartupInfo parameter specifies a STARTUPINFOEX structure.
			Windows Server 2003 and Windows XP: This value is not supported.
		INHERIT_PARENT_AFFINITY 0x00010000
			The process inherits its parent's affinity. If the parent process has threads in more than one processor group, 
			the new process inherits the group-relative affinity of an arbitrary group in use by the parent.
			Windows Server 2008, Windows Vista, Windows Server 2003 and Windows XP: This value is not supported.
	*/
	/*
		typedef struct _STARTUPINFOA {
			DWORD  cb;
			LPSTR  lpReserved;
			LPSTR  lpDesktop;
			LPSTR  lpTitle;
			DWORD  dwX;
			DWORD  dwY;
			DWORD  dwXSize;
			DWORD  dwYSize;
			DWORD  dwXCountChars;
			DWORD  dwYCountChars;
			DWORD  dwFillAttribute;
			DWORD  dwFlags;
			WORD   wShowWindow;
			WORD   cbReserved2;
			LPBYTE lpReserved2;
			HANDLE hStdInput;
			HANDLE hStdOutput;
			HANDLE hStdError;
		} STARTUPINFOA, *LPSTARTUPINFOA;
	*/
	//::CreateProcessW()
}

SlProcess::StartUpBlock::StartUpBlock() : Flags(0), ShowWindowFlags(0), P_AttributeList(0)
{
	ConsoleCharCount.Z();
}

SlProcess::StartUpBlock::~StartUpBlock()
{
	if(P_AttributeList) {
		DeleteProcThreadAttributeList(static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(P_AttributeList));
	}
}

bool SlProcess::Helper_SetParam(SStringU & rInner, const char * pParamUtf8)
{
	bool   ok = true;
	if(isempty(pParamUtf8)) {
		rInner.Z();
	}
	else {
		SString temp_buf(pParamUtf8);
		ok = temp_buf.IsLegalUtf8() ? rInner.CopyFromUtf8(temp_buf) : false;
	}
	return ok;
}

bool SlProcess::Helper_SetParam(SStringU & rInner, const wchar_t * pParam)
{
	bool   ok = true;
	if(isempty(pParam)) {
		rInner.Z();
	}
	else {
		rInner = pParam;
	}
	return ok;
}

bool SlProcess::Helper_SsAdd(StringSet & rInner, const char * pAddendumUtf8)
{
	bool   ok = true;
	if(isempty(pAddendumUtf8)) {
		;
	}
	else {
		SString temp_buf(pAddendumUtf8);
		if(temp_buf.IsLegalUtf8()) {
			// @todo Здесь надо как-то проверить добавляемую строку на предмет того, что 
			// ничего подобного в контейнере rInner нет, но в виду различной природы
			// контейнеров не понятно как это сделать унифицированным образом.
			rInner.add(temp_buf);
		}
		else
			ok = false;
	}
	return ok;
}

bool SlProcess::Helper_SsAdd(StringSet & rInner, const wchar_t * pAddendum)
{
	bool   ok = true;
	if(isempty(pAddendum)) {
		;
	}
	else {
		SString temp_buf;
		if(temp_buf.CopyUtf8FromUnicode(pAddendum, sstrlen(pAddendum), 1) && temp_buf.IsLegalUtf8()) {
			// @todo Здесь надо как-то проверить добавляемую строку на предмет того, что 
			// ничего подобного в контейнере rInner нет, но в виду различной природы
			// контейнеров не понятно как это сделать унифицированным образом.
			rInner.add(temp_buf);
		}
		else
			ok = false;
	}
	return ok;	
}

bool SlProcess::SetAppName(const char * pAppNameUtf8) { return Helper_SetParam(AppName, pAppNameUtf8); }
bool SlProcess::SetAppName(const wchar_t * pAppName) { return Helper_SetParam(AppName, pAppName); }
bool SlProcess::SetPath(const char * pPathUtf8) { return Helper_SetParam(Path, pPathUtf8); }
bool SlProcess::SetPath(const wchar_t * pPath) { return Helper_SetParam(Path, pPath); }
bool SlProcess::SetWorkingDir(const char * pWorkingDirUtf8) { return Helper_SetParam(WorkingDir, pWorkingDirUtf8); }
bool SlProcess::SetWorkingDir(const wchar_t * pWorkingDir) { return Helper_SetParam(WorkingDir, pWorkingDir); }

bool SlProcess::AddArg(const char * pArgUtf8) { return Helper_SsAdd(SsArgUtf8, pArgUtf8); }
bool SlProcess::AddArg(const wchar_t * pArg) { return Helper_SsAdd(SsArgUtf8, pArg); }
bool SlProcess::AddEnv(const char * pKeyUtf8, const char * pValUtf8)
{
	bool   ok = true;
	if(sstrchr(pKeyUtf8, '=') || sstrchr(pValUtf8, '=')) {
		ok = false; // @todo @err
	}
	else {
		SString temp_buf;
		temp_buf.CatEq(pKeyUtf8, pValUtf8);
		ok = Helper_SsAdd(SsEnvUtf8, temp_buf);
	}
	return ok;
}

bool   SlProcess::AddEnv(const wchar_t * pKey, const wchar_t * pVal)
{
	bool   ok = true;
	if(sstrchr(pKey, L'=') || sstrchr(pVal, L'=')) {
		ok = false; // @todo @err
	}
	else {
		SStringU temp_buf;
		temp_buf.CatEq(pKey, pVal);
		ok = Helper_SsAdd(SsEnvUtf8, temp_buf);
	}
	return ok;
}

int SlProcess::Run()
{
	int    ok = 0;
	THROW(Path.NotEmpty()); // @todo @err
	{
		/*
		wchar_t cmd_line_[512];
		wchar_t working_dir_[512];
		SString temp_buf;
		SStringU cmd_line_u;
		SStringU working_dir_u;
		cmd_line_u.CopyFromUtf8(pPe->FullResolvedPath);
		STRNSCPY(cmd_line_, cmd_line_u);
		SPathStruc ps(pPe->FullResolvedPath);
		ps.Merge(SPathStruc::fDrv|SPathStruc::fDir, temp_buf);
		working_dir_u.CopyFromUtf8(temp_buf.SetLastSlash());
		STRNSCPY(working_dir_, working_dir_u);
		SECURITY_ATTRIBUTES process_attr;
		SECURITY_ATTRIBUTES thread_attr;
		BOOL   inherit_handles = FALSE;
		DWORD  creation_flags = 0;
		void * p_env = 0;
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		MEMSZERO(si);
		si.cb = sizeof(si);
		MEMSZERO(pi);
		r = ::CreateProcessW(0, cmd_line_, 0, 0, inherit_handles, creation_flags, p_env, working_dir_, &si, &pi);
		*/
		const wchar_t * p_app_name = 0;
		wchar_t * p_cmd_line = 0;
		STempBuffer cmd_line(1024 * sizeof(wchar_t));
		SECURITY_ATTRIBUTES * p_prc_attr_list = 0;
		SECURITY_ATTRIBUTES * p_thread_attr_list = 0;
		BOOL inherit_handles = false;
		DWORD creation_flags = 0;
		void * p_env = 0;
		const wchar_t * p_curr_dir = 0;
		STARTUPINFOEXW startup_info;
		/*_Out_*/PROCESS_INFORMATION prc_info;
		//
		if(AppName.NotEmpty()) {
			p_app_name = AppName.ucptr();
		}
		//
		//int r = ::CreateProcessW(p_app_name, p_cmd_line, p_prc_attr_list, p_thread_attr_list, inherit_handles, creation_flags, p_env, p_curr_dir, &startup_info, &prc_info);
	}
	CATCHZOK
	return ok;
}
