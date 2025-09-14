/*
 * xmlcatalog.c : a small utility program to handle XML catalogs
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */
#include <slib-internal.h>
#pragma hdrstop
#ifdef HAVE_LIBREADLINE
	#include <readline/readline.h>
	#ifdef HAVE_LIBHISTORY
		#include <readline/history.h>
	#endif
#endif

#if defined(LIBXML_CATALOG_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
static int shell = 0;
static int sgml = 0;
static int noout = 0;
static int create = 0;
static int add = 0;
static int del = 0;
static int convert = 0;
static int no_super_update = 0;
static int verbose = 0;
static char * filename = NULL;

#ifndef XML_SGML_DEFAULT_CATALOG
	#define XML_SGML_DEFAULT_CATALOG "/etc/sgml/catalog"
#endif
// 
// Shell Interface
// 
/**
 * xmlShellReadline:
 * @prompt:  the prompt value
 *
 * Read a string
 *
 * Returns a pointer to it or NULL on EOF the caller is expected to
 *   free the returned string.
 */
static char * xmlShellReadline(const char * prompt) 
{
#ifdef HAVE_LIBREADLINE
	/* Get a line from the user. */
	char * line_read = readline(prompt);
	/* If the line has any text in it, save it on the history. */
	if(line_read && *line_read)
		add_history(line_read);
	return (line_read);
#else
	char   line_read[501];
	char * ret;
	int    len;
	if(prompt)
		fprintf(stdout, "%s", prompt);
	if(!fgets(line_read, 500, stdin))
		return 0;
	line_read[500] = 0;
	len = sstrlen(line_read);
	ret = static_cast<char *>(SAlloc::M(len+1));
	if(ret) {
		memcpy(ret, line_read, len + 1);
	}
	return ret;
#endif
}

