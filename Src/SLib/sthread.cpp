// STHREAD.CPP
// Copyright (c) A.Sobolev 2003, 2005, 2007, 2008, 2010, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

void FASTCALL SDelay(uint msec) { ::Sleep(msec); }
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

ACount::operator long() const { return C; }

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

long ACount::Incr() { return ::InterlockedIncrement(&C); }
long ACount::Decr() { return ::InterlockedDecrement(&C); }
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

int FASTCALL ReadWriteLock::WriteLockT_(long timeout) { return Helper_WriteLock(timeout); }
int ReadWriteLock::WriteLock_() { return Helper_WriteLock(-1); }

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

bool SReadWriteLocker::operator !() const { return LOGIC(State & stError); }
int  SReadWriteLocker::GetState() const { return State; }
int  FASTCALL SReadWriteLocker::Toggle(Type t) { return Toggle(t, 0, 0); }

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
		// SFsPath::NormalizePath(pSrcFileName, SFsPath::npfSlash, TempBuf);
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
