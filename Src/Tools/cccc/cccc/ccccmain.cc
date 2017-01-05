// ccccmain.cc
// command line interface implementation for the cccc project



#include "cccc.h"

#include "cccc_ver.h"

#include <fstream>
#include <list>
#include <iterator>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define HANDLE intptr_t
#define INVALID_HANDLE_VALUE -1
#else
#include <sys/stat.h>
#endif

#include "cccc_itm.h"
#include "cccc_opt.h"
#include "cccc_met.h"
#include "cccc_db.h"
#include "cccc_utl.h"
#include "cccc_htm.h"
#include "cccc_xml.h"

// support for languages is now a compile-time option
#ifdef CC_INCLUDED
#include "CParser.h"
#include "CLexer.h"
#endif

#ifdef JAVA_INCLUDED
#include "JParser.h"
#include "JLexer.h"
#endif

#ifdef ADA_INCLUDED
#include "AdaPrser.h"
#include "ALexer.h"
#endif

// class "DLGFileInput" is required below (Andrey Kurilov, 2008.01.29)
#include "DLexerBase.h"

#define NEW_PAGE "\f\n"

CCCC_Project *prj=NULL;
int DebugMask=0;
int dont_free=0;

char *skip_identifiers[SKIP_IDENTIFIERS_ARRAY_SIZE];

#if defined(_WIN32) && defined(__GNUG__) 
// Cygnus gcc for Win32 B19 will try to expand wildcards that are given at
// the commandline. Sadly this "globbing" will not work the way it is 
// supposed in Win32, but luckily the whole globbing can be disabled by 
// defining the following variable: 
int _CRT_glob = 0;
#endif


/*
** global variables to hold default values for various things
*/
string current_filename, current_rule, parse_language;

// class Main encapsulates the top level of control for the program
// including command line handling
class Main 
{
  // most of the data members of this class are either set
  // by default or gleaned from the command line

// each of the data members of type string or int can be 
// set by a command line flag of the type --<member name>=<value>

  string outdir;
  string db_infile;
  string db_outfile;
  string opt_infile;
  string opt_outfile;
  string html_outfile;
  string xml_outfile;
  string lang;
  int report_mask; 
  int debug_mask;
  int files_parsed;

  // As we gather up the list of files to be processed
  // we work out and record the appropriate language to 
  // use for each.
  typedef std::pair<string,string> file_entry;
  std::list<file_entry> file_list;

// this function encapsulates adding an argument to the file_list
// for the time being, on Win32 only, it also performs filename globbing
  void AddFileArgument(const string&);

public:

  Main();
  void HandleArgs(int argc, char**argv);
  void HandleDebugOption(const string&);
  void HandleReportOption(const string&);
  void PrintCredits(ostream& os);
  void PrintUsage(ostream& os);
  int ParseFiles();
  int DumpDatabase();
  int LoadDatabase();
  void GenerateHtml();
  void GenerateXml();
  void DescribeOutput();
  int filesParsed();

  friend int main(int argc, char** argv);
};

Main *app=NULL;

Main::Main()
{
  report_mask=0xFFFF&(~(rtPROC2|rtSTRUCT2)) ;  
  debug_mask=0;
  files_parsed=0;
}

