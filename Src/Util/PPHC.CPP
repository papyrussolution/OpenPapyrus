// PPHC.CPP
// Copyright (c) A.Sobolev 1998, 2007
// __SERG__
// Derived from Borland's Turbo Vision TVHC.CPP
// Copyright (c) 1991 by Borland International
//
#define Uses_TObject
#define Uses_TPoint
#include <tv.h>
#include <slib.h>
#include <string.h>
#include <limits.h>
#include <dos.h>
#include <dir.h>
#include <io.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <conio.h>
#include <helpbase.h>

const MAXSIZE    =   80;
const MAXSTRSIZE =  256;
const bufferSize = 4096;

typedef enum State { undefined, wrapping, notWrapping };
//
// Topic Reference
//
struct TReference {
	char * topic;
	int    resolved;
	int    value;
};

IMPL_CMPFUNC(TReference,i1,i2)
{
	char *s1 = ((TReference *)i1)->topic;
	char *s2 = ((TReference *)i2)->topic;
	return strcmp(s1,s2);
}

class TRefTable : public SArray {
public:
	SLAPI TRefTable();
	virtual void SLAPI freeItem(void *item);
	TReference * SLAPI getReference(char *topic);
};

#define ENDTEXTSYM	26

TRefTable * refTable = 0;
char tline[MAXSTRSIZE] = "";
int  tlineFlag = 0;
int  lineCount = 0;
int  valueRef = 3;
char textName[MAXPATH], helpName[MAXPATH];

//_OP

SLAPI TRefTable::TRefTable() : SArray(sizeof(TReference),1)
{
}

void SLAPI TRefTable::freeItem(void *item)
{
	TReference * ref = (TReference *)item;
	delete ref->topic;
}

TReference * SLAPI TRefTable::getReference(char *topic)
{
	TReference * ref = 0;
	TReference   cmpi;
	cmpi.topic = topic;
	uint i = 0;
	if(lsearch(&cmpi, &i, PTR_CMPFUNC(TReference))) //bsearch
		ref = (TReference *) at(i);
	else {
		ref = new TReference;
		ref->topic = newStr(topic);
		ref->resolved = 0;
		insert(ref);
		ref = (TReference *) at(getCount()-1);
//		ordInsert(ref, 0, PTR_CMPFUNC(TReference));
	}
	return ref;
}

int initRefTable()
{
	return ((refTable = new TRefTable)==NULL) ? 0 : 1;
}

void ERROR(char *str)
{
	printf("ERROR:\tLineCount=%d\n\tTextFile=%s\n\tHelpFile=%s\n\t%s\n", lineCount, textName, helpName, str);
	exit(1);
}

void WARNING(char *str)
{
	printf("WARNING:\tLineCount=%d\n\tTextFile=%s\n\tHelpFile=%s\n\t%s\n", lineCount, textName, helpName, str);
}

void readLine(FILE *f)
{
	int c = 0,i = 0;
	if(tlineFlag)
		return;
	tline[0] = 0;
	while((c = fgetc(f)) != '\n' && !feof(f) && i < (MAXSTRSIZE-1))
   		tline[i++] = c;
	tline[i]  = 0;
	tlineFlag = 1;
	lineCount++;
}

void readLineDelVoid(FILE *f)
{
	if(!tlineFlag)
		do {
			tlineFlag = 0;
			readLine(f);
		} while(tline[0] == 0 && !feof(f));
}

void getWord(char * word, int *i)
{
	int j;
	int p = *i;
	char * strptr;
	int tlen = strlen(tline);
	while(p <= tlen && tline[p] == ' ' || tline[p] == 8)
		p++;
	j = p;
	if(j > tlen)
		word[0] = 0;
	else {
		p++;
		if(isalnum(tline[j]) || tline[j]=='_')
			while(p <= tlen && (isalnum(tline[p]) || tline[p] == '_'))
				p++;
		strptr = tline + j;
		strncpy(word, strptr, p - j);
		word[p-j] = 0;
	}
	*i = p;
}

