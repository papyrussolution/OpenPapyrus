#include "esa.h"
#include <math.h>
#include <direct.h>

EsaHeap * heap_alloc_induced(int n, const char * c_dir, const char * c_file) 
{ //ok
	int i;
	EsaHeap * h = (EsaHeap*)malloc(sizeof(EsaHeap));
	h->P_Heap = (heap_node**)malloc(n * sizeof(heap_node*));
	if(!h) 
		perror("heap_alloc_induced(h)");
	if(!h->P_Heap) 
		perror("heap_alloc_induced(h->EsaHeap)");
	h->lcp_son_dad = (IntLCP*)malloc(n * sizeof(IntLCP));
	h->lcp_left_right = (IntLCP*)malloc(n * sizeof(IntLCP));
	if(!h->lcp_son_dad) 
		perror("heap_alloc_induced(h->lcp_son_dad)");
	if(!h->lcp_left_right) 
		perror("heap_alloc_induced(h->lcp_left_right)");

#if _OUTPUT_BUFFER
	h->out_buffer = (t_GSA*)malloc( (OUTPUT_SIZE+1) * sizeof(t_GSA));
	if(!h->out_buffer) 
		perror("heap_alloc_induced(h->out_buffer)");
	h->size_out_buffer = 0;
#endif
#if INDUCED_BUFFER
	i = 1;
	for(; i < SIGMA+1; i++) {
		h->induced_buffer[i] = (t_INDUCED*)malloc(INDUCED_SIZE*sizeof(t_INDUCED));
		h->size_induced_buffer[i] = 0;
	}
#endif
	h->size = 0;
	char c_aux[200];
	sprintf(c_aux, "%stmp/\0", c_dir);
	mkdir(c_aux);
	i = 0;
	for(; i < SIGMA+1; i++) {
		sprintf(h->cSIGMA[i], "%stmp/%d.esa.tmp\0", c_dir, i);
		h->fSIGMA[i] = fopen(h->cSIGMA[i], "wb");
		if(!h->fSIGMA[i]) 
			perror("heap_alloc_induced(h->fSIGMA)");
		fseek(h->fSIGMA[i], 0, SEEK_SET);
		h->induced[i] = 0;
		h->lcp_induced[i] = INT_MAX;
	}
	char c_out_esa[500];
	sprintf(c_out_esa, "%s%s.esa\0", c_dir, c_file);
	h->f_out_ESA = fopen(c_out_esa, "wb");
	if(!h->f_out_ESA) 
		perror("heap_alloc_induced(h->f_out_ESA)");
	fseek(h->f_out_ESA, 0, SEEK_SET);
#if INPUT_CAT
	char c_in_esa[500];
	sprintf(c_in_esa, "%s%s.esa\0", c_dir, c_file);
	h->f_in_ESA = fopen(c_in_esa, "rb");
	if(!h->f_in_ESA) 
		perror("heap_alloc_induced(h->f_in_ESA)");
	char c_in_seq[500];
	sprintf(c_in_seq, "%s%s.bin\0", c_dir, c_file);
	h->f_in_seq = fopen(c_in_seq, "rb");
	if(!h->f_in_seq) 
		perror("heap_alloc_induced(h->f_in_seq)");
#endif
	return h;
}

int heap_free(EsaHeap * h, const char * c_dir) 
{
	free(h->P_Heap);
	int i = 0;
	for(; i < SIGMA+1; i++) {
		fclose(h->fSIGMA[i]);
	}
	free(h->lcp_left_right);
	free(h->lcp_son_dad);
#if _OUTPUT_BUFFER
	free(h->out_buffer);
#endif
	fclose(h->f_out_ESA);
#if INPUT_CAT
	fclose(h->f_in_ESA);
	fclose(h->f_in_seq);
#endif
	free(h);
	return 0;
}

