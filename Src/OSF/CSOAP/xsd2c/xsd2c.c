/******************************************************************
 *  $Id: xsd2c.c,v 1.7 2004/10/15 13:35:39 snowdrop Exp $
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
 ******************************************************************/

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>


#include <stdio.h>
#include <string.h>
#include "obj.h"
#include "tr.h"
#include "formatter.h"
#include "xsd2c.h"

#include <sys/stat.h>

#define NODE_NAME_EQUALS(xmlnode, text) \
  (!xmlStrcmp(xmlnode->name, (const xmlChar *)text))

static xmlNodePtr _xmlGetChild (xmlNodePtr node);
static xmlNodePtr _xmlGetNext (xmlNodePtr node);
static HCOMPLEXTYPE xsdProcComplexType (xmlNodePtr node, const char *type);

static xsdKeyword
xsdGetKeyword (xmlNodePtr node)
{
  if (node == NULL)
    return XSD_UNKNOWN;

  if (NODE_NAME_EQUALS (node, XSD_ALL_STR))
    return XSD_ALL;
  if (NODE_NAME_EQUALS (node, XSD_ANNOTATION_STR))
    return XSD_ANNOTATION;
  if (NODE_NAME_EQUALS (node, XSD_ANY_STR))
    return XSD_ANY;
  if (NODE_NAME_EQUALS (node, XSD_ANY_ATTRIBUTE_STR))
    return XSD_ANY_ATTRIBUTE;
  if (NODE_NAME_EQUALS (node, XSD_APPINFO_STR))
    return XSD_APPINFO;
  if (NODE_NAME_EQUALS (node, XSD_ATTRIBUTE_STR))
    return XSD_ATTRIBUTE;
  if (NODE_NAME_EQUALS (node, XSD_ATTRIBUTE_GROUP_STR))
    return XSD_ATTRIBUTE_GROUP;
  if (NODE_NAME_EQUALS (node, XSD_CHOICE_STR))
    return XSD_CHOICE;
  if (NODE_NAME_EQUALS (node, XSD_COMPLEX_TYPE_STR))
    return XSD_COMPLEX_TYPE;
  if (NODE_NAME_EQUALS (node, XSD_COMPLEX_CONTENT_STR))
    return XSD_COMPLEX_CONTENT;
  if (NODE_NAME_EQUALS (node, XSD_DOCUMENTATION_STR))
    return XSD_DOCUMENTATION;
  if (NODE_NAME_EQUALS (node, XSD_ELEMENT_STR))
    return XSD_ELEMENT;
  if (NODE_NAME_EQUALS (node, XSD_ENUMERATION_STR))
    return XSD_ENUMERATION;
  if (NODE_NAME_EQUALS (node, XSD_EXTENSION_STR))
    return XSD_EXTENSION;
  if (NODE_NAME_EQUALS (node, XSD_FIELD_STR))
    return XSD_FIELD;
  if (NODE_NAME_EQUALS (node, XSD_GROUP_STR))
    return XSD_GROUP;
  if (NODE_NAME_EQUALS (node, XSD_IMPORT_STR))
    return XSD_IMPORT;
  if (NODE_NAME_EQUALS (node, XSD_INCLUDE_STR))
    return XSD_INCLUDE;
  if (NODE_NAME_EQUALS (node, XSD_KEY_STR))
    return XSD_KEY;
  if (NODE_NAME_EQUALS (node, XSD_KEYREF_STR))
    return XSD_KEYREF;
  if (NODE_NAME_EQUALS (node, XSD_LIST_STR))
    return XSD_LIST;
  if (NODE_NAME_EQUALS (node, XSD_NOTATION_STR))
    return XSD_NOTATION;
  if (NODE_NAME_EQUALS (node, XSD_REDEFINE_STR))
    return XSD_REDEFINE;
  if (NODE_NAME_EQUALS (node, XSD_RESTRICTION_STR))
    return XSD_RESTRICTION;
  if (NODE_NAME_EQUALS (node, XSD_SCHEMA_STR))
    return XSD_SCHEMA;
  if (NODE_NAME_EQUALS (node, XSD_SELECTOR_STR))
    return XSD_SELECTOR;
  if (NODE_NAME_EQUALS (node, XSD_SEQUENCE_STR))
    return XSD_SEQUENCE;
  if (NODE_NAME_EQUALS (node, XSD_SIMPLE_CONTENT_STR))
    return XSD_SIMPLE_CONTENT;
  if (NODE_NAME_EQUALS (node, XSD_SIMPLE_TYPE_STR))
    return XSD_SIMPLE_TYPE;
  if (NODE_NAME_EQUALS (node, XSD_UNION_STR))
    return XSD_UNION;
  if (NODE_NAME_EQUALS (node, XSD_UNIQUE_STR))
    return XSD_UNIQUE;
  if (NODE_NAME_EQUALS (node, XSD_MIN_INCLUSIVE_STR))
    return XSD_MIN_INCLUSIVE;
  if (NODE_NAME_EQUALS (node, XSD_MAX_INCLUSIVE_STR))
    return XSD_MAX_INCLUSIVE;
  if (NODE_NAME_EQUALS (node, XSD_MIN_EXCLUSIVE_STR))
    return XSD_MIN_EXCLUSIVE;
  if (NODE_NAME_EQUALS (node, XSD_MAX_EXCLUSIVE_STR))
    return XSD_MAX_EXCLUSIVE;

  return XSD_UNKNOWN;
}


