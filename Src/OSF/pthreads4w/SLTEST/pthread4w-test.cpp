// PTHREAD4W-TEST.CPP
//
#include <sl_pthreads4w.h>
#pragma hdrstop
#include "sltest/test.h"

int assertE;

const char * PThr4wErrorString[] = {
  "ZERO_or_EOK",
  "EPERM",
  "ENOFILE_or_ENOENT",
  "ESRCH",
  "EINTR",
  "EIO",
  "ENXIO",
  "E2BIG",
  "ENOEXEC",
  "EBADF",
  "ECHILD",
  "EAGAIN",
  "ENOMEM",
  "EACCES",
  "EFAULT",
  "UNKNOWN_15",
  "EBUSY",
  "EEXIST",
  "EXDEV",
  "ENODEV",
  "ENOTDIR",
  "EISDIR",
  "EINVAL",
  "ENFILE",
  "EMFILE",
  "ENOTTY",
  "UNKNOWN_26",
  "EFBIG",
  "ENOSPC",
  "ESPIPE",
  "EROFS",
  "EMLINK",
  "EPIPE",
  "EDOM",
  "ERANGE",
  "UNKNOWN_35",
  "EDEADLOCK_or_EDEADLK",
  "UNKNOWN_37",
  "ENAMETOOLONG",
  "ENOLCK",
  "ENOSYS",
  "ENOTEMPTY",
#if  __PTW32_VERSION_MAJOR > 2
  "EILSEQ",
#else
  "EILSEQ_or_EOWNERDEAD",
  "ENOTRECOVERABLE"
#endif
};

void Implement_AssertE(bool condition, int iE, const char * pE, const char * pO, const char * pR, const char * pFile, const int line) 
{
	assertE = iE;
	if(condition) {
		if(ASSERT_TRACE) {
			fprintf(stderr, "Assertion succeeded: (%s), file %s, line %d\n", pE, pFile, line);
			fflush(stderr);
		}
	}
	else {
		if(assertE <= (int)(sizeof(PThr4wErrorString)/sizeof(PThr4wErrorString[0]))) {
			fprintf(stderr, "Assertion failed: (%s %s %s), file %s, line %d, error %s\n", pE, pO, pR, pFile, line, PThr4wErrorString[assertE]);
		}
		else {
			fprintf(stderr, "Assertion failed: (%s %s %s), file %s, line %d, error %d\n", pE, pO, pR, pFile, line, assertE);
		}
		exit(1);
	}
	//(((assertE = e) o (r)) ? ((ASSERT_TRACE) ? fprintf(stderr, "Assertion succeeded: (%s), file %s, line %d\n", \
	//#e, __FILE__, (int)__LINE__), fflush(stderr) : 0) : \
	//(assertE <= (int)(sizeof(PThr4wErrorString)/sizeof(PThr4wErrorString[0]))) ? \
	//(fprintf(stderr, "Assertion failed: (%s %s %s), file %s, line %d, error %s\n", #e,#o,#r, __FILE__, (int) __LINE__, error_string[assertE]), exit(1), 0) :\
	//(fprintf(stderr, "Assertion failed: (%s %s %s), file %s, line %d, error %d\n", #e,#o,#r, __FILE__, (int) __LINE__, assertE), exit(1), 0))
}

int PThr4wTest_Sizes()
{
	class InnerBlock {
	public:
		static void PrintTaggedSize(const char * pTitle, size_t sz)
		{
			if(pTitle == 0)
				printf("-------------------------------\n");
			else if(sz == 0)
				printf("%s\n", pTitle);
			else
				printf("%30s %4d\n", pTitle, static_cast<int>(sz));
		}
	};
	InnerBlock::PrintTaggedSize("Sizes of pthreads-win32 structs", 0);
	InnerBlock::PrintTaggedSize(0, 0);
	InnerBlock::PrintTaggedSize("pthread_t", sizeof(pthread_t));
	InnerBlock::PrintTaggedSize("__ptw32_thread_t", sizeof(__ptw32_thread_t));
	InnerBlock::PrintTaggedSize("pthread_attr_t_", sizeof(struct pthread_attr_t_));
	InnerBlock::PrintTaggedSize("sem_t_", sizeof(struct sem_t_));
	InnerBlock::PrintTaggedSize("pthread_mutex_t_", sizeof(struct pthread_mutex_t_));
	InnerBlock::PrintTaggedSize("pthread_mutexattr_t_", sizeof(struct pthread_mutexattr_t_));
	InnerBlock::PrintTaggedSize("pthread_spinlock_t_", sizeof(struct pthread_spinlock_t_));
	InnerBlock::PrintTaggedSize("pthread_barrier_t_", sizeof(struct pthread_barrier_t_));
	InnerBlock::PrintTaggedSize("pthread_barrierattr_t_", sizeof(struct pthread_barrierattr_t_));
	InnerBlock::PrintTaggedSize("pthread_key_t_", sizeof(struct pthread_key_t_));
	InnerBlock::PrintTaggedSize("pthread_cond_t_", sizeof(struct pthread_cond_t_));
	InnerBlock::PrintTaggedSize("pthread_condattr_t_", sizeof(struct pthread_condattr_t_));
	InnerBlock::PrintTaggedSize("pthread_rwlock_t_", sizeof(struct pthread_rwlock_t_));
	InnerBlock::PrintTaggedSize("pthread_rwlockattr_t_", sizeof(struct pthread_rwlockattr_t_));
	InnerBlock::PrintTaggedSize("pthread_once_t_", sizeof(struct pthread_once_t_));
	InnerBlock::PrintTaggedSize("__ptw32_cleanup_t", sizeof(struct __ptw32_cleanup_t));
	InnerBlock::PrintTaggedSize("__ptw32_mcs_node_t_", sizeof(struct __ptw32_mcs_node_t_));
	InnerBlock::PrintTaggedSize("sched_param", sizeof(struct sched_param));
	InnerBlock::PrintTaggedSize(0, 0);
	return 0;
}