void Main::HandleArgs(int argc, char **argv)
{
  bool accepting_options=true;

  for(int i=1; i<argc; i++)
    {
      string next_arg=argv[i];
      if(
	 (accepting_options==false) ||
	 (next_arg.substr(0,2)!="--")
	 )
	{
	  // normally this will be a single file name, but 
	  // the function below also encapsulates handling of 
	  // globbing (only required under Win32) and
	  // the conventional interpretation of '-' to mean
	  // read a list of files from standard input
	  AddFileArgument(next_arg);
	}
      else if(next_arg=="--")
	{
	  // we support the conventional use of a bare -- to turn
	  // off option processing and allow filenames that look like
	  // options to be accepted
	  accepting_options=false;
	}
      else if(next_arg=="--help")
	{
	  PrintUsage(cout);
	  exit(1);
	}
      else
	{
	  // the options below this point are all of the form --opt=val,
	  // so we parse the argument to find the assignment 
	  unsigned int assignment_pos=next_arg.find("=");
	  if(assignment_pos==string::npos)
	    {
	      cerr << "Unexpected option " << next_arg << endl;
	      PrintUsage(cerr);
	      exit(2);
	    }
	  else
	    {
	      string next_opt=next_arg.substr(0,assignment_pos);
	      string next_val=next_arg.substr(assignment_pos+1);

	      if(next_opt=="--outdir")
		{
		  outdir=next_val;
		}
	      else if(next_opt=="--db_infile")
		{
		  db_infile=next_val;
		}
	      else if(next_opt=="--db_outfile")
		{
		  db_outfile=next_val;
		}
	      else if(next_opt=="--opt_infile")
		{
		  opt_infile=next_val;
		}
	      else if(next_opt=="--opt_outfile")
		{
		  opt_outfile=next_val;
		}
	      else if(next_opt=="--html_outfile")
		{
		  html_outfile=next_val;
		}
	      else if(next_opt=="--xml_outfile")
		{
		  xml_outfile=next_val;
		}

	      else if(next_opt=="--lang")
		{
		  lang=next_val;
		}
	      else if(next_opt=="--report_mask")
		{
		  // The report option may either be an integer flag vector
		  // in numeric format (including hex) or may be a string of
		  // characters (see HandleReportOption for what they mean).
		  HandleReportOption(next_val.c_str());
		}
	      else if(next_opt=="--debug_mask")
		{
		  // The report option may either be an integer flag vector
		  // in numeric format (including hex) or may be a string of
		  // characters (see HandleDebugOption for what they mean).
		  HandleDebugOption(next_val.c_str());
		}
	      else
		{
		  cerr << "Unexpected option " << next_opt << endl;
		  PrintUsage(cerr);
		  exit(3);
		}
	    }
	}
    }

  // we fill in defaults for things which have not been set
  if(outdir=="")
    {
      outdir=".cccc";
    }
  if(db_outfile=="")
    {
      db_outfile=outdir+"/cccc.db";
    }
  if(html_outfile=="")
    {
      html_outfile=outdir+"/cccc.html";
    }
  if(xml_outfile=="")
    {
      xml_outfile=outdir+"/cccc.xml";
    }
  if(opt_outfile=="")
    {
      opt_outfile=outdir+"/cccc.opt";
    }

  // the other strings all default to empty values


  if(opt_infile=="")
    {
      CCCC_Options::Load_Options();

      // save the options so that they can be edited
      CCCC_Options::Save_Options(opt_outfile);
    }
  else
    {
      CCCC_Options::Load_Options(opt_infile);
    }
}

void Main::AddFileArgument(const string& file_arg) 
{
  if(file_arg=="-")
    {
      /*
      ** add files listed on standard input to the list of files 
      ** to be processed
      */
      while(!std::cin.eof())
	{
	  string filename;
	  std::cin >> filename;
	  file_entry file_entry(filename,lang);
	  file_list.push_back(file_entry);
	}
    }
  else
    {

#ifdef _WIN32
      /*
      ** In Win32 we will have to expand all wildcards ourself, because the
      ** shell won't do this for us.
      ** This code reworked for 3.1.1 because we are now using the Visual C++ 2003
      ** Toolkit, which does not provide the same APIs as Visual Studio 5/6.
      */
      _finddata_t fd;
      HANDLE sh = _findfirst(file_arg.c_str(), &fd);

      // 3.pre40
      // I discovered (by behaviour, not documentation) that
      // the structure returned by FindFirstFile etc. only includes
      // the final filename component, even if the search pattern
      // included a directory.
      // This is going to be a bugger to fix...
      string directoryPrefix;
      size_t directoryPrefixLength = file_arg.find_last_of("/\\");
      if(directoryPrefixLength!=string::npos)
      {
        directoryPrefix = string(file_arg,0,directoryPrefixLength+1);
      }
      // Was it as easy as that?...

      int findnextReturnValue = 0;
      while (findnextReturnValue==0)
	{
	  string sFileName=directoryPrefix;
          sFileName.append(fd.name);
	  file_entry file_entry(sFileName,lang);
	  file_list.push_back(file_entry);
	  findnextReturnValue = _findnext(sh, &fd);
	}
      _findclose(sh);
#else
      file_entry file_entry(file_arg,lang);
      file_list.push_back(file_entry);
      cout << file_arg << endl;
#endif      
    }
}


