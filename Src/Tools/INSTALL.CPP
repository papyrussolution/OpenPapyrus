// INSTALL.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2006, 2008
//
#define Uses_TScreen
#include <pp.h>
#include <ppdlgs.h>
#pragma hdrstop
#include <stdio.h>
#include <dir.h>
#include <direct.h>
#include <string.h>
#include <dos.h>
#include <process.h>
#include <ctype.h>
#include <ppinstapp.h>

#define MAX_PATH_SZ 256

// Prototype
int IsCDROMDrive(int drv /* 0 'A', 1 - 'B' ... */);
//
// Stub (instead PPLIB\PPDBUTIL.CPP
//
int CallbackCompress(long, long, const char *, int)
{
	return 1;
}

#define OK              1
#define ERR             0
#define NOT            -1
#define MORE           -2
//
// MODES
//
#define NOSET           1
#define NEWPROGRAMM     2
#define UNDO            3
//
// FPRT_MODES
//
#define LOGNAME         "install.log"
#define LOGTMPNAME      "instlog.tmp"
#define ARCFILENAME     "ppy_undo.zip"
#define ARCFILEOLD      "ppy_undo.old"
#define LOG_START       0
#define LOG_NEWVER      1
#define LOG_MAKEDIR     2
#define LOG_MAKEFILE    3
#define LOG_ARC         4
#define LOG_UNDO        8
#define LOG_STOP        9
//
// ERRORS
//
#define ERR_NOT_DRV       -100
#define ERR_NOT_ROOT      -101
#define ERR_NOT_PATH      -102
#define ERR_GET_PATH      -103
#define ERR_MAKE_DIR      -104
#define ERR_FILE_NOT_OPEN -105
#define ERR_USER_BREAK    -106
#define ERR_DEL_FILE      -107
#define ERR_UNPACK        -108
#define ERR_OPEN_FILE     -109
#define ERR_NOT_FILE      -110
#define ERR_NOT_MEM       -111
#define ERR_NOT_TMP       -112
#define ERR_COPY_FILE     -113
#define ERR_LOG_ID        -114
#define ERR_LOG_STRUCT    -115
#define ERR_MOREERR       -116
#define ERR_NO            -117
#define ERR_STR_LOG       -118
#define ERR_DEL_DIR       -119
#define ERR_PACK          -120
#define ERR_LOAD_DIALOG   -121
#define ERR_EXEC          -122
#define ERR_INVINIPATH    -123
//
// DIRS
//
#define P_ROOT               0
#define P_BIN                1
#define P_SYS                2
#define P_DAT                3
#define P_ARC                5
#define P_LOG                6
#define P_TEMP              10
#define NDIRS               13
#define NINSTFILES           4

uint PPInstVer      = 0x0100;
uint PPInstRevision = 0;

unsigned _stklen    = 0x5000U;
unsigned _ovrbuffer = 0x2000U;

const char * DirParam[NDIRS] = {
	"@ROOT", "@BIN", "@SYS", "@DAT", "@DOC", "@ARC",
	"@LOG",  "@REC", "@IN",  "@OUT", "@TMP", "@PACK", "@DD"
};

const char * InstFiles[NINSTFILES] = {
	"install.res", "install.ini", "pkunzip.exe", "pkzip.exe"
};

#define Pack	"pkzip.exe"
#define Unpack	"pkunzip.exe"
//
// Errors
//
static char errbuf[64];
static char errstr[256];
static int  errcode=OK;

struct PRG {
	int    Flag;
	char   PathRoot[MAX_PATH_SZ];
	int    OldArcFlag;
	char   OldArcPath[MAX_PATH_SZ];
	int    LogFlag;
	char   LogPath[MAX_PATH_SZ];
	char   Ver[32];
};

class Install {
public:
	Install();
	~Install();
	void   StartInstall();
private:
	int    SetNewProgramm();
	int    Undo(int mode);
	int    GetOldVersion();
	int    GetMode();
	int    GetPaths();
	int    BeginInst();
	int    EndInst();
	int    OpenLogFile();
	int    CloseLogFile(int mode);
	int    GetLastStrLOG(StringSet *s);
	int    AddLogFile(int ID, char *param1, char *param2);
	int    YN(char *str);
	int    ExecFiles();
	void   SetErr(int ID, const char *str);
	int    ChgDir(char *Path);
	int    GetCurPath(char *Path);
	int    GetTmpPath();
	int    MakeDir(char *Path);
	int    FindFileInCurDir(char *Path, const char *FFile);
	int    FindFile(char *disc, char *Path, const char *FFile);
	int    FindAllFilesInCurDir(char *Paths[], int *n, char *FFile, int MaxFiles);
	int    FindAllFiles(char *Paths[], int *n, char *FFile, int MaxFiles);

	int    PackFile(char * arc, char * src);
	int    UnpackFile(char * arc, char * fname, char * dest);

	PPIniFile * inifile; // "install.ini";
	FILE      * logfile; // протокол
	long        logstartpos;
	ushort      Smode;
	int         UndoFlag;
	PRG         OldVer;
	char        NewVersion[20];
	char        InstPaths[NDIRS][MAX_PATH_SZ];
	char        InstCurPath[MAX_PATH_SZ];
	char        InstTmpPath[MAX_PATH_SZ];
};

Install::Install()
{
	inifile     = 0;
	logfile     = 0;
	logstartpos = 0;
	Smode       = NOSET;
	OldVer.Flag = 0;
	OldVer.OldArcFlag = 0;
	OldVer.LogFlag    = 0;
	UndoFlag          = 0;
}

Install::~Install()
{
}

int Install::PackFile(char * arc, char * src)
{
	char exe[MAX_PATH_SZ];
	sprintf(exe, "%s\\%s", InstCurPath, Pack);
	if(spawnlp(P_WAIT, exe, exe, arc, src, NULL) == -1) { // В архив
		SetErr(ERR_PACK, src);
		return 0;
	}
	return 1;
}

int Install::UnpackFile(char * arc, char * fname, char * dest)
{
	char exe[MAX_PATH_SZ];
	sprintf(exe, "%s\\%s", InstCurPath, Unpack);
	if(spawnl(0, exe, exe, "-)", arc, fname, dest, NULL) == -1) {
		SetErr(ERR_UNPACK, arc);
		return 0;
	}
	return 1;
}

static int IsDiskReady(int driveNo)
{
	struct diskfree_t dtable;
	if(_dos_getdiskfree(driveNo, &dtable) == 0)
		return 1;
	else
		return 0;
}