int PThr4wTest_Affinity1();
int PThr4wTest_Affinity2();
int PThr4wTest_Affinity3();
int PThr4wTest_Affinity4();
int PThr4wTest_Affinity5();
int PThr4wTest_Affinity6();
int PThr4wTest_Barrier1();
int PThr4wTest_Barrier2();
int PThr4wTest_Barrier3();
int PThr4wTest_Barrier4();
int PThr4wTest_Barrier5();
int PThr4wTest_Barrier6();
int PThr4wTest_Create1();
int PThr4wTest_Create2();
int PThr4wTest_Create3();
int PThr4wTest_Mutex1();
int PThr4wTest_Mutex1e();
int PThr4wTest_Mutex1n();
int PThr4wTest_Mutex1r();
int PThr4wTest_Mutex2();
int PThr4wTest_Mutex2e();
int PThr4wTest_Mutex2r();
int PThr4wTest_Mutex3();
int PThr4wTest_Mutex3e();
int PThr4wTest_Mutex3r();
int PThr4wTest_Mutex4();
int PThr4wTest_Mutex5();
int PThr4wTest_Mutex6();
int PThr4wTest_Mutex6e();
int PThr4wTest_Mutex6es();
int PThr4wTest_Mutex6n();
int PThr4wTest_Mutex6r();
int PThr4wTest_Mutex6rs();
int PThr4wTest_Mutex6s();
int PThr4wTest_Mutex7();
int PThr4wTest_Mutex7e();
int PThr4wTest_Mutex7n();
int PThr4wTest_Mutex7r();
int PThr4wTest_Mutex8();
int PThr4wTest_Mutex8e();
int PThr4wTest_Mutex8n();
int PThr4wTest_Mutex8r();

int PThr4wTest_CleanUp0();
int PThr4wTest_CleanUp1();
int PThr4wTest_CleanUp2();
int PThr4wTest_CleanUp3();

int PThr4wTest_Cancel1();
int PThr4wTest_Cancel2();
int PThr4wTest_Cancel3();
int PThr4wTest_Cancel4();
int PThr4wTest_Cancel5();
int PThr4wTest_Cancel6a();
int PThr4wTest_Cancel6d();
int PThr4wTest_Cancel7();
int PThr4wTest_Cancel8();
int PThr4wTest_Cancel9();

int PThr4wTest_Join0();
int PThr4wTest_Join1();
int PThr4wTest_Join2();
int PThr4wTest_Join3();
int PThr4wTest_Join4();

int PThr4wTest_Tsd1();
int PThr4wTest_Tsd2();
int PThr4wTest_Tsd3();

int PThr4wTest_Valid1();
int PThr4wTest_Valid2();

int PThr4wTest_RwLock1();
int PThr4wTest_RwLock2();
int PThr4wTest_RwLock2t();
int PThr4wTest_RwLock3();
int PThr4wTest_RwLock3t();
int PThr4wTest_RwLock4();
int PThr4wTest_RwLock4t();
int PThr4wTest_RwLock5();
int PThr4wTest_RwLock5t();
int PThr4wTest_RwLock6();
int PThr4wTest_RwLock6t();
int PThr4wTest_RwLock6t2();
int PThr4wTest_RwLock7();
int PThr4wTest_RwLock71();
int PThr4wTest_RwLock8();
int PThr4wTest_RwLock81();

int PThr4wTest_Semaphore1();
int PThr4wTest_Semaphore2();
int PThr4wTest_Semaphore3();
int PThr4wTest_Semaphore4();
int PThr4wTest_Semaphore4t();
int PThr4wTest_Semaphore5();

