// SCP2RC.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000-2002, 2005, 2007, 2011, 2013, 2016, 2017, 2019, 2020, 2021, 2022, 2023
// @codepage UTF-8
//
#include <slib.h>

#define __Dialog       0
#define __Button       1
#define __Static       2
#define __ColoredText  3
#define __InputLine    4
#define __Label        5
#define __History      6
#define __InputLong    7
#define __CheckBox     8
#define __RadioButton  9
#define __MultiCB     10
#define __ListBox     11
#define __Memo        12
#define __ScrollBar   13
#define __Combo       14
#define __Done        -1

#define DEF_CTL_ID "control"
/*
TInputLine
	Тип данных записывается в поле EXTRA[1] в формате:
		ТИП, ДЛИНА[, ТОЧНОСТЬ] (default S_ZSTRING, maxLen, 0)
	Формат записывается в поле EXTRA[2] в формате
		ФЛАГИ, ДЛИНА[, ТОЧНОСТЬ] (default maxLen-1, 0)
	TCalcInputLine записывается в поле EXTRA[3] в формате:
		Идентификатор (VB_XXX - max 31 символ)
	Длина InputLine +=1 для размещения TCalcInputLine
SmartListBox
	Тип данных в поле EXTRA[1]
		ТИП, ДЛИНА, ТОЧНОСТЬ
	Формат в поле EXTRA[2]
		ФЛАГИ, ДЛИНА[, ТОЧНОСТЬ]
	Опции в поле EXTRA[3]
		ФЛАГИ

	EXTRA[4]:
		Length,Flags,ColumnName;...
		Flags = L(eft) | R(ight) | C(enter)
TButton
	Идентификатор Bitmap в поле EXTRA[3]
*/

static void error(const char * s)
{
	printf("Error: %s\n", s);
	exit(-1);
}

#define fldoff(k,f)  offsetof(k##Spec, f)

static const char * P_TvNames[] = {
	"TV_DIALOG",
	"TV_BUTTON",
	"TV_STATIC",
	"TV_STATIC",
	"TV_INPUTLINE",
	"TV_LABEL",
	"TV_HISTORY",
	"TV_INPUTLINE",
	"TV_CHECKBOXES",
	"TV_RADIOBUTTONS",
	"TV_RADIOBUTTONS",
	"TV_LISTBOX",
	"TV_MEMO",
	"TV_SCROLLBAR",
	"TV_COMBO"
};

