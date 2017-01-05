// RPTCREAT.CPP
// Copyright (c) A.Sobolev 1996-1999, 2001, 2010
//
#include "_report.h"
#include <tv.h>
#include <dir.h>
#include <lex.h>
#include "_rpty.h"

int  PPErrCode = 0;
char ResFileName[MAXPATH]; // Used in RPTY.Y

// @Stub
int SLAPI PPSetAddedMsgString(const char * /*pStr*/)
{
	//AddedMsgString = pStr;
	return 1;
}

struct RES_DATA {
	uint ID;
	char * Name;
};

static FILE     * RcFile = 0;
static RES_DATA * P_ResData = 0;
static uint       ResCnt = 0;

static int SLAPI ReleaseResData()
{
	if(ResCnt) {
		for(uint i = 0; i < ResCnt; i++)
			delete P_ResData[i].Name;
		ZFREE(P_ResData);
	}
	return 1;
}

int SLAPI SReport::addField(int id, TYPEID typ, long fmt, uint rptfmt, char * nam)
{
	fields = (Field *)realloc(fields, sizeof(Field) * (fldCount + 1));
	if(fields) {
		Field * f  = fields + fldCount;
		// @ {
		if(nam) {
			int l = strlen(nam) + 1;
			if((text = (char *)realloc(text, textlen + l)) != 0) {
				memcpy(text + textlen, nam, l);
				f->name = textlen;
				textlen += l;
			}
			else {
				textlen = 0;
				fldCount = 0;
				return 0;
			}
		}
		else
			f->name = -1;
		// } @
		f->id      = id;
		f->type    = typ;
		f->format  = fmt;
		f->fldfmt  = rptfmt;
		f->data    = 0;
		f->lastval = 0;
		return (++fldCount);
	}
	fldCount = 0;
	return 0;
}

int SLAPI SReport::setAggrToField(int fld, int aggr, int dpnd)
{
	int bt1, bt2, i, a;
	fld--;
	if(fld >= 0 && fld < fldCount) {
		Field * f = &fields[fld];
		bt1       = stbase(f->type);
		bt2       = stbase(fields[dpnd - 1].type);
		if((bt1 != BTS_INT && bt1 != BTS_REAL) || (bt2 != BTS_INT && bt2 != BTS_REAL))
			if(aggr != AGGR_COUNT)
				return 0;
		f->fldfmt |= FLDFMT_AGGR;
		for(i = 0, a = -1; i < agrCount && a == -1; i++)
			if(agrs[i].fld == (fld + 1))
				a = i;
		if(a == -1)
			if((agrs = (Aggr *) realloc(agrs, sizeof(Aggr) * (agrCount + 1))) != 0)
				a = agrCount++;
			else
				return (agrCount = 0);
		agrs[a].fld   = fld + 1;
		agrs[a].aggr  = aggr;
		agrs[a].dpnd  = dpnd;
		agrs[a].scope = -1;
		agrs[a].ptemp = 0;
		return 1;
	}
	return 0;
}

int SLAPI SReport::addText(char * txt)
{
	int l  = strlen(txt) + 1;
	fields = (Field *)realloc(fields, sizeof(Field) * (fldCount + 1));
	if(fields)
		if((text = (char *)realloc(text, textlen + l)) != 0) {
			memcpy(text + textlen, txt, l);
			Field * f  = fields + fldCount;
			f->id      = 0;
			f->name    = -1; // @
			f->type    = 0;
			f->len     = l - 1;
			f->fldfmt  = 0;
			f->offs    = textlen;
			f->lastval = 0;
			textlen   += l;
			return (++fldCount);
		}
		else {
			textlen = 0;
			return 0;
		}
	fldCount = 0;
	return 0;
}

