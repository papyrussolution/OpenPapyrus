; PAPYRUS.NSI
; Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2016, 2017, 2018, 2019, 2020, 2021, 2022
; Скрипт создания инсталляции системы Papyrus
;
;
; Определения, которые должны быть заданы из-вне
; через параметры командной строки /DXXX=YYY
;
;!define PRODUCT_NAME "Papyrus"
;!define PRODUCT_VERSION "7.2.6"
;!define SRC_ROOT   "d:\papyrus"
; Установка сервера
;!define INSTALL_SERVER
; Установка клиента
;!define INSTALL_CLIENT
; Установка обновления
;!define INSTALL_UPDATE
; Установка Demo-версии
;!define INSTALL_DEMO
; Установка OpenSource-варианта сборки
;!define OPENSOURCE
; Дополнительные исполняемые и DLL-модули совместимые с Windows-XP
;!define XPCOMPAT

;
; Внимание: функция сборки дистрибутива, встроенная в Papyrus полагается на следующие значения префиксов дистрибутивов
;
!ifdef OPENSOURCE
	!define BASEPROD_NAME "OpenPapyrus"
	!define PRODUCT_PREFIX "OPpy"
!else
	!define BASEPROD_NAME "Papyrus"
	!define PRODUCT_PREFIX "Ppy"
!endif
!ifdef INSTALL_SERVER
	!define PRODUCT_NAME "${BASEPROD_NAME} Server"
!else ifdef INSTALL_UPDATE
	!define PRODUCT_NAME "${BASEPROD_NAME} Update"
!else ifdef INSTALL_CLIENT
	!define PRODUCT_NAME "${BASEPROD_NAME} Client"
!else ifdef INSTALL_DEMO
	!define PRODUCT_NAME "${BASEPROD_NAME} Demo"
!else ifdef INSTALL_MANUAL
	!define PRODUCT_NAME "${BASEPROD_NAME} Manual"
!else 
	!define PRODUCT_NAME "${BASEPROD_NAME}"
!endif
!define PRODUCT_PUBLISHER        "A.Fokin, A.Sobolev"
!define PRODUCT_WEB_SITE         "http://www.petroglif.ru"
!define PRODUCT_DIR_REGKEY       "Software\Papyrus\System\InstallPath"
!define PRODUCT_UNINST_KEY       "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY  "HKLM"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"
!define DL200XLA                 "dl200.xla"
!define SRC_REDIST               "${SRC_ROOT}\src\redist"
!define SRC_3P_REDIST            "${SRC_ROOT}\redist"
!define SRC_TOOLS                "${SRC_ROOT}\tools"
!define SRC_TARGET               "${SRC_ROOT}\PPY\BIN"
!define DIR_BIN                  "$INSTDIR\BIN"
!define DIR_DRV                  "${DIR_BIN}\DRV"
!define FILE_PPINI               "${DIR_BIN}\PP.INI"
!define DIR_PACK                 "$INSTDIR\PACK"
!define DIR_SBII                 "$INSTDIR\PACK\StyloBhtII"
!define DIR_LOG                  "$INSTDIR\LOG"
!define DIR_TEMP                 "$INSTDIR\DATA\TEMP"
!define DIR_ARC                  "$INSTDIR\DATA\ARC"

!ifndef PRODUCT_VERSION
	!define PRODUCT_VERSION "Null version"
!endif
;
; Утилита, собираемая проектом VersionSelector
;
!define VERSELDLL  "versel.dll"

SetCompressor lzma
; @v5.7.0 {
Caption "${PRODUCT_NAME} Setup"
;BGGradient 007070 00e0f0 FFFFFF
;InstallColors FF8080 000030
XPStyle on
BrandingText " "
; } @v5.7.0

;MUI 1.67 compatible ------
!define MUI_VERBOSE 4 ; @v5.7.0
!include "MUI2.nsh"
!include "Library.nsh"

; Languages
;!insertmacro MUI_LANGUAGE "English" ; The first language is the default language
;!insertmacro MUI_LANGUAGE "Russian" 
;!insertmacro MUI_LANGUAGE "German"
;!insertmacro MUI_LANGUAGE "Dutch"
;!insertmacro MUI_LANGUAGE "Portuguese"

;MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON   "${SRC_ROOT}\SRC\RSRC\ICO\P2.ICO"
!define MUI_UNICON "${SRC_ROOT}\SRC\RSRC\ICO\P2.ICO"

!ifdef INSTALL_UPDATE
	var Dialog
	var Label
	var Text
!endif

Function .onInit
	IfFileExists "$SYSDIR\${VERSELDLL}" 0 +2
	Delete "$SYSDIR\${VERSELDLL}"
FunctionEnd

!ifndef INSTALL_SERVER
	
	Function SelectDir
		;SetOutPath $TEMP\Papyrus             ; create temp directory
		;SetOverwrite on
		;File "${SRC_ROOT}\Src\build\${VERSELDLL}"

		; debug
		;MessageBox MB_OK "Calling ${TEMP}\Papyrus\${VERSELDLL}"
		; debug

		!insertmacro InstallLib DLL NOSHARED NOREBOOT_NOTPROTECTED ${VERSELDLL} $SYSDIR\${VERSELDLL} $SYSDIR
		Push $2
		Push $1
		Push $0
		StrCpy $0 "1" ${NSIS_MAX_STRLEN}
		System::Call 'versel::SelectVersion(i $HWNDPARENT,t r0r2,i 0) i.r1'
		StrCmp $1 '1' 0 +2
		StrCpy $INSTDIR $2
		Pop $0
		Pop $1
		Pop $2
	FunctionEnd
	
!endif