class ScpDialogProcessor {
public:
	ScpDialogProcessor(SFile & rOut) : R_Out(rOut)
	{
	}
	void   Run(const char * pFileName)
	{
		char   str[256];
		char   orgID[32];
		SString line_buf;
		SString temp_buf;
		int    kind;
		int    ctlNo = 1;
		int    vb_no = 1;
		FILE * f_log = fopen("scp2rc.log", "a");

		In.Open(pFileName, SFile::mRead);
		if(!In.IsValid()) {
			(LineBuf = "Unable open input file").Space().Cat(pFileName);
			error(LineBuf);
		}
		else {
			fscanf(In, "%s", str);
			if(!sstreq(str, "SCRIPT1"))
				error("Input file is not script file");
			fprintf(R_Out, "/*\n    Source file: %s\n*/\n", pFileName);
			GetString(str);
			GetUINT();
			while((kind = GetINT()) != -1) {
				Kind = kind;
				Fix.get(*this);
				strcpy(orgID, Fix.id);
				if(ctlNo != 0 && !oneof2(kind, __Dialog, __ScrollBar))
					if(Fix.id[0] == 0 || strncmp(Fix.id, DEF_CTL_ID, strlen(DEF_CTL_ID)) == 0)
						_itoa(/*20000*/4000+(ctlNo++), Fix.id, 10);
					/*
					else
						ctlNo = 1;
					*/
				switch(kind) {
					case __Dialog:
						{
							char symb[256];
							STRNSCPY(symb, Fix.fieldName);
							if(atol(symb) != 0)
								symb[0] = 0;
							if(f_log)
								fprintf(f_log, "%s\n", Fix.fieldName);
							Ctrl.Dlg.get(*this);
							fprintf(R_Out, "%s %s\nBEGIN\n", Fix.fieldName, P_TvNames[kind]);
							fprintf(R_Out, "\t%u, %u, %u, %u, \"%s\\0\", \"%s\\0\"\n",
								Fix.x, Fix.y, Fix.x1, Fix.y1, Ctrl.Dlg.title, symb);
						}
						break;
					case __Button:
						{
							char   bmp[64];
							Ctrl.Button.get(*this);
							WriteHeader(kind);
							STRNSCPY(bmp, Fix.extra[3]);
							if(*strip(bmp) == 0) {
								bmp[0] = '0';
								bmp[1] =  0;
							}
							fprintf(R_Out, ", \"%s\\0\", %s, \"%s\\0\", %u, %s, %s\n", Ctrl.Button.text,
								Ctrl.Button.txtCmd, Ctrl.Button.txtCmd, Ctrl.Button.flags, Fix.helpCtxSym, bmp);
						}
						break;
					case __Static:
						Ctrl.Static.get(*this);
						WriteHeader(kind);
						(line_buf = 0).CatDiv(',', 2);
						line_buf.CatQStr((temp_buf = Ctrl.Static.text).Cat("\\0")).CatDiv(',', 2);
						line_buf.CatQStr((temp_buf = Fix.extra[2]).Strip().Cat("\\0")).CatDiv(',', 2);
						line_buf.CatQStr((temp_buf = Fix.extra[3]).Strip().Cat("\\0")).CR();
						// @v10.9.0 fprintf(R_Out, line_buf.cptr());
						fputs(line_buf.cptr(), R_Out); // @v10.9.0
						// @v9.5.6 fprintf(R_Out, ", \"%s\\0\", \"%s\\0\", \"%s\\0\"\n", Ctrl.Static.text, strip(Fix.extra[2]), strip(Fix.extra[3]));
						break;
					case __ColoredText:
						Ctrl.Static.get(*this);
						WriteHeader(kind);
						fprintf(R_Out, ", \"%s\\0\\n", Ctrl.Static.text);
						break;
					case __InputLine:
						Ctrl.Input.get(*this);
						GetType();
						if(typeLen == 0)
							typeLen = Ctrl.Input.maxLen;
						GetFormat();
						if(fmtLen == 0)
							fmtLen = Ctrl.Input.maxLen/* - 1*/;
						{
							PTR32(virtButtonId)[0] = 0;
							char * p = strip(Fix.extra[2]);
							if(_strnicmp(p, "VB_", (size_t)3) == 0) {
								STRNSCPY(virtButtonId, p);
							}
							else {
								virtButtonId[0] = '0';
								virtButtonId[1] =  0;
							}
						}
						if(strcmp(virtButtonId, "0"))
							buttonCtrlId = 4256 - (vb_no++);
						else
							buttonCtrlId = 0;
						WriteHeader(kind);
						fprintf(R_Out, ", %s, %u, %u, %s, %u, %u, %s, %u, %s\n",
							typeName, typeLen, typeDec, fmtFlags, fmtLen, fmtDec,
							virtButtonId, buttonCtrlId, Fix.helpCtxSym);
						break;
					case __Label:
						Ctrl.Label.get(*this);
						WriteHeader(kind);
						ReplaceID(Ctrl.Label.link);
						fprintf(R_Out, ", \"%s\\0\", %s\n", Ctrl.Label.text, Ctrl.Label.link);
						break;
					/*
					case __History:
						blk.Ctrl.History.get();
						break;
					*/
					case __CheckBox:
					case __RadioButton:
					case __MultiCB:
						{
							Ctrl.Cluster.get(*this);
							WriteHeader(kind);
							fprintf(R_Out, ", %s\n", Fix.helpCtxSym);
							for(unsigned j = 0; j < Ctrl.Cluster.numItems; j++) {
								fprintf(R_Out, "\t\t\"%s\\0\"\n", Ctrl.Cluster.labels[j]);
								SAlloc::F(Ctrl.Cluster.labels[j]);
							}
							fprintf(R_Out, "\tTV_END\n");
						}
						break;
					case __ListBox:
						Ctrl.ListBox.get(*this);
						GetType();
						GetFormat();
						GetFlags();
						//GetNumCols(blk);
						//GetColNames(blk);
						//GetColWidth(blk);
						if(Ctrl.ListBox.scrollBar[0])
							ReplaceID(Ctrl.ListBox.scrollBar);
						WriteHeader(kind);
						fprintf(R_Out, ", %s, %s, %u, %u, %s, %u, %u, %s, \"%s\\0\"\n",
							Flags, typeName, typeLen, typeDec, fmtFlags, fmtLen,
							fmtDec, Fix.helpCtxSym, strip(Fix.extra[3]));
						break;
					/*
					case __Memo:
						Ctrl.Memo.get();
						break;
					*/
					case __ScrollBar:
						// Присутствие ScrollBar'а засечет ListBox
						break;
					case __Combo:
						Ctrl.Combo.get(*this);
						GetFlags();
						ReplaceID(Ctrl.Combo.link);
						WriteHeader(kind);
						fprintf(R_Out, ", %s, %s, %s, 0\n", Flags, Ctrl.Combo.link, Fix.helpCtxSym);
						break;
					default:
						{
							MsgBuf.Printf("Undefined control kind %d in file %s\n", kind, pFileName);
							error(MsgBuf);
						}
				}
				PushBlock(kind, Fix.id, orgID);
			}
			fprintf(R_Out, "END\n\n");
		}
		if(f_log)
			fclose(f_log);
	}
	int IsString()
	{
		int    c = fgetc(In);
		ungetc(c, In);
		return (fgetc(In) == '"');
	}
	void SkipWS()
	{
		char   c = fgetc(In);
		while(!feof(static_cast<FILE *>(In)) && oneof3(c, ' ', '\t', '\n'))
			c = fgetc(In);
		ungetc(c, In);
	}
	char * GetString(char * buf)
	{
		char * p = buf;
		SkipWS();
		int c = fgetc(In);
		if(c == '"') {
			do {
				c = fgetc(In);
				*p++ = c;
			} while(!feof(static_cast<FILE *>(In)) && c != '"');
			if(*(p-1) == '"')
				p--;
		}
		else
			ungetc(c, In);
		*p = 0;
		return buf;
	}
	char ** GetStrColl(unsigned count)
	{
		char   buf[128];
		char ** pp_coll = static_cast<char **>(SAlloc::C(count, sizeof(char *)));
		for(uint i = 0; i < count; i++) {
			GetString(buf);
			pp_coll[i] = sstrdup(buf);
		}
		return pp_coll;
	}
	uint GetUINT()
	{
		uint   i;
		fscanf(In, "%u", &i);
		return i;
	}
	int GetINT()
	{
		int i;
		fscanf(In, "%d", &i);
		return i;
	}
	long GetLONG()
	{
		long i;
		fscanf(In, "%ld", &i);
		return i;
	}
	uchar GetUCHAR()
	{
		uchar c;
		fscanf(In, "%c", &c);
		return c;
	}
	struct FixPart {
		FixPart()
		{
			THISZERO();
		}
		int    get(ScpDialogProcessor & rB)
		{
			rB.GetString(baseObj);
			rB.GetString(obj);
			x         = rB.GetUINT();
			y         = rB.GetUINT();
			x1        = rB.GetUINT();
			y1        = rB.GetUINT();
			defOpt    = rB.GetUINT();
			opt       = rB.GetUINT();
			defEvMask = rB.GetUINT();
			evMask    = rB.GetUINT();
			hlpCtx    = rB.GetUINT();
			growMode  = rB.GetUINT();
			for(int i = 0; i < 6; i++)
				rB.GetString(extra[i]);
			rB.GetString(helpCtxSym);
			if(helpCtxSym[0] == '\0')
				strcpy(helpCtxSym, "hcNoContext");
			rB.GetString(fieldName);
			rB.GetString(id);
			return 1;
		}
		char   baseObj[64];
		char   obj[64];
		uint   x, y, x1, y1, defOpt, opt, defEvMask, evMask, hlpCtx, growMode;
		char   extra[6][64];
		char   helpCtxSym[64];
		char   fieldName[64];
		char   id[64];
	};

