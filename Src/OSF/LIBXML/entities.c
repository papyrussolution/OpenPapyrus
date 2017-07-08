/*
 * entities.c : implementation for the XML entities handling
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */
// 
#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop
#include <libxml/entities.h>
#include <libxml/parserInternals.h>
#include <libxml/dict.h>
#include "save.h"

/*
 * The XML predefined entities.
 */

static xmlEntity xmlEntityLt = {
	NULL, XML_ENTITY_DECL, BAD_CAST "lt",
	NULL, NULL, NULL, NULL, NULL, NULL,
	BAD_CAST "<", BAD_CAST "<", 1,
	XML_INTERNAL_PREDEFINED_ENTITY,
	NULL, NULL, NULL, NULL, 0, 1
};
static xmlEntity xmlEntityGt = {
	NULL, XML_ENTITY_DECL, BAD_CAST "gt",
	NULL, NULL, NULL, NULL, NULL, NULL,
	BAD_CAST ">", BAD_CAST ">", 1,
	XML_INTERNAL_PREDEFINED_ENTITY,
	NULL, NULL, NULL, NULL, 0, 1
};
static xmlEntity xmlEntityAmp = {
	NULL, XML_ENTITY_DECL, BAD_CAST "amp",
	NULL, NULL, NULL, NULL, NULL, NULL,
	BAD_CAST "&", BAD_CAST "&", 1,
	XML_INTERNAL_PREDEFINED_ENTITY,
	NULL, NULL, NULL, NULL, 0, 1
};
static xmlEntity xmlEntityQuot = {
	NULL, XML_ENTITY_DECL, BAD_CAST "quot",
	NULL, NULL, NULL, NULL, NULL, NULL,
	BAD_CAST "\"", BAD_CAST "\"", 1,
	XML_INTERNAL_PREDEFINED_ENTITY,
	NULL, NULL, NULL, NULL, 0, 1
};
static xmlEntity xmlEntityApos = {
	NULL, XML_ENTITY_DECL, BAD_CAST "apos",
	NULL, NULL, NULL, NULL, NULL, NULL,
	BAD_CAST "'", BAD_CAST "'", 1,
	XML_INTERNAL_PREDEFINED_ENTITY,
	NULL, NULL, NULL, NULL, 0, 1
};

/**
 * xmlEntitiesErrMemory:
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void xmlEntitiesErrMemory(const char * extra)
{
	__xmlSimpleError(XML_FROM_TREE, XML_ERR_NO_MEMORY, NULL, NULL, extra);
}

/**
 * xmlEntitiesErr:
 * @code:  the error code
 * @msg:  the message
 *
 * Handle an out of memory condition
 */
static void xmlEntitiesErr(xmlParserErrors code, const char * msg)
{
	__xmlSimpleError(XML_FROM_TREE, code, NULL, msg, NULL);
}

/*
 * xmlFreeEntity : clean-up an entity record.
 */
static void FASTCALL xmlFreeEntity(xmlEntityPtr entity)
{
	if(entity) {
		xmlDictPtr dict = entity->doc ? entity->doc->dict : 0;
		if(entity->children && (entity->owner == 1) && (entity == (xmlEntityPtr)entity->children->parent))
			xmlFreeNodeList(entity->children);
		if(dict) {
			if(entity->name && !xmlDictOwns(dict, entity->name))
				SAlloc::F((char*)entity->name);
			if(entity->ExternalID && (!xmlDictOwns(dict, entity->ExternalID)))
				SAlloc::F((char*)entity->ExternalID);
			if(entity->SystemID && (!xmlDictOwns(dict, entity->SystemID)))
				SAlloc::F((char*)entity->SystemID);
			if(entity->URI && (!xmlDictOwns(dict, entity->URI)))
				SAlloc::F((char*)entity->URI);
			if(entity->content && (!xmlDictOwns(dict, entity->content)))
				SAlloc::F((char*)entity->content);
			if(entity->orig && (!xmlDictOwns(dict, entity->orig)))
				SAlloc::F((char*)entity->orig);
		}
		else {
			SAlloc::F((char*)entity->name);
			SAlloc::F((char*)entity->ExternalID);
			SAlloc::F((char*)entity->SystemID);
			SAlloc::F((char*)entity->URI);
			SAlloc::F((char*)entity->content);
			SAlloc::F((char*)entity->orig);
		}
		SAlloc::F(entity);
	}
}

/*
 * xmlCreateEntity:
 *
 * internal routine doing the entity node strutures allocations
 */
