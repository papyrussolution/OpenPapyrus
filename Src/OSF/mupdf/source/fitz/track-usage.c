//
//
#include "mupdf/fitz.h"
#pragma hdrstop

#ifdef TRACK_USAGE

static track_usage_data_t * usage_head = NULL;

static void dump_usage(void)
{
	track_usage_data_t * u = usage_head;
	while(u) {
		slfprintf_stderr("USAGE: %s (%s:%d) %d calls\n",
		    u->desc, u->function, u->line, u->count);
		u = u->next;
	}
}

void track_usage(track_usage_data_t * data, const char * function, int line, const char * desc)
{
	int c = data->count++;
	if(!c) {
		data->function = function;
		data->line = line;
		data->desc = desc;
		if(usage_head == NULL)
			atexit(dump_usage);
		data->next = usage_head;
		usage_head = data;
	}
}

#endif