static void usershell() 
{
	char * cmdline = NULL;
	char * cur;
	int    nbargs;
	char   command[100];
	char   arg[400];
	char * argv[20];
	int    i, ret;
	xmlChar * ans;
	while(1) {
		cmdline = xmlShellReadline("> ");
		if(cmdline == NULL)
			return;
		/*
		 * Parse the command itself
		 */
		cur = cmdline;
		nbargs = 0;
		while(oneof2(*cur, ' ', '\t')) 
			cur++;
		i = 0;
		while(!oneof4(*cur, ' ', '\t', '\n', '\r')) {
			if(*cur == 0)
				break;
			command[i++] = *cur++;
		}
		command[i] = 0;
		if(!i) {
			SAlloc::F(cmdline);
			continue;
		}
		/*
		 * Parse the argument string
		 */
		memzero(arg, sizeof(arg));
		while(oneof2(*cur, ' ', '\t'))
			cur++;
		i = 0;
		while((*cur != '\n') && (*cur != '\r') && (*cur != 0)) {
			if(*cur == 0)
				break;
			arg[i++] = *cur++;
		}
		arg[i] = 0;
		/*
		 * Parse the arguments
		 */
		i = 0;
		nbargs = 0;
		cur = arg;
		memzero(argv, sizeof(argv));
		while(*cur) {
			while(oneof2(*cur, ' ', '\t')) 
				cur++;
			if(*cur == '\'') {
				cur++;
				argv[i] = cur;
				while((*cur != 0) && (*cur != '\'')) 
					cur++;
				if(*cur == '\'') {
					*cur = 0;
					nbargs++;
					i++;
					cur++;
				}
			}
			else if(*cur == '"') {
				cur++;
				argv[i] = cur;
				while((*cur != 0) && (*cur != '"')) cur++;
				if(*cur == '"') {
					*cur = 0;
					nbargs++;
					i++;
					cur++;
				}
			}
			else {
				argv[i] = cur;
				while((*cur != 0) && !oneof2(*cur, ' ', '\t'))
					cur++;
				*cur = 0;
				nbargs++;
				i++;
				cur++;
			}
		}
		/*
		 * start interpreting the command
		 */
		if(sstreq(command, "exit") || sstreq(command, "quit") || sstreq(command, "bye")) {
			SAlloc::F(cmdline);
			break;
		}
		if(sstreq(command, "public")) {
			if(nbargs != 1) {
				printf("public requires 1 arguments\n");
			}
			else {
				ans = xmlCatalogResolvePublic((const xmlChar *)argv[0]);
				if(!ans)
					printf("No entry for PUBLIC %s\n", argv[0]);
				else {
					printf("%s\n", PTRCHRC_(ans));
					SAlloc::F(ans);
				}
			}
		}
		else if(sstreq(command, "system")) {
			if(nbargs != 1)
				printf("system requires 1 arguments\n");
			else {
				ans = xmlCatalogResolveSystem((const xmlChar *)argv[0]);
				if(!ans)
					printf("No entry for SYSTEM %s\n", argv[0]);
				else {
					printf("%s\n", PTRCHRC_(ans));
					SAlloc::F(ans);
				}
			}
		}
		else if(sstreq(command, "add")) {
			if(sgml) {
				if((nbargs != 3) && (nbargs != 2)) {
					printf("add requires 2 or 3 arguments\n");
				}
				else {
					if(argv[2] == NULL)
						ret = xmlCatalogAdd(BAD_CAST argv[0], NULL, BAD_CAST argv[1]);
					else
						ret = xmlCatalogAdd(BAD_CAST argv[0], BAD_CAST argv[1], BAD_CAST argv[2]);
					if(ret)
						printf("add command failed\n");
				}
			}
			else {
				if((nbargs != 3) && (nbargs != 2))
					printf("add requires 2 or 3 arguments\n");
				else {
					if(argv[2] == NULL)
						ret = xmlCatalogAdd(BAD_CAST argv[0], NULL, BAD_CAST argv[1]);
					else
						ret = xmlCatalogAdd(BAD_CAST argv[0], BAD_CAST argv[1], BAD_CAST argv[2]);
					if(ret)
						printf("add command failed\n");
				}
			}
		}
		else if(sstreq(command, "del")) {
			if(nbargs != 1)
				printf("del requires 1\n");
			else {
				ret = xmlCatalogRemove(BAD_CAST argv[0]);
				if(ret <= 0)
					printf("del command failed\n");
			}
		}
		else if(sstreq(command, "resolve")) {
			if(nbargs != 2)
				printf("resolve requires 2 arguments\n");
			else {
				ans = xmlCatalogResolve(BAD_CAST argv[0], BAD_CAST argv[1]);
				if(!ans)
					printf("Resolver failed to find an answer\n");
				else {
					printf("%s\n", PTRCHRC_(ans));
					SAlloc::F(ans);
				}
			}
		}
		else if(sstreq(command, "dump")) {
			if(nbargs != 0) {
				printf("dump has no arguments\n");
			}
			else {
				xmlCatalogDump(stdout);
			}
		}
		else if(sstreq(command, "debug")) {
			if(nbargs != 0) {
				printf("debug has no arguments\n");
			}
			else {
				verbose++;
				xmlCatalogSetDebug(verbose);
			}
		}
		else if(sstreq(command, "quiet")) {
			if(nbargs != 0) {
				printf("quiet has no arguments\n");
			}
			else {
				if(verbose > 0)
					verbose--;
				xmlCatalogSetDebug(verbose);
			}
		}
		else {
			if(strcmp(command, "help")) {
				printf("Unrecognized command %s\n", command);
			}
			printf("Commands available:\n");
			printf("\tpublic PublicID: make a PUBLIC identifier lookup\n");
			printf("\tsystem SystemID: make a SYSTEM identifier lookup\n");
			printf("\tresolve PublicID SystemID: do a full resolver lookup\n");
			printf("\tadd 'type' 'orig' 'replace' : add an entry\n");
			printf("\tdel 'values' : remove values\n");
			printf("\tdump: print the current catalog state\n");
			printf("\tdebug: increase the verbosity level\n");
			printf("\tquiet: decrease the verbosity level\n");
			printf("\texit:  quit the shell\n");
		}
		SAlloc::F(cmdline); /* not free here ! */
	}
}
// 
// Main
// 
static void usage(const char * name) 
{
	/* split into 2 printf's to avoid overly long string (gcc warning) */
	printf("\
Usage : %s [options] catalogfile entities...\n\
\tParse the catalog file and query it for the entities\n\
\t--sgml : handle SGML Super catalogs for --add and --del\n\
\t--shell : run a shell allowing interactive queries\n\
\t--create : create a new catalog\n\
\t--add 'type' 'orig' 'replace' : add an XML entry\n\
\t--add 'entry' : add an SGML entry\n"                                                                                                                                                                                                                                                                                                                                         ,
	    name);
	printf("\
\t--del 'values' : remove values\n\
\t--noout: avoid dumping the result on stdout\n\
\t         used with --add or --del, it saves the catalog changes\n\
\t         and with --sgml it automatically updates the super catalog\n\
\t--no-super-update: do not update the SGML super catalog\n\
\t-v --verbose : provide debug informations\n"                                                                                                                                                                                                                                                                                                               );
}