static xmlEntityPtr xmlCreateEntity(xmlDictPtr dict, const xmlChar * name, int type,
    const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content) 
{
	xmlEntityPtr ret = (xmlEntityPtr)SAlloc::M(sizeof(xmlEntity));
	if(!ret) {
		xmlEntitiesErrMemory("xmlCreateEntity: malloc failed");
		return 0;
	}
	memzero(ret, sizeof(xmlEntity));
	ret->type = XML_ENTITY_DECL;
	ret->checked = 0;
	/*
	 * fill the structure.
	 */
	ret->etype = (xmlEntityType)type;
	if(!dict) {
		ret->name = xmlStrdup(name);
		if(ExternalID != NULL)
			ret->ExternalID = xmlStrdup(ExternalID);
		if(SystemID != NULL)
			ret->SystemID = xmlStrdup(SystemID);
	}
	else {
		ret->name = xmlDictLookup(dict, name, -1);
		if(ExternalID != NULL)
			ret->ExternalID = xmlDictLookup(dict, ExternalID, -1);
		if(SystemID != NULL)
			ret->SystemID = xmlDictLookup(dict, SystemID, -1);
	}
	if(content != NULL) {
		ret->length = sstrlen(content);
		ret->content = (dict && (ret->length < 5)) ? (xmlChar*)xmlDictLookup(dict, content, ret->length) : xmlStrndup(content, ret->length);
	}
	else {
		ret->length = 0;
		ret->content = NULL;
	}
	ret->URI = NULL; // to be computed by the layer knowing the defining entity 
	ret->orig = NULL;
	ret->owner = 0;
	return ret;
}
/*
 * xmlAddEntity : register a new entity for an entities table.
 */
static xmlEntityPtr xmlAddEntity(xmlDtdPtr dtd, const xmlChar * name, int type,
    const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content)
{
	xmlEntityPtr ret = 0;
	xmlDictPtr dict = NULL;
	xmlEntitiesTablePtr table = NULL;
	if(name && dtd) {
		if(dtd->doc != NULL)
			dict = dtd->doc->dict;
		switch(type) {
			case XML_INTERNAL_GENERAL_ENTITY:
			case XML_EXTERNAL_GENERAL_PARSED_ENTITY:
			case XML_EXTERNAL_GENERAL_UNPARSED_ENTITY:
				SETIFZ(dtd->entities, xmlHashCreateDict(0, dict));
				table = (xmlEntitiesTablePtr)dtd->entities;
				break;
			case XML_INTERNAL_PARAMETER_ENTITY:
			case XML_EXTERNAL_PARAMETER_ENTITY:
				SETIFZ(dtd->pentities, xmlHashCreateDict(0, dict));
				table = (xmlEntitiesTablePtr)dtd->pentities;
				break;
			case XML_INTERNAL_PREDEFINED_ENTITY:
				return 0;
		}
		if(table) {
			ret = xmlCreateEntity(dict, name, type, ExternalID, SystemID, content);
			if(ret) {
				ret->doc = dtd->doc;
				if(xmlHashAddEntry(table, name, ret)) {
					// entity was already defined at another level.
					xmlFreeEntity(ret);
					ret = 0;
				}
			}
		}
		else
			ret = 0;
	}
	return ret;
}

/**
 * xmlGetPredefinedEntity:
 * @name:  the entity name
 *
 * Check whether this name is an predefined entity.
 *
 * Returns NULL if not, otherwise the entity
 */
xmlEntityPtr xmlGetPredefinedEntity(const xmlChar * name) 
{
	if(name) {
		switch(name[0]) {
			case 'l':
				if(sstreq(name, BAD_CAST "lt"))
					return(&xmlEntityLt);
				break;
			case 'g':
				if(sstreq(name, BAD_CAST "gt"))
					return(&xmlEntityGt);
				break;
			case 'a':
				if(sstreq(name, BAD_CAST "amp"))
					return(&xmlEntityAmp);
				if(sstreq(name, BAD_CAST "apos"))
					return(&xmlEntityApos);
				break;
			case 'q':
				if(sstreq(name, BAD_CAST "quot"))
					return(&xmlEntityQuot);
				break;
			default:
				break;
		}
	}
	return 0;
}

/**
 * xmlAddDtdEntity:
 * @doc:  the document
 * @name:  the entity name
 * @type:  the entity type XML_xxx_yyy_ENTITY
 * @ExternalID:  the entity external ID if available
 * @SystemID:  the entity system ID if available
 * @content:  the entity content
 *
 * Register a new entity for this document DTD external subset.
 *
 * Returns a pointer to the entity or NULL in case of error
 */
