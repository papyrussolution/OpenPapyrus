// RPTWRITE.CPP
// Copyright (c) A.Sobolev 1996-1999

#include "_report.h"
#include <tvdefs.h>

int SLAPI SReport::writeResource(FILE * f, uint res_no)
{
	int i, j;
	if(res_no)
		fprintf(f, "\n%u %u {\n", res_no, TV_REPORT);
	else
		fprintf(f, "\nREPORT_%s TV_REPORT {\n", strupr(name));
	if(data_name)
		fprintf(f, "\t\"%s\\0\", \"%s\\0\",", name, data_name);
	else
		fprintf(f, "\t\"%s\\0\", \"\\0\",", name);
	fprintf(f, "%6d,", (int)(main_id >> 16));
	fprintf(f, "%6d,", (int)(main_id & 0xFFFFL));
	fprintf(f, "%6d,", textlen);

	if(textlen & 1)
		textlen++;
	for(i = 0; i < (textlen / 2); i++) {
		if(i % 8 == 0)
			fputs("\n\t", f);
		fprintf(f, "%6d, ", ((int *) text)[i]);
	}
	fprintf(f, "\n\t%6d, ", fldCount);
	for(i = 0; i < fldCount; i++) {
		fprintf(f, "\n\t%6d, %6d, %6d, %6d, %6d, %6d, %6d, %6d, ",
			fields[i].id, fields[i].name, fields[i].type,
			(int) LoWord(fields[i].format), (int) HiWord(fields[i].format),
			(int) fields[i].fldfmt,
			(int) LoWord(fields[i].offs), (int) HiWord(fields[i].offs));
	}
	fprintf(f, "\n\t%2d, ", agrCount);
	for(i = 0; i < agrCount; i++)
		fprintf(f, "\n\t%2d, %2d, %2d, %2d, ",
			agrs[i].fld, agrs[i].aggr, agrs[i].dpnd, agrs[i].scope);
	fprintf(f, "\n\t%2d, ", grpCount);
	for(i = 0; i < grpCount; i++) {
		fprintf(f, "\n\t%2d, ", groups[i].band);
		if(groups[i].fields)
			for(j = 0; j <= groups[i].fields[0]; j++)
				fprintf(f, "%2d, ", groups[i].fields[j]);
		else
			fprintf(f, "%2d, ", (int) 0);
	}
	fprintf(f, "\n\t%2d, ", bandCount);
	for(i = 0; i < bandCount; i++) {
		fprintf(f, "\n\t%2d, %2d, %2d, %2d, ", bands[i].kind, bands[i].ht,
			bands[i].group, (int) bands[i].options);
		fputs("\n\t", f);
		if(bands[i].fields)
			for(j = 0; j <= bands[i].fields[0]; j++)
				if(i == (bandCount - 1) && j == bands[i].fields[0])
					fprintf(f, "%2d\n", bands[i].fields[j]);
				else
					fprintf(f, "%2d, ", bands[i].fields[j]);
		else if(i == (bandCount - 1))
			fprintf(f, "%2d\n", (int) 0);
		else
			fprintf(f, "%2d, ", (int) 0);
	}
	fprintf(f, "\n\t%d, %d, %d\n", pglen, leftmarg, prnoptions);
	fprintf(f, "}\n");
	return 1;
}