/*
** method to parse all of the supplied list of files
*/
int Main::ParseFiles() 
{
  FILE *f;
  
  std::list<file_entry>::iterator file_iterator=file_list.begin();
  while(file_iterator!=file_list.end())
    {
	  const file_entry &entry=*file_iterator;

      string filename=entry.first;
      string file_language=entry.second;
      ParseStore ps(filename);

      // The following objects are used to assist in the parsing 
      // process.

      if(file_language.size()==0)
	{
	  file_language=CCCC_Options::getFileLanguage(filename);
	}

      // CCCC supports a convention that the language may include an
      // embedded '.', in which case the part before the . controls 
      // which parser runs, while the whole can be examined inside
      // the parser to check for special dialect handling.
      unsigned int period_pos=file_language.find(".");
      string base_language=file_language.substr(0,period_pos);

      f=fopen(filename.c_str(),"r");
      if( f == NULL ) 
	{
	  cerr << "Couldn't open " << filename << endl;
	} else {
	  DLGFileInput in(f);

	  // show progress 
	  cerr << "Processing " << filename;

	  // The first case is just to allow symetric handling
	  // of the optional inclusion of support for each language
	  if(0)
	    {
	    }
#ifdef CC_INCLUDED
	  else if(
	     (base_language=="c++") ||
	     (base_language=="c") 
	     )
	    {
	      cerr << " as C/C++ (" << file_language << ")" 
		   << endl;

	      CLexer theLexer(&in);
	      ANTLRTokenBuffer thePipe(&theLexer);
	      theLexer.setToken(&currentLexerToken);
	      CParser theParser(&thePipe);
	      ParseUtility pu(&theParser);

	      theParser.init(filename,file_language);

	      // This function turns of the annoying "guess failed" messages
	      // every time a syntactic predicate fails.
	      // This message is enabled by default when PCCTS is run with
	      // tracing turned on (as it is by default in this application).
	      // In the current case this is inappropriate as the C++ parser
	      // uses guessing heavily to break ambiguities, and we expect 
	      // large numbers of guesses to be tested and to fail.
	      // This message and the flag which gates it were added around 
	      // PCCTS 1.33 MR10. 
	      // If you are building with an earlier version, this line should
	      // cause an error and can safely be commented out.
	      theParser.traceGuessOption(-1);
	      theParser.start();
              files_parsed++;
	    }
#endif // CC_INCLUDED
#ifdef JAVA_INCLUDED
	  else if(base_language=="java")
	    {
	      cerr << " as Java" << endl;

	      JLexer theLexer(&in);
	      ANTLRTokenBuffer thePipe(&theLexer);
	      theLexer.setToken(&currentLexerToken);
	      JParser theParser(&thePipe);
	      theParser.init(filename,file_language);
	      theParser.traceGuessOption(-1);
	      theParser.compilationUnit();
              files_parsed++;
	    }
#endif // JAVA_INCLUDED
#ifdef ADA_INCLUDED
	  else if(base_language=="ada")
	    {
	      cerr << " as Ada" << endl;

	      ALexer theLexer(&in);
	      ANTLRTokenBuffer thePipe(&theLexer);
	      theLexer.setToken(&currentLexerToken);
	      AdaPrser theParser(&thePipe);
	      theParser.init(filename,file_language);
	      theParser.traceGuessOption(-1);
	      theParser.goal_symbol();
              files_parsed++;
	    }
#endif // ADA_INCLUDED
	  else if(base_language=="")
	  {
		cerr << " - no parseable language identified";		
	  }
	  else
	    {
	      cerr << "Unexpected language " << base_language.c_str()
		   << " (" << file_language.c_str() 
		   << ") for file " << filename.c_str() << endl;
	    }

	  // close the file
	  fclose(f);
	}

      file_iterator++;
    }

  return 0;
}

