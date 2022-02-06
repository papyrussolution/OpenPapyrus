rem D:\TOOLS\Perl64\perl\bin\perl.exe -I..\common collate-sbl

tclsh unicode\uniParse.tcl unicode\UnicodeData.txt 14.0.0 unicode\unicode-data.cc
D:\TOOLS\Perl64\perl\bin\perl.exe unicode\gen_c_istab unicode\c_istab.h

D:\TOOLS\Perl64\perl\bin\perl.exe -Icommon weight\collate-idf-norm weight
D:\TOOLS\Perl64\perl\bin\perl.exe -Icommon weight\collate-wdf-norm weight
