/******************************************************************
 *  $Id: xsd2c.h,v 1.3 2004/10/15 13:35:39 snowdrop Exp $
 *
 * CSOAP Project:  A SOAP client/server library in C
 * Copyright (C) 2003  Ferhat Ayaz
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 * 
 * Email: ayaz@jprogrammer.net
 */
#ifndef XSD2C_H
#define XSD2C_H

#include <libxml/tree.h>



#define   XSD_ALL_STR   "all"
#define   XSD_ANNOTATION_STR		"annotation"
#define   XSD_ANY_STR		""
#define   XSD_ANY_ATTRIBUTE_STR		"any"
#define   XSD_APPINFO_STR		"appInfo"
#define   XSD_ATTRIBUTE_STR		"attribute"
#define   XSD_ATTRIBUTE_GROUP_STR		"attributeGroup"
#define   XSD_CHOICE_STR		"choice"
#define   XSD_COMPLEX_TYPE_STR		"complexType"
#define   XSD_COMPLEX_CONTENT_STR		"complexContent"
#define   XSD_DOCUMENTATION_STR		"documentation"
#define   XSD_ELEMENT_STR		"element"
#define   XSD_ENUMERATION_STR		"enumeration"
#define   XSD_EXTENSION_STR		"extension"
#define   XSD_FIELD_STR		"field"
#define   XSD_GROUP_STR		"group"
#define   XSD_IMPORT_STR		"import"
#define   XSD_INCLUDE_STR		"include"
#define   XSD_KEY_STR		"key"
#define   XSD_KEYREF_STR		"keyref"
#define   XSD_LIST_STR		"list"
#define   XSD_NOTATION_STR		"notation"
#define   XSD_REDEFINE_STR		"redefine"
#define   XSD_RESTRICTION_STR		"restriction"
#define   XSD_SCHEMA_STR		"schema"
#define   XSD_SELECTOR_STR		"selector"
#define   XSD_SEQUENCE_STR		"sequence"
#define   XSD_SIMPLE_CONTENT_STR		"simpleContent"
#define   XSD_SIMPLE_TYPE_STR		"simpleType"
#define   XSD_UNION_STR		"union"
#define   XSD_UNIQUE_STR		"unique"
#define   XSD_MIN_INCLUSIVE_STR "minInclusive"
#define   XSD_MAX_INCLUSIVE_STR "maxInclusive"
#define   XSD_MIN_EXCLUSIVE_STR "minExclusive"
#define   XSD_MAX_EXCLUSIVE_STR "maxExclusive"

#define   ATTR_TYPE_STR "type"
#define   ATTR_NAME_STR "name"
#define   ATTR_BASE_STR "base"
#define   ATTR_VALUE_STR "value"
#define   ATTR_MIN_OCCURS_STR "minOccurs"
#define   ATTR_MAX_OCCURS_STR "maxOccurs"

#define   ATTR_VALUE_UNBOUNDED "unbounded"

enum _xsdAttr
{
  ATTR_UNKNOWN,
  ATTR_TYPE,
  ATTR_NAME,
  ATTR_BASE,
  ATTR_MIN_OCCURS,
  ATTR_MAX_OCCURS
};

enum _xsdKeyword 
{
  XSD_UNKNOWN,
  XSD_ALL,
  XSD_ANNOTATION, 
  XSD_ANY, 
  XSD_ANY_ATTRIBUTE, 
  XSD_APPINFO, 
  XSD_ATTRIBUTE, 
  XSD_ATTRIBUTE_GROUP, 
  XSD_CHOICE, 
  XSD_COMPLEX_TYPE, 
  XSD_COMPLEX_CONTENT, 
  XSD_DOCUMENTATION, 
  XSD_ELEMENT, 
  XSD_ENUMERATION,
  XSD_EXTENSION, 
  XSD_FIELD, 
  XSD_GROUP, 
  XSD_IMPORT, 
  XSD_INCLUDE, 
  XSD_KEY, 
  XSD_KEYREF, 
  XSD_LIST, 
  XSD_NOTATION, 
  XSD_REDEFINE, 
  XSD_RESTRICTION, 
  XSD_SCHEMA, 
  XSD_SELECTOR, 
  XSD_SEQUENCE, 
  XSD_SIMPLE_CONTENT, 
  XSD_SIMPLE_TYPE, 
  XSD_UNION, 
  XSD_UNIQUE,
  XSD_MIN_INCLUSIVE,
  XSD_MAX_INCLUSIVE,
  XSD_MIN_EXCLUSIVE,
  XSD_MAX_EXCLUSIVE,
};

typedef enum _xsdKeyword xsdKeyword; 
typedef enum _xsdAttr xsdAttr; 

int xsdEngineRun(xmlNodePtr xsdNode, const char* destDir);
void xsdSetDestDir(const char* destDir);
xmlNodePtr xsdLoadFile(const char* filename);
xmlNodePtr wsdlLoadFile(const char* filename);

#endif
