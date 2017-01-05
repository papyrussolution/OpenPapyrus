// CRSRNUM.CPP
//
#include <pp.h>
#include <idea.h>
#include <private\regnum.h>

int SLAPI CreateSerial(PPRegistrInfo * pRegInfo)
{
	int    ok = -1;
	uint   i;
	PPRegistrInfo reg_info;
	uint name_len = strlen(pRegInfo->ClientName);
	char digit[33], * serial = 0;
	ulong ulong_digit;

	if(name_len > 0) {
		THROW_MEM(serial = new char[name_len * 33]);
		memset(digit, 0, sizeof(digit));
		memset(serial, 0, sizeof(serial));
		memset(&reg_info, 0, sizeof(PPRegistrInfo));
		reg_info = *pRegInfo;
		srand((unsigned) time(NULL));
		for(i =	0; (i < sizeof(reg_info.SerialKey)) && (i < name_len); i++) {
			ulong_digit = rand() % ((ushort) reg_info.ClientName[i]);
			ultoa(ulong_digit, digit, 10);
			strcat(serial, digit);
		}
		STRNSCPY(reg_info.SerialKey, serial);
		ASSIGN_PTR(pRegInfo, reg_info);
		ok = 1;
	}
	CATCH
		ok = 0;
	ENDCATCH
	delete [] serial;
	return ok;
}
