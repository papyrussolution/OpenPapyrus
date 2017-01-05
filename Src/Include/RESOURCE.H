// RESOURCE.H  Turbo Vision 1.0
// Copyright (c) Borland International 1991

class istream;
class TDialog;
class TMenu;
class TStatusDef;

extern int (SLAPI * getUserControl)(istream &, TDialog*);

int    SLAPI LoadDialog(char * fName, uint dialogID, TDialog * dlg);
TMenu * SLAPI LoadMenu(char * fName, uint menuID);
TStatusDef * SLAPI LoadStatusDef(char * fName, uint statusID);

#if defined(Uses_TStringCollection) && !defined(__TStringCollection)
#define __TStringCollection

class TStringCollection : public TSortedCollection {
IOPStrDecl(TStringCollection);
public:
	TStringCollection(short aLimit, short aDelta);
private:
	virtual int compare(void *key1, void *key2);
	virtual void freeItem(void *item);
#ifdef _TV_STREAM_
	virtual void *readItem(ipstream&);
	virtual void writeItem(void *, opstream&);
#endif
};

IOPStrManip(TStringCollection);

#endif  // Uses_TStringCollection

#ifdef _TV_RESOURCE_
#if defined(Uses_TResourceItem) && !defined(__TResourceItem)
#define __TResourceItem

struct TResourceItem {
	long pos;
	long size;
	char *key;
};

#endif  // Uses_TResourceItem

#if defined(Uses_TResourceCollection) && !defined(__TResourceCollection)
#define __TResourceCollection

class TResourceCollection: public TStringCollection {
IOPStrDecl(TResourceCollection);
public:
	TResourceCollection(short aLimit, short aDelta);
	virtual void *keyOf(void *item);
private:
	virtual void freeItem(void *item);
#ifdef _TV_STREAM_
	virtual void *readItem(ipstream&);
	virtual void writeItem(void *, opstream&);
#endif
};

IOPStrManip(TResourceCollection);

#endif  // Uses_TResourceCollection

#if defined(Uses_TResourceFile) && !defined(__TResourceFile)
#define __TResourceFile

class far TResourceCollection;
class far fpstream;

class TResourceFile: public TObject {
public:
	TResourceFile(fpstream *aStream);
	~TResourceFile();
	short count();
	void remove(const char *key);
	void flush();
	void *get(const char *key);
	const char *keyAt(short i);
	void put(TStreamable *item, const char *key);
	fpstream *switchTo(fpstream *aStream, Boolean pack);
protected:
	fpstream *stream;
	Boolean modified;
	long basePos;
	long indexPos;
	TResourceCollection *index;
};

#endif  // Uses_TResourceFile

#if defined(Uses_TStrIndexRec) && !defined(__TStrIndexRec)
#define __TStrIndexRec

class TStrIndexRec {
public:
	TStrIndexRec();
	ushort key;
	ushort count;
	ushort offset;
};

#endif  // Uses_TStrIndexRec

#if defined(Uses_TStringList) && !defined(__TStringList)
#define __TStringList

class far TStrIndexRec;

class TStringList : public TObject, public TStreamable {
IOPStrDeclRW(TStringList);
public:
	~TStringList();
	void get(char *dest, ushort key);
private:
	ipstream *ip;
	long basePos;
	short indexSize;
	TStrIndexRec *index;
};

IOPStrManip(TStringList);

#endif  // Uses_TStringList

#if defined(Uses_TStrListMaker) && !defined(__TStrListMaker)
#define __TStrListMaker

class TStrListMaker : public TObject, public TStreamable {
IOPStrDeclRW(TStrListMaker);
public:
	TStrListMaker(ushort aStrSize, ushort aIndexSize);
	~TStrListMaker();
	void put(ushort key, char *str);
private:
	ushort strPos;
	ushort strSize;
	char *strings;
	ushort indexPos;
	ushort indexSize;
	TStrIndexRec *index;
	TStrIndexRec cur;
	void closeCurrent();
};

IOPStrManip(TStrListMaker);

#endif // Uses_TStrListMaker
#endif // _TV_RESOURCE_

