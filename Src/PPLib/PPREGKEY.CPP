// PPREGKEY.CPP
// Copyright (c) A.Sobolev 2002, 2003, 2005, 2007, 2008, 2009, 2010, 2011, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
// @v9.6.3 #include <idea.h>

//#define REGKEY_DRIVER
//#define REGKEY_GENMATRIX

#define MASK_MATRIX_SIZE 32

static const uint16 MaskMatrix[MASK_MATRIX_SIZE] = {
	0xF69B, 0x23F3, 0xFA07, 0x69D4, 0x7179, 0xEC0A, 0x527A, 0xE51C,
	0xA4AD, 0x2804, 0x7B94, 0x0C12, 0x9D0C, 0x3489, 0x0756, 0x1F57,
	0x24E5, 0x7D20, 0x4832, 0x016A, 0x2D4F, 0xE112, 0x0AA0, 0x0BBF,
	0xD396, 0x698B, 0x5B2D, 0x64E8, 0x7D79, 0x591A, 0xE17E, 0xF1D1
};

int SLAPI Helper_CalcRegisterKey(size_t mi, const char * pName, char * pBuf)
{
	char   buf[64];
	size_t i = 0, l = strlen(pName);
	if(l < MASK_MATRIX_SIZE) {
		i = MASK_MATRIX_SIZE-l;
		for(size_t j = 0; j < i; j++)
			buf[j] = (char)(j+1);
	}
	else {
		i = 0;
		l = MASK_MATRIX_SIZE;
	}
	memcpy(buf+i, pName, l);
	CRC32  _crc32;
	ulong  crc = _crc32.Calc(0, (uint8*)buf, MASK_MATRIX_SIZE);
	char   target[32];
	uint   target_len = 0;
	uint   ci = 0;
	for(uint offs = 0; target_len < 12; offs++)
		for(i = 0; target_len < 12 && i < 16; i++)
			if(MaskMatrix[mi] & (1 << i)) {
				char c = (MaskMatrix[offs%MASK_MATRIX_SIZE+i] ^
					(uint16)buf[offs%MASK_MATRIX_SIZE+i] ^
					(uint16)((char*)&crc)[ci % 4]) % 36;
				ci++;
				if(c < 10)
					target[target_len++] = '0' + c;
				else
					target[target_len++] = 'A' + (c - 10);
			}
	target[target_len] = 0;
	strcpy(pBuf, target);
	return 1;
}

int SLAPI PPCalcRegisterKey(const char * pName, char * pBuf)
{
	char   buf[64];
	IdeaRandMem(buf, sizeof(buf));
	size_t mi = (size_t)(*(ulong*)(buf+14) % MASK_MATRIX_SIZE); // @v6.1.0 (long*)-->(ulong*)
	return Helper_CalcRegisterKey(mi, pName, pBuf);
}

int SLAPI PPCheckRegisterKey(const char * pName, const char * pRegKey)
{
	for(size_t i = 0; i < MASK_MATRIX_SIZE; i++) {
		char   buf[32];
		Helper_CalcRegisterKey(i, pName, buf);
		if(strcmp(pRegKey, buf) == 0)
			return 1;
		else
			memzero(buf, sizeof(buf));
	}
	return 0;
}
//
//
//
SLAPI PPLicData::PPLicData()
{
    THISZERO();
}
//
//
//
static void SLAPI GetRegDataSign(char * pSign)
{
	// "-{+7g./0a"
	pSign[7] = '0';
	pSign[1] = '{';
	pSign[3] = '7';
	pSign[8] = 'a';
	pSign[4] = 'g';
	pSign[9] = '\0';
	pSign[0] = '-';
	pSign[5] = '.';
	pSign[2] = '+';
	pSign[6] = '/';
}

int SLAPI PPWriteLicData(const char * pFileName, const PPLicData * pData)
{
	int    ok = 1;
	const  size_t file_size = 4096;
	uint32 offs;
	char   sign[16];
	char * p_image = 0;
	PPLicData data = *pData;
	FILE * f = fopen(pFileName, "wb");
	THROW_PP(f, PPERR_REGFILEWRITEFAULT);
	THROW_MEM(p_image = (char *)SAlloc::M(file_size));
	IdeaRandMem(p_image, file_size);
	offs = (uint32)labs((*(uint32 *)(p_image + 19)) % 3071L) + 37;
	GetRegDataSign(sign);
	memcpy(p_image, sign, 9);
	memcpy(p_image+11, &offs, sizeof(offs));
	memcpy(p_image+(size_t)offs, &data, sizeof(data));
	IdeaEncrypt(sign, p_image, file_size);
	THROW_PP(fwrite(p_image, file_size, 1, f), PPERR_REGFILEWRITEFAULT);
	CATCHZOK
	SFile::ZClose(&f);
	SAlloc::F(p_image);
	memset(sign, ' ', sizeof(sign));
	memset(&data, ' ', sizeof(data));
	return ok;
}

