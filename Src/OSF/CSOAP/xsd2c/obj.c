/******************************************************************
 *  $Id: obj.c,v 1.4 2004/10/15 13:35:39 snowdrop Exp $
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
#include "obj.h"
#include "tr.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

struct tagObjRegistry
{
  HCOMPLEXTYPE head;
  HCOMPLEXTYPE tail;
}objRegistry;


static HFIELD fieldCreate(const char* name, const char* type, int flag, int mino, int maxo, HCOMPLEXTYPE parentObj);
static void fieldFree(HFIELD field);
static void objAddField(HCOMPLEXTYPE obj, HFIELD field);
static void objFreeComplexType(HCOMPLEXTYPE obj);
static void objRegistryAddComplexType(HCOMPLEXTYPE obj);

static char *targetNS = NULL;
static int _objInitialized = 0;

/* ------------------------------------------------------------------ */

void objInitModule(const char* tns)
{
  if (tns != NULL) {
    targetNS = (char*)malloc(strlen(tns)+1);
    strcpy(targetNS, tns);
  } 
  if (_objInitialized) return;

  objRegistry.head = NULL;
  objRegistry.tail = NULL;
  _objInitialized = 1;
}

void objFreeModule()
{
  HCOMPLEXTYPE cur;

  cur = objRegistry.head;
  while (cur != NULL)
  {
    objFreeComplexType(cur);
    cur = cur->next;
  }

  free(targetNS);
}


HCOMPLEXTYPE objCreateComplexType(const char* typename)
{
  HCOMPLEXTYPE ct;
  char buf[1054];

  ct = (HCOMPLEXTYPE)malloc(sizeof(struct COMPLEXTYPE));
  ct->type = (char*)malloc(strlen(typename)+1);
  strcpy(ct->type, typename);
  ct->head = ct->tail = NULL;
  ct->next = NULL;
  ct->base_type = NULL;
  ct->restriction = NULL;
  ct->isSimpleContent = 0;
  strcpy(ct->targetNS, targetNS);
  sprintf(ct->fullName, "%s:%s", targetNS, typename);
  objRegistryAddComplexType(ct);
  
  sprintf(buf, "struct %s*", typename);
  trRegisterTypeNS(targetNS, typename, buf, 0);
  sprintf(buf, "struct %s_List*", typename);
  trRegisterListTypeNS(targetNS, typename, buf);
  return ct;
}

HFIELD objAddElement(HCOMPLEXTYPE obj, const char* name, const char* type, int flag, int mino, int maxo)
{
  HFIELD field;

  field = fieldCreate(name, type, flag, mino, maxo, obj);
  objAddField(obj, field);

  return field;
}

HFIELD objAddAttribute(HCOMPLEXTYPE obj, const char* name, const char* type, int flag)
{
  char buffer[1054];
  HFIELD field;

  sprintf(buffer, "attr_%s", name);

  field = fieldCreate(buffer, type, flag, 1, 1, obj);
  field->attrName = (char*)malloc(strlen(name)+1);
  strcpy(field->attrName, name);

  objAddField(obj, field);

  return field;
}

void objSetBaseType(HCOMPLEXTYPE obj, const char* typename)
{
  if (obj->base_type != NULL) free(obj->base_type);
  obj->base_type = (char*)malloc(strlen(typename)+1);
  strcpy(obj->base_type, typename);
}

HCOMPLEXTYPE objRegistryGetComplexType(const char* typename)
{
  HCOMPLEXTYPE cur;

  cur = objRegistry.head;
  while (cur != NULL)
  {
    if (!strcmp(cur->fullName, typename))
      return cur;
    cur = cur->next;
  }
  return NULL;
}


void objRegistryEnumComplexType(CT_ENUM callback)
{
  HCOMPLEXTYPE cur;

  cur = objRegistry.head;
  while (cur != NULL)
  {
    if (!callback(cur)) return;
    cur = cur->next;
  }
}


/* ========================================================================= */

static
HFIELD fieldCreate(const char* name, const char* type, int flag, int mino, int maxo, HCOMPLEXTYPE parentObj)
{
  HFIELD field;
	
  field = (HFIELD)malloc(sizeof(struct FIELD));
  field->name = (char*)malloc(strlen(name)+1);
  field->type = (char*)malloc(strlen(type)+1);
  field->flag = flag;
  field->next = NULL;
  field->flag = 0;
  field->minOccurs = mino;
  field->maxOccurs = maxo;
  field->parentObj = parentObj;
  field->attrName = NULL;

  strcpy(field->name, name);
  strcpy(field->type, type);
  
  return field;
}


static 
void fieldFree(HFIELD field)
{
  if (field)
  {
    if (field->name) free(field->name);
    if (field->type) free(field->type);
    if (field->attrName) free(field->attrName);
    if (field->next) fieldFree(field->next);
    
    free(field);
  }
}


static 
void objAddField(HCOMPLEXTYPE obj, HFIELD field)
{
  printf("ADD FIELD %s %s\n", obj->type, field->type);
  if (obj->tail)
  {
    obj->tail->next = field;
  }

  if (!obj->head)
  {
    obj->head = field;
  }

  obj->tail = field;
}


static 
void objFreeComplexType(HCOMPLEXTYPE obj)
{
  if (obj)
  {
    if (obj->type) free(obj->type);
    if (obj->base_type) free(obj->base_type);
    if (obj->head) fieldFree(obj->head);
    
    free(obj);
  }
}


static void objRegistryAddComplexType(HCOMPLEXTYPE obj)
{
  if (objRegistry.tail)
  {
    objRegistry.tail->next = obj;
  }

  if (!objRegistry.head)
  {
    objRegistry.head = obj;
  }

  objRegistry.tail = obj;
}



HRESTRICTION resCreate(const char *type)
{
  HRESTRICTION res = (HRESTRICTION)malloc(sizeof(struct RESTRICTION));
  res->mode = RES_MODE_EMPTY;
  res->type = (char*)malloc(strlen(type)+1);
  strcpy(res->type, type);

  res->minInclusiveSet = 0;
  res->maxInclusiveSet = 0;
  res->minExclusiveSet = 0;
  res->maxExclusiveSet = 0;

  res->enumeration = Enumeration_Create();

  return res;
}