!ifdef INSTALL_UPDATE

	Function CheckLock
		ClearErrors
		Delete "${DIR_BIN}\ppw.exe"
		ifErrors +2 0
		StrCpy $0 "FREE"
		Return
		StrCpy $0 "LOCKED"
	FunctionEnd

	Function PrepareIfLocked
		call CheckLock
		StrCmp $0 "FREE" exit
		; спросить пользователя как долго подождать
		nsDialogs::Create /NOUNLOAD 1018
		Pop $Dialog
		${If} $Dialog == error
			Return
		${EndIf}
		${NSD_CreateLabel} 0 0 100% 25u "Исполняемый файл приложения заблокирован. Укажите время ожидания (в секундах) до принудительного завершения (работающие пользователи будут заранее уведомлены):"
		Pop $Label
		${NSD_CreateText} 0 26u 20% 15u "300"
		Pop $Text
		nsDialogs::Show
	exit:
	FunctionEnd

	Function TryUnlock
		call CheckLock
		StrCmp $0 "FREE" continue
		${NSD_GetText} $Text $0
		FileOpen $1 "${DIR_BIN}\pplock" w
		IfErrors abort_locked
		FileWrite $1 "$0"	;seconds
		FileClose $1
	loop_check:
		Sleep 4096
		call CheckLock
		StrCmp $0 "FREE" continue loop_check
	abort_locked:
		MessageBox MB_ICONSTOP|MB_OK "Установка невозможна из-за того, что файл ${DIR_BIN}\ppw.exe заблокирован."
		Quit
	continue:
		Delete "${DIR_BIN}\pplock"
	FunctionEnd

!endif ; INSTALL_UPDATE
;
; Welcome page
;
!define MUI_WELCOMEPAGE_TITLE          "Вас приветствует мастер установки ${PRODUCT_NAME}"
!define MUI_WELCOMEFINISHPAGE_BITMAP   "${SRC_ROOT}\Src\Rsrc\Bitmap\nsis-welcome-02.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${SRC_ROOT}\Src\Rsrc\Bitmap\nsis-welcome-02.bmp"
!insertmacro MUI_PAGE_WELCOME
;
; License page
;
!ifdef OPENSOURCE
	!insertmacro MUI_PAGE_LICENSE "${SRC_ROOT}\SRC\Doc\license-gnu-affero.txt" ; @v10.8.2
!else
	!insertmacro MUI_PAGE_LICENSE "${SRC_ROOT}\SRC\Doc\LicenseAgreement.rtf"
!endif
;
; Components page
;
!ifdef INSTALL_SERVER | INSTALL_CLIENT | INSTALL_DEMO
	!insertmacro MUI_PAGE_COMPONENTS
!endif
;
; Directory page
;
!ifdef INSTALL_UPDATE | INSTALL_CLIENT | INSTALL_MANUAL
	!verbose push
	!verbose ${MUI_VERBOSE}
	!insertmacro MUI_PAGE_INIT
	!insertmacro MUI_SET MUI_${MUI_PAGE_UNINSTALLER_PREFIX}DIRECTORYPAGE 1
	!insertmacro MUI_DEFAULT MUI_DIRECTORYPAGE_TEXT_TOP ""
	!insertmacro MUI_DEFAULT MUI_DIRECTORYPAGE_TEXT_DESTINATION ""
	PageEx directory
		PageCallbacks SelectDir
		DirText "${MUI_DIRECTORYPAGE_TEXT_TOP}" "${MUI_DIRECTORYPAGE_TEXT_DESTINATION}"
		!ifdef MUI_DIRECTORYPAGE_VARIABLE
			DirVar "${MUI_DIRECTORYPAGE_VARIABLE}"
		!endif
		!ifdef MUI_DIRECTORYPAGE_VERIFYONLEAVE
			DirVerify leave
		!endif
	PageExEnd
	!ifdef INSTALL_UPDATE
		Page custom PrepareIfLocked TryUnlock
	!endif
	!undef MUI_DIRECTORYPAGE_TEXT_TOP
	!undef MUI_DIRECTORYPAGE_TEXT_DESTINATION
	!insertmacro MUI_UNSET MUI_DIRECTORYPAGE_VARIABLE
	!insertmacro MUI_UNSET MUI_DIRECTORYPAGE_VERIFYONLEAVE
	!verbose pop	
!else	
	!insertmacro MUI_PAGE_DIRECTORY
!endif
;
; Start menu page
;
!ifndef INSTALL_UPDATE
	var ICONS_GROUP
!endif

!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "${BASEPROD_NAME}"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${PRODUCT_STARTMENU_REGVAL}"
!ifdef INSTALL_SERVER | INSTALL_CLIENT | INSTALL_DEMO
	!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP
!endif	
;
; Instfiles page
;
!insertmacro MUI_PAGE_INSTFILES
;
; Finish page
;
!ifdef INSTALL_CLIENT | INSTALL_DEMO
	!define MUI_FINISHPAGE_RUN "$INSTDIR\BIN\ppw.exe"
!endif
!ifdef INSTALL_UPDATE
	!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
	!define MUI_FINISHPAGE_SHOWREADME_TEXT "Показать файл version.txt"
	!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\DOC\version.txt"
!endif

!define MUI_FINISHPAGE_TITLE "Завершение работы мастера установки ${PRODUCT_NAME}"
!insertmacro MUI_PAGE_FINISH	
;Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES
;Language files
!insertmacro MUI_LANGUAGE "Russian"
;Reserve files
; @v5.7.0 !insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
!ifdef INSTALL_SERVER
	OutFile ${PRODUCT_PREFIX}Server_${PRODUCT_VERSION}.exe
!else ifdef INSTALL_CLIENT
	OutFile ${PRODUCT_PREFIX}Client_${PRODUCT_VERSION}.exe
!else ifdef INSTALL_UPDATE
	OutFile ${PRODUCT_PREFIX}Update_${PRODUCT_VERSION}.exe
!else ifdef INSTALL_DEMO
	OutFile ${PRODUCT_PREFIX}Demo_${PRODUCT_VERSION}.exe
!else ifdef INSTALL_MANUAL
	OutFile ${PRODUCT_PREFIX}Manual_${PRODUCT_VERSION}.exe
