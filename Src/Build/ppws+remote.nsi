; PPWS.NSI
; Copyright (c) A.Sobolev 2005, 2006, 2007, 2008
; Скрипт создания инсталляции сервера задач системы Papyrus
; *******************************************************************************
; Definitions {
!define PRODUCT_PUBLISHER "A.Fokin, A.Sobolev"
!define PRODUCT_WEB_SITE "http://www.petroglif.ru"
!define PRODUCT_VERSION "5.7.5" ; @debug
!define SRC_ROOT   "j:\papyrus" ; @debug
!define SRC_REDIST   "${SRC_ROOT}\src\redist"
!define SRC_TARGET   "${SRC_ROOT}\PPY\BIN"
!define PRODUCT_NAME "Papyrus Job Server"

!define PRODUCT_UNINST_ROOT_KEY "HKLM"
; HKEY_CLASSES_ROOT		= 0x80000000
; HKEY_CURRENT_USER		= 0x80000001
; HKEY_LOCAL_MACHINE	= 0x80000002
; HKEY_USERS			= 0x80000003
; HKEY_CURRENT_CONFIG	= 0x80000004
!define PRODUCT_UNINST_ROOT_KEY_ID 0x80000002 ; for "HKLM"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

!define SERVICE_ALL_ACCESS    0xF01FF
!define SC_MANAGER_ALL_ACCESS 0x3F
!define SERVICE_CONTROL_STOP  1
!define FALSEPASSW            "FaLsEpAsSwOrD"
!define SERVICE_NAME          "PAPYRUS_SERVICE"
!define UNINSTALLER			"uninst-jobserver.exe"

Var hwnd
Var hSCManager
Var hPPWS
Var FuncName

Var InstTaskServ
Var AccountName
Var Password
Var Err

Var InstallType
Var TargetServer
; WMI access
Var user
Var pasw

SetCompressor bzip2
; @v5.7.0 {
Caption "Papyrus JobServer Setup"
BGGradient 007070 00e0f0 FFFFFF
InstallColors FF8080 000030
XPStyle on
BrandingText " "
; } @v5.7.0

;MUI 1.67 compatible ------
!include "MUI2.nsh"
!include "Library.nsh"
!include "FileFunc.nsh"

;MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${SRC_ROOT}\SRC\RSRC\ICO\P2.ICO"
!define MUI_UNICON "${SRC_ROOT}\SRC\RSRC\ICO\P2.ICO"
!define MUI_FINISHPAGE_NOAUTOCLOSE
; } Definitions
; *******************************************************************************
; Functions {
Function .onInit
	InitPluginsDir
	File /oname=$PLUGINSDIR\ppws.ini "ppws.ini"
FunctionEnd
; спросить логин/пароль для доступа к серверу (опционально)
Function AskAccountSettings
	StrCpy $0 "--------------------------------------------------------------------------------"
	StrCpy $2 $0
	System::Call 'wmi_connection::AuthBox(t r0r1, t r2r3, i 80) i.r4'
	IntCmp $4 0 UNDEF_ACCOUNT_SETTINGS
	StrCpy $user $1
	StrCpy $pasw $3
	GoTo DONE
UNDEF_ACCOUNT_SETTINGS:
	StrCpy $user ""
	StrCpy $pasw ""
DONE:
FunctionEnd
; если $INSTDIR содержит remote путь то определить имя сервера и установить $InstallType="remote"
; иначе установить $InstallType="local"
Function DetermineInstallType
	StrCpy $0 $INSTDIR 2
	StrCmp $0 "\\" REMOTE 
	StrCpy $InstallType "local"
	GoTo DONE
REMOTE:
	StrCpy $InstallType "remote"
	; значит надо определить имя сервера
	StrCpy $0 $INSTDIR
	System::Call 'wmi_connection::GetServerName(t r0, t .r1, i 64) i r0'
	StrCpy $TargetServer $1
	call AskAccountSettings
DONE:
FunctionEnd
; диалог выбора установленной версии
Function SelectVersion
	System::Call 'versel::SelectVersion(i 0, t r0r2, i 0) i.r1'
	StrCmp $2 "" SET_DEFAULT_PATH
	StrCpy $INSTDIR "$2\bin"
	GoTo DONE
SET_DEFAULT_PATH:
	StrCpy $INSTDIR "C:\papyrus\ppy\bin"
DONE:
FunctionEnd
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
;
Function CheckPath
	${GetDrives} "HDD" "GetDiskName"
	call DetermineInstallType
	StrCmp $Err "ERRORPATH" +1 DONE
DONE:
FunctionEnd
;
Function CheckPPWS
	StrCmp $InstallType "remote" REMOTE ; переход на удаленный вариант выполнения функции
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
REMOTE:
	StrCpy $0 $TargetServer
	StrCpy $1 $user
	StrCpy $2 $pasw
	System::Call 'wmi_connection::Connect(t r0, t r1, t r2, t NULL) i .r3'
	IntCmp $3 0 CONNECTION_ERROR
	StrCpy $0 ${SERVICE_NAME}
	;System::Call 'wmi_connection::WinSvcDelete(t r0) v'
	System::Call 'wmi_connection::WinSvcExists(t r0, i 1) i .r1'
	IntCmp $1 0 WMI_DISCONNECT
	DetailPrint "Обнаружено, что сервис на удаленной машине уже установлен, попытаемся определить его аккаунт"
	StrCpy $1 "StartName"
	System::Call 'wmi_connection::WinSvcGetProperty(t r0, t r1, i 1, t .r2, i 64) t r0'
	StrCmp $2 "LocalSystem" FORCE_EMPTY_NAME
	WriteINIStr "$PLUGINSDIR\ppws.ini" "Field 3" "state" $2
FORCE_EMPTY_NAME:
	WriteINIStr "$PLUGINSDIR\ppws.ini" "Field 3" "state" ""
	WriteINIStr "$PLUGINSDIR\ppws.ini" "Field 4" "state" ${FALSEPASSW}
	WriteINIStr "$PLUGINSDIR\ppws.ini" "Field 5" "state" ${FALSEPASSW}
	GoTo WMI_DISCONNECT
CONNECTION_ERROR:
	DetailPrint "Не удалось соединиться со службой WMI на $TargetServer"
	Abort
ERROR:
	StrCpy $Err ERROR
	DetailPrint "Ошибка установки сервера задач. Функция - $FuncName"
	Abort
WMI_DISCONNECT:
	System::Call 'wmi_connection::Release(v) v'
DONE:
FunctionEnd
;
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
;
Function SetCustom
	;Display the InstallOptions dialog
	; In this mode InstallOptions returns the window handle so we can use it
	call CheckPPWS
	InstallOptions::initDialog /NOUNLOAD "$PLUGINSDIR\ppws.ini"
	Pop $hwnd
	call ReloadPPWSWindow
	InstallOptions::show
	Pop $0
FunctionEnd
;
Function ValidateCustom
CHECK_PRESS:
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
ERRORNAME:
	MessageBox MB_ICONEXCLAMATION|MB_OK "Некорректно введена учетная запись!"
	Abort ; Return to the page
DONE:
	ReadINIStr $AccountName "$PLUGINSDIR\ppws.ini" "Field 3" "State"
FunctionEnd
;
Function un.onUninstSuccess
	HideWindow
	MessageBox MB_ICONINFORMATION|MB_OK "Удаление программы $(^Name) было успешно завершено."
FunctionEnd
;
Function un.onInit
	MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Вы уверены в том, что желаете удалить $(^Name) и все компоненты программы?" IDYES +2
	Abort
FunctionEnd
; } Functions
; *******************************************************************************
; Pages {
; welcome
!define MUI_WELCOMEPAGE_TITLE "Вас приветствеут мастер установки ${PRODUCT_NAME}"
!define MUI_WELCOMEFINISHPAGE_BITMAP   "${SRC_ROOT}\Src\Rsrc\Bitmap\nsis-welcome-02.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${SRC_ROOT}\Src\Rsrc\Bitmap\nsis-welcome-02.bmp"
!insertmacro MUI_PAGE_WELCOME
; выбор и уточнение пути установки
PageEx directory
	PageCallbacks SelectVersion "" CheckPath
PageExEnd
; параметры установки
Page custom SetCustom ValidateCustom ": Сервер задач"
; } Pages
; *******************************************************************************
; Sections {
;Start menu page
var ICONS_GROUP
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
OutFile PpyJobSrvr_${PRODUCT_VERSION}.exe

InstallDir "C:\PPY\BIN"
;InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Section "Файлы приложения" SEC01
	StrCmp $InstallType "remote" REMOTE
	; Остановлю сервис
	DetailPrint "Остановка сервера задач"
	ExecWait '"$INSTDIR\ppws.exe" servicestop' $0
	IntCmp $0 0 +2
	DetailPrint "! Ошибка остановки сервиса"
	; Скопирую exe файл сервера задач
	SetOutPath "$INSTDIR"
	SetOverwrite on
	File "${SRC_TARGET}\ppws.exe"
	;DetailPrint "Де-регистрация сервера задач"
	;ExecWait '"$INSTDIR\ppws.exe" uninstall'
	; Подготовка регистров
	StrCpy $2 ppws
	ReadINIStr $R2 "$PLUGINSDIR\ppws.ini" "Field 3" "State"
	ReadINIStr $R3 "$PLUGINSDIR\ppws.ini" "Field 4" "State"
	; Регистрация сервиса
	DetailPrint "Регистрация cервера задач"
	ExecWait '"$INSTDIR\ppws.exe" install $R2 $R3' $0
	IntCmp $0 0 +2
	DetailPrint "! Ошибка регистрации сервиса"
	; Запуск сервиса
	DetailPrint "Запуск сервера задач"
	ExecWait '"$INSTDIR\ppws.exe" servicestart' $0
	IntCmp $0 0 +2
	DetailPrint "! Ошибка запуска сервиса"
	GoTo DONE
REMOTE:
	StrCpy $0 $TargetServer
	StrCpy $1 $user
	StrCpy $2 $pasw
	DetailPrint "Соединение с $1@$0 ..."
	System::Call "wmi_connection::Connect(t r0, t r1, t r2, t NULL) i .r3"
	IntCmp $3 0 CONNECTION_ERROR
	System::Call "wmi_connection::GetLastMsg(t .r2, i 64) v"
	DetailPrint "$2"
	;
	StrCpy $0 $INSTDIR
	System::Call "wmi_connection::GetLocalFromUnc(t r0, t .r1, 256) i r0"
	StrCpy $0 "$1\ppws.exe"
	StrCpy $1 "$0 servicestop"
	DetailPrint "Попытка остановки сервиса на удаленной машине..."
	System::Call "wmi_connection::WinProcessCreate(t r1, t '') i r2"
	;System::Call "wmi_connection::WinSvcStop(t r0) v" ; остановить указанный сервис
	System::Call "wmi_connection::GetLastMsg(t .r1, i 128) v"
	DetailPrint "$1"
	; Скопирую exe файл сервера задач
	SetOutPath "$INSTDIR"
	SetOverwrite on
	File "${SRC_TARGET}\ppws.exe"
	; Регистрация сервиса
	ReadINIStr $R2 "$PLUGINSDIR\ppws.ini" "Field 3" "State"
	ReadINIStr $R3 "$PLUGINSDIR\ppws.ini" "Field 4" "State"
	StrCpy $1 "$0 install"
	StrCmp $R2 "" REG_SVC
	StrCpy $2 "$1 $R2"
	StrCpy $1 $2
	StrCmp $R3 "" REG_SVC
	StrCpy $3 "$1 $R3"
	StrCpy $1 $3
REG_SVC:
	DetailPrint "Удаленная регистрация cервиса..."
	System::Call "wmi_connection::WinProcessCreate(t r1, t '') i .r2"
	;System::Call "wmi_connection::WinSvcCreate(t r0, t 'Papyrus Job Server', t r2, i 16, i 1, i 1, t r12, t r13, t NULL, t NULL, t NULL) v"	
	System::Call "wmi_connection::GetLastMsg(t .r1, 128) v"
	DetailPrint "$1"
	DetailPrint "Удаленный запуск сервиса..."
	StrCpy $1 "$0 service"
	System::Call "wmi_connection::WinProcessCreate(t r1, t '') v"
	System::Call "wmi_connection::GetLastMsg(t .r1, 128) v"
	DetailPrint "$1"
	GoTo DONE
WMI_DISCONNECT:
	System::Call "wmi_connection::Release(v) v"
	GoTo DONE
CONNECTION_ERROR:
	DetailPrint "Не удалось соединиться со службой WMI на $TargetServer"
	Abort
DONE:
SectionEnd
;
Section -Post
	StrCmp $Err ERROR DONE
	;
	; записываем анинсталлер в директорию
	;
	WriteUninstaller "$INSTDIR\${UNINSTALLER}"
	StrCmp $InstallType "remote" REMOTE
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\${UNINSTALLER}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\ppws.exe"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
	GoTo DONE
REMOTE:
	StrCpy $0 $TargetServer
	StrCpy $1 $user
	StrCpy $2 $pasw
	DetailPrint "Соединение с $1@$0 ..."
	System::Call "wmi_connection::Connect(t r0, t r1, t r2, t NULL) i .r3"
	IntCmp $3 0 CONNECTION_ERROR
	System::Call "wmi_connection::GetLastMsg(t .r1, 128) i .r3"
	DetailPrint "$1"
	;
	DetailPrint "Запись значений в реестр на удаленой машине..."
	;
	StrCpy $0 "${PRODUCT_UNINST_KEY}"
	StrCpy $1 "UninstallString"
	StrCpy $2 "$INSTDIR\${UNINSTALLER}"
	System::Call "wmi_connection::GetLocalFromUnc(t r2, t .r3, i 256) i .r3"
	System::Call "wmi_connection::WinRegAddStr(i ${PRODUCT_UNINST_ROOT_KEY_ID}, t r0, t r1, t r3) v"
	;
	StrCpy $1 "$INSTDIR\ppws.exe"
	System::Call "wmi_connection::GetLocalFromUnc(t r1, t .r2, 256)"
	;
	StrCpy $1 "DisplayIcon"
	System::Call "wmi_connection::WinRegAddStr(i ${PRODUCT_UNINST_ROOT_KEY_ID}, t r0, t r1, t r2) v"
	;
	StrCpy $1 "DisplayVersion"
	StrCpy $2 "${PRODUCT_VERSION}"
	System::Call "wmi_connection::WinRegAddStr(i ${PRODUCT_UNINST_ROOT_KEY_ID}, t r0, t r1, t r2) v"
	;
	StrCpy $1 "URLInfoAbout"
	StrCpy $2 "${PRODUCT_WEB_SITE}"
	System::Call "wmi_connection::WinRegAddStr(i ${PRODUCT_UNINST_ROOT_KEY_ID}, t r0, t r1, t r2) v"
	;
	StrCpy $1 "Publisher"
	StrCpy $2 "${PRODUCT_PUBLISHER}"
	System::Call "wmi_connection::WinRegAddStr(i ${PRODUCT_UNINST_ROOT_KEY_ID}, t r0, t r1, t r2) v"
	;
	System::Call "wmi_connection::GetLastMsg(t .r1, 128)"
	DetailPrint "$1"
WMI_DISCONNECT:
	System::Call "wmi_connection::Release(v) v"
	GoTo DONE
CONNECTION_ERROR:
	DetailPrint "Не удалось соединиться со службой WMI на $TargetServer"
	Abort
DONE:
SectionEnd
;
Section Uninstall
	Delete "$INSTDIR\${UNINSTALLER}"
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
ERROR:
	DetailPrint "ОШИБКА удаления сервера задач"
DONE:
SectionEnd
; } Sections
