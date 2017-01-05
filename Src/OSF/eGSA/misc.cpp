// 
//
#include "esa.h"

void print1(int8* a, unsigned n, const char * comment);
int is_sorted_GSA(t_TEXT * Text, size_t size, IntText n, t_TEXT * t_Aux);
int is_LCP_GSA(t_TEXT * Text, size_t size, IntText n, t_TEXT * t_Aux);
int is_permutation_GSA(t_TEXT * Text, size_t size, IntText n, t_TEXT * t_Aux);
int is_BWT(t_TEXT * Text, size_t size, IntText n, t_TEXT * t_Aux);

//read fasta and convert it (sequence.bin)
int preprocessing(t_TEXT * Text, const char * c_dir, const char * c_file, IntText n)
{ //ok
	printf("### PREPROCESSING ###\n");
	// @unref char c_aux[200];
#if INPUT_CAT
	sprintf(c_aux, "%s%s.bin\0", c_dir, c_file);
	remove_file(c_aux);
#endif
	size_t begin = 0;
	size_t sum = 0;
	for(IntText i = 0; i < n; i++) {
#if INPUT_CAT
		char c_input[200];
		sprintf(c_input, "%s%s\0", c_dir, c_file);
		open_fasta(&Text[i], c_input, begin);
#else
		open_fasta(&Text[i], Text[i].c_file, begin);
#endif
		begin = load_fasta(&Text[i]);
		write_sequence(&Text[i], c_dir, c_file);
#if DEBUG
		printf("T_%d\t%d\n", i, Text[i].length);
#endif
		free(Text[i].c_buffer);
#if INPUT_CAT
		Text[i].begin = sum;
		sum += ++Text[i].length;
#endif
	}
#if INPUT_CAT
	write_file(Text, c_dir, c_file, n);
#endif
	return 0;
}

/**********************************************************************/
void validate(t_TEXT * Text, IntText n, const char * c_dir, const char * c_file)
{
	size_t size = 0;
	IntText k = 0;
	for(; k < n; k++) {
		open_sequence(&Text[k], c_dir, c_file); //open bin
#if INPUT_CAT
		seek_sequence(Text[k].f_in, Text[k].begin);                             //seek
#else
		seek_sequence(Text[k].f_in, 0);                                                 //seek
#endif
		load_sequence(&Text[k]);                //load sequence
		size += Text[k].length;
	}
	t_TEXT * t_Aux = (t_TEXT*)malloc(sizeof(t_TEXT));
	t_Aux->c_file = (char*)malloc(sizeof(char)*200);
	sprintf(t_Aux->c_file, "%s%s\0", c_dir, c_file);
	printf("%s\n", t_Aux->c_file);
	/*
	   #if INPUT_CAT
	        strcat (c_aux, ".esa");
	   #endif
	 */

	esa_open(t_Aux);
	esa_malloc(t_Aux);
	/**/
	if(!is_sorted_GSA(Text, size, n, t_Aux)) 
		printf("isNotSorted!!\n");
	else 
		printf("isSorted!!\n");
	esa_close(t_Aux);
	/**/
	esa_open(t_Aux);
	esa_malloc(t_Aux);
	if(!is_LCP_GSA(Text, size, n, t_Aux)) 
		printf("isNotLCP!!\n");
	else 
		printf("isLCP!!\n");
	esa_close(t_Aux);
	/**/
	esa_open(t_Aux);
	esa_malloc(t_Aux);
	if(!is_BWT(Text, size, n, t_Aux)) 
		printf("isNotBWT!!\n");
	else 
		printf("isBWT!!\n");
	esa_close(t_Aux);
	/**/

	/**/
	esa_open(t_Aux);
	esa_malloc(t_Aux);
	if(!is_permutation_GSA(Text, size, n, t_Aux)) 
		printf("isNotPermutation!!\n");
	else 
		printf("isPermutation!!\n");
	esa_close(t_Aux);
}

/**********************************************************************/

double time_stop(time_t t_time, clock_t c_clock)
{
	double aux1 = (clock() - c_clock) / (double)(CLOCKS_PER_SEC);
	double aux2 = difftime(time(NULL), t_time);
	printf("CLOCK = %lf TIME = %lf\n", aux1, aux2);
	return aux2;
}

/**********************************************************************/

void print1(int8 * a, uint n, const char * comment)
{ //1 byte
	printf("%s\n", comment);
	for(uint i = n-5; i < n; i++) {
		printf("%d, %d\n", i, a[i]);
	}
	printf("\n");
}

void print4(uint32 * a, uint n, const char * comment) //4 bytes
{
	printf("%s:\n", comment);
	for(uint i = 0; i < n; i++)
		printf("%d\n", a[i]);
}

/**********************************************************************/

