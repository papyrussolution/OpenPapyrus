// PRINT.C
//
#include "ag.h"
#pragma hdrstop

int first_file_match = 1; // @global
const char * color_reset = "\033[0m\033[K"; // @global
const char * truncate_marker = " [...]"; // @global

struct AgPrintContext {
	size_t line;
	char ** context_prev_lines;
	size_t prev_line;
	size_t last_prev_line;
	size_t prev_line_offset;
	size_t line_preceding_current_match_offset;
	size_t lines_since_last_match;
	size_t last_printed_match;
	int in_a_match;
	int printing_a_match;
};

#ifdef _WIN32
	#define fprintf(...) fprintf_w32(__VA_ARGS__)
	__declspec(thread) AgPrintContext __print_context; // @global
#else
	__thread AgPrintContext __print_context; // @global
#endif

void print_init_context(void) 
{
	if(!__print_context.context_prev_lines) {
		__print_context.context_prev_lines = (char **)ag_calloc(sizeof(char *), (opts.before + 1));
		__print_context.line = 1;
		__print_context.prev_line = 0;
		__print_context.last_prev_line = 0;
		__print_context.prev_line_offset = 0;
		__print_context.line_preceding_current_match_offset = 0;
		__print_context.lines_since_last_match = INT_MAX;
		__print_context.last_printed_match = 0;
		__print_context.in_a_match = FALSE;
		__print_context.printing_a_match = FALSE;
	}
}

void print_cleanup_context(void) 
{
	if(__print_context.context_prev_lines) {
		for(size_t i = 0; i < opts.before; i++) {
			if(__print_context.context_prev_lines[i] != NULL) {
				SAlloc::F(__print_context.context_prev_lines[i]);
			}
		}
		SAlloc::F(__print_context.context_prev_lines);
		__print_context.context_prev_lines = NULL;
	}
}

void print_context_append(const char * line, size_t len) 
{
	if(opts.before) {
		SAlloc::F(__print_context.context_prev_lines[__print_context.last_prev_line]);
		__print_context.context_prev_lines[__print_context.last_prev_line] = ag_strndup(line, len);
		__print_context.last_prev_line = (__print_context.last_prev_line + 1) % opts.before;
	}
}

void print_trailing_context(const char * path, const char * buf, size_t n) 
{
	char sep = '-';
	if(opts.ackmate || opts.vimgrep) {
		sep = ':';
	}
	if(__print_context.lines_since_last_match != 0 && __print_context.lines_since_last_match <= opts.after) {
		if(opts.print_path == PATH_PRINT_EACH_LINE) {
			print_path(path, ':');
		}
		print_line_number(__print_context.line, sep);
		fwrite(buf, 1, n, out_fd);
		fputc('\n', out_fd);
	}
	__print_context.line++;
	if(!__print_context.in_a_match && __print_context.lines_since_last_match < INT_MAX) {
		__print_context.lines_since_last_match++;
	}
}

void print_path(const char * path, const char sep) 
{
	if(opts.print_path == PATH_PRINT_NOTHING && !opts.vimgrep) {
		return;
	}
	path = normalize_path(path);
	if(opts.ackmate) {
		fprintf(out_fd, ":%s%c", path, sep);
	}
	else if(opts.vimgrep) {
		fprintf(out_fd, "%s%c", path, sep);
	}
	else {
		if(opts.color)
			fprintf(out_fd, "%s%s%s%c", opts.color_path, path, color_reset, sep);
		else
			fprintf(out_fd, "%s%c", path, sep);
	}
}

void print_path_count(const char * path, const char sep, const size_t count) 
{
	if(*path) {
		print_path(path, ':');
	}
	if(opts.color) {
		fprintf(out_fd, "%s%lu%s%c", opts.color_line_number, (ulong)count, color_reset, sep);
	}
	else {
		fprintf(out_fd, "%lu%c", (ulong)count, sep);
	}
}

void print_line(const char * buf, size_t buf_pos, size_t prev_line_offset) 
{
	size_t write_chars = buf_pos - prev_line_offset + 1;
	if(opts.width > 0 && opts.width < write_chars) {
		write_chars = opts.width;
	}
	fwrite(buf + prev_line_offset, 1, write_chars, out_fd);
}

