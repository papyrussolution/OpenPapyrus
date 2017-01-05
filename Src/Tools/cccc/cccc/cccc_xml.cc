/*
    CCCC - C and C++ Code Counter
    Copyright (C) 1994-2005 Tim Littlefair (tim_littlefair@hotmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
*/
// cccc_xml.cc

// this file defines XML output facilities for the CCCC project

#include "cccc.h"
#include "cccc_itm.h"
#include "cccc_xml.h"

// I would love to use the C++ standard preprocessor
// directive #if here, but I have had reports before now
// of people who are using compilers which only support
// #ifdef.
#ifdef CCCC_CONF_W32VC
#include <direct.h>
#else
#ifdef CCCC_CONF_W32BC
#include <direct.h>
#else
#include <unistd.h>
#endif
#endif

#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "cccc_utl.h"


// class static data members
CCCC_Project* CCCC_Xml_Stream::prjptr;
string CCCC_Xml_Stream::outdir;
string CCCC_Xml_Stream::libdir;

static const string XML_PREAMBLE = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
static const string XML_COMMENT_BEGIN = "<!--";
static const string XML_COMMENT_END = "-->";
static const string XML_TAG_OPEN_BEGIN = "<";
static const string XML_TAG_OPEN_END = ">";
static const string XML_TAG_CLOSE_BEGIN = "</";
static const string XML_TAG_CLOSE_END = ">";
static const string XML_TAG_INLINE_BEGIN = "<";
static const string XML_TAG_INLINE_END   = "/>";
static const string XML_SPACE         = " ";
static const string XML_NEWLINE       = "\n";
static const string XML_DQUOTE        = "\"";
static const string XML_EQUALS        = "=";

static const string PROJECT_NODE_NAME = "CCCC_Project";
static const string TIMESTAMP_NODE_NAME = "timestamp";
static const string SUMMARY_NODE_NAME = "project_summary";
static const string MODSUM_NODE_NAME = "module_summary";
static const string MODDET_NODE_NAME  = "module_detail";
static const string PROCSUM_NODE_NAME = "procedural_summary";
static const string PROCDET_NODE_NAME = "procedural_detail";
static const string STRUCTSUM_NODE_NAME = "structural_summary";
static const string STRUCTDET_NODE_NAME = "structural_detail";
static const string OODESIGN_NODE_NAME = "oo_design";
static const string OTHER_NODE_NAME    = "other_extents";
static const string REJECTED_NODE_NAME = "rejected_extent";
static const string NAME_NODE_NAME    = "name";
static const string MODULE_NODE_NAME  = "module";
static const string MEMBER_NODE_NAME  = "member_function";
static const string EXTENT_NODE_NAME  = "extent";
static const string SUPPLIERS_NODE_NAME = "suppliers";
static const string SUPMOD_NODE_NAME    = "supplier_module";
static const string CLIENTS_NODE_NAME = "clients";
static const string CLIMOD_NODE_NAME  = "client_module";
static const string DESC_NODE_NAME    = "description";
static const string SRCREF_NODE_NAME     = "source_reference";

static const string NOM_NODE_NAME       = "number_of_modules"; 
static const string LOC_NODE_NAME       = "lines_of_code";
static const string LOCPERMOD_NODE_NAME = "lines_of_code_per_module";
static const string LOCPERCOM_NODE_NAME = "lines_of_code_per_line_of_comment";
static const string LOCPERMEM_NODE_NAME = "lines_of_code_per_member_function";
static const string MVG_NODE_NAME       = "McCabes_cyclomatic_complexity";
static const string MVGPERMOD_NODE_NAME = "McCabes_cyclomatic_complexity_per_module";
static const string MVGPERCOM_NODE_NAME = "McCabes_cyclomatic_complexity_per_line_of_comment";
static const string MVGPERMEM_NODE_NAME = "McCabes_cyclomatic_complexity_per_member_function";
static const string COM_NODE_NAME       = "lines_of_comment";
static const string COMPERMOD_NODE_NAME = "lines_of_comment_per_module";
static const string COMPERMEM_NODE_NAME = "lines_of_comment_per_member_function";
static const string WMC1_NODE_NAME      = "weighted_methods_per_class_unity";
static const string WMCV_NODE_NAME      = "weighted_methods_per_class_visibility";
static const string DIT_NODE_NAME       = "depth_of_inheritance_tree";
static const string NOC_NODE_NAME       = "number_of_children";
static const string CBO_NODE_NAME       = "coupling_between_objects";
static const string IF4_NODE_NAME       = "IF4";
static const string IF4PERMOD_NODE_NAME = "IF4_per_module";
static const string IF4PERMEM_NODE_NAME = "IF4_per_member_function";
static const string IF4VIS_NODE_NAME   = "IF4_visible";
static const string IF4VISPERMOD_NODE_NAME   = "IF4_visible_per_module";
static const string IF4VISPERMEM_NODE_NAME = "IF4_visible_per_member_function";
static const string IF4CON_NODE_NAME   = "IF4_concrete";
static const string IF4CONPERMOD_NODE_NAME   = "IF4_concrete";
static const string IF4CONPERMEM_NODE_NAME = "IF4_concrete_per_member_function";
static const string FO_NODE_NAME        = "fan_out";
static const string FOV_NODE_NAME        = "fan_out_visible";
static const string FOC_NODE_NAME        = "fan_out_concrete";
static const string FI_NODE_NAME        = "fan_in";
static const string FIV_NODE_NAME        = "fan_in_visible";
static const string FIC_NODE_NAME        = "fan_in_concrete";
static const string REJ_LOC_NODE_NAME   = "rejected_lines_of_code";