int PThr4wTest_CondVar1();
int PThr4wTest_CondVar11();
int PThr4wTest_CondVar12();
int PThr4wTest_CondVar2();
int PThr4wTest_CondVar21();
int PThr4wTest_CondVar3();
int PThr4wTest_CondVar31();
int PThr4wTest_CondVar32();
int PThr4wTest_CondVar33();
int PThr4wTest_CondVar4();
int PThr4wTest_CondVar5();
int PThr4wTest_CondVar6();
int PThr4wTest_CondVar7();
int PThr4wTest_CondVar8();
int PThr4wTest_CondVar9();

int PThr4wTest_Spin1();
int PThr4wTest_Spin2();
int PThr4wTest_Spin3();
int PThr4wTest_Spin4();

int PThr4wTest_Delay1();
int PThr4wTest_Delay2();
int PThr4wTest_Detach1();

int PThr4wTest_Equal0();
int PThr4wTest_Equal1();
int PThr4wTest_Errno0();
int PThr4wTest_Errno1();
int PThr4wTest_Exception1();
int PThr4wTest_Exception2(int argc, char* argv[]);
int PThr4wTest_Exception3();
int PThr4wTest_Exception30();
int PThr4wTest_Exit1();
int PThr4wTest_Exit2();
int PThr4wTest_Exit3();
int PThr4wTest_Exit4();
int PThr4wTest_Exit5();
int PThr4wTest_Exit6();
int PThr4wTest_Inherit1();
int PThr4wTest_Kill1();
int PThr4wTest_Once1();
int PThr4wTest_Once2();
int PThr4wTest_Once3();
int PThr4wTest_Once4();
int PThr4wTest_NameNp1();
int PThr4wTest_NameNp2();

int PThr4wTest_Context1();
int PThr4wTest_Context2();
int PThr4wTest_Count1();
int PThr4wTest_Robust1();
int PThr4wTest_Robust2();
int PThr4wTest_Robust3();
int PThr4wTest_Robust4();
int PThr4wTest_Robust5();
int PThr4wTest_TryEnterCs1();
int PThr4wTest_TryEnterCs2();
int PThr4wTest_Timeouts();
int PThr4wTest_ThreeStage(int argc, char * argv[]);
int PThr4wTest_Stress1();

int PThr4wTest_Priority1();
int PThr4wTest_Priority2();
int PThr4wTest_Reinit1();
int PThr4wTest_Reuse1();
int PThr4wTest_Reuse2();
int PThr4wTest_Self1();
int PThr4wTest_Self2();
int PThr4wTest_Eyal1();
int PThr4wTest_Sequence1();

int PThr4wTest_Benchtest1();
int PThr4wTest_Benchtest2();
int PThr4wTest_Benchtest3();
int PThr4wTest_Benchtest4();
int PThr4wTest_Benchtest5();

