// DRIVER.C
//
#include <slib.h>
#include "header.h"
#pragma hdrstop

// @sobolev // {
#define DISABLE_JAVA
#define DISABLE_PASCAL
#define DISABLE_PYTHON
#define DISABLE_JS
#define DISABLE_CSHARP
#define DISABLE_RUST
#define DISABLE_GO
// } @sobolev 

#define DEFAULT_JAVA_PACKAGE "org.tartarus.snowball.ext"
#define DEFAULT_JAVA_BASE_CLASS "org.tartarus.snowball.SnowballProgram"
#define DEFAULT_JAVA_AMONG_CLASS "org.tartarus.snowball.Among"
#define DEFAULT_JAVA_STRING_CLASS "java.lang.StringBuilder"
#define DEFAULT_GO_PACKAGE "snowball"
#define DEFAULT_GO_SNOWBALL_RUNTIME "github.com/snowballstem/snowball/go"
#define DEFAULT_CS_NAMESPACE "Snowball"
#define DEFAULT_CS_BASE_CLASS "Stemmer"
#define DEFAULT_CS_AMONG_CLASS "Among"
#define DEFAULT_CS_STRING_CLASS "StringBuilder"
#define DEFAULT_JS_BASE_CLASS "BaseStemmer"
#define DEFAULT_PYTHON_BASE_CLASS "BaseStemmer"

//static int eq__Removed(const char * s1, const char * s2) { return strcmp(s1, s2) == 0; }

static void print_arglist(int exit_code) 
{
	FILE * f = exit_code ? stderr : stdout;
	fprintf(f, "Usage: snowball SOURCE_FILE... [OPTIONS]\n\n"
	    "Supported options:\n"
	    "  -o[utput] file\n"
	    "  -s[yntax]\n"
	    "  -comments\n"
#ifndef DISABLE_JAVA
	    "  -j[ava]\n"
#endif
#ifndef DISABLE_CSHARP
	    "  -cs[harp]\n"
#endif
	    "  -c++\n"
#ifndef DISABLE_PASCAL
	    "  -pascal\n"
#endif
#ifndef DISABLE_PYTHON
	    "  -py[thon]\n"
#endif
#ifndef DISABLE_JS
	    "  -js\n"
#endif
#ifndef DISABLE_RUST
	    "  -rust\n"
#endif
#ifndef DISABLE_GO
	    "  -go\n"
#endif
	    "  -w[idechars]\n"
	    "  -u[tf8]\n"
	    "  -n[ame] class name\n"
	    "  -ep[refix] string\n"
	    "  -vp[refix] string\n"
	    "  -i[nclude] directory\n"
	    "  -r[untime] path to runtime headers\n"
	    "  -p[arentclassname] fully qualified parent class name\n"
#if !defined(DISABLE_JAVA) || !defined(DISABLE_CSHARP)
	    "  -P[ackage] package name for stemmers\n"
	    "  -S[tringclass] StringBuffer-compatible class\n"
	    "  -a[mongclass] fully qualified name of the Among class\n"
#endif
#ifndef DISABLE_GO
	    "  -gop[ackage] Go package name for stemmers\n"
	    "  -gor[untime] Go snowball runtime package\n"
#endif
	    "  --help        display this help and exit\n"
	    "  --version     output version information and exit\n"
	    );
	exit(exit_code);
}

static void check_lim(int i, int argc) 
{
	if(i >= argc) {
		slfprintf_stderr("argument list is one short\n");
		print_arglist(1);
	}
}

static FILE * get_output(symbol * b) 
{
	char * s = b_to_s(b);
	FILE * output = fopen(s, "w");
	if(output == 0) {
		slfprintf_stderr("Can't open output %s\n", s);
		exit(1);
	}
	SAlloc::F(s);
	return output;
}