#ifdef PPMKREG // {

int FASTCALL PPSetError(int errCode) { PPErrCode = errCode; return 0; }
int PPSetErrorNoMem() { return PPSetError(PPERR_NOMEM); }
int PPSetErrorSLib() { return PPSetError(PPERR_SLIB); }

#endif // } PPMKREG

int SLAPI PPReadLicData(const char * pFileName, PPLicData * pData)
{
	int    ok = 1;
	size_t file_size = 0;
	uint32 offs = 0;
	char   sign[16];
	char * p_image = 0;

	PPLicData data;
	FILE * f = fopen(pFileName, "rb");
	THROW_PP(f, PPERR_REGFILEREADFAULT);
	fseek(f, 0L, SEEK_END);
	file_size = (size_t)ftell(f);
	rewind(f);
	THROW_MEM(p_image = (char *)SAlloc::M(file_size));
	THROW_PP(fread(p_image, file_size, 1, f), PPERR_REGFILEREADFAULT);
	GetRegDataSign(sign);
	IdeaDecrypt(sign, p_image, file_size);
	THROW_PP(memcmp(p_image, sign, 9) == 0, PPERR_REGFILEREADFAULT);
	offs = *(uint32 *)(p_image+11);
	memcpy(&data, p_image+(size_t)offs, sizeof(data));
	*pData = data;
	CATCHZOK
	SFile::ZClose(&f);
	if(p_image) {
		memset(p_image, ' ', file_size);
		SAlloc::F(p_image);
	}
	memset(sign, ' ', sizeof(sign));
	memset(&data, ' ', sizeof(data));
	return ok;
}

#ifndef PPMKREG // {

int SLAPI PPGetLicData(PPLicData * pData)
{
	int    ok = -1;
	SString path;
	PPGetFilePath(PPPATH_BIN, PPFILNAM_REGDATA, path);
	if(fileExists(path)) {
		ok = PPReadLicData(path, pData);
		if(ok > 0 && pData->Transport)
			ok = (PPErrCode = PPERR_TRANSPORTREGFILE, -1);
	}
	else {
		memzero(pData, sizeof(PPLicData));
		ok = (PPErrCode = PPERR_PGMNOTREGISTERED, -1);
	}
	if(ok > 0) {
		int32 used_lic  = 0;
		DS.GetUsedLicCount(&used_lic);
		pData->FreeLicCount = (pData->LicCount > used_lic) ? pData->LicCount - used_lic : 0;
	}
	return ok;
}

int SLAPI PPUpdateLic(const char * pSrcFile, const char * pRegName, const char * pRegNum)
{
	int    ok = 1;
	SString path;
	PPLicData data;
	PPLicData prev;
	int    is_prev = 0;
	THROW_PP(PPCheckRegisterKey(pRegName, pRegNum), PPERR_INVREGKEY);
	if(pSrcFile) {
		PPSetAddedMsgString(pSrcFile);
		THROW(PPReadLicData(pSrcFile, &data));
		THROW_PP(data.Transport && strcmp(pRegName, data.RegName) == 0 &&
			strcmp(pRegNum, data.RegNumber) == 0, PPERR_INVLICFILE);
	}
	else {
		STRNSCPY(data.RegName, pRegName);
		STRNSCPY(data.RegNumber, pRegNum);
		data.LicCount = 1;
	}
	data.Transport  = 0;
	data.LastUpdate = getcurdate_();
	PPGetFilePath(PPPATH_BIN, PPFILNAM_REGDATA, path);
	if(fileExists(path)) {
		THROW(PPReadLicData(path, &prev));
		is_prev = 1;
		if(data.ExtFunc & PPLicData::effRemove)
			data.ExtFunc = (prev.ExtFunc & ~data.ExtFunc);
		else
			data.ExtFunc = (prev.ExtFunc | data.ExtFunc);
	}
	data.ExtFunc &= ~PPLicData::effRemove;
	THROW(PPWriteLicData(path, &data));
	CATCHZOK
	memset(&data, ' ', sizeof(data));
	return ok;
}

