/**
 * catalog.c: set of generic Catalog related routines
 *
 * Reference:  SGML Open Technical Resolution TR9401:1997.
 *        http://www.jclark.com/sp/catalog.htm
 *
 *        XML Catalogs Working Draft 06 August 2001
 *        http://www.oasis-open.org/committees/entity/spec-2001-08-06.html
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */
#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop
//#include <tchar.h> // @sobolev
#ifdef LIBXML_CATALOG_ENABLED
	//#ifdef HAVE_SYS_TYPES_H
		//#include <sys/types.h>
	//#endif
	//#ifdef HAVE_SYS_STAT_H
		//#include <sys/stat.h>
	//#endif
	//#ifdef HAVE_UNISTD_H
		//#include <unistd.h>
	//#endif
	//#ifdef HAVE_FCNTL_H
		//#include <fcntl.h>
	//#endif
	
	#define MAX_DELEGATE    50
	#define MAX_CATAL_DEPTH 50
	#ifdef _WIN32
		#define PATH_SEAPARATOR ';'
	#else
		#define PATH_SEAPARATOR ':'
	#endif

/**
 * @todo 
 *
 * macro to flag unimplemented blocks
 * XML_CATALOG_PREFER user env to select between system/public prefered
 * option. C.f. Richard Tobin <richard@cogsci.ed.ac.uk>
 **> Just FYI, I am using an environment variable XML_CATALOG_PREFER with
 **> values "system" and "public".  I have made the default be "system" to
 **> match yours.
 */
	#define TODO xmlGenericError(0, "Unimplemented block at %s:%d\n", __FILE__, __LINE__);

	#define XML_URN_PUBID "urn:publicid:"
	#define XML_CATAL_BREAK ((xmlChar *)-1)
	#ifndef XML_XML_DEFAULT_CATALOG
		#define XML_XML_DEFAULT_CATALOG "file:///etc/xml/catalog"
	#endif
	#ifndef XML_SGML_DEFAULT_CATALOG
		#define XML_SGML_DEFAULT_CATALOG "file:///etc/sgml/catalog"
	#endif
	#if defined(_WIN32) && defined(_MSC_VER)
		#undef XML_XML_DEFAULT_CATALOG
		static char XML_XML_DEFAULT_CATALOG[256] = "file:///etc/xml/catalog";
		#if defined(_WIN32_WCE)
/* Windows CE don't have a A variant */
		#define GetModuleHandleA GetModuleHandle
		#define GetModuleFileNameA GetModuleFileName
	#else
		#if !defined(_WINDOWS_)
			void * __stdcall GetModuleHandleA(const char *);
			unsigned long __stdcall GetModuleFileNameA(void*, char*, unsigned long);
		#endif
	#endif
#endif

static xmlChar * xmlCatalogNormalizePublic(const xmlChar * pubID);
static int xmlExpandCatalog(xmlCatalogPtr catal, const char * filename);
//
// Types, all private
//
typedef enum {
	XML_CATA_REMOVED = -1,
	XML_CATA_NONE = 0,
	XML_CATA_CATALOG,
	XML_CATA_BROKEN_CATALOG,
	XML_CATA_NEXT_CATALOG,
	XML_CATA_GROUP,
	XML_CATA_PUBLIC,
	XML_CATA_SYSTEM,
	XML_CATA_REWRITE_SYSTEM,
	XML_CATA_DELEGATE_PUBLIC,
	XML_CATA_DELEGATE_SYSTEM,
	XML_CATA_URI,
	XML_CATA_REWRITE_URI,
	XML_CATA_DELEGATE_URI,
	SGML_CATA_SYSTEM,
	SGML_CATA_PUBLIC,
	SGML_CATA_ENTITY,
	SGML_CATA_PENTITY,
	SGML_CATA_DOCTYPE,
	SGML_CATA_LINKTYPE,
	SGML_CATA_NOTATION,
	SGML_CATA_DELEGATE,
	SGML_CATA_BASE,
	SGML_CATA_CATALOG,
	SGML_CATA_DOCUMENT,
	SGML_CATA_SGMLDECL
} xmlCatalogEntryType;

//typedef struct _xmlCatalogEntry xmlCatalogEntry;
struct xmlCatalogEntry {
	xmlCatalogEntry * next;
	xmlCatalogEntry * parent;
	xmlCatalogEntry * children;
	xmlCatalogEntryType type;
	xmlChar * name;
	xmlChar * value;
	xmlChar * URL; /* The expanded URL using the base */
	xmlCatalogPrefer prefer;
	int dealloc;
	int depth;
	xmlCatalogEntry * group;
};

typedef xmlCatalogEntry * xmlCatalogEntryPtr;

typedef enum {
	XML_XML_CATALOG_TYPE = 1,
	XML_SGML_CATALOG_TYPE
} xmlCatalogType;

#define XML_MAX_SGML_CATA_DEPTH 10
struct _xmlCatalog {
	xmlCatalogType type;    /* either XML or SGML */
	/*
	 * SGML Catalogs are stored as a simple hash table of catalog entries
	 * Catalog stack to check against overflows when building the
	 * SGML catalog
	 */
	char * catalTab[XML_MAX_SGML_CATA_DEPTH]; /* stack of catals */
	int catalNr;            /* Number of current catal streams */
	int catalMax;           /* Max number of catal streams */
	xmlHashTable * sgml;
	/*
	 * XML Catalogs are stored as a tree of Catalog entries
	 */
	xmlCatalogPrefer prefer;
	xmlCatalogEntryPtr xml;
};
//
// Global variables
//
/*
 * Those are preferences
 */
static int xmlDebugCatalogs = 0; // @global @debug 
static xmlCatalogAllow xmlCatalogDefaultAllow = XML_CATA_ALLOW_ALL;
static xmlCatalogPrefer xmlCatalogDefaultPrefer = XML_CATA_PREFER_PUBLIC;
/*
 * Hash table containing all the trees of XML catalogs parsed by the application.
 */
static xmlHashTable * xmlCatalogXMLFiles = NULL; // @global
/*
 * The default catalog in use by the application
 */
static xmlCatalogPtr xmlDefaultCatalog = NULL; // @global
/*
 * A mutex for modifying the shared global catalog(s) xmlDefaultCatalog tree.
 * It also protects xmlCatalogXMLFiles
 * The core of this readers/writer scheme is in xmlFetchXMLCatalogFile()
 */
static xmlRMutex * xmlCatalogMutex = NULL; // @global
/*
 * Whether the catalog support was initialized.
 */
static int xmlCatalogInitialized = 0; // @global
//
// Catalog error handlers
//
/**
 * xmlCatalogErrMemory:
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void FASTCALL xmlCatalogErrMemory(const char * extra)
{
	__xmlRaiseError(0, 0, 0, 0, 0, XML_FROM_CATALOG, XML_ERR_NO_MEMORY, XML_ERR_ERROR, NULL, 0, extra, 0, 0, 0, 0, "Memory allocation failed : %s\n", extra);
}
/**
 * xmlCatalogErr:
 * @catal: the Catalog entry
 * @node: the context node
 * @msg:  the error message
 * @extra:  extra informations
 *
 * Handle a catalog error
 */
static void FASTCALL xmlCatalogErr(xmlCatalogEntryPtr catal, xmlNode * pNode, int error, const char * msg, const xmlChar * str1, const xmlChar * str2, const xmlChar * str3)
{
	__xmlRaiseError(0, 0, 0, catal, pNode, XML_FROM_CATALOG, error, XML_ERR_ERROR, NULL, 0, PTRCHRC_(str1), PTRCHRC_(str2), PTRCHRC_(str3), 0, 0,
	    msg, str1, str2, str3);
}
//
// Allocation and Freeing
//
/**
 * xmlNewCatalogEntry:
 * @type:  type of entry
 * @name:  name of the entry
 * @value:  value of the entry
 * @prefer:  the PUBLIC vs. SYSTEM current preference value
 * @group:  for members of a group, the group entry
 *
 * create a new Catalog entry, this type is shared both by XML and
 * SGML catalogs, but the acceptable types values differs.
 *
 * Returns the xmlCatalogEntryPtr or NULL in case of error
 */
static xmlCatalogEntryPtr xmlNewCatalogEntry(xmlCatalogEntryType type, const xmlChar * name,
    const xmlChar * value, const xmlChar * URL, xmlCatalogPrefer prefer, xmlCatalogEntryPtr group) 
{
	xmlChar * normid = NULL;
	xmlCatalogEntryPtr ret = (xmlCatalogEntry *)SAlloc::M(sizeof(xmlCatalogEntry));
	if(!ret) {
		xmlCatalogErrMemory("allocating catalog entry");
	}
	else {
		ret->next = NULL;
		ret->parent = NULL;
		ret->children = NULL;
		ret->type = type;
		if(type == XML_CATA_PUBLIC || type == XML_CATA_DELEGATE_PUBLIC) {
			normid = xmlCatalogNormalizePublic(name);
			if(normid != NULL)
				name = (*normid != 0 ? normid : NULL);
		}
		ret->name = sstrdup(name);
		SAlloc::F(normid);
		ret->value = sstrdup(value);
		SETIFZ(URL, value);
		ret->URL = sstrdup(URL);
		ret->prefer = prefer;
		ret->dealloc = 0;
		ret->depth = 0;
		ret->group = group;
	}
	return ret;
}

static void xmlFreeCatalogEntryList(xmlCatalogEntryPtr ret);

/**
 * xmlFreeCatalogEntry:
 * @ret:  a Catalog entry
 *
 * Free the memory allocated to a Catalog entry
 */
static void xmlFreeCatalogEntry(xmlCatalogEntryPtr ret) 
{
	if(ret) {
		// 
		// Entries stored in the file hash must be deallocated only by the file hash cleaner !
		// 
		if(ret->dealloc != 1) {
			if(xmlDebugCatalogs) {
				if(ret->name)
					xmlGenericError(0, "Free catalog entry %s\n", ret->name);
				else if(ret->value)
					xmlGenericError(0, "Free catalog entry %s\n", ret->value);
				else
					xmlGenericError(0, "Free catalog entry\n");
			}
			SAlloc::F(ret->name);
			SAlloc::F(ret->value);
			SAlloc::F(ret->URL);
			SAlloc::F(ret);
		}
	}
}
/**
 * xmlFreeCatalogEntryList:
 * @ret:  a Catalog entry list
 *
 * Free the memory allocated to a full chained list of Catalog entries
 */
static void xmlFreeCatalogEntryList(xmlCatalogEntryPtr ret) 
{
	while(ret) {
		xmlCatalogEntryPtr next = ret->next;
		xmlFreeCatalogEntry(ret);
		ret = next;
	}
}
/**
 * xmlFreeCatalogHashEntryList:
 * @ret:  a Catalog entry list
 *
 * Free the memory allocated to list of Catalog entries from the
 * catalog file hash.
 */
static void xmlFreeCatalogHashEntryList(xmlCatalogEntryPtr catal) 
{
	if(catal) {
		xmlCatalogEntryPtr children = catal->children;
		while(children != NULL) {
			xmlCatalogEntryPtr next = children->next;
			children->dealloc = 0;
			children->children = NULL;
			xmlFreeCatalogEntry(children);
			children = next;
		}
		catal->dealloc = 0;
		xmlFreeCatalogEntry(catal);
	}
}
/**
 * xmlCreateNewCatalog:
 * @type:  type of catalog
 * @prefer:  the PUBLIC vs. SYSTEM current preference value
 *
 * create a new Catalog, this type is shared both by XML and
 * SGML catalogs, but the acceptable types values differs.
 *
 * Returns the xmlCatalogPtr or NULL in case of error
 */
static xmlCatalog * xmlCreateNewCatalog(xmlCatalogType type, xmlCatalogPrefer prefer) 
{
	xmlCatalog * ret = static_cast<xmlCatalog *>(SAlloc::M(sizeof(xmlCatalog)));
	if(!ret) {
		xmlCatalogErrMemory("allocating catalog");
	}
	else {
		memzero(ret, sizeof(xmlCatalog));
		ret->type = type;
		ret->catalNr = 0;
		ret->catalMax = XML_MAX_SGML_CATA_DEPTH;
		ret->prefer = prefer;
		if(ret->type == XML_SGML_CATALOG_TYPE)
			ret->sgml = xmlHashCreate(10);
	}
	return ret;
}
/**
 * xmlFreeCatalog:
 * @catal:  a Catalog
 *
 * Free the memory allocated to a Catalog
 */
void xmlFreeCatalog(xmlCatalogPtr catal) 
{
	if(catal) {
		xmlFreeCatalogEntryList(catal->xml);
		xmlHashFree(catal->sgml, (xmlHashDeallocator)xmlFreeCatalogEntry);
		SAlloc::F(catal);
	}
}
//
// Serializing Catalogs
//
#ifdef LIBXML_OUTPUT_ENABLED
/**
 * xmlCatalogDumpEntry:
 * @entry:  the catalog entry
 * @out:  the file.
 *
 * Serialize an SGML Catalog entry
 */
