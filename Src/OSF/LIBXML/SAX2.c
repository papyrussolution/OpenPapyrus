/*
 * SAX2.c : Default SAX2 handler to build a tree.
 *
 * See Copyright for the status of this software.
 *
 * Daniel Veillard <daniel@veillard.com>
 */
#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop

/* Define SIZE_T_MAX unless defined through <limits.h>. */
#ifndef SIZE_T_MAX
	#define SIZE_T_MAX     ((size_t)-1)
#endif /* !SIZE_T_MAX */
/* #define DEBUG_SAX2 */
/* #define DEBUG_SAX2_TREE */

xmlSAXHandler::xmlSAXHandler() // @v10.7.9
{
	THISZERO();
}

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

/*
 * xmlSAX2ErrMemory:
 * @ctxt:  an XML validation parser context
 * @msg:   a string to accompany the error message
 */
static void FASTCALL xmlSAX2ErrMemory(xmlParserCtxt * ctxt, const char * msg)
{
	xmlStructuredErrorFunc schannel = NULL;
	const char * str1 = "out of memory\n";
	if(ctxt) {
		ctxt->errNo = XML_ERR_NO_MEMORY;
		if(ctxt->sax && (ctxt->sax->initialized == XML_SAX2_MAGIC))
			schannel = ctxt->sax->serror;
		__xmlRaiseError(schannel, ctxt->vctxt.error, ctxt->vctxt.userData, ctxt, NULL, XML_FROM_PARSER, XML_ERR_NO_MEMORY,
		    XML_ERR_ERROR, NULL, 0, PTRCHRC_(str1), NULL, NULL, 0, 0, msg, PTRCHRC_(str1), 0);
		ctxt->errNo = XML_ERR_NO_MEMORY;
		ctxt->instate = XML_PARSER_EOF;
		ctxt->disableSAX = 1;
	}
	else {
		__xmlRaiseError(schannel, 0, 0, ctxt, NULL, XML_FROM_PARSER, XML_ERR_NO_MEMORY, XML_ERR_ERROR, NULL, 0, PTRCHRC_(str1),
		    NULL, NULL, 0, 0, msg, PTRCHRC_(str1), 0);
	}
}
/**
 * xmlValidError:
 * @ctxt:  an XML validation parser context
 * @error:  the error number
 * @msg:  the error message
 * @str1:  extra data
 * @str2:  extra data
 *
 * Handle a validation error
 */
static void FASTCALL xmlErrValid(xmlParserCtxt * ctxt, xmlParserErrors error, const char * msg, const char * str1, const char * str2)
{
	if(!xmlParserCtxt::IsEofInNonSaxMode(ctxt)) {
		xmlStructuredErrorFunc schannel = NULL;
		if(ctxt) {
			ctxt->errNo = error;
			if(ctxt->sax && (ctxt->sax->initialized == XML_SAX2_MAGIC))
				schannel = ctxt->sax->serror;
			__xmlRaiseError(schannel, ctxt->vctxt.error, ctxt->vctxt.userData, ctxt, NULL, XML_FROM_DTD, error,
				XML_ERR_ERROR, NULL, 0, PTRCHRC_(str1), PTRCHRC_(str2), NULL, 0, 0, msg, PTRCHRC_(str1), PTRCHRC_(str2));
			ctxt->valid = 0;
		}
		else {
			__xmlRaiseError(schannel, 0, 0, ctxt, NULL, XML_FROM_DTD, error,
				XML_ERR_ERROR, NULL, 0, PTRCHRC_(str1), PTRCHRC_(str2), NULL, 0, 0, msg, PTRCHRC_(str1), PTRCHRC_(str2));
		}
	}
}

/**
 * xmlFatalErrMsg:
 * @ctxt:  an XML parser context
 * @error:  the error number
 * @msg:  the error message
 * @str1:  an error string
 * @str2:  an error string
 *
 * Handle a fatal parser error, i.e. violating Well-Formedness constraints
 */
static void FASTCALL xmlFatalErrMsg(xmlParserCtxt * ctxt, xmlParserErrors error, const char * msg, const xmlChar * str1, const xmlChar * str2)
{
	if(!xmlParserCtxt::IsEofInNonSaxMode(ctxt)) {
		if(ctxt)
			ctxt->errNo = error;
		__xmlRaiseError(0, 0, 0, ctxt, 0, XML_FROM_PARSER, error, XML_ERR_FATAL, NULL, 0, PTRCHRC_(str1), PTRCHRC_(str2), NULL, 0, 0, msg, str1, str2);
		if(ctxt) {
			ctxt->wellFormed = 0;
			ctxt->valid = 0;
			if(ctxt->recovery == 0)
				ctxt->disableSAX = 1;
		}
	}
}
/**
 * xmlWarnMsg:
 * @ctxt:  an XML parser context
 * @error:  the error number
 * @msg:  the error message
 * @str1:  an error string
 * @str2:  an error string
 *
 * Handle a parser warning
 */
static void xmlWarnMsg(xmlParserCtxt * ctxt, xmlParserErrors error, const char * msg, const xmlChar * str1)
{
	if(!xmlParserCtxt::IsEofInNonSaxMode(ctxt)) {
		if(ctxt)
			ctxt->errNo = error;
		__xmlRaiseError(0, 0, 0, ctxt, 0, XML_FROM_PARSER, error, XML_ERR_WARNING, NULL, 0, PTRCHRC_(str1), NULL, NULL, 0, 0, msg, str1);
	}
}
/**
 * xmlNsErrMsg:
 * @ctxt:  an XML parser context
 * @error:  the error number
 * @msg:  the error message
 * @str1:  an error string
 * @str2:  an error string
 *
 * Handle a namespace__ error
 */
static void FASTCALL xmlNsErrMsg(xmlParserCtxt * ctxt, xmlParserErrors error, const char * msg, const xmlChar * str1, const xmlChar * str2)
{
	if(!xmlParserCtxt::IsEofInNonSaxMode(ctxt)) {
		if(ctxt)
			ctxt->errNo = error;
		__xmlRaiseError(0, 0, 0, ctxt, 0, XML_FROM_NAMESPACE, error, XML_ERR_ERROR, NULL, 0, PTRCHRC_(str1), PTRCHRC_(str2), NULL, 0, 0, msg, str1, str2);
	}
}

/**
 * xmlNsWarnMsg:
 * @ctxt:  an XML parser context
 * @error:  the error number
 * @msg:  the error message
 * @str1:  an error string
 *
 * Handle a namespace__ warning
 */
static void FASTCALL xmlNsWarnMsg(xmlParserCtxt * ctxt, xmlParserErrors error, const char * msg, const xmlChar * str1, const xmlChar * str2)
{
	if(!xmlParserCtxt::IsEofInNonSaxMode(ctxt)) {
		if(ctxt)
			ctxt->errNo = error;
		__xmlRaiseError(0, 0, 0, ctxt, 0, XML_FROM_NAMESPACE, error, XML_ERR_WARNING, NULL, 0, PTRCHRC_(str1), PTRCHRC_(str2), NULL, 0, 0, msg, str1, str2);
	}
}
/**
 * xmlSAX2GetPublicId:
 * @ctx: the user data (XML parser context)
 *
 * Provides the public ID e.g. "-//SGMLSOURCE//DTD DEMO//EN"
 *
 * Returns a xmlChar *
 */
const xmlChar * xmlSAX2GetPublicId(void * ctx ATTRIBUTE_UNUSED)
{
	/* xmlParserCtxt * ctxt = (xmlParserCtxt *) ctx; */
	return 0;
}

/**
 * xmlSAX2GetSystemId:
 * @ctx: the user data (XML parser context)
 *
 * Provides the system ID, basically URL or filename e.g.
 * http://www.sgmlsource.com/dtds/memo.dtd
 *
 * Returns a xmlChar *
 */
const xmlChar * xmlSAX2GetSystemId(void * ctx)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	return (ctx && ctxt->input) ? ((const xmlChar *)ctxt->input->filename) : 0;
}
/**
 * xmlSAX2GetLineNumber:
 * @ctx: the user data (XML parser context)
 *
 * Provide the line number of the current parsing point.
 *
 * Returns an int
 */
int xmlSAX2GetLineNumber(void * ctx)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	return (ctx && ctxt->input) ? ctxt->input->line : 0;
}
/**
 * xmlSAX2GetColumnNumber:
 * @ctx: the user data (XML parser context)
 *
 * Provide the column number of the current parsing point.
 *
 * Returns an int
 */
int xmlSAX2GetColumnNumber(void * ctx)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	return (ctx && ctxt->input) ? ctxt->input->col : 0;
}
/**
 * xmlSAX2IsStandalone:
 * @ctx: the user data (XML parser context)
 *
 * Is this document tagged standalone ?
 *
 * Returns 1 if true
 */
int xmlSAX2IsStandalone(void * ctx)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	return (ctx && ctxt->myDoc) ? (ctxt->myDoc->standalone == 1) : 0;
}
/**
 * xmlSAX2HasInternalSubset:
 * @ctx: the user data (XML parser context)
 *
 * Does this document has an internal subset
 *
 * Returns 1 if true
 */
int xmlSAX2HasInternalSubset(void * ctx)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	return (ctxt && ctxt->myDoc) ? (ctxt->myDoc->intSubset != NULL) : 0;
}
/**
 * xmlSAX2HasExternalSubset:
 * @ctx: the user data (XML parser context)
 *
 * Does this document has an external subset
 *
 * Returns 1 if true
 */
int xmlSAX2HasExternalSubset(void * ctx)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	return (ctxt && ctxt->myDoc) ? (ctxt->myDoc->extSubset != NULL) : 0;
}
/**
 * xmlSAX2InternalSubset:
 * @ctx:  the user data (XML parser context)
 * @name:  the root element name
 * @ExternalID:  the external ID
 * @SystemID:  the SYSTEM ID (e.g. filename or URL)
 *
 * Callback on internal subset declaration.
 */
void xmlSAX2InternalSubset(void * ctx, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	if(ctx) {
#ifdef DEBUG_SAX
		xmlGenericError(0, "SAX.xmlSAX2InternalSubset(%s, %s, %s)\n", name, ExternalID, SystemID);
#endif
		if(ctxt->myDoc) {
			xmlDtd * dtd = xmlGetIntSubset(ctxt->myDoc);
			if(dtd) {
				if(ctxt->html)
					return;
				xmlUnlinkNode((xmlNode *)dtd);
				xmlFreeDtd(dtd);
				ctxt->myDoc->intSubset = NULL;
			}
			ctxt->myDoc->intSubset = xmlCreateIntSubset(ctxt->myDoc, name, ExternalID, SystemID);
			if(ctxt->myDoc->intSubset == NULL)
				xmlSAX2ErrMemory(ctxt, "xmlSAX2InternalSubset");
		}
	}
}

/**
 * xmlSAX2ExternalSubset:
 * @ctx: the user data (XML parser context)
 * @name:  the root element name
 * @ExternalID:  the external ID
 * @SystemID:  the SYSTEM ID (e.g. filename or URL)
 *
 * Callback on external subset declaration.
 */
