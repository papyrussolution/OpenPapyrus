// TEST-AGNER.CPP
// 
// Author:        Agner Fog
// Date created:  2007-06-14
// Last modified: 2011-07-17
// Project:       asmlib.zip
// Source URL:    www.agner.org/optimize
// Description: Simple test of asmlib library
// Instructions: Compile for console mode and link with the appropriate version of asmlib
// Further documentation: The file asmlib-instructions.pdf contains further documentation and instructions.
// 
// Copyright 2007-2011 by Agner Fog.
// GNU General Public License http://www.gnu.org/licenses/gpl.html
// Adopted to SLIB by A.Sobolev
// 
#include <pp.h>
#pragma hdrstop

/*void Failure(const char * text) 
{
	// Report if test failure
	printf("\nTest failed: %s\n", text);
	exit(1);
}*/

SLTEST_R(AgnerFog)
{
	SString temp_buf;
	// test InstructionSet()
	SetInfo(temp_buf.Z().CatEq("InstructionSet", InstructionSet()).CR());
	// test cpuid_abcd
	int abcd[4]; char s[16];
	cpuid_abcd(abcd, 0);
	*(int*)(s+0) = abcd[1];        // ebx
	*(int*)(s+4) = abcd[3];        // edx
	*(int*)(s+8) = abcd[2];        // ecx
	s[12] = 0;                     // terminate string
	SetInfo(temp_buf.Z().CatEq("Vendor string", s).CR());
	// test ProcessorName()
	SetInfo(temp_buf.Z().CatEq("ProcessorName", ProcessorName()).CR());
	// test CpuType
	int vendor, family, model;
	CpuType(&vendor, &family, &model);
	//printf("\nCpuType: vendor %i, family 0x%X, model 0x%X", vendor, family, model);
	temp_buf.Z().Cat("CpuType").CatDiv(':', 2).Cat("vendor").Space().Cat(vendor).CatDiv(',', 2).Cat(family).Space().CatHex(family).CatDiv(',', 2).
		Cat("model").Space().CatHex(model).CR();
	SetInfo(temp_buf);
	// test DataCacheSize
	//printf("\nData cache size: L1 %ikb, L2 %ikb, L3 %ikb", (int)DataCacheSize(1)/1024, (int)DataCacheSize(2)/1024, (int)DataCacheSize(3)/1024);
	temp_buf.Z().Cat("Data cache size").CatDiv(':', 2).Cat("L1").Space().Cat((int)DataCacheSize(1)/1024).Cat("kb").CatDiv(',', 2).
		Cat("L2").Space().Cat((int)DataCacheSize(2)/1024).Cat("kb").CatDiv(',', 2).Cat("L3").Space().Cat((int)DataCacheSize(3)/1024).Cat("kb").CR();
	SetInfo(temp_buf);
	// test ReadTSC()
	ReadTSC();
	int tsc = (int)ReadTSC();
	tsc = (int)ReadTSC() - tsc;
	//printf("\nReadTSC takes %i clocks\n\n", tsc);
	temp_buf.Z().Cat("ReadTSC takes").Space().Cat(tsc).Space().Cat("clocks").CR();
	SetInfo(temp_buf);
	// test Round();
	double d;
	for(d = -1; d <= 1; d += 0.5) {
		//printf("Round %f = %i = %i\n", d, Round(d), Round(float(d)));
		temp_buf.Z().Cat("Round").Space().Cat(d).CatDiv('=', 1).Cat(Round(d)).CatDiv('=', 1).Cat(Round(float(d))).CR();
		SetInfo(temp_buf);
	}
	// Test memory and string functions
	int i, n;
	const int strsize = 256;
	char string1[strsize], string2[strsize];
	const char * teststring = "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 1234567890 @`'{}[]()<>";

	// Initialize strings
	A_memset(string1, 0, strsize);
	A_memset(string2, 0, strsize);

	// Test A_strcpy, A_strcat, A_strlen
	A_strcpy(string1, teststring);
	n = strsize/(int)A_strlen(teststring);
	for(i = 0; i < n-1; i++) {
		A_strcat(string1, teststring);
	}
	SLCHECK_EQ(A_strlen(string1), n * A_strlen(teststring)); // Failure("A_strcpy, A_strcat, A_strlen");
	// Test A_stricmp
	A_memcpy(string2, string1, strsize);
	string2[4] ^= 0x20;  string1[30] ^= 0x20;// Change case
	SLCHECK_Z(A_stricmp(string1, string2)); // Failure("A_stricmp");
	{
		string2[8] += 2; // Make strings different
		SLCHECK_LT(A_stricmp(string1, string2), 0); // Failure("A_stricmp");
	}
	string2[7] -= 2; // Make strings different
	SLCHECK_LE(0, A_stricmp(string1, string2)); // Failure("A_stricmp");
	// test A_strtolower and A_strtoupper
	A_strcpy(string1, teststring);
	A_strcpy(string2, teststring);
	A_strtolower(string1);
	A_strtoupper(string2);
	//printf("\nstring converted to lower and upper case:\n%s\n%s\n%s", teststring, string1, string2);
	temp_buf.Z().Cat("string converted to lower and upper case").CatDiv(':', 2).Cat(teststring).Slash().Cat(string1).Slash().Cat(string2).CR();
	SetInfo(temp_buf);
	// test strspn and strcspn
	int n1, n2;
	const int nset = 4;
	const char * tset[] = {"abc", "", "01234567890123456789", "abcdefghijklmnopqrstuvwxyz"};
	for(i = 0; i < nset; i++) {
		n1 = A_strspn(teststring, tset[i]);
		n2 = strspn(teststring, tset[i]);
		SLCHECK_EQ(n1, n2); // Failure("A_strspn");
		n1 = A_strcspn(teststring, tset[i]);
		n2 = strcspn(teststring, tset[i]);
		SLCHECK_EQ(n1, n2); // Failure("A_strcspn");
	}

	// Test A_memmove with overlapping source and destination
	A_memcpy(string2, string1, strsize);

	A_memcpy(string1+5, string1+12, 12);
	memcpy(string2+5, string2+12, 12);
	SLCHECK_Z(A_stricmp(string1, string2)); // Failure("memcpy");

	A_memcpy(string1+5, string1+12, 130);
	memcpy(string2+5, string2+12, 130);
	SLCHECK_Z(A_stricmp(string1, string2)); // Failure("memcpy");

	A_memmove(string1+5, string1+2, 12);
	memmove(string2+5, string2+2, 12);
	SLCHECK_Z(A_stricmp(string1, string2)); // Failure("A_memmove");

	A_memmove(string1+3, string1+8, 12);
	memmove(string2+3, string2+8, 12);
	SLCHECK_Z(A_stricmp(string1, string2)); // Failure("A_memmove");

	A_memmove(string1+41, string1+30, 100);
	memmove(string2+41, string2+30, 100);
	SLCHECK_Z(A_stricmp(string1, string2)); // Failure("A_memmove");

	A_memmove(string1+32, string1+48, 177);
	memmove(string2+32, string2+48, 177);
	SLCHECK_Z(A_stricmp(string1, string2)); // Failure("A_memmove");
	//printf("\n\nTests passed OK\n");
	return CurrentStatus;
}