static void xmlCatalogDumpEntry(xmlCatalogEntryPtr entry, FILE * out) 
{
	if(entry && out) {
		switch(entry->type) {
			case SGML_CATA_ENTITY: fprintf(out, "ENTITY "); break;
			case SGML_CATA_PENTITY: fprintf(out, "ENTITY %%"); break;
			case SGML_CATA_DOCTYPE: fprintf(out, "DOCTYPE "); break;
			case SGML_CATA_LINKTYPE: fprintf(out, "LINKTYPE "); break;
			case SGML_CATA_NOTATION: fprintf(out, "NOTATION "); break;
			case SGML_CATA_PUBLIC: fprintf(out, "PUBLIC "); break;
			case SGML_CATA_SYSTEM: fprintf(out, "SYSTEM "); break;
			case SGML_CATA_DELEGATE: fprintf(out, "DELEGATE "); break;
			case SGML_CATA_BASE: fprintf(out, "BASE "); break;
			case SGML_CATA_CATALOG: fprintf(out, "CATALOG "); break;
			case SGML_CATA_DOCUMENT: fprintf(out, "DOCUMENT "); break;
			case SGML_CATA_SGMLDECL: fprintf(out, "SGMLDECL "); break;
			default: return;
		}
		switch(entry->type) {
			case SGML_CATA_ENTITY:
			case SGML_CATA_PENTITY:
			case SGML_CATA_DOCTYPE:
			case SGML_CATA_LINKTYPE:
			case SGML_CATA_NOTATION: fprintf(out, "%s", (const char *)entry->name); break;
			case SGML_CATA_PUBLIC:
			case SGML_CATA_SYSTEM:
			case SGML_CATA_SGMLDECL:
			case SGML_CATA_DOCUMENT:
			case SGML_CATA_CATALOG:
			case SGML_CATA_BASE:
			case SGML_CATA_DELEGATE: fprintf(out, "\"%s\"", entry->name); break;
			default: break;
		}
		switch(entry->type) {
			case SGML_CATA_ENTITY:
			case SGML_CATA_PENTITY:
			case SGML_CATA_DOCTYPE:
			case SGML_CATA_LINKTYPE:
			case SGML_CATA_NOTATION:
			case SGML_CATA_PUBLIC:
			case SGML_CATA_SYSTEM:
			case SGML_CATA_DELEGATE: fprintf(out, " \"%s\"", entry->value); break;
			default: break;
		}
		fprintf(out, "\n");
	}
}
/**
 * xmlDumpXMLCatalogNode:
 * @catal:  top catalog entry
 * @catalog: pointer to the xml tree
 * @doc: the containing document
 * @ns: the current namespace
 * @cgroup: group node for group members
 *
 * Serializes a Catalog entry, called by xmlDumpXMLCatalog and recursively
 * for group entries
 */
static void xmlDumpXMLCatalogNode(xmlCatalogEntryPtr catal, xmlNode * catalog, xmlDoc * doc, xmlNs * ns, xmlCatalogEntryPtr cgroup) 
{
	xmlNode * P_Node;
	/*
	 * add all the catalog entries
	 */
	xmlCatalogEntryPtr cur = catal;
	while(cur) {
		if(cur->group == cgroup) {
			switch(cur->type) {
				case XML_CATA_REMOVED:
				    break;
				case XML_CATA_BROKEN_CATALOG:
				case XML_CATA_CATALOG:
				    if(cur == catal) {
					    cur = cur->children;
					    continue;
				    }
				    break;
				case XML_CATA_NEXT_CATALOG:
				    P_Node = xmlNewDocNode(doc, ns, reinterpret_cast<const xmlChar *>("nextCatalog"), 0);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("catalog"), cur->value);
				    xmlAddChild(catalog, P_Node);
				    break;
				case XML_CATA_NONE:
				    break;
				case XML_CATA_GROUP:
				    P_Node = xmlNewDocNode(doc, ns, reinterpret_cast<const xmlChar *>("group"), 0);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("id"), cur->name);
				    if(cur->value != NULL) {
					    xmlNs * xns = xmlSearchNsByHref(doc, P_Node, XML_XML_NAMESPACE);
					    if(xns)
						    xmlSetNsProp(P_Node, xns, reinterpret_cast<const xmlChar *>("base"), cur->value);
				    }
				    switch(cur->prefer) {
					    case XML_CATA_PREFER_NONE: break;
					    case XML_CATA_PREFER_PUBLIC: xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("prefer"), reinterpret_cast<const xmlChar *>("public")); break;
					    case XML_CATA_PREFER_SYSTEM: xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("prefer"), reinterpret_cast<const xmlChar *>("system")); break;
				    }
				    xmlDumpXMLCatalogNode(cur->next, P_Node, doc, ns, cur);
				    xmlAddChild(catalog, P_Node);
				    break;
				case XML_CATA_PUBLIC:
				    P_Node = xmlNewDocNode(doc, ns, reinterpret_cast<const xmlChar *>("public"), 0);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("publicId"), cur->name);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("uri"), cur->value);
				    xmlAddChild(catalog, P_Node);
				    break;
				case XML_CATA_SYSTEM:
				    P_Node = xmlNewDocNode(doc, ns, reinterpret_cast<const xmlChar *>("system"), 0);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("systemId"), cur->name);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("uri"), cur->value);
				    xmlAddChild(catalog, P_Node);
				    break;
				case XML_CATA_REWRITE_SYSTEM:
				    P_Node = xmlNewDocNode(doc, ns, reinterpret_cast<const xmlChar *>("rewriteSystem"), 0);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("systemIdStartString"), cur->name);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("rewritePrefix"), cur->value);
				    xmlAddChild(catalog, P_Node);
				    break;
				case XML_CATA_DELEGATE_PUBLIC:
				    P_Node = xmlNewDocNode(doc, ns, reinterpret_cast<const xmlChar *>("delegatePublic"), 0);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("publicIdStartString"), cur->name);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("catalog"), cur->value);
				    xmlAddChild(catalog, P_Node);
				    break;
				case XML_CATA_DELEGATE_SYSTEM:
				    P_Node = xmlNewDocNode(doc, ns, reinterpret_cast<const xmlChar *>("delegateSystem"), 0);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("systemIdStartString"), cur->name);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("catalog"), cur->value);
				    xmlAddChild(catalog, P_Node);
				    break;
				case XML_CATA_URI:
				    P_Node = xmlNewDocNode(doc, ns, reinterpret_cast<const xmlChar *>("uri"), 0);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("name"), cur->name);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("uri"), cur->value);
				    xmlAddChild(catalog, P_Node);
				    break;
				case XML_CATA_REWRITE_URI:
				    P_Node = xmlNewDocNode(doc, ns, reinterpret_cast<const xmlChar *>("rewriteURI"), 0);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("uriStartString"), cur->name);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("rewritePrefix"), cur->value);
				    xmlAddChild(catalog, P_Node);
				    break;
				case XML_CATA_DELEGATE_URI:
				    P_Node = xmlNewDocNode(doc, ns, reinterpret_cast<const xmlChar *>("delegateURI"), 0);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("uriStartString"), cur->name);
				    xmlSetProp(P_Node, reinterpret_cast<const xmlChar *>("catalog"), cur->value);
				    xmlAddChild(catalog, P_Node);
				    break;
				case SGML_CATA_SYSTEM:
				case SGML_CATA_PUBLIC:
				case SGML_CATA_ENTITY:
				case SGML_CATA_PENTITY:
				case SGML_CATA_DOCTYPE:
				case SGML_CATA_LINKTYPE:
				case SGML_CATA_NOTATION:
				case SGML_CATA_DELEGATE:
				case SGML_CATA_BASE:
				case SGML_CATA_CATALOG:
				case SGML_CATA_DOCUMENT:
				case SGML_CATA_SGMLDECL:
				    break;
			}
		}
		cur = cur->next;
	}
}

static int xmlDumpXMLCatalog(FILE * out, xmlCatalogEntryPtr catal) 
{
	int ret;
	xmlNs * ns;
	xmlDtd * dtd;
	xmlNode * catalog;
	xmlOutputBuffer * buf;
	/*
	 * Rebuild a catalog
	 */
	xmlDoc * doc = xmlNewDoc(NULL);
	if(!doc)
		return -1;
	dtd = xmlNewDtd(doc, reinterpret_cast<const xmlChar *>("catalog"), reinterpret_cast<const xmlChar *>("-//OASIS//DTD Entity Resolution XML Catalog V1.0//EN"),
	    reinterpret_cast<const xmlChar *>("http://www.oasis-open.org/committees/entity/release/1.0/catalog.dtd"));
	xmlAddChild((xmlNode *)doc, (xmlNode *)dtd);
	ns = xmlNewNs(NULL, XML_CATALOGS_NAMESPACE, 0);
	if(ns == NULL) {
		xmlFreeDoc(doc);
		return -1;
	}
	catalog = xmlNewDocNode(doc, ns, reinterpret_cast<const xmlChar *>("catalog"), 0);
	if(catalog == NULL) {
		xmlFreeNs(ns);
		xmlFreeDoc(doc);
		return -1;
	}
	catalog->nsDef = ns;
	xmlAddChild((xmlNode *)doc, catalog);
	xmlDumpXMLCatalogNode(catal, catalog, doc, ns, 0);
	/*
	 * reserialize it
	 */
	buf = xmlOutputBufferCreateFile(out, 0);
	if(!buf) {
		xmlFreeDoc(doc);
		return -1;
	}
	ret = xmlSaveFormatFileTo(buf, doc, NULL, 1);
	/*
	 * Free it
	 */
	xmlFreeDoc(doc);
	return ret;
}

#endif /* LIBXML_OUTPUT_ENABLED */
//
// Converting SGML Catalogs to XML
//
/**
 * xmlCatalogConvertEntry:
 * @entry:  the entry
 * @catal:  pointer to the catalog being converted
 *
 * Convert one entry from the catalog
 */
static void xmlCatalogConvertEntry(xmlCatalogEntryPtr entry, xmlCatalogPtr catal, const xmlChar * pName) 
{
	if(!entry || !catal || !catal->sgml || !catal->xml)
		return;
	switch(entry->type) {
		case SGML_CATA_ENTITY: entry->type = XML_CATA_PUBLIC; break;
		case SGML_CATA_PENTITY: entry->type = XML_CATA_PUBLIC; break;
		case SGML_CATA_DOCTYPE: entry->type = XML_CATA_PUBLIC; break;
		case SGML_CATA_LINKTYPE: entry->type = XML_CATA_PUBLIC; break;
		case SGML_CATA_NOTATION: entry->type = XML_CATA_PUBLIC; break;
		case SGML_CATA_PUBLIC: entry->type = XML_CATA_PUBLIC; break;
		case SGML_CATA_SYSTEM: entry->type = XML_CATA_SYSTEM; break;
		case SGML_CATA_DELEGATE: entry->type = XML_CATA_DELEGATE_PUBLIC; break;
		case SGML_CATA_CATALOG: entry->type = XML_CATA_CATALOG; break;
		default:
		    xmlHashRemoveEntry(catal->sgml, entry->name, (xmlHashDeallocator)xmlFreeCatalogEntry);
		    return;
	}
	//
	// Conversion successful, remove from the SGML catalog and add it to the default XML one
	//
	xmlHashRemoveEntry(catal->sgml, entry->name, 0);
	entry->parent = catal->xml;
	entry->next = NULL;
	if(catal->xml->children == NULL)
		catal->xml->children = entry;
	else {
		xmlCatalogEntry * prev = catal->xml->children;
		while(prev->next)
			prev = prev->next;
		prev->next = entry;
	}
}
/**
 * xmlConvertSGMLCatalog:
 * @catal: the catalog
 *
 * Convert all the SGML catalog entries as XML ones
 *
 * Returns the number of entries converted if successful, -1 otherwise
 */
int xmlConvertSGMLCatalog(xmlCatalogPtr catal) 
{
	if((catal == NULL) || (catal->type != XML_SGML_CATALOG_TYPE))
		return -1;
	if(xmlDebugCatalogs) {
		xmlGenericError(0, "Converting SGML catalog to XML\n");
	}
	xmlHashScan(catal->sgml, (xmlHashScanner)xmlCatalogConvertEntry, &catal);
	return 0;
}
//
// Helper function
//
/**
 * xmlCatalogUnWrapURN:
 * @urn:  an "urn:publicid:" to unwrap
 *
 * Expand the URN into the equivalent Public Identifier
 *
 * Returns the new identifier or NULL, the string must be deallocated
 *    by the caller.
 */
static xmlChar * xmlCatalogUnWrapURN(const xmlChar * urn) 
{
	xmlChar result[2000];
	uint i = 0;
	if(xmlStrncmp(urn, BAD_CAST XML_URN_PUBID, sizeof(XML_URN_PUBID) - 1))
		return 0;
	urn += sizeof(XML_URN_PUBID) - 1;
	while(*urn != 0) {
		if(i > sizeof(result) - 4)
			break;
		if(*urn == '+') {
			result[i++] = ' ';
			urn++;
		}
		else if(*urn == ':') {
			result[i++] = '/';
			result[i++] = '/';
			urn++;
		}
		else if(*urn == ';') {
			result[i++] = ':';
			result[i++] = ':';
			urn++;
		}
		else if(*urn == '%') {
			if((urn[1] == '2') && (urn[2] == 'B'))
				result[i++] = '+';
			else if((urn[1] == '3') && (urn[2] == 'A'))
				result[i++] = ':';
			else if((urn[1] == '2') && (urn[2] == 'F'))
				result[i++] = '/';
			else if((urn[1] == '3') && (urn[2] == 'B'))
				result[i++] = ';';
			else if((urn[1] == '2') && (urn[2] == '7'))
				result[i++] = '\'';
			else if((urn[1] == '3') && (urn[2] == 'F'))
				result[i++] = '?';
			else if((urn[1] == '2') && (urn[2] == '3'))
				result[i++] = '#';
			else if((urn[1] == '2') && (urn[2] == '5'))
				result[i++] = '%';
			else {
				result[i++] = *urn;
				urn++;
				continue;
			}
			urn += 3;
		}
		else {
			result[i++] = *urn;
			urn++;
		}
	}
	result[i] = 0;
	return sstrdup(result);
}
/**
 * xmlParseCatalogFile:
 * @filename:  the filename
 *
 * parse an XML file and build a tree. It's like xmlParseFile()
 * except it bypass all catalog lookups.
 *
 * Returns the resulting document tree or NULL in case of error
 */
xmlDoc * xmlParseCatalogFile(const char * filename) 
{
	xmlDoc * ret;
	char * directory = NULL;
	xmlParserInput * inputStream;
	xmlParserInputBuffer * buf;
	xmlParserCtxt * ctxt = xmlNewParserCtxt();
	if(!ctxt) {
#ifdef LIBXML_SAX1_ENABLED
		if(xmlDefaultSAXHandler.error != NULL) {
			xmlDefaultSAXHandler.error(NULL, "out of memory\n");
		}
#endif
		return 0;
	}
	buf = xmlParserInputBufferCreateFilename(filename, XML_CHAR_ENCODING_NONE);
	if(!buf) {
		xmlFreeParserCtxt(ctxt);
		return 0;
	}
	inputStream = xmlNewInputStream(ctxt);
	if(!inputStream) {
		xmlFreeParserCtxt(ctxt);
		return 0;
	}
	inputStream->filename = (char *)xmlCanonicPath((const xmlChar *)filename);
	inputStream->buf = buf;
	xmlBufResetInput(buf->buffer, inputStream);
	inputPush(ctxt, inputStream);
	if(!ctxt->directory && !directory)
		directory = xmlParserGetDirectory(filename);
	if(!ctxt->directory && directory)
		ctxt->directory = directory;
	ctxt->valid = 0;
	ctxt->validate = 0;
	ctxt->loadsubset = 0;
	ctxt->pedantic = 0;
	ctxt->dictNames = 1;
	xmlParseDocument(ctxt);
	if(ctxt->wellFormed)
		ret = ctxt->myDoc;
	else {
		ret = NULL;
		xmlFreeDoc(ctxt->myDoc);
		ctxt->myDoc = NULL;
	}
	xmlFreeParserCtxt(ctxt);
	return ret;
}

