// PATH.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2013, 2015, 2016, 2017
// @codepage windows-1251
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
// PPPath
//
struct PathItem { // Variable length struct
	SLAPI  PathItem(PPID pathID = 0, short flags = 0, const char * str = 0);
	void * SLAPI operator new(size_t sz, const char * str = 0);
	void   SLAPI operator delete(void *, const char *);
	// @v9.4.8 int    SLAPI GetPath(char * buf, size_t bufSize) const;
	SString & FASTCALL GetPath(SString & rBuf) const
	{
		return (rBuf = Path());
	}
	const  char * SLAPI Path() const
	{
		return (Size > sizeof(PathItem)) ? (char *)(this + 1) : 0;
	}
	PPID   ID;
	int16  Flags;
	uint16 Size;
	// ... zstring
};

struct PathData {          // @persistent @store(PropertyTbl)
	PPID   SecurObj;       // SecurType
	PPID   SecurID;        // SecurID
	PPID   Prop;
	char   Reserve1[64];
	int16  Reserve2;
	uint16 Flags;
	uint32 TailSize;
	// ... PathItem[]
};

SLAPI PathItem::PathItem(PPID pathID, short flags, const char * str)
{
	ID    = pathID;
	Flags = flags;
	if(!isempty(str)) {
		const size_t len = strlen(str) + 1;
		Size  = (uint16)(sizeof(PathItem) + len);
		memcpy(this + 1, str, len);
	}
	else
		Size = sizeof(PathItem);
}

void * SLAPI PathItem::operator new(size_t sz, const char * str)
{
	const size_t len = (str && str[0]) ? (strlen(str) + 1) : 0;
	return ::new char[sz + len];
}

void SLAPI PathItem::operator delete(void * p, const char *)
{
	::delete(p);
}

/*int SLAPI PathItem::GetPath(char * pBuf, size_t bufLen) const
{
	strnzcpy(pBuf, Path(), bufLen);
	return 1;
}*/
//
//
//
SLAPI PPPaths::PPPaths()
{
	P = 0;
}

SLAPI PPPaths::~PPPaths()
{
	Empty();
}

void SLAPI PPPaths::Empty()
{
	ZFREE(P);
}

int SLAPI PPPaths::IsEmpty() const
{
	return (P == 0);
}

PPPaths & FASTCALL PPPaths::operator = (const PPPaths & src)
{
	Resize(src.Size());
	if(P)
		memmove(P, src.P, src.Size());
	return *this;
}

size_t SLAPI PPPaths::Size() const
{
	return P ? (sizeof(PathData) + (size_t)P->TailSize) : 0;
}

int SLAPI PPPaths::Resize(size_t sz)
{
	int    ok = 1;
	if(sz == 0)
		ZFREE(P);
	else {
		const size_t prev_size = Size();
		sz = (sz < sizeof(PathData)) ? sizeof(PathData) : sz;
		P = (PathData*)realloc(P, sz);
		if(P) {
			if(sz > prev_size)
				memzero(((char*)P) + prev_size, sz - prev_size);
			P->TailSize = sz - sizeof(PathData);
		}
		else
			ok = PPSetErrorNoMem();
	}
	return ok;
}

/* @v9.4.8 int SLAPI PPPaths::GetPath(PPID pathID, short * pFlags, char * pBuf, size_t bufLen) const
{
	if(P) {
		for(uint s = 0; s < P->TailSize;) {
			const PathItem * p = (const PathItem*)(((char*)(P + 1)) + s);
			if(p->ID == pathID) {
				ASSIGN_PTR(pFlags, p->Flags);
				return p->GetPath(pBuf, bufLen);
			}
			s += p->Size;
		}
	}
	ASSIGN_PTR(pFlags, PATHF_EMPTY);
	ASSIGN_PTR(pBuf, 0);
	return -1;
}*/

int SLAPI PPPaths::GetPath(PPID pathID, short * pFlags, SString & rBuf) const
{
	rBuf = 0;
	if(P) {
		for(uint s = 0; s < P->TailSize;) {
			const PathItem * p = (const PathItem*)(((const char*)(P + 1)) + s);
			if(p->ID == pathID) {
				ASSIGN_PTR(pFlags, p->Flags);
				return p->GetPath(rBuf).NotEmpty() ? 1 : -1;
			}
			s += p->Size;
		}
	}
	ASSIGN_PTR(pFlags, PATHF_EMPTY);
	return -1;
}

