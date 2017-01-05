//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Jan 20 11:13:16 GMT-0800 1998
// Last Modified: Sun Jan  9 10:34:22 PST 2000 (minor changes)
// Last Modified: Thu Aug  2 13:52:52 PDT 2001 (added stdlib.h for new gcc)
// Last Modified  Tue Apr  9 12:24:27 PST 2002 (added Mac options)
// Last Modified  Mon Jan 20 21:12:52 PST 2003 (enabled conversion from mac to other)
// Last Modified  Wed May 18 14:03:03 PDT 2005 (updated for newer C++ compilers)
// Last Modified  Sun Aug 21 21:41:47 PDT 2005 (fixed so running -d twice works)
// Web Address:   http://www-ccrma.stanford.edu/~craig/utility/flip/flip.cpp
// Syntax:        C++ 
// $Smake:        g++ -ansi -O3 -o %b %f -static && strip %b
// $Smake-osx:    g++2 -ansi -O3 -o %b %f && strip %b
//
// Description: Utility program to convert text files between
//              UNIX or Mac newlines and DOS linefeed + newlines.
//
//              Unix uses the character 0x0a (decimal 10) to end a line
//              while DOS uses two characters: 0x0d 0x0a, where
//              (hex number 0x0d is equal to 13 decimal).
//              Mac OSes (prior to OS X), use 0x0d to end a line.
//              "Can't we just all get along?"
//
//              Options are:
//                 -t guess the OS format type of a text file.
//                    If the file is in mixed format, determine
//                    that the file is in DOS format.
//                 -u make the file Unix/MAC conformant
//                 -d make the file DOS/Window conformant
//                 -m make the file Macintosh (<10) conformant
//              Multiple files can be processed at once.
//
//              I wrote this program so that I could have the same
//              program for both Unix and MS Windows programs, but
//              an easy way to accomplish the same thing in Unix is
//              with using the sed command.  To make Unix formatted 
//              newlines, run the command:
//                 sed 's/^M//' filename > temp; mv temp filename
//              where ^M is the "control-M" key or the return key
//              which is usually created on the command line by pressing 
//              control-v and then the enter key.  Be careful not to write 
//              the output of sed to the file you are modifying or you will 
//              lose the file.  Likewise, you can go from Unix to DOS 
//              formats by running the command:
//                 sed 's/$/^M/' filename > temp; mv temp filename
//
//              You can also use the PERL language interpreter
//              to convert the file contents:
//
//              Mac to Unix:
//                 perl -i -pe 's/\015/\012/g' mac-file
//              Mac to DOS:
//                 perl -i -pe 's/\015/\015\012/g' mac-file
//              Unix to Mac:
//                 perl -i -pe 's/\012/\015/g' unix-file
//              Unix to DOS:
//                 perl -i -pe 's/\012/\015\012/g' unix-file
//              DOS to Unix:
//                 perl -i -pe 's/\015\012/\012/g' mac-file
//              DOS to Mac:
//                 perl -i -pe 's/\015\012/\012/g' mac-file
// 
//              Note 0x0a (hex) = \012 (octal) = 10 (decimal)
//              Note 0x0d (hex) = \015 (octal) = 13 (decimal)
//
//              But why would you want to remember all of that when you
//              can do this with the flip program:
//
//              Anything to Unix:
//                 flip -u file
//              Anything to DOS:
//                 flip -d file
//              Anything to Macintosh:
//                 flip -m file
//              To determine what the current line-flavor is:
//                 flip -t file
//              
//              Note that the flip program is destructive.  It
//              overwrites the contents of the old file with the new
//              newline converted file.  Multiple files can be
//              converted at the same time with a wild card:
//                  flip -u *
//              This will flip newlines to the Unix style in all files
//              in the current directory.  You should not run the flip
//              program on binary data, because the 0x0a and 0x0d do
//              not necessarily mean newlines in binary data.
//

// define the following if you are compiling in MS-DOS/Windows 95/NT/98/2000
// #define MSDOS

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>

#define SSTREAM stringstream
#define CSTRING str().c_str()

using namespace std;

// function declarations
void exitUsage            (const char* command);
void translateToUnix      (const char* filename);
void translateToDos       (const char* filename);
void translateToMacintosh (const char* filename);
void determineType        (const char* filename);


///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) 
{
   if(argc < 3) 
	   exitUsage(argv[0]);
   if(argv[1][0] != '-') 
	   exitUsage(argv[0]);
   char option = argv[1][1];
   if (!(option == 'u' || option == 'd' || option == 't'|| option == 'm')) {
      exitUsage(argv[0]);
   }
   for(int i=0; i<argc-2; i++) {
      if(option == 'd') 
		  translateToDos(argv[i+2]);
      else if (option == 'u') 
		  translateToUnix(argv[i+2]);
      else if (option == 'm') 
		  translateToMacintosh(argv[i+2]);
      else 
		  determineType(argv[i+2]);
   }
   return 0;
}

///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// determineType
//