void xmlSAX2ExternalSubset(void * ctx, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	if(!ctx) return;
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2ExternalSubset(%s, %s, %s)\n", name, ExternalID, SystemID);
#endif
	if((ExternalID || SystemID) && (((ctxt->validate) || ctxt->loadsubset) && (ctxt->wellFormed && ctxt->myDoc))) {
		/*
		 * Try to fetch and parse the external subset.
		 */
		xmlParserInput * oldinput;
		int oldinputNr;
		int oldinputMax;
		xmlParserInput ** oldinputTab;
		xmlParserInput * input = NULL;
		xmlCharEncoding enc;
		int oldcharset;
		const xmlChar * oldencoding;
		/*
		 * Ask the Entity resolver to load the damn thing
		 */
		if(ctxt->sax && ctxt->sax->resolveEntity)
			input = ctxt->sax->resolveEntity(ctxt->userData, ExternalID, SystemID);
		if(!input) {
			return;
		}
		xmlNewDtd(ctxt->myDoc, name, ExternalID, SystemID);
		/*
		 * make sure we won't destroy the main document context
		 */
		oldinput = ctxt->input;
		oldinputNr = ctxt->inputNr;
		oldinputMax = ctxt->inputMax;
		oldinputTab = ctxt->inputTab;
		oldcharset = ctxt->charset;
		oldencoding = ctxt->encoding;
		ctxt->encoding = NULL;
		ctxt->inputTab = (xmlParserInput **)SAlloc::M(5 * sizeof(xmlParserInput *));
		if(ctxt->inputTab == NULL) {
			xmlSAX2ErrMemory(ctxt, "xmlSAX2ExternalSubset");
			ctxt->input = oldinput;
			ctxt->inputNr = oldinputNr;
			ctxt->inputMax = oldinputMax;
			ctxt->inputTab = oldinputTab;
			ctxt->charset = oldcharset;
			ctxt->encoding = oldencoding;
			return;
		}
		ctxt->inputNr = 0;
		ctxt->inputMax = 5;
		ctxt->input = NULL;
		xmlPushInput(ctxt, input);
		/*
		 * On the fly encoding conversion if needed
		 */
		if(ctxt->input->length >= 4) {
			enc = xmlDetectCharEncoding(ctxt->input->cur, 4);
			xmlSwitchEncoding(ctxt, enc);
		}
		SETIFZ(input->filename, (char *)xmlCanonicPath(SystemID));
		input->line = 1;
		input->col = 1;
		input->base = ctxt->input->cur;
		input->cur = ctxt->input->cur;
		input->free = NULL;
		/*
		 * let's parse that entity knowing it's an external subset.
		 */
		xmlParseExternalSubset(ctxt, ExternalID, SystemID);
		/*
		 * Free up the external entities
		 */
		while(ctxt->inputNr > 1)
			xmlPopInput(ctxt);
		xmlFreeInputStream(ctxt->input);
		SAlloc::F(ctxt->inputTab);
		/*
		 * Restore the parsing context of the main entity
		 */
		ctxt->input = oldinput;
		ctxt->inputNr = oldinputNr;
		ctxt->inputMax = oldinputMax;
		ctxt->inputTab = oldinputTab;
		ctxt->charset = oldcharset;
		if(ctxt->encoding && ((ctxt->dict == NULL) || (!xmlDictOwns(ctxt->dict, ctxt->encoding))))
			SAlloc::F((xmlChar *)ctxt->encoding);
		ctxt->encoding = oldencoding;
		/* ctxt->wellFormed = oldwellFormed; */
	}
}

/**
 * xmlSAX2ResolveEntity:
 * @ctx: the user data (XML parser context)
 * @publicId: The public ID of the entity
 * @systemId: The system ID of the entity
 *
 * The entity loader, to control the loading of external entities,
 * the application can either:
 *  - override this xmlSAX2ResolveEntity() callback in the SAX block
 *  - or better use the xmlSetExternalEntityLoader() function to
 * set up it's own entity resolution routine
 *
 * Returns the xmlParserInputPtr if inlined or NULL for DOM behaviour.
 */
xmlParserInput * xmlSAX2ResolveEntity(void * ctx, const xmlChar * publicId, const xmlChar * systemId)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlParserInput * ret;
	xmlChar * URI;
	const char * base = NULL;
	if(!ctx)
		return 0;
	if(ctxt->input)
		base = ctxt->input->filename;
	if(base == NULL)
		base = ctxt->directory;
	URI = xmlBuildURI(systemId, (const xmlChar *)base);
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2ResolveEntity(%s, %s)\n", publicId, systemId);
#endif
	ret = xmlLoadExternalEntity((const char *)URI, (const char *)publicId, ctxt);
	SAlloc::F(URI);
	return ret;
}

/**
 * xmlSAX2GetEntity:
 * @ctx: the user data (XML parser context)
 * @name: The entity name
 *
 * Get an entity by name
 *
 * Returns the xmlEntityPtr if found.
 */
xmlEntity * xmlSAX2GetEntity(void * ctx, const xmlChar * name)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlEntity * ret = NULL;
	if(!ctx)
		return 0;
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2GetEntity(%s)\n", name);
#endif
	if(ctxt->inSubset == 0) {
		ret = xmlGetPredefinedEntity(name);
		if(ret)
			return ret;
	}
	if(ctxt->myDoc && (ctxt->myDoc->standalone == 1)) {
		if(ctxt->inSubset == 2) {
			ctxt->myDoc->standalone = 0;
			ret = xmlGetDocEntity(ctxt->myDoc, name);
			ctxt->myDoc->standalone = 1;
		}
		else {
			ret = xmlGetDocEntity(ctxt->myDoc, name);
			if(!ret) {
				ctxt->myDoc->standalone = 0;
				ret = xmlGetDocEntity(ctxt->myDoc, name);
				if(ret) {
					xmlFatalErrMsg(ctxt, XML_ERR_NOT_STANDALONE, "Entity(%s) document marked standalone but requires external subset\n", name, 0);
				}
				ctxt->myDoc->standalone = 1;
			}
		}
	}
	else {
		ret = xmlGetDocEntity(ctxt->myDoc, name);
	}
	if(ret && ((ctxt->validate) || (ctxt->replaceEntities)) && !ret->children && (ret->etype == XML_EXTERNAL_GENERAL_PARSED_ENTITY)) {
		int val;
		/*
		 * for validation purposes we really need to fetch and
		 * parse the external entity
		 */
		xmlNode * children;
		ulong  oldnbent = ctxt->nbentities;
		val = xmlParseCtxtExternalEntity(ctxt, ret->URI, ret->ExternalID, &children);
		if(val == 0) {
			xmlAddChildList((xmlNode *)ret, children);
		}
		else {
			xmlFatalErrMsg(ctxt, XML_ERR_ENTITY_PROCESSING, "Failure to process entity %s\n", name, 0);
			ctxt->validate = 0;
			return 0;
		}
		ret->owner = 1;
		if(ret->checked == 0) {
			ret->checked = (ctxt->nbentities - oldnbent + 1) * 2;
			if(ret->content && (xmlStrchr(ret->content, '<')))
				ret->checked |= 1;
		}
	}
	return ret;
}

/**
 * xmlSAX2GetParameterEntity:
 * @ctx: the user data (XML parser context)
 * @name: The entity name
 *
 * Get a parameter entity by name
 *
 * Returns the xmlEntityPtr if found.
 */
xmlEntity * xmlSAX2GetParameterEntity(void * ctx, const xmlChar * name)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlEntity * ret = 0;
	if(ctx) {
#ifdef DEBUG_SAX
		xmlGenericError(0, "SAX.xmlSAX2GetParameterEntity(%s)\n", name);
#endif
		ret = xmlGetParameterEntity(ctxt->myDoc, name);
	}
	return ret;
}

/**
 * xmlSAX2EntityDecl:
 * @ctx: the user data (XML parser context)
 * @name:  the entity name
 * @type:  the entity type
 * @publicId: The public ID of the entity
 * @systemId: The system ID of the entity
 * @content: the entity value (without processing).
 *
 * An entity definition has been parsed
 */
void xmlSAX2EntityDecl(void * ctx, const xmlChar * name, int type, const xmlChar * publicId, const xmlChar * systemId, xmlChar * content)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	if(ctx) {
	#ifdef DEBUG_SAX
		xmlGenericError(0, "SAX.xmlSAX2EntityDecl(%s, %d, %s, %s, %s)\n", name, type, publicId, systemId, content);
	#endif
		if(ctxt->inSubset == 1) {
			xmlEntity * ent = xmlAddDocEntity(ctxt->myDoc, name, type, publicId, systemId, content);
			if(!ent && (ctxt->pedantic))
				xmlWarnMsg(ctxt, XML_WAR_ENTITY_REDEFINED, "Entity(%s) already defined in the internal subset\n", name);
			if(ent && !ent->URI && systemId) {
				const char * base = NULL;
				if(ctxt->input)
					base = ctxt->input->filename;
				SETIFZ(base, ctxt->directory);
				xmlChar * p_uri = xmlBuildURI(systemId, (const xmlChar *)base);
				ent->URI = p_uri;
			}
		}
		else if(ctxt->inSubset == 2) {
			xmlEntity * ent = xmlAddDtdEntity(ctxt->myDoc, name, type, publicId, systemId, content);
			if(!ent && ctxt->pedantic && ctxt->sax && ctxt->sax->warning)
				ctxt->sax->warning(ctxt->userData, "Entity(%s) already defined in the external subset\n", name);
			if(ent && !ent->URI && systemId) {
				const char * base = ctxt->input ? ctxt->input->filename : 0;
				SETIFZ(base, ctxt->directory);
				xmlChar * p_uri = xmlBuildURI(systemId, (const xmlChar *)base);
				ent->URI = p_uri;
			}
		}
		else {
			xmlFatalErrMsg(ctxt, XML_ERR_ENTITY_PROCESSING, "SAX.xmlSAX2EntityDecl(%s) called while not in subset\n", name, 0);
		}
	}
}
/**
 * xmlSAX2AttributeDecl:
 * @ctx: the user data (XML parser context)
 * @elem:  the name of the element
 * @fullname:  the attribute name
 * @type:  the attribute type
 * @def:  the type of default value
 * @defaultValue: the attribute default value
 * @tree:  the tree of enumerated value set
 *
 * An attribute definition has been parsed
 */
void xmlSAX2AttributeDecl(void * ctx, const xmlChar * elem, const xmlChar * fullname, int type, int def, const xmlChar * defaultValue, xmlEnumeration * tree)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlAttribute * attr;
	xmlChar * name = NULL, * prefix = NULL;
	if(!ctxt || (ctxt->myDoc == NULL))
		return;
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2AttributeDecl(%s, %s, %d, %d, %s, ...)\n", elem, fullname, type, def, defaultValue);
#endif
	if(sstreq(fullname, reinterpret_cast<const xmlChar *>("xml:id")) && type != XML_ATTRIBUTE_ID) {
		/*
		 * Raise the error but keep the validity flag
		 */
		int tmp = ctxt->valid;
		xmlErrValid(ctxt, XML_DTD_XMLID_TYPE, "xml:id : attribute type should be ID\n", 0, 0);
		ctxt->valid = tmp;
	}
	/* @todo optimize name/prefix allocation */
	name = xmlSplitQName(ctxt, fullname, &prefix);
	ctxt->vctxt.valid = 1;
	if(ctxt->inSubset == 1)
		attr = xmlAddAttributeDecl(&ctxt->vctxt, ctxt->myDoc->intSubset, elem, name, prefix, (xmlAttributeType)type, (xmlAttributeDefault)def, defaultValue, tree);
	else if(ctxt->inSubset == 2)
		attr = xmlAddAttributeDecl(&ctxt->vctxt, ctxt->myDoc->extSubset, elem, name, prefix, (xmlAttributeType)type, (xmlAttributeDefault)def, defaultValue, tree);
	else {
		xmlFatalErrMsg(ctxt, XML_ERR_INTERNAL_ERROR, "SAX.xmlSAX2AttributeDecl(%s) called while not in subset\n", name, 0);
		xmlFreeEnumeration(tree);
		return;
	}
#ifdef LIBXML_VALID_ENABLED
	if(ctxt->vctxt.valid == 0)
		ctxt->valid = 0;
	if(attr && ctxt->validate && ctxt->wellFormed && ctxt->myDoc->intSubset)
		ctxt->valid &= xmlValidateAttributeDecl(&ctxt->vctxt, ctxt->myDoc, attr);
#endif /* LIBXML_VALID_ENABLED */
	SAlloc::F(prefix);
	SAlloc::F(name);
}

/**
 * xmlSAX2ElementDecl:
 * @ctx: the user data (XML parser context)
 * @name:  the element name
 * @type:  the element type
 * @content: the element value tree
 *
 * An element definition has been parsed
 */
void xmlSAX2ElementDecl(void * ctx, const xmlChar * name, int type, xmlElementContent * content)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlElement * elem = NULL;
	if(!ctxt || (ctxt->myDoc == NULL))
		return;
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2ElementDecl(%s, %d, ...)\n", name, type);
#endif
	if(ctxt->inSubset == 1)
		elem = xmlAddElementDecl(&ctxt->vctxt, ctxt->myDoc->intSubset, name, (xmlElementTypeVal)type, content);
	else if(ctxt->inSubset == 2)
		elem = xmlAddElementDecl(&ctxt->vctxt, ctxt->myDoc->extSubset, name, (xmlElementTypeVal)type, content);
	else {
		xmlFatalErrMsg(ctxt, XML_ERR_INTERNAL_ERROR, "SAX.xmlSAX2ElementDecl(%s) called while not in subset\n", name, 0);
		return;
	}