void heapfy_up(EsaHeap * h, int i_son) 
{
	int i_dad = (int)floor((i_son-1.0)/2.0);
	while(i_son > 0 && is_less(h, h->P_Heap[i_son], h->P_Heap[i_dad])) {
		//swap pointers on EsaHeap
		heap_node * auxn = h->P_Heap[i_son];
		h->P_Heap[i_son] = h->P_Heap[i_dad];
		h->P_Heap[i_dad] = auxn;
		i_son = i_dad;
		i_dad = (int)floor((i_son-1.0)/2.0);
	}
}

static int heapfy_down(EsaHeap * h, size_t i_node) 
{
	size_t l = 2*i_node+1; //left child
	size_t r = l+1; //right child
	size_t i_son;
	// Selects the childs of i with smaller c_buffer:
	if(r < h->size) {
		if(h->lcp_son_dad[l] != h->lcp_son_dad[r])
			i_son = (h->lcp_son_dad[l] > h->lcp_son_dad[r]) ? l : r;
		else
			i_son = (is_less_left_right(h, h->P_Heap[l], h->P_Heap[r], &h->lcp_left_right[l])) ? l : r;
	}
	else if(l == h->size-1)
		i_son = l;
	else
		return 0;
	IntLCP lcp_son_ant = h->lcp_son_dad[i_son];
	heap_node * aux;
	// Swaps down:
	while(is_less_down(h, h->P_Heap[i_son], h->P_Heap[i_node], &h->lcp_son_dad[i_son], h->lcp_son_dad[i_node])) {
		h->lcp_son_dad[i_node] = lcp_son_ant;
		if(i_son == l && r < h->size) {
			h->lcp_son_dad[r] =  h->lcp_left_right[l];
			compare(h, h->P_Heap[l], h->P_Heap[r], &h->lcp_son_dad[r]);
		}
		else if(i_son == r) {
			h->lcp_son_dad[l] =  h->lcp_left_right[l];
			compare(h, h->P_Heap[l], h->P_Heap[r], &h->lcp_son_dad[l]);
		}
		h->lcp_left_right[l] = min(h->lcp_son_dad[i_son], h->lcp_left_right[l]);
		aux = h->P_Heap[i_node];
		h->P_Heap[i_node] = h->P_Heap[i_son];
		h->P_Heap[i_son] = aux;
		i_node = i_son;
		l = 2*i_node+1;
		r = l+1;
		if(r < h->size) {
			if(h->lcp_son_dad[l] != h->lcp_son_dad[r])
				i_son = (h->lcp_son_dad[l] > h->lcp_son_dad[r]) ? l : r;
			else
				i_son = (is_less_left_right(h, h->P_Heap[l], h->P_Heap[r], &h->lcp_left_right[l])) ? l : r;
		}
		else if(l == h->size-1) {
			i_son = l;
		}
		else
			return 0;
		lcp_son_ant = h->lcp_son_dad[i_son];
	}
	if(i_son == l && r < h->size) {
		h->lcp_son_dad[r] = min(h->lcp_left_right[l], h->lcp_son_dad[l]);
		compare(h, h->P_Heap[r], h->P_Heap[i_node], &h->lcp_son_dad[r]);
	}
	else if(i_son == r) {
		h->lcp_son_dad[l] = min(h->lcp_left_right[l], h->lcp_son_dad[r]);
		compare(h, h->P_Heap[l], h->P_Heap[i_node], &h->lcp_son_dad[l]);
	}
	return 0;
}

int heap_lcp(EsaHeap * h, size_t i_node) 
{ //updates lcp
	size_t l = 2*i_node+1; //left child
	size_t r = l+1; //right child
	if(l < h->size) {
		compare(h, h->P_Heap[i_node], h->P_Heap[l], &h->lcp_son_dad[l]);
		heap_lcp(h, l); // @recursion
	}
	if(r < h->size) {
		compare(h, h->P_Heap[i_node], h->P_Heap[r], &h->lcp_son_dad[r]);
		heap_lcp(h, r); // @recursion
	}
	return 0;
}

/**********************************************************************/

