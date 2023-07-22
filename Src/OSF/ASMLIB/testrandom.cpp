/*************************** random.cpp **********************************
 * Author:        Agner Fog
 * Date created:  2013-09-09
 * Last modified: 2013-09-09
 * Project:       asmlib.zip
 * Source URL:    www.agner.org/optimize
 *
 * Description:
 * Test random number generators
 *
 * Instructions:
 * Compile for console mode and link with the appropriate version of asmlib
 *
 * Further documentation:
 * The file asmlib-instructions.pdf contains further documentation and
 * instructions.
 *
 * Copyright 2007-2011 by Agner Fog.
 * GNU General Public License http://www.gnu.org/licenses/gpl.html
 *****************************************************************************/

#include <pp.h>
#pragma hdrstop
//#include <stdio.h>
#include "asmlibran.h"
#include "randomc.h"
#include "sfmt.h"
#include "mersenne.cpp"
#include "mother.cpp"
#include "sfmt.cpp"

const int includeMother = 1;
const int useInitByArray = 1;

int main() {
	int i;
	uint32_t a, b, c;
	const int numseeds = 5;
	int seeds[numseeds] = {1, 2, 3, 4, 5};
	i = PhysicalSeed(seeds, 1);
	printf("\nSeed: %08X, physical seed source: %i\n", seeds[0], i);

	CRandomMersenneA mersa(0);
	CRandomMersenne mersc(0);
	mersa.RandomInit(seeds[0]);
	mersc.RandomInit(seeds[0]);
	MersenneRandomInit(seeds[0]);

	if(useInitByArray) {
		mersa.RandomInitByArray(seeds, numseeds);
		mersc.RandomInitByArray(seeds, numseeds);
		MersenneRandomInitByArray(seeds, numseeds);
	}

	printf("\nMersenne:");
	for(i = 0; i<1000; i++) {
		a = mersa.BRandom();
		b = MersenneBRandom();
		c = mersc.BRandom();
		if(a != b || a != c) {
			printf("\nerror: %08X %08X %08X", a, b, c);
			break;
		}
		else if(i == 0 || i == 99) {
			printf("\n %08X %08X %08X", a, b, c);
		}
	}
	printf("\n %8i %8i %8i", mersa.IRandom(0, 9999), mersc.IRandom(0, 9999), MersenneIRandom(0, 9999));
	printf("\n %8i %8i %8i", mersa.IRandomX(0, 9999), mersc.IRandomX(0, 9999), MersenneIRandomX(0, 9999));
	printf("\n %12.8f %12.8f %12.8f", mersa.Random(), mersc.Random(), MersenneRandom());

	CRandomMotherA motha(0);
	CRandomMother mothc(0);
	motha.RandomInit(seeds[0]);
	mothc.RandomInit(seeds[0]);
	MotherRandomInit(seeds[0]);

	printf("\n\nMother:");
	for(i = 0; i<1000; i++) {
		a = motha.BRandom();
		b = MotherBRandom();
		c = mothc.BRandom();
		if(a != b || a != c) {
			printf("\nerror: %08X %08X %08X", a, b, c);
			break;
		}
		else if(i == 0 || i == 99) {
			printf("\n %08X %08X %08X", a, b, c);
		}
	}
	printf("\n %8i %8i %8i", motha.IRandom(0, 9999), mothc.IRandom(0, 9999), MotherIRandom(0, 9999));
	printf("\n %12.8f %12.8f %12.8f", motha.Random(), mothc.Random(), MotherRandom());

	CRandomSFMTA sfmta(0, includeMother);
	CRandomSFMT sfmtc(0, includeMother);
	sfmta.RandomInit(1, includeMother);
	sfmtc.RandomInit(1);
	SFMTgenRandomInit(1, includeMother);

	if(useInitByArray) {
		sfmta.RandomInitByArray(seeds, numseeds, includeMother);
		sfmtc.RandomInitByArray(seeds, numseeds);
		SFMTgenRandomInitByArray(seeds, numseeds, includeMother);
	}

	printf("\n\nSFMT:");
	for(i = 0; i<1000; i++) {
		a = sfmta.BRandom();
		b = SFMTgenBRandom();
		c = sfmtc.BRandom();
		if(a != b || a != c) {
			printf("\nerror @%i: %08X %08X %08X", i, a, b, c);
			break;
		}
		else if(i == 0 || i == 99) {
			printf("\n %08X %08X %08X", a, b, c);
		}
	}
	printf("\n %8i %8i %8i", sfmta.IRandom(0, 9999), sfmtc.IRandom(0, 9999), SFMTgenIRandom(0, 9999));
	printf("\n %8i %8i %8i", sfmta.IRandomX(0, 9999), sfmtc.IRandomX(0, 9999), SFMTgenIRandomX(0, 9999));
	printf("\n %12.8f %12.8f %12.8f", sfmta.Random(), sfmtc.Random(), SFMTgenRandom());

	printf("\n");

	return 0;
}
