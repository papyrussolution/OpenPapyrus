// TblRecover.cpp : Defines the entry point for the console application.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <slib.h>
#include <ctype.h>

/*struct BillRec {
	long    ID;
	char    Code[10];
	LDATE   Dt;
	long    BillNo;
	long    OprKind;
	long    UserID;
	long    Location;
	long    Object;
	long    Object2;
	long    CurID;
	double  CRate;
	char    Amount[8];
	long    LinkBillID;
	long    Flags;
	short   AccessLevel;
	//long    SCardID;
	//char    Memo[160];
};*/

struct BillRec {
	long   ID;
	char   Code[10];
	LDATE  Dt;
	long   BillNo; 
	long   OprKind;
	long   UserID;
	long   Location;
	long   Object;
	long   Object2;
	long   CurID;
	double CRate;
	char   Amount[8];
	long   LinkBillID;
	long   Flags;
	int16  AccessLevel;
	long   SCardID;
	char   Memo[160];
};

#define MONEYTOLDBL(m)      dectobin((m),sizeof(m),2)

static PrintBillRec(FILE * f, const BillRec * pRec)
{
	char datebuf[32];
	datefmt(&pRec->Dt, DATF_DMY|DATF_CENTURY, datebuf);
	double amount = MONEYTOLDBL(pRec->Amount);
	fprintf(f, "%ld;%s;%s;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%lf;%.2lf;%ld;%ld;%d\n", /*;%ld*/
		pRec->ID, pRec->Code, datebuf, pRec->BillNo, pRec->OprKind,
		pRec->UserID, pRec->Location, pRec->Object, pRec->Object2,
		pRec->CurID, pRec->CRate, amount, pRec->LinkBillID, pRec->Flags,
		pRec->AccessLevel/*, pRec->SCardID*/);
	return 1;
}

#define MAXBILLID 100000L

int split_file(char * fname, size_t pgSz, size_t recSz, int isVLR)
{
	long count = 0, recs_count = 0, data_pg_count = 0;
	long prev_recs_count = 0;
	char out_fname[MAXPATH], oi_fname[MAXPATH], rec_fname[MAXPATH], re1_fname[MAXPATH];

	replaceExt(strcpy(out_fname, fname), ".OUT", 1);
	replaceExt(strcpy(oi_fname, fname), ".OI", 1);
	replaceExt(strcpy(rec_fname, fname), ".REC", 1);
	replaceExt(strcpy(re1_fname, fname), ".RE1", 1);

	FILE * in = fopen(fname, "rb");
	FILE * out = fopen(out_fname, "wb");
	FILE * oi = fopen(oi_fname, "wb");
	FILE * rec = fopen(rec_fname, "wb");
   	FILE * re1 = fopen(re1_fname, "wb");

	char * buf = (char*)malloc(pgSz);

	while(fread(buf, pgSz, 1, in) == 1) {
		//uchar hdr[32];

        /*
		int  err_hdr_count = 0;
		do {
	        if(fread(hdr, 2, 1, in) != 1)
    	    	goto __endloop;
			err_hdr_count++;
        } while(!(
		(hdr[0] == 0x00 && hdr[1] == 0x44) ||
		(hdr[0] == 0x50 && hdr[1] == 0x50) ||
		(hdr[0] == 0x46 && hdr[1] == 0x43) ||
		(hdr[0] == 0x00 && hdr[1] == 0x80) ||
		(hdr[0] == 0x00 && hdr[1] == 0x81) ||
		(hdr[0] == 0x00 && hdr[1] == 0x82) ||
		(hdr[0] == 0x00 && hdr[1] == 0x83)
		));
		if(err_hdr_count > 1)
			printf("\terr_hdr_count = %d\n", err_hdr_count);
		fseek(in, -2, SEEK_CUR);
		if(fread(buf, pgSz, 1, in) != 1)
			goto __endloop;
		*/
		count++;
		fprintf(out, "\n--- Page %ld ---\n", count);
		fwrite(buf, pgSz, 1, out);
		if(buf[1] == 'D') {
			data_pg_count++;
			fprintf(oi, "--- Page %04ld --- ", count);
			for(size_t i = 0; i < 16; i++) {
				//fwrite(buf+i, 1, 1, oi);
				fprintf(oi, "%02x ", buf[i] & 0x00ff);
			}
			fprintf(oi, "\n");
		}

		if(buf[1] == 'D') {
			if(recSz) {
				size_t inc = /*isVLR ? 6 : 2*/2;
				size_t ofs = 6 + inc;
				size_t recs_per_pg = (pgSz-2) / (recSz+inc);
				for(uint j = 0; j < recs_per_pg; j++) {
					BillRec * p_rec = (BillRec *)(buf+ofs);

					fprintf(rec, "%d,", recSz);
					fwrite(buf+ofs, recSz, 1, rec);
					fwrite("\xD\xA", 2, 1, rec);

					if((!p_rec->Dt || checkdate(&p_rec->Dt)) && p_rec->ID > 0 && p_rec->ID <= MAXBILLID) {
            	        PrintBillRec(re1, (BillRec *)(buf+ofs));
						recs_count++;
					}
					ofs += (recSz+6);
				}
			}
		}
		if(recs_count != prev_recs_count) {
			printf("%8ld records; %8ld pages\n", recs_count, count);
			prev_recs_count = recs_count;
		}
	}
__endloop:
	if(oi)
		fclose(oi);
	if(out)
		fclose(out);
	if(in)
		fclose(in);
	if(rec)
		fclose(rec);
   	if(re1)
		fclose(re1);
	free(buf);

	printf("\n");
	printf("Number of blocks:      %ld\n", count);
	printf("Number of data blocks: %ld\n", data_pg_count);
	printf("Number of recs:        %ld\n", recs_count);

	return 1;
}

void main(int argc, char ** argv)
{
	if(argc < 5) {
		printf("Usage: bp btr_fname page_size rec_size is_vlr\n");
	}
	else {
		//split_goods2_file(argv[1]);
		if(fileExists(argv[1]))
			split_file(argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
		else
			printf("File %s not found\n", argv[1]);
	}
}