int Install::GetTmpPath()
{
	char *ptr=NULL;
	int fnd=0, i;
	ptr=getenv("TMP");
	if(ptr==NULL) {
		ptr=getenv("TEMP");
		if(ptr==NULL) {
			ptr="A:\\";
			for(i='C'; i<='Z'; i++) {
				if(_chdrive(i-'A'+1)==0 && IsDiskReady(i-'A'+1)) {
					ptr[0]=i;
					fnd=1;
					break;
				}
			}
			if(!fnd) {
				SetErr(ERR_NOT_TMP, 0);
				return ERR;
			}
		}
	}
	i = strlen(ptr) - 1;
	if(ptr[i] == '\\' || ptr[i]=='/')
		ptr[i] = 0;
	strcpy(InstTmpPath, ptr);
	return OK;
}

static struct errtab_tag {
	int    code;
	char * msg;
} errtab[] = {
	{ERR_NOT_DRV,       "Не найден диск"},
	{ERR_NOT_ROOT,      "Невозможно открыть корневой каталог"},
	{ERR_NOT_PATH,      "Невозможно открыть каталог"},
	{ERR_GET_PATH,      "Не найден текущий каталог"},
	{ERR_MAKE_DIR,      "Невозможно создать каталог"},
	{ERR_FILE_NOT_OPEN, "Файл не открыт"},
	{ERR_USER_BREAK,    "Процесс прерван по желанию пользователя"},
	{ERR_DEL_FILE,    "Невозможно удалить файл"},
	{ERR_UNPACK,      "Ошибка при распаковке файла"},
	{ERR_OPEN_FILE,   "Невозможно открыть файл"},
	{ERR_NOT_FILE,    "Не найден файл"},
	{ERR_NOT_MEM,     "Недостаточно памяти для операции"},
	{ERR_NOT_TMP,     "Не найден временный каталог"},
	{ERR_COPY_FILE,   "Ошибка при копировании файла"},
	{ERR_LOG_ID,      "Недопустимый идентификатор в файле протокола"},
	{ERR_LOG_STRUCT,  "Ошибка в структуре файла протокола"},
	{ERR_MOREERR, ""},
	{ERR_NO, ""},
	{ERR_STR_LOG,     "Ошибка в строке файла протокола"},
	{ERR_DEL_DIR,     "Ошибка при удалении каталога"},
	{ERR_PACK,        "Ошибка при архивации файла"},
	{ERR_LOAD_DIALOG, "Ошибка загрузки диалога"},
	{ERR_EXEC,        "Ошибка при запуске файла"},
	{ERR_INVINIPATH,  "Недопустимый каталог в INI файле"}
};

