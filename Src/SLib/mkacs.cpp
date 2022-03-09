// MKACS.CPP
// Copyright (c) Sobolev A. 1996

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

//#pragma warn -pro

int is_rus_letter( int l )
{
asm mov ax, l
asm cmp al, 80h
asm jb  __norus
asm cmp al, 0afh
asm jbe __rus
asm cmp al, 0e0h
asm jb  __norus
asm cmp al, 0f1h
asm jg  __norus
__rus:
asm mov ax, 1
asm jmp __end
__norus:
asm xor ax, ax
__end:
	return _AX;
}

/*
	Функция rus_tolower()
	переводит символы русского алфавита в нижний регистр
	если символ не является русской буквой, то вызывается
	функция TurboC tolower()
*/
int rus_tolower( int l )
{
	unsigned ch = (unsigned)l;
	if( is_rus_letter( ch ) ) {
		if( ch >= 0xff80 && ch < 0xff90 )
			ch += 0x20;
		else
			if( ch >= 0xff90 && ch < 0xffa0 )
				ch += 0x50;
			else
				if( ch == 0xfff0 )
					ch += 0x01;
	}
	else
		if( isalpha( ch ) && isupper( ch ) )
			ch = _tolower( ch );
	return ch;
}

/*
	Функция rus_toupper()
	переводит символы русского алфавита в верхний регистр
	если символ не является русской буквой, то вызывается
	функция TurboC toupper()
*/
int rus_toupper( int l )
{
	unsigned char ch = (unsigned char)l;
	if( is_rus_letter( ch ) ) {
		if( ch >= 0xa0U && ch < 0xb0U )
			ch -= 0x20;
		else
			if( ch >= 0xe0U && ch < 0xf0U )
				ch -= 0x50;
			else
				if( ch == 0xf1U )
					ch -= 0x01U;
	}
	else
		if( isalpha( ch ) && islower( ch ) )
			ch = _toupper( ch );
	return ch;
}

void main()
{
	int i;
	/*
		RUSCASE
	*/
	FILE * f = fopen("ruscase.alt", "wb");
	if(f == NULL) {
		printf("Error: Unable to open file ruscase.alt\n");
		exit(-1);
	}
	fputc(0xAC, f);
	fputs("RUSCASE ", f);
	for(i = 0; i < 256; i++)
		fputc(i, f);
	fclose(f);
	/*
		RUSNCASE
	*/
	f = fopen("rusncase.alt", "wb");
	if(f == NULL) {
		printf("Error: Unable to open file rusncase.alt\n");
		exit(-1);
	}
	fputc(0xAC, f);
	fputs("RUSNCASE", f);
	for(i = 0; i < 256; i++)
		fputc(rus_toupper(i), f);
	fclose(f);
}
