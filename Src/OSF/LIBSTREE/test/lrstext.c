/*

Copyright (C) 2003-2006 Christian Kreibich <christian@whoop.org>
                        Andreas Gustafsson <gson@gson.org>

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

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <lst_structs.h>
#include <lst_stree.h>
#include <lst_string.h>
#include <lst_algorithms.h>


static void
test_usage(char *progname)
{
  printf("USAGE: %s minlen maxlen <strings ...>\n"
        "This test program computes the longest repeated substring(s)\n"
        "no longer than maxlen characters (or as long as possible if\n"
        "maxlen is 0) of the strings provided on the command line.\n", progname);
  exit(0);
}


void
node_cb(LST_Node *node, void *data)
{
  if (node->up_edge)
    printf("Node: %u, string index: %i\n", node->id,
          lst_stree_get_string_index((LST_STree *) data, node->up_edge->range.string));
}

void
string_cb(LST_String *string, void *data)
{
  printf("%s ", lst_string_print(string));
}

int
main(int argc, char **argv)
{
  LST_STree     *tree;
  LST_StringSet *set, *result;
  int i;
  u_int min_len, max_len;
  
  if (argc < 4)
    test_usage(argv[0]);

  min_len = atoi(argv[1]);
  max_len = atoi(argv[2]);

  /* Create a string set to conveniently hold all our strings: */
  set = lst_stringset_new();

  /* Add all strings passed on the command line to the set.
   * Note that we pass the string length excluding the null
   * terminator, libstree handles that generically.
   */
  for (i = 3; i < argc; i++)
    lst_stringset_add(set, lst_string_new(argv[i], 1, strlen(argv[i])));

  /* Create a suffix tree for all strings in the set: */
  tree = lst_stree_new(set);

  /* Find longest repeated substring(s) */
  result = lst_alg_longest_repeated_substring(tree, min_len, max_len);
  
  /* Print them out, if any. */
  if (result)
    {
      lst_stringset_foreach(result, string_cb, NULL);
      printf("\n");
    }
   
  /* Free suffix tree: */
  lst_stree_free(tree);

  return 0;
}
