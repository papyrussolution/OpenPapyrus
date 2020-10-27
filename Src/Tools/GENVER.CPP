// GENVER.CPP
// Copyright (c) A.Sobolev 2001, 2011, 2015, 2016
//
// For private use only. Redistribution prohibited !

#ifdef GENVERC // {
	#include <slib.h>
	#include <pp.h>
#else
	#include <pp.h>
	#pragma hdrstop
#endif
#include <idea.h>

SLAPI PapyrusPrivateBlock::PapyrusPrivateBlock()
{
	MEMSZERO(Signature);
	StructVer = 1;
	StartDevYear = 0;
	LastDevYear = 0;
	Ver.Set(0, 0, 0);
	MinVer.Set(0, 0, 0);
	Demo = 0;
	AssemblyN = 0;
}

int SLAPI PapyrusPrivateBlock::RestoreBuffer(const void * pSrcData, size_t srcDataSize)
{
	int    ok = 1;
	const  size_t sign_size = sizeof(Signature);
	THROW(pSrcData && srcDataSize > sign_size);
	{
		STempBuffer tbuf(srcDataSize);
		THROW(tbuf.IsValid());
		memcpy(tbuf, pSrcData, srcDataSize);
		memcpy(Signature, tbuf, sign_size);
		IdeaDecrypt(Signature, tbuf + sign_size, tbuf.GetSize() - sign_size);
		{
			SBuffer sbuff;
			SSerializeContext sctx;
			THROW(sbuff.Write(tbuf, tbuf.GetSize()));
			THROW(Serialize(-1, sbuff, &sctx));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PapyrusPrivateBlock::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->SerializeBlock(dir, sizeof(Signature), Signature, rBuf, 0));
	THROW(pSCtx->Serialize(dir, StructVer, rBuf));
	THROW(pSCtx->Serialize(dir, StartDevYear, rBuf));
	THROW(pSCtx->Serialize(dir, LastDevYear, rBuf));
	THROW(Ver.Serialize(dir, rBuf, pSCtx));
	THROW(MinVer.Serialize(dir, rBuf, pSCtx));
	THROW(pSCtx->Serialize(dir, AssemblyN, rBuf));
	THROW(pSCtx->Serialize(dir, Demo, rBuf));
	THROW(pSCtx->Serialize(dir, ProductName, rBuf));
	THROW(pSCtx->Serialize(dir, Team, rBuf));
	THROW(pSCtx->Serialize(dir, Secret, rBuf));
	THROW(pSCtx->Serialize(dir, DefPassword, rBuf));
	THROW(pSCtx->Serialize(dir, MsftTranslAcc, rBuf));
	THROW(pSCtx->Serialize(dir, UhttAcc, rBuf));
	CATCHZOK
	return ok;
}

#ifdef GENVERC // {

//int PPErrCode;

/*

[version]
Signature    = 46V9FLAG19CZ5GPF
Product      = Project Papyrus
Team         = Andrey Fokin, Anton Sobolev
Secret       = any_secret_word
StartDevYear = 1996
LastDevYear  = 2001
MajorVer     = 3
MinorVer     = 2
Revision     = 7
Demo         = 0
MinMajorVer  = 3
MinMinorVer  = 2
MinRevision  = 0

CCP = "xxxxx"
UHTT = "xxxx:xxxxx"
MSTR = "xxxx:xxxx"
*/

/* @v5.1.6
Pchar SLAPI replaceExt(Pchar fileName, PCchar newExt, int force)
{
	char drv[MAXDRIVE], dir[MAXPATH], nam[MAXFILE], ext[MAXEXT];
	fnsplit(fileName, drv, dir, nam, ext);
	if(force || *ext == 0) {
		if(newExt[0] != '.') {
			ext[0] = '.';
			strnzcpy(ext+1, newExt, sizeof(ext)-1);
		}
		else
			strnzcpy(ext, newExt, sizeof(ext));
		fnmerge(fileName, drv, dir, nam, ext);
	}
	return fileName;
}
*/

int SLAPI CreateVerRcFile(char * pInFileName, char * pRcFileName)
{
	FILE * pF = fopen(pRcFileName, "w");
	char product[64];
	char team[64];
	int  start = 0;
	int  last = 0;
	int  maj = 0;
	int  min = 0;
	int  rev = 0;
	int  lnk = 0;

	start = GetPrivateProfileInt("Version", "StartDevYear", start, pInFileName);
	lnk = GetLastError();
	last = GetPrivateProfileInt("Version", "LastDevYear", last, pInFileName);
	maj = GetPrivateProfileInt("Version", "MajorVer", maj, pInFileName);
	min = GetPrivateProfileInt("Version", "MinorVer", min, pInFileName);
	rev = GetPrivateProfileInt("Version", "Revision", rev, pInFileName);
	lnk = GetPrivateProfileInt("Version", "AssemblyVer", lnk, pInFileName);
	lnk++;
	sprintf(team, "%d", lnk);
	WritePrivateProfileString("Version", "AssemblyVer", team, pInFileName);

	GetPrivateProfileString("Version", "Product", "Project Papyrus", product, sizeof(product), pInFileName);
	GetPrivateProfileString("Version", "Team", "Andrey Fokin, Anton Sobolev", team, sizeof(team), pInFileName);

	fprintf(pF, "#include <afxres.h>\n");
	fprintf(pF, "\nVS_VERSION_INFO VERSIONINFO\n");
	fprintf(pF, "FILEVERSION %d,%d,%d,%d\n", maj, min, rev, lnk);
	fprintf(pF, "PRODUCTVERSION %d,%d,%d,%d\n", maj, min, rev, lnk);
 	fprintf(pF, "FILEFLAGSMASK 0x3fL\n");
	fprintf(pF, "#ifdef _DEBUG\n");
	fprintf(pF, "FILEFLAGS 0x1L\n");
	fprintf(pF, "#else\n");
 	fprintf(pF, "FILEFLAGS 0x0L\n");
	fprintf(pF, "#endif\n");
	fprintf(pF, "FILEOS 0x40004L\n");
 	fprintf(pF, "FILETYPE 0x1L\n");
 	fprintf(pF, "FILESUBTYPE 0x0L\n");
	fprintf(pF, "BEGIN\n");
	fprintf(pF, "\tBLOCK \"StringFileInfo\"\n");
	fprintf(pF, "\tBEGIN\n");
	fprintf(pF, "\t\tBLOCK \"041904e3\"\n");
	fprintf(pF, "\t\tBEGIN\n");
	fprintf(pF, "\t\t\tVALUE \"Comments\", \"Система управления предприятием\"\n");
	fprintf(pF, "\t\t\tVALUE \"CompanyName\", \"Petroglif\"\n");
	fprintf(pF, "\t\t\tVALUE \"FileDescription\", \"%s\"\n", product);
	fprintf(pF, "\t\t\tVALUE \"FileVersion\", \"%d.%d.%d\"\n", maj, min, rev);
	fprintf(pF, "\t\t\tVALUE \"InternalName\", \"pp\"\n");
	uchar ch = 169;
	fprintf(pF, "\t\t\tVALUE \"LegalCopyright\", \"Copyright %c %s %d - %d\"\n", ch, team, start, last);
	fprintf(pF, "\t\t\tVALUE \"OriginalFilename\", \"ppw.exe\"\n");
	fprintf(pF, "\t\t\tVALUE \"ProductName\", \"Papyrus\"\n");
	fprintf(pF, "\t\t\tVALUE \"ProductVersion\", \"%d.%d.%d\"\n", maj, min, rev);
	fprintf(pF, "\t\tEND\n");
	fprintf(pF, "\tEND\n");
	fprintf(pF, "\tBLOCK \"VarFileInfo\"\n");
	fprintf(pF, "\tBEGIN\n");
	fprintf(pF, "\t\tVALUE \"Translation\", 0x419, 1251\n");
	fprintf(pF, "\tEND\n");
	fprintf(pF, "END\n");
	fclose(pF);
	return 1;
}

int SLAPI CreateVerSrcFile(char * pInFileName, char * pOutFileName)
{
//	PPIniFile ini_file(pInFileName);
	FILE * p_out_file = fopen(pOutFileName, "w");

	char signature[32];
	char product[64];
	char team[64];
	char secret[32];
	char demo[16];
	int  start_dev_year = 0;
	int  last_dev_year = 0;
	int  major_ver = 0;
	int  minor_ver = 0;
	int  revision = 0;
	int  is_demo = 0;
	int  min_major_ver = 0;
	int  min_minor_ver = 0;
	int  min_revision  = 0;

	uint8 * p_data_buf = (uint8*)malloc(1024);
	size_t data_size = 0;
	size_t sign_size = 0;

	memzero(signature, sizeof(signature));
	memzero(product, sizeof(product));
	memzero(team, sizeof(team));
	memzero(secret, sizeof(secret));
	memzero(demo, sizeof(demo));

	GetPrivateProfileString("Version", "Signature", "46V9FLAG19CZ5GPF", signature, sizeof(signature), pInFileName);
	GetPrivateProfileString("Version", "Product", "Project Papyrus", product, sizeof(product), pInFileName);
	GetPrivateProfileString("Version", "Team", "Andrey Fokin, Anton Sobolev", team, sizeof(team), pInFileName);
	GetPrivateProfileString("Version", "Secret", "jumbo07scale", secret, sizeof(secret), pInFileName);

	start_dev_year = GetPrivateProfileInt("Version", "StartDevYear", start_dev_year, pInFileName);
	last_dev_year = GetPrivateProfileInt("Version", "LastDevYear", last_dev_year, pInFileName);
	major_ver = GetPrivateProfileInt("Version", "MajorVer", major_ver, pInFileName);
	minor_ver = GetPrivateProfileInt("Version", "MinorVer", minor_ver, pInFileName);
	revision = GetPrivateProfileInt("Version", "Revision", revision, pInFileName);
	is_demo = GetPrivateProfileInt("Version", "Demo", is_demo, pInFileName);
	min_major_ver = GetPrivateProfileInt("Version", "MinMajorVer", min_major_ver, pInFileName);
	min_minor_ver = GetPrivateProfileInt("Version", "MinMinorVer", min_minor_ver, pInFileName);
	min_revision  = GetPrivateProfileInt("Version", "MinRevision",  min_revision,  pInFileName);

	uint16 tmp_int;
	uint16 struc_size = 0;
	size_t sz;

	sign_size = sz = strlen(signature)+1;
	memcpy(p_data_buf+data_size, signature, sz);
	data_size += sz;

	data_size += sizeof(struc_size);

	sz = strlen(product)+1;
	memcpy(p_data_buf+data_size, product, sz);
	data_size += sz;

	sz = strlen(team)+1;
	memcpy(p_data_buf+data_size, team, sz);
	data_size += sz;

	if(is_demo)
		strcpy(demo, "DEMO");
	else
		memzero(demo, sizeof(demo));
	sz = strlen(demo)+1;
	memcpy(p_data_buf+data_size, demo, sz);
	data_size += sz;

	sz = strlen(secret)+1;
	memcpy(p_data_buf+data_size, secret, sz);
	data_size += sz;

	sz = sizeof(tmp_int);
	tmp_int = start_dev_year;
	memcpy(p_data_buf+data_size, &tmp_int, sz);
	data_size += sz;

	sz = sizeof(tmp_int);
	tmp_int = last_dev_year;
	memcpy(p_data_buf+data_size, &tmp_int, sz);
	data_size += sz;

	sz = sizeof(tmp_int);
	tmp_int = major_ver;
	memcpy(p_data_buf+data_size, &tmp_int, sz);
	data_size += sz;

	sz = sizeof(tmp_int);
	tmp_int = minor_ver;
	memcpy(p_data_buf+data_size, &tmp_int, sz);
	data_size += sz;

	sz = sizeof(tmp_int);
	tmp_int = revision;
	memcpy(p_data_buf+data_size, &tmp_int, sz);
	data_size += sz;

	struc_size = data_size;
	memcpy(p_data_buf+sign_size, &struc_size, sizeof(struc_size));
	sign_size += sizeof(struc_size);

	// @v5.3.3 AHTOXA {
	sz = sizeof(tmp_int);
	tmp_int = min_major_ver;
	memcpy(p_data_buf+data_size, &tmp_int, sz);
	data_size += sz;

	sz = sizeof(tmp_int);
	tmp_int = min_minor_ver;
	memcpy(p_data_buf+data_size, &tmp_int, sz);
	data_size += sz;

	sz = sizeof(tmp_int);
	tmp_int = min_revision;
	memcpy(p_data_buf+data_size, &tmp_int, sz);
	data_size += sz;
	// } @v5.3.3 AHTOXA

	IdeaEncrypt(signature, p_data_buf+sign_size, data_size-sign_size);

	fprintf(p_out_file, "/*\n\t%s\n\tAutomaticaly generated file\n\tDon't modify !\n*/\n", pOutFileName);
	//fprintf(p_out_file, "static unsigned char G_PPVerInfo[] = {\n\t");
	for(uint i = 0; i < data_size; i++) {
		fprintf(p_out_file, "0x%02x, ", p_data_buf[i]);
		if(((i+1) % 8) == 0)
			fprintf(p_out_file, "\n");
	}
	//fprintf(p_out_file, "\n};\n\n");
	//fprintf(p_out_file, "static const size_t G_PPVerInfoSize = %uU;\n", data_size);

	fclose(p_out_file);
	return 1;
}

int SLAPI PapyrusPrivateBlock::ReadFromIni(SIniFile & rIniFile)
{
	int    ok = 1;
	SString temp_buf;
	//rIniFile.GetParam("Version", "Signature",
	rIniFile.GetParam("Version", "Signature", temp_buf);
	IdeaRandMem(Signature, sizeof(Signature));
	STRNSCPY(Signature, temp_buf);
	rIniFile.GetParam("Version", "Product", ProductName);
	rIniFile.GetParam("Version", "Team", Team);
	rIniFile.GetParam("Version", "Secret", Secret);
	rIniFile.GetParam("Version", "DefPassword", DefPassword);
	rIniFile.GetParam("Version", "MsftTranslAcc", MsftTranslAcc);
	rIniFile.GetParam("Version", "UhttAcc", UhttAcc);
	{
		int    mj = 0, mn = 0, r = 0, asmn = 0;
		rIniFile.GetIntParam("Version", "MajorVer", &mj);
		rIniFile.GetIntParam("Version", "MinorVer", &mn);
		rIniFile.GetIntParam("Version", "Revision", &r);
		Ver.Set(mj, mn, r);
		rIniFile.GetIntParam("Version", "AssemblyVer", &asmn);
		AssemblyN = asmn;
	}
	{
		int    min_mj = 0, min_mn = 0, min_r = 0;
		rIniFile.GetIntParam("Version", "MinMajorVer", &min_mj);
		rIniFile.GetIntParam("Version", "MinMinorVer", &min_mn);
		rIniFile.GetIntParam("Version", "MinRevision", &min_r);
		MinVer.Set(min_mj, min_mn, min_r);
	}
	{
		int    d = 0;
		rIniFile.GetIntParam("Version", "Demo", &d);
		Demo = d;
	}
	{
		int    start_dev_year = 0, last_dev_year = 0;
		rIniFile.GetIntParam("Version", "StartDevYear", &start_dev_year);
		rIniFile.GetIntParam("Version", "LastDevYear", &last_dev_year);
		StartDevYear = start_dev_year;
		LastDevYear = last_dev_year;
	}
	return ok;
}

int SLAPI PapyrusPrivateBlock::WriteToSrc(const char * pFileName)
{
	int    ok = 1;
	SBuffer sbuff;
	SSerializeContext sctx;
	THROW(Serialize(+1, sbuff, &sctx));
	{
		const size_t src_size = sbuff.GetAvailableSize();
		const size_t sign_size = sizeof(Signature);
		STempBuffer tbuf(ALIGNSIZE(src_size, 7));
		THROW(tbuf.IsValid());
		IdeaRandMem(tbuf, tbuf.GetSize());
		memcpy(tbuf, sbuff.GetBuf(sbuff.GetRdOffs()), src_size);
		THROW(strlen(Signature) < sign_size);
		THROW(sstreq(Signature, tbuf));
		IdeaEncrypt(Signature, tbuf + sign_size, tbuf.GetSize() - sign_size);
		{
			SString temp_buf;
			Generator_CPP gencpp(pFileName);
			THROW(gencpp.IsValid());
			gencpp.Wr_Comment(0);
			gencpp.Wr_Comment((temp_buf = 0).Tab().Cat(pFileName));
			gencpp.Wr_Comment((temp_buf = 0).Tab().Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, 0));
			gencpp.Wr_Comment((temp_buf = 0).Tab().Cat("Automaticaly generated file"));
			gencpp.Wr_Comment((temp_buf = 0).Tab().Cat("Don't modify!"));
			gencpp.Wr_Comment(0);
			gencpp.IndentInc();
			//fprintf(p_out_file, "/*\n\t%s\n\tAutomaticaly generated file\n\tDon't modify !\n*/\n", pOutFileName);
			//fprintf(p_out_file, "static unsigned char G_PPVerInfo[] = {\n\t");
			assert((tbuf.GetSize() % 4) == 0);
			temp_buf = 0;
			for(size_t i = 0; i < tbuf.GetSize(); i++) {
				temp_buf.Cat("0x").CatHex(tbuf.ucptr()[i]);
				//fprintf(p_out_file, "0x%02x, ", p_data_buf[i]);
				if(i != (tbuf.GetSize()-1))
					temp_buf.CatDiv(',', 2);
				if(((i+1) % 32) == 0) {
					temp_buf.CR();
					//fprintf(p_out_file, "\n");
				}
			}
			gencpp.WriteLine(temp_buf);
			gencpp.IndentDec();
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI CreateVerSrcFile2(const char * pInFileName, const char * pOutFileName)
{
	int    ok = 1;
	//SIniFile ini_file(pInFileName);
	//if(ini_file.)
	PapyrusPrivateBlock ppb;
	SIniFile ini_file(pInFileName);
	THROW(ini_file.IsValid());
	THROW(ppb.ReadFromIni(ini_file));
	THROW(ppb.WriteToSrc(pOutFileName));
	CATCHZOK
	return ok;
}

int SLAPI CreateVerRcFile2(const char * pInFileName, const char * pRcFileName)
{
	int    ok = 1;
	SString temp_buf;
	SString line_buf;
	PapyrusPrivateBlock ppb;
	int    assembly_n = 0;
	{
		SIniFile ini_file(pInFileName);
		THROW(ini_file.IsValid());
		THROW(ppb.ReadFromIni(ini_file));
	}
	{
		assembly_n = ppb.AssemblyN+1;
		(temp_buf = 0).Cat(assembly_n);
		WritePrivateProfileString("Version", "AssemblyVer", temp_buf, pInFileName);
	}
	{
		int    maj, min, rev;
		SFile f_out(pRcFileName, SFile::mWrite);
		THROW(f_out.IsValid());
		ppb.Ver.Get(&maj, &min, &rev);
        (line_buf = 0).Cat("#include <afxres.h>").CR().CR();
        line_buf.Cat("VS_VERSION_INFO").Space().Cat("VERSIONINFO").CR();
        line_buf.Cat("FILEVERSION").Space().Cat(maj).CatChar(',').Cat(min).CatChar(',').Cat(rev).CatChar(',').Cat(assembly_n).CR();
        line_buf.Cat("PRODUCTVERSION").Space().Cat(maj).CatChar(',').Cat(min).CatChar(',').Cat(rev).CatChar(',').Cat(assembly_n).CR();
        line_buf.Cat("FILEFLAGSMASK").Space().Cat("0x3fL").CR();
        line_buf.Cat("#ifdef _DEBUG").CR();
        line_buf.Tab().Cat("FILEFLAGS").Space().Cat("0x1L").CR();
        line_buf.Cat("#else").CR();
        line_buf.Tab().Cat("FILEFLAGS").Space().Cat("0x0L").CR();
        line_buf.Cat("#endif").CR();
        line_buf.Cat("FILEOS").Space().Cat("0x40004L").CR();
        line_buf.Cat("FILETYPE").Space().Cat("0x1L").CR();
        line_buf.Cat("FILESUBTYPE").Space().Cat("0x0L").CR();
        line_buf.Cat("BEGIN").CR();
			line_buf.Tab().Cat("BLOCK").Space().CatQStr("StringFileInfo").CR();
			line_buf.Tab().Cat("BEGIN").CR();
				line_buf.Tab(2).Cat("BLOCK").Space().CatQStr("041904e3").CR();
				line_buf.Tab(2).Cat("BEGIN").CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("Comments").CatDiv(',', 2).CatQStr("Система управления предприятием").CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("CompanyName").CatDiv(',', 2).CatQStr("Petroglif").CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("FileDescription").CatDiv(',', 2).CatQStr(ppb.ProductName).CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("FileVersion").CatDiv(',', 2).CatQStr(ppb.Ver.ToStr(temp_buf = 0)).CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("InternalName").CatDiv(',', 2).CatQStr("pp").CR();
					// fprintf(pF, "\t\t\tVALUE \"LegalCopyright\", \"Copyright %c %s %d - %d\"\n", ch, team, start, last);
					(temp_buf = 0).Cat("Copyright").Space().CatChar(169).Space().Cat(ppb.Team).Space().
						Cat(ppb.StartDevYear).CatDiv('-', 1).Cat(ppb.LastDevYear);
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("LegalCopyright").CatDiv(',', 2).CatQStr(temp_buf).CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("OriginalFilename").CatDiv(',', 2).CatQStr("ppw.exe").CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("ProductName").CatDiv(',', 2).CatQStr("Papyrus").CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("ProductVersion").CatDiv(',', 2).CatQStr(ppb.Ver.ToStr(temp_buf = 0)).CR();
				line_buf.Tab(2).Cat("END").CR();
				line_buf.Tab(2).Cat("BLOCK").Space().CatQStr("VarFileInfo").CR();
				line_buf.Tab(2).Cat("BEGIN").CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("Translation").CatDiv(',', 2).Cat("0x419").CatDiv(',', 2).Cat("1251").CR();
				line_buf.Tab(2).Cat("END").CR();
			line_buf.Tab().Cat("END").CR();
        line_buf.Cat("END").CR();
        f_out.WriteLine(line_buf);
	}
	CATCHZOK
	return ok;
}

int main(int argc, char * argv[])
{
	char in_file_name[MAXPATH];
	char out_file_name[MAXPATH];
	char rc_file_name[MAXPATH];
	if(argc < 2) {
		printf("genver - helper utility for Project Papyrus\n");
		printf("Copyright (c) A.Sobolev, 2001\n");
		printf("Usage: genver input_file_name[.ini] [output_file_name[.c] [rc_file_name[.rc]]]\n");
		return -1;
	}
	else {
		if(argc >= 2) {
			strip(strcpy(in_file_name, argv[1]));
			replaceExt(in_file_name, "ini", 0);
		}
		strcpy(out_file_name, "stdout");
		strcpy(rc_file_name, "stdout");
		if(argc >= 3) {
			strip(strcpy(out_file_name, argv[2]));
			replaceExt(out_file_name, "c", 0);
		}
		if(argc >= 4) {
			strip(strcpy(rc_file_name, argv[3]));
			replaceExt(rc_file_name, "rc", 0);
		}
		CreateVerRcFile(in_file_name, rc_file_name);
		CreateVerSrcFile(in_file_name, out_file_name);
		return 0;
	}
}

#endif // } GENVERC