void Install::SetErr(int ID, const char *str)
{
	errcode = ID;
	if(errcode != ERR_NO)
		for(int i = 0; i < sizeof(errtab) / sizeof(errtab_tag); i++)
			if(errtab[i].code == ID) {
				errstr[0] = '\003';
				strcpy(errstr + 1, errtab[i].msg);
				if(str && str[0]) {
					strcat(errstr, "\n\003");
					strcat(errstr, str);
				}
				return;
			}
	errstr[0] = 0;
}
//
// Перейти в заданный каталог
//
int Install::ChgDir(char *Path)
{
	int drv, ok=OK;
	rmvLastSlash(strupr(strip(Path)));
	if(Path[1] == ':') {
		drv = Path[0]-'A'+1;
		if(_chdrive(drv) == -1) {
			strcpy(errbuf, "A:");
			errbuf[0] = drv;
			SetErr(ERR_NOT_DRV, errbuf);
			CALLEXCEPT();
		}
		if(chdir("\\") == -1) {
			SetErr(ERR_NOT_ROOT,0);
			CALLEXCEPT();
		}
		if(Path[2] && Path[3])
			if(chdir(Path) == -1) {
				SetErr(ERR_NOT_PATH, Path);
				CALLEXCEPT();
			}
	}
	else {
		if(chdir(Path) == -1) {
			SetErr(ERR_NOT_PATH, Path);
			CALLEXCEPT();
		}
	}
	CATCH
		ok=ERR;
	ENDCATCH
	return ok;
}
//
// Запомнить текущий путь
//
int Install::GetCurPath(char *Path)
{
	strcpy(Path,"A:\\");
	Path[0] = 'A' + getdisk();
	if(getcurdir(0, Path+3) == -1) {
		SetErr(ERR_GET_PATH, 0);
		return ERR;
	}
	strupr(Path);
	return OK;
}
//
// Создать все каталоги из пути
//			OK	Готово
//			NOT	Путь уже был
//
int Install::MakeDir(char *Path)
{
	char CurPath[MAX_PATH_SZ];
	char NewPath[MAX_PATH_SZ];
	char MDir[MAX_PATH_SZ], c;
	int  i = 0, j = 0, ok = OK, l;
	strupr(strip(Path));
	THROW(GetCurPath(CurPath));
	if(Path[1] == ':')
		strcpy(NewPath, Path);
	else{
		strcpy(NewPath, CurPath);
		NewPath[3] = 0;
		if(Path[0] == '\\')
			strcat(NewPath, Path+1);
		else
			strcat(NewPath, Path);
	}
	if(ChgDir(NewPath) == OK) {
		if(ChgDir(CurPath) == ERR)
			return ERR;
		else
			return NOT;
	}
	if(_chdrive(NewPath[0]-'A'+1) == -1) {
		strcpy(errbuf, "A:");
		errbuf[0] = NewPath[0]-'A'+1;
		SetErr(ERR_NOT_DRV, errbuf);
		CALLEXCEPT();
	}
	for(i = 3, l = strlen(NewPath); i <= l; i++) {
		c = NewPath[i];
		MDir[j++] = c;
		if(c == '\\' || c == 0) {
			NewPath[i] = 0;
			MDir[j-1] = 0; j = 0;
			if(chdir(NewPath) == -1) {     // Не найден
				if(mkdir(MDir) == -1) {    // Создать
					SetErr(ERR_MAKE_DIR, MDir);
					CALLEXCEPT();
				}
				if(chdir(MDir) == -1) {
					SetErr(ERR_NOT_PATH, MDir);
					CALLEXCEPT();
				}
				THROW(AddLogFile(LOG_MAKEDIR, NewPath,0));
			}
			NewPath[i]=c;
		}
	}
	THROW(ChgDir(CurPath));
	CATCH
		ok = ERR;
	ENDCATCH
	return ok;
}
//
// Поиск файла в текущем каталоге и во всех его подкаталогах
//			OK	Найден
//			NOT	Не найден
//
int Install::FindFileInCurDir(char *Path, const char *FFile)
{
	int i, ok;
	struct ffblk fd;
	if(ChgDir(Path) == ERR)
		return ERR;
	if(findfirst("*.*", &fd, FA_DIREC)==0)
		do {
			if((fd.ff_attrib & FA_DIREC) && (fd.ff_name[0] != '.')) {
				i = strlen(strip(Path));
				setLastSlash(Path);
				strcat(Path, fd.ff_name);
				setLastSlash(Path);
				ok = FindFileInCurDir(Path, FFile);
				if(ok != NOT)
					return ok;
				Path[i] = 0;
			}
			if(stricmp(fd.ff_name, FFile) == 0) {
				i = strlen(Path)-1;
				if(Path[i] == '\\')
					Path[i] = 0;
				return OK;
			}
		} while(findnext(&fd) == 0);
	return NOT;
}
//
// Поиск файла по всем дискам или по заданному
//			OK	Найден
//			NOT	Не найден
//
int Install::FindFile(char *disc, char *Path, const char *FFile)
{
	char CurPath[MAX_PATH_SZ], buf[MAX_PATH_SZ];
	int ok=NOT;
	int i;
	if(disc == NULL || disc[0] == 0)
		return NOT;
	if(Path == NULL || Path[0] == 0)
		buf[0]=0;
	else
		strcpy(buf, Path);
	strip(buf);
	if(buf[0] == '\\')
		buf[0] = ' ';
	rmvLastSlash(strip(buf));
	THROW(GetCurPath(CurPath));
	sprintf(Path, "A:\\%s", buf);
	if(disc[0] == '*') {
		for(i = 3; i < 27; i++)
			if(!IsCDROMDrive(i-1) && _chdrive(i) == 0 && IsDiskReady(i)) {
				Path[0] = 'A'+i-1;
				THROW(ok = FindFileInCurDir(Path, FFile));
				if(ok == OK)
					break;
			}
	}
	else {
		Path[0] = disc[0];
		THROW(ok = FindFileInCurDir(Path, FFile));
	}
	THROW(ChgDir(CurPath));
	CATCH
		ok = ERR;
	ENDCATCH
	return ok;
}
//
// Поиск всех файлов в текущем каталоге и во всех его подкаталогах
//			OK	Найден
//			NOT	Не найден ни один
//			MORE слишком много найдено файлов
//
int Install::FindAllFilesInCurDir(char *Paths[], int *n, char *FFile, int MaxFiles)
{
	int ok, i;
	struct ffblk fd;
	if(ChgDir(Paths[*n])==ERR)
		return NOT; // ERR;
	if(findfirst("*.*", &fd, FA_DIREC) == 0)
		do {
			if((fd.ff_attrib & FA_DIREC) && (fd.ff_name[0] != '.')) {
				i = strlen(strip(Paths[*n]));
				if(Paths[*n][i-1] != '\\') {
					Paths[*n][i] = '\\';
					Paths[*n][++i] = 0;
				}
				strcat(Paths[*n], fd.ff_name);
				ok = FindAllFilesInCurDir(Paths, n, FFile, MaxFiles);
				if(ok == MORE || ok == ERR)
					return ok;
				Paths[*n][i] = 0;
			}
			if(stricmp(fd.ff_name, FFile)==0) {
				(*n)++;
				if(*n >= MaxFiles)
					return MORE;
				strcpy(Paths[*n], Paths[*n-1]);
			}
		} while(findnext(&fd) == 0);
	if(*n==0)
		return NOT;
	return OK;
}
//
// Поиск всех файлов по всем дискам начиная с 'C'
//			OK	Найден(ы)
//			NOT	Не найден ни один
//			MORE слишком много файлов
//
int Install::FindAllFiles(char *Paths[], int *n, char *FFile, int MaxFiles)
{
	int r = OK, i;
	char CurPath[MAX_PATH_SZ];
	*n = 0;
	THROW(GetCurPath(CurPath));
	for(i = 0; i < MaxFiles; i++)
		sprintf(Paths[i], "A:\\");
	for(i = 'C'; i <= 'Z'; i++)
		if(!IsCDROMDrive(i - 'A') && _chdrive(i-'A'+1) == 0 && IsDiskReady(i - 'A' + 1)) {
			Paths[*n][0] = i;
			THROW(r = FindAllFilesInCurDir(Paths, n, FFile, MaxFiles));
			if(r == MORE)
				break;
		}
	THROW(ChgDir(CurPath));
	if(*n == 0)
		r = NOT;
	CATCH
		r = ERR;
	ENDCATCH
	return r;
}
//
// Программа Инсталляции
//
void Install::StartInstall()
{
	THROW(BeginInst());   // подготовка : xxx.x --> TMP
	THROW(GetOldVersion());
	THROW(GetMode());
	switch(Smode) {
		case NEWPROGRAMM: {
			THROW(GetPaths());
			THROW(OpenLogFile());
			THROW(AddLogFile(LOG_NEWVER, NewVersion,0));
			THROW(SetNewProgramm());
			}
			break;
		case UNDO:
			if(OldVer.Flag) {
				THROW(OpenLogFile());
				THROW(Undo(OK));
			}
			else {
				SetErr(ERR_MOREERR,"\n\n\03Невозможная ошибка !!!");
				CALLEXCEPT();
			}
			break;
		case NOSET: {
			SetErr(ERR_USER_BREAK,0);
			CALLEXCEPT();
		}
		default:
			break;
	}
	CloseLogFile(OK);
	if(OldVer.OldArcFlag) {
		THROW(ChgDir(OldVer.OldArcPath));
		remove(ARCFILEOLD);
	}
//    THROW(ExecFiles());
	PPMessage(mfInfo|mfOK, 0, "\003Процесс успешно завершен !");
	CATCH
		PPMessage(mfOK,0, errstr);
		if(UndoFlag)
			THROW(Undo(ERR));
		if(OldVer.OldArcFlag) {
			ChgDir(OldVer.OldArcPath);
			rename (ARCFILEOLD, ARCFILENAME);
		}
		CloseLogFile(ERR);
	ENDCATCH
	EndInst();
}
//
// Функции
//
int Install::BeginInst()
{
	int  i, ok = OK;
	char buf[MAX_PATH_SZ], buf1[MAX_PATH_SZ];
	THROW(GetCurPath(InstCurPath));
	THROW(GetTmpPath());
	THROW(ChgDir(InstCurPath));
	PPWait(1);
	PPWaitMsg("Подготовка к инсталляции ...");
	for(i = 0; i < NINSTFILES; i++) {
		sprintf(buf, "%s\\%s", InstCurPath, InstFiles[i]);
		sprintf(buf1, "%s\\%s", InstTmpPath, InstFiles[i]);
		if(fileExists(buf) == ERR) {
			SetErr(ERR_NOT_FILE, InstFiles[i]);
			CALLEXCEPT();
		}
		if(copyFileByName(buf, buf1)==ERR) {
			SetErr(ERR_COPY_FILE, InstFiles[i]);
			CALLEXCEPT();
		}
		if(i == 0)
			GlobalRez = new TVRez(buf1, 1);
	}
	THROW(ChgDir(InstTmpPath));
	setLastSlash(strcpy(buf, InstTmpPath));
	strcat(buf, "INSTALL.INI");
	inifile = new PPIniFile(buf);
	if(inifile->Valid() == ERR) {
		SetErr(ERR_NOT_FILE, "install.ini");
		CALLEXCEPT();
	}
	inifile->GetParam("Version", "Ver",    NewVersion, sizeof(NewVersion));
	for(i = 0; i < NDIRS; i++)
		inifile->GetParam("DirStruct", (char *)DirParam[i], InstPaths[i], MAX_PATH_SZ);
	CATCH
		ok = ERR;
	ENDCATCH
	PPWait(0);
	return ok;
}

