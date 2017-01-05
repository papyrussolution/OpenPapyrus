/***********************************************************************

   eGSA: External Generalized Suffix Array Construction Algorithm
   Louza, aug 2012.

   Copyright (C) 2013 Felipe Louza <felipelouza@gmail.com>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "esa.h"

/**********************************************************************/

t_TEXT * EGSA_Text;
size_t EGSA_Size; // sizeof generalized suffix array

time_t t_start, t_total;
clock_t c_start, c_total;

int main(int argc, char ** argv) 
{
	SString db_path, db_name;
	uint pre_option = 1, validation = 1;
	// @unref unsigned induced;
	int n;

	sscanf(argv[1], "%u", &pre_option);
	(db_path = argv[2]).SetLastSlash();
	db_name = argv[3];
	sscanf(argv[4], "%d", &n); //n sequences
	if(argc > 5)
		sscanf(argv[5], "%u", &validation);

	printf("PRE = %d\n", pre_option);
	printf("SIGMA = %d\n", SIGMA);
	printf("DIR = %s\n", (const char *)db_path);
	printf("DATABASE = %s\n", (const char *)db_name);
	printf("SEQUENCES = %d\n", n);
	printf("VALIDATION = %d\n", validation);
	printf("\n");

	printf("BLOCK_ESA_SIZE = %d\n", BLOCK_ESA_SIZE);
	printf("C_BUFFER_SIZE = %d\n", C_BUFFER_SIZE);
	printf("C_OVERFLOW_SIZE = %d\n\n", C_OVERFLOW_SIZE);

	printf("_OUTPUT_BUFFER = %d\n", _OUTPUT_BUFFER);
#if _OUTPUT_BUFFER
	printf("OUTPUT_SIZE = %d\n", OUTPUT_SIZE);
#endif
	printf("\n##################\n");
	printf("PREFIX = %d\n\n", PREFIX_SIZE);
	printf("INDUCED_BUFFER = %d\n", INDUCED_BUFFER);
#if INDUCED_BUFFER
	printf("INDUCED_SIZE = %d\n", INDUCED_SIZE);
#endif
	printf("##################\n");
	printf("\n");
	printf("sizeof(t_TEXT) = %lu\n", sizeof(t_TEXT));
	printf("sizeof(heap_node) = %lu\n", sizeof(heap_node));
	printf("\n");

	printf("sizeof(t_ESA) = %lu\n", sizeof(t_ESA));
	printf("sizeof(t_GSA) = %lu\n", sizeof(t_GSA));
	printf("[text, suff, lcp] -> [%lu, %lu, %lu] bytes\n", sizeof(IntText), sizeof(IntSuff), sizeof(IntLCP));
	printf("\n");

	t_start = time(NULL);
	EGSA_Text = (t_TEXT*)malloc(sizeof(t_TEXT) * n);
	open_input(EGSA_Text, db_path.SetLastSlash(), db_name, n);
	if(pre_option == 1) {
		preprocessing(EGSA_Text, db_path, db_name, n);
	}
#if INPUT_CAT
	else {
		read_file(EGSA_Text, db_path, db_name, n);
	}
#endif
	time_stop(t_start, c_start);
	t_total = time(NULL);
	c_total = clock();
	//
	// phase 1
	//
	t_start = time(NULL);
	c_start =  clock();
	printf("\n### PHASE 1 ###\n");
	esa_build(EGSA_Text, n, SIGMA, db_path, db_name);
	time_stop(t_start, c_start);
	//
	// phase 2
	//
	t_start = time(NULL);
	c_start =  clock();
	printf("\n### PHASE 2 ###\n");
	merge_sort(EGSA_Text, n, &EGSA_Size, db_path, db_name);
	time_stop(t_start, c_start);

	printf("\n### TOTAL ###\n");
	printf("milisecond per byte = %.9lf\n", (time_stop(t_total, c_total)/(double)EGSA_Size)*1000000.0);
	printf("size = %zu\n", EGSA_Size);
	//
	// validation
	//
	if(validation == 1) {
		t_start = time(NULL);
		c_start =  clock();
		printf("\n### VALIDATION ###\n");
		validate(EGSA_Text, n, db_path, db_name);
		time_stop(t_start, c_start);
	}
	printf("\n");

	ZFREE(EGSA_Text);
	return 0;
}