/**
 * xmlLoadFileContent:
 * @filename:  a file path
 *
 * Load a file content into memory.
 *
 * Returns a pointer to the 0 terminated string or NULL in case of error
 */
static xmlChar * xmlLoadFileContent(const char * filename)
{
#ifdef HAVE_STAT
	int fd;
#else
	FILE * fd;
#endif
	int len;
	long size;
#ifdef HAVE_STAT
	struct stat info;
#endif
	xmlChar * content;
	if(filename == NULL)
		return 0;
#ifdef HAVE_STAT
	if(stat(filename, &info) < 0)
		return 0;
#endif
#ifdef HAVE_STAT
	if((fd = open(filename, O_RDONLY)) < 0)
#else
	if((fd = fopen(filename, "rb")) == NULL)
#endif
	{
		return 0;
	}
#ifdef HAVE_STAT
	size = info.st_size;
#else
	if(fseek(fd, 0, SEEK_END) || (size = ftell(fd)) == EOF || fseek(fd, 0, SEEK_SET)) { // File operations denied? ok, just close and return failure
		fclose(fd);
		return 0;
	}
#endif
	content = static_cast<xmlChar *>(SAlloc::M(size + 10));
	if(content == NULL) {
		xmlCatalogErrMemory("allocating catalog data");
#ifdef HAVE_STAT
		close(fd);
#else
		fclose(fd);
#endif
		return 0;
	}
#ifdef HAVE_STAT
	len = read(fd, content, size);
	close(fd);
#else
	len = fread(content, 1, size, fd);
	fclose(fd);
#endif
	if(len < 0) {
		SAlloc::F(content);
		return 0;
	}
	content[len] = 0;
	return (content);
}
/**
 * xmlCatalogNormalizePublic:
 * @pubID:  the public ID string
 *
 *  Normalizes the Public Identifier
 *
 * Implements 6.2. Public Identifier Normalization
 * from http://www.oasis-open.org/committees/entity/spec-2001-08-06.html
 *
 * Returns the new string or NULL, the string must be deallocated
 *    by the caller.
 */
static xmlChar * xmlCatalogNormalizePublic(const xmlChar * pubID)
{
	int    ok = 1;
	int white;
	const xmlChar * p;
	xmlChar * ret;
	xmlChar * q;
	if(pubID == NULL)
		return 0;
	white = 1;
	for(p = pubID; *p != 0 && ok; p++) {
		if(!xmlIsBlank_ch(*p))
			white = 0;
		else if(*p == 0x20 && !white)
			white = 1;
		else
			ok = 0;
	}
	if(ok && !white) /* is normalized */
		return 0;
	ret = sstrdup(pubID);
	q = ret;
	white = 0;
	for(p = pubID; *p != 0; p++) {
		if(xmlIsBlank_ch(*p)) {
			if(q != ret)
				white = 1;
		}
		else {
			if(white) {
				*(q++) = 0x20;
				white = 0;
			}
			*(q++) = *p;
		}
	}
	*q = 0;
	return ret;
}
//
// The XML Catalog parser
//
static xmlCatalogEntryPtr xmlParseXMLCatalogFile(xmlCatalogPrefer prefer, const xmlChar * filename);
static void xmlParseXMLCatalogNodeList(xmlNode * cur, xmlCatalogPrefer prefer, xmlCatalogEntryPtr parent, xmlCatalogEntryPtr cgroup);
static xmlChar * xmlCatalogListXMLResolve(xmlCatalogEntryPtr catal, const xmlChar * pubID, const xmlChar * sysID);
static xmlChar * xmlCatalogListXMLResolveURI(xmlCatalogEntryPtr catal, const xmlChar * URI);
/**
 * xmlGetXMLCatalogEntryType:
 * @name:  the name
 *
 * lookup the internal type associated to an XML catalog entry name
 *
 * Returns the type associated with that name
 */
static xmlCatalogEntryType xmlGetXMLCatalogEntryType(const xmlChar * name) 
{
	xmlCatalogEntryType type = XML_CATA_NONE;
	if(sstreq(name, "system"))
		type = XML_CATA_SYSTEM;
	else if(sstreq(name, "public"))
		type = XML_CATA_PUBLIC;
	else if(sstreq(name, "rewriteSystem"))
		type = XML_CATA_REWRITE_SYSTEM;
	else if(sstreq(name, "delegatePublic"))
		type = XML_CATA_DELEGATE_PUBLIC;
	else if(sstreq(name, "delegateSystem"))
		type = XML_CATA_DELEGATE_SYSTEM;
	else if(sstreq(name, "uri"))
		type = XML_CATA_URI;
	else if(sstreq(name, "rewriteURI"))
		type = XML_CATA_REWRITE_URI;
	else if(sstreq(name, "delegateURI"))
		type = XML_CATA_DELEGATE_URI;
	else if(sstreq(name, "nextCatalog"))
		type = XML_CATA_NEXT_CATALOG;
	else if(sstreq(name, "catalog"))
		type = XML_CATA_CATALOG;
	return (type);
}
/**
 * xmlParseXMLCatalogOneNode:
 * @cur:  the XML node
 * @type:  the type of Catalog entry
 * @name:  the name of the node
 * @attrName:  the attribute holding the value
 * @uriAttrName:  the attribute holding the URI-Reference
 * @prefer:  the PUBLIC vs. SYSTEM current preference value
 * @cgroup:  the group which includes this node
 *
 * Finishes the examination of an XML tree node of a catalog and build
 * a Catalog entry from it.
 *
 * Returns the new Catalog entry node or NULL in case of error.
 */
static xmlCatalogEntryPtr FASTCALL xmlParseXMLCatalogOneNode(xmlNode * cur, xmlCatalogEntryType type, const xmlChar * name, 
	const xmlChar * attrName, const xmlChar * uriAttrName, xmlCatalogPrefer prefer, xmlCatalogEntryPtr cgroup) 
{
	int    ok = 1;
	xmlChar * uriValue;
	xmlChar * nameValue = NULL;
	xmlChar * base = NULL;
	xmlChar * URL = NULL;
	xmlCatalogEntryPtr ret = NULL;
	if(attrName != NULL) {
		nameValue = xmlGetProp(cur, attrName);
		if(nameValue == NULL) {
			xmlCatalogErr(ret, cur, XML_CATALOG_MISSING_ATTR, "%s entry lacks '%s'\n", name, attrName, 0);
			ok = 0;
		}
	}
	uriValue = xmlGetProp(cur, uriAttrName);
	if(uriValue == NULL) {
		xmlCatalogErr(ret, cur, XML_CATALOG_MISSING_ATTR, "%s entry lacks '%s'\n", name, uriAttrName, 0);
		ok = 0;
	}
	if(!ok) {
		SAlloc::F(nameValue);
		SAlloc::F(uriValue);
		return 0;
	}
	base = xmlNodeGetBase(cur->doc, cur);
	URL = xmlBuildURI(uriValue, base);
	if(URL) {
		if(xmlDebugCatalogs > 1) {
			if(nameValue != NULL)
				xmlGenericError(0, "Found %s: '%s' '%s'\n", name, nameValue, URL);
			else
				xmlGenericError(0, "Found %s: '%s'\n", name, URL);
		}
		ret = xmlNewCatalogEntry(type, nameValue, uriValue, URL, prefer, cgroup);
	}
	else {
		xmlCatalogErr(ret, cur, XML_CATALOG_ENTRY_BROKEN, "%s entry '%s' broken ?: %s\n", name, uriAttrName, uriValue);
	}
	SAlloc::F(nameValue);
	SAlloc::F(uriValue);
	SAlloc::F(base);
	SAlloc::F(URL);
	return ret;
}

/**
 * xmlParseXMLCatalogNode:
 * @cur:  the XML node
 * @prefer:  the PUBLIC vs. SYSTEM current preference value
 * @parent:  the parent Catalog entry
 * @cgroup:  the group which includes this node
 *
 * Examines an XML tree node of a catalog and build
 * a Catalog entry from it adding it to its parent. The examination can
 * be recursive.
 */
static void xmlParseXMLCatalogNode(xmlNode * cur, xmlCatalogPrefer prefer, xmlCatalogEntryPtr parent, xmlCatalogEntryPtr cgroup)
{
	xmlChar * base = NULL;
	xmlCatalogEntryPtr entry = NULL;
	if(!cur)
		return;
	if(sstreq(cur->name, "group")) {
		xmlCatalogPrefer pref = XML_CATA_PREFER_NONE;
		xmlChar * prop = xmlGetProp(cur, reinterpret_cast<const xmlChar *>("prefer"));
		if(prop != NULL) {
			if(sstreq(prop, "system")) {
				prefer = XML_CATA_PREFER_SYSTEM;
			}
			else if(sstreq(prop, "public")) {
				prefer = XML_CATA_PREFER_PUBLIC;
			}
			else {
				xmlCatalogErr(parent, cur, XML_CATALOG_PREFER_VALUE, "Invalid value for prefer: '%s'\n", prop, 0, 0);
			}
			SAlloc::F(prop);
			pref = prefer;
		}
		prop = xmlGetProp(cur, reinterpret_cast<const xmlChar *>("id"));
		base = xmlGetNsProp(cur, reinterpret_cast<const xmlChar *>("base"), XML_XML_NAMESPACE);
		entry = xmlNewCatalogEntry(XML_CATA_GROUP, prop, base, NULL, pref, cgroup);
		SAlloc::F(prop);
	}
	else if(sstreq(cur->name, "public")) {
		entry = xmlParseXMLCatalogOneNode(cur, XML_CATA_PUBLIC, reinterpret_cast<const xmlChar *>("public"), reinterpret_cast<const xmlChar *>("publicId"), reinterpret_cast<const xmlChar *>("uri"), prefer, cgroup);
	}
	else if(sstreq(cur->name, "system")) {
		entry = xmlParseXMLCatalogOneNode(cur, XML_CATA_SYSTEM, reinterpret_cast<const xmlChar *>("system"), reinterpret_cast<const xmlChar *>("systemId"), reinterpret_cast<const xmlChar *>("uri"), prefer, cgroup);
	}
	else if(sstreq(cur->name, "rewriteSystem")) {
		entry = xmlParseXMLCatalogOneNode(cur, XML_CATA_REWRITE_SYSTEM, reinterpret_cast<const xmlChar *>("rewriteSystem"), reinterpret_cast<const xmlChar *>("systemIdStartString"), reinterpret_cast<const xmlChar *>("rewritePrefix"), prefer, cgroup);
	}
	else if(sstreq(cur->name, "delegatePublic")) {
		entry = xmlParseXMLCatalogOneNode(cur, XML_CATA_DELEGATE_PUBLIC, reinterpret_cast<const xmlChar *>("delegatePublic"), reinterpret_cast<const xmlChar *>("publicIdStartString"), reinterpret_cast<const xmlChar *>("catalog"), prefer, cgroup);
	}
	else if(sstreq(cur->name, "delegateSystem")) {
		entry = xmlParseXMLCatalogOneNode(cur, XML_CATA_DELEGATE_SYSTEM, reinterpret_cast<const xmlChar *>("delegateSystem"), reinterpret_cast<const xmlChar *>("systemIdStartString"), reinterpret_cast<const xmlChar *>("catalog"), prefer, cgroup);
	}
	else if(sstreq(cur->name, "uri")) {
		entry = xmlParseXMLCatalogOneNode(cur, XML_CATA_URI, reinterpret_cast<const xmlChar *>("uri"), reinterpret_cast<const xmlChar *>("name"), reinterpret_cast<const xmlChar *>("uri"), prefer, cgroup);
	}
	else if(sstreq(cur->name, "rewriteURI")) {
		entry = xmlParseXMLCatalogOneNode(cur, XML_CATA_REWRITE_URI, reinterpret_cast<const xmlChar *>("rewriteURI"), reinterpret_cast<const xmlChar *>("uriStartString"), reinterpret_cast<const xmlChar *>("rewritePrefix"), prefer, cgroup);
	}
	else if(sstreq(cur->name, "delegateURI")) {
		entry = xmlParseXMLCatalogOneNode(cur, XML_CATA_DELEGATE_URI, reinterpret_cast<const xmlChar *>("delegateURI"), reinterpret_cast<const xmlChar *>("uriStartString"), reinterpret_cast<const xmlChar *>("catalog"), prefer, cgroup);
	}
	else if(sstreq(cur->name, "nextCatalog")) {
		entry = xmlParseXMLCatalogOneNode(cur, XML_CATA_NEXT_CATALOG, reinterpret_cast<const xmlChar *>("nextCatalog"), NULL, reinterpret_cast<const xmlChar *>("catalog"), prefer, cgroup);
	}
	if(entry) {
		if(parent) {
			entry->parent = parent;
			if(parent->children == NULL)
				parent->children = entry;
			else {
				xmlCatalogEntryPtr prev = parent->children;
				while(prev->next)
					prev = prev->next;
				prev->next = entry;
			}
		}
		if(entry->type == XML_CATA_GROUP) {
			//
			// Recurse to propagate prefer to the subtree (xml:base handling is automated)
			//
			xmlParseXMLCatalogNodeList(cur->children, prefer, parent, entry);
		}
	}
	SAlloc::F(base);
}

/**
 * xmlParseXMLCatalogNodeList:
 * @cur:  the XML node list of siblings
 * @prefer:  the PUBLIC vs. SYSTEM current preference value
 * @parent:  the parent Catalog entry
 * @cgroup:  the group which includes this list
 *
 * Examines a list of XML sibling nodes of a catalog and build
 * a list of Catalog entry from it adding it to the parent.
 * The examination will recurse to examine node subtrees.
 */