int Install::EndInst()
{
	int ok = OK, i;
	THROW(ChgDir(InstTmpPath));
	for(i = 0; i < NINSTFILES; i++)
		remove(InstFiles[i]);
	THROW(ChgDir(InstCurPath));
	CATCH
		ok = ERR;
	ENDCATCH
	return ok;
}

int Install::GetOldVersion()
{
	//Найти все старые программы и определить их версии
	char * OldPaths[20], buf[MAX_PATH_SZ+22], bufv[20];
	uint ver, rev;
	int  i, n=0, p=0, ret=OK;
	long r = 0;
	OldVer.Flag=0;
	OldVer.OldArcFlag=0;
	OldVer.LogFlag=0;
	PPWait(1);
	for(i = 0; i < 20; i++) {
		OldPaths[i] = new char[MAX_PATH_SZ+22];
		if(OldPaths[i] == NULL) {
			SetErr(ERR_NOT_MEM,0);
			CALLEXCEPT();
		}
	}
	PPWaitMsg("Поиск предыдущих установок программы ...");
	THROW(ret = FindAllFiles(OldPaths, &n, "pp.exe", 20));
	if(ret == MORE) {
		PPWait(0);
		PPMessage(mfWarn|mfOK,0,"\03Обнаружено более 20 программ !");
		PPWait(1);
	}
	if(ret != NOT) {
		StringListBoxDef * dbx = new StringListBoxDef(MAX_PATH_SZ, lbtDisposeData);
		if(dbx == NULL) {
			SetErr(ERR_NOT_MEM,0);
			CALLEXCEPT();
		}
		PPWaitMsg("Проверка номеров версий ...");
		p = 0;
		strcpy(buf,"<< Новая установка >>");
		dbx->addItem(p++, buf);
		for(i = 0; i < n; i++) {
			FILE * fil = 0;
			long begfstr;
			THROW(ChgDir(OldPaths[i]));
			if((fil = fopen("pp.exe", "rb")) == NULL) {
				SetErr(ERR_OPEN_FILE, "pp.exe");
				CALLEXCEPT();
			}
			begfstr = SearchStrInFile(fil, 0L, "(c) Andrey Fokin, Anton Sobolev", 0);
			if(begfstr >= 0) {
				fseek(fil, begfstr, SEEK_SET);
				while(getc(fil) != 0);
				fread(&ver, sizeof(uint16), 1, fil);
				fread(&rev, sizeof(uint16), 1, fil);
				sprintf(bufv, "%d.%d.%d", HiByte(ver), LoByte(ver), rev);
				sprintf(buf, "%11s - %s", bufv, OldPaths[i]);
				if(strcmp((char *)(buf+strlen(buf)-3), "SAV") !=0)
					dbx->addItem(p++, buf);
			}
			else {
				PPVersionInfo ppver("pp.exe");
				if(ppver.GetVersionText(bufv, sizeof(bufv)))
					sprintf(buf, "%11s - %s", bufv, OldPaths[i]);
				else {
					bufv[0] = 0;
					sprintf(buf, "%11s   %s", bufv, OldPaths[i]);
				}
				dbx->addItem(p++, buf);
			}
			if(fil)
				fclose(fil);
		}
		THROW(ChgDir(InstTmpPath));
		ListWindow * lw = new ListWindow(TRect(15,7,65,18), dbx, "Найденные версии программ", 0);
		if(lw == NULL) {
			SetErr(ERR_NOT_MEM,0);
			CALLEXCEPT();
		}
		PPWait(0);
		ret = ERR;
		do {
			if(ExecView(lw) == cmOK) {
				SString cur_str;
				lw->getResult(&r);
				lw->listBox()->getCurString(cur_str);
				if(r > 0) {
					OldVer.Flag = 1;
					strcpy(OldVer.PathRoot, ((const char *)cur_str)+14);
					cur_str.Trim(11).Strip();
					strcpy(OldVer.Ver, cur_str);
					for(i = strlen(OldVer.PathRoot); i >= 0; i--)
						if(OldVer.PathRoot[i] == '\\') {
							OldVer.PathRoot[i] = 0;
							break;
						}
					strcpy(InstPaths[P_ROOT], OldVer.PathRoot);
					THROW(ChgDir(InstPaths[P_ROOT]));
					strcpy(OldVer.OldArcPath, InstPaths[P_ROOT]);
					if(FindFileInCurDir(OldVer.OldArcPath, ARCFILENAME)==OK) {
						THROW(ChgDir(OldVer.OldArcPath));
						OldVer.OldArcFlag=1;
						remove(ARCFILEOLD);
						if(copyFileByName(ARCFILENAME, ARCFILEOLD)==ERR) {
						   	SetErr(ERR_COPY_FILE, ARCFILENAME);
							CALLEXCEPT();
						}
					}
					THROW(ChgDir(InstPaths[P_ROOT]));
					strcpy(OldVer.LogPath, InstPaths[P_ROOT]);
					if(FindFileInCurDir(OldVer.LogPath, LOGNAME)==OK) {
						THROW(ChgDir(OldVer.LogPath));
						OldVer.LogFlag=1;
						setLastSlash(strcpy(buf, InstTmpPath));
						strcat(buf, LOGTMPNAME);
						if(copyFileByName("install.log", buf)==ERR) {
						   	SetErr(ERR_COPY_FILE, LOGNAME);
							CALLEXCEPT();
						}
					}
				}
				THROW(ChgDir(InstTmpPath));
				ret = OK;
			}
			else {
				THROW(YN(0));
			}
		} while(ret == ERR);
	}
	for(i = 0; i < 20; i++) {
		delete OldPaths[i];
		OldPaths[i] = 0;
	}
	THROW(ChgDir(InstTmpPath));
	ret = OK;
	CATCH
		ret = ERR;
	ENDCATCH
	return ret;
}