void scanCrossRef(THelpTopic *topic,int offset)
{
	char buf[MAXSTRSIZE],errstr[80];
	TCrossRef ref;
	TReference *reference;
	int j = 0, i = 0, l = strlen(tline), sbeg, sr, send;
	while(i < l) {
		if(tline[i] == '{') {
			sbeg = i;
			for(j = sbeg; j < l; j++)
				tline[j] = tline[j+1];
			j = i;
			l--;
			while(tline[j] != ':' && j < l)
				j++;
			if(j >= l)
				ERROR("Отсутствует разделитель ':'");
			sr = j++;
			getWord(buf, &j);
			while(tline[j] != '}' && j < l)
				j++;
			if(j >= l)
				ERROR("Отсутствует  '}'");
			send = j;
			for(j = sr; j < l; j++)
				tline[j] = tline[j+send-sr+1];
			l -= (send-sr+1);
			reference = refTable->getReference(buf);
			ref.ref = 0;
			ref.offset = offset+sbeg;
			ref.length = sr-sbeg;
			if(reference->resolved)
				ref.ref = reference->value;
			else{
				sprintf(errstr, "Ссылка на неопределенную константу %s.", buf);
				WARNING(errstr);
			}
			topic->addCrossRef(ref);
		}
		i++;
	}
}

void readReference(FILE *f)
{
	char buf[MAXSTRSIZE];
	int i = 0, l = 0;
	TReference * ref = 0;
	fseek(f, 0L, SEEK_SET);
	tlineFlag = 0;
	while(!feof(f)) {
		readLine(f);
		if(tline[0] == '.') {
			i = 1;
			l = strlen(tline);
		   	getWord(buf, &i);
			if(strcmp(strupr(buf), "TOPIC") != 0)
				ERROR("Неизвестная команда ");
			if(i >= l)
				ERROR("Отсутствует идентификатор");
			while(i < l) {
				getWord(buf, &i);
				if(buf[0] == 0)
					ERROR("Отсутствует идентификатор");
				ref = refTable->getReference(buf);
				if(ref->resolved == 0) {
					ref->resolved = 1;
					ref->value    = valueRef++;
				}
				else
					ERROR("Идентификатор определен более одного раза");
				getWord(buf, &i); // должна быть ','
			}
		}
		tlineFlag = 0;
	}
	fseek(f, 0L, SEEK_SET);
}

void readTopic(FILE *f, THelpFile *hfile)
{
	const size_t MAXBUF = 16U*1024U;
	char buf[MAXSTRSIZE];
	int  i = 0, l = 0, offset = 0;
	char * text = 0;
	TReference * reference = 0;
	THelpTopic * topic     = 0;
	TParagraph * par       = 0;
	TCrossRef    ref;
	//
	// Командная строка
	//
	readLineDelVoid(f);
	l = strlen(tline);
	getWord(buf, &i);
	if(buf[0] == ENDTEXTSYM || feof(f))
		return;
	text = (char *)calloc(1, MAXBUF);
	if(text == 0)
		ERROR("Недостаточно памяти для буфера параграфа");
	if(strcmp(buf, ".") != 0)
		ERROR("(проход 2) В командной строке отсутствует символ '.'");
   	getWord(buf, &i);
	if(strcmp(strupr(buf), "TOPIC") != 0)
		ERROR("(проход 2) Неизвестная команда ");
	if(i >= l)
		ERROR("(проход 2) Отсутствует идентификатор");
	while(i < l) {
		getWord(buf, &i);
		if(buf[0] == 0)
			break;
		reference = refTable->getReference(buf);
		hfile->recordPositionInIndex(reference->value);
		getWord(buf, &i); // должна быть ','
	}
	tlineFlag = 0;
	i = 0;
	//
	// Параграфы
	//
	topic = new THelpTopic;
	readLineDelVoid(f);
	offset = 0;
	int wrap = 0;
	while(tline[0] != '.' && tline[0] != ENDTEXTSYM && !feof(f)) {
		par = new TParagraph;
		par->next = 0;
		text[0]   = 0;
		par->wrap = (tline[0] == ' ') ? 0 : 1;
		scanCrossRef(topic, offset);
		if((strlen(text) + strlen(tline) + 16) >= MAXBUF)
			ERROR("Слишком большой параграф.");
		strcat(text, tline);
		if(par->wrap)
			strcat(text, " ");
		else
			strcat(text, "\n");
		offset += strlen(tline)+1;
		tlineFlag = 0;
		readLine(f);
		wrap = (tline[0] == ' ') ? 0 : 1;
		while(wrap == par->wrap && tline[0] != '.' && tline[0] != ENDTEXTSYM && !feof(f)) {
			// Поиск ссылок
			scanCrossRef(topic,offset);
			if((strlen(text) + strlen(tline) + 16) >=MAXBUF)
				ERROR("Слишком большой параграф.");
			strcat(text, tline);
			if(wrap)
				strcat(text, " ");
			else
				strcat(text, "\n");
			offset += strlen(tline)+1;
			tlineFlag = 0;
			readLine(f);
			wrap = (tline[0] == ' ') ? 0 : 1;
		}
		if(tline[0] != '.' && !feof(f))
			tlineFlag = 0;
		if(par->wrap) {
			strcat(text, "\n");
			offset++;
		}
		par->text = new char[strlen(text)+1];
		if(par->text == 0)
			ERROR("Недостаточно памяти для буфера параграфа");
		strcpy(par->text, text);
		par->size = strlen(text);
		topic->addParagraph(par);
	}
	hfile->putTopic(topic);
	delete topic;
	free(text);
}

