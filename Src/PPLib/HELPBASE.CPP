// HELPBASE.CPP  Turbo Vision 1.0
// Copyright (c) 1991 by Borland International
// @SERG

#define Uses_TPoint
#define Uses_TRect

#include <string.h>
#include <limits.h>
#include <sys\stat.h>
#include <ctype.h>
#include <io.h>
#include <tv.h>
#include <helpbase.h>

#pragma warn -dsz
#define magicHeader 0x46484246L  // "FBHF"
//
THelpTopic::THelpTopic()
{
	paragraphs = 0;
	numRefs = 0;
	crossRefs = 0;
	width = 0;
	lastOffset = 0;
	lastLine = INT_MAX;
	lastParagraph = 0;
}

THelpTopic::~THelpTopic()
{
	TParagraph *p = paragraphs, *t=0;
	while(p) {
		t = p;
		p = p->next;
		delete t->text;
		delete t;
	}
	if(crossRefs)
		delete [numRefs]crossRefs;
}

void THelpTopic::read(FILE *f)
{
	int  i=0, size=0, temp=0;
	TParagraph **p=0;
	fread(&i,sizeof(int),1,f);
	p = &paragraphs;
	while(i > 0) {
		(*p) = new TParagraph;
		fread(&size,sizeof(int),1,f);
		(*p)->next = 0;
		(*p)->text = new char[size];
		(*p)->size = (int) size;
		fread(&temp,sizeof(int),1,f);
		(*p)->wrap = temp;
		fread((*p)->text,size,1,f);
		p = &((*p)->next);
		i--;
	}
	*p = 0;
	fread(&numRefs,sizeof(int),1,f);
	crossRefs = new TCrossRef[numRefs];
	fread(crossRefs,numRefs*sizeof(TCrossRef),1,f);
	width = 0;
	lastLine = INT_MAX;
}

void THelpTopic::write(FILE *f)
{
	TParagraph * p = paragraphs;
	int i;
	for(i = 0; p; i++)
		p = p->next;
	fwrite(&i, sizeof(int), 1, f);
	for(p = paragraphs; p; p = p->next) {
		fwrite(&(p->size),sizeof(int),1,f);
		fwrite(&(p->wrap),sizeof(int),1,f);
		fwrite( (p->text),p->size,1,f);
	}
	fwrite(&numRefs, sizeof(int), 1, f);
	fwrite(crossRefs, numRefs*sizeof(TCrossRef), 1, f);
}

void THelpTopic::addCrossRef(TCrossRef ref)
{
	crossRefs=(TCrossRef *)realloc(crossRefs,sizeof(TCrossRef)*(numRefs+1));
	crossRefs[numRefs] = ref;
	numRefs++;
}

void THelpTopic::addParagraph(TParagraph * p)
{
	TParagraph *pp;
	if(paragraphs == 0)
		paragraphs = p;
	else {
		pp = paragraphs;
		while(pp->next)
			pp = pp->next;
		pp->next = p;
	}
	p->next = 0;
}

void THelpTopic::getCrossRef(int i, TPoint *loc, int *length, int *ref)
{
	TParagraph * p           = paragraphs;
	int          offset      = crossRefs[i].offset+1;
	int          line        = 0;
	int          oldOffset   = 0;
	int          curOffset   = 0;
	int          paraOffset  = 0;
	while(paraOffset + curOffset < offset) {
		oldOffset = paraOffset + curOffset;
		wrapText(p->text, p->size, &curOffset, p->wrap);
		line++;
		if(curOffset >= p->size) {
			paraOffset += p->size;
			p = p->next;
			curOffset = 0;
		}
	}
	loc->x  = offset - oldOffset-1;
	loc->y  = line;
	*length = crossRefs[i].length;
	*ref    = crossRefs[i].ref;
}

void THelpTopic::getLine(char *buffer,int line)
{
	int offset, i;
	TParagraph *p;
	if(lastLine < line) {
		i        = line;
		line    -= lastLine;
		lastLine = i;
		offset   = lastOffset;
		p        = lastParagraph;
	}
	else {
		p        = paragraphs;
		offset   = 0;
		lastLine = line;
	}
	buffer[0] = 0;
	while(p) {
		while(offset < p->size) {
			strcpy(buffer, wrapText(p->text, p->size, &offset, p->wrap));
			if(--line == 0) {
				lastOffset    = offset;
				lastParagraph = p;
				return;
			}
		}
		p = p->next;
		offset = 0;
	}
	buffer[0]=0;
}

int THelpTopic::getNumCrossRefs()
{
	return numRefs;
}

int THelpTopic::numLines()
{
	int offset,lines = 0;
	TParagraph * p = paragraphs;
	while(p) {
		offset = 0;
		while(offset < p->size) {
			wrapText(p->text, p->size, &offset, p->wrap);
			lines++;
		}
		p = p->next;
	}
	return lines;
}

void THelpTopic::setCrossRef(int i, TCrossRef *ref)
{
	if(i < numRefs)
		crossRefs[i] = *ref;
}

void THelpTopic::setNumCrossRefs(int i)
{
	if(numRefs != i) {
		crossRefs=(TCrossRef *)realloc(crossRefs,i*sizeof(TCrossRef));
		numRefs = i;
	}
}

void THelpTopic::setWidth(int aWidth)
{
	width = aWidth;
}

