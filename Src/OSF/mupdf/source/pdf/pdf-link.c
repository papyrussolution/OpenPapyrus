//
//
#include "mupdf/fitz.h"
#pragma hdrstop
//#include "mupdf/pdf.h"

static pdf_obj * resolve_dest_rec(fz_context * ctx, pdf_document * doc, pdf_obj * dest, int depth)
{
	if(depth > 10)  /* Arbitrary to avoid infinite recursion */
		return NULL;
	if(pdf_is_name(ctx, dest) || pdf_is_string(ctx, dest)) {
		dest = pdf_lookup_dest(ctx, doc, dest);
		dest = resolve_dest_rec(ctx, doc, dest, depth+1);
		return dest;
	}
	else if(pdf_is_array(ctx, dest)) {
		return dest;
	}
	else if(pdf_is_dict(ctx, dest)) {
		dest = pdf_dict_get(ctx, dest, PDF_NAME(D));
		return resolve_dest_rec(ctx, doc, dest, depth+1);
	}
	else if(pdf_is_indirect(ctx, dest))
		return dest;
	return NULL;
}

static pdf_obj * resolve_dest(fz_context * ctx, pdf_document * doc, pdf_obj * dest)
{
	return resolve_dest_rec(ctx, doc, dest, 0);
}

char * pdf_parse_link_dest(fz_context * ctx, pdf_document * doc, pdf_obj * dest)
{
	pdf_obj * obj, * pageobj;
	const char * ld;
	int page;
	dest = resolve_dest(ctx, doc, dest);
	if(dest == NULL) {
		fz_warn(ctx, "undefined link destination");
		return NULL;
	}
	if(pdf_is_name(ctx, dest)) {
		ld = pdf_to_name(ctx, dest);
		return fz_strdup(ctx, ld);
	}
	else if(pdf_is_string(ctx, dest)) {
		ld = pdf_to_str_buf(ctx, dest);
		return fz_strdup(ctx, ld);
	}
	pageobj = pdf_array_get(ctx, dest, 0);
	if(pdf_is_int(ctx, pageobj)) {
		page = pdf_to_int(ctx, pageobj);
		pageobj = pdf_lookup_page_obj(ctx, doc, page);
	}
	else {
		fz_try(ctx)
		page = pdf_lookup_page_number(ctx, doc, pageobj);
		fz_catch(ctx)
		page = -1;
	}
	if(page < 0)
		return NULL;
	obj = pdf_array_get(ctx, dest, 1);
	if(obj) {
		pdf_obj * xo = NULL;
		pdf_obj * yo = NULL;
		if(pdf_name_eq(ctx, obj, PDF_NAME(XYZ))) {
			xo = pdf_array_get(ctx, dest, 2);
			yo = pdf_array_get(ctx, dest, 3);
		}
		else if(pdf_name_eq(ctx, obj, PDF_NAME(FitR))) {
			xo = pdf_array_get(ctx, dest, 2);
			yo = pdf_array_get(ctx, dest, 5);
		}
		else if(pdf_name_eq(ctx, obj, PDF_NAME(FitH)) || pdf_name_eq(ctx, obj, PDF_NAME(FitBH))) {
			yo = pdf_array_get(ctx, dest, 2);
		}
		else if(pdf_name_eq(ctx, obj, PDF_NAME(FitV)) || pdf_name_eq(ctx, obj, PDF_NAME(FitBV))) {
			xo = pdf_array_get(ctx, dest, 2);
		}
		if(xo || yo) {
			int x, y, h;
			fz_rect mediabox;
			fz_matrix pagectm;
			/* Link coords use a coordinate space that does not seem to respect Rotate or UserUnit. */
			/* All we need to do is figure out the page height to flip the coordinate space. */
			pdf_page_obj_transform(ctx, pageobj, &mediabox, &pagectm);
			mediabox = fz_transform_rect(mediabox, pagectm);
			h = mediabox.y1 - mediabox.y0;
			x = xo ? pdf_to_int(ctx, xo) : 0;
			y = yo ? h - pdf_to_int(ctx, yo) : 0;
			return fz_asprintf(ctx, "#%d,%d,%d", page + 1, x, y);
		}
	}
	return fz_asprintf(ctx, "#%d", page + 1);
}

