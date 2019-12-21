/*
 * tree.c : implementation of access function for an XML tree.
 *
 * References:
 * XHTML 1.0 W3C REC: http://www.w3.org/TR/2002/REC-xhtml1-20020801/
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 *
 */
#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop
// @v10.6.5 #include "save.h"

int __xmlRegisterCallbacks = 0;
// 
// Forward declarations
// 
static xmlNs *   xmlNewReconciliedNs(xmlDoc * doc, xmlNode * tree, xmlNs * ns);
static xmlChar * FASTCALL xmlGetPropNodeValueInternal(const xmlAttr * prop);
// 
// Tree memory error handler
// 
/**
 * xmlTreeErrMemory:
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void FASTCALL xmlTreeErrMemory(const char * extra)
{
	__xmlSimpleError(XML_FROM_TREE, XML_ERR_NO_MEMORY, NULL, NULL, extra);
}
/**
 * xmlTreeErr:
 * @code:  the error number
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void FASTCALL xmlTreeErr(int code, xmlNode * pNode, const char * extra)
{
	const char * msg = NULL;
	switch(code) {
		case XML_TREE_INVALID_HEX: msg = "invalid hexadecimal character value\n"; break;
		case XML_TREE_INVALID_DEC: msg = "invalid decimal character value\n"; break;
		case XML_TREE_UNTERMINATED_ENTITY: msg = "unterminated entity reference %15s\n"; break;
		case XML_TREE_NOT_UTF8: msg = "string is not in UTF-8\n"; break;
		default: msg = "unexpected error number\n";
	}
	__xmlSimpleError(XML_FROM_TREE, code, pNode, msg, extra);
}
// 
// A few static variables and macros
// 
/* #undef xmlStringText */
const xmlChar xmlStringText[] = { 't', 'e', 'x', 't', 0 };
/* #undef xmlStringTextNoenc */
const xmlChar xmlStringTextNoenc[] = { 't', 'e', 'x', 't', 'n', 'o', 'e', 'n', 'c', 0 };
/* #undef xmlStringComment */
const xmlChar xmlStringComment[] = { 'c', 'o', 'm', 'm', 'e', 'n', 't', 0 };

static int xmlCompressMode = 0;
static int xmlCheckDTD = 1;

static void FASTCALL __UpdateLastChildAndParent(xmlNode * pNode)
{
	if(pNode) {
		xmlNode * ulccur = pNode->children;
		if(ulccur) {
			while(ulccur->next) {
				ulccur->P_ParentNode = pNode;
				ulccur = ulccur->next;
			}
			ulccur->P_ParentNode = pNode;
			pNode->last = ulccur;
		}
		else
			pNode->last = NULL;
	}
}

#define UPDATE_LAST_CHILD_AND_PARENT(n) __UpdateLastChildAndParent(n);

/*#define UPDATE_LAST_CHILD_AND_PARENT(n) if(n) {	       \
		xmlNode * ulccur = (n)->children;				    \
		if(ulccur == NULL) {						   \
			(n)->last = NULL;						\
		} else {							    \
			while(ulccur->next) {				       \
				ulccur->parent = (n);					\
				ulccur = ulccur->next;					\
			}								\
			ulccur->parent = (n);						\
			(n)->last = ulccur;						\
		}}*/

#define IS_STR_XML(str) ((str) && (str[0] == 'x') && (str[1] == 'm') && (str[2] == 'l') && (str[3] == 0))

/* #define DEBUG_BUFFER */
/* #define DEBUG_TREE */
// 
// Functions to move to entities.c once the
// API freeze is smoothen and they can be made public.
// 
#ifdef LIBXML_TREE_ENABLED
/**
 * xmlGetEntityFromDtd:
 * @dtd:  A pointer to the DTD to search
 * @name:  The entity name
 *
 * Do an entity lookup in the DTD entity hash table and
 * return the corresponding entity, if found.
 *
 * Returns A pointer to the entity structure or NULL if not found.
 */
static xmlEntity * xmlGetEntityFromDtd(const xmlDtd * dtd, const xmlChar * name)
{
	if(dtd && dtd->entities) {
		xmlEntitiesTable * table = static_cast<xmlEntitiesTable *>(dtd->entities);
		return static_cast<xmlEntity *>(xmlHashLookup(table, name));
		// return (xmlGetEntityFromTable(table, name)); 
	}
	else
		return 0;
}
/**
 * xmlGetParameterEntityFromDtd:
 * @dtd:  A pointer to the DTD to search
 * @name:  The entity name
 *
 * Do an entity lookup in the DTD pararmeter entity hash table and
 * return the corresponding entity, if found.
 *
 * Returns A pointer to the entity structure or NULL if not found.
 */
static xmlEntity * xmlGetParameterEntityFromDtd(const xmlDtd * dtd, const xmlChar * name)
{
	if(dtd && dtd->pentities) {
		xmlEntitiesTable * table = static_cast<xmlEntitiesTable *>(dtd->pentities);
		return static_cast<xmlEntity *>(xmlHashLookup(table, name));
		// return (xmlGetEntityFromTable(table, name)); 
	}
	else
		return 0;
}

#endif /* LIBXML_TREE_ENABLED */
// 
// QName handling helper
// 
/**
 * xmlBuildQName:
 * @ncname:  the Name
 * @prefix:  the prefix
 * @memory:  preallocated memory
 * @len:  preallocated memory length
 *
 * Builds the QName @prefix:@ncname in @memory if there is enough space
 * and prefix is not NULL nor empty, otherwise allocate a new string.
 * If prefix is NULL or empty it returns ncname.
 *
 * Returns the new string which must be freed by the caller if different from
 *    @memory and @ncname or NULL in case of error
 */
xmlChar * FASTCALL xmlBuildQName(const xmlChar * ncname, const xmlChar * prefix, xmlChar * memory, int len)
{
	xmlChar * ret = 0;
	if(ncname) {
		if(!prefix)
			ret = (xmlChar *)ncname;
		else {
			const int lenn = sstrlen(ncname);
			const int lenp = sstrlen(prefix);
			if(!memory || len < (lenn + lenp + 2)) {
				ret = static_cast<xmlChar *>(SAlloc::M(lenn + lenp + 2));
				if(!ret) {
					xmlTreeErrMemory("building QName");
					return 0;
				}
			}
			else
				ret = memory;
			memcpy(&ret[0], prefix, lenp);
			ret[lenp] = ':';
			memcpy(&ret[lenp + 1], ncname, lenn);
			ret[lenn + lenp + 1] = 0;
		}
	}
	return ret;
}
/**
 * xmlSplitQName2:
 * @name:  the full QName
 * @prefix:  a xmlChar **
 *
 * parse an XML qualified name string
 *
 * [NS 5] QName ::= (Prefix ':')? LocalPart
 *
 * [NS 6] Prefix ::= NCName
 *
 * [NS 7] LocalPart ::= NCName
 *
 * Returns NULL if not a QName, otherwise the local part, and prefix
 * is updated to get the Prefix if any.
 */
xmlChar * FASTCALL xmlSplitQName2(const xmlChar * name, xmlChar ** prefix)
{
	xmlChar * ret = NULL;
	if(prefix) {
		*prefix = NULL;
		if(name) {
#ifndef XML_XML_NAMESPACE
			// xml: prefix is not really a namespace 
			if((name[0] == 'x') && (name[1] == 'm') && (name[2] == 'l') && (name[3] == ':'))
				return 0;
#endif
			if(name[0] != ':') { // nasty but valid 
				// 
				// we are not trying to validate but just to cut, and yes it will
				// work even if this is as set of UTF-8 encoded chars
				// 
				int len = 0;
				while(name[len] && name[len] != ':')
					len++;
				if(name[len]) {
					*prefix = xmlStrndup(name, len);
					if(*prefix == NULL)
						xmlTreeErrMemory("QName split");
					else {
						ret = sstrdup(&name[len + 1]);
						if(!ret) {
							xmlTreeErrMemory("QName split");
							ZFREE(*prefix);
						}
					}
				}
			}
		}
	}
	return ret;
}
/**
 * xmlSplitQName3:
 * @name:  the full QName
 * @len: an int *
 *
 * parse an XML qualified name string,i
 *
 * returns NULL if it is not a Qualified Name, otherwise, update len
 *    with the length in byte of the prefix and return a pointer
 *    to the start of the name without the prefix
 */
const xmlChar * xmlSplitQName3(const xmlChar * name, int * len) 
{
	int l = 0;
	if(!name || !len) 
		return 0;
	// nasty but valid 
	if(name[0] == ':')
		return 0;
	/*
	 * we are not trying to validate but just to cut, and yes it will
	 * work even if this is as set of UTF-8 encoded chars
	 */
	while((name[l] != 0) && (name[l] != ':'))
		l++;
	if(name[l] == 0)
		return 0;
	*len = l;
	return &name[l+1];
}
// 
// Check Name, NCName and QName strings
// 
#define CUR_SCHAR(s, l) xmlStringCurrentChar(NULL, s, &l)

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_DEBUG_ENABLED) || \
	defined (LIBXML_HTML_ENABLED) || defined(LIBXML_SAX1_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_WRITER_ENABLED) || \
	defined(LIBXML_DOCB_ENABLED) || defined(LIBXML_LEGACY_ENABLED)
/**
 * xmlValidateNCName:
 * @value: the value to check
 * @space: allow spaces in front and end of the string
 *
 * Check that a value conforms to the lexical space of NCName
 *
 * Returns 0 if this validates, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int xmlValidateNCName(const xmlChar * value, int space) 
{
	const xmlChar * cur = value;
	int c, l;
	if(!value)
		return -1;
	/*
	 * First quick algorithm for ASCII range
	 */
	if(space)
		while(IS_BLANK_CH(*cur)) 
			cur++;
	if(((*cur >= 'a') && (*cur <= 'z')) || ((*cur >= 'A') && (*cur <= 'Z')) || (*cur == '_'))
		cur++;
	else
		goto try_complex;
	while(((*cur >= 'a') && (*cur <= 'z')) || ((*cur >= 'A') && (*cur <= 'Z')) ||
	    ((*cur >= '0') && (*cur <= '9')) || (*cur == '_') || (*cur == '-') || (*cur == '.'))
		cur++;
	if(space)
		while(IS_BLANK_CH(*cur)) 
			cur++;
	if(*cur == 0)
		return 0;
try_complex:
	/*
	 * Second check for chars outside the ASCII range
	 */
	cur = value;
	c = CUR_SCHAR(cur, l);
	if(space) {
		while(IS_BLANK(c)) {
			cur += l;
			c = CUR_SCHAR(cur, l);
		}
	}
	if((!IS_LETTER(c)) && (c != '_'))
		return 1;
	cur += l;
	c = CUR_SCHAR(cur, l);
	while(IS_LETTER(c) || IS_DIGIT(c) || (c == '.') || (c == '-') || (c == '_') || IS_COMBINING(c) || IS_EXTENDER(c)) {
		cur += l;
		c = CUR_SCHAR(cur, l);
	}
	if(space) {
		while(IS_BLANK(c)) {
			cur += l;
			c = CUR_SCHAR(cur, l);
		}
	}
	return (c != 0) ? 1 : 0;
}

#endif

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
/**
 * xmlValidateQName:
 * @value: the value to check
 * @space: allow spaces in front and end of the string
 *
 * Check that a value conforms to the lexical space of QName
 *
 * Returns 0 if this validates, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int xmlValidateQName(const xmlChar * value, int space) 
{
	const xmlChar * cur = value;
	int c, l;
	if(!value)
		return -1;
	/*
	 * First quick algorithm for ASCII range
	 */
	if(space)
		while(IS_BLANK_CH(*cur)) 
			cur++;
	if(((*cur >= 'a') && (*cur <= 'z')) || ((*cur >= 'A') && (*cur <= 'Z')) || (*cur == '_'))
		cur++;
	else
		goto try_complex;
	while(((*cur >= 'a') && (*cur <= 'z')) || ((*cur >= 'A') && (*cur <= 'Z')) || ((*cur >= '0') && (*cur <= '9')) ||
	    (*cur == '_') || (*cur == '-') || (*cur == '.'))
		cur++;
	if(*cur == ':') {
		cur++;
		if(((*cur >= 'a') && (*cur <= 'z')) || ((*cur >= 'A') && (*cur <= 'Z')) || (*cur == '_'))
			cur++;
		else
			goto try_complex;
		while(((*cur >= 'a') && (*cur <= 'z')) || ((*cur >= 'A') && (*cur <= 'Z')) ||
		    ((*cur >= '0') && (*cur <= '9')) || (*cur == '_') || (*cur == '-') || (*cur == '.'))
			cur++;
	}
	if(space)
		while(IS_BLANK_CH(*cur)) 
			cur++;
	if(*cur == 0)
		return 0;
try_complex:
	/*
	 * Second check for chars outside the ASCII range
	 */
	cur = value;
	c = CUR_SCHAR(cur, l);
	if(space) {
		while(IS_BLANK(c)) {
			cur += l;
			c = CUR_SCHAR(cur, l);
		}
	}
	if((!IS_LETTER(c)) && (c != '_'))
		return 1;
	cur += l;
	c = CUR_SCHAR(cur, l);
	while(IS_LETTER(c) || IS_DIGIT(c) || (c == '.') || (c == '-') || (c == '_') || IS_COMBINING(c) || IS_EXTENDER(c)) {
		cur += l;
		c = CUR_SCHAR(cur, l);
	}
	if(c == ':') {
		cur += l;
		c = CUR_SCHAR(cur, l);
		if((!IS_LETTER(c)) && (c != '_'))
			return 1;
		cur += l;
		c = CUR_SCHAR(cur, l);
		while(IS_LETTER(c) || IS_DIGIT(c) || (c == '.') || (c == '-') || (c == '_') || IS_COMBINING(c) || IS_EXTENDER(c)) {
			cur += l;
			c = CUR_SCHAR(cur, l);
		}
	}
	if(space) {
		while(IS_BLANK(c)) {
			cur += l;
			c = CUR_SCHAR(cur, l);
		}
	}
	return (c != 0) ? 1 : 0;
}
/**
 * xmlValidateName:
 * @value: the value to check
 * @space: allow spaces in front and end of the string
 *
 * Check that a value conforms to the lexical space of Name
 *
 * Returns 0 if this validates, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int xmlValidateName(const xmlChar * value, int space) 
{
	const xmlChar * cur = value;
	int c, l;
	if(!value)
		return -1;
	/*
	 * First quick algorithm for ASCII range
	 */
	if(space)
		while(IS_BLANK_CH(*cur)) cur++;
	if(((*cur >= 'a') && (*cur <= 'z')) || ((*cur >= 'A') && (*cur <= 'Z')) || (*cur == '_') || (*cur == ':'))
		cur++;
	else
		goto try_complex;
	while(((*cur >= 'a') && (*cur <= 'z')) || ((*cur >= 'A') && (*cur <= 'Z')) || ((*cur >= '0') && (*cur <= '9')) ||
	    (*cur == '_') || (*cur == '-') || (*cur == '.') || (*cur == ':'))
		cur++;
	if(space)
		while(IS_BLANK_CH(*cur)) 
			cur++;
	if(*cur == 0)
		return 0;
try_complex:
	/*
	 * Second check for chars outside the ASCII range
	 */
	cur = value;
	c = CUR_SCHAR(cur, l);
	if(space) {
		while(IS_BLANK(c)) {
			cur += l;
			c = CUR_SCHAR(cur, l);
		}
	}
	if((!IS_LETTER(c)) && (c != '_') && (c != ':'))
		return 1;
	cur += l;
	c = CUR_SCHAR(cur, l);
	while(IS_LETTER(c) || IS_DIGIT(c) || oneof4(c, '.', ':', '-', '_') || IS_COMBINING(c) || IS_EXTENDER(c)) {
		cur += l;
		c = CUR_SCHAR(cur, l);
	}
	if(space) {
		while(IS_BLANK(c)) {
			cur += l;
			c = CUR_SCHAR(cur, l);
		}
	}
	return (c != 0) ? 1 : 0;
}
/**
 * xmlValidateNMToken:
 * @value: the value to check
 * @space: allow spaces in front and end of the string
 *
 * Check that a value conforms to the lexical space of NMToken
 *
 * Returns 0 if this validates, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int xmlValidateNMToken(const xmlChar * value, int space) 
{
	const xmlChar * cur = value;
	int c, l;
	if(!value)
		return -1;
	/*
	 * First quick algorithm for ASCII range
	 */
	if(space)
		while(IS_BLANK_CH(*cur)) 
			cur++;
	if(((*cur >= 'a') && (*cur <= 'z')) || ((*cur >= 'A') && (*cur <= 'Z')) || ((*cur >= '0') && (*cur <= '9')) || (*cur == '_') || (*cur == '-') || (*cur == '.') || (*cur == ':'))
		cur++;
	else
		goto try_complex;
	while(((*cur >= 'a') && (*cur <= 'z')) || ((*cur >= 'A') && (*cur <= 'Z')) || ((*cur >= '0') && (*cur <= '9')) || (*cur == '_') || (*cur == '-') || (*cur == '.') || (*cur == ':'))
		cur++;
	if(space)
		while(IS_BLANK_CH(*cur)) cur++;
	if(*cur == 0)
		return 0;
try_complex:
	/*
	 * Second check for chars outside the ASCII range
	 */
	cur = value;
	c = CUR_SCHAR(cur, l);
	if(space) {
		while(IS_BLANK(c)) {
			cur += l;
			c = CUR_SCHAR(cur, l);
		}
	}
	if(!(IS_LETTER(c) || IS_DIGIT(c) || (c == '.') || (c == ':') || (c == '-') || (c == '_') || IS_COMBINING(c) || IS_EXTENDER(c)))
		return 1;
	cur += l;
	c = CUR_SCHAR(cur, l);
	while(IS_LETTER(c) || IS_DIGIT(c) || (c == '.') || (c == ':') || (c == '-') || (c == '_') || IS_COMBINING(c) || IS_EXTENDER(c)) {
		cur += l;
		c = CUR_SCHAR(cur, l);
	}
	if(space) {
		while(IS_BLANK(c)) {
			cur += l;
			c = CUR_SCHAR(cur, l);
		}
	}
	return (c != 0) ? 1 : 0;
}

#endif /* LIBXML_TREE_ENABLED */
// 
// Allocation and deallocation of basic structures
// 
/**
 * xmlSetBufferAllocationScheme:
 * @scheme:  allocation method to use
 *
 * Set the buffer allocation method.  Types are
 * XML_BUFFER_ALLOC_EXACT - use exact sizes, keeps memory usage down
 * XML_BUFFER_ALLOC_DOUBLEIT - double buffer when extra needed,
 *                        improves performance
 */
void xmlSetBufferAllocationScheme(xmlBufferAllocationScheme scheme) 
{
	if(oneof3(scheme, XML_BUFFER_ALLOC_EXACT, XML_BUFFER_ALLOC_DOUBLEIT, XML_BUFFER_ALLOC_HYBRID))
		xmlBufferAllocScheme = scheme;
}
/**
 * xmlGetBufferAllocationScheme:
 *
 * Types are
 * XML_BUFFER_ALLOC_EXACT - use exact sizes, keeps memory usage down
 * XML_BUFFER_ALLOC_DOUBLEIT - double buffer when extra needed,
 *                        improves performance
 * XML_BUFFER_ALLOC_HYBRID - use exact sizes on small strings to keep memory usage tight
 *                       in normal usage, and doubleit on large strings to avoid
 *                       pathological performance.
 *
 * Returns the current allocation scheme
 */
xmlBufferAllocationScheme xmlGetBufferAllocationScheme() 
{
	return xmlBufferAllocScheme;
}
/**
 * xmlNewNs:
 * @node:  the element carrying the namespace
 * @href:  the URI associated
 * @prefix:  the prefix for the namespace
 *
 * Creation of a new Namespace. This function will refuse to create
 * a namespace with a similar prefix than an existing one present on this
 * node.
 * Note that for a default namespace, @prefix should be NULL.
 *
 * We use href==NULL in the case of an element creation where the namespace
 * was not defined.
 *
 * Returns a new namespace pointer or NULL
 */
xmlNs * FASTCALL xmlNewNs(xmlNode * P_Node, const xmlChar * href, const xmlChar * prefix) 
{
	xmlNs * cur;
	if(P_Node && P_Node->type != XML_ELEMENT_NODE)
		return 0;
	if(prefix && (sstreq(prefix, reinterpret_cast<const xmlChar *>("xml")))) {
		/* xml namespace is predefined, no need to add it */
		if(sstreq(href, XML_XML_NAMESPACE))
			return 0;
		/*
		 * Problem, this is an attempt to bind xml prefix to a wrong
		 * namespace, which breaks
		 * Namespace constraint: Reserved Prefixes and Namespace Names
		 * from XML namespace. But documents authors may not care in
		 * their context so let's proceed.
		 */
	}
	/*
	 * Allocate a new Namespace and fill the fields.
	 */
	cur = static_cast<xmlNs *>(SAlloc::M(sizeof(xmlNs)));
	if(!cur) {
		xmlTreeErrMemory("building namespace");
		return 0;
	}
	memzero(cur, sizeof(xmlNs));
	cur->type = XML_LOCAL_NAMESPACE;
	cur->href = sstrdup(href);
	cur->prefix = sstrdup(prefix);
	/*
	 * Add it at the end to preserve parsing order ...
	 * and checks for existing use of the prefix
	 */
	if(P_Node) {
		if(P_Node->nsDef == NULL) {
			P_Node->nsDef = cur;
		}
		else {
			xmlNs * prev = P_Node->nsDef;
			if((!prev->prefix && !cur->prefix) || sstreq(prev->prefix, cur->prefix)) {
				xmlFreeNs(cur);
				return 0;
			}
			else {
				while(prev->next) {
					prev = prev->next;
					if((!prev->prefix && !cur->prefix) || sstreq(prev->prefix, cur->prefix)) {
						xmlFreeNs(cur);
						return 0;
					}
				}
				prev->next = cur;
			}
		}
	}
	return cur;
}
/**
 * xmlSetNs:
 * @node:  a node in the document
 * @ns:  a namespace pointer
 *
 * Associate a namespace to a node, a posteriori.
 */
void xmlSetNs(xmlNode * P_Node, xmlNs * ns) 
{
	if(!P_Node) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlSetNs: node == NULL\n");
#endif
		return;
	}
	if(oneof2(P_Node->type, XML_ELEMENT_NODE, XML_ATTRIBUTE_NODE))
		P_Node->ns = ns;
}
/**
 * xmlFreeNs:
 * @cur:  the namespace pointer
 *
 * Free up the structures associated to a namespace
 */
void FASTCALL xmlFreeNs(xmlNs * cur)
{
	if(!cur) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlFreeNs : ns == NULL\n");
#endif
		return;
	}
	SAlloc::F((char *)cur->href);
	SAlloc::F((char *)cur->prefix);
	SAlloc::F(cur);
}
/**
 * xmlFreeNsList:
 * @cur:  the first namespace pointer
 *
 * Free up all the structures associated to the chained namespaces.
 */
void FASTCALL xmlFreeNsList(xmlNs * cur)
{
	xmlNs * next;
	if(!cur) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlFreeNsList : ns == NULL\n");
#endif
		return;
	}
	while(cur) {
		next = cur->next;
		xmlFreeNs(cur);
		cur = next;
	}
}
/**
 * xmlNewDtd:
 * @doc:  the document pointer
 * @name:  the DTD name
 * @ExternalID:  the external ID
 * @SystemID:  the system ID
 *
 * Creation of a new DTD for the external subset. To create an
 * internal subset, use xmlCreateIntSubset().
 *
 * Returns a pointer to the new DTD structure
 */
xmlDtd * xmlNewDtd(xmlDoc * doc, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID) 
{
	xmlDtd * cur = 0;
	if(doc && doc->extSubset) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNewDtd(%s): document %s already have a DTD %s\n", /* !!! */ (char *)name, doc->name, /* !!! */ (char *)doc->extSubset->name);
#endif
	}
	else {
		// 
		// Allocate a new DTD and fill the fields.
		// 
		cur = (xmlDtd *)SAlloc::M(sizeof(xmlDtd));
		if(!cur)
			xmlTreeErrMemory("building DTD");
		else {
			memzero(cur, sizeof(xmlDtd));
			cur->type = XML_DTD_NODE;
			cur->name = sstrdup(name);
			cur->ExternalID = sstrdup(ExternalID);
			cur->SystemID = sstrdup(SystemID);
			if(doc)
				doc->extSubset = cur;
			cur->doc = doc;
			if(__xmlRegisterCallbacks && xmlRegisterNodeDefaultValue)
				xmlRegisterNodeDefaultValue((xmlNode *)cur);
		}
	}
	return cur;
}
/**
 * xmlGetIntSubset:
 * @doc:  the document pointer
 *
 * Get the internal subset of a document
 * Returns a pointer to the DTD structure or NULL if not found
 */
xmlDtd * FASTCALL xmlGetIntSubset(const xmlDoc * doc)
{
	if(!doc)
		return 0;
    else {
        for(xmlNode * cur = doc->children; cur; cur = cur->next) {
            if(cur->type == XML_DTD_NODE)
                return (xmlDtd *)cur;
        }
        return (xmlDtd *)doc->intSubset;
    }
}
/**
 * xmlCreateIntSubset:
 * @doc:  the document pointer
 * @name:  the DTD name
 * @ExternalID:  the external (PUBLIC) ID
 * @SystemID:  the system ID
 *
 * Create the internal subset of a document
 * Returns a pointer to the new DTD structure
 */
xmlDtd * xmlCreateIntSubset(xmlDoc * doc, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID)
{
	xmlDtd * cur;
	if(doc && xmlGetIntSubset(doc)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlCreateIntSubset(): document %s already have an internal subset\n", doc->name);
#endif
		return 0;
	}
	/*
	 * Allocate a new DTD and fill the fields.
	 */
	cur = static_cast<xmlDtd *>(SAlloc::M(sizeof(xmlDtd)));
	if(!cur) {
		xmlTreeErrMemory("building internal subset");
		return 0;
	}
	memzero(cur, sizeof(xmlDtd));
	cur->type = XML_DTD_NODE;
	if(name) {
		cur->name = sstrdup(name);
		if(!cur->name) {
			xmlTreeErrMemory("building internal subset");
			SAlloc::F(cur);
			return 0;
		}
	}
	if(ExternalID) {
		cur->ExternalID = sstrdup(ExternalID);
		if(!cur->ExternalID) {
			xmlTreeErrMemory("building internal subset");
			SAlloc::F((void *)cur->name); // @badcast
			SAlloc::F(cur);
			return 0;
		}
	}
	if(SystemID) {
		cur->SystemID = sstrdup(SystemID);
		if(!cur->SystemID) {
			xmlTreeErrMemory("building internal subset");
			SAlloc::F(const_cast<xmlChar *>(cur->name)); // @badcast
			SAlloc::F(const_cast<xmlChar *>(cur->ExternalID)); // @badcast
			SAlloc::F(cur);
			return 0;
		}
	}
	if(doc) {
		doc->intSubset = cur;
		cur->P_ParentNode = reinterpret_cast<xmlNode *>(doc);
		cur->doc = doc;
		if(doc->children == NULL) {
			doc->children = reinterpret_cast<xmlNode *>(cur);
			doc->last = reinterpret_cast<xmlNode *>(cur);
		}
		else {
			if(doc->type == XML_HTML_DOCUMENT_NODE) {
				xmlNode * prev = doc->children;
				prev->prev = (xmlNode *)cur;
				cur->next = prev;
				doc->children = (xmlNode *)cur;
			}
			else {
				xmlNode * next = doc->children;
				while(next && (next->type != XML_ELEMENT_NODE))
					next = next->next;
				if(!next) {
					cur->prev = doc->last;
					cur->prev->next = (xmlNode *)cur;
					cur->next = NULL;
					doc->last = (xmlNode *)cur;
				}
				else {
					cur->next = next;
					cur->prev = next->prev;
					if(cur->prev == NULL)
						doc->children = (xmlNode *)cur;
					else
						cur->prev->next = (xmlNode *)cur;
					next->prev = (xmlNode *)cur;
				}
			}
		}
	}
	if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
		xmlRegisterNodeDefaultValue((xmlNode *)cur);
	return cur;
}
/**
 * @str:  a string
 *
 * Free a string if it is not owned by the "dict" dictionnary in the
 * current scope
 */
//#define DICT_FREE(p_dict__, str) if((str) && ((!p_dict__) || (xmlDictOwns(p_dict__, (const xmlChar *)(str)) == 0))) SAlloc::F((char *)(str));
/**
 * DICT_COPY:
 * @str:  a string
 *
 * Copy a string using a "dict" dictionnary in the current scope,
 * if availabe.
 */
#define DICT_COPY(str, cpy) \
	if(str) { \
		if(dict) { \
			cpy = xmlDictOwns(dict, (const xmlChar *)(str)) ? (xmlChar *)(str) : (xmlChar *)xmlDictLookupSL((dict), (const xmlChar *)(str)); \
		} else \
			cpy = sstrdup((const xmlChar *)(str)); }

/**
 * DICT_CONST_COPY:
 * @str:  a string
 *
 * Copy a string using a "dict" dictionnary in the current scope,
 * if availabe.
 */
#define DICT_CONST_COPY(str, cpy) \
	if(str) { if(dict) { cpy = xmlDictOwns(dict, (const xmlChar *)(str)) ? (const xmlChar *)(str) : xmlDictLookupSL((dict), (const xmlChar *)(str)); } \
	else cpy = sstrdup((const xmlChar *)(str)); }

/**
 * xmlFreeDtd:
 * @cur:  the DTD structure to free up
 *
 * Free a DTD structure.
 */
void FASTCALL xmlFreeDtd(xmlDtd * pCur) 
{
	if(pCur) {
		xmlDict * p_dict = pCur->doc ? pCur->doc->dict : 0;
		if(__xmlRegisterCallbacks && (xmlDeregisterNodeDefaultValue))
			xmlDeregisterNodeDefaultValue((xmlNode *)pCur);
		if(pCur->children) {
			// 
			// Cleanup all nodes which are not part of the specific lists
			// of notations, elements, attributes and entities.
			// 
			for(xmlNode * c = pCur->children; c;) {
				xmlNode * next = c->next;
				if(!oneof4(c->type, XML_NOTATION_NODE, XML_ELEMENT_DECL, XML_ATTRIBUTE_DECL, XML_ENTITY_DECL)) {
					xmlUnlinkNode(c);
					xmlFreeNode(c);
				}
				c = next;
			}
		}
		XmlDestroyStringWithDict(p_dict, (xmlChar *)pCur->name); // @badcast
		XmlDestroyStringWithDict(p_dict, (xmlChar *)pCur->SystemID); // @badcast
		XmlDestroyStringWithDict(p_dict, (xmlChar *)pCur->ExternalID); // @badcast
		/* @todo !!! */
		xmlFreeNotationTable((xmlNotationTable *)pCur->notations);
		xmlFreeElementTable((xmlElementTable *)pCur->elements);
		xmlFreeAttributeTable((xmlAttributeTable *)pCur->attributes);
		xmlFreeEntitiesTable((xmlEntitiesTable *)pCur->entities);
		xmlFreeEntitiesTable((xmlEntitiesTable *)pCur->pentities);
		SAlloc::F(pCur);
	}
}
/**
 * xmlNewDoc:
 * @version:  xmlChar string giving the version of XML "1.0"
 *
 * Creates a new XML document
 *
 * Returns a new document
 */
xmlDoc * xmlNewDoc(const xmlChar * version)
{
	SETIFZ(version, (const xmlChar *)"1.0");
	/*
	 * Allocate a new document and fill the fields.
	 */
	xmlDoc * cur = (xmlDoc *)SAlloc::M(sizeof(xmlDoc));
	if(!cur) {
		xmlTreeErrMemory("building doc");
	}
	else {
		memzero(cur, sizeof(xmlDoc));
		cur->type = XML_DOCUMENT_NODE;
		cur->version = sstrdup(version);
		if(cur->version == NULL) {
			xmlTreeErrMemory("building doc");
			ZFREE(cur);
		}
		else {
			cur->standalone = -1;
			cur->compression = -1; /* not initialized */
			cur->doc = cur;
			cur->parseFlags = 0;
			cur->properties = XML_DOC_USERBUILT;
			/*
			 * The in memory encoding is always UTF8
			 * This field will never change and would be obsolete if not for binary compatibility.
			 */
			cur->charset = XML_CHAR_ENCODING_UTF8;
			if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
				xmlRegisterNodeDefaultValue((xmlNode *)cur);
		}
	}
	return cur;
}
/**
 * @cur:  pointer to the document
 *
 * Free up all the structures used by a document, tree included.
 */
void FASTCALL xmlFreeDoc(xmlDoc * pDoc)
{
	xmlDtd * extSubset;
	xmlDtd * intSubset;
	xmlDict * p_dict = NULL;
	if(pDoc) {
#ifdef LIBXML_DEBUG_RUNTIME
	#ifdef LIBXML_DEBUG_ENABLED
		xmlDebugCheckDocument(stderr, cur);
	#endif
#endif
		p_dict = pDoc->dict;
		if(__xmlRegisterCallbacks && xmlDeregisterNodeDefaultValue)
			xmlDeregisterNodeDefaultValue((xmlNode *)pDoc);
		// 
		// Do this before freeing the children list to avoid ID lookups
		// 
		xmlFreeIDTable((xmlIDTable *)pDoc->ids);
		pDoc->ids = NULL;
		xmlFreeRefTable((xmlRefTable *)pDoc->refs);
		pDoc->refs = NULL;
		extSubset = pDoc->extSubset;
		intSubset = pDoc->intSubset;
		if(intSubset == extSubset)
			extSubset = NULL;
		if(extSubset) {
			xmlUnlinkNode((xmlNode *)pDoc->extSubset);
			pDoc->extSubset = NULL;
			xmlFreeDtd(extSubset);
		}
		if(intSubset) {
			xmlUnlinkNode((xmlNode *)pDoc->intSubset);
			pDoc->intSubset = NULL;
			xmlFreeDtd(intSubset);
		}
		xmlFreeNodeList(pDoc->children);
		if(pDoc->oldNs)
			xmlFreeNsList(pDoc->oldNs);
		XmlDestroyStringWithDict(p_dict, (xmlChar *)pDoc->version); // @badcast
		XmlDestroyStringWithDict(p_dict, (xmlChar *)pDoc->name); // @badcast
		XmlDestroyStringWithDict(p_dict, (xmlChar *)pDoc->encoding); // @badcast
		XmlDestroyStringWithDict(p_dict, (xmlChar *)pDoc->URL); // @badcast
		SAlloc::F(pDoc);
		xmlDictFree(p_dict);
	}
}

/**
 * xmlStringLenGetNodeList:
 * @doc:  the document
 * @value:  the value of the text
 * @len:  the length of the string value
 *
 * Parse the value string and build the node list associated. Should
 * produce a flat tree with only TEXTs and ENTITY_REFs.
 * Returns a pointer to the first child
 */
