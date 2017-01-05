#ifndef ESA_H
#define ESA_H

#include <limits.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <slib.h>
//
// defs {
//
#define DEBUG           0
#define BWT             1
#define BLOCK_ESA_SIZE  327680          // size of buffer (BLOCK_ESA_SIZE > 0)
#define C_BUFFER_SIZE   10000           // size of composed suffix
#define C_OVERFLOW_SIZE 10000           // size of composed suffix
#define _OUTPUT_BUFFER  0
#define OUTPUT_SIZE     7456540         // size of output buffer
#define _PREFIX_ASSY    1
#if _PREFIX_ASSY                                        // size of buffer (PREFIX_SIZE <= C_BUFFER_SIZE)
	#define PREFIX_SIZE 10
#endif
#define INDUCED_BUFFER 1
#define INDUCED_SIZE   4194304         // size of induced buffer (16Mb)
#define MAX_FASTA      104857600       //used in CAT input mode
#define DNA            1
#define PROTEIN        0
#if DNA
	#define SIGMA      4 // size of \Sigma
	#define INPUT_CAT  0 //input concatenado (1 file com N fastas)
#elif PROTEIN
	#define SIGMA     21 // size of \Sigma
	#define INPUT_CAT  1 //input concatenado (1 file com N fastas)
#endif
//
// } defs
//
//
// tad {
//
typedef int IntSuff;
typedef int IntLCP;

#if DNA
	typedef int8 IntText;
#elif PROTEIN
	typedef int IntText;
#endif

#pragma pack(1)

struct t_ESA {
	IntSuff sa;
	IntLCP lcp;
	int8 prefix[PREFIX_SIZE];
	int8 c_ant; // bwt
};

struct t_GSA {
	IntText text;
	IntSuff suff;
	IntLCP lcp;
	int8 bwt;
};

struct t_INDUCED {
	IntText text;
	IntLCP lcp;
};

struct t_TEXT { // Text struct
	char * c_file;                // associated filename
	FILE * f_in;                  // pointer to the text file
#if !INPUT_CAT
	FILE * f_ESA;                // pointer to the ESA input file
#endif
	t_ESA * ESA;
	int8 * c_buffer;              // buffer used for I/O	-> prefix
	int8 * c_overflow;    // buffer used for I/O
#if INPUT_CAT
	size_t begin;                           // start and end in input.bin
	int seeked;
#endif
	IntSuff length;                        // length of partition
	//EsaHeap sort
	IntText key;                           // indetifier of sequence
	uint   u_idx;
	IntLCP i_height;
	IntLCP i_loaded;
};

#pragma pack()
//
// } tad
//
// file {
//
int  open_input(t_TEXT * Text, const char * c_dir, const char * c_file, IntText n);
void close(FILE* File);

int write_file(t_TEXT * Text, char * c_dir, char* c_file, int n);
int read_file(t_TEXT * Text, char * c_dir, char* cFile, IntText n);
int  open_sequence(t_TEXT * Text, const char * c_dir, const char * c_file);
void close_sequence(t_TEXT * Text);
void seek_sequence(FILE * File, size_t pos);
int  write_sequence(t_TEXT * Text, const char * c_dir, const char * c_file);
void load_sequence(t_TEXT * Text);
int8 read_sequence(t_TEXT * Text);
int open_fasta(t_TEXT * Text, char * c_file, size_t begin);
int load_fasta(t_TEXT * Text);
void remove_file(const char * c_file);
//
// } file
//
//
// EsaHeap {
//

//typedef struct heap_node heap_node;
typedef t_TEXT heap_node;

struct EsaHeap {
	size_t GetSize() const
	{
		return size;
	}
	heap_node ** P_Heap;
	IntLCP * lcp_son_dad;
	IntLCP * lcp_left_right;
	size_t size;	
	FILE * f_out_ESA;			// pointer to the ESA output file
#if INPUT_CAT
	FILE * f_in_ESA;		// pointer to the ESA input file
	FILE * f_in_seq;		// pointer to the sequence input file
#endif
	size_t induced[SIGMA+1];
	char   cSIGMA[SIGMA+1][200];
	FILE * fSIGMA[SIGMA+1];
#if _OUTPUT_BUFFER
	t_GSA * out_buffer;
	int    size_out_buffer;
#endif
#if INDUCED_BUFFER
	t_INDUCED * induced_buffer[SIGMA+1];
	int    size_induced_buffer[SIGMA+1];
#endif //INDUCED_BUFFER
	//lcp
	IntLCP lcp_induced[SIGMA+1];			
};