static char * pdf_parse_file_spec(fz_context * ctx, pdf_document * doc, pdf_obj * file_spec, pdf_obj * dest)
{
	pdf_obj * filename = NULL;
	const char * path;
	char * uri;
	char frag[256];
	if(pdf_is_string(ctx, file_spec))
		filename = file_spec;
	if(pdf_is_dict(ctx, file_spec)) {
#ifdef _WIN32
		filename = pdf_dict_get(ctx, file_spec, PDF_NAME(DOS));
#else
		filename = pdf_dict_get(ctx, file_spec, PDF_NAME(Unix));
#endif
		if(!filename)
			filename = pdf_dict_geta(ctx, file_spec, PDF_NAME(UF), PDF_NAME(F));
	}
	if(!pdf_is_string(ctx, filename)) {
		fz_warn(ctx, "cannot parse file specification");
		return NULL;
	}
	if(pdf_is_array(ctx, dest))
		fz_snprintf(frag, sizeof frag, "#page=%d", pdf_array_get_int(ctx, dest, 0) + 1);
	else if(pdf_is_name(ctx, dest))
		fz_snprintf(frag, sizeof frag, "#%s", pdf_to_name(ctx, dest));
	else if(pdf_is_string(ctx, dest))
		fz_snprintf(frag, sizeof frag, "#%s", pdf_to_str_buf(ctx, dest));
	else
		frag[0] = 0;
	path = pdf_to_text_string(ctx, filename);
	uri = NULL;
#ifdef _WIN32
	if(!pdf_name_eq(ctx, pdf_dict_get(ctx, file_spec, PDF_NAME(FS)), PDF_NAME(URL))) {
		/* Fix up the drive letter (change "/C/Documents/Foo" to "C:/Documents/Foo") */
		if(path[0] == '/' && isasciialpha(path[1]) && path[2] == '/')
			uri = fz_asprintf(ctx, "file://%c:%s%s", path[1], path+2, frag);
	}
#endif
	if(!uri)
		uri = fz_asprintf(ctx, "file://%s%s", path, frag);
	return uri;
}

const char * pdf_embedded_file_name(fz_context * ctx, pdf_obj * fs)
{
	pdf_obj * filename = pdf_dict_get(ctx, fs, PDF_NAME(UF));
	SETIFZ(filename, pdf_dict_get(ctx, fs, PDF_NAME(F)));
	SETIFZ(filename, pdf_dict_get(ctx, fs, PDF_NAME(Unix)));
	SETIFZ(filename, pdf_dict_get(ctx, fs, PDF_NAME(DOS)));
	SETIFZ(filename, pdf_dict_get(ctx, fs, PDF_NAME(Mac)));
	return pdf_to_text_string(ctx, filename);
}

pdf_obj * pdf_embedded_file_stream(fz_context * ctx, pdf_obj * fs)
{
	pdf_obj * ef = pdf_dict_get(ctx, fs, PDF_NAME(EF));
	pdf_obj * file = pdf_dict_get(ctx, ef, PDF_NAME(UF));
	if(!file) file = pdf_dict_get(ctx, ef, PDF_NAME(F));
	if(!file) file = pdf_dict_get(ctx, ef, PDF_NAME(Unix));
	if(!file) file = pdf_dict_get(ctx, ef, PDF_NAME(DOS));
	if(!file) file = pdf_dict_get(ctx, ef, PDF_NAME(Mac));
	return file;
}