#ifdef LIBXML_VALID_ENABLED
	if(elem == NULL)
		ctxt->valid = 0;
	if(ctxt->validate && ctxt->wellFormed && ctxt->myDoc && ctxt->myDoc->intSubset)
		ctxt->valid &= xmlValidateElementDecl(&ctxt->vctxt, ctxt->myDoc, elem);
#endif /* LIBXML_VALID_ENABLED */
}

/**
 * xmlSAX2NotationDecl:
 * @ctx: the user data (XML parser context)
 * @name: The name of the notation
 * @publicId: The public ID of the entity
 * @systemId: The system ID of the entity
 *
 * What to do when a notation declaration has been parsed.
 */
void xmlSAX2NotationDecl(void * ctx, const xmlChar * name, const xmlChar * publicId, const xmlChar * systemId)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlNotation * nota = NULL;
	if(!ctxt || (ctxt->myDoc == NULL))
		return;
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2NotationDecl(%s, %s, %s)\n", name, publicId, systemId);
#endif
	if(!publicId && !systemId) {
		xmlFatalErrMsg(ctxt, XML_ERR_NOTATION_PROCESSING, "SAX.xmlSAX2NotationDecl(%s) externalID or PublicID missing\n", name, 0);
		return;
	}
	else if(ctxt->inSubset == 1)
		nota = xmlAddNotationDecl(&ctxt->vctxt, ctxt->myDoc->intSubset, name, publicId, systemId);
	else if(ctxt->inSubset == 2)
		nota = xmlAddNotationDecl(&ctxt->vctxt, ctxt->myDoc->extSubset, name, publicId, systemId);
	else {
		xmlFatalErrMsg(ctxt, XML_ERR_NOTATION_PROCESSING, "SAX.xmlSAX2NotationDecl(%s) called while not in subset\n", name, 0);
		return;
	}
#ifdef LIBXML_VALID_ENABLED
	if(!nota)
		ctxt->valid = 0;
	if((ctxt->validate) && (ctxt->wellFormed) && ctxt->myDoc->intSubset)
		ctxt->valid &= xmlValidateNotationDecl(&ctxt->vctxt, ctxt->myDoc, nota);
#endif /* LIBXML_VALID_ENABLED */
}

/**
 * xmlSAX2UnparsedEntityDecl:
 * @ctx: the user data (XML parser context)
 * @name: The name of the entity
 * @publicId: The public ID of the entity
 * @systemId: The system ID of the entity
 * @notationName: the name of the notation
 *
 * What to do when an unparsed entity declaration is parsed
 */
void xmlSAX2UnparsedEntityDecl(void * ctx, const xmlChar * name, const xmlChar * publicId, const xmlChar * systemId, const xmlChar * notationName)
{
	xmlEntity * ent;
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	if(!ctx) return;
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2UnparsedEntityDecl(%s, %s, %s, %s)\n", name, publicId, systemId, notationName);
#endif
	if(ctxt->inSubset == 1) {
		ent = xmlAddDocEntity(ctxt->myDoc, name, XML_EXTERNAL_GENERAL_UNPARSED_ENTITY, publicId, systemId, notationName);
		if(!ent && (ctxt->pedantic) && ctxt->sax && ctxt->sax->warning)
			ctxt->sax->warning(ctxt->userData, "Entity(%s) already defined in the internal subset\n", name);
		if(ent && !ent->URI && systemId) {
			xmlChar * URI;
			const char * base = ctxt->input ? ctxt->input->filename : 0;
			SETIFZ(base, ctxt->directory);
			URI = xmlBuildURI(systemId, (const xmlChar *)base);
			ent->URI = URI;
		}
	}
	else if(ctxt->inSubset == 2) {
		ent = xmlAddDtdEntity(ctxt->myDoc, name, XML_EXTERNAL_GENERAL_UNPARSED_ENTITY, publicId, systemId, notationName);
		if(!ent && (ctxt->pedantic) && ctxt->sax && ctxt->sax->warning)
			ctxt->sax->warning(ctxt->userData, "Entity(%s) already defined in the external subset\n", name);
		if(ent && !ent->URI && systemId) {
			xmlChar * URI;
			const char * base = ctxt->input ? ctxt->input->filename : 0;
			SETIFZ(base, ctxt->directory);
			URI = xmlBuildURI(systemId, (const xmlChar *)base);
			ent->URI = URI;
		}
	}
	else {
		xmlFatalErrMsg(ctxt, XML_ERR_INTERNAL_ERROR, "SAX.xmlSAX2UnparsedEntityDecl(%s) called while not in subset\n", name, 0);
	}
}

/**
 * xmlSAX2SetDocumentLocator:
 * @ctx: the user data (XML parser context)
 * @loc: A SAX Locator
 *
 * Receive the document locator at startup, actually xmlDefaultSAXLocator
 * Everything is available on the context, so this is useless in our case.
 */
void xmlSAX2SetDocumentLocator(void * ctx ATTRIBUTE_UNUSED, xmlSAXLocator * loc ATTRIBUTE_UNUSED)
{
	/* xmlParserCtxt * ctxt = (xmlParserCtxt *) ctx; */
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2SetDocumentLocator()\n");
#endif
}
/**
 * xmlSAX2StartDocument:
 * @ctx: the user data (XML parser context)
 *
 * called when the document start being processed.
 */
void xmlSAX2StartDocument(void * ctx)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	if(ctx) {
#ifdef DEBUG_SAX
		xmlGenericError(0, "SAX.xmlSAX2StartDocument()\n");
#endif
		if(ctxt->html) {
#ifdef LIBXML_HTML_ENABLED
			SETIFZ(ctxt->myDoc, htmlNewDocNoDtD(NULL, NULL));
			if(!ctxt->myDoc) {
				xmlSAX2ErrMemory(ctxt, "xmlSAX2StartDocument");
				return;
			}
			ctxt->myDoc->properties = XML_DOC_HTML;
			ctxt->myDoc->parseFlags = ctxt->options;
#else
			xmlGenericError(0, "libxml2 built without HTML support\n");
			ctxt->errNo = XML_ERR_INTERNAL_ERROR;
			ctxt->instate = XML_PARSER_EOF;
			ctxt->disableSAX = 1;
			return;
#endif
		}
		else {
			xmlDoc * doc = ctxt->myDoc = xmlNewDoc(ctxt->version);
			if(doc) {
				doc->properties = 0;
				if(ctxt->options & XML_PARSE_OLD10)
					doc->properties |= XML_DOC_OLD10;
				doc->parseFlags = ctxt->options;
				doc->encoding = sstrdup(ctxt->encoding);
				doc->standalone = ctxt->standalone;
				if(ctxt->dictNames) {
					doc->dict = ctxt->dict;
					xmlDictReference(doc->dict);
				}
			}
			else {
				xmlSAX2ErrMemory(ctxt, "xmlSAX2StartDocument");
				return;
			}
		}
		if(ctxt->myDoc && !ctxt->myDoc->URL && ctxt->input && ctxt->input->filename) {
			ctxt->myDoc->URL = xmlPathToURI((const xmlChar *)ctxt->input->filename);
			if(!ctxt->myDoc->URL)
				xmlSAX2ErrMemory(ctxt, "xmlSAX2StartDocument");
		}
	}
}
/**
 * xmlSAX2EndDocument:
 * @ctx: the user data (XML parser context)
 *
 * called when the document end has been detected.
 */
void xmlSAX2EndDocument(void * ctx)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2EndDocument()\n");
#endif
	if(!ctx) return;
#ifdef LIBXML_VALID_ENABLED
	if(ctxt->validate && ctxt->wellFormed && ctxt->myDoc && ctxt->myDoc->intSubset)
		ctxt->valid &= xmlValidateDocumentFinal(&ctxt->vctxt, ctxt->myDoc);
#endif /* LIBXML_VALID_ENABLED */
	/*
	 * Grab the encoding if it was added on-the-fly
	 */
	if(ctxt->encoding && ctxt->myDoc && (ctxt->myDoc->encoding == NULL)) {
		ctxt->myDoc->encoding = ctxt->encoding;
		ctxt->encoding = NULL;
	}
	if(ctxt->inputTab && (ctxt->inputNr > 0) && ctxt->inputTab[0] && ctxt->inputTab[0]->encoding && ctxt->myDoc && (ctxt->myDoc->encoding == NULL)) {
		ctxt->myDoc->encoding = sstrdup(ctxt->inputTab[0]->encoding);
	}
	if((ctxt->charset != XML_CHAR_ENCODING_NONE) && ctxt->myDoc && (ctxt->myDoc->charset == XML_CHAR_ENCODING_NONE)) {
		ctxt->myDoc->charset = ctxt->charset;
	}
}

#if defined(LIBXML_SAX1_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_WRITER_ENABLED) || defined(LIBXML_DOCB_ENABLED) || \
	defined(LIBXML_LEGACY_ENABLED)
/**
 * xmlSAX2AttributeInternal:
 * @ctx: the user data (XML parser context)
 * @fullname:  The attribute name, including namespace__ prefix
 * @value:  The attribute value
 * @prefix: the prefix on the element node
 *
 * Handle an attribute that has been read by the parser.
 * The default handling is to convert the attribute into an
 * DOM subtree and past it in a new xmlAttr element added to
 * the element.
 */
static void xmlSAX2AttributeInternal(void * ctx, const xmlChar * fullname, const xmlChar * value, const xmlChar * prefix ATTRIBUTE_UNUSED)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlAttr * ret;
	xmlChar * name;
	xmlChar * ns;
	xmlChar * nval;
	xmlNs * namespace__;
	if(ctxt->html) {
		name = sstrdup(fullname);
		ns = NULL;
		namespace__ = NULL;
	}
	else {
		/*
		 * Split the full name into a namespace__ prefix and the tag name
		 */
		name = xmlSplitQName(ctxt, fullname, &ns);
		if(name && (name[0] == 0)) {
			if(sstreq(ns, "xmlns")) {
				xmlNsErrMsg(ctxt, XML_ERR_NS_DECL_ERROR, "invalid namespace__ declaration '%s'\n", fullname, 0);
			}
			else {
				xmlNsWarnMsg(ctxt, XML_WAR_NS_COLUMN, "Avoid attribute ending with ':' like '%s'\n", fullname, 0);
			}
			ZFREE(ns);
			SAlloc::F(name);
			name = sstrdup(fullname);
		}
	}
	if(!name) {
		xmlSAX2ErrMemory(ctxt, "xmlSAX2StartElement");
		SAlloc::F(ns);
		return;
	}
#ifdef LIBXML_HTML_ENABLED
	if(ctxt->html && !value && htmlIsBooleanAttr(fullname)) {
		nval = sstrdup(fullname);
		value = (const xmlChar *)nval;
	}
	else
