#include "esa.h"

/**********************************************************************/
int esa_open(t_TEXT * Text)
{
	char c_aux[200];
	sprintf(c_aux, "%s.esa\0", Text->c_file);
	Text->f_ESA = fopen(c_aux, "rb"); //rb
	if(!Text->f_ESA) 
		perror("esa_malloc(f_ESA)");
	fseek(Text->f_ESA, 0, SEEK_SET);
	return 0;
}

int esa_malloc(t_TEXT * Text)
{
	Text->ESA = (t_ESA*)malloc(sizeof(t_ESA) * (BLOCK_ESA_SIZE + 1));
	if(!Text->ESA) 
		perror("esa_malloc(ESA)");
	Text->c_buffer = (int8*)malloc(sizeof(int8) * (C_BUFFER_SIZE + 2));
	if(!Text->c_buffer) 
		perror("esa_malloc(C_buffer)");
	Text->c_overflow = (int8*)malloc(sizeof(int8) * (C_OVERFLOW_SIZE + 2));
	if(!Text->c_overflow) 
		perror("esa_malloc(c_overflow)");
	return 0;
}

int esa_close(t_TEXT * Text)
{
#if !INPUT_CAT
	fclose(Text->f_ESA);
#endif
	free(Text->ESA);
	return 0;
}

/**********************************************************************/

void esa_seek(FILE * File, size_t pos) 
{
	fseek(File, pos, SEEK_SET);
}

void esa_read(t_ESA * ESA, FILE * File) 
{
	fread(ESA, sizeof(t_ESA), BLOCK_ESA_SIZE, File);
}

void esa_read_sa(unsigned * SA, size_t n, FILE * File) 
{
	fread(SA, sizeof(unsigned), n, File);
}

void esa_read_gsa(t_GSA * GSA, size_t n, FILE * File) 
{
	fread(GSA, sizeof(t_GSA), n, File);
}

int esa_read_all(t_TEXT * Text) 
{
	Text->ESA = (t_ESA*)malloc(sizeof(t_ESA) * Text->length);
	if(!Text->ESA) 
		perror("esa_read_all");
	//fread(Text->ESA, sizeof(t_ESA), Text->length, Text->f_ESA);
	return 0;
}

int esa_print(t_TEXT * Text, size_t size)
{
	printf("esa\n");
	size_t end = BLOCK_ESA_SIZE+1;
	if(Text->length < end) 
		end = Text->length+1;
	size_t j, i = end - size;
	for(i = 0; i < size; i++) {
		printf("%zu\t|%d\t(%d)\t", i, Text->ESA[i].sa, Text->ESA[i].lcp);
		for(j = 0; j < PREFIX_SIZE; j++) {
			printf("%d ", Text->ESA[i].prefix[j]);
		}
		printf("\t|%u|\n", Text->ESA[i].c_ant);
	}
	return 0;
}

int esa_print_suff(IntSuff * SA, IntLCP * LCP, size_t size)
{
	size_t i = 0;
	for(; i < size; i++) {
		printf("%zu\t|%u\t(%u)\n", i, SA[i], LCP[i]);
	}
	return 0;
}

int esa_print_gsa(t_GSA * GSA, size_t size)
{
	printf("i\t|text, suff \t(lcp) \t(bwt)\n");
	for(size_t i = 0; i < size; i++) {
		printf("%zu\t|%u, %u \t(%u) \t(%d)\n", i, GSA[i].text, GSA[i].suff, GSA[i].lcp, GSA[i].bwt);
	}
	return 0;
}