int Install::GetMode()
{
	TDialog *dlg = 0;
	uint m=0;
	int ok=OK, r;
	if(!CheckDialogPtr(&(dlg = new TDialog(DLG_INSTMODE)))) {
		Smode=NOSET;
		SetErr(ERR_LOAD_DIALOG,0);
		CALLEXCEPT();
	}
	dlg->setCtrlData(CTL_INSTMODE_OLD_VERSION, " Нет ");
	dlg->setCtrlData(CTL_INSTMODE_NEW_VERSION, NewVersion);
	dlg->disableCtrl(CTL_INSTMODE_OLD_VERSION, 1);
	dlg->disableCtrl(CTL_INSTMODE_NEW_VERSION, 1);
	dlg->selectCtrl(CTL_INSTMODE_MODE);
	dlg->setCtrlData(CTL_INSTMODE_MODE,&m);
	if(OldVer.Flag) {
		dlg->setCtrlData(CTL_INSTMODE_OLD_VERSION, OldVer.Ver);
		if(!OldVer.LogFlag)
			dlg->disableCtrl(CTL_INSTMODE_MODE,1);
	}
	else
		dlg->disableCtrl(CTL_INSTMODE_MODE,1);
	r=ERR;
	do {
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_INSTMODE_MODE,&m);
			if(m==0)Smode=NEWPROGRAMM;
			if(m==1)Smode=UNDO;
			r=OK;
		}
		else
			THROW(YN(0));
	} while(r==ERR);
	THROW(ChgDir(InstTmpPath));
	ok=OK;
	CATCH
	   	ok=ERR;
	ENDCATCH
	TObject::destroy(dlg);
	return ok;
}

int Install::GetPaths()
{
	TDialog *dlg = 0;
	int i=0, m=0, ok=OK, r;
	if(OldVer.Flag)
		return OK;
	if(!CheckDialogPtr(&(dlg = new TDialog(DLG_INSTPATH)))) {
		SetErr(ERR_LOAD_DIALOG,0);
		CALLEXCEPT();
	}
	dlg->setCtrlData(CTL_INSTPATH_ROOT, InstPaths[P_ROOT]);
	r = ERR;
	do {
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_INSTPATH_ROOT, InstPaths[P_ROOT]);
			strip(InstPaths[P_ROOT]);
			if(InstPaths[P_ROOT][1]!=':') {
				THROW(ChgDir(InstTmpPath));
				if(InstPaths[P_ROOT][0]=='\\')
					m=2;
				else
					m = 3;
				for(i=strlen(InstPaths[P_ROOT])-1; i>=0; i--)
					InstPaths[P_ROOT][i+m]=InstPaths[P_ROOT][i];
				InstPaths[P_ROOT][0]=getdisk()+'A';
				InstPaths[P_ROOT][1]=':';
				InstPaths[P_ROOT][2]='\\';
			}
			r = OK;
		}
		else
			THROW(YN(0));
	} while(r == ERR);
	THROW(ChgDir(InstTmpPath));
	ok = OK;
	CATCH
		ok = ERR;
	ENDCATCH
	TObject::destroy(dlg);
	return ok;
}

int Install::OpenLogFile()
{
	int ok=OK;
	char buf[MAX_PATH_SZ], buf1[MAX_PATH_SZ];
	if(OldVer.LogFlag) {
		sprintf(buf,"%s\\%s", InstTmpPath, LOGTMPNAME);
		sprintf(buf1,"%s\\%s", OldVer.LogPath, LOGNAME);
		if(copyFileByName(buf1, buf)==ERR) {
			SetErr(ERR_COPY_FILE, buf1);
			CALLEXCEPT();
		}
	}
	THROW(ChgDir(InstTmpPath));
	if((logfile=fopen(LOGTMPNAME,"a+t"))==NULL) {
		SetErr(ERR_OPEN_FILE,0);
		CALLEXCEPT();
	}
	fseek(logfile,0L, SEEK_END);
	logstartpos=ftell(logfile);
	if(Smode==NEWPROGRAMM)
		THROW(AddLogFile(LOG_START,0,0));
	THROW(ChgDir(InstTmpPath));
	ok=OK;
	CATCH
		ok=ERR;
	ENDCATCH
	return ok;
}

int Install::CloseLogFile(int mode)
{
	char buf[MAX_PATH_SZ], buf1[MAX_PATH_SZ];
	int ok;
	if(!logfile) {
		SetErr(ERR_FILE_NOT_OPEN,0);
		CALLEXCEPT();
	}
	if((mode==OK)&&(Smode==NEWPROGRAMM)) {
	 	if(!OldVer.LogFlag) {
			sprintf(buf,"%s\\%s", InstPaths[P_ROOT], InstPaths[P_LOG]);
			THROW(AddLogFile(LOG_MAKEFILE, buf, LOGNAME));
		}
		THROW(AddLogFile(LOG_STOP,0,0));
	}
	if(logfile)
		fclose(logfile);
	if(mode == OK) {
		sprintf(buf1,"%s\\%s", InstTmpPath, LOGTMPNAME);
		if(OldVer.LogFlag) {
			sprintf(buf,"%s\\%s", OldVer.LogPath, LOGNAME);
			if(copyFileByName(buf1, buf)==ERR) {
				SetErr(ERR_COPY_FILE, buf1);
				CALLEXCEPT();
			}
		}
		else{
			sprintf(buf,"%s\\%s", InstPaths[P_ROOT], InstPaths[P_LOG]);
			if(ChgDir(buf)==OK) {
				strcat(buf,"\\");
				strcat(buf, LOGNAME);
				if(copyFileByName(buf1, buf)==ERR) {
					SetErr(ERR_COPY_FILE, buf1);
					CALLEXCEPT();
				}
			}
		}
	}
	THROW(ChgDir(InstTmpPath));
	remove(LOGTMPNAME); // Удалить временные файлы
	ok=OK;
	CATCH
		ok=ERR;
	ENDCATCH
	return ok;
}

