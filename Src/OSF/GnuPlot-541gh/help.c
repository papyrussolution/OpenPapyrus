// GNUPLOT - help.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop
#ifdef NO_GIH
	#include <stdio.h>
void EndOutput()
{
}

//void StartOutput()
void GnuPlot::StartOutput()
{
}

void OutLine(const char * M)
{
	fputs(M, stderr);
}

#else
#ifdef __DJGPP__
	#include <pc.h>
#endif
#if defined(__WATCOMC__) && defined(MSDOS)
	#include <graph.h>
#endif
/*
** help -- help subsystem that understands defined keywords
**
** Looks for the desired keyword in the help file at runtime, so you
** can give extra help or supply local customizations by merely editing
** the help file.
**
** The original (single-file) idea and algorithm is by John D. Johnson,
** Hewlett-Packard Company.  Thanx and a tip of the Hatlo hat!
**
** Much extension by David Kotz for use in gnutex, and then in gnuplot.
** Added output paging support, both unix and builtin. Rewrote completely
** to read helpfile into memory, avoiding reread of help file. 12/89.
**
** The help file looks like this (the question marks are really in column 1):
**
**   ?topic
**   This line is printed when the user wants help on "topic".
**   ?keyword
**   ?Keyword
**   ?KEYWORD
**   These lines will be printed on the screen if the user wanted
**   help on "keyword", "Keyword", or "KEYWORD".  No casefolding is
**   done on the keywords.
**   ?subject
**   ?alias
**   This line is printed for help on "subject" and "alias".
**   ?
**   ??
**   Since there is a null keyword for this line, this section
**   is printed when the user wants general help (when a help
**   keyword isn't given).  A command summary is usually here.
**   Notice that the null keyword is equivalent to a "?" keyword
**   here, because of the '?' and '??' topic lines above.
**   If multiple keywords are given, the first is considered the
**   'primary' keyword. This affects a listing of available topics.
**   ?last-subject
**   Note that help sections are terminated by the start of the next
**   '?' entry or by EOF.  So you can't have a leading '?' on a line
**   of any help section.  You can re-define the magic character to
**   recognize in column 1, though, if '?' is too useful.  (Try ^A.)
*/

#define KEYFLAG '?'             /* leading char in help file topic lines */

/*
** Calling sequence:
**   int result;             # 0 == success
**   char *keyword;          # topic to give help on
**   char *pathname;         # path of help file
**   int subtopics;          # set to TRUE if only subtopics to be listed
**                           # returns TRUE if subtopics were found
**   result = help(keyword, pathname, &subtopics);
** Sample:
**   cmd = "search\n";
**   helpfile = "/usr/local/lib/program/program.help";
**   subtopics = FALSE;
**   if (help(cmd, helpfile, &subtopics) != H_FOUND)
**           printf("Sorry, no help for %s", cmd);
**
**
** Speed this up by replacing the stdio calls with open/close/read/write.
*/
#ifdef WDLEN
	#define PATHSIZE WDLEN
#else
	#define PATHSIZE BUFSIZ
#endif

typedef struct line_s LINEBUF;
struct line_s {
	char * line; /* the text of this line */
	LINEBUF * next; /* the next line */
};

typedef struct linkey_s LINKEY;
struct linkey_s {
	char * key; /* the name of this key */
	long pos; /* ftell position */
	LINEBUF * text; /* the text for this key */
	bool primary; /* TRUE -> is a primary name for a text block */
	LINKEY * next; /* the next key in linked list */
};

typedef struct key_s KEY;
struct key_s {
	char * key; /* the name of this key */
	long pos; /* ftell position */
	LINEBUF * text; /* the text for this key */
	bool primary; /* TRUE -> is a primary name for a text block */
};

static LINKEY * keylist = NULL; /* linked list of keys */
static KEY * keys = NULL; /* array of keys */
static int keycount = 0; /* number of keys */
static FILE * helpfp = NULL;
static KEY empty_key = {NULL, 0, NULL, 0};

static int LoadHelp(const char * path);
static void sortkeys();
static int keycomp(SORTFUNC_ARGS a, SORTFUNC_ARGS b);
static LINEBUF * storeline(char * text);
static LINKEY * storekey(char * key);
static KEY * FindHelp(char * keyword);
static bool Ambiguous(struct key_s * key, size_t len);

