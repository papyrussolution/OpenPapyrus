// cccc_itm.h
#ifndef __CCCC_ITM_H
#define __CCCC_ITM_H

#include "cccc.h"

// Class CCCC_Item is a wrapper for a C++ standard string which allows
// insertion and extraction of fields using a standard delimiter.
// It is intended for use in the following contexts:
// 1. for transmission of extent information from the parser to the database
// 2. for transmission of option information from the main line to the database
// 3. for storage from the database to a flat file
// 4. for reloading from a flat file to the database.

class CCCC_Item
{
private:
  char delimiter;
  string buffer;
  bool good;

public:
  CCCC_Item(const string& s, char c);
  CCCC_Item(const string& s);
  CCCC_Item(); 

  bool Insert(const string& s); 
  bool Insert(const char* cptr); 
  bool Extract(string& s);
  bool Insert(int n); 
  bool Extract(int& n);
  bool Insert(char c);
  bool Extract(char& c);
  bool Insert(float f);
  bool Extract(float& f);

  bool ToFile(ofstream& ofstr);
  bool FromFile(ifstream& ifstr);
};
  
#endif