int Install::SetNewProgramm()
{
	char buf[MAX_PATH_SZ], buf1[MAX_PATH_SZ], ndbuf[256], num[16];
	char pbuf[MAX_PATH_SZ], PathSave[MAX_PATH_SZ];
	char arhfile[MAX_PATH_SZ], extfile[32];

	uint i=0, j=0;
	int  m=0, ok=OK;
	StringSet s(";"), param(",");
	FILE * f = 0;
	struct ffblk fd;

	PPWait(1);
	THROW(ChgDir(InstTmpPath));
	// Структура каталогов
	THROW(MakeDir(InstPaths[P_ROOT]));
	for(i = 1; i < NDIRS; i++) {
		sprintf(buf, "%s\\%s", InstPaths[P_ROOT], InstPaths[i]);
		THROW(MakeDir(buf));
		UndoFlag = 1;
	}
   	// Каталог SAVE
	sprintf(PathSave, "%s\\SAVE", InstPaths[P_ROOT]);
	THROW(MakeDir(PathSave));
	strcat(PathSave,"\\");
	strcat(PathSave, ARCFILENAME);
	// Разархивировать требуемые файлы
	PPWait(0);
	THROW(ChgDir(InstTmpPath));
	TScreen::clearScreen();
	inifile->GetEntries("Files", &s);
	for(i = 0; s.get(&i, extfile, sizeof(extfile));) {
		char ovrwrtParam[8];
		int  dontOvrwrt = 0; // Не переписывать файл
		int  asGoodAs   = 0; // Файл не в архиве. Переписать с дистрибутива как есть
		inifile->GetParam("Files", extfile, buf, sizeof(buf));
		param.setBuf(buf, strlen(buf)+1);
		j = 0;
		param.get(&j, buf1, sizeof(buf1));

		if(!param.get(&j, arhfile, sizeof(arhfile)) || *strip(arhfile) == 0) {
			asGoodAs = 1;
			strcpy(arhfile, extfile);
		}
		if(param.get(&j, ovrwrtParam, sizeof(ovrwrtParam))) {
			strip(ovrwrtParam);
			if(toupper(ovrwrtParam[0]) == 'N')
				dontOvrwrt = 1;
		}

		inifile->GetParam("Arcs", arhfile, num, sizeof(num));
		sprintf(ndbuf,"\n\003Файл %s не обнаружен. \n\03Вставте дискету с номером %s.", arhfile, num);
		THROW(ChgDir(InstCurPath));
		while(!fileExists(arhfile)) {
			char alt_path[MAXPATH];
			if(num[0] == '#') {
				sprintf(alt_path, "..\\disk%s\\%s", num+1, arhfile);
				if(fileExists(alt_path)) {
					strcpy(arhfile, alt_path);
					break;
				}
			}
			APPL->redraw();
			if(PPMessage(mfWarn|mfOK|mfCancel, 0, ndbuf) != cmOK)
				THROW(YN(0));
			TScreen::clearScreen();
		}
		sprintf(buf, "%s\\%s", InstCurPath, arhfile);
		strcpy(arhfile, buf);
		sprintf(pbuf, "%s\\", InstPaths[P_ROOT]);
		for(m = 0; m < NDIRS; m++)
			if(stricmp(DirParam[m], buf1) == 0)
				break;
		if(m >= NDIRS) {
			SetErr(ERR_INVINIPATH, buf1);
			CALLEXCEPT();
		}
		strcat(pbuf, InstPaths[m]);
		THROW(ChgDir(pbuf));
		if(fileExists(extfile)) { // Если файл существует
			if(dontOvrwrt)
				continue;
			int l = strlen(pbuf);
			THROW(PackFile(PathSave, extfile));
			strcat(pbuf, "\\");
			strcat(pbuf, extfile);
			THROW(AddLogFile(LOG_ARC, pbuf, PathSave));
			pbuf[l] = 0;
			if(::remove(extfile) == -1) { // Удалить extfile;
				SetErr(ERR_DEL_FILE, extfile);
				CALLEXCEPT();
			}
		}
		THROW(ChgDir(InstTmpPath));
		if(asGoodAs) {
			THROW(ChgDir(pbuf));
			if(copyFileByName(arhfile, extfile) == ERR) {
				SetErr(ERR_COPY_FILE, InstFiles[i]);
				CALLEXCEPT();
			}
		}
		else {
			THROW(UnpackFile(arhfile, pbuf, extfile));
		}
		THROW(AddLogFile(LOG_MAKEFILE, pbuf, extfile));
	}
	APPL->redraw();
	PPWait(1);
	// Создать файлы PPSTART.BAT  PP.INI  LOCAL.ALD  LOCAL.RPT
	sprintf(pbuf, "%s\\%s", InstPaths[P_ROOT], InstPaths[P_BIN]);
	THROW(ChgDir(InstPaths[P_ROOT]));
	if(fileExists("ppstart.bat") == ERR) {
		if((f = fopen("ppstart.bat", "wt")) == NULL) {
	   		SetErr(ERR_OPEN_FILE, "ppstart.bat");
			CALLEXCEPT();
		}
		THROW(AddLogFile(LOG_MAKEFILE, InstPaths[P_ROOT],"ppstart.bat"));
		strcpy(buf, "PUBLIC");
		PPWaitMsg("Поиск каталога Brequest...");
		if(FindFile("*", buf, "brequest.exe")==OK) {
			fprintf(f,"path=%%path%%;%s\n", buf);
			fprintf(f,"brequest\n%s\\pp.exe\nbrequtil -stop\n", pbuf);//BIN
		}
		else {
			strcpy(buf, "");
			PPWaitMsg("Поиск каталога Btrieve...");
			if(FindFile("*", buf, "btrieve.exe")==OK) {
				fprintf(f,"path=%%path%%;%s\n", buf);
				fprintf(f,"btrieve\n%s\\pp.exe\nbutil -stop\n", pbuf);
			}
			else {
				PPMessage(mfInfo|mfOK, 0, "\003Не обнаружена установка Btrieve\n\003Сконфигурируйте ppstart.bat самостоятельно !");
				fprintf(f,"rem path=%%path%%;%s\n", buf);
				fprintf(f,"rem btrieve\n%s\\pp.exe\nrem butil -stop\n", pbuf);
			}
		}
		fclose(f);
	}
	// PP.INI
	sprintf(pbuf,"%s\\%s", InstPaths[P_ROOT], InstPaths[P_BIN]);
	THROW(ChgDir(pbuf));
	f = 0;
	if(findfirst("pp.ini", &fd, 0) != 0) {
		if((f = fopen("pp.ini", "wt")) == NULL) {
			SetErr(ERR_OPEN_FILE, "pp.ini");
			CALLEXCEPT();
		}
		sprintf(pbuf, "%s\\%s", InstPaths[P_ROOT], InstPaths[P_SYS]);
		fprintf(f, "[path]\nsys=%s\n", pbuf);
		sprintf(pbuf, "%s\\%s", InstPaths[P_ROOT], InstPaths[P_DAT]);
		fprintf(f, "dat=%s\n", pbuf);
		sprintf(pbuf, "%s\\%s", InstPaths[P_ROOT], InstPaths[P_TEMP]);
		fprintf(f, "temp=%s\n", pbuf);
		sprintf(pbuf, "%s\\%s", InstPaths[P_ROOT], InstPaths[P_ARC]);
		fprintf(f, "arc=%s\n\n[dbname]\n", pbuf);
		fclose(f);
		sprintf(pbuf, "%s\\%s", InstPaths[P_ROOT], InstPaths[P_BIN]);
		THROW(AddLogFile(LOG_MAKEFILE, pbuf, "pp.ini"));
	}
	// LOCAL.ALD
	sprintf(pbuf,"%s\\DD", InstPaths[P_ROOT]);
	THROW(ChgDir(pbuf));
	f = 0;
	if(findfirst("local.ald", &fd, 0) != 0) {
		if((f = fopen("local.ald", "wt")) == NULL) {
			SetErr(ERR_OPEN_FILE, "local.ald");
			CALLEXCEPT();
		}
		fprintf(f, "IMPORT dd.ald\n");
		fclose(f);
		THROW(AddLogFile(LOG_MAKEFILE, pbuf, "local.ald"));
	}
	// LOCAL.RPT
	sprintf(pbuf,"%s\\DD", InstPaths[P_ROOT]);
	THROW(ChgDir(pbuf));
	f = 0;
	if(findfirst("local.rpt", &fd, 0) != 0) {
		if((f = fopen("local.rpt", "wt")) == NULL) {
			SetErr(ERR_OPEN_FILE, "local.rpt");
			CALLEXCEPT();
		}
		fprintf(f, "\n");
		fclose(f);
		THROW(AddLogFile(LOG_MAKEFILE, pbuf, "local.rpt"));
	}
	THROW(ChgDir(InstTmpPath));
	ok = OK;
	CATCH
		ok = ERR;
	ENDCATCH
	APPL->redraw();
	PPWait(0);
	return ok;
}

