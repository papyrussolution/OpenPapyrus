/******************************************************************
 *  $Id: tr.c,v 1.5 2005/05/27 19:28:16 snowdrop Exp $
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
 * Email: ayaz@jprogrammet.net
 ******************************************************************/

#include "tr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XSD2C_MAP(xsdtype, ctype, builtin) \
  trRegisterType(xsdtype, ctype, builtin);
 
#define XSD2C_MAPNS(xsdtype, ctype, builtin) \
  trRegisterTypeNS(trXSDNS, xsdtype, ctype, builtin);
 
#define XSD2C_MAPNS_LIST(xsdtype, ctype) \
  trRegisterListTypeNS(trXSDNS, xsdtype, ctype);
 
struct XSD2C_TypeRegistry
{
  char *xsd_type;
  char *c_type;
  int isbuildin;
  struct XSD2C_TypeRegistry* next;
};

static struct XSD2C_TypeRegistry* tr_head;
static struct XSD2C_TypeRegistry* tr_tail;
static struct XSD2C_TypeRegistry* trl_head;
static struct XSD2C_TypeRegistry* trl_tail;


static char trXSDNS[15];
static int _trInitialized = 0;
static char* _trC2XSD(const char* cType, struct XSD2C_TypeRegistry* head);
static char* _trXSD2C(const char* xsdType, struct XSD2C_TypeRegistry* head);


/*void trInitModule(const char* ns)*/
void trInitModule()
{
  if (_trInitialized)
   return;
/*  struct XSD2C_TypeRegistry* cur;*/
  tr_head = NULL; 
  tr_tail = NULL;
  trl_head = NULL; 
  trl_tail = NULL;
/*
  strcpy(trXSDNS, ns);

  #include "types.map"

  cur = tr_head;

  while (cur != NULL)
  {
    cur->isbuildin = 1;
    cur = cur->next;
  }
*/
  _trInitialized  = 1;
};


void trFreeModule()
{
  struct XSD2C_TypeRegistry* cur, *tmp;

  cur = tr_head;

  while (cur != NULL)
  {
    if (cur->xsd_type) free(cur->xsd_type);
    if (cur->c_type) free(cur->c_type);
    tmp = cur->next;
    free(cur);
    cur = tmp;
  }
  cur = trl_head;

  while (cur != NULL)
  {
    if (cur->xsd_type) free(cur->xsd_type);
    if (cur->c_type) free(cur->c_type);
    tmp = cur->next;
    free(cur);
    cur = tmp;
  }
}


int trGetBuildInFlag(const char* xsdType)
{
  struct XSD2C_TypeRegistry* cur;

  cur = tr_head;

  while (cur != NULL)
  {
    if (!strcmp(cur->xsd_type, xsdType))
    {
      return cur->isbuildin;
    }
    cur = cur->next;
  }
  return -1;
}


char* trC2XSD(const char* cType)
{
  return _trC2XSD(cType, tr_head);
}

char* trXSD2C(const char* xsdType)
{
  return _trXSD2C(xsdType, tr_head);
}

char* trC2XSDList(const char* cType)
{
  return _trC2XSD(cType, trl_head);
}

char* trXSD2CList(const char* xsdType)
{
  return _trXSD2C(xsdType, trl_head);
}

void trRegisterType(const char* xsdType, const char* cType, int builtin)
{
  trRegisterTypeNS(NULL, xsdType, cType, builtin);
}

void trRegisterTypeNS(const char* ns, const char* xsdType, const char* cType, int builtin)
{
  struct XSD2C_TypeRegistry* reg;
  if (xsdType == NULL || cType == NULL)
  {
    fprintf(stderr, "WARNING: Can not register type\n");
    return;
  }

  reg = (struct XSD2C_TypeRegistry*)malloc(sizeof(struct XSD2C_TypeRegistry));
  reg->xsd_type = (char*)malloc((ns?strlen(ns):0)+strlen(xsdType)+2);
  reg->c_type = (char*)malloc(strlen(cType)+1);
  reg->next = NULL;
  reg->isbuildin = builtin;

  if (ns) 
    sprintf(reg->xsd_type, "%s:%s",ns,xsdType);
  else
    strcpy(reg->xsd_type, xsdType);
 
  strcpy(reg->c_type, cType);
  printf("[TYPE] Registered '%s'->'%s'\n", reg->xsd_type, reg->c_type);

  if (tr_tail)
  {
    tr_tail->next = reg;
  }

  if (tr_head == NULL)
  {
    tr_head = reg;
  }

  tr_tail = reg;
}

char* trXSDParseNs(char* xsdType)
{
	int c = 0;
	while (xsdType[c] != '\0' ) {
		if (xsdType[c] == ':') return &xsdType[++c];
		c++;
	}
	return xsdType;
}

void trRegisterListType(const char* xsdType, const char* cType)
{
  trRegisterListTypeNS(NULL, xsdType, cType);
}

void trRegisterListTypeNS(const char* ns, const char* xsdType, const char* cType)
{
  struct XSD2C_TypeRegistry* reg;
  if (xsdType == NULL || cType == NULL)
  {
    fprintf(stderr, "WARNING: Can not register type\n");
    return;
  }

  reg = (struct XSD2C_TypeRegistry*)malloc(sizeof(struct XSD2C_TypeRegistry));
  reg->xsd_type = (char*)malloc((ns?strlen(ns):0)+strlen(xsdType)+2);
  reg->c_type = (char*)malloc(strlen(cType)+1);
  reg->next = NULL;
  reg->isbuildin = 0;

  if (ns)
    sprintf(reg->xsd_type, "%s:%s",ns,xsdType);
  else
    strcpy(reg->xsd_type, xsdType);
 
  strcpy(reg->c_type, cType);

  if (trl_tail)
  {
    trl_tail->next = reg;
  }

  if (trl_head == NULL)
  {
    trl_head = reg;
  }

  trl_tail = reg;
}


static 
char* _trC2XSD(const char* cType, struct XSD2C_TypeRegistry* head)
{
  struct XSD2C_TypeRegistry* cur;

  printf("[TYPE] Search: '%s'   ", cType?cType:"null");
  cur = head;

  while (cur != NULL)
  {
    if (!strcmp(cur->c_type, cType))
    {
      printf("FOUND\n");
      return cur->xsd_type;
    }
    cur = cur->next;
  }

      printf("NOT FOUND\n");
  return NULL;
}

static char* _trXSD2C(const char* xsdType, struct XSD2C_TypeRegistry* head)
{
  struct XSD2C_TypeRegistry* cur;

  cur = head;

  while (cur != NULL)
  {
  /*  printf("%s\n", cur->xsd_type);*/
    if (!strcmp(cur->xsd_type, xsdType))
    {
      return cur->c_type;
    }
    cur = cur->next;
  }

  return NULL;
}