int Main::DumpDatabase()
{
  ofstream outfile(db_outfile.c_str());
  return prj->ToFile(outfile);
}

int Main::LoadDatabase()
{
  int retval=0;
  if(db_infile!="")
    {
      ifstream infile(db_infile.c_str());
      retval=prj->FromFile(infile);
    }
  return retval;
}

void Main::GenerateHtml()
{
  cerr << endl << "Generating HTML reports" << endl;

  CCCC_Html_Stream::GenerateReports(prj,report_mask,html_outfile,outdir);

}

void Main::GenerateXml()
{
  cerr << endl << "Generating XML reports" << endl;

  CCCC_Xml_Stream::GenerateReports(prj,report_mask,xml_outfile,outdir);

}

void Main::HandleDebugOption(const string& arg) 
{
  /*
  ** arg may either be a number, or a string of letters denoting
  ** facilities to be debugged.  
  ** the string of letters is the public way - allowing input of a 
  ** number is just there to support quickly adding a category of
  ** debug messages without having to change this file immediately
  */
  DebugMask=atoi(arg.c_str());
  for (int i=0; arg[i]!='\0'; i++) 
    {
      switch (arg[i]) {
      case 'p' :
	DebugMask |= PARSER;
	break;

      case 'l' :
	DebugMask |= LEXER;
	break;

      case 'c' :
	DebugMask |= COUNTER;
	break;

      case 'm' :
	DebugMask |= MEMORY;
	break;

      case 'x' :
      case 'X' :
	DebugMask = 0xFF;
	break;
      }
    }
}

void Main::HandleReportOption(const string& arg) {
  /*
  ** arg may either be a number, or a string of letters denoting
  ** reports to be generated  
  */
  report_mask=atoi(arg.c_str());
  for (int i=0; arg[i]!='\0'; i++) {
    switch (arg[i]) {

    case 'c':
      report_mask |= rtCONTENTS;
      break;

    case 's' :
      report_mask |= rtSUMMARY;
      break;

    case 'p' :
      report_mask |= rtPROC1;
      break;

    case 'P':
      report_mask |= rtPROC2;
      break;

    case 'r' :
      report_mask |= rtSTRUCT1;
      break;

    case 'R' :
      report_mask |= rtSTRUCT2;
      break;

    case 'S' :
      report_mask |= rtSEPARATE_MODULES;
      break;

    case 'o' :
      report_mask |= rtOODESIGN;
      break;

    case 'L' : // for 'listing' as s and S for 'source' are already used
      report_mask |= rtSOURCE;
      break;

    case 'j' :
      report_mask |= rtOTHER;
      break;

    case 'h' :
      report_mask |= rtCCCC;
      break;

    case 't' :
      report_mask |= rtSHOW_GEN_TIME;
      break;

    default:
      cerr << "Unexpected report requested:" << arg[i] << endl;
      PrintUsage(cerr);
      exit(-1);
    }
  }
}

/*
** giving credit where it is due
*/
void Main::PrintCredits(ostream& os) 
{
  // the principal purpose of the constructor is to set up the
  // two lots of boilerplate text that this class requires
  string version_string="Version ";
  version_string.append(CCCC_VERSION_STRING);

  const char *credit_strings[] =
  {
    "CCCC - a code counter for C and C++",
    "===================================",
    "",
    "A program to analyse C and C++ source code and report on",
    "some simple software metrics",
    version_string.c_str(),
    "Copyright Tim Littlefair, 1995, 1996, 1997, 1998, 1999, 2000", 
    "with contributions from Bill McLean, Herman Hueni, Lynn Wilson ", 
    "Peter Bell, Thomas Hieber and Kenneth H. Cox.", 
    "",
    "The development of this program was heavily dependent on",
    "the Purdue Compiler Construction Tool Set (PCCTS) ",
    "by Terence Parr, Will Cohen, Hank Dietz, Russel Quoung,",
    "Tom Moog and others.",
    "",
    "CCCC comes with ABSOLUTELY NO WARRANTY.",
    "This is free software, and you are welcome to redistribute it",
    "under certain conditions.  See the file COPYING in the source",
    "code distribution for details.",
    NULL
  };
  const char **string_ptr=credit_strings;
  while(*string_ptr!=NULL)
    {
      os << *string_ptr << endl;
      string_ptr++;
    }


}