!else	
	OutFile "setup.exe"
!endif
!ifdef OPENSOURCE
	InstallDir "C:\OPPY"
!else
	InstallDir "C:\PPY"
!endif
InstallDirRegKey HKCU "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show
;
; INSTALL_SERVER, INSTALL_UPDATE, INSTALL_DEMO
;
!ifdef INSTALL_SERVER | INSTALL_UPDATE | INSTALL_DEMO

Section "Файлы приложения" SEC01
	SetOutPath "${DIR_BIN}"
	SetOverwrite on
	File "${SRC_TARGET}\ppw.exe"
	File "${SRC_TARGET}\ppwmt.dll"
	!ifdef XPCOMPAT
		File "${SRC_TARGET}\ppw-xp.exe" ; @v10.6.1
		File "${SRC_TARGET}\ppwmt-xp.dll" ; @v10.6.1
	!endif
	File "${SRC_TARGET}\ppShuttleSG15_20.exe" ; @v7.4.10
	File "${SRC_TARGET}\PPSoapUhtt.dll"       ; @v7.3.0
	!ifndef OPENSOURCE
		File "${SRC_TARGET}\PPSoapPepsi.dll"      ; @v9.2.3
		File "${SRC_TARGET}\PPSoapEfes.dll"       ; @v9.5.5
		File "${SRC_TARGET}\PPSoapSfaHeineken.dll" ; @v9.9.9
	!endif
	File "${SRC_TARGET}\ppraw.res"
	; @v8.9.8 File "${SRC_TARGET}\pprpt.res"
	; @v5.8.8 File "${SRC_TARGET}\pp.str"
	File "${SRC_TARGET}\pp.res"
	; @v5.7.0 File "${SRC_TARGET}\dd.bin"
	File "${SRC_TARGET}\ppexp.bin"
	File "${SRC_TARGET}\ppifc.bin"
	File "${SRC_TARGET}\ppdbs.bin"
	File "${SRC_TARGET}\ppstr.bin"
	File "${SRC_TARGET}\ppstr-en.bin" ; @v10.4.4
	File "${SRC_TARGET}\ppstr-nl.bin" ; @v10.4.4
	File "${SRC_TARGET}\ppstr-de.bin" ; @v10.5.8
	File "${SRC_TARGET}\ppstr-pt.bin" ; @v10.5.8
	File "${SRC_TARGET}\ppstr-es.bin" ; @v11.2.0
	File "${SRC_TARGET}\ppdv.wta" ; @v9.2.0
	File "${SRC_TARGET}\pphelp.chm"
	File "${SRC_TARGET}\ppgplot.exe"
	File "${SRC_TARGET}\icudt70.dll" ; @v11.3.1 Данные ICU
	File "${SRC_TOOLS}\dl600c.exe"
	File "${SRC_TOOLS}\dl600c-xp.exe" ; @v10.9.3
	; @v8.9.8 File "${SRC_REDIST}\stdrpt.ini"	
	File "${SRC_ROOT}\src\rsrc\rpt\stdrpt.ini" ; @v8.9.8
	File "${SRC_REDIST}\mkmenu.bat"
    File "${SRC_REDIST}\ppos.exe"
	File "${SRC_ROOT}\src\include\ppmenu.h"
	File "${SRC_ROOT}\src\rsrc\rc\ppmenu.rc"
	!ifdef INSTALL_SERVER | INSTALL_DEMO
		File "${SRC_TOOLS}\rc.exe"
		File "${SRC_TOOLS}\rc.gid"
		File "${SRC_TOOLS}\rcdll.dll"
	!endif
	File "${SRC_ROOT}\src\rsrc\data\styloq_ignition_servers" ; @v11.3.10
	SetOverwrite ifnewer
	File "${SRC_ROOT}\Src\PPEquip\ppdrv.ini" ; @v7.2.6
	File "${SRC_REDIST}\dll\w3dbav80.dll"
	File "${SRC_REDIST}\dll\gdiplus.dll"
	File "${SRC_REDIST}\dll\fptr10.dll" ; @v10.3.9 драйвер торгового оборудования АТОЛ
	;
	; OpenSSL
	;
	; @v9.6.8 File "${SRC_ROOT}\src\osf\openssl-1.0.1e\bin\libeay32.dll" ; @v7.6.4
	; @v9.6.8 File "${SRC_ROOT}\src\osf\openssl-1.0.1e\bin\ssleay32.dll" ; @v7.6.4
	File "${SRC_ROOT}\src\rsrc\data\cacerts.pem"        ; @v7.9.12 
	;
	SetOverwrite off
	File "${SRC_REDIST}\clibnk.ini"
	File "${SRC_ROOT}\src\ppequip\stdslip.fmt"
	File "${SRC_ROOT}\src\ppequip\barlabel.lbl"

	SetOutPath "${DIR_DRV}"
	SetOverwrite on
	File "${SRC_TARGET}\ppdrv-pirit.dll"            ; @v7.2.5
	File "${SRC_TARGET}\ppdrv-cd-VikiVision.dll"    ; @v7.3.4
	File "${SRC_TARGET}\ppdrv-cd-VFD-Epson.dll"     ; @v7.3.4
	File "${SRC_TARGET}\ppdrv-cd-Shtrih-DPD201.dll" ; @v7.3.4
	File "${SRC_TARGET}\ppdrv-cd-posiflex.dll"      ; @v7.3.4
	File "${SRC_TARGET}\ppdrv-cd-FlyTechVFD-Epson.dll" ; @v8.2.11
	File "${SRC_TARGET}\ppdrv-ie-korus.dll"         ; @v7.6.5
	File "${SRC_TARGET}\ppdrv-ie-edisoft.dll"       ; @v8.0.3
	File "${SRC_TARGET}\ppdrv-ie-kontur.dll"        ; @v8.2.7 
	File "${SRC_TARGET}\ppdrv-ie-leradata.dll"      ; @v8.3.0
	; @v11.0.6 File "${SRC_TARGET}\ppdrv-ie-alcodeclbill.dll"  ; @v7.6.7
	File "${SRC_TARGET}\ppdrv-ctrl-reversk2.dll"    ; @v7.8.1
	File "${SRC_TARGET}\ppdrv-crdr-emmarine.dll"    ; @v7.9.6
	File "${SRC_TARGET}\ppdrv-bnkt-sberbank.dll"    ; @v8.1.8
	File "${SRC_TARGET}\ppdrv-bnkt-emul.dll"        ; @v10.1.3
	File "${SRC_TARGET}\ppdrv-bnkt-inpas.dll"       ; @v10.1.3
	File "${SRC_TARGET}\ppdrv-bnkt-ingenicospdh.dll" ; @v11.0.9
	!ifdef XPCOMPAT
		File "${SRC_TARGET}\ppdrv-pirit-xp.dll"            ; @v10.6.4
		File "${SRC_TARGET}\ppdrv-bnkt-sberbank-xp.dll"    ; @v10.6.4
		File "${SRC_TARGET}\ppdrv-bnkt-inpas-xp.dll"       ; @v10.6.4
	!endif
	!ifdef INSTALL_UPDATE
		Exec "${DIR_BIN}\mkmenu.bat"
		;
		; @v5.3.3 { Изменение файла pp.ini при обновлении системы
		;
		WriteINIStr ${FILE_PPINI} "config" "supportmail" "psupport@petroglif.ru"
		;
		; } @v5.3.3
		;
	!endif