xmlEntityPtr xmlAddDtdEntity(xmlDocPtr doc, const xmlChar * name, int type,
    const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content) 
{
	xmlEntityPtr ret;
	xmlDtdPtr dtd;
	if(doc == NULL) {
		xmlEntitiesErr(XML_DTD_NO_DOC, "xmlAddDtdEntity: document is NULL");
		return 0;
	}
	if(doc->extSubset == NULL) {
		xmlEntitiesErr(XML_DTD_NO_DTD, "xmlAddDtdEntity: document without external subset");
		return 0;
	}
	dtd = doc->extSubset;
	ret = xmlAddEntity(dtd, name, type, ExternalID, SystemID, content);
	if(!ret) return 0;

	/*
	 * Link it to the DTD
	 */
	ret->parent = dtd;
	ret->doc = dtd->doc;
	if(dtd->last == NULL) {
		dtd->children = dtd->last = (xmlNode *)ret;
	}
	else {
		dtd->last->next = (xmlNode *)ret;
		ret->prev = dtd->last;
		dtd->last = (xmlNode *)ret;
	}
	return ret;
}

/**
 * xmlAddDocEntity:
 * @doc:  the document
 * @name:  the entity name
 * @type:  the entity type XML_xxx_yyy_ENTITY
 * @ExternalID:  the entity external ID if available
 * @SystemID:  the entity system ID if available
 * @content:  the entity content
 *
 * Register a new entity for this document.
 *
 * Returns a pointer to the entity or NULL in case of error
 */
xmlEntityPtr xmlAddDocEntity(xmlDocPtr doc, const xmlChar * name, int type,
    const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content) 
{
	xmlEntityPtr ret = 0;
	if(doc == NULL) {
		xmlEntitiesErr(XML_DTD_NO_DOC, "xmlAddDocEntity: document is NULL");
	}
	else if(doc->intSubset == NULL) {
		xmlEntitiesErr(XML_DTD_NO_DTD, "xmlAddDocEntity: document without internal subset");
	}
	else {
		xmlDtdPtr dtd = doc->intSubset;
		ret = xmlAddEntity(dtd, name, type, ExternalID, SystemID, content);
		if(ret) {
			// Link it to the DTD
			ret->parent = dtd;
			ret->doc = dtd->doc;
			if(dtd->last == NULL) {
				dtd->children = dtd->last = (xmlNode *)ret;
			}
			else {
				dtd->last->next = (xmlNode *)ret;
				ret->prev = dtd->last;
				dtd->last = (xmlNode *)ret;
			}
		}
	}
	return ret;
}

/**
 * xmlNewEntity:
 * @doc:  the document
 * @name:  the entity name
 * @type:  the entity type XML_xxx_yyy_ENTITY
 * @ExternalID:  the entity external ID if available
 * @SystemID:  the entity system ID if available
 * @content:  the entity content
 *
 * Create a new entity, this differs from xmlAddDocEntity() that if
 * the document is NULL or has no internal subset defined, then an
 * unlinked entity structure will be returned, it is then the responsability
 * of the caller to link it to the document later or free it when not needed
 * anymore.
 *
 * Returns a pointer to the entity or NULL in case of error
 */
xmlEntityPtr xmlNewEntity(xmlDocPtr doc, const xmlChar * name, int type, const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content)
{
	if(doc && doc->intSubset) {
		return xmlAddDocEntity(doc, name, type, ExternalID, SystemID, content);
	}
	else {
		xmlDictPtr dict = doc ? doc->dict : 0;
		xmlEntityPtr ret = xmlCreateEntity(dict, name, type, ExternalID, SystemID, content);
		if(ret)
			ret->doc = doc;
		return ret;
	}
}
/**
 * xmlGetEntityFromTable:
 * @table:  an entity table
 * @name:  the entity name
 * @parameter:  look for parameter entities
 *
 * Do an entity lookup in the table.
 * returns the corresponding parameter entity, if found.
 *
 * Returns A pointer to the entity structure or NULL if not found.
 */
static xmlEntityPtr xmlGetEntityFromTable(xmlEntitiesTablePtr table, const xmlChar * name)
{
	return (xmlEntityPtr)xmlHashLookup(table, name);
}

/**
 * xmlGetParameterEntity:
 * @doc:  the document referencing the entity
 * @name:  the entity name
 *
 * Do an entity lookup in the internal and external subsets and
 * returns the corresponding parameter entity, if found.
 *
 * Returns A pointer to the entity structure or NULL if not found.
 */
