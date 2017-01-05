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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <lst_structs.h>
#include <lst_stree.h>
#include <lst_string.h>
#include <lst_algorithms.h>

/* Quick hack to test correct repeated operation of LCS algorithm on the
 * same suffix tree:
 */
#define LCS_REPS 1

static void
test_usage(char *progname)
{
  printf("USAGE: %s <file1 file2 ...>\n"
	 "This test program computes the longest common substring(s)\n"
	 "of the files provided on the command line.\n", progname);
  exit(0);
}


void
string_cb(LST_String *string, void *data)
{
  printf("%s\n", lst_string_print(string));
  return;

  data = 0;
}

int
main(int argc, char **argv)
{
  LST_STree       *tree;
  LST_StringSet   *set, *result;
  LST_String      *string;
  int i;
  
  if (argc < 2)
    test_usage(argv[0]);

  /* Create a string class with a special print method, otherwise
   * we can use the defaults:
   */
  lst_stringclass_set_defaults(NULL, NULL, lst_string_print_hex);
  
  /* Create a string set to conveniently hold all our strings: */
  set = lst_stringset_new();

  /* Add all strings passed on the command line to the set.
   * Note that we pass the string length excluding the null
   * terminator, libstree handles that generically.
   */
  for (i = 1; i < argc; i++)
    {
      FILE *f;
      u_char *data;
      struct stat st;
      
      printf("Reading file %s.\n", argv[i]);

      if (stat(argv[i], & st) < 0)
	{	  
	  printf("Skipping file %s.\n", argv[i]);
	  continue;
	}

      data = malloc(st.st_size);
      if (!data)
	{
	  printf("File %s too big, skipping.\n", argv[i]);
	  continue;
	}
      
      if ( (f = fopen(argv[i], "r")) == NULL)
	{
	  printf("Couldn't open %s, skipping.\n", argv[i]);
	  free(data);
	  continue;
	}

      if (fread(data, 1, st.st_size, f) != (size_t) st.st_size)
	{
	  printf("Error reading %s -- skipping.\n", argv[i]);
	  free(data);
	  continue;
	}

      string = lst_string_new(data, 1, st.st_size);
      lst_stringset_add(set, string);
      
      fclose(f);
      free(data);
    }

  /* Create a suffix tree for all strings in the set: */
  tree = lst_stree_new(set);

  for (i = 0; i < LCS_REPS; i++)
    {
      printf("Run %i:\n", i);

      /* Find longest common substring(s) */
      result = lst_alg_longest_common_substring(tree, 0, 0);
      
      /* Print them out: */
      lst_stringset_foreach(result, string_cb, NULL);
      printf("\n");
    }

  /* Free suffix tree: */
  lst_stree_free(tree);

  return 0;
}
