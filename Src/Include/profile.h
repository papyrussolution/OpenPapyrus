// PROFILE.H
// Copyright (c) A.Sobolev 2002, 2005, 2006, 2007
//
#ifndef __PROFILE_H
#define __PROFILE_H

#include <pp.h>
#include <time.h>
//
//
//
struct ProfileEntry;

class Profile : public SArray {
public:
	SLAPI  Profile();
	SLAPI ~Profile();
	int    SLAPI Output(uint fileId, const char * pDescription);
	int    SLAPI Start(const char * pFileName, long lineNum, const char * pAddedInfo = 0);
	int    SLAPI Finish(const char * pFileName, long lineNum);
private:
	virtual void SLAPI freeItem(void *);
	ProfileEntry & SLAPI at(uint);
	int    SLAPI Search(const char * pFileName, long lineNum, uint * p);
	int    SLAPI Insert(ProfileEntry *, uint * p);
	int    SLAPI AddEntry(const char * pFileName, long lineNum, int iterOp, const char * pAddedInfo = 0);

	int64  StartClock; // время в промежутках по 100 нс начиная с полуночи 01/01/1601 GMT
	int64  EndClock;   // время в промежутках по 100 нс начиная с полуночи 01/01/1601 GMT
};

extern Profile * P_Profiler;

#if SL_PROFILE // {

#define PROFILE_INIT delete P_Profiler; P_Profiler = new Profile;

#define PROFILE(line) \
	{P_Profiler->Start(__FILE__, __LINE__); \
	line;             \
	P_Profiler->Finish(__FILE__, __LINE__);}
#define PROFILE_S(line,s) \
	{P_Profiler->Start(__FILE__, __LINE__, s); \
	line;             \
	P_Profiler->Finish(__FILE__, __LINE__);}

#define PROFILE_START      {long ln_num = __LINE__; P_Profiler->Start(__FILE__, ln_num);
#define PROFILE_START_S(s) {long ln_num = __LINE__; P_Profiler->Start(__FILE__, ln_num, s);
#define PROFILE_END        P_Profiler->Finish(__FILE__, ln_num);}
#define PROFILE_IF(ln)     (P_Profiler->Start(__FILE__,__LINE__) && ((ln) + P_Profiler->Finish(__FILE__,__LINE__)-1 > 0))
#define PROFILE_IF_S(ln,s) (P_Profiler->Start(__FILE__,__LINE__,s) && ((ln) + P_Profiler->Finish(__FILE__,__LINE__)-1 > 0))

#define PROFILE_REPORT(description) P_Profiler->Output(0, description)

#else

#define PROFILE_INIT
#define PROFILE(line) line
#define PROFILE_S(line,s) line
#define PROFILE_START
#define PROFILE_START_S(s)
#define PROFILE_END
#define PROFILE_IF(line) line
#define PROFILE_IF_S(line,s) line
#define PROFILE_REPORT(description)

#endif // } SL_PROFILE

#endif /* __PROFILE_H */
