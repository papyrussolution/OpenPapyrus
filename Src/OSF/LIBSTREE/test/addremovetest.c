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

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <lst_debug.h>
#include <lst_structs.h>
#include <lst_stree.h>
#include <lst_string.h>
#include <lst_algorithms.h>


static void
test_usage(char *progname)
{
  printf("USAGE: %s <string(s) ...>\n"
	 "This test program first incrementally inserts the strings\n"
	 "provided into a suffix tree, and then removes them\n", progname);
  exit(0);
}


void 
add_cb(LST_String *string, LST_STree *tree)
{
  lst_stree_add_string(tree, string);

  printf("\n\nAfter adding %s\n", lst_string_print(string));
  lst_debug_print_tree(tree);
}


void 
remove_cb(LST_String *string, LST_STree *tree)
{
  lst_stree_remove_string(tree, string);

  printf("\n\nAfter removing %s\n", lst_string_print(string));
  lst_debug_print_tree(tree);
}


int
main(int argc, char **argv)
{
  LST_STree     *tree;
  LST_StringSet *set;
  int i;
  
  if (argc < 2)
    test_usage(argv[0]);

  /* Create a suffix tree for all strings: */
  tree = lst_stree_new(NULL);

  /* Create a string set to conveniently hold all our strings: */
  set = lst_stringset_new();

  /* Add all strings passed on the command line to the set.
   * Note that we pass the string length excluding the null
   * terminator, libstree handles that generically.
   */
  for (i = 1; i < argc; i++)
    lst_stringset_add(set, lst_string_new(argv[i], 1, strlen(argv[i])));

  lst_stringset_foreach(set, (LST_StringCB) add_cb, tree);
  lst_stringset_foreach(set, (LST_StringCB) remove_cb, tree);
  
  /* Free suffix tree: */
  lst_stree_free(tree);

  return 0;
}
