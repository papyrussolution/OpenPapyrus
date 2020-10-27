// XCRU.CPP
// Copyright (c) A.Sobolev 2002, 2008
// Утилита преобразования выходного файла кассового драйвера Кристалл-УКМ
// для минимизации трафика при передаче по модему
//
#include <db.h>
#include <ppdbs.h>
#include <conio.h>
//
// Stub (instead PPLIB\PPDBUTIL.CPP
//
int CallbackCompress(long, long, const char *, int)
{
	return 1;
}

static void SLAPI Error(const char * pMsg)
{
	char zero_str[8];
	zero_str[0] = 0;
	printf("Error: %s\n", pMsg ? pMsg : zero_str);
	exit(-1);
}

typedef TSArray <SDirEntry> PPFileNameArray;

static int SLAPI GetFileList(const char * pPath, char ** ppFileNames, int isBtrFile)
{
	SDirEntry fb;
	PPFileNameArray fary;
	SDirec  sdirec;
	char file_name[MAXPATH];

	for(size_t i = 0; i < 2; i++)
		ppFileNames[i] = 0;
	strcat(setLastSlash(strcpy(file_name, pPath)), "cq??????.");
	if(isBtrFile)
		strcat(file_name, "btr");
	else
		strcat(file_name, "dat");
	for(sdirec.Init(file_name); sdirec.Next(&fb) > 0;)
		if(!fary.ordInsert(&fb, 0, PTR_CMPFUNC(SDirEntry_Time)))
			Error("Недостаточно памяти");
	if(fary.getCount() == 0)
		Error("Не обнаружено ни одного файла данных");
	strcat(setLastSlash(strcpy(file_name, pPath)), fary.at(fary.getCount()-1).FileName);
	ppFileNames[0] = newStr(file_name);
	if(fary.getCount() > 1) {
		strcat(setLastSlash(strcpy(file_name, pPath)), fary.at(fary.getCount()-2).FileName);
		ppFileNames[1] = newStr(file_name);
	}
	return 1;
}

static const char * FileSign = "CCSF";

struct CompressedSalesHdr { // Size = 64
	long   Sign;        // "CCSF"
	long   RecSize;
	long   RecsCount;
	ulong  Crc;
	char   Reserve[48];
};

struct CompressedSalesRec {
	//long    Code;          // * ╚─ ЄютрЁр
	//double  Quant;         // * Quantity
	char    Operation[2];  // * ╩юф юяхЁрЎшш LSTRING
	LDATE   DateOperation; // * ─рЄр юяхЁрЎшш
	//double  Price;         // * ╓хэр (Ёєс)
	long    Ck_Number;     // * ═юьхЁ ўхър
	double  Ck_Disc;       // * ╤ъшфър эр ўхъ т Ёєсы ї
	long    Cash_Code;     // * ╩юф ърёё√
	//long    Ck_Card;       // * ╩юф ъЁхфшЄэющ ърЁЄ√
	LTIME   Times;         // * ┬Ёхь  яЁюфрцш
	double  Summa;         // * ╤єььр яюъєяъш (Ёєс)
	long    NSmena;        // * ═юьхЁ ёьхэ√
};

static void SLAPI PrintRec(FILE * pStream, CompressedSalesRec * pRec)
{
	if(pStream) {
		char buf_dt[16], buf_tm[16];
		fprintf(pStream, "%-2c%10s%10s%8ld%8ld%8ld%8.2lf%8.2lf\n", pRec->Operation[1],
			datefmt(&pRec->DateOperation, MKSFMT(8, DATF_DMY), buf_dt),
			timefmt(pRec->Times, MKSFMT(8, TIMF_HMS), buf_tm),
			pRec->NSmena, pRec->Cash_Code, pRec->Ck_Number,
			pRec->Summa, pRec->Ck_Disc);
	}
}

