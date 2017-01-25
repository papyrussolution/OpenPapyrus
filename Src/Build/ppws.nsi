; PPWS.NSI
; Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2017
; Скрипт создания инсталляции сервера задач системы Papyrus
;
!define PRODUCT_PUBLISHER "A.Fokin, A.Sobolev"
!define PRODUCT_WEB_SITE "http://www.petroglif.ru"
;!define PRODUCT_VERSION "5.8.x" ; @debug							!!!!
;!define SRC_ROOT   "c:\papyrus" ; @debug							!!!!
!define SRC_REDIST   "${SRC_ROOT}\src\redist"
!define SRC_TARGET   "${SRC_ROOT}\PPY\BIN"
!define PRODUCT_NAME "Papyrus Job Server"

!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

!define SERVICE_ALL_ACCESS    0xF01FF
!define SC_MANAGER_ALL_ACCESS 0x3F
!define SERVICE_CONTROL_STOP  1
!define FALSEPASSW            "FaLsEpAsSwOrD"
!define SERVICE_NAME          "PAPYRUS_SERVICE"
;
; Внимание: функция сборки дистрибутива, встроенная в Papyrus полагается на следующие значения префиксов дистрибутивов
;
!ifdef OPENSOURCE
	!define PRODUCT_PREFIX "OPpy"
!else
	!define PRODUCT_PREFIX "Ppy"
!endif

Var hwnd
Var hSCManager
Var hPPWS
Var FuncName

Var InstTaskServ
Var AccountName
Var Password

Var Err

SetCompressor lzma
Caption "Papyrus JobServer Setup"
BGGradient 007070 00e0f0 FFFFFF
InstallColors FF8080 000030
XPStyle on
BrandingText " "

;MUI 1.67 compatible ------
!include "MUI2.nsh"
!include "Library.nsh"
!include "FileFunc.nsh"

;MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${SRC_ROOT}\SRC\RSRC\ICO\P2.ICO"
!define MUI_UNICON "${SRC_ROOT}\SRC\RSRC\ICO\P2.ICO"
!define MUI_FINISHPAGE_NOAUTOCLOSE

Function .onInit
	InitPluginsDir
	File /oname=$PLUGINSDIR\ppws.ini "ppws.ini"
FunctionEnd

Function CheckPPWS
	StrCpy $hSCManager 0
	;
	; Установка соединения с менеджером системных служб
	;
	DetailPrint "Установка соединения с менеджером системных служб"
	StrCpy $FuncName "OpenSCManagerA"
	StrCpy $0 "false"
	System::Call 'advapi32::OpenSCManagerA(n, n, i ${SC_MANAGER_ALL_ACCESS})i.r4'
	StrCpy $hSCManager $4
	IntCmp $hSCManager 0 ERROR
	;
	; Если сервер задач установлен, получу указатель на него,
	;
	StrCpy $hPPWS 0
	DetailPrint "Проверка факта установки сервера задач"
	StrCpy $FuncName OpenServiceA
	System::Call 'advapi32::OpenServiceA(i $hSCManager, t ${SERVICE_NAME}, i ${SERVICE_ALL_ACCESS})i.r5'
	StrCpy $hPPWS $5
	IntCmp $hPPWS 0 DONE
	;
	; изменю надписи объектов окна
	;
	WriteINIStr "$PLUGINSDIR\ppws.ini" "Field 1" "text" "Обновление сервера задач"
	WriteINIStr "$PLUGINSDIR\ppws.ini" "Field 2" "text" "Обновить сервер задач"
	;
	; и заполню поле с login'ом
	;
	ReadRegStr $AccountName HKLM "SYSTEM\CurrentControlSet\Services\${SERVICE_NAME}" "ObjectName"
	WriteINIStr "$PLUGINSDIR\ppws.ini" "Field 3" "state" $AccountName
	WriteINIStr "$PLUGINSDIR\ppws.ini" "Field 4" "state" ${FALSEPASSW}
	WriteINIStr "$PLUGINSDIR\ppws.ini" "Field 5" "state" ${FALSEPASSW}
	GoTo DONE
ERROR:
	StrCpy $Err ERROR
	DetailPrint "Ошибка установки сервера задач. Функция - $FuncName"
	Abort
DONE:
FunctionEnd

