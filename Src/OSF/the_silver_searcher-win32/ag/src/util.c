// UTIL.C
//
#include "ag.h"
#pragma hdrstop

#ifdef _WIN32
	//#include <windows.h>
	#define flockfile(x)
	#define funlockfile(x)
	#define getc_unlocked(x) getc(x)
#endif

#define CHECK_AND_RETURN(ptr) if(!ptr) { die("Memory allocation failed."); } return ptr;

FILE * out_fd = NULL; // @global
ag_stats stats; // @global

void * FASTCALL ag_malloc(size_t size) 
{
	void * ptr = SAlloc::M(size);
	CHECK_AND_RETURN(ptr)
}

void * FASTCALL ag_realloc(void * ptr, size_t size) 
{
	void * new_ptr = SAlloc::R(ptr, size);
	CHECK_AND_RETURN(new_ptr)
}

void * FASTCALL ag_calloc(size_t count, size_t size) 
{
	void * ptr = SAlloc::C(count, size);
	CHECK_AND_RETURN(ptr)
}

char * FASTCALL ag_strdup(const char * s) 
{
	char * str = _strdup(s);
	CHECK_AND_RETURN(str)
}

char * FASTCALL ag_strndup(const char * s, size_t size) 
{
#ifdef HAVE_STRNDUP
	char * str = strndup(s, size);
	CHECK_AND_RETURN(str)
#else
	char * str = (char *)ag_malloc(size + 1);
	strlcpy(str, s, size + 1);
	return str;
#endif
}

void FASTCALL free_strings(char ** strs, const size_t strs_len) 
{
	if(strs) {
		for(size_t i = 0; i < strs_len; i++) {
			SAlloc::F(strs[i]);
		}
		SAlloc::F(strs);
	}
}

void generate_alpha_skip(const char * find, size_t f_len, size_t skip_lookup[], const int case_sensitive) 
{
	size_t i;
	for(i = 0; i < 256; i++) {
		skip_lookup[i] = f_len;
	}
	f_len--;
	for(i = 0; i < f_len; i++) {
		if(case_sensitive) {
			skip_lookup[(uchar)find[i]] = f_len - i;
		}
		else {
			skip_lookup[(uchar)tolower(find[i])] = f_len - i;
			skip_lookup[(uchar)toupper(find[i])] = f_len - i;
		}
	}
}

int is_prefix(const char * s, const size_t s_len, const size_t pos, const int case_sensitive) 
{
	for(size_t i = 0; pos + i < s_len; i++) {
		if(case_sensitive) {
			if(s[i] != s[i + pos]) {
				return 0;
			}
		}
		else {
			if(tolower(s[i]) != tolower(s[i + pos])) {
				return 0;
			}
		}
	}
	return 1;
}

size_t suffix_len(const char * s, const size_t s_len, const size_t pos, const int case_sensitive) 
{
	size_t i = 0;
	for(; i < pos; i++) {
		if(case_sensitive) {
			if(s[pos - i] != s[s_len - i - 1]) {
				break;
			}
		}
		else {
			if(tolower(s[pos - i]) != tolower(s[s_len - i - 1])) {
				break;
			}
		}
	}
	return i;
}

void generate_find_skip(const char * find, const size_t f_len, size_t ** skip_lookup, const int case_sensitive) 
{
	size_t i;
	size_t s_len;
	size_t * sl = (size_t *)ag_malloc(f_len * sizeof(size_t));
	*skip_lookup = sl;
	size_t last_prefix = f_len;
	for(i = last_prefix; i > 0; i--) {
		if(is_prefix(find, f_len, i, case_sensitive)) {
			last_prefix = i;
		}
		sl[i - 1] = last_prefix + (f_len - i);
	}
	for(i = 0; i < f_len; i++) {
		s_len = suffix_len(find, f_len, i, case_sensitive);
		if(find[i - s_len] != find[f_len - 1 - s_len]) {
			sl[f_len - 1 - s_len] = f_len - 1 - i + s_len;
		}
	}
}

