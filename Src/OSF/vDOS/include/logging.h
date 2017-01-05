#ifndef vDOS_LOGGING_H
#define vDOS_LOGGING_H
enum LOG_TYPES {
	LOG_ALL,
	LOG_VGA, LOG_INT10,
	LOG_CPU,
	LOG_FILES,LOG_IOCTL,LOG_EXEC,LOG_DOSMISC,
	LOG_PIT,LOG_KEYBOARD,LOG_PIC,
	LOG_MOUSE,LOG_BIOS,LOG_MISC
};

enum LOG_SEVERITIES {
	LOG_NORMAL,
	LOG_WARN,
	LOG_ERROR
};

struct LOG
{
	LOG(LOG_TYPES, LOG_SEVERITIES)												{ }
	void operator()(char const*)												{ }
	void operator()(char const* , double)										{ }
	void operator()(char const* , double , double)								{ }
	void operator()(char const* , double , double , double)						{ }
	void operator()(char const* , double , double , double , double)			{ }
	void operator()(char const* , double , double , double , double , double)	{ }

	void operator()(char const* , char const*)									{ }
	void operator()(char const* , char const* , double)							{ }
	void operator()(char const* , char const* , double ,double)					{ }
	void operator()(char const* , double , char const*)							{ }
	void operator()(char const* , double , double, char const*)					{ }
	void operator()(char const* , char const*, char const*)						{ }
};	// add missing operators to here
	// try to avoid anything smaller than bit32...

void vLog(char const* format,...);

#endif