xmlNode * xmlStringLenGetNodeList(const xmlDoc * doc, const xmlChar * value, int len)
{
	xmlNode * ret = NULL;
	xmlNode * last = NULL;
	xmlNode * P_Node;
	xmlChar * val;
	const xmlChar * cur = value, * end = cur + len;
	const xmlChar * q;
	xmlEntity * ent;
	xmlBufPtr buf;
	if(!value)
		return 0;
	buf = xmlBufCreateSize(0);
	if(!buf)
		return 0;
	xmlBufSetAllocationScheme(buf, XML_BUFFER_ALLOC_HYBRID);
	q = cur;
	while((cur < end) && (*cur != 0)) {
		if(cur[0] == '&') {
			int charval = 0;
			xmlChar tmp;
			/*
			 * Save the current text.
			 */
			if(cur != q) {
				if(xmlBufAdd(buf, q, cur - q))
					goto out;
			}
			q = cur;
			if((cur + 2 < end) && (cur[1] == '#') && (cur[2] == 'x')) {
				cur += 3;
				tmp = (cur < end) ? *cur : 0;
				while(tmp != ';') { /* Non input consuming loop */
					if((tmp >= '0') && (tmp <= '9'))
						charval = charval * 16 + (tmp - '0');
					else if((tmp >= 'a') && (tmp <= 'f'))
						charval = charval * 16 + (tmp - 'a') + 10;
					else if((tmp >= 'A') && (tmp <= 'F'))
						charval = charval * 16 + (tmp - 'A') + 10;
					else {
						xmlTreeErr(XML_TREE_INVALID_HEX, (xmlNode *)doc, NULL);
						charval = 0;
						break;
					}
					cur++;
					if(cur < end)
						tmp = *cur;
					else
						tmp = 0;
				}
				if(tmp == ';')
					cur++;
				q = cur;
			}
			else if((cur + 1 < end) && (cur[1] == '#')) {
				cur += 2;
				tmp = (cur < end) ? *cur : 0;
				while(tmp != ';') { /* Non input consuming loops */
					if((tmp >= '0') && (tmp <= '9'))
						charval = charval * 10 + (tmp - '0');
					else {
						xmlTreeErr(XML_TREE_INVALID_DEC, (xmlNode *)doc, NULL);
						charval = 0;
						break;
					}
					cur++;
					tmp = (cur < end) ? *cur : 0;
				}
				if(tmp == ';')
					cur++;
				q = cur;
			}
			else {
				/*
				 * Read the entity string
				 */
				cur++;
				q = cur;
				while((cur < end) && (*cur != 0) && (*cur != ';')) cur++;
				if((cur >= end) || (*cur == 0)) {
					xmlTreeErr(XML_TREE_UNTERMINATED_ENTITY, (xmlNode *)doc, (const char *)q);
					goto out;
				}
				if(cur != q) {
					/*
					 * Predefined entities don't generate nodes
					 */
					val = xmlStrndup(q, cur - q);
					ent = xmlGetDocEntity(doc, val);
					if(ent && (ent->etype == XML_INTERNAL_PREDEFINED_ENTITY)) {
						if(xmlBufCat(buf, ent->content))
							goto out;
					}
					else {
						/*
						 * Flush buffer so far
						 */
						if(!xmlBufIsEmpty(buf)) {
							P_Node = xmlNewDocText(doc, 0);
							if(!P_Node) {
								SAlloc::F(val);
								goto out;
							}
							P_Node->content = xmlBufDetach(buf);
							if(last == NULL) {
								last = ret = P_Node;
							}
							else {
								last = xmlAddNextSibling(last, P_Node);
							}
						}
						/*
						 * Create a new REFERENCE_REF node
						 */
						P_Node = xmlNewReference(doc, val);
						if(!P_Node) {
							SAlloc::F(val);
							goto out;
						}
						else if(ent && (ent->children == NULL)) {
							xmlNode * temp;
							ent->children = xmlStringGetNodeList(doc, (const xmlChar *)P_Node->content);
							ent->owner = 1;
							temp = ent->children;
							while(temp) {
								temp->P_ParentNode = reinterpret_cast<xmlNode *>(ent);
								ent->last = temp;
								temp = temp->next;
							}
						}
						if(last == NULL) {
							last = ret = P_Node;
						}
						else {
							last = xmlAddNextSibling(last, P_Node);
						}
					}
					SAlloc::F(val);
				}
				cur++;
				q = cur;
			}
			if(charval != 0) {
				xmlChar buffer[10];
				int l = xmlCopyCharMultiByte(buffer, charval);
				buffer[l] = 0;
				if(xmlBufCat(buf, buffer))
					goto out;
				charval = 0;
			}
		}
		else
			cur++;
	}
	if(cur != q) {
		/*
		 * Handle the last piece of text.
		 */
		if(xmlBufAdd(buf, q, cur - q))
			goto out;
	}
	if(!xmlBufIsEmpty(buf)) {
		P_Node = xmlNewDocText(doc, 0);
		if(!P_Node)
			goto out;
		P_Node->content = xmlBufDetach(buf);
		if(last == NULL) {
			last = ret = P_Node;
		}
		else {
			last = xmlAddNextSibling(last, P_Node);
		}
	}
	else if(!ret) {
		ret = xmlNewDocText(doc, reinterpret_cast<const xmlChar *>(""));
	}
out:
	xmlBufFree(buf);
	return ret;
}
/**
 * xmlStringGetNodeList:
 * @doc:  the document
 * @value:  the value of the attribute
 *
 * Parse the value string and build the node list associated. Should
 * produce a flat tree with only TEXTs and ENTITY_REFs.
 * Returns a pointer to the first child
 */
xmlNode * xmlStringGetNodeList(const xmlDoc * doc, const xmlChar * value)
{
	xmlNode * ret = NULL;
	xmlNode * last = NULL;
	xmlNode * P_Node;
	xmlChar * val;
	const xmlChar * cur = value;
	const xmlChar * q;
	xmlEntity * ent;
	xmlBufPtr buf;
	if(!value)
		return 0;
	buf = xmlBufCreateSize(0);
	if(!buf)
		return 0;
	xmlBufSetAllocationScheme(buf, XML_BUFFER_ALLOC_HYBRID);
	q = cur;
	while(*cur != 0) {
		if(cur[0] == '&') {
			int charval = 0;
			xmlChar tmp;
			/*
			 * Save the current text.
			 */
			if(cur != q) {
				if(xmlBufAdd(buf, q, cur - q))
					goto out;
			}
			q = cur;
			if((cur[1] == '#') && (cur[2] == 'x')) {
				cur += 3;
				tmp = *cur;
				while(tmp != ';') { /* Non input consuming loop */
					if((tmp >= '0') && (tmp <= '9'))
						charval = charval * 16 + (tmp - '0');
					else if((tmp >= 'a') && (tmp <= 'f'))
						charval = charval * 16 + (tmp - 'a') + 10;
					else if((tmp >= 'A') && (tmp <= 'F'))
						charval = charval * 16 + (tmp - 'A') + 10;
					else {
						xmlTreeErr(XML_TREE_INVALID_HEX, (xmlNode *)doc, NULL);
						charval = 0;
						break;
					}
					cur++;
					tmp = *cur;
				}
				if(tmp == ';')
					cur++;
				q = cur;
			}
			else if(cur[1] == '#') {
				cur += 2;
				tmp = *cur;
				while(tmp != ';') { /* Non input consuming loops */
					if((tmp >= '0') && (tmp <= '9'))
						charval = charval * 10 + (tmp - '0');
					else {
						xmlTreeErr(XML_TREE_INVALID_DEC, (xmlNode *)doc, NULL);
						charval = 0;
						break;
					}
					cur++;
					tmp = *cur;
				}
				if(tmp == ';')
					cur++;
				q = cur;
			}
			else {
				/*
				 * Read the entity string
				 */
				cur++;
				q = cur;
				while((*cur != 0) && (*cur != ';')) 
					cur++;
				if(*cur == 0) {
					xmlTreeErr(XML_TREE_UNTERMINATED_ENTITY, (xmlNode *)doc, (const char *)q);
					goto out;
				}
				if(cur != q) {
					/*
					 * Predefined entities don't generate nodes
					 */
					val = xmlStrndup(q, cur - q);
					ent = xmlGetDocEntity(doc, val);
					if(ent && (ent->etype == XML_INTERNAL_PREDEFINED_ENTITY)) {
						if(xmlBufCat(buf, ent->content))
							goto out;
					}
					else {
						/*
						 * Flush buffer so far
						 */
						if(!xmlBufIsEmpty(buf)) {
							P_Node = xmlNewDocText(doc, 0);
							P_Node->content = xmlBufDetach(buf);
							if(!last) {
								last = ret = P_Node;
							}
							else {
								last = xmlAddNextSibling(last, P_Node);
							}
						}
						/*
						 * Create a new REFERENCE_REF node
						 */
						P_Node = xmlNewReference(doc, val);
						if(!P_Node) {
							SAlloc::F(val);
							goto out;
						}
						else if(ent && ent->children == NULL) {
							xmlNode * temp;
							ent->children = xmlStringGetNodeList(doc, (const xmlChar *)P_Node->content);
							ent->owner = 1;
							temp = ent->children;
							while(temp) {
								temp->P_ParentNode = reinterpret_cast<xmlNode *>(ent);
								temp = temp->next;
							}
						}
						if(!last) {
							last = ret = P_Node;
						}
						else {
							last = xmlAddNextSibling(last, P_Node);
						}
					}
					SAlloc::F(val);
				}
				cur++;
				q = cur;
			}
			if(charval != 0) {
				xmlChar buffer[10];
				int len = xmlCopyCharMultiByte(buffer, charval);
				buffer[len] = 0;
				if(xmlBufCat(buf, buffer))
					goto out;
				charval = 0;
			}
		}
		else
			cur++;
	}
	if((cur != q) || (ret == NULL)) {
		/*
		 * Handle the last piece of text.
		 */
		xmlBufAdd(buf, q, cur - q);
	}
	if(!xmlBufIsEmpty(buf)) {
		P_Node = xmlNewDocText(doc, 0);
		P_Node->content = xmlBufDetach(buf);
		if(!last) {
			last = ret = P_Node;
		}
		else {
			last = xmlAddNextSibling(last, P_Node);
		}
	}
out:
	xmlBufFree(buf);
	return ret;
}
/**
 * xmlNodeListGetString:
 * @doc:  the document
 * @list:  a Node list
 * @inLine:  should we replace entity contents or show their external form
 *
 * Build the string equivalent to the text contained in the Node list
 * made of TEXTs and ENTITY_REFs
 *
 * Returns a pointer to the string copy, the caller must free it with SAlloc::F().
 */
xmlChar * FASTCALL xmlNodeListGetString(xmlDoc * doc, const xmlNode * list, int inLine)
{
	const xmlNode * p_node = list;
	xmlChar * ret = NULL;
	xmlEntity * ent;
	if(list) {
		int attr = BIN(list->P_ParentNode && list->P_ParentNode->type == XML_ATTRIBUTE_NODE);
		while(p_node) {
			if(oneof2(p_node->type, XML_TEXT_NODE, XML_CDATA_SECTION_NODE)) {
				if(inLine) {
					ret = xmlStrcat(ret, p_node->content);
				}
				else {
					xmlChar * buffer = attr ? xmlEncodeAttributeEntities(doc, p_node->content) : xmlEncodeEntitiesReentrant(doc, p_node->content);
					if(buffer) {
						ret = xmlStrcat(ret, buffer);
						SAlloc::F(buffer);
					}
				}
			}
			else if(p_node->type == XML_ENTITY_REF_NODE) {
				if(inLine) {
					ent = xmlGetDocEntity(doc, p_node->name);
					if(ent) {
						/* an entity content can be any "well balanced chunk",
						 * i.e. the result of the content [43] production:
						 * http://www.w3.org/TR/REC-xml#NT-content.
						 * So it can contain text, CDATA section or nested
						 * entity reference nodes (among others).
						 * -> we recursive  call xmlNodeListGetString()
						 * which handles these types */
						xmlChar * buffer = xmlNodeListGetString(doc, ent->children, 1);
						if(buffer) {
							ret = xmlStrcat(ret, buffer);
							SAlloc::F(buffer);
						}
					}
					else {
						ret = xmlStrcat(ret, p_node->content);
					}
				}
				else {
					xmlChar buf[2];
					buf[0] = '&';
					buf[1] = 0;
					ret = xmlStrncat(ret, buf, 1);
					ret = xmlStrcat(ret, p_node->name);
					buf[0] = ';';
					buf[1] = 0;
					ret = xmlStrncat(ret, buf, 1);
				}
			}
	#if 0
			else {
				xmlGenericError(0, "xmlGetNodeListString : invalid node type %d\n", node->type);
			}
	#endif
			p_node = p_node->next;
		}
	}
	return ret;
}

#ifdef LIBXML_TREE_ENABLED
/**
 * xmlNodeListGetRawString:
 * @doc:  the document
 * @list:  a Node list
 * @inLine:  should we replace entity contents or show their external form
 *
 * Builds the string equivalent to the text contained in the Node list
 * made of TEXTs and ENTITY_REFs, contrary to xmlNodeListGetString()
 * this function doesn't do any character encoding handling.
 *
 * Returns a pointer to the string copy, the caller must free it with SAlloc::F().
 */
xmlChar * xmlNodeListGetRawString(const xmlDoc * doc, const xmlNode * list, int inLine)
{
	const xmlNode * P_Node = list;
	xmlChar * ret = NULL;
	xmlEntity * ent;
	if(list) {
		while(P_Node) {
			if((P_Node->type == XML_TEXT_NODE) || (P_Node->type == XML_CDATA_SECTION_NODE)) {
				if(inLine) {
					ret = xmlStrcat(ret, P_Node->content);
				}
				else {
					xmlChar * buffer = xmlEncodeSpecialChars(doc, P_Node->content);
					if(buffer) {
						ret = xmlStrcat(ret, buffer);
						SAlloc::F(buffer);
					}
				}
			}
			else if(P_Node->type == XML_ENTITY_REF_NODE) {
				if(inLine) {
					ent = xmlGetDocEntity(doc, P_Node->name);
					if(ent) {
						/* an entity content can be any "well balanced chunk",
						* i.e. the result of the content [43] production:
						* http://www.w3.org/TR/REC-xml#NT-content.
						* So it can contain text, CDATA section or nested
						* entity reference nodes (among others).
						* -> we recursive  call xmlNodeListGetRawString()
						* which handles these types */
						xmlChar * buffer = xmlNodeListGetRawString(doc, ent->children, 1);
						if(buffer) {
							ret = xmlStrcat(ret, buffer);
							SAlloc::F(buffer);
						}
					}
					else {
						ret = xmlStrcat(ret, P_Node->content);
					}
				}
				else {
					xmlChar buf[2];
					buf[0] = '&';
					buf[1] = 0;
					ret = xmlStrncat(ret, buf, 1);
					ret = xmlStrcat(ret, P_Node->name);
					buf[0] = ';';
					buf[1] = 0;
					ret = xmlStrncat(ret, buf, 1);
				}
			}
	#if 0
			else {
				xmlGenericError(0, "xmlGetNodeListString : invalid node type %d\n", node->type);
			}
	#endif
			P_Node = P_Node->next;
		}
	}
	return ret;
}

#endif /* LIBXML_TREE_ENABLED */

static xmlAttr * FASTCALL xmlNewPropInternal(xmlNode * pNode, xmlNs * ns, const xmlChar * name, const xmlChar * value, int eatname)
{
	xmlAttr * cur;
	xmlDoc * doc = NULL;
	if(pNode && pNode->type != XML_ELEMENT_NODE) {
		if((eatname == 1) && (!pNode->doc || (!(xmlDictOwns(pNode->doc->dict, name)))))
			SAlloc::F((xmlChar *)name);
		return 0;
	}
	/*
	 * Allocate a new property and fill the fields.
	 */
	cur = static_cast<xmlAttr *>(SAlloc::M(sizeof(xmlAttr)));
	if(!cur) {
		if((eatname == 1) && (!pNode || !pNode->doc || (!(xmlDictOwns(pNode->doc->dict, name)))))
			SAlloc::F((xmlChar *)name);
		xmlTreeErrMemory("building attribute");
		return 0;
	}
	memzero(cur, sizeof(xmlAttr));
	cur->type = XML_ATTRIBUTE_NODE;
	cur->parent = pNode;
	if(pNode) {
		doc = pNode->doc;
		cur->doc = doc;
	}
	cur->ns = ns;
	if(eatname == 0)
		cur->name = (doc && doc->dict) ? (xmlChar *)xmlDictLookupSL(doc->dict, name) : sstrdup(name);
	else
		cur->name = name;
	if(value) {
		if(!xmlCheckUTF8(value)) {
			xmlTreeErr(XML_TREE_NOT_UTF8, (xmlNode *)doc, 0);
			if(doc)
				doc->encoding = sstrdup(reinterpret_cast<const xmlChar *>("ISO-8859-1"));
		}
		cur->children = xmlNewDocText(doc, value);
		cur->last = NULL;
		for(xmlNode * tmp = cur->children; tmp; tmp = tmp->next) {
			tmp->P_ParentNode = (xmlNode *)cur;
			if(!tmp->next)
				cur->last = tmp;
		}
	}
	/*
	 * Add it at the end to preserve parsing order ...
	 */
	if(pNode) {
		if(pNode->properties == NULL) {
			pNode->properties = cur;
		}
		else {
			xmlAttr * prev = pNode->properties;
			while(prev->next)
				prev = prev->next;
			prev->next = cur;
			cur->prev = prev;
		}
	}
	if(value && pNode && xmlIsID(pNode->doc, pNode, cur) == 1)
		xmlAddID(NULL, pNode->doc, value, cur);
	if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
		xmlRegisterNodeDefaultValue((xmlNode *)cur);
	return (cur);
}

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
/**
 * xmlNewProp:
 * @node:  the holding node
 * @name:  the name of the attribute
 * @value:  the value of the attribute
 *
 * Create a new property carried by a node.
 * Returns a pointer to the attribute
 */
xmlAttr * xmlNewProp(xmlNode * pNode, const xmlChar * name, const xmlChar * value)
{
	if(!name) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNewProp : name == NULL\n");
#endif
		return 0;
	}
	return xmlNewPropInternal(pNode, NULL, name, value, 0);
}

#endif /* LIBXML_TREE_ENABLED */
/**
 * xmlNewNsProp:
 * @node:  the holding node
 * @ns:  the namespace
 * @name:  the name of the attribute
 * @value:  the value of the attribute
 *
 * Create a new property tagged with a namespace and carried by a node.
 * Returns a pointer to the attribute
 */
xmlAttr * xmlNewNsProp(xmlNode * pNode, xmlNs * ns, const xmlChar * name, const xmlChar * value)
{
	if(!name) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNewNsProp : name == NULL\n");
#endif
		return 0;
	}
	return xmlNewPropInternal(pNode, ns, name, value, 0);
}
/**
 * xmlNewNsPropEatName:
 * @node:  the holding node
 * @ns:  the namespace
 * @name:  the name of the attribute
 * @value:  the value of the attribute
 *
 * Create a new property tagged with a namespace and carried by a node.
 * Returns a pointer to the attribute
 */
xmlAttr * xmlNewNsPropEatName(xmlNode * pNode, xmlNs * ns, xmlChar * name, const xmlChar * value)
{
	if(!name) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNewNsPropEatName : name == NULL\n");
#endif
		return 0;
	}
	return xmlNewPropInternal(pNode, ns, name, value, 1);
}
/**
 * xmlNewDocProp:
 * @doc:  the document
 * @name:  the name of the attribute
 * @value:  the value of the attribute
 *
 * Create a new property carried by a document.
 * Returns a pointer to the attribute
 */
xmlAttr * FASTCALL xmlNewDocProp(xmlDoc * doc, const xmlChar * name, const xmlChar * value)
{
	xmlAttr * cur = 0;
	if(!name) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNewDocProp : name == NULL\n");
#endif
	}
	else {
		// 
		// Allocate a new property and fill the fields.
		// 
		cur = static_cast<xmlAttr *>(SAlloc::M(sizeof(xmlAttr)));
		if(!cur) {
			xmlTreeErrMemory("building attribute");
		}
		else {
			memzero(cur, sizeof(xmlAttr));
			cur->type = XML_ATTRIBUTE_NODE;
			cur->name = (doc && doc->dict) ? xmlDictLookupSL(doc->dict, name) : sstrdup(name);
			cur->doc = doc;
			if(value) {
				cur->children = xmlStringGetNodeList(doc, value);
				cur->last = NULL;
				for(xmlNode * tmp = cur->children; tmp; tmp = tmp->next) {
					tmp->P_ParentNode = (xmlNode *)cur;
					if(!tmp->next)
						cur->last = tmp;
				}
			}
			if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
				xmlRegisterNodeDefaultValue((xmlNode *)cur);
		}
	}
	return cur;
}
/**
 * xmlFreePropList:
 * @cur:  the first property in the list
 *
 * Free a property and all its siblings, all the children are freed too.
 */
void xmlFreePropList(xmlAttr * cur)
{
	while(cur) {
		xmlAttr * next = cur->next;
		xmlFreeProp(cur);
		cur = next;
	}
}
/**
 * xmlFreeProp:
 * @cur:  an attribute
 *
 * Free one attribute, all the content is freed too
 */
void FASTCALL xmlFreeProp(xmlAttr * cur)
{
	if(cur) {
		xmlDict * p_dict = cur->doc ? cur->doc->dict : 0;
		if(__xmlRegisterCallbacks && (xmlDeregisterNodeDefaultValue))
			xmlDeregisterNodeDefaultValue((xmlNode *)cur);
		// Check for ID removal -> leading to invalid references ! 
		if(cur->doc && (cur->atype == XML_ATTRIBUTE_ID))
			xmlRemoveID(cur->doc, cur);
		xmlFreeNodeList(cur->children);
		XmlDestroyStringWithDict(p_dict, (xmlChar *)cur->name); // @badcast
		SAlloc::F(cur);
	}
}
/**
 * xmlRemoveProp:
 * @cur:  an attribute
 *
 * Unlink and free one attribute, all the content is freed too
 * Note this doesn't work for namespace definition attributes
 *
 * Returns 0 if success and -1 in case of error.
 */
int xmlRemoveProp(xmlAttr * cur)
{
	if(!cur) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlRemoveProp : cur == NULL\n");
#endif
		return -1;
	}
	else if(cur->parent == NULL) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlRemoveProp : cur->parent == NULL\n");
#endif
		return -1;
	}
	else {
		xmlAttr * tmp = cur->parent->properties;
		if(tmp == cur) {
			cur->parent->properties = cur->next;
			if(cur->next)
				cur->next->prev = NULL;
			xmlFreeProp(cur);
			return 0;
		}
		else {
			while(tmp) {
				if(tmp->next == cur) {
					tmp->next = cur->next;
					if(tmp->next)
						tmp->next->prev = tmp;
					xmlFreeProp(cur);
					return 0;
				}
				tmp = tmp->next;
			}
#ifdef DEBUG_TREE
			xmlGenericError(0, "xmlRemoveProp : attribute not owned by its node\n");
#endif
			return -1;
		}
	}
}

/**
 * xmlNewDocPI:
 * @doc:  the target document
 * @name:  the processing instruction name
 * @content:  the PI content
 *
 * Creation of a processing instruction element.
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewDocPI(xmlDoc * doc, const xmlChar * name, const xmlChar * content)
{
	xmlNode * cur = 0;
	if(!name) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNewPI : name == NULL\n");
#endif
		return 0;
	}
	/*
	 * Allocate a new node and fill the fields.
	 */
	cur = static_cast<xmlNode *>(SAlloc::M(sizeof(xmlNode)));
	if(!cur) {
		xmlTreeErrMemory("building PI");
		return 0;
	}
	memzero(cur, sizeof(xmlNode));
	cur->type = XML_PI_NODE;
	cur->name = (doc && doc->dict) ? xmlDictLookupSL(doc->dict, name) : sstrdup(name);
	if(content)
		cur->content = sstrdup(content);
	cur->doc = doc;
	if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
		xmlRegisterNodeDefaultValue(cur);
	return cur;
}

/**
 * xmlNewPI:
 * @name:  the processing instruction name
 * @content:  the PI content
 *
 * Creation of a processing instruction element.
 * Use xmlDocNewPI preferably to get string interning
 *
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewPI(const xmlChar * name, const xmlChar * content)
{
	return xmlNewDocPI(NULL, name, content);
}
/**
 * xmlNewNode:
 * @ns:  namespace if any
 * @name:  the node name
 *
 * Creation of a new node element. @ns is optional (NULL).
 *
 * Returns a pointer to the new node object. Uses sstrdup() to make
 * copy of @name.
 */
xmlNode * FASTCALL xmlNewNode(xmlNs * ns, const xmlChar * name)
{
	xmlNode * cur = 0;
	if(!name) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNewNode : name == NULL\n");
#endif
		return 0;
	}
	/*
	 * Allocate a new node and fill the fields.
	 */
	cur = static_cast<xmlNode *>(SAlloc::M(sizeof(xmlNode)));
	if(!cur) {
		xmlTreeErrMemory("building node");
		return 0;
	}
	memzero(cur, sizeof(xmlNode));
	cur->type = XML_ELEMENT_NODE;
	cur->name = sstrdup(name);
	cur->ns = ns;
	if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
		xmlRegisterNodeDefaultValue(cur);
	return cur;
}
/**
 * xmlNewNodeEatName:
 * @ns:  namespace if any
 * @name:  the node name
 *
 * Creation of a new node element. @ns is optional (NULL).
 *
 * Returns a pointer to the new node object, with pointer @name as
 * new node's name. Use xmlNewNode() if a copy of @name string is
 * is needed as new node's name.
 */
xmlNode * xmlNewNodeEatName(xmlNs * ns, xmlChar * name)
{
	xmlNode * cur = 0;
	if(!name) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNewNode : name == NULL\n");
#endif
		return 0;
	}
	/*
	 * Allocate a new node and fill the fields.
	 */
	cur = static_cast<xmlNode *>(SAlloc::M(sizeof(xmlNode)));
	if(!cur) {
		xmlTreeErrMemory("building node");
		// we can't check here that name comes from the doc dictionnary 
		return 0;
	}
	memzero(cur, sizeof(xmlNode));
	cur->type = XML_ELEMENT_NODE;
	cur->name = name;
	cur->ns = ns;
	if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
		xmlRegisterNodeDefaultValue((xmlNode *)cur);
	return cur;
}
/**
 * xmlNewDocNode:
 * @doc:  the document
 * @ns:  namespace if any
 * @name:  the node name
 * @content:  the XML text content if any
 *
 * Creation of a new node element within a document. @ns and @content
 * are optional (NULL).
 * NOTE: @content is supposed to be a piece of XML CDATA, so it allow entities
 *  references, but XML special chars need to be escaped first by using
 *  xmlEncodeEntitiesReentrant(). Use xmlNewDocRawNode() if you don't
 *  need entities support.
 *
 * Returns a pointer to the new node object.
 */
xmlNode * FASTCALL xmlNewDocNode(xmlDoc * doc, xmlNs * ns, const xmlChar * name, const xmlChar * content)
{
	xmlNode * cur = (doc && doc->dict) ? xmlNewNodeEatName(ns, (xmlChar *)xmlDictLookupSL(doc->dict, name)) : xmlNewNode(ns, name);
	if(cur) {
		cur->doc = doc;
		if(content) {
			cur->children = xmlStringGetNodeList(doc, content);
			UPDATE_LAST_CHILD_AND_PARENT(cur)
		}
	}
	return cur;
}
/**
 * xmlNewDocNodeEatName:
 * @doc:  the document
 * @ns:  namespace if any
 * @name:  the node name
 * @content:  the XML text content if any
 *
 * Creation of a new node element within a document. @ns and @content
 * are optional (NULL).
 * NOTE: @content is supposed to be a piece of XML CDATA, so it allow entities
 *  references, but XML special chars need to be escaped first by using
 *  xmlEncodeEntitiesReentrant(). Use xmlNewDocRawNode() if you don't
 *  need entities support.
 *
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewDocNodeEatName(xmlDoc * doc, xmlNs * ns, xmlChar * name, const xmlChar * content)
{
	xmlNode * cur = xmlNewNodeEatName(ns, name);
	if(cur) {
		cur->doc = doc;
		if(content) {
			cur->children = xmlStringGetNodeList(doc, content);
			UPDATE_LAST_CHILD_AND_PARENT(cur)
		}
	}
	else {
		// if name don't come from the doc dictionnary free it here 
		if(name && doc && !(xmlDictOwns(doc->dict, name)))
			SAlloc::F(name);
	}
	return cur;
}

#ifdef LIBXML_TREE_ENABLED
/**
 * xmlNewDocRawNode:
 * @doc:  the document
 * @ns:  namespace if any
 * @name:  the node name
 * @content:  the text content if any
 *
 * Creation of a new node element within a document. @ns and @content
 * are optional (NULL).
 *
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewDocRawNode(xmlDoc * doc, xmlNs * ns, const xmlChar * name, const xmlChar * content)
{
	xmlNode * cur = xmlNewDocNode(doc, ns, name, 0);
	if(cur) {
		cur->doc = doc;
		if(content) {
			cur->children = xmlNewDocText(doc, content);
			UPDATE_LAST_CHILD_AND_PARENT(cur)
		}
	}
	return cur;
}
/**
 * xmlNewDocFragment:
 * @doc:  the document owning the fragment
 *
 * Creation of a new Fragment node.
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewDocFragment(xmlDoc * doc)
{
	//
	// Allocate a new DocumentFragment node and fill the fields.
	//
	xmlNode * cur = static_cast<xmlNode *>(SAlloc::M(sizeof(xmlNode)));
	if(!cur)
		xmlTreeErrMemory("building fragment");
	else {
		memzero(cur, sizeof(xmlNode));
		cur->type = XML_DOCUMENT_FRAG_NODE;
		cur->doc = doc;
		if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
			xmlRegisterNodeDefaultValue(cur);
	}
	return cur;
}

#endif /* LIBXML_TREE_ENABLED */

/**
 * xmlNewText:
 * @content:  the text content
 *
 * Creation of a new text node.
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewText(const xmlChar * content)
{
	/*
	 * Allocate a new node and fill the fields.
	 */
	xmlNode * cur = static_cast<xmlNode *>(SAlloc::M(sizeof(xmlNode)));
	if(!cur)
		xmlTreeErrMemory("building text");
	else {
		memzero(cur, sizeof(xmlNode));
		cur->type = XML_TEXT_NODE;
		cur->name = xmlStringText;
		if(content)
			cur->content = sstrdup(content);
		if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
			xmlRegisterNodeDefaultValue(cur);
	}
	return cur;
}

#ifdef LIBXML_TREE_ENABLED
/**
 * xmlNewTextChild:
 * @parent:  the parent node
 * @ns:  a namespace if any
 * @name:  the name of the child
 * @content:  the text content of the child if any.
 *
 * Creation of a new child element, added at the end of @parent children list.
 * @ns and @content parameters are optional (NULL). If @ns is NULL, the newly
 * created element inherits the namespace of @parent. If @content is non NULL,
 * a child TEXT node will be created containing the string @content.
 * NOTE: Use xmlNewChild() if @content will contain entities that need to be
 * preserved. Use this function, xmlNewTextChild(), if you need to ensure that
 * reserved XML chars that might appear in @content, such as the ampersand,
 * greater-than or less-than signs, are automatically replaced by their XML
 * escaped entity representations.
 *
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewTextChild(xmlNode * parent, xmlNs * ns, const xmlChar * name, const xmlChar * content)
{
	xmlNode * cur = 0;
	if(parent == NULL) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNewTextChild : parent == NULL\n");
#endif
	}
	else if(!name) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNewTextChild : name == NULL\n");
#endif
	}
	else {
		/*
		 * Allocate a new node
		 */
		if(parent->type == XML_ELEMENT_NODE)
			cur = xmlNewDocRawNode(parent->doc, ns ? ns : parent->ns, name, content);
		else if(oneof2(parent->type, XML_DOCUMENT_NODE, XML_HTML_DOCUMENT_NODE))
			cur = xmlNewDocRawNode((xmlDoc *)parent, ns, name, content);
		else if(parent->type == XML_DOCUMENT_FRAG_NODE)
			cur = xmlNewDocRawNode(parent->doc, ns, name, content);
		if(cur) {
			/*
			 * add the new element at the end of the children list.
			 */
			cur->type = XML_ELEMENT_NODE;
			cur->P_ParentNode = parent;
			cur->doc = parent->doc;
			if(parent->children == NULL) {
				parent->children = cur;
				parent->last = cur;
			}
			else {
				xmlNode * prev = parent->last;
				prev->next = cur;
				cur->prev = prev;
				parent->last = cur;
			}
		}
	}
	return cur;
}

#endif /* LIBXML_TREE_ENABLED */

/**
 * xmlNewCharRef:
 * @doc: the document
 * @name:  the char ref string, starting with # or "&# ... ;"
 *
 * Creation of a new character reference node.
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewCharRef(xmlDoc * doc, const xmlChar * name)
{
	xmlNode * cur = 0;
	if(name) {
		//
		// Allocate a new node and fill the fields.
		//
		cur = static_cast<xmlNode *>(SAlloc::M(sizeof(xmlNode)));
		if(!cur)
			xmlTreeErrMemory("building character reference");
		else {
			memzero(cur, sizeof(xmlNode));
			cur->type = XML_ENTITY_REF_NODE;
			cur->doc = doc;
			if(name[0] == '&') {
				int len = sstrlen(++name);
				if(name[len-1] == ';')
					cur->name = xmlStrndup(name, len - 1);
				else
					cur->name = xmlStrndup(name, len);
			}
			else
				cur->name = sstrdup(name);
			if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
				xmlRegisterNodeDefaultValue(cur);
		}
	}
	return cur;
}

/**
 * xmlNewReference:
 * @doc: the document
 * @name:  the reference name, or the reference string with & and ;
 *
 * Creation of a new reference node.
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewReference(const xmlDoc * doc, const xmlChar * name)
{
	xmlNode * cur = 0;
	if(name) {
		// Allocate a new node and fill the fields.
		cur = static_cast<xmlNode *>(SAlloc::M(sizeof(xmlNode)));
		if(!cur) {
			xmlTreeErrMemory("building reference");
		}
		else {
			xmlEntity * ent;
			memzero(cur, sizeof(xmlNode));
			cur->type = XML_ENTITY_REF_NODE;
			cur->doc = (xmlDoc *)doc;
			if(name[0] == '&') {
				int len = sstrlen(++name);
				cur->name = (name[len-1] == ';') ? xmlStrndup(name, len-1) : xmlStrndup(name, len);
			}
			else
				cur->name = sstrdup(name);
			ent = xmlGetDocEntity(doc, cur->name);
			if(ent) {
				cur->content = const_cast<xmlChar *>(ent->content); // @badcast
				/*
				 * The parent pointer in entity is a DTD pointer and thus is NOT
				 * updated.  Not sure if this is 100% correct.
				 *  -George
				 */
				cur->children = reinterpret_cast<xmlNode *>(ent);
				cur->last = reinterpret_cast<xmlNode *>(ent);
			}
			if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
				xmlRegisterNodeDefaultValue(cur);
		}
	}
	return cur;
}
/**
 * xmlNewDocText:
 * @doc: the document
 * @content:  the text content
 *
 * Creation of a new text node within a document.
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewDocText(const xmlDoc * doc, const xmlChar * content) 
{
	xmlNode * cur = xmlNewText(content);
	if(cur) 
		cur->doc = (xmlDoc *)doc;
	return cur;
}
/**
 * xmlNewTextLen:
 * @content:  the text content
 * @len:  the text len.
 *
 * Creation of a new text node with an extra parameter for the content's length
 * Returns a pointer to the new node object.
 */