//size_t ag_max(size_t a, size_t b) { return (b > a) ? b : a; }
//size_t ag_min(size_t a, size_t b) { return (b < a) ? b : a; }

void generate_hash(const char * find, const size_t f_len, uint8 * h_table, const int case_sensitive) 
{
	for(int i = f_len - sizeof(uint16_t); i >= 0; i--) {
		// Add all 2^sizeof(uint16_t) combinations of capital letters to the hash table
		for(int caps_set = 0; caps_set < (1 << sizeof(uint16_t)); caps_set++) {
			word_t word;
			memcpy(&word.as_chars, find + i, sizeof(uint16_t));
			int cap_index;
			// Capitalize the letters whose corresponding bits in caps_set are 1
			for(cap_index = 0; caps_set >> cap_index; cap_index++) {
				if((caps_set >> cap_index) & 1)
					word.as_chars[cap_index] -= 'a' - 'A';
			}
			size_t h;
			// Find next free cell
			for(h = word.as_word % H_SIZE; h_table[h]; h = (h + 1) % H_SIZE)
				;
			h_table[h] = i + 1;
			// Don't add capital letters if case sensitive
			if(case_sensitive)
				break;
		}
	}
}

// Boyer-Moore strstr 
const char * boyer_moore_strnstr(const char * s, const char * find, const size_t s_len, const size_t f_len,
    const size_t alpha_skip_lookup[], const size_t * find_skip_lookup, const int case_insensitive) 
{
	ssize_t i;
	size_t pos = f_len - 1;
	while(pos < s_len) {
		for(i = f_len - 1; i >= 0 && (case_insensitive ? tolower(s[pos]) : s[pos]) == find[i]; pos--, i--) {
			;
		}
		if(i < 0) {
			return s + pos + 1;
		}
		pos += smax(alpha_skip_lookup[(uchar)s[pos]], find_skip_lookup[i]);
	}
	return NULL;
}

// Clang's -fsanitize=alignment (included in -fsanitize=undefined) will flag
// the intentional unaligned access here, so suppress it for this function
NO_SANITIZE_ALIGNMENT const char * hash_strnstr(const char * s, const char * find, const size_t s_len, const size_t f_len,
    uint8 * h_table, const int case_sensitive) 
{
	if(s_len < f_len)
		return NULL;
	// Step through s
	const size_t step = f_len - sizeof(uint16_t) + 1;
	size_t s_i = f_len - sizeof(uint16_t);
	for(; s_i <= s_len - f_len; s_i += step) {
		size_t h;
		for(h = *(const uint16_t*)(s + s_i) % H_SIZE; h_table[h]; h = (h + 1) % H_SIZE) {
			const char * R = s + s_i - (h_table[h] - 1);
			size_t i;
			// Check putative match
			for(i = 0; i < f_len; i++) {
				if((case_sensitive ? R[i] : tolower(R[i])) != find[i])
					goto next_hash_cell;
			}
			return R; // Found
next_hash_cell:         ;
		}
	}
	// Check tail
	for(s_i = s_i - step + 1; s_i <= s_len - f_len; s_i++) {
		const char * R = s + s_i;
		for(size_t i = 0; i < f_len; i++) {
			char s_c = case_sensitive ? R[i] : tolower(R[i]);
			if(s_c != find[i])
				goto next_start;
		}
		return R;
next_start:     ;
	}
	return NULL;
}

