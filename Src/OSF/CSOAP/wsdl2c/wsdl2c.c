/******************************************************************
 *  $Id: wsdl2c.c,v 1.9 2005/05/27 19:28:16 snowdrop Exp $
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
#include <libxml/xpath.h>
#include <xsd2c/xsd2c.h>
#include <xsd2c/util.h> /* parseNS */
#include <xsd2c/formatter.h> /* formatter sax serializer */
#include <xsd2c/obj.h> /* formatter sax serializer */
#include <string.h>

#define _DESERIALIZER_DISABLE_
#include "CallList.h"

/* ------------- Just for test --------------------------------- */

int Writer_Tab = 0;
int Writer_Ret = 0;

void Writer_StartElement(const char* element_name, int attr_count, char **keys, char **values, void* userData)
{
  int i, j;
  if (Writer_Ret) printf("\n");
  for (j=0;j<Writer_Tab;j++) printf("\t");
  printf("<%s", element_name);
  for (i=0;i<attr_count;i++)
  {
    printf(" %s = \"%s\"", keys[i], values[i]);
  }
  printf(">");
  Writer_Tab++;
  Writer_Ret = 1;
}

void Writer_Characters(const char* element_name, const char* chars, void* userData)
{
  printf("%s", chars!=NULL?chars:"null");
}

void Writer_EndElement(const char* element_name, void* userData)
{
  int j;
  Writer_Tab--;
  if (!Writer_Ret)
    for (j=0;j<Writer_Tab;j++) printf("\t");
  printf("</%s>\n", element_name);
  Writer_Ret = 0;
}

/* ------------------------------------------------------------- */

struct CallList* callList = NULL;
char soap_prefix[50];
char wsdl_prefix[50];
char target_ns[150];
char target_prefix[50];

xmlXPathObjectPtr xpath_eval(xmlDocPtr doc, const char *xpath)
{
  xmlXPathContextPtr context;
  xmlXPathObjectPtr result;
  xmlNodePtr node;
  xmlNsPtr default_ns;

  node = xmlDocGetRootElement(doc);
  if (node == NULL) {
    fprintf(stderr,"empty document\n");
    return NULL;
  }

  
  context = xmlXPathNewContext(doc);

  context->namespaces = xmlGetNsList(doc, node);
  context->nsNr = 0;
  if (context->namespaces != NULL) {
     while (context->namespaces[context->nsNr] != NULL) {
/*      fprintf(stdout, "NS: '%s' - '%s'\n", context->namespaces[context->nsNr]->href, 
  context->namespaces[context->nsNr]->prefix); */
      context->nsNr++;
     }
  }

  /* search default namespace */
  default_ns = xmlSearchNs(doc, node, NULL);
  if (default_ns != NULL && default_ns->href != NULL) {
    fprintf(stdout, "Register default namespace '%s'\n", (const char*)default_ns->href);
    /* default_ns : "http://schemas.xmlsoap.org/wsdl/" */
    xmlXPathRegisterNs(context, "wsdl", default_ns->href );
  } 
  

  printf("XPath: '%s'\n", xpath);
  result = xmlXPathEvalExpression((xmlChar*)xpath, context);  
  if (result == NULL) {
    fprintf(stderr, "ERROR: XPath error!\n");
    return NULL;
  }

  if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
    /* no result */
    printf("XPath result is empty\n");
    return NULL;
  }

  xmlXPathFreeContext(context);
  return result;  
}

xmlNodePtr findPortType(xmlDocPtr doc, const char *name)
{
  xmlNodePtr root;
  xmlNodePtr cur;
  xmlChar *attr_name;

  root = xmlDocGetRootElement(doc);
  if (root == NULL) {
    fprintf(stderr, "Empty document!\n");
    return NULL;
  }

  cur = root->xmlChildrenNode;

  while (cur != NULL) {
    
    if (cur->type != XML_ELEMENT_NODE) {
      cur = cur->next;
      continue;
    }

    if (xmlStrcmp(cur->name, (const xmlChar*)"portType")) {
      cur = cur->next;
      continue;
    }

    attr_name = xmlGetProp(cur, "name");  
    if (attr_name == NULL) {
      cur = cur->next;
      continue;
    }

    if (!xmlStrcmp(attr_name, (const xmlChar*)name)) {
      /*xmlFree(attr_name);*/
      return cur;
    }
    
    /*xmlFree(attr_name);*/
    cur = cur->next;
  }

  return NULL;
}