int SLAPI PPPaths::SetPath(PPID pathID, const char * pBuf, short flags, int replace)
{
	int    ok = 1;
	char * cp = 0;
	size_t hs = sizeof(PathData);
	size_t ts = 0;
	size_t s  = 0;
	int    found = 0;
	PathItem * pi = 0;
	if(P == 0)
		THROW(Resize(sizeof(PathData)));
	cp = (char*)(P + 1);
	hs = sizeof(PathData);
	ts = (size_t)P->TailSize;
	THROW(pi = new(pBuf) PathItem(pathID, flags, pBuf));
	while(s < ts && !found) {
		PathItem * p = (PathItem*)(cp + s);
		const size_t os = p->Size;
		if(p->ID == 0 || os == 0) {
			//
			// “ака€ ситуаци€ встречатьс€ не должна, но на вс€кий случай обработать ее стоит.
			// ¬ этом случае будем считать, что достигли конца структуры и не нашли заданный путь.
			//
			P->TailSize = s;
			ts = (size_t)P->TailSize;
			break;
		}
		else if(p->ID == pathID) {
			if(isempty(pBuf)) {
				//
				// Remove item and resize buffer
				//
				memmove(cp + s, cp + s + os, ts - s - os);
				THROW(Resize(hs + ts - os));
			}
			else if(replace) {
				if(pi->Size == os)
					//
					// Simply copy new data
					//
					memmove(p, pi, pi->Size);
				else {
					//
					// Resize buffer and copy new data
					//
					if(pi->Size > os) {
						THROW(Resize(hs + ts + pi->Size - os));
						cp = (char*)(P + 1);
					}
					memmove(cp + s + pi->Size, cp + s + os, ts - s - os);
					memmove(cp + s, pi, pi->Size);
					if(pi->Size < os) {
						THROW(Resize(hs + ts + pi->Size - os));
						cp = (char*)(P + 1);
					}
				}
			}
			found = 1;
		}
		else
			s += os;
	}
	if(!found && pBuf && pBuf[0]) {
		THROW(Resize(hs + ts + pi->Size));
		cp = (char*)(P + 1);
		memmove(cp + s, pi, pi->Size);
	}
	CATCHZOK
	PathItem::operator delete(pi, 0);
	return ok;
}

/* @v9.4.8 int SLAPI PPPaths::Get(PPID obj, PPID id, PPID pathID, char * pBuf, size_t bufLen)
{
	SString temp_buf;
	int    ok = Get(obj, id, pathID, temp_buf);
	temp_buf.CopyTo(pBuf, bufLen);
	return ok;
} */