void Main::DescribeOutput()
{
  if(files_parsed>0)
  {
      // make sure the user knows where the real output went
      // make sure the user knows where the real output went
      cerr << endl 
           << "Primary HTML output is in " << html_outfile << endl;
      if(report_mask & rtSEPARATE_MODULES)
      { 
         cerr << "Detailed HTML reports on modules and source are in " << outdir << endl;
      }
      cerr << "Primary XML output is in " << xml_outfile << endl ;
      if(report_mask & rtSEPARATE_MODULES)
      { 
         cerr << "Detailed XML reports on modules are in " << outdir << endl;
      }
      cerr << "Database dump is in " << db_outfile << endl << endl;
  }
  else
  {
      cerr << endl << "No files parsed on this run" << endl << endl;
  }
}

/* 
** the usage message is printed on cerr if unexpected options are found,
** and on cout if option --help is found.  
*/
void Main::PrintUsage(ostream& os) 
{
  const char *usage_strings[] = 
  { 
    "Usage: ",
    "cccc [options] file1.c ...  ",
    "Process files listed on command line.",
    "If the filenames include '-', read a list of files from standard input.",
    "Command Line Options: (default arguments/behaviour specified in braces)",
    "--help                   * generate this help message",
    "--outdir=<dname>         * directory for generated files {.cccc}",
    "--html_outfile=<fname>   * name of main HTML report {<outdir>/cccc.html}",
    "--xml_outfile=<fname>    * name of main XML report {<outdir>/cccc.xml}",
    "--db_infile=<fname>      * preload internal database from named file",
    "                           {empty file}",
    "--db_outfile=<fname>     * save internal database to file {<outdir>/cccc.db}",
    "--opt_infile=<fname>     * load options from named file {hard coded, see below}",
    "--opt_outfile=<fname>    * save options to named file {<outdir>/cccc.opt}",
    "--lang=<string>          * use language specified for files specified ",
    "                           after this option (c,c++,ada,java, no default)",
    "--report_mask=<hex>      * control report content ",
    "--debug_mask=<hex>       * control debug output content ",
    "                           (refer to ccccmain.cc for mask values)",
    "Refer to ccccmain.cc for usage of --report_mask and --debug_mask.",
    "Refer to cccc_opt.cc for hard coded default option values, including default ",
    "extension/language mapping and metric treatment thresholds.",
    NULL
  };
  const char **string_ptr=usage_strings;
  while(*string_ptr!=NULL)
    {
      os << *string_ptr << endl;
      string_ptr++;
    }
}

int Main::filesParsed()
{
  return files_parsed;
}

int main(int argc, char **argv)
{
  app=new Main;
  prj=new CCCC_Project;

  // process command line
  app->HandleArgs(argc, argv);

  // If we are still running, acknowledge those who helped
  app->PrintCredits(cerr);

  cerr << "Parsing" << endl;
  CCCC_Record::set_active_project(prj);
  app->ParseFiles();
  CCCC_Record::set_active_project(NULL);

  if(app->filesParsed()>0)
  {
      prj->reindex();
#ifdef _WIN32
      _mkdir(app->outdir.c_str());
#else
      mkdir(app->outdir.c_str(),0777);
#endif
      app->DumpDatabase();

      // generate html output
      app->GenerateHtml();
      app->GenerateXml();
  }

  app->DescribeOutput();
  delete app;
  delete prj;

  return 0;
}