	struct DlgSpec {
		int    get(ScpDialogProcessor & rB)
		{
			palette  = rB.GetUINT();
			winFlags = rB.GetUINT();
			rB.GetString(dlgProcName);
			rB.GetString(id);
			rB.GetString(title);
			return 1;
		}
		uint   palette;
		uint   winFlags;
		char   dlgProcName[64];
		char   id[64];
		char   title[64];
	};
	struct ButtonSpec {
		int    get(ScpDialogProcessor & rB)
		{
			rB.GetString(txtCmd);
			rB.GetString(text);
			cmd   = rB.GetUINT();
			flags = rB.GetUINT();
			if(*strip(txtCmd) == 0) {
				txtCmd[0] = '0';
				txtCmd[1] = 0;
			}
			return 1;
		}
		char   txtCmd[64];
		char   text[64];
		uint   cmd;
		uint   flags;
	};
	struct StaticSpec {
		int    get(ScpDialogProcessor & rB)
		{
			attr = rB.GetUINT();
			rB.GetString(text);
			return 1;
		}
		uint   attr;
		char   text[64];
	};
	struct InputSpec {
		int get(ScpDialogProcessor & rB)
		{
			maxLen  = rB.GetUINT();
			valType = rB.GetINT();
			rB.GetString(valName);
			if(valType != -1) {
				printf("Error: validator not supported!\n");
				return 0;
			}
			else
				return 1;
		}
		uint   maxLen;
		int    valType;
		char   valName[64];
		/* Data type stored in extra[0] field of FixPart */
	};
	struct LabelSpec {
		int    get(ScpDialogProcessor & rB)
		{
			rB.GetString(text);
			rB.GetString(link);
			return 1;
		}
		char   text[64];
		char   link[64];
	};
	struct HistorySpec {
		int    get(ScpDialogProcessor & rB)
		{
			histID = rB.GetUINT();
			rB.GetString(link);
			return 1;
		}
		uint   histID;
		char   link[64];
	};
	struct ClusterSpec {
		int    get(ScpDialogProcessor & rB)
		{
			numItems = rB.GetUINT();
			mask = rB.GetLONG();
			labels = rB.GetStrColl(numItems);
			//rsrv1 = rB.GetUINT();
			//rsrv2 = rB.GetUCHAR();
			//rB.GetString(rsrv3);
			return 1;
		}
		uint   numItems;
		long   mask;
		char ** labels;
		uint   rsrv1;
		uchar  rsrv2;
		char   rsrv3[32];
	};
	struct ListBoxSpec {
		int    get(ScpDialogProcessor & rB)
		{
			numCols = rB.GetUINT();
			rB.GetString(scrollBar);
			return 1;
		}
		uint   numCols;
		char   scrollBar[32];
	};
	struct MemoSpec {
		int    get(ScpDialogProcessor & rB)
		{
			rB.GetString(fldName);
			bufSize = rB.GetUINT();
			rB.GetString(vscroll);
			rB.GetString(hscroll);
			return 1;
		}
		char   fldName[32];
		uint   bufSize;
		char   vscroll[32];
		char   hscroll[32];
	};
	struct ComboSpec {
		int get(ScpDialogProcessor & rB)
		{
			rB.GetString(symb);
			flags = rB.GetUINT();
			rB.GetString(link);
			numItems = rB.GetUINT();
			items = rB.GetStrColl(numItems);
			return 1;
		}
		char   symb[8];
		uint   flags;
		char   link[32];
		uint   numItems;
		char ** items;
	};
	int    Kind;
	FixPart Fix;
	union Spec {
		Spec()
		{
			THISZERO();
		}
		DlgSpec        Dlg;
		ButtonSpec     Button;
		StaticSpec     Static;
		InputSpec      Input;
		LabelSpec      Label;
		HistorySpec    History;
		ClusterSpec    Cluster;
		ListBoxSpec    ListBox;
		MemoSpec       Memo;
		ComboSpec      Combo;
	};
	Spec   Ctrl;
	SFile  In;
	SFile & R_Out;
	SString LineBuf;
	SString MsgBuf;
	char    typeName[64];
	uint    typeLen;
	uint    typeDec;
	char    fmtFlags[128];
	uint    fmtLen;
	uint    fmtDec;
	char    virtButtonId[64];
	uint    buttonCtrlId;
	char    Flags[128];
	uint    numCols;
	char    colNames[512];
	char    colWidth[128];

