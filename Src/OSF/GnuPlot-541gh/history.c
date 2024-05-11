// GNUPLOT - history.c 
// Copyright 1986 - 1993, 1999, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop
//
// Public variables
//
//int  gnuplot_history_size = HISTORY_SIZE;
//bool history_quiet = false;
//bool history_full = false;

#if defined(READLINE)
//
// Built-in readline 
//
//HIST_ENTRY * history = NULL; // last entry in the history list, no history yet 
//HIST_ENTRY * cur_entry = NULL;
//int history_length = 0; // number of entries in history list 
//int history_base = 1;
//
// add line to the history 
//
//void add_history(const char * line)
void GnuPlot::AddHistory(const char * pLine)
{
	HIST_ENTRY * p_entry = (HIST_ENTRY *)SAlloc::M(sizeof(HIST_ENTRY));
	p_entry->line = sstrdup(pLine);
	p_entry->data = NULL;
	p_entry->prev = Hist.P_History;
	p_entry->next = NULL;
	if(Hist.P_History)
		Hist.P_History->next = p_entry;
	else
		Hist.P_CurEntry = p_entry;
	Hist.P_History = p_entry;
	Hist.HistoryLength++;
}
// 
// write history to a file
// 
//int write_history(char * filename)
int GnuPlot::WriteHistory(const char * pFileName)
{
	WriteHistoryN(0, pFileName, "w");
	return 0;
}
// 
// routine to read history entries from a file
// 
//void read_history(char * filename)
void GnuPlot::ReadHistory(const char * pFileName)
{
	Implement_ReadHistory(pFileName);
}

void using_history()
{
	// Nothing to do. 
}

//void clear_history()
void GnuPlot::ClearHistory()
{
	for(HIST_ENTRY * entry = Hist.P_History; entry;) {
		HIST_ENTRY * prev = entry->prev;
		SAlloc::F(entry->line);
		SAlloc::F(entry);
		entry = prev;
	}
	Hist.HistoryLength = 0;
	Hist.P_CurEntry = NULL;
	Hist.P_History = NULL;
}

//int where_history()
int GnuPlot::WhereHistory()
{
	HIST_ENTRY * entry = Hist.P_History; // last_entry 
	int hist_index = Hist.HistoryLength;
	if(entry == NULL)
		return 0; // no history yet 
	if(!Hist.P_CurEntry)
		return Hist.HistoryLength;
	// find the current history entry and count backwards 
	while(entry->prev && (entry != Hist.P_CurEntry)) {
		entry = entry->prev;
		hist_index--;
	}
	if(hist_index > 0)
		hist_index--;
	return hist_index;
}

//int history_set_pos(int offset)
int GnuPlot::HistorySetPos(int offset)
{
	HIST_ENTRY * entry = Hist.P_History; // last_entry 
	int hist_index = Hist.HistoryLength - 1;
	if((offset < 0) || (offset > Hist.HistoryLength) || !Hist.P_History)
		return 0;
	if(offset == Hist.HistoryLength) {
		Hist.P_CurEntry = NULL;
		return 1;
	}
	else {
		// seek backwards 
		while(entry) {
			if(hist_index == offset) {
				Hist.P_CurEntry = entry;
				return 1;
			}
			entry = entry->prev;
			hist_index--;
		}
		return 0;
	}
}

//HIST_ENTRY * history_get(int offset)
HIST_ENTRY * GnuPlot::HistoryGet(int offset)
{
	HIST_ENTRY * entry = Hist.P_History; // last_entry 
	int hist_index = Hist.HistoryLength - 1;
	int hist_ofs = offset - Hist.HistoryBase;
	if((hist_ofs < 0) || (hist_ofs >= Hist.HistoryLength) || !Hist.P_History)
		return NULL;
	// find the current history entry and count backwards 
	// seek backwards 
	while(entry) {
		if(hist_index == hist_ofs)
			return entry;
		entry = entry->prev;
		hist_index--;
	}
	return NULL;
}

//HIST_ENTRY * current_history()
HIST_ENTRY * GnuPlot::CurrentHistory()
{
	return Hist.P_CurEntry;
}

//HIST_ENTRY * previous_history()
HIST_ENTRY * GnuPlot::PreviousHistory()
{
	if(!Hist.P_CurEntry)
		return (Hist.P_CurEntry = Hist.P_History);
	else if(Hist.P_CurEntry && Hist.P_CurEntry->prev)
		return (Hist.P_CurEntry = Hist.P_CurEntry->prev);
	else
		return NULL;
}

//HIST_ENTRY * next_history()
HIST_ENTRY * GnuPlot::NextHistory()
{
	if(Hist.P_CurEntry)
		Hist.P_CurEntry = Hist.P_CurEntry->next;
	return Hist.P_CurEntry;
}