int sleq(int8 * s1, int8 * s2) //ok
{ 
	int    r = 0;
	if(!s1[0] && !s2[0])
		r = 1;
	else if(s1[0] < s2[0]) 
		r = 1;
	else if(s1[0] > s2[0]) {
		printf("$%d, %d$ (zero)\n", s1[0], s2[0]);
		r = 0;
	}
	else {
		r = sleq(s1+1, s2+1); // @recursion
		if(!r)
			printf("$%d, %d$\n", s1[0], s2[0]);
	}
	return r;
}

/***********************************************************************/

int is_LCP_GSA(t_TEXT * Text, size_t size, IntText n, t_TEXT * t_Aux)
{
	size_t lcp_media = 0;
	size_t pos;
	t_GSA * GSA = (t_GSA*)malloc((BLOCK_ESA_SIZE+1) * sizeof(t_GSA));
	if(!GSA) 
		perror("is_LCP_GSA");
	esa_read_gsa(GSA, BLOCK_ESA_SIZE+1, t_Aux->f_ESA);
	pos = ftell(t_Aux->f_ESA);
	esa_seek(t_Aux->f_ESA, pos - (unsigned)sizeof(t_GSA));

	//esa_print_gsa(GSA, 20);

	for(size_t k = 0; k < size/BLOCK_ESA_SIZE+1; k++) {
		for(uint i = 0; i < BLOCK_ESA_SIZE; i++) {
			if(i+(k*BLOCK_ESA_SIZE) == size-1) {
				printf("LCP media = %zu\n", lcp_media/size);
				free(GSA);
				return 1;
			}
			lcp_media += GSA[i+1].lcp;
			IntLCP h = 0;
			for(h = 0; h < (IntLCP)size; h++) {
				if(Text[GSA[i].text].c_buffer[GSA[i].suff + h] != Text[GSA[i+1].text].c_buffer[GSA[i+1].suff + h])
					break;
				if(!Text[GSA[i].text].c_buffer[GSA[i].suff + h] && !Text[GSA[i+1].text].c_buffer[GSA[i+1].suff + h]) {
					h++;
					break;
				}
			}
			if(GSA[i+1].lcp != h) {
				printf("&%zu) [%u,%u], [%u, %u] -> [%d, %d]&\n", i+(k*BLOCK_ESA_SIZE),
				    GSA[i].text, GSA[i].suff, GSA[i+1].text, GSA[i+1].suff, GSA[i].suff, GSA[i+1].suff);
				printf("i = %zu - %zu\n", i+(k*BLOCK_ESA_SIZE), size-1);
				printf("lcp = %u, h = %u\n", GSA[i+1].lcp, h);
				for(IntLCP j = 0; j < (h+10); j++) {
					printf("%d) %d != %d\n", j, Text[GSA[i].text].c_buffer[GSA[i].suff + j], Text[GSA[i+1].text].c_buffer[GSA[i+1].suff + j]);
				}
				free(GSA);
				return 0;
			}
		}
		esa_read_gsa(GSA, BLOCK_ESA_SIZE+1, t_Aux->f_ESA);
		pos = ftell(t_Aux->f_ESA);
		esa_seek(t_Aux->f_ESA, pos - sizeof(t_GSA));
	}
	printf("LCP media = %zu\n", lcp_media/size);
	return 1;
}

/**********************************************************************/

int is_sorted_GSA(t_TEXT * Text, size_t size, IntText n, t_TEXT * t_Aux) { //ok
	printf("size = %zu\n", size);

	size_t k = 0;
	size_t pos;

	t_GSA * GSA = (t_GSA*)malloc((BLOCK_ESA_SIZE+1) * sizeof(t_GSA));
	if(!GSA) perror("is_sorted_GSA");

	esa_read_gsa(GSA, BLOCK_ESA_SIZE+1, t_Aux->f_ESA);
	pos = ftell(t_Aux->f_ESA);
	esa_seek(t_Aux->f_ESA, pos - sizeof(t_GSA));

	esa_print_gsa(GSA, 20);

	for(; k < size/BLOCK_ESA_SIZE+1; k++) {
		unsigned i;
		for(i = 0; i < BLOCK_ESA_SIZE; i++) {
			if(i+(k*BLOCK_ESA_SIZE) == size-1) {
				printf("k = %zu\n", k);
				free(GSA);
				return 1;
			}

			if(!sleq(Text[GSA[i].text].c_buffer+GSA[i].suff, Text[GSA[i+1].text].c_buffer + GSA[i+1].suff)) {
				printf("&%zu) [%u,%u], [%u, %u]&\n",
				    i+(k*BLOCK_ESA_SIZE),
				    GSA[i].text,
				    GSA[i].suff,
				    GSA[i+1].text,
				    GSA[i+1].suff);
				printf("i = %zu - %zu\n", i+(k*BLOCK_ESA_SIZE), size-1);

				free(GSA);
				return 0;
			}
		}
		esa_read_gsa(GSA, BLOCK_ESA_SIZE+1, t_Aux->f_ESA);
		pos = ftell(t_Aux->f_ESA);
		esa_seek(t_Aux->f_ESA, pos - sizeof(t_GSA));
	}

	return 1;
}