const char * pdf_embedded_file_type(fz_context * ctx, pdf_obj * fs)
{
	pdf_obj * file = pdf_embedded_file_stream(ctx, fs);
	pdf_obj * subtype = pdf_dict_get(ctx, file, PDF_NAME(Subtype));
	return subtype ? pdf_to_name(ctx, subtype) : "application/octet-stream";
}

int pdf_is_embedded_file(fz_context * ctx, pdf_obj * fs) { return pdf_is_stream(ctx, pdf_embedded_file_stream(ctx, fs)); }
fz_buffer * pdf_load_embedded_file(fz_context * ctx, pdf_obj * fs) { return pdf_load_stream(ctx, pdf_embedded_file_stream(ctx, fs)); }

const char * pdf_guess_mime_type_from_file_name(fz_context * ctx, const char * filename) // @todo use SFileFormat
{
	const char * ext = sstrrchr(filename, '.');
	if(ext) {
		if(sstreqi_ascii(ext, ".pdf")) return "application/pdf";
		if(sstreqi_ascii(ext, ".xml")) return "application/xml";
		if(sstreqi_ascii(ext, ".zip")) return "application/zip";
		if(sstreqi_ascii(ext, ".tar")) return "application/x-tar";

		/* Text */
		if(sstreqi_ascii(ext, ".txt")) return "text/plain";
		if(sstreqi_ascii(ext, ".rtf")) return "application/rtf";
		if(sstreqi_ascii(ext, ".csv")) return "text/csv";
		if(sstreqi_ascii(ext, ".html")) return "text/html";
		if(sstreqi_ascii(ext, ".htm")) return "text/html";
		if(sstreqi_ascii(ext, ".css")) return "text/css";

		/* Office */
		if(sstreqi_ascii(ext, ".doc")) return "application/msword";
		if(sstreqi_ascii(ext, ".ppt")) return "application/vnd.ms-powerpoint";
		if(sstreqi_ascii(ext, ".xls")) return "application/vnd.ms-excel";
		if(sstreqi_ascii(ext, ".docx")) return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
		if(sstreqi_ascii(ext, ".pptx")) return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
		if(sstreqi_ascii(ext, ".xlsx")) return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
		if(sstreqi_ascii(ext, ".odt")) return "application/vnd.oasis.opendocument.text";
		if(sstreqi_ascii(ext, ".odp")) return "application/vnd.oasis.opendocument.presentation";
		if(sstreqi_ascii(ext, ".ods")) return "application/vnd.oasis.opendocument.spreadsheet";

		/* Image */
		if(sstreqi_ascii(ext, ".bmp")) return "image/bmp";
		if(sstreqi_ascii(ext, ".gif")) return "image/gif";
		if(sstreqi_ascii(ext, ".jpeg")) return "image/jpeg";
		if(sstreqi_ascii(ext, ".jpg")) return "image/jpeg";
		if(sstreqi_ascii(ext, ".png")) return "image/png";
		if(sstreqi_ascii(ext, ".svg")) return "image/svg+xml";
		if(sstreqi_ascii(ext, ".tif")) return "image/tiff";
		if(sstreqi_ascii(ext, ".tiff")) return "image/tiff";

		/* Sound */
		if(sstreqi_ascii(ext, ".flac")) return "audio/flac";
		if(sstreqi_ascii(ext, ".mp3")) return "audio/mpeg";
		if(sstreqi_ascii(ext, ".ogg")) return "audio/ogg";
		if(sstreqi_ascii(ext, ".wav")) return "audio/wav";

		/* Movie */
		if(sstreqi_ascii(ext, ".avi")) return "video/x-msvideo";
		if(sstreqi_ascii(ext, ".mov")) return "video/quicktime";
		if(sstreqi_ascii(ext, ".mp4")) return "video/mp4";
		if(sstreqi_ascii(ext, ".webm")) return "video/webm";
	}
	return "application/octet-stream";
}