void print_binary_file_matches(const char * path) 
{
	path = normalize_path(path);
	print_file_separator();
	fprintf(out_fd, "Binary file %s matches.\n", path);
}

void print_file_matches(const char * path, const char * buf, const size_t buf_len, const match_t matches[], const size_t matches_len) 
{
	size_t cur_match = 0;
	ssize_t lines_to_print = 0;
	char sep = '-';
	size_t i, j;
	int blanks_between_matches = opts.context || opts.after || opts.before;
	if(opts.ackmate || opts.vimgrep) {
		sep = ':';
	}
	print_file_separator();
	if(opts.print_path == PATH_PRINT_DEFAULT) {
		opts.print_path = PATH_PRINT_TOP;
	}
	else if(opts.print_path == PATH_PRINT_DEFAULT_EACH_LINE) {
		opts.print_path = PATH_PRINT_EACH_LINE;
	}
	if(opts.print_path == PATH_PRINT_TOP) {
		if(opts.print_count) {
			print_path_count(path, opts.path_sep, matches_len);
		}
		else {
			print_path(path, opts.path_sep);
		}
	}
	for(i = 0; i <= buf_len && (cur_match < matches_len || __print_context.lines_since_last_match <= opts.after); i++) {
		if(cur_match < matches_len && i == matches[cur_match].start) {
			__print_context.in_a_match = TRUE;
			// We found the start of a match 
			if(cur_match > 0 && blanks_between_matches && __print_context.lines_since_last_match > (opts.before + opts.after + 1)) {
				fprintf(out_fd, "--\n");
			}
			if(__print_context.lines_since_last_match > 0 && opts.before > 0) {
				// TODO: better, but still needs work 
				// print the previous line(s) 
				lines_to_print = __print_context.lines_since_last_match - (opts.after + 1);
				if(lines_to_print < 0) {
					lines_to_print = 0;
				}
				else if((size_t)lines_to_print > opts.before) {
					lines_to_print = opts.before;
				}

				for(j = (opts.before - lines_to_print); j < opts.before; j++) {
					__print_context.prev_line = (__print_context.last_prev_line + j) % opts.before;
					if(__print_context.context_prev_lines[__print_context.prev_line] != NULL) {
						if(opts.print_path == PATH_PRINT_EACH_LINE) {
							print_path(path, ':');
						}
						print_line_number(__print_context.line - (opts.before - j), sep);
						fprintf(out_fd, "%s\n", __print_context.context_prev_lines[__print_context.prev_line]);
					}
				}
			}
			__print_context.lines_since_last_match = 0;
		}
		if(cur_match < matches_len && i == matches[cur_match].end) {
			// We found the end of a match. 
			cur_match++;
			__print_context.in_a_match = FALSE;
		}
		// We found the end of a line. 
		if((i == buf_len || buf[i] == '\n') && opts.before > 0) {
			// We don't want to strcpy the \n 
			print_context_append(&buf[__print_context.prev_line_offset], i - __print_context.prev_line_offset);
		}
		if(i == buf_len || buf[i] == '\n') {
			if(__print_context.lines_since_last_match == 0) {
				if(opts.print_path == PATH_PRINT_EACH_LINE && !opts.search_stream) {
					print_path(path, ':');
				}
				if(opts.ackmate) {
					// print headers for ackmate to parse */
					print_line_number(__print_context.line, ';');
					for(; __print_context.last_printed_match < cur_match; __print_context.last_printed_match++) {
						size_t start = matches[__print_context.last_printed_match].start - __print_context.line_preceding_current_match_offset;
						fprintf(out_fd, "%lu %lu", start, matches[__print_context.last_printed_match].end - matches[__print_context.last_printed_match].start);
						__print_context.last_printed_match == cur_match - 1 ? fputc(':', out_fd) : fputc(',', out_fd);
					}
					print_line(buf, i, __print_context.prev_line_offset);
				}
				else if(opts.vimgrep) {
					for(; __print_context.last_printed_match < cur_match; __print_context.last_printed_match++) {
						print_path(path, sep);
						print_line_number(__print_context.line, sep);
						print_column_number(matches, __print_context.last_printed_match, __print_context.prev_line_offset, sep);
						print_line(buf, i, __print_context.prev_line_offset);
					}
				}
				else {
					print_line_number(__print_context.line, ':');
					int printed_match = FALSE;
					if(opts.column) {
						print_column_number(matches, __print_context.last_printed_match, __print_context.prev_line_offset, ':');
					}
					if(__print_context.printing_a_match && opts.color) {
						fprintf(out_fd, "%s", opts.color_match);
					}
					for(j = __print_context.prev_line_offset; j <= i; j++) {
						// close highlight of match term 
						if(__print_context.last_printed_match < matches_len && j == matches[__print_context.last_printed_match].end) {
							if(opts.color) {
								fprintf(out_fd, "%s", color_reset);
							}
							__print_context.printing_a_match = FALSE;
							__print_context.last_printed_match++;
							printed_match = TRUE;
							if(opts.only_matching) {
								fputc('\n', out_fd);
							}
						}
						// skip remaining characters if truncation width exceeded, needs to be done
						// before highlight opening 
						if(j < buf_len && opts.width > 0 && j - __print_context.prev_line_offset >= opts.width) {
							if(j < i) {
								fputs(truncate_marker, out_fd);
							}
							fputc('\n', out_fd);
							// prevent any more characters or highlights 
							j = i;
							__print_context.last_printed_match = matches_len;
						}
						// open highlight of match term 
						if(__print_context.last_printed_match < matches_len && j == matches[__print_context.last_printed_match].start) {
							if(opts.only_matching && printed_match) {
								if(opts.print_path == PATH_PRINT_EACH_LINE) {
									print_path(path, ':');
								}
								print_line_number(__print_context.line, ':');
								if(opts.column) {
									print_column_number(matches, __print_context.last_printed_match, __print_context.prev_line_offset, ':');
								}
							}
							if(opts.color) {
								fprintf(out_fd, "%s", opts.color_match);
							}
							__print_context.printing_a_match = TRUE;
						}
						// Don't print the null terminator 
						if(j < buf_len) {
							// if only_matching is set, print only matches and newlines */
							if(!opts.only_matching || __print_context.printing_a_match) {
								if(opts.width == 0 || j - __print_context.prev_line_offset < opts.width) {
									fputc(buf[j], out_fd);
								}
							}
						}
					}
					if(__print_context.printing_a_match && opts.color) {
						fprintf(out_fd, "%s", color_reset);
					}
				}
			}
			if(opts.search_stream) {
				__print_context.last_printed_match = 0;
				break;
			}
			// print context after matching line 
			print_trailing_context(path, &buf[__print_context.prev_line_offset], i - __print_context.prev_line_offset);
			__print_context.prev_line_offset = i + 1; /* skip the newline */
			if(!__print_context.in_a_match) {
				__print_context.line_preceding_current_match_offset = i + 1;
			}
			// File doesn't end with a newline. Print one so the output is pretty. 
			if(i == buf_len && buf[i - 1] != '\n') {
				fputc('\n', out_fd);
			}
		}
	}
	// Flush output if stdout is not a tty 
	if(opts.stdout_inode) {
		fflush(out_fd);
	}
}

