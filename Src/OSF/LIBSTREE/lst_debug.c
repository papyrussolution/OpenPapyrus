/*

Copyright (C) 2003-2006 Christian Kreibich <christian@whoop.org>.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies of the Software and its documentation and acknowledgment shall be
given in the documentation and software packages that this Software was
used.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "libstree.h"

struct lst_stree_qi
{
  TAILQ_ENTRY(lst_stree_qi) entries;

  LST_Node *node;
};

static struct lst_stree_qi * lst_debug_qi_new(LST_Node *node)
{
  struct lst_stree_qi * qi = (struct lst_stree_qi *)calloc(1, sizeof(struct lst_stree_qi));
  qi->node = node;
  return qi;
}


void 
lst_debug_print_tree(LST_STree *tree)
{
  LST_Edge *edge;
  struct lst_stree_qi *qi, *dummy_qi;
  TAILQ_HEAD(tailhead, lst_stree_qi) queue;

  dummy_qi = lst_debug_qi_new(NULL);
  qi = lst_debug_qi_new(tree->root_node);

  TAILQ_INIT(&queue);
  TAILQ_INSERT_HEAD(&queue, qi, entries);
  TAILQ_INSERT_TAIL(&queue, dummy_qi, entries);

  while (queue.tqh_first)
    {
      qi = queue.tqh_first;

      TAILQ_REMOVE(&queue, queue.tqh_first, entries);

      if (qi->node == NULL)
	{
	  if (queue.tqh_first)
	    {
	      dummy_qi = lst_debug_qi_new(NULL);
	      TAILQ_INSERT_TAIL(&queue, dummy_qi, entries);
	    }
	  continue;
	}


      /* Output current node: */
      fprintf(stderr, "[%u (%u)", qi->node->id, qi->node->visitors);
      
      if (qi->node->suffix_link_node)
	fprintf(stderr, " -> %u]\n", qi->node->suffix_link_node->id);
      else
	fprintf(stderr, "]\n");

      /* Now output edge labels and adjacent-node ids: */
      if (!qi->node->kids.lh_first)
	{
	  fprintf(stderr, "\t(leaf)\n");
	}
      else
	{
	  for (edge = qi->node->kids.lh_first; edge; edge = edge->siblings.le_next)
	    {
	      if (edge->dst_node->kids.lh_first)
		{
		  fprintf(stderr, "\t'%s' %u (%s)\n",
			  edge->range.string->sclass->print_func(&edge->range),
			  edge->dst_node->id,
			  lst_string_print(edge->range.string));
		  qi = lst_debug_qi_new(edge->dst_node);
		  TAILQ_INSERT_TAIL(&queue, qi, entries);
		}
	      else
		{
		  fprintf(stderr, "\t'%s' [%i] %u%c%c (%s)\n",
			  edge->range.string->sclass->print_func(&edge->range),
			  edge->dst_node->index,
			  edge->dst_node->id,
			  (edge->range.end_index == &edge->range.end_index_local) ? 'l' : 't',
			  (edge->range.end_index == tree->phase) ? 'c' : ' ',
			  lst_string_print(edge->range.string));
		}
	    }
	}
    }
}

#ifdef LST_DEBUG

char * lst_debug_print_substring(LST_String *string,
			  uint start_index,
			  uint end_index,
			  uint extra_index)
{
  LST_StringIndex tmp_range;

  lst_string_index_init(&tmp_range);

  tmp_range.string = string;
  tmp_range.start_index = start_index;
  *(tmp_range.end_index) = end_index;
  tmp_range.extra_index = extra_index;

  return string->sclass->print_func(&tmp_range);
}


#endif