static int read_options(Options * o, int argc, char * argv[]) 
{
	char * s;
	int i = 1;
	int new_argc = 1;
	// Note down the last option used to specify an explicit encoding so
	// we can warn we ignored it for languages with a fixed encoding.
	const char * encoding_opt = NULL;
	// set defaults: 
	o->output_file = 0;
	o->syntax_tree = false;
	o->comments = false;
	o->externals_prefix = NULL;
	o->variables_prefix = 0;
	o->runtime_path = 0;
	o->parent_class_name = NULL;
	o->string_class = NULL;
	o->among_class = NULL;
	o->package = NULL;
	o->go_snowball_runtime = DEFAULT_GO_SNOWBALL_RUNTIME;
	o->name = NULL;
	o->make_lang = Options::LANG_C;
	o->includes = 0;
	o->includes_end = 0;
	o->encoding = ENC_SINGLEBYTE;
	/* read options: */
	while(i < argc) {
		s = argv[i++];
		if(s[0] != '-') {
			/* Non-option argument - shuffle down. */
			argv[new_argc++] = s;
			continue;
		}
		{
			if(sstreq(s, "-o") || sstreq(s, "-output")) {
				check_lim(i, argc);
				o->output_file = argv[i++];
				continue;
			}
			if(sstreq(s, "-n") || sstreq(s, "-name")) {
				check_lim(i, argc);
				o->name = argv[i++];
				continue;
			}
#ifndef DISABLE_JS
			if(sstreq(s, "-js")) {
				o->make_lang = Options::LANG_JAVASCRIPT;
				continue;
			}
#endif
#ifndef DISABLE_RUST
			if(sstreq(s, "-rust")) {
				o->make_lang = Options::LANG_RUST;
				continue;
			}
#endif
#ifndef DISABLE_GO
			if(sstreq(s, "-go")) {
				o->make_lang = Options::LANG_GO;
				continue;
			}
#endif
#ifndef DISABLE_JAVA
			if(sstreq(s, "-j") || sstreq(s, "-java")) {
				o->make_lang = Options::LANG_JAVA;
				continue;
			}
#endif
#ifndef DISABLE_CSHARP
			if(sstreq(s, "-cs") || sstreq(s, "-csharp")) {
				o->make_lang = Options::LANG_CSHARP;
				continue;
			}
#endif
			if(sstreq(s, "-c++")) {
				o->make_lang = Options::LANG_CPLUSPLUS;
				continue;
			}
#ifndef DISABLE_PASCAL
			if(sstreq(s, "-pascal")) {
				o->make_lang = Options::LANG_PASCAL;
				continue;
			}
#endif
#ifndef DISABLE_PYTHON
			if(sstreq(s, "-py") || sstreq(s, "-python")) {
				o->make_lang = Options::LANG_PYTHON;
				continue;
			}
#endif
			if(sstreq(s, "-w") || sstreq(s, "-widechars")) {
				encoding_opt = s;
				o->encoding = ENC_WIDECHARS;
				continue;
			}
			if(sstreq(s, "-s") || sstreq(s, "-syntax")) {
				o->syntax_tree = true;
				continue;
			}
			if(sstreq(s, "-comments")) {
				o->comments = true;
				continue;
			}
			if(sstreq(s, "-ep") || sstreq(s, "-eprefix")) {
				check_lim(i, argc);
				o->externals_prefix = argv[i++];
				continue;
			}
			if(sstreq(s, "-vp") || sstreq(s, "-vprefix")) {
				check_lim(i, argc);
				o->variables_prefix = argv[i++];
				continue;
			}
			if(sstreq(s, "-i") || sstreq(s, "-include")) {
				check_lim(i, argc);
				{
					Include * p = static_cast<Include *>(malloc(sizeof(Include)));
					symbol * b = add_s_to_b(0, argv[i++]);
					b = add_s_to_b(b, "/");
					p->next = 0; p->b = b;
					if(o->includes == 0) 
						o->includes = p; 
					else
						o->includes_end->next = p;
					o->includes_end = p;
				}
				continue;
			}
			if(sstreq(s, "-r") || sstreq(s, "-runtime")) {
				check_lim(i, argc);
				o->runtime_path = argv[i++];
				continue;
			}
			if(sstreq(s, "-u") || sstreq(s, "-utf8")) {
				encoding_opt = s;
				o->encoding = ENC_UTF8;
				continue;
			}
			if(sstreq(s, "-p") || sstreq(s, "-parentclassname")) {
				check_lim(i, argc);
				o->parent_class_name = argv[i++];
				continue;
			}
#if !defined(DISABLE_JAVA) || !defined(DISABLE_CSHARP)
			if(sstreq(s, "-P") || sstreq(s, "-Package")) {
				check_lim(i, argc);
				o->package = argv[i++];
				continue;
			}
			if(sstreq(s, "-S") || sstreq(s, "-stringclass")) {
				check_lim(i, argc);
				o->string_class = argv[i++];
				continue;
			}
			if(sstreq(s, "-a") || sstreq(s, "-amongclass")) {
				check_lim(i, argc);
				o->among_class = argv[i++];
				continue;
			}
#endif
#ifndef DISABLE_GO
			if(sstreq(s, "-gop") || sstreq(s, "-gopackage")) {
				check_lim(i, argc);
				o->package = argv[i++];
				continue;
			}
			if(sstreq(s, "-gor") || sstreq(s, "-goruntime")) {
				check_lim(i, argc);
				o->go_snowball_runtime = argv[i++];
				continue;
			}
#endif
			if(sstreq(s, "--help")) {
				print_arglist(0);
			}
			if(sstreq(s, "--version")) {
				printf("Snowball compiler version " SNOWBALL_VERSION "\n");
				exit(0);
			}
			slfprintf_stderr("'%s' misplaced\n", s);
			print_arglist(1);
		}
	}
	if(new_argc == 1) {
		slfprintf_stderr("no source files specified\n");
		print_arglist(1);
	}
	argv[new_argc] = NULL;
	// Set language-dependent defaults
	switch(o->make_lang) {
		case Options::LANG_C:
		case Options::LANG_CPLUSPLUS:
		    encoding_opt = NULL;
		    break;
		case Options::LANG_CSHARP:
		    o->encoding = ENC_WIDECHARS;
			SETIFZ(o->parent_class_name, DEFAULT_CS_BASE_CLASS);
			SETIFZ(o->string_class, DEFAULT_CS_STRING_CLASS);
			SETIFZ(o->among_class, DEFAULT_CS_AMONG_CLASS);
			SETIFZ(o->package, DEFAULT_CS_NAMESPACE);
		    break;
		case Options::LANG_GO:
		    o->encoding = ENC_UTF8;
			SETIFZ(o->package, DEFAULT_GO_PACKAGE);
		    break;
		case Options::LANG_JAVA:
		    o->encoding = ENC_WIDECHARS;
			SETIFZ(o->parent_class_name, DEFAULT_JAVA_BASE_CLASS);
			SETIFZ(o->string_class, DEFAULT_JAVA_STRING_CLASS);
			SETIFZ(o->among_class, DEFAULT_JAVA_AMONG_CLASS);
			SETIFZ(o->package, DEFAULT_JAVA_PACKAGE);
		    break;
		case Options::LANG_JAVASCRIPT:
		    o->encoding = ENC_WIDECHARS;
			SETIFZ(o->parent_class_name, DEFAULT_JS_BASE_CLASS);
		    break;
		case Options::LANG_PYTHON:
		    o->encoding = ENC_WIDECHARS;
			SETIFZ(o->parent_class_name, DEFAULT_PYTHON_BASE_CLASS);
		    break;
		case Options::LANG_RUST:
		    o->encoding = ENC_UTF8;
		    break;
		default:
		    break;
	}
	if(encoding_opt) {
		slfprintf_stderr("warning: %s only meaningful for C and C++\n", encoding_opt);
	}
	if(o->make_lang != Options::LANG_C && o->make_lang != Options::LANG_CPLUSPLUS) {
		if(o->runtime_path) {
			slfprintf_stderr("warning: -r/-runtime only meaningful for C and C++\n");
		}
		if(o->externals_prefix) {
			slfprintf_stderr("warning: -ep/-eprefix only meaningful for C and C++\n");
		}
	}
	SETIFZ(o->externals_prefix, "");
	if(!o->name && o->output_file) {
		// Default class name to basename of output_file - this is the standard convention for at least Java and C#.
		const char * slash = strrchr(o->output_file, '/');
		size_t len;
		const char * leaf = (slash == NULL) ? o->output_file : slash + 1;
		slash = strrchr(leaf, '\\');
		if(slash != NULL) 
			leaf = slash + 1;
		{
			const char * dot = strchr(leaf, '.');
			len = (dot == NULL) ? strlen(leaf) : (size_t)(dot - leaf);
		}
		{
			char * new_name = (char *)SAlloc::M(len + 1);
			switch(o->make_lang) {
				case Options::LANG_CSHARP:
				case Options::LANG_PASCAL:
				    // Upper case initial letter
				    memcpy(new_name, leaf, len);
				    new_name[0] = toupper(new_name[0]);
				    break;
				case Options::LANG_JAVASCRIPT:
				case Options::LANG_PYTHON: {
				    // Upper case initial letter and change each
				    // underscore+letter or hyphen+letter to an upper case letter.
				    size_t j = 0;
				    int uc_next = true;
				    for(size_t i = 0; i != len; ++i) {
					    uchar ch = leaf[i];
					    if(ch == '_' || ch == '-') {
						    uc_next = true;
					    }
					    else {
						    if(uc_next) {
							    new_name[j] = toupper(ch);
							    uc_next = false;
						    }
						    else
							    new_name[j] = ch;
						    ++j;
					    }
				    }
				    len = j;
				    break;
			    }
				default: // Just copy
				    memcpy(new_name, leaf, len);
				    break;
			}
			new_name[len] = '\0';
			o->name = new_name;
		}
	}
	return new_argc;
}

