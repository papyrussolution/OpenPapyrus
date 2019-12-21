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
// @v10.6.5 #include "save.h"
/*
 * The XML predefined entities.
 */
static xmlEntity xmlEntityLt   = { 0, XML_ENTITY_DECL, reinterpret_cast<const xmlChar *>("lt"), 0, 0, 0, 0, 0, 0, reinterpret_cast<const xmlChar *>("<"), reinterpret_cast<const xmlChar *>("<"), 1, XML_INTERNAL_PREDEFINED_ENTITY, 0, 0, 0, 0, 0, 1 };
static xmlEntity xmlEntityGt   = { 0, XML_ENTITY_DECL, reinterpret_cast<const xmlChar *>("gt"), 0, 0, 0, 0, 0, 0, reinterpret_cast<const xmlChar *>(">"), reinterpret_cast<const xmlChar *>(">"), 1, XML_INTERNAL_PREDEFINED_ENTITY, 0, 0, 0, 0, 0, 1 };
static xmlEntity xmlEntityAmp  = { 0, XML_ENTITY_DECL, reinterpret_cast<const xmlChar *>("amp"), 0, 0, 0, 0, 0, 0, reinterpret_cast<const xmlChar *>("&"), reinterpret_cast<const xmlChar *>("&"), 1, XML_INTERNAL_PREDEFINED_ENTITY, 0, 0, 0, 0, 0, 1 };
static xmlEntity xmlEntityQuot = { 0, XML_ENTITY_DECL, reinterpret_cast<const xmlChar *>("quot"), 0, 0, 0, 0, 0, 0, reinterpret_cast<const xmlChar *>("\""), reinterpret_cast<const xmlChar *>("\""), 1, XML_INTERNAL_PREDEFINED_ENTITY, 0, 0, 0, 0, 0, 1 };
static xmlEntity xmlEntityApos = { 0, XML_ENTITY_DECL, reinterpret_cast<const xmlChar *>("apos"), 0, 0, 0, 0, 0, 0, reinterpret_cast<const xmlChar *>("'"), reinterpret_cast<const xmlChar *>("'"), 1, XML_INTERNAL_PREDEFINED_ENTITY, 0, 0, 0, 0, 0, 1 };
/**
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void FASTCALL xmlEntitiesErrMemory(const char * extra)
{
	__xmlSimpleError(XML_FROM_TREE, XML_ERR_NO_MEMORY, NULL, NULL, extra);
}
/**
 * @code:  the error code
 * @msg:  the message
 *
 * Handle an out of memory condition
 */
static void FASTCALL xmlEntitiesErr(xmlParserErrors code, const char * msg)
{
	__xmlSimpleError(XML_FROM_TREE, code, NULL, msg, 0);
}
/*
 * xmlFreeEntity : clean-up an entity record.
 */
static void FASTCALL xmlFreeEntity(xmlEntity * entity)
{
	if(entity) {
		xmlDict * dict = entity->doc ? entity->doc->dict : 0;
		if(entity->children && (entity->owner == 1) && (entity == (xmlEntity *)entity->children->P_ParentNode))
			xmlFreeNodeList(entity->children);
		if(dict) {
			if(entity->name && !xmlDictOwns(dict, entity->name))
				SAlloc::F((char *)entity->name);
			if(entity->ExternalID && (!xmlDictOwns(dict, entity->ExternalID)))
				SAlloc::F((char *)entity->ExternalID);
			if(entity->SystemID && (!xmlDictOwns(dict, entity->SystemID)))
				SAlloc::F((char *)entity->SystemID);
			if(entity->URI && (!xmlDictOwns(dict, entity->URI)))
				SAlloc::F((char *)entity->URI);
			if(entity->content && (!xmlDictOwns(dict, entity->content)))
				SAlloc::F((char *)entity->content);
			if(entity->orig && (!xmlDictOwns(dict, entity->orig)))
				SAlloc::F((char *)entity->orig);
		}
		else {
			SAlloc::F((char *)entity->name);
			SAlloc::F((char *)entity->ExternalID);
			SAlloc::F((char *)entity->SystemID);
			SAlloc::F((char *)entity->URI);
			SAlloc::F((char *)entity->content);
			SAlloc::F((char *)entity->orig);
		}
		SAlloc::F(entity);
	}
}
/*
 * xmlCreateEntity:
 *
 * internal routine doing the entity node strutures allocations
 */
