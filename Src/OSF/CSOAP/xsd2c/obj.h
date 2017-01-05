/******************************************************************
 *  $Id: obj.h,v 1.3 2004/10/15 13:35:39 snowdrop Exp $
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
#ifndef XSD2C_OBJ_H
#define XSD2C_OBJ_H

#include "Enumeration.h"

typedef struct FIELD* HFIELD;
typedef struct COMPLEXTYPE* HCOMPLEXTYPE;
typedef struct RESTRICTION* HRESTRICTION;
typedef int (*CT_ENUM)(HCOMPLEXTYPE);

struct FIELD
{
  char *name;
  char *type;
  int flag;
  int minOccurs;
  int maxOccurs;
  char *attrName;
  HFIELD next;
  HCOMPLEXTYPE parentObj;
};

struct COMPLEXTYPE
{
  char *type;
  char *base_type;
  HFIELD head;
  HFIELD tail;
  HCOMPLEXTYPE next;
  int isSimpleContent;
  HRESTRICTION restriction;
  char targetNS[150];
  char fullName[250];
};

#define RES_MODE_EMPTY 0
#define RES_MODE_MINMAX 1
#define RES_MODE_ENUMERATION 2

struct RESTRICTION
{
  int mode;

  char *type;

  /* Defines a list of acceptable values*/
  struct Enumeration *enumeration;

  /* Specifies the maximum number of decimal places allowed. 
      Must be equal to or greater than zero */
  int fractionDigits; 	

  /* Specifies the exact number of characters or list items allowed. 
  Must be equal to or greater than zero*/
  int length; 	

  /* Specifies the upper bounds for numeric 
   values (the value must be less than this value) */
  int maxExclusive;
  int maxExclusiveSet;

  /* Specifies the upper bounds for numeric values 
    (the value must be less than or equal to this value) */
  int maxInclusive; 	
  int maxInclusiveSet; 	

  /* Specifies the lower bounds for numeric values 
    (the value must be greater than this value) */
  int minExclusive; 	
  int minExclusiveSet; 	

  /* Specifies the lower bounds for numeric values 
    (the value must be greater than or equal to this value) */
  int minInclusive; 	
  int minInclusiveSet; 	

  /* Specifies the maximum number of characters or list items allowed. 
    Must be equal to or greater than zero */
  int maxLength; 	


  /* Specifies the minimum number of characters or list items allowed. 
    Must be equal to or greater than zero */
  int minLength; 	

  /* Defines the exact sequence of 
    characters that are acceptable */
  char pattern[150]; 	

  /* Specifies the exact number of digits allowed. 
    Must be greater than zero */
  int totalDigits; 	

  /* Specifies how white space (line feeds, tabs, 
    spaces, and carriage returns) is handled  */
  char whiteSpace[50]; 	
};

void objInitModule();
void objFreeModule();

HCOMPLEXTYPE objCreateComplexType(const char* typename);
void objSetBaseType(HCOMPLEXTYPE obj, const char* typename);
HFIELD objAddElement(HCOMPLEXTYPE obj, const char* name, const char* type, int flag, int mino, int maxo);
HFIELD objAddAttribute(HCOMPLEXTYPE obj, const char* name, const char* type, int flag);

HCOMPLEXTYPE objRegistryGetComplexType(const char* typename);
void objRegistryEnumComplexType(CT_ENUM callback);



HRESTRICTION resCreate(const char* type);

#endif