size_t invert_matches(const char * buf, const size_t buf_len, match_t matches[], size_t matches_len) 
{
	size_t i;
	size_t match_read_index = 0;
	size_t inverted_match_count = 0;
	size_t inverted_match_start = 0;
	size_t last_line_end = 0;
	int in_inverted_match = TRUE;
	match_t next_match;
	log_debug("Inverting %u matches.", matches_len);
	if(matches_len > 0) {
		next_match = matches[0];
	}
	else {
		next_match.start = buf_len + 1;
	}
	/* No matches, so the whole buffer is now a match. */
	if(matches_len == 0) {
		matches[0].start = 0;
		matches[0].end = buf_len - 1;
		return 1;
	}
	for(i = 0; i < buf_len; i++) {
		if(i == next_match.start) {
			i = next_match.end - 1;
			match_read_index++;
			if(match_read_index < matches_len) {
				next_match = matches[match_read_index];
			}
			if(in_inverted_match && last_line_end > inverted_match_start) {
				matches[inverted_match_count].start = inverted_match_start;
				matches[inverted_match_count].end = last_line_end - 1;
				inverted_match_count++;
			}
			in_inverted_match = FALSE;
		}
		else if(i == buf_len - 1 && in_inverted_match) {
			matches[inverted_match_count].start = inverted_match_start;
			matches[inverted_match_count].end = i;
			inverted_match_count++;
		}
		else if(buf[i] == '\n') {
			last_line_end = i + 1;
			if(!in_inverted_match) {
				inverted_match_start = last_line_end;
			}
			in_inverted_match = TRUE;
		}
	}
	for(i = 0; i < matches_len; i++) {
		log_debug("Inverted match %i start %i end %i.", i, matches[i].start, matches[i].end);
	}
	return inverted_match_count;
}

void realloc_matches(match_t ** matches, size_t * matches_size, size_t matches_len) 
{
	if(matches_len >= *matches_size) {
		// TODO: benchmark initial size of matches. 100 may be too small/big
		*matches_size = *matches ? *matches_size * 2 : 100;
		*matches = (match_t *)ag_realloc(*matches, *matches_size * sizeof(match_t));
	}
}

void compile_study(pcre ** re, pcre_extra ** re_extra, char * q, const int pcre_opts, const int study_opts) 
{
	const char * pcre_err = NULL;
	int pcre_err_offset = 0;
	*re = pcre_compile(q, pcre_opts, &pcre_err, &pcre_err_offset, NULL);
	if(*re == NULL) {
		die("Bad regex! pcre_compile() failed at position %i: %s\nIf you meant to search for a literal string, run ag with -Q", pcre_err_offset, pcre_err);
	}
	*re_extra = pcre_study(*re, study_opts, &pcre_err);
	if(*re_extra == NULL) {
		log_debug("pcre_study returned nothing useful. Error: %s", pcre_err);
	}
}
//
// This function is very hot. It's called on every file. 
//
int is_binary(const void * buf, const size_t buf_len) 
{
	size_t suspicious_bytes = 0;
	size_t total_bytes = buf_len > 512 ? 512 : buf_len;
	const uchar * buf_c = (const uchar *)buf;
	size_t i;
	if(buf_len == 0) {
		return 0; // Is an empty file binary? Is it text? 
	}
	if(buf_len >= 3 && buf_c[0] == 0xEF && buf_c[1] == 0xBB && buf_c[2] == 0xBF) {
		return 0; // UTF-8 BOM. This isn't binary. 
	}
	if(buf_len >= 5 && strncmp((const char *)buf, "%PDF-", 5) == 0) {
		return 1; // PDF. This is binary. 
	}
	for(i = 0; i < total_bytes; i++) {
		if(buf_c[i] == '\0') {
			return 1; // NULL char. It's binary 
		}
		else if((buf_c[i] < 7 || buf_c[i] > 14) && (buf_c[i] < 32 || buf_c[i] > 127)) {
			// UTF-8 detection 
			if(buf_c[i] > 193 && buf_c[i] < 224 && i + 1 < total_bytes) {
				i++;
				if(buf_c[i] > 127 && buf_c[i] < 192) {
					continue;
				}
			}
			else if(buf_c[i] > 223 && buf_c[i] < 240 && i + 2 < total_bytes) {
				i++;
				if(buf_c[i] > 127 && buf_c[i] < 192 && buf_c[i + 1] > 127 && buf_c[i + 1] < 192) {
					i++;
					continue;
				}
			}
			suspicious_bytes++;
			// Disk IO is so slow that it's worthwhile to do this calculation after every suspicious byte.
			// This is true even on a 1.6Ghz Atom with an Intel 320 SSD. 
			// Read at least 32 bytes before making a decision 
			if(i >= 32 && (suspicious_bytes * 100) / total_bytes > 10) {
				return 1;
			}
		}
	}
	if((suspicious_bytes * 100) / total_bytes > 10) {
		return 1;
	}
	return 0;
}