/* Help output */
//static void PrintHelp(struct key_s * key, bool * subtopics);
static void ShowSubtopics(struct key_s * key, bool * subtopics);
static void OutLine_InternalPager(const char * line);

#if defined(PIPES)
static FILE * outfile; /* for unix pager, if any */
#endif
static int pagelines; /* count for builtin pager */
static int screensize; /* lines on screen (got with env var) */
#define SCREENSIZE 24           /* lines on screen (most have at least 24) */

/* help:
 * print a help message
 * also print available subtopics, if subtopics is TRUE
 */
//int help(char * keyword/* on this topic */, char * path/* from this file */, bool * subtopics/* (in) - subtopics only? */)
/* (out) - are there subtopics? */
int GnuPlot::Help(char * pKeyword/* on this topic */, const char * pPath/* from this file */, bool * pSubTopics/* (in) - subtopics only? */)
{
	static char * oldpath = NULL; /* previous help file */
	int status; /* result of LoadHelp */
	KEY * key; /* key that matches keyword */
	/*
	** Load the help file if necessary (say, first time we enter this routine,
	** or if the help file changes from the last time we were called).
	** Also may occur if in-memory copy was freed.
	** Calling routine may access errno to determine cause of H_ERROR.
	*/
	errno = 0;
	if(oldpath && strcmp(oldpath, pPath) != 0)
		FreeHelp();
	if(keys == NULL) {
		status = LoadHelp(pPath);
		if(status == H_ERROR)
			return (status);
		// save the new path in oldpath 
		SAlloc::F(oldpath);
		oldpath = sstrdup(pPath);
	}
	// look for the keyword in the help file 
	key = FindHelp(pKeyword);
	if(key) {
		// found the keyword: if help exists, print and return 
		if(key->text)
			PrintHelp(key, pSubTopics);
		status = H_FOUND;
	}
	else {
		status = H_NOTFOUND;
	}
	return (status);
}
//
// we only read the file into memory once
//
static int LoadHelp(const char * path)
{
	LINKEY * key = 0; /* this key */
	long pos = 0; /* ftell location within help file */
	char buf[MAX_LINE_LEN]; /* line from help file */
	LINEBUF * head; /* head of text list  */
	LINEBUF * firsthead = NULL;
	bool primary; /* first ? line of a set is primary */
	bool flag;
	if((helpfp = fopen(path, "r")) == NULL) {
		/* can't open help file, so error exit */
		return (H_ERROR);
	}
	/*
	** The help file is open.  Look in there for the keyword.
	*/
	if(!fgets(buf, MAX_LINE_LEN - 1, helpfp) || *buf != KEYFLAG)
		return (H_ERROR); /* it is probably not the .gih file */
	while(!feof(helpfp)) {
		/*
		** Make an entry for each synonym keyword
		*/
		primary = TRUE;
		while(buf[0] == KEYFLAG) {
			key = storekey(buf + 1); /* store this key */
			key->primary = primary;
			key->text = NULL; /* fill in with real value later */
			key->pos = 0; /* fill in with real value later */
			primary = FALSE;
			pos = static_cast<long>(ftell(helpfp));
			if(fgets(buf, MAX_LINE_LEN - 1, helpfp) == (char *)NULL)
				break;
		}
		/*
		** Now store the text for this entry.
		** buf already contains the first line of text.
		*/
		firsthead = storeline(buf);
		head = firsthead;
		while((fgets(buf, MAX_LINE_LEN - 1, helpfp) != (char *)NULL) && (buf[0] != KEYFLAG)) {
			/* save text line */
			head->next = storeline(buf);
			head = head->next;
		}
		/* make each synonym key point to the same text */
		do {
			key->pos = pos;
			key->text = firsthead;
			flag = key->primary;
			key = key->next;
		} while(flag != TRUE && key);
	}
	fclose(helpfp);
	// we sort the keys so we can use binary search later 
	sortkeys();
	return (H_FOUND); /* ok */
}
//
// make a new line buffer and save this string there 
//
static LINEBUF * storeline(char * text)
{
	LINEBUF * p_new = (LINEBUF*)SAlloc::M(sizeof(LINEBUF));
	p_new->line = sstrdup(text);
	p_new->next = NULL;
	return (p_new);
}
//
// Add this keyword to the keys list, with the given text 
//
static LINKEY * storekey(char * key)
{
	key[strlen(key)-1] = '\0'; // cut off \n 
	LINKEY * p_new = (LINKEY*)SAlloc::M(sizeof(LINKEY));
	if(key)
		p_new->key = sstrdup(key);
	// add to front of list 
	p_new->next = keylist;
	keylist = p_new;
	keycount++;
	return (p_new);
}

