/*
	PPALDDC.H
*/
#ifndef _PPALDDC_
#define _PPALDDC_
#endif

#include <ppaldd.h>

#define TMP_SIZE    255

#ifndef MAXPATH
#	define MAXPATH     _MAX_PATH
#endif

//#define MAXFILE     _MAX_FNAME
//#define MAXDIR      _MAX_DIR
//#define MAXEXT      _MAX_EXT
//#define MAXDRIVE    _MAX_DRIVE
#define BUFF_SIZE   100000l

char PREFIX[8];
char PPFILTER[] = "PPViewFilt";

extern long yyline;
char tmpbuff[TMP_SIZE];
long datacnt = 0;
long varcnt = 0;
long itercnt = 1;
long ordercnt = 0;
long iter;

struct VARDEFS {
	long   ID;
	long   type;
	long   iterID;
	long   dataID;
	long   rel;
	long   node_offset;
	long   node_cnt;
	char   name[NAME_SIZE];
	char   alias[16];
	FormatSpec format;
};

struct ORDERDEFS {
	long   ID;
	long   iterID;
	long   dataID;
	char   name[NAME_SIZE];
};

struct ITERDEFS {
	long   ID;
	long   type;
	long   dataID;
	char   name[NAME_SIZE];
};

#define DATADECLF_DESTROY 0x0001L // declare "Destroy"
#define DATADECLF_DOSSTUB 0x0002L // declare "DOSSTUB"

struct DATADEFS {
	long   HasParent;
	long   ParentId;
	long   TypeId;
	long   DeclareFlags;
	char   Name[NAME_SIZE];
	char   ParentName[NAME_SIZE];
};

extern VARDEFS   * VarDefs;
extern ITERDEFS  * IterDefs;
extern DATADEFS  * DataDefs;
extern ORDERDEFS * OrderDefs;

void DataRedef(char* str, long);
void IterRedef(char* str, long dd);
void VarsRedef(char* str, long dd, long ii);

void WriteHFile(FILE * fs, long);
void WriteCppFile(FILE * fs, long);
void WriteFields(FILE * fs, long dID);
void WriteIterFields(FILE * fs, long dID, long iID);
long GetDataID(char * name, long);
long GetIterID(long dt, char * name);
int  GetVarID(char *, long, long);
void FillData(long);
NodeType GetNodeType(long opType, NodeType* args);
void SaveNode(NodeType);
FormatSpec SetFormat(char *, long);

void yyerror(char * str);
int  yylex(void);