!ifdef INSTALL_SERVER | INSTALL_CLIENT | INSTALL_DEMO
	;
	; Shortcuts
	;
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
	CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
	CreateShortCut  "$SMPROGRAMS\$ICONS_GROUP\${BASEPROD_NAME}.lnk" "$INSTDIR\BIN\ppw.exe"
	CreateShortCut  "$DESKTOP\${BASEPROD_NAME}.lnk" "$INSTDIR\BIN\ppw.exe"
	!insertmacro MUI_STARTMENU_WRITE_END
!endif	
SectionEnd

Section -PPALDD SEC_PPALDD
	SetOutPath "$INSTDIR\DD"
	SetOverwrite on
	File "${SRC_ROOT}\SRC\RSRC\DL600\ppexp.dl6"
	File "${SRC_ROOT}\SRC\RSRC\DL600\makedl6.bat"
	File "${SRC_ROOT}\SRC\RSRC\DL600\makedl6-xp.bat" ; @v10.9.3
	File "${SRC_TOOLS}\dl200c.exe" ; @v7.5.3 SRC_REDIST-->SRC_TOOLS
	File "${SRC_REDIST}\pattern.dl2"
	File "${SRC_ROOT}\SRC\VBA\dl200.xla"
	;
	; @v8.3.8
	; Файлы данных (текстовые)
	;	
	File "${SRC_ROOT}\SRC\RSRC\DATA\RAR-AlcoholCategory.txt"
	File "${SRC_ROOT}\SRC\RSRC\DATA\replacer-rule-goods.sr" ; @v9.7.5
	File "${SRC_ROOT}\SRC\RSRC\DATA\editorlangmodel.xml" ; @v9.8.2
	File "${SRC_ROOT}\SRC\RSRC\DATA\editorstyles.xml"    ; @v9.8.2
	; @v10.8.3 Изображения
	File "${SRC_ROOT}\SRC\RSRC\BITMAP\nophoto.png"       ; @v10.8.3
	;
	; @v6.7.1
	; WTM-файлы
	;
	SetOutPath "$INSTDIR\DD\WTM"
	SetOverwrite on
	File "${SRC_ROOT}\SRC\RSRC\WTM\cafetable.wta"
	;
	; Примеры и шаблоны для работы на VBA
	;
	SetOutPath "$INSTDIR\DD\VBA\EXAMPLE"
	SetOverwrite on	
	File "${SRC_ROOT}\SRC\VBA\slib.bas"
	File "${SRC_ROOT}\SRC\VBA\template.xls"
	File "${SRC_ROOT}\SRC\VBA\BalanceSheet.xls"
	File "${SRC_ROOT}\SRC\VBA\IncomeStatement.xls"
	;
	;
	;
	!ifdef INSTALL_UPDATE
		SetOutPath "$INSTDIR\DD"
		; @v5.7.0 Exec "$INSTDIR\DD\makedd.bat"
		Exec "$INSTDIR\DD\makedl6.bat"
	!endif	
SectionEnd	

Section -DOC SEC_DOC
	SetOutPath "$INSTDIR\DOC"
	SetOverwrite on
	File "${SRC_ROOT}\SRC\DOC\version.txt"
	File "${SRC_ROOT}\SRC\DOC\qref.txt"
	File "${SRC_ROOT}\SRC\DOC\pp_ini.txt"
	File "${SRC_ROOT}\SRC\DOC\cmdline.txt"
	File "${SRC_ROOT}\SRC\DOC\import.txt"
SectionEnd

Section -Reports SEC_REP
	SetOutPath "${DIR_BIN}\RPT"
	SetOverwrite on
	File "${SRC_ROOT}\SRC\RSRC\CRPT\*.rpt"
	File "${SRC_ROOT}\SRC\RSRC\CRPT\*.ttf"
SectionEnd	

Section -DATADICT SEC_DATADICT
	!ifdef INSTALL_UPDATE
		Push $0
		ReadINIStr $0 ${FILE_PPINI} path sys
		SetOutPath $0
		Pop $0
	!else
		SetOutPath "$INSTDIR\DATA\SYS"
	!endif
	SetOverwrite on
	File "${SRC_ROOT}\BASE\INIT_DL6\*.ddf"
SectionEnd