static char outDir[1054];




static xmlNodePtr
xmlFindSubElement (xmlNodePtr root, const char *element_name)
{
  xmlNodePtr cur;

  cur = root->xmlChildrenNode;
  while (cur != NULL)
  {

    if (cur->type != XML_ELEMENT_NODE)
    {
      cur = cur->next;
      continue;
    }

    if (!xmlStrcmp (cur->name, (const xmlChar *) element_name))
    {
      return cur;
    }

    cur = cur->next;
  }

  return NULL;
}

xmlNodePtr
xsdLoadFile (const char *filename)
{
  xmlDocPtr doc;
  xmlNodePtr cur;

  doc = xmlParseFile (filename);
  if (doc == NULL)
    return NULL;

  cur = xmlDocGetRootElement (doc);

  return cur;
}


xmlNodePtr
wsdlLoadFile (const char *filename)
{
  xmlDocPtr doc;
  xmlNodePtr cur;
  xmlNodePtr sub;
  xsdKeyword keyword;

  doc = xmlParseFile (filename);
  if (doc == NULL)
    return NULL;

  cur = xmlDocGetRootElement (doc);
  if (cur == NULL)
  {
    return NULL;
  }

  cur = xmlFindSubElement (cur, "types");
  if (cur == NULL)
    return NULL;
/*
  sub = xmlFindSubElement(cur, "schema");
  if (sub != NULL)
    return sub;
*/
  /* some wsdl's defines xsd without root <schema> element */
  sub = _xmlGetChild (cur);
  keyword = xsdGetKeyword (sub);
  switch (keyword)
  {
  case XSD_ELEMENT:
  case XSD_COMPLEX_TYPE:
  case XSD_SIMPLE_TYPE:
  case XSD_SCHEMA:
    return sub;
  default:
    fprintf (stderr, "Unexpected node: '%s'\n", cur->name);
  }

  return NULL;
}


static xmlNodePtr
_xmlGetChild (xmlNodePtr node)
{
  xmlNodePtr cur = NULL;
  cur = node->xmlChildrenNode;
  while (cur != NULL)
  {
    if (cur->type != XML_ELEMENT_NODE)
    {
      cur = cur->next;
      continue;
    }
    return cur;
  }
  return cur;
}

static xmlNodePtr
_xmlGetNext (xmlNodePtr node)
{
  xmlNodePtr cur = NULL;
  cur = node->next;
  while (cur != NULL)
  {
    if (cur->type != XML_ELEMENT_NODE)
    {
      cur = cur->next;
      continue;
    }
    return cur;
  }
  return cur;
}

static void
xsdProcAttribute (HCOMPLEXTYPE parent, xmlNodePtr node)
{
  char *name, *type;
/*  xmlNodePtr cur;
  char buffer[1054];
  */
  name = xmlGetProp (node, ATTR_NAME_STR);
  type = xmlGetProp (node, ATTR_TYPE_STR);

  /* printf("  %s: %s\n", type?type:"(null)",
     name?name:"(null)");
   */
  if (name == NULL)
  {
    fprintf (stderr, "WARNING: Attribute without name!\n");
    return;
  }

  if (type == NULL)
  {
    fprintf (stderr, "WARNING: Attribute '%s' has no type\n", name);
  }

/*  sprintf(buffer, "attr_%s", name); */

  objAddAttribute (parent, name, type, 0);
}