xmlNode * FASTCALL xmlNewTextLen(const xmlChar * content, int len) 
{
	// Allocate a new node and fill the fields.
	xmlNode * cur = static_cast<xmlNode *>(SAlloc::M(sizeof(xmlNode)));
	if(!cur)
		xmlTreeErrMemory("building text");
	else {
		memzero(cur, sizeof(xmlNode));
		cur->type = XML_TEXT_NODE;
		cur->name = xmlStringText;
		if(content) {
			cur->content = xmlStrndup(content, len);
		}
		if(__xmlRegisterCallbacks && xmlRegisterNodeDefaultValue)
			xmlRegisterNodeDefaultValue(cur);
	}
	return cur;
}
/**
 * xmlNewDocTextLen:
 * @doc: the document
 * @content:  the text content
 * @len:  the text len.
 *
 * Creation of a new text node with an extra content length parameter. The
 * text node pertain to a given document.
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewDocTextLen(xmlDoc * doc, const xmlChar * content, int len) 
{
	xmlNode * cur = xmlNewTextLen(content, len);
	if(cur) 
		cur->doc = doc;
	return cur;
}
/**
 * xmlNewComment:
 * @content:  the comment content
 *
 * Creation of a new node containing a comment.
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewComment(const xmlChar * content) 
{
	/*
	 * Allocate a new node and fill the fields.
	 */
	xmlNode * cur = static_cast<xmlNode *>(SAlloc::M(sizeof(xmlNode)));
	if(!cur) {
		xmlTreeErrMemory("building comment");
		return 0;
	}
	memzero(cur, sizeof(xmlNode));
	cur->type = XML_COMMENT_NODE;
	cur->name = xmlStringComment;
	if(content)
		cur->content = sstrdup(content);
	if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
		xmlRegisterNodeDefaultValue(cur);
	return cur;
}

/**
 * xmlNewCDataBlock:
 * @doc:  the document
 * @content:  the CDATA block content content
 * @len:  the length of the block
 *
 * Creation of a new node containing a CDATA block.
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewCDataBlock(xmlDoc * doc, const xmlChar * content, int len) 
{
	/*
	 * Allocate a new node and fill the fields.
	 */
	xmlNode * cur = static_cast<xmlNode *>(SAlloc::M(sizeof(xmlNode)));
	if(!cur) {
		xmlTreeErrMemory("building CDATA");
		return 0;
	}
	memzero(cur, sizeof(xmlNode));
	cur->type = XML_CDATA_SECTION_NODE;
	cur->doc = doc;
	if(content) {
		cur->content = xmlStrndup(content, len);
	}
	if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
		xmlRegisterNodeDefaultValue(cur);
	return cur;
}

/**
 * xmlNewDocComment:
 * @doc:  the document
 * @content:  the comment content
 *
 * Creation of a new node containing a comment within a document.
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewDocComment(xmlDoc * doc, const xmlChar * content) 
{
	xmlNode * cur = xmlNewComment(content);
	if(cur) 
		cur->doc = doc;
	return cur;
}
/**
 * xmlSetTreeDoc:
 * @tree:  the top element
 * @doc:  the document
 *
 * update all nodes under the tree to point to the right document
 */
void FASTCALL xmlSetTreeDoc(xmlNode * tree, xmlDoc * doc) 
{
	if(tree && tree->type != XML_NAMESPACE_DECL) {
		if(tree->doc != doc) {
			if(tree->type == XML_ELEMENT_NODE) {
				for(xmlAttr * prop = tree->properties; prop; prop = prop->next) {
					prop->doc = doc;
					xmlSetListDoc(prop->children, doc);
				}
			}
			if(tree->children)
				xmlSetListDoc(tree->children, doc);
			tree->doc = doc;
		}
	}
}
/**
 * xmlSetListDoc:
 * @list:  the first element
 * @doc:  the document
 *
 * update all nodes in the list to point to the right document
 */
void FASTCALL xmlSetListDoc(xmlNode * list, xmlDoc * doc) 
{
	if(list && list->type != XML_NAMESPACE_DECL) {
		for(xmlNode * cur = list; cur; cur = cur->next) {
			if(cur->doc != doc)
				xmlSetTreeDoc(cur, doc);
		}
	}
}

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
/**
 * xmlNewChild:
 * @parent:  the parent node
 * @ns:  a namespace if any
 * @name:  the name of the child
 * @content:  the XML content of the child if any.
 *
 * Creation of a new child element, added at the end of @parent children list.
 * @ns and @content parameters are optional (NULL). If @ns is NULL, the newly
 * created element inherits the namespace of @parent. If @content is non NULL,
 * a child list containing the TEXTs and ENTITY_REFs node will be created.
 * NOTE: @content is supposed to be a piece of XML CDATA, so it allows entity
 *  references. XML special chars must be escaped first by using
 *  xmlEncodeEntitiesReentrant(), or xmlNewTextChild() should be used.
 *
 * Returns a pointer to the new node object.
 */
xmlNode * xmlNewChild(xmlNode * parent, xmlNs * ns, const xmlChar * name, const xmlChar * content) 
{
	xmlNode * cur = 0;
	if(parent == NULL) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNewChild : parent == NULL\n");
#endif
	}
	else if(!name) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNewChild : name == NULL\n");
#endif
	}
	else {
		// 
		// Allocate a new node
		// 
		if(parent->type == XML_ELEMENT_NODE)
			cur = xmlNewDocNode(parent->doc, ns ? ns : parent->ns, name, content);
		else if(oneof2(parent->type, XML_DOCUMENT_NODE, XML_HTML_DOCUMENT_NODE))
			cur = xmlNewDocNode((xmlDoc *)parent, ns, name, content);
		else if(parent->type == XML_DOCUMENT_FRAG_NODE)
			cur = xmlNewDocNode(parent->doc, ns, name, content);
		else
			return 0;
		if(cur) {
			// 
			// add the new element at the end of the children list.
			// 
			cur->type = XML_ELEMENT_NODE;
			cur->P_ParentNode = parent;
			cur->doc = parent->doc;
			if(parent->children == NULL) {
				parent->children = cur;
				parent->last = cur;
			}
			else {
				xmlNode * prev = parent->last;
				prev->next = cur;
				cur->prev = prev;
				parent->last = cur;
			}
		}
	}
	return cur;
}

#endif /* LIBXML_TREE_ENABLED */
/**
 * xmlAddPropSibling:
 * @prev:  the attribute to which @prop is added after
 * @cur:   the base attribute passed to calling function
 * @prop:  the new attribute
 *
 * Add a new attribute after @prev using @cur as base attribute.
 * When inserting before @cur, @prev is passed as @cur->prev.
 * When inserting after @cur, @prev is passed as @cur.
 * If an existing attribute is found it is detroyed prior to adding @prop.
 *
 * Returns the attribute being inserted or NULL in case of error.
 */
static xmlNode * FASTCALL xmlAddPropSibling(xmlNode * prev, xmlNode * cur, xmlNode * prop)
{
	xmlAttr * attr;
	if(!cur || (cur->type != XML_ATTRIBUTE_NODE) || !prop || (prop->type != XML_ATTRIBUTE_NODE) || (prev && prev->type != XML_ATTRIBUTE_NODE))
		return 0;
	// check if an attribute with the same name exists 
	attr = xmlHasNsProp(cur->P_ParentNode, prop->name, prop->ns ? prop->ns->href : 0);
	if(prop->doc != cur->doc) {
		xmlSetTreeDoc(prop, cur->doc);
	}
	prop->P_ParentNode = cur->P_ParentNode;
	prop->prev = prev;
	if(prev) {
		prop->next = prev->next;
		prev->next = prop;
		if(prop->next)
			prop->next->prev = prop;
	}
	else {
		prop->next = cur;
		cur->prev = prop;
	}
	if(!prop->prev && prop->P_ParentNode)
		prop->P_ParentNode->properties = (xmlAttr *)prop;
	if(attr && (attr->type != XML_ATTRIBUTE_DECL)) {
		// different instance, destroy it (attributes must be unique) 
		xmlRemoveProp(attr);
	}
	return prop;
}
/**
 * xmlAddNextSibling:
 * @cur:  the child node
 * @elem:  the new node
 *
 * Add a new node @elem as the next sibling of @cur
 * If the new node was already inserted in a document it is
 * first unlinked from its existing context.
 * As a result of text merging @elem may be freed.
 * If the new node is ATTRIBUTE, it is added into properties instead of children.
 * If there is an attribute with equal name, it is first destroyed.
 *
 * Returns the new node or NULL in case of error.
 */
xmlNode * FASTCALL xmlAddNextSibling(xmlNode * cur, xmlNode * elem) 
{
	if(!cur || (cur->type == XML_NAMESPACE_DECL)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlAddNextSibling : cur == NULL\n");
#endif
		return 0;
	}
	if(!elem || (elem->type == XML_NAMESPACE_DECL)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlAddNextSibling : elem == NULL\n");
#endif
		return 0;
	}
	if(cur == elem) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlAddNextSibling : cur == elem\n");
#endif
		return 0;
	}
	xmlUnlinkNode(elem);
	if(elem->type == XML_TEXT_NODE) {
		if(cur->type == XML_TEXT_NODE) {
			xmlNodeAddContent(cur, elem->content);
			xmlFreeNode(elem);
			return cur;
		}
		if(cur->next && (cur->next->type == XML_TEXT_NODE) && (cur->name == cur->next->name)) {
			xmlChar * tmp = sstrdup(elem->content);
			tmp = xmlStrcat(tmp, cur->next->content);
			xmlNodeSetContent(cur->next, tmp);
			SAlloc::F(tmp);
			xmlFreeNode(elem);
			return (cur->next);
		}
	}
	else if(elem->type == XML_ATTRIBUTE_NODE) {
		return xmlAddPropSibling(cur, cur, elem);
	}
	if(elem->doc != cur->doc) {
		xmlSetTreeDoc(elem, cur->doc);
	}
	elem->P_ParentNode = cur->P_ParentNode;
	elem->prev = cur;
	elem->next = cur->next;
	cur->next = elem;
	if(elem->next)
		elem->next->prev = elem;
	if(elem->P_ParentNode && elem->P_ParentNode->last == cur)
		elem->P_ParentNode->last = elem;
	return (elem);
}

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED)
/**
 * xmlAddPrevSibling:
 * @cur:  the child node
 * @elem:  the new node
 *
 * Add a new node @elem as the previous sibling of @cur
 * merging adjacent TEXT nodes (@elem may be freed)
 * If the new node was already inserted in a document it is
 * first unlinked from its existing context.
 * If the new node is ATTRIBUTE, it is added into properties instead of children.
 * If there is an attribute with equal name, it is first destroyed.
 *
 * Returns the new node or NULL in case of error.
 */
xmlNode * FASTCALL xmlAddPrevSibling(xmlNode * cur, xmlNode * elem) 
{
	if(!cur || (cur->type == XML_NAMESPACE_DECL)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlAddPrevSibling : cur == NULL\n");
#endif
		return 0;
	}
	if((elem == NULL) || (elem->type == XML_NAMESPACE_DECL)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlAddPrevSibling : elem == NULL\n");
#endif
		return 0;
	}
	if(cur == elem) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlAddPrevSibling : cur == elem\n");
#endif
		return 0;
	}
	xmlUnlinkNode(elem);
	if(elem->type == XML_TEXT_NODE) {
		if(cur->type == XML_TEXT_NODE) {
			xmlChar * tmp = sstrdup(elem->content);
			tmp = xmlStrcat(tmp, cur->content);
			xmlNodeSetContent(cur, tmp);
			SAlloc::F(tmp);
			xmlFreeNode(elem);
			return cur;
		}
		if(cur->prev && (cur->prev->type == XML_TEXT_NODE) && (cur->name == cur->prev->name)) {
			xmlNodeAddContent(cur->prev, elem->content);
			xmlFreeNode(elem);
			return (cur->prev);
		}
	}
	else if(elem->type == XML_ATTRIBUTE_NODE) {
		return xmlAddPropSibling(cur->prev, cur, elem);
	}
	if(elem->doc != cur->doc) {
		xmlSetTreeDoc(elem, cur->doc);
	}
	elem->P_ParentNode = cur->P_ParentNode;
	elem->next = cur;
	elem->prev = cur->prev;
	cur->prev = elem;
	if(elem->prev)
		elem->prev->next = elem;
	if(elem->P_ParentNode && elem->P_ParentNode->children == cur) {
		elem->P_ParentNode->children = elem;
	}
	return elem;
}

#endif /* LIBXML_TREE_ENABLED */

/**
 * xmlAddSibling:
 * @cur:  the child node
 * @elem:  the new node
 *
 * Add a new element @elem to the list of siblings of @cur
 * merging adjacent TEXT nodes (@elem may be freed)
 * If the new element was already inserted in a document it is
 * first unlinked from its existing context.
 *
 * Returns the new element or NULL in case of error.
 */
xmlNode * xmlAddSibling(xmlNode * cur, xmlNode * elem) 
{
	xmlNode * parent;
	if(!cur || (cur->type == XML_NAMESPACE_DECL)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlAddSibling : cur == NULL\n");
#endif
		return 0;
	}
	if((elem == NULL) || (elem->type == XML_NAMESPACE_DECL)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlAddSibling : elem == NULL\n");
#endif
		return 0;
	}
	if(cur == elem) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlAddSibling : cur == elem\n");
#endif
		return 0;
	}
	/*
	 * Constant time is we can rely on the ->parent->last to find
	 * the last sibling.
	 */
	if((cur->type != XML_ATTRIBUTE_NODE) && cur->P_ParentNode && cur->P_ParentNode->children && cur->P_ParentNode->last && !(cur->P_ParentNode->last->next)) {
		cur = cur->P_ParentNode->last;
	}
	else {
		while(cur->next) 
			cur = cur->next;
	}
	xmlUnlinkNode(elem);
	if((cur->type == XML_TEXT_NODE) && (elem->type == XML_TEXT_NODE) && (cur->name == elem->name)) {
		xmlNodeAddContent(cur, elem->content);
		xmlFreeNode(elem);
		return cur;
	}
	else if(elem->type == XML_ATTRIBUTE_NODE) {
		return xmlAddPropSibling(cur, cur, elem);
	}
	if(elem->doc != cur->doc) {
		xmlSetTreeDoc(elem, cur->doc);
	}
	parent = cur->P_ParentNode;
	elem->prev = cur;
	elem->next = NULL;
	elem->P_ParentNode = parent;
	cur->next = elem;
	if(parent)
		parent->last = elem;
	return elem;
}
/**
 * xmlAddChildList:
 * @parent:  the parent node
 * @cur:  the first node in the list
 *
 * Add a list of node at the end of the child list of the parent
 * merging adjacent TEXT nodes (@cur may be freed)
 *
 * Returns the last child or NULL in case of error.
 */
xmlNode * xmlAddChildList(xmlNode * parent, xmlNode * cur)
{
	xmlNode * prev;
	if((parent == NULL) || (parent->type == XML_NAMESPACE_DECL)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlAddChildList : parent == NULL\n");
#endif
		return 0;
	}
	if(!cur || cur->type == XML_NAMESPACE_DECL) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlAddChildList : child == NULL\n");
#endif
		return 0;
	}
	if(cur->doc && parent->doc && cur->doc != parent->doc) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "Elements moved to a different document\n");
#endif
	}
	/*
	 * add the first element at the end of the children list.
	 */

	if(parent->children == NULL) {
		parent->children = cur;
	}
	else {
		/*
		 * If cur and parent->last both are TEXT nodes, then merge them.
		 */
		if((cur->type == XML_TEXT_NODE) && (parent->last->type == XML_TEXT_NODE) && (cur->name == parent->last->name)) {
			xmlNodeAddContent(parent->last, cur->content);
			/*
			 * if it's the only child, nothing more to be done.
			 */
			if(cur->next == NULL) {
				xmlFreeNode(cur);
				return parent->last;
			}
			prev = cur;
			cur = cur->next;
			xmlFreeNode(prev);
		}
		prev = parent->last;
		prev->next = cur;
		cur->prev = prev;
	}
	while(cur->next) {
		cur->P_ParentNode = parent;
		if(cur->doc != parent->doc) {
			xmlSetTreeDoc(cur, parent->doc);
		}
		cur = cur->next;
	}
	cur->P_ParentNode = parent;
	/* the parent may not be linked to a doc ! */
	if(cur->doc != parent->doc) {
		xmlSetTreeDoc(cur, parent->doc);
	}
	parent->last = cur;
	return cur;
}

/**
 * xmlAddChild:
 * @parent:  the parent node
 * @cur:  the child node
 *
 * Add a new node to @parent, at the end of the child (or property) list
 * merging adjacent TEXT nodes (in which case @cur is freed)
 * If the new node is ATTRIBUTE, it is added into properties instead of children.
 * If there is an attribute with equal name, it is first destroyed.
 *
 * Returns the child or NULL in case of error.
 */
xmlNode * FASTCALL xmlAddChild(xmlNode * parent, xmlNode * cur) 
{
	xmlNode * prev;
	if((parent == NULL) || (parent->type == XML_NAMESPACE_DECL)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlAddChild : parent == NULL\n");
#endif
		return 0;
	}
	if(!cur || (cur->type == XML_NAMESPACE_DECL)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlAddChild : child == NULL\n");
#endif
		return 0;
	}
	if(parent == cur) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlAddChild : parent == cur\n");
#endif
		return 0;
	}
	/*
	 * If cur is a TEXT node, merge its content with adjacent TEXT nodes
	 * cur is then freed.
	 */
	if(cur->type == XML_TEXT_NODE) {
		if((parent->type == XML_TEXT_NODE) && parent->content && (parent->name == cur->name)) {
			xmlNodeAddContent(parent, cur->content);
			xmlFreeNode(cur);
			return (parent);
		}
		if(parent->last && (parent->last->type == XML_TEXT_NODE) && (parent->last->name == cur->name) && (parent->last != cur)) {
			xmlNodeAddContent(parent->last, cur->content);
			xmlFreeNode(cur);
			return (parent->last);
		}
	}
	/*
	 * add the new element at the end of the children list.
	 */
	prev = cur->P_ParentNode;
	cur->P_ParentNode = parent;
	if(cur->doc != parent->doc) {
		xmlSetTreeDoc(cur, parent->doc);
	}
	/* this check prevents a loop on tree-traversions if a developer
	 * tries to add a node to its parent multiple times
	 */
	if(prev == parent)
		return cur;
	/*
	 * Coalescing
	 */
	if((parent->type == XML_TEXT_NODE) && parent->content && (parent != cur)) {
		xmlNodeAddContent(parent, cur->content);
		xmlFreeNode(cur);
		return parent;
	}
	if(cur->type == XML_ATTRIBUTE_NODE) {
		if(parent->type != XML_ELEMENT_NODE)
			return 0;
		if(parent->properties) {
			/* check if an attribute with the same name exists */
			xmlAttr * lastattr = cur->ns ? xmlHasNsProp(parent, cur->name, cur->ns->href) : xmlHasNsProp(parent, cur->name, 0);
			if(lastattr && (lastattr != (xmlAttr *)cur) && (lastattr->type != XML_ATTRIBUTE_DECL)) {
				// different instance, destroy it (attributes must be unique) 
				xmlUnlinkNode((xmlNode *)lastattr);
				xmlFreeProp(lastattr);
			}
			if(lastattr == (xmlAttr *)cur)
				return cur;
		}
		if(parent->properties == NULL) {
			parent->properties = (xmlAttr *)cur;
		}
		else {
			// find the end 
			xmlAttr * lastattr = parent->properties;
			while(lastattr->next)
				lastattr = lastattr->next;
			lastattr->next = (xmlAttr *)cur;
			((xmlAttr *)cur)->prev = lastattr;
		}
	}
	else {
		if(parent->children == NULL) {
			parent->children = cur;
			parent->last = cur;
		}
		else {
			prev = parent->last;
			prev->next = cur;
			cur->prev = prev;
			parent->last = cur;
		}
	}
	return cur;
}
/**
 * xmlGetLastChild:
 * @parent:  the parent node
 *
 * Search the last child of a node.
 * Returns the last child or NULL if none.
 */
xmlNode * FASTCALL xmlGetLastChild(const xmlNode * pParent) 
{
	if(!pParent || pParent->type == XML_NAMESPACE_DECL) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlGetLastChild : parent == NULL\n");
#endif
		return 0;
	}
	return pParent->last;
}

#ifdef LIBXML_TREE_ENABLED
/*
 * 5 interfaces from DOM ElementTraversal
 */

/**
 * xmlChildElementCount:
 * @parent: the parent node
 *
 * Finds the current number of child nodes of that element which are
 * element nodes.
 * Note the handling of entities references is different than in
 * the W3C DOM element traversal spec since we don't have back reference
 * from entities content to entities references.
 *
 * Returns the count of element child or 0 if not available
 */
unsigned long xmlChildElementCount(xmlNode * parent) 
{
	unsigned long ret = 0;
	xmlNode * cur = NULL;
	if(parent == NULL)
		return 0;
	switch(parent->type) {
		case XML_ELEMENT_NODE:
		case XML_ENTITY_NODE:
		case XML_DOCUMENT_NODE:
		case XML_DOCUMENT_FRAG_NODE:
		case XML_HTML_DOCUMENT_NODE:
		    cur = parent->children;
		    break;
		default:
		    return 0;
	}
	while(cur) {
		if(cur->type == XML_ELEMENT_NODE)
			ret++;
		cur = cur->next;
	}
	return ret;
}

/**
 * xmlFirstElementChild:
 * @parent: the parent node
 *
 * Finds the first child node of that element which is a Element node
 * Note the handling of entities references is different than in
 * the W3C DOM element traversal spec since we don't have back reference
 * from entities content to entities references.
 *
 * Returns the first element child or NULL if not available
 */
xmlNode * xmlFirstElementChild(xmlNode * parent) 
{
	xmlNode * cur = NULL;
	if(parent == NULL)
		return 0;
	switch(parent->type) {
		case XML_ELEMENT_NODE:
		case XML_ENTITY_NODE:
		case XML_DOCUMENT_NODE:
		case XML_DOCUMENT_FRAG_NODE:
		case XML_HTML_DOCUMENT_NODE:
		    cur = parent->children;
		    break;
		default:
		    return 0;
	}
	while(cur) {
		if(cur->type == XML_ELEMENT_NODE)
			return cur;
		cur = cur->next;
	}
	return 0;
}

/**
 * xmlLastElementChild:
 * @parent: the parent node
 *
 * Finds the last child node of that element which is a Element node
 * Note the handling of entities references is different than in
 * the W3C DOM element traversal spec since we don't have back reference
 * from entities content to entities references.
 *
 * Returns the last element child or NULL if not available
 */
xmlNode * xmlLastElementChild(xmlNode * parent) 
{
	xmlNode * cur = NULL;
	if(parent == NULL)
		return 0;
	switch(parent->type) {
		case XML_ELEMENT_NODE:
		case XML_ENTITY_NODE:
		case XML_DOCUMENT_NODE:
		case XML_DOCUMENT_FRAG_NODE:
		case XML_HTML_DOCUMENT_NODE:
		    cur = parent->last;
		    break;
		default:
		    return 0;
	}
	while(cur) {
		if(cur->type == XML_ELEMENT_NODE)
			return cur;
		cur = cur->prev;
	}
	return 0;
}

/**
 * xmlPreviousElementSibling:
 * @node: the current node
 *
 * Finds the first closest previous sibling of the node which is an
 * element node.
 * Note the handling of entities references is different than in
 * the W3C DOM element traversal spec since we don't have back reference
 * from entities content to entities references.
 *
 * Returns the previous element sibling or NULL if not available
 */
xmlNode * xmlPreviousElementSibling(xmlNode * P_Node) {
	if(!P_Node)
		return 0;
	switch(P_Node->type) {
		case XML_ELEMENT_NODE:
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_ENTITY_REF_NODE:
		case XML_ENTITY_NODE:
		case XML_PI_NODE:
		case XML_COMMENT_NODE:
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END:
		    P_Node = P_Node->prev;
		    break;
		default:
		    return 0;
	}
	while(P_Node) {
		if(P_Node->type == XML_ELEMENT_NODE)
			return (P_Node);
		P_Node = P_Node->prev;
	}
	return 0;
}

/**
 * xmlNextElementSibling:
 * @node: the current node
 *
 * Finds the first closest next sibling of the node which is an
 * element node.
 * Note the handling of entities references is different than in
 * the W3C DOM element traversal spec since we don't have back reference
 * from entities content to entities references.
 *
 * Returns the next element sibling or NULL if not available
 */
xmlNode * xmlNextElementSibling(xmlNode * P_Node) {
	if(!P_Node)
		return 0;
	switch(P_Node->type) {
		case XML_ELEMENT_NODE:
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_ENTITY_REF_NODE:
		case XML_ENTITY_NODE:
		case XML_PI_NODE:
		case XML_COMMENT_NODE:
		case XML_DTD_NODE:
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END:
		    P_Node = P_Node->next;
		    break;
		default:
		    return 0;
	}
	while(P_Node) {
		if(P_Node->type == XML_ELEMENT_NODE)
			return (P_Node);
		P_Node = P_Node->next;
	}
	return 0;
}

#endif /* LIBXML_TREE_ENABLED */
/**
 * xmlFreeNodeList:
 * @cur:  the first node in the list
 *
 * Free a node and all its siblings, this is a recursive behaviour, all
 * the children are freed too.
 */
void FASTCALL xmlFreeNodeList(xmlNode * cur)
{
	if(cur) {
		if(cur->type == XML_NAMESPACE_DECL) {
			xmlFreeNsList((xmlNs *)cur);
		}
		else if((cur->type == XML_DOCUMENT_NODE) ||
	#ifdef LIBXML_DOCB_ENABLED
			(cur->type == XML_DOCB_DOCUMENT_NODE) ||
	#endif
			(cur->type == XML_HTML_DOCUMENT_NODE)) {
			xmlFreeDoc((xmlDoc *)cur);
		}
		else {
			xmlDict * p_dict = cur->doc ? cur->doc->dict : 0;
			while(cur) {
				xmlNode * next = cur->next;
				if(cur->type != XML_DTD_NODE) {
					if(__xmlRegisterCallbacks && xmlDeregisterNodeDefaultValue)
						xmlDeregisterNodeDefaultValue(cur);
					if(cur->children && cur->type != XML_ENTITY_REF_NODE)
						xmlFreeNodeList(cur->children);
					if(oneof3(cur->type, XML_ELEMENT_NODE, XML_XINCLUDE_START, XML_XINCLUDE_END) && cur->properties)
						xmlFreePropList(cur->properties);
					if(!oneof4(cur->type, XML_ELEMENT_NODE, XML_XINCLUDE_START, XML_XINCLUDE_END, XML_ENTITY_REF_NODE) && cur->content != (xmlChar *)&(cur->properties)) {
						XmlDestroyStringWithDict(p_dict, cur->content);
					}
					if(oneof3(cur->type, XML_ELEMENT_NODE, XML_XINCLUDE_START, XML_XINCLUDE_END) && cur->nsDef)
						xmlFreeNsList(cur->nsDef);
					/*
					* When a node is a text node or a comment, it uses a global static
					* variable for the name of the node.
					* Otherwise the node name might come from the document's dictionnary
					*/
					if(cur->name && !oneof2(cur->type, XML_TEXT_NODE, XML_COMMENT_NODE))
						XmlDestroyStringWithDict(p_dict, (xmlChar *)cur->name); // @badcast
					SAlloc::F(cur);
				}
				cur = next;
			}
		}
	}
}
/**
 * xmlFreeNode:
 * @cur:  the node
 *
 * Free a node, this is a recursive behaviour, all the children are freed too.
 * This doesn't unlink the child from the list, use xmlUnlinkNode() first.
 */
void FASTCALL xmlFreeNode(xmlNode * pCur)
{
	if(pCur) {
		if(pCur->type == XML_DTD_NODE) // use xmlFreeDtd for DTD nodes
			xmlFreeDtd((xmlDtd *)pCur);
		else if(pCur->type == XML_NAMESPACE_DECL)
			xmlFreeNs((xmlNs *)pCur);
		else if(pCur->type == XML_ATTRIBUTE_NODE)
			xmlFreeProp((xmlAttr *)pCur);
		else {
			xmlDict * p_dict = 0;
			if((__xmlRegisterCallbacks) && (xmlDeregisterNodeDefaultValue))
				xmlDeregisterNodeDefaultValue(pCur);
			if(pCur->doc)
				p_dict = pCur->doc->dict;
			if(pCur->type == XML_ENTITY_DECL) {
				xmlEntity * p_ent = (xmlEntity *)pCur;
				XmlDestroyStringWithDict(p_dict, (xmlChar *)p_ent->SystemID); // @badcast
				XmlDestroyStringWithDict(p_dict, (xmlChar *)p_ent->ExternalID); // @badcast
			}
			if(pCur->children && pCur->type != XML_ENTITY_REF_NODE)
				xmlFreeNodeList(pCur->children);
			if(oneof3(pCur->type, XML_ELEMENT_NODE, XML_XINCLUDE_START, XML_XINCLUDE_END) && pCur->properties)
				xmlFreePropList(pCur->properties);
			if(!oneof4(pCur->type, XML_ELEMENT_NODE, XML_ENTITY_REF_NODE, XML_XINCLUDE_END, XML_XINCLUDE_START) && pCur->content && (pCur->content != (xmlChar *)&(pCur->properties))) {
				XmlDestroyStringWithDict(p_dict, pCur->content);
			}
			/*
			 * When a node is a text node or a comment, it uses a global static
			 * variable for the name of the node.
			 * Otherwise the node name might come from the document's dictionnary
			 */
			if(pCur->name && !oneof2(pCur->type, XML_TEXT_NODE, XML_COMMENT_NODE))
				XmlDestroyStringWithDict(p_dict, (xmlChar *)pCur->name); // @badcast
			if(oneof3(pCur->type, XML_ELEMENT_NODE, XML_XINCLUDE_START, XML_XINCLUDE_END) && pCur->nsDef)
				xmlFreeNsList(pCur->nsDef);
			SAlloc::F(pCur);
		}
	}
}
/**
 * xmlUnlinkNode:
 * @cur:  the node
 *
 * Unlink a node from it's current context, the node is not freed
 * If one need to free the node, use xmlFreeNode() routine after the
 * unlink to discard it.
 * Note that namespace nodes can't be unlinked as they do not have
 * pointer to their parent.
 */
void FASTCALL xmlUnlinkNode(xmlNode * cur)
{
	if(!cur) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlUnlinkNode : node == NULL\n");
#endif
		return;
	}
	if(cur->type == XML_NAMESPACE_DECL)
		return;
	if(cur->type == XML_DTD_NODE) {
		xmlDoc * doc = cur->doc;
		if(doc) {
			if(doc->intSubset == (xmlDtd *)cur)
				doc->intSubset = NULL;
			if(doc->extSubset == (xmlDtd *)cur)
				doc->extSubset = NULL;
		}
	}
	if(cur->type == XML_ENTITY_DECL) {
		xmlDoc * doc = cur->doc;
		if(doc) {
			if(doc->intSubset) {
				if(xmlHashLookup((xmlHashTable *)doc->intSubset->entities, cur->name) == cur)
					xmlHashRemoveEntry((xmlHashTable *)doc->intSubset->entities, cur->name, 0);
				if(xmlHashLookup((xmlHashTable *)doc->intSubset->pentities, cur->name) == cur)
					xmlHashRemoveEntry((xmlHashTable *)doc->intSubset->pentities, cur->name, 0);
			}
			if(doc->extSubset) {
				if(xmlHashLookup((xmlHashTable *)doc->extSubset->entities, cur->name) == cur)
					xmlHashRemoveEntry((xmlHashTable *)doc->extSubset->entities, cur->name, 0);
				if(xmlHashLookup((xmlHashTable *)doc->extSubset->pentities, cur->name) == cur)
					xmlHashRemoveEntry((xmlHashTable *)doc->extSubset->pentities, cur->name, 0);
			}
		}
	}
	if(cur->P_ParentNode) {
		xmlNode * parent = cur->P_ParentNode;
		if(cur->type == XML_ATTRIBUTE_NODE) {
			if(parent->properties == (xmlAttr *)cur)
				parent->properties = ((xmlAttr *)cur)->next;
		}
		else {
			if(parent->children == cur)
				parent->children = cur->next;
			if(parent->last == cur)
				parent->last = cur->prev;
		}
		cur->P_ParentNode = NULL;
	}
	if(cur->next)
		cur->next->prev = cur->prev;
	if(cur->prev)
		cur->prev->next = cur->next;
	cur->next = cur->prev = NULL;
}

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED)
/**
 * xmlReplaceNode:
 * @old:  the old node
 * @cur:  the node
 *
 * Unlink the old node from its current context, prune the new one
 * at the same place. If @cur was already inserted in a document it is
 * first unlinked from its existing context.
 *
 * Returns the @old node
 */
xmlNode * xmlReplaceNode(xmlNode * old, xmlNode * cur)
{
	if(old == cur) 
		return 0;
	if(!old || (old->type == XML_NAMESPACE_DECL) || !old->P_ParentNode) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlReplaceNode : old == NULL or without parent\n");
#endif
		return 0;
	}
	if(!cur || (cur->type == XML_NAMESPACE_DECL)) {
		xmlUnlinkNode(old);
		return old;
	}
	if(cur == old) {
		return old;
	}
	if((old->type==XML_ATTRIBUTE_NODE) && (cur->type!=XML_ATTRIBUTE_NODE)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlReplaceNode : Trying to replace attribute node with other node type\n");
#endif
		return old;
	}
	if((cur->type==XML_ATTRIBUTE_NODE) && (old->type!=XML_ATTRIBUTE_NODE)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlReplaceNode : Trying to replace a non-attribute node with attribute node\n");
#endif
		return old;
	}
	xmlUnlinkNode(cur);
	xmlSetTreeDoc(cur, old->doc);
	cur->P_ParentNode = old->P_ParentNode;
	cur->next = old->next;
	if(cur->next)
		cur->next->prev = cur;
	cur->prev = old->prev;
	if(cur->prev)
		cur->prev->next = cur;
	if(cur->P_ParentNode) {
		if(cur->type == XML_ATTRIBUTE_NODE) {
			if(cur->P_ParentNode->properties == (xmlAttr *)old)
				cur->P_ParentNode->properties = ((xmlAttr *)cur);
		}
		else {
			if(cur->P_ParentNode->children == old)
				cur->P_ParentNode->children = cur;
			if(cur->P_ParentNode->last == old)
				cur->P_ParentNode->last = cur;
		}
	}
	old->next = old->prev = NULL;
	old->P_ParentNode = NULL;
	return old;
}

#endif /* LIBXML_TREE_ENABLED */
// 
// Copy operations
// 
/**
 * xmlCopyNamespace:
 * @cur:  the namespace
 *
 * Do a copy of the namespace.
 *
 * Returns: a new #xmlNsPtr, or NULL in case of error.
 */
xmlNs * xmlCopyNamespace(xmlNs * cur)
{
	xmlNs * ret = 0;
	if(cur) {
		switch(cur->type) {
			case XML_LOCAL_NAMESPACE:
				ret = xmlNewNs(NULL, cur->href, cur->prefix);
				break;
			default:
#ifdef DEBUG_TREE
				xmlGenericError(0, "xmlCopyNamespace: invalid type %d\n", cur->type);
#endif
				break;
		}
	}
	return ret;
}
/**
 * xmlCopyNamespaceList:
 * @cur:  the first namespace
 *
 * Do a copy of an namespace list.
 *
 * Returns: a new #xmlNsPtr, or NULL in case of error.
 */
xmlNs * xmlCopyNamespaceList(xmlNs * cur)
{
	xmlNs * ret = NULL;
	xmlNs * p = NULL;
	while(cur) {
		xmlNs * q = xmlCopyNamespace(cur);
		if(!p) {
			ret = p = q;
		}
		else {
			p->next = q;
			p = q;
		}
		cur = cur->next;
	}
	return ret;
}

