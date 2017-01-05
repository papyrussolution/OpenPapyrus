del pp.hlp
\bc\pp\obj\pphc pphelp.txt pp.hlp
bcc -ml -I\bc\toolkit\include;\bc\pp\include;\bc\pp\include\empty;\bc\include helpindx.cpp
helpindx.exe
del pp.hlp
copy pphelp_.hlp ..\obj\pp.hlp
del pphelp_.hlp
del helpindx.*