Section -PACK SEC_PACK
	SetOutPath "${DIR_PACK}"
	SetOverwrite on
	; @v7.5.3 File "${SRC_REDIST}\ppfiles.7z"
	File "${SRC_TOOLS}\7z.exe"
	; @v7.5.3 File File "${SRC_ROOT}\BASE\empty.7z"
	; @v7.5.3 File File "${SRC_ROOT}\BASE\sample.7z"
	; @v9.4.9 {
	!ifdef OPENSOURCE
		File "${SRC_ROOT}\BASE\empty-open.zip"
		File "${SRC_ROOT}\BASE\sample-open.zip"
	!else
		File "${SRC_ROOT}\BASE\empty.zip"
		File "${SRC_ROOT}\BASE\sample.zip"
	!endif
	; } @v9.4.9
	File "${SRC_ROOT}\BASE\INIT_DDF\800\*.ddf"
SectionEnd

!ifndef OPENSOURCE
	Section -SBII SEC_SBII
		SetOutPath "${DIR_SBII}"
		SetOverwrite on
        	File "${SRC_3P_REDIST}\SBII\BHT400BWCE_StylobhtII.exe"
	        File "${SRC_3P_REDIST}\SBII\DatalogicMemor_StyloBhtII.exe"
        	File "${SRC_3P_REDIST}\SBII\Falcon4420_StylobhtII.exe"
	        File "${SRC_3P_REDIST}\SBII\MC6200S_StylobhtII.exe"
        	File "${SRC_3P_REDIST}\SBII\SymbolMC9090_StyloBhtII.exe"
	SectionEnd
!endif

!endif

!ifdef INSTALL_MANUAL
Section -MANUAL SEC_MAN
	SetOutPath "$INSTDIR\DOC"
	SetOverwrite on
	File "${SRC_ROOT}\ManWork\LaTex\ppmanual.pdf"
	CreateShortCut "$DESKTOP\PPY Manual.lnk" "$INSTDIR\DOC\ppmanual.pdf"
SectionEnd
!endif
;
; INSTALL_SERVER, INSTALL_DEMO
;
!ifdef INSTALL_SERVER | INSTALL_DEMO
Section -BASEEMPTY SEC_BASEEMPTY
	SetOutPath "$INSTDIR\DATA\EMPTY"
	SetOverwrite off
	!ifdef OPENSOURCE
		File "${SRC_ROOT}\BASE\OPENPAPYRUS\EMPTY\*.btr"
	!else
		File "${SRC_ROOT}\BASE\EMPTY\*.btr"
	!endif
SectionEnd

Section -BASESAMPLE SEC_BASESAMPLE
	SetOutPath "$INSTDIR\DATA\SAMPLE"
	SetOverwrite off
	!ifdef OPENSOURCE
		File "${SRC_ROOT}\BASE\OPENPAPYRUS\SAMPLE\*.btr"
	!else
		File "${SRC_ROOT}\BASE\SAMPLE\*.btr"
	!endif
SectionEnd

Section "Руководство пользователя" SEC_MANUAL
	SetOutPath "$INSTDIR\MANUAL"
	SetOverwrite on
	File "${SRC_ROOT}\ManWork\LaTex\ppmanual.pdf"
SectionEnd

Section "PervasiveSQL Server" SEC_PSQL_SRV
	;
	; Установлю флаг перезагрузки
	;
	;IfRebootFlag 0 +4
	;SetRebootFlag true
	;
	;	Вытащу имя диска, куда ставится сервер Papyrus
	;
	var /GLOBAL DiskName
	StrCpy $DiskName "$INSTDIR" 2
	;
	; Создам папку для хранения Log'а PSQL 8.0 Server
	;
	CreateDirectory "$DiskName\PVSW\BIN\MKDE\LOG"

	SetOutPath "$DiskName\PVSW\BIN"
	SetOverwrite ifnewer
	File "${SRC_3P_REDIST}\PVSWSrv\*.dll"
	File "${SRC_3P_REDIST}\PVSWSrv\*.exe"
	;
	; Удалю файлик sc.exe из папки для PSQL 8.0 Server (он там не нужен)
	;
	Delete "$DiskName\PVSW\BIN\sc.exe"
	;
	; Удалю файлик Msvcp60.dll из папки для PSQL 8.0 Server (он там не нужен)
	;
	Delete "$DiskName\PVSW\BIN\Msvcp60.dll"
	;
	; И скопирую их в системную директорию
	;
	SetOutPath "$SYSDIR"
	SetOverwrite ifnewer
	; @v9.7.10 File "${SRC_3P_REDIST}\PVSWSrv\sc.exe"
	; @v9.7.10 File "${SRC_3P_REDIST}\PVSWSrv\Msvcp60.dll"	
	;File "${SRC_3P_REDIST}\PVSWSrv\LegacyLM.dll"
	;
	; Регистрирую в системе сервисы PSQL
	; Сервис Pervasive.SQL (relational)
	;
	ExecWait  "sc create $\"Pervasive.SQL (relational)$\" Start= auto binPath= $DiskName\PVSW\BIN\w3sqlmgr.exe DisplayName= $\"Pervasive.SQL (relational)$\""
	;
	; Сервис Pervasive.SQL (transactional)
	;
	ExecWait  "sc create $\"Pervasive.SQL (transactional)$\" Start= auto binPath= $DiskName\PVSW\BIN\ntbtrv.exe DisplayName= $\"Pervasive.SQL (transactional)$\""
	;
	; И запускаю их
	;
	ExecWait  "sc start $\"Pervasive.SQL (relational)$\""
	ExecWait  "sc start $\"Pervasive.SQL (transactional)$\""
	;
	; Регистрирую dll, отвечающую за "ошибку 161" PSQL 8.0 Server
	;
	SetOutPath "$DiskName\PVSW\BIN\"
	RegDLL "$DiskName\PVSW\BIN\LegacyLM.dll"
	;
	; И вношу изменения в реестр
	;
	!define Error161 "CLSID\{2B241F22-57F7-4cde-BB41-EA3EC6EC40B8}"
	WriteRegStr HKCR "${Error161}"  "0"            "T9Y2SA6HN65DAZPAQ9G2DTY2"
	WriteRegStr HKCR "${Error161}"  "1"            "7L6Q9PX8CQES8DSJDXRSNFFEQ2ZS"
	WriteRegStr HKCR "${Error161}\Version"  ""            "-2"
	
	!define RegPSQL_ServerSettings "Software\Pervasive Software\MicroKernel Server Engine\Version 8\Settings"
	WriteRegDWORD     HKLM "${RegPSQL_ServerSettings}"  "Max MicroKernel Memory Usage"	60
	;WriteRegExpandStr HKLM "${RegPSQL_ServerSettings}"  "Supported Protocols"						"TCP/IP"
	WriteRegExpandStr HKLM "${RegPSQL_ServerSettings}"  "Transaction Logging"						"NO"
	WriteRegDWORD     HKLM "${RegPSQL_ServerSettings}"  "Remote Read Buffer Size"       8
	;	
	; Shortcuts
	;
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
	!insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section -SetupPPINI SEC_PPINI
	CreateDirectory "${DIR_TEMP}"
	CreateDirectory "${DIR_ARC}"
	CreateDirectory "${DIR_LOG}"
	CreateDirectory "$INSTDIR\IN"
	CreateDirectory "$INSTDIR\OUT"
	WriteINIStr ${FILE_PPINI} "path" "sys"  "$INSTDIR\DATA\SYS"
	WriteINIStr ${FILE_PPINI} "path" "dat"  "$INSTDIR\DATA\EMPTY"
	WriteINIStr ${FILE_PPINI} "path" "pack" "${DIR_PACK}"
	WriteINIStr ${FILE_PPINI} "path" "temp" "${DIR_TEMP}"
	WriteINIStr ${FILE_PPINI} "path" "log"  "${DIR_LOG}"
		
	WriteINIStr ${FILE_PPINI} "dbname" "empty" "Empty Base,$INSTDIR\DATA\Empty"
	WriteINIStr ${FILE_PPINI} "dbname" "sample" "Sample Base,$INSTDIR\DATA\SAMPLE"

	WriteINIStr ${FILE_PPINI} "backup" "Резервная копия normal" "normal,${DIR_ARC},1,0,5"
	
	WriteINIStr ${FILE_PPINI} "config" "supportmail" "psupport@petroglif.ru"