pdf_obj * pdf_add_embedded_file(fz_context * ctx, pdf_document * doc,
    const char * filename, const char * mimetype, fz_buffer * contents)
{
	const char * s;
	char asciiname[1024];
	pdf_obj * file = NULL;
	pdf_obj * filespec = NULL;
	pdf_obj * ef;
	size_t i;

	fz_var(file);
	fz_var(filespec);

	for(i = 0, s = filename; *s && i + 1 < sizeof asciiname; ++i) {
		int c;
		s += fz_chartorune(&c, s);
		asciiname[i] = (c >= 32 && c <= 126) ? c : '_';
	}
	asciiname[i] = 0;

	if(!mimetype)
		mimetype = pdf_guess_mime_type_from_file_name(ctx, filename);

	fz_try(ctx)
	{
		file = pdf_add_new_dict(ctx, doc, 3);
		pdf_dict_put(ctx, file, PDF_NAME(Type), PDF_NAME(EmbeddedFile));
		pdf_dict_put_name(ctx, file, PDF_NAME(Subtype), mimetype);
		pdf_update_stream(ctx, doc, file, contents, 0);

		filespec = pdf_add_new_dict(ctx, doc, 4);
		pdf_dict_put(ctx, filespec, PDF_NAME(Type), PDF_NAME(Filespec));
		pdf_dict_put_text_string(ctx, filespec, PDF_NAME(F), asciiname);
		pdf_dict_put_text_string(ctx, filespec, PDF_NAME(UF), filename);
		ef = pdf_dict_put_dict(ctx, filespec, PDF_NAME(EF), 1);
		pdf_dict_put(ctx, ef, PDF_NAME(F), file);
	}
	fz_always(ctx)
	{
		pdf_drop_obj(ctx, file);
	}
	fz_catch(ctx)
	{
		pdf_drop_obj(ctx, filespec);
		fz_rethrow(ctx);
	}

	return filespec;
}

char * pdf_parse_link_action(fz_context * ctx, pdf_document * doc, pdf_obj * action, int pagenum)
{
	pdf_obj * obj, * dest, * file_spec;

	if(!action)
		return NULL;

	obj = pdf_dict_get(ctx, action, PDF_NAME(S));
	if(pdf_name_eq(ctx, PDF_NAME(GoTo), obj)) {
		dest = pdf_dict_get(ctx, action, PDF_NAME(D));
		return pdf_parse_link_dest(ctx, doc, dest);
	}
	else if(pdf_name_eq(ctx, PDF_NAME(URI), obj)) {
		/* URI entries are ASCII strings */
		const char * uri = pdf_dict_get_text_string(ctx, action, PDF_NAME(URI));
		if(!fz_is_external_link(ctx, uri)) {
			pdf_obj * uri_base_obj = pdf_dict_getp(ctx, pdf_trailer(ctx, doc), "Root/URI/Base");
			const char * uri_base = uri_base_obj ? pdf_to_text_string(ctx, uri_base_obj) : "file://";
			char * new_uri = (char *)Memento_label(fz_malloc(ctx, strlen(uri_base) + strlen(uri) + 1), "link_action");
			strcpy(new_uri, uri_base);
			strcat(new_uri, uri);
			return new_uri;
		}
		return fz_strdup(ctx, uri);
	}
	else if(pdf_name_eq(ctx, PDF_NAME(Launch), obj)) {
		file_spec = pdf_dict_get(ctx, action, PDF_NAME(F));
		return pdf_parse_file_spec(ctx, doc, file_spec, NULL);
	}
	else if(pdf_name_eq(ctx, PDF_NAME(GoToR), obj)) {
		dest = pdf_dict_get(ctx, action, PDF_NAME(D));
		file_spec = pdf_dict_get(ctx, action, PDF_NAME(F));
		return pdf_parse_file_spec(ctx, doc, file_spec, dest);
	}
	else if(pdf_name_eq(ctx, PDF_NAME(Named), obj)) {
		dest = pdf_dict_get(ctx, action, PDF_NAME(N));

		if(pdf_name_eq(ctx, PDF_NAME(FirstPage), dest))
			pagenum = 0;
		else if(pdf_name_eq(ctx, PDF_NAME(LastPage), dest))
			pagenum = pdf_count_pages(ctx, doc) - 1;
		else if(pdf_name_eq(ctx, PDF_NAME(PrevPage), dest) && pagenum >= 0) {
			if(pagenum > 0)
				pagenum--;
		}
		else if(pdf_name_eq(ctx, PDF_NAME(NextPage), dest) && pagenum >= 0) {
			if(pagenum < pdf_count_pages(ctx, doc) - 1)
				pagenum++;
		}
		else
			return NULL;

		return fz_asprintf(ctx, "#%d", pagenum + 1);
	}

	return NULL;
}