int is_regex(const char * query) 
{
	const char regex_chars[] = { '$', '(', ')', '*', '+', '.', '?', '[', '\\', '^', '{', '|', '\0' };
	return (strpbrk(query, regex_chars) != NULL);
}

int is_fnmatch(const char * filename) 
{
	const char fnmatch_chars[] = { '!', '*', '?', '[', ']', '\0' };
	return (strpbrk(filename, fnmatch_chars) != NULL);
}

int binary_search(const char * needle, char ** haystack, int start, int end) 
{
	int mid = -1;
	if(start != end) {
		mid = start + ((end - start) / 2);
		const int rc = strcmp(needle, haystack[mid]);
		if(rc < 0) {
			return binary_search(needle, haystack, start, mid);
		}
		else if(rc > 0) {
			return binary_search(needle, haystack, mid + 1, end);
		}
	}
	return mid;
}

static int wordchar_table[256]; // @global

void init_wordchar_table() 
{
	for(int i = 0; i < 256; ++i) {
		char ch = (char)i;
		wordchar_table[i] = isasciialnum(ch) || ch == '_';
	}
}

int is_wordchar(char ch) { return wordchar_table[(uchar)ch]; }

int is_lowercase(const char * s) 
{
	for(uint i = 0; s[i] != '\0'; i++) {
		if(!isascii(s[i]) || isupper(s[i])) {
			return FALSE;
		}
	}
	return TRUE;
}

int is_directory(const char * path, const struct dirent * d) 
{
#ifdef HAVE_DIRENT_DTYPE
	/* Some filesystems, e.g. ReiserFS, always return a type DT_UNKNOWN from readdir or scandir. */
	/* Call stat if we don't find DT_DIR to get the information we need. */
	/* Also works for symbolic links to directories. */
	if(d->d_type != DT_UNKNOWN && d->d_type != DT_LNK) {
		return d->d_type == DT_DIR;
	}
#endif
	char * full_path;
	struct stat s;
	ag_asprintf(&full_path, "%s/%s", path, d->d_name);
	if(stat(full_path, &s) != 0) {
		SAlloc::F(full_path);
		return FALSE;
	}
#ifdef _WIN32
	int is_dir = GetFileAttributesA(full_path) & FILE_ATTRIBUTE_DIRECTORY;
#else
	int is_dir = S_ISDIR(s.st_mode);
#endif
	SAlloc::F(full_path);
	return is_dir;
}

int is_symlink(const char * path, const struct dirent * d) 
{
#ifdef _WIN32
	char full_path[MAX_PATH + 1] = { 0 };
	sprintf(full_path, "%s\\%s", path, d->d_name);
	return (GetFileAttributesA(full_path) & FILE_ATTRIBUTE_REPARSE_POINT);
#else
#ifdef HAVE_DIRENT_DTYPE
	/* Some filesystems, e.g. ReiserFS, always return a type DT_UNKNOWN from readdir or scandir. */
	/* Call lstat if we find DT_UNKNOWN to get the information we need. */
	if(d->d_type != DT_UNKNOWN) {
		return (d->d_type == DT_LNK);
	}
#endif
	char * full_path;
	struct stat s;
	ag_asprintf(&full_path, "%s/%s", path, d->d_name);
	if(lstat(full_path, &s) != 0) {
		SAlloc::F(full_path);
		return FALSE;
	}
	SAlloc::F(full_path);
	return S_ISLNK(s.st_mode);
#endif
}