static void
xsdProcElement (HCOMPLEXTYPE parent, xmlNodePtr node)
{
  char *name, *type, *minostr, *maxostr;
  xmlNodePtr cur;
  xsdKeyword keyword;
  char buffer[1054];
  HCOMPLEXTYPE ct;
  int mino, maxo;

  name = xmlGetProp (node, ATTR_NAME_STR);
  type = xmlGetProp (node, ATTR_TYPE_STR);
  minostr = xmlGetProp (node, ATTR_MIN_OCCURS_STR);
  maxostr = xmlGetProp (node, ATTR_MAX_OCCURS_STR);

/*  printf("  %s: %s\n", type?type:"(null)",
    name?name:"(null)");
*/
  if (minostr == NULL)
    mino = 1;
  else
    mino = atoi (minostr);

  if (maxostr == NULL)
    maxo = 1;
  else
  {
    if (!strcmp (maxostr, ATTR_VALUE_UNBOUNDED))
      maxo = -1;
    else
      maxo = atoi (maxostr);
  }


  if (type == NULL)
  {
    /* check for complexType */
    cur = _xmlGetChild (node);
    if (cur == NULL)
    {
      fprintf (stderr, "WARNING: Element '%s' has no childs\n", name);
      return;
    }

    do
    {
      keyword = xsdGetKeyword (cur);

      switch (keyword)
      {
      case XSD_COMPLEX_TYPE:
        /*
           type = xmlGetProp(cur, ATTR_NAME_STR);
           if (type == NULL) 
           {
           fprintf(stderr, "WARNING: Type name not found\n");
           break;
           }
         */

        sprintf (buffer, "%s_%s", parent->type, (const char *) name);
        ct = xsdProcComplexType (cur, (const char *) buffer);
        if (ct != NULL)
        {
          objAddElement (parent, name, buffer, 0, mino, maxo);
        }
        break;

      default:
        fprintf (stderr, "Unexpected node: '%s'\n", cur->name);
      }

    } while ((cur = _xmlGetNext (cur)) != NULL);
  }
  else
  {
    objAddElement (parent, name, type, 0, mino, maxo);
  }

/*  if (name) xmlFree(name);
  if (type) xmlFree(type);*/
}

static void
xsdProcSequence (HCOMPLEXTYPE ct, xmlNodePtr node)
{
  xmlNodePtr cur = NULL;
  xsdKeyword keyword;

  cur = _xmlGetChild (node);
  if (cur == NULL)
  {
    fprintf (stderr, "WARNING: Empty sequence\n");
    return;
  }

  do
  {
    keyword = xsdGetKeyword (cur);

    switch (keyword)
    {
    case XSD_ANNOTATION:
      /* nothing to do */
      break;

    case XSD_GROUP:
      fprintf (stderr, "WARNING: %s not supported\n", XSD_GROUP_STR);
      break;

    case XSD_CHOICE:
      fprintf (stderr, "WARNING: %s not supported\n", XSD_CHOICE_STR);
      break;

    case XSD_SEQUENCE:
      fprintf (stderr, "WARNING: %s not supported\n", XSD_SEQUENCE_STR);
      break;

    case XSD_ANY:
      fprintf (stderr, "WARNING: %s not supported\n", XSD_ANY_STR);
      break;

    case XSD_ELEMENT:
      xsdProcElement (ct, cur);
      break;

    default:
      fprintf (stderr, "WARNING: Unknown child ('%s')!\n",
               (char *) cur->name);
    };
  } while ((cur = _xmlGetNext (cur)) != NULL);
}


