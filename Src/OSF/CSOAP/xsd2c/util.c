/******************************************************************
 *  $Id: util.c,v 1.2 2005/05/27 19:28:16 snowdrop Exp $
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

int parseNS(const char* fullname, char *ns, char *name)
{
  int len, i, found;

  if (fullname == NULL || ns == NULL || name == NULL) return 0;
  
  len = strlen(fullname);

  found = 0;
  for (i = len - 1; i > 0; i--) 
  {
    if (fullname[i] == ':') { found = 1; break; }
  }

  if (found) strncpy(ns, fullname, i);
  else ns[0] = '\0';

  strcpy(name, 
      (fullname[i] == ':')?
      (&fullname[i+1]):(&fullname[i]));

  return 1;
}


void toUpperCase(const char* src, char *dest)
{
  int i, len;
  if (src == NULL) return;

  len = strlen(src);
  for (i=0;i<len;i++) dest[i] = toupper(src[i]);
  dest[i] = '\0';
}


void toLowerCase(const char* src, char *dest)
{
  int i, len;
  if (src == NULL) return;

  len = strlen(src);
  for (i=0;i<len;i++) dest[i] = tolower(src[i]);
  dest[i] = '\0';
}

int test_parseNS_main(int argc, char *argv[])
{
  char ns[50];
  char name[50];

  char *fullname1 = "xsd:test1";
  char *fullname2 = ":test2";
  char *fullname3 = "test3";
  char *fullname4 = "test4:";

  ns[0] = '\0'; name[0] = '\0';
  if (parseNS(fullname1, ns, name)) {
    printf("%-5s | %-10s\n", ns, name);
  }
  ns[0] = '\0'; name[0] = '\0';
  if (parseNS(fullname2, ns, name)) {
    printf("%-5s | %-10s\n", ns, name);
  }
  ns[0] = '\0'; name[0] = '\0';
  if (parseNS(fullname3, ns, name)) {
    printf("%-5s | %-10s\n", ns, name);
  }
  ns[0] = '\0'; name[0] = '\0';
  if (parseNS(fullname4, ns, name)) {
    printf("%-5s | %-10s\n", ns, name);
  }

  return 0;
}