static fz_link * pdf_load_link(fz_context * ctx, pdf_document * doc, pdf_obj * dict, int pagenum, fz_matrix page_ctm)
{
	pdf_obj * action;
	pdf_obj * obj;
	fz_rect bbox;
	char * uri;
	fz_link * link = NULL;

	obj = pdf_dict_get(ctx, dict, PDF_NAME(Subtype));
	if(!pdf_name_eq(ctx, obj, PDF_NAME(Link)))
		return NULL;

	obj = pdf_dict_get(ctx, dict, PDF_NAME(Rect));
	if(!obj)
		return NULL;

	bbox = pdf_to_rect(ctx, obj);
	bbox = fz_transform_rect(bbox, page_ctm);

	obj = pdf_dict_get(ctx, dict, PDF_NAME(Dest));
	if(obj)
		uri = pdf_parse_link_dest(ctx, doc, obj);
	else {
		action = pdf_dict_get(ctx, dict, PDF_NAME(A));
		/* fall back to additional action button's down/up action */
		if(!action)
			action = pdf_dict_geta(ctx, pdf_dict_get(ctx, dict, PDF_NAME(AA)), PDF_NAME(U), PDF_NAME(D));
		uri = pdf_parse_link_action(ctx, doc, action, pagenum);
	}

	if(!uri)
		return NULL;

	fz_try(ctx)
	link = fz_new_link(ctx, bbox, uri);
	fz_always(ctx)
	fz_free(ctx, uri);
	fz_catch(ctx)
	fz_rethrow(ctx);

	return link;
}

fz_link * pdf_load_link_annots(fz_context * ctx, pdf_document * doc, pdf_obj * annots, int pagenum, fz_matrix page_ctm)
{
	fz_link * link = 0;
	fz_link * head = 0;
	fz_link * tail = 0;
	pdf_obj * obj;
	const int n = pdf_array_len(ctx, annots);
	for(int i = 0; i < n; i++) {
		/* FIXME: Move the try/catch out of the loop for performance? */
		fz_try(ctx)
		{
			obj = pdf_array_get(ctx, annots, i);
			link = pdf_load_link(ctx, doc, obj, pagenum, page_ctm);
		}
		fz_catch(ctx)
		{
			fz_rethrow_if(ctx, FZ_ERROR_TRYLATER);
			link = NULL;
		}
		if(link) {
			if(!head)
				head = tail = link;
			else {
				tail->next = link;
				tail = link;
			}
		}
	}
	return head;
}

int pdf_resolve_link(fz_context * ctx, pdf_document * doc, const char * uri, float * xp, float * yp)
{
	if(uri && uri[0] == '#') {
		int page = satoi(uri + 1) - 1;
		if(xp || yp) {
			const char * x = sstrchr(uri, ',');
			const char * y = sstrrchr(uri, ',');
			if(x && y) {
				ASSIGN_PTR(xp, satoi(x + 1));
				ASSIGN_PTR(yp, satoi(y + 1));
			}
		}
		return page;
	}
	fz_warn(ctx, "unknown link uri '%s'", uri);
	return -1;
}