static int ProcessFile(const char * pFileName, const Options * pOpts)
{
	int    ok = 1;
	//char * file = argv[1];
	symbol * u = get_input(pFileName);
	if(u == 0) {
		slfprintf_stderr("Can't open input %s\n", pFileName);
		ok = 0;
	}
	else {
		Tokeniser * t = create_tokeniser(u, pFileName);
		Analyser * a = t->CreateAnalyser();
		Input ** next_input_ptr = &(t->P_Next);
		a->Encoding = t->Encoding = pOpts->encoding;
		t->P_Includes = pOpts->includes;
		// If multiple source files are specified, set up the others to be
		// read after the first in order, using the same mechanism as 'get' uses.
		/*for(int i = 2; i != argc; ++i) {
			NEW(input, q);
			pFileName = argv[i]; // !
			u = get_input(pFileName);
			if(u == 0) {
				slfprintf_stderr("Can't open input %s\n", pFileName);
				exit(1);
			}
			q->p = u;
			q->c = 0;
			q->file = pFileName;
			q->file_needs_freeing = false;
			q->line_number = 1;
			*next_input_ptr = q;
			next_input_ptr = &(q->next);
		}*/
		*next_input_ptr = NULL;
		read_program(a);
		if(t->ErrCount > 0) {
			ok = 0;
			//exit(1);
		}
		else {
			if(pOpts->syntax_tree) 
				print_program(a);
			close_tokeniser(t);
			if(!pOpts->syntax_tree) {
				Generator * g;
				const char * s = pOpts->output_file;
				if(!s) {
					slfprintf_stderr("Please include the -o option\n");
					print_arglist(1);
				}
				g = create_generator(a, pOpts);
				if(oneof2(pOpts->make_lang, Options::LANG_C, Options::LANG_CPLUSPLUS)) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".h");
					pOpts->output_h = get_output(b);
					b[SIZE(b) - 1] = 'c';
					if(pOpts->make_lang == Options::LANG_CPLUSPLUS) {
						b = add_s_to_b(b, "c");
					}
					pOpts->output_src = get_output(b);
					lose_b(b);
					generate_program_c(g);
					fclose(pOpts->output_src);
					fclose(pOpts->output_h);
				}
	#ifndef DISABLE_JAVA
				if(pOpts->make_lang == Options::LANG_JAVA) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".java");
					pOpts->output_src = get_output(b);
					lose_b(b);
					generate_program_java(g);
					fclose(pOpts->output_src);
				}
	#endif
	#ifndef DISABLE_PASCAL
				if(pOpts->make_lang == Options::LANG_PASCAL) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".pas");
					pOpts->output_src = get_output(b);
					lose_b(b);
					generate_program_pascal(g);
					fclose(pOpts->output_src);
				}
	#endif
	#ifndef DISABLE_PYTHON
				if(pOpts->make_lang == Options::LANG_PYTHON) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".py");
					pOpts->output_src = get_output(b);
					lose_b(b);
					generate_program_python(g);
					fclose(pOpts->output_src);
				}
	#endif
	#ifndef DISABLE_JS
				if(pOpts->make_lang == Options::LANG_JAVASCRIPT) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".js");
					pOpts->output_src = get_output(b);
					lose_b(b);
					generate_program_js(g);
					fclose(pOpts->output_src);
				}
	#endif
	#ifndef DISABLE_CSHARP
				if(pOpts->make_lang == Options::LANG_CSHARP) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".cs");
					pOpts->output_src = get_output(b);
					lose_b(b);
					generate_program_csharp(g);
					fclose(pOpts->output_src);
				}
	#endif
	#ifndef DISABLE_RUST
				if(pOpts->make_lang == Options::LANG_RUST) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".rs");
					pOpts->output_src = get_output(b);
					lose_b(b);
					generate_program_rust(g);
					fclose(pOpts->output_src);
				}
	#endif
	#ifndef DISABLE_GO
				if(pOpts->make_lang == Options::LANG_GO) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".go");
					pOpts->output_src = get_output(b);
					lose_b(b);
					generate_program_go(g);
					fclose(pOpts->output_src);
				}
	#endif
				close_generator(g);
			}
			close_analyser(a);
		}
	}
	lose_b(u);
	return ok;
}

