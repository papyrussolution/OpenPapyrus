// GENVER.CPP
// Copyright (c) A.Sobolev 2001, 2011, 2015, 2016, 2017, 2018, 2020
//
// For private use only. Redistribution prohibited !

#include <pp.h>
#pragma hdrstop
// @v9.6.3 #include <idea.h>

SLAPI PapyrusPrivateBlock::PapyrusPrivateBlock()
{
	Clear();
}

SLAPI PapyrusPrivateBlock::~PapyrusPrivateBlock()
{
	Clear();
}

void SLAPI PapyrusPrivateBlock::Clear()
{
	MEMSZERO(H);
	StructVer = 2; // @v9.4.8 1-->2
	StartDevYear = 0;
	LastDevYear = 0;
	Ver.Set(0, 0, 0);
	MinVer.Set(0, 0, 0);
	Flags = 0;
	AssemblyN = 0;
	ProductName.Obfuscate();
	Team.Obfuscate();
	Secret.Obfuscate();
	DefPassword.Obfuscate();
	MsftTranslAcc.Obfuscate();
	UhttAcc.Obfuscate();
}

int SLAPI PapyrusPrivateBlock::RestoreBuffer(const void * pSrcData, size_t srcDataSize)
{
	int    ok = 1;
	const  size_t h_size = sizeof(H);
	THROW(pSrcData && srcDataSize > (h_size + sizeof(uint32)));
	{
		STempBuffer tbuf(srcDataSize);
		THROW(tbuf.IsValid());
		memcpy(tbuf, pSrcData, srcDataSize);
		memcpy(&H, tbuf + sizeof(uint32), h_size);
		THROW(srcDataSize >= H.RawBufferSize);
		IdeaDecrypt(H.Signature, tbuf + sizeof(uint32) + h_size, H.RawBufferSize - sizeof(uint32) - h_size);
		{
			SBuffer sbuff;
			SSerializeContext sctx;
			THROW(sbuff.Write(tbuf, H.RawBufferSize));
			THROW(Serialize(-1, sbuff, &sctx));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PapyrusPrivateBlock::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->SerializeBlock(dir, sizeof(H), &H, rBuf, 0));
	THROW(pSCtx->Serialize(dir, StructVer, rBuf));
	THROW(pSCtx->Serialize(dir, StartDevYear, rBuf));
	THROW(pSCtx->Serialize(dir, LastDevYear, rBuf));
	THROW(Ver.Serialize(dir, rBuf, pSCtx));
	THROW(MinVer.Serialize(dir, rBuf, pSCtx));
	THROW(pSCtx->Serialize(dir, AssemblyN, rBuf));
	// @v9.4.8 THROW(pSCtx->Serialize(dir, Demo, rBuf));
	THROW(pSCtx->Serialize(dir, Flags, rBuf)); // @v9.4.8
	THROW(pSCtx->Serialize(dir, ProductName, rBuf));
	THROW(pSCtx->Serialize(dir, Team, rBuf));
	THROW(pSCtx->Serialize(dir, Secret, rBuf));
	THROW(pSCtx->Serialize(dir, DefPassword, rBuf));
	THROW(pSCtx->Serialize(dir, MsftTranslAcc, rBuf));
	THROW(pSCtx->Serialize(dir, UhttAcc, rBuf));
	CATCHZOK
	return ok;
}

int SLAPI PapyrusPrivateBlock::ReadFromIni(SIniFile & rIniFile)
{
	int    ok = 1;
	SString temp_buf;
	//rIniFile.GetParam("Version", "Signature",
	rIniFile.GetParam("Version", "Signature", temp_buf);
	IdeaRandMem(H.Signature, sizeof(H.Signature));
	STRNSCPY(H.Signature, temp_buf);
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
		int    o = 0;
		rIniFile.GetIntParam("Version", "Demo", &d);
		rIniFile.GetIntParam("Version", "OpenSource", &o);
		SETFLAG(Flags, fDemo, d);
		SETFLAG(Flags, fOpenSource, o);
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
OpenSouruce  = 0
MinMajorVer  = 3
MinMinorVer  = 2
MinRevision  = 0

CCP = "xxxxx"
UHTT = "xxxx:xxxxx"
MSTR = "xxxx:xxxx"
*/
int SLAPI PapyrusPrivateBlock::WriteToSrc(const char * pFileName)
{
	int    ok = 1;
	SBuffer sbuff;
	SSerializeContext sctx;
	THROW(Serialize(+1, sbuff, &sctx));
	{
		const size_t src_size = sbuff.GetAvailableSize();
		const size_t h_size = sizeof(H);
		STempBuffer tbuf(ALIGNSIZE(src_size, 7));
		THROW(tbuf.IsValid());
		IdeaRandMem(tbuf, tbuf.GetSize());
		memcpy(tbuf, sbuff.GetBuf(sbuff.GetRdOffs()), src_size);
		THROW(sstrlen(H.Signature) < sizeof(H.Signature));
		THROW(sstreq(H.Signature, tbuf + sizeof(uint32))); // Буфер Signature сериализуется как сплошной участок памяти - в префиксе размер (uint32).
		PTR32(tbuf + sizeof(uint32) + sizeof(H.Signature))[0] = tbuf.GetSize();
		IdeaEncrypt(H.Signature, tbuf + sizeof(uint32) + h_size, tbuf.GetSize() - sizeof(uint32) - h_size);
		{
			SString temp_buf;
			Generator_CPP gencpp(pFileName);
			THROW(gencpp.IsValid());
			gencpp.Wr_Comment(0);
			gencpp.Wr_Comment(temp_buf.Z().Tab().Cat(pFileName));
			gencpp.Wr_Comment(temp_buf.Z().Tab().Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, 0));
			gencpp.Wr_Comment(temp_buf.Z().Tab().Cat("Automaticaly generated file"));
			gencpp.Wr_Comment(temp_buf.Z().Tab().Cat("Don't modify!"));
			gencpp.Wr_Comment(0);
			gencpp.IndentInc();
			//fprintf(p_out_file, "/*\n\t%s\n\tAutomaticaly generated file\n\tDon't modify !\n*/\n", pOutFileName);
			//fprintf(p_out_file, "static unsigned char G_PPVerInfo[] = {\n\t");
			assert((tbuf.GetSize() % 4) == 0);
			temp_buf.Z();
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

static int SLAPI CreateVerSrcFile2(const char * pInFileName, const char * pOutFileName)
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

static int SLAPI CreateVerRcFile2(const char * pInFileName, const char * pRcFileName, const char * pInFileNameOpen)
{
	int    ok = 1;
	SString temp_buf;
	SString line_buf;
	PapyrusPrivateBlock ppb;
	int    maj, min, rev;
	int    assembly_n = 0;
	{
		SIniFile ini_file(pInFileName);
		THROW(ini_file.IsValid());
		THROW(ppb.ReadFromIni(ini_file));
	}
	ppb.Ver.Get(&maj, &min, &rev);
	{
		assembly_n = ppb.AssemblyN+1;
		temp_buf.Z().Cat(assembly_n);
		WritePrivateProfileString(_T("Version"), _T("AssemblyVer"), SUcSwitch(temp_buf), SUcSwitch(pInFileName));
	}
	if(!isempty(pInFileNameOpen) && fileExists(pInFileNameOpen)) {
		int    min_maj, min_min, min_rev;
		ppb.MinVer.Get(&min_maj, &min_min, &min_rev);
		WritePrivateProfileString(_T("Version"), _T("MajorVer"), SUcSwitch(temp_buf.Z().Cat(maj)), SUcSwitch(pInFileNameOpen));
		WritePrivateProfileString(_T("Version"), _T("MinorVer"), SUcSwitch(temp_buf.Z().Cat(min)), SUcSwitch(pInFileNameOpen));
		WritePrivateProfileString(_T("Version"), _T("Revision"), SUcSwitch(temp_buf.Z().Cat(rev)), SUcSwitch(pInFileNameOpen));
		WritePrivateProfileString(_T("Version"), _T("MinMajorVer"), SUcSwitch(temp_buf.Z().Cat(min_maj)), SUcSwitch(pInFileNameOpen));
		WritePrivateProfileString(_T("Version"), _T("MinMinorVer"), SUcSwitch(temp_buf.Z().Cat(min_min)), SUcSwitch(pInFileNameOpen));
		WritePrivateProfileString(_T("Version"), _T("MinRevision"), SUcSwitch(temp_buf.Z().Cat(min_rev)), SUcSwitch(pInFileNameOpen));
		temp_buf.Z().Cat(assembly_n);
		WritePrivateProfileString(_T("Version"), _T("AssemblyVer"), SUcSwitch(temp_buf), SUcSwitch(pInFileNameOpen));
	}
	{
		SFile f_out(pRcFileName, SFile::mWrite);
		THROW(f_out.IsValid());
        line_buf.Z().Cat("#include <afxres.h>").CR().CR();
        line_buf.Cat("VS_VERSION_INFO").Space().Cat("VERSIONINFO").CR();
        line_buf.Cat("FILEVERSION").Space().Cat(maj).CatChar(',').Cat(min).CatChar(',').Cat(rev).CatChar(',').Cat(assembly_n).CR();
        line_buf.Cat("PRODUCTVERSION").Space().Cat(maj).CatChar(',').Cat(min).CatChar(',').Cat(rev).CatChar(',').Cat(assembly_n).CR();
        line_buf.Cat("FILEFLAGSMASK").Space().Cat("0x3fL").CR();
        line_buf.Cat("#ifdef _DEBUG").CR();
        line_buf.Tab().Cat("FILEFLAGS").Space().Cat("0x1L").CR();
        line_buf.Cat("#else").CR();
        line_buf.Tab().Cat("FILEFLAGS").Space().Cat("0x0L").CR();
        line_buf.Cat("#endif").CR();
        line_buf.Cat("FILEOS").Space().Cat(/*"VOS__WINDOWS32"*/"0x40004L").CR(); // @v9.4.8 0x40004L-->VOS__WINDOWS32
        line_buf.Cat("FILETYPE").Space().Cat(/*"VFT_APP"*/"0x1L").CR(); // @v9.4.8 0x1L-->VFT_APP
        line_buf.Cat("FILESUBTYPE").Space().Cat(/*"VFT2_UNKNOWN"*/"0x0L").CR(); // @v9.4.8 0x0L-->VFT2_UNKNOWN
        line_buf.Cat("BEGIN").CR();
			line_buf.Tab().Cat("BLOCK").Space().CatQStr("StringFileInfo").CR();
			line_buf.Tab().Cat("BEGIN").CR();
				line_buf.Tab(2).Cat("BLOCK").Space().CatQStr("041904e3").CR();
				line_buf.Tab(2).Cat("BEGIN").CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("Comments").CatDiv(',', 2).CatQStr("Система управления предприятием").CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("CompanyName").CatDiv(',', 2).CatQStr("Petroglif").CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("FileDescription").CatDiv(',', 2).CatQStr(ppb.ProductName).CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("FileVersion").CatDiv(',', 2).CatQStr(ppb.Ver.ToStr(temp_buf.Z())).CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("InternalName").CatDiv(',', 2).CatQStr("pp").CR();
					// fprintf(pF, "\t\t\tVALUE \"LegalCopyright\", \"Copyright %c %s %d - %d\"\n", ch, team, start, last);
					temp_buf.Z().Cat("Copyright").Space().CatChar(169).Space().Cat(ppb.Team).Space().
						Cat(ppb.StartDevYear).CatDiv('-', 1).Cat(ppb.LastDevYear);
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("LegalCopyright").CatDiv(',', 2).CatQStr(temp_buf).CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("OriginalFilename").CatDiv(',', 2).CatQStr("ppw.exe").CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("ProductName").CatDiv(',', 2).CatQStr("Papyrus").CR();
					line_buf.Tab(3).Cat("VALUE").Space().CatQStr("ProductVersion").CatDiv(',', 2).CatQStr(ppb.Ver.ToStr(temp_buf.Z())).CR();
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
	SString in_file_name;
	SString in_file_name_open; // Дублер входящего файла для OpenSource-ветки
	SString out_file_name;
	SString rc_file_name;
	SLS.Init("PPGENVER");
	if(argc < 2) {
		printf("genver - helper utility for Project Papyrus\n");
		printf("Copyright (c) A.Sobolev, 2001-2017\n");
		printf("Usage: genver input_file_name[.ini] [output_file_name[.c] [rc_file_name[.rc]]]\n");
		return -1;
	}
	else {
		if(argc >= 2) {
			(in_file_name = argv[1]).Strip();
			SPathStruc::ReplaceExt(in_file_name, "ini", 0);
			{
				SPathStruc ps(in_file_name);
				ps.Nam.Cat("-open");
				ps.Merge(in_file_name_open);
				if(!fileExists(in_file_name_open))
					in_file_name_open = 0;
			}
			if(!fileExists(in_file_name) && fileExists(in_file_name_open)) {
				in_file_name = in_file_name_open;
				in_file_name_open = 0;
			}
		}
		out_file_name = "stdout";
		rc_file_name = "stdout";
		if(argc >= 3) {
			(out_file_name = argv[2]).Strip();
			SPathStruc::ReplaceExt(out_file_name, "c", 0);
		}
		if(argc >= 4) {
			(rc_file_name = argv[3]).Strip();
			SPathStruc::ReplaceExt(rc_file_name, "rc", 0);
		}
		CreateVerRcFile2(in_file_name, rc_file_name, in_file_name_open);
		CreateVerSrcFile2(in_file_name, out_file_name);
		return 0;
	}
}

#endif // } GENVERC