static void
xsdProcExtension (HCOMPLEXTYPE ct, xmlNodePtr node, const char *type)
{
  xmlNodePtr cur = NULL;
  xsdKeyword keyword;
  char *base;


  base = xmlGetProp (node, ATTR_BASE_STR);
  if (base == NULL)
  {
    fprintf (stderr, "WARNING: No base defined\n");
    return;
  }

  printf (" =[Base] -> %s\n", base);
  objSetBaseType (ct, base);
/*  xmlFree(base);*/

  cur = _xmlGetChild (node);
  if (cur == NULL)
  {
    fprintf (stderr, "WARNING: Empty node\n");
    return;
  }

  do
  {
    keyword = xsdGetKeyword (cur);

    switch (keyword)
    {
    case XSD_ANNOTATION:
      /* nothing to do */
      break;

    case XSD_ALL:
      fprintf (stderr, " WARNING: %s not supported\n", XSD_ALL_STR);
      break;

    case XSD_GROUP:
      fprintf (stderr, "WARNING: %s not supported\n", XSD_GROUP_STR);
      break;

    case XSD_CHOICE:
      fprintf (stderr, "WARNING: %s not supported\n", XSD_CHOICE_STR);
      break;

    case XSD_ATTRIBUTE:
      xsdProcAttribute (ct, cur);
      break;

    case XSD_ATTRIBUTE_GROUP:
      fprintf (stderr, "WARNING: %s not supported\n",
               XSD_ATTRIBUTE_GROUP_STR);
      break;

    case XSD_ANY_ATTRIBUTE:
      fprintf (stderr, "WARNING: %s not supported\n", XSD_ANY_ATTRIBUTE_STR);
      break;

    case XSD_SEQUENCE:
      xsdProcSequence (ct, cur);
      break;

    default:
      fprintf (stderr, "WARNING: Unknown child ('%s')!\n",
               (char *) cur->name);
    };

  } while ((cur = _xmlGetNext (cur)) != NULL);
}


static void
xsdProcComplexContent (HCOMPLEXTYPE ct, xmlNodePtr node, const char *type)
{
  xmlNodePtr cur = NULL;
  xsdKeyword keyword;

  cur = _xmlGetChild (node);
  if (cur == NULL)
  {
    fprintf (stderr, "WARNING: Empty sequence\n");
    return;
  }

  do
  {
    keyword = xsdGetKeyword (cur);

    switch (keyword)
    {
    case XSD_ANNOTATION:
      /* nothing to do */
      break;

    case XSD_EXTENSION:
      xsdProcExtension (ct, cur, type);
      break;

    case XSD_RESTRICTION:
      fprintf (stderr, "WARNING: %s not supported\n", XSD_RESTRICTION_STR);
      break;

    default:
      fprintf (stderr, "WARNING: Unknown child ('%s')!\n",
               (char *) cur->name);
    };

  } while ((cur = _xmlGetNext (cur)) != NULL);
}

void
xsdProcRestriction(HCOMPLEXTYPE ct, xmlNodePtr node, const char *type)
{
  xmlNodePtr cur = NULL;
  xsdKeyword keyword;
  char *value;
  HRESTRICTION res;
  char *base;


  base = xmlGetProp (node, ATTR_BASE_STR);
  if (base == NULL)
  {
    fprintf (stderr, "WARNING: No base defined\n");
    return;
  }

  printf (" =[Base] -> %s\n", base);
  res = resCreate(base);
  ct->restriction = res;

/*  xmlFree(base);*/

  cur = _xmlGetChild (node);
  if (cur == NULL)
  {
    fprintf (stderr, "WARNING: Empty sequence\n");
    return;
  }

  do
  {
    value = xmlGetProp (cur, ATTR_VALUE_STR);
    if (!value) {
      fprintf(stderr, "WARNING: Found SimpleContent->%s without attribute value.\n", 
        cur->name);
      continue;
    }

    keyword = xsdGetKeyword (cur);

    switch (keyword)
    {
    case XSD_MIN_INCLUSIVE:
      res->minInclusive = atoi(value);
      res->minInclusiveSet = 1;
      res->mode = RES_MODE_MINMAX;
      break;

    case XSD_MAX_INCLUSIVE:
      res->maxInclusive = atoi(value);
      res->maxInclusiveSet = 1;
      res->mode = RES_MODE_MINMAX;
      break;

    case XSD_MIN_EXCLUSIVE:
      res->minExclusive = atoi(value);
      res->minExclusiveSet = 1;
      res->mode = RES_MODE_MINMAX;
      break;

    case XSD_MAX_EXCLUSIVE:
      res->maxExclusive = atoi(value);
      res->maxExclusiveSet = 1;
      res->mode = RES_MODE_MINMAX;
      break;

    case XSD_ENUMERATION:
      Enumeration_Add_value(res->enumeration, value);
      res->mode = RES_MODE_ENUMERATION;
    default:
      fprintf (stderr, "WARNING: Unknown child (SimpleContent->'%s')!\n",
               (char *) cur->name);
    };

  } while ((cur = _xmlGetNext (cur)) != NULL);

}