xmlEntityPtr xmlGetParameterEntity(xmlDocPtr doc, const xmlChar * name)
{
	xmlEntitiesTablePtr table;
	xmlEntityPtr ret;
	if(doc) {
		if(doc->intSubset && doc->intSubset->pentities) {
			table = (xmlEntitiesTablePtr)doc->intSubset->pentities;
			ret = xmlGetEntityFromTable(table, name);
			if(ret)
				return ret;
		}
		if(doc->extSubset && doc->extSubset->pentities) {
			table = (xmlEntitiesTablePtr)doc->extSubset->pentities;
			return xmlGetEntityFromTable(table, name);
		}
	}
	return 0;
}
/**
 * xmlGetDtdEntity:
 * @doc:  the document referencing the entity
 * @name:  the entity name
 *
 * Do an entity lookup in the DTD entity hash table and
 * returns the corresponding entity, if found.
 * Note: the first argument is the document node, not the DTD node.
 *
 * Returns A pointer to the entity structure or NULL if not found.
 */
xmlEntityPtr xmlGetDtdEntity(xmlDocPtr doc, const xmlChar * name)
{
	if(doc && doc->extSubset && doc->extSubset->entities) {
		xmlEntitiesTablePtr table = (xmlEntitiesTablePtr)doc->extSubset->entities;
		return xmlGetEntityFromTable(table, name);
	}
	return 0;
}

/**
 * xmlGetDocEntity:
 * @doc:  the document referencing the entity
 * @name:  the entity name
 *
 * Do an entity lookup in the document entity hash table and
 * returns the corresponding entity, otherwise a lookup is done
 * in the predefined entities too.
 *
 * Returns A pointer to the entity structure or NULL if not found.
 */
xmlEntityPtr xmlGetDocEntity(const xmlDoc * doc, const xmlChar * name)
{
	xmlEntityPtr cur;
	xmlEntitiesTablePtr table;
	if(doc != NULL) {
		if((doc->intSubset != NULL) && (doc->intSubset->entities != NULL)) {
			table = (xmlEntitiesTablePtr)doc->intSubset->entities;
			cur = xmlGetEntityFromTable(table, name);
			if(cur)
				return cur;
		}
		if(doc->standalone != 1) {
			if((doc->extSubset != NULL) &&
			    (doc->extSubset->entities != NULL)) {
				table = (xmlEntitiesTablePtr)doc->extSubset->entities;
				cur = xmlGetEntityFromTable(table, name);
				if(cur)
					return cur;
			}
		}
	}
	return(xmlGetPredefinedEntity(name));
}

/*
 * Macro used to grow the current buffer.
 */
#define growBufferReentrant() {						\
		xmlChar * tmp;							     \
		size_t new_size = buffer_size * 2;				    \
		if(new_size < buffer_size) goto mem_error;			   \
		tmp = (xmlChar*)SAlloc::R(buffer, new_size);			  \
		if(tmp == NULL) goto mem_error;					   \
		buffer = tmp;							    \
		buffer_size = new_size;						    \
}

/**
 * xmlEncodeEntitiesInternal:
 * @doc:  the document containing the string
 * @input:  A string to convert to XML.
 * @attr: are we handling an atrbute value
 *
 * Do a global encoding of a string, replacing the predefined entities
 * and non ASCII values with their entities and CharRef counterparts.
 * Contrary to xmlEncodeEntities, this routine is reentrant, and result
 * must be deallocated.
 *
 * Returns A newly allocated string with the substitution done.
 */
