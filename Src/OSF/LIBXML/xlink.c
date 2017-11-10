/*
 * xlink.c : implementation of the hyperlinks detection module
 *           This version supports both XML XLinks and HTML simple links
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */
#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop
#ifdef LIBXML_XPTR_ENABLED
#ifdef HAVE_SYS_STAT_H
	//#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
	//#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif
#ifdef HAVE_ZLIB_H
	#include <zlib.h>
#endif
#include <libxml/xlink.h>

#define XLINK_NAMESPACE (BAD_CAST "http://www.w3.org/1999/xlink/namespace/")
#define XHTML_NAMESPACE (BAD_CAST "http://www.w3.org/1999/xhtml/")

/****************************************************************
*								*
*           Default setting and related functions		*
*								*
****************************************************************/

static xlinkHandlerPtr xlinkDefaultHandler = NULL;
static xlinkNodeDetectFunc xlinkDefaultDetect = NULL;

/**
 * xlinkGetDefaultHandler:
 *
 * Get the default xlink handler.
 *
 * Returns the current xlinkHandlerPtr value.
 */
xlinkHandlerPtr xlinkGetDefaultHandler() 
{
	return(xlinkDefaultHandler);
}
/**
 * xlinkSetDefaultHandler:
 * @handler:  the new value for the xlink handler block
 *
 * Set the default xlink handlers
 */
void xlinkSetDefaultHandler(xlinkHandlerPtr handler) 
{
	xlinkDefaultHandler = handler;
}
/**
 * xlinkGetDefaultDetect:
 *
 * Get the default xlink detection routine
 *
 * Returns the current function or NULL;
 */
xlinkNodeDetectFunc xlinkGetDefaultDetect() 
{
	return(xlinkDefaultDetect);
}

/**
 * xlinkSetDefaultDetect:
 * @func: pointer to the new detection routine.
 *
 * Set the default xlink detection routine
 */
void xlinkSetDefaultDetect(xlinkNodeDetectFunc func) 
{
	xlinkDefaultDetect = func;
}

/****************************************************************
*								*
*                  The detection routines			*
*								*
****************************************************************/

/**
 * xlinkIsLink:
 * @doc:  the document containing the node
 * @node:  the node pointer itself
 *
 * Check whether the given node carries the attributes needed
 * to be a link element (or is one of the linking elements issued
 * from the (X)HTML DtDs).
 * This routine don't try to do full checking of the link validity
 * but tries to detect and return the appropriate link type.
 *
 * Returns the xlinkType of the node (XLINK_TYPE_NONE if there is no
 *         link detected.
 */
xlinkType xlinkIsLink(xmlDocPtr doc, xmlNodePtr P_Node) 
{
	xmlChar * type = NULL, * role = NULL;
	xlinkType ret = XLINK_TYPE_NONE;
	if(!P_Node) 
		return XLINK_TYPE_NONE;
	SETIFZ(doc, P_Node->doc);
	if(doc && (doc->type == XML_HTML_DOCUMENT_NODE)) {
		/*
		 * This is an HTML document.
		 */
	}
	else if(P_Node->ns && (sstreq(P_Node->ns->href, XHTML_NAMESPACE))) {
		/*
		 * !!!! We really need an IS_XHTML_ELEMENT function from HTMLtree.h @@@
		 */
		/*
		 * This is an XHTML element within an XML document
		 * Check whether it's one of the element able to carry links
		 * and in that case if it holds the attributes.
		 */
	}
	/*
	 * We don't prevent a-priori having XML Linking constructs on XHTML elements
	 */
	type = xmlGetNsProp(P_Node, BAD_CAST "type", XLINK_NAMESPACE);
	if(type) {
		if(sstreq(type, BAD_CAST "simple")) {
			ret = XLINK_TYPE_SIMPLE;
		}
		else if(sstreq(type, "extended")) {
			role = xmlGetNsProp(P_Node, BAD_CAST "role", XLINK_NAMESPACE);
			if(role) {
				xmlNs * xlink = xmlSearchNs(doc, P_Node, XLINK_NAMESPACE);
				if(xlink == NULL) {
					/* Humm, fallback method */
					if(sstreq(role, "xlink:external-linkset"))
						ret = XLINK_TYPE_EXTENDED_SET;
				}
				else {
					xmlChar buf[200];
					snprintf((char*)buf, sizeof(buf), "%s:external-linkset", (char*)xlink->prefix);
					buf[sizeof(buf) - 1] = 0;
					if(sstreq(role, buf))
						ret = XLINK_TYPE_EXTENDED_SET;
				}
			}
			ret = XLINK_TYPE_EXTENDED;
		}
	}
	SAlloc::F(type);
	SAlloc::F(role);
	return ret;
}

#endif /* LIBXML_XPTR_ENABLED */
#define bottom_xlink
#include "elfgcchack.h"