SectionEnd
!endif
;
; INSTALL_CLIENT, INSTALL_DEMO
;
!ifdef INSTALL_CLIENT | INSTALL_DEMO
	
Section "-Ярлыки на рабочем столе" SEC_SHORTCUTS
	;
	; Shortcuts
	;
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
	CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
	CreateShortCut  "$SMPROGRAMS\$ICONS_GROUP\${BASEPROD_NAME}.lnk" "$INSTDIR\BIN\ppw.exe"
	CreateShortCut  "$DESKTOP\${BASEPROD_NAME}.lnk" "$INSTDIR\BIN\ppw.exe"
	!insertmacro MUI_STARTMENU_WRITE_END
SectionEnd	
	
Section "PervasiveSQL Client" SEC_PSQL_CLI
	SetOutPath "$SYSDIR"
	SetOverwrite ifnewer
	File "${SRC_3P_REDIST}\PVSWClnt\*.dll"
	File "${SRC_3P_REDIST}\PVSWClnt\*.exe"

	!define RegPSQL_RouterSettings  "Software\Pervasive Software\Microkernel Router\Version 8\Settings"
	!define RegPSQL_ReqSettings     "Software\Pervasive Software\Btrieve Requester\Version 8\Settings"
	!define RegPSQL_CommReqSettings "Software\Pervasive Software\Communications Requester\Version 8\Settings"

	WriteRegExpandStr HKLM "${RegPSQL_RouterSettings}"  "Local"            "Yes"
	WriteRegExpandStr HKLM "${RegPSQL_RouterSettings}"  "Requester"        "Yes"
	WriteRegExpandStr HKLM "${RegPSQL_RouterSettings}"  "Use Cache Engine" "no"
	WriteRegExpandStr HKLM "${RegPSQL_RouterSettings}"  "Use IDS"          "no"
	WriteRegDWORD     HKLM "${RegPSQL_RouterSettings}"  "Load Retries"     5
	WriteRegDWORD     HKLM "${RegPSQL_RouterSettings}"  "Target Engine"    2
		
	WriteRegExpandStr HKLM "${RegPSQL_ReqSettings}"     "Embedded Spaces"  "no"
	WriteRegExpandStr HKLM "${RegPSQL_ReqSettings}"     "Splash Screen"    "no"
	
	WriteRegExpandStr HKLM "${RegPSQL_CommReqSettings}" "Enable AutoReconnect" "No"
	WriteRegExpandStr HKLM "${RegPSQL_CommReqSettings}" "Log Statistics"       "No"
	WriteRegExpandStr HKLM "${RegPSQL_CommReqSettings}" "Runtime server"       "No"
	WriteRegExpandStr HKLM "${RegPSQL_CommReqSettings}" "Supported protocols"  "No"
	WriteRegDWORD     HKLM "${RegPSQL_CommReqSettings}" "TCP connect timeout"  15
	;
	; Shortcuts
	;
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
	!insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section "CrystalReports 10.0" SEC_CR10
	;
	; Копируем *.dll
	;
	SetOutPath "$SYSDIR"
	SetOverwrite on
	File "${SRC_3P_REDIST}\CRR10\*.dll"
	;
	; Регистрируем dll
	;
	SetOutPath "$SYSDIR"
	RegDLL "$SYSDIR\crqe.dll"
	RegDLL "$SYSDIR\crtslv.dll"
	RegDLL "$SYSDIR\exportmodeller.dll"
	RegDLL "$SYSDIR\ReportRenderer.dll"
	RegDLL "$SYSDIR\webreporting.dll"
	RegDLL "$SYSDIR\pageobjectmodel.dll"
	RegDLL "$SYSDIR\rptdefmodel.dll"
	RegDLL "$SYSDIR\commonobjmodel.dll"
	;
	; Shortcuts
	;
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
	!insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