static const string VALUE_ATTR          = "value";
static const string LEVEL_ATTR          = "level";
static const string FILE_ATTR           = "file";
static const string LINE_ATTR           = "line";
static const string VISIBLE_ATTR        = "visible";
static const string CONCRETE_ATTR       = "concrete";

static const string LEVEL_NORMAL        = "0";
static const string LEVEL_MEDIUM        = "1";
static const string LEVEL_HIGH          = "2";
static const string BOOL_FALSE          = "false";
static const string BOOL_TRUE           = "true";

static string ltrim(string value)
{
   const int MAX_LENGTH = 1000;
   int i = 0;
   while(i<value.size() && value[i]==' ')
   {
      ++i;
   }
   return value.substr(i,MAX_LENGTH);
}

void CCCC_Xml_Stream::GenerateReports(CCCC_Project* prj, 
				       int report_mask, 
				       const string& file, 
				       const string& dir) 
{ 
  prjptr=prj;
  outdir=dir;

  CCCC_Xml_Stream main_xml_stream(file.c_str(),"Report on software metrics");

  // For testing purposes, we want to be able to disable the inclusion
  // of the current time in the report.  This enables us to store a
  // reference version of the report in RCS and expect the program
  // to generate an identical one at regression testing time.
  if(report_mask & rtSHOW_GEN_TIME)
  {
     main_xml_stream.Timestamp();
  }

  if(report_mask & rtSUMMARY)
    {
      main_xml_stream.Project_Summary();
    }

  if(report_mask & rtPROC1)
    {
      main_xml_stream.Procedural_Summary();
    }

  if(report_mask & rtPROC2)
    {
      main_xml_stream.Procedural_Detail();
    }
  
  if(report_mask & rtOODESIGN)
    {
      main_xml_stream.OO_Design();
    }

  if(report_mask & rtSTRUCT1)
    {
      main_xml_stream.Structural_Summary();
    }
    
  if(report_mask & rtSTRUCT2)
    {
      main_xml_stream.Structural_Detail();
    }

  if(report_mask & rtSEPARATE_MODULES)
    {
      main_xml_stream.Separate_Modules();
    }

  if(report_mask & rtOTHER)
    {
      main_xml_stream.Other_Extents();
    }
}

CCCC_Xml_Stream::CCCC_Xml_Stream(const string& fname, const string& info)
{
  // cerr << "Attempting to open file in directory " << outdir.c_str() << endl;
  fstr.open(fname.c_str());
  if(fstr.good() != TRUE)
    {
      cerr << "failed to open " << fname.c_str() 
	   << " for output in directory " << outdir.c_str() << endl;
      exit(1);
    }
    
  fstr << XML_PREAMBLE << endl
       << XML_COMMENT_BEGIN << info << XML_COMMENT_END << endl
       << XML_TAG_OPEN_BEGIN << PROJECT_NODE_NAME << XML_TAG_OPEN_END << endl;
}

CCCC_Xml_Stream::~CCCC_Xml_Stream()
{
  fstr << XML_TAG_CLOSE_BEGIN << PROJECT_NODE_NAME << XML_TAG_CLOSE_END << endl; 
  fstr.close();
}

void CCCC_Xml_Stream::Timestamp()
{
      time_t generationTime=time(NULL);
      fstr << XML_TAG_OPEN_BEGIN << TIMESTAMP_NODE_NAME << XML_TAG_OPEN_END 
           << ctime(&generationTime) 
           << XML_TAG_CLOSE_BEGIN << TIMESTAMP_NODE_NAME << XML_TAG_CLOSE_END 	
           << endl;
}