void
xsdProcSimpleContent (HCOMPLEXTYPE ct, xmlNodePtr node, const char *type)
{
  xmlNodePtr cur = NULL;
  xsdKeyword keyword;

  ct->isSimpleContent = 1;

  cur = _xmlGetChild (node);
  if (cur == NULL)
  {
    fprintf (stderr, "WARNING: Empty node\n");
    return;
  }

  do
  {
    keyword = xsdGetKeyword (cur);

    switch (keyword)
    {
    case XSD_ANNOTATION:
      /* nothing to do */
      break;

    case XSD_RESTRICTION:
      xsdProcRestriction(ct, cur, type);
      break;

    case XSD_ATTRIBUTE:
      xsdProcAttribute (ct, cur);
      break;

    default:
      fprintf (stderr, "WARNING: Unknown child (SimpleContent->'%s')!\n",
               (char *) cur->name);
    };

  } while ((cur = _xmlGetNext (cur)) != NULL);
}

static HCOMPLEXTYPE
xsdProcComplexType (xmlNodePtr node, const char *type)
{
  char *name;
  xmlNodePtr cur = NULL;
  xsdKeyword keyword;
  HCOMPLEXTYPE ct;


  if (!type)
    name = xmlGetProp (node, ATTR_NAME_STR);
  else
  {
    name = (char *) malloc (strlen (type) + 1);
    strcpy (name, type);
  }

  if (!name)
  {
    fprintf (stderr, "\nWARNING: complexType has no typename!\n");
    return NULL;
  }

  ct = objCreateComplexType (name);

  printf ("\ncomplexType->%s\n", name);

  cur = _xmlGetChild (node);
  if (cur == NULL)
  {
    fprintf (stderr, "WARNING: Empty complexType\n");
    return ct;
  }
 


  do
  {
    keyword = xsdGetKeyword (cur);

    switch (keyword)
    {
    case XSD_ANNOTATION:
      /* nothing to do */
      break;

    case XSD_SIMPLE_CONTENT:
      xsdProcSimpleContent (ct, cur, name);
      break;

    case XSD_COMPLEX_CONTENT:
      xsdProcComplexContent (ct, cur, name);
      /*  fprintf(stderr, "WARNING: %s not supported\n", XSD_COMPLEX_CONTENT_STR); */
      break;

    case XSD_ALL:
      fprintf (stderr, "WARNING: %s not supported\n", XSD_ALL_STR);
      break;

    case XSD_GROUP:
      fprintf (stderr, "WARNING: %s not supported\n", XSD_GROUP_STR);
      break;

    case XSD_CHOICE:
      fprintf (stderr, "WARNING: %s not supported\n", XSD_CHOICE_STR);
      break;

    case XSD_ATTRIBUTE:
      xsdProcAttribute (ct, cur);
      break;

    case XSD_ATTRIBUTE_GROUP:
      fprintf (stderr, "WARNING: %s not supported\n",
               XSD_ATTRIBUTE_GROUP_STR);
      break;

    case XSD_ANY_ATTRIBUTE:
      fprintf (stderr, "WARNING: %s not supported\n", XSD_ANY_ATTRIBUTE_STR);
      break;

    case XSD_SEQUENCE:
      xsdProcSequence (ct, cur);
      break;

    default:
      fprintf (stderr, "WARNING: Unknown child ('%s')!\n",
               (char *) cur->name);
    };

  } while ((cur = _xmlGetNext (cur)) != NULL);

/*  xmlFree(name);*/
  return ct;
}



