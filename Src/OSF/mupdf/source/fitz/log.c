//
//
#include "mupdf/fitz.h"
#pragma hdrstop

fz_output * fz_new_log_for_module(fz_context * ctx, const char * module)
{
	char text[256];
	const char * s = NULL;
	if(module) {
		fz_snprintf(text, sizeof(text), "FZ_LOG_FILE_%s", module);
		s = getenv(text);
	}
	if(!s)
		s = getenv("FZ_LOG_FILE");
	if(!s)
		s = "fitz_log.txt";
	return fz_new_output_with_path(ctx, s, 1);
}

void fz_log(fz_context * ctx, const char * fmt, ...)
{
	fz_output * out;
	va_list args;
	va_start(args, fmt);
	fz_try(ctx)
	{
		out = fz_new_log_for_module(ctx, NULL);
		fz_write_vprintf(ctx, out, fmt, args);
		fz_close_output(ctx, out);
	}
	fz_always(ctx)
	{
		va_end(args);
		fz_drop_output(ctx, out);
	}
	fz_catch(ctx)
	fz_rethrow(ctx);
}

void fz_log_module(fz_context * ctx, const char * module, const char * fmt, ...)
{
	fz_output * out;
	va_list args;
	va_start(args, fmt);
	fz_try(ctx)
	{
		out = fz_new_log_for_module(ctx, module);
		if(module)
			fz_write_printf(ctx, out, "%s\t", module);
		fz_write_vprintf(ctx, out, fmt, args);
		fz_close_output(ctx, out);
	}
	fz_always(ctx)
	{
		va_end(args);
		fz_drop_output(ctx, out);
	}
	fz_catch(ctx)
	fz_rethrow(ctx);
}