char * HLPreplaceEXT(char *fileName, char *nExt, int force)
{
	char dir[MAXPATH];
	char name[MAXFILE];
	char ext[MAXEXT];
	char drive[MAXDRIVE];
	char buffer[MAXPATH];
	fnsplit(fileName, drive, dir, name, ext);
	if(force || (strlen(ext) == 0)) {
		sprintf(buffer, "%s%s%s", dir, name, nExt);
		return buffer;
	}
	else
		return fileName;
}

int fExists(char *fileName)
{
	struct ffblk ffblk;
	if(findfirst(fileName,&ffblk,0))
		return 0;
	else
		return 1;
}
//
// Write the .H file containing all screen titles as constants.
//
int findstr(char *str,char *cmpi)
{
	int ls = strlen(str), lc = strlen(cmpi), i = 0, j = 0, k;
	if(ls >= lc) {
		for(i = 0;i < ls-lc+1; i++) {
			k = 1;
			for(j = 0; j < lc; j++)
				if(str[i+j] != cmpi[j])
					k = 0;
			if(k)
				return 1;
		}
	}
	return 0;
}

void getTypeCTX(char * buf, char * str)
{
	if(findstr(str, "DLG"))
		strcpy(buf, "_DLG");
	else if(findstr(str, "BROWSE"))
		strcpy(buf, "_BROWSER");
	else if(str[0] == 'c' && str[1] == 'm')
		strcpy(buf, "_MENU");
	else
		sprintf(buf, "-1");
}