static xmlNode * xmlStaticCopyNodeList(xmlNode * P_Node, xmlDoc * doc, xmlNode * parent);

static xmlAttr * xmlCopyPropInternal(xmlDoc * doc, xmlNode * target, xmlAttr * cur)
{
	xmlAttr * ret;
	if(!cur)
		return 0;
	if(target && target->type != XML_ELEMENT_NODE)
		return 0;
	if(target)
		ret = xmlNewDocProp(target->doc, cur->name, 0);
	else if(doc)
		ret = xmlNewDocProp(doc, cur->name, 0);
	else if(cur->parent)
		ret = xmlNewDocProp(cur->parent->doc, cur->name, 0);
	else if(cur->children)
		ret = xmlNewDocProp(cur->children->doc, cur->name, 0);
	else
		ret = xmlNewDocProp(NULL, cur->name, 0);
	if(!ret)
		return 0;
	ret->parent = target;
	if(cur->ns && target) {
		xmlNs * ns = xmlSearchNs(target->doc, target, cur->ns->prefix);
		if(!ns) {
			/*
			 * Humm, we are copying an element whose namespace is defined
			 * out of the new tree scope. Search it in the original tree
			 * and add it at the top of the new tree
			 */
			ns = xmlSearchNs(cur->doc, cur->parent, cur->ns->prefix);
			if(ns) {
				xmlNode * root = target;
				xmlNode * pred = NULL;
				while(root->P_ParentNode) {
					pred = root;
					root = root->P_ParentNode;
				}
				if(root == (xmlNode *)target->doc) {
					root = pred; // correct possibly cycling above the document elt
				}
				ret->ns = xmlNewNs(root, ns->href, ns->prefix);
			}
		}
		else {
			/*
			 * we have to find something appropriate here since
			 * we cant be sure, that the namespce we found is identified
			 * by the prefix
			 */
			if(sstreq(ns->href, cur->ns->href)) {
				ret->ns = ns; // this is the nice case
			}
			else {
				/*
				 * we are in trouble: we need a new reconcilied namespace.
				 * This is expensive
				 */
				ret->ns = xmlNewReconciliedNs(target->doc, target, cur->ns);
			}
		}
	}
	else
		ret->ns = NULL;
	if(cur->children) {
		ret->children = xmlStaticCopyNodeList(cur->children, ret->doc, (xmlNode *)ret);
		ret->last = NULL;
		for(xmlNode * tmp = ret->children; tmp; tmp = tmp->next) {
			/* tmp->parent = reinterpret_cast<xmlNode *>(ret); */
			if(tmp->next == NULL)
				ret->last = tmp;
		}
	}
	/*
	 * Try to handle IDs
	 */
	if(target && cur && target->doc && cur->doc && cur->doc->ids && cur->parent) {
		if(xmlIsID(cur->doc, cur->parent, cur)) {
			xmlChar * id = xmlNodeListGetString(cur->doc, cur->children, 1);
			if(id) {
				xmlAddID(NULL, target->doc, id, ret);
				SAlloc::F(id);
			}
		}
	}
	return ret;
}
/**
 * xmlCopyProp:
 * @target:  the element where the attribute will be grafted
 * @cur:  the attribute
 *
 * Do a copy of the attribute.
 *
 * Returns: a new #xmlAttrPtr, or NULL in case of error.
 */
xmlAttr * xmlCopyProp(xmlNode * target, xmlAttr * cur)
{
	return xmlCopyPropInternal(NULL, target, cur);
}
/**
 * xmlCopyPropList:
 * @target:  the element where the attributes will be grafted
 * @cur:  the first attribute
 *
 * Do a copy of an attribute list.
 *
 * Returns: a new #xmlAttrPtr, or NULL in case of error.
 */
xmlAttr * xmlCopyPropList(xmlNode * target, xmlAttr * cur)
{
	xmlAttr * ret = NULL;
	xmlAttr * p = NULL;
	xmlAttr * q;
	if(target && (target->type != XML_ELEMENT_NODE))
		return 0;
	while(cur) {
		q = xmlCopyProp(target, cur);
		if(q == NULL)
			return 0;
		if(!p) {
			ret = p = q;
		}
		else {
			p->next = q;
			q->prev = p;
			p = q;
		}
		cur = cur->next;
	}
	return ret;
}

/*
 * NOTE about the CopyNode operations !
 *
 * They are split into external and internal parts for one
 * tricky reason: namespaces. Doing a direct copy of a node
 * say RPM:Copyright without changing the namespace pointer to
 * something else can produce stale links. One way to do it is
 * to keep a reference counter but this doesn't work as soon
 * as one move the element or the subtree out of the scope of
 * the existing namespace. The actual solution seems to add
 * a copy of the namespace at the top of the copied tree if
 * not available in the subtree.
 * Hence two functions, the public front-end call the inner ones
 * The argument "recursive" normally indicates a recursive copy
 * of the node with values 0 (no) and 1 (yes).  For XInclude,
 * however, we allow a value of 2 to indicate copy properties and
 * namespace info, but don't recurse on children.
 */
static xmlNode * xmlStaticCopyNode(xmlNode * pNode, xmlDoc * doc, xmlNode * parent, int extended)
{
	xmlNode * ret;
	if(!pNode)
		return 0;
	switch(pNode->type) {
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_ELEMENT_NODE:
		case XML_DOCUMENT_FRAG_NODE:
		case XML_ENTITY_REF_NODE:
		case XML_ENTITY_NODE:
		case XML_PI_NODE:
		case XML_COMMENT_NODE:
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END:
		    break;
		case XML_ATTRIBUTE_NODE:
		    return ((xmlNode *)xmlCopyPropInternal(doc, parent, (xmlAttr *)pNode));
		case XML_NAMESPACE_DECL:
		    return ((xmlNode *)xmlCopyNamespaceList((xmlNs *)pNode));

		case XML_DOCUMENT_NODE:
		case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
		case XML_DOCB_DOCUMENT_NODE:
#endif
#ifdef LIBXML_TREE_ENABLED
		    return ((xmlNode *)xmlCopyDoc((xmlDoc *)pNode, extended));
#endif /* LIBXML_TREE_ENABLED */
		case XML_DOCUMENT_TYPE_NODE:
		case XML_NOTATION_NODE:
		case XML_DTD_NODE:
		case XML_ELEMENT_DECL:
		case XML_ATTRIBUTE_DECL:
		case XML_ENTITY_DECL:
		    return 0;
	}
	/*
	 * Allocate a new node and fill the fields.
	 */
	ret = static_cast<xmlNode *>(SAlloc::M(sizeof(xmlNode)));
	if(!ret) {
		xmlTreeErrMemory("copying node");
		return 0;
	}
	memzero(ret, sizeof(xmlNode));
	ret->type = pNode->type;
	ret->doc = doc;
	ret->P_ParentNode = parent;
	if(pNode->name == xmlStringText)
		ret->name = xmlStringText;
	else if(pNode->name == xmlStringTextNoenc)
		ret->name = xmlStringTextNoenc;
	else if(pNode->name == xmlStringComment)
		ret->name = xmlStringComment;
	else if(pNode->name) {
		if(doc && doc->dict)
			ret->name = xmlDictLookupSL(doc->dict, pNode->name);
		else
			ret->name = sstrdup(pNode->name);
	}
	if((pNode->type != XML_ELEMENT_NODE) && pNode->content && !oneof3(pNode->type, XML_ENTITY_REF_NODE, XML_XINCLUDE_END, XML_XINCLUDE_START))
		ret->content = sstrdup(pNode->content);
	else if(pNode->type == XML_ELEMENT_NODE)
		ret->line = pNode->line;
	if(parent) {
		xmlNode * tmp;
		/*
		 * this is a tricky part for the node register thing:
		 * in case ret does get coalesced in xmlAddChild
		 * the deregister-node callback is called; so we register ret now already
		 */
		if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
			xmlRegisterNodeDefaultValue((xmlNode *)ret);
		tmp = xmlAddChild(parent, ret);
		/* node could have coalesced */
		if(tmp != ret)
			return tmp;
	}
	if(!extended)
		goto out;
	if(((pNode->type == XML_ELEMENT_NODE) || (pNode->type == XML_XINCLUDE_START)) && pNode->nsDef)
		ret->nsDef = xmlCopyNamespaceList(pNode->nsDef);
	if(pNode->ns) {
		xmlNs * ns = xmlSearchNs(doc, ret, pNode->ns->prefix);
		if(ns == NULL) {
			/*
			 * Humm, we are copying an element whose namespace is defined
			 * out of the new tree scope. Search it in the original tree
			 * and add it at the top of the new tree
			 */
			ns = xmlSearchNs(pNode->doc, pNode, pNode->ns->prefix);
			if(ns) {
				xmlNode * root = ret;
				while(root->P_ParentNode)
					root = root->P_ParentNode;
				ret->ns = xmlNewNs(root, ns->href, ns->prefix);
			}
			else {
				ret->ns = xmlNewReconciliedNs(doc, ret, pNode->ns);
			}
		}
		else {
			/*
			 * reference the existing namespace definition in our own tree.
			 */
			ret->ns = ns;
		}
	}
	if(oneof2(pNode->type, XML_ELEMENT_NODE, XML_XINCLUDE_START) && pNode->properties)
		ret->properties = xmlCopyPropList(ret, pNode->properties);
	if(pNode->type == XML_ENTITY_REF_NODE) {
		if(!doc || (pNode->doc != doc)) {
			/*
			 * The copied node will go into a separate document, so
			 * to avoid dangling references to the ENTITY_DECL node
			 * we cannot keep the reference. Try to find it in the
			 * target document.
			 */
			ret->children = (xmlNode *)xmlGetDocEntity(doc, ret->name);
		}
		else {
			ret->children = pNode->children;
		}
		ret->last = ret->children;
	}
	else if(pNode->children && (extended != 2)) {
		ret->children = xmlStaticCopyNodeList(pNode->children, doc, ret);
		UPDATE_LAST_CHILD_AND_PARENT(ret)
	}
out:
	// if parent != NULL we already registered the node above 
	if((parent == NULL) && ((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue)))
		xmlRegisterNodeDefaultValue((xmlNode *)ret);
	return ret;
}

static xmlNode * xmlStaticCopyNodeList(xmlNode * pNode, xmlDoc * doc, xmlNode * parent)
{
	xmlNode * ret = NULL;
	xmlNode * p = NULL;
	xmlNode * q;
	while(pNode) {
#ifdef LIBXML_TREE_ENABLED
		if(pNode->type == XML_DTD_NODE) {
			if(!doc) {
				pNode = pNode->next;
				continue;
			}
			if(doc->intSubset == NULL) {
				q = (xmlNode *)xmlCopyDtd((xmlDtd *)pNode);
				if(q == NULL)
					return 0;
				q->doc = doc;
				q->P_ParentNode = parent;
				doc->intSubset = (xmlDtd *)q;
				xmlAddChild(parent, q);
			}
			else {
				q = (xmlNode *)doc->intSubset;
				xmlAddChild(parent, q);
			}
		}
		else
#endif /* LIBXML_TREE_ENABLED */
		q = xmlStaticCopyNode(pNode, doc, parent, 1);
		if(q == NULL)
			return 0;
		if(!ret) {
			q->prev = NULL;
			ret = p = q;
		}
		else if(p != q) {
			/* the test is required if xmlStaticCopyNode coalesced 2 text nodes */
			p->next = q;
			q->prev = p;
			p = q;
		}
		pNode = pNode->next;
	}
	return ret;
}
/**
 * xmlCopyNode:
 * @node:  the node
 * @extended:   if 1 do a recursive copy (properties, namespaces and children
 *			when applicable)
 *		if 2 copy properties and namespaces (when applicable)
 *
 * Do a copy of the node.
 *
 * Returns: a new #xmlNodePtr, or NULL in case of error.
 */
xmlNode * xmlCopyNode(xmlNode * P_Node, int extended)
{
	return xmlStaticCopyNode(P_Node, NULL, NULL, extended);
}
/**
 * xmlDocCopyNode:
 * @node:  the node
 * @doc:  the document
 * @extended:   if 1 do a recursive copy (properties, namespaces and children
 *			when applicable)
 *		if 2 copy properties and namespaces (when applicable)
 *
 * Do a copy of the node to a given document.
 *
 * Returns: a new #xmlNodePtr, or NULL in case of error.
 */
xmlNode * xmlDocCopyNode(xmlNode * P_Node, xmlDoc * doc, int extended)
{
	return xmlStaticCopyNode(P_Node, doc, NULL, extended);
}

/**
 * xmlDocCopyNodeList:
 * @doc: the target document
 * @node:  the first node in the list.
 *
 * Do a recursive copy of the node list.
 *
 * Returns: a new #xmlNodePtr, or NULL in case of error.
 */
xmlNode * xmlDocCopyNodeList(xmlDoc * doc, xmlNode * pNode)
{
	return xmlStaticCopyNodeList(pNode, doc, 0);
}
/**
 * xmlCopyNodeList:
 * @node:  the first node in the list.
 *
 * Do a recursive copy of the node list.
 * Use xmlDocCopyNodeList() if possible to ensure string interning.
 *
 * Returns: a new #xmlNodePtr, or NULL in case of error.
 */
xmlNode * xmlCopyNodeList(xmlNode * pNode)
{
	return xmlStaticCopyNodeList(pNode, 0, 0);
}

#if defined(LIBXML_TREE_ENABLED)
/**
 * xmlCopyDtd:
 * @dtd:  the dtd
 *
 * Do a copy of the dtd.
 *
 * Returns: a new #xmlDtdPtr, or NULL in case of error.
 */
xmlDtd * xmlCopyDtd(xmlDtd * dtd)
{
	xmlDtd * ret;
	xmlNode * cur;
	xmlNode * p = NULL;
	xmlNode * q;
	if(dtd == NULL) 
		return 0;
	ret = xmlNewDtd(NULL, dtd->name, dtd->ExternalID, dtd->SystemID);
	if(!ret) 
		return 0;
	if(dtd->entities)
		ret->entities = xmlCopyEntitiesTable((xmlEntitiesTable *)dtd->entities);
	if(dtd->notations)
		ret->notations = xmlCopyNotationTable((xmlNotationTable *)dtd->notations);
	if(dtd->elements)
		ret->elements = xmlCopyElementTable((xmlElementTable *)dtd->elements);
	if(dtd->attributes)
		ret->attributes = xmlCopyAttributeTable((xmlAttributeTable *)dtd->attributes);
	if(dtd->pentities)
		ret->pentities = xmlCopyEntitiesTable((xmlEntitiesTable *)dtd->pentities);
	cur = dtd->children;
	while(cur) {
		q = NULL;
		if(cur->type == XML_ENTITY_DECL) {
			xmlEntity * tmp = (xmlEntity *)cur;
			switch(tmp->etype) {
				case XML_INTERNAL_GENERAL_ENTITY:
				case XML_EXTERNAL_GENERAL_PARSED_ENTITY:
				case XML_EXTERNAL_GENERAL_UNPARSED_ENTITY:
				    q = (xmlNode *)xmlGetEntityFromDtd(ret, tmp->name);
				    break;
				case XML_INTERNAL_PARAMETER_ENTITY:
				case XML_EXTERNAL_PARAMETER_ENTITY:
				    q = (xmlNode *)xmlGetParameterEntityFromDtd(ret, tmp->name);
				    break;
				case XML_INTERNAL_PREDEFINED_ENTITY:
				    break;
			}
		}
		else if(cur->type == XML_ELEMENT_DECL) {
			xmlElement * tmp = (xmlElement *)cur;
			q = (xmlNode *)xmlGetDtdQElementDesc(ret, tmp->name, tmp->prefix);
		}
		else if(cur->type == XML_ATTRIBUTE_DECL) {
			xmlAttribute * tmp = (xmlAttribute *)cur;
			q = (xmlNode *)xmlGetDtdQAttrDesc(ret, tmp->elem, tmp->name, tmp->prefix);
		}
		else if(cur->type == XML_COMMENT_NODE) {
			q = xmlCopyNode(cur, 0);
		}
		if(q == NULL) {
			cur = cur->next;
			continue;
		}
		if(!p)
			ret->children = q;
		else
			p->next = q;
		q->prev = p;
		q->P_ParentNode = reinterpret_cast<xmlNode *>(ret);
		q->next = NULL;
		ret->last = q;
		p = q;
		cur = cur->next;
	}
	return ret;
}

#endif

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
/**
 * xmlCopyDoc:
 * @doc:  the document
 * @recursive:  if not zero do a recursive copy.
 *
 * Do a copy of the document info. If recursive, the content tree will
 * be copied too as well as DTD, namespaces and entities.
 *
 * Returns: a new #xmlDocPtr, or NULL in case of error.
 */
xmlDoc * xmlCopyDoc(xmlDoc * doc, int recursive)
{
	xmlDoc * ret;
	if(!doc) 
		return 0;
	ret = xmlNewDoc(doc->version);
	if(!ret) 
		return 0;
	ret->name = sstrdup(doc->name);
	ret->encoding = sstrdup(doc->encoding);
	ret->URL = sstrdup(doc->URL);
	ret->charset = doc->charset;
	ret->compression = doc->compression;
	ret->standalone = doc->standalone;
	if(!recursive) 
		return ret;
	ret->last = NULL;
	ret->children = NULL;
#ifdef LIBXML_TREE_ENABLED
	if(doc->intSubset) {
		ret->intSubset = xmlCopyDtd(doc->intSubset);
		if(ret->intSubset == NULL) {
			xmlFreeDoc(ret);
			return 0;
		}
		xmlSetTreeDoc((xmlNode *)ret->intSubset, ret);
		ret->intSubset->P_ParentNode = reinterpret_cast<xmlNode *>(ret);
	}
#endif
	if(doc->oldNs)
		ret->oldNs = xmlCopyNamespaceList(doc->oldNs);
	if(doc->children) {
		ret->children = xmlStaticCopyNodeList(doc->children, ret, (xmlNode *)ret);
		ret->last = NULL;
		xmlNode * tmp = ret->children;
		while(tmp) {
			if(tmp->next == NULL)
				ret->last = tmp;
			tmp = tmp->next;
		}
	}
	return ret;
}

#endif /* LIBXML_TREE_ENABLED */
//
// Content access functions
//
/**
 * xmlGetLineNoInternal:
 * @node: valid node
 * @depth: used to limit any risk of recursion
 *
 * Get line number of @node.
 * Try to override the limitation of lines being store in 16 bits ints
 *
 * Returns the line number if successful, -1 otherwise
 */
static long xmlGetLineNoInternal(const xmlNode * P_Node, int depth)
{
	long result = -1;
	if(depth < 5) {
		if(P_Node) {
			if(oneof4(P_Node->type, XML_ELEMENT_NODE, XML_TEXT_NODE, XML_COMMENT_NODE, XML_PI_NODE)) {
				if(P_Node->line == 65535) {
					if((P_Node->type == XML_TEXT_NODE) && P_Node->psvi)
						result = (long)P_Node->psvi;
					else if((P_Node->type == XML_ELEMENT_NODE) && P_Node->children)
						result = xmlGetLineNoInternal(P_Node->children, depth + 1);
					else if(P_Node->next)
						result = xmlGetLineNoInternal(P_Node->next, depth + 1);
					else if(P_Node->prev)
						result = xmlGetLineNoInternal(P_Node->prev, depth + 1);
				}
				if((result == -1) || (result == 65535))
					result = (long)P_Node->line;
			}
			else if(P_Node->prev && oneof4(P_Node->prev->type, XML_ELEMENT_NODE, XML_TEXT_NODE, XML_COMMENT_NODE, XML_PI_NODE))
				result = xmlGetLineNoInternal(P_Node->prev, depth + 1);
			else if(P_Node->P_ParentNode && (P_Node->P_ParentNode->type == XML_ELEMENT_NODE))
				result = xmlGetLineNoInternal(P_Node->P_ParentNode, depth + 1);
		}
	}
	return result;
}

/**
 * xmlGetLineNo:
 * @node: valid node
 *
 * Get line number of @node.
 * Try to override the limitation of lines being store in 16 bits ints
 * if XML_PARSE_BIG_LINES parser option was used
 *
 * Returns the line number if successful, -1 otherwise
 */
long xmlGetLineNo(const xmlNode * P_Node)
{
	return xmlGetLineNoInternal(P_Node, 0);
}

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_DEBUG_ENABLED)
/**
 * xmlGetNodePath:
 * @node: a node
 *
 * Build a structure based Path for the given node
 *
 * Returns the new path or NULL in case of error. The caller must free
 *   the returned string
 */
xmlChar * xmlGetNodePath(const xmlNode * P_Node)
{
	const xmlNode * cur, * tmp, * next;
	xmlChar * buffer = NULL, * temp;
	size_t buf_len;
	xmlChar * buf;
	const char * sep;
	const char * name;
	char nametemp[100];
	int occur = 0, generic;
	if(!P_Node || (P_Node->type == XML_NAMESPACE_DECL))
		return 0;
	buf_len = 500;
	buffer = static_cast<xmlChar *>(SAlloc::M(buf_len * sizeof(xmlChar)));
	if(!buffer) {
		xmlTreeErrMemory("getting node path");
		return 0;
	}
	buf = static_cast<xmlChar *>(SAlloc::M(buf_len * sizeof(xmlChar)));
	if(!buf) {
		xmlTreeErrMemory("getting node path");
		SAlloc::F(buffer);
		return 0;
	}

	buffer[0] = 0;
	cur = P_Node;
	do {
		name = "";
		sep = "?";
		occur = 0;
		if((cur->type == XML_DOCUMENT_NODE) || (cur->type == XML_HTML_DOCUMENT_NODE)) {
			if(buffer[0] == '/')
				break;
			sep = "/";
			next = NULL;
		}
		else if(cur->type == XML_ELEMENT_NODE) {
			generic = 0;
			sep = "/";
			name = (const char *)cur->name;
			if(cur->ns) {
				if(cur->ns->prefix) {
					snprintf(nametemp, sizeof(nametemp) - 1, "%s:%s", (char *)cur->ns->prefix, (char *)cur->name);
					nametemp[sizeof(nametemp) - 1] = 0;
					name = nametemp;
				}
				else {
					/*
					 * We cannot express named elements in the default
					 * namespace, so use "*".
					 */
					generic = 1;
					name = "*";
				}
			}
			next = cur->P_ParentNode;
			/*
			 * Thumbler index computation
			 * @todo the ocurence test seems bogus for namespaced names
			 */
			tmp = cur->prev;
			while(tmp) {
				if((tmp->type == XML_ELEMENT_NODE) && (generic || (sstreq(cur->name, tmp->name) &&
					((tmp->ns == cur->ns) || (tmp->ns && cur->ns && (sstreq(cur->ns->prefix, tmp->ns->prefix)))))))
					occur++;
				tmp = tmp->prev;
			}
			if(occur == 0) {
				tmp = cur->next;
				while(tmp && occur == 0) {
					if((tmp->type == XML_ELEMENT_NODE) && (generic || (sstreq(cur->name, tmp->name) &&
						((tmp->ns == cur->ns) || (tmp->ns && cur->ns && (sstreq(cur->ns->prefix, tmp->ns->prefix)))))))
						occur++;
					tmp = tmp->next;
				}
				if(occur != 0)
					occur = 1;
			}
			else
				occur++;
		}
		else if(cur->type == XML_COMMENT_NODE) {
			sep = "/";
			name = "comment()";
			next = cur->P_ParentNode;
			/*
			 * Thumbler index computation
			 */
			tmp = cur->prev;
			while(tmp) {
				if(tmp->type == XML_COMMENT_NODE)
					occur++;
				tmp = tmp->prev;
			}
			if(occur == 0) {
				tmp = cur->next;
				while(tmp && occur == 0) {
					if(tmp->type == XML_COMMENT_NODE)
						occur++;
					tmp = tmp->next;
				}
				if(occur != 0)
					occur = 1;
			}
			else
				occur++;
		}
		else if((cur->type == XML_TEXT_NODE) || (cur->type == XML_CDATA_SECTION_NODE)) {
			sep = "/";
			name = "text()";
			next = cur->P_ParentNode;
			/*
			 * Thumbler index computation
			 */
			tmp = cur->prev;
			while(tmp) {
				if((tmp->type == XML_TEXT_NODE) || (tmp->type == XML_CDATA_SECTION_NODE))
					occur++;
				tmp = tmp->prev;
			}
			/*
			 * Evaluate if this is the only text- or CDATA-section-node;
			 * if yes, then we'll get "text()", otherwise "text()[1]".
			 */
			if(occur == 0) {
				tmp = cur->next;
				while(tmp) {
					if((tmp->type == XML_TEXT_NODE) || (tmp->type == XML_CDATA_SECTION_NODE)) {
						occur = 1;
						break;
					}
					tmp = tmp->next;
				}
			}
			else
				occur++;
		}
		else if(cur->type == XML_PI_NODE) {
			sep = "/";
			snprintf(nametemp, sizeof(nametemp) - 1, "processing-instruction('%s')", (char *)cur->name);
			nametemp[sizeof(nametemp) - 1] = 0;
			name = nametemp;
			next = cur->P_ParentNode;
			/*
			 * Thumbler index computation
			 */
			tmp = cur->prev;
			while(tmp) {
				if((tmp->type == XML_PI_NODE) && (sstreq(cur->name, tmp->name)))
					occur++;
				tmp = tmp->prev;
			}
			if(occur == 0) {
				tmp = cur->next;
				while(tmp && occur == 0) {
					if((tmp->type == XML_PI_NODE) && (sstreq(cur->name, tmp->name)))
						occur++;
					tmp = tmp->next;
				}
				if(occur != 0)
					occur = 1;
			}
			else
				occur++;
		}
		else if(cur->type == XML_ATTRIBUTE_NODE) {
			sep = "/@";
			name = (const char *)(((xmlAttr *)cur)->name);
			if(cur->ns) {
				if(cur->ns->prefix)
					snprintf(nametemp, sizeof(nametemp) - 1, "%s:%s", (char *)cur->ns->prefix, (char *)cur->name);
				else
					snprintf(nametemp, sizeof(nametemp) - 1, "%s", (char *)cur->name);
				nametemp[sizeof(nametemp) - 1] = 0;
				name = nametemp;
			}
			next = ((xmlAttr *)cur)->parent;
		}
		else {
			next = cur->P_ParentNode;
		}
		/*
		 * Make sure there is enough room
		 */
		if(sstrlen(buffer) + sizeof(nametemp) + 20 > buf_len) {
			buf_len = 2 * buf_len + sstrlen(buffer) + sizeof(nametemp) + 20;
			temp = static_cast<xmlChar *>(SAlloc::R(buffer, buf_len));
			if(temp == NULL) {
				xmlTreeErrMemory("getting node path");
				SAlloc::F(buf);
				SAlloc::F(buffer);
				return 0;
			}
			buffer = temp;
			temp = static_cast<xmlChar *>(SAlloc::R(buf, buf_len));
			if(temp == NULL) {
				xmlTreeErrMemory("getting node path");
				SAlloc::F(buf);
				SAlloc::F(buffer);
				return 0;
			}
			buf = temp;
		}
		if(occur == 0)
			snprintf((char *)buf, buf_len, "%s%s%s", sep, name, (char *)buffer);
		else
			snprintf((char *)buf, buf_len, "%s%s[%d]%s", sep, name, occur, (char *)buffer);
		snprintf((char *)buffer, buf_len, "%s", (char *)buf);
		cur = next;
	} while(cur);
	SAlloc::F(buf);
	return (buffer);
}

#endif /* LIBXML_TREE_ENABLED */

/**
 * xmlDocGetRootElement:
 * @doc:  the document
 *
 * Get the root element of the document (doc->children is a list
 * containing possibly comments, PIs, etc ...).
 *
 * Returns the #xmlNodePtr for the root or NULL
 */
xmlNode * FASTCALL xmlDocGetRootElement(const xmlDoc * doc)
{
	xmlNode * ret = 0;
	if(doc) {
		for(ret = doc->children; ret; ret = ret->next) {
			if(ret->type == XML_ELEMENT_NODE)
				break;
		}
	}
	return ret;
}

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED)
/**
 * xmlDocSetRootElement:
 * @doc:  the document
 * @root:  the new document root element, if root is NULL no action is taken,
 *    to remove a node from a document use xmlUnlinkNode(root) instead.
 *
 * Set the root element of the document (doc->children is a list
 * containing possibly comments, PIs, etc ...).
 *
 * Returns the old root element if any was found, NULL if root was NULL
 */
xmlNode * xmlDocSetRootElement(xmlDoc * doc, xmlNode * root)
{
	xmlNode * old = NULL;
	if(!doc) return 0;
	if((root == NULL) || (root->type == XML_NAMESPACE_DECL))
		return 0;
	xmlUnlinkNode(root);
	xmlSetTreeDoc(root, doc);
	root->P_ParentNode = (xmlNode *)doc;
	old = doc->children;
	while(old) {
		if(old->type == XML_ELEMENT_NODE)
			break;
		old = old->next;
	}
	if(old == NULL) {
		if(doc->children == NULL) {
			doc->children = root;
			doc->last = root;
		}
		else {
			xmlAddSibling(doc->children, root);
		}
	}
	else {
		xmlReplaceNode(old, root);
	}
	return old;
}

#endif

#if defined(LIBXML_TREE_ENABLED)
/**
 * xmlNodeSetLang:
 * @cur:  the node being changed
 * @lang:  the language description
 *
 * Set the language of a node, i.e. the values of the xml:lang
 * attribute.
 */
void xmlNodeSetLang(xmlNode * cur, const xmlChar * lang)
{
	xmlNs * ns;
	if(!cur) return;
	switch(cur->type) {
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_COMMENT_NODE:
		case XML_DOCUMENT_NODE:
		case XML_DOCUMENT_TYPE_NODE:
		case XML_DOCUMENT_FRAG_NODE:
		case XML_NOTATION_NODE:
		case XML_HTML_DOCUMENT_NODE:
		case XML_DTD_NODE:
		case XML_ELEMENT_DECL:
		case XML_ATTRIBUTE_DECL:
		case XML_ENTITY_DECL:
		case XML_PI_NODE:
		case XML_ENTITY_REF_NODE:
		case XML_ENTITY_NODE:
		case XML_NAMESPACE_DECL:
#ifdef LIBXML_DOCB_ENABLED
		case XML_DOCB_DOCUMENT_NODE:
#endif
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END:
		    return;
		case XML_ELEMENT_NODE:
		case XML_ATTRIBUTE_NODE:
		    break;
	}
	ns = xmlSearchNsByHref(cur->doc, cur, XML_XML_NAMESPACE);
	if(ns)
		xmlSetNsProp(cur, ns, reinterpret_cast<const xmlChar *>("lang"), lang);
}

#endif /* LIBXML_TREE_ENABLED */

/**
 * xmlNodeGetLang:
 * @cur:  the node being checked
 *
 * Searches the language of a node, i.e. the values of the xml:lang
 * attribute or the one carried by the nearest ancestor.
 *
 * Returns a pointer to the lang value, or NULL if not found
 *   It's up to the caller to free the memory with SAlloc::F().
 */
xmlChar * xmlNodeGetLang(const xmlNode * cur)
{
	if(cur && cur->type != XML_NAMESPACE_DECL) {
		for(; cur; cur = cur->P_ParentNode) {
			xmlChar * lang = xmlGetNsProp(cur, reinterpret_cast<const xmlChar *>("lang"), XML_XML_NAMESPACE);
			if(lang)
				return lang;
		}
	}
	return 0;
}

#ifdef LIBXML_TREE_ENABLED
/**
 * xmlNodeSetSpacePreserve:
 * @cur:  the node being changed
 * @val:  the xml:space value ("0": default, 1: "preserve")
 *
 * Set (or reset) the space preserving behaviour of a node, i.e. the
 * value of the xml:space attribute.
 */
void xmlNodeSetSpacePreserve(xmlNode * cur, int val)
{
	xmlNs * ns;
	if(!cur) 
		return;
	switch(cur->type) {
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_COMMENT_NODE:
		case XML_DOCUMENT_NODE:
		case XML_DOCUMENT_TYPE_NODE:
		case XML_DOCUMENT_FRAG_NODE:
		case XML_NOTATION_NODE:
		case XML_HTML_DOCUMENT_NODE:
		case XML_DTD_NODE:
		case XML_ELEMENT_DECL:
		case XML_ATTRIBUTE_DECL:
		case XML_ENTITY_DECL:
		case XML_PI_NODE:
		case XML_ENTITY_REF_NODE:
		case XML_ENTITY_NODE:
		case XML_NAMESPACE_DECL:
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END:
#ifdef LIBXML_DOCB_ENABLED
		case XML_DOCB_DOCUMENT_NODE:
#endif
		    return;
		case XML_ELEMENT_NODE:
		case XML_ATTRIBUTE_NODE:
		    break;
	}
	ns = xmlSearchNsByHref(cur->doc, cur, XML_XML_NAMESPACE);
	if(ns == NULL)
		return;
	switch(val) {
		case 0:
		    xmlSetNsProp(cur, ns, reinterpret_cast<const xmlChar *>("space"), reinterpret_cast<const xmlChar *>("default"));
		    break;
		case 1:
		    xmlSetNsProp(cur, ns, reinterpret_cast<const xmlChar *>("space"), reinterpret_cast<const xmlChar *>("preserve"));
		    break;
	}
}

#endif /* LIBXML_TREE_ENABLED */

/**
 * xmlNodeGetSpacePreserve:
 * @cur:  the node being checked
 *
 * Searches the space preserving behaviour of a node, i.e. the values
 * of the xml:space attribute or the one carried by the nearest
 * ancestor.
 *
 * Returns -1 if xml:space is not inherited, 0 if "default", 1 if "preserve"
 */
int xmlNodeGetSpacePreserve(const xmlNode * cur)
{
	xmlChar * space;
	if(!cur || (cur->type != XML_ELEMENT_NODE))
		return -1;
	while(cur) {
		space = xmlGetNsProp(cur, reinterpret_cast<const xmlChar *>("space"), XML_XML_NAMESPACE);
		if(space) {
			if(sstreq(space, reinterpret_cast<const xmlChar *>("preserve"))) {
				SAlloc::F(space);
				return 1;
			}
			if(sstreq(space, reinterpret_cast<const xmlChar *>("default"))) {
				SAlloc::F(space);
				return 0;
			}
			SAlloc::F(space);
		}
		cur = cur->P_ParentNode;
	}
	return -1;
}

#ifdef LIBXML_TREE_ENABLED
/**
 * xmlNodeSetName:
 * @cur:  the node being changed
 * @name:  the new tag name
 *
 * Set (or reset) the name of a node.
 */
void xmlNodeSetName(xmlNode * cur, const xmlChar * name)
{
	xmlDoc * doc;
	xmlDict * dict;
	const xmlChar * freeme = NULL;
	if(!cur) return;
	if(!name) return;
	switch(cur->type) {
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_COMMENT_NODE:
		case XML_DOCUMENT_TYPE_NODE:
		case XML_DOCUMENT_FRAG_NODE:
		case XML_NOTATION_NODE:
		case XML_HTML_DOCUMENT_NODE:
		case XML_NAMESPACE_DECL:
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END:
#ifdef LIBXML_DOCB_ENABLED
		case XML_DOCB_DOCUMENT_NODE:
#endif
		    return;
		case XML_ELEMENT_NODE:
		case XML_ATTRIBUTE_NODE:
		case XML_PI_NODE:
		case XML_ENTITY_REF_NODE:
		case XML_ENTITY_NODE:
		case XML_DTD_NODE:
		case XML_DOCUMENT_NODE:
		case XML_ELEMENT_DECL:
		case XML_ATTRIBUTE_DECL:
		case XML_ENTITY_DECL:
		    break;
	}
	doc = cur->doc;
	dict = doc ? doc->dict : 0;
	if(dict) {
		if(cur->name && (!xmlDictOwns(dict, cur->name)))
			freeme = cur->name;
		cur->name = xmlDictLookupSL(dict, name);
	}
	else {
		if(cur->name)
			freeme = cur->name;
		cur->name = sstrdup(name);
	}
	SAlloc::F((xmlChar *)freeme);
}