#if 0 // @v12.4.0 {
	int main(int argc, char ** argv) 
	{
		int i;
		int ret;
		int exit_value = 0;
		if(argc <= 1) {
			usage(argv[0]);
			return 1;
		}
		LIBXML_TEST_VERSION
		for(i = 1; i < argc; i++) {
			if(sstreq(argv[i], "-"))
				break;
			if(argv[i][0] != '-')
				break;
			if(sstreq(argv[i], "-verbose") || sstreq(argv[i], "-v") || sstreq(argv[i], "--verbose")) {
				verbose++;
				xmlCatalogSetDebug(verbose);
			}
			else if(sstreq(argv[i], "-noout") || sstreq(argv[i], "--noout")) {
				noout = 1;
			}
			else if(sstreq(argv[i], "-shell") || sstreq(argv[i], "--shell")) {
				shell++;
				noout = 1;
			}
			else if(sstreq(argv[i], "-sgml") || sstreq(argv[i], "--sgml")) {
				sgml++;
			}
			else if(sstreq(argv[i], "-create") || sstreq(argv[i], "--create")) {
				create++;
			}
			else if(sstreq(argv[i], "-convert") || sstreq(argv[i], "--convert")) {
				convert++;
			}
			else if(sstreq(argv[i], "-no-super-update") || sstreq(argv[i], "--no-super-update")) {
				no_super_update++;
			}
			else if(sstreq(argv[i], "-add") || sstreq(argv[i], "--add")) {
				if(sgml)
					i += 2;
				else
					i += 3;
				add++;
			}
			else if(sstreq(argv[i], "-del") || sstreq(argv[i], "--del")) {
				i += 1;
				del++;
			}
			else {
				slfprintf_stderr("Unknown option %s\n", argv[i]);
				usage(argv[0]);
				return 1;
			}
		}
		for(i = 1; i < argc; i++) {
			if(sstreq(argv[i], "-add") || sstreq(argv[i], "--add")) {
				if(sgml)
					i += 2;
				else
					i += 3;
				continue;
			}
			else if(sstreq(argv[i], "-del") || sstreq(argv[i], "--del")) {
				i += 1;
				/* No catalog entry specified */
				if(i == argc || (sgml && i + 1 == argc)) {
					slfprintf_stderr("No catalog entry specified to remove from\n");
					usage(argv[0]);
					return 1;
				}
				continue;
			}
			else if(argv[i][0] == '-')
				continue;
			filename = argv[i];
			ret = xmlLoadCatalog(argv[i]);
			if((ret < 0) && (create)) {
				xmlCatalogAdd(reinterpret_cast<const xmlChar *>("catalog"), BAD_CAST argv[i], 0);
			}
			break;
		}
		if(convert)
			ret = xmlCatalogConvert();
		if((add) || (del)) {
			for(i = 1; i < argc; i++) {
				if(sstreq(argv[i], "-"))
					break;
				if(argv[i][0] != '-')
					continue;
				if(strcmp(argv[i], "-add") && strcmp(argv[i], "--add") && strcmp(argv[i], "-del") && strcmp(argv[i], "--del"))
					continue;
				if(sgml) {
					/*
					 * Maintenance of SGML catalogs.
					 */
					xmlCatalogPtr super = NULL;
					xmlCatalogPtr catal = xmlLoadSGMLSuperCatalog(argv[i+1]);
					if(sstreq(argv[i], "-add") || sstreq(argv[i], "--add")) {
						SETIFZQ(catal, xmlNewCatalog(1));
						xmlACatalogAdd(catal, reinterpret_cast<const xmlChar *>("CATALOG"), BAD_CAST argv[i+2], 0);
						if(!no_super_update) {
							super = xmlLoadSGMLSuperCatalog(XML_SGML_DEFAULT_CATALOG);
							SETIFZ(super, xmlNewCatalog(1));
							xmlACatalogAdd(super, reinterpret_cast<const xmlChar *>("CATALOG"), BAD_CAST argv[i+1], 0);
						}
					}
					else {
						if(catal)
							ret = xmlACatalogRemove(catal, BAD_CAST argv[i+2]);
						else
							ret = -1;
						if(ret < 0) {
							slfprintf_stderr("Failed to remove entry from %s\n", argv[i+1]);
							exit_value = 1;
						}
						if(!no_super_update && noout && catal && (xmlCatalogIsEmpty(catal))) {
							super = xmlLoadSGMLSuperCatalog(XML_SGML_DEFAULT_CATALOG);
							if(super) {
								ret = xmlACatalogRemove(super, BAD_CAST argv[i+1]);
								if(ret < 0) {
									slfprintf_stderr("Failed to remove entry from %s\n", XML_SGML_DEFAULT_CATALOG);
									exit_value = 1;
								}
							}
						}
					}
					if(noout) {
						if(xmlCatalogIsEmpty(catal)) {
							remove(argv[i+1]);
						}
						else {
							FILE * out = fopen(argv[i+1], "w");
							if(!out) {
								slfprintf_stderr("could not open %s for saving\n", argv[i+1]);
								exit_value = 2;
								noout = 0;
							}
							else {
								xmlACatalogDump(catal, out);
								fclose(out);
							}
						}
						if(!no_super_update && super) {
							if(xmlCatalogIsEmpty(super)) {
								remove(XML_SGML_DEFAULT_CATALOG);
							}
							else {
								FILE * out = fopen(XML_SGML_DEFAULT_CATALOG, "w");
								if(!out) {
									slfprintf_stderr("could not open %s for saving\n", XML_SGML_DEFAULT_CATALOG);
									exit_value = 2;
									noout = 0;
								}
								else {
									xmlACatalogDump(super, out);
									fclose(out);
								}
							}
						}
					}
					else {
						xmlACatalogDump(catal, stdout);
					}
					i += 2;
				}
				else {
					if(sstreq(argv[i], "-add") || sstreq(argv[i], "--add")) {
						if((argv[i + 3] == NULL) || (argv[i + 3][0] == 0))
							ret = xmlCatalogAdd(BAD_CAST argv[i+1], NULL, BAD_CAST argv[i+2]);
						else
							ret = xmlCatalogAdd(BAD_CAST argv[i+1], BAD_CAST argv[i+2], BAD_CAST argv[i + 3]);
						if(ret) {
							printf("add command failed\n");
							exit_value = 3;
						}
						i += 3;
					}
					else if(sstreq(argv[i], "-del") || sstreq(argv[i], "--del")) {
						ret = xmlCatalogRemove(BAD_CAST argv[i+1]);
						if(ret < 0) {
							slfprintf_stderr("Failed to remove entry %s\n", argv[i+1]);
							exit_value = 1;
						}
						i += 1;
					}
				}
			}
		}
		else if(shell) {
			usershell();
		}
		else {
			for(i++; i < argc; i++) {
				xmlChar * ans;
				xmlURI * uri = xmlParseURI(argv[i]);
				if(!uri) {
					ans = xmlCatalogResolvePublic((const xmlChar *)argv[i]);
					if(ans == NULL) {
						printf("No entry for PUBLIC %s\n", argv[i]);
						exit_value = 4;
					}
					else {
						printf("%s\n", PTRCHRC_(ans));
						SAlloc::F(ans);
					}
				}
				else {
					xmlFreeURI(uri);
					ans = xmlCatalogResolveSystem((const xmlChar *)argv[i]);
					if(ans == NULL) {
						printf("No entry for SYSTEM %s\n", argv[i]);
						ans = xmlCatalogResolveURI((const xmlChar *)argv[i]);
						if(ans == NULL) {
							printf("No entry for URI %s\n", argv[i]);
							exit_value = 4;
						}
						else {
							printf("%s\n", PTRCHRC_(ans));
							SAlloc::F(ans);
						}
					}
					else {
						printf("%s\n", PTRCHRC_(ans));
						SAlloc::F(ans);
					}
				}
			}
		}
		if((!sgml) && ((add) || (del) || (create) || (convert))) {
			if(noout && filename && *filename) {
				FILE * out = fopen(filename, "w");
				if(!out) {
					slfprintf_stderr("could not open %s for saving\n", filename);
					exit_value = 2;
					noout = 0;
				}
				else
					xmlCatalogDump(out);
			}
			else
				xmlCatalogDump(stdout);
		}
		/*
		 * Cleanup and check for memory leaks
		 */
		xmlCleanupParser();
		xmlMemoryDump();
		return (exit_value);
	}
	#endif // } @v12.4.0
#else
int main(int argc ATTRIBUTE_UNUSED, char ** argv ATTRIBUTE_UNUSED) 
{
	slfprintf_stderr("libxml was not compiled with catalog and output support\n");
	return 1;
}
#endif