int heap_insert(EsaHeap * h, int key, heap_node* node, uint u_idx) 
{ //ok
	//insert key in the end of EsaHeap
	h->P_Heap[h->size] = node;
	memcpy(node->c_buffer, node->ESA[u_idx].prefix, PREFIX_SIZE);
	node->i_loaded = node->i_height = PREFIX_SIZE;
	node->c_buffer[PREFIX_SIZE] = SIGMA+1;
	h->size++;
	return 0;
}

int heap_update(EsaHeap * h, int key, heap_node* node, uint u_idx) 
{
	h->P_Heap[h->size] = node;
	if(!node->ESA[u_idx].lcp) {
		memcpy(node->c_buffer, node->ESA[u_idx].prefix, PREFIX_SIZE);
		node->i_loaded = node->i_height = PREFIX_SIZE;
		node->c_buffer[PREFIX_SIZE] = SIGMA+1;
	}
	heapfy_up(h, h->size++);
	return 0;
}

/*******************************************************************************/

int heap_delete_min(EsaHeap * h) 
{ //outputs min
	heap_node * node = h->P_Heap[0];
	//int8 alfa = h->EsaHeap[0]->c_buffer[0];

	int j = 1;
	for(; j < SIGMA+1; j++) //RMQ
		if(h->lcp_induced[j] > h->lcp_son_dad[0]) 
			h->lcp_induced[j] = h->lcp_son_dad[0];
	esa_write_induced(h, node, node->c_buffer[0], h->lcp_son_dad[0]);
	if(++node->u_idx == BLOCK_ESA_SIZE) {
		node->u_idx = 0;
#if INPUT_CAT
		esa_seek(h->f_in_ESA, ((node->begin + node->key) + (node->seeked++) * BLOCK_ESA_SIZE) * sizeof(t_ESA));
		esa_read(node->ESA, h->f_in_ESA);
#else
		esa_read(node->ESA, node->f_ESA);
#endif
	}

	// Mount PREFIX
	if(node->ESA[node->u_idx].lcp < node->i_height)
		node->i_height = node->ESA[node->u_idx].lcp;
	if(node->i_height < C_BUFFER_SIZE) {
		IntLCP aux = PREFIX_SIZE;
		if(node->i_height + PREFIX_SIZE > C_BUFFER_SIZE) 
			aux = C_BUFFER_SIZE - node->i_height;
		memcpy(node->c_buffer + node->i_height, node->ESA[node->u_idx].prefix, aux);
		node->c_buffer[node->i_height+aux] = SIGMA +1;
		node->i_height += aux;
	}
	node->i_loaded = node->i_height;
	h->lcp_son_dad[0] = node->ESA[node->u_idx].lcp;
	if(h->lcp_son_dad[0] <= h->lcp_son_dad[1] || h->lcp_son_dad[0] <= h->lcp_son_dad[2])
		heapfy_down(h, 0);
	return 0;
}

/**********************************************************************/

int heap_pass_induced(EsaHeap * h, t_TEXT * Text, size_t * pos, int8 alfa) 
{
	//induced_buffer
#if INDUCED_BUFFER
	fwrite(h->induced_buffer[alfa], sizeof(t_INDUCED), h->size_induced_buffer[alfa], h->fSIGMA[alfa]);
#endif //INDUCED_BUFFER
	*(pos) += h->induced[alfa];
	fclose(h->fSIGMA[alfa]);
	printf("%d) total = %zu\n", alfa, h->induced[alfa]);
	h->fSIGMA[alfa] = fopen(h->cSIGMA[alfa], "rb");
	if(!h->fSIGMA[alfa]) 
		perror("heap_pass_induced");
	fseek(h->fSIGMA[alfa], 0, SEEK_SET);
	t_INDUCED induced;
	fread(&induced, sizeof(t_INDUCED), 1, h->fSIGMA[alfa]);
	induced.lcp = 0;
	size_t i = 0;
	for(; i < h->induced[alfa]; i++) {
		int j = 1;
		for(; j < SIGMA+1; j++) //RMQ
			if(h->lcp_induced[j] > induced.lcp)
				h->lcp_induced[j] = induced.lcp;
		esa_write_induced(h, &Text[induced.text], alfa, h->lcp_induced[alfa]);
		h->lcp_induced[alfa] = INT_MAX;
		if(++Text[induced.text].u_idx == BLOCK_ESA_SIZE) {
			Text[induced.text].u_idx = 0;
#if INPUT_CAT
			esa_seek(h->f_in_ESA, ((Text[induced.text].begin +
				Text[induced.text].key) + (Text[induced.text].seeked++)*BLOCK_ESA_SIZE) * sizeof(t_ESA));
			esa_read(Text[induced.text].ESA, h->f_in_ESA);
#else
			esa_read(Text[induced.text].ESA, Text[induced.text].f_ESA);
#endif
		}
		fread(&induced, sizeof(t_INDUCED), 1, h->fSIGMA[alfa]);
	}
	return 0;
}