void determineType(const char* filename) 
{
	fstream infile(filename, ios::in);
   if (!infile) {
      cout << "Error: cannot find file " << filename << endl;
      return;
   }

   char ch;
   infile.get(ch);
   int d0count = 0;
   int a0count = 0;
   while (!infile.eof()) {
      if (ch == 0x0d) {
         d0count++;
      } else if (ch == 0x0a) {
         a0count++;
      }
      infile.get(ch);
   }

   infile.close();
   if ((a0count == d0count) && (d0count != 0)) {
      cout << filename << ": DOS" << endl;
   } else if ((a0count > 0) && (d0count == 0)) {
      cout << filename << ": UNIX" << endl;
   } else if ((a0count == 0) && (d0count > 0)) {
      cout << filename << ": MAC" << endl;
   } else if ((a0count > 0) && (d0count > 0)) {
      cout << filename << ": MIXED" << endl;
   } else {
      cout << filename << ": UNKNOWN" << endl;
   }
}



//////////////////////////////
//
// translateToDos
//

void translateToDos(const char* filename) 
{
	fstream infile(filename, ios::in|ios::binary);
   if (!infile) {
      cout << "Error: cannot find file: " << filename << endl;
      return;
   }
   SSTREAM outstring;
   char ch, lastch;
   infile.get(ch);
   lastch = ch;
   int peekch;
   while(!infile.eof()) {
      if(ch == 0x0a && lastch != 0x0d) {  // convert newline from Unix to MS-DOS
         outstring << (char)0x0d;
         outstring << ch;
         lastch = ch;
      } 
	  else if(ch == 0x0d) {             // convert newline from Mac to MS-DOS
         peekch = infile.peek();
         if(peekch != 0x0a) {
            outstring << ch;
            outstring << (char)0x0a;
            lastch = 0x0a;
         } 
		 else {
            lastch = 0x0d;
            // Bug fix here reported by Shelley Adams: running -d
            // twice in a row was generating Unix style newlines
            // without the following statement:
            outstring << (char)0x0d;
         }
      } 
	  else {
         outstring << ch;
         lastch = ch;
      }
      infile.get(ch);
   }
   infile.close();
   fstream outfile(filename, ios::out|ios::binary);
   if(!outfile) {
      cout << "Error: cannot write to file: " << filename << endl;
      return;
   }
   outstring << ends;
   outfile << outstring.CSTRING;
   outfile.close();
}

//////////////////////////////
//
// translateToMacintosh
//

void translateToMacintosh(const char* filename) 
{
	fstream infile(filename, ios::in);
   if (!infile) {
      cout << "Error: cannot find file: " << filename << endl;
      return;
   }

   SSTREAM outstring;
   char ch;
   infile.get(ch);
   int lastchar = '\0';
   while(!infile.eof()) {
      if (ch == 0x0a) {                        // convert newline from MSDOS to Mac
         if (lastchar == 0x0d) {
           // do nothing: already the newline was written 
         } 
		 else {
            outstring << (char)0x0d;           // convert newline from Unix to Mac
         }
      } 
	  else {
         outstring << ch;
      }
      lastchar = ch;
      infile.get(ch);      
   }

   infile.close();
   #ifdef MSDOS
      fstream outfile(filename, ios::out | ios::binary);
   #else
      fstream outfile(filename, ios::out);
   #endif
   if (!outfile.is_open()) {
      cout << "Error: cannot write to file: " << filename << endl;
      return;
   }
   outstring << ends;
   outfile << outstring.CSTRING;
   outfile.close();
}



//////////////////////////////
//
// translateToUnix
//

void translateToUnix(const char* filename) 
{
	fstream infile(filename, ios::in);
   if (!infile) {
      cout << "Error: cannot find file: " << filename << endl;
      return;
   }

   SSTREAM outstring;
   char ch, lastch;
   infile.get(ch);
   while (!infile.eof()) {
      if (ch == 0x0d) {
         outstring << (char)0x0a;
      } else if (ch == 0x0a) {
         if (lastch == 0x0d) {
            // do nothing: already converted MSDOS newline to Unix form
         } else {
            outstring << (char)0x0a;   // convert newline from Unix to Unix 
         }
      } else {
         outstring << ch;
      }
      lastch = ch;
      infile.get(ch);      
   }

   infile.close();
   #ifdef MSDOS
      fstream outfile(filename, ios::out | ios::binary);
   #else
      fstream outfile(filename, ios::out);
   #endif
   if (!outfile.is_open()) {
      cout << "Error: cannot write to file: " << filename << endl;
      return;
   }
   outstring << ends;
   outfile << outstring.CSTRING;
   outfile.close();
}
         


//////////////////////////////
//
// exitUsage
//

void exitUsage(const char* commandName) 
{
   cout << "\nUsage: " << commandName << " [-t|-u|-d|-m] filename[s]\n"
           "   Converts ASCII files between Unix, MS-DOS/Windows, or Macintosh newline formats\n\n"
           "   Options: \n"
           "      -u  =  convert file(s) to Unix newline format (newline)\n"
           "      -d  =  convert file(s) to MS-DOS/Windows newline format (linefeed + newline)\n"
           "      -m  =  convert file(s) to Macintosh newline format (linefeed)\n"
           "      -t  =  display current file type, no file modifications\n" 
        << endl;

   exit(1);
}
