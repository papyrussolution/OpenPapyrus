; Script generated with the Venis Install Wizard

; Define your application name
!define APPNAME "Papyrus XLA for Excel"
!define APPNAMEANDVERSION "Надстройка Papyrus для Excel"
!define FileName "PpyComTest.xla"
!define DIR_DD    "$INSTDIR\BIN"
!define VERSELDLL  "versel.dll"

; Main Install settings
Name "${APPNAMEANDVERSION}"
InstallDir "H:\ppy\install"
InstallDirRegKey HKCR "Software\Microsoft\Office\" ""
OutFile "..\..\DISTRIB\PpyXLA\ppyxla.exe"

; Use compression
SetCompressor LZMA

; Modern interface settings
!define MUI_VERBOSE 4
!include "MUI2.nsh"
!include "Library.nsh"

!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
;
; Directory page
;
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
	!undef MUI_DIRECTORYPAGE_TEXT_TOP
	!undef MUI_DIRECTORYPAGE_TEXT_DESTINATION
	!insertmacro MUI_UNSET MUI_DIRECTORYPAGE_VARIABLE
	!insertmacro MUI_UNSET MUI_DIRECTORYPAGE_VERIFYONLEAVE
	!verbose pop	

!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Set languages (first is default language)
!insertmacro MUI_LANGUAGE "Russian"

Section "Файлы приложения" Section1

	; Set Section properties
	SetOverwrite on

	; Set Section Files and Shortcuts
	SetOutPath "$INSTDIR\"
	File "..\VBA\DL200.xla"

SectionEnd

Section -FinishSection
	Var /GLOBAL office_ver
	Var /GLOBAL file_path
	Var /GLOBAL fname_len

	StrLen $fname_len ${FileName}
	StrCpy $file_path "$INSTDIR\${FileName}"
	StrCpy $office_ver 10
main_loop:
	Var /GLOBAL idx
	Var /GLOBAL reg_name
	Var /GLOBAL reg_val
	Var /GLOBAL sub_key
	Var /GLOBAL open_max
	Var /GLOBAL file_name
	Var /GLOBAL reg_name2
		
	StrCpy $open_max 0
	StrCpy $idx 0
	StrCpy $sub_key "Software\Microsoft\Office\$office_ver.0\Excel\Options"
loop:
	EnumRegValue $reg_name HKCU $sub_key $idx
	StrCmp $reg_name "" done
	IfErrors done
 	IntOp $idx $idx + 1
	StrCpy $reg_name2 $reg_name 4
	StrCmp $reg_name2 "OPEN" read_val loop
read_val:
	Var /GLOBAL len1
	StrLen $len1 $reg_name
	
	IntCmp $len1 4 0 0 +3
		StrCpy $reg_name2 0
		Goto +3
		IntOp  $len1 $len1 - 4
		StrCpy $reg_name2 $reg_name "" -$len1

	IntCmp $open_max $reg_name2 0 0 +4
		StrCpy $open_max $reg_name2
		Goto +2
		IntOp  $open_max $open_max + 1

	ReadRegStr $reg_val HKCU $sub_key $reg_name
	StrCpy $file_name $reg_val "" -$fname_len
	StrCmp $file_name ${FileName} 0 loop
	StrCpy $open_max $reg_name2
	StrCmp $reg_val $file_path +3 0
	Delete $reg_val
done:
	IntCmp $open_max 0 +3
	StrCpy $reg_val "OPEN$open_max"
	Goto +2
	StrCpy $reg_val "OPEN"

	WriteRegStr HKCU $sub_key $reg_val $file_path
	IntOp $office_ver $office_ver + 1
	IntCmp $office_ver, 15  main_done main_loop
main_done:
	Call DelEntryFromAddInnManager
SectionEnd

; Modern install component descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${Section1} ""
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;Uninstall section
Section Uninstall
SectionEnd

; On initialization
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
	StrCpy $INSTDIR "$2\DD"
	Pop $0
	Pop $1
	Pop $2
FunctionEnd

Function DelEntryFromAddInnManager
	ClearErrors

	StrLen $fname_len ${FileName}
	StrCpy $office_ver 10
main_loop:
	StrCpy $idx 0
	StrCpy $sub_key "Software\Microsoft\Office\$office_ver.0\Excel\Add-in Manager"
loop:
	EnumRegValue $reg_name HKCU $sub_key $idx
	StrCmp $reg_name "" done
	IntOp $idx $idx + 1
	StrCpy $file_name $reg_name "" -$fname_len

	StrCmp $file_name ${FileName} 0 loop
	DeleteRegValue HKCU $sub_key $reg_name
	IntOp $idx $idx - 1
	Goto loop
done:
	IntOp $office_ver $office_ver + 1
	WriteRegStr HKCU $sub_key $file_path ""
	IntCmp $office_ver, 15  main_done main_loop
main_done:
	Return
FunctionEnd

; eof