extern int main(int argc, char * argv[]) 
{
	// \papyrus\tools\xapian-snowball.exe %(FullPath) -c++ -u -n InternalStem -p SnowballStemImplementation -o %(Filename).sbl
	SLS.Init("xapian-lang-compiler", 0);
	Options * o = static_cast<Options *>(malloc(sizeof(Options)));
	argc = read_options(o, argc, argv);
	if(argc > 1 && argv[1]) {
		uint fcount = 0;
		const SString original_cls_name(o->name);
		SString one_cls_name;
		SString output_file_name;
		SString base_dir;
		SString file_name_to_process;
		SString lang_buf;
		SString temp_buf;
		StringSet ss_filename;
		(temp_buf = argv[1]).Strip();
		if(fileExists(temp_buf)) {
			SPathStruc ps(temp_buf);
			ps.Merge(SPathStruc::fDrv|SPathStruc::fDir, base_dir);
			base_dir.SetLastSlash();
			file_name_to_process = temp_buf;
			{
				SPathStruc ps2(file_name_to_process);
				lang_buf = ps2.Nam;
			}
			output_file_name = file_name_to_process;
			o->output_file = output_file_name.cptr();
			if(original_cls_name.NotEmpty()) {
				(one_cls_name = original_cls_name).Cat((temp_buf = lang_buf).SetCase(CCAS_CAPITAL));
				o->name = one_cls_name.cptr();
			}
			if(ProcessFile(file_name_to_process, o)) {
				if(lang_buf.NotEmpty())
					ss_filename.add(lang_buf);
				fcount++;
			}
			else {
				exit(1);
			}
		}
		else if(IsWild(temp_buf)) {
			SDirEntry de;
			SPathStruc ps(temp_buf);
			ps.Merge(SPathStruc::fDrv|SPathStruc::fDir, base_dir);
			base_dir.SetLastSlash();
			for(SDirec dir(temp_buf); dir.Next(&de) > 0;) {
				if(!de.IsFolder()) {
					(file_name_to_process = base_dir).Cat(de.FileName);
					if(fileExists(file_name_to_process)) {
						{
							SPathStruc ps2(file_name_to_process);
							lang_buf = ps2.Nam;
						}
						output_file_name = file_name_to_process;
						o->output_file = output_file_name.cptr();
						if(original_cls_name.NotEmpty()) {
							(one_cls_name = original_cls_name).Cat((temp_buf = lang_buf).SetCase(CCAS_CAPITAL));
							o->name = one_cls_name.cptr();
						}
						if(ProcessFile(file_name_to_process, o)) {
							if(lang_buf.NotEmpty())
								ss_filename.add(lang_buf);
							fcount++;
						}
						else {
							exit(1);
						}
					}
				}
			}
		}
		if(fcount) {
			// allsnowballheaders.h
			(temp_buf = base_dir).Cat("allsnowballheaders.h");
			//SFile f_allh(temp_buf, SFile::mWrite);
			Generator_CPP gen_cpp(temp_buf);
			const SString one_incl_protector("__ALLSNOWBALLHEADERS_H");
			SString lang_string;
			SString item_buf;

			gen_cpp.Wr_Comment((temp_buf = "allsnowballheaders.h").ToUpper());
			gen_cpp.Wr_Comment(0);
			gen_cpp.Wr_IfDef(one_incl_protector, 1);
			gen_cpp.Wr_Define(one_incl_protector, 0);
			{
				for(uint ssp = 0; ss_filename.get(&ssp, item_buf);) {
					lang_string.CatDivIfNotEmpty(' ', 0).Cat((temp_buf = item_buf).ToLower());
					(temp_buf = item_buf).ToLower().Dot().Cat("sbl").Dot().Cat("h");
					gen_cpp.Wr_Include(temp_buf, 0);
				}
				gen_cpp.WriteBlancLine();
				gen_cpp.Wr_Define("LANGSTRING", lang_string);
				gen_cpp.WriteBlancLine();
			}
			{
				gen_cpp.Wr_StartClassDecl(Generator_CPP::clsEnum, "SnowballLang", 0, 0, 0);
				gen_cpp.IndentInc();
				gen_cpp.Wr_Indent();
				//gen_cpp.WriteLine((temp_buf = "NONE").CatChar(',').CR());
				long enum_id = 1;
				for(uint ssp = 0; ss_filename.get(&ssp, item_buf);) {
					gen_cpp.Wr_Indent();
					gen_cpp.WriteLine((temp_buf = item_buf).ToUpper().Space().CatChar('=').Space().Cat(enum_id++).CatChar(',').CR());
				}
				gen_cpp.IndentDec();
				gen_cpp.Wr_CloseBrace(1);
			}
			gen_cpp.Wr_EndIf(one_incl_protector);
		}
	}
#if 0 // {
	{
		int i;
		char * file = argv[1];
		symbol * u = get_input(file);
		if(u == 0) {
			slfprintf_stderr("Can't open input %s\n", file);
			exit(1);
		}
		{
			Tokeniser * t = create_tokeniser(u, file);
			Analyser * a = t->CreateAnalyser();
			Input ** next_input_ptr = &(t->P_Next);
			a->Encoding = t->Encoding = o->encoding;
			t->P_Includes = o->includes;
			// If multiple source files are specified, set up the others to be
			// read after the first in order, using the same mechanism as 'get' uses.
			for(i = 2; i != argc; ++i) {
				NEW(Input, q);
				file = argv[i];
				u = get_input(file);
				if(u == 0) {
					slfprintf_stderr("Can't open input %s\n", file);
					exit(1);
				}
				q->P_Symb = u;
				q->C = 0;
				q->P_FileName = file;
				q->FileNeedsFreeing = false;
				q->LineNumber = 1;
				*next_input_ptr = q;
				next_input_ptr = &(q->P_Next);
			}
			*next_input_ptr = NULL;
			read_program(a);
			if(t->ErrCount > 0) 
				exit(1);
			if(o->syntax_tree) 
				print_program(a);
			close_tokeniser(t);
			if(!o->syntax_tree) {
				Generator * g;
				const char * s = o->output_file;
				if(!s) {
					slfprintf_stderr("Please include the -o option\n");
					print_arglist(1);
				}
				g = create_generator(a, o);
				if(oneof2(o->make_lang, Options::LANG_C, Options::LANG_CPLUSPLUS)) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".h");
					o->output_h = get_output(b);
					b[SIZE(b) - 1] = 'c';
					if(o->make_lang == Options::LANG_CPLUSPLUS) {
						b = add_s_to_b(b, "c");
					}
					o->output_src = get_output(b);
					lose_b(b);
					generate_program_c(g);
					fclose(o->output_src);
					fclose(o->output_h);
				}
#ifndef DISABLE_JAVA
				if(o->make_lang == Options::LANG_JAVA) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".java");
					o->output_src = get_output(b);
					lose_b(b);
					generate_program_java(g);
					fclose(o->output_src);
				}
