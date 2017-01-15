if not exist local.rc goto no_local
copy ppmenu.rc+local.rc menu.rc
goto mak
:no_local
copy ppmenu.rc menu.rc
:mak
rc -r menu.rc
move pp.res pp.old
copy /B ppraw.res + menu.res pp.res
del menu.res
del menu.rc