#endif

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED)
/**
 * xmlNodeSetBase:
 * @cur:  the node being changed
 * @uri:  the new base URI
 *
 * Set (or reset) the base URI of a node, i.e. the value of the
 * xml:base attribute.
 */
void xmlNodeSetBase(xmlNode * cur, const xmlChar* uri)
{
	if(cur) {
		xmlNs * ns;
		xmlChar * fixed;
		switch(cur->type) {
			case XML_TEXT_NODE:
			case XML_CDATA_SECTION_NODE:
			case XML_COMMENT_NODE:
			case XML_DOCUMENT_TYPE_NODE:
			case XML_DOCUMENT_FRAG_NODE:
			case XML_NOTATION_NODE:
			case XML_DTD_NODE:
			case XML_ELEMENT_DECL:
			case XML_ATTRIBUTE_DECL:
			case XML_ENTITY_DECL:
			case XML_PI_NODE:
			case XML_ENTITY_REF_NODE:
			case XML_ENTITY_NODE:
			case XML_NAMESPACE_DECL:
			case XML_XINCLUDE_START:
			case XML_XINCLUDE_END:
				return;
			case XML_ELEMENT_NODE:
			case XML_ATTRIBUTE_NODE:
				break;
			case XML_DOCUMENT_NODE:
	#ifdef LIBXML_DOCB_ENABLED
			case XML_DOCB_DOCUMENT_NODE:
	#endif
			case XML_HTML_DOCUMENT_NODE: {
				xmlDoc * doc = (xmlDoc *)cur;
				SAlloc::F((xmlChar *)doc->URL);
				doc->URL = uri ? xmlPathToURI(uri) : 0;
				return;
			}
		}
		ns = xmlSearchNsByHref(cur->doc, cur, XML_XML_NAMESPACE);
		if(ns == NULL)
			return;
		fixed = xmlPathToURI(uri);
		if(fixed) {
			xmlSetNsProp(cur, ns, reinterpret_cast<const xmlChar *>("base"), fixed);
			SAlloc::F(fixed);
		}
		else {
			xmlSetNsProp(cur, ns, reinterpret_cast<const xmlChar *>("base"), uri);
		}
	}
}

#endif /* LIBXML_TREE_ENABLED */

/**
 * xmlNodeGetBase:
 * @doc:  the document the node pertains to
 * @cur:  the node being checked
 *
 * Searches for the BASE URL. The code should work on both XML
 * and HTML document even if base mechanisms are completely different.
 * It returns the base as defined in RFC 2396 sections
 * 5.1.1. Base URI within Document Content
 * and
 * 5.1.2. Base URI from the Encapsulating Entity
 * However it does not return the document base (5.1.3), use
 * doc->URL in this case
 *
 * Returns a pointer to the base URL, or NULL if not found
 *   It's up to the caller to free the memory with SAlloc::F().
 */
xmlChar * FASTCALL xmlNodeGetBase(const xmlDoc * doc, const xmlNode * cur)
{
	xmlChar * oldbase = NULL;
	xmlChar * base;
	if(!cur && !doc)
		return 0;
	if(cur && (cur->type == XML_NAMESPACE_DECL))
		return 0;
	if(!doc) 
		doc = cur->doc;
	if(doc && (doc->type == XML_HTML_DOCUMENT_NODE)) {
		cur = doc->children;
		while(cur && cur->name) {
			if(cur->type != XML_ELEMENT_NODE) {
				cur = cur->next;
				continue;
			}
			if(sstreqi_ascii(cur->name, reinterpret_cast<const xmlChar *>("html"))) {
				cur = cur->children;
				continue;
			}
			if(sstreqi_ascii(cur->name, reinterpret_cast<const xmlChar *>("head"))) {
				cur = cur->children;
				continue;
			}
			if(sstreqi_ascii(cur->name, reinterpret_cast<const xmlChar *>("base"))) {
				return xmlGetProp(cur, reinterpret_cast<const xmlChar *>("href"));
			}
			cur = cur->next;
		}
		return 0;
	}
	while(cur) {
		if(cur->type == XML_ENTITY_DECL) {
			const xmlEntity * ent = reinterpret_cast<const xmlEntity *>(cur);
			return sstrdup(ent->URI);
		}
		if(cur->type == XML_ELEMENT_NODE) {
			base = xmlGetNsProp(cur, reinterpret_cast<const xmlChar *>("base"), XML_XML_NAMESPACE);
			if(base) {
				if(oldbase) {
					xmlChar * newbase = xmlBuildURI(oldbase, base);
					if(newbase) {
						SAlloc::F(oldbase);
						SAlloc::F(base);
						oldbase = newbase;
					}
					else {
						SAlloc::F(oldbase);
						SAlloc::F(base);
						return 0;
					}
				}
				else {
					oldbase = base;
				}
				if((!xmlStrncmp(oldbase, reinterpret_cast<const xmlChar *>("http://"), 7)) || (!xmlStrncmp(oldbase, reinterpret_cast<const xmlChar *>("ftp://"), 6)) || (!xmlStrncmp(oldbase, reinterpret_cast<const xmlChar *>("urn:"), 4)))
					return (oldbase);
			}
		}
		cur = cur->P_ParentNode;
	}
	if(doc && doc->URL) {
		if(oldbase == NULL)
			return sstrdup(doc->URL);
		else {
			xmlChar * newbase = xmlBuildURI(oldbase, doc->URL);
			SAlloc::F(oldbase);
			return (newbase);
		}
	}
	return (oldbase);
}
/**
 * xmlNodeBufGetContent:
 * @buffer:  a buffer
 * @cur:  the node being read
 *
 * Read the value of a node @cur, this can be either the text carried
 * directly by this node if it's a TEXT node or the aggregate string
 * of the values carried by this node child's (TEXT and ENTITY_REF).
 * Entity references are substituted.
 * Fills up the buffer @buffer with this value
 *
 * Returns 0 in case of success and -1 in case of error.
 */
int xmlNodeBufGetContent(xmlBuffer * buffer, const xmlNode * cur)
{
	xmlBufPtr buf;
	int ret;
	if(!cur || (buffer == NULL)) 
		return -1;
	buf = xmlBufFromBuffer(buffer);
	ret = xmlBufGetNodeContent(buf, cur);
	buffer = xmlBufBackToBuffer(buf);
	if((ret < 0) || (buffer == NULL))
		return -1;
	return 0;
}
/**
 * xmlBufGetNodeContent:
 * @buf:  a buffer xmlBufPtr
 * @cur:  the node being read
 *
 * Read the value of a node @cur, this can be either the text carried
 * directly by this node if it's a TEXT node or the aggregate string
 * of the values carried by this node child's (TEXT and ENTITY_REF).
 * Entity references are substituted.
 * Fills up the buffer @buf with this value
 *
 * Returns 0 in case of success and -1 in case of error.
 */
int FASTCALL xmlBufGetNodeContent(xmlBuf * buf, const xmlNode * cur)
{
	if(!cur || (buf == NULL)) 
		return -1;
	switch(cur->type) {
		case XML_CDATA_SECTION_NODE:
		case XML_TEXT_NODE:
		    xmlBufCat(buf, cur->content);
		    break;
		case XML_DOCUMENT_FRAG_NODE:
		case XML_ELEMENT_NODE: 
			{
				const xmlNode * tmp = cur;
				while(tmp) {
					switch(tmp->type) {
						case XML_CDATA_SECTION_NODE:
						case XML_TEXT_NODE:
						if(tmp->content)
							xmlBufCat(buf, tmp->content);
						break;
						case XML_ENTITY_REF_NODE:
						xmlBufGetNodeContent(buf, tmp);
						break;
						default:
						break;
					}
					/*
					 * Skip to next node
					 */
					if(tmp->children) {
						if(tmp->children->type != XML_ENTITY_DECL) {
							tmp = tmp->children;
							continue;
						}
					}
					if(tmp == cur)
						break;
					if(tmp->next) {
						tmp = tmp->next;
						continue;
					}
					do {
						tmp = tmp->P_ParentNode;
						if(!tmp)
							break;
						if(tmp == cur) {
							tmp = NULL;
							break;
						}
						if(tmp->next) {
							tmp = tmp->next;
							break;
						}
					} while(tmp);
				}
				break;
			}
		case XML_ATTRIBUTE_NODE: {
		    xmlAttr * attr = (xmlAttr *)cur;
		    for(xmlNode * tmp = attr->children; tmp; tmp = tmp->next) {
			    if(tmp->type == XML_TEXT_NODE)
				    xmlBufCat(buf, tmp->content);
			    else
				    xmlBufGetNodeContent(buf, tmp);
		    }
		    break;
	    }
		case XML_COMMENT_NODE:
		case XML_PI_NODE:
		    xmlBufCat(buf, cur->content);
		    break;
		case XML_ENTITY_REF_NODE: {
		    xmlNode * tmp;
		    // lookup entity declaration 
		    xmlEntity * ent = xmlGetDocEntity(cur->doc, cur->name);
		    if(ent == NULL)
			    return -1;
		    /* an entity content can be any "well balanced chunk",
		 * i.e. the result of the content [43] production:
		 * http://www.w3.org/TR/REC-xml#NT-content
		 * -> we iterate through child nodes and recursive call
		 * xmlNodeGetContent() which handles all possible node types */
		    for(tmp = ent->children; tmp; tmp = tmp->next)
			    xmlBufGetNodeContent(buf, tmp);
		    break;
	    }
		case XML_ENTITY_NODE:
		case XML_DOCUMENT_TYPE_NODE:
		case XML_NOTATION_NODE:
		case XML_DTD_NODE:
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END:
		    break;
		case XML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
		case XML_DOCB_DOCUMENT_NODE:
#endif
		case XML_HTML_DOCUMENT_NODE:
		    for(cur = cur->children; cur; cur = cur->next) {
			    if(oneof3(cur->type, XML_ELEMENT_NODE, XML_TEXT_NODE, XML_CDATA_SECTION_NODE))
				    xmlBufGetNodeContent(buf, cur);
		    }
		    break;
		case XML_NAMESPACE_DECL:
		    xmlBufCat(buf, ((xmlNs *)cur)->href);
		    break;
		case XML_ELEMENT_DECL:
		case XML_ATTRIBUTE_DECL:
		case XML_ENTITY_DECL:
		    break;
	}
	return 0;
}
/**
 * xmlNodeGetContent:
 * @cur:  the node being read
 *
 * Read the value of a node, this can be either the text carried
 * directly by this node if it's a TEXT node or the aggregate string
 * of the values carried by this node child's (TEXT and ENTITY_REF).
 * Entity references are substituted.
 * Returns a new #xmlChar * or NULL if no content is available.
 *   It's up to the caller to free the memory with SAlloc::F().
 */
xmlChar * FASTCALL xmlNodeGetContent(const xmlNode * cur)
{
	if(cur) {
		switch(cur->type) {
			case XML_DOCUMENT_FRAG_NODE:
			case XML_ELEMENT_NODE: 
				{
					xmlChar * ret = 0;
					xmlBuf * buf = xmlBufCreateSize(64);
					if(buf) {
						xmlBufGetNodeContent(buf, cur);
						ret = xmlBufDetach(buf);
						xmlBufFree(buf);
					}
					return ret;
				}
			case XML_ATTRIBUTE_NODE: return xmlGetPropNodeValueInternal((xmlAttr *)cur);
			case XML_COMMENT_NODE:
			case XML_PI_NODE: return sstrdup(cur->content);
			case XML_ENTITY_REF_NODE: 
				{
					xmlChar * ret = 0;
					// lookup entity declaration 
					xmlEntity * ent = xmlGetDocEntity(cur->doc, cur->name);
					if(ent) {
						xmlBuf * buf = xmlBufCreate();
						if(buf) {
							xmlBufGetNodeContent(buf, cur);
							ret = xmlBufDetach(buf);
							xmlBufFree(buf);
						}
					}
					return ret;
				}
			case XML_ENTITY_NODE:
			case XML_DOCUMENT_TYPE_NODE:
			case XML_NOTATION_NODE:
			case XML_DTD_NODE:
			case XML_XINCLUDE_START:
			case XML_XINCLUDE_END: return 0;
			case XML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
			case XML_DOCB_DOCUMENT_NODE:
#endif
			case XML_HTML_DOCUMENT_NODE: 
				{
					xmlChar * ret = 0;
					xmlBuf * buf = xmlBufCreate();
					if(buf) {
						xmlBufGetNodeContent(buf, (xmlNode *)cur);
						ret = xmlBufDetach(buf);
						xmlBufFree(buf);
					}
					return ret;
				}
			case XML_NAMESPACE_DECL: return sstrdup(((xmlNs *)cur)->href);
			case XML_ELEMENT_DECL:
				/* @todo !!! */
				return 0;
			case XML_ATTRIBUTE_DECL:
				/* @todo !!! */
				return 0;
			case XML_ENTITY_DECL:
				/* @todo !!! */
				return 0;
			case XML_CDATA_SECTION_NODE:
			case XML_TEXT_NODE: return sstrdup(cur->content);
		}
	}
	return 0;
}
/**
 * xmlNodeSetContent:
 * @cur:  the node being modified
 * @content:  the new value of the content
 *
 * Replace the content of a node.
 * NOTE: @content is supposed to be a piece of XML CDATA, so it allows entity
 *  references, but XML special chars need to be escaped first by using
 *  xmlEncodeEntitiesReentrant() resp. xmlEncodeSpecialChars().
 */
void xmlNodeSetContent(xmlNode * cur, const xmlChar * content) 
{
	if(!cur) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNodeSetContent : node == NULL\n");
#endif
		return;
	}
	switch(cur->type) {
		case XML_DOCUMENT_FRAG_NODE:
		case XML_ELEMENT_NODE:
		case XML_ATTRIBUTE_NODE:
		    xmlFreeNodeList(cur->children);
		    cur->children = xmlStringGetNodeList(cur->doc, content);
		    UPDATE_LAST_CHILD_AND_PARENT(cur)
		    break;
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_ENTITY_REF_NODE:
		case XML_ENTITY_NODE:
		case XML_PI_NODE:
		case XML_COMMENT_NODE:
		    if(cur->content && (cur->content != (xmlChar *)&(cur->properties))) {
			    if(!(cur->doc && cur->doc->dict && (xmlDictOwns(cur->doc->dict, cur->content))))
				    SAlloc::F(cur->content);
		    }
			xmlFreeNodeList(cur->children);
		    cur->last = cur->children = NULL;
		    cur->content = sstrdup(content);
		    cur->properties = NULL;
		    cur->nsDef = NULL;
		    break;
		case XML_DOCUMENT_NODE:
		case XML_HTML_DOCUMENT_NODE:
		case XML_DOCUMENT_TYPE_NODE:
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END:
#ifdef LIBXML_DOCB_ENABLED
		case XML_DOCB_DOCUMENT_NODE:
#endif
		    break;
		case XML_NOTATION_NODE: break;
		case XML_DTD_NODE: break;
		case XML_NAMESPACE_DECL: break;
		case XML_ELEMENT_DECL:
		    /* @todo !!! */
		    break;
		case XML_ATTRIBUTE_DECL:
		    /* @todo !!! */
		    break;
		case XML_ENTITY_DECL:
		    /* @todo !!! */
		    break;
	}
}

#ifdef LIBXML_TREE_ENABLED
/**
 * xmlNodeSetContentLen:
 * @cur:  the node being modified
 * @content:  the new value of the content
 * @len:  the size of @content
 *
 * Replace the content of a node.
 * NOTE: @content is supposed to be a piece of XML CDATA, so it allows entity
 *  references, but XML special chars need to be escaped first by using
 *  xmlEncodeEntitiesReentrant() resp. xmlEncodeSpecialChars().
 */
void xmlNodeSetContentLen(xmlNode * cur, const xmlChar * content, int len) 
{
	if(!cur) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNodeSetContentLen : node == NULL\n");
#endif
		return;
	}
	switch(cur->type) {
		case XML_DOCUMENT_FRAG_NODE:
		case XML_ELEMENT_NODE:
		case XML_ATTRIBUTE_NODE:
		    xmlFreeNodeList(cur->children);
		    cur->children = xmlStringLenGetNodeList(cur->doc, content, len);
		    UPDATE_LAST_CHILD_AND_PARENT(cur)
		    break;
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_ENTITY_REF_NODE:
		case XML_ENTITY_NODE:
		case XML_PI_NODE:
		case XML_COMMENT_NODE:
		case XML_NOTATION_NODE:
		    if(cur->content && (cur->content != (xmlChar *)&(cur->properties))) {
			    if(!(cur->doc && cur->doc->dict && (xmlDictOwns(cur->doc->dict, cur->content))))
				    SAlloc::F(cur->content);
		    }
		    xmlFreeNodeList(cur->children);
		    cur->children = cur->last = NULL;
		    cur->content = content ? xmlStrndup(content, len) : 0;
		    cur->properties = NULL;
		    cur->nsDef = NULL;
		    break;
		case XML_DOCUMENT_NODE:
		case XML_DTD_NODE:
		case XML_HTML_DOCUMENT_NODE:
		case XML_DOCUMENT_TYPE_NODE:
		case XML_NAMESPACE_DECL:
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END:
#ifdef LIBXML_DOCB_ENABLED
		case XML_DOCB_DOCUMENT_NODE:
#endif
		    break;
		case XML_ELEMENT_DECL:
		    /* @todo !!! */
		    break;
		case XML_ATTRIBUTE_DECL:
		    /* @todo !!! */
		    break;
		case XML_ENTITY_DECL:
		    /* @todo !!! */
		    break;
	}
}

#endif /* LIBXML_TREE_ENABLED */

/**
 * xmlNodeAddContentLen:
 * @cur:  the node being modified
 * @content:  extra content
 * @len:  the size of @content
 *
 * Append the extra substring to the node content.
 * NOTE: In contrast to xmlNodeSetContentLen(), @content is supposed to be
 *  raw text, so unescaped XML special chars are allowed, entity
 *  references are not supported.
 */
void FASTCALL xmlNodeAddContentLen(xmlNode * cur, const xmlChar * content, int len) 
{
	if(!cur) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNodeAddContentLen : node == NULL\n");
#endif
	}
	else if(len > 0) {
		switch(cur->type) {
			case XML_DOCUMENT_FRAG_NODE:
			case XML_ELEMENT_NODE: 
				{
					xmlNode * last = cur->last;
					for(xmlNode * newNode = xmlNewTextLen(content, len); newNode; ) {
						xmlNode * tmp = xmlAddChild(cur, newNode);
						if(tmp != newNode)
							return;
						if(last && (last->next == newNode))
							xmlTextMerge(last, newNode);
					}
				}
				break;
			case XML_ATTRIBUTE_NODE:
				break;
			case XML_TEXT_NODE:
			case XML_CDATA_SECTION_NODE:
			case XML_ENTITY_REF_NODE:
			case XML_ENTITY_NODE:
			case XML_PI_NODE:
			case XML_COMMENT_NODE:
			case XML_NOTATION_NODE:
				if(content) {
					if((cur->content == (xmlChar *)&(cur->properties)) || (cur->doc && cur->doc->dict && xmlDictOwns(cur->doc->dict, cur->content))) {
						cur->content = xmlStrncatNew(cur->content, content, len);
						cur->properties = NULL;
						cur->nsDef = NULL;
						break;
					}
					cur->content = xmlStrncat(cur->content, content, len);
				}
			case XML_DOCUMENT_NODE:
			case XML_DTD_NODE:
			case XML_HTML_DOCUMENT_NODE:
			case XML_DOCUMENT_TYPE_NODE:
			case XML_NAMESPACE_DECL:
			case XML_XINCLUDE_START:
			case XML_XINCLUDE_END:
#ifdef LIBXML_DOCB_ENABLED
			case XML_DOCB_DOCUMENT_NODE:
#endif
				break;
			case XML_ELEMENT_DECL:
			case XML_ATTRIBUTE_DECL:
			case XML_ENTITY_DECL:
				break;
		}
	}
}
/**
 * xmlNodeAddContent:
 * @cur:  the node being modified
 * @content:  extra content
 *
 * Append the extra substring to the node content.
 * NOTE: In contrast to xmlNodeSetContent(), @content is supposed to be
 *  raw text, so unescaped XML special chars are allowed, entity
 *  references are not supported.
 */
void FASTCALL xmlNodeAddContent(xmlNode * cur, const xmlChar * content) 
{
	if(!cur) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNodeAddContent : node == NULL\n");
#endif
	}
	else if(content) {
		int len = sstrlen(content);
		xmlNodeAddContentLen(cur, content, len);
	}
}
/**
 * xmlTextMerge:
 * @first:  the first text node
 * @second:  the second text node being merged
 *
 * Merge two text nodes into one
 * Returns the first text node augmented
 */
xmlNode * xmlTextMerge(xmlNode * first, xmlNode * second) 
{
	if(!first) 
		return second;
	else {
		if(second && first->type == XML_TEXT_NODE && second->type == XML_TEXT_NODE) {
			if(second->name != first->name)
				return first;
			else {
				xmlNodeAddContent(first, second->content);
				xmlUnlinkNode(second);
				xmlFreeNode(second);
			}
		}
		return first;
	}
}

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
/**
 * xmlGetNsList:
 * @doc:  the document
 * @node:  the current node
 *
 * Search all the namespace applying to a given element.
 * Returns an NULL terminated array of all the #xmlNsPtr found
 *    that need to be freed by the caller or NULL if no
 *    namespace if defined
 */
xmlNs ** xmlGetNsList(const xmlDoc * doc ATTRIBUTE_UNUSED, const xmlNode * P_Node)
{
	xmlNs * cur;
	xmlNs ** ret = NULL;
	int nbns = 0;
	int maxns = 10;
	int i;
	if(!P_Node || (P_Node->type == XML_NAMESPACE_DECL))
		return 0;
	while(P_Node) {
		if(P_Node->type == XML_ELEMENT_NODE) {
			cur = P_Node->nsDef;
			while(cur) {
				if(!ret) {
					ret = static_cast<xmlNs **>(SAlloc::M((maxns + 1) * sizeof(xmlNs *)));
					if(!ret) {
						xmlTreeErrMemory("getting namespace list");
						return 0;
					}
					ret[nbns] = NULL;
				}
				for(i = 0; i < nbns; i++) {
					if((cur->prefix == ret[i]->prefix) || (sstreq(cur->prefix, ret[i]->prefix)))
						break;
				}
				if(i >= nbns) {
					if(nbns >= maxns) {
						maxns *= 2;
						ret = static_cast<xmlNs **>(SAlloc::R(ret, (maxns + 1) * sizeof(xmlNs *)));
						if(!ret) {
							xmlTreeErrMemory("getting namespace list");
							return 0;
						}
					}
					ret[nbns++] = cur;
					ret[nbns] = NULL;
				}

				cur = cur->next;
			}
		}
		P_Node = P_Node->P_ParentNode;
	}
	return ret;
}

#endif /* LIBXML_TREE_ENABLED */
/*
 * xmlTreeEnsureXMLDecl:
 * @doc: the doc
 *
 * Ensures that there is an XML namespace declaration on the doc.
 *
 * Returns the XML ns-struct or NULL on API and internal errors.
 */
static xmlNs * FASTCALL xmlTreeEnsureXMLDecl(xmlDoc * doc)
{
	xmlNs * ns = 0;
	if(doc) {
		if(doc->oldNs)
			ns = doc->oldNs;
		else {
			ns = static_cast<xmlNs *>(SAlloc::M(sizeof(xmlNs)));
			if(ns == NULL) {
				xmlTreeErrMemory("allocating the XML namespace");
			}
			else {
				memzero(ns, sizeof(xmlNs));
				ns->type = XML_LOCAL_NAMESPACE;
				ns->href = sstrdup(XML_XML_NAMESPACE);
				ns->prefix = sstrdup((const xmlChar *)"xml");
				doc->oldNs = ns;
			}
		}
	}
	return ns;
}
/**
 * xmlSearchNs:
 * @doc:  the document
 * @node:  the current node
 * @nameSpace:  the namespace prefix
 *
 * Search a Ns registered under a given name space for a document.
 * recurse on the parents until it finds the defined namespace
 * or return NULL otherwise.
 * @nameSpace can be NULL, this is a search for the default namespace.
 * We don't allow to cross entities boundaries. If you don't declare
 * the namespace within those you will be in troubles !!! A warning
 * is generated to cover this case.
 *
 * Returns the namespace pointer or NULL.
 */
xmlNs * FASTCALL xmlSearchNs(xmlDoc * doc, xmlNode * P_Node, const xmlChar * nameSpace) 
{
	xmlNs * cur = 0;
	const xmlNode * orig = P_Node;
	if(P_Node && P_Node->type != XML_NAMESPACE_DECL) {
		if(nameSpace && sstreq(nameSpace, "xml")) {
			if(!doc && P_Node->type == XML_ELEMENT_NODE) {
				// 
				// The XML-1.0 namespace is normally held on the root
				// element. In this case exceptionally create it on the node element.
				// 
				cur = static_cast<xmlNs *>(SAlloc::M(sizeof(xmlNs)));
				if(!cur) {
					xmlTreeErrMemory("searching namespace");
				}
				else {
					memzero(cur, sizeof(xmlNs));
					cur->type = XML_LOCAL_NAMESPACE;
					cur->href = sstrdup(XML_XML_NAMESPACE);
					cur->prefix = sstrdup((const xmlChar *)"xml");
					cur->next = P_Node->nsDef;
					P_Node->nsDef = cur;
				}
			}
			else {
				SETIFZ(doc, P_Node->doc);
				// 
				// Return the XML namespace declaration held by the doc.
				// 
				cur = doc ? (doc->oldNs ? doc->oldNs : xmlTreeEnsureXMLDecl(doc)) : 0;
			}
			return cur;
		}
		else {
			for(; P_Node; P_Node = P_Node->P_ParentNode) {
				if(P_Node->type == XML_ELEMENT_NODE) {
					for(cur = P_Node->nsDef; cur; cur = cur->next) {
						if(cur->href && sstreq(cur->prefix, nameSpace))
							return cur;
					}
					if(orig != P_Node) {
						cur = P_Node->ns;
						if(cur && cur->href && sstreq(cur->prefix, nameSpace))
							return cur;
					}
				}
				else if(oneof3(P_Node->type, XML_ENTITY_REF_NODE, XML_ENTITY_NODE, XML_ENTITY_DECL))
					return 0;
			}
		}
	}
	return 0;
}
/**
 * xmlNsInScope:
 * @doc:  the document
 * @node:  the current node
 * @ancestor:  the ancestor carrying the namespace
 * @prefix:  the namespace prefix
 *
 * Verify that the given namespace held on @ancestor is still in scope
 * on node.
 *
 * Returns 1 if true, 0 if false and -1 in case of error.
 */
static int xmlNsInScope(xmlDoc * doc ATTRIBUTE_UNUSED, xmlNode * P_Node, xmlNode * ancestor, const xmlChar * prefix)
{
	while(P_Node && P_Node != ancestor) {
		if(oneof3(P_Node->type, XML_ENTITY_REF_NODE, XML_ENTITY_NODE, XML_ENTITY_DECL))
			return -1;
		else {
			if(P_Node->type == XML_ELEMENT_NODE) {
				for(xmlNs * tst = P_Node->nsDef; tst; tst = tst->next)
					if(sstreq(tst->prefix, prefix))
						return 0;
			}
			P_Node = P_Node->P_ParentNode;
		}
	}
	return (P_Node != ancestor) ? -1 : 1;
}
/**
 * xmlSearchNsByHref:
 * @doc:  the document
 * @node:  the current node
 * @href:  the namespace value
 *
 * Search a Ns aliasing a given URI. Recurse on the parents until it finds
 * the defined namespace or return NULL otherwise.
 * Returns the namespace pointer or NULL.
 */
xmlNs * xmlSearchNsByHref(xmlDoc * doc, xmlNode * P_Node, const xmlChar * href)
{
	xmlNs * cur;
	xmlNode * orig = P_Node;
	int is_attr;
	if(!P_Node || (P_Node->type == XML_NAMESPACE_DECL) || !href)
		return 0;
	if(sstreq(href, XML_XML_NAMESPACE)) {
		/*
		 * Only the document can hold the XML spec namespace.
		 */
		if(!doc && (P_Node->type == XML_ELEMENT_NODE)) {
			/*
			 * The XML-1.0 namespace is normally held on the root
			 * element. In this case exceptionally create it on the
			 * node element.
			 */
			cur = static_cast<xmlNs *>(SAlloc::M(sizeof(xmlNs)));
			if(!cur) {
				xmlTreeErrMemory("searching namespace");
				return 0;
			}
			memzero(cur, sizeof(xmlNs));
			cur->type = XML_LOCAL_NAMESPACE;
			cur->href = sstrdup(XML_XML_NAMESPACE);
			cur->prefix = sstrdup((const xmlChar *)"xml");
			cur->next = P_Node->nsDef;
			P_Node->nsDef = cur;
			return (cur);
		}
		if(!doc) {
			doc = P_Node->doc;
			if(!doc)
				return 0;
		}
		// Return the XML namespace declaration held by the doc.
		return doc->oldNs ? doc->oldNs : xmlTreeEnsureXMLDecl(doc);
	}
	is_attr = (P_Node->type == XML_ATTRIBUTE_NODE);
	while(P_Node) {
		if(oneof3(P_Node->type, XML_ENTITY_REF_NODE, XML_ENTITY_NODE, XML_ENTITY_DECL))
			return 0;
		if(P_Node->type == XML_ELEMENT_NODE) {
			cur = P_Node->nsDef;
			while(cur) {
				if(cur->href && href && sstreq(cur->href, href)) {
					if(((!is_attr) || cur->prefix) && (xmlNsInScope(doc, orig, P_Node, cur->prefix) == 1))
						return (cur);
				}
				cur = cur->next;
			}
			if(orig != P_Node) {
				cur = P_Node->ns;
				if(cur) {
					if(cur->href && href && (sstreq(cur->href, href))) {
						if(((!is_attr) || cur->prefix) && (xmlNsInScope(doc, orig, P_Node, cur->prefix) == 1))
							return (cur);
					}
				}
			}
		}
		P_Node = P_Node->P_ParentNode;
	}
	return 0;
}

/**
 * xmlNewReconciliedNs:
 * @doc:  the document
 * @tree:  a node expected to hold the new namespace
 * @ns:  the original namespace
 *
 * This function tries to locate a namespace definition in a tree
 * ancestors, or create a new namespace definition node similar to
 * @ns trying to reuse the same prefix. However if the given prefix is
 * null (default namespace) or reused within the subtree defined by
 * @tree or on one of its ancestors then a new prefix is generated.
 * Returns the (new) namespace definition or NULL in case of error
 */
static xmlNs * xmlNewReconciliedNs(xmlDoc * doc, xmlNode * tree, xmlNs * ns) 
{
	xmlNs * def;
	xmlChar prefix[50];
	int counter = 1;
	if((tree == NULL) || (tree->type != XML_ELEMENT_NODE)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNewReconciliedNs : tree == NULL\n");
#endif
		return 0;
	}
	if((ns == NULL) || (ns->type != XML_NAMESPACE_DECL)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlNewReconciliedNs : ns == NULL\n");
#endif
		return 0;
	}
	/*
	 * Search an existing namespace definition inherited.
	 */
	def = xmlSearchNsByHref(doc, tree, ns->href);
	if(def)
		return (def);
	/*
	 * Find a close prefix which is not already in use.
	 * Let's strip namespace prefixes longer than 20 chars !
	 */
	if(ns->prefix == NULL)
		snprintf((char *)prefix, sizeof(prefix), "default");
	else
		snprintf((char *)prefix, sizeof(prefix), "%.20s", (char *)ns->prefix);
	def = xmlSearchNs(doc, tree, prefix);
	while(def) {
		if(counter > 1000) 
			return 0;
		if(ns->prefix == NULL)
			snprintf((char *)prefix, sizeof(prefix), "default%d", counter++);
		else
			snprintf((char *)prefix, sizeof(prefix), "%.20s%d", (char *)ns->prefix, counter++);
		def = xmlSearchNs(doc, tree, prefix);
	}
	/*
	 * OK, now we are ready to create a new one.
	 */
	def = xmlNewNs(tree, ns->href, prefix);
	return def;
}

#ifdef LIBXML_TREE_ENABLED
/**
 * xmlReconciliateNs:
 * @doc:  the document
 * @tree:  a node defining the subtree to reconciliate
 *
 * This function checks that all the namespaces declared within the given
 * tree are properly declared. This is needed for example after Copy or Cut
 * and then paste operations. The subtree may still hold pointers to
 * namespace declarations outside the subtree or invalid/masked. As much
 * as possible the function try to reuse the existing namespaces found in
 * the new environment. If not possible the new namespaces are redeclared
 * on @tree at the top of the given subtree.
 * Returns the number of namespace declarations created or -1 in case of error.
 */