int SLAPI PPUpdateLic(const char * pSrcFile)
{
	int    ok = -1;
	PPLicData data;
	if((ok = PPGetLicData(&data)) > 0)
		ok = PPUpdateLic(pSrcFile, data.RegName, data.RegNumber);
	memset(&data, ' ', sizeof(data));
	return ok;
}

#endif // } PPMKREG
//
//
//
#ifdef REGKEY_GENMATRIX

static int SLAPI GenMaskMatrix()
{
	size_t i;
	FILE * f = fopen("matrix.", "w");
	uint16 list[MASK_MATRIX_SIZE];
	IdeaRandMem(list, sizeof(list));
	for(i = 0; i < MASK_MATRIX_SIZE; i++)
		if(f)
			fprintf(f, "0x%04X, ", list[i]);
	SFile::ZClose(&f);
	return 1;
}

#endif // } REGKEY_GENMATRIX

struct PPCreateLicParam {
	uint   LicCount;
	SString RegName;
	SString RegCode;
	StringSet FuncList;
	SString OutFileName;
};

struct PPLicFuncKey {
	const char * P_Ticker;
	uint32 Flag;
};

static PPLicFuncKey LfKeys[] = {
	{ "JobServer",  PPLicData::effJobServer },
	{ "DebtorStat", PPLicData::effDebtorStat },
	{ "Remove",     PPLicData::effRemove },
	{ "EGAIS",      PPLicData::effEgais } // @v8.8.2
};

int SLAPI PPCreateLic(PPCreateLicParam * pParam)
{
	int    ok = 1;
	int    is_new = 0;
	uint32 func_flags = 0;
	SString func_ticker;
	pParam->RegName.Strip();
	pParam->RegCode.Strip();
	pParam->OutFileName.Strip();
	THROW_PP(pParam->RegName.NotEmpty(), PPERR_UNDEFLICREGNAME);
	if(pParam->RegCode.Len() == 1 && pParam->RegCode.C(0) == '?') {
		is_new = 1;
		pParam->RegCode = 0;
	}
	THROW_PP(pParam->LicCount <= 200, PPERR_INVLICCOUNT);
	if(is_new) {
		char temp_buf[256];
		PPCalcRegisterKey(pParam->RegName, temp_buf);
		pParam->RegCode = temp_buf;
	}
	else {
		THROW_PP(PPCheckRegisterKey(pParam->RegName, pParam->RegCode), PPERR_INVREGKEY);
	}
	for(uint pos = 0; pParam->FuncList.get(&pos, func_ticker);) {
		for(uint i = 0; i < SIZEOFARRAY(LfKeys); i++) {
			if(func_ticker.CmpNC(LfKeys[i].P_Ticker) == 0) {
				func_flags |= LfKeys[i].Flag;
				break;
			}
		}
	}
	if(pParam->OutFileName.NotEmpty()) {
		PPLicData lic;
		MEMSZERO(lic);
		pParam->RegName.CopyTo(lic.RegName, sizeof(lic.RegName));
		pParam->RegCode.CopyTo(lic.RegNumber, sizeof(lic.RegNumber));
		lic.LicCount   = pParam->LicCount;
		lic.Transport  = 1;
		lic.ExtFunc    = func_flags;
		lic.LastUpdate = getcurdate_();
		THROW(PPWriteLicData(pParam->OutFileName, &lic)); // "Error: lic file writing fault\n"
	}
	CATCHZOK
	return ok;
}

#ifdef PPMKREG

int PPErrCode;

static void PPRegKeyHelp()
{
	printf("PPMKREG Copyright (c) Petroglif 2002\n");
	printf("Usage: PPMKREG regname regnum|? lic_count [func_ticker...] [key_file_name]\n");
	printf("Usage: PPMKREG /i filename\n");
	printf("func_ticker :\n");
	for(uint i = 0; i < SIZEOFARRAY(LfKeys); i++)
		printf("\t%s\n", LfKeys[i].P_Ticker);
}