int SLAPI PPPaths::Get(PPID obj, PPID id, PPID pathID, SString & rBuf)
{
	int    ok = 1;
	SPathStruc ps;
	if(obj && id)
		THROW(Get(obj, id));
   	GetPath(pathID, 0, rBuf);
	rBuf.Strip();
	if(pathID == PPPATH_DRIVE) {
		if(rBuf.Empty()) {
			// @v5.5.9 rBuf.CatChar('A'+getdisk()).CatChar(':');
			; // @v5.5.9 ¬идимо, не следует подставл€ть текущий диск, ибо возможно указание относительных каталогов
		}
		else if(rBuf.Len() == 1)
			rBuf.CatChar(':');
	}
	else {
		ps.Split(rBuf.SetLastSlash());
		if(!(ps.Flags & SPathStruc::fDrv)) {
			//
			// ќбработка неполного каталога
			//
			if(pathID == PPPATH_ROOT) {
				//
				// ≈сли извлекаем корневой каталог, то остаетс€ добавить только драйвер
				//
				THROW(Get(0, 0, PPPATH_DRIVE, ps.Drv)); // @recursion
				if(ps.Drv.NotEmpty()) {
					ps.Flags |= SPathStruc::fDrv;
					if(!oneof2(ps.Dir.C(0), '/', '\\'))
						ps.Dir.PadLeft(1, '\\');
				}
				ps.Merge(rBuf);
			}
			else {
				//
				// ≈сли извлекаем что-то кроме корн€ или драйвера, то
				// два случа€:
				//
				if(ps.Flags & SPathStruc::fDir && oneof2(ps.Dir.C(0), '/', '\\')) {
					//
					// 1. ≈сли путь rBuf содержит каталог и начинаетс€ с '\', то
					//    добавл€ем к имени спереди только драйвер
					//
					THROW(Get(0, 0, PPPATH_DRIVE, ps.Dir)); // @recursion
					ps.Dir.RmvLastSlash();
				}
				else {
					//
					// 2. ¬ противном случае добавл€ем к имени спереди весь корневой каталог
					//
					THROW(Get(0, 0, PPPATH_ROOT, ps.Dir)); // @recursion
				}
				rBuf = ps.Dir.Cat(rBuf);
			}
		}
		rBuf.SetLastSlash();
	}
	CATCH
		ok = 0;
		rBuf = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPPaths::Get(PPID securType, PPID securID)
{
	int    ok = 1, r;
	Reference * p_ref = PPRef;
	size_t sz = 2048; // @v6.1.12 1024-->2048
	uint   s;
	//
	//  аталоги TEMP, LOG, PACK могут быть устанавлены до считывани€ из базы данных.
	// ¬ этом случае не допускаем переписывани€ уже установленных значений теми, что указаны
	// в конфигурации.
	//
	SString temp_path, log_path, pack_path;
	GetPath(PPPATH_TEMP, 0, temp_path);
	GetPath(PPPATH_LOG, 0, log_path);
	GetPath(PPPATH_PACK, 0, pack_path); // @v7.8.2
	PathItem * p = 0;
	THROW(Resize(sz));
	THROW(r = p_ref->GetConfig(securType, securID, PPPRP_PATHS, P, sz));
	if(r > 0) {
		if(Size() > sz) {
			THROW(Resize(sz = Size()));
			THROW(p_ref->GetConfig(securType, securID, PPPRP_PATHS, P, sz) > 0);
		}
		if(P->SecurObj < securType) {
			P->Flags |= PATHF_INHERITED;
			for(s = 0; s < P->TailSize; s += p->Size) {
				p = (PathItem*)(((char*)(P + 1)) + s);
				p->Flags |= PATHF_INHERITED;
			}
		}
		if(oneof2(P->SecurObj, PPOBJ_USRGRP, PPOBJ_USR)) {
			//
			// Ќеобходимо унаследовать от более высоких уровней иерархии
			// пути, которые не определены на заданном уровне (–екурси€)
			//
			PPPaths temp;
			PPID   prev_type = (securType == PPOBJ_USRGRP) ? PPOBJ_CONFIG : PPOBJ_USRGRP;
			THROW(p_ref->GetItem(securType, securID) > 0);
			THROW(temp.Get(prev_type, ((PPSecur*)&p_ref->data)->ParentID)); // @recursion
			uint prev_s = UINT_MAX;
			for(s = 0; s < temp.P->TailSize; s += p->Size) {
				//
				// «ащита от бесконечного цикла {
				//
				if(s == prev_s)
					break;
				else
					prev_s = s;
				// }
				p = (PathItem*)(PTR8(temp.P + 1) + s);
				if(p->Path())
					THROW(SetPath(p->ID, p->Path(), p->Flags | PATHF_INHERITED, 0));
			}
		}
		THROW(Resize(Size()));
	}
	else
		Empty();
	//
	// ¬осстановление значений сохраненных ранее TEMP и LOG каталогов
	//
	if(temp_path.NotEmptyS())
		SetPath(PPPATH_TEMP, temp_path, 0, 1);
	if(log_path.NotEmptyS())
		SetPath(PPPATH_LOG, log_path, 0, 1);
	// @v7.8.2 {
	if(pack_path.NotEmptyS())
		SetPath(PPPATH_PACK, pack_path, 0, 1);
	// } @v7.8.2
	CATCHZOK
	return ok;
}

int SLAPI PPPaths::Put(PPID securType, PPID securID)
{
	int    ok = 1;
	if(!IsEmpty()) {
		P->Flags   &= ~PATHF_INHERITED;
		P->SecurObj = securType;
		P->SecurID  = securID;
		P->Prop     = PPPRP_PATHS;
		memzero(P->Reserve1, sizeof(P->Reserve2));
		P->Reserve2 = 0;
		for(uint s = 0; s < P->TailSize;) {
			PathItem * p = (PathItem*)(((char*)(P + 1)) + s);
			if(!p->Path() || p->Flags & PATHF_INHERITED) {
				THROW(SetPath(p->ID, 0, 0, 1));
			}
			else {
				p->Flags &= ~PATHF_INHERITED;
				s += p->Size;
			}
		}
		THROW(PPRef->SetConfig(securType, securID, PPPRP_PATHS, P, Size()));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPPaths::Remove(PPID securType, PPID securID)
	{ return PPRef->RemoveProp(securType, securID, PPPRP_PATHS, 0); }
int FASTCALL PPGetPath(PPID pathID, SString & rBuf)
	{ return DS.GetPath(pathID, rBuf); }

int FASTCALL PPGetFilePath(PPID pathID, const char * pFileName, SString & rBuf)
{
	rBuf = 0;
	if(PPGetPath(pathID, rBuf)) {
		rBuf.SetLastSlash().Cat(pFileName);
		return 1;
	}
	else
		return 0;
}

int FASTCALL PPGetFilePath(PPID pathID, uint fileNameID, SString & rBuf)
{
	SString temp_buf;
	return PPGetFilePath(pathID, PPGetFileName(fileNameID, temp_buf), rBuf);
}

SString & FASTCALL PPGetFileName(uint fnameID, SString & rBuf)
{
	// PPGetSubStr(PPTXT_FILENAMES, fnameID, rBuf);
	PPLoadText(fnameID, rBuf);
	return rBuf;
}

SString & SLAPI PPMakeTempFileName(const char * pPrefix, const char * pExt, long * pStart, SString & rBuf)
{
	SString path;
	PPGetPath(PPPATH_TEMP, path);
	return MakeTempFileName(path.SetLastSlash(), pPrefix, pExt, pStart, rBuf);
}

int SLAPI PPRemoveFiles(const PPFileNameArray * pFileList)
{
	int    ok = -1;
	if(pFileList) {
		SString file_path;
		for(uint i = 0; pFileList->Enum(&i, 0, &file_path);)
			SFile::Remove(file_path);
		ok = 1;
	}
	return ok;
}

int SLAPI PPRemoveFilesByExt(const char * pSrc, const char * pExt)
{
	PPFileNameArray fary;
	SString wildcard;
	wildcard.CatChar('*');
	if(pExt && pExt[0] != '.')
		wildcard.Dot();
	wildcard.Cat(pExt);
	fary.Scan(pSrc, wildcard);
	return PPRemoveFiles(&fary);
}
//
//
//
SLAPI PPFileNameArray::PPFileNameArray() : TSArray <SDirEntry>()
{
}

int SLAPI PPFileNameArray::Scan(const char * pPath, const char * pWildcard)
{
	int    ok = 1;
	freeAll();
	Path = pPath;
	if(!pPath)
		ok = PPSetErrorInvParam();
	else {
		SDirec sdirec;
		SDirEntry fb;
		SString srch_path = pPath;
		if(pWildcard)
			srch_path.SetLastSlash().Cat(pWildcard);
		for(sdirec.Init(srch_path); ok && sdirec.Next(&fb) > 0;)
			if(!ordInsert(&fb, 0, PTR_CMPFUNC(SDirEntry_Time)))
				ok = PPSetErrorSLib();
	}
	return ok;
}

const SString & SLAPI PPFileNameArray::GetPath() const
{
	return Path;
}

int SLAPI PPFileNameArray::Enum(uint * pIdx, SDirEntry * pEntry, SString * pFullPath) const
{
	SDirEntry * p_item;
	if(enumItems(pIdx, (void **)&p_item)) {
		if(pFullPath)
			(*pFullPath = Path).SetLastSlash().Cat(p_item->FileName);
		ASSIGN_PTR(pEntry, *p_item);
		return 1;
	}
	else
		return 0;
}