/* we sort the keys so we can use binary search later */
/* We have a linked list of keys and the number.
 * to sort them we need an array, so we reform them into an array,
 * and then throw away the list.
 */
static void sortkeys()
{
	LINKEY * p, * n; /* pointers to linked list */
	int i; /* index into key array */
	/* allocate the array */
	keys = (KEY*)SAlloc::M((keycount + 1) * sizeof(KEY));
	/* copy info from list to array, freeing list */
	for(p = keylist, i = 0; p; p = n, i++) {
		keys[i].key = p->key;
		keys[i].pos = p->pos;
		keys[i].text = p->text;
		keys[i].primary = p->primary;
		n = p->next;
		SAlloc::F((char *)p);
	}
	/* a null entry to terminate subtopic searches */
	keys[keycount].key = NULL;
	keys[keycount].pos = 0;
	keys[keycount].text = NULL;
	/* sort the array */
	/* note that it only moves objects of size (two pointers + long + int) */
	/* it moves no strings */
	qsort(keys, keycount, sizeof(KEY), keycomp);
}

/* HBB 20010720: changed to make this match the prototype qsort()
 * really expects. Casting function pointers, as we did before, is
 * illegal! */
static int keycomp(SORTFUNC_ARGS arg1, SORTFUNC_ARGS arg2)
{
	const KEY * a = (const KEY *)arg1;
	const KEY * b = (const KEY *)arg2;
	return (strcmp(a->key, b->key));
}
//
// Free the help file from memory. 
// May be called externally if space is needed 
//
void FreeHelp()
{
	if(keys) {
		for(int i = 0; i < keycount; i++) {
			SAlloc::F((char *)keys[i].key);
			if(keys[i].primary) { // only try to release text once! 
				LINEBUF * next;
				for(LINEBUF * t = keys[i].text; t; t = next) {
					SAlloc::F((char *)t->line);
					next = t->next;
					SAlloc::F((char *)t);
				}
			}
		}
		ZFREE(keys);
		keycount = 0;
	}
}
/* FindHelp:
 *  Find the key that matches the keyword.
 *  The keys[] array is sorted by key.
 *  We could use a binary search, but a linear search will aid our
 *  attempt to allow abbreviations. We search for the first thing that
 *  matches all the text we're given. If not an exact match, then
 *  it is an abbreviated match, and there must be no other abbreviated
 *  matches -- for if there are, the abbreviation is ambiguous.
 *  We print the ambiguous matches in that case, and return not found.
 */
static KEY * /*NULL if not found*/ FindHelp(char * keyword/* string we look for */)
{
	size_t len = strcspn(keyword, " ");
	for(KEY * key = keys; key->key; key++) {
		if(!strncmp(keyword, key->key, len)) { /* we have a match! */
			if(!Ambiguous(key, len)) {
				size_t key_len = strlen(key->key);
				size_t keyword_len = strlen(keyword);
				if(key_len != len) {
					// Expand front portion of keyword 
					int shift = key_len - len;
					for(int i = keyword_len+shift; i >= static_cast<ssize_t>(len) && i >= shift; i--)
						keyword[i] = keyword[i-shift];
					strncpy(keyword, key->key, key_len); /* give back the full spelling */
					len = key_len;
					keyword_len += shift;
				}
				// Check for another subword 
				if(keyword[len] == ' ') {
					len = len + 1 + strcspn(keyword + len + 1, " ");
					key--; // start with current key 
				}
				else
					return key; // found!!  non-ambiguous abbreviation 
			}
			else
				return &empty_key;
		}
	}
	return NULL; // not found 
}

/* Ambiguous:
 * Check the key for ambiguity up to the given length.
 * It is ambiguous if it is not a complete string and there are other
 * keys following it with the same leading substring.
 */