void  CCCC_Xml_Stream::Project_Summary() {
  // calculate the counts on which all displayed data will be based
  int nom=prjptr->get_count("NOM");  // number of modules 
  int loc=prjptr->get_count("LOC");  // lines of code
  int mvg=prjptr->get_count("MVG");  // McCabes cyclomatic complexity
  int com=prjptr->get_count("COM");  // lines of comment
  int if4=prjptr->get_count("IF4");    // intermodule complexity (all couplings)
  int if4v=prjptr->get_count("IF4v");  // intermodule complexity (visible only)
  int if4c=prjptr->get_count("IF4c");  // intermodule complexity (concrete only)
  int rej=prjptr->rejected_extent_table.get_count("LOC");
  
  fstr << XML_TAG_OPEN_BEGIN << SUMMARY_NODE_NAME << XML_TAG_OPEN_END << endl;

  Put_Metric_Node(NOM_NODE_NAME,nom);
  Put_Metric_Node(LOC_NODE_NAME,loc,"LOCp");
  Put_Metric_Node(LOCPERMOD_NODE_NAME,loc,nom,"LOCper");
  Put_Metric_Node(MVG_NODE_NAME,mvg,"MVGp");
  Put_Metric_Node(MVGPERMOD_NODE_NAME,mvg,nom,"MVGper");
  Put_Metric_Node(COM_NODE_NAME,com,"COM");
  Put_Metric_Node(COMPERMOD_NODE_NAME,com,nom,"COMper");
  Put_Metric_Node(LOCPERCOM_NODE_NAME,loc,com,"L_C");
  Put_Metric_Node(MVGPERCOM_NODE_NAME,mvg,com,"M_C");
  Put_Metric_Node(IF4_NODE_NAME,if4);
  Put_Metric_Node(IF4PERMOD_NODE_NAME,if4,nom,"8.3");
  Put_Metric_Node(IF4VIS_NODE_NAME,if4v);
  Put_Metric_Node(IF4VISPERMOD_NODE_NAME,if4v,nom,"8.3");
  Put_Metric_Node(IF4CON_NODE_NAME,if4c);
  Put_Metric_Node(IF4CONPERMOD_NODE_NAME,if4c,nom,"8.3");
  Put_Metric_Node(REJ_LOC_NODE_NAME,rej,"REJ");
  fstr << XML_TAG_CLOSE_BEGIN << SUMMARY_NODE_NAME << XML_TAG_CLOSE_END << endl;
}

void CCCC_Xml_Stream::OO_Design() 
{
  fstr << XML_TAG_OPEN_BEGIN << OODESIGN_NODE_NAME << XML_TAG_OPEN_END << endl;

  CCCC_Module* mod_ptr=prjptr->module_table.first_item();
  int i=0;
  while(mod_ptr!=NULL)
    { 
      i++;
      if( mod_ptr->is_trivial() == FALSE)
	{
	  fstr << XML_TAG_OPEN_BEGIN << MODULE_NODE_NAME << XML_TAG_OPEN_END << endl;
	  Put_Label_Node(NAME_NODE_NAME,mod_ptr->name(nlSIMPLE).c_str(),0,"","");

	  CCCC_Metric wmc1(mod_ptr->get_count("WMC1"),"WMC1");
	  CCCC_Metric wmcv(mod_ptr->get_count("WMCv"),"WMCv");
	  CCCC_Metric dit(mod_ptr->get_count("DIT"),"DIT");
	  CCCC_Metric noc(mod_ptr->get_count("NOC"),"NOC");
	  CCCC_Metric cbo(mod_ptr->get_count("CBO"),"CBO");

	  Put_Metric_Node(WMC1_NODE_NAME,wmc1);
	  Put_Metric_Node(WMCV_NODE_NAME,wmcv);
	  Put_Metric_Node(DIT_NODE_NAME,dit);
	  Put_Metric_Node(NOC_NODE_NAME,noc);
	  Put_Metric_Node(CBO_NODE_NAME,cbo);

	  fstr << XML_TAG_CLOSE_BEGIN << MODULE_NODE_NAME << XML_TAG_CLOSE_END << endl;

	}
      mod_ptr=prjptr->module_table.next_item();
    }
  fstr << XML_TAG_CLOSE_BEGIN << OODESIGN_NODE_NAME << XML_TAG_CLOSE_END << endl;
}