int xmlReconciliateNs(xmlDoc * doc, xmlNode * tree) 
{
	xmlNs ** oldNs = NULL;
	xmlNs ** newNs = NULL;
	int sizeCache = 0;
	int nbCache = 0;
	xmlNs * n;
	xmlNode * P_Node = tree;
	xmlAttr * attr;
	int ret = 0, i;
	if(!P_Node || (P_Node->type != XML_ELEMENT_NODE)) 
		return -1;
	if(!doc || (doc->type != XML_DOCUMENT_NODE)) 
		return -1;
	if(P_Node->doc != doc) 
		return -1;
	while(P_Node) {
		/*
		 * Reconciliate the node namespace
		 */
		if(P_Node->ns) {
			/*
			 * initialize the cache if needed
			 */
			if(sizeCache == 0) {
				sizeCache = 10;
				oldNs = (xmlNs **)SAlloc::M(sizeCache * sizeof(xmlNs *));
				if(oldNs == NULL) {
					xmlTreeErrMemory("fixing namespaces");
					return -1;
				}
				newNs = (xmlNs **)SAlloc::M(sizeCache * sizeof(xmlNs *));
				if(newNs == NULL) {
					xmlTreeErrMemory("fixing namespaces");
					SAlloc::F(oldNs);
					return -1;
				}
			}
			for(i = 0; i < nbCache; i++) {
				if(oldNs[i] == P_Node->ns) {
					P_Node->ns = newNs[i];
					break;
				}
			}
			if(i == nbCache) {
				/*
				 * OK we need to recreate a new namespace definition
				 */
				n = xmlNewReconciliedNs(doc, tree, P_Node->ns);
				if(n) { /* :-( what if else ??? */
					/*
					 * check if we need to grow the cache buffers.
					 */
					if(sizeCache <= nbCache) {
						sizeCache *= 2;
						oldNs = (xmlNs **)SAlloc::R(oldNs, sizeCache * sizeof(xmlNs *));
						if(oldNs == NULL) {
							xmlTreeErrMemory("fixing namespaces");
							SAlloc::F(newNs);
							return -1;
						}
						newNs = (xmlNs **)SAlloc::R(newNs, sizeCache * sizeof(xmlNs *));
						if(newNs == NULL) {
							xmlTreeErrMemory("fixing namespaces");
							SAlloc::F(oldNs);
							return -1;
						}
					}
					newNs[nbCache] = n;
					oldNs[nbCache++] = P_Node->ns;
					P_Node->ns = n;
				}
			}
		}
		/*
		 * now check for namespace hold by attributes on the node.
		 */
		if(P_Node->type == XML_ELEMENT_NODE) {
			attr = P_Node->properties;
			while(attr) {
				if(attr->ns) {
					/*
					 * initialize the cache if needed
					 */
					if(sizeCache == 0) {
						sizeCache = 10;
						oldNs = (xmlNs **)SAlloc::M(sizeCache * sizeof(xmlNs *));
						if(oldNs == NULL) {
							xmlTreeErrMemory("fixing namespaces");
							return -1;
						}
						newNs = (xmlNs **)SAlloc::M(sizeCache * sizeof(xmlNs *));
						if(newNs == NULL) {
							xmlTreeErrMemory("fixing namespaces");
							SAlloc::F(oldNs);
							return -1;
						}
					}
					for(i = 0; i < nbCache; i++) {
						if(oldNs[i] == attr->ns) {
							attr->ns = newNs[i];
							break;
						}
					}
					if(i == nbCache) {
						/*
						 * OK we need to recreate a new namespace definition
						 */
						n = xmlNewReconciliedNs(doc, tree, attr->ns);
						if(n) { /* :-( what if else ??? */
							/*
							 * check if we need to grow the cache buffers.
							 */
							if(sizeCache <= nbCache) {
								sizeCache *= 2;
								oldNs = (xmlNs **)SAlloc::R(oldNs, sizeCache * sizeof(xmlNs *));
								if(oldNs == NULL) {
									xmlTreeErrMemory("fixing namespaces");
									SAlloc::F(newNs);
									return -1;
								}
								newNs = (xmlNs **)SAlloc::R(newNs, sizeCache * sizeof(xmlNs *));
								if(newNs == NULL) {
									xmlTreeErrMemory("fixing namespaces");
									SAlloc::F(oldNs);
									return -1;
								}
							}
							newNs[nbCache] = n;
							oldNs[nbCache++] = attr->ns;
							attr->ns = n;
						}
					}
				}
				attr = attr->next;
			}
		}
		/*
		 * Browse the full subtree, deep first
		 */
		if(P_Node->children && (P_Node->type != XML_ENTITY_REF_NODE)) {
			P_Node = P_Node->children; // deep first 
		}
		else if((P_Node != tree) && P_Node->next) {
			P_Node = P_Node->next; // then siblings 
		}
		else if(P_Node != tree) {
			/* go up to parents->next if needed */
			while(P_Node != tree) {
				if(P_Node->P_ParentNode)
					P_Node = P_Node->P_ParentNode;
				if((P_Node != tree) && P_Node->next) {
					P_Node = P_Node->next;
					break;
				}
				if(!P_Node->P_ParentNode) {
					P_Node = NULL;
					break;
				}
			}
			/* exit condition */
			if(P_Node == tree)
				P_Node = NULL;
		}
		else
			break;
	}
	SAlloc::F(oldNs);
	SAlloc::F(newNs);
	return ret;
}

#endif /* LIBXML_TREE_ENABLED */

static xmlAttr * FASTCALL xmlGetPropNodeInternal(const xmlNode * P_Node, const xmlChar * name, const xmlChar * nsName, int useDTD)
{
	xmlAttr * prop;
	if(!P_Node || (P_Node->type != XML_ELEMENT_NODE) || !name)
		return 0;
	if(P_Node->properties) {
		prop = P_Node->properties;
		if(nsName == NULL) {
			/*
			 * We want the attr to be in no namespace.
			 */
			do {
				if((prop->ns == NULL) && sstreq(prop->name, name)) {
					return (prop);
				}
				prop = prop->next;
			} while(prop);
		}
		else {
			/*
			 * We want the attr to be in the specified namespace.
			 */
			do {
				if(prop->ns && sstreq(prop->name, name) && ((prop->ns->href == nsName) || sstreq(prop->ns->href, nsName))) {
					return (prop);
				}
				prop = prop->next;
			} while(prop);
		}
	}
#ifdef LIBXML_TREE_ENABLED
	if(!useDTD)
		return 0;
	/*
	 * Check if there is a default/fixed attribute declaration in
	 * the internal or external subset.
	 */
	if(P_Node->doc && P_Node->doc->intSubset) {
		xmlDoc * doc = P_Node->doc;
		xmlAttribute * attrDecl = NULL;
		xmlChar * elemQName, * tmpstr = NULL;
		/*
		 * We need the QName of the element for the DTD-lookup.
		 */
		if(P_Node->ns && P_Node->ns->prefix) {
			tmpstr = sstrdup(P_Node->ns->prefix);
			tmpstr = xmlStrcat(tmpstr, reinterpret_cast<const xmlChar *>(":"));
			tmpstr = xmlStrcat(tmpstr, P_Node->name);
			if(tmpstr == NULL)
				return 0;
			elemQName = tmpstr;
		}
		else
			elemQName = (xmlChar *)P_Node->name;
		if(nsName == NULL) {
			/*
			 * The common and nice case: Attr in no namespace.
			 */
			attrDecl = xmlGetDtdQAttrDesc(doc->intSubset, elemQName, name, 0);
			if(!attrDecl && doc->extSubset) {
				attrDecl = xmlGetDtdQAttrDesc(doc->extSubset, elemQName, name, 0);
			}
		}
		else {
			/*
			 * The ugly case: Search using the prefixes of in-scope
			 * ns-decls corresponding to @nsName.
			 */
			xmlNs ** nsList = xmlGetNsList(P_Node->doc, P_Node);
			if(nsList == NULL) {
				SAlloc::F(tmpstr);
				return 0;
			}
			else {
				for(xmlNs ** pp_cur = nsList; *pp_cur; pp_cur++) {
					if(sstreq((*pp_cur)->href, nsName)) {
						attrDecl = xmlGetDtdQAttrDesc(doc->intSubset, elemQName, name, (*pp_cur)->prefix);
						if(attrDecl)
							break;
						if(doc->extSubset) {
							attrDecl = xmlGetDtdQAttrDesc(doc->extSubset, elemQName, name, (*pp_cur)->prefix);
							if(attrDecl)
								break;
						}
					}
				}
				SAlloc::F(nsList);
			}
		}
		SAlloc::F(tmpstr);
		/*
		 * Only default/fixed attrs are relevant.
		 */
		if(attrDecl && attrDecl->defaultValue)
			return (xmlAttr *)attrDecl;
	}
#endif /* LIBXML_TREE_ENABLED */
	return 0;
}

static xmlChar * FASTCALL xmlGetPropNodeValueInternal(const xmlAttr * prop)
{
	if(prop) {
		if(prop->type == XML_ATTRIBUTE_NODE) {
			/*
			 * Note that we return at least the empty string.
			 * @todo Do we really always want that?
			 */
			if(prop->children) {
				if(!prop->children->next && oneof2(prop->children->type, XML_TEXT_NODE, XML_CDATA_SECTION_NODE)) {
					/*
					 * Optimization for the common case: only 1 text node.
					 */
					return sstrdup(prop->children->content);
				}
				else {
					xmlChar * ret = xmlNodeListGetString(prop->doc, prop->children, 1);
					if(ret)
						return ret;
				}
			}
			return sstrdup((xmlChar *)"");
		}
		else if(prop->type == XML_ATTRIBUTE_DECL) {
			return sstrdup(((xmlAttribute *)prop)->defaultValue);
		}
	}
	return 0;
}
/**
 * xmlHasProp:
 * @node:  the node
 * @name:  the attribute name
 *
 * Search an attribute associated to a node
 * This function also looks in DTD attribute declaration for #FIXED or
 * default declaration values unless DTD use has been turned off.
 *
 * Returns the attribute or the attribute declaration or NULL if
 *    neither was found.
 */
xmlAttr * FASTCALL xmlHasProp(const xmlNode * pNode, const xmlChar * name) 
{
	xmlAttr * prop;
	xmlDoc * doc;
	if(!pNode || pNode->type != XML_ELEMENT_NODE || !name)
		return 0;
	/*
	 * Check on the properties attached to the node
	 */
	prop = pNode->properties;
	while(prop) {
		if(sstreq(prop->name, name)) {
			return prop;
		}
		prop = prop->next;
	}
	if(!xmlCheckDTD) 
		return 0;
	/*
	 * Check if there is a default declaration in the internal
	 * or external subsets
	 */
	doc = pNode->doc;
	if(doc) {
		xmlAttribute * attrDecl;
		if(doc->intSubset) {
			attrDecl = xmlGetDtdAttrDesc(doc->intSubset, pNode->name, name);
			if(!attrDecl && doc->extSubset)
				attrDecl = xmlGetDtdAttrDesc(doc->extSubset, pNode->name, name);
			if(attrDecl && attrDecl->defaultValue)
				// return attribute declaration only if a default value is given (that includes #FIXED declarations) 
				return (xmlAttr *)attrDecl;
		}
	}
	return 0;
}

/**
 * xmlHasNsProp:
 * @node:  the node
 * @name:  the attribute name
 * @nameSpace:  the URI of the namespace
 *
 * Search for an attribute associated to a node
 * This attribute has to be anchored in the namespace specified.
 * This does the entity substitution.
 * This function looks in DTD attribute declaration for #FIXED or
 * default declaration values unless DTD use has been turned off.
 * Note that a namespace of NULL indicates to use the default namespace.
 *
 * Returns the attribute or the attribute declaration or NULL
 *   if neither was found.
 */
xmlAttr * xmlHasNsProp(const xmlNode * P_Node, const xmlChar * name, const xmlChar * nameSpace) 
{
	return xmlGetPropNodeInternal(P_Node, name, nameSpace, xmlCheckDTD);
}
/**
 * xmlGetProp:
 * @node:  the node
 * @name:  the attribute name
 *
 * Search and get the value of an attribute associated to a node
 * This does the entity substitution.
 * This function looks in DTD attribute declaration for #FIXED or
 * default declaration values unless DTD use has been turned off.
 * NOTE: this function acts independently of namespaces associated
 *  to the attribute. Use xmlGetNsProp() or xmlGetNoNsProp()
 *  for namespace aware processing.
 *
 * Returns the attribute value or NULL if not found.
 *   It's up to the caller to free the memory with SAlloc::F().
 */
xmlChar * FASTCALL xmlGetProp(const xmlNode * pNode, const xmlChar * name) 
{
	xmlAttr * prop = xmlHasProp(pNode, name);
	return prop ? xmlGetPropNodeValueInternal(prop) : 0;
}
/**
 * xmlGetNoNsProp:
 * @node:  the node
 * @name:  the attribute name
 *
 * Search and get the value of an attribute associated to a node
 * This does the entity substitution.
 * This function looks in DTD attribute declaration for #FIXED or
 * default declaration values unless DTD use has been turned off.
 * This function is similar to xmlGetProp except it will accept only
 * an attribute in no namespace.
 *
 * Returns the attribute value or NULL if not found.
 *   It's up to the caller to free the memory with SAlloc::F().
 */
xmlChar * FASTCALL xmlGetNoNsProp(const xmlNode * P_Node, const xmlChar * name) 
{
	const xmlAttr * prop = xmlGetPropNodeInternal(P_Node, name, NULL, xmlCheckDTD);
	return prop ? xmlGetPropNodeValueInternal(prop) : 0;
}
/**
 * xmlGetNsProp:
 * @node:  the node
 * @name:  the attribute name
 * @nameSpace:  the URI of the namespace
 *
 * Search and get the value of an attribute associated to a node
 * This attribute has to be anchored in the namespace specified.
 * This does the entity substitution.
 * This function looks in DTD attribute declaration for #FIXED or
 * default declaration values unless DTD use has been turned off.
 *
 * Returns the attribute value or NULL if not found.
 *   It's up to the caller to free the memory with SAlloc::F().
 */
xmlChar * FASTCALL xmlGetNsProp(const xmlNode * P_Node, const xmlChar * name, const xmlChar * nameSpace) 
{
	const xmlAttr * prop = xmlGetPropNodeInternal(P_Node, name, nameSpace, xmlCheckDTD);
	return prop ? xmlGetPropNodeValueInternal(prop) : 0;
}
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
/**
 * xmlUnsetProp:
 * @node:  the node
 * @name:  the attribute name
 *
 * Remove an attribute carried by a node.
 * This handles only attributes in no namespace.
 * Returns 0 if successful, -1 if not found
 */
int xmlUnsetProp(xmlNode * pNode, const xmlChar * name) 
{
	xmlAttr * prop = xmlGetPropNodeInternal(pNode, name, NULL, 0);
	if(prop == NULL)
		return -1;
	else {
		xmlUnlinkNode((xmlNode *)prop);
		xmlFreeProp(prop);
		return 0;
	}
}
/**
 * xmlUnsetNsProp:
 * @node:  the node
 * @ns:  the namespace definition
 * @name:  the attribute name
 *
 * Remove an attribute carried by a node.
 * Returns 0 if successful, -1 if not found
 */
int xmlUnsetNsProp(xmlNode * pNode, xmlNs * ns, const xmlChar * name) 
{
	xmlAttr * prop = xmlGetPropNodeInternal(pNode, name, ns ? ns->href : NULL, 0);
	if(prop == NULL)
		return -1;
	else {
		xmlUnlinkNode((xmlNode *)prop);
		xmlFreeProp(prop);
		return 0;
	}
}
#endif

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_HTML_ENABLED)
/**
 * xmlSetProp:
 * @node:  the node
 * @name:  the attribute name (a QName)
 * @value:  the attribute value
 *
 * Set (or reset) an attribute carried by a node.
 * If @name has a prefix, then the corresponding
 * namespace-binding will be used, if in scope; it is an
 * error it there's no such ns-binding for the prefix in
 * scope.
 * Returns the attribute pointer.
 *
 */
xmlAttr * FASTCALL xmlSetProp(xmlNode * pNode, const xmlChar * name, const xmlChar * value) 
{
	if(!pNode || !name || (pNode->type != XML_ELEMENT_NODE))
		return 0;
	/*
	 * handle QNames
	 */
	int len;
	const xmlChar * nqname = xmlSplitQName3(name, &len);
	if(nqname) {
		xmlChar * prefix = xmlStrndup(name, len);
		xmlNs * ns = xmlSearchNs(pNode->doc, pNode, prefix);
		SAlloc::F(prefix);
		if(ns)
			return xmlSetNsProp(pNode, ns, nqname, value);
	}
	return xmlSetNsProp(pNode, NULL, name, value);
}
/**
 * xmlSetNsProp:
 * @node:  the node
 * @ns:  the namespace definition
 * @name:  the attribute name
 * @value:  the attribute value
 *
 * Set (or reset) an attribute carried by a node.
 * The ns structure must be in scope, this is not checked
 *
 * Returns the attribute pointer.
 */
xmlAttr * FASTCALL xmlSetNsProp(xmlNode * P_Node, xmlNs * ns, const xmlChar * name, const xmlChar * value)
{
	if(ns && (ns->href == NULL))
		return 0;
	xmlAttr * prop = xmlGetPropNodeInternal(P_Node, name, ns ? ns->href : NULL, 0);
	if(prop) {
		/*
		 * Modify the attribute's value.
		 */
		if(prop->atype == XML_ATTRIBUTE_ID) {
			xmlRemoveID(P_Node->doc, prop);
			prop->atype = XML_ATTRIBUTE_ID;
		}
		xmlFreeNodeList(prop->children);
		prop->children = NULL;
		prop->last = NULL;
		prop->ns = ns;
		if(value) {
			if(!xmlCheckUTF8(value)) {
				xmlTreeErr(XML_TREE_NOT_UTF8, (xmlNode *)P_Node->doc, 0);
				if(P_Node->doc)
					P_Node->doc->encoding = sstrdup(reinterpret_cast<const xmlChar *>("ISO-8859-1"));
			}
			prop->children = xmlNewDocText(P_Node->doc, value);
			prop->last = NULL;
			{
				for(xmlNode * tmp = prop->children; tmp; tmp = tmp->next) {
					tmp->P_ParentNode = (xmlNode *)prop;
					if(tmp->next == NULL)
						prop->last = tmp;
				}
			}
		}
		if(prop->atype == XML_ATTRIBUTE_ID)
			xmlAddID(NULL, P_Node->doc, value, prop);
		return prop;
	}
	/*
	 * No equal attr found; create a new one.
	 */
	return xmlNewPropInternal(P_Node, ns, name, value, 0);
}

#endif /* LIBXML_TREE_ENABLED */
/**
 * xmlNodeIsText:
 * @node:  the node
 *
 * Is this node a Text node ?
 * Returns 1 yes, 0 no
 */
int FASTCALL xmlNodeIsText(const xmlNode * pNode) 
{
	return pNode ? BIN(pNode->type == XML_TEXT_NODE) : 0;
}
/**
 * xmlIsBlankNode:
 * @node:  the node
 *
 * Checks whether this node is an empty or whitespace only (and possibly ignorable) text-node.
 *
 * Returns 1 yes, 0 no
 */
int FASTCALL xmlIsBlankNode(const xmlNode * pNode) 
{
	if(!pNode || !oneof2(pNode->type, XML_TEXT_NODE, XML_CDATA_SECTION_NODE))
		return 0;
	else {
		if(pNode->content) {
			for(const xmlChar * cur = pNode->content; *cur != 0; cur++) {
				if(!IS_BLANK_CH(*cur)) 
					return 0;
			}
		}
		return 1;
	}
}
/**
 * xmlTextConcat:
 * @node:  the node
 * @content:  the content
 * @len:  @content length
 *
 * Concat the given string at the end of the existing node content
 *
 * Returns -1 in case of error, 0 otherwise
 */
int xmlTextConcat(xmlNode * P_Node, const xmlChar * content, int len) 
{
	if(!P_Node) 
		return -1;
	if(!oneof4(P_Node->type, XML_TEXT_NODE, XML_CDATA_SECTION_NODE, XML_COMMENT_NODE, XML_PI_NODE)) {
#ifdef DEBUG_TREE
		xmlGenericError(0, "xmlTextConcat: node is not text nor CDATA\n");
#endif
		return -1;
	}
	/* need to check if content is currently in the dictionary */
	if((P_Node->content == (xmlChar *)&(P_Node->properties)) || (P_Node->doc && P_Node->doc->dict && xmlDictOwns(P_Node->doc->dict, P_Node->content))) {
		P_Node->content = xmlStrncatNew(P_Node->content, content, len);
	}
	else {
		P_Node->content = xmlStrncat(P_Node->content, content, len);
	}
	P_Node->properties = NULL;
	return (P_Node->content == NULL) ?  -1 : 0;
}
//
// Output : to a FILE or in memory
//
/**
 * xmlBufferCreate:
 *
 * routine to create an XML buffer.
 * returns the new structure.
 */
xmlBuffer * xmlBufferCreate()
{
	xmlBuffer * ret = static_cast<xmlBuffer *>(SAlloc::M(sizeof(xmlBuffer)));
	if(!ret) {
		xmlTreeErrMemory("creating buffer");
	}
	else {
		ret->use = 0;
		ret->size = xmlDefaultBufferSize;
		ret->alloc = xmlBufferAllocScheme;
		ret->content = static_cast<xmlChar *>(SAlloc::M(ret->size * sizeof(xmlChar)));
		if(ret->content == NULL) {
			xmlTreeErrMemory("creating buffer");
			ZFREE(ret);
		}
		else {
			ret->content[0] = 0;
			ret->contentIO = NULL;
		}
	}
	return ret;
}

/**
 * xmlBufferCreateSize:
 * @size: initial size of buffer
 *
 * routine to create an XML buffer.
 * returns the new structure.
 */
xmlBuffer * xmlBufferCreateSize(size_t size)
{
	xmlBuffer * ret = static_cast<xmlBuffer *>(SAlloc::M(sizeof(xmlBuffer)));
	if(!ret) {
		xmlTreeErrMemory("creating buffer");
	}
	else {
		ret->use = 0;
		ret->alloc = xmlBufferAllocScheme;
		ret->size = (size ? size+2 : 0);     /* +1 for ending null */
		if(ret->size) {
			ret->content = static_cast<xmlChar *>(SAlloc::M(ret->size * sizeof(xmlChar)));
			if(ret->content == NULL) {
				xmlTreeErrMemory("creating buffer");
				SAlloc::F(ret);
				return 0;
			}
			ret->content[0] = 0;
		}
		else
			ret->content = NULL;
		ret->contentIO = NULL;
	}
	return ret;
}

/**
 * xmlBufferDetach:
 * @buf:  the buffer
 *
 * Remove the string contained in a buffer and gie it back to the
 * caller. The buffer is reset to an empty content.
 * This doesn't work with immutable buffers as they can't be reset.
 *
 * Returns the previous string contained by the buffer.
 */
xmlChar * xmlBufferDetach(xmlBuffer * buf)
{
	xmlChar * ret = 0;
	if(buf && buf->alloc != XML_BUFFER_ALLOC_IMMUTABLE) {
		ret = buf->content;
		buf->content = NULL;
		buf->size = 0;
		buf->use = 0;
	}
	return ret;
}
/**
 * xmlBufferCreateStatic:
 * @mem: the memory area
 * @size:  the size in byte
 *
 * routine to create an XML buffer from an immutable memory area.
 * The area won't be modified nor copied, and is expected to be
 * present until the end of the buffer lifetime.
 *
 * returns the new structure.
 */
xmlBuffer * xmlBufferCreateStatic(void * mem, size_t size)
{
	xmlBuffer * ret;
	if((mem == NULL) || (size == 0))
		return 0;
	ret = static_cast<xmlBuffer *>(SAlloc::M(sizeof(xmlBuffer)));
	if(!ret) {
		xmlTreeErrMemory("creating buffer");
		return 0;
	}
	ret->use = size;
	ret->size = size;
	ret->alloc = XML_BUFFER_ALLOC_IMMUTABLE;
	ret->content = (xmlChar *)mem;
	return ret;
}

/**
 * xmlBufferSetAllocationScheme:
 * @buf:  the buffer to tune
 * @scheme:  allocation scheme to use
 *
 * Sets the allocation scheme for this buffer
 */
void xmlBufferSetAllocationScheme(xmlBuffer * buf, xmlBufferAllocationScheme scheme)
{
	if(!buf) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufferSetAllocationScheme: buf == NULL\n");
#endif
	}
	else if(!oneof2(buf->alloc, XML_BUFFER_ALLOC_IMMUTABLE, XML_BUFFER_ALLOC_IO)) {
		if(oneof4(scheme, XML_BUFFER_ALLOC_DOUBLEIT, XML_BUFFER_ALLOC_EXACT, XML_BUFFER_ALLOC_HYBRID, XML_BUFFER_ALLOC_IMMUTABLE))
			buf->alloc = scheme;
	}
}
/**
 * @buf:  the buffer to free
 *
 * Frees an XML buffer. It frees both the content and the structure which
 * encapsulate it.
 */
void FASTCALL xmlBufferFree(xmlBuffer * pBuf)
{
	if(pBuf) {
		if((pBuf->alloc == XML_BUFFER_ALLOC_IO) && pBuf->contentIO)
			SAlloc::F(pBuf->contentIO);
		else if(pBuf->content && (pBuf->alloc != XML_BUFFER_ALLOC_IMMUTABLE))
			SAlloc::F(pBuf->content);
		SAlloc::F(pBuf);
	}
}
/**
 * @buf:  the buffer
 *
 * empty a buffer.
 */
void xmlBufferEmpty(xmlBuffer * buf)
{
	if(buf && buf->content) {
		buf->use = 0;
		if(buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) {
			buf->content = const_cast<xmlChar *>(reinterpret_cast<const xmlChar *>("")); // @badcast
		}
		else if((buf->alloc == XML_BUFFER_ALLOC_IO) && buf->contentIO) {
			size_t start_buf = buf->content - buf->contentIO;
			buf->size += start_buf;
			buf->content = buf->contentIO;
			buf->content[0] = 0;
		}
		else {
			buf->content[0] = 0;
		}
	}
}
/**
 * xmlBufferShrink:
 * @buf:  the buffer to dump
 * @len:  the number of xmlChar to remove
 *
 * Remove the beginning of an XML buffer.
 *
 * Returns the number of #xmlChar removed, or -1 in case of failure.
 */
int xmlBufferShrink(xmlBuffer * buf, uint len)
{
	if(!buf)
		return -1;
	if(len == 0)
		return 0;
	if(len > buf->use)
		return -1;
	buf->use -= len;
	if((buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) || ((buf->alloc == XML_BUFFER_ALLOC_IO) && buf->contentIO)) {
		/*
		 * we just move the content pointer, but also make sure
		 * the perceived buffer size has shrinked accordingly
		 */
		buf->content += len;
		buf->size -= len;

		/*
		 * sometimes though it maybe be better to really shrink
		 * on IO buffers
		 */
		if((buf->alloc == XML_BUFFER_ALLOC_IO) && buf->contentIO) {
			size_t start_buf = buf->content - buf->contentIO;
			if(start_buf >= buf->size) {
				memmove(buf->contentIO, &buf->content[0], buf->use);
				buf->content = buf->contentIO;
				buf->content[buf->use] = 0;
				buf->size += start_buf;
			}
		}
	}
	else {
		memmove(buf->content, &buf->content[len], buf->use);
		buf->content[buf->use] = 0;
	}
	return (len);
}

/**
 * xmlBufferGrow:
 * @buf:  the buffer
 * @len:  the minimum free size to allocate
 *
 * Grow the available space of an XML buffer.
 *
 * Returns the new available space or -1 in case of error
 */
int xmlBufferGrow(xmlBuffer * buf, uint len)
{
	int size;
	xmlChar * newbuf;
	if(!buf)
        return -1;
	if(buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE)
        return 0;
	if(len + buf->use < buf->size)
        return 0;
	/*
	 * Windows has a BIG problem on realloc timing, so we try to double
	 * the buffer size (if that's enough) (bug 146697)
	 * Apparently BSD too, and it's probably best for linux too
	 * On an embedded system this may be something to change
	 */
#if 1
	size = (buf->size > len) ? (buf->size * 2) : (buf->use + len + 100);
#else
	size = buf->use + len + 100;
#endif

	if((buf->alloc == XML_BUFFER_ALLOC_IO) && buf->contentIO) {
		size_t start_buf = buf->content - buf->contentIO;
		newbuf = static_cast<xmlChar *>(SAlloc::R(buf->contentIO, start_buf + size));
		if(newbuf == NULL) {
			xmlTreeErrMemory("growing buffer");
			return -1;
		}
		buf->contentIO = newbuf;
		buf->content = newbuf + start_buf;
	}
	else {
		newbuf = static_cast<xmlChar *>(SAlloc::R(buf->content, size));
		if(newbuf == NULL) {
			xmlTreeErrMemory("growing buffer");
			return -1;
		}
		buf->content = newbuf;
	}
	buf->size = size;
	return (buf->size - buf->use);
}
/**
 * xmlBufferDump:
 * @file:  the file output
 * @buf:  the buffer to dump
 *
 * Dumps an XML buffer to  a FILE *.
 * Returns the number of #xmlChar written
 */
int xmlBufferDump(FILE * file, xmlBuffer * buf) 
{
	int ret;
	if(!buf) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufferDump: buf == NULL\n");
#endif
		return 0;
	}
	if(buf->content == NULL) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufferDump: buf->content == NULL\n");
#endif
		return 0;
	}
	SETIFZ(file, stdout);
	ret = fwrite(buf->content, sizeof(xmlChar), buf->use, file);
	return ret;
}

/**
 * xmlBufferContent:
 * @buf:  the buffer
 *
 * Function to extract the content of a buffer
 *
 * Returns the internal content
 */
const xmlChar * xmlBufferContent(const xmlBuffer * buf)
{
	return buf ? buf->content : 0;
}
/**
 * xmlBufferLength:
 * @buf:  the buffer
 *
 * Function to get the length of a buffer
 *
 * Returns the length of data in the internal content
 */
int xmlBufferLength(const xmlBuffer * buf)
{
	return buf ? buf->use : 0;
}
/**
 * xmlBufferResize:
 * @buf:  the buffer to resize
 * @size:  the desired size
 *
 * Resize a buffer to accommodate minimum size of @size.
 *
 * Returns  0 in case of problems, 1 otherwise
 */
int xmlBufferResize(xmlBuffer * buf, uint size)
{
	uint newSize;
	xmlChar* rebuf = NULL;
	size_t start_buf;
	if(!buf)
		return 0;
	if(buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE)
        return 0;
	/* Don't resize if we don't have to */
	if(size < buf->size)
		return 1;
	/* figure out new size */
	switch(buf->alloc) {
		case XML_BUFFER_ALLOC_IO:
		case XML_BUFFER_ALLOC_DOUBLEIT:
		    /*take care of empty case*/
		    newSize = (buf->size ? buf->size*2 : size + 10);
		    while(size > newSize) {
			    if(newSize > UINT_MAX / 2) {
				    xmlTreeErrMemory("growing buffer");
				    return 0;
			    }
			    newSize *= 2;
		    }
		    break;
		case XML_BUFFER_ALLOC_EXACT:
		    newSize = size+10;
		    break;
		case XML_BUFFER_ALLOC_HYBRID:
		    if(buf->use < BASE_BUFFER_SIZE)
			    newSize = size;
		    else {
			    newSize = buf->size * 2;
			    while(size > newSize) {
				    if(newSize > UINT_MAX / 2) {
					    xmlTreeErrMemory("growing buffer");
					    return 0;
				    }
				    newSize *= 2;
			    }
		    }
		    break;

		default:
		    newSize = size+10;
		    break;
	}
	if((buf->alloc == XML_BUFFER_ALLOC_IO) && buf->contentIO) {
		start_buf = buf->content - buf->contentIO;
		if(start_buf > newSize) {
			/* move data back to start */
			memmove(buf->contentIO, buf->content, buf->use);
			buf->content = buf->contentIO;
			buf->content[buf->use] = 0;
			buf->size += start_buf;
		}
		else {
			rebuf = static_cast<xmlChar *>(SAlloc::R(buf->contentIO, start_buf + newSize));
			if(rebuf == NULL) {
				xmlTreeErrMemory("growing buffer");
				return 0;
			}
			buf->contentIO = rebuf;
			buf->content = rebuf + start_buf;
		}
	}
	else {
		if(buf->content == NULL) {
			rebuf = static_cast<xmlChar *>(SAlloc::M(newSize));
		}
		else if(buf->size - buf->use < 100) {
			rebuf = static_cast<xmlChar *>(SAlloc::R(buf->content, newSize));
		}
		else {
			/*
			 * if we are reallocating a buffer far from being full, it's
			 * better to make a new allocation and copy only the used range
			 * and free the old one.
			 */
			rebuf = static_cast<xmlChar *>(SAlloc::M(newSize));
			if(rebuf) {
				memcpy(rebuf, buf->content, buf->use);
				SAlloc::F(buf->content);
				rebuf[buf->use] = 0;
			}
		}
		if(rebuf == NULL) {
			xmlTreeErrMemory("growing buffer");
			return 0;
		}
		buf->content = rebuf;
	}
	buf->size = newSize;
	return 1;
}
/**
 * xmlBufferAdd:
 * @buf:  the buffer to dump
 * @str:  the #xmlChar string
 * @len:  the number of #xmlChar to add
 *
 * Add a string range to an XML buffer. if len == -1, the length of
 * str is recomputed.
 *
 * Returns 0 successful, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int FASTCALL xmlBufferAdd(xmlBuffer * buf, const xmlChar * str, int len)
{
	uint needSize;
	if(!str || !buf) {
		return -1;
	}
	if(buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE)
		return -1;
	if(len < -1) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufferAdd: len < 0\n");
#endif
		return -1;
	}
	if(len == 0)
		return 0;
	if(len < 0)
		len = sstrlen(str);
	if(len < 0)
		return -1;
	if(len == 0)
		return 0;
	needSize = buf->use + len + 2;
	if(needSize > buf->size) {
		if(!xmlBufferResize(buf, needSize)) {
			xmlTreeErrMemory("growing buffer");
			return XML_ERR_NO_MEMORY;
		}
	}
	memmove(&buf->content[buf->use], str, len*sizeof(xmlChar));
	buf->use += len;
	buf->content[buf->use] = 0;
	return 0;
}

/**
 * xmlBufferAddHead:
 * @buf:  the buffer
 * @str:  the #xmlChar string
 * @len:  the number of #xmlChar to add
 *
 * Add a string range to the beginning of an XML buffer.
 * if len == -1, the length of @str is recomputed.
 *
 * Returns 0 successful, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int xmlBufferAddHead(xmlBuffer * buf, const xmlChar * str, int len)
{
	uint needSize;
	if(!buf)
		return -1;
	if(buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE)
		return -1;
	if(!str) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufferAddHead: str == NULL\n");
#endif
		return -1;
	}
	if(len < -1) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufferAddHead: len < 0\n");
#endif
		return -1;
	}
	if(len == 0)
		return 0;
	if(len < 0)
		len = sstrlen(str);
	if(len <= 0)
		return -1;
	if((buf->alloc == XML_BUFFER_ALLOC_IO) && buf->contentIO) {
		size_t start_buf = buf->content - buf->contentIO;
		if(start_buf > (uint)len) {
			/*
			 * We can add it in the space previously shrinked
			 */
			buf->content -= len;
			memmove(&buf->content[0], str, len);
			buf->use += len;
			buf->size += len;
			return 0;
		}
	}
	needSize = buf->use + len + 2;
	if(needSize > buf->size) {
		if(!xmlBufferResize(buf, needSize)) {
			xmlTreeErrMemory("growing buffer");
			return XML_ERR_NO_MEMORY;
		}
	}
	memmove(&buf->content[len], &buf->content[0], buf->use);
	memmove(&buf->content[0], str, len);
	buf->use += len;
	buf->content[buf->use] = 0;
	return 0;
}
/**
 * xmlBufferCat:
 * @buf:  the buffer to add to
 * @str:  the #xmlChar string
 *
 * Append a zero terminated string to an XML buffer.
 *
 * Returns 0 successful, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int FASTCALL xmlBufferCat(xmlBuffer * pBuf, const xmlChar * pStr)
{
	if(pBuf && pStr)
		return (pBuf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) ? -1 : xmlBufferAdd(pBuf, pStr, -1);
	else
		return -1;
}
/**
 * xmlBufferCCat:
 * @buf:  the buffer to dump
 * @str:  the C char string
 *
 * Append a zero terminated C string to an XML buffer.
 *
 * Returns 0 successful, a positive error code number otherwise
 *    and -1 in case of internal or API error.
 */
int FASTCALL xmlBufferCCat(xmlBuffer * pBuf, const char * pStr)
{
	const char * cur;
	if(!pBuf || pBuf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) 
		return -1;
	if(pStr == NULL) {
#ifdef DEBUG_BUFFER
		xmlGenericError(0, "xmlBufferCCat: str == NULL\n");
#endif
		return -1;
	}
	for(cur = pStr; *cur != 0; cur++) {
		if(pBuf->use  + 10 >= pBuf->size) {
			if(!xmlBufferResize(pBuf, pBuf->use+10)) {
				xmlTreeErrMemory("growing buffer");
				return XML_ERR_NO_MEMORY;
			}
		}
		pBuf->content[pBuf->use++] = *cur;
	}
	pBuf->content[pBuf->use] = 0;
	return 0;
}
/**
 * xmlBufferWriteCHAR:
 * @buf:  the XML buffer
 * @string:  the string to add
 *
 * routine which manages and grows an output buffer. This one adds
 * xmlChars at the end of the buffer.
 */