static void xmlParseXMLCatalogNodeList(xmlNode * cur, xmlCatalogPrefer prefer, xmlCatalogEntryPtr parent, xmlCatalogEntryPtr cgroup) 
{
	while(cur) {
		if(cur->ns && (cur->ns->href != NULL) && (sstreq(cur->ns->href, XML_CATALOGS_NAMESPACE))) {
			xmlParseXMLCatalogNode(cur, prefer, parent, cgroup);
		}
		cur = cur->next;
	}
	/* @todo sort the list according to REWRITE lengths and prefer value */
}

/**
 * xmlParseXMLCatalogFile:
 * @prefer:  the PUBLIC vs. SYSTEM current preference value
 * @filename:  the filename for the catalog
 *
 * Parses the catalog file to extract the XML tree and then analyze the
 * tree to build a list of Catalog entries corresponding to this catalog
 *
 * Returns the resulting Catalog entries list
 */
static xmlCatalogEntryPtr xmlParseXMLCatalogFile(xmlCatalogPrefer prefer, const xmlChar * filename) 
{
	xmlDoc * doc;
	xmlNode * cur;
	xmlChar * prop;
	xmlCatalogEntryPtr parent = NULL;
	if(filename == NULL)
		return 0;
	doc = xmlParseCatalogFile((const char *)filename);
	if(!doc) {
		if(xmlDebugCatalogs)
			xmlGenericError(0, "Failed to parse catalog %s\n", filename);
		return 0;
	}
	if(xmlDebugCatalogs)
		xmlGenericError(0, "%d Parsing catalog %s\n", xmlGetThreadId(), filename);

	cur = xmlDocGetRootElement(doc);
	if(cur && (sstreq(cur->name, reinterpret_cast<const xmlChar *>("catalog"))) && cur->ns && cur->ns->href && (sstreq(cur->ns->href, XML_CATALOGS_NAMESPACE))) {
		parent = xmlNewCatalogEntry(XML_CATA_CATALOG, NULL, (const xmlChar *)filename, NULL, prefer, 0);
		if(parent == NULL) {
			xmlFreeDoc(doc);
			return 0;
		}
		prop = xmlGetProp(cur, reinterpret_cast<const xmlChar *>("prefer"));
		if(prop != NULL) {
			if(sstreq(prop, "system")) {
				prefer = XML_CATA_PREFER_SYSTEM;
			}
			else if(sstreq(prop, "public")) {
				prefer = XML_CATA_PREFER_PUBLIC;
			}
			else {
				xmlCatalogErr(NULL, cur, XML_CATALOG_PREFER_VALUE, "Invalid value for prefer: '%s'\n", prop, 0, 0);
			}
			SAlloc::F(prop);
		}
		cur = cur->children;
		xmlParseXMLCatalogNodeList(cur, prefer, parent, 0);
	}
	else {
		xmlCatalogErr(NULL, (xmlNode *)doc, XML_CATALOG_NOT_CATALOG, "File %s is not an XML Catalog\n", filename, 0, 0);
		xmlFreeDoc(doc);
		return 0;
	}
	xmlFreeDoc(doc);
	return (parent);
}
/**
 * xmlFetchXMLCatalogFile:
 * @catal:  an existing but incomplete catalog entry
 *
 * Fetch and parse the subcatalog referenced by an entry
 *
 * Returns 0 in case of success, -1 otherwise
 */
static int FASTCALL xmlFetchXMLCatalogFile(xmlCatalogEntry * catal) 
{
	xmlCatalogEntry * doc;
	if(catal == NULL)
		return -1;
	if(catal->URL == NULL)
		return -1;
	/*
	 * lock the whole catalog for modification
	 */
	xmlRMutexLock(xmlCatalogMutex);
	if(catal->children != NULL) {
		/* Okay someone else did it in the meantime */
		xmlRMutexUnlock(xmlCatalogMutex);
		return 0;
	}
	if(xmlCatalogXMLFiles != NULL) {
		doc = (xmlCatalogEntry *)xmlHashLookup(xmlCatalogXMLFiles, catal->URL);
		if(doc) {
			if(xmlDebugCatalogs)
				xmlGenericError(0, "Found %s in file hash\n", catal->URL);
			catal->children = (catal->type == XML_CATA_CATALOG) ? doc->children : doc;
			catal->dealloc = 0;
			xmlRMutexUnlock(xmlCatalogMutex);
			return 0;
		}
		if(xmlDebugCatalogs)
			xmlGenericError(0, "%s not found in file hash\n", catal->URL);
	}
	/*
	 * Fetch and parse. Note that xmlParseXMLCatalogFile does not
	 * use the existing catalog, there is no recursion allowed at
	 * that level.
	 */
	doc = xmlParseXMLCatalogFile(catal->prefer, catal->URL);
	if(!doc) {
		catal->type = XML_CATA_BROKEN_CATALOG;
		xmlRMutexUnlock(xmlCatalogMutex);
		return -1;
	}
	catal->children = (catal->type == XML_CATA_CATALOG) ? doc->children : doc;
	doc->dealloc = 1;
	SETIFZ(xmlCatalogXMLFiles, xmlHashCreate(10));
	if(xmlCatalogXMLFiles != NULL) {
		if(xmlDebugCatalogs)
			xmlGenericError(0, "%s added to file hash\n", catal->URL);
		xmlHashAddEntry(xmlCatalogXMLFiles, catal->URL, doc);
	}
	xmlRMutexUnlock(xmlCatalogMutex);
	return 0;
}
//
// XML Catalog handling
//
/**
 * xmlAddXMLCatalog:
 * @catal:  top of an XML catalog
 * @type:  the type of record to add to the catalog
 * @orig:  the system, public or prefix to match (or NULL)
 * @replace:  the replacement value for the match
 *
 * Add an entry in the XML catalog, it may overwrite existing but
 * different entries.
 *
 * Returns 0 if successful, -1 otherwise
 */
static int xmlAddXMLCatalog(xmlCatalogEntryPtr catal, const xmlChar * type, const xmlChar * orig, const xmlChar * replace) 
{
	xmlCatalogEntryPtr cur;
	xmlCatalogEntryType typ;
	int doregister = 0;
	if((catal == NULL) || ((catal->type != XML_CATA_CATALOG) && (catal->type != XML_CATA_BROKEN_CATALOG)))
		return -1;
	if(catal->children == NULL) {
		xmlFetchXMLCatalogFile(catal);
	}
	if(catal->children == NULL)
		doregister = 1;
	typ = xmlGetXMLCatalogEntryType(type);
	if(typ == XML_CATA_NONE) {
		if(xmlDebugCatalogs)
			xmlGenericError(0, "Failed to add unknown element %s to catalog\n", type);
		return -1;
	}
	cur = catal->children;
	/*
	 * Might be a simple "update in place"
	 */
	if(cur) {
		while(cur) {
			if(orig && (cur->type == typ) && sstreq(orig, cur->name)) {
				if(xmlDebugCatalogs)
					xmlGenericError(0, "Updating element %s to catalog\n", type);
				SAlloc::F(cur->value);
				SAlloc::F(cur->URL);
				cur->value = sstrdup(replace);
				cur->URL = sstrdup(replace);
				return 0;
			}
			if(cur->next == NULL)
				break;
			cur = cur->next;
		}
	}
	if(xmlDebugCatalogs)
		xmlGenericError(0, "Adding element %s to catalog\n", type);
	if(!cur)
		catal->children = xmlNewCatalogEntry(typ, orig, replace, NULL, catal->prefer, 0);
	else
		cur->next = xmlNewCatalogEntry(typ, orig, replace, NULL, catal->prefer, 0);
	if(doregister) {
		catal->type = XML_CATA_CATALOG;
		cur = (xmlCatalogEntry *)xmlHashLookup(xmlCatalogXMLFiles, catal->URL);
		if(cur)
			cur->children = catal->children;
	}
	return 0;
}
/**
 * xmlDelXMLCatalog:
 * @catal:  top of an XML catalog
 * @value:  the value to remove from the catalog
 *
 * Remove entries in the XML catalog where the value or the URI
 * is equal to @value
 *
 * Returns the number of entries removed if successful, -1 otherwise
 */
static int xmlDelXMLCatalog(xmlCatalogEntryPtr catal, const xmlChar * value) 
{
	xmlCatalogEntryPtr cur;
	int ret = 0;
	if((catal == NULL) || ((catal->type != XML_CATA_CATALOG) && (catal->type != XML_CATA_BROKEN_CATALOG)))
		return -1;
	if(!value)
		return -1;
	if(catal->children == NULL) {
		xmlFetchXMLCatalogFile(catal);
	}
	/*
	 * Scan the children
	 */
	cur = catal->children;
	while(cur) {
		if((cur->name && sstreq(value, cur->name)) || sstreq(value, cur->value)) {
			if(xmlDebugCatalogs) {
				xmlGenericError(0, "Removing element %s from catalog\n", cur->name ? cur->name : cur->value);
			}
			cur->type = XML_CATA_REMOVED;
		}
		cur = cur->next;
	}
	return ret;
}
/**
 * xmlCatalogXMLResolve:
 * @catal:  a catalog list
 * @pubID:  the public ID string
 * @sysID:  the system ID string
 *
 * Do a complete resolution lookup of an External Identifier for a
 * list of catalog entries.
 *
 * Implements (or tries to) 7.1. External Identifier Resolution
 * from http://www.oasis-open.org/committees/entity/spec-2001-08-06.html
 *
 * Returns the URI of the resource or NULL if not found
 */
static xmlChar * xmlCatalogXMLResolve(xmlCatalogEntryPtr catal, const xmlChar * pubID, const xmlChar * sysID) 
{
	xmlChar * ret = NULL;
	xmlCatalogEntryPtr cur;
	int haveDelegate = 0;
	int haveNext = 0;
	/*
	 * protection against loops
	 */
	if(catal->depth > MAX_CATAL_DEPTH) {
		xmlCatalogErr(catal, NULL, XML_CATALOG_RECURSION, "Detected recursion in catalog %s\n", catal->name, 0, 0);
		return 0;
	}
	catal->depth++;
	/*
	 * First tries steps 2/ 3/ 4/ if a system ID is provided.
	 */
	if(sysID != NULL) {
		xmlCatalogEntryPtr rewrite = NULL;
		int lenrewrite = 0, len;
		cur = catal;
		haveDelegate = 0;
		while(cur) {
			switch(cur->type) {
				case XML_CATA_SYSTEM:
				    if(sstreq(sysID, cur->name)) {
					    if(xmlDebugCatalogs)
						    xmlGenericError(0, "Found system match %s, using %s\n", cur->name, cur->URL);
					    catal->depth--;
					    return sstrdup(cur->URL);
				    }
				    break;
				case XML_CATA_REWRITE_SYSTEM:
				    len = sstrlen(cur->name);
				    if((len > lenrewrite) && (!xmlStrncmp(sysID, cur->name, len))) {
					    lenrewrite = len;
					    rewrite = cur;
				    }
				    break;
				case XML_CATA_DELEGATE_SYSTEM:
				    if(!xmlStrncmp(sysID, cur->name, sstrlen(cur->name)))
					    haveDelegate++;
				    break;
				case XML_CATA_NEXT_CATALOG:
				    haveNext++;
				    break;
				default:
				    break;
			}
			cur = cur->next;
		}
		if(rewrite != NULL) {
			if(xmlDebugCatalogs)
				xmlGenericError(0, "Using rewriting rule %s\n", rewrite->name);
			ret = sstrdup(rewrite->URL);
			if(ret)
				ret = xmlStrcat(ret, &sysID[lenrewrite]);
			catal->depth--;
			return ret;
		}
		if(haveDelegate) {
			const xmlChar * delegates[MAX_DELEGATE];
			int nbList = 0, i;
			/*
			 * Assume the entries have been sorted by decreasing substring
			 * matches when the list was produced.
			 */
			cur = catal;
			while(cur) {
				if((cur->type == XML_CATA_DELEGATE_SYSTEM) && (!xmlStrncmp(sysID, cur->name, sstrlen(cur->name)))) {
					for(i = 0; i < nbList; i++)
						if(sstreq(cur->URL, delegates[i]))
							break;
					if(i < nbList) {
						cur = cur->next;
						continue;
					}
					if(nbList < MAX_DELEGATE)
						delegates[nbList++] = cur->URL;
					if(cur->children == NULL) {
						xmlFetchXMLCatalogFile(cur);
					}
					if(cur->children) {
						if(xmlDebugCatalogs)
							xmlGenericError(0, "Trying system delegate %s\n", cur->URL);
						ret = xmlCatalogListXMLResolve(cur->children, NULL, sysID);
						if(ret) {
							catal->depth--;
							return ret;
						}
					}
				}
				cur = cur->next;
			}
			/*
			 * Apply the cut algorithm explained in 4/
			 */
			catal->depth--;
			return XML_CATAL_BREAK;
		}
	}
	/*
	 * Then tries 5/ 6/ if a public ID is provided
	 */
	if(pubID != NULL) {
		cur = catal;
		haveDelegate = 0;
		while(cur) {
			switch(cur->type) {
				case XML_CATA_PUBLIC:
				    if(sstreq(pubID, cur->name)) {
					    if(xmlDebugCatalogs)
						    xmlGenericError(0, "Found public match %s\n", cur->name);
					    catal->depth--;
					    return sstrdup(cur->URL);
				    }
				    break;
				case XML_CATA_DELEGATE_PUBLIC:
				    if(!xmlStrncmp(pubID, cur->name, sstrlen(cur->name)) && (cur->prefer == XML_CATA_PREFER_PUBLIC))
					    haveDelegate++;
				    break;
				case XML_CATA_NEXT_CATALOG:
				    if(sysID == NULL)
					    haveNext++;
				    break;
				default:
				    break;
			}
			cur = cur->next;
		}
		if(haveDelegate) {
			const xmlChar * delegates[MAX_DELEGATE];
			int nbList = 0, i;
			/*
			 * Assume the entries have been sorted by decreasing substring
			 * matches when the list was produced.
			 */
			cur = catal;
			while(cur) {
				if((cur->type == XML_CATA_DELEGATE_PUBLIC) && (cur->prefer == XML_CATA_PREFER_PUBLIC) && (!xmlStrncmp(pubID, cur->name, sstrlen(cur->name)))) {
					for(i = 0; i < nbList; i++)
						if(sstreq(cur->URL, delegates[i]))
							break;
					if(i < nbList) {
						cur = cur->next;
						continue;
					}
					if(nbList < MAX_DELEGATE)
						delegates[nbList++] = cur->URL;
					if(cur->children == NULL) {
						xmlFetchXMLCatalogFile(cur);
					}
					if(cur->children) {
						if(xmlDebugCatalogs)
							xmlGenericError(0, "Trying public delegate %s\n", cur->URL);
						ret = xmlCatalogListXMLResolve(cur->children, pubID, 0);
						if(ret) {
							catal->depth--;
							return ret;
						}
					}
				}
				cur = cur->next;
			}
			/*
			 * Apply the cut algorithm explained in 4/
			 */
			catal->depth--;
			return XML_CATAL_BREAK;
		}
	}
	if(haveNext) {
		cur = catal;
		while(cur) {
			if(cur->type == XML_CATA_NEXT_CATALOG) {
				if(cur->children == NULL) {
					xmlFetchXMLCatalogFile(cur);
				}
				if(cur->children) {
					ret = xmlCatalogListXMLResolve(cur->children, pubID, sysID);
					if(ret) {
						catal->depth--;
						return ret;
					}
					else if(catal->depth > MAX_CATAL_DEPTH) {
						return 0;
					}
				}
			}
			cur = cur->next;
		}
	}
	catal->depth--;
	return 0;
}
/**
 * xmlCatalogXMLResolveURI:
 * @catal:  a catalog list
 * @URI:  the URI
 * @sysID:  the system ID string
 *
 * Do a complete resolution lookup of an External Identifier for a
 * list of catalog entries.
 *
 * Implements (or tries to) 7.2.2. URI Resolution
 * from http://www.oasis-open.org/committees/entity/spec-2001-08-06.html
 *
 * Returns the URI of the resource or NULL if not found
 */