static bool Ambiguous(KEY * key, size_t len)
{
	char * first;
	char * prev;
	bool status = FALSE; /* assume not ambiguous */
	int compare;
	size_t sublen;
	if(!key->key[len])
		return FALSE;
	for(prev = first = key->key, compare = 0, key++; key->key && compare == 0; key++) {
		compare = strncmp(first, key->key, len);
		if(compare == 0) {
			/* So this key matches the first one, up to len.
			 * But is it different enough from the previous one
			 * to bother printing it as a separate choice?
			 */
			sublen = strcspn(key->key + len, " ");
			if(strncmp(key->key, prev, len + sublen) != 0) {
				/* yup, this is different up to the next space */
				if(!status) {
					/* first one we have printed is special */
					fprintf(stderr,    "Ambiguous request '%.*s'; possible matches:\n",
					    (int)len, first);
					fprintf(stderr, "\t%s\n", prev);
					status = TRUE;
				}
				fprintf(stderr, "\t%s\n", key->key);
				prev = key->key;
			}
		}
	}
	return (status);
}
// 
// PrintHelp:
// print the text for key
// 
//static void PrintHelp(KEY * key, bool * subtopics/* (in) - subtopics only? (out) - are there subtopics? */)
void GnuPlot::PrintHelp(void * pKey_, bool * pSubTopics/* (in) - subtopics only? (out) - are there subtopics? */)
{
	KEY * p_key = static_cast<KEY *>(pKey_);
	StartOutput();
	if(!pSubTopics || !*pSubTopics) {
		for(LINEBUF * t = p_key->text; t; t = t->next)
			OutLine(t->line); /* print text line */
	}
	ShowSubtopics(p_key, pSubTopics);
	OutLine_InternalPager("\n");
	EndOutput();
}
// 
// ShowSubtopics:
// Print a list of subtopic names
// 
#define PER_LINE 4 // The maximum number of subtopics per line 

static void ShowSubtopics(KEY * key/* the topic */, bool * subtopics/* (out) are there any subtopics */)
{
	int subt = 0; /* printed any subtopics yet? */
	KEY * subkey; /* subtopic key */
	size_t len; /* length of key name */
	char line[BUFSIZ]; /* subtopic output line */
	char * start; /* position of subname in key name */
	size_t sublen; /* length of subname */
	char * prev = NULL; /* the last thing we put on the list */
#define MAXSTARTS 256
	int stopics = 0; /* count of (and index to next) subtopic name */
	char * starts[MAXSTARTS]; /* saved positions of subnames */
	*line = '\0';
	len = strlen(key->key);
	for(subkey = key + 1; subkey->key; subkey++) {
		if(strncmp(subkey->key, key->key, len) == 0) {
			/* find this subtopic name */
			start = subkey->key + len;
			if(len > 0) {
				if(*start == ' ')
					start++; /* skip space */
				else
					break; /* not the same topic after all  */
			}
			else {
				/* here we are looking for main topics */
				if(!subkey->primary)
					continue; /* not a main topic */
			}
			sublen = strcspn(start, " ");
			if(prev == NULL || strncmp(start, prev, sublen) != 0) {
				if(subt == 0) {
					subt++;
					if(len) {
						strcpy(line, "\nSubtopics available for ");
						strncat(line, key->key, sizeof(line) - strlen(line)- strlen(":\n")- 1);
						strcat(line, ":\n");
					}
					else
						strcpy(line, "\nHelp topics available:\n");
					OutLine_InternalPager(line);
					*line = '\0';
				}
				starts[stopics++] = start;
				prev = start;
			}
		}
		else {
			/* new topic */
			break;
		}
	}
#define FIRSTCOL         4 // The number of the first column for subtopic entries 
#define COLLENGTH       18 // Length of a subtopic entry; if COLLENGTH is exceeded, the next column is skipped 
#ifndef COLUMN_HELP
	{
		/* sort subtopics by row - default */
		int subtopic;
		int spacelen = 0, ispacelen;
		int pos = 0;
		for(subtopic = 0; subtopic < stopics; subtopic++) {
			start = starts[subtopic];
			sublen = strcspn(start, " ");
			if(pos == 0)
				spacelen = FIRSTCOL;
			/* adapted by DvdSchaaf */
			for(ispacelen = 0; ispacelen < spacelen; ispacelen++)
				strcat(line, " ");
			strncat(line, start, sublen);
			spacelen = COLLENGTH - sublen;
			while(spacelen <= 0) {
				spacelen += COLLENGTH;
				pos++;
			}
			pos++;
			if(pos >= PER_LINE) {
				strcat(line, "\n");
				OutLine_InternalPager(line);
				*line = '\0';
				pos = 0;
			}
		}
		/* put out the last line */
		if(subt > 0 && pos > 0) {
			strcat(line, "\n");
			OutLine_InternalPager(line);
		}
	}
#else /* COLUMN_HELP */
	{
		/* sort subtopics by column */
		int subtopic, sublen;
		int spacelen = 0, ispacelen;
		int row, col;
		int rows = (int)(stopics / PER_LINE) + 1;
		for(row = 0; row < rows; row++) {
			*line = '\0';
			for(ispacelen = 0; ispacelen < FIRSTCOL; ispacelen++)
				strcat(line, " ");
			for(col = 0; col < PER_LINE; col++) {
				subtopic = row + rows * col;
				if(subtopic >= stopics) {
					break;
				}
				else {
					start = starts[subtopic];
					sublen = strcspn(start, " ");
					strncat(line, start, sublen);
					spacelen = COLLENGTH - sublen;
					if(spacelen <= 0)
						spacelen = 1;
					for(ispacelen = 0; ispacelen < spacelen; ispacelen++)
						strcat(line, " ");
				}
			}
			strcat(line, "\n");
			OutLine_InternalPager(line);
		}
	}
#endif /* COLUMN_HELP */
	ASSIGN_PTR(subtopics, LOGIC(subt));
}
// 
// StartOutput:
// Open a file pointer to a pipe to user's $PAGER, if there is one,
// otherwise use our own pager.
// 
void GnuPlot::StartOutput()
{
	char * line_count = NULL;
#if defined(PIPES)
	char * pager_name = getenv("PAGER");
	if(!isempty(pager_name)) {
		RestrictPOpen();
		if((outfile = popen(pager_name, "w")) != (FILE*)NULL)
			return; /* success */
	}
	outfile = stderr;
	// @fallthrough to built-in pager 
#endif
	// built-in dumb pager: use the line count provided by the terminal 
	line_count = getenv("LINES");
	screensize = SCREENSIZE;
	if(line_count)
		screensize = (int)strtol(line_count, NULL, 0);
#ifdef __DJGPP__
	if(line_count == NULL)
		screensize = ScreenRows();
#endif
	if(screensize < 3)
		screensize = SCREENSIZE;
	// built-in pager 
	pagelines = 0;
}