void esa_write(EsaHeap * h, heap_node * node, IntLCP lcp)
{
#if _OUTPUT_BUFFER
	if(h->size_out_buffer == OUTPUT_SIZE) {       //output_buffer
		h->size_out_buffer = 0;
		fwrite(h->out_buffer, sizeof(t_GSA), OUTPUT_SIZE, h->f_out_ESA);
	}
	h->out_buffer[h->size_out_buffer].text = node->key;
	h->out_buffer[h->size_out_buffer].suff = node->ESA[node->u_idx].sa;
#if BWT
	h->out_buffer[h->size_out_buffer].bwt = node->ESA[node->u_idx].c_ant;
#endif
	h->out_buffer[h->size_out_buffer++].lcp = lcp;
#else
	fwrite(&node->key, sizeof(IntText), 1, h->f_out_ESA);
	fwrite(&node->ESA[node->u_idx].sa, sizeof(IntSuff), 1, h->f_out_ESA);
	fwrite(&lcp, sizeof(IntLCP), 1, h->f_out_ESA);
#if BWT
	fwrite(&node->ESA[node->u_idx].c_ant, sizeof(int8), 1, h->f_out_ESA);
#endif
#endif //_OUTPUT_BUFFER
}

void esa_write_induced(EsaHeap * h, heap_node * node, int8 alfa, IntLCP lcp) 
{
#if _OUTPUT_BUFFER
	if(h->size_out_buffer == OUTPUT_SIZE) {
		h->size_out_buffer = 0;
		fwrite(h->out_buffer, sizeof(t_GSA), OUTPUT_SIZE, h->f_out_ESA);
	}
	h->out_buffer[h->size_out_buffer].text = node->key;
	h->out_buffer[h->size_out_buffer].suff = node->ESA[node->u_idx].sa;
#if BWT
	h->out_buffer[h->size_out_buffer].bwt = node->ESA[node->u_idx].c_ant;
#endif
	h->out_buffer[h->size_out_buffer++].lcp = lcp;
#else
	fwrite(&node->key, sizeof(IntText), 1, h->f_out_ESA);
	fwrite(&node->ESA[node->u_idx].sa, sizeof(IntSuff), 1, h->f_out_ESA);
	fwrite(&lcp, sizeof(IntLCP), 1, h->f_out_ESA);
#if BWT
	fwrite(&node->ESA[node->u_idx].c_ant, sizeof(int8), 1, h->f_out_ESA);
#endif
#endif //_OUTPUT_BUFFER
	if(node->ESA[node->u_idx].c_ant > alfa)
		induce(h, node, h->lcp_induced[node->ESA[node->u_idx].c_ant]+1);
}

void induce(EsaHeap* h, heap_node * node, IntLCP lcp)
{
#if INDUCED_BUFFER
	if(h->size_induced_buffer[node->ESA[node->u_idx].c_ant] == INDUCED_SIZE) {       //induced_buffer
		h->size_induced_buffer[node->ESA[node->u_idx].c_ant] = 0;
		fwrite(h->induced_buffer[node->ESA[node->u_idx].c_ant], sizeof(t_INDUCED), INDUCED_SIZE,
		    h->fSIGMA[node->ESA[node->u_idx].c_ant]);
	}
	h->induced_buffer[node->ESA[node->u_idx].c_ant][h->size_induced_buffer[node->ESA[node->u_idx].c_ant]].text = node->key;
	h->induced_buffer[node->ESA[node->u_idx].c_ant][h->size_induced_buffer[node->ESA[node->u_idx].c_ant]++].lcp = lcp;
#else
	fwrite(&node->key, sizeof(IntText), 1, h->fSIGMA[node->ESA[node->u_idx].c_ant]);
	fwrite(&lcp, sizeof(IntLCP), 1, h->fSIGMA[node->ESA[node->u_idx].c_ant]);
#endif
	h->lcp_induced[node->ESA[node->u_idx].c_ant] = INT_MAX;
	h->induced[node->ESA[node->u_idx].c_ant]++; //contar quantos foram induzidos;
}