#endif
#ifndef DISABLE_PASCAL
				if(o->make_lang == Options::LANG_PASCAL) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".pas");
					o->output_src = get_output(b);
					lose_b(b);
					generate_program_pascal(g);
					fclose(o->output_src);
				}
#endif
#ifndef DISABLE_PYTHON
				if(o->make_lang == Options::LANG_PYTHON) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".py");
					o->output_src = get_output(b);
					lose_b(b);
					generate_program_python(g);
					fclose(o->output_src);
				}
#endif
#ifndef DISABLE_JS
				if(o->make_lang == Options::LANG_JAVASCRIPT) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".js");
					o->output_src = get_output(b);
					lose_b(b);
					generate_program_js(g);
					fclose(o->output_src);
				}
#endif
#ifndef DISABLE_CSHARP
				if(o->make_lang == Options::LANG_CSHARP) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".cs");
					o->output_src = get_output(b);
					lose_b(b);
					generate_program_csharp(g);
					fclose(o->output_src);
				}
#endif
#ifndef DISABLE_RUST
				if(o->make_lang == Options::LANG_RUST) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".rs");
					o->output_src = get_output(b);
					lose_b(b);
					generate_program_rust(g);
					fclose(o->output_src);
				}
#endif
#ifndef DISABLE_GO
				if(o->make_lang == Options::LANG_GO) {
					symbol * b = add_s_to_b(0, s);
					b = add_s_to_b(b, ".go");
					o->output_src = get_output(b);
					lose_b(b);
					generate_program_go(g);
					fclose(o->output_src);
				}
#endif
				close_generator(g);
			}
			close_analyser(a);
		}
		lose_b(u);
	}
#endif // } 0
	{   
		Include * p = o->includes;
	    while(p) {
		    Include * q = p->next;
		    lose_b(p->b); 
			FREE(p); 
			p = q;
	    }
	}
	FREE(o);
	if(space_count) 
		slfprintf_stderr("%d blocks unfreed\n", space_count);
	return 0;
}