void FASTCALL xmlBufferWriteCHAR(xmlBuffer * buf, const xmlChar * string) 
{
	if(buf && buf->alloc != XML_BUFFER_ALLOC_IMMUTABLE) 
		xmlBufferCat(buf, string);
}
/**
 * xmlBufferWriteChar:
 * @buf:  the XML buffer output
 * @string:  the string to add
 *
 * routine which manage and grows an output buffer. This one add
 * C chars at the end of the array.
 */
void FASTCALL xmlBufferWriteChar(xmlBuffer * pBuf, const char * pString) 
{
	if(pBuf && pBuf->alloc != XML_BUFFER_ALLOC_IMMUTABLE) 
		xmlBufferCCat(pBuf, pString);
}
/**
 * xmlBufferWriteQuotedString:
 * @buf:  the XML buffer output
 * @string:  the string to add
 *
 * routine which manage and grows an output buffer. This one writes
 * a quoted or double quoted #xmlChar string, checking first if it holds
 * quote or double-quotes internally
 */
void FASTCALL xmlBufferWriteQuotedString(xmlBuffer * buf, const xmlChar * string) 
{
	const xmlChar * cur, * base;
	if(buf && buf->alloc != XML_BUFFER_ALLOC_IMMUTABLE) {
		if(xmlStrchr(string, '\"')) {
			if(xmlStrchr(string, '\'')) {
	#ifdef DEBUG_BUFFER
				xmlGenericError(0, "xmlBufferWriteQuotedString: string contains quote and double-quotes !\n");
	#endif
				xmlBufferCCat(buf, "\"");
				base = cur = string;
				while(*cur != 0) {
					if(*cur == '"') {
						if(base != cur)
							xmlBufferAdd(buf, base, cur - base);
						xmlBufferAdd(buf, reinterpret_cast<const xmlChar *>("&quot;"), 6);
						cur++;
						base = cur;
					}
					else
						cur++;
				}
				if(base != cur)
					xmlBufferAdd(buf, base, cur - base);
				xmlBufferCCat(buf, "\"");
			}
			else {
				xmlBufferCCat(buf, "\'");
				xmlBufferCat(buf, string);
				xmlBufferCCat(buf, "\'");
			}
		}
		else {
			xmlBufferCCat(buf, "\"");
			xmlBufferCat(buf, string);
			xmlBufferCCat(buf, "\"");
		}
	}
}
/**
 * xmlGetDocCompressMode:
 * @doc:  the document
 *
 * get the compression ratio for a document, ZLIB based
 * Returns 0 (uncompressed) to 9 (max compression)
 */
int xmlGetDocCompressMode(const xmlDoc * doc) 
{
	return doc ? doc->compression : -1;
}
/**
 * xmlSetDocCompressMode:
 * @doc:  the document
 * @mode:  the compression ratio
 *
 * set the compression ratio for a document, ZLIB based
 * Correct values: 0 (uncompressed) to 9 (max compression)
 */
void xmlSetDocCompressMode(xmlDoc * doc, int mode) 
{
	if(doc) {
		if(mode < 0) 
			doc->compression = 0;
		else if(mode > 9) 
			doc->compression = 9;
		else 
			doc->compression = mode;
	}
}
/**
 * xmlGetCompressMode:
 *
 * get the default compression mode used, ZLIB based.
 * Returns 0 (uncompressed) to 9 (max compression)
 */
int xmlGetCompressMode()
{
	return (xmlCompressMode);
}
/**
 * xmlSetCompressMode:
 * @mode:  the compression ratio
 *
 * set the default compression mode used, ZLIB based
 * Correct values: 0 (uncompressed) to 9 (max compression)
 */
void xmlSetCompressMode(int mode) 
{
	if(mode < 0) xmlCompressMode = 0;
	else if(mode > 9) xmlCompressMode = 9;
	else xmlCompressMode = mode;
}

#define XML_TREE_NSMAP_PARENT -1
#define XML_TREE_NSMAP_XML -2
#define XML_TREE_NSMAP_DOC -3
#define XML_TREE_NSMAP_CUSTOM -4

struct xmlNsMapItem {
	xmlNsMapItem * next;
	xmlNsMapItem * prev;
	xmlNs * oldNs; /* old ns decl reference */
	xmlNs * newNs; /* new ns decl reference */
	int shadowDepth; /* Shadowed at this depth */
	/*
	 * depth:
	 * >= 0 == @node's ns-decls
	 * -1   == @parent's ns-decls
	 * -2   == the doc->oldNs XML ns-decl
	 * -3   == the doc->oldNs storage ns-decls
	 * -4   == ns-decls provided via custom ns-handling
	 */
	int depth;
};

struct xmlNsMap {
	xmlNsMapItem * first;
	xmlNsMapItem * last;
	xmlNsMapItem * pool;
};

//typedef struct xmlNsMapItem * xmlNsMapItemPtr;
//typedef struct xmlNsMap * xmlNsMapPtr;

#define XML_NSMAP_NOTEMPTY(m) (((m) != NULL) && ((m)->first != NULL))
#define XML_NSMAP_FOREACH(m, i) for(i = (m)->first; i != NULL; i = (i)->next)
#define XML_NSMAP_POP(m, i) \
	i = (m)->last; \
	(m)->last = (i)->prev; \
	if((m)->last == NULL) \
		(m)->first = NULL; \
	else \
		(m)->last->next = NULL;	\
	(i)->next = (m)->pool; \
	(m)->pool = i;

/*
 * xmlDOMWrapNsMapFree:
 * @map: the ns-map
 *
 * Frees the ns-map
 */
static void FASTCALL xmlDOMWrapNsMapFree(xmlNsMap * nsmap)
{
	if(nsmap) {
		xmlNsMapItem * cur = nsmap->pool;
		while(cur) {
			xmlNsMapItem * tmp = cur;
			cur = cur->next;
			SAlloc::F(tmp);
		}
		cur = nsmap->first;
		while(cur) {
			xmlNsMapItem * tmp = cur;
			cur = cur->next;
			SAlloc::F(tmp);
		}
		SAlloc::F(nsmap);
	}
}
/*
 * xmlDOMWrapNsMapAddItem:
 * @map: the ns-map
 * @oldNs: the old ns-struct
 * @newNs: the new ns-struct
 * @depth: depth and ns-kind information
 *
 * Adds an ns-mapping item.
 */
static xmlNsMapItem * FASTCALL xmlDOMWrapNsMapAddItem(xmlNsMap ** nsmap, int position, xmlNs * oldNs, xmlNs * newNs, int depth)
{
	xmlNsMapItem * ret;
	xmlNsMap * map;
	if(!nsmap)
		return 0;
	if(position != -1 && position != 0)
		return 0;
	map = *nsmap;
	if(map == NULL) {
		// Create the ns-map.
		map = static_cast<xmlNsMap *>(SAlloc::M(sizeof(struct xmlNsMap)));
		if(map == NULL) {
			xmlTreeErrMemory("allocating namespace map");
			return 0;
		}
		memzero(map, sizeof(struct xmlNsMap));
		*nsmap = map;
	}
	if(map->pool) {
		// Reuse an item from the pool.
		ret = map->pool;
		map->pool = ret->next;
		memzero(ret, sizeof(struct xmlNsMapItem));
	}
	else {
		// Create a new item.
		ret = static_cast<xmlNsMapItem *>(SAlloc::M(sizeof(struct xmlNsMapItem)));
		if(!ret) {
			xmlTreeErrMemory("allocating namespace map item");
			return 0;
		}
		memzero(ret, sizeof(struct xmlNsMapItem));
	}
	if(map->first == NULL) {
		// First ever.
		map->first = ret;
		map->last = ret;
	}
	else if(position == -1) {
		// Append.
		ret->prev = map->last;
		map->last->next = ret;
		map->last = ret;
	}
	else if(position == 0) {
		// Set on first position.
		map->first->prev = ret;
		ret->next = map->first;
		map->first = ret;
	}
	ret->oldNs = oldNs;
	ret->newNs = newNs;
	ret->shadowDepth = -1;
	ret->depth = depth;
	return ret;
}
/*
 * xmlDOMWrapStoreNs:
 * @doc: the doc
 * @nsName: the namespace name
 * @prefix: the prefix
 *
 * Creates or reuses an xmlNs struct on doc->oldNs with
 * the given prefix and namespace name.
 *
 * Returns the aquired ns struct or NULL in case of an API
 *    or internal error.
 */
static xmlNs * xmlDOMWrapStoreNs(xmlDoc * doc, const xmlChar * nsName, const xmlChar * prefix)
{
	if(doc) {
		xmlNs * ns = xmlTreeEnsureXMLDecl(doc);
		if(ns) {
			if(ns->next) {
				/* Reuse. */
				ns = ns->next;
				while(ns) {
					if(((ns->prefix == prefix) || sstreq(ns->prefix, prefix)) && sstreq(ns->href, nsName)) {
						return (ns);
					}
					if(ns->next == NULL)
						break;
					ns = ns->next;
				}
			}
			/* Create. */
			if(ns) {
				ns->next = xmlNewNs(NULL, nsName, prefix);
				return (ns->next);
			}
		}
	}
	return 0;
}
/*
 * xmlDOMWrapNewCtxt:
 *
 * Allocates and initializes a new DOM-wrapper context.
 *
 * Returns the xmlDOMWrapCtxtPtr or NULL in case of an internal error.
 */
xmlDOMWrapCtxtPtr xmlDOMWrapNewCtxt()
{
	xmlDOMWrapCtxtPtr ret = static_cast<xmlDOMWrapCtxtPtr>(SAlloc::M(sizeof(xmlDOMWrapCtxt)));
	if(!ret)
		xmlTreeErrMemory("allocating DOM-wrapper context");
	else
		memzero(ret, sizeof(xmlDOMWrapCtxt));
	return ret;
}
/*
 * xmlDOMWrapFreeCtxt:
 * @ctxt: the DOM-wrapper context
 *
 * Frees the DOM-wrapper context.
 */
void xmlDOMWrapFreeCtxt(xmlDOMWrapCtxtPtr ctxt)
{
	if(ctxt) {
		xmlDOMWrapNsMapFree(static_cast<xmlNsMap *>(ctxt->namespaceMap));
		/*
		 * @todo Store the namespace map in the context.
		 */
		SAlloc::F(ctxt);
	}
}
/*
 * xmlTreeLookupNsListByPrefix:
 * @nsList: a list of ns-structs
 * @prefix: the searched prefix
 *
 * Searches for a ns-decl with the given prefix in @nsList.
 *
 * Returns the ns-decl if found, NULL if not found and on
 *    API errors.
 */
static xmlNs * xmlTreeNSListLookupByPrefix(xmlNs * nsList, const xmlChar * prefix)
{
	if(nsList) {
		xmlNs * ns = nsList;
		do {
			if(sstreq(prefix, ns->prefix)) {
				return (ns);
			}
			ns = ns->next;
		} while(ns);
	}
	return 0;
}
/*
 * @map: the namespace map
 * @node: the node to start with
 *
 * Puts in-scope namespaces into the ns-map.
 *
 * Returns 0 on success, -1 on API or internal errors.
 */
static int FASTCALL xmlDOMWrapNSNormGatherInScopeNs(xmlNsMap ** map, xmlNode * P_Node)
{
	xmlNode * cur;
	xmlNs * ns;
	xmlNsMapItem * mi;
	int shadowed;
	if((map == NULL) || *map)
		return -1;
	if(!P_Node || (P_Node->type == XML_NAMESPACE_DECL))
		return -1;
	/*
	 * Get in-scope ns-decls of @parent.
	 */
	cur = P_Node;
	while(cur && (cur != (xmlNode *)cur->doc)) {
		if(cur->type == XML_ELEMENT_NODE) {
			if(cur->nsDef) {
				ns = cur->nsDef;
				do {
					shadowed = 0;
					if(XML_NSMAP_NOTEMPTY(*map)) {
						// Skip shadowed prefixes.
						XML_NSMAP_FOREACH(*map, mi) {
							if(sstreq(ns->prefix, mi->newNs->prefix)) {
								shadowed = 1;
								break;
							}
						}
					}
					/*
					 * Insert mapping.
					 */
					mi = xmlDOMWrapNsMapAddItem(map, 0, NULL, ns, XML_TREE_NSMAP_PARENT);
					if(mi == NULL)
						return -1;
					if(shadowed)
						mi->shadowDepth = 0;
					ns = ns->next;
				} while(ns);
			}
		}
		cur = cur->P_ParentNode;
	}
	return 0;
}
/*
 * XML_TREE_ADOPT_STR: If we have a dest-dict, put @str in the dict;
 * otherwise copy it, when it was in the source-dict.
 */
#define XML_TREE_ADOPT_STR(str)	\
	if(adoptStr && str) {	\
		if(destDoc->dict) { \
			const xmlChar * old = str;   \
			str = xmlDictLookupSL(destDoc->dict, str); \
			if(!sourceDoc || !sourceDoc->dict || !xmlDictOwns(sourceDoc->dict, old)) \
				SAlloc::F((char *)old); \
		} else if((sourceDoc) && (sourceDoc->dict) && xmlDictOwns(sourceDoc->dict, str)) { \
			str = sstrdup(str); \
		} \
	}

/*
 * XML_TREE_ADOPT_STR_2: If @str was in the source-dict, then
 * put it in dest-dict or copy it.
 */
#define XML_TREE_ADOPT_STR_2(str) \
	if(adoptStr && (str) && sourceDoc && sourceDoc->dict && xmlDictOwns(sourceDoc->dict, cur->content)) { \
		cur->content = destDoc->dict ? (xmlChar *)xmlDictLookupSL(destDoc->dict, cur->content) : sstrdup(BAD_CAST cur->content); \
	}

/*
 * xmlDOMWrapNSNormAddNsMapItem2:
 *
 * For internal use. Adds a ns-decl mapping.
 *
 * Returns 0 on success, -1 on internal errors.
 */
static int xmlDOMWrapNSNormAddNsMapItem2(xmlNs *** list, int * size, int * number, xmlNs * oldNs, xmlNs * newNs)
{
	if(*list == NULL) {
		*list = (xmlNs **)SAlloc::M(6 * sizeof(xmlNs *));
		if(*list == NULL) {
			xmlTreeErrMemory("alloc ns map item");
			return -1;
		}
		*size = 3;
		*number = 0;
	}
	else if((*number) >= (*size)) {
		*size *= 2;
		*list = (xmlNs **)SAlloc::R(*list, (*size) * 2 * sizeof(xmlNs *));
		if(*list == NULL) {
			xmlTreeErrMemory("realloc ns map item");
			return -1;
		}
	}
	(*list)[2 * (*number)] = oldNs;
	(*list)[2 * (*number) +1] = newNs;
	(*number)++;
	return 0;
}
/*
 * xmlDOMWrapRemoveNode:
 * @ctxt: a DOM wrapper context
 * @doc: the doc
 * @node: the node to be removed.
 * @options: set of options, unused at the moment
 *
 * Unlinks the given node from its owner.
 * This will substitute ns-references to node->nsDef for
 * ns-references to doc->oldNs, thus ensuring the removed
 * branch to be autark wrt ns-references.
 *
 * NOTE: This function was not intensively tested.
 *
 * Returns 0 on success, 1 if the node is not supported,
 *    -1 on API and internal errors.
 */
int xmlDOMWrapRemoveNode(xmlDOMWrapCtxtPtr ctxt, xmlDoc * doc, xmlNode * P_Node, int options ATTRIBUTE_UNUSED)
{
	xmlNs ** list = NULL;
	int sizeList, nbList, i, j;
	xmlNs * ns;
	if(!P_Node || !doc || P_Node->doc != doc)
		return -1;
	/* @todo 0 or -1 ? */
	if(!P_Node->P_ParentNode)
		return 0;
	switch(P_Node->type) {
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_ENTITY_REF_NODE:
		case XML_PI_NODE:
		case XML_COMMENT_NODE:
		    xmlUnlinkNode(P_Node);
		    return 0;
		case XML_ELEMENT_NODE:
		case XML_ATTRIBUTE_NODE:
		    break;
		default:
		    return 1;
	}
	xmlUnlinkNode(P_Node);
	/*
	 * Save out-of-scope ns-references in doc->oldNs.
	 */
	do {
		switch(P_Node->type) {
			case XML_ELEMENT_NODE:
			    if(!ctxt && P_Node->nsDef) {
				    ns = P_Node->nsDef;
				    do {
					    if(xmlDOMWrapNSNormAddNsMapItem2(&list, &sizeList, &nbList, ns, ns) == -1)
						    goto internal_error;
					    ns = ns->next;
				    } while(ns);
			    }
			/* No break on purpose. */
			case XML_ATTRIBUTE_NODE:
			    if(P_Node->ns) {
				    /*
				 * Find a mapping.
				     */
				    if(list) {
					    for(i = 0, j = 0; i < nbList; i++, j += 2) {
						    if(P_Node->ns == list[j]) {
							    P_Node->ns = list[++j];
							    goto next_node;
						    }
					    }
				    }
				    ns = NULL;
				    if(ctxt) {
					    /*
					 * User defined.
					     */
				    }
				    else {
					    /*
					 * Add to doc's oldNs.
					     */
					    ns = xmlDOMWrapStoreNs(doc, P_Node->ns->href,
					    P_Node->ns->prefix);
					    if(ns == NULL)
						    goto internal_error;
				    }
				    if(ns) {
					    /*
					 * Add mapping.
					     */
					    if(xmlDOMWrapNSNormAddNsMapItem2(&list, &sizeList, &nbList, P_Node->ns, ns) == -1)
						    goto internal_error;
				    }
				    P_Node->ns = ns;
			    }
			    if((P_Node->type == XML_ELEMENT_NODE) && P_Node->properties) {
				    P_Node = (xmlNode *)P_Node->properties;
				    continue;
			    }
			    break;
			default:
			    goto next_sibling;
		}
next_node:
		if((P_Node->type == XML_ELEMENT_NODE) && P_Node->children) {
			P_Node = P_Node->children;
			continue;
		}
next_sibling:
		if(!P_Node)
			break;
		if(P_Node->next)
			P_Node = P_Node->next;
		else {
			P_Node = P_Node->P_ParentNode;
			goto next_sibling;
		}
	} while(P_Node);
	SAlloc::F(list);
	return 0;
internal_error:
	SAlloc::F(list);
	return -1;
}
/*
 * xmlSearchNsByNamespaceStrict:
 * @doc: the document
 * @node: the start node
 * @nsName: the searched namespace name
 * @retNs: the resulting ns-decl
 * @prefixed: if the found ns-decl must have a prefix (for attributes)
 *
 * Dynamically searches for a ns-declaration which matches
 * the given @nsName in the ancestor-or-self axis of @node.
 *
 * Returns 1 if a ns-decl was found, 0 if not and -1 on API
 *    and internal errors.
 */
static int xmlSearchNsByNamespaceStrict(xmlDoc * doc, xmlNode * P_Node, const xmlChar* nsName, xmlNs ** retNs, int prefixed)
{
	xmlNode * cur;
	xmlNode * prev = NULL;
	xmlNode * out = NULL;
	xmlNs * ns;
	xmlNs * prevns;
	if(!doc || (nsName == NULL) || (retNs == NULL))
		return -1;
	if(!P_Node || (P_Node->type == XML_NAMESPACE_DECL))
		return -1;
	*retNs = NULL;
	if(sstreq(nsName, XML_XML_NAMESPACE)) {
		*retNs = xmlTreeEnsureXMLDecl(doc);
		return (*retNs == NULL) ? -1 : 1;
	}
	cur = P_Node;
	do {
		if(cur->type == XML_ELEMENT_NODE) {
			if(cur->nsDef) {
				for(ns = cur->nsDef; ns; ns = ns->next) {
					if(prefixed && (ns->prefix == NULL))
						continue;
					if(prev) {
						// Check the last level of ns-decls for a shadowing prefix.
						prevns = prev->nsDef;
						do {
							if((prevns->prefix == ns->prefix) || (prevns->prefix && ns->prefix && sstreq(prevns->prefix, ns->prefix))) {
								// Shadowed.
								break;
							}
							prevns = prevns->next;
						} while(prevns);
						if(prevns)
							continue;
					}
					/*
					 * Ns-name comparison.
					 */
					if((nsName == ns->href) || sstreq(nsName, ns->href)) {
						/*
						 * At this point the prefix can only be shadowed,
						 * if we are the the (at least) 3rd level of
						 * ns-decls.
						 */
						if(out) {
							int ret = xmlNsInScope(doc, P_Node, prev, ns->prefix);
							if(ret < 0)
								return -1;
							/*
							 * @todo Should we try to find a matching ns-name
							 * only once? This here keeps on searching.
							 * I think we should try further since, there might
							 * be an other matching ns-decl with an unshadowed
							 * prefix.
							 */
							if(!ret)
								continue;
						}
						*retNs = ns;
						return 1;
					}
				}
				out = prev;
				prev = cur;
			}
		}
		else if(oneof2(cur->type, XML_ENTITY_NODE, XML_ENTITY_DECL))
			return 0;
		cur = cur->P_ParentNode;
	} while(cur && (cur->doc != (xmlDoc *)cur));
	return 0;
}
/*
 * xmlSearchNsByPrefixStrict:
 * @doc: the document
 * @node: the start node
 * @prefix: the searched namespace prefix
 * @retNs: the resulting ns-decl
 *
 * Dynamically searches for a ns-declaration which matches
 * the given @nsName in the ancestor-or-self axis of @node.
 *
 * Returns 1 if a ns-decl was found, 0 if not and -1 on API
 *    and internal errors.
 */
static int xmlSearchNsByPrefixStrict(xmlDoc * doc, xmlNode * P_Node, const xmlChar* prefix, xmlNs ** retNs)
{
	xmlNode * cur;
	xmlNs * ns;
	if(!doc || !P_Node || (P_Node->type == XML_NAMESPACE_DECL))
		return -1;
	ASSIGN_PTR(retNs, NULL);
	if(IS_STR_XML(prefix)) {
		if(retNs) {
			*retNs = xmlTreeEnsureXMLDecl(doc);
			if(*retNs == NULL)
				return -1;
		}
		return 1;
	}
	cur = P_Node;
	do {
		if(cur->type == XML_ELEMENT_NODE) {
			if(cur->nsDef) {
				ns = cur->nsDef;
				do {
					if(sstreq(prefix, ns->prefix)) {
						/*
						 * Disabled namespaces, e.g. xmlns:abc="".
						 */
						if(ns->href == NULL)
							return 0;
						else {
							ASSIGN_PTR(retNs, ns);
							return 1;
						}
					}
					ns = ns->next;
				} while(ns);
			}
		}
		else if(oneof2(cur->type, XML_ENTITY_NODE, XML_ENTITY_DECL))
			return 0;
		cur = cur->P_ParentNode;
	} while(cur && (cur->doc != (xmlDoc *)cur));
	return 0;
}
/*
 * xmlDOMWrapNSNormDeclareNsForced:
 * @doc: the doc
 * @elem: the element-node to declare on
 * @nsName: the namespace-name of the ns-decl
 * @prefix: the preferred prefix of the ns-decl
 * @checkShadow: ensure that the new ns-decl doesn't shadow ancestor ns-decls
 *
 * Declares a new namespace on @elem. It tries to use the
 * given @prefix; if a ns-decl with the given prefix is already existent
 * on @elem, it will generate an other prefix.
 *
 * Returns 1 if a ns-decl was found, 0 if not and -1 on API
 *    and internal errors.
 */
static xmlNs * xmlDOMWrapNSNormDeclareNsForced(xmlDoc * doc, xmlNode * elem, const xmlChar * nsName, const xmlChar * prefix, int checkShadow)
{
	xmlNs * ret;
	char buf[50];
	const xmlChar * pref;
	int counter = 0;
	if(!doc || !elem || elem->type != XML_ELEMENT_NODE)
		return 0;
	/*
	 * Create a ns-decl on @anchor.
	 */
	pref = prefix;
	while(1) {
		/*
		 * Lookup whether the prefix is unused in elem's ns-decls.
		 */
		if(elem->nsDef && xmlTreeNSListLookupByPrefix(elem->nsDef, pref))
			goto ns_next_prefix;
		if(checkShadow && elem->P_ParentNode && ((xmlNode *)elem->P_ParentNode->doc != elem->P_ParentNode)) {
			/*
			 * Does it shadow ancestor ns-decls?
			 */
			if(xmlSearchNsByPrefixStrict(doc, elem->P_ParentNode, pref, NULL) == 1)
				goto ns_next_prefix;
		}
		ret = xmlNewNs(NULL, nsName, pref);
		if(!ret)
			return 0;
		if(elem->nsDef == NULL)
			elem->nsDef = ret;
		else {
			xmlNs * ns2 = elem->nsDef;
			while(ns2->next)
				ns2 = ns2->next;
			ns2->next = ret;
		}
		return ret;
ns_next_prefix:
		counter++;
		if(counter > 1000)
			return 0;
		if(prefix == NULL) {
			snprintf((char *)buf, sizeof(buf), "ns_%d", counter);
		}
		else
			snprintf((char *)buf, sizeof(buf), "%.30s_%d", (char *)prefix, counter);
		pref = BAD_CAST buf;
	}
}
/*
 * xmlDOMWrapNSNormAquireNormalizedNs:
 * @doc: the doc
 * @elem: the element-node to declare namespaces on
 * @ns: the ns-struct to use for the search
 * @retNs: the found/created ns-struct
 * @nsMap: the ns-map
 * @depth: the current tree depth
 * @ancestorsOnly: search in ancestor ns-decls only
 * @prefixed: if the searched ns-decl must have a prefix (for attributes)
 *
 * Searches for a matching ns-name in the ns-decls of @nsMap, if not
 * found it will either declare it on @elem, or store it in doc->oldNs.
 * If a new ns-decl needs to be declared on @elem, it tries to use the
 * @ns->prefix for it, if this prefix is already in use on @elem, it will
 * change the prefix or the new ns-decl.
 *
 * Returns 0 if succeeded, -1 otherwise and on API/internal errors.
 */
static int xmlDOMWrapNSNormAquireNormalizedNs(xmlDoc * doc, xmlNode * elem, xmlNs * ns, xmlNs ** retNs, xmlNsMap ** nsMap, int depth, int ancestorsOnly, int prefixed)
{
	xmlNsMapItem * mi;
	if(!doc || (ns == NULL) || (retNs == NULL) || (nsMap == NULL))
		return -1;
	*retNs = NULL;
	/*
	 * Handle XML namespace.
	 */
	if(IS_STR_XML(ns->prefix)) {
		/*
		 * Insert XML namespace mapping.
		 */
		*retNs = xmlTreeEnsureXMLDecl(doc);
		return (*retNs == NULL) ? -1 : 0;
	}
	/*
	 * If the search should be done in ancestors only and no
	 * @elem (the first ancestor) was specified, then skip the search.
	 */
	if((XML_NSMAP_NOTEMPTY(*nsMap)) && (!(ancestorsOnly && (elem == NULL)))) {
		/*
		 * Try to find an equal ns-name in in-scope ns-decls.
		 */
		XML_NSMAP_FOREACH(*nsMap, mi) {
			if((mi->depth >= XML_TREE_NSMAP_PARENT) &&
			    /*
			 * ancestorsOnly: This should be turned on to gain speed,
			 * if one knows that the branch itself was already
			 * ns-wellformed and no stale references existed.
			 * I.e. it searches in the ancestor axis only.
			     */
			    ((!ancestorsOnly) || (mi->depth == XML_TREE_NSMAP_PARENT)) &&
			    /* Skip shadowed prefixes. */
			    (mi->shadowDepth == -1) &&
			    /* Skip xmlns="" or xmlns:foo="". */
			    !isempty(mi->newNs->href) &&
			    /* Ensure a prefix if wanted. */
			    (!prefixed || mi->newNs->prefix) &&
			    /* Equal ns name */
			    sstreq(mi->newNs->href, ns->href)) {
				/* Set the mapping. */
				mi->oldNs = ns;
				*retNs = mi->newNs;
				return 0;
			}
		}
	}
	/*
	 * No luck, the namespace is out of scope or shadowed.
	 */
	if(elem == NULL) {
		/*
		 * Store ns-decls in "oldNs" of the document-node.
		 */
		xmlNs * tmpns = xmlDOMWrapStoreNs(doc, ns->href, ns->prefix);
		if(tmpns == NULL)
			return -1;
		/*
		 * Insert mapping.
		 */
		if(xmlDOMWrapNsMapAddItem(nsMap, -1, ns, tmpns, XML_TREE_NSMAP_DOC) == NULL) {
			xmlFreeNs(tmpns);
			return -1;
		}
		*retNs = tmpns;
	}
	else {
		xmlNs * tmpns = xmlDOMWrapNSNormDeclareNsForced(doc, elem, ns->href, ns->prefix, 0);
		if(tmpns == NULL)
			return -1;
		if(*nsMap) {
			/*
			 * Does it shadow ancestor ns-decls?
			 */
			XML_NSMAP_FOREACH(*nsMap, mi) {
				if((mi->depth < depth) && (mi->shadowDepth == -1) && ((ns->prefix == mi->newNs->prefix) || sstreq(ns->prefix, mi->newNs->prefix))) {
					// Shadows.	
					mi->shadowDepth = depth;
					break;
				}
			}
		}
		if(xmlDOMWrapNsMapAddItem(nsMap, -1, ns, tmpns, depth) == NULL) {
			xmlFreeNs(tmpns);
			return -1;
		}
		*retNs = tmpns;
	}
	return 0;
}

typedef enum {
	XML_DOM_RECONNS_REMOVEREDUND = 1<<0
} xmlDOMReconcileNSOptions;

/*
 * xmlDOMWrapReconcileNamespaces:
 * @ctxt: DOM wrapper context, unused at the moment
 * @elem: the element-node
 * @options: option flags
 *
 * Ensures that ns-references point to ns-decls hold on element-nodes.
 * Ensures that the tree is namespace wellformed by creating additional
 * ns-decls where needed. Note that, since prefixes of already existent
 * ns-decls can be shadowed by this process, it could break QNames in
 * attribute values or element content.
 *
 * NOTE: This function was not intensively tested.
 *
 * Returns 0 if succeeded, -1 otherwise and on API/internal errors.
 */
int xmlDOMWrapReconcileNamespaces(xmlDOMWrapCtxtPtr ctxt ATTRIBUTE_UNUSED, xmlNode * elem, int options)
{
	int depth = -1, adoptns = 0, parnsdone = 0;
	xmlNs * ns;
	xmlNs * prevns;
	xmlDoc * doc;
	xmlNode * cur;
	xmlNode * curElem = NULL;
	xmlNsMap * nsMap = NULL;
	xmlNsMapItem * /* topmi = NULL, */ mi;
	/* @ancestorsOnly should be set by an option flag. */
	int ancestorsOnly = 0;
	int optRemoveRedundantNS = BIN((xmlDOMReconcileNSOptions)options & XML_DOM_RECONNS_REMOVEREDUND);
	xmlNs ** listRedund = NULL;
	int sizeRedund = 0, nbRedund = 0, ret, i, j;
	if(!elem || !elem->doc || elem->type != XML_ELEMENT_NODE)
		return -1;
	doc = elem->doc;
	cur = elem;
	do {
		switch(cur->type) {
			case XML_ELEMENT_NODE:
			    adoptns = 1;
			    curElem = cur;
			    depth++;
			    /*
			 * Namespace declarations.
			     */
			    if(cur->nsDef) {
				    prevns = NULL;
				    ns = cur->nsDef;
				    while(ns) {
					    if(!parnsdone) {
						    if(elem->P_ParentNode && ((xmlNode *)elem->P_ParentNode->doc != elem->P_ParentNode)) {
							    /*
							 * Gather ancestor in-scope ns-decls.
							     */
							    if(xmlDOMWrapNSNormGatherInScopeNs(&nsMap, elem->P_ParentNode) == -1)
								    goto internal_error;
						    }
						    parnsdone = 1;
					    }
					    /*
					 * Lookup the ns ancestor-axis for equal ns-decls in scope.
					     */
					    if(optRemoveRedundantNS && XML_NSMAP_NOTEMPTY(nsMap)) {
						    XML_NSMAP_FOREACH(nsMap, mi) {
							    if((mi->depth >= XML_TREE_NSMAP_PARENT) && (mi->shadowDepth == -1) &&
							    ((ns->prefix == mi->newNs->prefix) || sstreq(ns->prefix, mi->newNs->prefix)) && 
									((ns->href == mi->newNs->href) || sstreq(ns->href, mi->newNs->href))) {
								    /*
								 * A redundant ns-decl was found.
								 * Add it to the list of redundant ns-decls.
								     */
								    if(xmlDOMWrapNSNormAddNsMapItem2(&listRedund, &sizeRedund, &nbRedund, ns, mi->newNs) == -1)
									    goto internal_error;
								    /*
								 * Remove the ns-decl from the element-node.
								     */
								    if(prevns)
									    prevns->next = ns->next;
								    else
									    cur->nsDef = ns->next;
								    goto next_ns_decl;
							    }
						    }
					    }
					    /*
					 * Skip ns-references handling if the referenced
					 * ns-decl is declared on the same element.
					     */
					    if(cur->ns && adoptns && (cur->ns == ns))
						    adoptns = 0;
					    /*
					 * Does it shadow any ns-decl?
					     */
					    if(XML_NSMAP_NOTEMPTY(nsMap)) {
						    XML_NSMAP_FOREACH(nsMap, mi) {
							    if((mi->depth >= XML_TREE_NSMAP_PARENT) && (mi->shadowDepth == -1) && ((ns->prefix == mi->newNs->prefix) || sstreq(ns->prefix, mi->newNs->prefix))) {
								    mi->shadowDepth = depth;
							    }
						    }
					    }
					    /*
					 * Push mapping.
					     */
					    if(xmlDOMWrapNsMapAddItem(&nsMap, -1, ns, ns, depth) == NULL)
						    goto internal_error;
					    prevns = ns;
next_ns_decl:
					    ns = ns->next;
				    }
			    }
			    if(!adoptns)
				    goto ns_end;
			/* No break on purpose. */
			case XML_ATTRIBUTE_NODE:
			    /* No ns, no fun. */
			    if(cur->ns == NULL)
				    goto ns_end;
			    if(!parnsdone) {
				    if(elem->P_ParentNode && ((xmlNode *)elem->P_ParentNode->doc != elem->P_ParentNode)) {
					    if(xmlDOMWrapNSNormGatherInScopeNs(&nsMap, elem->P_ParentNode) == -1)
						    goto internal_error;
				    }
				    parnsdone = 1;
			    }
			    /*
			 * Adjust the reference if this was a redundant ns-decl.
			     */
			    if(listRedund) {
				    for(i = 0, j = 0; i < nbRedund; i++, j += 2) {
					    if(cur->ns == listRedund[j]) {
						    cur->ns = listRedund[++j];
						    break;
					    }
				    }
			    }
			    /*
			 * Adopt ns-references.
			     */
			    if(XML_NSMAP_NOTEMPTY(nsMap)) {
				    /*
				 * Search for a mapping.
				     */
				    XML_NSMAP_FOREACH(nsMap, mi) {
					    if((mi->shadowDepth == -1) && (cur->ns == mi->oldNs)) {
						    cur->ns = mi->newNs;
						    goto ns_end;
					    }
				    }
			    }
			    /*
			 * Aquire a normalized ns-decl and add it to the map.
			     */
			    if(xmlDOMWrapNSNormAquireNormalizedNs(doc, curElem, cur->ns, &ns, &nsMap, depth, ancestorsOnly, (cur->type == XML_ATTRIBUTE_NODE) ? 1 : 0) == -1)
				    goto internal_error;
			    cur->ns = ns;
ns_end:
			    if((cur->type == XML_ELEMENT_NODE) && cur->properties) {
				    // Process attributes.
				    cur = (xmlNode *)cur->properties;
				    continue;
			    }
			    break;
			default:
			    goto next_sibling;
		}
into_content:
		if((cur->type == XML_ELEMENT_NODE) && cur->children) {
			// Process content of element-nodes only.
			cur = cur->children;
			continue;
		}
next_sibling:
		if(cur == elem)
			break;
		if(cur->type == XML_ELEMENT_NODE) {
			if(XML_NSMAP_NOTEMPTY(nsMap)) {
				// Pop mappings.
				while(nsMap->last && (nsMap->last->depth >= depth)) {
					XML_NSMAP_POP(nsMap, mi)
				}
				// Unshadow.
				XML_NSMAP_FOREACH(nsMap, mi) {
					if(mi->shadowDepth >= depth)
						mi->shadowDepth = -1;
				}
			}
			depth--;
		}
		if(cur->next)
			cur = cur->next;
		else {
			if(cur->type == XML_ATTRIBUTE_NODE) {
				cur = cur->P_ParentNode;
				goto into_content;
			}
			cur = cur->P_ParentNode;
			goto next_sibling;
		}
	} while(cur);

	ret = 0;
	goto exit;
internal_error:
	ret = -1;
exit:
	if(listRedund) {
		for(i = 0, j = 0; i < nbRedund; i++, j += 2) {
			xmlFreeNs(listRedund[j]);
		}
		SAlloc::F(listRedund);
	}
	xmlDOMWrapNsMapFree(nsMap);
	return ret;
}
/*
 * xmlDOMWrapAdoptBranch:
 * @ctxt: the optional context for custom processing
 * @sourceDoc: the optional sourceDoc
 * @node: the element-node to start with
 * @destDoc: the destination doc for adoption
 * @destParent: the optional new parent of @node in @destDoc
 * @options: option flags
 *
 * Ensures that ns-references point to @destDoc: either to
 * elements->nsDef entries if @destParent is given, or to
 * @destDoc->oldNs otherwise.
 * If @destParent is given, it ensures that the tree is namespace
 * wellformed by creating additional ns-decls where needed.
 * Note that, since prefixes of already existent ns-decls can be
 * shadowed by this process, it could break QNames in attribute
 * values or element content.
 *
 * NOTE: This function was not intensively tested.
 *
 * Returns 0 if succeeded, -1 otherwise and on API/internal errors.
 */