/**********************************************************************/

int is_permutation_GSA(t_TEXT * Text, size_t size, IntText n, t_TEXT * t_Aux) 
{ //ok
	size_t i, k;
	t_GSA * GSA = (t_GSA*)malloc((BLOCK_ESA_SIZE)*sizeof(t_GSA));
	if(!GSA) 
		perror("is_permutation_GSA");
	IntText y;
	for(y = 0; y < n; y++)
		for(i = 0; i < Text[y].length; i++)
			Text[y].c_buffer[i] = 0;  //zera todos
	esa_read_gsa(GSA, BLOCK_ESA_SIZE, t_Aux->f_ESA);
	for(k = 0; k < size/BLOCK_ESA_SIZE+1; k++) {
		for(i = 0; i < BLOCK_ESA_SIZE; i++) {
			if(i+(k*BLOCK_ESA_SIZE) == size)
				break;
			Text[GSA[i].text].c_buffer[GSA[i].suff] = 1;
			//printf("%d) %d, %d = %d\n", i+(k*BLOCK_ESA_SIZE), GSA[i].text, GSA[i].suff, GSA[i].suff);
		}
		esa_read_gsa(GSA, BLOCK_ESA_SIZE, t_Aux->f_ESA);
	}
	for(y = 0; y < n; y++)
		for(i = 0; i < Text[y].length; i++) {
			//printf("%d) %d\n", i, Text[0].c_buffer[i]);
			if(!Text[y].c_buffer[i])
				return 0;
		}

	return 1;
}

/**********************************************************************/

int is_BWT(t_TEXT * Text, size_t size, IntText n, t_TEXT * t_Aux) 
{ //ok
	size_t pos;
	t_GSA * p_gsa = (t_GSA*)malloc((BLOCK_ESA_SIZE+1) * sizeof(t_GSA));
	if(!p_gsa) 
		perror("is_sorted_GSA");
	esa_read_gsa(p_gsa, BLOCK_ESA_SIZE+1, t_Aux->f_ESA);
	pos = ftell(t_Aux->f_ESA);
	esa_seek(t_Aux->f_ESA, pos - sizeof(t_GSA));
	for(size_t k = 0; k < size/BLOCK_ESA_SIZE+1; k++) {
		for(uint i = 0; i < BLOCK_ESA_SIZE; i++) {
			if(i+(k*BLOCK_ESA_SIZE) == size-1) {
				free(p_gsa);
				return 1;
			}
			if(Text[p_gsa[i].text].c_buffer[p_gsa[i].suff-1] != p_gsa[i].bwt) {
				printf("&%zu) |%u|%u|, (%u)&\n", i+(k*BLOCK_ESA_SIZE), Text[p_gsa[i].text].c_buffer[p_gsa[i].suff-1],
				    Text[p_gsa[i].text].c_buffer[p_gsa[i].suff], p_gsa[i].bwt);
				printf("i = %zu - %zu\n", i+(k*BLOCK_ESA_SIZE), size-1);
				free(p_gsa);
				return 0;
			}
		}
		esa_read_gsa(p_gsa, BLOCK_ESA_SIZE+1, t_Aux->f_ESA);
		pos = ftell(t_Aux->f_ESA);
		esa_seek(t_Aux->f_ESA, pos - sizeof(t_GSA));
	}
	return 1;
}

int is_sorted_SA(IntSuff * SA, size_t size, t_TEXT * Text) 
{
	for(size_t i = 0; i < size-1; i++) {
		if(!sleq(Text->c_buffer+SA[i], Text->c_buffer+SA[i+1])) {
			printf("&%d) %d, %d&\n", i, SA[i], SA[i+1]);
			return 0;
		}
	}
	return 1;
}

void check_lcp_array(int8 * t, IntSuff * SA, size_t size, IntLCP * lcp)
{
	IntLCP h;
	for(size_t i = 1; i<size; i++) {
		size_t j = SA[i-1]; 
		size_t k = SA[i];
		for(h = 0; (j+h) < size && (k+h) < size; h++)
			if(t[j+h]!=t[k+h]) 
				break;
		if(lcp[i]!=h) {
			fprintf(stdout, "FATAL ERROR! Incorrect LCP value: lcp[%zu]=%d!=%d\n", i, lcp[i], h);
			return;
		}
	}
	fprintf(stdout, "LCP array OK!\n");
}

IntLCP lcp_media(IntLCP* LCP, size_t size)
{
	size_t media = 0;
	for(size_t i = 0; i < size; i++) {
		media += LCP[i];
	}
	return (IntLCP)media/size;
}