static xmlChar * xmlCatalogXMLResolveURI(xmlCatalogEntryPtr catal, const xmlChar * URI) 
{
	xmlChar * ret = NULL;
	xmlCatalogEntryPtr cur;
	int haveDelegate = 0;
	int haveNext = 0;
	xmlCatalogEntryPtr rewrite = NULL;
	int lenrewrite = 0, len;
	if(catal == NULL)
		return 0;
	if(URI == NULL)
		return 0;
	if(catal->depth > MAX_CATAL_DEPTH) {
		xmlCatalogErr(catal, NULL, XML_CATALOG_RECURSION, "Detected recursion in catalog %s\n", catal->name, 0, 0);
		return 0;
	}
	/*
	 * First tries steps 2/ 3/ 4/ if a system ID is provided.
	 */
	cur = catal;
	haveDelegate = 0;
	while(cur) {
		switch(cur->type) {
			case XML_CATA_URI:
			    if(sstreq(URI, cur->name)) {
				    if(xmlDebugCatalogs)
					    xmlGenericError(0, "Found URI match %s\n", cur->name);
				    return sstrdup(cur->URL);
			    }
			    break;
			case XML_CATA_REWRITE_URI:
			    len = sstrlen(cur->name);
			    if((len > lenrewrite) && (!xmlStrncmp(URI, cur->name, len))) {
				    lenrewrite = len;
				    rewrite = cur;
			    }
			    break;
			case XML_CATA_DELEGATE_URI:
			    if(!xmlStrncmp(URI, cur->name, sstrlen(cur->name)))
				    haveDelegate++;
			    break;
			case XML_CATA_NEXT_CATALOG:
			    haveNext++;
			    break;
			default:
			    break;
		}
		cur = cur->next;
	}
	if(rewrite != NULL) {
		if(xmlDebugCatalogs)
			xmlGenericError(0, "Using rewriting rule %s\n", rewrite->name);
		ret = sstrdup(rewrite->URL);
		if(ret)
			ret = xmlStrcat(ret, &URI[lenrewrite]);
		return ret;
	}
	if(haveDelegate) {
		const xmlChar * delegates[MAX_DELEGATE];
		int nbList = 0, i;
		/*
		 * Assume the entries have been sorted by decreasing substring
		 * matches when the list was produced.
		 */
		cur = catal;
		while(cur) {
			if(((cur->type == XML_CATA_DELEGATE_SYSTEM) || (cur->type == XML_CATA_DELEGATE_URI)) && (!xmlStrncmp(URI, cur->name, sstrlen(cur->name)))) {
				for(i = 0; i < nbList; i++)
					if(sstreq(cur->URL, delegates[i]))
						break;
				if(i < nbList) {
					cur = cur->next;
					continue;
				}
				if(nbList < MAX_DELEGATE)
					delegates[nbList++] = cur->URL;

				if(cur->children == NULL) {
					xmlFetchXMLCatalogFile(cur);
				}
				if(cur->children) {
					if(xmlDebugCatalogs)
						xmlGenericError(0, "Trying URI delegate %s\n", cur->URL);
					ret = xmlCatalogListXMLResolveURI(cur->children, URI);
					if(ret)
						return ret;
				}
			}
			cur = cur->next;
		}
		/*
		 * Apply the cut algorithm explained in 4/
		 */
		return XML_CATAL_BREAK;
	}
	if(haveNext) {
		cur = catal;
		while(cur) {
			if(cur->type == XML_CATA_NEXT_CATALOG) {
				if(cur->children == NULL) {
					xmlFetchXMLCatalogFile(cur);
				}
				if(cur->children) {
					ret = xmlCatalogListXMLResolveURI(cur->children, URI);
					if(ret)
						return ret;
				}
			}
			cur = cur->next;
		}
	}
	return 0;
}

/**
 * xmlCatalogListXMLResolve:
 * @catal:  a catalog list
 * @pubID:  the public ID string
 * @sysID:  the system ID string
 *
 * Do a complete resolution lookup of an External Identifier for a
 * list of catalogs
 *
 * Implements (or tries to) 7.1. External Identifier Resolution
 * from http://www.oasis-open.org/committees/entity/spec-2001-08-06.html
 *
 * Returns the URI of the resource or NULL if not found
 */
static xmlChar * xmlCatalogListXMLResolve(xmlCatalogEntryPtr catal, const xmlChar * pubID, const xmlChar * sysID) 
{
	xmlChar * ret = NULL;
	xmlChar * urnID = NULL;
	if(catal && (pubID || sysID)) {
		xmlChar * normid = xmlCatalogNormalizePublic(pubID);
		if(normid != NULL)
			pubID = (*normid != 0 ? normid : NULL);
		if(!xmlStrncmp(pubID, BAD_CAST XML_URN_PUBID, sizeof(XML_URN_PUBID) - 1)) {
			urnID = xmlCatalogUnWrapURN(pubID);
			if(xmlDebugCatalogs) {
				if(urnID == NULL)
					xmlGenericError(0, "Public URN ID %s expanded to NULL\n", pubID);
				else
					xmlGenericError(0, "Public URN ID expanded to %s\n", urnID);
			}
			ret = xmlCatalogListXMLResolve(catal, urnID, sysID);
			SAlloc::F(urnID);
			SAlloc::F(normid);
		}
		else if(!xmlStrncmp(sysID, BAD_CAST XML_URN_PUBID, sizeof(XML_URN_PUBID) - 1)) {
			urnID = xmlCatalogUnWrapURN(sysID);
			if(xmlDebugCatalogs) {
				if(urnID == NULL)
					xmlGenericError(0, "System URN ID %s expanded to NULL\n", sysID);
				else
					xmlGenericError(0, "System URN ID expanded to %s\n", urnID);
			}
			if(pubID == NULL)
				ret = xmlCatalogListXMLResolve(catal, urnID, 0);
			else if(sstreq(pubID, urnID))
				ret = xmlCatalogListXMLResolve(catal, pubID, 0);
			else {
				ret = xmlCatalogListXMLResolve(catal, pubID, urnID);
			}
			SAlloc::F(urnID);
			SAlloc::F(normid);
		}
		else {
			while(catal) {
				if(catal->type == XML_CATA_CATALOG) {
					if(catal->children == NULL) {
						xmlFetchXMLCatalogFile(catal);
					}
					if(catal->children != NULL) {
						ret = xmlCatalogXMLResolve(catal->children, pubID, sysID);
						if(ret) {
							break;
						}
						else if((catal->children != NULL) && (catal->children->depth > MAX_CATAL_DEPTH)) {
							ret = NULL;
							break;
						}
					}
				}
				catal = catal->next;
			}
			SAlloc::F(normid);
		}
	}
	return ret;
}
/**
 * xmlCatalogListXMLResolveURI:
 * @catal:  a catalog list
 * @URI:  the URI
 *
 * Do a complete resolution lookup of an URI for a list of catalogs
 *
 * Implements (or tries to) 7.2. URI Resolution
 * from http://www.oasis-open.org/committees/entity/spec-2001-08-06.html
 *
 * Returns the URI of the resource or NULL if not found
 */
static xmlChar * xmlCatalogListXMLResolveURI(xmlCatalogEntryPtr catal, const xmlChar * URI) 
{
	xmlChar * ret = NULL;
	if(catal && URI) {
		if(!xmlStrncmp(URI, BAD_CAST XML_URN_PUBID, sizeof(XML_URN_PUBID) - 1)) {
			xmlChar * urnID = xmlCatalogUnWrapURN(URI);
			if(xmlDebugCatalogs) {
				if(urnID == NULL)
					xmlGenericError(0, "URN ID %s expanded to NULL\n", URI);
				else
					xmlGenericError(0, "URN ID expanded to %s\n", urnID);
			}
			ret = xmlCatalogListXMLResolve(catal, urnID, 0);
			SAlloc::F(urnID);
		}
		else {
			while(catal) {
				if(catal->type == XML_CATA_CATALOG) {
					if(catal->children == NULL) {
						xmlFetchXMLCatalogFile(catal);
					}
					if(catal->children != NULL) {
						ret = xmlCatalogXMLResolveURI(catal->children, URI);
						if(ret)
							return ret;
					}
				}
				catal = catal->next;
			}
		}
	}
	return ret;
}
//
// The SGML Catalog parser
//
#define RAW * cur
#define NEXT cur ++;
#define SKIP(x) cur += x;

#define SKIP_BLANKS while(IS_BLANK_CH(*cur)) NEXT;

/**
 * xmlParseSGMLCatalogComment:
 * @cur:  the current character
 *
 * Skip a comment in an SGML catalog
 *
 * Returns new current character
 */
static const xmlChar * xmlParseSGMLCatalogComment(const xmlChar * cur) 
{
	if((cur[0] != '-') || (cur[1] != '-'))
		return cur;
	else {
		SKIP(2);
		while((cur[0] != 0) && ((cur[0] != '-') || ((cur[1] != '-'))))
			NEXT;
		return (cur[0] == 0) ? 0 : (cur + 2);
	}
}
/**
 * xmlParseSGMLCatalogPubid:
 * @cur:  the current character
 * @id:  the return location
 *
 * Parse an SGML catalog ID
 *
 * Returns new current character and store the value in @id
 */
static const xmlChar * xmlParseSGMLCatalogPubid(const xmlChar * cur, xmlChar ** id) 
{
	xmlChar * buf = NULL, * tmp;
	int len = 0;
	int size = 50;
	xmlChar stop;
	int count = 0;
	*id = NULL;
	if(RAW == '"') {
		NEXT;
		stop = '"';
	}
	else if(RAW == '\'') {
		NEXT;
		stop = '\'';
	}
	else {
		stop = ' ';
	}
	buf = static_cast<xmlChar *>(SAlloc::M(size * sizeof(xmlChar)));
	if(!buf) {
		xmlCatalogErrMemory("allocating public ID");
		return 0;
	}
	while(IS_PUBIDCHAR_CH(*cur) || (*cur == '?')) {
		if((*cur == stop) && (stop != ' '))
			break;
		if((stop == ' ') && (IS_BLANK_CH(*cur)))
			break;
		if(len + 1 >= size) {
			size *= 2;
			tmp = static_cast<xmlChar *>(SAlloc::R(buf, size * sizeof(xmlChar)));
			if(!tmp) {
				xmlCatalogErrMemory("allocating public ID");
				SAlloc::F(buf);
				return 0;
			}
			buf = tmp;
		}
		buf[len++] = *cur;
		count++;
		NEXT;
	}
	buf[len] = 0;
	if(stop == ' ') {
		if(!IS_BLANK_CH(*cur)) {
			SAlloc::F(buf);
			return 0;
		}
	}
	else {
		if(*cur != stop) {
			SAlloc::F(buf);
			return 0;
		}
		NEXT;
	}
	*id = buf;
	return cur;
}
/**
 * xmlParseSGMLCatalogName:
 * @cur:  the current character
 * @name:  the return location
 *
 * Parse an SGML catalog name
 *
 * Returns new current character and store the value in @name
 */
static const xmlChar * xmlParseSGMLCatalogName(const xmlChar * cur, xmlChar ** name) 
{
	xmlChar buf[XML_MAX_NAMELEN + 5];
	int len = 0;
	int c;
	*name = NULL;
	/*
	 * Handler for more complex cases
	 */
	c = *cur;
	if((!IS_LETTER(c) && !oneof2(c, '_', ':'))) {
		return 0;
	}
	else {
		while(((IS_LETTER(c)) || (IS_DIGIT(c)) || oneof4(c, '.', '-', '_', ':'))) {
			buf[len++] = c;
			cur++;
			c = *cur;
			if(len >= XML_MAX_NAMELEN)
				return 0;
		}
		*name = xmlStrndup(buf, len);
		return cur;
	}
}
/**
 * xmlGetSGMLCatalogEntryType:
 * @name:  the entry name
 *
 * Get the Catalog entry type for a given SGML Catalog name
 *
 * Returns Catalog entry type
 */