static int SLAPI ExtractData(char * pSrcFile)
{
	printf("Упаковка файла %s\n", pSrcFile);

	if(!fileExists(pSrcFile))
		Error("Исходный файл не существует");

	FILE * out_stream = 0;
	FILE * log_stream = 0;
	char   dest_fname[MAXPATH];
	CRC32  _crc32;
	Cr_SaleTbl::Rec  src_rec;
	Cr_SaleTbl::Key3 k3;
	BTable src_tbl(pSrcFile);
	RECORDNUMBER numrecs = 0, count = 0;
	CompressedSalesHdr hdr;
	CompressedSalesRec dest_rec;

	memset(&hdr, 0, sizeof(hdr));
	memcpy(&hdr.Sign, FileSign, sizeof(hdr.Sign));
	hdr.RecSize = sizeof(CompressedSalesRec);

	src_tbl.getNumRecs(&numrecs);
	STRNSCPY(dest_fname, pSrcFile);
	replaceExt(dest_fname, "dat", 1);
	out_stream = fopen(dest_fname, "wb");
	if(!out_stream)
		Error("Ошибка создания файла результата");
	fseek(out_stream, sizeof(CompressedSalesHdr), SEEK_SET);
	replaceExt(dest_fname, "log", 1);
	log_stream = fopen(dest_fname, "w");

	printf("\nВсего %ld записей\n", numrecs);
	memset(&k3, 0, sizeof(k3));
	src_tbl.setDataBuf(&src_rec, sizeof(src_rec));
	if(src_tbl.search(3, &k3, spFirst)) {
		long  last_pct  = 0;
		LDATE last_dt   = ZERODATE;
		long  last_chk  = 0;
		long  last_cash = 0;
		memset(&dest_rec, 0, sizeof(dest_rec));
		do {
			if(last_dt != 0 && (
				(last_dt != src_rec.DateOperation || last_chk != src_rec.Ck_Number ||
				last_cash != src_rec.Cash_Code))) {
				hdr.Crc = _crc32.Calc(hdr.Crc, (uint8*)&dest_rec, sizeof(dest_rec));
				hdr.RecsCount++;
				fwrite(&dest_rec, sizeof(dest_rec), 1, out_stream);
				PrintRec(log_stream, &dest_rec);
				memset(&dest_rec, 0, sizeof(dest_rec));
			}
			memcpy(dest_rec.Operation, src_rec.Operation, sizeof(dest_rec.Operation));
			dest_rec.DateOperation = src_rec.DateOperation;
			/*
			if((src_rec.Quant + dest_rec.Quant) != 0)
				dest_rec.Price = ((src_rec.Price * src_rec.Quant - src_rec.Ck_Disc) +
					(dest_rec.Price * dest_rec.Quant - dest_rec.Ck_Disc) /
					(src_rec.Quant + dest_rec.Quant);
			else
				dest_rec.Price = src_rec.Price;
			dest_rec.Quant    += src_rec.Quant;
			dest_rec.Code      = src_rec.Code;
			*/
			dest_rec.Ck_Disc  += src_rec.Ck_Disc;
			dest_rec.Summa    += src_rec.Summa;
			dest_rec.Ck_Number = src_rec.Ck_Number;
			dest_rec.Cash_Code = src_rec.Cash_Code;
			//dest_rec.Ck_Card   = src_rec.Ck_Card;
			dest_rec.Times     = src_rec.Times;
			dest_rec.NSmena    = src_rec.NSmena;

			last_dt   = src_rec.DateOperation;
			last_chk  = src_rec.Ck_Number;
			last_cash = src_rec.Cash_Code;

			count++;
			if((100L * count / numrecs) != last_pct) {
				last_pct = (100L * count / numrecs);
				printf("%ld%% competed", last_pct);
				gotoxy(1, wherey());
			}
		} while(src_tbl.search(3, &k3, spNext));
		printf("\n");
	}
	if(out_stream) {
		if(dest_rec.DateOperation != 0) {
			hdr.Crc = _crc32.Calc(hdr.Crc, (uint8*)&dest_rec, sizeof(dest_rec));
			hdr.RecsCount++;
			fwrite(&dest_rec, sizeof(dest_rec), 1, out_stream); // Writing last record
			PrintRec(log_stream, &dest_rec);
			fseek(out_stream, 0, SEEK_SET);
			fwrite(&hdr, sizeof(hdr), 1, out_stream); // Writing header
		}
		fclose(out_stream);
	}
	if(log_stream)
		fclose(log_stream);
	return 1;
}