int Install::AddLogFile(int ID, char * param1, char * param2)
{
	LDATE  dt;
	LTIME  tm;
	char * ptr = 0;
	char   comment[64], timebuf[16], datebuf[16];
	getcurdatetime(&dt, &tm);
	datefmt(&dt, DATF_DMY, datebuf);
	timefmt(tm, TIMF_HMS, timebuf);
	switch(ID) {
		case LOG_START:    ptr = "StartInstallation"; break;
		case LOG_NEWVER:   ptr = "Version";           break;
		case LOG_MAKEDIR:  ptr = "CreateDir";         break;
		case LOG_MAKEFILE: ptr = "CopyFile";          break;
		case LOG_ARC:      ptr = "BackupFile";        break;
		case LOG_UNDO:     ptr = "Rollback";          break;
		case LOG_STOP:     ptr = "EndInstallation";   break;
		default: SetErr(ERR_LOG_ID,0);
	}
	if(ptr)
		strcpy(comment, ptr);
	if(ID == LOG_START || ID == LOG_UNDO || ID==LOG_STOP)
		fprintf(logfile,"%d,%s, Date: %s, Time: %s\n",
			ID, comment, datebuf, timebuf);
	else {
		if(param1 == NULL)
			param1 = "";
		if(param2 == NULL)
			param2 = "";
		fprintf(logfile, "%d,%s,%s,%s\n", ID, comment, param1, param2);
	}
	return OK;
}

int Install::YN(char *str)
{
	int  ok;
	char buf[300];
	if(str == NULL || str[0] == 0)
		strcpy(buf,"\n\003Вы действительно хотите прервать процесс ?");
	else {
		strcpy(buf,"\n\003");
		strcat(buf, str);
	}
	ok = OK;
	if(PPMessage(mfWarn|mfOK|mfCancel,0, buf) == cmOK) {
		SetErr(ERR_USER_BREAK,0);
		ok = ERR;
	}
	return ok;
}
//
int Install::GetLastStrLOG(StringSet *s)
{
	char buf[MAX_PATH_SZ*3];
	long startpos, endpos;
	int  c, ok=OK;
	while(1) {
		if(logstartpos <= 0) {
			SetErr(ERR_LOG_STRUCT, 0);
			CALLEXCEPT();
		}
		startpos = endpos = logstartpos;
		do {
			fseek(logfile, endpos, SEEK_SET);
			c = getc(logfile);
			endpos--;
		} while(c == '\n' && endpos >= -1);
		if(endpos < -1) {
			SetErr(ERR_LOG_STRUCT,0);
			CALLEXCEPT();
		}
		endpos++;
		startpos = endpos;
		do {
			fseek(logfile, startpos, SEEK_SET);
			c = getc(logfile);
			startpos--;
		} while(c != '\n' && startpos >= 0);
		startpos += 2;
		logstartpos = startpos-1;
		fseek(logfile, startpos, SEEK_SET);
		fread(buf, (int)(endpos-startpos+1),1, logfile);
		buf[(int)(endpos-startpos+1)] = 0;
		strip(buf);
		if(buf[0] != 0) {
			s->setBuf(buf, strlen(buf)+1);
			break;
		}
	}
	ok = OK;
	CATCH
		ok = ERR;
	ENDCATCH
	return ok;
}