static void
runGenerator (xmlNodePtr xsdRoot)
{
  xmlNodePtr cur;
  xmlNodePtr node;
  xmlChar *type;

  cur = xsdRoot->xmlChildrenNode;
  while (cur != NULL)
  {

    if (cur->type != XML_ELEMENT_NODE)
    {
      cur = cur->next;
      continue;
    }

    if (xsdGetKeyword (cur) == XSD_COMPLEX_TYPE)
    {

      xsdProcComplexType (cur, NULL);

    }
    else if (xsdGetKeyword (cur) == XSD_SIMPLE_TYPE)
    {

      fprintf (stderr, "WARNING: SimpleType not supported!\n");

    }
    else if (xsdGetKeyword (cur) == XSD_ELEMENT)
    {

      type = xmlGetProp (cur, "name");
      if (type == NULL)
      {
        fprintf (stderr, "WARNING: Element found without name  ('%s')\n",
                 cur->name);
      }
      else
      {

        node = xmlFindSubElement (cur, XSD_COMPLEX_TYPE_STR);
        if (node != NULL)
        {
          xsdProcComplexType (node, type);
        }
        else
        {
          /* fprintf (stderr, "WARNING: Element on root node will be ignored\n"); */
          /* TODO: I don't know if this is a good idea!? 
          make typedef instead.*/
          xsdProcComplexType(cur, type);
        }
      }
      /*xsdProcElement(..., cur); */
    }
    else
    {
      fprintf(stderr, "WARNING: '%s' not supported!\n", cur->name);
    }

    cur = cur->next;
  }
}

int
declareStructs (HCOMPLEXTYPE ct)
{
  char fname[255];
  FILE *f;

  sprintf (fname, "%s/%s.h", outDir, ct->type); /* _xsd */
  printf ("Generating file '%s' ...\n", fname);
  f = fopen (fname, "w");
  if (f == NULL)
  {
    fprintf (stderr, "Can not open '%s'\n", fname);
    return 0;
  }

  writeComplexTypeHeaderFile (f, ct);
  fclose (f);

  return 1;
}

int
writeSource (HCOMPLEXTYPE ct)
{
  char fname[255];
  FILE *f;

  sprintf (fname, "%s/%s.c", outDir, ct->type); /* _xsd */
  printf ("Generating file '%s' ...\n", fname);
  f = fopen (fname, "w");
  if (f == NULL)
  {
    fprintf (stderr, "Can not open '%s'\n", fname);
    return 0;
  }

  writeComplexTypeSourceFile (f, ct);
  fclose (f);

  return 1;
}