	struct CtlStackItem {
		uint   CtlKind;
		char   Id[64];
		char   OrgId[64];
	};
	TSStack <CtlStackItem> Stk;

	int PushBlock(uint kind, const char * pId, const char * pOrgId)
	{
		CtlStackItem item;
		item.CtlKind = kind;
		STRNSCPY(item.Id, pId);
		STRNSCPY(item.OrgId, pOrgId);
		Stk.push(item);
		return 1;
	}
	void ReplaceID(char * b)
	{
		int    ok = 0;
		uint   i = Stk.getPointer();
		if(i) do {
			const CtlStackItem & r_item = *(const CtlStackItem *)Stk.at(--i);
			if(strcmp(b, r_item.OrgId) == 0) {
				strcpy(b, r_item.Id);
				ok = 1;
			}
		} while(!ok && i);
		if(!ok)
			error((MsgBuf = "Unresolved reference").Space().Cat(b));
	}
	void   GetType()
	{
		typeLen = 0;
		typeDec = 0;
		typeName[0] = 0;
		char * p = strip(Fix.extra[0]);
		char * n = typeName;
		char buf[128];
		while(*p && *p != ',')
			*n++ = *p++;
		if(*p == ',')
			p++;
		*n = 0;
		strip(typeName);
		n = buf;
		while(*p && *p != ',')
			*n++ = *p++;
		if(*p == ',')
			p++;
		*n = 0;
		typeLen = satoi(strip(buf));
		n = buf;
		while(*p && *p != ',')
			*n++ = *p++;
		*n = 0;
		if(*strip(buf))
			typeDec = satoi(buf);
		if(typeName[0] == 0)
			strcpy(typeName, "S_ZSTRING");
	}
	void GetFormat()
	{
		fmtLen = 0;
		fmtDec = 0;
		fmtFlags[0] = 0;
		char * p = strip(Fix.extra[1]);
		char * n = fmtFlags;
		char buf[32];
		while(*p && *p != ',')
			*n++ = *p++;
		if(*p == ',')
			p++;
		*n = 0;
		strip(fmtFlags);
		n = buf;
		while(*p && *p != ',')
			*n++ = *p++;
		if(*p == ',')
			p++;
		*n = 0;
		fmtLen = satoi(strip(buf));
		n = buf;
		while(*p && *p != ',')
			*n++ = *p++;
		*n = 0;
		if(*strip(buf))
			fmtDec = satoi(buf);
		if(fmtFlags[0] == 0)
			strcpy(fmtFlags, "0");
	}
	void GetFlags()
	{
		Flags[0] = 0;
		strcpy(Flags, strip(Fix.extra[2]));
		if(Flags[0] == 0)
			strcpy(Flags, "0");
	}
	void GetNumCols()
	{
		numCols = 0;
		numCols = satoi(strip(Fix.extra[3]));
	}
	void GetColNames()
	{
		colNames[0] = 0;
		strcpy(colNames, strip(Fix.extra[4]));
	}
	void GetColWidth()
	{
		colWidth[0] = 0;
		strcpy(colWidth, strip(Fix.extra[5]));
	}
	//
	// Записывает наименование элемента, координаты и идентификатор
	//
	void WriteHeader(int kind)
	{
		char   symb[256];
		STRNSCPY(symb, Fix.id);
		if(atol(symb) != 0)
			symb[0] = 0;
		fprintf(R_Out, "\t%-18s %2u, %2u, %2u, %2u, %s, \"%s\\0\"", P_TvNames[kind], Fix.x, Fix.y, Fix.x1, Fix.y1, Fix.id, symb);
	}
};

