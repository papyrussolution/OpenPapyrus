lex -e rptlex
move rptlex.c rptlex.cpp
yacc -d -r -v -b rpty rptyy.y
move rpty_cod.c rpty.cpp
move rpty_tab.c rpty_tab.cpp
rem move rpty_tab.h _rpty.h