void CCCC_Xml_Stream::Procedural_Summary() 
{

  fstr << XML_TAG_OPEN_BEGIN << PROCSUM_NODE_NAME << XML_TAG_OPEN_END << endl;

  CCCC_Module* mod_ptr=prjptr->module_table.first_item();
  int i=0;
  while(mod_ptr!=NULL)
    {
      i++;
      if( mod_ptr->is_trivial() == FALSE)
	{
	  fstr << XML_TAG_OPEN_BEGIN << MODULE_NODE_NAME << XML_TAG_OPEN_END << endl;
	  Put_Label_Node(NAME_NODE_NAME,mod_ptr->name(nlSIMPLE).c_str(),0,"","");

	  int loc=mod_ptr->get_count("LOC");
	  int mvg=mod_ptr->get_count("MVG");
	  int com=mod_ptr->get_count("COM");
	  CCCC_Metric mloc(loc,"LOCm");
	  CCCC_Metric mmvg(mvg,"MVGm");
	  CCCC_Metric ml_c(loc,com,"L_C");
	  CCCC_Metric mm_c(mvg,com,"M_C");

	  Put_Metric_Node(LOC_NODE_NAME,mloc);
	  Put_Metric_Node(MVG_NODE_NAME,mmvg);
	  Put_Metric_Node(COM_NODE_NAME,com);
	  Put_Metric_Node(LOCPERCOM_NODE_NAME,ml_c);
	  Put_Metric_Node(MVGPERCOM_NODE_NAME,mm_c);
	  fstr << XML_TAG_CLOSE_BEGIN << MODULE_NODE_NAME << XML_TAG_CLOSE_END << endl;
	}
      mod_ptr=prjptr->module_table.next_item();
    }

    fstr << XML_TAG_CLOSE_BEGIN << PROCSUM_NODE_NAME << XML_TAG_CLOSE_END << endl;
}

void CCCC_Xml_Stream::Structural_Summary() 
{
  fstr << XML_TAG_OPEN_BEGIN << STRUCTSUM_NODE_NAME << XML_TAG_OPEN_END << endl;

  CCCC_Module* module_ptr=prjptr->module_table.first_item();
  while(module_ptr!=NULL)
    {
      if(module_ptr->is_trivial()==FALSE)
	{
	  fstr << XML_TAG_OPEN_BEGIN << MODULE_NODE_NAME << XML_TAG_OPEN_END << endl;
	  Put_Label_Node(NAME_NODE_NAME,module_ptr->name(nlSIMPLE).c_str(),0,"","");

	  int fov=module_ptr->get_count("FOv");
	  int foc=module_ptr->get_count("FOc");
	  int fo=module_ptr->get_count("FO");

	  int fiv=module_ptr->get_count("FIv");
	  int fic=module_ptr->get_count("FIc");
	  int fi=module_ptr->get_count("FI");

	  int if4v=module_ptr->get_count("IF4v");
	  int if4c=module_ptr->get_count("IF4c");
	  int if4=module_ptr->get_count("IF4");

	  // the last two arguments here turn on links to enable jumping between
	  // the summary and detail cells for the same module
	  Put_Metric_Node(FOV_NODE_NAME,CCCC_Metric(fov,"FOv"));
	  Put_Metric_Node(FOC_NODE_NAME,CCCC_Metric(foc,"FOc"));
	  Put_Metric_Node(FO_NODE_NAME,CCCC_Metric(fo,"FO"));
	  Put_Metric_Node(FIV_NODE_NAME,CCCC_Metric(fiv,"FIv"));
	  Put_Metric_Node(FIC_NODE_NAME,CCCC_Metric(fic,"FIc"));
	  Put_Metric_Node(FI_NODE_NAME,CCCC_Metric(fi,"FI"));
	  Put_Metric_Node(IF4VIS_NODE_NAME,CCCC_Metric(if4v,"IF4v"));
	  Put_Metric_Node(IF4CON_NODE_NAME,CCCC_Metric(if4c,"IF4c"));
	  Put_Metric_Node(IF4_NODE_NAME,CCCC_Metric(if4,"IF4"));

	  fstr << XML_TAG_CLOSE_BEGIN << MODULE_NODE_NAME << XML_TAG_CLOSE_END << endl;
	}
      module_ptr=prjptr->module_table.next_item();
    }  
    fstr << XML_TAG_CLOSE_BEGIN << STRUCTSUM_NODE_NAME << XML_TAG_CLOSE_END << endl;
}

void CCCC_Xml_Stream::Put_Structural_Details_Node(
   CCCC_Module *mod, CCCC_Project *prj, int mask, UserelNameLevel nl)
{

#if 0
  std::cerr << "Relationships for " << mod->name(nlMODULE_NAME) 
	    << " (" << mod << ")" << std::endl;
#endif


  CCCC_Module::relationship_map_t::iterator iter;
  CCCC_Module::relationship_map_t *relationship_map=NULL;

  string nodeTag;

  if(mask==rmeCLIENT)
    {
      nodeTag = CLIMOD_NODE_NAME;	
      relationship_map=&(mod->client_map);

    }
  else if(mask==rmeSUPPLIER)
    {
      nodeTag = SUPMOD_NODE_NAME;	
      relationship_map=&(mod->supplier_map);
    }

  if(relationship_map==NULL)
    {
      cerr << "unexpected relationship mask " << mask  << endl;
    }
  else
    {
      fstr << XML_TAG_OPEN_BEGIN << nodeTag << XML_TAG_OPEN_END << endl;
      for(
	  iter=relationship_map->begin(); 
	  iter!=relationship_map->end();
	  iter++
	  )
	{
	  CCCC_UseRelationship *ur_ptr=(*iter).second;
	  Put_Label_Node(NAME_NODE_NAME, ur_ptr->name(nl).c_str(),0,"","");

	  AugmentedBool vis=ur_ptr->is_visible();
	  AugmentedBool con=ur_ptr->is_concrete();

          string visvalue = BOOL_FALSE;
          string convalue = BOOL_FALSE;
	  if(vis!=abFALSE)
	  {
             visvalue = BOOL_TRUE;
          }
          if(con!=abFALSE)
          {
             convalue = BOOL_TRUE;
          }
	  Put_Label_Node(VISIBLE_ATTR,visvalue,0,"","");
	  Put_Label_Node(CONCRETE_ATTR,convalue,0,"","");

	  Put_Extent_List(*ur_ptr,true);
	}
      fstr << XML_TAG_CLOSE_BEGIN << nodeTag << XML_TAG_CLOSE_END << endl;
    }
}