static int __Execute(const char * pCmdLine, HANDLE hIn, HANDLE hOut, uint * pExitCode)
{
	int    ok = 0;
	STARTUPINFO si;
	DWORD  exit_code = 0;
	PROCESS_INFORMATION pi;
	MEMSZERO(si);
	si.cb = sizeof(si);
	if(hIn || hOut) {
		si.hStdInput = hIn;
		si.hStdOutput = hOut;
		si.dwFlags |= STARTF_USESTDHANDLES;
	}
	MEMSZERO(pi);
	SString temp_buf(pCmdLine);
	STempBuffer cmd_line((temp_buf.Len() + 32) * sizeof(TCHAR));
	strnzcpy(static_cast<TCHAR *>(cmd_line.vptr()), SUcSwitch(temp_buf), cmd_line.GetSize() / sizeof(TCHAR));
	int    r = ::CreateProcess(0, static_cast<TCHAR *>(cmd_line.vptr()), 0, 0, /*FALSE*/TRUE, 0, 0, 0, &si, &pi);
	if(r) {
		WaitForSingleObject(pi.hProcess, INFINITE); // Wait until child process exits.
		GetExitCodeProcess(pi.hProcess, &exit_code);
		ok = 1;
	}
	ASSIGN_PTR(pExitCode, exit_code);
	return ok;
}

static int ProcessFileName(SString & rFileName, const SString & rInPath, const SString & rDlgDsnExec)
{
	int    ok = 1;
	SString temp_buf, msg_buf;
	SString org_ext;
	SString file_name = rFileName;
	SString temp_file_name;
	SFsPath ps, ps_path;
	ps.Split(file_name);
	ps_path.Split(rInPath);
	if(ps.Drv.IsEmpty() && ps_path.Drv.NotEmpty())
		ps.Drv = ps_path.Drv;
	if(ps.Dir.IsEmpty() && ps_path.Dir.NotEmpty())
		ps.Dir = ps_path.Dir;
	(org_ext = ps.Ext).ToLower();
	if(org_ext.IsEmpty())
		org_ext = "dlg";
	ps.Merge(file_name);
	ps.Nam = "temp";
	ps.Ext = "scp";
	ps.Merge(temp_file_name);
	if(fileExists(file_name)) {
		if(org_ext == "dlg") {
			if(rDlgDsnExec.NotEmpty()) {
				//..\..\tools\dlgdsn $(InputPath) /S$(OutDir)\$(InputName).scp
				temp_buf.Z().CatQStr(rDlgDsnExec).Space().Cat(file_name).Space().Cat("/S").Cat(temp_file_name);
				uint   exit_code;
				if(!__Execute(temp_buf, 0, 0, &exit_code)) {
					error((msg_buf = "Fault execute command").Space().Cat(temp_buf));
				}
				else if(exit_code == 0) {
					rFileName = temp_file_name;
					ok = 1;
				}
			}
			else {
				printf((msg_buf = "Warning: undefined DlgDsn.exe path").CR());
				ok = 0;
			}
		}
		else if(org_ext == "scp") {
			rFileName = file_name;
			ok = 1;
		}
		else {
			printf((msg_buf = "Warning: invalid file ext (must be .dlg or .scp)").Space().Cat(file_name).Space().CR());
			ok = 0;
		}
	}
	else {
		printf((msg_buf = "Warning: file not found").Space().Cat(file_name).CR());
		ok = 0;
	}
	return ok;
}