#endif
	{
#ifdef LIBXML_VALID_ENABLED
		/*
		 * Do the last stage of the attribute normalization
		 * Needed for HTML too:
		 * http://www.w3.org/TR/html4/types.html#h-6.2
		 */
		ctxt->vctxt.valid = 1;
		nval = xmlValidCtxtNormalizeAttributeValue(&ctxt->vctxt, ctxt->myDoc, ctxt->P_Node, fullname, value);
		if(ctxt->vctxt.valid != 1) {
			ctxt->valid = 0;
		}
		if(nval)
			value = nval;
#else
		nval = NULL;
#endif /* LIBXML_VALID_ENABLED */
	}

	/*
	 * Check whether it's a namespace__ definition
	 */
	if((!ctxt->html) && !ns && (name[0] == 'x') && (name[1] == 'm') && (name[2] == 'l') && (name[3] == 'n') && (name[4] == 's') && (name[5] == 0)) {
		xmlNs * nsret;
		xmlChar * val;
		if(!ctxt->replaceEntities) {
			ctxt->depth++;
			val = xmlStringDecodeEntities(ctxt, value, XML_SUBSTITUTE_REF, 0, 0, 0);
			ctxt->depth--;
			if(!val) {
				xmlSAX2ErrMemory(ctxt, "xmlSAX2StartElement");
				SAlloc::F(name);
				return;
			}
		}
		else {
			val = (xmlChar *)value;
		}
		if(val[0] != 0) {
			xmlURI * uri = xmlParseURI((const char *)val);
			if(!uri) {
				if(ctxt->sax && ctxt->sax->warning)
					ctxt->sax->warning(ctxt->userData, "xmlns: %s not a valid URI\n", val);
			}
			else {
				if(uri->scheme == NULL) {
					if(ctxt->sax && ctxt->sax->warning)
						ctxt->sax->warning(ctxt->userData, "xmlns: URI %s is not absolute\n", val);
				}
				xmlFreeURI(uri);
			}
		}
		/* a default namespace__ definition */
		nsret = xmlNewNs(ctxt->P_Node, val, 0);
#ifdef LIBXML_VALID_ENABLED
		/*
		 * Validate also for namespace__ decls, they are attributes from
		 * an XML-1.0 perspective
		 */
		if(nsret && ctxt->validate && ctxt->wellFormed && ctxt->myDoc && ctxt->myDoc->intSubset)
			ctxt->valid &= xmlValidateOneNamespace(&ctxt->vctxt, ctxt->myDoc, ctxt->P_Node, prefix, nsret, val);
#endif /* LIBXML_VALID_ENABLED */
		SAlloc::F(name);
		SAlloc::F(nval);
		if(val != value)
			SAlloc::F(val);
		return;
	}
	if((!ctxt->html) && ns && (ns[0] == 'x') && (ns[1] == 'm') && (ns[2] == 'l') && (ns[3] == 'n') && (ns[4] == 's') && (ns[5] == 0)) {
		xmlNs * nsret;
		xmlChar * val;
		if(!ctxt->replaceEntities) {
			ctxt->depth++;
			val = xmlStringDecodeEntities(ctxt, value, XML_SUBSTITUTE_REF, 0, 0, 0);
			ctxt->depth--;
			if(!val) {
				xmlSAX2ErrMemory(ctxt, "xmlSAX2StartElement");
				SAlloc::F(ns);
				SAlloc::F(name);
				return;
			}
		}
		else {
			val = (xmlChar *)value;
		}
		if(val[0] == 0) {
			xmlNsErrMsg(ctxt, XML_NS_ERR_EMPTY, "Empty namespace__ name for prefix %s\n", name, 0);
		}
		if((ctxt->pedantic != 0) && (val[0] != 0)) {
			xmlURI * uri = xmlParseURI((const char *)val);
			if(!uri) {
				xmlNsWarnMsg(ctxt, XML_WAR_NS_URI, "xmlns:%s: %s not a valid URI\n", name, value);
			}
			else {
				if(uri->scheme == NULL) {
					xmlNsWarnMsg(ctxt, XML_WAR_NS_URI_RELATIVE, "xmlns:%s: URI %s is not absolute\n", name, value);
				}
				xmlFreeURI(uri);
			}
		}
		/* a standard namespace__ definition */
		nsret = xmlNewNs(ctxt->P_Node, val, name);
		SAlloc::F(ns);
#ifdef LIBXML_VALID_ENABLED
		/*
		 * Validate also for namespace__ decls, they are attributes from
		 * an XML-1.0 perspective
		 */
		if(nsret && ctxt->validate && ctxt->wellFormed && ctxt->myDoc && ctxt->myDoc->intSubset)
			ctxt->valid &= xmlValidateOneNamespace(&ctxt->vctxt, ctxt->myDoc, ctxt->P_Node, prefix, nsret, value);
#endif /* LIBXML_VALID_ENABLED */
		SAlloc::F(name);
		SAlloc::F(nval);
		if(val != value)
			SAlloc::F(val);
		return;
	}
	if(ns) {
		namespace__ = xmlSearchNs(ctxt->myDoc, ctxt->P_Node, ns);
		if(namespace__ == NULL) {
			xmlNsErrMsg(ctxt, XML_NS_ERR_UNDEFINED_NAMESPACE, "Namespace prefix %s of attribute %s is not defined\n", ns, name);
		}
		else {
			xmlAttr * prop = ctxt->P_Node->properties;
			while(prop) {
				if(prop->ns) {
					if((sstreq(name, prop->name)) && ((namespace__ == prop->ns) || (sstreq(namespace__->href, prop->ns->href)))) {
						xmlNsErrMsg(ctxt, XML_ERR_ATTRIBUTE_REDEFINED, "Attribute %s in %s redefined\n", name, namespace__->href);
						ctxt->wellFormed = 0;
						if(ctxt->recovery == 0) 
							ctxt->disableSAX = 1;
						goto error;
					}
				}
				prop = prop->next;
			}
		}
	}
	else {
		namespace__ = NULL;
	}
	/* !!!!!! <a toto:arg="" xmlns:toto="http://toto.com"> */
	ret = xmlNewNsPropEatName(ctxt->P_Node, namespace__, name, 0);
	if(ret) {
		if((ctxt->replaceEntities == 0) && (!ctxt->html)) {
			xmlNode * tmp;
			ret->children = xmlStringGetNodeList(ctxt->myDoc, value);
			tmp = ret->children;
			while(tmp) {
				tmp->P_ParentNode = reinterpret_cast<xmlNode *>(ret);
				if(tmp->next == NULL)
					ret->last = tmp;
				tmp = tmp->next;
			}
		}
		else if(value) {
			ret->children = xmlNewDocText(ctxt->myDoc, value);
			ret->last = ret->children;
			if(ret->children)
				ret->children->P_ParentNode = reinterpret_cast<xmlNode *>(ret);
		}
	}

#ifdef LIBXML_VALID_ENABLED
	if((!ctxt->html) && ctxt->validate && ctxt->wellFormed && ctxt->myDoc && ctxt->myDoc->intSubset) {
		/*
		 * If we don't substitute entities, the validation should be
		 * done on a value with replaced entities anyway.
		 */
		if(!ctxt->replaceEntities) {
			xmlChar * val;
			ctxt->depth++;
			val = xmlStringDecodeEntities(ctxt, value, XML_SUBSTITUTE_REF, 0, 0, 0);
			ctxt->depth--;
			if(!val)
				ctxt->valid &= xmlValidateOneAttribute(&ctxt->vctxt, ctxt->myDoc, ctxt->P_Node, ret, value);
			else {
				/*
				 * Do the last stage of the attribute normalization
				 * It need to be done twice ... it's an extra burden related
				 * to the ability to keep xmlSAX2References in attributes
				 */
				xmlChar * nvalnorm = xmlValidNormalizeAttributeValue(ctxt->myDoc, ctxt->P_Node, fullname, val);
				if(nvalnorm) {
					SAlloc::F(val);
					val = nvalnorm;
				}
				ctxt->valid &= xmlValidateOneAttribute(&ctxt->vctxt, ctxt->myDoc, ctxt->P_Node, ret, val);
				SAlloc::F(val);
			}
		}
		else {
			ctxt->valid &= xmlValidateOneAttribute(&ctxt->vctxt, ctxt->myDoc, ctxt->P_Node, ret, value);
		}
	}
	else
#endif /* LIBXML_VALID_ENABLED */
	if(!(ctxt->loadsubset & XML_SKIP_IDS) && (((ctxt->replaceEntities == 0) && (ctxt->external != 2)) || (ctxt->replaceEntities && (ctxt->inSubset == 0)))) {
		/*
		 * when validating, the ID registration is done at the attribute
		 * validation level. Otherwise we have to do specific handling here.
		 */
		if(sstreq(fullname, "xml:id")) {
			/*
			 * Add the xml:id value
			 *
			 * Open issue: normalization of the value.
			 */
			if(xmlValidateNCName(value, 1) != 0) {
				xmlErrValid(ctxt, XML_DTD_XMLID_VALUE, "xml:id : attribute value %s is not an NCName\n", (const char *)value, 0);
			}
			xmlAddID(&ctxt->vctxt, ctxt->myDoc, value, ret);
		}
		else if(xmlIsID(ctxt->myDoc, ctxt->P_Node, ret))
			xmlAddID(&ctxt->vctxt, ctxt->myDoc, value, ret);
		else if(xmlIsRef(ctxt->myDoc, ctxt->P_Node, ret))
			xmlAddRef(&ctxt->vctxt, ctxt->myDoc, value, ret);
	}
error:
	SAlloc::F(nval);
	SAlloc::F(ns);
}

/*
 * xmlCheckDefaultedAttributes:
 *
 * Check defaulted attributes from the DTD
 */
static void xmlCheckDefaultedAttributes(xmlParserCtxt * ctxt, const xmlChar * name,
    const xmlChar * prefix, const xmlChar ** atts) {
	xmlElement * elemDecl;
	const xmlChar * att;
	int internal = 1;
	int i;

	elemDecl = xmlGetDtdQElementDesc(ctxt->myDoc->intSubset, name, prefix);
	if(elemDecl == NULL) {
		elemDecl = xmlGetDtdQElementDesc(ctxt->myDoc->extSubset, name, prefix);
		internal = 0;
	}
process_external_subset:
	if(elemDecl) {
		xmlAttribute * attr = elemDecl->attributes;
		/*
		 * Check against defaulted attributes from the external subset if the document is stamped as standalone
		 */
		if((ctxt->myDoc->standalone == 1) && ctxt->myDoc->extSubset && (ctxt->validate)) {
			while(attr) {
				if(attr->defaultValue && (xmlGetDtdQAttrDesc(ctxt->myDoc->extSubset, attr->elem, attr->name, attr->prefix) == attr) &&
				    (xmlGetDtdQAttrDesc(ctxt->myDoc->intSubset, attr->elem, attr->name, attr->prefix) == NULL)) {
					xmlChar * fulln;
					if(attr->prefix) {
						fulln = sstrdup(attr->prefix);
						fulln = xmlStrcat(fulln, reinterpret_cast<const xmlChar *>(":"));
						fulln = xmlStrcat(fulln, attr->name);
					}
					else {
						fulln = sstrdup(attr->name);
					}
					if(fulln == NULL) {
						xmlSAX2ErrMemory(ctxt, "xmlSAX2StartElement");
						break;
					}

					/*
					 * Check that the attribute is not declared in the
					 * serialization
					 */
					att = NULL;
					if(atts) {
						i = 0;
						att = atts[i];
						while(att) {
							if(sstreq(att, fulln))
								break;
							i += 2;
							att = atts[i];
						}
					}
					if(att == NULL) {
						xmlErrValid(ctxt, XML_DTD_STANDALONE_DEFAULTED, "standalone: attribute %s on %s defaulted from external subset\n", (const char *)fulln, (const char *)attr->elem);
					}
					SAlloc::F(fulln);
				}
				attr = attr->nexth;
			}
		}
		/*
		 * Actually insert defaulted values when needed
		 */
		attr = elemDecl->attributes;
		while(attr) {
			/*
			 * Make sure that attributes redefinition occuring in the
			 * internal subset are not overriden by definitions in the
			 * external subset.
			 */
			if(attr->defaultValue) {
				/*
				 * the element should be instantiated in the tree if:
				 *  - this is a namespace__ prefix
				 *  - the user required for completion in the tree
				 *  like XSLT
				 *  - there isn't already an attribute definition
				 *  in the internal subset overriding it.
				 */
				if((attr->prefix && (sstreq(attr->prefix, "xmlns"))) || (!attr->prefix && (sstreq(attr->name, "xmlns"))) || (ctxt->loadsubset & XML_COMPLETE_ATTRS)) {
					xmlAttribute * tst = xmlGetDtdQAttrDesc(ctxt->myDoc->intSubset, attr->elem, attr->name, attr->prefix);
					if((tst == attr) || (tst == NULL)) {
						xmlChar fn[50];
						xmlChar * fulln = xmlBuildQName(attr->name, attr->prefix, fn, 50);
						if(fulln == NULL) {
							xmlSAX2ErrMemory(ctxt, "xmlSAX2StartElement");
							return;
						}
						/*
						 * Check that the attribute is not declared in the
						 * serialization
						 */
						att = NULL;
						if(atts) {
							i = 0;
							att = atts[i];
							while(att) {
								if(sstreq(att, fulln))
									break;
								i += 2;
								att = atts[i];
							}
						}
						if(att == NULL) {
							xmlSAX2AttributeInternal(ctxt, fulln, attr->defaultValue, prefix);
						}
						if((fulln != fn) && (fulln != attr->name))
							SAlloc::F(fulln);
					}
				}
			}
			attr = attr->nexth;
		}
		if(internal == 1) {
			elemDecl = xmlGetDtdQElementDesc(ctxt->myDoc->extSubset, name, prefix);
			internal = 0;
			goto process_external_subset;
		}
	}
}