static xmlChar * xmlEncodeEntitiesInternal(xmlDocPtr doc, const xmlChar * input, int attr) 
{
	const xmlChar * cur = input;
	xmlChar * buffer = NULL;
	xmlChar * out = NULL;
	size_t buffer_size = 0;
	int html = 0;
	if(input == NULL) 
		return 0;
	if(doc != NULL)
		html = (doc->type == XML_HTML_DOCUMENT_NODE);
	/*
	 * allocate an translation buffer.
	 */
	buffer_size = 1000;
	buffer = (xmlChar*)SAlloc::M(buffer_size * sizeof(xmlChar));
	if(!buffer) {
		xmlEntitiesErrMemory("xmlEncodeEntities: malloc failed");
		return 0;
	}
	out = buffer;
	while(*cur != '\0') {
		size_t indx = out - buffer;
		if(indx + 100 > buffer_size) {
			growBufferReentrant();
			out = &buffer[indx];
		}

		/*
		 * By default one have to encode at least '<', '>', '"' and '&' !
		 */
		if(*cur == '<') {
			const xmlChar * end;
			/*
			 * Special handling of server side include in HTML attributes
			 */
			if(html && attr && (cur[1] == '!') && (cur[2] == '-') && (cur[3] == '-') && ((end = xmlStrstr(cur, BAD_CAST "-->")) != NULL)) {
				while(cur != end) {
					*out++ = *cur++;
					indx = out - buffer;
					if(indx + 100 > buffer_size) {
						growBufferReentrant();
						out = &buffer[indx];
					}
				}
				*out++ = *cur++;
				*out++ = *cur++;
				*out++ = *cur++;
				continue;
			}
			*out++ = '&';
			*out++ = 'l';
			*out++ = 't';
			*out++ = ';';
		}
		else if(*cur == '>') {
			*out++ = '&';
			*out++ = 'g';
			*out++ = 't';
			*out++ = ';';
		}
		else if(*cur == '&') {
			/*
			 * Special handling of &{...} construct from HTML 4, see
			 * http://www.w3.org/TR/html401/appendix/notes.html#h-B.7.1
			 */
			if(html && attr && (cur[1] == '{') && (strchr((const char*)cur, '}'))) {
				while(*cur != '}') {
					*out++ = *cur++;
					indx = out - buffer;
					if(indx + 100 > buffer_size) {
						growBufferReentrant();
						out = &buffer[indx];
					}
				}
				*out++ = *cur++;
				continue;
			}
			*out++ = '&';
			*out++ = 'a';
			*out++ = 'm';
			*out++ = 'p';
			*out++ = ';';
		}
		else if(((*cur >= 0x20) && (*cur < 0x80)) || (*cur == '\n') || (*cur == '\t') || ((html) && (*cur == '\r'))) {
			/*
			 * default case, just copy !
			 */
			*out++ = *cur;
		}
		else if(*cur >= 0x80) {
			if(((doc != NULL) && (doc->encoding != NULL)) || (html)) {
				/*
				 * Bjørn Reese <br@sseusa.com> provided the patch
				   xmlChar xc;
				   xc = (*cur & 0x3F) << 6;
				   if (cur[1] != 0) {
				    xc += *(++cur) & 0x3F;
				 **out++ = xc;
				   } else
				 */
				*out++ = *cur;
			}
			else {
				/*
				 * We assume we have UTF-8 input.
				 */
				char buf[11], * ptr;
				int val = 0, l = 1;
				if(*cur < 0xC0) {
					xmlEntitiesErr(XML_CHECK_NOT_UTF8, "xmlEncodeEntities: input not UTF-8");
					if(doc != NULL)
						doc->encoding = xmlStrdup(BAD_CAST "ISO-8859-1");
					snprintf(buf, sizeof(buf), "&#%d;", *cur);
					buf[sizeof(buf) - 1] = 0;
					ptr = buf;
					while(*ptr != 0) *out++ = *ptr++;
					cur++;
					continue;
				}
				else if(*cur < 0xE0) {
					val = (cur[0]) & 0x1F;
					val <<= 6;
					val |= (cur[1]) & 0x3F;
					l = 2;
				}
				else if(*cur < 0xF0) {
					val = (cur[0]) & 0x0F;
					val <<= 6;
					val |= (cur[1]) & 0x3F;
					val <<= 6;
					val |= (cur[2]) & 0x3F;
					l = 3;
				}
				else if(*cur < 0xF8) {
					val = (cur[0]) & 0x07;
					val <<= 6;
					val |= (cur[1]) & 0x3F;
					val <<= 6;
					val |= (cur[2]) & 0x3F;
					val <<= 6;
					val |= (cur[3]) & 0x3F;
					l = 4;
				}
				if((l == 1) || (!IS_CHAR(val))) {
					xmlEntitiesErr(XML_ERR_INVALID_CHAR, "xmlEncodeEntities: char out of range\n");
					if(doc != NULL)
						doc->encoding = xmlStrdup(BAD_CAST "ISO-8859-1");
					snprintf(buf, sizeof(buf), "&#%d;", *cur);
					buf[sizeof(buf) - 1] = 0;
					ptr = buf;
					while(*ptr != 0) *out++ = *ptr++;
					cur++;
					continue;
				}
				/*
				 * We could do multiple things here. Just save as a char ref
				 */
				snprintf(buf, sizeof(buf), "&#x%X;", val);
				buf[sizeof(buf) - 1] = 0;
				ptr = buf;
				while(*ptr != 0) *out++ = *ptr++;
				cur += l;
				continue;
			}
		}
		else if(IS_BYTE_CHAR(*cur)) {
			char buf[11], * ptr;

			snprintf(buf, sizeof(buf), "&#%d;", *cur);
			buf[sizeof(buf) - 1] = 0;
			ptr = buf;
			while(*ptr != 0) *out++ = *ptr++;
		}
		cur++;
	}
	*out = 0;
	return(buffer);
mem_error:
	xmlEntitiesErrMemory("xmlEncodeEntities: realloc failed");
	SAlloc::F(buffer);
	return 0;
}