static xmlEntity * xmlCreateEntity(xmlDict * dict, const xmlChar * name, int type, const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content) 
{
	xmlEntity * ret = static_cast<xmlEntity *>(SAlloc::M(sizeof(xmlEntity)));
	if(!ret)
		xmlEntitiesErrMemory("xmlCreateEntity: malloc failed");
	else {
		memzero(ret, sizeof(xmlEntity));
		ret->type = XML_ENTITY_DECL;
		ret->checked = 0;
		/*
		 * fill the structure.
		 */
		ret->etype = (xmlEntityType)type;
		if(!dict) {
			ret->name = sstrdup(name);
			ret->ExternalID = sstrdup(ExternalID);
			ret->SystemID = sstrdup(SystemID);
		}
		else {
			ret->name = xmlDictLookupSL(dict, name);
			if(ExternalID)
				ret->ExternalID = xmlDictLookupSL(dict, ExternalID);
			if(SystemID)
				ret->SystemID = xmlDictLookupSL(dict, SystemID);
		}
		if(content) {
			ret->length = sstrlen(content);
			ret->content = (dict && (ret->length < 5)) ? (xmlChar *)xmlDictLookup(dict, content, ret->length) : xmlStrndup(content, ret->length);
		}
		else {
			ret->length = 0;
			ret->content = NULL;
		}
		ret->URI = NULL; // to be computed by the layer knowing the defining entity 
		ret->orig = NULL;
		ret->owner = 0;
	}
	return ret;
}
/*
 * xmlAddEntity : register a new entity for an entities table.
 */