/*******************************************************************************/

int load_buffer(EsaHeap * h, heap_node * node, int8** pt, IntLCP length)
{
	//carrega buffer (c_buffer)
#if INPUT_CAT
	seek_sequence(h->f_in_seq, (size_t)node->ESA[node->u_idx].sa + node->begin + (size_t)length);
#else
	seek_sequence(node->f_in, (size_t)node->ESA[node->u_idx].sa + (size_t)length);
#endif //INPUT_CAT
	if(C_BUFFER_SIZE - (int)length > 0) {
#if INPUT_CAT
		fread(*pt, sizeof(int8), C_BUFFER_SIZE - length, h->f_in_seq);
#else
		fread(*pt, sizeof(int8), C_BUFFER_SIZE - length, node->f_in);
#endif
		node->c_buffer[C_BUFFER_SIZE] = SIGMA +1;
		node->i_loaded = C_BUFFER_SIZE-1;
	}
	else{
#if INPUT_CAT
		fread(node->c_overflow, sizeof(int8), C_OVERFLOW_SIZE, h->f_in_seq);
#else
		fread(node->c_overflow, sizeof(int8), C_OVERFLOW_SIZE, node->f_in);
#endif
		node->c_overflow[C_OVERFLOW_SIZE] = SIGMA +1;
		*pt = node->c_overflow;
	}
	return 0;
}

/**********************************************************************/

int is_less_down(EsaHeap * h, heap_node * node_son, heap_node * node_dad, IntLCP* lcp_AB, IntLCP lcp_AD)
{
	if(lcp_AD < (*lcp_AB)) {
		(*lcp_AB) = lcp_AD;
		return 1;
	}
	else if(lcp_AD > (*lcp_AB)) {
		return 0;
	}
	IntLCP min = min(node_dad->i_loaded, node_son->i_loaded);
	if((*lcp_AB) >= min) {
		(*lcp_AB) = min-1;
	}
	if(*lcp_AB) (*lcp_AB)--;
	int8 * p1 = &node_son->c_buffer[*lcp_AB];
	int8 * p2 = &node_dad->c_buffer[*lcp_AB];
	if((*p1) == SIGMA+1 && (*p2) == SIGMA+1) {
		return 0;
	}
	while(*p1 && *p2) {
		if(*p1 < *p2) 
			return 1;
		else if(*p1 > *p2) 
			return 0;
		(*lcp_AB)++; //assert(*lcp_AB >= 0);
		p1++; p2++;
		if(*p1 == SIGMA+1) {
			load_buffer(h, node_son, &p1, *lcp_AB);
		}
		if(*p2 == SIGMA+1) {
			load_buffer(h, node_dad, &p2, *lcp_AB);
		}
	}
	if(!(*p1) && !(*p2)) {
		(*lcp_AB)++;
		return (node_son->key < node_dad->key ? 1 : 0);
	}
	else{
		return ((*p1) < (*p2) ? 1 : 0);
	}
}