/**
 * xmlEncodeAttributeEntities:
 * @doc:  the document containing the string
 * @input:  A string to convert to XML.
 *
 * Do a global encoding of a string, replacing the predefined entities
 * and non ASCII values with their entities and CharRef counterparts for
 * attribute values.
 *
 * Returns A newly allocated string with the substitution done.
 */
xmlChar * xmlEncodeAttributeEntities(xmlDocPtr doc, const xmlChar * input)
{
	return xmlEncodeEntitiesInternal(doc, input, 1);
}

/**
 * xmlEncodeEntitiesReentrant:
 * @doc:  the document containing the string
 * @input:  A string to convert to XML.
 *
 * Do a global encoding of a string, replacing the predefined entities
 * and non ASCII values with their entities and CharRef counterparts.
 * Contrary to xmlEncodeEntities, this routine is reentrant, and result
 * must be deallocated.
 *
 * Returns A newly allocated string with the substitution done.
 */
xmlChar * xmlEncodeEntitiesReentrant(xmlDocPtr doc, const xmlChar * input)
{
	return xmlEncodeEntitiesInternal(doc, input, 0);
}
/**
 * xmlEncodeSpecialChars:
 * @doc:  the document containing the string
 * @input:  A string to convert to XML.
 *
 * Do a global encoding of a string, replacing the predefined entities
 * this routine is reentrant, and result must be deallocated.
 *
 * Returns A newly allocated string with the substitution done.
 */
xmlChar * xmlEncodeSpecialChars(const xmlDoc * doc ATTRIBUTE_UNUSED, const xmlChar * input)
{
	const xmlChar * cur = input;
	xmlChar * buffer = NULL;
	xmlChar * out = NULL;
	size_t buffer_size = 0;
	if(input == NULL) return 0;
	/*
	 * allocate an translation buffer.
	 */
	buffer_size = 1000;
	buffer = (xmlChar*)SAlloc::M(buffer_size * sizeof(xmlChar));
	if(!buffer) {
		xmlEntitiesErrMemory("xmlEncodeSpecialChars: malloc failed");
		return 0;
	}
	out = buffer;
	while(*cur != '\0') {
		size_t indx = out - buffer;
		if(indx + 10 > buffer_size) {
			growBufferReentrant();
			out = &buffer[indx];
		}
		/*
		 * By default one have to encode at least '<', '>', '"' and '&' !
		 */
		if(*cur == '<') {
			*out++ = '&';
			*out++ = 'l';
			*out++ = 't';
			*out++ = ';';
		}
		else if(*cur == '>') {
			*out++ = '&';
			*out++ = 'g';
			*out++ = 't';
			*out++ = ';';
		}
		else if(*cur == '&') {
			*out++ = '&';
			*out++ = 'a';
			*out++ = 'm';
			*out++ = 'p';
			*out++ = ';';
		}
		else if(*cur == '"') {
			*out++ = '&';
			*out++ = 'q';
			*out++ = 'u';
			*out++ = 'o';
			*out++ = 't';
			*out++ = ';';
		}
		else if(*cur == '\r') {
			*out++ = '&';
			*out++ = '#';
			*out++ = '1';
			*out++ = '3';
			*out++ = ';';
		}
		else {
			/*
			 * Works because on UTF-8, all extended sequences cannot
			 * result in bytes in the ASCII range.
			 */
			*out++ = *cur;
		}
		cur++;
	}
	*out = 0;
	return(buffer);
mem_error:
	xmlEntitiesErrMemory("xmlEncodeSpecialChars: realloc failed");
	SAlloc::F(buffer);
	return 0;
}

/**
 * xmlCreateEntitiesTable:
 *
 * create and initialize an empty entities hash table.
 * This really doesn't make sense and should be deprecated
 *
 * Returns the xmlEntitiesTablePtr just created or NULL in case of error.
 */