static xmlCatalogEntryType xmlGetSGMLCatalogEntryType(const xmlChar * name) 
{
	xmlCatalogEntryType type = XML_CATA_NONE;
	if(sstreq(name, "SYSTEM"))
		type = SGML_CATA_SYSTEM;
	else if(sstreq(name, "PUBLIC"))
		type = SGML_CATA_PUBLIC;
	else if(sstreq(name, "DELEGATE"))
		type = SGML_CATA_DELEGATE;
	else if(sstreq(name, "ENTITY"))
		type = SGML_CATA_ENTITY;
	else if(sstreq(name, "DOCTYPE"))
		type = SGML_CATA_DOCTYPE;
	else if(sstreq(name, "LINKTYPE"))
		type = SGML_CATA_LINKTYPE;
	else if(sstreq(name, "NOTATION"))
		type = SGML_CATA_NOTATION;
	else if(sstreq(name, "SGMLDECL"))
		type = SGML_CATA_SGMLDECL;
	else if(sstreq(name, "DOCUMENT"))
		type = SGML_CATA_DOCUMENT;
	else if(sstreq(name, "CATALOG"))
		type = SGML_CATA_CATALOG;
	else if(sstreq(name, "BASE"))
		type = SGML_CATA_BASE;
	return (type);
}
/**
 * xmlParseSGMLCatalog:
 * @catal:  the SGML Catalog
 * @value:  the content of the SGML Catalog serialization
 * @file:  the filepath for the catalog
 * @super:  should this be handled as a Super Catalog in which case
 *     parsing is not recursive
 *
 * Parse an SGML catalog content and fill up the @catal hash table with
 * the new entries found.
 *
 * Returns 0 in case of success, -1 in case of error.
 */
static int xmlParseSGMLCatalog(xmlCatalogPtr catal, const xmlChar * value, const char * file, int super) 
{
	const xmlChar * cur = value;
	xmlChar * base = NULL;
	int res;
	if(!cur || (file == NULL))
		return -1;
	base = sstrdup((uchar *)file);
	while(cur && (cur[0] != 0)) {
		SKIP_BLANKS;
		if(cur[0] == 0)
			break;
		if((cur[0] == '-') && (cur[1] == '-')) {
			cur = xmlParseSGMLCatalogComment(cur);
			if(!cur) {
				/* error */
				break;
			}
		}
		else {
			xmlChar * sysid = NULL;
			xmlChar * name = NULL;
			xmlCatalogEntryType type = XML_CATA_NONE;
			cur = xmlParseSGMLCatalogName(cur, &name);
			if(!name) {
				/* error */
				break;
			}
			if(!IS_BLANK_CH(*cur)) {
				/* error */
				break;
			}
			SKIP_BLANKS;
			if(sstreq(name, "SYSTEM"))
				type = SGML_CATA_SYSTEM;
			else if(sstreq(name, "PUBLIC"))
				type = SGML_CATA_PUBLIC;
			else if(sstreq(name, "DELEGATE"))
				type = SGML_CATA_DELEGATE;
			else if(sstreq(name, "ENTITY"))
				type = SGML_CATA_ENTITY;
			else if(sstreq(name, "DOCTYPE"))
				type = SGML_CATA_DOCTYPE;
			else if(sstreq(name, "LINKTYPE"))
				type = SGML_CATA_LINKTYPE;
			else if(sstreq(name, "NOTATION"))
				type = SGML_CATA_NOTATION;
			else if(sstreq(name, "SGMLDECL"))
				type = SGML_CATA_SGMLDECL;
			else if(sstreq(name, "DOCUMENT"))
				type = SGML_CATA_DOCUMENT;
			else if(sstreq(name, "CATALOG"))
				type = SGML_CATA_CATALOG;
			else if(sstreq(name, "BASE"))
				type = SGML_CATA_BASE;
			else if(sstreq(name, "OVERRIDE")) {
				SAlloc::F(name);
				cur = xmlParseSGMLCatalogName(cur, &name);
				if(!name) {
					/* error */
					break;
				}
				SAlloc::F(name);
				continue;
			}
			ZFREE(name);
			switch(type) {
				case SGML_CATA_ENTITY:
				    if(*cur == '%')
					    type = SGML_CATA_PENTITY;
				case SGML_CATA_PENTITY:
				case SGML_CATA_DOCTYPE:
				case SGML_CATA_LINKTYPE:
				case SGML_CATA_NOTATION:
				    cur = xmlParseSGMLCatalogName(cur, &name);
				    if(!cur) {
					    break; /* error */
				    }
				    if(!IS_BLANK_CH(*cur)) {
					    break; /* error */
				    }
				    SKIP_BLANKS;
				    cur = xmlParseSGMLCatalogPubid(cur, &sysid);
				    if(!cur) {
					    break; /* error */
				    }
				    break;
				case SGML_CATA_PUBLIC:
				case SGML_CATA_SYSTEM:
				case SGML_CATA_DELEGATE:
				    cur = xmlParseSGMLCatalogPubid(cur, &name);
				    if(!cur) {
					    break; /* error */
				    }
				    if(type != SGML_CATA_SYSTEM) {
					    xmlChar * normid = xmlCatalogNormalizePublic(name);
					    if(normid) {
						    SAlloc::F(name);
						    if(*normid != 0)
							    name = normid;
						    else {
							    SAlloc::F(normid);
							    name = NULL;
						    }
					    }
				    }
				    if(!IS_BLANK_CH(*cur)) {
					    break; /* error */
				    }
				    SKIP_BLANKS;
				    cur = xmlParseSGMLCatalogPubid(cur, &sysid);
				    if(!cur) {
					    break; /* error */
				    }
				    break;
				case SGML_CATA_BASE:
				case SGML_CATA_CATALOG:
				case SGML_CATA_DOCUMENT:
				case SGML_CATA_SGMLDECL:
				    cur = xmlParseSGMLCatalogPubid(cur, &sysid);
				    if(!cur) {
					    break; /* error */
				    }
				    break;
				default:
				    break;
			}
			if(!cur) {
				SAlloc::F(name);
				SAlloc::F(sysid);
				break;
			}
			else if(type == SGML_CATA_BASE) {
				SAlloc::F(base);
				base = sstrdup(sysid);
			}
			else if(oneof2(type, SGML_CATA_PUBLIC, SGML_CATA_SYSTEM)) {
				xmlChar * filename = xmlBuildURI(sysid, base);
				if(filename) {
					xmlCatalogEntryPtr entry = xmlNewCatalogEntry(type, name, filename, NULL, XML_CATA_PREFER_NONE, 0);
					res = xmlHashAddEntry(catal->sgml, name, entry);
					if(res < 0) {
						xmlFreeCatalogEntry(entry);
					}
					SAlloc::F(filename);
				}
			}
			else if(type == SGML_CATA_CATALOG) {
				if(super) {
					xmlCatalogEntryPtr entry = xmlNewCatalogEntry(type, sysid, NULL, NULL, XML_CATA_PREFER_NONE, 0);
					res = xmlHashAddEntry(catal->sgml, sysid, entry);
					if(res < 0) {
						xmlFreeCatalogEntry(entry);
					}
				}
				else {
					xmlChar * filename = xmlBuildURI(sysid, base);
					if(filename) {
						xmlExpandCatalog(catal, (const char *)filename);
						SAlloc::F(filename);
					}
				}
			}
			// drop anything else we won't handle it
			SAlloc::F(name);
			SAlloc::F(sysid);
		}
	}
	SAlloc::F(base);
	return cur ? 0 : -1;
}
// 
// SGML Catalog handling
// 
/**
 * xmlCatalogGetSGMLPublic:
 * @catal:  an SGML catalog hash
 * @pubID:  the public ID string
 *
 * Try to lookup the catalog local reference associated to a public ID
 *
 * Returns the local resource if found or NULL otherwise.
 */
static const xmlChar * xmlCatalogGetSGMLPublic(xmlHashTable * catal, const xmlChar * pubID) 
{
	xmlCatalogEntryPtr entry;
	xmlChar * normid;
	if(catal == NULL)
		return 0;
	normid = xmlCatalogNormalizePublic(pubID);
	if(normid != NULL)
		pubID = (*normid != 0 ? normid : NULL);
	entry = (xmlCatalogEntry *)xmlHashLookup(catal, pubID);
	if(entry == NULL) {
		SAlloc::F(normid);
		return 0;
	}
	if(entry->type == SGML_CATA_PUBLIC) {
		SAlloc::F(normid);
		return (entry->URL);
	}
	SAlloc::F(normid);
	return 0;
}
/**
 * xmlCatalogGetSGMLSystem:
 * @catal:  an SGML catalog hash
 * @sysID:  the system ID string
 *
 * Try to lookup the catalog local reference for a system ID
 *
 * Returns the local resource if found or NULL otherwise.
 */
static const xmlChar * xmlCatalogGetSGMLSystem(xmlHashTable * catal, const xmlChar * sysID) 
{
	if(!catal)
		return 0;
	else {
		const xmlCatalogEntry * entry = static_cast<const xmlCatalogEntry *>(xmlHashLookup(catal, sysID));
		return (entry && entry->type == SGML_CATA_SYSTEM) ? (entry->URL) : 0;
	}
}
/**
 * xmlCatalogSGMLResolve:
 * @catal:  the SGML catalog
 * @pubID:  the public ID string
 * @sysID:  the system ID string
 *
 * Do a complete resolution lookup of an External Identifier
 *
 * Returns the URI of the resource or NULL if not found
 */
static const xmlChar * xmlCatalogSGMLResolve(xmlCatalogPtr catal, const xmlChar * pubID, const xmlChar * sysID) 
{
	const xmlChar * ret = NULL;
	if(catal->sgml == NULL)
		return 0;
	if(pubID != NULL)
		ret = xmlCatalogGetSGMLPublic(catal->sgml, pubID);
	if(ret)
		return ret;
	if(sysID != NULL)
		ret = xmlCatalogGetSGMLSystem(catal->sgml, sysID);
	if(ret)
		return ret;
	return 0;
}
//
// Specific Public interfaces
//
/**
 * xmlLoadSGMLSuperCatalog:
 * @filename:  a file path
 *
 * Load an SGML super catalog. It won't expand CATALOG or DELEGATE
 * references. This is only needed for manipulating SGML Super Catalogs
 * like adding and removing CATALOG or DELEGATE entries.
 *
 * Returns the catalog parsed or NULL in case of error
 */
xmlCatalogPtr xmlLoadSGMLSuperCatalog(const char * filename)
{
	xmlCatalogPtr catal;
	int ret;
	xmlChar * content = xmlLoadFileContent(filename);
	if(content == NULL)
		return 0;
	catal = xmlCreateNewCatalog(XML_SGML_CATALOG_TYPE, xmlCatalogDefaultPrefer);
	if(catal == NULL) {
		SAlloc::F(content);
		return 0;
	}
	ret = xmlParseSGMLCatalog(catal, content, filename, 1);
	SAlloc::F(content);
	if(ret < 0) {
		xmlFreeCatalog(catal);
		return 0;
	}
	return (catal);
}
/**
 * xmlLoadACatalog:
 * @filename:  a file path
 *
 * Load the catalog and build the associated data structures.
 * This can be either an XML Catalog or an SGML Catalog
 * It will recurse in SGML CATALOG entries. On the other hand XML
 * Catalogs are not handled recursively.
 *
 * Returns the catalog parsed or NULL in case of error
 */
xmlCatalogPtr xmlLoadACatalog(const char * filename)
{
	xmlChar * first;
	xmlCatalogPtr catal;
	int ret;
	xmlChar * content = xmlLoadFileContent(filename);
	if(content == NULL)
		return 0;
	first = content;
	while((*first != 0) && (*first != '-') && (*first != '<') && (!(((*first >= 'A') && (*first <= 'Z')) || ((*first >= 'a') && (*first <= 'z')))))
		first++;
	if(*first != '<') {
		catal = xmlCreateNewCatalog(XML_SGML_CATALOG_TYPE, xmlCatalogDefaultPrefer);
		if(catal == NULL) {
			SAlloc::F(content);
			return 0;
		}
		ret = xmlParseSGMLCatalog(catal, content, filename, 0);
		if(ret < 0) {
			xmlFreeCatalog(catal);
			SAlloc::F(content);
			return 0;
		}
	}
	else {
		catal = xmlCreateNewCatalog(XML_XML_CATALOG_TYPE, xmlCatalogDefaultPrefer);
		if(catal == NULL) {
			SAlloc::F(content);
			return 0;
		}
		catal->xml = xmlNewCatalogEntry(XML_CATA_CATALOG, NULL, NULL, BAD_CAST filename, xmlCatalogDefaultPrefer, 0);
	}
	SAlloc::F(content);
	return (catal);
}
/**
 * xmlExpandCatalog:
 * @catal:  a catalog
 * @filename:  a file path
 *
 * Load the catalog and expand the existing catal structure.
 * This can be either an XML Catalog or an SGML Catalog
 *
 * Returns 0 in case of success, -1 in case of error
 */
static int xmlExpandCatalog(xmlCatalogPtr catal, const char * filename)
{
	int ret;
	if((catal == NULL) || (filename == NULL))
		return -1;
	if(catal->type == XML_SGML_CATALOG_TYPE) {
		xmlChar * content = xmlLoadFileContent(filename);
		if(content == NULL)
			return -1;
		ret = xmlParseSGMLCatalog(catal, content, filename, 0);
		if(ret < 0) {
			SAlloc::F(content);
			return -1;
		}
		SAlloc::F(content);
	}
	else {
		xmlCatalogEntryPtr tmp = xmlNewCatalogEntry(XML_CATA_CATALOG, NULL, NULL, BAD_CAST filename, xmlCatalogDefaultPrefer, 0);
		xmlCatalogEntryPtr cur = catal->xml;
		if(!cur) {
			catal->xml = tmp;
		}
		else {
			while(cur->next) 
				cur = cur->next;
			cur->next = tmp;
		}
	}
	return 0;
}
/**
 * xmlACatalogResolveSystem:
 * @catal:  a Catalog
 * @sysID:  the system ID string
 *
 * Try to lookup the catalog resource for a system ID
 *
 * Returns the resource if found or NULL otherwise, the value returned
 * must be freed by the caller.
 */
xmlChar * xmlACatalogResolveSystem(xmlCatalogPtr catal, const xmlChar * sysID) 
{
	xmlChar * ret = NULL;
	if(sysID && catal) {
		if(xmlDebugCatalogs)
			xmlGenericError(0, "Resolve sysID %s\n", sysID);
		if(catal->type == XML_XML_CATALOG_TYPE) {
			ret = xmlCatalogListXMLResolve(catal->xml, NULL, sysID);
			if(ret == XML_CATAL_BREAK)
				ret = NULL;
		}
		else
			ret = sstrdup(xmlCatalogGetSGMLSystem(catal->sgml, sysID));
	}
	return ret;
}
/**
 * xmlACatalogResolvePublic:
 * @catal:  a Catalog
 * @pubID:  the public ID string
 *
 * Try to lookup the catalog local reference associated to a public ID in that catalog
 *
 * Returns the local resource if found or NULL otherwise, the value returned
 * must be freed by the caller.
 */