int is_named_pipe(const char * path, const struct dirent * d) 
{
#ifdef HAVE_DIRENT_DTYPE
	if(d->d_type != DT_UNKNOWN && d->d_type != DT_LNK) {
		return d->d_type == DT_FIFO || d->d_type == DT_SOCK;
	}
#endif
	char * full_path;
	struct stat s;
	ag_asprintf(&full_path, "%s/%s", path, d->d_name);
	if(stat(full_path, &s) != 0) {
		SAlloc::F(full_path);
		return FALSE;
	}
	SAlloc::F(full_path);
	return S_ISFIFO(s.st_mode)
#ifdef S_ISSOCK
	      || S_ISSOCK(s.st_mode)
#endif
	;
}

void ag_asprintf(char ** ppRet, const char * pFmt, ...) 
{
	if(ppRet) {
		va_list args;
		va_start(args, pFmt);
		SString temp_buf;
		slvprintf(temp_buf, pFmt, args);
		*ppRet = _strdup(temp_buf);
		//if(vasprintf(ppRet, pFmt, args) == -1) { die("vasprintf returned -1"); }
		va_end(args);
	}
}

void die(const char * fmt, ...) 
{
	va_list args;
	va_start(args, fmt);
	vplog(LOG_LEVEL_ERR, fmt, args);
	va_end(args);
	exit(2);
}

#ifndef HAVE_FGETLN
	char * fgetln(FILE * fp, size_t * lenp) 
	{
		char * buf = NULL;
		int c, used = 0, len = 0;
		flockfile(fp);
		while((c = getc_unlocked(fp)) != EOF) {
			if(!buf || len >= used) {
				const size_t nsize = used + BUFSIZ;
				char * newbuf = (char *)SAlloc::R(buf, nsize);
				if(!newbuf) {
					funlockfile(fp);
					SAlloc::F(buf);
					return NULL;
				}
				buf = newbuf;
				used = nsize;
			}
			buf[len++] = c;
			if(c == '\n') {
				break;
			}
		}
		funlockfile(fp);
		*lenp = len;
		return buf;
	}
#endif
#ifndef HAVE_GETLINE
	// 
	// Do it yourself getline() implementation
	// 
	ssize_t getline(char ** lineptr, size_t * n, FILE * stream) 
	{
		size_t len = 0;
		char * srcln = NULL;
		char * newlnptr = NULL;
		/* get line, bail on error */
		if(!(srcln = fgetln(stream, &len))) {
			return -1;
		}
		if(len >= *n) {
			/* line is too big for buffer, must realloc */
			/* double the buffer, bail on error */
			if(!(newlnptr = (char *)SAlloc::R(*lineptr, len * 2))) {
				return -1;
			}
			*lineptr = newlnptr;
			*n = len * 2;
		}
		memcpy(*lineptr, srcln, len);
	#ifndef HAVE_FGETLN
		// Our own implementation of fgetln() returns a malloc()d buffer that we must free
		SAlloc::F(srcln);
	#endif
		(*lineptr)[len] = '\0';
		return len;
	}
#endif

ssize_t buf_getline(const char ** line, const char * buf, const size_t buf_len, const size_t buf_offset) 
{
	const char * cur = buf + buf_offset;
	ssize_t i = 0;
	for(; (buf_offset + i < buf_len) && cur[i] != '\n'; i++) {
	}
	*line = cur;
	return i;
}

#ifndef HAVE_REALPATH
	// 
	// realpath() for Windows. Turns slashes into backslashes and calls _fullpath
	// 
	char * realpath(const char * path, char * resolved_path) 
	{
		char * p;
		char tmp[_MAX_PATH + 1];
		strlcpy(tmp, path, sizeof(tmp));
		p = tmp;
		while(*p) {
			if(*p == '/') {
				*p = '\\';
			}
			p++;
		}
		return _fullpath(resolved_path, tmp, _MAX_PATH);
	}
