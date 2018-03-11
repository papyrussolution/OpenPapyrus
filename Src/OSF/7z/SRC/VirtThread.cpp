// VirtThread.cpp
//
#include <7z-internal.h>
#pragma hdrstop

static THREAD_FUNC_DECL CoderThread(void * p)
{
	for(;; ) {
		CVirtThread * t = (CVirtThread*)p;
		t->StartEvent.Lock();
		if(t->Exit)
			return 0;
		t->Execute();
		t->FinishedEvent.Set();
	}
}

WRes CVirtThread::Create()
{
	RINOK(StartEvent.CreateIfNotCreated());
	RINOK(FinishedEvent.CreateIfNotCreated());
	StartEvent.Reset();
	FinishedEvent.Reset();
	Exit = false;
	return Thread.IsCreated() ? S_OK : Thread.Create(CoderThread, this);
}

void CVirtThread::Start()
{
	Exit = false;
	StartEvent.Set();
}

void CVirtThread::WaitThreadFinish()
{
	Exit = true;
	if(StartEvent.IsCreated())
		StartEvent.Set();
	if(Thread.IsCreated()) {
		Thread.Wait();
		Thread.Close();
	}
}