xmlChar * xmlACatalogResolvePublic(xmlCatalogPtr catal, const xmlChar * pubID) 
{
	xmlChar * ret = NULL;
	if(pubID && catal) {
		if(xmlDebugCatalogs)
			xmlGenericError(0, "Resolve pubID %s\n", pubID);
		if(catal->type == XML_XML_CATALOG_TYPE) {
			ret = xmlCatalogListXMLResolve(catal->xml, pubID, 0);
			if(ret == XML_CATAL_BREAK)
				ret = NULL;
		}
		else {
			const xmlChar * sgml = xmlCatalogGetSGMLPublic(catal->sgml, pubID);
			if(sgml)
				ret = sstrdup(sgml);
		}
	}
	return ret;
}
/**
 * xmlACatalogResolve:
 * @catal:  a Catalog
 * @pubID:  the public ID string
 * @sysID:  the system ID string
 *
 * Do a complete resolution lookup of an External Identifier
 *
 * Returns the URI of the resource or NULL if not found, it must be freed
 * by the caller.
 */
xmlChar * xmlACatalogResolve(xmlCatalogPtr catal, const xmlChar * pubID, const xmlChar * sysID)
{
	xmlChar * ret = 0;
	if((pubID || sysID) && catal) {
		if(xmlDebugCatalogs) {
			if((pubID != NULL) && (sysID != NULL)) {
				xmlGenericError(0, "Resolve: pubID %s sysID %s\n", pubID, sysID);
			}
			else if(pubID != NULL) {
				xmlGenericError(0, "Resolve: pubID %s\n", pubID);
			}
			else {
				xmlGenericError(0, "Resolve: sysID %s\n", sysID);
			}
		}
		if(catal->type == XML_XML_CATALOG_TYPE) {
			ret = xmlCatalogListXMLResolve(catal->xml, pubID, sysID);
			if(ret == XML_CATAL_BREAK)
				ret = NULL;
		}
		else {
			const xmlChar * sgml = xmlCatalogSGMLResolve(catal, pubID, sysID);
			if(sgml)
				ret = sstrdup(sgml);
		}
	}
	return ret;
}
/**
 * xmlACatalogResolveURI:
 * @catal:  a Catalog
 * @URI:  the URI
 *
 * Do a complete resolution lookup of an URI
 *
 * Returns the URI of the resource or NULL if not found, it must be freed
 * by the caller.
 */
xmlChar * xmlACatalogResolveURI(xmlCatalogPtr catal, const xmlChar * URI) 
{
	xmlChar * ret = 0;
	if(URI && catal) {
		if(xmlDebugCatalogs)
			xmlGenericError(0, "Resolve URI %s\n", URI);
		if(catal->type == XML_XML_CATALOG_TYPE) {
			ret = xmlCatalogListXMLResolveURI(catal->xml, URI);
			if(ret == XML_CATAL_BREAK)
				ret = NULL;
		}
		else {
			const xmlChar * sgml = xmlCatalogSGMLResolve(catal, NULL, URI);
			if(sgml)
				ret = sstrdup(sgml);
		}
	}
	return ret;
}

#ifdef LIBXML_OUTPUT_ENABLED
/**
 * xmlACatalogDump:
 * @catal:  a Catalog
 * @out:  the file.
 *
 * Dump the given catalog to the given file.
 */
void xmlACatalogDump(xmlCatalogPtr catal, FILE * out) 
{
	if(out && catal) {
		if(catal->type == XML_XML_CATALOG_TYPE) {
			xmlDumpXMLCatalog(out, catal->xml);
		}
		else {
			xmlHashScan(catal->sgml, (xmlHashScanner)xmlCatalogDumpEntry, out);
		}
	}
}

#endif /* LIBXML_OUTPUT_ENABLED */

/**
 * xmlACatalogAdd:
 * @catal:  a Catalog
 * @type:  the type of record to add to the catalog
 * @orig:  the system, public or prefix to match
 * @replace:  the replacement value for the match
 *
 * Add an entry in the catalog, it may overwrite existing but
 * different entries.
 *
 * Returns 0 if successful, -1 otherwise
 */
int xmlACatalogAdd(xmlCatalogPtr catal, const xmlChar * type, const xmlChar * orig, const xmlChar * replace)
{
	int res = -1;
	if(catal) {
		if(catal->type == XML_XML_CATALOG_TYPE) {
			res = xmlAddXMLCatalog(catal->xml, type, orig, replace);
		}
		else {
			xmlCatalogEntryType cattype = xmlGetSGMLCatalogEntryType(type);
			if(cattype != XML_CATA_NONE) {
				xmlCatalogEntryPtr entry = xmlNewCatalogEntry(cattype, orig, replace, NULL, XML_CATA_PREFER_NONE, 0);
				SETIFZ(catal->sgml, xmlHashCreate(10));
				res = xmlHashAddEntry(catal->sgml, orig, entry);
			}
		}
	}
	return res;
}
/**
 * xmlACatalogRemove:
 * @catal:  a Catalog
 * @value:  the value to remove
 *
 * Remove an entry from the catalog
 *
 * Returns the number of entries removed if successful, -1 otherwise
 */
int xmlACatalogRemove(xmlCatalogPtr catal, const xmlChar * value) 
{
	int res = -1;
	if(!catal || !value)
		return -1;
	if(catal->type == XML_XML_CATALOG_TYPE) {
		res = xmlDelXMLCatalog(catal->xml, value);
	}
	else {
		res = xmlHashRemoveEntry(catal->sgml, value, (xmlHashDeallocator)xmlFreeCatalogEntry);
		if(res == 0)
			res = 1;
	}
	return res;
}
/**
 * xmlNewCatalog:
 * @sgml:  should this create an SGML catalog
 *
 * create a new Catalog.
 *
 * Returns the xmlCatalogPtr or NULL in case of error
 */
xmlCatalogPtr xmlNewCatalog(int sgml) 
{
	xmlCatalogPtr catal = NULL;
	if(sgml) {
		catal = xmlCreateNewCatalog(XML_SGML_CATALOG_TYPE, xmlCatalogDefaultPrefer);
		if(catal && catal->sgml == NULL)
			catal->sgml = xmlHashCreate(10);
	}
	else
		catal = xmlCreateNewCatalog(XML_XML_CATALOG_TYPE, xmlCatalogDefaultPrefer);
	return (catal);
}
/**
 * xmlCatalogIsEmpty:
 * @catal:  should this create an SGML catalog
 *
 * Check is a catalog is empty
 *
 * Returns 1 if the catalog is empty, 0 if not, amd -1 in case of error.
 */
int xmlCatalogIsEmpty(xmlCatalogPtr catal) 
{
	if(catal == NULL)
		return -1;
	else if(catal->type == XML_XML_CATALOG_TYPE) {
		if(catal->xml == NULL)
			return 1;
		else if(!oneof2(catal->xml->type, XML_CATA_CATALOG, XML_CATA_BROKEN_CATALOG))
			return -1;
		else if(catal->xml->children == NULL)
			return 1;
		else
			return 0;
	}
	else {
		if(catal->sgml == NULL)
			return 1;
		else {
			int res = xmlHashSize(catal->sgml);
			if(res == 0)
				return 1;
			else if(res < 0)
				return -1;
		}
	}
	return 0;
}
//
// Public interfaces manipulating the global shared default catalog
//
/**
 * xmlInitializeCatalogData:
 *
 * Do the catalog initialization only of global data, doesn't try to load
 * any catalog actually.
 * this function is not thread safe, catalog initialization should
 * preferably be done once at startup
 */
static void xmlInitializeCatalogData() 
{
	if(!xmlCatalogInitialized) {
		if(getenv("XML_DEBUG_CATALOG"))
			xmlDebugCatalogs = 1;
		xmlCatalogMutex = xmlNewRMutex();
		xmlCatalogInitialized = 1;
	}
}
/**
 * xmlInitializeCatalog:
 *
 * Do the catalog initialization.
 * this function is not thread safe, catalog initialization should
 * preferably be done once at startup
 */
void xmlInitializeCatalog() 
{
	if(!xmlCatalogInitialized) {
		xmlInitializeCatalogData();
		xmlRMutexLock(xmlCatalogMutex);
		if(getenv("XML_DEBUG_CATALOG"))
			xmlDebugCatalogs = 1;
		if(xmlDefaultCatalog == NULL) {
			char * path;
			const char * cur, * paths;
			xmlCatalogPtr catal;
			xmlCatalogEntryPtr * nextent;
			const char * catalogs = static_cast<const char *>(getenv("XML_CATALOG_FILES"));
			if(catalogs == NULL)
#if defined(_WIN32) && defined(_MSC_VER)
			{
				void * hmodule = GetModuleHandle(_T("libxml2.dll"));
				if(hmodule == NULL)
					hmodule = GetModuleHandle(NULL);
				if(hmodule != NULL) {
					char buf[256];
					// @v10.3.11 {
					SString module_file_name;
					int    mfn_len = SSystem::SGetModuleFileName(static_cast<HMODULE>(hmodule), module_file_name);
					STRNSCPY(buf, module_file_name);
					ulong len = sstrlen(buf);
					// } @v10.3.11 
					// @v10.3.11 ulong len = GetModuleFileName((HMODULE)hmodule, buf, 255); // @unicodeproblem
					if(len != 0) {
						char * p = &(buf[len]);
						while(*p != '\\' && p > buf)
							p--;
						if(p != buf) {
							xmlChar * uri;
							strncpy(p, "\\..\\etc\\catalog", 255 - (p - buf));
							uri = xmlCanonicPath((const xmlChar *)buf);
							if(uri) {
								strncpy(XML_XML_DEFAULT_CATALOG, (const char *)uri, 255);
								SAlloc::F(uri);
							}
						}
					}
				}
				catalogs = XML_XML_DEFAULT_CATALOG;
			}
#else
				catalogs = XML_XML_DEFAULT_CATALOG;
#endif
			catal = xmlCreateNewCatalog(XML_XML_CATALOG_TYPE, xmlCatalogDefaultPrefer);
			if(catal) {
				// the XML_CATALOG_FILES envvar is allowed to contain a space-separated list of entries. 
				cur = catalogs;
				nextent = &catal->xml;
				while(*cur != '\0') {
					while(xmlIsBlank_ch(*cur))
						cur++;
					if(*cur != 0) {
						paths = cur;
						while((*cur != 0) && (!xmlIsBlank_ch(*cur)))
							cur++;
						path = (char *)xmlStrndup((const xmlChar *)paths, cur - paths);
						if(path) {
							*nextent = xmlNewCatalogEntry(XML_CATA_CATALOG, NULL, NULL, BAD_CAST path, xmlCatalogDefaultPrefer, 0);
							if(*nextent)
								nextent = &((*nextent)->next);
							SAlloc::F(path);
						}
					}
				}
				xmlDefaultCatalog = catal;
			}
		}
		xmlRMutexUnlock(xmlCatalogMutex);
	}
}
/**
 * xmlLoadCatalog:
 * @filename:  a file path
 *
 * Load the catalog and makes its definitions effective for the default
 * external entity loader. It will recurse in SGML CATALOG entries.
 * this function is not thread safe, catalog initialization should
 * preferably be done once at startup
 *
 * Returns 0 in case of success -1 in case of error
 */
int xmlLoadCatalog(const char * filename)
{
	int ret;
	xmlCatalogPtr catal;
	if(!xmlCatalogInitialized)
		xmlInitializeCatalogData();
	xmlRMutexLock(xmlCatalogMutex);
	if(xmlDefaultCatalog == NULL) {
		catal = xmlLoadACatalog(filename);
		if(catal == NULL) {
			xmlRMutexUnlock(xmlCatalogMutex);
			return -1;
		}
		xmlDefaultCatalog = catal;
		xmlRMutexUnlock(xmlCatalogMutex);
		return 0;
	}
	ret = xmlExpandCatalog(xmlDefaultCatalog, filename);
	xmlRMutexUnlock(xmlCatalogMutex);
	return ret;
}
/**
 * xmlLoadCatalogs:
 * @pathss:  a list of directories separated by a colon or a space.
 *
 * Load the catalogs and makes their definitions effective for the default
 * external entity loader.
 * this function is not thread safe, catalog initialization should
 * preferably be done once at startup
 */
void xmlLoadCatalogs(const char * pathss) 
{
	xmlChar * path;
#ifdef _WIN32
	int i, iLen;
#endif
	if(pathss) {
		for(const char * cur = pathss; *cur != 0;) {
			while(xmlIsBlank_ch(*cur)) 
				cur++;
			if(*cur != 0) {
				const char * paths = cur;
				while((*cur != 0) && (*cur != PATH_SEAPARATOR) && (!xmlIsBlank_ch(*cur)))
					cur++;
				path = xmlStrndup((const xmlChar *)paths, cur - paths);
#ifdef _WIN32
				iLen = sstrlen(path);
				for(i = 0; i < iLen; i++) {
					if(path[i] == '\\') {
						path[i] = '/';
					}
				}
#endif
				if(path) {
					xmlLoadCatalog((const char *)path);
					SAlloc::F(path);
				}
			}
			while(*cur == PATH_SEAPARATOR)
				cur++;
		}
	}
}
/**
 * xmlCatalogCleanup:
 *
 * Free up all the memory associated with catalogs
 */
void xmlCatalogCleanup() 
{
	if(xmlCatalogInitialized) {
		xmlRMutexLock(xmlCatalogMutex);
		if(xmlDebugCatalogs)
			xmlGenericError(0, "Catalogs cleanup\n");
		xmlHashFree(xmlCatalogXMLFiles, (xmlHashDeallocator)xmlFreeCatalogHashEntryList);
		xmlCatalogXMLFiles = NULL;
		if(xmlDefaultCatalog != NULL)
			xmlFreeCatalog(xmlDefaultCatalog);
		xmlDefaultCatalog = NULL;
		xmlDebugCatalogs = 0;
		xmlCatalogInitialized = 0;
		xmlRMutexUnlock(xmlCatalogMutex);
		xmlFreeRMutex(xmlCatalogMutex);
	}
}
/**
 * xmlCatalogResolveSystem:
 * @sysID:  the system ID string
 *
 * Try to lookup the catalog resource for a system ID
 *
 * Returns the resource if found or NULL otherwise, the value returned
 * must be freed by the caller.
 */