int DoTest_PThr4w()
{
	pthread_win32_process_attach_np();
	//PThr4wTest_Robust1();
	//PThr4wTest_Robust2();
	//PThr4wTest_Robust3();
	//PThr4wTest_Robust4();
	//PThr4wTest_Robust5();
	PThr4wTest_Sizes();
	PThr4wTest_Eyal1();
	//PThr4wTest_Sequence1();
	PThr4wTest_TryEnterCs1();
	PThr4wTest_TryEnterCs2();
	PThr4wTest_Timeouts();
	PThr4wTest_Stress1();
	PThr4wTest_Affinity1();
	PThr4wTest_Affinity2();
	PThr4wTest_Affinity3();
	PThr4wTest_Affinity4();
	PThr4wTest_Affinity5();
	PThr4wTest_Affinity6();
	PThr4wTest_Join0();
	PThr4wTest_Join1();
	PThr4wTest_Join2();
	PThr4wTest_Join3();
	PThr4wTest_Join4();
	PThr4wTest_Create1();
	PThr4wTest_Create2();
	PThr4wTest_Create3();
	PThr4wTest_Kill1();
	PThr4wTest_Delay1();
	PThr4wTest_Delay2();
	PThr4wTest_Tsd1();
	PThr4wTest_Tsd2();
	PThr4wTest_Tsd3();
	PThr4wTest_Valid1();
	PThr4wTest_Valid2();
	PThr4wTest_Once1();
	PThr4wTest_Once2();
	PThr4wTest_Once3();
	PThr4wTest_Once4();
	PThr4wTest_Barrier1();
	PThr4wTest_Barrier2();
	PThr4wTest_Barrier3();
	PThr4wTest_Barrier4();
	PThr4wTest_Barrier5();
	PThr4wTest_Barrier6();
	PThr4wTest_Mutex1();
	PThr4wTest_Mutex2();
	PThr4wTest_Mutex2e();
	PThr4wTest_Mutex2r();
	PThr4wTest_Mutex3();
	PThr4wTest_Mutex3e();
	PThr4wTest_Mutex3r();
	PThr4wTest_Mutex4();
	PThr4wTest_Mutex5();
	PThr4wTest_Mutex6();
	PThr4wTest_Mutex6e();
	PThr4wTest_Mutex6es();
	PThr4wTest_Mutex6n();
	PThr4wTest_Mutex6r();
	PThr4wTest_Mutex6rs();
	PThr4wTest_Mutex6s();
	PThr4wTest_Mutex7();
	PThr4wTest_Mutex7e();
	PThr4wTest_Mutex7n();
	PThr4wTest_Mutex7r();
	PThr4wTest_Mutex8();
	PThr4wTest_Mutex8e();
	PThr4wTest_Mutex8n();
	PThr4wTest_Mutex8r();
	PThr4wTest_Mutex1e();
	PThr4wTest_Mutex1n();
	PThr4wTest_Mutex1r();
	PThr4wTest_Semaphore1();
	PThr4wTest_Semaphore2();
	PThr4wTest_Semaphore3();
	PThr4wTest_Semaphore4();
	PThr4wTest_Semaphore4t();
	PThr4wTest_Semaphore5();
	PThr4wTest_RwLock1();
	PThr4wTest_RwLock2();
	PThr4wTest_RwLock2t();
	PThr4wTest_RwLock3();
	PThr4wTest_RwLock3t();
	PThr4wTest_RwLock4();
	PThr4wTest_RwLock4t();
	PThr4wTest_RwLock5();
	PThr4wTest_RwLock5t();
	PThr4wTest_RwLock6();
	PThr4wTest_RwLock6t();
	PThr4wTest_RwLock6t2();
	PThr4wTest_RwLock7();
	PThr4wTest_RwLock71();
	PThr4wTest_RwLock8();
	PThr4wTest_RwLock81();
	PThr4wTest_CondVar1();
	PThr4wTest_CondVar11();
	PThr4wTest_CondVar12();
	PThr4wTest_CondVar2();
	PThr4wTest_CondVar21();
	PThr4wTest_CondVar3();
	PThr4wTest_CondVar31();
	PThr4wTest_CondVar32();
	PThr4wTest_CondVar33();
	PThr4wTest_CondVar4();
	PThr4wTest_CondVar5();
	PThr4wTest_CondVar6();
	PThr4wTest_CondVar7();
	PThr4wTest_CondVar8();
	PThr4wTest_CondVar9();
	PThr4wTest_Spin1();
	PThr4wTest_Spin2();
	PThr4wTest_Spin3();
	PThr4wTest_Spin4();
	PThr4wTest_CleanUp0();
	PThr4wTest_CleanUp1();
	PThr4wTest_CleanUp2();
	PThr4wTest_CleanUp3();
	PThr4wTest_Equal0();
	PThr4wTest_Equal1();
	PThr4wTest_Errno0();
	PThr4wTest_Errno1();
	PThr4wTest_Exception1();
	/*{
		char * pp_argv[] = { "PThr4wTest_Exception2", "dummy" };
		PThr4wTest_Exception2(SIZEOFARRAY(pp_argv), pp_argv);
	}*/
	// (trouble - unhandled exception) PThr4wTest_Exception3();
	// (trouble - unhandled exception) PThr4wTest_Exception30();
	PThr4wTest_Inherit1();
	PThr4wTest_NameNp1();
	PThr4wTest_NameNp2();
	PThr4wTest_Context1();
	PThr4wTest_Context2();
	PThr4wTest_Count1();

	PThr4wTest_Priority1();
	PThr4wTest_Priority2();
	// (trouble - unhandled exception) PThr4wTest_Reinit1();
	// (fail) PThr4wTest_Reuse1();
	PThr4wTest_Reuse2();
	PThr4wTest_Self1();
	PThr4wTest_Self2();

	PThr4wTest_Cancel1();
	PThr4wTest_Cancel2();
	PThr4wTest_Cancel3();
	PThr4wTest_Cancel4();
	PThr4wTest_Cancel5();
	PThr4wTest_Cancel6a();
	PThr4wTest_Cancel6d();
	// (fail) PThr4wTest_Cancel7();
	// (fail) PThr4wTest_Cancel8();
	PThr4wTest_Cancel9();
	// PThr4wTest_Detach1(); // fail
	// (trouble - exit) PThr4wTest_Exit1();
	PThr4wTest_Exit2();
	PThr4wTest_Exit3();
	PThr4wTest_Exit4();
	// (fail) PThr4wTest_Exit5();
	PThr4wTest_Exit6();
	PThr4wTest_Benchtest1();
	PThr4wTest_Benchtest2();
	PThr4wTest_Benchtest3();
	PThr4wTest_Benchtest4();
	PThr4wTest_Benchtest5();
	return 0;
}
