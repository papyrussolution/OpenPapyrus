#include "codewriter.h"
#include <xsd2c/tr.h>
#include <xsd2c/obj.h>

#include <stdio.h>

static FILE *header;
static FILE *source;
static char* ns = "impl";

static int openSourceFile(const char* filename);
static void closeSourceFile();

static int openHeaderFile(const char* filename);
static void closeHeaderFile();

static void writeOperation(struct CallFunc *call)
{
  struct CallVar_List* var;

  fprintf(header, "%s %s_%s(", trXSD2C(call->out->type), ns, call->name);
  
  var = CallFunc_Get_in(call);

  while (var) {
    fprintf(header, "const %s %s", trXSD2C(var->value->type), var->value->name);
    var = var->next;
    if (var) fprintf(header, ",");
  }

  fprintf(header, ")");
}

void codeWriteStubHeader(struct CallList *cl, const char* filename)
{
  struct CallFunc_List *call;
  struct CallVar_List* var;
  HCOMPLEXTYPE ct;

  openHeaderFile(filename);

  fprintf(header, "#ifndef __TEST_SOAP_H__\n#define __TEST_SOAP_H__\n\n");

  /* Find header files */
  call = CallList_Get_operation(cl);
  while (call) {
    var = CallFunc_Get_in(call->value);
    while (var) {
      ct = objRegistryGetComplexType(var->value->type);
      if (ct /* TODO: && not in written before  */)
      fprintf(header, "#include \"%s.h\"\n", trXSDParseNs(var->value->type)); /* _xsd*/
      var = var->next;
    }
    call = call->next;
  }

  fprintf(header, "\n");
  call = CallList_Get_operation(cl);
  while (call) {
    
    writeOperation(call->value);
    fprintf(header, ";\n");
    call = call->next;
  }

  fprintf(header, "\n\n#endif\n");

  closeHeaderFile();
}


void codeWriteStubSource(struct CallList *cl, const char* filename)
{
  struct CallFunc_List *call;

  openSourceFile(filename);
  
  fprintf(source, "#include \"test_stub.h\"\n\n");

  call = CallList_Get_operation(cl);
  while (call) {
    
    writeOperation(call->value);
/*    fwrite(source, "\n{\n\t*/
    call = call->next;
  }
  

  fprintf(source, "\n");

  closeSourceFile();
}



static 
int openSourceFile(const char* filename)
{
  source = fopen(filename, "w");
  if (!source) return 0;
  return 1;
}

static 
void closeSourceFile()
{
  fclose(source);
}

static 
int openHeaderFile(const char* filename)
{
  header= fopen(filename, "w");
  if (!header) return 0;
  return 1;
}


static 
void closeHeaderFile()
{
  fclose(header);
}


