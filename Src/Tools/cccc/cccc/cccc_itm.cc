// cccc_itm.cc

#include "cccc.h"
#include <fstream>

#include "cccc_itm.h"
#include <cstdio>
#include <cmath>

CCCC_Item::CCCC_Item(const string& s, char c)
{
  buffer=s;
  delimiter=c;
  good=true;
}

CCCC_Item::CCCC_Item(const string& s) 
{
  buffer=s;
  delimiter='@';
  good=true;
}

CCCC_Item::CCCC_Item() 
{
  buffer="";
  delimiter='@';
  good=true;
}

bool CCCC_Item::Insert(const string& s)
{
  buffer+=s;
  buffer+=delimiter;
#if 0
  cerr << buffer << endl;
#endif
  return good;
}

bool CCCC_Item::Insert(const char* cptr)
{
  string s(cptr);
  return Insert(s);
}
 
bool CCCC_Item::Extract(string& s)
{
  size_t delimiter_position=buffer.find(delimiter);
  if(delimiter_position!=string::npos)
    {
      good=true;
      s=buffer.substr(0,delimiter_position);
      string tempBuffer=buffer.substr(delimiter_position+1);
	  buffer=tempBuffer;
    }
  else
    { 
      good=false;
    }
  return good;
}

bool CCCC_Item::Insert(int n)
{
  char numbuf[64];
  sprintf(numbuf,"%d",n);
  return Insert(numbuf);
}

bool CCCC_Item::Extract(int& n)
{
  string numstr;
  bool retval=Extract(numstr);
  n=atoi(numstr.c_str());
  return retval;
}

bool CCCC_Item::Insert(char c)
{
  char charbuf[2];
  sprintf(charbuf,"%c",c);
  return Insert(charbuf);
}

bool CCCC_Item::Extract(char& c)
{
  string charstr;
  bool retval=Extract(charstr);
  if(charstr.size()==1)
    {
      c=charstr[0];
    }
  return retval;
}

bool CCCC_Item::Insert(float f)
{
  char numbuf[64];
  sprintf(numbuf,"%f",f);
  return Insert(numbuf);
}

bool CCCC_Item::Extract(float& f)
{
  string numstr;
  bool retval=Extract(numstr);
  f=atof(numstr.c_str());
  return retval;
}

bool CCCC_Item::ToFile(ofstream& ofstr)
{
  ofstr << buffer << endl;
  good=ofstr.good();
  return good;
}

bool CCCC_Item::FromFile(ifstream& ifstr)
{
  good=false;
  char line_buffer[1024];
  ifstr.getline(line_buffer,1023);
  buffer=line_buffer;
  if(ifstr.good() && buffer.size()>0 && buffer.size()<1023)
    {
      delimiter=buffer[buffer.size()-1];
      good=true;
#if 0
      cerr << "Delimiter is " << delimiter << endl;
#endif
    }
  return good;
}

  


