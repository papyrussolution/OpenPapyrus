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

void SCriticalSection::Data::Enter()
{
	EnterCriticalSection(&C);
}

int SCriticalSection::Data::TryEnter()
{
	return TryEnterCriticalSection(&C);
}

void SCriticalSection::Data::Leave()
{
	LeaveCriticalSection(&C);
}
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

int FASTCALL SReadWriteLocker::Toggle(Type t)
{
	return Toggle(t, 0, 0);
	/*
	assert(oneof3(t, Read, Write, None));
	int    ok = 0;
	if(t == Read) {
		if(State & stRLocked)
			ok = 1;
		else if(State & stWLocked) {
			Unlock();
			ok = R_L.Helper_ReadLock(Timeout);
			if(ok > 0)
				State = stRLocked;
		}
	}
	else if(t == Write) {
		if(State & stWLocked)
			ok = 1;
		else if(State & stRLocked) {
			Unlock();
			ok = R_L.Helper_WriteLock(Timeout);
			if(ok > 0)
				State = stWLocked;
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
	*/
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
#define SIGN_SLTHREAD 0x09970199UL

SlThread::SlThread(void * pInitData, long stopTimeout) : Sign(SIGN_SLTHREAD), P_StartupSignal(0), P_Creation(0), P_Tla(0)
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

bool SlThread::IsConsistent() const { return (Sign == SIGN_SLTHREAD); }
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
#if SLTEST_RUNNING // {

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
			*P_SignalVar = -(int)Id; // @v9.6.3
			evnt_finish.Signal();
			// @v9.6.3 *P_SignalVar = -(int)Id;
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
	SLTEST_CHECK_NZ((HANDLE)ev_start);
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
				SLTEST_CHECK_NZ((HANDLE)ev_finish);
#ifndef NDEBUG
				assert(signal_var == 0);
#else
				SLTEST_CHECK_NZ(signal_var == 0);
#endif
				ev_start.Signal();
				SDelay(10);
#ifndef NDEBUG
				assert(signal_var == tid);
				assert(ev_finish.Wait(-1) > 0);
				assert(signal_var == -(int)tid);
#else
				SLTEST_CHECK_NZ(signal_var == tid);
				SLTEST_CHECK_NZ(ev_finish.Wait(-1) > 0);
				SLTEST_CHECK_NZ(signal_var == -(int)tid);
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
			CATCH
				//if(lck)
					//P_Lock->Unlock();
				*P_Result = 0;
			ENDCATCH
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
			p_w->Start(1);
			tl[tc++] = *p_w;
		}
		{
			Thread_R * p_r = new Thread_R(8000);
			p_r->Start(1);
			tl[tc++] = *p_r;
		}
		{
			Thread_W * p_w = new Thread_W(8000);
			p_w->Start(1);
			tl[tc++] = *p_w;
		}
		{
			Thread_R * p_r = new Thread_R(8000);
			p_r->Start(1);
			tl[tc++] = *p_r;
		}
		{
			Thread_R * p_r = new Thread_R(8000);
			p_r->Start(1);
			tl[tc++] = *p_r;
		}
		{
			Thread_W * p_w = new Thread_W(8000);
			p_w->Start(1);
			tl[tc++] = *p_w;
		}
		{
			Thread_R * p_r = new Thread_R(8000);
			p_r->Start(1);
			tl[tc++] = *p_r;
		}
		{
			Thread_R * p_r = new Thread_R(8000);
			p_r->Start(1);
			tl[tc++] = *p_r;
		}
		{
			Thread_W * p_w = new Thread_W(8000);
			p_w->Start(1);
			tl[tc++] = *p_w;
		}
		{
			Thread_R * p_r = new Thread_R(8000);
			p_r->Start(1);
			tl[tc++] = *p_r;
		}
		{
			Thread_R * p_r = new Thread_R(8000);
			p_r->Start(1);
			tl[tc++] = *p_r;
		}
		{
			Thread_W * p_w = new Thread_W(8000);
			p_w->Start(1);
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
			p_thread->Start(0);
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

#endif // } SLTEST_RUNNING