!define FontSubstReg "Software\Microsoft\Windows NT\CurrentVersion\FontSubstitutes"
!define MainRptFont "Times New Roman"

Section -SetupFontRegister SEC_FONTREG
	WriteRegExpandStr HKLM "${FontSubstReg}" "${MainRptFont},0"    "${MainRptFont},204"
	DeleteRegValue    HKLM "${FontSubstReg}" "${MainRptFont} CE,238"
	WriteRegExpandStr HKLM "${FontSubstReg}" "${MainRptFont} CE,0" "${MainRptFont},204"
SectionEnd

!endif

!ifndef INSTALL_MANUAL
Section -Post
	!ifdef INSTALL_CLIENT
		;
		; Создам папку для хранения UnInst.exe в AppData
		;
		CreateDirectory "$CommonFiles\PPY"
		;
		; записываем анинсталлер в директорию
		;
		WriteUninstaller "$CommonFiles\PPY\uninst.exe"
		WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$CommonFiles\PPY\uninst.exe"
	!else
		WriteUninstaller "$INSTDIR\uninst.exe"
		WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
	!endif
	WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\BIN\ppw.exe"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
	;WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\BIN\ppw.exe"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
	!ifdef INSTALL_UPDATE
		Delete "${DIR_BIN}\pplock"
	!endif
SectionEnd
!endif

;Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!ifdef INSTALL_SERVER | INSTALL_UPDATE | INSTALL_DEMO
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC01} "Основные исполняемые и вспомогательные файлы системы ${BASEPROD_NAME}"
!endif	
!ifdef INSTALL_SERVER | INSTALL_DEMO
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_PSQL_SRV} "Сервер базы данных Pervasive.SQL 8.0. Устанавливается на том компьютере, на котором будет располагаться база данных ${BASEPROD_NAME}"
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_MANUAL} "Руководство пользователя (Adobe Acrobat)"
!endif
!ifdef INSTALL_CLIENT | INSTALL_DEMO
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_PSQL_CLI} "Клиент сервера базы данных Pervasive.SQL 8.0"
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_CR10} "Crystal Reports 10.0 Подсистема создания отчетов"
!endif
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Function un.onUninstSuccess
	HideWindow
	MessageBox MB_ICONINFORMATION|MB_OK "Удаление программы $(^Name) было успешно завершено."
FunctionEnd

Function un.onInit
	MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Вы уверены в том, что желаете удалить $(^Name) и все компоненты программы?" IDYES +2
	Abort
FunctionEnd

