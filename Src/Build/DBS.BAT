if exist \papyrus\src\BUILD\NOTEBOOK.SIGN (
	set INIT_PATH=\papyrus\initbase\init
	set INIT_DDF_PATH=\papyrus\initbase\init_ddf
	set DDL_PATH=\papyrus\src\ppdbs
	set REDIST_PATH=\papyrus\src\redist
	set LOCAL_PATH=\papyrus\base\init
) else (
	set INIT_PATH=v:\papyrus\initbase\init
	set INIT_DDF_PATH=v:\papyrus\initbase\init_ddf
	set DDL_PATH=\papyrus\src\ppdbs
	set REDIST_PATH=\papyrus\src\redist
	set LOCAL_PATH=D:\papyrus\base\init
)

del       %INIT_PATH%\*.btr
del       %INIT_PATH%\*.ddf
xcopy     %INIT_DDF_PATH%\*.ddf %INIT_PATH%
..\..\tools\ddl %DDL_PATH%\ppdbs.ddl /SC /D%INIT_PATH% /F%INIT_PATH% /Q\papyrus\src\model\ppdbs.sql
@pause

del       %INIT_PATH%\tmp_*.btr
del       ..\src\include\ppdbs.h
move      %DDL_PATH%\ppdbs.h   \papyrus\src\include
del       ..\ppkernel\ppdbs.cpp
move      %DDL_PATH%\ppdbs.cpp ..\ppkernel
del       %REDIST_PATH%\ppfiles.7z
7z a      %REDIST_PATH%\ppfiles.7z %INIT_PATH%\*.*
xcopy     %INIT_PATH%\*.* %LOCAL_PATH% /Y