// Formar ESA para gravar em disco..
int esa_write_all(IntSuff* SA, IntLCP* LCP, t_TEXT * Text, const char * c_dir, const char * c_file) 
{
	char c_aux[200];
	FILE * f_out;
#if INPUT_CAT
	sprintf(c_aux, "%s%s.esa\0", c_dir, c_file);
	f_out = fopen(c_aux, "ab");        //wb
#else
	sprintf(c_aux, "%s.esa\0", Text->c_file);
	f_out = fopen(c_aux, "wb");        //wb
#endif
	if(!f_out) 
		perror("esa_write_all_induced");
	IntSuff inicio;
	IntLCP i_height = 0;
	int8 c_ant;
	size_t i = 0;
	for(; i < Text->length; i++) {
		i_height += PREFIX_SIZE;
		if(LCP[i] < i_height) 
			i_height = LCP[i];
		inicio = SA[i] + i_height;
		if(Text->c_buffer[SA[i]] > Text->c_buffer[SA[i]+1]) {
			LCP[i+1] = 0;
			inicio = SA[i];
			i_height = PREFIX_SIZE;
		}
		//write the node into the file
		fwrite(&SA[i], sizeof(IntSuff), 1, f_out);
		fwrite(&LCP[i], sizeof(IntLCP), 1, f_out);
		fwrite(&Text->c_buffer[inicio], sizeof(int8), PREFIX_SIZE, f_out);
		//bwt
		c_ant = 0;
		if(SA[i] > 0) 
			c_ant = Text->c_buffer[SA[i] - 1];
		fwrite(&c_ant, sizeof(int8), 1, f_out);
	}
	//extra node [n, 0, 500]
	fwrite(&Text->length, sizeof(IntSuff), 1, f_out);
	i = 0;
	fwrite(&i, sizeof(IntLCP), 1, f_out);
	int8 aux2[PREFIX_SIZE+1] = {SIGMA+1};
	fwrite(aux2, sizeof(int8), PREFIX_SIZE, f_out);
	c_ant = 0;
	fwrite(&c_ant, sizeof(int8), 1, f_out);
	//fflush(Text->f_out);
	if(fclose(f_out)==EOF) 
		printf("error closing file %s.\n\n\n", c_aux);
	return 0;
}

/**********************************************************************/

int esa_build(t_TEXT * Text, IntText n, int sigma, const char * c_dir, const char * c_file)
{
#if INPUT_CAT
	char c_aux[200];
	sprintf(c_aux, "%s%s.esa\0", c_dir, c_file);
	remove_file(c_aux);
#endif
	IntSuff * SA;
	IntLCP * LCP;
	IntText k = 0;
	for(; k < n; k++) {
		open_sequence(&Text[k], c_dir, c_file);                         //open bin
#if INPUT_CAT
		seek_sequence(Text[k].f_in, Text[k].begin);                     //seek
#else
		seek_sequence(Text[k].f_in, 0);                                         //seek
#endif
		//#if DEBUG
		printf("T_%d\t%d\n", k, Text[k].length);
		//#endif
		load_sequence(&Text[k]);                                                        //load sequence
		Text[k].key = k;
		SA  = (IntSuff*)malloc((Text[k].length+1) * sizeof(IntSuff));
		if(!SA) 
			perror("esa_build");
		LCP = (IntLCP*)malloc((Text[k].length+1) * sizeof(IntLCP));
		if(!LCP) 
			perror("esa_build");
		sais_lcp(Text[k].c_buffer, SA, LCP, Text[k].length);
		/**************************************************************/
		//validate
#if DEBUG
		if(!is_sorted_SA(SA, Text[k].length, &Text[k]))
			printf("isNotSorted!!\n");
		else
			printf("isSorted!!\n");
		check_lcp_array(Text[k].c_buffer, SA, Text[k].length, LCP);
		printf("media(LCP) = %d\n", lcp_media(LCP, Text[k].length));
#endif //DEBUG
		/**************************************************************/
		Text[k].c_buffer[Text[k].length] = SIGMA+1; //after $, put # ($<A<C<G<T<#)
		//write on disk
		esa_write_all(SA, LCP, &Text[k], c_dir, c_file);
		/**************************************************************/
		free(SA);
		free(LCP);
		free(Text[k].c_buffer);
	}
	return 0;
}