!ifndef INSTALL_MANUAL
Section Uninstall
	!ifdef INSTALL_SERVER | INSTALL_CLIENT | INSTALL_DEMO
		!insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP
	!endif

	!ifdef INSTALL_SERVER | INSTALL_UPDATE | INSTALL_DEMO
		;
		; Остановлю сервисы PSQL Server
		;
		ExecWait  "sc stop $\"Pervasive.SQL (transactional)$\""
		ExecWait  "sc stop $\"Pervasive.SQL (relational)$\""
		;
		; И удалю их
		;
		ExecWait  "sc delete $\"Pervasive.SQL (transactional)$\""
		ExecWait  "sc delete $\"Pervasive.SQL (relational)$\""
		;
		; Вытащу имя диска, куда ставится сервер Papyrus
		;
		var /GLOBAL UnDiskName
		StrCpy $UnDiskName "$INSTDIR" 2
		;
		; ДЕ - Регистрирую LegacyLM.dll , отвечающую за "ошибку 161" PSQL 8.0 Server
		; UnRegDLL "$SysDir\LegacyLM.dll"
		SetOutPath "$UnDiskName\PVSW\BIN\"
		UnRegDLL "$UnDiskName\PVSW\BIN\LegacyLM.dll"
		;
		; И отменю изменения в реестр
		;
		!define UnError161 "CLSID\{2B241F22-57F7-4cde-BB41-EA3EC6EC40B8}"
		DeleteRegKey HKCR "${UnError161}"
		;
		; И удалю ее из системной дирректории (он там не нужен)
		;
		;Delete "$SysDir\LegacyLM.dll"
		;
		; Удалю папку Papyrus
		;RMDir /r "$INSTDIR"
		;
		; Удаляю папку PSQL Server ,пока не освободится ее последний файл
		;
		SetOutPath "$UnDiskName\temp"
		ClearErrors
		RMDir /r "$UnDiskName\PVSW"
		ifErrors  0 +3
			Rename "$UnDiskName\PVSW" "$UnDiskName\PVSW"
		GoTo -4

		Delete "$INSTDIR\uninst.exe"
		Delete "$INSTDIR\BIN\ppw.exe"
		;RMDir "$INSTDIR\BIN"
	!endif

	!ifdef INSTALL_CLIENT | INSTALL_DEMO
		;
		; Отменю регистрацию dll для CR10
		;
		UnRegDLL "$SYSDIR\crqe.dll"
		UnRegDLL "$SYSDIR\crtslv.dll"
		UnRegDLL "$SYSDIR\exportmodeller.dll"
		UnRegDLL "$SYSDIR\ReportRenderer.dll"
		UnRegDLL "$SYSDIR\webreporting.dll"
		UnRegDLL "$SYSDIR\pageobjectmodel.dll"
		UnRegDLL "$SYSDIR\rptdefmodel.dll"
		UnRegDLL "$SYSDIR\commonobjmodel.dll"
		;
		; Clean up CR10
		;
		Delete "$SYSDIR\commonobjmodel.dll"
		Delete "$SYSDIR\crdb_p2bbtrv.dll"
		Delete "$SYSDIR\crdb_pc_res_en.dll"
		Delete "$SYSDIR\crheapalloc.dll"
		Delete "$SYSDIR\crpe32.dll"
		Delete "$SYSDIR\crqe.dll"
		Delete "$SYSDIR\crqe_res_en.dll"
		Delete "$SYSDIR\crtslv.dll"
		Delete "$SYSDIR\crxf_html.dll"
		Delete "$SYSDIR\crxf_html_res_en.dll"
		Delete "$SYSDIR\crxf_pdf.dll"
		Delete "$SYSDIR\crxf_pdf_res_en.dll"
		Delete "$SYSDIR\crxf_rtf.dll"
		Delete "$SYSDIR\crxf_rtf_res_en.dll"
		Delete "$SYSDIR\crxf_wordw.dll"
		Delete "$SYSDIR\crxf_wordw_res_en.dll"
		Delete "$SYSDIR\crxf_xls.dll"
		Delete "$SYSDIR\crxf_xls_res_en.dll"
		Delete "$SYSDIR\cxlib-1-6.dll"
		Delete "$SYSDIR\cxlibw-1-6.dll"
		Delete "$SYSDIR\ExportModeller.dll"
		Delete "$SYSDIR\p2bbtrv.dll"
		Delete "$SYSDIR\p2ctbtrv.dll"
		Delete "$SYSDIR\p3tbten.dll"
		Delete "$SYSDIR\pageObjectModel.dll"
		Delete "$SYSDIR\ReportRenderer.dll"
		Delete "$SYSDIR\rptdefmodel.dll"
		Delete "$SYSDIR\u252000.dll"
		Delete "$SYSDIR\u25dts.dll"
		Delete "$SYSDIR\u25samp1.dll"
		Delete "$SYSDIR\u2dapp.dll"
		Delete "$SYSDIR\u2ddisk.dll"
		Delete "$SYSDIR\u2fcompress.dll"
		Delete "$SYSDIR\u2fsepv.dll" ; @v5.8 ANDREW
		Delete "$SYSDIR\u2ftext.dll"
		Delete "$SYSDIR\u2fxml.dll"
		Delete "$SYSDIR\u2l2000.dll"
		Delete "$SYSDIR\u2lcom.dll"
		Delete "$SYSDIR\u2ldts.dll"
		Delete "$SYSDIR\u2lexch.dll"
		Delete "$SYSDIR\u2lfinra.dll"
		Delete "$SYSDIR\u3520en.dll"
		Delete "$SYSDIR\u35dten.dll"
		Delete "$SYSDIR\u35s1en.dll"
		Delete "$SYSDIR\u3l20en.dll"
		Delete "$SYSDIR\u3ldten.dll"
		Delete "$SYSDIR\u3lfren.dll"
		Delete "$SYSDIR\u3lxcen.dll"
		Delete "$SYSDIR\ufmanager.dll"
		Delete "$SYSDIR\webReporting.dll"
		Delete "$SYSDIR\x3ddken.dll"
		Delete "$SYSDIR\x3fsven.dll" ; @v5.8 ANDREW
		Delete "$SYSDIR\x3fxmen.dll"
		Delete "$SYSDIR\nsclient100w.dll"
		Delete "$SYSDIR\nsldap32v50.dll"
		Delete "$SYSDIR\nsldappr32v50.dll"
		Delete "$SYSDIR\nsldapssl32v50.dll"
		Delete "$SYSDIR\nspr4.dll"
		Delete "$SYSDIR\nss3.dll"
		Delete "$SYSDIR\plc4.dll"
		Delete "$SYSDIR\plds4.dll"
		Delete "$SYSDIR\Roboex32.dll"
		Delete "$SYSDIR\s2dtconv.dll"
		Delete "$SYSDIR\ssl3.dll"
		Delete "$SYSDIR\stringres100_en.dll"
		;
		; Удалю ветки реестра для PSQL 8.0 Client
		;
		;!define RegPSQL_RouterSettings  "Software\Pervasive Software\Microkernel Router\Version 8\Settings"
		;!define RegPSQL_ReqSettings     "Software\Pervasive Software\Btrieve Requester\Version 8\Settings"
		;!define RegPSQL_CommReqSettings "Software\Pervasive Software\Communications Requester\Version 8\Settings"
		DeleteRegKey HKLM "${RegPSQL_RouterSettings}"
		DeleteRegKey HKLM "${RegPSQL_ReqSettings}"
		DeleteRegKey HKLM "${RegPSQL_CommReqSettings}"
		;
		; Clean up PSQL 8.0 Client
		;
		Delete "$SYSDIR\part.reg"
		Delete "$SYSDIR\pscore.dll"
		Delete "$SYSDIR\w3aif10B.dll"
		Delete "$SYSDIR\w3bif143.dll"
		Delete "$SYSDIR\w3btrv7.dll"
		Delete "$SYSDIR\w3crs10F.dll"
		Delete "$SYSDIR\w3dbav80.dll"
		Delete "$SYSDIR\w3dbsmgr.exe"
		Delete "$SYSDIR\w3enc106.dll"
		Delete "$SYSDIR\w3mif145.dll"
		Delete "$SYSDIR\w3nsl235.dll"
		Delete "$SYSDIR\w3scmv7.dll"
		Delete "$SYSDIR\W3UPI22D.dll"
		Delete "$SYSDIR\wbtrv32.dll"
		;
		; Remove remaining directories
		;
		RMDir "$CommonFiles\PPY"
		;RMDir "$SYSDIR\PSQLS\"
		;RMDir "$SYSDIR\Crystal\"
	!endif
	DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
	;
	; !!!
	;
	Delete "$DESKTOP\${BASEPROD_NAME}.lnk"
	Delete "$SMPROGRAMS\$ICONS_GROUP\${BASEPROD_NAME}.lnk"
	RMDir "$SMPROGRAMS\$ICONS_GROUP"
	DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
	;
	; !!!
	;
	SetAutoClose true
SectionEnd
!endif