xmlEntitiesTablePtr xmlCreateEntitiesTable() {
	return((xmlEntitiesTablePtr)xmlHashCreate(0));
}

/**
 * xmlFreeEntityWrapper:
 * @entity:  An entity
 * @name:  its name
 *
 * Deallocate the memory used by an entities in the hash table.
 */
static void xmlFreeEntityWrapper(xmlEntityPtr entity, const xmlChar * name ATTRIBUTE_UNUSED)
{
	xmlFreeEntity(entity);
}

/**
 * xmlFreeEntitiesTable:
 * @table:  An entity table
 *
 * Deallocate the memory used by an entities hash table.
 */
void xmlFreeEntitiesTable(xmlEntitiesTablePtr table)
{
	xmlHashFree(table, (xmlHashDeallocator)xmlFreeEntityWrapper);
}

#ifdef LIBXML_TREE_ENABLED
/**
 * xmlCopyEntity:
 * @ent:  An entity
 *
 * Build a copy of an entity
 *
 * Returns the new xmlEntitiesPtr or NULL in case of error.
 */
static xmlEntityPtr xmlCopyEntity(xmlEntityPtr ent)
{
	xmlEntityPtr cur = (xmlEntityPtr)SAlloc::M(sizeof(xmlEntity));
	if(!cur) {
		xmlEntitiesErrMemory("xmlCopyEntity:: malloc failed");
		return 0;
	}
	memzero(cur, sizeof(xmlEntity));
	cur->type = XML_ENTITY_DECL;
	cur->etype = ent->etype;
	if(ent->name != NULL)
		cur->name = xmlStrdup(ent->name);
	if(ent->ExternalID != NULL)
		cur->ExternalID = xmlStrdup(ent->ExternalID);
	if(ent->SystemID != NULL)
		cur->SystemID = xmlStrdup(ent->SystemID);
	if(ent->content != NULL)
		cur->content = xmlStrdup(ent->content);
	if(ent->orig != NULL)
		cur->orig = xmlStrdup(ent->orig);
	if(ent->URI != NULL)
		cur->URI = xmlStrdup(ent->URI);
	return cur;
}

/**
 * xmlCopyEntitiesTable:
 * @table:  An entity table
 *
 * Build a copy of an entity table.
 *
 * Returns the new xmlEntitiesTablePtr or NULL in case of error.
 */
xmlEntitiesTablePtr xmlCopyEntitiesTable(xmlEntitiesTablePtr table) {
	return(xmlHashCopy(table, (xmlHashCopier)xmlCopyEntity));
}

#endif /* LIBXML_TREE_ENABLED */

#ifdef LIBXML_OUTPUT_ENABLED

/**
 * xmlDumpEntityContent:
 * @buf:  An XML buffer.
 * @content:  The entity content.
 *
 * This will dump the quoted string value, taking care of the special
 * treatment required by %
 */
static void xmlDumpEntityContent(xmlBufferPtr buf, const xmlChar * content) {
	if(buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) return;
	if(xmlStrchr(content, '%')) {
		const xmlChar * base, * cur;

		xmlBufferCCat(buf, "\"");
		base = cur = content;
		while(*cur != 0) {
			if(*cur == '"') {
				if(base != cur)
					xmlBufferAdd(buf, base, cur - base);
				xmlBufferAdd(buf, BAD_CAST "&quot;", 6);
				cur++;
				base = cur;
			}
			else if(*cur == '%') {
				if(base != cur)
					xmlBufferAdd(buf, base, cur - base);
				xmlBufferAdd(buf, BAD_CAST "&#x25;", 6);
				cur++;
				base = cur;
			}
			else {
				cur++;
			}
		}
		if(base != cur)
			xmlBufferAdd(buf, base, cur - base);
		xmlBufferCCat(buf, "\"");
	}
	else {
		xmlBufferWriteQuotedString(buf, content);
	}
}

/**
 * xmlDumpEntityDecl:
 * @buf:  An XML buffer.
 * @ent:  An entity table
 *
 * This will dump the content of the entity table as an XML DTD definition
 */