int SLAPI RestoreData(const char * pFileName)
{
	printf("Восстановление файла %s\n", pFileName);

	if(!fileExists(pFileName))
		Error("Не найден исходный файл");

	FILE * in_stream = 0;
	char   fname[MAXPATH];
	long   last_pct = 0;
	SString pattern_path;
	getExecPath(pattern_path).SetLastSlash().Cat("sale.btr");
	if(!fileExists(pattern_path))
		Error("В каталоге, где находится XCRU.EXE должен быть файл SALE.BTR");

	STRNSCPY(fname, pFileName);
	replaceExt(fname, "btr", 1);
	if(fileExists(fname))
		remove(fname);
	copyFileByName(pattern_path, fname);

	in_stream = fopen(pFileName, "rb");
	if(!in_stream)
		Error("Не удается открыть исходный файл");

	Cr_SaleTbl::Rec  dest_rec;
	Cr_SaleTbl::Key3 k3;
	BTable dest_tbl(fname);
	RECORDNUMBER count = 0;
	CompressedSalesHdr hdr;
	CompressedSalesRec rec;
	ulong  crc = 0;
	CRC32  _crc32;

	if(fread(&hdr, sizeof(hdr), 1, in_stream) != 1)
		Error("Ошибка чтения записи из исходного файла");
	printf("\n");
	printf("Проверка целостности файла...\n");
	if(memcmp(&hdr.Sign, FileSign, sizeof(hdr.Sign)) != 0)
		Error("Неправильная сигнатура исходного файла");
	for(count = 0; count < hdr.RecsCount; count++) {
		if(fread(&rec, sizeof(rec), 1, in_stream) != 1)
			Error("Ошибка чтения записи из исходного файла");
		crc = _crc32.Calc(crc, (uint8*)&rec, sizeof(rec));
	}
	if(crc != hdr.Crc)
		Error("Неправильная контрольная сумма исходного файла");
	printf("Файл %s корректен\n", pFileName);

	fseek(in_stream, sizeof(hdr), SEEK_SET);
	dest_tbl.setDataBuf(&dest_rec, sizeof(dest_rec));
	printf("Всего %ld записей\n", hdr.RecsCount);
	for(count = 0; count < hdr.RecsCount; count++) {
		if(fread(&rec, sizeof(rec), 1, in_stream) != 1)
			Error("Ошибка чтения записи из исходного файла");
		memset(&dest_rec, 0, sizeof(dest_rec));

		memcpy(dest_rec.Operation, rec.Operation, sizeof(dest_rec.Operation));
		dest_rec.DateOperation = rec.DateOperation;

		//dest_rec.Price = rec.Price;
		//dest_rec.Quant = rec.Quant;
		//dest_rec.Code  = rec.Code;
		//dest_rec.Ck_Card = rec.Ck_Card;

		dest_rec.Price = rec.Summa;
		dest_rec.Quant = 1;
		dest_rec.Code  = 0;
		dest_rec.Ck_Card = 0;

		dest_rec.Ck_Disc   = rec.Ck_Disc;
		dest_rec.Summa     = rec.Summa;
		dest_rec.Ck_Number = rec.Ck_Number;
		dest_rec.Cash_Code = rec.Cash_Code;
		dest_rec.Times     = rec.Times;
		dest_rec.NSmena    = rec.NSmena;
		if(!dest_tbl.insertRec())
			Error("Ошибка добавления записи в восстанавливаемый файл");
		if((100L * count / hdr.RecsCount) != last_pct) {
			last_pct = (100L * count / hdr.RecsCount);
			printf("%ld%% competed", last_pct);
			gotoxy(1, wherey());
		}
	}
	printf("\n");
	if(in_stream)
		fclose(in_stream);
	return 1;
}

int main(int argc, char ** argv)
{
	int b_major, b_minor, b_isnet;
	char path[MAXPATH];
	char * p_file_list[2];
	p_file_list[0] = p_file_list[1] = 0;
	if(argc < 2) {
		printf("Usage: XCRU [/R] path\n");
		return -1;
	}
	if(!Btrieve::GetVersion(&b_major, &b_minor, &b_isnet))
		Error("Btrieve или Brequest не загружен");
	if(b_major < 6)
		Error("Требуется Btrieve версии 6 или выше");
	if(stricmp(argv[1], "/R") == 0) {
		if(argc < 3) {
			printf("Usage: XCRU /R filename\n");
			return -1;
		}
		GetFileList(argv[2], p_file_list, 0);
		if(p_file_list[1]) {
			RestoreData(p_file_list[1]);
			ZDELETE(p_file_list[1]);
		}
		if(p_file_list[0]) {
			RestoreData(p_file_list[0]);
			ZDELETE(p_file_list[0]);
		}
	}
	else {
		GetFileList(argv[1], p_file_list, 1);
		if(p_file_list[1]) {
			ExtractData(p_file_list[1]);
			ZDELETE(p_file_list[1]);
		}
		if(p_file_list[0]) {
			ExtractData(p_file_list[0]);
			ZDELETE(p_file_list[0]);
		}
	}
	return 0;
}