void CCCC_Xml_Stream::Structural_Detail() 
{
  fstr << XML_TAG_OPEN_BEGIN << STRUCTDET_NODE_NAME << XML_TAG_OPEN_END << endl;
  CCCC_Module* module_ptr=prjptr->module_table.first_item();
  while(module_ptr!=NULL)
    {
      if(module_ptr->is_trivial()==FALSE)
	{
	  Structural_Detail(module_ptr);
	}
      module_ptr=prjptr->module_table.next_item();
    }  
  fstr << XML_TAG_CLOSE_BEGIN << STRUCTDET_NODE_NAME << XML_TAG_CLOSE_END << endl;
}

void CCCC_Xml_Stream::Procedural_Detail() {

  fstr << XML_TAG_OPEN_BEGIN << PROCDET_NODE_NAME << XML_TAG_OPEN_END << endl;

  CCCC_Module* mod_ptr=prjptr->module_table.first_item();
  while(mod_ptr!=NULL)
    {
      if( 
	 (mod_ptr->name(nlMODULE_TYPE)!="builtin") &&
	 (mod_ptr->name(nlMODULE_TYPE)!="enum") &&
	 (mod_ptr->name(nlMODULE_TYPE)!="union") 
	 )
	{
          fstr << XML_TAG_OPEN_BEGIN << MODULE_NODE_NAME << XML_TAG_OPEN_END << endl;
	  Put_Label_Node(NAME_NODE_NAME,mod_ptr->name(nlSIMPLE).c_str(),50,
			 "procdet","procsum",mod_ptr);
	  Procedural_Detail(mod_ptr);
          fstr << XML_TAG_CLOSE_BEGIN << MODULE_NODE_NAME << XML_TAG_CLOSE_END << endl;
	}
      mod_ptr=prjptr->module_table.next_item();
    }  
  fstr << XML_TAG_CLOSE_BEGIN << PROCDET_NODE_NAME << XML_TAG_CLOSE_END << endl;
}

void CCCC_Xml_Stream::Other_Extents() 
{
   fstr << XML_TAG_OPEN_BEGIN << OTHER_NODE_NAME << XML_TAG_OPEN_END << endl;

   CCCC_Extent *extent_ptr=prjptr->rejected_extent_table.first_item();
   while(extent_ptr!=NULL)
   {
      fstr << XML_TAG_OPEN_BEGIN << REJECTED_NODE_NAME << XML_TAG_OPEN_END << endl;
      Put_Label_Node(NAME_NODE_NAME,extent_ptr->name(nlDESCRIPTION).c_str());
      Put_Extent_Node(*extent_ptr,0);
      Put_Metric_Node(LOC_NODE_NAME,extent_ptr->get_count("LOC"),"");
      Put_Metric_Node(COM_NODE_NAME,extent_ptr->get_count("COM"),"");
      Put_Metric_Node(MVG_NODE_NAME,extent_ptr->get_count("MVG"),"");
      extent_ptr=prjptr->rejected_extent_table.next_item();
      fstr << XML_TAG_CLOSE_BEGIN << REJECTED_NODE_NAME << XML_TAG_CLOSE_END << endl;
   }

   fstr << XML_TAG_CLOSE_BEGIN << OTHER_NODE_NAME << XML_TAG_CLOSE_END << endl;
}

void CCCC_Xml_Stream::Put_Label_Node(string nodeTag, string label, int width,
				     string ref_name, string ref_href, 
				     CCCC_Record *rec_ptr)
{
  if(label.size()>0)
    {
      fstr << XML_TAG_OPEN_BEGIN << nodeTag << XML_TAG_OPEN_END;
      *this << label; 
      fstr << XML_TAG_CLOSE_BEGIN << nodeTag << XML_TAG_CLOSE_END 
           << endl;
    }
  else
    {
	// Do nothing
    }

  if(rec_ptr != 0)
    {
      Put_Extent_List(*rec_ptr,true);
    }
}
  