//HIST_ENTRY * replace_history_entry(int which, const char * line, histdata_t data)
HIST_ENTRY * GnuPlot::ReplaceHistoryEntry(int which, const char * line, histdata_t data)
{
	HIST_ENTRY * entry = HistoryGet(which + 1);
	HIST_ENTRY * prev_entry = 0;
	if(entry == NULL)
		return NULL;
	else {
		// save contents: allocate new entry 
		prev_entry = (HIST_ENTRY *)SAlloc::M(sizeof(HIST_ENTRY));
		if(entry) {
			memzero(prev_entry, sizeof(HIST_ENTRY));
			prev_entry->line = entry->line;
			prev_entry->data = entry->data;
		}
		// set new value 
		entry->line = sstrdup(line);
		entry->data = data;
		return prev_entry;
	}
}

//HIST_ENTRY * remove_history(int which)
HIST_ENTRY * GnuPlot::RemoveHistory(int which)
{
	HIST_ENTRY * entry = HistoryGet(which + Hist.HistoryBase);
	if(entry) {
		// remove entry from chain 
		if(entry->prev)
			entry->prev->next = entry->next;
		if(entry->next)
			entry->next->prev = entry->prev;
		else
			Hist.P_History = entry->prev; /* last entry */
		if(Hist.P_CurEntry == entry)
			Hist.P_CurEntry = entry->prev;
		// adjust length 
		Hist.HistoryLength--;
	}
	return entry;
}

#endif

#if defined(READLINE) || defined(HAVE_LIBEDITLINE)
	histdata_t free_history_entry(HIST_ENTRY * histent)
	{
		histdata_t data = 0;
		if(histent) {
			data = histent->data;
			SAlloc::F((void *)(histent->line));
			SAlloc::F(histent);
		}
		return data;
	}
#endif

#if defined(READLINE) || defined(HAVE_WINEDITLINE)
//int history_search(const char * string, int direction)
int GnuPlot::HistorySearch(const char * pString, int direction)
{
	// Work-around for WinEditLine: 
	int once = 1; /* ensure that we try seeking at least one position */
	char * pos;
	int start = WhereHistory();
	HIST_ENTRY * entry = CurrentHistory();
	while((entry && entry->line) || once) {
		if(entry && entry->line && ((pos = strstr(entry->line, pString)) != NULL))
			return (pos - entry->line);
		entry = (direction < 0) ? PreviousHistory() : NextHistory();
		once = 0;
	}
	// not found 
	HistorySetPos(start);
	return -1;
}

//int history_search_prefix(const char * string, int direction)
int GnuPlot::HistorySearchPrefix(const char * string, int direction)
{
	// Work-around for WinEditLine: 
	int once = 1; // ensure that we try seeking at least one position 
	size_t len = strlen(string);
	int start = WhereHistory();
	for(HIST_ENTRY * entry = CurrentHistory(); (entry && entry->line) || once;) {
		if(entry && entry->line && (strncmp(entry->line, string, len) == 0))
			return 0;
		entry = (direction < 0) ? PreviousHistory() : NextHistory();
		once = 0;
	}
	// not found 
	HistorySetPos(start);
	return -1;
}

#endif
#ifdef GNUPLOT_HISTORY
// 
// routine to read history entries from a file,
// this complements write_history and is necessary for
// saving of history when we are not using libreadline
// 
//int gp_read_history(const char * filename)
int GnuPlot::Implement_ReadHistory(const char * pFileName)
{
	FILE * hist_file = fopen(pFileName, "r");
	if(hist_file) {
		while(!feof(hist_file)) {
			char line[MAX_LINE_LEN + 1];
			char * pline = fgets(line, MAX_LINE_LEN, hist_file);
			if(pline) {
				// remove trailing linefeed 
				if((pline = sstrrchr(line, '\n')))
					*pline = '\0';
				if((pline = sstrrchr(line, '\r')))
					*pline = '\0';
				// skip leading whitespace 
				pline = line;
				while(isspace((uchar)*pline))
					pline++;
				// avoid adding empty lines 
				if(*pline)
					AddHistory(pline);
			}
		}
		fclose(hist_file);
		return 0;
	}
	else
		return errno;
}
#endif
#ifdef USE_READLINE
// 
// Save history to file, or write to stdout or pipe.
// For pipes, only "|" works, pipes starting with ">" get a strange
// filename like in the non-readline version.
// 
// Peter Weilbacher, 28Jun2004
// 
//void write_history_list(const int num, const char * const filename, const char * mode)
void GnuPlot::WriteHistoryList(const int num, const char * const filename, const char * mode)
{
	const HIST_ENTRY * list_entry;
	FILE * out = stdout;
	int is_pipe = 0;
	int is_file = 0;
	int is_quiet = 0;
	int i, istart;
	if(!isempty(filename)) {
		// good filename given and not quiet 
#ifdef PIPES
		if(filename[0] == '|') {
			RestrictPOpen();
			out = popen(filename + 1, "w");
			is_pipe = 1;
		}
		else
#endif
		{
			out = fopen(filename, mode);
			if(!out) {
				IntWarn(NO_CARET, "Cannot open file to save history, using standard output.\n");
				out = stdout;
			}
			else
				is_file = 1;
		}
	}
	else if(filename && !filename[0]) {
		is_quiet = 1;
	}
	// Determine starting point and output in loop. 
	istart = (num > 0) ? (Hist.HistoryLength - num - 1) : 0;
	if(istart < 0 || istart > Hist.HistoryLength)
		istart = 0;
	for(i = istart; (list_entry = HistoryGet(i + Hist.HistoryBase)); i++) {
		// don't add line numbers when writing to file to make file loadable 
		if(!is_file && !is_quiet)
			fprintf(out, "%5i   %s\n", i + Hist.HistoryBase, list_entry->line);
		else
			fprintf(out, "%s\n", list_entry->line);
	}
#ifdef PIPES
	if(is_pipe) 
		pclose(out);
#endif
	if(is_file) 
		fclose(out);
}
//
// This is the function getting called in command.c 
//
//void write_history_n(const int n, const char * filename, const char * mode)
void GnuPlot::WriteHistoryN(const int n, const char * pFileName, const char * pMode)
{
	WriteHistoryList(n, pFileName, pMode);
}