int Install::Undo(int mode)
{
	char buf[MAX_PATH_SZ], buf1[MAX_PATH_SZ], buf2[MAX_PATH_SZ], name[25], ext[5];
	StringSet s(",");
	uint j;
	int  ID, ok = OK, i = 0;
	TScreen::clearScreen();
	UndoFlag=0;
	SetErr(OK,0);
	THROW(ChgDir(InstTmpPath));
	fclose(logfile);
	if((logfile = fopen(LOGTMPNAME, "rt")) == NULL) {
		SetErr(ERR_OPEN_FILE, 0);
		CALLEXCEPT();
	}
	fseek(logfile,0L, SEEK_END);
	logstartpos=ftell(logfile)-1;
	do {
		THROW(GetLastStrLOG(&s));
		j = 0;
		if(!s.get(&j, buf, sizeof(buf)))
			ID = NOT;
		else
			ID = atoi(buf);
		if(!s.get(&j, buf, sizeof(buf)))
			ID = NOT;
		switch(ID) {
			case LOG_START		:
				if(mode==OK) {
					fseek(logfile,0, SEEK_END);
					THROW(AddLogFile(LOG_UNDO,0,0));
				}
				SetErr(NOT,0);
				break;
			case LOG_NEWVER	:// Пропустить
				SetErr(OK,0);
				break;
			case LOG_MAKEDIR	:// Удалить последний каталог в пути
				if(!s.get(&j, buf, sizeof(buf))) {
					SetErr(ERR_STR_LOG,0);
					break;
				}
				if(rmdir(buf)==-1) {
					SetErr(ERR_DEL_DIR, buf);
					break;
				}
				break;
			case LOG_MAKEFILE	:// Удалить файл в данном каталоге
				if(!s.get(&j, buf, sizeof(buf))) {
					SetErr(ERR_STR_LOG,0);
					break;
				}
				if(!s.get(&j, buf1, sizeof(buf1))) {
					SetErr(ERR_STR_LOG,0);
					break;
				}
				strcat(buf, "\\");
				strcat(buf, buf1);
				if(remove(buf)==-1) {
					SetErr(ERR_DEL_FILE,0);
					break;
				}
				break;
			case LOG_ARC		:// файл из архива
				if(!s.get(&j, buf, sizeof(buf))) {
					SetErr(ERR_STR_LOG,0);
					break;
				}
				if(!s.get(&j, buf1, sizeof(buf1))) {
					SetErr(ERR_STR_LOG,0);
					break;
				}
				fnsplit(buf, NULL, NULL, name, ext);
				for(i = strlen(buf); i >= 0; i--)
					if(buf[i] == '\\') {
						buf[i] = 0;
						break;
					}
				strcat(name, ext);
				if(!UnpackFile(buf1, buf, name))
					break;
				break;
			case LOG_UNDO		:// Откат уже был
				if(!s.get(&j, buf, sizeof(buf))) {
					SetErr(ERR_STR_LOG, 0);
					break;
				}
				if(!s.get(&j, buf1, sizeof(buf1))) {
					SetErr(ERR_STR_LOG, 0);
					break;
				}
				APPL->redraw();
				sprintf(buf2, "\003Откат уже был: \n\003%s\n\003%s", buf, buf1);
				PPMessage(mfWarn|mfOK,0, buf2);
				SetErr(NOT,0);
				mode=ERR;
				break;
			case LOG_STOP		:// Пропустить
				break;
			default:
				break;
		}
		if(errcode == NOT)
			break;
		if(errcode != OK) {
			strcat(errstr, "\n\n\003пропустить ошибку ?");
			APPL->redraw();
			THROW(PPMessage(mfError|mfOK|mfCancel, 0, errstr) == cmOK);
			SetErr(OK, 0);
			TScreen::clearScreen();
		}
	} while(1);
	if(mode == OK) {
		THROW(ChgDir(InstTmpPath));
		fclose(logfile);
		if((logfile = fopen(LOGTMPNAME,"at")) == NULL) {
			SetErr(ERR_OPEN_FILE, LOGTMPNAME);
			CALLEXCEPT();
		}
		THROW(AddLogFile(LOG_UNDO,0,0));
	}
	ok = OK;
	CATCH
		ok = ERR;
	ENDCATCH;
	APPL->redraw();
 	return ok;
}

int Install::ExecFiles()
{
	char buf[MAX_PATH_SZ], name[MAX_PATH_SZ];
	StringSet s(",");
	uint j  = 0;
	int  ok = OK;
	if(Smode == NEWPROGRAMM && OldVer.Flag) {
		inifile->GetParam("Exec", "mkmenu", buf, sizeof(buf));
		s.setBuf(buf, strlen(buf)+1);
		s.get(&j, name, sizeof(name));
		s.get(&j, buf, sizeof(buf));
		strip(buf);
		if(buf[0] == '@') {
			for(int m = 0; m < NDIRS; m++)
				if(strcmp(DirParam[m], buf) == 0)
					break;
			sprintf(buf,"%s\\%s", InstPaths[P_ROOT], InstPaths[m]);
		}
		THROW(ChgDir(buf));
		TScreen::clearScreen();
		if(system(name) == -1) {
			SetErr(ERR_EXEC, name);
			CALLEXCEPT();
		}
		THROW(ChgDir(InstTmpPath));
	}
	CATCH
		ok = ERR;
	ENDCATCH
	APPL->redraw();
	return ok;
}

char * SLAPI PPGetVersion(char * buf)
{
	sprintf(buf, "%d.%d.%d", HiByte(PPInstVer), LoByte(PPInstVer), PPInstRevision);
	return buf;
}

IMPL_HANDLE_EVENT(PPInstApp)
{
	TApplication::handleEvent(event);
	if(TVCOMMAND)
		if(TVCMD == cmInit)
			install();
		else
			return;
}

PPInstApp::PPInstApp() : TProgInit(PPInstApp::initStatusLine, PPInstApp::initMenuBar, PPInstApp::initDeskTop)
{
	TEvent event;
	putEvent(event.setCmd(cmInit, 0));
}

void PPInstApp::install()
{
	Install inst;
	inst.StartInstall();
	message(this, evCommand, cmQuit,0);
}

int IsCDROMDrive(int drv /* 0 'A', 1 - 'B' ... */)
{
	REGS  in,out;
	SREGS seg;
	int   ndrv;
	char  drvs[26];
	in.x.ax = 0x1500;               // получить кол-во устройств CD
	in.x.bx = 0x0000;
	int86(0x2F, &in, &out);
	if((ndrv=out.x.bx) > 0) {       // если кол-во устройств > 0
		in.x.ax = 0x150D;           // получить строку номеров устройств
		seg.es  = FP_SEG(drvs);     // ES:BX - указатель на строку
		in.x.bx = FP_OFF(drvs);
		int86x(0x2F, &in, &out, &seg);
		for(int i = 0; i < ndrv; i++)
			if(drvs[i] == drv)
				return 1;
	}
	return 0;
}

int main()
{
	RegisterBIST();
	PPInstApp app;
	app.run();
	TScreen::clearScreen();
	return 0;
}