void CCCC_Xml_Stream::Put_Metric_Node(string nodeTag,
				       int count, string tag)
{
  CCCC_Metric m(count, tag.c_str());
  Put_Metric_Node(nodeTag,m);
}

void CCCC_Xml_Stream::Put_Metric_Node(string nodeTag,
				       int num, int denom, string tag)
{
  CCCC_Metric m(num,denom, tag.c_str());
  Put_Metric_Node(nodeTag,m);
}

void  CCCC_Xml_Stream::Put_Metric_Node(string nodeTag,const CCCC_Metric& metric)
{
  fstr << XML_TAG_INLINE_BEGIN << nodeTag << XML_SPACE 
       << VALUE_ATTR << XML_EQUALS << XML_DQUOTE;
  *this << metric;
  fstr << XML_DQUOTE << XML_SPACE
       << LEVEL_ATTR << XML_EQUALS << XML_DQUOTE;

  switch(metric.emphasis_level())
    {
    case elMEDIUM:
      fstr << LEVEL_MEDIUM;
      break;
    case elHIGH:
      fstr << LEVEL_HIGH;
      break;
    default:
      fstr << LEVEL_NORMAL;
      break;
    }
  fstr << XML_DQUOTE << XML_SPACE << XML_TAG_INLINE_END << endl;
}

void CCCC_Xml_Stream::Put_Extent_URL(const CCCC_Extent& extent)
{
  string filename=extent.name(nlFILENAME);
  int linenumber=atoi(extent.name(nlLINENUMBER).c_str());
  fstr << XML_TAG_INLINE_BEGIN << SRCREF_NODE_NAME << XML_SPACE 
       << FILE_ATTR << XML_EQUALS << XML_DQUOTE;
  *this << filename;
  fstr << XML_DQUOTE << XML_SPACE
       << LINE_ATTR << XML_EQUALS << XML_DQUOTE;
  *this << linenumber;
  fstr << XML_DQUOTE << XML_SPACE << XML_TAG_INLINE_END << endl;
}

void CCCC_Xml_Stream::Put_Extent_Node(const CCCC_Extent& extent, int width, bool withDescription) 
{
  if(withDescription)
  {
     fstr << XML_TAG_OPEN_BEGIN << DESC_NODE_NAME << XML_TAG_OPEN_END 
          << extent.name(nlDESCRIPTION)
          << XML_TAG_CLOSE_BEGIN << DESC_NODE_NAME << XML_TAG_CLOSE_END 
          << endl;
  }
  Put_Extent_URL(extent);
}

void CCCC_Xml_Stream::Put_Extent_List(CCCC_Record& record, bool withDescription) 
{
  CCCC_Extent *ext_ptr=record.extent_table.first_item();
  while(ext_ptr!=NULL)
    {
      fstr << XML_TAG_OPEN_BEGIN << EXTENT_NODE_NAME << XML_TAG_OPEN_END
           << endl;
      if(withDescription)
      {
         fstr << XML_TAG_OPEN_BEGIN << DESC_NODE_NAME << XML_TAG_OPEN_END 
              << ext_ptr->name(nlDESCRIPTION)
              << XML_TAG_CLOSE_BEGIN << DESC_NODE_NAME << XML_TAG_CLOSE_END 
              << endl;
      }
      Put_Extent_URL(*ext_ptr);
      fstr << XML_TAG_CLOSE_BEGIN << EXTENT_NODE_NAME << XML_TAG_CLOSE_END
           << endl;
      ext_ptr=record.extent_table.next_item();
    }
}

// the next two methods define the two basic output operations through which
// all of the higher level output operations are composed
CCCC_Xml_Stream& operator <<(CCCC_Xml_Stream& os, const string& stg) 
{
  // initialise a character pointer to the start of the string's buffer
  const char *cptr=stg.c_str();
  while(*cptr!='\000') {
    char c=*cptr;
	
    // the purpose of this is to filter out the characters which
    // must be escaped in HTML
    switch(c) {
    case '>': os.fstr << "&gt;" ; break;
    case '<': os.fstr << "&lt;" ; break;
    case '&': os.fstr << "&amp;"; break;
    default : os.fstr << c;
    }
    cptr++;
  }
  return os;
}

CCCC_Xml_Stream& operator <<(CCCC_Xml_Stream& os, const CCCC_Metric& mtc) 
{
  // by writing to the underlying ostream object, we avoid the escape
  // functionality
  os.fstr << ltrim(mtc.value_string()) ;
  return os;
}