static int ProcessFileNameVDos(StringSet & rFileNames, StringSet & rResultFileNames, ExecVDosParam & rEvdp, const SString & rInPath, const SString & rToolsPath)
{
	rResultFileNames.Z();

	int    ok = 1;
	SString temp_buf, msg_buf;
	SString org_ext;
	SString org_file_name;
	SString file_name;
	SString temp_file_name;
	SString temp_file_path;
	SFsPath ps, ps_path;
	for(uint fnp = 0; ok && rFileNames.get(&fnp, org_file_name);) {
		ps.Split(org_file_name);
		ps_path.Split(rInPath);
		if(ps.Drv.IsEmpty() && ps_path.Drv.NotEmpty())
			ps.Drv = ps_path.Drv;
		if(ps.Dir.IsEmpty() && ps_path.Dir.NotEmpty())
			ps.Dir = ps_path.Dir;
		(org_ext = ps.Ext).ToLower();
		org_ext.SetIfEmpty("dlg");
		ps.Merge(file_name);
		(temp_file_name = ps.Nam).Dot().Cat("scp");
		ps.Ext = "scp";
		ps.Merge(temp_file_path);
		if(fileExists(file_name)) {
			if(org_ext == "dlg") {
				if(rToolsPath.NotEmpty()) {
					const char * p_dlg_src_path = "c:\\src\\rsrc\\dlg\\";
					SString vdos_temp_file_path;
					(vdos_temp_file_path = p_dlg_src_path).Cat(temp_file_name);
					(temp_buf = "c:\\tools\\dlgdsn.exe").Space().Cat(p_dlg_src_path).Cat(org_file_name).Space().Cat("/S").Cat(vdos_temp_file_path);
					rEvdp.Batch.add(temp_buf);
					rResultFileNames.add(temp_file_path);
				}
			}
			else {
				printf((msg_buf = "Warning: invalid file ext (must be .dlg)").Space().Cat(file_name).Space().CR());
			}
		}
		else {
			printf((msg_buf = "Warning: file not found").Space().Cat(file_name).CR());
		}
	}
	return ok;
}