Function ReloadPPWSWindow
	;
	; Подготовлю флаги установки сервера задач
	;
	GetDlgItem $1 $hwnd 1202 ; PathRequest control (1200 + field 3 - 1)
	EnableWindow $1 0
	GetDlgItem $1 $hwnd 1203 ; PathRequest control (1200 + field 4 - 1)
	EnableWindow $1 0
	GetDlgItem $1 $hwnd 1204 ; PathRequest control (1200 + field 5 - 1)
	EnableWindow $1 0
	StrCpy $InstTaskServ ""
	StrCpy $Password ""
	StrCpy $AccountName ""
	StrCpy $Err GOOD

	ReadINIStr $0 "$PLUGINSDIR\ppws.ini" "Field 2" "State"
	IntCmp $0 0 DONE
	StrCpy $InstTaskServ 1
	GetDlgItem $1 $hwnd 1202 ; PathRequest control (1200 + field 3 - 1)
	EnableWindow $1 1
	GetDlgItem $1 $hwnd 1203 ; PathRequest control (1200 + field 4 - 1)
	EnableWindow $1 1
	GetDlgItem $1 $hwnd 1204 ; PathRequest control (1200 + field 5 - 1)
	EnableWindow $1 1
DONE:
FunctionEnd

Function SetCustom
	call CheckPPWS
	;Display the InstallOptions dialog
	; In this mode InstallOptions returns the window handle so we can use it
	InstallOptions::initDialog /NOUNLOAD "$PLUGINSDIR\ppws.ini"
	Pop $hwnd
	call ReloadPPWSWindow
	InstallOptions::show
	Pop $0
FunctionEnd

Function ValidateCustom
	;
	; Пользователь нажал на объект
	; буду выяснять на какой
	;
	ReadINIStr $0 "$PLUGINSDIR\ppws.ini" "Settings" "State"
	IntCmp $0 0 NEXTBUTTON	;Кнопка Next?
	;
	; Иначе Reload страницы
	;
	call ReloadPPWSWindow
	Abort ; Return to the page
	;
	; Пользователь нажал на кнопку Далее
	;
NEXTBUTTON:
	;
	; Проверю корректность заполнения комба с именем и паролем, если надо,
	; если не выбрана установка сервера задач, то иду дальше
	;
	StrCmp $InstTaskServ "" DONE
	ReadINIStr $AccountName "$PLUGINSDIR\ppws.ini" "Field 3" "State"
	;StrCmp $AccountName "" ERRORNAME
	ReadINIStr $Password "$PLUGINSDIR\ppws.ini" "Field 4" "State"
	ReadINIStr $1 "$PLUGINSDIR\ppws.ini" "Field 5" "State"
	StrCmp $Password $1 DONE
	MessageBox MB_ICONEXCLAMATION|MB_OK "Некорректно введен пароль!"
	Abort ; Return to the page
;ERRORNAME:
;	MessageBox MB_ICONEXCLAMATION|MB_OK "Некорректно введена учетная запись!"
;	Abort ; Return to the page
DONE:
	ReadINIStr $AccountName "$PLUGINSDIR\ppws.ini" "Field 3" "State"
FunctionEnd
;
; Welcome page
;
!define MUI_WELCOMEPAGE_TITLE "Вас приветствеут мастер установки ${PRODUCT_NAME}"
!define MUI_WELCOMEFINISHPAGE_BITMAP   "${SRC_ROOT}\Src\Rsrc\Bitmap\nsis-welcome-02.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${SRC_ROOT}\Src\Rsrc\Bitmap\nsis-welcome-02.bmp"
!insertmacro MUI_PAGE_WELCOME
;
; Страница настройки параметров установки сервера задач
;
Page custom SetCustom ValidateCustom ": Сервер задач"
;
; СТРАНИЦА ДИРЕКТОРИИ {
;
!insertmacro GetDrives

Function GetDiskName
	StrCpy $ERR "ERRORPATH"
	StrCpy $1 "$INSTDIR" 3
	StrCmp $1 $9 +1 DONE
	StrCpy $0 "StopGetDrives"
	StrCpy $ERR ""
DONE:
	Push $0
FunctionEnd

Function CheckPath
	${GetDrives} "HDD" "GetDiskName"
	StrCmp $Err "ERRORPATH" +1 DONE
	MessageBox MB_OK "Указанный диск не является локальным"
	Abort
DONE:
FunctionEnd

Function SelectVersion
	System::Call 'versel::SelectVersion(i 0, t r0r2, i 0) i.r1'
	StrCmp $2 "" SET_DEFAULT_PATH
	StrCpy $INSTDIR "$2\bin"
	GoTo DONE
SET_DEFAULT_PATH:
	StrCpy $INSTDIR "C:\papyrus\ppy\bin"
DONE:
FunctionEnd

PageEx directory
	PageCallbacks SelectVersion "" CheckPath
PageExEnd

;!insertmacro MUI_PAGE_DIRECTORY
;
; } КОНЕЦ СТРАНИЦЫ ДИРЕКТОРИИ
;
;Start menu page
;var ICONS_GROUP
;!define MUI_STARTMENUPAGE_NODISABLE
;!define MUI_STARTMENUPAGE_DEFAULTFOLDER "Papyrus"
;!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
;!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
;!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${PRODUCT_STARTMENU_REGVAL}"