/* write a line of help output  */
/* line should contain only one \n, at the end */
void OutLine(const char * line)
{
	int c; /* dummy input char */
#if defined(PIPES)
	if(outfile != stderr) {
		fputs(line, outfile);
		return;
	}
#endif
	// built-in dumb pager 
	// leave room for prompt line 
	if(pagelines >= screensize - 2) {
		fputs("Press return for more: ", stderr);
#if defined(_WIN32)
		do { c = getchar(); } while(c != EOF && c != '\n' && c != '\r');
#else
		do { c = getchar(); } while(c != EOF && c != '\n');
#endif
		pagelines = 0;
	}
	fputs(line, stderr);
	pagelines++;
}

/* Same as Outline, but does not go through the external pager.
 * Used for the list of available subtopics because if it would be passed to
 * 'less' (for example), the list would not be displayed anymore after 'less'
 * has exited and the user is asked for a subtopic */
void OutLine_InternalPager(const char * line)
{
	int c; /* dummy input char */
#if defined(PIPES)
	if(outfile != stderr) {
		// do not go through external pager 
		fputs(line, stderr);
		return;
	}
#endif /* PIPES */
	// built-in dumb pager 
	// leave room for prompt line 
	if(pagelines >= screensize - 2) {
		fputs("Press return for more: ", stderr);
#if defined(_WIN32) || defined(MSDOS)
		do { c = getchar(); } while(c != EOF && c != '\n' && c != '\r');
#else
		do { c = getchar(); } while(c != EOF && c != '\n');
#endif
		pagelines = 0;
	}
	fputs(line, stderr);
	pagelines++;
}

void EndOutput()
{
#if defined(PIPES)
	if(outfile != stderr)
		pclose(outfile);
#endif
}

#endif  /* #ifdef NO_GIH */
