#include "xsd2c.h"
#include "formatter.h"
#include "tr.h"
#include "obj.h"

#include <string.h>


void usage(const char* execName);

int main(int argc, char *argv[])
{
  int i;
  char outDir[1054];
  xmlNodePtr xsdNode = NULL;
  char fname[255];
  int wsdl = 0;

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
    else if (!strcmp(argv[i], "-wsdl"))
      wsdl = 1;
    else strcpy(fname, argv[i]);
  }
  

  if (wsdl)
    xsdNode = wsdlLoadFile(fname);
  else
    xsdNode = xsdLoadFile(fname);

  if (xsdNode == NULL) {
    fprintf(stderr, "can not load xsd file!\n");
    return 1;
  }


  if (!xsdInitTrModule(xsdNode))
    return 1;

  if (!xsdInitObjModule(xsdNode))
    return 1;


  xsdEngineRun(xsdNode, outDir);
	
  xmlFreeDoc(xsdNode->doc);

  return 0;
}



void usage(const char* execName) 
{
  printf("usage: %s [-d <destdir> -S -D] <xsd filename>\n", execName);
}