void print_line_number(size_t line, const char sep) 
{
	if(opts.print_line_numbers) {
		if(opts.color) {
			fprintf(out_fd, "%s%lu%s%c", opts.color_line_number, (ulong)line, color_reset, sep);
		}
		else {
			fprintf(out_fd, "%lu%c", (ulong)line, sep);
		}
	}
}

void print_column_number(const match_t matches[], size_t last_printed_match, size_t prev_line_offset, const char sep) 
{
	size_t column = 0;
	if(prev_line_offset <= matches[last_printed_match].start) {
		column = (matches[last_printed_match].start - prev_line_offset) + 1;
	}
	fprintf(out_fd, "%lu%c", (ulong)column, sep);
}

void print_file_separator(void) 
{
	if(first_file_match == 0 && opts.print_break) {
		fprintf(out_fd, "\n");
	}
	first_file_match = 0;
}

const char * normalize_path(const char * path) 
{
	if(strlen(path) < 3) {
		return path;
	}
	if(path[0] == '.' && path[1] == '/') {
		return path + 2;
	}
	if(path[0] == '/' && path[1] == '/') {
		return path + 1;
	}
	return path;
}

#ifdef _WIN32

#ifndef FOREGROUND_MASK
	#define FOREGROUND_MASK (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#endif
