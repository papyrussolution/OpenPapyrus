#include "esa.h"

int print_delete(EsaHeap * H, size_t i, size_t size, IntText n)
{
	printf("\n#####%zu/%zu######", i, size);
	IntText k = 0;
	for(; k < n; k++)
		print_node(H->P_Heap[k]);
	for(k = 0; k < n; k++)
		printf("heap_son[%d] = %d\n", k, H->lcp_son_dad[k]);
	printf("-\n");
	for(k = 0; k < n; k++)
		printf("heap_left[%d] = %d\n", k, H->lcp_left_right[k]);

	return 0;
}

/**********************************************************************/

int merge_sort(t_TEXT * Text, IntText n, size_t * size, const char * c_dir, const char * c_file)
{
	*size = 0;
	//load all partition (beginning)

	EsaHeap * H = heap_alloc_induced(n, c_dir, c_file);
	IntText k = 0;
	for(; k < n; k++) {
		Text[k].key = k;

		#if !INPUT_CAT
		open_sequence(&Text[k], c_dir, c_file);        //open bin
		esa_open(&Text[k]);
		#endif

		esa_malloc(&Text[k]);

		#if INPUT_CAT
		Text[k].seeked = 0;
		esa_seek(H->f_in_ESA, ((Text[k].begin + k) + (Text[k].seeked++) * BLOCK_ESA_SIZE) * sizeof(t_ESA));
		esa_read(Text[k].ESA, H->f_in_ESA);
		#else
		esa_read(Text[k].ESA, Text[k].f_ESA);
		#endif

		#if DEBUG
		printf("T_%d\t%d\n", k, Text[k].length);
		esa_print(&Text[k], 5); printf("\n");
		#endif

		Text[k].u_idx = Text[k].i_height = Text[k].i_loaded = 0;

		*size += Text[k].length;
	}

	printf("size = %zu\n", *size);

	for(k = 0; k < n; k++) {
		H->lcp_son_dad[k] = H->lcp_left_right[k] = 0;
		heap_insert(H, k, &Text[k], 0);
	}

	heap_lcp(H, 0);
	printf("\n");

	int8 alfa = 0;
	size_t i = 0;

	for(; i < *size; i++) {
		if(H->P_Heap[0]->c_buffer[0] > alfa) {
			printf("i = %zu\n", i);
			alfa = H->P_Heap[0]->c_buffer[0];

			//RMQ
			int j = 1;
			for(; j < SIGMA+1; j++) H->lcp_induced[j] = 0;

			if(H->induced[alfa]) { //induceds > 0
				heap_pass_induced(H, Text, &i, alfa);
				H->size = 0;
				for(j = 0; j < n; j++) {
					H->lcp_son_dad[j] = H->lcp_left_right[j] = 0;
					heap_update(H, j, &Text[j], Text[j].u_idx);
				}
				if(H->P_Heap[0]->c_buffer[0] > alfa) {
					i--; continue;
				}
				H->lcp_son_dad[0] = 1;
				heap_lcp(H, 0);
			}
		}
		//print_delete(H, i, *size, n);
		heap_delete_min(H);
	}
#if _OUTPUT_BUFFER
	fwrite(H->out_buffer, sizeof(t_GSA), H->size_out_buffer, H->f_out_ESA);        //fflush out_buffer
#endif
	i = 0;
	for(; i < n; i++) {
		esa_close(&Text[i]); //free(Text[i].ESA);
		free(Text[i].c_buffer);
		free(Text[i].c_overflow);
	}
	heap_free(H, c_dir);
	return 0;
}

