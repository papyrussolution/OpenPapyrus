/*
 * testSAX.c : a small tester program for parsing using the SAX API.
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */
#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop
#ifdef LIBXML_READER_ENABLED
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <libxml/xmlreader.h>

static int debug = 0;
static int dump = 0;
static int noent = 0;
static int count = 0;
static int valid = 0;
static int consumed = 0;

static void usage(const char * progname) {
	printf("Usage : %s [options] XMLfiles ...\n", progname);
	printf("\tParse the XML files using the xmlTextReader API\n");
	printf("\t --count: count the number of attribute and elements\n");
	printf("\t --valid: validate the document\n");
	printf("\t --consumed: count the number of bytes consumed\n");
	exit(1);
}

static int elem, attrs;

static void processNode(xmlTextReader * reader) 
{
	int type = xmlTextReaderNodeType(reader);
	if(count) {
		if(type == 1) {
			elem++;
			attrs += xmlTextReaderAttributeCount(reader);
		}
	}
}

static void handleFile(const char * filename) 
{
	xmlTextReader * reader;
	int ret;
	if(count) {
		elem = 0;
		attrs = 0;
	}
	reader = xmlNewTextReaderFilename(filename);
	if(reader != NULL) {
		if(valid)
			xmlTextReaderSetParserProp(reader, XML_PARSER_VALIDATE, 1);

		/*
		 * Process all nodes in sequence
		 */
		ret = xmlTextReaderRead(reader);
		while(ret == 1) {
			processNode(reader);
			ret = xmlTextReaderRead(reader);
		}

		/*
		 * Done, cleanup and status
		 */
		if(consumed)
			printf("%ld bytes consumed by parser\n", xmlTextReaderByteConsumed(reader));
		xmlFreeTextReader(reader);
		if(ret != 0) {
			printf("%s : failed to parse\n", filename);
		}
		else if(count)
			printf("%s : %d elements, %d attributes\n", filename, elem, attrs);
	}
	else {
		fprintf(stderr, "Unable to open %s\n", filename);
	}
}

int main(int argc, char ** argv) {
	int i;
	int files = 0;

	if(argc <= 1) {
		usage(argv[0]);
		return(1);
	}
	LIBXML_TEST_VERSION
	for(i = 1; i < argc; i++) {
		if((sstreq(argv[i], "-debug")) || (sstreq(argv[i], "--debug")))
			debug++;
		else if((sstreq(argv[i], "-dump")) || (sstreq(argv[i], "--dump")))
			dump++;
		else if((sstreq(argv[i], "-count")) || (sstreq(argv[i], "--count")))
			count++;
		else if((sstreq(argv[i], "-consumed")) || (sstreq(argv[i], "--consumed")))
			consumed++;
		else if((sstreq(argv[i], "-valid")) || (sstreq(argv[i], "--valid")))
			valid++;
		else if((sstreq(argv[i], "-noent")) || (sstreq(argv[i], "--noent")))
			noent++;
	}
	if(noent != 0) xmlSubstituteEntitiesDefault(1);
	for(i = 1; i < argc; i++) {
		if(argv[i][0] != '-') {
			handleFile(argv[i]);
			files++;
		}
	}
	xmlCleanupParser();
	xmlMemoryDump();

	return 0;
}

#else
int main(int argc ATTRIBUTE_UNUSED, char ** argv ATTRIBUTE_UNUSED) {
	printf("%s : xmlReader parser support not compiled in\n", argv[0]);
	return 0;
}

#endif /* LIBXML_READER_ENABLED */
