
:make_resource
make -f ppres.mak
make -f ppinsres.mak

:create_makefiles
goto make_tools
prj2mak ppdb.prj
prj2mak ppslapi.prj
prj2mak pptv.prj
prj2mak pptvo.prj

prj2mak ppkernel.prj
prj2mak pplib.prj
prj2mak ppbill.prj
prj2mak ppequip.prj
prj2mak ppgoods.prj
prj2mak ppacc.prj
prj2mak ppperson.prj
prj2mak ppview.prj
prj2mak pp.prj

:make_tools
make -f ppdb.mak
make -f ppslapi.mak
make -f pptv.mak
make -f pptvo.mak

:copy_tools_lib
xcopy ..\obj\ppdb.lib    ..\lib /Y
xcopy ..\obj\ppslapi.lib ..\lib /Y
xcopy ..\obj\pptv.lib    ..\lib /Y
xcopy ..\obj\pptvo.lib   ..\lib /Y

:make_pp
make -f ppkernel.mak
make -f pplib.mak
make -f ppbill.mak
make -f ppequip.mak
make -f ppgoods.mak
make -f ppacc.mak
make -f ppperson.mak
make -f ppview.mak
make -f pp.mak

:make_install
rem make -f install.mak

:clean_up
goto done
del ppslapi.cfg
del ppdb.cfg
del pptv.cfg
del pptvo.cfg

del ppkernel.cfg
del pplib.cfg
del ppbill.cfg
del ppequip.cfg
del ppgoods.cfg
del ppacc.cfg
del ppperson.cfg
del ppview.cfg
del pp.cfg

:done