#endif
#ifndef HAVE_STRLCPY
	size_t strlcpy(char * dst, const char * src, size_t size) 
	{
		char * d = dst;
		const char * s = src;
		size_t n = size;
		// Copy as many bytes as will fit 
		if(n != 0) {
			while(--n != 0) {
				if((*d++ = *s++) == '\0') {
					break;
				}
			}
		}
		// Not enough room in dst, add NUL and traverse rest of src 
		if(!n) {
			if(size != 0) {
				*d = '\0'; // NUL-terminate dst 
			}
			while(*s++) {
			}
		}
		return (s - src - 1); // count does not include NUL 
	}
#endif
#ifndef HAVE_VASPRINTF
	int vasprintf(char ** ret, const char * fmt, va_list args) 
	{
		int rv;
		*ret = NULL;
		va_list args2 = 0;
	// vsnprintf can destroy args, so we need to copy it for the second call 
	#ifdef __va_copy
		__va_copy(args2, args); // non-standard macro, but usually exists 
	#elif va_copy
		va_copy(args2, args); // C99 macro. We compile with -std=c89 but you never know 
	#else
		memcpy(args2, args, sizeof(va_list)); // Ancient compiler. This usually works but there are no guarantees
	#endif
		rv = vsnprintf(NULL, 0, fmt, args);
		va_end(args);
		if(rv < 0) {
			return rv;
		}
		*ret = (char *)SAlloc::M(++rv); /* vsnprintf doesn't count \0 */
		if(*ret == NULL) {
			return -1;
		}
		rv = vsnprintf(*ret, rv, fmt, args2);
		va_end(args2);
		if(rv < 0) {
			SAlloc::F(*ret);
		}
		return rv;
	}
#endif
//
// LOG
//
pthread_mutex_t print_mtx = PTHREAD_MUTEX_INITIALIZER; // @global
static enum log_level log_threshold = LOG_LEVEL_ERR; // @global

void set_log_level(enum log_level threshold) 
{
	log_threshold = threshold;
}

void log_debug(const char * fmt, ...) 
{
	va_list args;
	va_start(args, fmt);
	vplog(LOG_LEVEL_DEBUG, fmt, args);
	va_end(args);
}

void log_msg(const char * fmt, ...) 
{
	va_list args;
	va_start(args, fmt);
	vplog(LOG_LEVEL_MSG, fmt, args);
	va_end(args);
}

void log_warn(const char * fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vplog(LOG_LEVEL_WARN, fmt, args);
	va_end(args);
}

void log_err(const char * fmt, ...) 
{
	va_list args;
	va_start(args, fmt);
	vplog(LOG_LEVEL_ERR, fmt, args);
	va_end(args);
}

void vplog(const uint level, const char * fmt, va_list args) 
{
	if(static_cast<int>(level) >= log_threshold) {
		pthread_mutex_lock(&print_mtx);
		FILE * stream = out_fd;
		switch(level) {
			case LOG_LEVEL_DEBUG: fprintf(stream, "DEBUG: "); break;
			case LOG_LEVEL_MSG:   fprintf(stream, "MSG: "); break;
			case LOG_LEVEL_WARN:  fprintf(stream, "WARN: "); break;
			case LOG_LEVEL_ERR:
				stream = stderr;
				fprintf(stream, "ERR: ");
				break;
		}
		vfprintf(stream, fmt, args);
		fprintf(stream, "\n");
		pthread_mutex_unlock(&print_mtx);
	}
}

void plog(const uint level, const char * fmt, ...) 
{
	va_list args;
	va_start(args, fmt);
	vplog(level, fmt, args);
	va_end(args);
}
