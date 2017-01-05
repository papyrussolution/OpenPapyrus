// STRSTORW.CPP
// Copyright (c) A.Sobolev 1996-1999, 2004, 2010, 2016
//
#include <slib.h>
#pragma hdrstop
#include <strstore.h>

int SLAPI StringStore::addString(int group, int code, Pchar str)
{
	int    we = 0; // Признак ошибки записи
	int16  len;
	uint   tmp;
	IdxItem item;
	if(!writing) {
		SLibError = SLERR_WRITEFAULT;
		error = 1;
	}
	if(!error) {
		len = (int16)strlen(str);
		we = (fwrite(&len, sizeof(len), 1, file) != 1);
		if(!we)
			we = (fwrite(str, len, 1, file) != 1);
		if(we) {
			fseek(file, last << 4, SEEK_SET);
			SLibError = SLERR_WRITEFAULT;
			error = 1;
		}
		else {
			item.group = group;
			item.code = code;
			item.offs = last;
			if(!ordInsert(&item, &tmp, CMPF_LONG)) {
				if(SLibError != SLERR_ARYDUPITEM)
					error = 1;
				else	// В случае дублирования элемента массива просто
					// возвращаем нулевое значение. При этом дальнейшие
					// операции не будут запрещены.
					return 0;
			}
			else {
				len += sizeof(len);
				tmp = (((len + 15) >> 4) << 4);
				fseek(file, tmp - len, SEEK_CUR);
				last += (tmp >> 4);
			}
		}
	}
	return !error;
}