int SLAPI SReport::addBand(SReport::Band * band, int *grp_fld, uint * pos)
{
	int i, j;
	bands = (Band *) realloc(bands, sizeof(Band) * (bandCount + 1));
	if(bands) {
		memcpy(bands + bandCount, band, sizeof(Band));
		if(band->kind == GROUP_HEAD) {
			if(grp_fld == 0 || grp_fld[0] == 0)
				return 0;
			groups = (Group *) realloc(groups, sizeof(Group) * (grpCount + 1));
			if(groups) {
				groups[grpCount].fields  = (int16 *) malloc((grp_fld[0] + 1) * sizeof(int));
				memcpy(groups[grpCount].fields, grp_fld, (grp_fld[0] + 1) * sizeof(int));
				groups[grpCount].lastval = 0;
				groups[grpCount].band    = bandCount;
				bands[bandCount].group   = grpCount;
				grpCount++;
			}
			else {
				grpCount = 0;
				return 0;
			}
		}
		else if(band->kind == GROUP_FOOT) {
			int d = 0, f = 0;
			for(j = 0, i = bandCount - 1; i >= 0; i--) {
				if(bands[i].kind == GROUP_FOOT)
					j++;
				else if(bands[i].kind == GROUP_HEAD) {
					if(d && j == 0) {
						j = i;
						f = 1;
						break;
					}
					j--;
				}
				else if(bands[i].kind == DETAIL_BODY)
					d = 1;
			}
			if(f) {
				bands[bandCount].options = bands[j].options;
				bands[bandCount].group   = bands[j].group;
			}
			else
				return 0;
		}
		if(band->kind == GROUP_HEAD || band->kind == GROUP_FOOT)
			for(i = 1; i <= band->fields[0]; i++)
				if(fields[band->fields[i] - 1].fldfmt & FLDFMT_AGGR)
					for(j = 0; j < agrCount; j++)
						if(agrs[j].fld == band->fields[i]) {
							if(agrs[j].scope < bands[bandCount].group)
								agrs[j].scope = bands[bandCount].group;
							break;
						}
		*pos = bandCount;
		bandCount++;
		return 1;
	}
	bandCount = 0;
	return 0;
}

int SLAPI SReport::Band::addField(int fid)
{
	int count = fields ? fields[0] : 0;
	fields = (int16 *) realloc(fields, sizeof(int) * (count + 2));
	if(fields) {
		fields[count + 1] = fid;
		fields[0] = count + 1;
		return 1;
	}
	return 0;
}

extern FILE * header_file;
extern int yyparse();

void write_header_file(SReport * rpt)
{
	static int counter = 1;
	char buf[64], buf1[64], nam[64];
	if(header_file && rpt) {
		sprintf(nam, "REPORT_%s", strupr(strcpy(buf, rpt->name)));
		fprintf(header_file, "\n#define %-32s %5d\n", nam, counter++);
		if(rpt->data_name == 0 || rpt->data_name[0] == 0)
			for(int i = 0; i < vartab.getCount(); i++)
				if((vartab.at(i).id != 0) && ((vartab.at(i).name[1] < '0') || (vartab.at(i).name[1] > '9'))) {
					strupr(strcpy(buf1, vartab.at(i).name + 1));
					sprintf(nam, "RPTF_%s_%s", buf, buf1);
					fprintf(header_file, "#\tdefine %-32s %5d\n", nam, vartab.at(i).id);
				}
	}
}

void SLAPI accept_report(SReport * rpt)
{
	uint res_no = 0;
	if(ResCnt == 0)
		write_header_file(rpt);
	if(RcFile) {
		if(ResCnt) {
			for(; res_no < ResCnt; res_no++)
				if(!stricmp(P_ResData[res_no].Name, rpt->name))
					break;
			if(res_no == ResCnt) {
				printf("Invalid name of resource \"%s\"\n", rpt->name);
				exit(-1);
			}
			res_no = P_ResData[res_no].ID;
		}
		if(!rpt->writeResource(RcFile, res_no)) {
			printf("Error writing resource\n");
			exit(-1);
		}
	}
}

#if 1