xmlNodePtr findMessage(xmlDocPtr doc, const char *name)
{
  xmlNodePtr root;
  xmlNodePtr cur;
  xmlChar *attr_name;

  root = xmlDocGetRootElement(doc);
  if (root == NULL) {
    fprintf(stderr, "Empty document!\n");
    return NULL;
  }

  cur = root->xmlChildrenNode;

  while (cur != NULL) {
    
    if (cur->type != XML_ELEMENT_NODE) {
      cur = cur->next;
      continue;
    }

    if (xmlStrcmp(cur->name, (const xmlChar*)"message")) {
      cur = cur->next;
      continue;
    }

    attr_name = xmlGetProp(cur, "name");  
    if (attr_name == NULL) {
      cur = cur->next;
      continue;
    }

    if (!xmlStrcmp(attr_name, (const xmlChar*)name)) {
      /*xmlFree(attr_name);*/
      return cur;
    }
    
    /*xmlFree(attr_name);*/
    cur = cur->next;
  }

  return NULL;
}


xmlNodePtr findSubNode(xmlNodePtr root, const char *element_name)
{
  xmlNodePtr cur;

  cur = root->xmlChildrenNode;

  while (cur != NULL) {
    
    if (cur->type != XML_ELEMENT_NODE) {
      cur = cur->next;
      continue;
    }

    if (!xmlStrcmp(cur->name, (const xmlChar*)element_name)) {
      return cur;
    }

    cur = cur->next;
  }

  return NULL;
}


void handleInputParameters(xmlDocPtr doc, const char *name, struct CallFunc *func)
{
  char ns[10];
  char message_name[255];
  xmlNodePtr node;
  xmlNodePtr cur;
  struct CallVar *var;
  xmlChar *var_name, *var_type;

  parseNS(name, ns, message_name); /* check why to pase ns? */

  node = findMessage(doc, message_name);
  if (node == NULL) {
    fprintf(stderr, "ERROR: Can not find message '%s'\n", message_name);
    return;
  }

  cur = node->xmlChildrenNode;
  while (cur != NULL) {
    
    if (cur->type != XML_ELEMENT_NODE) {
      cur = cur->next;
      continue;
    }

    if (xmlStrcmp(cur->name, (const xmlChar*)"part")) {
      cur = cur->next;
      continue;
    }

    var_name = xmlGetProp(cur, "name");  
    if (var_name == NULL) {
      fprintf(stderr, "ERROR: Found <part> without attrbute 'name'! (message:'%s')\n", 
        message_name);
      cur = cur->next;
      continue;
    }

    var_type = xmlGetProp(cur, "type");  
    if (var_type == NULL) {
      var_type = xmlGetProp(cur, "element");  
    } 

    if (var_type == NULL) {
      fprintf(stderr, "ERROR: Found <part> without attrbute 'type' or 'element'! (message:'%s')\n", 
        message_name);
      /*xmlFree(var_name);*/
      cur = cur->next;
      continue;
    } 

    var = CallVar_Create();
    CallVar_Set_name(var, (const char*)var_name);
    CallVar_Set_type(var, (const char*)var_type);
    CallFunc_Add_in(func, var);

    fprintf(stdout, "\t\t(%s,%s)\n", trXSD2C((const char*)var_type), (const char*)var_name);

    /*xmlFree(var_name);
    xmlFree(var_type);*/

    cur = cur->next;
  }

}