/**
 * xmlSAX2StartElement:
 * @ctx: the user data (XML parser context)
 * @fullname:  The element name, including namespace__ prefix
 * @atts:  An array of name/value attributes pairs, NULL terminated
 *
 * called when an opening tag has been processed.
 */
void xmlSAX2StartElement(void * ctx, const xmlChar * fullname, const xmlChar ** atts)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlNode * ret;
	xmlNode * parent;
	xmlNs * ns;
	xmlChar * name;
	xmlChar * prefix;
	const xmlChar * att;
	const xmlChar * value;
	int i;
	if(!ctx || (fullname == NULL) || (ctxt->myDoc == NULL)) 
		return;
	parent = ctxt->P_Node;
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2StartElement(%s)\n", fullname);
#endif
	/*
	 * First check on validity:
	 */
	if(ctxt->validate && !ctxt->myDoc->extSubset && (!ctxt->myDoc->intSubset ||
	    (!ctxt->myDoc->intSubset->notations && !ctxt->myDoc->intSubset->elements &&
		!ctxt->myDoc->intSubset->attributes && !ctxt->myDoc->intSubset->entities))) {
		xmlErrValid(ctxt, XML_ERR_NO_DTD, "Validation failed: no DTD found !", 0, 0);
		ctxt->validate = 0;
	}
	/*
	 * Split the full name into a namespace__ prefix and the tag name
	 */
	name = xmlSplitQName(ctxt, fullname, &prefix);
	/*
	 * Note : the namespace__ resolution is deferred until the end of the
	 *   attributes parsing, since local namespace__ can be defined as
	 *   an attribute at this level.
	 */
	ret = xmlNewDocNodeEatName(ctxt->myDoc, NULL, name, 0);
	if(!ret) {
		SAlloc::F(prefix);
		xmlSAX2ErrMemory(ctxt, "xmlSAX2StartElement");
		return;
	}
	if(ctxt->myDoc->children == NULL) {
#ifdef DEBUG_SAX_TREE
		xmlGenericError(0, "Setting %s as root\n", name);
#endif
		xmlAddChild((xmlNode *)ctxt->myDoc, ret);
	}
	else if(parent == NULL) {
		parent = ctxt->myDoc->children;
	}
	ctxt->nodemem = -1;
	if(ctxt->linenumbers) {
		if(ctxt->input) {
			ret->line = (ctxt->input->line < 65535) ? static_cast<short>(ctxt->input->line) : 65535;
		}
	}
	/*
	 * We are parsing a new node.
	 */
#ifdef DEBUG_SAX_TREE
	xmlGenericError(0, "pushing(%s)\n", name);
#endif
	nodePush(ctxt, ret);
	/*
	 * Link the child element
	 */
	if(parent) {
		if(parent->type == XML_ELEMENT_NODE) {
#ifdef DEBUG_SAX_TREE
			xmlGenericError(0, "adding child %s to %s\n", name, parent->name);
#endif
			xmlAddChild(parent, ret);
		}
		else {
#ifdef DEBUG_SAX_TREE
			xmlGenericError(0, "adding sibling %s to ", name);
			xmlDebugDumpOneNode(stderr, parent, 0);
#endif
			xmlAddSibling(parent, ret);
		}
	}
	/*
	 * Insert all the defaulted attributes from the DTD especially namespaces
	 */
	if((!ctxt->html) && (ctxt->myDoc->intSubset || ctxt->myDoc->extSubset)) {
		xmlCheckDefaultedAttributes(ctxt, name, prefix, atts);
	}
	/*
	 * process all the attributes whose name start with "xmlns"
	 */
	if(atts) {
		i = 0;
		att = atts[i++];
		value = atts[i++];
		if(!ctxt->html) {
			while(att && value) {
				if((att[0] == 'x') && (att[1] == 'm') && (att[2] == 'l') && (att[3] == 'n') && (att[4] == 's'))
					xmlSAX2AttributeInternal(ctxt, att, value, prefix);
				att = atts[i++];
				value = atts[i++];
			}
		}
	}
	/*
	 * Search the namespace__, note that since the attributes have been
	 * processed, the local namespaces are available.
	 */
	ns = xmlSearchNs(ctxt->myDoc, ret, prefix);
	if(!ns && parent)
		ns = xmlSearchNs(ctxt->myDoc, parent, prefix);
	if(prefix && !ns) {
		ns = xmlNewNs(ret, NULL, prefix);
		xmlNsWarnMsg(ctxt, XML_NS_ERR_UNDEFINED_NAMESPACE, "Namespace prefix %s is not defined\n", prefix, 0);
	}
	/*
	 * set the namespace__ node, making sure that if the default namspace
	 * is unbound on a parent we simply kee it NULL
	 */
	if(ns && ns->href && ((ns->href[0] != 0) || ns->prefix))
		xmlSetNs(ret, ns);
	/*
	 * process all the other attributes
	 */
	if(atts) {
		i = 0;
		att = atts[i++];
		value = atts[i++];
		if(ctxt->html) {
			while(att) {
				xmlSAX2AttributeInternal(ctxt, att, value, 0);
				att = atts[i++];
				value = atts[i++];
			}
		}
		else {
			while(att && value) {
				if((att[0] != 'x') || (att[1] != 'm') || (att[2] != 'l') || (att[3] != 'n') || (att[4] != 's'))
					xmlSAX2AttributeInternal(ctxt, att, value, 0);
				/*
				 * Next ones
				 */
				att = atts[i++];
				value = atts[i++];
			}
		}
	}

#ifdef LIBXML_VALID_ENABLED
	/*
	 * If it's the Document root, finish the DTD validation and
	 * check the document root element for validity
	 */
	if((ctxt->validate) && (ctxt->vctxt.finishDtd == XML_CTXT_FINISH_DTD_0)) {
		int chk = xmlValidateDtdFinal(&ctxt->vctxt, ctxt->myDoc);
		if(chk <= 0)
			ctxt->valid = 0;
		if(chk < 0)
			ctxt->wellFormed = 0;
		ctxt->valid &= xmlValidateRoot(&ctxt->vctxt, ctxt->myDoc);
		ctxt->vctxt.finishDtd = XML_CTXT_FINISH_DTD_1;
	}
#endif /* LIBXML_VALID_ENABLED */
	SAlloc::F(prefix);
}

/**
 * xmlSAX2EndElement:
 * @ctx: the user data (XML parser context)
 * @name:  The element name
 *
 * called when the end of an element has been detected.
 */
void xmlSAX2EndElement(void * ctx, const xmlChar * name ATTRIBUTE_UNUSED)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	if(ctx) {
		xmlNode * cur = ctxt->P_Node;
#ifdef DEBUG_SAX
		if(!name)
			xmlGenericError(0, "SAX.xmlSAX2EndElement(NULL)\n");
		else
			xmlGenericError(0, "SAX.xmlSAX2EndElement(%s)\n", name);
#endif
		/* Capture end position and add node */
		if(cur && ctxt->record_info) {
			ctxt->nodeInfo->end_pos = ctxt->input->cur - ctxt->input->base;
			ctxt->nodeInfo->end_line = ctxt->input->line;
			ctxt->nodeInfo->P_Node = cur;
			xmlParserAddNodeInfo(ctxt, ctxt->nodeInfo);
		}
		ctxt->nodemem = -1;
#ifdef LIBXML_VALID_ENABLED
		if(ctxt->validate && ctxt->wellFormed && ctxt->myDoc && ctxt->myDoc->intSubset)
			ctxt->valid &= xmlValidateOneElement(&ctxt->vctxt, ctxt->myDoc, cur);
#endif /* LIBXML_VALID_ENABLED */
		/*
		 * end of parsing of this node.
		 */
#ifdef DEBUG_SAX_TREE
		xmlGenericError(0, "popping(%s)\n", cur->name);
#endif
		nodePop(ctxt);
	}
}

#endif /* LIBXML_SAX1_ENABLED || LIBXML_HTML_ENABLED || LIBXML_LEGACY_ENABLED */

/*
 * xmlSAX2TextNode:
 * @ctxt:  the parser context
 * @str:  the input string
 * @len: the string length
 *
 * Callback for a text node
 *
 * Returns the newly allocated string or NULL if not needed or error
 */
static xmlNode * xmlSAX2TextNode(xmlParserCtxt * ctxt, const xmlChar * str, int len)
{
	xmlNode * ret;
	const xmlChar * intern = NULL;
	/*
	 * Allocate
	 */
	if(ctxt->freeElems) {
		ret = ctxt->freeElems;
		ctxt->freeElems = ret->next;
		ctxt->freeElemsNr--;
	}
	else {
		ret = static_cast<xmlNode *>(SAlloc::M(sizeof(xmlNode)));
	}
	if(!ret) {
		xmlErrMemory(ctxt, "xmlSAX2Characters");
		return 0;
	}
	memzero(ret, sizeof(xmlNode));
	/*
	 * intern the formatting blanks found between tags, or the
	 * very short strings
	 */
	if(ctxt->dictNames) {
		xmlChar cur = str[len];
		if((len < (int)(2 * sizeof(void *))) && (ctxt->options & XML_PARSE_COMPACT)) {
			/* store the string in the node overriding properties and nsDef */
			xmlChar * tmp = (xmlChar *)&(ret->properties);
			memcpy(tmp, str, len);
			tmp[len] = 0;
			intern = tmp;
		}
		else if((len <= 3) && ((cur == '"') || (cur == '\'') || ((cur == '<') && (str[len + 1] != '!')))) {
			intern = xmlDictLookup(ctxt->dict, str, len);
		}
		else if(IS_BLANK_CH(*str) && (len < 60) && (cur == '<') && (str[len + 1] != '!')) {
			for(int i = 1; i < len; i++) {
				if(!IS_BLANK_CH(str[i])) 
					goto skip;
			}
			intern = xmlDictLookup(ctxt->dict, str, len);
		}
	}
skip:
	ret->type = XML_TEXT_NODE;
	ret->name = xmlStringText;
	if(intern == NULL) {
		ret->content = xmlStrndup(str, len);
		if(ret->content == NULL) {
			xmlSAX2ErrMemory(ctxt, "xmlSAX2TextNode");
			SAlloc::F(ret);
			return 0;
		}
	}
	else
		ret->content = (xmlChar *)intern;
	if(ctxt->linenumbers) {
		if(ctxt->input) {
			if(ctxt->input->line < 65535)
				ret->line = static_cast<short>(ctxt->input->line);
			else {
				ret->line = 65535;
				if(ctxt->options & XML_PARSE_BIG_LINES)
					ret->psvi = (void *)(long)ctxt->input->line;
			}
		}
	}
	if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
		xmlRegisterNodeDefaultValue(ret);
	return ret;
}

#ifdef LIBXML_VALID_ENABLED
/*
 * xmlSAX2DecodeAttrEntities:
 * @ctxt:  the parser context
 * @str:  the input string
 * @len: the string length
 *
 * Remove the entities from an attribute value
 *
 * Returns the newly allocated string or NULL if not needed or error
 */
static xmlChar * xmlSAX2DecodeAttrEntities(xmlParserCtxt * ctxt, const xmlChar * str, const xmlChar * end) 
{
	xmlChar * ret;
	const xmlChar * in = str;
	while(in < end)
		if(*in++ == '&')
			goto decode;
	return 0;
decode:
	ctxt->depth++;
	ret = xmlStringLenDecodeEntities(ctxt, str, end - str, XML_SUBSTITUTE_REF, 0, 0, 0);
	ctxt->depth--;
	return ret;
}

#endif /* LIBXML_VALID_ENABLED */

/**
 * xmlSAX2AttributeNs:
 * @ctx: the user data (XML parser context)
 * @localname:  the local name of the attribute
 * @prefix:  the attribute namespace__ prefix if available
 * @URI:  the attribute namespace__ name if available
 * @value:  Start of the attribute value
 * @valueend: end of the attribute value
 *
 * Handle an attribute that has been read by the parser.
 * The default handling is to convert the attribute into an
 * DOM subtree and past it in a new xmlAttr element added to
 * the element.
 */
