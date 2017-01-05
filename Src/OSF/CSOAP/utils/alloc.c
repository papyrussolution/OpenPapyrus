/******************************************************************
 *  $Id: alloc.c,v 1.1 2004/10/29 09:28:25 snowdrop Exp $
 *
 * CSOAP Project:  A http client/server library in C
 * Copyright (C) 2003-2004  Ferhat Ayaz
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
 * Email: ferhatayaz@yahoo.com
 ******************************************************************/

#include <stdio.h>
#include <string.h>

#include "alloc.h"

#undef malloc
#undef free

typedef struct _alloc_node
{
  char func[150];
  char file[150];
  int line;
  int size;
  void* pointer;
  struct _alloc_node *next;
}alloc_node_t;


static alloc_node_t *alloc_first  = NULL;
static alloc_node_t *alloc_last   = NULL;

void* _mem_alloc(const char* func, const char* file, int line, size_t size)
{

	int i = strlen(file)-1;

  alloc_node_t *node = (alloc_node_t*)malloc(sizeof(alloc_node_t));
  strcpy(node->func, func);
	while (i>0 && file[i]!='\\')i--;
	strcpy(node->file, (file[i]!='\\')?file:(file+i+1));
  node->line = line;
  node->size = size;
  node->pointer = malloc(size);
  node->next = NULL;

  if (alloc_first == NULL)
    alloc_first = alloc_last = node;
  else {
    alloc_last->next = node;
    alloc_last = node;
  }

  return node->pointer;
}


void _mem_free(void *p)
{
  alloc_node_t *node = alloc_first;

  while (node) {
    if (node->pointer == p) {
      free(p);
      node->pointer = NULL;
      return;
    }
    node = node->next;
  }
}


void _mem_report()
{
  alloc_node_t *tmp, *node = alloc_first;
	printf("****************** String Report **********************\n");

  while (node) {
    if (node->pointer) {
      fprintf(stdout, "[%s:%d] \t%s()\t%d bytes\n",
        node->file, node->line, node->func, node->size);
    }
    tmp = node->next;
    free(node);
    node = tmp;
  }
	printf("****************** End Report **********************\n");
}