int main(int argc, char ** argv)
{
	enum {
		cmdlNone = 0,
		cmdlInFile,
		cmdlInList,
		cmdlOutFile,
		cmdlInPath,
		cmdlDlgDsnPath,
		cmdlToolsPath
	};
	struct CmdLineSyntax {
		const char * P_Text;
		int    Cmd;
	};
	CmdLineSyntax syntax_tab[] = {
		{ "input;in", cmdlInFile },
		{ "list;lst", cmdlInList },
		{ "output;out", cmdlOutFile },
		{ "path;inpath", cmdlInPath },
		{ "dlgdsn", cmdlDlgDsnPath },
		{ "toolspath", cmdlToolsPath }
	};
	const  int use_vdos = 1;
	int    i;
	SFile  outf;
	if(argc < 3)
		error("Usage: SPC2RC OUTFILE [@]INFILE");
	else {
		SString out_file_name, final_file_name;
		SString inp_file_name;
		SString inp_file_name_list;
		SString inp_file_path;
		SString tools_path;
		SString dlgdsn_exec_path;
		SString perl_cmd;
		SString arg_line, arg_val, temp_buf, msg_buf;
		SFsPath ps;
		for(i = 1; i < argc; i++) {
			(arg_line = argv[i]).Strip();
			arg_val = 0;
			if(arg_line.C(0) == '-') {
				do {
					arg_line.ShiftLeft();
				} while(arg_line.C(0) == '-');
				arg_line.Strip();
				int    cmd = 0;
				int    colon = 0;
				for(uint n = 0; !cmd && n < SIZEOFARRAY(syntax_tab); n++) {
					const CmdLineSyntax & r_se = syntax_tab[n];
					StringSet ss(';', r_se.P_Text);
					for(uint j = 0; !cmd && ss.get(&j, temp_buf);) {
						const size_t cmd_len = temp_buf.Len();
						if(arg_line.CmpPrefix(temp_buf, 1) == 0) {
							cmd = r_se.Cmd;
							if(arg_line.Len() > cmd_len) {
								if(arg_line.C(cmd_len) == ':' || arg_line.C(cmd_len) == '=') {
									(arg_val = arg_line.cptr() + cmd_len + 1).Strip();
								}
								else
									cmd = 0;
							}
						}
					}
				}
				if(cmd) {
					if(arg_val.IsEmpty()) {
						i++;
						if(i < argc)
							(arg_val = argv[i]).Strip();
					}
					switch(cmd) {
						case cmdlInFile: inp_file_name = arg_val; break;
						case cmdlInList: inp_file_name_list = arg_val; break;
						case cmdlInPath: inp_file_path = arg_val; break;
						case cmdlOutFile: out_file_name = arg_val; break;
						case cmdlDlgDsnPath: dlgdsn_exec_path = arg_val; break;
						case cmdlToolsPath: tools_path = arg_val; break;
					}
				}
				else {
					(temp_buf = "Warning: unknown argument ").Cat(arg_line).CR();
					printf(temp_buf);
				}
			}
			else if(i == 1) {
				out_file_name = arg_line;
			}
			else if(i == 2) {
				if(arg_line.C(0) == '@') {
					inp_file_name_list = arg_line.ShiftLeft(1);
				}
				else {
					inp_file_name = arg_line;
				}
			}
		}
		if(out_file_name.IsEmpty()) {
			error("Undefined output file name");
		}
		if(inp_file_name.IsEmpty() && inp_file_name_list.IsEmpty()) {
			error("Undefined input file name or list of file");
		}
		{
			ps.Split(out_file_name);
			ps.Nam = "ppdlgw";
			ps.Ext = "rc";
			ps.Merge(final_file_name);
		}
		// fprintf(_out, "/* %s\nGenerate by SCP2RC */\n\n#include <tvdefs.h>\n\n", strupr(argv[1]));

		if(inp_file_name_list.NotEmpty()) {
			if(!fileExists(inp_file_name_list)) {
				(msg_buf = "File").Space().CatChar('\'').Cat(inp_file_name_list).CatChar('\'').Space().Cat("not fount");
				error(msg_buf);
			}
			SFile rspf(inp_file_name_list, SFile::mRead);
			if(!rspf.IsValid()) {
				msg_buf.Printf("Unable open list file %s", inp_file_name_list.cptr());
				error(msg_buf);
			}
			int    do_process = 0;
			if(!fileExists(final_file_name) || !fileExists(out_file_name)) {
				do_process = 1;
			}
			else {
				LDATETIME finalf_dtm = ZERODATETIME;
				SFile finalf(final_file_name, SFile::mRead);
				if(finalf.IsValid() && finalf.GetDateTime(0, 0, &finalf_dtm)) {
					SFsPath ps_path;
					while(!do_process && rspf.ReadLine(temp_buf)) {
						if(temp_buf.Chomp().NotEmptyS()) {
							if(temp_buf.CmpPrefix("//", 0) != 0 && temp_buf.CmpPrefix("--", 0) != 0) { // @v10.5.3 comments
								LDATETIME  depf_dtm;
								ps.Split(temp_buf);
								ps_path.Split(inp_file_path);
								if(ps.Drv.IsEmpty() && ps_path.Drv.NotEmpty())
									ps.Drv = ps_path.Drv;
								if(ps.Dir.IsEmpty() && ps_path.Dir.NotEmpty())
									ps.Dir = ps_path.Dir;
								ps.Merge(temp_buf);
								SFile depf(temp_buf, SFile::mRead);
								if(depf.IsValid() && depf.GetDateTime(0, 0, &depf_dtm) && cmp(depf_dtm, finalf_dtm) > 0)
									do_process = 1;
							}
						}
					}
				}
				else {
					finalf.Close();
					SFile::Remove(final_file_name);
					do_process = 1;
				}
			}
			if(do_process) {
				SFile outf(out_file_name, SFile::mWrite);
				if(!outf.IsValid()) {
					msg_buf.Printf("Unable open output file %s", out_file_name.cptr());
					error(msg_buf);
				}
				// Проверяя время модификации файлов мы переместили текущую позицию - вернем назад
				rspf.Seek(0); 
				//
				if(tools_path.NotEmptyS()) {
					if(!dlgdsn_exec_path.NotEmptyS()) {
						(dlgdsn_exec_path = tools_path).SetLastSlash().Cat("dlgdsn.exe");
					}
					(perl_cmd = tools_path).SetLastSlash().Cat("perl").SetLastSlash().Cat("perl").Space().
						Cat(tools_path).SetLastSlash().Cat("RC_CONV.PL");
				}
				if(use_vdos) {
					ExecVDosParam evd_param;
					StringSet ss_in_files;
					StringSet ss_out_files;
					while(rspf.ReadLine(temp_buf)) {
						if(temp_buf.Chomp().NotEmptyS()) {
							if(temp_buf.CmpPrefix("//", 0) != 0 && temp_buf.CmpPrefix("--", 0) != 0) { // @v10.5.3 comments
								//printf((msg_buf = "Processing file").Space().Cat(temp_buf).CR());
								ss_in_files.add(temp_buf);
							}
						}
					}
					(evd_param.ExePath = tools_path).SetLastSlash().Cat("vdos");
					(evd_param.StartUpPath = evd_param.ExePath).SetLastSlash().Cat("..").SetLastSlash().Cat("..");
					evd_param.Flags |= (evd_param.fExitAfter|evd_param.fWait);
					if(ProcessFileNameVDos(ss_in_files, ss_out_files, evd_param, inp_file_path, tools_path)) {
						printf((msg_buf = "Processing dialog files").CR());
						if(!ExecVDos(evd_param)) {
							error((msg_buf = "Fault execute vdos"));
						}
						else {
							for(uint ssp = 0; ss_out_files.get(&ssp, temp_buf);) {
								if(::fileExists(temp_buf)) {
									fprintf(outf, "\n\n");
									ScpDialogProcessor prc(outf);
									prc.Run(temp_buf);
								}
								else {
									printf((msg_buf = "Warning: file").Space().Cat(temp_buf).Space().Cat("not found").CR());
								}
							}
						}
					}
				}
				else {
					while(rspf.ReadLine(temp_buf)) {
						if(temp_buf.Chomp().NotEmptyS()) {
							printf((msg_buf = "Processing file").Space().Cat(temp_buf).CR());
							if(ProcessFileName(temp_buf, inp_file_path, dlgdsn_exec_path)) {
								fprintf(outf, "\n\n");
								ScpDialogProcessor prc(outf);
								prc.Run(temp_buf);
							}
						}
					}
				}
				outf.Close();
				if(perl_cmd.NotEmptyS()) {
					//
					// Надо отконвертировать кодировку символов OemToChar
					// Для этого придется создать временный файл с расширением tmp и пренести в него 
					// файл out_file_name в перекодированном виде.
					//
					SString temp_file_name;
					temp_file_name = out_file_name;
					SFsPath::ReplaceExt(temp_file_name = out_file_name, "tmp", 1);
					{
						SFile tempf(temp_file_name, SFile::mWrite);
						if(!tempf.IsValid()) {
							error((msg_buf = "Error opening file").Space().Cat(temp_file_name));
						}
						outf.Open(out_file_name, SFile::mRead);
						if(!outf.IsValid()) {
							error((msg_buf = "Error opening file").Space().Cat(out_file_name));
						}
						while(outf.ReadLine(temp_buf)) {
							temp_buf.ToChar();
							tempf.WriteLine(temp_buf);
						}
						outf.Close();
					}
					//
					//(perl_cmd = tools_path).SetLastSlash().Cat("perl").SetLastSlash().Cat("perl").Space().
					//	Cat(tools_path).SetLastSlash().Cat("RC_CONV.PL").Space().CatChar('<').Space().Cat(outf.GetName()).Space().CatChar('>').Space().Cat(final_file_name);
					SECURITY_ATTRIBUTES sa;
					sa.nLength = sizeof(SECURITY_ATTRIBUTES);
					sa.bInheritHandle = TRUE;
					sa.lpSecurityDescriptor = NULL;

					HANDLE h_inp = ::CreateFile(SUcSwitch(temp_file_name),  GENERIC_READ,  FILE_SHARE_READ, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
					HANDLE h_out = ::CreateFile(SUcSwitch(final_file_name), GENERIC_WRITE, FILE_SHARE_WRITE, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
					uint   exit_code;
					if(!__Execute(perl_cmd, h_inp, h_out, &exit_code)) {
						error((msg_buf = "Fault execute command").Space().Cat(perl_cmd));
					}
					else if(exit_code == 0) {
						// @ok
					}
					::CloseHandle(h_inp);
					::CloseHandle(h_out);
					SFile::Remove(temp_file_name);
				}
			}
			else {
				printf((msg_buf = "All files are up to date").CR());
			}
		}
		else {
			SFile outf(out_file_name, SFile::mWrite);
			if(!outf.IsValid()) {
				msg_buf.Printf("Unable open output file %s", out_file_name.cptr());
				error(msg_buf);
			}
			if(!fileExists(inp_file_name)) {
				error((msg_buf = "File").Space().CatChar('\'').Cat(inp_file_name).CatChar('\'').Space().Cat("not fount"));
			}
			if(ProcessFileName(inp_file_name, inp_file_path, dlgdsn_exec_path)) {
				fprintf(outf, "\n\n");
				ScpDialogProcessor prc(outf);
				prc.Run(inp_file_name);
			}
		}
	}
	return 0;
}