static void xmlSAX2AttributeNs(xmlParserCtxt * ctxt, const xmlChar * localname, const xmlChar * prefix, const xmlChar * value, const xmlChar * valueend)
{
	xmlAttr * ret;
	xmlNs * namespace__ = NULL;
	xmlChar * dup = NULL;
	/*
	 * Note: if prefix == NULL, the attribute is not in the default namespace__
	 */
	if(prefix)
		namespace__ = xmlSearchNs(ctxt->myDoc, ctxt->P_Node, prefix);
	/*
	 * allocate the node
	 */
	if(ctxt->freeAttrs) {
		ret = ctxt->freeAttrs;
		ctxt->freeAttrs = ret->next;
		ctxt->freeAttrsNr--;
		memzero(ret, sizeof(xmlAttr));
		ret->type = XML_ATTRIBUTE_NODE;
		ret->parent = ctxt->P_Node;
		ret->doc = ctxt->myDoc;
		ret->ns = namespace__;
		ret->name = ctxt->dictNames ? localname : sstrdup(localname);
		/* link at the end to preserv order, TODO speed up with a last */
		if(ctxt->P_Node->properties == NULL) {
			ctxt->P_Node->properties = ret;
		}
		else {
			xmlAttr * prev = ctxt->P_Node->properties;
			while(prev->next) 
				prev = prev->next;
			prev->next = ret;
			ret->prev = prev;
		}
		if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
			xmlRegisterNodeDefaultValue((xmlNode *)ret);
	}
	else {
		if(ctxt->dictNames)
			ret = xmlNewNsPropEatName(ctxt->P_Node, namespace__, (xmlChar *)localname, 0);
		else
			ret = xmlNewNsProp(ctxt->P_Node, namespace__, localname, 0);
		if(!ret) {
			xmlErrMemory(ctxt, "xmlSAX2AttributeNs");
			return;
		}
	}
	if((ctxt->replaceEntities == 0) && (!ctxt->html)) {
		xmlNode * tmp;
		/*
		 * We know that if there is an entity reference, then
		 * the string has been dup'ed and terminates with 0
		 * otherwise with ' or "
		 */
		if(*valueend != 0) {
			tmp = xmlSAX2TextNode(ctxt, value, valueend - value);
			ret->children = tmp;
			ret->last = tmp;
			if(tmp) {
				tmp->doc = ret->doc;
				tmp->P_ParentNode = reinterpret_cast<xmlNode *>(ret);
			}
		}
		else {
			ret->children = xmlStringLenGetNodeList(ctxt->myDoc, value, valueend - value);
			tmp = ret->children;
			while(tmp) {
				tmp->doc = ret->doc;
				tmp->P_ParentNode = reinterpret_cast<xmlNode *>(ret);
				if(tmp->next == NULL)
					ret->last = tmp;
				tmp = tmp->next;
			}
		}
	}
	else if(value) {
		xmlNode * tmp = xmlSAX2TextNode(ctxt, value, valueend - value);
		ret->children = tmp;
		ret->last = tmp;
		if(tmp) {
			tmp->doc = ret->doc;
			tmp->P_ParentNode = reinterpret_cast<xmlNode *>(ret);
		}
	}
#ifdef LIBXML_VALID_ENABLED
	if((!ctxt->html) && ctxt->validate && ctxt->wellFormed && ctxt->myDoc && ctxt->myDoc->intSubset) {
		/*
		 * If we don't substitute entities, the validation should be
		 * done on a value with replaced entities anyway.
		 */
		if(!ctxt->replaceEntities) {
			dup = xmlSAX2DecodeAttrEntities(ctxt, value, valueend);
			if(dup == NULL) {
				if(*valueend == 0) {
					ctxt->valid &= xmlValidateOneAttribute(&ctxt->vctxt, ctxt->myDoc, ctxt->P_Node, ret, value);
				}
				else {
					/*
					 * That should already be normalized.
					 * cheaper to finally allocate here than duplicate
					 * entry points in the full validation code
					 */
					dup = xmlStrndup(value, valueend - value);
					ctxt->valid &= xmlValidateOneAttribute(&ctxt->vctxt, ctxt->myDoc, ctxt->P_Node, ret, dup);
				}
			}
			else {
				/*
				 * dup now contains a string of the flattened attribute
				 * content with entities substitued. Check if we need to
				 * apply an extra layer of normalization.
				 * It need to be done twice ... it's an extra burden related
				 * to the ability to keep references in attributes
				 */
				if(ctxt->attsSpecial) {
					xmlChar * nvalnorm;
					xmlChar fn[50];
					xmlChar * fullname = xmlBuildQName(localname, prefix, fn, 50);
					if(fullname) {
						ctxt->vctxt.valid = 1;
						nvalnorm = xmlValidCtxtNormalizeAttributeValue(&ctxt->vctxt, ctxt->myDoc, ctxt->P_Node, fullname, dup);
						if(ctxt->vctxt.valid != 1)
							ctxt->valid = 0;
						if((fullname != fn) && (fullname != localname))
							SAlloc::F(fullname);
						if(nvalnorm) {
							SAlloc::F(dup);
							dup = nvalnorm;
						}
					}
				}
				ctxt->valid &= xmlValidateOneAttribute(&ctxt->vctxt, ctxt->myDoc, ctxt->P_Node, ret, dup);
			}
		}
		else {
			/*
			 * if entities already have been substitued, then
			 * the attribute as passed is already normalized
			 */
			dup = xmlStrndup(value, valueend - value);
			ctxt->valid &= xmlValidateOneAttribute(&ctxt->vctxt, ctxt->myDoc, ctxt->P_Node, ret, dup);
		}
	}
	else
#endif /* LIBXML_VALID_ENABLED */
	if(((ctxt->loadsubset & XML_SKIP_IDS) == 0) && (((ctxt->replaceEntities == 0) && (ctxt->external != 2)) || ((ctxt->replaceEntities != 0) && (ctxt->inSubset == 0)))) {
		/*
		 * when validating, the ID registration is done at the attribute
		 * validation level. Otherwise we have to do specific handling here.
		 */
		if((prefix == ctxt->str_xml) && (localname[0] == 'i') && (localname[1] == 'd') && (localname[2] == 0)) {
			/*
			 * Add the xml:id value
			 *
			 * Open issue: normalization of the value.
			 */
			SETIFZ(dup, xmlStrndup(value, valueend - value));
#if defined(LIBXML_SAX1_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_WRITER_ENABLED) || defined(LIBXML_DOCB_ENABLED) || \
			defined(LIBXML_LEGACY_ENABLED)
#ifdef LIBXML_VALID_ENABLED
			if(xmlValidateNCName(dup, 1) != 0) {
				xmlErrValid(ctxt, XML_DTD_XMLID_VALUE, "xml:id : attribute value %s is not an NCName\n", (const char *)dup, 0);
			}
#endif
#endif
			xmlAddID(&ctxt->vctxt, ctxt->myDoc, dup, ret);
		}
		else if(xmlIsID(ctxt->myDoc, ctxt->P_Node, ret)) {
			/* might be worth duplicate entry points and not copy */
			SETIFZ(dup, xmlStrndup(value, valueend - value));
			xmlAddID(&ctxt->vctxt, ctxt->myDoc, dup, ret);
		}
		else if(xmlIsRef(ctxt->myDoc, ctxt->P_Node, ret)) {
			SETIFZ(dup, xmlStrndup(value, valueend - value));
			xmlAddRef(&ctxt->vctxt, ctxt->myDoc, dup, ret);
		}
	}
	SAlloc::F(dup);
}

/**
 * xmlSAX2StartElementNs:
 * @ctx:  the user data (XML parser context)
 * @localname:  the local name of the element
 * @prefix:  the element namespace__ prefix if available
 * @URI:  the element namespace__ name if available
 * @nb_namespaces:  number of namespace__ definitions on that node
 * @namespaces:  pointer to the array of prefix/URI pairs namespace__ definitions
 * @nb_attributes:  the number of attributes on that node
 * @nb_defaulted:  the number of defaulted attributes.
 * @attributes:  pointer to the array of (localname/prefix/URI/value/end)
 *          attribute values.
 *
 * SAX2 callback when an element start has been detected by the parser.
 * It provides the namespace__ informations for the element, as well as
 * the new namespace__ declarations on the element.
 */