int main(int argc, char *argv[])
{
	if(argc < 2) {
		printf("Usage: rptgen src_file [/af:aldd_file] [/rf[:res_file]]\n");
		return -1;
	}
	char buf[80], drv[MAXDRIVE], dir[MAXDIR], nam[MAXFILE], ext[MAXEXT];
	RegisterBIST();
	strcpy(ResFileName, "ld.bin");
	if(argc > 2)
		for(int i = 2; i < argc; i++)
			if((argv[i][0] == '/' || argv[i][0] == '-')) {
				if(strnicmp(argv[i]+1, "RF", 2) == 0) {
					if(argv[i][3] == ':')
						replaceExt(strcpy(buf, argv[i]+4), ".RES", 0);
					else
						strcpy(buf, "pp.res");
					TVRez rez(buf);
					if(rez.error) {
						printf("Ошибка: невозможно открыть файл %s\n", buf);
						return -1;
					}
					ulong ofs = 0;
					uint  res_id;
					ReleaseResData();
					for(ResCnt = 0; rez.enumResources(TV_REPORT, &res_id, &ofs) > 0; ResCnt++) {
						P_ResData = (RES_DATA *)realloc(P_ResData, (res_id)*sizeof(RES_DATA));
						if(!P_ResData) {
							printf("Ошибка: недостаточно памяти\n");
							return -1;
						}
						P_ResData[ResCnt].ID = res_id;
						rez.findResource(res_id, TV_REPORT);
						buf[0] = 0;
						rez.getString(buf);
						P_ResData[ResCnt].Name = newStr(buf);
					}
				}
				else if(strnicmp(argv[i]+1, "AF:", 2) == 0)
					replaceExt(strcpy(ResFileName, argv[i]+4), ".BIN", 0);
			}
	replaceExt(strcpy(buf, argv[1]), ".RPT", 0);
	lexin = fopen(buf, "r");
	if(lexin == 0) {
		printf("Ошибка: невозможно открыть файл %s\n", buf);
		return -1;
	}
	if(ResCnt == 0) {
		replaceExt(buf, ".H", 1);
		header_file = fopen(buf, "w");
		if(header_file == 0) {
			printf("Ошибка: невозможно открыть файл %s\n", buf);
			return -1;
		}
		fprintf(header_file, "/*\n Сгенерирован программой RPTGEN\n*/\n");
	}
	replaceExt(buf, ".RC", 1);
	RcFile = fopen(buf, "w");
	if(RcFile == 0) {
		printf("Ошибка: невозможно открыть файл %s\n", buf);
		return -1;
	}
	fprintf(RcFile, "/*\n Сгенерирован программой RPTGEN\n*/\n");
	fnsplit(buf, drv, dir, nam, ext);
	if(ResCnt == 0) {
		fprintf(RcFile, "\n#include <tvdefs.h>\n\n");
		fprintf(RcFile, "\n#include <%s.h>\n\n", nam);
	}
	if(yyparse()) {
		printf("Ошибка разбора исходного файла\n");
		return -1;
	}
	fclose(RcFile);
	RcFile = 0;
	if(!ResCnt)
		fclose(header_file);
	fclose(lexin);
	ReleaseResData();
	return 0;
}

#endif // 0

#if 0

//
// DEBUG !!!
//

extern SReport *_report;
extern FILE *header_file;

struct BalItem {
	LDATE date;
	int ac;
	int sb;
	double dbt, crd, dbtRest, crdRest;
};

extern "C" LDBL round(LDBL n, int prec);

SArray * readList()
{
	char buf[64];
	LDBL v, r;
	LDBL ip;
	BalItem item;
	SArray *ary = new SArray(sizeof(BalItem), 1);
	FILE *f = fopen("bal.txt", "rt");
	if(f == 0)
		return 0;
	while(!feof(f)) {
		fscanf(f, "%s", buf);
		if(feof(f))
			break;
		strtodate(buf, DATF_DMY, &item.date);
		fscanf(f, "%Lf", &v);
		r = round(modfl(v, &ip) * (LDBL) 100., 0);
		item.sb = r;
		item.ac = ip;
		fscanf(f, "%lf%lf%lf%lf", &item.dbt, &item.crd,
			   &item.dbtRest, &item.crdRest);
		ary->insert(&item);
	}
	fclose(f);
	return ary;
}

static SArray *_array = 0;
static BalItem _item;

int _iterator(int first)
{
	static int count;
	if(first)
		count = 0;
	if(_array && (count < _array->getCount())) {
		_item = *(BalItem *) _array->at(count++);
		return 1;
	}
	else
		return -1;
}

#include "bal.h"

int _initData(SReport * rpt)
{
	static LDATE curdate;
	// static LTIME curtime;
	static LDATE beg, end;
	_array = readList();
	if(_array == 0)
		return 0;
	curdate = getcurdate_();
	rpt->setData(RPTF_BAL_1, &curdate);
	// rpt->setData(RPTF_BAL_2, &curtime);
	encodedate(1, 5, 1996, &beg);
	encodedate(12, 5, 1996, &end);
	rpt->setData(RPTF_BAL_3, &beg);
	rpt->setData(RPTF_BAL_4, &end);

	rpt->setData(RPTF_BAL_6, &_item.ac);
	rpt->setData(RPTF_BAL_11, &_item.sb);
	rpt->setData(RPTF_BAL_7, &_item.dbt);
	rpt->setData(RPTF_BAL_8, &_item.crd);
	rpt->setData(RPTF_BAL_9, &_item.dbtRest);
	rpt->setData(RPTF_BAL_10, &_item.crdRest);

	return 1;
}

int yyparse();

void main()
{
	RegisterBIST();
	lexin = fopen("balance.rpt", "r");
	header_file = fopen("balance.h", "w");
	FILE *rc = fopen("balance.rc", "w");
	yyparse();
	if(rc)
		_report->writeResource(rc);
	_report->iterator = _iterator;
	if(!_initData(_report))
		return;
	_report->print();

	SReport rpt("bal");
	if(rpt.readResource("balance.res", REPORT_BAL)) {
		rpt.iterator = _iterator;
		if(!_initData(&rpt))
			return;
		rpt.print();
	}

	fclose(header_file);
	fclose(lexin);
}

#endif // 0