;Instfiles page
!insertmacro MUI_PAGE_INSTFILES

;Finish page
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
OutFile ${PRODUCT_PREFIX}JobSrvr_${PRODUCT_VERSION}.exe
ShowInstDetails show
ShowUnInstDetails show

Section "Файлы приложения" SEC01
	;
	; Остановлю сервис
	;
	DetailPrint "Остановка сервера задач"
	ExecWait '"$INSTDIR\ppws.exe" servicestop' $0
	IntCmp $0 0 +2
	DetailPrint "! Ошибка остановки сервиса"
	;
	; Скопирую exe файл сервера задач
	;
	SetOutPath "$INSTDIR"
	SetOverwrite on
	File "${SRC_TARGET}\ppws.exe"
	;DetailPrint "Де-регистрация сервера задач"
	;ExecWait '"$INSTDIR\ppws.exe" uninstall'
	;
	; Подготовка регистров
	;
	StrCpy $2 ppws
	ReadINIStr $R2 "$PLUGINSDIR\ppws.ini" "Field 3" "State"
	ReadINIStr $R3 "$PLUGINSDIR\ppws.ini" "Field 4" "State"
	;
	; Регистрация сервиса
	;
	DetailPrint "Регистрация cервера задач"
	ExecWait '"$INSTDIR\ppws.exe" install $R2 $R3' $0
	IntCmp $0 0 +2
	DetailPrint "! Ошибка регистрации сервиса"
	;
	; Запуск сервиса
	;
	DetailPrint "Запуск сервера задач"
	ExecWait '"$INSTDIR\ppws.exe" servicestart' $0
	IntCmp $0 0 +2
	DetailPrint "! Ошибка запуска сервиса"
SectionEnd

Section -Post
	StrCmp $Err ERROR DONE
	;
	; Создам папку для хранения UnInst.exe в AppData
	;
	CreateDirectory "$CommonFiles\PPWS"
	;
	; записываем анинсталлер в директорию
	;
	WriteUninstaller "$CommonFiles\PPWS\uninst.exe"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$CommonFiles\PPWS\uninst.exe"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\ppws.exe"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
DONE:
SectionEnd

Function un.onUninstSuccess
	HideWindow
	MessageBox MB_ICONINFORMATION|MB_OK "Удаление программы $(^Name) было успешно завершено."
FunctionEnd

Function un.onInit
	MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Вы уверены в том, что желаете удалить $(^Name) и все компоненты программы?" IDYES +2
	Abort
FunctionEnd

Section Uninstall
	Delete "$CommonFiles\PPWS\uninst.exe"
	DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
	StrCpy $hSCManager 0

	DetailPrint "Остановка сервера задач"
	ExecWait '"$INSTDIR\ppws.exe" stopservice'
	DetailPrint "Де-регистрация сервера задач"
	ExecWait '"$INSTDIR\ppws.exe" uninstall'
	;
	; Удалю exe файл
	;
	SetOutPath "$INSTDIR"
	ClearErrors
	Delete "$INSTDIR\ppws.exe"
	ifErrors  -2
	GoTo DONE
;ERROR:
;	DetailPrint "ОШИБКА удаления сервера задач"
DONE:
SectionEnd