void handleOutputParameters(xmlDocPtr doc, const char *name, struct CallFunc *func)
{
  char ns[10];
  char message_name[255];
  xmlNodePtr node;
  xmlNodePtr cur;
  struct CallVar *var;
  xmlChar *var_name, *var_type;

  parseNS(name, ns, message_name); /* check why to pase ns? */

  node = findMessage(doc, message_name);
  if (node == NULL) {
    fprintf(stderr, "ERROR: Can not find message '%s'\n", message_name);
    return;
  }

  cur = node->xmlChildrenNode;
  while (cur != NULL) {
    
    if (cur->type != XML_ELEMENT_NODE) {
      cur = cur->next;
      continue;
    }

    if (xmlStrcmp(cur->name, (const xmlChar*)"part")) {
      cur = cur->next;
      continue;
    }

    var_name = xmlGetProp(cur, "name");  
    if (var_name == NULL) {
      fprintf(stderr, "ERROR: Found <part> without attrbute 'name'! (message:'%s')\n", 
        message_name);
      cur = cur->next;
      continue;
    }

    var_type = xmlGetProp(cur, "type");  
    if (var_type == NULL) {
      var_type = xmlGetProp(cur, "element");  
    } 

    if (var_type == NULL) {
      fprintf(stderr, "ERROR: Found <part> without attrbute 'type' or 'element'! (message:'%s')\n", 
        message_name);
      /*xmlFree(var_name);*/
      cur = cur->next;
      continue;
    } 

    var = CallVar_Create();
    CallVar_Set_name(var, (const char*)var_name);
    CallVar_Set_type(var, (const char*)var_type);
    CallFunc_Set_out(func, var);

    fprintf(stdout, "\t\t(%s,%s)\n", trXSD2C((const char*)var_type), (const char*)var_name);

    /*xmlFree(var_name);
    xmlFree(var_type);*/
    
    cur = cur->next;

    break; /* only one return parameter */
  }

}

void handlePortType(xmlDocPtr doc, const char *name)
{
  xmlNodePtr node;
  xmlNodePtr input;
  xmlNodePtr output;
  xmlNodePtr cur;
  xmlChar *attr_name;
  xmlChar *message;
  struct CallFunc *func;
  char opname[255];

  node = findPortType(doc, name);
  if (node == NULL) {
    fprintf(stderr, "PortType '%s' not found!\n", name);
    return;
  }

  cur = node->xmlChildrenNode;
  while (cur != NULL) {
    
    if (cur->type != XML_ELEMENT_NODE) {
      cur = cur->next;
      continue;
    }

    if (xmlStrcmp(cur->name, (const xmlChar*)"operation")) {
      cur = cur->next;
      continue;
    }

    attr_name = xmlGetProp(cur, "name");  
    if (attr_name == NULL) {
      cur = cur->next;
      continue;
    }

    strcpy(opname, (const char*)attr_name);
    /*xmlFree(attr_name);*/
    
    func = CallFunc_Create();
    CallList_Add_operation(callList, func);
  
    CallFunc_Set_name(func, opname);
    fprintf(stdout, "Operation -> '%s'\n", opname);

    /* handle input */
    input = findSubNode(cur, "input");
    if (input == NULL) {
      fprintf(stderr, "WARNING: No input for operation '%'\n", opname);
      cur = cur->next;
      continue;
    }

    message = xmlGetProp(input, "message");  
    if (message == NULL) {
      fprintf(stderr, "ERROR: No message attribute for input operation '%s'\n", opname);
      cur = cur->next;
      continue;
    }
    
    fprintf(stdout, "\tinput  = '%s'\n", (const char*)message);
    
    handleInputParameters(doc, (const char*)message, func);

    /*xmlFree(message);*/
  
    /* handle output */
    output = findSubNode(cur, "output");
    if (output == NULL) { /* one way operation is ok */
      cur = cur->next;
      continue;
    }

    message = xmlGetProp(output, "message");  
    if (message == NULL) {
      fprintf(stderr, "ERROR: No message attribute for output operation '%s'\n", opname);
      cur = cur->next;
      continue;
    }
    
    fprintf(stdout, "\toutput = '%s'\n", (const char*)message);
    /*xmlFree(message);*/
  
    handleOutputParameters(doc, (const char*)message, func);
    cur = cur->next;
  }

}

