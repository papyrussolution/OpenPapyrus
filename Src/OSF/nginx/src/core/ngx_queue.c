/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop
/*
 * find the middle queue element if the queue has odd number of elements
 * or the first element of the queue's second part otherwise
 */
ngx_queue_t * ngx_queue_middle(ngx_queue_t * queue)
{
	ngx_queue_t * middle = ngx_queue_head(queue);
	if(middle == ngx_queue_last(queue)) {
		return middle;
	}
	else {
		ngx_queue_t * next = ngx_queue_head(queue);
		for(;; ) {
			middle = ngx_queue_next(middle);
			next = ngx_queue_next(next);
			if(next == ngx_queue_last(queue)) {
				return middle;
			}
			else {
				next = ngx_queue_next(next);
				if(next == ngx_queue_last(queue)) {
					return middle;
				}
			}
		}
	}
}

/* the stable insertion sort */

void ngx_queue_sort(ngx_queue_t * queue, ngx_int_t (* cmp)(const ngx_queue_t *, const ngx_queue_t *))
{
	ngx_queue_t * prev, * next;
	ngx_queue_t * q = ngx_queue_head(queue);
	if(q != ngx_queue_last(queue)) {
		for(q = ngx_queue_next(q); q != ngx_queue_sentinel(queue); q = next) {
			prev = ngx_queue_prev(q);
			next = ngx_queue_next(q);
			ngx_queue_remove(q);
			do {
				if(cmp(prev, q) <= 0) {
					break;
				}
				prev = ngx_queue_prev(prev);
			} while(prev != ngx_queue_sentinel(queue));
			ngx_queue_insert_after(prev, q);
		}
	}
}
