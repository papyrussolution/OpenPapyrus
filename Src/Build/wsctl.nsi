; WSCTL.NSI @construction
; Copyright (c) A.Sobolev 2024
; Скрипт для создания дистрибутива компонента WSCTL
;
!define BASEPROD_NAME "WSCTL"

!define PRODUCT_PUBLISHER "A.Fokin, A.Sobolev"
!define PRODUCT_WEB_SITE "http://www.petroglif.ru"
!define SRC_REDIST               "${SRC_ROOT}\src\redist"
!define SRC_TARGET               "${SRC_ROOT}\PPY\BIN"
!define SRC_3P_REDIST            "${SRC_ROOT}\redist"
!define SRC_TOOLS                "${SRC_ROOT}\tools"
!define DIR_BIN                  "$INSTDIR\BIN"
!define DIR_DRV                  "${DIR_BIN}\DRV"
!define FILE_PPINI               "${DIR_BIN}\PP.INI"
!define DIR_PACK                 "$INSTDIR\PACK"
!define DIR_LOG                  "$INSTDIR\LOG"
!define DIR_TEMP                 "$INSTDIR\DATA\TEMP"
!define PRODUCT_NAME            "WSCTL"

!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

SetCompressor lzma
Caption "${PRODUCT_NAME} Setup"
;XPStyle on
BrandingText " "

!define MUI_VERBOSE 4
!include "MUI2.nsh"
!include "Library.nsh"

!define MUI_ABORTWARNING
!define MUI_ICON   "${SRC_ROOT}\SRC\RSRC\ICO\P2.ICO"
!define MUI_UNICON "${SRC_ROOT}\SRC\RSRC\ICO\P2.ICO"
;
; “тилита, собираемаЯ проектом VersionSelector
;
!define VERSELDLL  "versel.dll"

Function .onInit
	IfFileExists "$SYSDIR\${VERSELDLL}" 0 +2
	Delete "$SYSDIR\${VERSELDLL}"
FunctionEnd

Function SelectDir
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
!insertmacro MUI_PAGE_LICENSE "${SRC_ROOT}\SRC\Doc\LicenseAgreement.rtf"
;
; Components page
;
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
;
; Start menu page
;
var ICONS_GROUP

!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "${BASEPROD_NAME}"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${PRODUCT_STARTMENU_REGVAL}"
!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP
;
; Instfiles page
;
!insertmacro MUI_PAGE_INSTFILES
;
; Finish page
;
!define MUI_FINISHPAGE_TITLE "Завершение работы мастера установки ${PRODUCT_NAME}"
!insertmacro MUI_PAGE_FINISH	
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Russian"


Section "Файлы приложения" SEC01
	SetOutPath "${DIR_BIN}"
	SetOverwrite on
	File "${SRC_TARGET}\wsctl.exe"
	File "${SRC_TARGET}\pp.res"
	File "${SRC_TARGET}\ppstr.bin"
	File "${SRC_TARGET}\ppstr-en.bin"
	File "${SRC_TARGET}\ppstr-nl.bin"
	File "${SRC_TARGET}\ppstr-de.bin"
	File "${SRC_TARGET}\ppstr-pt.bin"
	File "${SRC_TARGET}\ppstr-es.bin"
	File "${SRC_TARGET}\ppdv.wta"
	File "${SRC_TARGET}\icudt70.dll"
	File "${SRC_TARGET}\icudt70l.dat"
SectionEnd

Section -PPALDD SEC_PPALDD
	SetOutPath "$INSTDIR\DD"
	SetOverwrite on
	File "${SRC_ROOT}\SRC\RSRC\BITMAP\nophoto.png"
	;
	; Fonts
	;
	SetOutPath "$INSTDIR\DD\Font"
	File "${SRC_ROOT}\SRC\RSRC\Font\imgui\Cousine-Regular.ttf"
	File "${SRC_ROOT}\SRC\RSRC\Font\imgui\DroidSans.ttf"
	File "${SRC_ROOT}\SRC\RSRC\Font\imgui\Karla-Regular.ttf"
	File "${SRC_ROOT}\SRC\RSRC\Font\imgui\ProggyClean.ttf"
	File "${SRC_ROOT}\SRC\RSRC\Font\imgui\ProggyTiny.ttf"
	File "${SRC_ROOT}\SRC\RSRC\Font\imgui\Roboto-Medium.ttf"
	;
	; Styles
	;
	SetOutPath "$INSTDIR\DD\UID"
	File "${SRC_ROOT}\SRC\RSRC\Style\uid-papyrus.json"
	File "${SRC_ROOT}\SRC\RSRC\Style\uid-wsctl.json"
SectionEnd	

Section Uninstall
SectionEnd