#endif

#ifdef USE_READLINE
//
// finds and returns a command from the history list by number 
//
//const char * history_find_by_number(int n)
const char * GnuPlot::HistoryFindByNumber(int n)
{
	return (0 < n && n < Hist.HistoryLength) ? HistoryGet(n)->line : NULL;
}
// 
// finds and returns a command from the history which starts with <cmd>
// Returns NULL if nothing found
// 
// Peter Weilbacher, 28Jun2004
// 
//const char * history_find(char * cmd)
const char * GnuPlot::HistoryFind(char * pCmd)
{
	int len;
	// remove quotes 
	if(*pCmd == '"')
		pCmd++;
	if(!*pCmd)
		return NULL;
	len = strlen(pCmd);
	if(pCmd[len-1] == '"')
		pCmd[--len] = '\0';
	if(!*pCmd)
		return NULL;
	// Start at latest entry 
#if !defined(HAVE_LIBEDITLINE)
	HistorySetPos(Hist.HistoryLength);
#else
	while(PreviousHistory());
#endif
	//* Anchored backward search for prefix 
	if(HistorySearchPrefix(pCmd, -1) == 0)
		return CurrentHistory()->line;
	return NULL;
}
// 
// finds and print all occurencies of commands from the history which start with <cmd>
// Returns the number of found entries on success, and 0 if no such entry exists
// 
// Peter Weilbacher 28Jun2004
// 
//int history_find_all(char * cmd)
int GnuPlot::HistoryFindAll(char * pCmd)
{
	int len;
	int found;
	int ret;
	int number = 0; // each entry found increases this 
	// remove quotes 
	if(*pCmd == '"')
		pCmd++;
	if(!*pCmd)
		return 0;
	len = strlen(pCmd);
	if(pCmd[len-1] == '"')
		pCmd[--len] = 0;
	if(!*pCmd)
		return 0;
	// Output matching history entries in chronological order (not backwards
	// so we have to start at the beginning of the history list.
#if !defined(HAVE_LIBEDITLINE)
	ret = HistorySetPos(0);
	if(ret == 0) {
		fprintf(stderr, "ERROR (history_find_all): could not rewind history\n");
		return 0;
	}
#else /* HAVE_LIBEDITLINE */
	// libedit's GnuPlot::HistorySetPos() does not work properly, so we manually go to the oldest entry. Note that directions are reversed. 
	while(NextHistory());
#endif
	do {
		found = HistorySearchPrefix(pCmd, 1); // Anchored backward search for prefix 
		if(found == 0) {
			number++;
#if !defined(HAVE_LIBEDITLINE)
			printf("%5i  %s\n", WhereHistory() + Hist.HistoryBase, CurrentHistory()->line);
			// Advance one step or we find always the same entry
			if(NextHistory() == NULL)
				break; /* finished if stepping didn't work */
#else /* HAVE_LIBEDITLINE */
			// libedit's history indices are reversed wrt GNU readline 
			printf("%5i  %s\n", Hist.HistoryLength - WhereHistory() + Hist.HistoryBase, CurrentHistory()->line);
			// Advance one step or we find always the same entry.
			if(!PreviousHistory())
				break; /* finished if stepping didn't work */
#endif
		} /* (found == 0) */
	} while(found > -1);
	return number;
}

#endif /* READLINE */