int main(int argc, char ** argv)
{
	PPCreateLicParam param;
	param.LicCount = 0;
	if(argc < 3) {
		PPRegKeyHelp();
		return -1;
	}
	else if((argv[1][0] == '/' || argv[1][0] == '-') && toupper(argv[1][1]) == 'I') {
		param.OutFileName = argv[2];
		if(fileExists(param.OutFileName)) {
			PPLicData ld;
			if(PPReadLicData(param.OutFileName, &ld)) {
				printf("REGNAME: %s\nREGNUM: %s\nLicCount: %ld\nTransport: %d\n", ld.RegName, ld.RegNumber, ld.LicCount, ld.Transport);
				for(uint i = 0; i < SIZEOFARRAY(LfKeys); i++) {
					if(ld.ExtFunc & LfKeys[i].Flag) {
						printf("\t%s\n", LfKeys[i].P_Ticker);
					}
				}
				return 0;
			}
			else {
				printf("Error reading file '%s'. May be this isn't lic file\n", (const char *)param.OutFileName);
				return -1;
			}
		}
		else {
			printf("File '%s' dosn't exists\n", (const char *)param.OutFileName);
			return -1;
		}
	}
	else if(argc < 4) {
		PPRegKeyHelp();
		return -1;
	}
	else {
		param.RegName = argv[1];
		param.RegCode = argv[2];
		param.LicCount = (uint)atoi(argv[3]);
		int    arg_idx = 4;
		if(argc > arg_idx) {
			int    is_ticker = 0;
			do {
				is_ticker = 0;
				for(uint i = 0; !is_ticker && i < SIZEOFARRAY(LfKeys); i++) {
					if(sstreqi_ascii(argv[arg_idx], LfKeys[i].P_Ticker)) {
						param.FuncList.add(LfKeys[i].P_Ticker, 0);
						is_ticker = 1;
						arg_idx++;
					}
				}
			} while(is_ticker && argc > arg_idx);
			if(argc > arg_idx)
				param.OutFileName = argv[arg_idx];
		}
		if(PPCreateLic(&param)) {
			printf("\nRegisterName=%s\nRegisterNumber=%s\nLicCount=%u\n",
				(const char *)param.RegName, (const char *)param.RegCode, param.LicCount);
			SString func_name;
			for(uint pos = 0; param.FuncList.get(&pos, func_name);)
				printf("\t%s\n", (const char *)func_name);
		}
		else {
			SString msg_buf;
			if(PPErrCode == PPERR_UNDEFLICREGNAME)
				msg_buf = "Undefined register name";
			else if(PPErrCode == PPERR_INVLICCOUNT)
				msg_buf = "Invalid license count";
			else if(PPErrCode == PPERR_INVREGKEY)
				msg_buf = "Invalid register number";
			else if(PPErrCode == PPERR_REGFILEWRITEFAULT)
				msg_buf = "Lic file writing fault";
			else if(PPErrCode == PPERR_NOMEM)
				msg_buf = "Not enough memory";
			else
				msg_buf = "Unknown";
			printf("Error: %s\n", (const char *)msg_buf);
			return -1;
		}
	}
	return 0;
}

#endif // PPMKREG

#if SLTEST_RUNNING // {

SLTEST_R(CreatePapyrusLic)
{
	int    ok = 1;
	SString out_file_name = MakeOutputFilePath("test_lic");
	SString temp_buf;
	SFile in_file(MakeInputFilePath("shortstrings"), SFile::mRead);
	SString line_buf;
	THROW(SLTEST_CHECK_NZ(in_file.IsValid()));
	while(in_file.ReadLine(line_buf) > 0) {
		line_buf.Chomp();
		PPLicData data;
		PPCreateLicParam param;
		param.RegName = line_buf;
		param.RegCode = "?";
		param.LicCount = 1;
		param.OutFileName = out_file_name;
		int  r = PPCreateLic(&param);
		(temp_buf = "\n\t").Cat(param.RegName).CatDiv(':', 1).Cat("PPCreateLic");
		if(!_check_nz(PPCreateLic(&param), temp_buf))
			ok = 0;
		else {
			temp_buf.Z().CR().Tab().Cat(param.RegName).CatDiv(':', 1).Cat("PPCheckRegisterKey");
			if(!_check_nz(PPCheckRegisterKey(param.RegName, param.RegCode), temp_buf))
				ok = 0;
			else {
				temp_buf.Z().CR().Tab().Cat(param.RegName).CatDiv(':', 1).Cat("PPReadLicData");
				if(!_check_nz(PPReadLicData(param.OutFileName, &data), temp_buf))
					ok = 0;
				else {
					temp_buf.Z().CR().Tab().Cat(param.RegName).CatDiv(':', 1).Cat("PPCheckRegisterKey 2");
					if(!_check_nz(PPCheckRegisterKey(data.RegName, data.RegNumber), temp_buf))
						ok = 0;
				}
			}
		}
		SFile::Remove(param.OutFileName);
	}
	CATCH
		ok = CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