char *THelpTopic::wrapText(char *text, int size, int *offset, int wrap)
{
	char line[1024];
	int start=0,end=-1,len=0,l=0,i;
	start=*offset;
	for(i=start;(i<size)&&(text[i]!=' ')&&(text[i]!='\n');i++);
	l=i-start;
	for(i=start;(i<size)&&(text[i]!='\n');i++);
	end=i;
	len=end-start;
	if((len>=width)&&(wrap)){
		for(i=start+width;text[i]!=' ';i--)
		if(i<start+l){
			i=start+l;
			break;
		}
		end=i;
	}
	*offset=end+1;
	strncpy(line,text+start,end-start);
	line[end-start]=0;
	return line;
}
/*
//   Original
char *THelpTopic::wrapText(char *text, int size, int *offset, int wrap)
{
	char line[256];
	int  i;
	int  incr = 0;
	char *temp1 = text + (*offset);
	char *temp2 = strchr(temp1, '\n');
	i= (temp2 && ((int)(temp2-temp1) <= 256)) ? ((int)(temp2-temp1)+1) : 256;
	if(i + *offset > size)
		i = size - *offset;
	if(i >= width && wrap) {
		i = *offset + width;
		if(i > size)
			i = size;
		else {
			while( (i > *offset) && (text[i] != ' ') ) i--;
			if(i == *offset)
				i = *offset + width;
			else
				incr = 0;//1;// ??
		}
		i -= *offset;
	}
	else
		incr = 0;//1;
	strncpy(line, text + *offset, i);
	line[i] = 0;
	int len = strlen(line);
	if(line[len - 1] == '\n')
		line[len - 1] = 0;
	*offset += (i + incr);
	return line;
}
*/
///
THelpIndex::THelpIndex(void)
{
	size  = 0;
	index = 0;
}

THelpIndex::~THelpIndex()
{
	if(index)
		delete [size] index;
}

void THelpIndex::write(FILE *f)
{
	fwrite(&size,sizeof(int),1,f);
	fwrite(index,sizeof(long)*size,1,f);
}

void THelpIndex::read(FILE *f)
{
	fread(&size,sizeof(int),1,f);
	if(size <= 0)
		index = 0;
	else{
		index=new long[size];
		fread(index,sizeof(long)*size,1,f);
	}
}

long THelpIndex::position(int i)
{
	return (i < size) ? index[i] : -1L;// 0xFFFFFFFF;    // -1
}

void THelpIndex::add(int i, long val)
{
	int    k = 0,newSize;
	if(i >= size) {
		newSize = i + 10; // Добавляем сразу >10 элементов
		index = (long *)realloc(index,sizeof(long)*newSize);
		for(k = size; k < newSize; k++)
			index[k] = -1L; // 0xFFFFFFFF; // значения = -1
		size = newSize;
	}
	index[i] = val;
}

THelpFile::THelpFile(FILE *f)
{
	long magic = 0, size = 0;
	convTablePos = 0;
	fseek(f, 0L, SEEK_END);
	size = ftell(f);
	fseek(f, 0L, SEEK_SET);
	if(size > sizeof(long))
		fread(&magic,sizeof(long),1,f);
	if(magic != magicHeader) {  // создание нового файла
		indexPos = 12;
		fseek(f,indexPos,SEEK_SET);
		index = new THelpIndex;
		modified = 1;
	}
	else {  // чтение сущ. файла
		fseek(f, 4, SEEK_SET);
		fread(&indexPos, sizeof(long), 1, f);
		fread(&convTablePos, sizeof(long), 1, f);
		fseek(f, indexPos, SEEK_SET);
		index = new THelpIndex;
		index->read(f);
		modified = 0;
	}
	file = f;
}

THelpFile::~THelpFile(void)
{
	long magic = magicHeader;
	if(modified) {
		fseek(file,0L,SEEK_SET);
		fwrite(&magic,sizeof(long),1,file);
		fwrite(&indexPos,sizeof(long),1,file);
		fwrite(&convTablePos,sizeof(long),1,file);
		fseek(file,indexPos,SEEK_SET);
		index -> write(file);
	}
	delete index;
	SFile::ZClose(&file);
}

THelpTopic * THelpFile::getTopic(int i)
{
	long pos;
	THelpTopic *topic = 0;
	pos = index->position(i);
	if(pos > 0) {
		topic = new THelpTopic;
		fseek(file, pos, SEEK_SET);
		topic->read(file);
	}
	else
		topic = invalidTopic(i);
	return topic;
}

THelpTopic *THelpFile::invalidTopic(int i)
{
	THelpTopic *topic=0;
	TParagraph *para=0;
	char invalidText[256];
	sprintf(invalidText, "\n В этом контексте помощь не доступна", i);
	topic = new THelpTopic;
	para  = new TParagraph;
	para->text = new char[strlen(invalidText)];
	strncpy(para->text,invalidText,strlen(invalidText));
	para->size = strlen(invalidText);
	para->wrap = 0;
	para->next = 0;
	topic->addParagraph(para);
	return topic;
}

void THelpFile::recordPositionInIndex(int i)
{
	index->add(i, indexPos);
	modified = 1;
}

void THelpFile::putTopic(THelpTopic *topic)
{
	fseek(file, indexPos, SEEK_SET);
	topic->write(file);
	indexPos = ftell(file);
	modified = 1;
}

int THelpFile::getConvCtx(int context)
{
	long curpos=ftell(file);
	int lenTable=0,ctx=0,ID=0,ctxID=0,i;
	fseek(file,convTablePos,SEEK_SET);
	fread(&lenTable,sizeof(int),1,file);
	if(lenTable>0){
		for(i=0;i<lenTable;i++){
			fread(&ID,sizeof(int),1,file);
			fread(&ctxID,sizeof(int),1,file);
			if(ID==context){
				ctx=ctxID;
				break;
			}
		}
	}
	else
		ctx=0;
	fseek(file,curpos,SEEK_SET);
	return ctx;
}

#pragma warn .dsz