void handleBinding(xmlDocPtr doc, const char *type)
{
  
  int i;
  xmlChar *name;
  xmlChar *btype;
  xmlNodeSetPtr nodeset; 
  xmlXPathObjectPtr xpathObj;
  char query[255];
  char port_name[255], port_ns[10];
  char binding_type[255], binding_ns[10];

  parseNS(type, binding_ns, binding_type);

  /* Set xpath  query */
  if (wsdl_prefix[0]) 
    sprintf(query, "//%s:definitions/%s:binding[@name=\"%s\"]", wsdl_prefix, wsdl_prefix, binding_type);
  else
    sprintf(query, "//definitions/binding[@name=\"%s\"]", binding_type);

  /* Find Bindings */
  xpathObj = xpath_eval(doc, query);
  
  if (xpathObj == NULL) {
    fprintf(stderr, "No Soap Binding found!\n");
    return;
  }

  /* Check if found nodes */
  nodeset = xpathObj->nodesetval;
  if (nodeset == NULL) {
    fprintf(stderr, "No Soap Binding found! nodeset empty!\n");
    return;
  }

  /* Iterate all soap services */
  for (i=0;i < nodeset->nodeNr; i++) {
    
    name = xmlGetProp(nodeset->nodeTab[i], "name");

    if (name == NULL) {
      fprintf(stderr, "WARNING: no attribute name found!\n");
    } else {
      fprintf(stdout, "Found Binding -> (name: '%s')\n", 
        (const char*)name);
    }

    btype = xmlGetProp(nodeset->nodeTab[i], "type");

    if (btype == NULL) {
      fprintf(stderr, "WARNING: no attribute type found!\n");
    } else {
      parseNS(btype, port_ns, port_name);
      handlePortType(doc, port_name);
    }

/*    if (name != NULL) xmlFree(name);
    if (btype != NULL) xmlFree(btype);  */
  }
  
}

void wsdlParse(xmlDocPtr doc)
{
  int i;
  xmlChar *name, *binding;
  xmlNodeSetPtr nodeset; 
  xmlXPathObjectPtr xpathObj;
  char query[255];

  if (wsdl_prefix[0]) 
    sprintf(query, "//%s:definitions/%s:service/%s:port[%s:address]",  wsdl_prefix, wsdl_prefix, wsdl_prefix, soap_prefix);
  else
    sprintf(query, "//definitions/service/port[%s:address]", soap_prefix);

  
  /* Find Soap Service */
  xpathObj = xpath_eval(doc, query);
  
  if (xpathObj == NULL) {
    fprintf(stderr, "No Soap Service  found!\n");
    return;
  }

  /* Check if found nodes */
  nodeset = xpathObj->nodesetval;
  if (nodeset == NULL) {
    fprintf(stderr, "No Soap Service  found! nodeset empty!\n");
    return;
  }

  /* Iterate all soap services */
  for (i=0;i < nodeset->nodeNr; i++) {
    
    name = xmlGetProp(nodeset->nodeTab[i], "name");
    binding = xmlGetProp(nodeset->nodeTab[i], "binding");

    if (name == NULL) {
      fprintf(stderr, "WARNING: no attribute type found!\n");
    } if (binding == NULL) {
      fprintf(stderr, "WARNING: no attribute binding found!\n");
    } else {
      fprintf(stdout, "Found SOAP port -> (type: '%s') (binding: '%s')\n", 
        (const char*)name, (const char*)binding);
    }

    handleBinding(doc, binding);

/*    if (name != NULL) xmlFree(name);
    if (binding != NULL) xmlFree(binding);*/
  }
  

}


static
void usage(const char* execname);

