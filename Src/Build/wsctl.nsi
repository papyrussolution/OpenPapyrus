; WSCTL.NSI @construction
; Copyright (c) A.Sobolev 2024
; Скрипт для создания дистрибутива компонента WSCTL
;
!define PRODUCT_PUBLISHER "A.Fokin, A.Sobolev"
!define PRODUCT_WEB_SITE "http://www.petroglif.ru"
!define SRC_REDIST   "${SRC_ROOT}\src\redist"
!define SRC_TARGET   "${SRC_ROOT}\PPY\BIN"
!define PRODUCT_NAME "WSCTL"

!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

SetCompressor lzma
Caption "${PRODUCT_NAME} Setup"
XPStyle on
BrandingText " "

!define MUI_VERBOSE 4
!include "MUI2.nsh"
!include "Library.nsh"

!define MUI_ABORTWARNING
!define MUI_ICON   "${SRC_ROOT}\SRC\RSRC\ICO\P2.ICO"
!define MUI_UNICON "${SRC_ROOT}\SRC\RSRC\ICO\P2.ICO"

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

Section Uninstall
SectionEnd