xmlChar * xmlCatalogResolveSystem(const xmlChar * sysID) 
{
	if(!xmlCatalogInitialized)
		xmlInitializeCatalog();
	return xmlACatalogResolveSystem(xmlDefaultCatalog, sysID);
}
/**
 * xmlCatalogResolvePublic:
 * @pubID:  the public ID string
 *
 * Try to lookup the catalog reference associated to a public ID
 *
 * Returns the resource if found or NULL otherwise, the value returned
 * must be freed by the caller.
 */
xmlChar * xmlCatalogResolvePublic(const xmlChar * pubID) 
{
	if(!xmlCatalogInitialized)
		xmlInitializeCatalog();
	return xmlACatalogResolvePublic(xmlDefaultCatalog, pubID);
}
/**
 * xmlCatalogResolve:
 * @pubID:  the public ID string
 * @sysID:  the system ID string
 *
 * Do a complete resolution lookup of an External Identifier
 *
 * Returns the URI of the resource or NULL if not found, it must be freed
 * by the caller.
 */
xmlChar * xmlCatalogResolve(const xmlChar * pubID, const xmlChar * sysID) 
{
	if(!xmlCatalogInitialized)
		xmlInitializeCatalog();
	return xmlACatalogResolve(xmlDefaultCatalog, pubID, sysID);
}
/**
 * xmlCatalogResolveURI:
 * @URI:  the URI
 *
 * Do a complete resolution lookup of an URI
 *
 * Returns the URI of the resource or NULL if not found, it must be freed
 * by the caller.
 */
xmlChar * xmlCatalogResolveURI(const xmlChar * URI) 
{
	if(!xmlCatalogInitialized)
		xmlInitializeCatalog();
	return xmlACatalogResolveURI(xmlDefaultCatalog, URI);
}

#ifdef LIBXML_OUTPUT_ENABLED
/**
 * xmlCatalogDump:
 * @out:  the file.
 *
 * Dump all the global catalog content to the given file.
 */
void xmlCatalogDump(FILE * out) 
{
	if(out) {
		if(!xmlCatalogInitialized)
			xmlInitializeCatalog();
		xmlACatalogDump(xmlDefaultCatalog, out);
	}
}

#endif /* LIBXML_OUTPUT_ENABLED */
/**
 * xmlCatalogAdd:
 * @type:  the type of record to add to the catalog
 * @orig:  the system, public or prefix to match
 * @replace:  the replacement value for the match
 *
 * Add an entry in the catalog, it may overwrite existing but
 * different entries.
 * If called before any other catalog routine, allows to override the
 * default shared catalog put in place by xmlInitializeCatalog();
 *
 * Returns 0 if successful, -1 otherwise
 */
int xmlCatalogAdd(const xmlChar * type, const xmlChar * orig, const xmlChar * replace) 
{
	int res = -1;
	if(!xmlCatalogInitialized)
		xmlInitializeCatalogData();
	xmlRMutexLock(xmlCatalogMutex);
	/*
	 * Specific case where one want to override the default catalog
	 * put in place by xmlInitializeCatalog();
	 */
	if(!xmlDefaultCatalog && sstreq(type, "catalog")) {
		xmlDefaultCatalog = xmlCreateNewCatalog(XML_XML_CATALOG_TYPE, xmlCatalogDefaultPrefer);
		xmlDefaultCatalog->xml = xmlNewCatalogEntry(XML_CATA_CATALOG, NULL, orig, NULL,  xmlCatalogDefaultPrefer, 0);
		xmlRMutexUnlock(xmlCatalogMutex);
		res = 0;
	}
	else {
		res = xmlACatalogAdd(xmlDefaultCatalog, type, orig, replace);
		xmlRMutexUnlock(xmlCatalogMutex);
	}
	return res;
}
/**
 * xmlCatalogRemove:
 * @value:  the value to remove
 *
 * Remove an entry from the catalog
 *
 * Returns the number of entries removed if successful, -1 otherwise
 */
int xmlCatalogRemove(const xmlChar * value) 
{
	int res;
	if(!xmlCatalogInitialized)
		xmlInitializeCatalog();
	xmlRMutexLock(xmlCatalogMutex);
	res = xmlACatalogRemove(xmlDefaultCatalog, value);
	xmlRMutexUnlock(xmlCatalogMutex);
	return res;
}
/**
 * xmlCatalogConvert:
 *
 * Convert all the SGML catalog entries as XML ones
 *
 * Returns the number of entries converted if successful, -1 otherwise
 */
int xmlCatalogConvert() 
{
	int res = -1;
	if(!xmlCatalogInitialized)
		xmlInitializeCatalog();
	xmlRMutexLock(xmlCatalogMutex);
	res = xmlConvertSGMLCatalog(xmlDefaultCatalog);
	xmlRMutexUnlock(xmlCatalogMutex);
	return res;
}
// 
// Public interface manipulating the common preferences
// 
/**
 * xmlCatalogGetDefaults:
 *
 * Used to get the user preference w.r.t. to what catalogs should
 * be accepted
 *
 * Returns the current xmlCatalogAllow value
 */
xmlCatalogAllow xmlCatalogGetDefaults() 
{
	return xmlCatalogDefaultAllow;
}
/**
 * xmlCatalogSetDefaults:
 * @allow:  what catalogs should be accepted
 *
 * Used to set the user preference w.r.t. to what catalogs should be accepted
 */
void xmlCatalogSetDefaults(xmlCatalogAllow allow) 
{
	if(xmlDebugCatalogs) {
		switch(allow) {
			case XML_CATA_ALLOW_NONE: xmlGenericError(0, "Disabling catalog usage\n"); break;
			case XML_CATA_ALLOW_GLOBAL: xmlGenericError(0, "Allowing only global catalogs\n"); break;
			case XML_CATA_ALLOW_DOCUMENT: xmlGenericError(0, "Allowing only catalogs from the document\n"); break;
			case XML_CATA_ALLOW_ALL: xmlGenericError(0, "Allowing all catalogs\n"); break;
		}
	}
	xmlCatalogDefaultAllow = allow;
}
/**
 * xmlCatalogSetDefaultPrefer:
 * @prefer:  the default preference for delegation
 *
 * Allows to set the preference between public and system for deletion
 * in XML Catalog resolution. C.f. section 4.1.1 of the spec
 * Values accepted are XML_CATA_PREFER_PUBLIC or XML_CATA_PREFER_SYSTEM
 *
 * Returns the previous value of the default preference for delegation
 */
xmlCatalogPrefer xmlCatalogSetDefaultPrefer(xmlCatalogPrefer prefer) 
{
	xmlCatalogPrefer ret = xmlCatalogDefaultPrefer;
	if(prefer != XML_CATA_PREFER_NONE) {
		if(xmlDebugCatalogs) {
			switch(prefer) {
				case XML_CATA_PREFER_PUBLIC: xmlGenericError(0, "Setting catalog preference to PUBLIC\n"); break;
				case XML_CATA_PREFER_SYSTEM: xmlGenericError(0, "Setting catalog preference to SYSTEM\n"); break;
				default: return ret;
			}
		}
		xmlCatalogDefaultPrefer = prefer;
	}
	return ret;
}
/**
 * xmlCatalogSetDebug:
 * @level:  the debug level of catalogs required
 *
 * Used to set the debug level for catalog operation, 0 disable
 * debugging, 1 enable it
 *
 * Returns the previous value of the catalog debugging level
 */
int xmlCatalogSetDebug(int level) 
{
	int ret = xmlDebugCatalogs;
	xmlDebugCatalogs = (level <= 0) ? 0 : level;
	return ret;
}
// 
// Minimal interfaces used for per-document catalogs by the parser
// 
/**
 * xmlCatalogFreeLocal:
 * @catalogs:  a document's list of catalogs
 *
 * Free up the memory associated to the catalog list
 */
void xmlCatalogFreeLocal(void * catalogs) 
{
	if(catalogs) {
		if(!xmlCatalogInitialized)
			xmlInitializeCatalog();
		xmlFreeCatalogEntryList((xmlCatalogEntry *)catalogs);
	}
}
/**
 * xmlCatalogAddLocal:
 * @catalogs:  a document's list of catalogs
 * @URL:  the URL to a new local catalog
 *
 * Add the new entry to the catalog list
 *
 * Returns the updated list
 */
void * xmlCatalogAddLocal(void * catalogs, const xmlChar * URL) 
{
	xmlCatalogEntryPtr catal, add;
	if(!xmlCatalogInitialized)
		xmlInitializeCatalog();
	if(URL == NULL)
		return catalogs;
	if(xmlDebugCatalogs)
		xmlGenericError(0, "Adding document catalog %s\n", URL);
	add = xmlNewCatalogEntry(XML_CATA_CATALOG, NULL, URL, NULL, xmlCatalogDefaultPrefer, 0);
	if(add == NULL)
		return (catalogs);
	catal = (xmlCatalogEntry *)catalogs;
	if(catal == NULL)
		return (void *)add;
	while(catal->next)
		catal = catal->next;
	catal->next = add;
	return catalogs;
}
/**
 * xmlCatalogLocalResolve:
 * @catalogs:  a document's list of catalogs
 * @pubID:  the public ID string
 * @sysID:  the system ID string
 *
 * Do a complete resolution lookup of an External Identifier using a
 * document's private catalog list
 *
 * Returns the URI of the resource or NULL if not found, it must be freed
 * by the caller.
 */
xmlChar * xmlCatalogLocalResolve(void * catalogs, const xmlChar * pubID, const xmlChar * sysID) 
{
	xmlChar * ret = 0;
	if(!xmlCatalogInitialized)
		xmlInitializeCatalog();
	if(pubID || sysID) {
		if(xmlDebugCatalogs) {
			if(pubID && sysID) {
				xmlGenericError(0, "Local Resolve: pubID %s sysID %s\n", pubID, sysID);
			}
			else if(pubID) {
				xmlGenericError(0, "Local Resolve: pubID %s\n", pubID);
			}
			else {
				xmlGenericError(0, "Local Resolve: sysID %s\n", sysID);
			}
		}
		xmlCatalogEntryPtr catal = (xmlCatalogEntry *)catalogs;
		if(catal) {
			ret = xmlCatalogListXMLResolve(catal, pubID, sysID);
			if(ret == XML_CATAL_BREAK)
				ret = 0;
		}
	}
	return ret;
}
/**
 * xmlCatalogLocalResolveURI:
 * @catalogs:  a document's list of catalogs
 * @URI:  the URI
 *
 * Do a complete resolution lookup of an URI using a
 * document's private catalog list
 *
 * Returns the URI of the resource or NULL if not found, it must be freed by the caller.
 */
xmlChar * xmlCatalogLocalResolveURI(void * catalogs, const xmlChar * URI) 
{
	xmlCatalogEntryPtr catal;
	xmlChar * ret = 0;
	if(!xmlCatalogInitialized)
		xmlInitializeCatalog();
	if(URI == NULL)
		return 0;
	if(xmlDebugCatalogs)
		xmlGenericError(0, "Resolve URI %s\n", URI);
	catal = (xmlCatalogEntry *)catalogs;
	if(catal == NULL)
		return 0;
	ret = xmlCatalogListXMLResolveURI(catal, URI);
	return (ret && ret != XML_CATAL_BREAK) ? ret : 0;
}
// 
// Deprecated interfaces
// 
/**
 * xmlCatalogGetSystem:
 * @sysID:  the system ID string
 *
 * Try to lookup the catalog reference associated to a system ID
 * DEPRECATED, use xmlCatalogResolveSystem()
 *
 * Returns the resource if found or NULL otherwise.
 */
const xmlChar * xmlCatalogGetSystem(const xmlChar * sysID) 
{
	xmlChar * ret;
	static xmlChar result[1000];
	static int msg = 0;
	if(!xmlCatalogInitialized)
		xmlInitializeCatalog();
	if(msg == 0) {
		xmlGenericError(0, "Use of deprecated xmlCatalogGetSystem() call\n");
		msg++;
	}
	if(sysID == NULL)
		return 0;
	/*
	 * Check first the XML catalogs
	 */
	if(xmlDefaultCatalog) {
		ret = xmlCatalogListXMLResolve(xmlDefaultCatalog->xml, NULL, sysID);
		if(ret && ret != XML_CATAL_BREAK) {
			snprintf((char *)result, sizeof(result) - 1, "%s", (char *)ret);
			result[sizeof(result) - 1] = 0;
			return result;
		}
	}
	return xmlDefaultCatalog ? xmlCatalogGetSGMLSystem(xmlDefaultCatalog->sgml, sysID) : 0;
}
/**
 * xmlCatalogGetPublic:
 * @pubID:  the public ID string
 *
 * Try to lookup the catalog reference associated to a public ID
 * DEPRECATED, use xmlCatalogResolvePublic()
 *
 * Returns the resource if found or NULL otherwise.
 */
const xmlChar * xmlCatalogGetPublic(const xmlChar * pubID) 
{
	xmlChar * ret;
	static xmlChar result[1000];
	static int msg = 0;
	if(!xmlCatalogInitialized)
		xmlInitializeCatalog();
	if(msg == 0) {
		xmlGenericError(0, "Use of deprecated xmlCatalogGetPublic() call\n");
		msg++;
	}
	if(pubID == NULL)
		return 0;
	// 
	// Check first the XML catalogs
	// 
	if(xmlDefaultCatalog) {
		ret = xmlCatalogListXMLResolve(xmlDefaultCatalog->xml, pubID, 0);
		if(ret && ret != XML_CATAL_BREAK) {
			snprintf((char *)result, sizeof(result) - 1, "%s", (char *)ret);
			result[sizeof(result) - 1] = 0;
			return result;
		}
	}
	return xmlDefaultCatalog ? xmlCatalogGetSGMLPublic(xmlDefaultCatalog->sgml, pubID) : 0;
}

#define bottom_catalog
//#include "elfgcchack.h"
#endif /* LIBXML_CATALOG_ENABLED */