int main(int argc, char *argv[])
{
  xmlDocPtr doc;
  xmlNodePtr cur;
  xmlNodePtr node;
  xmlNodePtr xsdRoot;
  xmlNsPtr default_ns;
  xmlNsPtr soap_ns;
  xmlNsPtr wsdl_ns;
  xmlNsPtr tar_ns;
  xmlChar *tar_ns_str;
  
  int i;
  char outDir[1054];
  char fname[255];

  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }


  
 strcpy(outDir, ".");

  for (i=1;i<argc;i++) 
  {
    if (!strcmp(argv[i], "-d"))
      if (i==argc-1) usage(argv[0]);
      else strcpy(outDir, argv[++i]);
    else if (!strcmp(argv[i], "-S"))
      formatter_generate_sax_serializer = 1;
    else strcpy(fname, argv[i]);
  }
  

  xsdRoot = wsdlLoadFile(fname);
  
  if (xsdRoot == NULL ) 
  {
    fprintf(stderr,"No Schema data found\n");
    doc = xmlParseFile(fname);
    if (doc == NULL) 
    {
	    fprintf(stderr,"Can not parse document\n");
	    return 1;
    }
  } 
  else 
  {
	  doc = xsdRoot->doc;
  }
  

  node = xmlDocGetRootElement(doc);
  if (node == NULL) {
    fprintf(stderr,"empty document\n");
    xmlFreeDoc(doc);
    return 1;
  }

  /* Search for wsdl soap namespace 
    http://schemas.xmlsoap.org/wsdl/soap/
  */
  soap_ns = xmlSearchNsByHref(doc, node, "http://schemas.xmlsoap.org/wsdl/soap/");
  if (soap_ns == NULL) {
    soap_ns = xmlSearchNsByHref(doc, node, "http://schemas.xmlsoap.org/wsdl/soap");
  }

  if (soap_ns != NULL && soap_ns->prefix != NULL) {
      fprintf(stdout, "Wsdl SOAP prefix: '%s'\n", soap_ns->prefix);
      strcpy(soap_prefix, soap_ns->prefix);
  } else { 
    fprintf(stderr,"Namespace 'http://schemas.xmlsoap.org/wsdl/soap/' expected\n");
    xmlFreeDoc(doc);
    return 1;
  }

  
  /* Search for wsdl namespace 
    http://schemas.xmlsoap.org/wsdl/
  */
  wsdl_ns = xmlSearchNsByHref(doc, node, "http://schemas.xmlsoap.org/wsdl/");
  if (wsdl_ns == NULL) {
    wsdl_ns = xmlSearchNsByHref(doc, node, "http://schemas.xmlsoap.org/wsdl");
  }


  if (wsdl_ns != NULL && wsdl_ns->prefix != NULL) {
      fprintf(stdout, "Wsdl prefix: '%s'\n", wsdl_ns->prefix);
      strcpy(wsdl_prefix, wsdl_ns->prefix);
  } else { 
    /* search default namespace */
    default_ns = xmlSearchNs(doc, node, NULL);
    if (default_ns == NULL || ( default_ns != NULL && default_ns->href == NULL)) {
      fprintf(stdout, "Adding default namespace 'http://schemas.xmlsoap.org/wsdl/'\n");
      xmlNewDocProp(doc,  "xmlns", "http://schemas.xmlsoap.org/wsdl/");
    } 
    
    /*strcpy(wsdl_prefix, "wsdl");*/
    wsdl_prefix[0] = '\0';
  }



  /* search targetNamespace */
  tar_ns_str = xmlGetProp(node, "targetNamespace");
  if (tar_ns_str) {
    strcpy(target_ns, tar_ns_str);
    /* Search for target namespace 
    */
    tar_ns = xmlSearchNsByHref(doc, node, target_ns);
    if (tar_ns && tar_ns->prefix) {
      strcpy(target_prefix, tar_ns->prefix);
      printf("Target namespace: %s\nTarget NS Prefix: %s\n", 
        target_ns, target_prefix);
    } else {
      target_prefix[0] = '\0';
    }
    /*xmlFree*/
  } else {
    target_ns[0] = '\0';
    target_prefix[0] = '\0';
  }

 
  xsdRoot = xsdRoot?xsdRoot:node;
	while (xsdRoot) {

	  if (xsdRoot->type != XML_ELEMENT_NODE) {
	     xsdRoot  = xsdRoot->next;
	     continue;
	  }

    if (!xsdInitTrModule(xsdRoot))
      return 1;
  
    if (!xsdInitObjModule(xsdRoot))
      return 1;


	  printf("Calling xsd engine\n");
    if (xsdEngineRun(xsdRoot, outDir)) {
     fprintf(stderr, "xsd2c engine error\n");
    	return 1;
    }
    xsdRoot  = xsdRoot->next;
  }
  callList = CallList_Create();
  wsdlParse(doc);

/*  CallList_Sax_Serialize(callList, "CallList", 
    Writer_StartElement, 
    Writer_Characters,
    Writer_EndElement, 0); 
*/
  codeWriteStubHeader(callList, "test_stub.h");

  CallList_Free(callList); 

  trFreeModule();
  objFreeModule();

  xmlFreeDoc(doc);
  return 0;
}



static
void usage(const char* execname)
{
	fprintf(stderr, "usage: %s -d dest <wsdl file>\n", execname);	
}