EsaHeap * heap_alloc_induced(int n, const char * c_dir, const char * c_file);
int heap_free(EsaHeap *h, const char * c_dir);

#define mod_aux(a, b) (a % b)

//is_less
int is_less(EsaHeap * h, heap_node * node1, heap_node * node2);
int compare(EsaHeap * h, heap_node * node1, heap_node * node2, IntLCP * lcp);
int is_less_left_right(EsaHeap *h, heap_node *node1, heap_node * node2, IntLCP * lcp);
int is_less_down(EsaHeap *h, heap_node *node1, heap_node * node2, IntLCP * LCP, IntLCP lcp);
int heap_insert(EsaHeap *h, int key, heap_node* node, uint idx);
int heap_update(EsaHeap *h, int key, heap_node* node, uint idx);
int heap_delete_min(EsaHeap *h);
int heap_delete_min_last(EsaHeap *h);
int heap_pass_induced(EsaHeap *h, t_TEXT *Text, size_t *n, int8 alfa);
int heap_pass_induced_last(EsaHeap *h, t_TEXT *Text, size_t *pos, int8 alfa);
int load_buffer(EsaHeap *h, heap_node *node, int8** pt, IntLCP length);
int print_node(heap_node *node);
int heap_lcp(EsaHeap *h, size_t i_node);
//
// } EsaHeap
//
// sais {
//
//
// find the suffix array SA of T[0..n-1]
// use a working space (excluding T and SA) of at most 2n+O(lg n)
//
int sais(const uchar * T, IntSuff * SA, IntLCP * LCP, int n);
int sais_lcp(const void * T, IntSuff * SA, IntLCP * LCP, int n);
//
// find the suffix array SA of T[0..n-1] in {0..k-1}^n
// use a working space (excluding T and SA) of at most MAX(4k,2n)
//
int sais_int(const int * T, IntSuff * SA, int n, int k);
//
// burrows-wheeler transform
//
int sais_bwt(const uchar * T, uchar * U, int * A, int n);
int sais_int_bwt(const int * T, int * U, int * A, int n, int k);
//
// } sais
//
int merge_sort(t_TEXT * Text, IntText n, size_t * size, const char * c_dir, const char * c_file);
int preprocessing(t_TEXT * Text, const char * c_dir, const char * c_file, IntText n);
double time_stop(time_t t_time, clock_t c_clock);
void validate(t_TEXT * Text, IntText n, const char * c_dir, const char* c_file);
//
//
//
int esa_malloc(t_TEXT *Text);
int esa_open(t_TEXT *Text);
int esa_close(t_TEXT *Text);

void esa_seek(FILE *File, size_t pos);
void esa_read(t_ESA *ESA, FILE *File);

void esa_read_sa(unsigned *SA, size_t n, FILE *File);
void esa_read_gsa(t_GSA *GSA, size_t n, FILE *File);

int esa_read_all(t_TEXT *Text);

int esa_print(t_TEXT *Text, size_t size);
int esa_print_suff(IntSuff *SA, IntLCP *LCP, size_t size);
int esa_print_gsa(t_GSA *GSA, size_t size);

void esa_write_induced(EsaHeap *h, heap_node *node, int8 alfa, IntLCP lcp);
void esa_write(EsaHeap *h, heap_node *node, IntLCP lcp);
void induce(EsaHeap *h, heap_node *node, IntLCP lcp);
int esa_write_all(IntSuff* SA, IntLCP* LCP, t_TEXT *Text, const char * c_dir, const char * c_file);
int esa_build(t_TEXT *Text, IntText n, int sigma, const char *c_dir, const char* c_file);

#endif