void xmlDumpEntityDecl(xmlBufferPtr buf, xmlEntityPtr ent) {
	if((buf == NULL) || (ent == NULL)) return;
	switch(ent->etype) {
		case XML_INTERNAL_GENERAL_ENTITY:
		    xmlBufferWriteChar(buf, "<!ENTITY ");
		    xmlBufferWriteCHAR(buf, ent->name);
		    xmlBufferWriteChar(buf, " ");
		    if(ent->orig != NULL)
			    xmlBufferWriteQuotedString(buf, ent->orig);
		    else
			    xmlDumpEntityContent(buf, ent->content);
		    xmlBufferWriteChar(buf, ">\n");
		    break;
		case XML_EXTERNAL_GENERAL_PARSED_ENTITY:
		    xmlBufferWriteChar(buf, "<!ENTITY ");
		    xmlBufferWriteCHAR(buf, ent->name);
		    if(ent->ExternalID != NULL) {
			    xmlBufferWriteChar(buf, " PUBLIC ");
			    xmlBufferWriteQuotedString(buf, ent->ExternalID);
			    xmlBufferWriteChar(buf, " ");
			    xmlBufferWriteQuotedString(buf, ent->SystemID);
		    }
		    else {
			    xmlBufferWriteChar(buf, " SYSTEM ");
			    xmlBufferWriteQuotedString(buf, ent->SystemID);
		    }
		    xmlBufferWriteChar(buf, ">\n");
		    break;
		case XML_EXTERNAL_GENERAL_UNPARSED_ENTITY:
		    xmlBufferWriteChar(buf, "<!ENTITY ");
		    xmlBufferWriteCHAR(buf, ent->name);
		    if(ent->ExternalID != NULL) {
			    xmlBufferWriteChar(buf, " PUBLIC ");
			    xmlBufferWriteQuotedString(buf, ent->ExternalID);
			    xmlBufferWriteChar(buf, " ");
			    xmlBufferWriteQuotedString(buf, ent->SystemID);
		    }
		    else {
			    xmlBufferWriteChar(buf, " SYSTEM ");
			    xmlBufferWriteQuotedString(buf, ent->SystemID);
		    }
		    if(ent->content != NULL) { /* Should be true ! */
			    xmlBufferWriteChar(buf, " NDATA ");
			    if(ent->orig != NULL)
				    xmlBufferWriteCHAR(buf, ent->orig);
			    else
				    xmlBufferWriteCHAR(buf, ent->content);
		    }
		    xmlBufferWriteChar(buf, ">\n");
		    break;
		case XML_INTERNAL_PARAMETER_ENTITY:
		    xmlBufferWriteChar(buf, "<!ENTITY % ");
		    xmlBufferWriteCHAR(buf, ent->name);
		    xmlBufferWriteChar(buf, " ");
		    if(ent->orig == NULL)
			    xmlDumpEntityContent(buf, ent->content);
		    else
			    xmlBufferWriteQuotedString(buf, ent->orig);
		    xmlBufferWriteChar(buf, ">\n");
		    break;
		case XML_EXTERNAL_PARAMETER_ENTITY:
		    xmlBufferWriteChar(buf, "<!ENTITY % ");
		    xmlBufferWriteCHAR(buf, ent->name);
		    if(ent->ExternalID != NULL) {
			    xmlBufferWriteChar(buf, " PUBLIC ");
			    xmlBufferWriteQuotedString(buf, ent->ExternalID);
			    xmlBufferWriteChar(buf, " ");
			    xmlBufferWriteQuotedString(buf, ent->SystemID);
		    }
		    else {
			    xmlBufferWriteChar(buf, " SYSTEM ");
			    xmlBufferWriteQuotedString(buf, ent->SystemID);
		    }
		    xmlBufferWriteChar(buf, ">\n");
		    break;
		default:
		    xmlEntitiesErr(XML_DTD_UNKNOWN_ENTITY, "xmlDumpEntitiesDecl: internal: unknown type entity type");
	}
}
/**
 * xmlDumpEntityDeclScan:
 * @ent:  An entity table
 * @buf:  An XML buffer.
 *
 * When using the hash table scan function, arguments need to be reversed
 */
static void xmlDumpEntityDeclScan(xmlEntityPtr ent, xmlBufferPtr buf)
{
	xmlDumpEntityDecl(buf, ent);
}
/**
 * xmlDumpEntitiesTable:
 * @buf:  An XML buffer.
 * @table:  An entity table
 *
 * This will dump the content of the entity table as an XML DTD definition
 */
void xmlDumpEntitiesTable(xmlBufferPtr buf, xmlEntitiesTablePtr table)
{
	xmlHashScan(table, (xmlHashScanner)xmlDumpEntityDeclScan, buf);
}

#endif /* LIBXML_OUTPUT_ENABLED */
#define bottom_entities
#include "elfgcchack.h"