void CCCC_Xml_Stream::Separate_Modules()
{
  // this function generates a separate HTML report for each non-trivial
  // module in the database

  CCCC_Module* mod_ptr=prjptr->module_table.first_item();
  while(mod_ptr!=NULL)
    {
      int trivial_module=mod_ptr->is_trivial();
      if(trivial_module==FALSE)
	{
	  string info="Detailed report on module " + mod_ptr->key();
	  string filename=outdir;
	  filename+="/";
	  filename+=mod_ptr->key()+".xml";
	  CCCC_Xml_Stream module_xml_str(filename,info.c_str());

	  module_xml_str.Module_Summary(mod_ptr);

          module_xml_str.fstr 
             << XML_TAG_OPEN_BEGIN << MODDET_NODE_NAME << XML_TAG_OPEN_END
             << endl;
	  module_xml_str.Module_Detail(mod_ptr);
          module_xml_str.fstr 
             << XML_TAG_CLOSE_BEGIN << MODDET_NODE_NAME << XML_TAG_CLOSE_END
             << endl;
   
          module_xml_str.fstr 
             << XML_TAG_OPEN_BEGIN << PROCDET_NODE_NAME << XML_TAG_OPEN_END
             << endl;
	  module_xml_str.Procedural_Detail(mod_ptr);
          module_xml_str.fstr 
             << XML_TAG_CLOSE_BEGIN << PROCDET_NODE_NAME << XML_TAG_CLOSE_END
             << endl;

          module_xml_str.fstr 
             << XML_TAG_OPEN_BEGIN << STRUCTDET_NODE_NAME << XML_TAG_OPEN_END
             << endl;
	  module_xml_str.Structural_Detail(mod_ptr);
          module_xml_str.fstr 
             << XML_TAG_CLOSE_BEGIN << STRUCTDET_NODE_NAME << XML_TAG_CLOSE_END
             << endl;

	}
      else
	{
#if 0
	  cerr << mod_ptr->module_type << " " << mod_ptr->key() 
	       << " is trivial" << endl;
#endif
	}
      mod_ptr=prjptr->module_table.next_item();
    }
}

void CCCC_Xml_Stream::Module_Detail(CCCC_Module *module_ptr)
{
  // this function generates the contents of the table of definition
  // and declaration extents for a single module
  
  // the output needs to be enveloped in a pair of <TABLE></TABLE> tags
  // these have not been put within the function because it is designed
  // to be used in two contexts:
  // 1. within the Separate_Modules function, wrapped directly in the table
  //    tags
  // 2. within the Module_Detail function, where the table tags are 
  //    around the output of many calls to this function (not yet implemented)

  CCCC_Record::Extent_Table::iterator eIter = module_ptr->extent_table.begin();
  while(eIter!=module_ptr->extent_table.end())
  {
     CCCC_Extent *ext_ptr=(*eIter).second;
     Put_Extent_Node(*ext_ptr,0,true);
     int loc=ext_ptr->get_count("LOC");
     int mvg=ext_ptr->get_count("MVG");
     int com=ext_ptr->get_count("COM");
     CCCC_Metric mloc(loc,"LOCf");
     CCCC_Metric mmvg(mvg,"MVGf");
     CCCC_Metric ml_c(loc,com,"L_C");
     CCCC_Metric mm_c(mvg,com,"M_C");

     Put_Metric_Node(LOC_NODE_NAME,mloc);
     Put_Metric_Node(MVG_NODE_NAME,mmvg);
     Put_Metric_Node(COM_NODE_NAME,com);
     Put_Metric_Node(LOCPERCOM_NODE_NAME,ml_c);
     Put_Metric_Node(MVGPERCOM_NODE_NAME,mm_c);
   
     eIter++;
  }
}

void CCCC_Xml_Stream::Procedural_Detail(CCCC_Module *module_ptr)
{
  // this function generates the contents of the procedural detail table
  // relating to a single module
  
  // the output needs to be enveloped in a pair of <TABLE></TABLE> tags
  // these have not been put within the function because it is designed
  // to be used in two contexts:
  // 1. within the Separate_Modules function, wrapped directly in the table
  //    tags
  // 2. within the Procedural_Detail function, where the table tags are 
  //    around the output of many calls to this function

  CCCC_Module::member_map_t::iterator iter = module_ptr->member_map.begin();

      while(iter!=module_ptr->member_map.end())
	{ 
          fstr << XML_TAG_OPEN_BEGIN << MEMBER_NODE_NAME << XML_TAG_OPEN_END << endl;

	  CCCC_Member *mem_ptr=(*iter).second;
	  Put_Label_Node(NAME_NODE_NAME,mem_ptr->name(nlLOCAL).c_str(),0,"","",mem_ptr);
	  int loc=mem_ptr->get_count("LOC");
	  int mvg=mem_ptr->get_count("MVG");
	  int com=mem_ptr->get_count("COM");
	  CCCC_Metric mloc(loc,"LOCf");
	  CCCC_Metric mmvg(mvg,"MVGf");
	  CCCC_Metric ml_c(loc,com,"L_C");
	  CCCC_Metric mm_c(mvg,com,"M_C");

          Put_Metric_Node(LOC_NODE_NAME,mloc);
          Put_Metric_Node(MVG_NODE_NAME,mmvg);
          Put_Metric_Node(COM_NODE_NAME,com);
          Put_Metric_Node(LOCPERCOM_NODE_NAME,ml_c);
          Put_Metric_Node(MVGPERCOM_NODE_NAME,mm_c);

	  iter++;

          fstr << XML_TAG_CLOSE_BEGIN << MEMBER_NODE_NAME << XML_TAG_CLOSE_END << endl;
	}
}

