// MKSTRING.CPP
// Copyright (c) A.Sobolev 1996, 2004, 2007, 2008, 2011, 2016, 2020, 2022
//
#include <slib-internal.h>
#pragma hdrstop

#ifdef SL_STRSTORE // {

#include <strstore.h>

int    line = 1;
SString InputFileName;

void error(const char * pStr)
{
	printf("%s(%d): error: %s\n", InputFileName.cptr(), line, pStr);
	//printf("Error (%d): %s\n", line, s);
	exit(1);
}

void warning(const char * pStr)
{
	printf("%s(%d): warning: %s\n", InputFileName.cptr(), line, pStr);
	//printf("Warning (%d): %s\n", line, s);
}

int getsymb(FILE * f, uchar * pC)
{
	uchar  c = fgetc(f);
	*pC = c;
	return feof(f) ? 0 : 1;
}

void get(char * inFile, char * outFile)
{
	const  size_t buf_size = 2048;
	
	int    group = 0;
	int    ident;
	char   buffer[buf_size];
	FILE * f = fopen(inFile, "rt");
	if(f == NULL)
		error("Unable open input file");
	StringStore store(outFile, 1);
	if(store.error)
		error("Unable open output file");
	int    ival;
	int    symb;
	char   number[32];
	uchar  c;
	while(getsymb(f, &c)) {
		switch(c) {
			case '\n':
				line++;
			case '\t':
			case ' ':
				break;
			case '[':
				ival = 0;
				while(getsymb(f, &c) && c != ']' && ival < (32 - 1))
					number[ival++] = c;
				if(feof(f))
					error("Unexpected EOF");
				else if(ival >= (32 - 1))
					error("Too long section number");
				number[ival] = '\0';
				ival = satoi(number);
				group = ival;
				break;
			case '"':
				ival = 0;
				while(getsymb(f, &c) && ival < (buf_size - 1)) {
					int nextc = 0;
					if(c == '"')
						break;
					else if(c == '\\') {
__next_c:
						if(!getsymb(f, &c))
							error("Unexpected EOF");
						else if(c == '\n') {
							line++;
							nextc = 1;
							goto __next_c;
						}
						else if(!nextc) {
							if(c == 'n')
								c = '\n';
							else if(c == 't')
								c = '\t';
							else if(c == 'x' || c == 'X') {
								uchar hex[4];
								if(!getsymb(f, &hex[0]) || !getsymb(f, &hex[1]))
									error("Unexpected EOF");
								hex[2] = 0;
								sscanf(strupr((char *)hex), "%X", &symb);
								c = symb;
							}
							else if(isdigit(c)) {
								uchar oct[4];
								oct[0] = c;
								if(!getsymb(f, &oct[1]) || !getsymb(f, &oct[2]))
									error("Unexpected EOF");
								oct[3] = 0;
								sscanf((char *)oct, "%o", &symb);
								c = symb;
							}
						}
					}
					buffer[ival++] = c;
				}
				if(ival >= (buf_size-1))
					error("String too long (max length 2047 bytes)");
				buffer[ival++] = '\0';
				if(!store.addString(group, ident, buffer))
					if(SLibError == SLERR_ARYDUPITEM)
						error("Duplicate string ident");
					else
						error("Unable add new string to StringStore object");
				// DEBUG !!!
				// else
				// printf("%d  %d  %s\n", group, ident, buffer);
				break;
			default:
				if(isdigit(c) || c == '-') {
					ival = 0;
					do
						number[ival++] = c;
					while(getsymb(f, &c) && isdigit(c) && ival < (32 - 1));
					if(feof(f))
						error("Unexpected EOF");
					else if(ival >= (32 - 1))
						error("Too long section number");
					number[ival] = '\0';
					ival = satoi(number);
					ident = ival;
				}
				break;
		}
	}
}

int main(int argc, char **argv)
{
	if(argc < 3)
		error("Usage: STRSTORE infile outfile");
	else {
		SLS.Init("STRSTORE");
		InputFileName = argv[1];
		get(argv[1], argv[2]);
		return 0;
	}
}

#endif // } SL_STRSTORE