int is_less_left_right(EsaHeap * h, heap_node * node_left, heap_node * node_right, IntLCP* lcp_BD)
{
	IntLCP min = min(node_left->i_loaded, node_right->i_loaded);
	if((*lcp_BD) >= min) {
		(*lcp_BD) = min-1;
	}
	int8 * p1 = &node_left->c_buffer[*lcp_BD];
	int8 * p2 = &node_right->c_buffer[*lcp_BD];
	/**/
	if((*p1) == SIGMA+1 && (*p2) == SIGMA+1) {
		return 0;
	}
	while(*p1 && *p2) {
		if(*p1 < *p2) 
			return 1;
		else if(*p1 > *p2) 
			return 0;
		(*lcp_BD)++; //assert(*lcp_BD >= 0);
		p1++; p2++;
		if(*p1 == SIGMA+1) {
			load_buffer(h, node_left, &p1, *lcp_BD);
		}
		if(*p2 == SIGMA+1) {
			load_buffer(h, node_right, &p2, *lcp_BD);
		}
	}
	if(!(*p1) && !(*p2)) {
		return (node_left->key < node_right->key ? 1 : 0);
	}
	else{
		return ((*p1) < (*p2) ? 1 : 0);
	}
}

int compare(EsaHeap * h, heap_node * node1, heap_node * node2, IntLCP* lcp)
{ //[left, right], [son, dad]
	IntLCP min = min(node1->i_loaded, node2->i_loaded);
	if((*lcp) >= min) {
		(*lcp) = min-1;
	}
	int8 * p1 = &node1->c_buffer[*lcp];
	int8 * p2 = &node2->c_buffer[*lcp];
	/**/
	if((*p1) == SIGMA+1 && (*p2) == SIGMA+1) {
		return 0;
	}
	while(*p1 && *p2) {
		if(*p1 != *p2) 
			return 0;
		(*lcp)++; //assert(*lcp >= 0);
		p1++; p2++;
		if(*p1 == SIGMA+1) {
			load_buffer(h, node1, &p1, *lcp);
		}
		if(*p2 == SIGMA+1) {
			load_buffer(h, node2, &p2, *lcp);
		}
	}
	if(!(*p1) && !(*p2)) {
		(*lcp)++;
	}
	return 0;
}

int is_less(EsaHeap * h, heap_node * node1, heap_node * node2)
{ //[left, right], [son, dad]
	IntLCP i = 0;
	int8 * p1 = &node1->c_buffer[i];
	int8 * p2 = &node2->c_buffer[i];
	while(*p1 && *p2) {
		if(*p1 < *p2) 
			return 1;
		else if(*p1 > *p2) 
			return 0;
		i++; //assert(i >= 0);
		p1++; p2++;
		if(*p1 == SIGMA+1) {
			load_buffer(h, node1, &p1, i);
		}
		if(*p2 == SIGMA+1) {
			load_buffer(h, node2, &p2, i);
		}
	}
	if(!(*p1) && !(*p2)) {
		return (node1->key < node2->key ? 1 : 0);
	}
	else{
		return ((*p1) < (*p2) ? 1 : 0);
	}
}

int print_node(heap_node * node)
{
	printf("\n[%d, %d] (lcp = %d) [%d] mem: ",
	    node->key,
	    node->ESA[node->u_idx].sa,
	    node->ESA[node->u_idx].lcp,
	    node->ESA[node->u_idx].c_ant);
	int8 * pt = node->c_buffer;
	size_t i = 0;
	for(; i < 2*C_BUFFER_SIZE+2; i++) {
		if(i < C_BUFFER_SIZE) {
			printf("|%d| ", *(pt++));
		}
		if(i == C_BUFFER_SIZE) {
			printf("-%d- ", *(pt++));
			pt = node->c_overflow;
		}
		if(i > C_BUFFER_SIZE) {
			printf(" %d  ", *(pt++));
		}
	}
	printf("\n");
	return 0;
}