int
xsdInitTrModule (xmlNodePtr xsdNode)
{
  xmlNsPtr ns = NULL;
  char xsd_ns[50];

  ns =
    xmlSearchNsByHref (xsdNode->doc, xsdNode,
                       "http://www.w3.org/2001/XMLSchema");
  if (ns == NULL)
    ns =
      xmlSearchNsByHref (xsdNode->doc, xsdNode,
                         "http://www.w3.org/2001/XMLSchema/");
  if (ns == NULL)
    ns =
      xmlSearchNsByHref (xsdNode->doc, xsdNode,
                         "http://www.w3.org/1999/XMLSchema");
  if (ns == NULL)
    ns =
      xmlSearchNsByHref (xsdNode->doc, xsdNode,
                         "http://www.w3.org/1999/XMLSchema/");
  if (ns == NULL)
    ns =
      xmlSearchNsByHref (xsdNode->doc, xmlDocGetRootElement (xsdNode->doc),
                         "http://www.w3.org/2001/XMLSchema");
  if (ns == NULL)
    ns =
      xmlSearchNsByHref (xsdNode->doc, xmlDocGetRootElement (xsdNode->doc),
                         "http://www.w3.org/2001/XMLSchema/");
  if (ns == NULL)
    ns =
      xmlSearchNsByHref (xsdNode->doc, xmlDocGetRootElement (xsdNode->doc),
                         "http://www.w3.org/1999/XMLSchema");
  if (ns == NULL)
    ns =
      xmlSearchNsByHref (xsdNode->doc, xmlDocGetRootElement (xsdNode->doc),
                         "http://www.w3.org/1999/XMLSchema/");

/*
    if (ns != NULL && ns->prefix != NULL) {
        fprintf(stdout, "XMLSchema namespace prefix: '%s'\n", ns->prefix);
        trInitModule(ns->prefix);
    } else { */
  /* 
     Search for:
     <definitions xmlns:xsd="http://www.w3.org/2001/XMLSchema">
     <type>
     <schema xmlns="http://www.w3.org/2001/XMLSchema"> 
     ...
   */
  /*if (ns != NULL && ns->prefix != NULL) {
     fprintf(stdout, "XMLSchema namespace prefix: '%s'\n", ns->prefix);
     trInitModule(ns->prefix);
     } else { 
     printf("Initializing XML Schema type register with default 'xs'\n");
     trInitModule("xs");
     }
     }

     } else {
     printf("Initializing XML Schema type register with default 'xs'\n");
     trInitModule("xs");
     }
   */
  if (ns != NULL && ns->prefix != NULL)
  {
    strcpy (xsd_ns, ns->prefix);
  }
  else
  {
    fprintf (stderr, "WARNING: using XML Schema prefix 'xsd' as default!\n");
    strcpy (xsd_ns, "xsd");
  }

  trInitModule ();

  fprintf(stdout, "XML Schema prefix: '%s'\n", xsd_ns);

  trRegisterTypeNS (xsd_ns, "ID", "char*", 1);
  trRegisterTypeNS (xsd_ns, "IDREF", "char*", 1);
  trRegisterTypeNS (xsd_ns, "IDREFS", "char*", 1);
  trRegisterTypeNS (xsd_ns, "string", "char*", 1);
  trRegisterTypeNS (xsd_ns, "integer", "int", 1);
  trRegisterTypeNS (xsd_ns, "int", "int", 1);
  trRegisterTypeNS (xsd_ns, "double", "double", 1);
  trRegisterTypeNS (xsd_ns, "float", "float", 1);
  trRegisterTypeNS (xsd_ns, "boolean", "int", 1);

  ns =  xmlSearchNsByHref (xsdNode->doc, xsdNode, "http://www.w3.org/2003/05/soap-encoding");
  if (ns == NULL)
  ns =  xmlSearchNsByHref (xsdNode->doc, xsdNode, "http://www.w3.org/2003/05/soap-encoding/");
  if (ns != NULL && ns->prefix != NULL) {
    strcpy(xsd_ns, ns->prefix);
    trRegisterTypeNS (xsd_ns, "ID", "char*", 1);
    trRegisterTypeNS (xsd_ns, "IDREF", "char*", 1);
    trRegisterTypeNS (xsd_ns, "IDREFS", "char*", 1);
    trRegisterTypeNS (xsd_ns, "string", "char*", 1);
    trRegisterTypeNS (xsd_ns, "integer", "int", 1);
    trRegisterTypeNS (xsd_ns, "int", "int", 1);
    trRegisterTypeNS (xsd_ns, "double", "double", 1);
    trRegisterTypeNS (xsd_ns, "float", "float", 1);
    trRegisterTypeNS (xsd_ns, "boolean", "int", 1);
  }
   
  return 1;
}


int
xsdInitObjModule (xmlNodePtr xsdNode)
{
  xmlChar *tns = NULL;
  xmlNsPtr ns;

  if (xsdNode != NULL)
    tns = xmlGetProp (xsdNode, (const xmlChar *) "targetNamespace");

  if (tns == NULL)
  {

    objInitModule (NULL);

  }
  else
  {

    ns = xmlSearchNsByHref (xsdNode->doc, xsdNode, tns);
    if (ns == NULL)
    {
      fprintf (stderr, "WARNING: Target namespace not found!\n");
      return 0;
    }

    if (ns->prefix == NULL)
    {
      fprintf (stderr, "WARNING: Target namespace not found!\n");
      return 0;
    }

    fprintf (stdout, "Target namespace ('%s') prefix: '%s'\n", tns,
             ns->prefix);
    objInitModule (ns->prefix);

  }


  return 1;
}

void
xsdSetDestDir (const char *destDir)
{
  strcpy (outDir, destDir);

#ifdef __MINGW32__
  mkdir (destDir);
#else
  mkdir (destDir, S_IRUSR | S_IWUSR | S_IXUSR |
         S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
}

int
xsdEngineRun (xmlNodePtr xsdNode, const char *destDir)
{


  xsdSetDestDir (destDir);

  if (xsdNode != NULL)
  {
    runGenerator (xsdNode);
    objRegistryEnumComplexType (declareStructs);
    objRegistryEnumComplexType (writeSource);
  }


  return 0;
}