#ifndef BACKGROUND_MASK
	#define BACKGROUND_MASK (BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY)
#endif
#define FG_RGB (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN)
#define BG_RGB (BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_GREEN)

// BUFSIZ is guarenteed to be "at least 256 bytes" which might
// not be enough for us. Use an arbitrary but reasonably big
// buffer. win32 colored output will be truncated to this length.
#define BUF_SIZE (16 * 1024)
#define MAX_VALUES 8 // max consecutive ansi sequence values beyond which we're aborting e.g. this is 3 values: \e[0;1;33m

static int g_use_ansi = 0;

void windows_use_ansi(int use_ansi) 
{
	g_use_ansi = use_ansi;
}

int fprintf_w32(FILE * fp, const char * format, ...) 
{
	va_list args;
	char buf[BUF_SIZE] = { 0 }, * ptr = buf;
	static WORD attr_reset;
	static BOOL attr_initialized = FALSE;
	HANDLE stdo = INVALID_HANDLE_VALUE;
	WORD attr;
	DWORD written, csize;
	CONSOLE_CURSOR_INFO cci;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD coord;

	// if we don't output to screen (tty) e.g. for pager/pipe or
	// if for other reason we can't access the screen info, of if
	// the user just prefers ansi, do plain passthrough.
	BOOL passthrough = g_use_ansi || !_isatty(_fileno(fp)) || INVALID_HANDLE_VALUE == (stdo = (HANDLE)_get_osfhandle(_fileno(fp))) || !GetConsoleScreenBufferInfo(stdo, &csbi);
	if(passthrough) {
		int rv;
		va_start(args, format);
		rv = vfprintf(fp, format, args);
		va_end(args);
		return rv;
	}
	va_start(args, format);
	// truncates to (null terminated) BUF_SIZE if too long.
	// if too long - vsnprintf will fill count chars without
	// terminating null. buf is zeroed, so make sure we don't fill it.
	vsnprintf(buf, BUF_SIZE - 1, format, args);
	va_end(args);
	attr = csbi.wAttributes;
	if(!attr_initialized) {
		// reset is defined to have all (non color) attributes off
		attr_reset = attr & (FG_RGB | BG_RGB);
		attr_initialized = TRUE;
	}

	while(*ptr) {
		if(*ptr == '\033') {
			uchar c;
			int i, n = 0, m = '\0', v[MAX_VALUES], w, h;
			for(i = 0; i < MAX_VALUES; i++)
				v[i] = -1;
			ptr++;
retry:
			if((c = *ptr++) == 0)
				break;
			if(isdigit(c)) {
				if(v[n] == -1)
					v[n] = c - '0';
				else
					v[n] = v[n] * 10 + c - '0';
				goto retry;
			}
			if(c == '[') {
				goto retry;
			}
			if(c == ';') {
				if(++n == MAX_VALUES)
					break;
				goto retry;
			}
			if(c == '>' || c == '?') {
				m = c;
				goto retry;
			}

			switch(c) {
				// n is the last occupied index, so we have n+1 values
				case 'h':
				    if(m == '?') {
					    for(i = 0; i <= n; i++) {
						    switch(v[i]) {
							    case 3:
									GetConsoleScreenBufferInfo(stdo, &csbi);
									w = csbi.dwSize.X;
									h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
									csize = w * (h + 1);
									coord.X = 0;
									coord.Y = csbi.srWindow.Top;
									FillConsoleOutputCharacter(stdo, ' ', csize, coord, &written);
									FillConsoleOutputAttribute(stdo, csbi.wAttributes, csize, coord, &written);
									SetConsoleCursorPosition(stdo, csbi.dwCursorPosition);
									csbi.dwSize.X = 132;
									SetConsoleScreenBufferSize(stdo, csbi.dwSize);
									csbi.srWindow.Right = csbi.srWindow.Left + 131;
									SetConsoleWindowInfo(stdo, TRUE, &csbi.srWindow);
									break;
							    case 5:
									attr = ((attr & FOREGROUND_MASK) << 4) | ((attr & BACKGROUND_MASK) >> 4);
									SetConsoleTextAttribute(stdo, attr);
									break;
							    case 9:
									break;
							    case 25:
									GetConsoleCursorInfo(stdo, &cci);
									cci.bVisible = TRUE;
									SetConsoleCursorInfo(stdo, &cci);
									break;
							    case 47:
									coord.X = 0;
									coord.Y = 0;
									SetConsoleCursorPosition(stdo, coord);
									break;
							    default:
									break;
						    }
					    }
				    }
				    else if(m == '>' && v[0] == 5) {
					    GetConsoleCursorInfo(stdo, &cci);
					    cci.bVisible = FALSE;
					    SetConsoleCursorInfo(stdo, &cci);
				    }
				    break;
				case 'l':
				    if(m == '?') {
					    for(i = 0; i <= n; i++) {
						    switch(v[i]) {
							    case 3:
									GetConsoleScreenBufferInfo(stdo, &csbi);
									w = csbi.dwSize.X;
									h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
									csize = w * (h + 1);
									coord.X = 0;
									coord.Y = csbi.srWindow.Top;
									FillConsoleOutputCharacter(stdo, ' ', csize, coord, &written);
									FillConsoleOutputAttribute(stdo, csbi.wAttributes, csize, coord, &written);
									SetConsoleCursorPosition(stdo, csbi.dwCursorPosition);
									csbi.srWindow.Right = csbi.srWindow.Left + 79;
									SetConsoleWindowInfo(stdo, TRUE, &csbi.srWindow);
									csbi.dwSize.X = 80;
									SetConsoleScreenBufferSize(stdo, csbi.dwSize);
									break;
							    case 5:
									attr = ((attr & FOREGROUND_MASK) << 4) | ((attr & BACKGROUND_MASK) >> 4);
									SetConsoleTextAttribute(stdo, attr);
									break;
							    case 25:
									GetConsoleCursorInfo(stdo, &cci);
									cci.bVisible = FALSE;
									SetConsoleCursorInfo(stdo, &cci);
									break;
							    default:
									break;
						    }
					    }
				    }
				    else if(m == '>' && v[0] == 5) {
					    GetConsoleCursorInfo(stdo, &cci);
					    cci.bVisible = TRUE;
					    SetConsoleCursorInfo(stdo, &cci);
				    }
				    break;
				case 'm':
				    for(i = 0; i <= n; i++) {
					    if(v[i] == -1 || v[i] == 0)
						    attr = attr_reset;
					    else if(v[i] == 1)
						    attr |= FOREGROUND_INTENSITY;
					    else if(v[i] == 4)
						    attr |= FOREGROUND_INTENSITY;
					    else if(v[i] == 5) // blink is typically applied as bg intensity
						    attr |= BACKGROUND_INTENSITY;
					    else if(v[i] == 7)
						    attr =
							((attr & FOREGROUND_MASK) << 4) |
							((attr & BACKGROUND_MASK) >> 4);
					    else if(v[i] == 10)
						    ; // symbol on
					    else if(v[i] == 11)
						    ; // symbol off
					    else if(v[i] == 22)
						    attr &= ~FOREGROUND_INTENSITY;
					    else if(v[i] == 24)
						    attr &= ~FOREGROUND_INTENSITY;
					    else if(v[i] == 25)
						    attr &= ~BACKGROUND_INTENSITY;
					    else if(v[i] == 27)
						    attr =
							((attr & FOREGROUND_MASK) << 4) |
							((attr & BACKGROUND_MASK) >> 4);
					    else if(v[i] >= 30 && v[i] <= 37) {
						    attr &= ~FG_RGB; // doesn't affect attributes
						    if((v[i] - 30) & 1)
							    attr |= FOREGROUND_RED;
						    if((v[i] - 30) & 2)
							    attr |= FOREGROUND_GREEN;
						    if((v[i] - 30) & 4)
							    attr |= FOREGROUND_BLUE;
					    }
					    else if(v[i] == 39) // reset fg color and attributes
						    attr = (attr & ~FOREGROUND_MASK) | (attr_reset & FG_RGB);
					    else if(v[i] >= 40 && v[i] <= 47) {
						    attr &= ~BG_RGB;
						    if((v[i] - 40) & 1)
							    attr |= BACKGROUND_RED;
						    if((v[i] - 40) & 2)
							    attr |= BACKGROUND_GREEN;
						    if((v[i] - 40) & 4)
							    attr |= BACKGROUND_BLUE;
					    }
					    else if(v[i] == 49) // reset bg color
						    attr = (attr & ~BACKGROUND_MASK) | (attr_reset & BG_RGB);
				    }
				    SetConsoleTextAttribute(stdo, attr);
				    break;
				case 'K':
				    GetConsoleScreenBufferInfo(stdo, &csbi);
				    coord = csbi.dwCursorPosition;
				    switch(v[0]) {
					    default:
					    case 0:
							csize = csbi.dwSize.X - coord.X;
							break;
					    case 1:
							csize = coord.X;
							coord.X = 0;
							break;
					    case 2:
							csize = csbi.dwSize.X;
							coord.X = 0;
							break;
				    }
				    FillConsoleOutputCharacter(stdo, ' ', csize, coord, &written);
				    FillConsoleOutputAttribute(stdo, csbi.wAttributes, csize, coord, &written);
				    SetConsoleCursorPosition(stdo, csbi.dwCursorPosition);
				    break;
				case 'J':
				    GetConsoleScreenBufferInfo(stdo, &csbi);
				    w = csbi.dwSize.X;
				    h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
				    coord = csbi.dwCursorPosition;
				    switch(v[0]) {
					    default:
					    case 0:
							csize = w * (h - coord.Y) - coord.X;
							coord.X = 0;
							break;
					    case 1:
							csize = w * coord.Y + coord.X;
							coord.X = 0;
							coord.Y = csbi.srWindow.Top;
							break;
					    case 2:
							csize = w * (h + 1);
							coord.X = 0;
							coord.Y = csbi.srWindow.Top;
							break;
				    }
				    FillConsoleOutputCharacter(stdo, ' ', csize, coord, &written);
				    FillConsoleOutputAttribute(stdo, csbi.wAttributes, csize, coord, &written);
				    SetConsoleCursorPosition(stdo, csbi.dwCursorPosition);
				    break;
				case 'H':
				    GetConsoleScreenBufferInfo(stdo, &csbi);
				    coord = csbi.dwCursorPosition;
				    if(v[0] != -1) {
					    if(v[1] != -1) {
						    coord.Y = csbi.srWindow.Top + v[0] - 1;
						    coord.X = v[1] - 1;
					    }
					    else
						    coord.X = v[0] - 1;
				    }
				    else {
					    coord.X = 0;
					    coord.Y = csbi.srWindow.Top;
				    }
				    if(coord.X < csbi.srWindow.Left)
					    coord.X = csbi.srWindow.Left;
				    else if(coord.X > csbi.srWindow.Right)
					    coord.X = csbi.srWindow.Right;
				    if(coord.Y < csbi.srWindow.Top)
					    coord.Y = csbi.srWindow.Top;
				    else if(coord.Y > csbi.srWindow.Bottom)
					    coord.Y = csbi.srWindow.Bottom;
				    SetConsoleCursorPosition(stdo, coord);
				    break;
				default:
				    break;
			}
		}
		else {
			putchar(*ptr);
			ptr++;
		}
	}
	return ptr - buf;
}

#endif /* _WIN32 */