static int xmlDOMWrapAdoptBranch(xmlDOMWrapCtxtPtr ctxt, xmlDoc * sourceDoc, xmlNode * P_Node, xmlDoc * destDoc, xmlNode * destParent, int options ATTRIBUTE_UNUSED)
{
	int ret = 0;
	xmlNode * cur;
	xmlNode * curElem = NULL;
	xmlNsMap * nsMap = NULL;
	xmlNsMapItem * mi;
	xmlNs * ns = NULL;
	int depth = -1;
	/* gather @parent's ns-decls. */
	int parnsdone;
	/* @ancestorsOnly should be set per option. */
	int ancestorsOnly = 0;
	/*
	 * Optimize string adoption for equal or none dicts.
	 */
	int adoptStr = (sourceDoc && (sourceDoc->dict == destDoc->dict)) ? 0 : 1;
	/*
	 * Get the ns-map from the context if available.
	 */
	if(ctxt)
		nsMap = (xmlNsMap *)ctxt->namespaceMap;
	/*
	 * Disable search for ns-decls in the parent-axis of the
	 * desination element, if:
	 * 1) there's no destination parent
	 * 2) custom ns-reference handling is used
	 */
	parnsdone = BIN(!destParent || (ctxt && ctxt->getNsForNodeFunc));
	cur = P_Node;
	if(cur && (cur->type == XML_NAMESPACE_DECL))
		goto internal_error;
	while(cur) {
		/*
		 * Paranoid source-doc sanity check.
		 */
		if(cur->doc != sourceDoc) {
			/*
			 * We'll assume XIncluded nodes if the doc differs.
			 * @todo Do we need to reconciliate XIncluded nodes?
			 * This here skips XIncluded nodes and tries to handle
			 * broken sequences.
			 */
			if(cur->next == NULL)
				goto leave_node;
			do {
				cur = cur->next;
				if((cur->type == XML_XINCLUDE_END) || (cur->doc == P_Node->doc))
					break;
			} while(cur->next);
			if(cur->doc != P_Node->doc)
				goto leave_node;
		}
		cur->doc = destDoc;
		switch(cur->type) {
			case XML_XINCLUDE_START:
			case XML_XINCLUDE_END:
				// @todo
			    return -1;
			case XML_ELEMENT_NODE:
			    curElem = cur;
			    depth++;
			    /*
			 * Namespace declarations.
			 * - ns->href and ns->prefix are never in the dict, so
			 * we need not move the values over to the destination dict.
			 * - Note that for custom handling of ns-references,
			 * the ns-decls need not be stored in the ns-map,
			 * since they won't be referenced by node->ns.
			     */
			    if((cur->nsDef) && ((ctxt == NULL) || (ctxt->getNsForNodeFunc == NULL))) {
				    if(!parnsdone) {
					    /*
					 * Gather @parent's in-scope ns-decls.
					     */
					    if(xmlDOMWrapNSNormGatherInScopeNs(&nsMap, destParent) == -1)
						    goto internal_error;
					    parnsdone = 1;
				    }
				    for(ns = cur->nsDef; ns; ns = ns->next) {
					    /*
					 * NOTE: ns->prefix and ns->href are never in the dict.
					 * XML_TREE_ADOPT_STR(ns->prefix)
					 * XML_TREE_ADOPT_STR(ns->href)
					     */
					    /*
					 * Does it shadow any ns-decl?
					     */
					    if(XML_NSMAP_NOTEMPTY(nsMap)) {
						    XML_NSMAP_FOREACH(nsMap, mi) {
							    if((mi->depth >= XML_TREE_NSMAP_PARENT) && (mi->shadowDepth == -1) && ((ns->prefix == mi->newNs->prefix) || sstreq(ns->prefix, mi->newNs->prefix))) {
								    mi->shadowDepth = depth;
							    }
						    }
					    }
					    /*
					 * Push mapping.
					     */
					    if(xmlDOMWrapNsMapAddItem(&nsMap, -1, ns, ns, depth) == NULL)
						    goto internal_error;
				    }
			    }
			/* No break on purpose. */
			case XML_ATTRIBUTE_NODE:
			    /* No namespace, no fun. */
			    if(cur->ns == NULL)
				    goto ns_end;
			    if(!parnsdone) {
				    if(xmlDOMWrapNSNormGatherInScopeNs(&nsMap, destParent) == -1)
					    goto internal_error;
				    parnsdone = 1;
			    }
			    /*
			 * Adopt ns-references.
			     */
			    if(XML_NSMAP_NOTEMPTY(nsMap)) {
				    /*
				 * Search for a mapping.
				     */
				    XML_NSMAP_FOREACH(nsMap, mi) {
					    if((mi->shadowDepth == -1) && (cur->ns == mi->oldNs)) {
						    cur->ns = mi->newNs;
						    goto ns_end;
					    }
				    }
			    }
			    /*
			 * No matching namespace in scope. We need a new one.
			     */
			    if(ctxt && (ctxt->getNsForNodeFunc)) {
				    /*
				 * User-defined behaviour.
				     */
				    ns = ctxt->getNsForNodeFunc(ctxt, cur, cur->ns->href, cur->ns->prefix);
				    /*
				 * Insert mapping if ns is available; it's the users fault if not.
				     */
				    if(xmlDOMWrapNsMapAddItem(&nsMap, -1, cur->ns, ns, XML_TREE_NSMAP_CUSTOM) == NULL)
					    goto internal_error;
				    cur->ns = ns;
			    }
			    else {
				    /*
				 * Aquire a normalized ns-decl and add it to the map.
				     */
				    if(xmlDOMWrapNSNormAquireNormalizedNs(destDoc,
				            /* ns-decls on curElem or on destDoc->oldNs */
					    destParent ? curElem : NULL, cur->ns, &ns, &nsMap, depth, ancestorsOnly,
				            /* ns-decls must be prefixed for attributes. */
					    (cur->type == XML_ATTRIBUTE_NODE) ? 1 : 0) == -1)
					    goto internal_error;
				    cur->ns = ns;
			    }
ns_end:
			    /*
			 * Further node properties.
			 * @todo Is this all?
			     */
			    XML_TREE_ADOPT_STR(cur->name)
			    if(cur->type == XML_ELEMENT_NODE) {
				    cur->psvi = NULL;
				    cur->line = 0;
				    cur->extra = 0;
				    /*
				 * Walk attributes.
				     */
				    if(cur->properties) {
						// Process first attribute node.
					    cur = (xmlNode *)cur->properties;
					    continue;
				    }
			    }
			    else {
				    /*
				 * Attributes.
				     */
				    if(sourceDoc && (((xmlAttr *)cur)->atype == XML_ATTRIBUTE_ID)) {
					    xmlRemoveID(sourceDoc, (xmlAttr *)cur);
				    }
				    ((xmlAttr *)cur)->atype = XML_ATTRIBUTE_UNDEF;
				    ((xmlAttr *)cur)->psvi = NULL;
			    }
			    break;
			case XML_TEXT_NODE:
			case XML_CDATA_SECTION_NODE:
			    /*
			 * This puts the content in the dest dict, only if
			 * it was previously in the source dict.
			     */
			    XML_TREE_ADOPT_STR_2(cur->content)
			    goto leave_node;
			case XML_ENTITY_REF_NODE:
			    /*
			 * Remove reference to the entitity-node.
			     */
			    cur->content = NULL;
			    cur->children = NULL;
			    cur->last = NULL;
			    if((destDoc->intSubset) || (destDoc->extSubset)) {
					// Assign new entity-node if available.
				    xmlEntity * ent = xmlGetDocEntity(destDoc, cur->name);
				    if(ent) {
					    cur->content = const_cast<xmlChar *>(ent->content); // @badcast
					    cur->children = reinterpret_cast<xmlNode *>(ent);
					    cur->last = reinterpret_cast<xmlNode *>(ent);
				    }
			    }
			    goto leave_node;
			case XML_PI_NODE:
			    XML_TREE_ADOPT_STR(cur->name)
			    XML_TREE_ADOPT_STR_2(cur->content)
			    break;
			case XML_COMMENT_NODE:
			    break;
			default:
			    goto internal_error;
		}
		/*
		 * Walk the tree.
		 */
		if(cur->children) {
			cur = cur->children;
			continue;
		}

leave_node:
		if(cur == P_Node)
			break;
		if(oneof3(cur->type, XML_ELEMENT_NODE, XML_XINCLUDE_START, XML_XINCLUDE_END)) {
			/*
			 * @todo Do we expect nsDefs on XML_XINCLUDE_START?
			 */
			if(XML_NSMAP_NOTEMPTY(nsMap)) {
				/*
				 * Pop mappings.
				 */
				while(nsMap->last && (nsMap->last->depth >= depth)) {
					XML_NSMAP_POP(nsMap, mi)
				}
				/*
				 * Unshadow.
				 */
				XML_NSMAP_FOREACH(nsMap, mi) {
					if(mi->shadowDepth >= depth)
						mi->shadowDepth = -1;
				}
			}
			depth--;
		}
		if(cur->next)
			cur = cur->next;
		else if((cur->type == XML_ATTRIBUTE_NODE) && cur->P_ParentNode->children) {
			cur = cur->P_ParentNode->children;
		}
		else {
			cur = cur->P_ParentNode;
			goto leave_node;
		}
	}
	goto exit;
internal_error:
	ret = -1;
exit:
	/*
	 * Cleanup.
	 */
	if(nsMap) {
		if(ctxt && (ctxt->namespaceMap == nsMap)) {
			// Just cleanup the map but don't free.
			if(nsMap->first) {
				if(nsMap->pool)
					nsMap->last->next = nsMap->pool;
				nsMap->pool = nsMap->first;
				nsMap->first = NULL;
			}
		}
		else
			xmlDOMWrapNsMapFree(nsMap);
	}
	return ret;
}
/*
 * xmlDOMWrapCloneNode:
 * @ctxt: the optional context for custom processing
 * @sourceDoc: the optional sourceDoc
 * @node: the node to start with
 * @resNode: the clone of the given @node
 * @destDoc: the destination doc
 * @destParent: the optional new parent of @node in @destDoc
 * @deep: descend into child if set
 * @options: option flags
 *
 * References of out-of scope ns-decls are remapped to point to @destDoc:
 * 1) If @destParent is given, then nsDef entries on element-nodes are used
 * 2) If *no* @destParent is given, then @destDoc->oldNs entries are used.
 *  This is the case when you don't know already where the cloned branch
 *  will be added to.
 *
 * If @destParent is given, it ensures that the tree is namespace
 * wellformed by creating additional ns-decls where needed.
 * Note that, since prefixes of already existent ns-decls can be
 * shadowed by this process, it could break QNames in attribute
 * values or element content.
 * @todo 
 * 1) What to do with XInclude? Currently this returns an error for XInclude.
 *
 * Returns 0 if the operation succeeded,
 *    1 if a node of unsupported (or not yet supported) type was given,
 *    -1 on API/internal errors.
 */
int xmlDOMWrapCloneNode(xmlDOMWrapCtxtPtr ctxt, xmlDoc * sourceDoc, xmlNode * P_Node,
    xmlNode ** resNode, xmlDoc * destDoc, xmlNode * destParent, int deep, int options ATTRIBUTE_UNUSED)
{
	int ret = 0;
	xmlNode * cur;
	xmlNode * curElem = NULL;
	xmlNsMap * nsMap = NULL;
	xmlNsMapItem * mi;
	xmlNs * ns;
	int depth = -1;
	/* int adoptStr = 1; */
	/* gather @parent's ns-decls. */
	int parnsdone = 0;
	/*
	 * @ancestorsOnly:
	 * @todo @ancestorsOnly should be set per option.
	 *
	 */
	int ancestorsOnly = 0;
	xmlNode * resultClone = NULL;
	xmlNode * clone = NULL;
	xmlNode * parentClone = NULL;
	xmlNode * prevClone = NULL;
	xmlNs * cloneNs = NULL;
	xmlNs ** cloneNsDefSlot = NULL;
	xmlDict * dict; /* The destination dict */
	if(!P_Node || (resNode == NULL) || (destDoc == NULL))
		return -1;
	/*
	 * @todo Initially we support only element-nodes.
	 */
	if(P_Node->type != XML_ELEMENT_NODE)
		return 1;
	/*
	 * Check node->doc sanity.
	 */
	if(P_Node->doc && sourceDoc && (P_Node->doc != sourceDoc)) {
		/*
		 * Might be an XIncluded node.
		 */
		return -1;
	}
	SETIFZ(sourceDoc, P_Node->doc);
	if(sourceDoc == NULL)
		return -1;
	dict = destDoc->dict;
	/*
	 * Reuse the namespace map of the context.
	 */
	if(ctxt)
		nsMap = (xmlNsMap *)ctxt->namespaceMap;
	*resNode = NULL;
	cur = P_Node;
	if(cur && (cur->type == XML_NAMESPACE_DECL))
		return -1;
	while(cur) {
		if(cur->doc != sourceDoc) {
			/*
			 * We'll assume XIncluded nodes if the doc differs.
			 * @todo Do we need to reconciliate XIncluded nodes?
			 * @todo This here returns -1 in this case.
			 */
			goto internal_error;
		}
		/*
		 * Create a new node.
		 */
		switch(cur->type) {
			case XML_XINCLUDE_START:
			case XML_XINCLUDE_END:
			    /*
			 * @todo What to do with XInclude?
			     */
			    goto internal_error;
			    break;
			case XML_ELEMENT_NODE:
			case XML_TEXT_NODE:
			case XML_CDATA_SECTION_NODE:
			case XML_COMMENT_NODE:
			case XML_PI_NODE:
			case XML_DOCUMENT_FRAG_NODE:
			case XML_ENTITY_REF_NODE:
			case XML_ENTITY_NODE:
			    /*
			 * Nodes of xmlNode structure.
			     */
			    clone = static_cast<xmlNode *>(SAlloc::M(sizeof(xmlNode)));
			    if(clone == NULL) {
				    xmlTreeErrMemory("xmlDOMWrapCloneNode(): allocating a node");
				    goto internal_error;
			    }
			    memzero(clone, sizeof(xmlNode));
			    /*
			 * Set hierachical links.
			     */
			    if(resultClone) {
				    clone->P_ParentNode = parentClone;
				    if(prevClone) {
					    prevClone->next = clone;
					    clone->prev = prevClone;
				    }
				    else
					    parentClone->children = clone;
			    }
			    else
				    resultClone = clone;

			    break;
			case XML_ATTRIBUTE_NODE:
			    /*
			 * Attributes (xmlAttr).
			     */
			    clone = (xmlNode *)SAlloc::M(sizeof(xmlAttr));
			    if(clone == NULL) {
				    xmlTreeErrMemory("xmlDOMWrapCloneNode(): allocating an attr-node");
				    goto internal_error;
			    }
			    memzero(clone, sizeof(xmlAttr));
			    /*
			 * Set hierachical links.
			 * @todo Change this to add to the end of attributes.
			     */
			    if(resultClone) {
				    clone->P_ParentNode = parentClone;
				    if(prevClone) {
					    prevClone->next = clone;
					    clone->prev = prevClone;
				    }
				    else
					    parentClone->properties = (xmlAttr *)clone;
			    }
			    else
				    resultClone = clone;
			    break;
			default:
			    /*
			 * @todo QUESTION: Any other nodes expected?
			     */
			    goto internal_error;
		}
		clone->type = cur->type;
		clone->doc = destDoc;
		/*
		 * Clone the name of the node if any.
		 */
		if(cur->name == xmlStringText)
			clone->name = xmlStringText;
		else if(cur->name == xmlStringTextNoenc)
			/*
			 * NOTE: Although xmlStringTextNoenc is never assigned to a node
			 * in tree.c, it might be set in Libxslt via
			 * "xsl:disable-output-escaping".
			 */
			clone->name = xmlStringTextNoenc;
		else if(cur->name == xmlStringComment)
			clone->name = xmlStringComment;
		else if(cur->name) {
			DICT_CONST_COPY(cur->name, clone->name);
		}

		switch(cur->type) {
			case XML_XINCLUDE_START:
			case XML_XINCLUDE_END:
				// @todo
			    return -1;
			case XML_ELEMENT_NODE:
			    curElem = cur;
			    depth++;
			    /*
			 * Namespace declarations.
			     */
			    if(cur->nsDef) {
				    if(!parnsdone) {
					    if(destParent && (ctxt == NULL)) {
						    /*
						 * Gather @parent's in-scope ns-decls.
						     */
						    if(xmlDOMWrapNSNormGatherInScopeNs(&nsMap, destParent) == -1)
							    goto internal_error;
					    }
					    parnsdone = 1;
				    }
				    /*
				 * Clone namespace declarations.
				     */
				    cloneNsDefSlot = &(clone->nsDef);
				    for(ns = cur->nsDef; ns; ns = ns->next) {
					    /*
					 * Create a new xmlNs.
					     */
					    cloneNs = static_cast<xmlNs *>(SAlloc::M(sizeof(xmlNs)));
					    if(cloneNs == NULL) {
						    xmlTreeErrMemory("xmlDOMWrapCloneNode(): allocating namespace");
						    return -1;
					    }
					    memzero(cloneNs, sizeof(xmlNs));
					    cloneNs->type = XML_LOCAL_NAMESPACE;
					    cloneNs->href = sstrdup(ns->href);
					    cloneNs->prefix = sstrdup(ns->prefix);
					    *cloneNsDefSlot = cloneNs;
					    cloneNsDefSlot = &(cloneNs->next);
					    /*
					 * Note that for custom handling of ns-references,
					 * the ns-decls need not be stored in the ns-map,
					 * since they won't be referenced by node->ns.
					     */
					    if(!ctxt || (ctxt->getNsForNodeFunc == NULL)) {
						    /*
						 * Does it shadow any ns-decl?
						     */
						    if(XML_NSMAP_NOTEMPTY(nsMap)) {
							    XML_NSMAP_FOREACH(nsMap, mi) {
								    if((mi->depth >= XML_TREE_NSMAP_PARENT) && (mi->shadowDepth == -1) && ((ns->prefix == mi->newNs->prefix) || sstreq(ns->prefix, mi->newNs->prefix))) {
									    mi->shadowDepth = depth; // Mark as shadowed at the current depth.
								    }
							    }
						    }
						    /*
						 * Push mapping.
						     */
						    if(xmlDOMWrapNsMapAddItem(&nsMap, -1, ns, cloneNs, depth) == NULL)
							    goto internal_error;
					    }
				    }
			    }
			    /* cur->ns will be processed further down. */
			    break;
			case XML_ATTRIBUTE_NODE:
			    /* IDs will be processed further down. */
			    /* cur->ns will be processed further down. */
			    break;
			case XML_TEXT_NODE:
			case XML_CDATA_SECTION_NODE:
			    /*
			 * Note that this will also cover the values of attributes.
			     */
			    DICT_COPY(cur->content, clone->content);
			    goto leave_node;
			case XML_ENTITY_NODE:
			    /* @todo What to do here? */
			    goto leave_node;
			case XML_ENTITY_REF_NODE:
			    if(sourceDoc != destDoc) {
				    if((destDoc->intSubset) || (destDoc->extSubset)) {
					    /*
					 * Different doc: Assign new entity-node if available.
					     */
					    xmlEntity * ent = xmlGetDocEntity(destDoc, cur->name);
					    if(ent) {
						    clone->content = const_cast<xmlChar *>(ent->content); // @badcast
						    clone->children = reinterpret_cast<xmlNode *>(ent);
						    clone->last = reinterpret_cast<xmlNode *>(ent);
					    }
				    }
			    }
			    else {
				    /*
				 * Same doc: Use the current node's entity declaration
				 * and value.
				     */
				    clone->content = cur->content;
				    clone->children = cur->children;
				    clone->last = cur->last;
			    }
			    goto leave_node;
			case XML_PI_NODE:
			    DICT_COPY(cur->content, clone->content);
			    goto leave_node;
			case XML_COMMENT_NODE:
			    DICT_COPY(cur->content, clone->content);
			    goto leave_node;
			default:
			    goto internal_error;
		}

		if(cur->ns == NULL)
			goto end_ns_reference;

/* handle_ns_reference: */
		/*
		** The following will take care of references to ns-decls ********
		** and is intended only for element- and attribute-nodes.
		**
		*/
		if(!parnsdone) {
			if(destParent && (ctxt == NULL)) {
				if(xmlDOMWrapNSNormGatherInScopeNs(&nsMap, destParent) == -1)
					goto internal_error;
			}
			parnsdone = 1;
		}
		/*
		 * Adopt ns-references.
		 */
		if(XML_NSMAP_NOTEMPTY(nsMap)) {
			/*
			 * Search for a mapping.
			 */
			XML_NSMAP_FOREACH(nsMap, mi) {
				if((mi->shadowDepth == -1) && (cur->ns == mi->oldNs)) {
					/*
					 * This is the nice case: a mapping was found.
					 */
					clone->ns = mi->newNs;
					goto end_ns_reference;
				}
			}
		}
		/*
		 * No matching namespace in scope. We need a new one.
		 */
		if(ctxt && ctxt->getNsForNodeFunc) {
			/*
			 * User-defined behaviour.
			 */
			ns = ctxt->getNsForNodeFunc(ctxt, cur, cur->ns->href, cur->ns->prefix);
			/*
			 * Add user's mapping.
			 */
			if(xmlDOMWrapNsMapAddItem(&nsMap, -1, cur->ns, ns, XML_TREE_NSMAP_CUSTOM) == NULL)
				goto internal_error;
			clone->ns = ns;
		}
		else {
			/*
			 * Aquire a normalized ns-decl and add it to the map.
			 */
			if(xmlDOMWrapNSNormAquireNormalizedNs(destDoc,
			            /* ns-decls on curElem or on destDoc->oldNs */
				    destParent ? curElem : NULL, cur->ns, &ns, &nsMap, depth,
			            /* if we need to search only in the ancestor-axis */
				    ancestorsOnly,
			            /* ns-decls must be prefixed for attributes. */
				    (cur->type == XML_ATTRIBUTE_NODE) ? 1 : 0) == -1)
				goto internal_error;
			clone->ns = ns;
		}
end_ns_reference:
		/*
		 * Some post-processing.
		 *
		 * Handle ID attributes.
		 */
		if((clone->type == XML_ATTRIBUTE_NODE) && clone->P_ParentNode) {
			if(xmlIsID(destDoc, clone->P_ParentNode, (xmlAttr *)clone)) {
				xmlChar * idVal = xmlNodeListGetString(cur->doc, cur->children, 1);
				if(idVal) {
					if(xmlAddID(NULL, destDoc, idVal, (xmlAttr *)cur) == NULL) {
						/* @todo error message. */
						SAlloc::F(idVal);
						goto internal_error;
					}
					SAlloc::F(idVal);
				}
			}
		}
		/*
		**
		** The following will traverse the tree **************************
		**
		*
		* Walk the element's attributes before descending into child-nodes.
		*/
		if((cur->type == XML_ELEMENT_NODE) && cur->properties) {
			prevClone = NULL;
			parentClone = clone;
			cur = (xmlNode *)cur->properties;
			continue;
		}
into_content:
		/*
		 * Descend into child-nodes.
		 */
		if(cur->children) {
			if(deep || (cur->type == XML_ATTRIBUTE_NODE)) {
				prevClone = NULL;
				parentClone = clone;
				cur = cur->children;
				continue;
			}
		}
leave_node:
		/*
		 * At this point we are done with the node, its content
		 * and an element-nodes's attribute-nodes.
		 */
		if(cur == P_Node)
			break;
		if(oneof3(cur->type, XML_ELEMENT_NODE, XML_XINCLUDE_START, XML_XINCLUDE_END)) {
			/*
			 * @todo Do we expect nsDefs on XML_XINCLUDE_START?
			 */
			if(XML_NSMAP_NOTEMPTY(nsMap)) {
				/*
				 * Pop mappings.
				 */
				while(nsMap->last && (nsMap->last->depth >= depth)) {
					XML_NSMAP_POP(nsMap, mi)
				}
				/*
				 * Unshadow.
				 */
				XML_NSMAP_FOREACH(nsMap, mi) {
					if(mi->shadowDepth >= depth)
						mi->shadowDepth = -1;
				}
			}
			depth--;
		}
		if(cur->next) {
			prevClone = clone;
			cur = cur->next;
		}
		else if(cur->type != XML_ATTRIBUTE_NODE) {
			/*
			 * Set clone->last.
			 */
			if(clone->P_ParentNode)
				clone->P_ParentNode->last = clone;
			clone = clone->P_ParentNode;
			if(clone)
				parentClone = clone->P_ParentNode;
			/*
			 * Process parent --> next;
			 */
			cur = cur->P_ParentNode;
			goto leave_node;
		}
		else {
			/* This is for attributes only. */
			clone = clone->P_ParentNode;
			parentClone = clone->P_ParentNode;
			/*
			 * Process parent-element --> children.
			 */
			cur = cur->P_ParentNode;
			goto into_content;
		}
	}
	goto exit;
internal_error:
	ret = -1;
exit:
	/*
	 * Cleanup.
	 */
	if(nsMap) {
		if(ctxt && (ctxt->namespaceMap == nsMap)) {
			/*
			 * Just cleanup the map but don't free.
			 */
			if(nsMap->first) {
				if(nsMap->pool)
					nsMap->last->next = nsMap->pool;
				nsMap->pool = nsMap->first;
				nsMap->first = NULL;
			}
		}
		else
			xmlDOMWrapNsMapFree(nsMap);
	}
	/*
	 * @todo Should we try a cleanup of the cloned node in case of a fatal error?
	 */
	*resNode = resultClone;
	return ret;
}

/*
 * xmlDOMWrapAdoptAttr:
 * @ctxt: the optional context for custom processing
 * @sourceDoc: the optional source document of attr
 * @attr: the attribute-node to be adopted
 * @destDoc: the destination doc for adoption
 * @destParent: the optional new parent of @attr in @destDoc
 * @options: option flags
 *
 * @attr is adopted by @destDoc.
 * Ensures that ns-references point to @destDoc: either to
 * elements->nsDef entries if @destParent is given, or to
 * @destDoc->oldNs otherwise.
 *
 * Returns 0 if succeeded, -1 otherwise and on API/internal errors.
 */
static int xmlDOMWrapAdoptAttr(xmlDOMWrapCtxtPtr ctxt, xmlDoc * sourceDoc, xmlAttr * attr, xmlDoc * destDoc, xmlNode * destParent, int options ATTRIBUTE_UNUSED)
{
	xmlNode * cur;
	int adoptStr = 1;
	if(!attr || (destDoc == NULL))
		return -1;
	attr->doc = destDoc;
	if(attr->ns) {
		xmlNs * ns = NULL;
		if(ctxt) {
			/* @todo User defined. */
		}
		/* XML Namespace. */
		if(IS_STR_XML(attr->ns->prefix)) {
			ns = xmlTreeEnsureXMLDecl(destDoc);
		}
		else if(destParent == NULL) {
			/*
			 * Store in @destDoc->oldNs.
			 */
			ns = xmlDOMWrapStoreNs(destDoc, attr->ns->href, attr->ns->prefix);
		}
		else {
			/*
			 * Declare on @destParent.
			 */
			if(xmlSearchNsByNamespaceStrict(destDoc, destParent, attr->ns->href, &ns, 1) == -1)
				goto internal_error;
			if(ns == NULL) {
				ns = xmlDOMWrapNSNormDeclareNsForced(destDoc, destParent, attr->ns->href, attr->ns->prefix, 1);
			}
		}
		if(ns == NULL)
			goto internal_error;
		attr->ns = ns;
	}
	XML_TREE_ADOPT_STR(attr->name);
	attr->atype = XML_ATTRIBUTE_UNDEF;
	attr->psvi = NULL;
	/*
	 * Walk content.
	 */
	if(attr->children == NULL)
		return 0;
	cur = attr->children;
	if(cur && (cur->type == XML_NAMESPACE_DECL))
		goto internal_error;
	while(cur) {
		cur->doc = destDoc;
		switch(cur->type) {
			case XML_TEXT_NODE:
			case XML_CDATA_SECTION_NODE:
			    XML_TREE_ADOPT_STR_2(cur->content)
			    break;
			case XML_ENTITY_REF_NODE:
			    /*
			 * Remove reference to the entitity-node.
			     */
			    cur->content = NULL;
			    cur->children = NULL;
			    cur->last = NULL;
			    if((destDoc->intSubset) || (destDoc->extSubset)) {
					// Assign new entity-node if available.
				    xmlEntity * ent = xmlGetDocEntity(destDoc, cur->name);
				    if(ent) {
					    cur->content = const_cast<xmlChar *>(ent->content); // @badcast
					    cur->children = reinterpret_cast<xmlNode *>(ent);
					    cur->last = reinterpret_cast<xmlNode *>(ent);
				    }
			    }
			    break;
			default:
			    break;
		}
		if(cur->children) {
			cur = cur->children;
			continue;
		}
next_sibling:
		if(cur == (xmlNode *)attr)
			break;
		if(cur->next)
			cur = cur->next;
		else {
			cur = cur->P_ParentNode;
			goto next_sibling;
		}
	}
	return 0;
internal_error:
	return -1;
}
/*
 * xmlDOMWrapAdoptNode:
 * @ctxt: the optional context for custom processing
 * @sourceDoc: the optional sourceDoc
 * @node: the node to start with
 * @destDoc: the destination doc
 * @destParent: the optional new parent of @node in @destDoc
 * @options: option flags
 *
 * References of out-of scope ns-decls are remapped to point to @destDoc:
 * 1) If @destParent is given, then nsDef entries on element-nodes are used
 * 2) If *no* @destParent is given, then @destDoc->oldNs entries are used
 *  This is the case when you have an unlinked node and just want to move it
 *  to the context of
 *
 * If @destParent is given, it ensures that the tree is namespace
 * wellformed by creating additional ns-decls where needed.
 * Note that, since prefixes of already existent ns-decls can be
 * shadowed by this process, it could break QNames in attribute
 * values or element content.
 * NOTE: This function was not intensively tested.
 *
 * Returns 0 if the operation succeeded,
 *    1 if a node of unsupported type was given,
 *    2 if a node of not yet supported type was given and
 *    -1 on API/internal errors.
 */
int xmlDOMWrapAdoptNode(xmlDOMWrapCtxtPtr ctxt, xmlDoc * sourceDoc, xmlNode * pNode, xmlDoc * destDoc, xmlNode * destParent, int options)
{
	if(!pNode || (pNode->type == XML_NAMESPACE_DECL) || (destDoc == NULL) || (destParent && (destParent->doc != destDoc)))
		return -1;
	/*
	 * Check node->doc sanity.
	 */
	if(pNode->doc && sourceDoc && (pNode->doc != sourceDoc)) {
		// Might be an XIncluded node.
		return -1;
	}
	SETIFZ(sourceDoc, pNode->doc);
	if(sourceDoc == destDoc)
		return -1;
	switch(pNode->type) {
		case XML_ELEMENT_NODE:
		case XML_ATTRIBUTE_NODE:
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_ENTITY_REF_NODE:
		case XML_PI_NODE:
		case XML_COMMENT_NODE:
		    break;
		case XML_DOCUMENT_FRAG_NODE:
		    /* @todo Support document-fragment-nodes. */
		    return (2);
		default:
		    return 1;
	}
	/*
	 * Unlink only if @node was not already added to @destParent.
	 */
	if(pNode->P_ParentNode && (destParent != pNode->P_ParentNode))
		xmlUnlinkNode(pNode);
	if(pNode->type == XML_ELEMENT_NODE) {
		return (xmlDOMWrapAdoptBranch(ctxt, sourceDoc, pNode, destDoc, destParent, options));
	}
	else if(pNode->type == XML_ATTRIBUTE_NODE) {
		return (xmlDOMWrapAdoptAttr(ctxt, sourceDoc, (xmlAttr *)pNode, destDoc, destParent, options));
	}
	else {
		xmlNode * cur = pNode;
		int adoptStr = 1;
		cur->doc = destDoc;
		/*
		 * Optimize string adoption.
		 */
		if(sourceDoc && (sourceDoc->dict == destDoc->dict))
			adoptStr = 0;
		switch(pNode->type) {
			case XML_TEXT_NODE:
			case XML_CDATA_SECTION_NODE:
			    XML_TREE_ADOPT_STR_2(pNode->content)
			    break;
			case XML_ENTITY_REF_NODE:
			    //
			    // Remove reference to the entitity-node.
			    //
			    pNode->content = NULL;
			    pNode->children = NULL;
			    pNode->last = NULL;
			    if((destDoc->intSubset) || (destDoc->extSubset)) {
					// Assign new entity-node if available.
				    xmlEntity * ent = xmlGetDocEntity(destDoc, pNode->name);
				    if(ent) {
					    pNode->content = const_cast<xmlChar *>(ent->content); // @badcast
					    pNode->children = reinterpret_cast<xmlNode *>(ent);
					    pNode->last = reinterpret_cast<xmlNode *>(ent);
				    }
			    }
			    XML_TREE_ADOPT_STR(pNode->name)
			    break;
			case XML_PI_NODE: {
			    XML_TREE_ADOPT_STR(pNode->name)
			    XML_TREE_ADOPT_STR_2(pNode->content)
			    break;
		    }
			default:
			    break;
		}
	}
	return 0;
}

#define bottom_tree
//#include "elfgcchack.h"