void xmlSAX2StartElementNs(void * ctx, const xmlChar * localname, const xmlChar * prefix, const xmlChar * URI,
    int nb_namespaces, const xmlChar ** namespaces, int nb_attributes, int nb_defaulted, const xmlChar ** attributes)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlNode * ret;
	xmlNode * parent;
	xmlNs * last = NULL;
	xmlNs * ns;
	const xmlChar * uri, * pref;
	xmlChar * lname = NULL;
	int i, j;
	if(!ctx) 
		return;
	parent = ctxt->P_Node;
	/*
	 * First check on validity:
	 */
	if(ctxt->validate && !ctxt->myDoc->extSubset && (!ctxt->myDoc->intSubset || (!ctxt->myDoc->intSubset->notations && 
		!ctxt->myDoc->intSubset->elements && !ctxt->myDoc->intSubset->attributes && !ctxt->myDoc->intSubset->entities))) {
		xmlErrValid(ctxt, XML_DTD_NO_DTD, "Validation failed: no DTD found !", 0, 0);
		ctxt->validate = 0;
	}
	/*
	 * Take care of the rare case of an undefined namespace__ prefix
	 */
	if(prefix && !URI) {
		if(ctxt->dictNames) {
			const xmlChar * fullname = xmlDictQLookup(ctxt->dict, prefix, localname);
			if(fullname)
				localname = fullname;
		}
		else {
			lname = xmlBuildQName(localname, prefix, NULL, 0);
		}
	}
	/*
	 * allocate the node
	 */
	if(ctxt->freeElems) {
		ret = ctxt->freeElems;
		ctxt->freeElems = ret->next;
		ctxt->freeElemsNr--;
		memzero(ret, sizeof(xmlNode));
		ret->type = XML_ELEMENT_NODE;
		if(ctxt->dictNames)
			ret->name = localname;
		else {
			ret->name = NZOR(lname, sstrdup(localname));
			if(ret->name == NULL) {
				xmlSAX2ErrMemory(ctxt, "xmlSAX2StartElementNs");
				return;
			}
		}
		if((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
			xmlRegisterNodeDefaultValue(ret);
	}
	else {
		if(ctxt->dictNames)
			ret = xmlNewDocNodeEatName(ctxt->myDoc, NULL, (xmlChar *)localname, 0);
		else if(lname == NULL)
			ret = xmlNewDocNode(ctxt->myDoc, NULL, localname, 0);
		else
			ret = xmlNewDocNodeEatName(ctxt->myDoc, NULL, (xmlChar *)lname, 0);
		if(!ret) {
			xmlSAX2ErrMemory(ctxt, "xmlSAX2StartElementNs");
			return;
		}
	}
	if(ctxt->linenumbers) {
		if(ctxt->input) {
			ret->line = (ctxt->input->line < 65535) ? static_cast<short>(ctxt->input->line) : 65535;
		}
	}
	if(parent == NULL) {
		xmlAddChild((xmlNode *)ctxt->myDoc, (xmlNode *)ret);
	}
	/*
	 * Build the namespace__ list
	 */
	for(i = 0, j = 0; j < nb_namespaces; j++) {
		pref = namespaces[i++];
		uri = namespaces[i++];
		ns = xmlNewNs(NULL, uri, pref);
		if(ns) {
			if(last == NULL) {
				ret->nsDef = last = ns;
			}
			else {
				last->next = ns;
				last = ns;
			}
			if(URI && (prefix == pref))
				ret->ns = ns;
		}
		else {
			/*
			 * any out of memory error would already have been raised
			 * but we can't be garanteed it's the actual error due to the
			 * API, best is to skip in this case
			 */
			continue;
		}
#ifdef LIBXML_VALID_ENABLED
		if(!ctxt->html && ctxt->validate && ctxt->wellFormed && ctxt->myDoc && ctxt->myDoc->intSubset) {
			ctxt->valid &= xmlValidateOneNamespace(&ctxt->vctxt, ctxt->myDoc, ret, prefix, ns, uri);
		}
#endif /* LIBXML_VALID_ENABLED */
	}
	ctxt->nodemem = -1;
	/*
	 * We are parsing a new node.
	 */
	nodePush(ctxt, ret);
	/*
	 * Link the child element
	 */
	if(parent) {
		if(parent->type == XML_ELEMENT_NODE) {
			xmlAddChild(parent, ret);
		}
		else {
			xmlAddSibling(parent, ret);
		}
	}
	/*
	 * Insert the defaulted attributes from the DTD only if requested:
	 */
	if(nb_defaulted && ((ctxt->loadsubset & XML_COMPLETE_ATTRS) == 0))
		nb_attributes -= nb_defaulted;
	/*
	 * Search the namespace__ if it wasn't already found
	 * Note that, if prefix is NULL, this searches for the default Ns
	 */
	if(URI && !ret->ns) {
		ret->ns = xmlSearchNs(ctxt->myDoc, parent, prefix);
		if(!ret->ns && sstreq(prefix, reinterpret_cast<const xmlChar *>("xml"))) {
			ret->ns = xmlSearchNs(ctxt->myDoc, ret, prefix);
		}
		if(!ret->ns) {
			ns = xmlNewNs(ret, NULL, prefix);
			if(ns == NULL) {
				xmlSAX2ErrMemory(ctxt, "xmlSAX2StartElementNs");
				return;
			}
			if(prefix)
				xmlNsWarnMsg(ctxt, XML_NS_ERR_UNDEFINED_NAMESPACE, "Namespace prefix %s was not found\n", prefix, 0);
			else
				xmlNsWarnMsg(ctxt, XML_NS_ERR_UNDEFINED_NAMESPACE, "Namespace default prefix was not found\n", 0, 0);
		}
	}
	/*
	 * process all the other attributes
	 */
	if(nb_attributes > 0) {
		for(j = 0, i = 0; i < nb_attributes; i++, j += 5) {
			/*
			 * Handle the rare case of an undefined atribute prefix
			 */
			if(attributes[j+1] && (attributes[j+2] == NULL)) {
				if(ctxt->dictNames) {
					const xmlChar * fullname = xmlDictQLookup(ctxt->dict, attributes[j+1], attributes[j]);
					if(fullname) {
						xmlSAX2AttributeNs(ctxt, fullname, NULL, attributes[j+3], attributes[j+4]);
						continue;
					}
				}
				else {
					lname = xmlBuildQName(attributes[j], attributes[j+1], NULL, 0);
					if(lname) {
						xmlSAX2AttributeNs(ctxt, lname, NULL, attributes[j+3], attributes[j+4]);
						SAlloc::F(lname);
						continue;
					}
				}
			}
			xmlSAX2AttributeNs(ctxt, attributes[j], attributes[j+1], attributes[j+3], attributes[j+4]);
		}
	}

#ifdef LIBXML_VALID_ENABLED
	/*
	 * If it's the Document root, finish the DTD validation and
	 * check the document root element for validity
	 */
	if((ctxt->validate) && (ctxt->vctxt.finishDtd == XML_CTXT_FINISH_DTD_0)) {
		int chk = xmlValidateDtdFinal(&ctxt->vctxt, ctxt->myDoc);
		if(chk <= 0)
			ctxt->valid = 0;
		if(chk < 0)
			ctxt->wellFormed = 0;
		ctxt->valid &= xmlValidateRoot(&ctxt->vctxt, ctxt->myDoc);
		ctxt->vctxt.finishDtd = XML_CTXT_FINISH_DTD_1;
	}
#endif /* LIBXML_VALID_ENABLED */
}

/**
 * xmlSAX2EndElementNs:
 * @ctx:  the user data (XML parser context)
 * @localname:  the local name of the element
 * @prefix:  the element namespace__ prefix if available
 * @URI:  the element namespace__ name if available
 *
 * SAX2 callback when an element end has been detected by the parser.
 * It provides the namespace__ informations for the element.
 */
void xmlSAX2EndElementNs(void * ctx, const xmlChar * localname ATTRIBUTE_UNUSED,
    const xmlChar * prefix ATTRIBUTE_UNUSED, const xmlChar * URI ATTRIBUTE_UNUSED)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlParserNodeInfo node_info;
	if(ctx) {
		xmlNode * cur = ctxt->P_Node;
		/* Capture end position and add node */
		if((ctxt->record_info) && cur) {
			node_info.end_pos = ctxt->input->cur - ctxt->input->base;
			node_info.end_line = ctxt->input->line;
			node_info.P_Node = cur;
			xmlParserAddNodeInfo(ctxt, &node_info);
		}
		ctxt->nodemem = -1;
#ifdef LIBXML_VALID_ENABLED
		if(ctxt->validate && ctxt->wellFormed && ctxt->myDoc && ctxt->myDoc->intSubset)
			ctxt->valid &= xmlValidateOneElement(&ctxt->vctxt, ctxt->myDoc, cur);
#endif /* LIBXML_VALID_ENABLED */
		/*
		 * end of parsing of this node.
		 */
		nodePop(ctxt);
	}
}
/**
 * xmlSAX2Reference:
 * @ctx: the user data (XML parser context)
 * @name:  The entity name
 *
 * called when an entity xmlSAX2Reference is detected.
 */
void xmlSAX2Reference(void * ctx, const xmlChar * name)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlNode * ret;
	if(!ctx)
		return;
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2Reference(%s)\n", name);
#endif
	if(name[0] == '#')
		ret = xmlNewCharRef(ctxt->myDoc, name);
	else
		ret = xmlNewReference(ctxt->myDoc, name);
#ifdef DEBUG_SAX_TREE
	xmlGenericError(0, "add xmlSAX2Reference %s to %s \n", name, ctxt->node->name);
#endif
	if(xmlAddChild(ctxt->P_Node, ret) == NULL) {
		xmlFreeNode(ret);
	}
}

/**
 * xmlSAX2Characters:
 * @ctx: the user data (XML parser context)
 * @ch:  a xmlChar string
 * @len: the number of xmlChar
 *
 * receiving some chars from the parser.
 */
void xmlSAX2Characters(void * ctx, const xmlChar * ch, int len)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlNode * lastChild;
	if(!ctx)
		return;
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2Characters(%.30s, %d)\n", ch, len);
#endif
	/*
	 * Handle the data if any. If there is no child
	 * add it as content, otherwise if the last child is text,
	 * concatenate it, else create a new node of type text.
	 */
	if(ctxt->P_Node == NULL) {
#ifdef DEBUG_SAX_TREE
		xmlGenericError(0, "add chars: ctxt->node == NULL !\n");
#endif
		return;
	}
	lastChild = ctxt->P_Node->last;
#ifdef DEBUG_SAX_TREE
	xmlGenericError(0, "add chars to %s \n", ctxt->node->name);
#endif
	/*
	 * Here we needed an accelerator mechanism in case of very large
	 * elements. Use an attribute in the structure !!!
	 */
	if(lastChild == NULL) {
		lastChild = xmlSAX2TextNode(ctxt, ch, len);
		if(lastChild) {
			ctxt->P_Node->children = lastChild;
			ctxt->P_Node->last = lastChild;
			lastChild->P_ParentNode = ctxt->P_Node;
			lastChild->doc = ctxt->P_Node->doc;
			ctxt->nodelen = len;
			ctxt->nodemem = len + 1;
		}
		else {
			xmlSAX2ErrMemory(ctxt, "xmlSAX2Characters");
			return;
		}
	}
	else {
		const int coalesceText = (lastChild != NULL) && (lastChild->type == XML_TEXT_NODE) && (lastChild->name == xmlStringText);
		if(coalesceText && ctxt->nodemem != 0) {
			/*
			 * The whole point of maintaining nodelen and nodemem,
			 * xmlTextConcat is too costly, i.e. compute length,
			 * reallocate a new buffer, move data, append ch. Here
			 * We try to minimaze realloc() uses and avoid copying
			 * and recomputing length over and over.
			 */
			if(lastChild->content == (xmlChar *)&(lastChild->properties)) {
				lastChild->content = sstrdup(lastChild->content);
				lastChild->properties = NULL;
			}
			else if((ctxt->nodemem == ctxt->nodelen + 1) && (xmlDictOwns(ctxt->dict, lastChild->content))) {
				lastChild->content = sstrdup(lastChild->content);
			}
			if(lastChild->content == NULL) {
				xmlSAX2ErrMemory(ctxt, "xmlSAX2Characters: xmlStrdup returned NULL");
				return;
			}
			if(((size_t)ctxt->nodelen + (size_t)len > XML_MAX_TEXT_LENGTH) && ((ctxt->options & XML_PARSE_HUGE) == 0)) {
				xmlSAX2ErrMemory(ctxt, "xmlSAX2Characters: huge text node");
				return;
			}
			if((size_t)ctxt->nodelen > SIZE_T_MAX - (size_t)len || (size_t)ctxt->nodemem + (size_t)len > SIZE_T_MAX / 2) {
				xmlSAX2ErrMemory(ctxt, "xmlSAX2Characters overflow prevented");
				return;
			}
			if((ctxt->nodelen + len) >= ctxt->nodemem) {
				size_t size = ctxt->nodemem + len;
				size *= 2;
				xmlChar * newbuf = static_cast<xmlChar *>(SAlloc::R(lastChild->content, size));
				if(newbuf == NULL) {
					xmlSAX2ErrMemory(ctxt, "xmlSAX2Characters");
					return;
				}
				ctxt->nodemem = size;
				lastChild->content = newbuf;
			}
			memcpy(&lastChild->content[ctxt->nodelen], ch, len);
			ctxt->nodelen += len;
			lastChild->content[ctxt->nodelen] = 0;
		}
		else if(coalesceText) {
			if(xmlTextConcat(lastChild, ch, len)) {
				xmlSAX2ErrMemory(ctxt, "xmlSAX2Characters");
			}
			if(ctxt->P_Node->children) {
				ctxt->nodelen = sstrlen(lastChild->content);
				ctxt->nodemem = ctxt->nodelen + 1;
			}
		}
		else {
			/* Mixed content, first time */
			lastChild = xmlSAX2TextNode(ctxt, ch, len);
			if(lastChild) {
				xmlAddChild(ctxt->P_Node, lastChild);
				if(ctxt->P_Node->children) {
					ctxt->nodelen = len;
					ctxt->nodemem = len + 1;
				}
			}
		}
	}
}

/**
 * xmlSAX2IgnorableWhitespace:
 * @ctx: the user data (XML parser context)
 * @ch:  a xmlChar string
 * @len: the number of xmlChar
 *
 * receiving some ignorable whitespaces from the parser.
 * UNUSED: by default the DOM building will use xmlSAX2Characters
 */
void xmlSAX2IgnorableWhitespace(void * ctx ATTRIBUTE_UNUSED, const xmlChar * ch ATTRIBUTE_UNUSED, int len ATTRIBUTE_UNUSED)
{
	/* xmlParserCtxt * ctxt = (xmlParserCtxt *) ctx; */
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2IgnorableWhitespace(%.30s, %d)\n", ch, len);
#endif
}

/**
 * xmlSAX2ProcessingInstruction:
 * @ctx: the user data (XML parser context)
 * @target:  the target name
 * @data: the PI data's
 *
 * A processing instruction has been parsed.
 */
void xmlSAX2ProcessingInstruction(void * ctx, const xmlChar * target, const xmlChar * data)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlNode * ret;
	xmlNode * parent;
	if(!ctx)
		return;
	parent = ctxt->P_Node;
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2ProcessingInstruction(%s, %s)\n", target, data);
#endif
	ret = xmlNewDocPI(ctxt->myDoc, target, data);
	if(!ret)
		return;
	if(ctxt->linenumbers) {
		if(ctxt->input)
			ret->line = (ctxt->input->line < 65535) ? static_cast<short>(ctxt->input->line) : 65535;
	}
	if(ctxt->inSubset == 1) {
		xmlAddChild((xmlNode *)ctxt->myDoc->intSubset, ret);
		return;
	}
	else if(ctxt->inSubset == 2) {
		xmlAddChild((xmlNode *)ctxt->myDoc->extSubset, ret);
		return;
	}
	if(parent == NULL) {
#ifdef DEBUG_SAX_TREE
		xmlGenericError(0, "Setting PI %s as root\n", target);
#endif
		xmlAddChild((xmlNode *)ctxt->myDoc, (xmlNode *)ret);
		return;
	}
	if(parent->type == XML_ELEMENT_NODE) {
#ifdef DEBUG_SAX_TREE
		xmlGenericError(0, "adding PI %s child to %s\n", target, parent->name);
#endif
		xmlAddChild(parent, ret);
	}
	else {
#ifdef DEBUG_SAX_TREE
		xmlGenericError(0, "adding PI %s sibling to ", target);
		xmlDebugDumpOneNode(stderr, parent, 0);
#endif
		xmlAddSibling(parent, ret);
	}
}