void CCCC_Xml_Stream::Structural_Detail(CCCC_Module *module_ptr) 
{
  fstr << XML_TAG_OPEN_BEGIN << MODULE_NODE_NAME << XML_TAG_OPEN_END << endl;
  Put_Label_Node(NAME_NODE_NAME,module_ptr->name(nlSIMPLE).c_str(), 0, "","");
  Put_Structural_Details_Node(module_ptr, prjptr, rmeCLIENT, nlCLIENT);
  Put_Structural_Details_Node(module_ptr, prjptr, rmeSUPPLIER, nlSUPPLIER);
  fstr << XML_TAG_CLOSE_BEGIN << MODULE_NODE_NAME << XML_TAG_CLOSE_END << endl;
}

void CCCC_Xml_Stream::Module_Summary(CCCC_Module *module_ptr) 
{
  // calculate the counts on which all displayed data will be based
  // int nof=module_ptr->member_table.records(); // Number of functions
  int nof=0;
  int loc=module_ptr->get_count("LOC");  // lines of code
  int mvg=module_ptr->get_count("MVG");  // McCabes cyclomatic complexity
  int com=module_ptr->get_count("COM");  // lines of comment

  // the variants of IF4 measure information flow and couplings
  int if4=module_ptr->get_count("IF4");   // (all couplings)
  int if4v=module_ptr->get_count("IF4v"); // (visible only)
  int if4c=module_ptr->get_count("IF4c"); // (concrete only)

  int wmc1=module_ptr->get_count("WMC1"); // Weighted methods/class (unity)
  int wmcv=module_ptr->get_count("WMCv"); // Weighted methods/class (visible)
  int dit=module_ptr->get_count("DIT");   // depth of inheritance tree
  int noc=module_ptr->get_count("NOC");   // number of children
  int cbo=module_ptr->get_count("CBO");   // coupling between objects

  fstr << XML_TAG_OPEN_BEGIN << MODSUM_NODE_NAME << XML_TAG_OPEN_END << endl;

  Put_Metric_Node(LOC_NODE_NAME,loc,"LOCm");
  Put_Metric_Node(LOCPERMEM_NODE_NAME,loc,nof,"LOCg");
  Put_Metric_Node(MVG_NODE_NAME,mvg,"MVGm");
  Put_Metric_Node(MVGPERMEM_NODE_NAME,mvg,nof,"MVGf");
  Put_Metric_Node(LOC_NODE_NAME,com,"COMm");
  Put_Metric_Node(LOCPERMEM_NODE_NAME,com,nof,"8.3");
  Put_Metric_Node(LOCPERCOM_NODE_NAME,loc,com,"L_C");
  Put_Metric_Node(MVGPERCOM_NODE_NAME,mvg,com,"M_C");
  Put_Metric_Node(WMC1_NODE_NAME,wmc1);
  Put_Metric_Node(WMCV_NODE_NAME,wmcv);
  Put_Metric_Node(DIT_NODE_NAME,dit);
  Put_Metric_Node(NOC_NODE_NAME,noc);
  Put_Metric_Node(CBO_NODE_NAME,cbo);
  Put_Metric_Node(IF4_NODE_NAME,if4,1,"IF4");
  Put_Metric_Node(IF4PERMEM_NODE_NAME,if4,nof,"8.3");
  Put_Metric_Node(IF4VIS_NODE_NAME,if4v,1,"IF4v");
  Put_Metric_Node(IF4VISPERMEM_NODE_NAME,if4v,nof,"8.3");
  Put_Metric_Node(IF4CON_NODE_NAME,if4c,1,"IF4c");
  Put_Metric_Node(IF4CONPERMEM_NODE_NAME,if4c,nof,"8.3");

  fstr << XML_TAG_CLOSE_BEGIN << MODSUM_NODE_NAME << XML_TAG_CLOSE_END << endl;
}



void CCCC_Xml_Stream::Source_Listing()
{
}

#if 0
static string pad_string(int target_width, string the_string, string padding)
{
  int spaces_required=target_width-the_string.size();
  string pad_string;
  while(spaces_required>0)
    {
      pad_string+=padding;
      spaces_required--;
    }
  return pad_string+the_string;
}
#endif



#ifdef UNIT_TEST
int main()
{
  CCCC_Project *prj_ptr=test_project_ptr();
  CCCC_Xml_Stream os(*prj_ptr,"cccc.htm",".");
  return 0;
}
#endif


