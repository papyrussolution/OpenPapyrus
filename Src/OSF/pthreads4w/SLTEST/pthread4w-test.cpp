// PTHREAD4W-TEST.CPP
//
#include <sl_pthreads4w.h>
#pragma hdrstop

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

int DoTest_PThr4w()
{
	PThr4wTest_Affinity1();
	PThr4wTest_Affinity2();
	PThr4wTest_Affinity3();
	PThr4wTest_Affinity4();
	PThr4wTest_Affinity5();
	PThr4wTest_Affinity6();
	PThr4wTest_Barrier1();
	PThr4wTest_Barrier2();
	PThr4wTest_Barrier3();
	PThr4wTest_Barrier4();
	PThr4wTest_Barrier5();
	PThr4wTest_Barrier6();
	PThr4wTest_Mutex1();
	PThr4wTest_Mutex1e(); // fail!
	PThr4wTest_Mutex1n();
	PThr4wTest_Mutex1r();
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
	return 0;
}