static xmlEntity * xmlAddEntity(xmlDtd * dtd, const xmlChar * name, int type,
    const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content)
{
	xmlEntity * ret = 0;
	xmlDict * dict = NULL;
	xmlEntitiesTablePtr table = NULL;
	if(name && dtd) {
		if(dtd->doc)
			dict = dtd->doc->dict;
		switch(type) {
			case XML_INTERNAL_GENERAL_ENTITY:
			case XML_EXTERNAL_GENERAL_PARSED_ENTITY:
			case XML_EXTERNAL_GENERAL_UNPARSED_ENTITY:
				SETIFZ(dtd->entities, xmlHashCreateDict(0, dict));
				table = (xmlEntitiesTable *)dtd->entities;
				break;
			case XML_INTERNAL_PARAMETER_ENTITY:
			case XML_EXTERNAL_PARAMETER_ENTITY:
				SETIFZ(dtd->pentities, xmlHashCreateDict(0, dict));
				table = (xmlEntitiesTable *)dtd->pentities;
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
xmlEntity * FASTCALL xmlGetPredefinedEntity(const xmlChar * name) 
{
	if(sstreq(name, "lt"))
		return &xmlEntityLt;
	else if(sstreq(name, "gt"))
		return &xmlEntityGt;
	else if(sstreq(name, "amp"))
		return &xmlEntityAmp;
	else if(sstreq(name, "apos"))
		return &xmlEntityApos;
	else if(sstreq(name, "quot"))
		return &xmlEntityQuot;
	else
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
xmlEntity * xmlAddDtdEntity(xmlDoc * doc, const xmlChar * name, int type,
    const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content) 
{
	xmlEntity * ret = 0;
	if(!doc)
		xmlEntitiesErr(XML_DTD_NO_DOC, "xmlAddDtdEntity: document is NULL");
	else if(doc->extSubset == NULL)
		xmlEntitiesErr(XML_DTD_NO_DTD, "xmlAddDtdEntity: document without external subset");
	else {
		xmlDtd * dtd = doc->extSubset;
		ret = xmlAddEntity(dtd, name, type, ExternalID, SystemID, content);
		if(ret) {
			// 
			// Link it to the DTD
			// 
			ret->parent = dtd;
			ret->doc = dtd->doc;
			if(!dtd->last) {
				dtd->children = dtd->last = reinterpret_cast<xmlNode *>(ret);
			}
			else {
				dtd->last->next = reinterpret_cast<xmlNode *>(ret);
				ret->prev = dtd->last;
				dtd->last = reinterpret_cast<xmlNode *>(ret);
			}
		}
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
xmlEntity * xmlAddDocEntity(xmlDoc * doc, const xmlChar * name, int type,
    const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content) 
{
	xmlEntity * ret = 0;
	if(!doc) {
		xmlEntitiesErr(XML_DTD_NO_DOC, "xmlAddDocEntity: document is NULL");
	}
	else if(doc->intSubset == NULL) {
		xmlEntitiesErr(XML_DTD_NO_DTD, "xmlAddDocEntity: document without internal subset");
	}
	else {
		xmlDtd * dtd = doc->intSubset;
		ret = xmlAddEntity(dtd, name, type, ExternalID, SystemID, content);
		if(ret) {
			// Link it to the DTD
			ret->parent = dtd;
			ret->doc = dtd->doc;
			if(dtd->last == NULL) {
				dtd->children = dtd->last = reinterpret_cast<xmlNode *>(ret);
			}
			else {
				dtd->last->next = reinterpret_cast<xmlNode *>(ret);
				ret->prev = dtd->last;
				dtd->last = reinterpret_cast<xmlNode *>(ret);
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
xmlEntity * xmlNewEntity(xmlDoc * doc, const xmlChar * name, int type, const xmlChar * ExternalID, const xmlChar * SystemID, const xmlChar * content)
{
	if(doc && doc->intSubset) {
		return xmlAddDocEntity(doc, name, type, ExternalID, SystemID, content);
	}
	else {
		xmlDict * dict = doc ? doc->dict : 0;
		xmlEntity * ret = xmlCreateEntity(dict, name, type, ExternalID, SystemID, content);
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
static xmlEntity * FASTCALL xmlGetEntityFromTable(xmlEntitiesTable * table, const xmlChar * name)
{
	return static_cast<xmlEntity *>(xmlHashLookup(table, name));
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
xmlEntity * xmlGetParameterEntity(xmlDoc * doc, const xmlChar * name)
{
	xmlEntitiesTablePtr table;
	xmlEntity * ret;
	if(doc) {
		if(doc->intSubset && doc->intSubset->pentities) {
			table = (xmlEntitiesTable *)doc->intSubset->pentities;
			ret = xmlGetEntityFromTable(table, name);
			if(ret)
				return ret;
		}
		if(doc->extSubset && doc->extSubset->pentities) {
			table = (xmlEntitiesTable *)doc->extSubset->pentities;
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
xmlEntity * xmlGetDtdEntity(xmlDoc * doc, const xmlChar * name)
{
	if(doc && doc->extSubset && doc->extSubset->entities) {
		xmlEntitiesTablePtr table = (xmlEntitiesTable *)doc->extSubset->entities;
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
xmlEntity * FASTCALL xmlGetDocEntity(const xmlDoc * doc, const xmlChar * name)
{
	if(doc) {
		if(doc->intSubset && doc->intSubset->entities) {
			xmlEntitiesTable * table = (xmlEntitiesTable *)doc->intSubset->entities;
			xmlEntity * cur = xmlGetEntityFromTable(table, name);
			if(cur)
				return cur;
		}
		if(doc->standalone != 1) {
			if(doc->extSubset && doc->extSubset->entities) {
				xmlEntitiesTable * table = (xmlEntitiesTable *)doc->extSubset->entities;
				xmlEntity * cur = xmlGetEntityFromTable(table, name);
				if(cur)
					return cur;
			}
		}
	}
	return xmlGetPredefinedEntity(name);
}
/*
 * Macro used to grow the current buffer.
 */
#define growBufferReentrant() {						\
		xmlChar * tmp;							     \
		size_t new_size = buffer_size * 2;				    \
		if(new_size < buffer_size) goto mem_error;			   \
		tmp = static_cast<xmlChar *>(SAlloc::R(buffer, new_size)); \
		if(!tmp) goto mem_error;					   \
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
static xmlChar * xmlEncodeEntitiesInternal(xmlDoc * doc, const xmlChar * input, int attr) 
{
	const xmlChar * cur = input;
	xmlChar * buffer = NULL;
	xmlChar * out = NULL;
	size_t buffer_size = 0;
	int html = 0;
	if(!input) 
		return 0;
	if(doc)
		html = (doc->type == XML_HTML_DOCUMENT_NODE);
	// 
	// allocate an translation buffer.
	// 
	buffer_size = 1000;
	buffer = static_cast<xmlChar *>(SAlloc::M(buffer_size * sizeof(xmlChar)));
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
		// 
		// By default one have to encode at least '<', '>', '"' and '&' !
		// 
		if(*cur == '<') {
			const xmlChar * end;
			/*
			 * Special handling of server side include in HTML attributes
			 */
			if(html && attr && (cur[1] == '!') && (cur[2] == '-') && (cur[3] == '-') && ((end = xmlStrstr(cur, reinterpret_cast<const xmlChar *>("-->"))) != NULL)) {
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
			// 
			// Special handling of &{...} construct from HTML 4, see
			// http://www.w3.org/TR/html401/appendix/notes.html#h-B.7.1
			// 
			if(html && attr && (cur[1] == '{') && (sstrchr((const char *)cur, '}'))) {
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
			if((doc && (doc->encoding != NULL)) || (html)) {
				/*
				 * Bjørn Reese <br@sseusa.com> provided the patch
				   xmlChar xc = (*cur & 0x3F) << 6;
				   if(cur[1] != 0) {
				    xc += *(++cur) & 0x3F;
				 **out++ = xc;
				   } else
				 */
				*out++ = *cur;
			}
			else {
				// 
				// We assume we have UTF-8 input.
				// 
				char   buf[11], * ptr;
				int    val = 0, l = 1;
				if(*cur < 0xC0) {
					xmlEntitiesErr(XML_CHECK_NOT_UTF8, "xmlEncodeEntities: input not UTF-8");
					if(doc)
						doc->encoding = sstrdup(reinterpret_cast<const xmlChar *>("ISO-8859-1"));
					snprintf(buf, sizeof(buf), "&#%d;", *cur);
					buf[sizeof(buf) - 1] = 0;
					ptr = buf;
					while(*ptr != 0) 
						*out++ = *ptr++;
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
					if(doc)
						doc->encoding = sstrdup(reinterpret_cast<const xmlChar *>("ISO-8859-1"));
					snprintf(buf, sizeof(buf), "&#%d;", *cur);
					buf[sizeof(buf) - 1] = 0;
					ptr = buf;
					while(*ptr != 0) 
						*out++ = *ptr++;
					cur++;
					continue;
				}
				/*
				 * We could do multiple things here. Just save as a char ref
				 */
				snprintf(buf, sizeof(buf), "&#x%X;", val);
				buf[sizeof(buf) - 1] = 0;
				ptr = buf;
				while(*ptr != 0) 
					*out++ = *ptr++;
				cur += l;
				continue;
			}
		}
		else if(IS_BYTE_CHAR(*cur)) {
			char buf[11], * ptr;
			snprintf(buf, sizeof(buf), "&#%d;", *cur);
			buf[sizeof(buf) - 1] = 0;
			ptr = buf;
			while(*ptr != 0) 
				*out++ = *ptr++;
		}
		cur++;
	}
	*out = 0;
	return buffer;
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
xmlChar * xmlEncodeAttributeEntities(xmlDoc * doc, const xmlChar * input)
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
xmlChar * xmlEncodeEntitiesReentrant(xmlDoc * doc, const xmlChar * input)
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
	if(!input) 
		return 0;
	/*
	 * allocate an translation buffer.
	 */
	buffer_size = 1000;
	buffer = static_cast<xmlChar *>(SAlloc::M(buffer_size * sizeof(xmlChar)));
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
	return (buffer);
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
xmlEntitiesTablePtr xmlCreateEntitiesTable() 
{
	return (xmlEntitiesTable *)xmlHashCreate(0);
}
/**
 * xmlFreeEntityWrapper:
 * @entity:  An entity
 * @name:  its name
 *
 * Deallocate the memory used by an entities in the hash table.
 */
static void xmlFreeEntityWrapper(xmlEntity * entity, const xmlChar * name ATTRIBUTE_UNUSED)
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
static xmlEntity * xmlCopyEntity(const xmlEntity * ent)
{
	xmlEntity * cur = static_cast<xmlEntity *>(SAlloc::M(sizeof(xmlEntity)));
	if(!cur) {
		xmlEntitiesErrMemory("xmlCopyEntity:: malloc failed");
	}
	else {
		memzero(cur, sizeof(xmlEntity));
		cur->type = XML_ENTITY_DECL;
		cur->etype = ent->etype;
		cur->name = sstrdup(ent->name);
		cur->ExternalID = sstrdup(ent->ExternalID);
		cur->SystemID = sstrdup(ent->SystemID);
		cur->content = sstrdup(ent->content);
		cur->orig = sstrdup(ent->orig);
		cur->URI = sstrdup(ent->URI);
	}
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
xmlEntitiesTablePtr xmlCopyEntitiesTable(const xmlEntitiesTablePtr table) 
{
	return xmlHashCopy(table, (xmlHashCopier)xmlCopyEntity);
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
static void xmlDumpEntityContent(xmlBuffer * buf, const xmlChar * content) 
{
	if(buf->alloc != XML_BUFFER_ALLOC_IMMUTABLE) {
		if(xmlStrchr(content, '%')) {
			const xmlChar * base;
			const xmlChar * cur;
			xmlBufferCCat(buf, "\"");
			base = cur = content;
			while(*cur != 0) {
				if(*cur == '"') {
					if(base != cur)
						xmlBufferAdd(buf, base, cur - base);
					xmlBufferAdd(buf, reinterpret_cast<const xmlChar *>("&quot;"), 6);
					cur++;
					base = cur;
				}
				else if(*cur == '%') {
					if(base != cur)
						xmlBufferAdd(buf, base, cur - base);
					xmlBufferAdd(buf, reinterpret_cast<const xmlChar *>("&#x25;"), 6);
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
		else
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
void xmlDumpEntityDecl(xmlBuffer * buf, xmlEntity * ent) 
{
	if(buf && ent) {
		switch(ent->etype) {
			case XML_INTERNAL_GENERAL_ENTITY:
				xmlBufferWriteChar(buf, "<!ENTITY ");
				xmlBufferWriteCHAR(buf, ent->name);
				xmlBufferWriteChar(buf, " ");
				if(ent->orig)
					xmlBufferWriteQuotedString(buf, ent->orig);
				else
					xmlDumpEntityContent(buf, ent->content);
				xmlBufferWriteChar(buf, ">\n");
				break;
			case XML_EXTERNAL_GENERAL_PARSED_ENTITY:
				xmlBufferWriteChar(buf, "<!ENTITY ");
				xmlBufferWriteCHAR(buf, ent->name);
				if(ent->ExternalID) {
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
				if(ent->ExternalID) {
					xmlBufferWriteChar(buf, " PUBLIC ");
					xmlBufferWriteQuotedString(buf, ent->ExternalID);
					xmlBufferWriteChar(buf, " ");
					xmlBufferWriteQuotedString(buf, ent->SystemID);
				}
				else {
					xmlBufferWriteChar(buf, " SYSTEM ");
					xmlBufferWriteQuotedString(buf, ent->SystemID);
				}
				if(ent->content) { /* Should be true ! */
					xmlBufferWriteChar(buf, " NDATA ");
					if(ent->orig)
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
				if(ent->ExternalID) {
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
}
/**
 * xmlDumpEntityDeclScan:
 * @ent:  An entity table
 * @buf:  An XML buffer.
 *
 * When using the hash table scan function, arguments need to be reversed
 */
static void xmlDumpEntityDeclScan(xmlEntity * ent, xmlBuffer * buf)
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
void xmlDumpEntitiesTable(xmlBuffer * buf, xmlEntitiesTablePtr table)
{
	xmlHashScan(table, (xmlHashScanner)xmlDumpEntityDeclScan, buf);
}

#endif /* LIBXML_OUTPUT_ENABLED */
#define bottom_entities
//#include "elfgcchack.h"