/**
 * xmlSAX2Comment:
 * @ctx: the user data (XML parser context)
 * @value:  the xmlSAX2Comment content
 *
 * A xmlSAX2Comment has been parsed.
 */
void xmlSAX2Comment(void * ctx, const xmlChar * value)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlNode * ret;
	xmlNode * parent;
	if(!ctx) 
		return;
	parent = ctxt->P_Node;
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.xmlSAX2Comment(%s)\n", value);
#endif
	ret = xmlNewDocComment(ctxt->myDoc, value);
	if(!ret)
		return;
	if(ctxt->linenumbers) {
		if(ctxt->input)
			ret->line = (ctxt->input->line < 65535) ? static_cast<short>(ctxt->input->line) : 65535;
	}
	if(ctxt->inSubset == 1) {
		xmlAddChild((xmlNode *)ctxt->myDoc->intSubset, ret);
		return;
	}
	else if(ctxt->inSubset == 2) {
		xmlAddChild((xmlNode *)ctxt->myDoc->extSubset, ret);
		return;
	}
	if(parent == NULL) {
#ifdef DEBUG_SAX_TREE
		xmlGenericError(0, "Setting xmlSAX2Comment as root\n");
#endif
		xmlAddChild((xmlNode *)ctxt->myDoc, (xmlNode *)ret);
		return;
	}
	if(parent->type == XML_ELEMENT_NODE) {
#ifdef DEBUG_SAX_TREE
		xmlGenericError(0, "adding xmlSAX2Comment child to %s\n", parent->name);
#endif
		xmlAddChild(parent, ret);
	}
	else {
#ifdef DEBUG_SAX_TREE
		xmlGenericError(0, "adding xmlSAX2Comment sibling to ");
		xmlDebugDumpOneNode(stderr, parent, 0);
#endif
		xmlAddSibling(parent, ret);
	}
}

/**
 * xmlSAX2CDataBlock:
 * @ctx: the user data (XML parser context)
 * @value:  The pcdata content
 * @len:  the block length
 *
 * called when a pcdata block has been parsed
 */
void xmlSAX2CDataBlock(void * ctx, const xmlChar * value, int len)
{
	xmlParserCtxt * ctxt = static_cast<xmlParserCtxt *>(ctx);
	xmlNode * ret;
	xmlNode * lastChild;
	if(!ctx) return;
#ifdef DEBUG_SAX
	xmlGenericError(0, "SAX.pcdata(%.10s, %d)\n", value, len);
#endif
	lastChild = xmlGetLastChild(ctxt->P_Node);
#ifdef DEBUG_SAX_TREE
	xmlGenericError(0, "add chars to %s \n", ctxt->node->name);
#endif
	if(lastChild && lastChild->type == XML_CDATA_SECTION_NODE) {
		xmlTextConcat(lastChild, value, len);
	}
	else {
		ret = xmlNewCDataBlock(ctxt->myDoc, value, len);
		xmlAddChild(ctxt->P_Node, ret);
	}
}

static int xmlSAX2DefaultVersionValue = 2;

#ifdef LIBXML_SAX1_ENABLED
/**
 * xmlSAXDefaultVersion:
 * @version:  the version, 1 or 2
 *
 * Set the default version of SAX used globally by the library.
 * By default, during initialization the default is set to 2.
 * Note that it is generally a better coding style to use
 * xmlSAXVersion() to set up the version explicitly for a given
 * parsing context.
 *
 * Returns the previous value in case of success and -1 in case of error.
 */
int xmlSAXDefaultVersion(int version)
{
	int ret = xmlSAX2DefaultVersionValue;
	if((version != 1) && (version != 2))
		return -1;
	xmlSAX2DefaultVersionValue = version;
	return ret;
}

#endif /* LIBXML_SAX1_ENABLED */

/**
 * xmlSAXVersion:
 * @hdlr:  the SAX handler
 * @version:  the version, 1 or 2
 *
 * Initialize the default XML SAX handler according to the version
 *
 * Returns 0 in case of success and -1 in case of error.
 */
int xmlSAXVersion(xmlSAXHandler * hdlr, int version)
{
	if(hdlr == NULL) 
		return -1;
	if(version == 2) {
		hdlr->startElement = NULL;
		hdlr->endElement = NULL;
		hdlr->startElementNs = xmlSAX2StartElementNs;
		hdlr->endElementNs = xmlSAX2EndElementNs;
		hdlr->serror = NULL;
		hdlr->initialized = XML_SAX2_MAGIC;
#ifdef LIBXML_SAX1_ENABLED
	}
	else if(version == 1) {
		hdlr->startElement = xmlSAX2StartElement;
		hdlr->endElement = xmlSAX2EndElement;
		hdlr->initialized = 1;
#endif /* LIBXML_SAX1_ENABLED */
	}
	else
		return -1;
	hdlr->internalSubset = xmlSAX2InternalSubset;
	hdlr->externalSubset = xmlSAX2ExternalSubset;
	hdlr->isStandalone = xmlSAX2IsStandalone;
	hdlr->hasInternalSubset = xmlSAX2HasInternalSubset;
	hdlr->hasExternalSubset = xmlSAX2HasExternalSubset;
	hdlr->resolveEntity = xmlSAX2ResolveEntity;
	hdlr->getEntity = xmlSAX2GetEntity;
	hdlr->getParameterEntity = xmlSAX2GetParameterEntity;
	hdlr->entityDecl = xmlSAX2EntityDecl;
	hdlr->attributeDecl = xmlSAX2AttributeDecl;
	hdlr->elementDecl = xmlSAX2ElementDecl;
	hdlr->notationDecl = xmlSAX2NotationDecl;
	hdlr->unparsedEntityDecl = xmlSAX2UnparsedEntityDecl;
	hdlr->setDocumentLocator = xmlSAX2SetDocumentLocator;
	hdlr->startDocument = xmlSAX2StartDocument;
	hdlr->endDocument = xmlSAX2EndDocument;
	hdlr->reference = xmlSAX2Reference;
	hdlr->characters = xmlSAX2Characters;
	hdlr->cdataBlock = xmlSAX2CDataBlock;
	hdlr->ignorableWhitespace = xmlSAX2Characters;
	hdlr->processingInstruction = xmlSAX2ProcessingInstruction;
	hdlr->comment = xmlSAX2Comment;
	hdlr->warning = xmlParserWarning;
	hdlr->error = xmlParserError;
	hdlr->fatalError = xmlParserError;
	return 0;
}

/**
 * xmlSAX2InitDefaultSAXHandler:
 * @hdlr:  the SAX handler
 * @warning:  flag if non-zero sets the handler warning procedure
 *
 * Initialize the default XML SAX2 handler
 */
void xmlSAX2InitDefaultSAXHandler(xmlSAXHandler * hdlr, int warning)
{
	if(hdlr && !hdlr->initialized) {
		xmlSAXVersion(hdlr, xmlSAX2DefaultVersionValue);
		hdlr->warning = warning ? xmlParserWarning : 0;
	}
}

/**
 * xmlDefaultSAXHandlerInit:
 *
 * Initialize the default SAX2 handler
 */
void xmlDefaultSAXHandlerInit()
{
#ifdef LIBXML_SAX1_ENABLED
	xmlSAXVersion((xmlSAXHandler *) &xmlDefaultSAXHandler, 1);
#endif /* LIBXML_SAX1_ENABLED */
}

#ifdef LIBXML_HTML_ENABLED

/**
 * xmlSAX2InitHtmlDefaultSAXHandler:
 * @hdlr:  the SAX handler
 *
 * Initialize the default HTML SAX2 handler
 */
void xmlSAX2InitHtmlDefaultSAXHandler(xmlSAXHandler * hdlr)
{
	if(hdlr && !hdlr->initialized) {
		hdlr->internalSubset = xmlSAX2InternalSubset;
		hdlr->externalSubset = NULL;
		hdlr->isStandalone = NULL;
		hdlr->hasInternalSubset = NULL;
		hdlr->hasExternalSubset = NULL;
		hdlr->resolveEntity = NULL;
		hdlr->getEntity = xmlSAX2GetEntity;
		hdlr->getParameterEntity = NULL;
		hdlr->entityDecl = NULL;
		hdlr->attributeDecl = NULL;
		hdlr->elementDecl = NULL;
		hdlr->notationDecl = NULL;
		hdlr->unparsedEntityDecl = NULL;
		hdlr->setDocumentLocator = xmlSAX2SetDocumentLocator;
		hdlr->startDocument = xmlSAX2StartDocument;
		hdlr->endDocument = xmlSAX2EndDocument;
		hdlr->startElement = xmlSAX2StartElement;
		hdlr->endElement = xmlSAX2EndElement;
		hdlr->reference = NULL;
		hdlr->characters = xmlSAX2Characters;
		hdlr->cdataBlock = xmlSAX2CDataBlock;
		hdlr->ignorableWhitespace = xmlSAX2IgnorableWhitespace;
		hdlr->processingInstruction = xmlSAX2ProcessingInstruction;
		hdlr->comment = xmlSAX2Comment;
		hdlr->warning = xmlParserWarning;
		hdlr->error = xmlParserError;
		hdlr->fatalError = xmlParserError;
		hdlr->initialized = 1;
	}
}

/**
 * htmlDefaultSAXHandlerInit:
 *
 * Initialize the default SAX handler
 */
void htmlDefaultSAXHandlerInit()
{
	xmlSAX2InitHtmlDefaultSAXHandler((xmlSAXHandler *) &htmlDefaultSAXHandler);
}

#endif /* LIBXML_HTML_ENABLED */

#ifdef LIBXML_DOCB_ENABLED

/**
 * xmlSAX2InitDocbDefaultSAXHandler:
 * @hdlr:  the SAX handler
 *
 * Initialize the default DocBook SAX2 handler
 */
void xmlSAX2InitDocbDefaultSAXHandler(xmlSAXHandler * hdlr)
{
	if(hdlr && !hdlr->initialized) {
		hdlr->internalSubset = xmlSAX2InternalSubset;
		hdlr->externalSubset = NULL;
		hdlr->isStandalone = xmlSAX2IsStandalone;
		hdlr->hasInternalSubset = xmlSAX2HasInternalSubset;
		hdlr->hasExternalSubset = xmlSAX2HasExternalSubset;
		hdlr->resolveEntity = xmlSAX2ResolveEntity;
		hdlr->getEntity = xmlSAX2GetEntity;
		hdlr->getParameterEntity = NULL;
		hdlr->entityDecl = xmlSAX2EntityDecl;
		hdlr->attributeDecl = NULL;
		hdlr->elementDecl = NULL;
		hdlr->notationDecl = NULL;
		hdlr->unparsedEntityDecl = NULL;
		hdlr->setDocumentLocator = xmlSAX2SetDocumentLocator;
		hdlr->startDocument = xmlSAX2StartDocument;
		hdlr->endDocument = xmlSAX2EndDocument;
		hdlr->startElement = xmlSAX2StartElement;
		hdlr->endElement = xmlSAX2EndElement;
		hdlr->reference = xmlSAX2Reference;
		hdlr->characters = xmlSAX2Characters;
		hdlr->cdataBlock = NULL;
		hdlr->ignorableWhitespace = xmlSAX2IgnorableWhitespace;
		hdlr->processingInstruction = NULL;
		hdlr->comment = xmlSAX2Comment;
		hdlr->warning = xmlParserWarning;
		hdlr->error = xmlParserError;
		hdlr->fatalError = xmlParserError;
		hdlr->initialized = 1;
	}
}
/**
 * docbDefaultSAXHandlerInit:
 *
 * Initialize the default SAX handler
 */
void docbDefaultSAXHandlerInit()
{
	xmlSAX2InitDocbDefaultSAXHandler((xmlSAXHandler *) &docbDefaultSAXHandler);
}

#endif /* LIBXML_DOCB_ENABLED */
#define bottom_SAX2
//#include "elfgcchack.h"