void writeCPPFile(FILE * cppFile, char */*hlpName*/)
{
	char   buf[16], *formstr = 0;
	char   line[256];
	FILE * datefile=0;
	TReference * p = 0;
	int c = 0;
	if((datefile = fopen("tvhc.dat","rt")) == NULL) {
		printf("\nНевозможно открыть файл данных TVHC.DAT.\n");
		return;
	}
	fseek(cppFile, 0,SEEK_SET);
	fseek(datefile,0,SEEK_SET);
	//
	// convert Table
	//
	c = fgetc(datefile);
	do {
		if(c == '@') {
			for(int i = 0; i < refTable->getCount(); i++) {
				p = (TReference *)refTable->at(i);
				if(p->resolved) {
					if(i == refTable->getCount()-1)
						formstr = "\t{%-30s,%7d,%-10s}\n";
					else
						formstr = "\t{%-30s,%7d,%-10s},\n";
					getTypeCTX(buf, p->topic);
					fprintf(cppFile, formstr, p->topic, p->value, buf);
				}
			}
		}
		else
			fputc(c, cppFile);
		c = fgetc(datefile);
	} while(!feof(datefile));
}
//
// Compile the given stream, and output a help file.                    //
//
void processText(FILE *textFile,FILE *helpFile,FILE *cppFile,char *hlpName)
{
	THelpFile * helpRez = new THelpFile(helpFile);
	initRefTable();
	tlineFlag = 0;
	lineCount = 0;
	readReference(textFile);
	tlineFlag = 0;
	lineCount = 0;
	while(!feof(textFile))
		readTopic(textFile, helpRez);
	writeCPPFile(cppFile, hlpName);
	delete helpRez;
}
//
// Check whether the output file name exists.  If it does, ask whether
// it's ok to overwrite it.
//
void checkOverwrite(char *fName)
{
	if(fExists(fName)) {
		printf("File already exists: %s.  Overwrite? (y/n) ",fName);
		char ch = getch();
		printf("%c\n",ch);
		if(toupper(ch) != 'Y')exit(1);
	}
}
//
//
//
void test(FILE *hfile)
{
	THelpFile *hf=new THelpFile(hfile);
	THelpTopic *topic;
	TParagraph *par;
//    int npar=0;
	TCrossRef *ref;
	unsigned char str[1024];
	unsigned char far *scr=(unsigned char far *)0xB8000000;
	for(int i=0;i<30;i++) {
		topic = hf->getTopic(i);
		topic->setWidth(25);
		par=topic->paragraphs;
		while(par) {
			strncpy(str,par->text,par->size);
			str[par->size]=0;
			for(int k=0;k<par->size;k++) {
				scr[k*2]=str[k];
				scr[k*2+1]=0x30;
			}
//	    	printf("WRAP:%d\nSIZE:%d\nTEXT:\n%s\n",par->wrap,par->size,str);
			par=par->next;
		}
		ref=topic->crossRefs;
		for(int j=0;j<topic->numRefs;j++) {
			for(int k=ref[j].offset;k<ref[j].offset+ref[j].length;k++)
				scr[k*2+1]=0x3E;
//			printf("NUMREFS:%d\n",topic->numRefs);
		}
		getch();
		for(int k = 0; k < 2000; k++) {
			scr[k*2]   = 32;
			scr[k*2+1] = 0x07;
		}
	}
}
//
// Program Block
//
int main(int argc, char **argv)
{
	char symbName[MAXPATH];
	FILE * helpFile = 0, *textFile = 0, *cppFile = 0;
	char bufStr[MAXSTRSIZE];
	printf("\nHelp Compiler  Version 1.0  Copyright (c) 1991 Borland International.\n");
	if(argc<2) {
		printf("\n  Syntax  TVHC <Help text>[.TXT] [<Help file>[.HLP]]\n\
\n     Help text   = Help file source\n     Help file   = Compiled help file\n");
		exit(1);
	}
	// Calculate file names
	strcpy(textName,HLPreplaceEXT(argv[1], ".TXT",0));
	if(!fExists(textName)) {
		sprintf(bufStr,"File %s not found.",textName);
		ERROR(bufStr);
	}
	if(argc >= 3)
		strcpy(helpName, HLPreplaceEXT(argv[2], ".HLP",0));
	else
		strcpy(helpName, HLPreplaceEXT(textName,".HLP",1));
	checkOverwrite(helpName);
	checkOverwrite("helpindx.cpp");
	textFile = fopen(textName, "r");
	if(!textFile)
		ERROR("Ошибка при открытии файла.");
	helpFile = fopen(helpName, "w+b");
	if(!helpFile)
		ERROR("Ошибка при открытии файла.");
	cppFile = fopen("helpindx.cpp", "wt");
	if(!cppFile)
		ERROR("Ошибка при открытии файла.");
	processText(textFile,helpFile,cppFile,helpName);
	fclose(helpFile);
	helpFile = fopen(helpName,"rb");
	if(helpFile == NULL)
		ERROR("Ошибка при открытии файла.");
//    test(helpFile);
	fcloseall();
	return 0;
}
