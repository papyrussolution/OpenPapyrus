// NPP-RESOURCE.H
// This file is part of Notepad++ project
// Copyright (C) 2021 Don HO <don.h@free.fr>
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// Heavily modified by Anton Sobolev
// 
#ifndef __NPP_RESOURCE_H
#define __NPP_RESOURCE_H
//#include "resource.h"
//
// Notepad++ version: begin
//
#define NOTEPAD_PLUS_VERSION TEXT("Notepad++ v8.1.4")

// should be X.Y : ie. if VERSION_DIGITALVALUE == 4, 7, 1, 0 , then X = 4, Y = 71
// ex : #define VERSION_VALUE TEXT("5.63\0")
#define VERSION_VALUE TEXT("8.14\0")
#define VERSION_DIGITALVALUE 8, 1, 4, 0

// Notepad++ version: end
#ifndef IDC_STATIC
	#define IDC_STATIC    -1
#endif
#define IDI_NPPABOUT_LOGO        99
#define IDI_M30ICON             100
#define IDI_CHAMELEON           101
#define IDI_CHAMELEON_DM        102
//#define IDI_JESUISCHARLIE     102
//#define IDI_GILETJAUNE        102
//#define IDI_SAMESEXMARRIAGE   102
#define IDR_RT_MANIFEST         103
#define IDI_ICONABSENT          104
//
// TOOLBAR ICO - set 1
//
#define IDI_NEW_ICON                      201
#define IDI_OPEN_ICON                     202
#define IDI_CLOSE_ICON                    203
#define IDI_CLOSEALL_ICON                 204
#define IDI_SAVE_ICON                     205
#define IDI_SAVEALL_ICON                  206
#define IDI_CUT_ICON                      207
#define IDI_COPY_ICON                     208
#define IDI_PASTE_ICON                    209
#define IDI_UNDO_ICON                     210
#define IDI_REDO_ICON                     211
#define IDI_FIND_ICON                     212
#define IDI_REPLACE_ICON                  213
#define IDI_ZOOMIN_ICON                   214
#define IDI_ZOOMOUT_ICON                  215
#define IDI_VIEW_UD_DLG_ICON              216
#define IDI_PRINT_ICON                    217
#define IDI_STARTRECORD_ICON              218
#define IDI_STARTRECORD_DISABLE_ICON      219
#define IDI_STOPRECORD_ICON               220
#define IDI_STOPRECORD_DISABLE_ICON       221
#define IDI_PLAYRECORD_ICON               222
#define IDI_PLAYRECORD_DISABLE_ICON       223
#define IDI_SAVERECORD_ICON               224
#define IDI_SAVERECORD_DISABLE_ICON       225
#define IDI_MMPLAY_DIS_ICON               226
#define IDI_MMPLAY_ICON                   227
#define IDI_VIEW_ALL_CHAR_ICON            228
#define IDI_VIEW_INDENT_ICON              229
#define IDI_VIEW_WRAP_ICON                230
#define IDI_SAVE_DISABLE_ICON             231
#define IDI_SAVEALL_DISABLE_ICON          232
#define IDI_CUT_DISABLE_ICON              233
#define IDI_COPY_DISABLE_ICON             234
#define IDI_PASTE_DISABLE_ICON            235
#define IDI_UNDO_DISABLE_ICON             236
#define IDI_REDO_DISABLE_ICON             237
#define IDI_SYNCV_ICON                    238
#define IDI_SYNCV_DISABLE_ICON            239
#define IDI_SYNCH_ICON                    240
#define IDI_SYNCH_DISABLE_ICON            241
#define IDI_VIEW_DOC_MAP_ICON             242
#define IDI_VIEW_FILEBROWSER_ICON         243
#define IDI_VIEW_FUNCLIST_ICON            244
#define IDI_VIEW_MONITORING_ICON          245
#define IDI_VIEW_DOCLIST_ICON             392 //continuing from IDI_VIEW_DOCLIST_ICON_DM2's ID
//
// TOOLBAR ICO - set 1, Dark Mode
//
#define IDI_NEW_ICON_DM                   246
#define IDI_OPEN_ICON_DM                  247
#define IDI_CLOSE_ICON_DM                 248
#define IDI_CLOSEALL_ICON_DM              249
#define IDI_SAVE_ICON_DM                  250
#define IDI_SAVEALL_ICON_DM               251
#define IDI_CUT_ICON_DM                   252
#define IDI_COPY_ICON_DM                  253
#define IDI_PASTE_ICON_DM                 254
#define IDI_UNDO_ICON_DM                  255
#define IDI_REDO_ICON_DM                  256
#define IDI_FIND_ICON_DM                  257
#define IDI_REPLACE_ICON_DM               258
#define IDI_ZOOMIN_ICON_DM                259
#define IDI_ZOOMOUT_ICON_DM               260
#define IDI_VIEW_UD_DLG_ICON_DM           261
#define IDI_PRINT_ICON_DM                 262
#define IDI_STARTRECORD_ICON_DM           263
#define IDI_STARTRECORD_DISABLE_ICON_DM   264
#define IDI_STOPRECORD_ICON_DM            265
#define IDI_STOPRECORD_DISABLE_ICON_DM    266
#define IDI_PLAYRECORD_ICON_DM            267
#define IDI_PLAYRECORD_DISABLE_ICON_DM    268
#define IDI_SAVERECORD_ICON_DM            269
#define IDI_SAVERECORD_DISABLE_ICON_DM    270
#define IDI_MMPLAY_DIS_ICON_DM            271
#define IDI_MMPLAY_ICON_DM                272
#define IDI_VIEW_ALL_CHAR_ICON_DM         273
#define IDI_VIEW_INDENT_ICON_DM           274
#define IDI_VIEW_WRAP_ICON_DM             275
#define IDI_SAVE_DISABLE_ICON_DM          276
#define IDI_SAVEALL_DISABLE_ICON_DM       277
#define IDI_CUT_DISABLE_ICON_DM           278
#define IDI_COPY_DISABLE_ICON_DM          279
#define IDI_PASTE_DISABLE_ICON_DM         280
#define IDI_UNDO_DISABLE_ICON_DM          281
#define IDI_REDO_DISABLE_ICON_DM          282
#define IDI_SYNCV_ICON_DM                 283
#define IDI_SYNCV_DISABLE_ICON_DM         284
#define IDI_SYNCH_ICON_DM                 285
#define IDI_SYNCH_DISABLE_ICON_DM         286
#define IDI_VIEW_DOC_MAP_ICON_DM          287
#define IDI_VIEW_FILEBROWSER_ICON_DM      288
#define IDI_VIEW_FUNCLIST_ICON_DM         289
#define IDI_VIEW_MONITORING_ICON_DM       290
#define IDI_VIEW_DOCLIST_ICON_DM          393 //continuing from IDI_VIEW_DOCLIST_ICON's ID
//
// TOOLBAR ICO - set 2
//
#define IDI_NEW_ICON2                     301
#define IDI_OPEN_ICON2                    302
#define IDI_CLOSE_ICON2                   303
#define IDI_CLOSEALL_ICON2                304
#define IDI_SAVE_ICON2                    305
#define IDI_SAVEALL_ICON2                 306
#define IDI_CUT_ICON2                     307
#define IDI_COPY_ICON2                    308
#define IDI_PASTE_ICON2                   309
#define IDI_UNDO_ICON2                    310
#define IDI_REDO_ICON2                    311
#define IDI_FIND_ICON2                    312
#define IDI_REPLACE_ICON2                 313
#define IDI_ZOOMIN_ICON2                  314
#define IDI_ZOOMOUT_ICON2                 315
#define IDI_VIEW_UD_DLG_ICON2             316
#define IDI_PRINT_ICON2                   317
#define IDI_STARTRECORD_ICON2             318
#define IDI_STARTRECORD_DISABLE_ICON2     319
#define IDI_STOPRECORD_ICON2              320
#define IDI_STOPRECORD_DISABLE_ICON2      321
#define IDI_PLAYRECORD_ICON2              322
#define IDI_PLAYRECORD_DISABLE_ICON2      323
#define IDI_SAVERECORD_ICON2              324
#define IDI_SAVERECORD_DISABLE_ICON2      325
#define IDI_MMPLAY_DIS_ICON2              326
#define IDI_MMPLAY_ICON2                  327
#define IDI_VIEW_ALL_CHAR_ICON2           328
#define IDI_VIEW_INDENT_ICON2             329
#define IDI_VIEW_WRAP_ICON2               330
#define IDI_SAVE_DISABLE_ICON2            331
#define IDI_SAVEALL_DISABLE_ICON2         332
#define IDI_CUT_DISABLE_ICON2             333
#define IDI_COPY_DISABLE_ICON2            334
#define IDI_PASTE_DISABLE_ICON2           335
#define IDI_UNDO_DISABLE_ICON2            336
#define IDI_REDO_DISABLE_ICON2            337
#define IDI_SYNCV_ICON2                   338
#define IDI_SYNCV_DISABLE_ICON2           339
#define IDI_SYNCH_ICON2                   340
#define IDI_SYNCH_DISABLE_ICON2           341
#define IDI_VIEW_DOC_MAP_ICON2            342
#define IDI_VIEW_FILEBROWSER_ICON2        343
#define IDI_VIEW_FUNCLIST_ICON2           344
#define IDI_VIEW_MONITORING_ICON2         345
#define IDI_VIEW_DOCLIST_ICON2            394 //continuing from IDI_VIEW_DOCLIST_ICON_DM's ID
//
// TOOLBAR ICO - set 2, Dark Mode
//
#define IDI_NEW_ICON_DM2                  346
#define IDI_OPEN_ICON_DM2                 347
#define IDI_CLOSE_ICON_DM2                348
#define IDI_CLOSEALL_ICON_DM2             349
#define IDI_SAVE_ICON_DM2                 350
#define IDI_SAVEALL_ICON_DM2              351
#define IDI_CUT_ICON_DM2                  352
#define IDI_COPY_ICON_DM2                 353
#define IDI_PASTE_ICON_DM2                354
#define IDI_UNDO_ICON_DM2                 355
#define IDI_REDO_ICON_DM2                 356
#define IDI_FIND_ICON_DM2                 357
#define IDI_REPLACE_ICON_DM2              358
#define IDI_ZOOMIN_ICON_DM2               359
#define IDI_ZOOMOUT_ICON_DM2              360
#define IDI_VIEW_UD_DLG_ICON_DM2          361
#define IDI_PRINT_ICON_DM2                362
#define IDI_STARTRECORD_ICON_DM2          363
#define IDI_STARTRECORD_DISABLE_ICON_DM2  364
#define IDI_STOPRECORD_ICON_DM2           365
#define IDI_STOPRECORD_DISABLE_ICON_DM2   366
#define IDI_PLAYRECORD_ICON_DM2           367
#define IDI_PLAYRECORD_DISABLE_ICON_DM2   368
#define IDI_SAVERECORD_ICON_DM2           369
#define IDI_SAVERECORD_DISABLE_ICON_DM2   370
#define IDI_MMPLAY_DIS_ICON_DM2           371
#define IDI_MMPLAY_ICON_DM2               372
#define IDI_VIEW_ALL_CHAR_ICON_DM2        373
#define IDI_VIEW_INDENT_ICON_DM2          374
#define IDI_VIEW_WRAP_ICON_DM2            375
#define IDI_SAVE_DISABLE_ICON_DM2         376
#define IDI_SAVEALL_DISABLE_ICON_DM2      377
#define IDI_CUT_DISABLE_ICON_DM2          378
#define IDI_COPY_DISABLE_ICON_DM2         379
#define IDI_PASTE_DISABLE_ICON_DM2        380
#define IDI_UNDO_DISABLE_ICON_DM2         381
#define IDI_REDO_DISABLE_ICON_DM2         382
#define IDI_SYNCV_ICON_DM2                383
#define IDI_SYNCV_DISABLE_ICON_DM2        384
#define IDI_SYNCH_ICON_DM2                385
#define IDI_SYNCH_DISABLE_ICON_DM2        386
#define IDI_VIEW_DOC_MAP_ICON_DM2         387
#define IDI_VIEW_FILEBROWSER_ICON_DM2     388
#define IDI_VIEW_FUNCLIST_ICON_DM2        389
#define IDI_VIEW_MONITORING_ICON_DM2      390
#define IDI_VIEW_DOCLIST_ICON_DM2         391

#define IDI_SAVED_ICON           501
#define IDI_UNSAVED_ICON         502
#define IDI_READONLY_ICON        503
#define IDI_FIND_RESULT_ICON     504
#define IDI_MONITORING_ICON      505
#define IDI_SAVED_ALT_ICON       506
#define IDI_UNSAVED_ALT_ICON     507
#define IDI_READONLY_ALT_ICON    508
#define IDI_SAVED_DM_ICON        509
#define IDI_UNSAVED_DM_ICON      510
#define IDI_MONITORING_DM_ICON   511
#define IDI_READONLY_DM_ICON     512
#define IDI_DELETE_ICON          525

#define IDI_PROJECT_WORKSPACE          601
#define IDI_PROJECT_WORKSPACEDIRTY     602
#define IDI_PROJECT_PROJECT            603
#define IDI_PROJECT_FOLDEROPEN         604
#define IDI_PROJECT_FOLDERCLOSE        605
#define IDI_PROJECT_FILE               606
#define IDI_PROJECT_FILEINVALID        607
#define IDI_FB_ROOTOPEN                608
#define IDI_FB_ROOTCLOSE               609
#define IDI_FB_SELECTCURRENTFILE       610
#define IDI_FB_FOLDALL                 611
#define IDI_FB_EXPANDALL               612
#define IDI_FB_SELECTCURRENTFILE_DM    613
#define IDI_FB_FOLDALL_DM              614
#define IDI_FB_EXPANDALL_DM            615

#define IDI_FUNCLIST_ROOT              620
#define IDI_FUNCLIST_NODE              621
#define IDI_FUNCLIST_LEAF              622

#define IDI_FUNCLIST_SORTBUTTON        631
#define IDI_FUNCLIST_RELOADBUTTON      632
#define IDI_FUNCLIST_SORTBUTTON_DM     633
#define IDI_FUNCLIST_RELOADBUTTON_DM   634

#define IDI_GET_INFO_FROM_TOOLTIP               641
#define IDC_MY_CUR     1402
#define IDC_UP_ARROW  1403
#define IDC_DRAG_TAB    1404
#define IDC_DRAG_INTERDIT_TAB 1405
#define IDC_DRAG_PLUS_TAB 1406
#define IDC_DRAG_OUT_TAB 1407
#define IDC_MACRO_RECORDING 1408
#define IDR_SAVEALL            1500
#define IDR_CLOSEFILE          1501
#define IDR_CLOSEALL           1502
#define IDR_FIND               1503
#define IDR_REPLACE            1504
#define IDR_ZOOMIN             1505
#define IDR_ZOOMOUT            1506
#define IDR_WRAP               1507
#define IDR_INVISIBLECHAR      1508
#define IDR_INDENTGUIDE        1509
#define IDR_SHOWPANNEL         1510
#define IDR_STARTRECORD        1511
#define IDR_STOPRECORD         1512
#define IDR_PLAYRECORD         1513
#define IDR_SAVERECORD         1514
#define IDR_SYNCV              1515
#define IDR_SYNCH              1516
#define IDR_FILENEW            1517
#define IDR_FILEOPEN           1518
#define IDR_FILESAVE           1519
#define IDR_PRINT              1520
#define IDR_CUT                1521
#define IDR_COPY               1522
#define IDR_PASTE              1523
#define IDR_UNDO               1524
#define IDR_REDO               1525
#define IDR_M_PLAYRECORD       1526
#define IDR_DOCMAP             1527
#define IDR_FUNC_LIST          1528
#define IDR_FILEBROWSER        1529
#define IDR_CLOSETAB           1530
#define IDR_CLOSETAB_INACT     1531
#define IDR_CLOSETAB_HOVER     1532
#define IDR_CLOSETAB_PUSH      1533
#define IDR_FUNC_LIST_ICO      1534
#define IDR_DOCMAP_ICO         1535
#define IDR_PROJECTPANEL_ICO   1536
#define IDR_CLIPBOARDPANEL_ICO 1537
#define IDR_ASCIIPANEL_ICO     1538
#define IDR_DOCSWITCHER_ICO    1539
#define IDR_FILEBROWSER_ICO    1540
#define IDR_FILEMONITORING     1541
#define IDR_CLOSETAB_DM        1542
#define IDR_CLOSETAB_INACT_DM  1543
#define IDR_CLOSETAB_HOVER_DM  1544
#define IDR_CLOSETAB_PUSH_DM   1545
#define IDR_DOCLIST            1546
#define IDR_DOCLIST_ICO        1547
#define IDR_FILEBROWSER_ICO2      1550
#define IDR_FILEBROWSER_ICO_DM    1551
#define IDR_FUNC_LIST_ICO2        1552
#define IDR_FUNC_LIST_ICO_DM      1553
#define IDR_DOCMAP_ICO2           1554
#define IDR_DOCMAP_ICO_DM         1555
#define IDR_DOCLIST_ICO2          1556
#define IDR_DOCLIST_ICO_DM        1557
#define IDR_PROJECTPANEL_ICO2     1558
#define IDR_PROJECTPANEL_ICO_DM   1559
#define IDR_CLIPBOARDPANEL_ICO2   1560
#define IDR_CLIPBOARDPANEL_ICO_DM 1561
#define IDR_ASCIIPANEL_ICO2       1562
#define IDR_ASCIIPANEL_ICO_DM     1563
#define ID_MACRO                           20000
//                                     O     .
//                                     C     .
//                                     C     .
//                                     U     .
//                                     P     .
//                                     I     .
//                                     E     .
//                                     D     .
#define ID_MACRO_LIMIT                     20499
#define ID_USER_CMD                        21000
//                                     O     .
//                                     C     .
//                                     C     .
//                                     U     .
//                                     P     .
//                                     I     .
//                                     E     .
//                                     D     .
#define ID_USER_CMD_LIMIT                  21499
#define ID_PLUGINS_CMD                     22000
//                                     O     .
//                                     C     .
//                                     C     .
//                                     U     .
//                                     P     .
//                                     I     .
//                                     E     .
//                                     D     .
#define ID_PLUGINS_CMD_LIMIT               22999
#define ID_PLUGINS_CMD_DYNAMIC             23000
//                                     O     .
//                                     C     .
//                                     C     .
//                                     U     .
//                                     P     .
//                                     I     .
//                                     E     .
//                                     D     .
#define ID_PLUGINS_CMD_DYNAMIC_LIMIT       24999
#define MARKER_PLUGINS          3
#define MARKER_PLUGINS_LIMIT   19
//#define IDM 40000
#define IDCMD 50000
//#define IDM_EDIT_AUTOCOMPLETE                (IDCMD+0)
//#define IDM_EDIT_AUTOCOMPLETE_CURRENTFILE    (IDCMD+1)
	#define IDC_PREV_DOC                    (IDCMD+3)
	#define IDC_NEXT_DOC                    (IDCMD+4)
	#define IDC_EDIT_TOGGLEMACRORECORDING    (IDCMD+5)
//#define IDC_KEY_HOME                    (IDCMD+6)
//#define IDC_KEY_END                        (IDCMD+7)
//#define IDC_KEY_SELECT_2_HOME            (IDCMD+8)
//#define IDC_KEY_SELECT_2_END            (IDCMD+9)
#define IDCMD_LIMIT                        (IDCMD+20)
#define IDSCINTILLA 60000
	#define IDSCINTILLA_KEY_HOME        (IDSCINTILLA+0)
	#define IDSCINTILLA_KEY_HOME_WRAP   (IDSCINTILLA+1)
	#define IDSCINTILLA_KEY_END         (IDSCINTILLA+2)
	#define IDSCINTILLA_KEY_END_WRAP    (IDSCINTILLA+3)
	#define IDSCINTILLA_KEY_LINE_DUP    (IDSCINTILLA+4)
	#define IDSCINTILLA_KEY_LINE_CUT    (IDSCINTILLA+5)
	#define IDSCINTILLA_KEY_LINE_DEL    (IDSCINTILLA+6)
	#define IDSCINTILLA_KEY_LINE_TRANS  (IDSCINTILLA+7)
	#define IDSCINTILLA_KEY_LINE_COPY   (IDSCINTILLA+8)
	#define IDSCINTILLA_KEY_CUT         (IDSCINTILLA+9)
	#define IDSCINTILLA_KEY_COPY        (IDSCINTILLA+10)
	#define IDSCINTILLA_KEY_PASTE       (IDSCINTILLA+11)
	#define IDSCINTILLA_KEY_DEL         (IDSCINTILLA+12)
	#define IDSCINTILLA_KEY_SELECTALL   (IDSCINTILLA+13)
	#define IDSCINTILLA_KEY_OUTDENT     (IDSCINTILLA+14)
	#define IDSCINTILLA_KEY_UNDO        (IDSCINTILLA+15)
	#define IDSCINTILLA_KEY_REDO        (IDSCINTILLA+16)
#define IDSCINTILLA_LIMIT        (IDSCINTILLA+30)
#define IDD_FILEVIEW_DIALOG                1000
#define IDD_CREATE_DIRECTORY            1100
#define IDC_STATIC_CURRENT_FOLDER       1101
#define IDC_EDIT_NEW_FOLDER             1102
#define IDD_INSERT_INPUT_TEXT            1200
#define IDC_EDIT_INPUT_VALUE            1201
#define IDC_STATIC_INPUT_TITLE            1202
#define IDC_ICON_INPUT_ICON                1203
#define IDR_M30_MENU                    1500
#define IDR_SYSTRAYPOPUP_MENU            1501
// #define IDD_FIND_REPLACE_DLG        1600
#define IDD_ABOUTBOX 1700
#define IDC_LICENCE_EDIT 1701
#define IDC_HOME_ADDR        1702
#define IDC_EMAIL_ADDR        1703
#define IDC_ONLINEHELP_ADDR 1704
#define IDC_AUTHOR_NAME 1705
#define IDC_BUILD_DATETIME 1706
#define IDC_VERSION_BIT 1707
#define IDD_DEBUGINFOBOX 1750
#define IDC_DEBUGINFO_EDIT 1751
#define IDC_DEBUGINFO_COPYLINK 1752
#define IDD_DOSAVEORNOTBOX  1760
#define IDC_DOSAVEORNOTTEXT 1761
#define IDD_DOSAVEALLBOX    1765
#define IDC_DOSAVEALLTEXT   1766
//#define IDD_USER_DEFINE_BOX       1800
//#define IDD_RUN_DLG               1900
//#define IDD_MD5FROMFILES_DLG      1920
//#define IDD_MD5FROMTEXT_DLG       1930
#define IDD_GOLINE        2000
#define ID_GOLINE_EDIT    (IDD_GOLINE + 1)
#define ID_CURRLINE        (IDD_GOLINE + 2)
#define ID_LASTLINE        (IDD_GOLINE + 3)
#define ID_URHERE_STATIC           (IDD_GOLINE + 4)
#define ID_UGO_STATIC                 (IDD_GOLINE + 5)
#define ID_NOMORETHAN_STATIC   (IDD_GOLINE + 6)
#define IDC_RADIO_GOTOLINE   (IDD_GOLINE + 7)
#define IDC_RADIO_GOTOOFFSET   (IDD_GOLINE + 8)
//#define IDD_COLUMNEDIT   2020 // voir columnEditor_rc.h
//#define IDD_COLOUR_POPUP   2100
//#define IDD_STYLER_DLG    2200 // See WordStyleDlgRes.h
//#define IDD_GLOBAL_STYLER_DLG    2300 // See WordStyleDlgRes.h
#define IDD_VALUE_DLG       2400
#define IDC_VALUE_STATIC  2401
#define IDC_VALUE_EDIT      2402
#define IDD_BUTTON_DLG       2410
#define IDC_RESTORE_BUTTON  2411
//#define IDD_TASKLIST_DLG    2450 // see TaskListDlg_rc.h
#define IDD_SETTING_DLG    2500
//#define IDD_SHORTCUTMAPPER_DLG      2600 //See ShortcutMapper_rc.h
//#define IDD_ANSIASCII_PANEL      2700 //See ansiCharPanel_rc.h
//#define IDD_CLIPBOARDHISTORY_PANEL      2800 //See clipboardHistoryPanel_rc.h
//#define IDD_FINDCHARACTERS      2900 //See findCharsInRange_rc.h
//#define IDD_DOCLIST      3000 //See VerticalFileSwitcher_rc.h
//#define IDD_PROJECTPANEL      3100 //See ProjectPanel_rc.h
//#define IDD_FILERELOCALIZER_DIALOG  3200 //See ProjectPanel_rc.h
//#define IDD_DOCUMENTMAP      3300 //See documentMap_rc.h
//#define IDD_FUNCLIST_PANEL   3400 //See functionListPanel_rc.h
//#define IDD_FILEBROWSER 3500 //See fileBrowser_rc.h
//#define IDD_DOCUMENSNAPSHOT 3600 //See documentSnapshot_rc.h
//#define IDD_REGEXT 4000 // See regExtDlg.h
//#define IDD_SHORTCUT_DLG      5000 // See shortcutRc.h
//#define IDD_PLUGINSADMIN_DLG 5500 // See pluginsAdminRes.h
//#define IDD_PREFERENCE_BOX 6000 // See preference.rc

#define NOTEPADPLUS_USER_INTERNAL     (WM_USER + 0000)
	#define NPPM_INTERNAL_USERCMDLIST_MODIFIED          (NOTEPADPLUS_USER_INTERNAL + 1)
	#define NPPM_INTERNAL_CMDLIST_MODIFIED              (NOTEPADPLUS_USER_INTERNAL + 2)
	#define NPPM_INTERNAL_MACROLIST_MODIFIED            (NOTEPADPLUS_USER_INTERNAL + 3)
	#define NPPM_INTERNAL_PLUGINCMDLIST_MODIFIED        (NOTEPADPLUS_USER_INTERNAL + 4)
	#define NPPM_INTERNAL_CLEARSCINTILLAKEY             (NOTEPADPLUS_USER_INTERNAL + 5)
	#define NPPM_INTERNAL_BINDSCINTILLAKEY              (NOTEPADPLUS_USER_INTERNAL + 6)
	#define NPPM_INTERNAL_SCINTILLAKEYMODIFIED          (NOTEPADPLUS_USER_INTERNAL + 7)
	#define NPPM_INTERNAL_SCINTILLAFINDERCOLLAPSE       (NOTEPADPLUS_USER_INTERNAL + 8)
	#define NPPM_INTERNAL_SCINTILLAFINDERUNCOLLAPSE     (NOTEPADPLUS_USER_INTERNAL + 9)
	#define NPPM_INTERNAL_DISABLEAUTOUPDATE             (NOTEPADPLUS_USER_INTERNAL + 10)
	#define NPPM_INTERNAL_SETTING_HISTORY_SIZE          (NOTEPADPLUS_USER_INTERNAL + 11)
	#define NPPM_INTERNAL_ISTABBARREDUCED               (NOTEPADPLUS_USER_INTERNAL + 12)
	#define NPPM_INTERNAL_ISFOCUSEDTAB                  (NOTEPADPLUS_USER_INTERNAL + 13)
	#define NPPM_INTERNAL_GETMENU                       (NOTEPADPLUS_USER_INTERNAL + 14)
	#define NPPM_INTERNAL_CLEARINDICATOR                (NOTEPADPLUS_USER_INTERNAL + 15)
	#define NPPM_INTERNAL_SCINTILLAFINDERCOPY           (NOTEPADPLUS_USER_INTERNAL + 16)
	#define NPPM_INTERNAL_SCINTILLAFINDERSELECTALL      (NOTEPADPLUS_USER_INTERNAL + 17)
	#define NPPM_INTERNAL_SETCARETWIDTH                 (NOTEPADPLUS_USER_INTERNAL + 18)
	#define NPPM_INTERNAL_SETCARETBLINKRATE             (NOTEPADPLUS_USER_INTERNAL + 19)
	#define NPPM_INTERNAL_CLEARINDICATORTAGMATCH        (NOTEPADPLUS_USER_INTERNAL + 20)
	#define NPPM_INTERNAL_CLEARINDICATORTAGATTR         (NOTEPADPLUS_USER_INTERNAL + 21)
	#define NPPM_INTERNAL_SWITCHVIEWFROMHWND            (NOTEPADPLUS_USER_INTERNAL + 22)
	#define NPPM_INTERNAL_UPDATETITLEBAR                (NOTEPADPLUS_USER_INTERNAL + 23)
	#define NPPM_INTERNAL_CANCEL_FIND_IN_FILES          (NOTEPADPLUS_USER_INTERNAL + 24)
	#define NPPM_INTERNAL_RELOADNATIVELANG              (NOTEPADPLUS_USER_INTERNAL + 25)
	#define NPPM_INTERNAL_PLUGINSHORTCUTMOTIFIED        (NOTEPADPLUS_USER_INTERNAL + 26)
	#define NPPM_INTERNAL_SCINTILLAFINDERCLEARALL       (NOTEPADPLUS_USER_INTERNAL + 27)
	#define NPPM_INTERNAL_CHANGETABBAEICONS             (NOTEPADPLUS_USER_INTERNAL + 28)
	#define NPPM_INTERNAL_SETTING_TAB_REPLCESPACE       (NOTEPADPLUS_USER_INTERNAL + 29)
	#define NPPM_INTERNAL_SETTING_TAB_SIZE              (NOTEPADPLUS_USER_INTERNAL + 30)
	#define NPPM_INTERNAL_RELOADSTYLERS                 (NOTEPADPLUS_USER_INTERNAL + 31)
	#define NPPM_INTERNAL_DOCORDERCHANGED               (NOTEPADPLUS_USER_INTERNAL + 32)
	#define NPPM_INTERNAL_SETMULTISELCTION              (NOTEPADPLUS_USER_INTERNAL + 33)
	#define NPPM_INTERNAL_SCINTILLAFINDEROPENALL        (NOTEPADPLUS_USER_INTERNAL + 34)
	#define NPPM_INTERNAL_RECENTFILELIST_UPDATE         (NOTEPADPLUS_USER_INTERNAL + 35)
	#define NPPM_INTERNAL_RECENTFILELIST_SWITCH         (NOTEPADPLUS_USER_INTERNAL + 36)
	#define NPPM_INTERNAL_GETSCINTEDTVIEW               (NOTEPADPLUS_USER_INTERNAL + 37)
	#define NPPM_INTERNAL_ENABLESNAPSHOT                (NOTEPADPLUS_USER_INTERNAL + 38)
	#define NPPM_INTERNAL_SAVECURRENTSESSION            (NOTEPADPLUS_USER_INTERNAL + 39)
	#define NPPM_INTERNAL_FINDINFINDERDLG               (NOTEPADPLUS_USER_INTERNAL + 40)
	#define NPPM_INTERNAL_REMOVEFINDER                  (NOTEPADPLUS_USER_INTERNAL + 41)
	#define NPPM_INTERNAL_RELOADSCROLLTOEND                     (NOTEPADPLUS_USER_INTERNAL + 42)  // Used by Monitoring feature
	#define NPPM_INTERNAL_FINDKEYCONFLICTS              (NOTEPADPLUS_USER_INTERNAL + 43)
	#define NPPM_INTERNAL_SCROLLBEYONDLASTLINE          (NOTEPADPLUS_USER_INTERNAL + 44)
	#define NPPM_INTERNAL_SETWORDCHARS                  (NOTEPADPLUS_USER_INTERNAL + 45)
	#define NPPM_INTERNAL_EXPORTFUNCLISTANDQUIT         (NOTEPADPLUS_USER_INTERNAL + 46)
	#define NPPM_INTERNAL_PRNTANDQUIT                   (NOTEPADPLUS_USER_INTERNAL + 47)
	#define NPPM_INTERNAL_SAVEBACKUP                            (NOTEPADPLUS_USER_INTERNAL + 48)
	#define NPPM_INTERNAL_STOPMONITORING                (NOTEPADPLUS_USER_INTERNAL + 49) // Used by Monitoring feature
	#define NPPM_INTERNAL_EDGEBACKGROUND                (NOTEPADPLUS_USER_INTERNAL + 50)
	#define NPPM_INTERNAL_EDGEMULTISETSIZE              (NOTEPADPLUS_USER_INTERNAL + 51)
	#define NPPM_INTERNAL_UPDATECLICKABLELINKS          (NOTEPADPLUS_USER_INTERNAL + 52)
	#define NPPM_INTERNAL_SCINTILLAFINDERWRAP           (NOTEPADPLUS_USER_INTERNAL + 53)
	#define NPPM_INTERNAL_MINIMIZED_TRAY                (NOTEPADPLUS_USER_INTERNAL + 54)
	#define NPPM_INTERNAL_SCINTILLAFINDERCOPYVERBATIM   (NOTEPADPLUS_USER_INTERNAL + 55)
	#define NPPM_INTERNAL_FINDINPROJECTS                (NOTEPADPLUS_USER_INTERNAL + 56)
	#define NPPM_INTERNAL_SCINTILLAFINDERPURGE          (NOTEPADPLUS_USER_INTERNAL + 57)
	#define NPPM_INTERNAL_UPDATETEXTZONEPADDING         (NOTEPADPLUS_USER_INTERNAL + 58)
	#define NPPM_INTERNAL_REFRESHDARKMODE                           (NOTEPADPLUS_USER_INTERNAL + 59)
	#define NPPM_INTERNAL_SCINTILLAFINDERCOPYPATHS      (NOTEPADPLUS_USER_INTERNAL + 60)
	#define NPPM_INTERNAL_REFRESHWORKDIR                (NOTEPADPLUS_USER_INTERNAL + 61)

// See Notepad_plus_msgs.h
//#define NOTEPADPLUS_USER   (WM_USER + 1000)

//
// Used by Doc Monitor plugin
//
	#define NPPM_INTERNAL_CHECKDOCSTATUS (NPPMSG + 53)
// VOID NPPM_CHECKDOCSTATUS(0, 0)
// check all opened documents status.
// If files are modified, then reloaod (with or without prompt, it depends on settings).
// if files are deleted, then prompt user to close the documents

	#define NPPM_INTERNAL_ENABLECHECKDOCOPT (NPPMSG + 54)
// VOID NPPM_ENABLECHECKDOCOPT(OPT, 0)
// where OPT is :
	#define CHECKDOCOPT_NONE 0
	#define CHECKDOCOPT_UPDATESILENTLY 1
	#define CHECKDOCOPT_UPDATEGO2END 2

//
// Used by netnote plugin
//
#define NPPM_INTERNAL_SETFILENAME (NPPMSG + 63)
//wParam: BufferID to rename
//lParam: name to set (TCHAR*)
//Buffer must have been previously unnamed (eg "new 1" document types)

#define SCINTILLA_USER     (WM_USER + 2000)

#define MACRO_USER    (WM_USER + 4000)
	#define WM_GETCURRENTMACROSTATUS (MACRO_USER + 01)
	#define WM_MACRODLGRUNMACRO       (MACRO_USER + 02)

// See Notepad_plus_msgs.h
//#define RUNCOMMAND_USER    (WM_USER + 3000)
#define SPLITTER_USER      (WM_USER + 4000)
#define WORDSTYLE_USER     (WM_USER + 5000)
#define COLOURPOPUP_USER   (WM_USER + 6000)
#define BABYGRID_USER      (WM_USER + 7000)

//#define IDD_DOCKING_MNG (IDM  + 7000)

#define MENUINDEX_FILE     0
#define MENUINDEX_EDIT     1
#define MENUINDEX_SEARCH   2
#define MENUINDEX_VIEW     3
#define MENUINDEX_FORMAT   4
#define MENUINDEX_LANGUAGE 5
#define MENUINDEX_SETTINGS 6
#define MENUINDEX_TOOLS    7
#define MENUINDEX_MACRO    8
#define MENUINDEX_RUN      9
#define MENUINDEX_PLUGINS  10
//
//#include "menuCmdID.h"
#define    IDM    40000

#define    IDM_FILE    (IDM + 1000)
// IMPORTANT: If list below is modified, you have to change the value of IDM_FILEMENU_LASTONE and
// IDM_FILEMENU_EXISTCMDPOSITION
    #define    IDM_FILE_NEW                              (IDM_FILE + 1)
    #define    IDM_FILE_OPEN                             (IDM_FILE + 2)
    #define    IDM_FILE_CLOSE                            (IDM_FILE + 3)
    #define    IDM_FILE_CLOSEALL                         (IDM_FILE + 4)
    #define    IDM_FILE_CLOSEALL_BUT_CURRENT             (IDM_FILE + 5)
    #define    IDM_FILE_SAVE                             (IDM_FILE + 6)
    #define    IDM_FILE_SAVEALL                          (IDM_FILE + 7)
    #define    IDM_FILE_SAVEAS                           (IDM_FILE + 8)
    #define    IDM_FILE_CLOSEALL_TOLEFT                  (IDM_FILE + 9)
    #define    IDM_FILE_PRINT                            (IDM_FILE + 10)
    #define    IDM_FILE_PRINTNOW                         1001
    #define    IDM_FILE_EXIT                             (IDM_FILE + 11)
    #define    IDM_FILE_LOADSESSION                      (IDM_FILE + 12)
    #define    IDM_FILE_SAVESESSION                      (IDM_FILE + 13)
    #define    IDM_FILE_RELOAD                           (IDM_FILE + 14)
    #define    IDM_FILE_SAVECOPYAS                       (IDM_FILE + 15)
    #define    IDM_FILE_DELETE                           (IDM_FILE + 16)
    #define    IDM_FILE_RENAME                           (IDM_FILE + 17)
    #define    IDM_FILE_CLOSEALL_TORIGHT                 (IDM_FILE + 18)
    #define    IDM_FILE_OPEN_FOLDER                      (IDM_FILE + 19)
    #define    IDM_FILE_OPEN_CMD                         (IDM_FILE + 20)
    #define    IDM_FILE_RESTORELASTCLOSEDFILE            (IDM_FILE + 21)
    #define    IDM_FILE_OPENFOLDERASWORSPACE             (IDM_FILE + 22)
    #define    IDM_FILE_OPEN_DEFAULT_VIEWER              (IDM_FILE + 23)
    #define    IDM_FILE_CLOSEALL_UNCHANGED               (IDM_FILE + 24)
    #define    IDM_FILE_CONTAININGFOLDERASWORKSPACE      (IDM_FILE + 25)
// IMPORTANT: If list above is modified, you have to change the following values:

// To be updated if new menu item(s) is (are) added in menu "File"
    #define    IDM_FILEMENU_LASTONE             IDM_FILE_CONTAININGFOLDERASWORKSPACE

// 0 based position of command "Exit" including the bars in the file menu
// and without counting "Recent files history" items

// 0  New
// 1  Open...
// 2  Open Containing Folder
// 3  Open Folder as Workspace
// 4  Open in Default Viewer
// 5  Reload from Disk
// 6  Save
// 7  Save As...
// 8  Save a Copy As...
// 9  Save All
//10  Rename...
//11  Close
//12  Close All
//13  Close More
//14  Move to Recycle Bin
//15  --------
//16  Load Session...
//17  Save Session...
//18  --------
//19  Print...
//20  Print Now
//21  --------
//22  Exit
    #define    IDM_FILEMENU_EXISTCMDPOSITION    22

#define    IDM_EDIT       (IDM + 2000)
    #define    IDM_EDIT_CUT                                     (IDM_EDIT + 1)
    #define    IDM_EDIT_COPY                                    (IDM_EDIT + 2)
    #define    IDM_EDIT_UNDO                                    (IDM_EDIT + 3)
    #define    IDM_EDIT_REDO                                    (IDM_EDIT + 4)
    #define    IDM_EDIT_PASTE                                   (IDM_EDIT + 5)
    #define    IDM_EDIT_DELETE                                  (IDM_EDIT + 6)
    #define    IDM_EDIT_SELECTALL                               (IDM_EDIT + 7)
    #define    IDM_EDIT_INS_TAB                                 (IDM_EDIT + 8)
    #define    IDM_EDIT_RMV_TAB                                 (IDM_EDIT + 9)
    #define    IDM_EDIT_DUP_LINE                                (IDM_EDIT + 10)
    #define    IDM_EDIT_TRANSPOSE_LINE                          (IDM_EDIT + 11)
    #define    IDM_EDIT_SPLIT_LINES                             (IDM_EDIT + 12)
    #define    IDM_EDIT_JOIN_LINES                              (IDM_EDIT + 13)
    #define    IDM_EDIT_LINE_UP                                 (IDM_EDIT + 14)
    #define    IDM_EDIT_LINE_DOWN                               (IDM_EDIT + 15)
    #define    IDM_EDIT_UPPERCASE                               (IDM_EDIT + 16)
    #define    IDM_EDIT_LOWERCASE                               (IDM_EDIT + 17)
    #define    IDM_MACRO_STARTRECORDINGMACRO                    (IDM_EDIT + 18)
    #define    IDM_MACRO_STOPRECORDINGMACRO                     (IDM_EDIT + 19)
    #define    IDM_EDIT_BEGINENDSELECT                          (IDM_EDIT + 20)
    #define    IDM_MACRO_PLAYBACKRECORDEDMACRO                  (IDM_EDIT + 21)
    #define    IDM_EDIT_BLOCK_COMMENT                           (IDM_EDIT + 22)
    #define    IDM_EDIT_STREAM_COMMENT                          (IDM_EDIT + 23)
    #define    IDM_EDIT_TRIMTRAILING                            (IDM_EDIT + 24)
    #define    IDM_MACRO_SAVECURRENTMACRO                       (IDM_EDIT + 25)
    #define    IDM_EDIT_RTL                                     (IDM_EDIT + 26)
    #define    IDM_EDIT_LTR                                     (IDM_EDIT + 27)
    #define    IDM_EDIT_SETREADONLY                             (IDM_EDIT + 28)
    #define    IDM_EDIT_FULLPATHTOCLIP                          (IDM_EDIT + 29)
    #define    IDM_EDIT_FILENAMETOCLIP                          (IDM_EDIT + 30)
    #define    IDM_EDIT_CURRENTDIRTOCLIP                        (IDM_EDIT + 31)
    #define    IDM_MACRO_RUNMULTIMACRODLG                       (IDM_EDIT + 32)
    #define    IDM_EDIT_CLEARREADONLY                           (IDM_EDIT + 33)
    #define    IDM_EDIT_COLUMNMODE                              (IDM_EDIT + 34)
    #define    IDM_EDIT_BLOCK_COMMENT_SET                       (IDM_EDIT + 35)
    #define    IDM_EDIT_BLOCK_UNCOMMENT                         (IDM_EDIT + 36)
    #define    IDM_EDIT_COLUMNMODETIP                           (IDM_EDIT + 37)
    #define    IDM_EDIT_PASTE_AS_HTML                           (IDM_EDIT + 38)
    #define    IDM_EDIT_PASTE_AS_RTF                            (IDM_EDIT + 39)
    #define    IDM_OPEN_ALL_RECENT_FILE                         (IDM_EDIT + 40)
    #define    IDM_CLEAN_RECENT_FILE_LIST                       (IDM_EDIT + 41)
    #define    IDM_EDIT_TRIMLINEHEAD                            (IDM_EDIT + 42)
    #define    IDM_EDIT_TRIM_BOTH                               (IDM_EDIT + 43)
    #define    IDM_EDIT_EOL2WS                                  (IDM_EDIT + 44)
    #define    IDM_EDIT_TRIMALL                                 (IDM_EDIT + 45)
    #define    IDM_EDIT_TAB2SW                                  (IDM_EDIT + 46)
    #define    IDM_EDIT_STREAM_UNCOMMENT                        (IDM_EDIT + 47)
    #define    IDM_EDIT_COPY_BINARY                             (IDM_EDIT + 48)
    #define    IDM_EDIT_CUT_BINARY                              (IDM_EDIT + 49)
    #define    IDM_EDIT_PASTE_BINARY                            (IDM_EDIT + 50)
    #define    IDM_EDIT_CHAR_PANEL                              (IDM_EDIT + 51)
    #define    IDM_EDIT_CLIPBOARDHISTORY_PANEL                  (IDM_EDIT + 52)
    #define    IDM_EDIT_SW2TAB_LEADING                          (IDM_EDIT + 53)
    #define    IDM_EDIT_SW2TAB_ALL                              (IDM_EDIT + 54)
    #define    IDM_EDIT_REMOVEEMPTYLINES                        (IDM_EDIT + 55)
    #define    IDM_EDIT_REMOVEEMPTYLINESWITHBLANK               (IDM_EDIT + 56)
    #define    IDM_EDIT_BLANKLINEABOVECURRENT                   (IDM_EDIT + 57)
    #define    IDM_EDIT_BLANKLINEBELOWCURRENT                   (IDM_EDIT + 58)
    #define    IDM_EDIT_SORTLINES_LEXICOGRAPHIC_ASCENDING       (IDM_EDIT + 59)
    #define    IDM_EDIT_SORTLINES_LEXICOGRAPHIC_DESCENDING      (IDM_EDIT + 60)
    #define    IDM_EDIT_SORTLINES_INTEGER_ASCENDING             (IDM_EDIT + 61)
    #define    IDM_EDIT_SORTLINES_INTEGER_DESCENDING            (IDM_EDIT + 62)
    #define    IDM_EDIT_SORTLINES_DECIMALCOMMA_ASCENDING        (IDM_EDIT + 63)
    #define    IDM_EDIT_SORTLINES_DECIMALCOMMA_DESCENDING       (IDM_EDIT + 64)
    #define    IDM_EDIT_SORTLINES_DECIMALDOT_ASCENDING          (IDM_EDIT + 65)
    #define    IDM_EDIT_SORTLINES_DECIMALDOT_DESCENDING         (IDM_EDIT + 66)
    #define    IDM_EDIT_PROPERCASE_FORCE                        (IDM_EDIT + 67)
    #define    IDM_EDIT_PROPERCASE_BLEND                        (IDM_EDIT + 68)
    #define    IDM_EDIT_SENTENCECASE_FORCE                      (IDM_EDIT + 69)
    #define    IDM_EDIT_SENTENCECASE_BLEND                      (IDM_EDIT + 70)
    #define    IDM_EDIT_INVERTCASE                              (IDM_EDIT + 71)
    #define    IDM_EDIT_RANDOMCASE                              (IDM_EDIT + 72)
    #define    IDM_EDIT_OPENASFILE                              (IDM_EDIT + 73)
    #define    IDM_EDIT_OPENINFOLDER                            (IDM_EDIT + 74)
    #define    IDM_EDIT_SEARCHONINTERNET                        (IDM_EDIT + 75)
    #define    IDM_EDIT_CHANGESEARCHENGINE                      (IDM_EDIT + 76)
    #define    IDM_EDIT_REMOVE_CONSECUTIVE_DUP_LINES            (IDM_EDIT + 77)
    #define    IDM_EDIT_SORTLINES_RANDOMLY                      (IDM_EDIT + 78)
    #define    IDM_EDIT_REMOVE_ANY_DUP_LINES                    (IDM_EDIT + 79)
    #define    IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_ASCENDING  (IDM_EDIT + 80)
    #define    IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_DESCENDING (IDM_EDIT + 81)
    #define    IDM_EDIT_COPY_LINK                               (IDM_EDIT + 82)
    #define    IDM_EDIT_SORTLINES_REVERSE_ORDER                 (IDM_EDIT + 83)
    #define    IDM_EDIT_INSERT_DATETIME_SHORT                   (IDM_EDIT + 84)
    #define    IDM_EDIT_INSERT_DATETIME_LONG                    (IDM_EDIT + 85)
    #define    IDM_EDIT_INSERT_DATETIME_CUSTOMIZED              (IDM_EDIT + 86)

    #define    IDM_EDIT_AUTOCOMPLETE                            (50000 + 0)
    #define    IDM_EDIT_AUTOCOMPLETE_CURRENTFILE                (50000 + 1)
    #define    IDM_EDIT_FUNCCALLTIP                             (50000 + 2)
    #define    IDM_EDIT_AUTOCOMPLETE_PATH                       (50000 + 6)

#define    IDM_SEARCH    (IDM + 3000)
    #define    IDM_SEARCH_FIND                 (IDM_SEARCH + 1)
    #define    IDM_SEARCH_FINDNEXT             (IDM_SEARCH + 2)
    #define    IDM_SEARCH_REPLACE              (IDM_SEARCH + 3)
    #define    IDM_SEARCH_GOTOLINE             (IDM_SEARCH + 4)
    #define    IDM_SEARCH_TOGGLE_BOOKMARK      (IDM_SEARCH + 5)
    #define    IDM_SEARCH_NEXT_BOOKMARK        (IDM_SEARCH + 6)
    #define    IDM_SEARCH_PREV_BOOKMARK        (IDM_SEARCH + 7)
    #define    IDM_SEARCH_CLEAR_BOOKMARKS      (IDM_SEARCH + 8)
    #define    IDM_SEARCH_GOTOMATCHINGBRACE    (IDM_SEARCH + 9)
    #define    IDM_SEARCH_FINDPREV             (IDM_SEARCH + 10)
    #define    IDM_SEARCH_FINDINCREMENT        (IDM_SEARCH + 11)
    #define    IDM_SEARCH_FINDINFILES          (IDM_SEARCH + 13)
    #define    IDM_SEARCH_VOLATILE_FINDNEXT    (IDM_SEARCH + 14)
    #define    IDM_SEARCH_VOLATILE_FINDPREV    (IDM_SEARCH + 15)
    #define    IDM_SEARCH_CUTMARKEDLINES       (IDM_SEARCH + 18)
    #define    IDM_SEARCH_COPYMARKEDLINES      (IDM_SEARCH + 19)
    #define    IDM_SEARCH_PASTEMARKEDLINES     (IDM_SEARCH + 20)
    #define    IDM_SEARCH_DELETEMARKEDLINES    (IDM_SEARCH + 21)
    #define    IDM_SEARCH_MARKALLEXT1          (IDM_SEARCH + 22)
    #define    IDM_SEARCH_UNMARKALLEXT1        (IDM_SEARCH + 23)
    #define    IDM_SEARCH_MARKALLEXT2          (IDM_SEARCH + 24)
    #define    IDM_SEARCH_UNMARKALLEXT2        (IDM_SEARCH + 25)
    #define    IDM_SEARCH_MARKALLEXT3          (IDM_SEARCH + 26)
    #define    IDM_SEARCH_UNMARKALLEXT3        (IDM_SEARCH + 27)
    #define    IDM_SEARCH_MARKALLEXT4          (IDM_SEARCH + 28)
    #define    IDM_SEARCH_UNMARKALLEXT4        (IDM_SEARCH + 29)
    #define    IDM_SEARCH_MARKALLEXT5          (IDM_SEARCH + 30)
    #define    IDM_SEARCH_UNMARKALLEXT5        (IDM_SEARCH + 31)
    #define    IDM_SEARCH_CLEARALLMARKS        (IDM_SEARCH + 32)

    #define    IDM_SEARCH_GOPREVMARKER1        (IDM_SEARCH + 33)
    #define    IDM_SEARCH_GOPREVMARKER2        (IDM_SEARCH + 34)
    #define    IDM_SEARCH_GOPREVMARKER3        (IDM_SEARCH + 35)
    #define    IDM_SEARCH_GOPREVMARKER4        (IDM_SEARCH + 36)
    #define    IDM_SEARCH_GOPREVMARKER5        (IDM_SEARCH + 37)
    #define    IDM_SEARCH_GOPREVMARKER_DEF     (IDM_SEARCH + 38)

    #define    IDM_SEARCH_GONEXTMARKER1        (IDM_SEARCH + 39)
    #define    IDM_SEARCH_GONEXTMARKER2        (IDM_SEARCH + 40)
    #define    IDM_SEARCH_GONEXTMARKER3        (IDM_SEARCH + 41)
    #define    IDM_SEARCH_GONEXTMARKER4        (IDM_SEARCH + 42)
    #define    IDM_SEARCH_GONEXTMARKER5        (IDM_SEARCH + 43)
    #define    IDM_SEARCH_GONEXTMARKER_DEF     (IDM_SEARCH + 44)

    #define    IDM_FOCUS_ON_FOUND_RESULTS      (IDM_SEARCH + 45)
    #define    IDM_SEARCH_GOTONEXTFOUND        (IDM_SEARCH + 46)
    #define    IDM_SEARCH_GOTOPREVFOUND        (IDM_SEARCH + 47)

    #define    IDM_SEARCH_SETANDFINDNEXT       (IDM_SEARCH + 48)
    #define    IDM_SEARCH_SETANDFINDPREV       (IDM_SEARCH + 49)
    #define    IDM_SEARCH_INVERSEMARKS         (IDM_SEARCH + 50)
    #define    IDM_SEARCH_DELETEUNMARKEDLINES  (IDM_SEARCH + 51)
    #define    IDM_SEARCH_FINDCHARINRANGE      (IDM_SEARCH + 52)
    #define    IDM_SEARCH_SELECTMATCHINGBRACES (IDM_SEARCH + 53)
    #define    IDM_SEARCH_MARK                 (IDM_SEARCH + 54)

    #define    IDM_SEARCH_STYLE1TOCLIP         (IDM_SEARCH + 55)
    #define    IDM_SEARCH_STYLE2TOCLIP         (IDM_SEARCH + 56)
    #define    IDM_SEARCH_STYLE3TOCLIP         (IDM_SEARCH + 57)
    #define    IDM_SEARCH_STYLE4TOCLIP         (IDM_SEARCH + 58)
    #define    IDM_SEARCH_STYLE5TOCLIP         (IDM_SEARCH + 59)
    #define    IDM_SEARCH_ALLSTYLESTOCLIP      (IDM_SEARCH + 60)
    #define    IDM_SEARCH_MARKEDTOCLIP         (IDM_SEARCH + 61)

    #define    IDM_SEARCH_MARKONEEXT1          (IDM_SEARCH + 62)
    #define    IDM_SEARCH_MARKONEEXT2          (IDM_SEARCH + 63)
    #define    IDM_SEARCH_MARKONEEXT3          (IDM_SEARCH + 64)
    #define    IDM_SEARCH_MARKONEEXT4          (IDM_SEARCH + 65)
    #define    IDM_SEARCH_MARKONEEXT5          (IDM_SEARCH + 66)

#define    IDM_MISC    (IDM + 3500)
    #define    IDM_DOCLIST_FILESCLOSE             (IDM_MISC + 1)
    #define    IDM_DOCLIST_FILESCLOSEOTHERS       (IDM_MISC + 2)

#define IDM_VIEW    (IDM + 4000)
//#define    IDM_VIEW_TOOLBAR_HIDE            (IDM_VIEW + 1)
    #define    IDM_VIEW_TOOLBAR_REDUCE            (IDM_VIEW + 2)
    #define    IDM_VIEW_TOOLBAR_ENLARGE           (IDM_VIEW + 3)
    #define    IDM_VIEW_TOOLBAR_STANDARD          (IDM_VIEW + 4)
    #define    IDM_VIEW_REDUCETABBAR              (IDM_VIEW + 5)
    #define    IDM_VIEW_LOCKTABBAR                (IDM_VIEW + 6)
    #define    IDM_VIEW_DRAWTABBAR_TOPBAR         (IDM_VIEW + 7)
    #define    IDM_VIEW_DRAWTABBAR_INACIVETAB     (IDM_VIEW + 8)
    #define    IDM_VIEW_POSTIT                    (IDM_VIEW + 9)
    #define    IDM_VIEW_TOGGLE_FOLDALL            (IDM_VIEW + 10)
    #define    IDM_VIEW_DISTRACTIONFREE           (IDM_VIEW + 11)
    #define    IDM_VIEW_LINENUMBER                (IDM_VIEW + 12)
    #define    IDM_VIEW_SYMBOLMARGIN              (IDM_VIEW + 13)
    #define    IDM_VIEW_FOLDERMAGIN               (IDM_VIEW + 14)
    #define    IDM_VIEW_FOLDERMAGIN_SIMPLE        (IDM_VIEW + 15)
    #define    IDM_VIEW_FOLDERMAGIN_ARROW         (IDM_VIEW + 16)
    #define    IDM_VIEW_FOLDERMAGIN_CIRCLE        (IDM_VIEW + 17)
    #define    IDM_VIEW_FOLDERMAGIN_BOX           (IDM_VIEW + 18)
    #define    IDM_VIEW_ALL_CHARACTERS            (IDM_VIEW + 19)
    #define    IDM_VIEW_INDENT_GUIDE              (IDM_VIEW + 20)
    #define    IDM_VIEW_CURLINE_HILITING          (IDM_VIEW + 21)
    #define    IDM_VIEW_WRAP                      (IDM_VIEW + 22)
    #define    IDM_VIEW_ZOOMIN                    (IDM_VIEW + 23)
    #define    IDM_VIEW_ZOOMOUT                   (IDM_VIEW + 24)
    #define    IDM_VIEW_TAB_SPACE                 (IDM_VIEW + 25)
    #define    IDM_VIEW_EOL                       (IDM_VIEW + 26)
    #define    IDM_VIEW_TOOLBAR_REDUCE_SET2       (IDM_VIEW + 27)
    #define    IDM_VIEW_TOOLBAR_ENLARGE_SET2      (IDM_VIEW + 28)
    #define    IDM_VIEW_TOGGLE_UNFOLDALL          (IDM_VIEW + 29)
    #define    IDM_VIEW_FOLD_CURRENT              (IDM_VIEW + 30)
    #define    IDM_VIEW_UNFOLD_CURRENT            (IDM_VIEW + 31)
    #define    IDM_VIEW_FULLSCREENTOGGLE          (IDM_VIEW + 32)
    #define    IDM_VIEW_ZOOMRESTORE               (IDM_VIEW + 33)
    #define    IDM_VIEW_ALWAYSONTOP               (IDM_VIEW + 34)
    #define    IDM_VIEW_SYNSCROLLV                (IDM_VIEW + 35)
    #define    IDM_VIEW_SYNSCROLLH                (IDM_VIEW + 36)
//#define    IDM_VIEW_EDGENONE                  (IDM_VIEW + 37)
    #define    IDM_VIEW_DRAWTABBAR_CLOSEBOTTUN    (IDM_VIEW + 38)
    #define    IDM_VIEW_DRAWTABBAR_DBCLK2CLOSE    (IDM_VIEW + 39)
    #define    IDM_VIEW_REFRESHTABAR              (IDM_VIEW + 40)
    #define    IDM_VIEW_WRAP_SYMBOL               (IDM_VIEW + 41)
    #define    IDM_VIEW_HIDELINES                 (IDM_VIEW + 42)
    #define    IDM_VIEW_DRAWTABBAR_VERTICAL       (IDM_VIEW + 43)
    #define    IDM_VIEW_DRAWTABBAR_MULTILINE      (IDM_VIEW + 44)
    #define    IDM_VIEW_DOCCHANGEMARGIN           (IDM_VIEW + 45)
    #define    IDM_VIEW_LWDEF                     (IDM_VIEW + 46)
    #define    IDM_VIEW_LWALIGN                   (IDM_VIEW + 47)
    #define    IDM_VIEW_LWINDENT                  (IDM_VIEW + 48)
    #define    IDM_VIEW_SUMMARY                   (IDM_VIEW + 49)

    #define    IDM_VIEW_FOLD                      (IDM_VIEW + 50)
	#define    IDM_VIEW_FOLD_1    (IDM_VIEW_FOLD + 1)
	#define    IDM_VIEW_FOLD_2    (IDM_VIEW_FOLD + 2)
	#define    IDM_VIEW_FOLD_3    (IDM_VIEW_FOLD + 3)
	#define    IDM_VIEW_FOLD_4    (IDM_VIEW_FOLD + 4)
	#define    IDM_VIEW_FOLD_5    (IDM_VIEW_FOLD + 5)
	#define    IDM_VIEW_FOLD_6    (IDM_VIEW_FOLD + 6)
	#define    IDM_VIEW_FOLD_7    (IDM_VIEW_FOLD + 7)
	#define    IDM_VIEW_FOLD_8    (IDM_VIEW_FOLD + 8)

    #define    IDM_VIEW_UNFOLD                    (IDM_VIEW + 60)
	#define    IDM_VIEW_UNFOLD_1    (IDM_VIEW_UNFOLD + 1)
	#define    IDM_VIEW_UNFOLD_2    (IDM_VIEW_UNFOLD + 2)
	#define    IDM_VIEW_UNFOLD_3    (IDM_VIEW_UNFOLD + 3)
	#define    IDM_VIEW_UNFOLD_4    (IDM_VIEW_UNFOLD + 4)
	#define    IDM_VIEW_UNFOLD_5    (IDM_VIEW_UNFOLD + 5)
	#define    IDM_VIEW_UNFOLD_6    (IDM_VIEW_UNFOLD + 6)
	#define    IDM_VIEW_UNFOLD_7    (IDM_VIEW_UNFOLD + 7)
	#define    IDM_VIEW_UNFOLD_8    (IDM_VIEW_UNFOLD + 8)

    #define    IDM_VIEW_DOCLIST                   (IDM_VIEW + 70)
    #define    IDM_VIEW_SWITCHTO_OTHER_VIEW       (IDM_VIEW + 72)
    #define    IDM_EXPORT_FUNC_LIST_AND_QUIT      (IDM_VIEW + 73)

    #define    IDM_VIEW_DOC_MAP                   (IDM_VIEW + 80)

    #define    IDM_VIEW_PROJECT_PANEL_1           (IDM_VIEW + 81)
    #define    IDM_VIEW_PROJECT_PANEL_2           (IDM_VIEW + 82)
    #define    IDM_VIEW_PROJECT_PANEL_3           (IDM_VIEW + 83)

    #define    IDM_VIEW_FUNC_LIST                 (IDM_VIEW + 84)
    #define    IDM_VIEW_FILEBROWSER               (IDM_VIEW + 85)

    #define    IDM_VIEW_TAB1                      (IDM_VIEW + 86)
    #define    IDM_VIEW_TAB2                      (IDM_VIEW + 87)
    #define    IDM_VIEW_TAB3                      (IDM_VIEW + 88)
    #define    IDM_VIEW_TAB4                      (IDM_VIEW + 89)
    #define    IDM_VIEW_TAB5                      (IDM_VIEW + 90)
    #define    IDM_VIEW_TAB6                      (IDM_VIEW + 91)
    #define    IDM_VIEW_TAB7                      (IDM_VIEW + 92)
    #define    IDM_VIEW_TAB8                      (IDM_VIEW + 93)
    #define    IDM_VIEW_TAB9                      (IDM_VIEW + 94)
    #define    IDM_VIEW_TAB_NEXT                  (IDM_VIEW + 95)
    #define    IDM_VIEW_TAB_PREV                  (IDM_VIEW + 96)
    #define    IDM_VIEW_MONITORING                (IDM_VIEW + 97)
    #define    IDM_VIEW_TAB_MOVEFORWARD           (IDM_VIEW + 98)
    #define    IDM_VIEW_TAB_MOVEBACKWARD          (IDM_VIEW + 99)
    #define    IDM_VIEW_IN_FIREFOX                (IDM_VIEW + 100)
    #define    IDM_VIEW_IN_CHROME                 (IDM_VIEW + 101)
    #define    IDM_VIEW_IN_EDGE                   (IDM_VIEW + 102)
    #define    IDM_VIEW_IN_IE                     (IDM_VIEW + 103)

    #define    IDM_VIEW_SWITCHTO_PROJECT_PANEL_1  (IDM_VIEW + 104)
    #define    IDM_VIEW_SWITCHTO_PROJECT_PANEL_2  (IDM_VIEW + 105)
    #define    IDM_VIEW_SWITCHTO_PROJECT_PANEL_3  (IDM_VIEW + 106)
    #define    IDM_VIEW_SWITCHTO_FILEBROWSER      (IDM_VIEW + 107)
    #define    IDM_VIEW_SWITCHTO_FUNC_LIST        (IDM_VIEW + 108)
    #define    IDM_VIEW_SWITCHTO_DOCLIST          (IDM_VIEW + 109)

    #define    IDM_VIEW_GOTO_ANOTHER_VIEW        10001
    #define    IDM_VIEW_CLONE_TO_ANOTHER_VIEW    10002
    #define    IDM_VIEW_GOTO_NEW_INSTANCE        10003
    #define    IDM_VIEW_LOAD_IN_NEW_INSTANCE     10004

#define    IDM_FORMAT    (IDM + 5000)
    #define    IDM_FORMAT_TODOS             (IDM_FORMAT + 1)
    #define    IDM_FORMAT_TOUNIX            (IDM_FORMAT + 2)
    #define    IDM_FORMAT_TOMAC             (IDM_FORMAT + 3)
    #define    IDM_FORMAT_ANSI              (IDM_FORMAT + 4)
    #define    IDM_FORMAT_UTF_8             (IDM_FORMAT + 5)
    #define    IDM_FORMAT_UTF_16BE          (IDM_FORMAT + 6)
    #define    IDM_FORMAT_UTF_16LE          (IDM_FORMAT + 7)
    #define    IDM_FORMAT_AS_UTF_8          (IDM_FORMAT + 8)
    #define    IDM_FORMAT_CONV2_ANSI        (IDM_FORMAT + 9)
    #define    IDM_FORMAT_CONV2_AS_UTF_8    (IDM_FORMAT + 10)
    #define    IDM_FORMAT_CONV2_UTF_8       (IDM_FORMAT + 11)
    #define    IDM_FORMAT_CONV2_UTF_16BE    (IDM_FORMAT + 12)
    #define    IDM_FORMAT_CONV2_UTF_16LE    (IDM_FORMAT + 13)

    #define    IDM_FORMAT_ENCODE            (IDM_FORMAT + 20)
    #define    IDM_FORMAT_WIN_1250          (IDM_FORMAT_ENCODE + 0)
    #define    IDM_FORMAT_WIN_1251          (IDM_FORMAT_ENCODE + 1)
    #define    IDM_FORMAT_WIN_1252          (IDM_FORMAT_ENCODE + 2)
    #define    IDM_FORMAT_WIN_1253          (IDM_FORMAT_ENCODE + 3)
    #define    IDM_FORMAT_WIN_1254          (IDM_FORMAT_ENCODE + 4)
    #define    IDM_FORMAT_WIN_1255          (IDM_FORMAT_ENCODE + 5)
    #define    IDM_FORMAT_WIN_1256          (IDM_FORMAT_ENCODE + 6)
    #define    IDM_FORMAT_WIN_1257          (IDM_FORMAT_ENCODE + 7)
    #define    IDM_FORMAT_WIN_1258          (IDM_FORMAT_ENCODE + 8)
    #define    IDM_FORMAT_ISO_8859_1        (IDM_FORMAT_ENCODE + 9)
    #define    IDM_FORMAT_ISO_8859_2        (IDM_FORMAT_ENCODE + 10)
    #define    IDM_FORMAT_ISO_8859_3        (IDM_FORMAT_ENCODE + 11)
    #define    IDM_FORMAT_ISO_8859_4        (IDM_FORMAT_ENCODE + 12)
    #define    IDM_FORMAT_ISO_8859_5        (IDM_FORMAT_ENCODE + 13)
    #define    IDM_FORMAT_ISO_8859_6        (IDM_FORMAT_ENCODE + 14)
    #define    IDM_FORMAT_ISO_8859_7        (IDM_FORMAT_ENCODE + 15)
    #define    IDM_FORMAT_ISO_8859_8        (IDM_FORMAT_ENCODE + 16)
    #define    IDM_FORMAT_ISO_8859_9        (IDM_FORMAT_ENCODE + 17)
//#define    IDM_FORMAT_ISO_8859_10       (IDM_FORMAT_ENCODE + 18)
//#define    IDM_FORMAT_ISO_8859_11       (IDM_FORMAT_ENCODE + 19)
    #define    IDM_FORMAT_ISO_8859_13       (IDM_FORMAT_ENCODE + 20)
    #define    IDM_FORMAT_ISO_8859_14       (IDM_FORMAT_ENCODE + 21)
    #define    IDM_FORMAT_ISO_8859_15       (IDM_FORMAT_ENCODE + 22)
//#define    IDM_FORMAT_ISO_8859_16       (IDM_FORMAT_ENCODE + 23)
    #define    IDM_FORMAT_DOS_437           (IDM_FORMAT_ENCODE + 24)
    #define    IDM_FORMAT_DOS_720           (IDM_FORMAT_ENCODE + 25)
    #define    IDM_FORMAT_DOS_737           (IDM_FORMAT_ENCODE + 26)
    #define    IDM_FORMAT_DOS_775           (IDM_FORMAT_ENCODE + 27)
    #define    IDM_FORMAT_DOS_850           (IDM_FORMAT_ENCODE + 28)
    #define    IDM_FORMAT_DOS_852           (IDM_FORMAT_ENCODE + 29)
    #define    IDM_FORMAT_DOS_855           (IDM_FORMAT_ENCODE + 30)
    #define    IDM_FORMAT_DOS_857           (IDM_FORMAT_ENCODE + 31)
    #define    IDM_FORMAT_DOS_858           (IDM_FORMAT_ENCODE + 32)
    #define    IDM_FORMAT_DOS_860           (IDM_FORMAT_ENCODE + 33)
    #define    IDM_FORMAT_DOS_861           (IDM_FORMAT_ENCODE + 34)
    #define    IDM_FORMAT_DOS_862           (IDM_FORMAT_ENCODE + 35)
    #define    IDM_FORMAT_DOS_863           (IDM_FORMAT_ENCODE + 36)
    #define    IDM_FORMAT_DOS_865           (IDM_FORMAT_ENCODE + 37)
    #define    IDM_FORMAT_DOS_866           (IDM_FORMAT_ENCODE + 38)
    #define    IDM_FORMAT_DOS_869           (IDM_FORMAT_ENCODE + 39)
    #define    IDM_FORMAT_BIG5              (IDM_FORMAT_ENCODE + 40)
    #define    IDM_FORMAT_GB2312            (IDM_FORMAT_ENCODE + 41)
    #define    IDM_FORMAT_SHIFT_JIS         (IDM_FORMAT_ENCODE + 42)
    #define    IDM_FORMAT_KOREAN_WIN        (IDM_FORMAT_ENCODE + 43)
    #define    IDM_FORMAT_EUC_KR            (IDM_FORMAT_ENCODE + 44)
    #define    IDM_FORMAT_TIS_620           (IDM_FORMAT_ENCODE + 45)
    #define    IDM_FORMAT_MAC_CYRILLIC      (IDM_FORMAT_ENCODE + 46)
    #define    IDM_FORMAT_KOI8U_CYRILLIC    (IDM_FORMAT_ENCODE + 47)
    #define    IDM_FORMAT_KOI8R_CYRILLIC    (IDM_FORMAT_ENCODE + 48)
    #define    IDM_FORMAT_ENCODE_END        IDM_FORMAT_KOI8R_CYRILLIC

//#define    IDM_FORMAT_CONVERT           200

#define    IDM_LANG    (IDM + 6000)
    #define    IDM_LANGSTYLE_CONFIG_DLG    (IDM_LANG + 1)
    #define    IDM_LANG_C                  (IDM_LANG + 2)
    #define    IDM_LANG_CPP                (IDM_LANG + 3)
    #define    IDM_LANG_JAVA               (IDM_LANG + 4)
    #define    IDM_LANG_HTML               (IDM_LANG + 5)
    #define    IDM_LANG_XML                (IDM_LANG + 6)
    #define    IDM_LANG_JS                 (IDM_LANG + 7)
    #define    IDM_LANG_PHP                (IDM_LANG + 8)
    #define    IDM_LANG_ASP                (IDM_LANG + 9)
    #define    IDM_LANG_CSS                (IDM_LANG + 10)
    #define    IDM_LANG_PASCAL             (IDM_LANG + 11)
    #define    IDM_LANG_PYTHON             (IDM_LANG + 12)
    #define    IDM_LANG_PERL               (IDM_LANG + 13)
    #define    IDM_LANG_OBJC               (IDM_LANG + 14)
    #define    IDM_LANG_ASCII              (IDM_LANG + 15)
    #define    IDM_LANG_TEXT               (IDM_LANG + 16)
    #define    IDM_LANG_RC                 (IDM_LANG + 17)
    #define    IDM_LANG_MAKEFILE           (IDM_LANG + 18)
    #define    IDM_LANG_INI                (IDM_LANG + 19)
    #define    IDM_LANG_SQL                (IDM_LANG + 20)
    #define    IDM_LANG_VB                 (IDM_LANG + 21)
    #define    IDM_LANG_BATCH              (IDM_LANG + 22)
    #define    IDM_LANG_CS                 (IDM_LANG + 23)
    #define    IDM_LANG_LUA                (IDM_LANG + 24)
    #define    IDM_LANG_TEX                (IDM_LANG + 25)
    #define    IDM_LANG_FORTRAN            (IDM_LANG + 26)
    #define    IDM_LANG_BASH               (IDM_LANG + 27)
    #define    IDM_LANG_FLASH              (IDM_LANG + 28)
    #define    IDM_LANG_NSIS               (IDM_LANG + 29)
    #define    IDM_LANG_TCL                (IDM_LANG + 30)
    #define    IDM_LANG_LISP               (IDM_LANG + 31)
    #define    IDM_LANG_SCHEME             (IDM_LANG + 32)
    #define    IDM_LANG_ASM                (IDM_LANG + 33)
    #define    IDM_LANG_DIFF               (IDM_LANG + 34)
    #define    IDM_LANG_PROPS              (IDM_LANG + 35)
    #define    IDM_LANG_PS                 (IDM_LANG + 36)
    #define    IDM_LANG_RUBY               (IDM_LANG + 37)
    #define    IDM_LANG_SMALLTALK          (IDM_LANG + 38)
    #define    IDM_LANG_VHDL               (IDM_LANG + 39)
    #define    IDM_LANG_CAML               (IDM_LANG + 40)
    #define    IDM_LANG_KIX                (IDM_LANG + 41)
    #define    IDM_LANG_ADA                (IDM_LANG + 42)
    #define    IDM_LANG_VERILOG            (IDM_LANG + 43)
    #define    IDM_LANG_AU3                (IDM_LANG + 44)
    #define    IDM_LANG_MATLAB             (IDM_LANG + 45)
    #define    IDM_LANG_HASKELL            (IDM_LANG + 46)
    #define    IDM_LANG_INNO               (IDM_LANG + 47)
    #define    IDM_LANG_CMAKE              (IDM_LANG + 48)
    #define    IDM_LANG_YAML               (IDM_LANG + 49)
    #define    IDM_LANG_COBOL              (IDM_LANG + 50)
    #define    IDM_LANG_D                  (IDM_LANG + 51)
    #define    IDM_LANG_GUI4CLI            (IDM_LANG + 52)
    #define    IDM_LANG_POWERSHELL         (IDM_LANG + 53)
    #define    IDM_LANG_R                  (IDM_LANG + 54)
    #define    IDM_LANG_JSP                (IDM_LANG + 55)
    #define    IDM_LANG_COFFEESCRIPT       (IDM_LANG + 56)
    #define    IDM_LANG_JSON               (IDM_LANG + 57)
    #define    IDM_LANG_FORTRAN_77         (IDM_LANG + 58)
    #define    IDM_LANG_BAANC              (IDM_LANG + 59)
    #define    IDM_LANG_SREC               (IDM_LANG + 60)
    #define    IDM_LANG_IHEX               (IDM_LANG + 61)
    #define    IDM_LANG_TEHEX              (IDM_LANG + 62)
    #define    IDM_LANG_SWIFT              (IDM_LANG + 63)
    #define    IDM_LANG_ASN1               (IDM_LANG + 64)
    #define    IDM_LANG_AVS                (IDM_LANG + 65)
    #define    IDM_LANG_BLITZBASIC         (IDM_LANG + 66)
    #define    IDM_LANG_PUREBASIC          (IDM_LANG + 67)
    #define    IDM_LANG_FREEBASIC          (IDM_LANG + 68)
    #define    IDM_LANG_CSOUND             (IDM_LANG + 69)
    #define    IDM_LANG_ERLANG             (IDM_LANG + 70)
    #define    IDM_LANG_ESCRIPT            (IDM_LANG + 71)
    #define    IDM_LANG_FORTH              (IDM_LANG + 72)
    #define    IDM_LANG_LATEX              (IDM_LANG + 73)
    #define    IDM_LANG_MMIXAL             (IDM_LANG + 74)
    #define    IDM_LANG_NIM                (IDM_LANG + 75)
    #define    IDM_LANG_NNCRONTAB          (IDM_LANG + 76)
    #define    IDM_LANG_OSCRIPT            (IDM_LANG + 77)
    #define    IDM_LANG_REBOL              (IDM_LANG + 78)
    #define    IDM_LANG_REGISTRY           (IDM_LANG + 79)
    #define    IDM_LANG_RUST               (IDM_LANG + 80)
    #define    IDM_LANG_SPICE              (IDM_LANG + 81)
    #define    IDM_LANG_TXT2TAGS           (IDM_LANG + 82)
    #define    IDM_LANG_VISUALPROLOG       (IDM_LANG + 83)
    #define    IDM_LANG_TYPESCRIPT         (IDM_LANG + 84)

    #define    IDM_LANG_EXTERNAL           (IDM_LANG + 165)
    #define    IDM_LANG_EXTERNAL_LIMIT     (IDM_LANG + 179)

    #define    IDM_LANG_USER               (IDM_LANG + 180)     //46180: Used for translation
    #define    IDM_LANG_USER_LIMIT         (IDM_LANG + 210)     //46210: Ajust with IDM_LANG_USER
    #define    IDM_LANG_USER_DLG           (IDM_LANG + 250)     //46250: Used for translation
    #define    IDM_LANG_OPENUDLDIR         (IDM_LANG + 300)

#define    IDM_ABOUT    (IDM  + 7000)
    #define    IDM_HOMESWEETHOME    (IDM_ABOUT  + 1)
    #define    IDM_PROJECTPAGE      (IDM_ABOUT  + 2)
    #define    IDM_ONLINEDOCUMENT   (IDM_ABOUT  + 3)
    #define    IDM_FORUM            (IDM_ABOUT  + 4)
//#define    IDM_PLUGINSHOME      (IDM_ABOUT  + 5)
    #define    IDM_UPDATE_NPP       (IDM_ABOUT  + 6)
    #define    IDM_WIKIFAQ          (IDM_ABOUT  + 7)
//#define    IDM_HELP             (IDM_ABOUT  + 8)
    #define    IDM_CONFUPDATERPROXY (IDM_ABOUT  + 9)
    #define    IDM_CMDLINEARGUMENTS (IDM_ABOUT  + 10)
//#define    IDM_ONLINESUPPORT    (IDM_ABOUT  + 11)
    #define    IDM_DEBUGINFO        (IDM_ABOUT  + 12)

#define    IDM_SETTING    (IDM + 8000)
//    #define    IDM_SETTING_TAB_SIZE                 (IDM_SETTING + 1)
//    #define    IDM_SETTING_TAB_REPLCESPACE          (IDM_SETTING + 2)
//    #define    IDM_SETTING_HISTORY_SIZE             (IDM_SETTING + 3)
//    #define    IDM_SETTING_EDGE_SIZE                (IDM_SETTING + 4)
    #define    IDM_SETTING_IMPORTPLUGIN             (IDM_SETTING + 5)
    #define    IDM_SETTING_IMPORTSTYLETHEMS         (IDM_SETTING + 6)
    #define    IDM_SETTING_TRAYICON                 (IDM_SETTING + 8)
    #define    IDM_SETTING_SHORTCUT_MAPPER          (IDM_SETTING + 9)
    #define    IDM_SETTING_REMEMBER_LAST_SESSION    (IDM_SETTING + 10)
    #define    IDM_SETTING_PREFERENCE               (IDM_SETTING + 11)
    #define    IDM_SETTING_OPENPLUGINSDIR           (IDM_SETTING + 14)
    #define    IDM_SETTING_PLUGINADM                (IDM_SETTING + 15)
    #define    IDM_SETTING_SHORTCUT_MAPPER_MACRO    (IDM_SETTING + 16)
    #define    IDM_SETTING_SHORTCUT_MAPPER_RUN      (IDM_SETTING + 17)
    #define    IDM_SETTING_EDITCONTEXTMENU          (IDM_SETTING + 18)

#define    IDM_TOOL  (IDM + 8500)
    #define    IDM_TOOL_MD5_GENERATE                    (IDM_TOOL + 1)
    #define    IDM_TOOL_MD5_GENERATEFROMFILE            (IDM_TOOL + 2)
    #define    IDM_TOOL_MD5_GENERATEINTOCLIPBOARD       (IDM_TOOL + 3)
    #define    IDM_TOOL_SHA256_GENERATE                 (IDM_TOOL + 4)
    #define    IDM_TOOL_SHA256_GENERATEFROMFILE         (IDM_TOOL + 5)
    #define    IDM_TOOL_SHA256_GENERATEINTOCLIPBOARD    (IDM_TOOL + 6)

#define    IDM_EXECUTE  (IDM + 9000)

#define IDM_SYSTRAYPOPUP     (IDM + 3100)
    #define IDM_SYSTRAYPOPUP_ACTIVATE         (IDM_SYSTRAYPOPUP + 1)
    #define IDM_SYSTRAYPOPUP_NEWDOC           (IDM_SYSTRAYPOPUP + 2)
    #define IDM_SYSTRAYPOPUP_NEW_AND_PASTE    (IDM_SYSTRAYPOPUP + 3)
    #define IDM_SYSTRAYPOPUP_OPENFILE         (IDM_SYSTRAYPOPUP + 4)
    #define IDM_SYSTRAYPOPUP_CLOSE            (IDM_SYSTRAYPOPUP + 5)
//
//#include "md5Dlgs_rc.h"
#define    IDD_HASHFROMFILES_DLG           1920
    #define    IDC_HASH_PATH_EDIT                (IDD_HASHFROMFILES_DLG + 1)
    #define    IDC_HASH_FILEBROWSER_BUTTON       (IDD_HASHFROMFILES_DLG + 2)
    #define    IDC_HASH_RESULT_EDIT              (IDD_HASHFROMFILES_DLG + 3)
    #define    IDC_HASH_TOCLIPBOARD_BUTTON       (IDD_HASHFROMFILES_DLG + 4)

#define    IDD_HASHFROMTEXT_DLG            1930
    #define    IDC_HASH_TEXT_EDIT                    (IDD_HASHFROMTEXT_DLG + 1)
    #define    IDC_HASH_EACHLINE_CHECK               (IDD_HASHFROMTEXT_DLG + 2)
    #define    IDC_HASH_RESULT_FOMTEXT_EDIT          (IDD_HASHFROMTEXT_DLG + 3)
    #define    IDC_HASH_FROMTEXT_TOCLIPBOARD_BUTTON  (IDD_HASHFROMTEXT_DLG + 4)
//
//#include "functionListPanel_rc.h"
#define IDD_FUNCLIST_PANEL              3400
#define IDC_LIST_FUNCLIST                (IDD_FUNCLIST_PANEL + 1)
#define IDC_LIST_FUNCLIST_AUX            (IDD_FUNCLIST_PANEL + 2)
#define IDC_SEARCHFIELD_FUNCLIST         (IDD_FUNCLIST_PANEL + 3)
#define IDC_RELOADBUTTON_FUNCLIST        (IDD_FUNCLIST_PANEL + 4)
#define IDC_SORTBUTTON_FUNCLIST          (IDD_FUNCLIST_PANEL + 5)
//
//#include "fileBrowser_rc.h"
#define IDD_FILEBROWSER         3500
#define IDD_FILEBROWSER_MENU            (IDD_FILEBROWSER + 10)
  #define IDM_FILEBROWSER_REMOVEROOTFOLDER (IDD_FILEBROWSER_MENU + 1)
  #define IDM_FILEBROWSER_REMOVEALLROOTS   (IDD_FILEBROWSER_MENU + 2)
  #define IDM_FILEBROWSER_ADDROOT          (IDD_FILEBROWSER_MENU + 3)
  #define IDM_FILEBROWSER_SHELLEXECUTE     (IDD_FILEBROWSER_MENU + 4)
  #define IDM_FILEBROWSER_OPENINNPP        (IDD_FILEBROWSER_MENU + 5)
  #define IDM_FILEBROWSER_COPYPATH         (IDD_FILEBROWSER_MENU + 6)
  #define IDM_FILEBROWSER_COPYFILENAME     (IDD_FILEBROWSER_MENU + 10)
  #define IDM_FILEBROWSER_FINDINFILES      (IDD_FILEBROWSER_MENU + 7)
  #define IDM_FILEBROWSER_EXPLORERHERE     (IDD_FILEBROWSER_MENU + 8)
  #define IDM_FILEBROWSER_CMDHERE          (IDD_FILEBROWSER_MENU + 9)
#define IDD_FILEBROWSER_CTRL            (IDD_FILEBROWSER + 30)
  #define       ID_FILEBROWSERTREEVIEW    (IDD_FILEBROWSER_CTRL + 1)
//
//#include "findCharsInRange_rc.h"
#define	IDD_FINDCHARACTERS		2900
#define	IDC_NONASCCI_RADIO    (IDD_FINDCHARACTERS + 1)
#define	IDC_ASCCI_RADIO    (IDD_FINDCHARACTERS + 2)
#define	IDC_MYRANGE_RADIO   (IDD_FINDCHARACTERS + 3)
#define	IDC_RANGESTART_EDIT   (IDD_FINDCHARACTERS + 4)
#define	IDC_RANGEEND_EDIT   (IDD_FINDCHARACTERS + 5)
#define	ID_FINDCHAR_DIRUP    (IDD_FINDCHARACTERS + 6)
#define	ID_FINDCHAR_DIRDOWN    (IDD_FINDCHARACTERS + 7)
#define	IDC_FINDCHAR_DIR_STATIC    (IDD_FINDCHARACTERS + 8)
#define	ID_FINDCHAR_WRAP           (IDD_FINDCHARACTERS + 9)
#define	ID_FINDCHAR_NEXT           (IDD_FINDCHARACTERS + 10)
//
//#include "documentMap_rc.h"
#define	IDD_DOCUMENTMAP      3300
#define	IDD_VIEWZONE_CLASSIC 3320
#define	IDD_VIEWZONE         3321
#define	IDC_VIEWZONECANVAS    (IDD_VIEWZONE + 1)
//
//#include "documentSnapshot_rc.h"
#define	IDD_DOCUMENTSNAPSHOT  3600
//
//#include "pluginsAdminRes.h"
#define IDD_PLUGINSADMIN_DLG    5500
#define IDC_PLUGINADM_SEARCH_STATIC (IDD_PLUGINSADMIN_DLG + 1)
#define IDC_PLUGINADM_SEARCH_EDIT (IDD_PLUGINSADMIN_DLG + 2)
#define IDC_PLUGINADM_INSTALL (IDD_PLUGINSADMIN_DLG + 3)
#define IDC_PLUGINADM_UPDATE (IDD_PLUGINSADMIN_DLG + 4)
#define IDC_PLUGINADM_REMOVE (IDD_PLUGINSADMIN_DLG + 5)
#define IDC_PLUGINADM_LISTVIEW (IDD_PLUGINSADMIN_DLG + 6)
#define IDC_PLUGINADM_EDIT (IDD_PLUGINSADMIN_DLG + 7)
#define IDC_PLUGINADM_RESEARCH_NEXT (IDD_PLUGINSADMIN_DLG + 8)
#define IDC_PLUGINADM_SETTINGS_BUTTON (IDD_PLUGINSADMIN_DLG + 9)
//
//#include "dockingResource.h"
#define DM_NOFOCUSWHILECLICKINGCAPTION TEXT("NOFOCUSWHILECLICKINGCAPTION")

#define IDD_PLUGIN_DLG                  103
#define IDC_EDIT1                       1000

#define IDB_CLOSE_DOWN                  137
#define IDB_CLOSE_UP                    138
#define IDD_CONTAINER_DLG               139

#define IDC_TAB_CONT                    1027
#define IDC_CLIENT_TAB                  1028
#define IDC_BTN_CAPTION                 1050

#define DMM_MSG                         0x5000
    #define DMM_CLOSE                   (DMM_MSG + 1)
    #define DMM_DOCK                    (DMM_MSG + 2)
    #define DMM_FLOAT                   (DMM_MSG + 3)
    #define DMM_DOCKALL                 (DMM_MSG + 4)
    #define DMM_FLOATALL                (DMM_MSG + 5)
    #define DMM_MOVE                    (DMM_MSG + 6)
    #define DMM_UPDATEDISPINFO          (DMM_MSG + 7)
    #define DMM_GETIMAGELIST                    (DMM_MSG + 8)
    #define DMM_GETICONPOS              (DMM_MSG + 9)
    #define DMM_DROPDATA                                (DMM_MSG + 10)
    #define DMM_MOVE_SPLITTER               (DMM_MSG + 11)
	#define DMM_CANCEL_MOVE                         (DMM_MSG + 12)
	#define DMM_LBUTTONUP                           (DMM_MSG + 13)

#define DMN_FIRST 1050
	#define DMN_CLOSE                                       (DMN_FIRST + 1)
//nmhdr.code = DWORD(DMN_CLOSE, 0));
//nmhdr.hwndFrom = hwndNpp;
//nmhdr.idFrom = ctrlIdNpp;

	#define DMN_DOCK                            (DMN_FIRST + 2)
    #define DMN_FLOAT                                   (DMN_FIRST + 3)
//nmhdr.code = DWORD(DMN_XXX, int newContainer);
//nmhdr.hwndFrom = hwndNpp;
//nmhdr.idFrom = ctrlIdNpp;

	#define DMN_SWITCHIN                (DMN_FIRST + 4)
	#define DMN_SWITCHOFF               (DMN_FIRST + 5)
	#define DMN_FLOATDROPPED                        (DMN_FIRST + 6)
//nmhdr.code = DWORD(DMN_XXX, 0);
//nmhdr.hwndFrom = DockingCont::_hself;
//nmhdr.idFrom = 0;
//
//#include "ColourPopupResource.h"
#define	IDD_COLOUR_POPUP  2100
#define	IDC_COLOUR_LIST   (IDD_COLOUR_POPUP + 1)
//
//#include "clipboardHistoryPanel_rc.h"
#define IDD_CLIPBOARDHISTORY_PANEL              2800
#define IDC_LIST_CLIPBOARD    (IDD_CLIPBOARDHISTORY_PANEL + 1)
//
//#include "ansiCharPanel_rc.h"
#define	IDD_ANSIASCII_PANEL		2700
#define	IDC_LIST_ANSICHAR    (IDD_ANSIASCII_PANEL + 1)
//
//#include "preference_rc.h"
	#define	PREF_MSG_ISCHECKED_GENERALPAGE (WM_USER + 1) // wParam:checkbox/radiobutton ID in General page. lParam is type of "bool *" to get result
	#define	PREF_MSG_SETTOOLICONSFROMSTDTOSMALL (WM_USER + 2) 
	#define	PREF_MSG_DISABLETABBARALTERNATEICONS (WM_USER + 3) 

#define	IDD_PREFERENCE_BOX 6000
	#define	IDC_BUTTON_CLOSE (IDD_PREFERENCE_BOX + 1)
	#define	IDC_LIST_DLGTITLE (IDD_PREFERENCE_BOX + 2)

#define	IDD_PREFERENCE_SUB_GENRAL 6100 //(IDD_PREFERENCE_BOX + 100)
	#define	IDC_TOOLBAR_GB_STATIC (IDD_PREFERENCE_SUB_GENRAL + 1)
	#define	IDC_CHECK_HIDE (IDD_PREFERENCE_SUB_GENRAL + 2)
	#define	IDC_RADIO_SMALLICON (IDD_PREFERENCE_SUB_GENRAL + 3)
	#define	IDC_RADIO_BIGICON (IDD_PREFERENCE_SUB_GENRAL + 4)
	#define	IDC_RADIO_STANDARD (IDD_PREFERENCE_SUB_GENRAL + 5)

	#define	IDC_TABBAR_GB_STATIC (IDD_PREFERENCE_SUB_GENRAL + 6)
	#define	IDC_CHECK_REDUCE (IDD_PREFERENCE_SUB_GENRAL + 7)
	#define	IDC_CHECK_LOCK (IDD_PREFERENCE_SUB_GENRAL + 8)
	#define	IDC_CHECK_DRAWINACTIVE (IDD_PREFERENCE_SUB_GENRAL + 9)
	#define	IDC_CHECK_ORANGE (IDD_PREFERENCE_SUB_GENRAL + 10)
	#define	IDC_CHECK_SHOWSTATUSBAR (IDD_PREFERENCE_SUB_GENRAL + 11)
	#define	IDC_CHECK_ENABLETABCLOSE (IDD_PREFERENCE_SUB_GENRAL + 12)
	#define	IDC_CHECK_DBCLICK2CLOSE (IDD_PREFERENCE_SUB_GENRAL + 13)
	#define	IDC_CHECK_ENABLEDOCSWITCHER (IDD_PREFERENCE_SUB_GENRAL + 14)
	#define	IDC_CHECK_MAINTAININDENT (IDD_PREFERENCE_SUB_GENRAL + 15)
	#define	IDC_CHECK_KEEPINSAMEDIR (IDD_PREFERENCE_SUB_GENRAL + 16)
	#define	IDC_CHECK_STYLEMRU (IDD_PREFERENCE_SUB_GENRAL + 17)
	#define	IDC_CHECK_TAB_HIDE (IDD_PREFERENCE_SUB_GENRAL + 18)
	#define	IDC_CHECK_TAB_MULTILINE (IDD_PREFERENCE_SUB_GENRAL + 19)
	#define	IDC_CHECK_TAB_VERTICAL (IDD_PREFERENCE_SUB_GENRAL + 20)
	#define	IDC_CHECK_TAB_LAST_EXIT (IDD_PREFERENCE_SUB_GENRAL + 21)
	#define	IDC_CHECK_HIDEMENUBAR (IDD_PREFERENCE_SUB_GENRAL + 22)
	#define	IDC_LOCALIZATION_GB_STATIC (IDD_PREFERENCE_SUB_GENRAL + 23)
	#define	IDC_COMBO_LOCALIZATION (IDD_PREFERENCE_SUB_GENRAL + 24)
	#define IDC_CHECK_TAB_ALTICONS  (IDD_PREFERENCE_SUB_GENRAL + 28)
	#define IDC_RADIO_SMALLICON2  (IDD_PREFERENCE_SUB_GENRAL + 29)
	#define IDC_RADIO_BIGICON2  (IDD_PREFERENCE_SUB_GENRAL + 30)

#define	IDD_PREFERENCE_SUB_MULTIINSTANCE 6150 //(IDD_PREFERENCE_BOX + 150)
	#define	IDC_MULTIINST_GB_STATIC (IDD_PREFERENCE_SUB_MULTIINSTANCE + 1)
	#define	IDC_SESSIONININST_RADIO (IDD_PREFERENCE_SUB_MULTIINSTANCE + 2)
	#define	IDC_MULTIINST_RADIO (IDD_PREFERENCE_SUB_MULTIINSTANCE + 3)
	#define	IDC_MONOINST_RADIO (IDD_PREFERENCE_SUB_MULTIINSTANCE + 4)
	#define	IDD_STATIC_RESTARTNOTE (IDD_PREFERENCE_SUB_MULTIINSTANCE + 5)

#define	IDD_PREFERENCE_WORDCHARLIST_BOX 6160 //(IDD_PREFERENCE_BOX + 160)
	#define	IDC_WORDCHARLIST_GB_STATIC (IDD_PREFERENCE_WORDCHARLIST_BOX + 1)
	#define	IDC_RADIO_WORDCHAR_DEFAULT (IDD_PREFERENCE_WORDCHARLIST_BOX + 2)
	#define	IDC_RADIO_WORDCHAR_CUSTOM (IDD_PREFERENCE_WORDCHARLIST_BOX + 3)
	#define	IDC_WORDCHAR_CUSTOM_EDIT (IDD_PREFERENCE_WORDCHARLIST_BOX + 4)
	#define	IDD_WORDCHAR_QUESTION_BUTTON (IDD_PREFERENCE_WORDCHARLIST_BOX + 5)
	#define	IDD_STATIC_WORDCHAR_WARNING (IDD_PREFERENCE_WORDCHARLIST_BOX + 6)

#define	IDD_PREFERENCE_SUB_DATETIMEFORMAT 6170 //(IDD_PREFERENCE_BOX + 170)
	#define	IDC_DATETIMEFORMAT_GB_STATIC           (IDD_PREFERENCE_SUB_DATETIMEFORMAT + 1)
	#define	IDD_DATETIMEFORMAT_STATIC              (IDD_PREFERENCE_SUB_DATETIMEFORMAT + 2)
	#define	IDC_DATETIMEFORMAT_EDIT                (IDD_PREFERENCE_SUB_DATETIMEFORMAT + 3)
	#define	IDD_DATETIMEFORMAT_RESULT_STATIC       (IDD_PREFERENCE_SUB_DATETIMEFORMAT + 4)
	#define	IDD_DATETIMEFORMAT_REVERSEORDER_CHECK  (IDD_PREFERENCE_SUB_DATETIMEFORMAT + 5)

#define	IDD_PREFERENCE_SUB_EDITING 6200 //(IDD_PREFERENCE_BOX + 200)
	#define	IDC_FMS_GB_STATIC (IDD_PREFERENCE_SUB_EDITING + 1)
	#define	IDC_RADIO_SIMPLE (IDD_PREFERENCE_SUB_EDITING + 2)
	#define	IDC_RADIO_ARROW (IDD_PREFERENCE_SUB_EDITING + 3)
	#define	IDC_RADIO_CIRCLE (IDD_PREFERENCE_SUB_EDITING + 4)
	#define	IDC_RADIO_BOX (IDD_PREFERENCE_SUB_EDITING + 5)

	#define	IDC_CHECK_LINENUMBERMARGE (IDD_PREFERENCE_SUB_EDITING + 6)
	#define	IDC_CHECK_BOOKMARKMARGE (IDD_PREFERENCE_SUB_EDITING + 7)

	#define	IDC_PADDING_STATIC (IDD_PREFERENCE_SUB_EDITING + 8)
	#define	IDC_PADDINGLEFT_STATIC (IDD_PREFERENCE_SUB_EDITING + 9)
	#define	IDC_PADDINGRIGHT_STATIC (IDD_PREFERENCE_SUB_EDITING + 10)

	#define	IDC_VES_GB_STATIC (IDD_PREFERENCE_SUB_EDITING + 11)
	#define	IDC_DISTRACTIONFREE_STATIC (IDD_PREFERENCE_SUB_EDITING + 12)
	#define	IDC_CHECK_EDGEBGMODE (IDD_PREFERENCE_SUB_EDITING + 13)
	#define	IDC_CHECK_CURRENTLINEHILITE (IDD_PREFERENCE_SUB_EDITING + 14)
	#define	IDC_CHECK_SMOOTHFONT (IDD_PREFERENCE_SUB_EDITING + 15)

	#define	IDC_CARETSETTING_STATIC (IDD_PREFERENCE_SUB_EDITING + 16)
	#define	IDC_WIDTH_STATIC (IDD_PREFERENCE_SUB_EDITING + 17)
	#define	IDC_WIDTH_COMBO (IDD_PREFERENCE_SUB_EDITING + 18)
	#define	IDC_BLINKRATE_STATIC (IDD_PREFERENCE_SUB_EDITING + 19)
	#define	IDC_CARETBLINKRATE_SLIDER (IDD_PREFERENCE_SUB_EDITING + 20)
	#define	IDC_CARETBLINKRATE_F_STATIC (IDD_PREFERENCE_SUB_EDITING + 21)
	#define	IDC_CARETBLINKRATE_S_STATIC (IDD_PREFERENCE_SUB_EDITING + 22)
	#define	IDC_CHECK_DOCCHANGESTATEMARGE (IDD_PREFERENCE_SUB_EDITING + 23)
	#define	IDC_DISTRACTIONFREE_SLIDER (IDD_PREFERENCE_SUB_EDITING + 24)
	#define	IDC_CHECK_MULTISELECTION (IDD_PREFERENCE_SUB_EDITING + 25)

	#define	IDC_RADIO_FOLDMARGENONE (IDD_PREFERENCE_SUB_EDITING + 26)

	#define	IDC_LW_GB_STATIC (IDD_PREFERENCE_SUB_EDITING + 27)
	#define	IDC_RADIO_LWDEF (IDD_PREFERENCE_SUB_EDITING + 28)
	#define	IDC_RADIO_LWALIGN (IDD_PREFERENCE_SUB_EDITING + 29)
	#define	IDC_RADIO_LWINDENT (IDD_PREFERENCE_SUB_EDITING + 30)
	
	#define	IDC_BORDERWIDTH_STATIC (IDD_PREFERENCE_SUB_EDITING + 31)
	#define	IDC_BORDERWIDTHVAL_STATIC (IDD_PREFERENCE_SUB_EDITING + 32)
	#define	IDC_BORDERWIDTH_SLIDER (IDD_PREFERENCE_SUB_EDITING + 33)
	#define	IDC_CHECK_DISABLEADVANCEDSCROLL (IDD_PREFERENCE_SUB_EDITING + 34)
	#define	IDC_CHECK_NOEDGE (IDD_PREFERENCE_SUB_EDITING + 35)
	#define	IDC_CHECK_SCROLLBEYONDLASTLINE (IDD_PREFERENCE_SUB_EDITING + 36)

	#define	IDC_STATIC_MULTILNMODE_TIP (IDD_PREFERENCE_SUB_EDITING + 37)
	#define	IDC_COLUMNPOS_EDIT (IDD_PREFERENCE_SUB_EDITING + 38)
	#define	IDC_CHECK_RIGHTCLICKKEEPSSELECTION (IDD_PREFERENCE_SUB_EDITING + 39)
	#define	IDC_PADDINGLEFT_SLIDER (IDD_PREFERENCE_SUB_EDITING + 40)
	#define	IDC_PADDINGRIGHT_SLIDER (IDD_PREFERENCE_SUB_EDITING + 41)
	#define	IDC_PADDINGLEFTVAL_STATIC (IDD_PREFERENCE_SUB_EDITING + 42)
	#define	IDC_PADDINGRIGHTVAL_STATIC (IDD_PREFERENCE_SUB_EDITING + 43)
	#define	IDC_DISTRACTIONFREEVAL_STATIC (IDD_PREFERENCE_SUB_EDITING + 44)

#define	IDD_PREFERENCE_SUB_DELIMITER 6250 //(IDD_PREFERENCE_BOX + 250)
	#define	IDC_DELIMITERSETTINGS_GB_STATIC (IDD_PREFERENCE_SUB_DELIMITER + 1)
	#define	IDD_STATIC_OPENDELIMITER (IDD_PREFERENCE_SUB_DELIMITER + 2)
	#define	IDC_EDIT_OPENDELIMITER (IDD_PREFERENCE_SUB_DELIMITER + 3)
	#define	IDC_EDIT_CLOSEDELIMITER (IDD_PREFERENCE_SUB_DELIMITER + 4)
	#define	IDD_STATIC_CLOSEDELIMITER (IDD_PREFERENCE_SUB_DELIMITER + 5)
	#define	IDD_SEVERALLINEMODEON_CHECK (IDD_PREFERENCE_SUB_DELIMITER + 6)
	#define	IDD_STATIC_BLABLA (IDD_PREFERENCE_SUB_DELIMITER + 7)
	#define	IDD_STATIC_BLABLA2NDLINE (IDD_PREFERENCE_SUB_DELIMITER + 8)

#define	IDD_PREFERENCE_SUB_CLOUD_LINK 6260 //(IDD_PREFERENCE_BOX + 250)
	#define	IDC_SETTINGSONCLOUD_WARNING_STATIC (IDD_PREFERENCE_SUB_CLOUD_LINK + 1)
	#define	IDC_SETTINGSONCLOUD_GB_STATIC (IDD_PREFERENCE_SUB_CLOUD_LINK + 2)
	#define	IDC_NOCLOUD_RADIO (IDD_PREFERENCE_SUB_CLOUD_LINK + 3)
	#define	IDC_URISCHEMES_STATIC (IDD_PREFERENCE_SUB_CLOUD_LINK + 4)
	#define	IDC_URISCHEMES_EDIT (IDD_PREFERENCE_SUB_CLOUD_LINK + 5)
	//#define	IDC_GOOGLEDRIVE_RADIO (IDD_PREFERENCE_SUB_CLOUD_LINK + 6)
	#define	IDC_WITHCLOUD_RADIO (IDD_PREFERENCE_SUB_CLOUD_LINK + 7)
	#define	IDC_CLOUDPATH_EDIT (IDD_PREFERENCE_SUB_CLOUD_LINK + 8)
	#define	IDD_CLOUDPATH_BROWSE_BUTTON (IDD_PREFERENCE_SUB_CLOUD_LINK + 9)

#define	IDD_PREFERENCE_SUB_SEARCHENGINE 6270 //(IDD_PREFERENCE_BOX + 250)
	#define	IDC_SEARCHENGINES_GB_STATIC (IDD_PREFERENCE_SUB_SEARCHENGINE + 1)
	#define	IDC_SEARCHENGINE_DUCKDUCKGO_RADIO (IDD_PREFERENCE_SUB_SEARCHENGINE + 2)
	#define	IDC_SEARCHENGINE_GOOGLE_RADIO (IDD_PREFERENCE_SUB_SEARCHENGINE + 3)
	//#define	IDC_SEARCHENGINE_BING_RADIO (IDD_PREFERENCE_SUB_SEARCHENGINE + 4)
	#define	IDC_SEARCHENGINE_YAHOO_RADIO (IDD_PREFERENCE_SUB_SEARCHENGINE + 5)
	#define	IDC_SEARCHENGINE_CUSTOM_RADIO (IDD_PREFERENCE_SUB_SEARCHENGINE + 6)
	#define	IDC_SEARCHENGINE_EDIT (IDD_PREFERENCE_SUB_SEARCHENGINE + 7)
	#define	IDD_SEARCHENGINE_NOTE_STATIC (IDD_PREFERENCE_SUB_SEARCHENGINE + 8)
	#define	IDC_SEARCHENGINE_STACKOVERFLOW_RADIO (IDD_PREFERENCE_SUB_SEARCHENGINE + 9)

#define	IDD_PREFERENCE_SUB_MARGING_BORDER_EDGE 6290 //(IDD_PREFERENCE_BOX + 290)
	#define	IDC_LINENUMBERMARGE_GB_STATIC (IDD_PREFERENCE_SUB_MARGING_BORDER_EDGE + 1)
	#define	IDC_RADIO_DYNAMIC (IDD_PREFERENCE_SUB_MARGING_BORDER_EDGE + 2)
	#define	IDC_RADIO_CONSTANT (IDD_PREFERENCE_SUB_MARGING_BORDER_EDGE + 3)

#define	IDD_PREFERENCE_SUB_MISC 6300 //(IDD_PREFERENCE_BOX + 300)
	#define	IDC_TABSETTING_GB_STATIC (IDD_PREFERENCE_SUB_MISC + 1)
	#define	IDC_CHECK_REPLACEBYSPACE (IDD_PREFERENCE_SUB_MISC + 2)
	#define	IDC_TABSIZE_STATIC (IDD_PREFERENCE_SUB_MISC + 3)
	#define	IDC_HISTORY_GB_STATIC (IDD_PREFERENCE_SUB_MISC + 4)
	#define	IDC_CHECK_DONTCHECKHISTORY (IDD_PREFERENCE_SUB_MISC + 5)
	#define	IDC_MAXNBFILE_STATIC (IDD_PREFERENCE_SUB_MISC + 6)
	#define	IDC_CHECK_MIN2SYSTRAY (IDD_PREFERENCE_SUB_MISC + 8)
	#define	IDC_CHECK_REMEMBERSESSION (IDD_PREFERENCE_SUB_MISC + 9)
	#define	IDC_TABSIZEVAL_STATIC (IDD_PREFERENCE_SUB_MISC + 10)
	#define	IDC_MAXNBFILEVAL_STATIC (IDD_PREFERENCE_SUB_MISC + 11)
	#define	IDC_FILEAUTODETECTION_STATIC (IDD_PREFERENCE_SUB_MISC + 12)
	#define	IDC_CHECK_UPDATESILENTLY (IDD_PREFERENCE_SUB_MISC + 13)
	#define	IDC_RADIO_BKNONE (IDD_PREFERENCE_SUB_MISC + 15)
	#define	IDC_RADIO_BKSIMPLE (IDD_PREFERENCE_SUB_MISC + 16)
	#define	IDC_RADIO_BKVERBOSE (IDD_PREFERENCE_SUB_MISC + 17)
	#define	IDC_CLICKABLELINK_STATIC (IDD_PREFERENCE_SUB_MISC + 18)
	#define	IDC_CHECK_CLICKABLELINK_ENABLE (IDD_PREFERENCE_SUB_MISC + 19)
	#define	IDC_CHECK_CLICKABLELINK_NOUNDERLINE (IDD_PREFERENCE_SUB_MISC + 20)
	#define	IDC_EDIT_SESSIONFILEEXT (IDD_PREFERENCE_SUB_MISC + 21)
	#define	IDC_SESSIONFILEEXT_STATIC (IDD_PREFERENCE_SUB_MISC + 22)
	#define	IDC_CHECK_AUTOUPDATE (IDD_PREFERENCE_SUB_MISC + 23)
	#define	IDC_DOCUMENTSWITCHER_STATIC (IDD_PREFERENCE_SUB_MISC + 24)
	#define	IDC_CHECK_UPDATEGOTOEOF (IDD_PREFERENCE_SUB_MISC + 25)
	#define	IDC_CHECK_ENABLSMARTHILITE (IDD_PREFERENCE_SUB_MISC + 26)
	#define	IDC_CHECK_ENABLTAGSMATCHHILITE (IDD_PREFERENCE_SUB_MISC + 27)
	#define	IDC_CHECK_ENABLTAGATTRHILITE (IDD_PREFERENCE_SUB_MISC + 28)
	#define	IDC_TAGMATCHEDHILITE_STATIC (IDD_PREFERENCE_SUB_MISC + 29)
	#define	IDC_CHECK_HIGHLITENONEHTMLZONE (IDD_PREFERENCE_SUB_MISC + 30)
	#define	IDC_CHECK_SHORTTITLE (IDD_PREFERENCE_SUB_MISC + 31)
	#define IDC_CHECK_SMARTHILITECASESENSITIVE (IDD_PREFERENCE_SUB_MISC + 32)
	#define IDC_SMARTHILITING_STATIC (IDD_PREFERENCE_SUB_MISC + 33)
	#define	IDC_CHECK_DETECTENCODING (IDD_PREFERENCE_SUB_MISC + 34)
	#define IDC_CHECK_BACKSLASHISESCAPECHARACTERFORSQL (IDD_PREFERENCE_SUB_MISC + 35)
	#define	IDC_EDIT_WORKSPACEFILEEXT (IDD_PREFERENCE_SUB_MISC + 36)
	#define	IDC_WORKSPACEFILEEXT_STATIC (IDD_PREFERENCE_SUB_MISC + 37)
	#define IDC_CHECK_SMARTHILITEWHOLEWORDONLY (IDD_PREFERENCE_SUB_MISC + 38)
	#define IDC_CHECK_SMARTHILITEUSEFINDSETTINGS (IDD_PREFERENCE_SUB_MISC + 39)
	#define IDC_CHECK_SMARTHILITEANOTHERRVIEW (IDD_PREFERENCE_SUB_MISC + 40)

	//-- xFileEditViewHistoryParameterGUI: Additional Checkbox for enabling the history for restoring the edit view per file.
	#define	IDC_CHECK_REMEMBEREDITVIEWPERFILE (IDD_PREFERENCE_SUB_MISC + 41)
	#define	IDC_REMEMBEREDITVIEWPERFILE_STATIC (IDD_PREFERENCE_SUB_MISC + 42)
	#define	IDC_EDIT_REMEMBEREDITVIEWPERFILE (IDD_PREFERENCE_SUB_MISC + 43)

	#define	IDC_DOCUMENTPEEK_STATIC (IDD_PREFERENCE_SUB_MISC + 44)
	#define	IDC_CHECK_ENABLEDOCPEEKER (IDD_PREFERENCE_SUB_MISC + 45)
	#define	IDC_CHECK_ENABLEDOCPEEKONMAP (IDD_PREFERENCE_SUB_MISC + 46)
	#define	IDC_COMBO_FILEUPDATECHOICE (IDD_PREFERENCE_SUB_MISC + 47)
	#define	IDC_CHECK_DIRECTWRITE_ENABLE (IDD_PREFERENCE_SUB_MISC + 49)
	#define	IDC_CHECK_CLICKABLELINK_FULLBOXMODE (IDD_PREFERENCE_SUB_MISC + 50)
	#define	IDC_MARKALL_STATIC (IDD_PREFERENCE_SUB_MISC + 51)
	#define	IDC_CHECK_MARKALLCASESENSITIVE (IDD_PREFERENCE_SUB_MISC + 52)
	#define	IDC_CHECK_MARKALLWHOLEWORDONLY (IDD_PREFERENCE_SUB_MISC + 53)
	#define	IDC_SMARTHILITEMATCHING_STATIC (IDD_PREFERENCE_SUB_MISC + 54)
	#define	IDC_CHECK_MUTE_SOUNDS (IDD_PREFERENCE_SUB_MISC + 60)
	#define	IDC_CHECK_SAVEALLCONFIRM (IDD_PREFERENCE_SUB_MISC + 61)

#define	IDD_PREFERENCE_SUB_NEWDOCUMENT 6400 //(IDD_PREFERENCE_BOX + 400)
	#define	IDC_FORMAT_GB_STATIC (IDD_PREFERENCE_SUB_NEWDOCUMENT + 1)
	#define	IDC_RADIO_F_WIN (IDD_PREFERENCE_SUB_NEWDOCUMENT + 2)
	#define	IDC_RADIO_F_UNIX (IDD_PREFERENCE_SUB_NEWDOCUMENT + 3)
	#define	IDC_RADIO_F_MAC (IDD_PREFERENCE_SUB_NEWDOCUMENT + 4)
	#define	IDC_ENCODING_STATIC (IDD_PREFERENCE_SUB_NEWDOCUMENT + 5)
	#define	IDC_RADIO_ANSI (IDD_PREFERENCE_SUB_NEWDOCUMENT + 6)
	#define	IDC_RADIO_UTF8SANSBOM (IDD_PREFERENCE_SUB_NEWDOCUMENT + 7)
	#define	IDC_RADIO_UTF8 (IDD_PREFERENCE_SUB_NEWDOCUMENT + 8)
	#define	IDC_RADIO_UTF16BIG (IDD_PREFERENCE_SUB_NEWDOCUMENT + 9)
	#define	IDC_RADIO_UTF16SMALL (IDD_PREFERENCE_SUB_NEWDOCUMENT + 10)
	#define	IDC_DEFAULTLANG_STATIC (IDD_PREFERENCE_SUB_NEWDOCUMENT + 11)
	#define	IDC_COMBO_DEFAULTLANG (IDD_PREFERENCE_SUB_NEWDOCUMENT + 12)
	#define	IDC_OPENSAVEDIR_GR_STATIC (IDD_PREFERENCE_SUB_NEWDOCUMENT + 13)
	#define	IDC_OPENSAVEDIR_FOLLOWCURRENT_RADIO (IDD_PREFERENCE_SUB_NEWDOCUMENT + 14)
	#define	IDC_OPENSAVEDIR_REMEMBERLAST_RADIO (IDD_PREFERENCE_SUB_NEWDOCUMENT + 15)
	#define	IDC_OPENSAVEDIR_ALWAYSON_RADIO (IDD_PREFERENCE_SUB_NEWDOCUMENT + 16)
	#define	IDC_OPENSAVEDIR_ALWAYSON_EDIT (IDD_PREFERENCE_SUB_NEWDOCUMENT + 17)
	#define	IDD_OPENSAVEDIR_ALWAYSON_BROWSE_BUTTON (IDD_PREFERENCE_SUB_NEWDOCUMENT + 18)
	#define	IDC_NEWDOCUMENT_GR_STATIC (IDD_PREFERENCE_SUB_NEWDOCUMENT + 19)
	#define	IDC_CHECK_OPENANSIASUTF8 (IDD_PREFERENCE_SUB_NEWDOCUMENT + 20)
	#define	IDC_RADIO_OTHERCP (IDD_PREFERENCE_SUB_NEWDOCUMENT + 21)
	#define	IDC_COMBO_OTHERCP (IDD_PREFERENCE_SUB_NEWDOCUMENT + 22)
	#define	IDC_GP_STATIC_RECENTFILES (IDD_PREFERENCE_SUB_NEWDOCUMENT + 23)
	#define	IDC_CHECK_INSUBMENU (IDD_PREFERENCE_SUB_NEWDOCUMENT + 24)
	#define	IDC_RADIO_ONLYFILENAME (IDD_PREFERENCE_SUB_NEWDOCUMENT + 25)
	#define	IDC_RADIO_FULLFILENAMEPATH (IDD_PREFERENCE_SUB_NEWDOCUMENT + 26)
	#define	IDC_RADIO_CUSTOMIZELENTH (IDD_PREFERENCE_SUB_NEWDOCUMENT + 27)
	#define	IDC_CUSTOMIZELENGTHVAL_STATIC (IDD_PREFERENCE_SUB_NEWDOCUMENT + 28)
	#define IDC_DISPLAY_STATIC (IDD_PREFERENCE_SUB_NEWDOCUMENT + 29)
	#define IDC_OPENSAVEDIR_CHECK_DRROPFOLDEROPENFILES (IDD_PREFERENCE_SUB_NEWDOCUMENT + 31)

#define	IDD_PREFERENCE_SUB_DEFAULTDIRECTORY 6450 //(IDD_PREFERENCE_BOX + 400)
#define	IDD_PREFERENCE_SUB_RECENTFILESHISTORY 6460 //(IDD_PREFERENCE_BOX + 400)

#define	IDD_PREFERENCE_SUB_LANGUAGE 6500 //(IDD_PREFERENCE_BOX + 500)
	#define	IDC_LIST_ENABLEDLANG (IDD_PREFERENCE_SUB_LANGUAGE + 1)
	#define	IDC_LIST_DISABLEDLANG (IDD_PREFERENCE_SUB_LANGUAGE + 2)
	#define	IDC_BUTTON_REMOVE (IDD_PREFERENCE_SUB_LANGUAGE + 3)
	#define	IDC_BUTTON_RESTORE (IDD_PREFERENCE_SUB_LANGUAGE + 4)
	#define	IDC_ENABLEDITEMS_STATIC (IDD_PREFERENCE_SUB_LANGUAGE + 5)
	#define	IDC_DISABLEDITEMS_STATIC (IDD_PREFERENCE_SUB_LANGUAGE + 6)
	#define	IDC_CHECK_LANGMENUCOMPACT (IDD_PREFERENCE_SUB_LANGUAGE + 7)
	#define	IDC_CHECK_LANGMENU_GR_STATIC (IDD_PREFERENCE_SUB_LANGUAGE + 8)
	#define	IDC_LIST_TABSETTNG (IDD_PREFERENCE_SUB_LANGUAGE + 9)
	#define	IDC_CHECK_DEFAULTTABVALUE (IDD_PREFERENCE_SUB_LANGUAGE + 10)
	#define	IDC_GR_TABVALUE_STATIC (IDD_PREFERENCE_SUB_LANGUAGE + 11)
	#define	IDC_TABSIZEVAL_DISABLE_STATIC (IDD_PREFERENCE_SUB_LANGUAGE + 12)
#define	IDD_PREFERENCE_SUB_HIGHLIGHTING 6550 //(IDD_PREFERENCE_BOX + 500)

#define	IDD_PREFERENCE_SUB_PRINT 6600 //(IDD_PREFERENCE_BOX + 600)
	#define	IDC_CHECK_PRINTLINENUM	(IDD_PREFERENCE_SUB_PRINT + 1)
	#define	IDC_COLOUROPT_STATIC	(IDD_PREFERENCE_SUB_PRINT + 2)
	#define	IDC_RADIO_WYSIWYG		(IDD_PREFERENCE_SUB_PRINT + 3)
	#define	IDC_RADIO_INVERT		(IDD_PREFERENCE_SUB_PRINT + 4)
	#define	IDC_RADIO_BW			(IDD_PREFERENCE_SUB_PRINT + 5)
	#define	IDC_RADIO_NOBG			(IDD_PREFERENCE_SUB_PRINT + 6)
	#define IDC_MARGESETTINGS_STATIC  (IDD_PREFERENCE_SUB_PRINT + 7)
	#define IDC_EDIT_ML               (IDD_PREFERENCE_SUB_PRINT + 8)
	#define IDC_EDIT_MT               (IDD_PREFERENCE_SUB_PRINT + 9)
	#define IDC_EDIT_MR               (IDD_PREFERENCE_SUB_PRINT + 10)
	#define IDC_EDIT_MB               (IDD_PREFERENCE_SUB_PRINT + 11)
	#define IDC_ML_STATIC             (IDD_PREFERENCE_SUB_PRINT + 12)
	#define IDC_MT_STATIC             (IDD_PREFERENCE_SUB_PRINT + 13)
	#define IDC_MR_STATIC             (IDD_PREFERENCE_SUB_PRINT + 14)
	#define IDC_MB_STATIC             (IDD_PREFERENCE_SUB_PRINT + 15)

#define	IDD_PREFERENCE_PRINT2_BOX 6700 //(IDD_PREFERENCE_BOX + 700)
	#define	IDC_EDIT_HLEFT		(IDD_PREFERENCE_PRINT2_BOX + 1)
	#define	IDC_EDIT_HMIDDLE		(IDD_PREFERENCE_PRINT2_BOX + 2)
	#define	IDC_EDIT_HRIGHT		(IDD_PREFERENCE_PRINT2_BOX + 3)
	#define	IDC_COMBO_HFONTNAME	(IDD_PREFERENCE_PRINT2_BOX + 4)
	#define	IDC_COMBO_HFONTSIZE	(IDD_PREFERENCE_PRINT2_BOX + 5)
	#define	IDC_CHECK_HBOLD	(IDD_PREFERENCE_PRINT2_BOX + 6)
	#define	IDC_CHECK_HITALIC (IDD_PREFERENCE_PRINT2_BOX + 7)
	#define	IDC_HGB_STATIC	(IDD_PREFERENCE_PRINT2_BOX + 8)
	#define	IDC_HL_STATIC	(IDD_PREFERENCE_PRINT2_BOX + 9)
	#define	IDC_HM_STATIC	(IDD_PREFERENCE_PRINT2_BOX + 10)
	#define	IDC_HR_STATIC	(IDD_PREFERENCE_PRINT2_BOX + 11)
	#define	IDC_EDIT_FLEFT	(IDD_PREFERENCE_PRINT2_BOX + 12)
	#define	IDC_EDIT_FMIDDLE	(IDD_PREFERENCE_PRINT2_BOX + 13)
	#define	IDC_EDIT_FRIGHT	(IDD_PREFERENCE_PRINT2_BOX + 14)
	#define	IDC_COMBO_FFONTNAME	(IDD_PREFERENCE_PRINT2_BOX + 15)
	#define	IDC_COMBO_FFONTSIZE	(IDD_PREFERENCE_PRINT2_BOX + 16)
	#define	IDC_CHECK_FBOLD	(IDD_PREFERENCE_PRINT2_BOX + 17)
	#define	IDC_CHECK_FITALIC	(IDD_PREFERENCE_PRINT2_BOX + 18)
	#define	IDC_FGB_STATIC	(IDD_PREFERENCE_PRINT2_BOX + 19)
	#define	IDC_FL_STATIC	(IDD_PREFERENCE_PRINT2_BOX + 20)
	#define	IDC_FM_STATIC	(IDD_PREFERENCE_PRINT2_BOX + 21)
	#define	IDC_FR_STATIC	(IDD_PREFERENCE_PRINT2_BOX + 22)
	#define IDC_BUTTON_ADDVAR  (IDD_PREFERENCE_PRINT2_BOX + 23)
	#define IDC_COMBO_VARLIST  (IDD_PREFERENCE_PRINT2_BOX + 24)
	#define IDC_VAR_STATIC    (IDD_PREFERENCE_PRINT2_BOX + 25)
	#define IDC_VIEWPANEL_STATIC  (IDD_PREFERENCE_PRINT2_BOX + 26)
	#define IDC_WHICHPART_STATIC  (IDD_PREFERENCE_PRINT2_BOX + 27)
	#define IDC_HEADERFPPTER_GR_STATIC  (IDD_PREFERENCE_PRINT2_BOX + 28)

#define	IDD_PREFERENCE_SUB_BACKUP 6800 //(IDD_PREFERENCE_BOX + 800)
	#define IDC_BACKUPDIR_GRP_STATIC  (IDD_PREFERENCE_SUB_BACKUP + 1)
	#define IDC_BACKUPDIR_CHECK  (IDD_PREFERENCE_SUB_BACKUP + 2)
	#define IDD_BACKUPDIR_STATIC  (IDD_PREFERENCE_SUB_BACKUP + 3)
	#define IDC_BACKUPDIR_USERCUSTOMDIR_GRPSTATIC  (IDD_PREFERENCE_SUB_BACKUP + 4)
	#define IDC_BACKUPDIR_EDIT  (IDD_PREFERENCE_SUB_BACKUP + 5)
	#define IDD_BACKUPDIR_BROWSE_BUTTON  (IDD_PREFERENCE_SUB_BACKUP + 6)
	#define IDD_AUTOC_GRPSTATIC  (IDD_PREFERENCE_SUB_BACKUP + 7)
	#define IDD_AUTOC_ENABLECHECK  (IDD_PREFERENCE_SUB_BACKUP + 8)
	#define IDD_AUTOC_FUNCRADIO  (IDD_PREFERENCE_SUB_BACKUP + 9)
	#define IDD_AUTOC_WORDRADIO  (IDD_PREFERENCE_SUB_BACKUP + 10)
	#define IDD_AUTOC_STATIC_FROM  (IDD_PREFERENCE_SUB_BACKUP + 11)
	#define IDD_AUTOC_STATIC_N  (IDD_PREFERENCE_SUB_BACKUP + 12)
	#define IDD_AUTOC_STATIC_CHAR  (IDD_PREFERENCE_SUB_BACKUP + 13)
	#define IDD_AUTOC_STATIC_NOTE  (IDD_PREFERENCE_SUB_BACKUP + 14)
	#define IDD_FUNC_CHECK         (IDD_PREFERENCE_SUB_BACKUP + 15)
	#define IDD_AUTOC_BOTHRADIO  (IDD_PREFERENCE_SUB_BACKUP + 16)
	#define IDC_BACKUPDIR_RESTORESESSION_GRP_STATIC  (IDD_PREFERENCE_SUB_BACKUP + 17)
	#define IDC_BACKUPDIR_RESTORESESSION_CHECK  (IDD_PREFERENCE_SUB_BACKUP + 18)
	#define IDD_BACKUPDIR_RESTORESESSION_STATIC1  (IDD_PREFERENCE_SUB_BACKUP + 19)
	#define IDC_BACKUPDIR_RESTORESESSION_EDIT  (IDD_PREFERENCE_SUB_BACKUP + 20)
	#define IDD_BACKUPDIR_RESTORESESSION_STATIC2  (IDD_PREFERENCE_SUB_BACKUP + 21)
	#define IDD_BACKUPDIR_RESTORESESSION_PATHLABEL_STATIC  (IDD_PREFERENCE_SUB_BACKUP + 22)
	#define IDD_BACKUPDIR_RESTORESESSION_PATH_EDIT  (IDD_PREFERENCE_SUB_BACKUP + 23)
	#define IDD_AUTOC_IGNORENUMBERS  (IDD_PREFERENCE_SUB_BACKUP + 24)

#define	IDD_PREFERENCE_SUB_AUTOCOMPLETION 6850 //(IDD_PREFERENCE_BOX + 850)
	#define IDD_AUTOCINSERT_GRPSTATIC (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 1)
	#define IDD_AUTOCPARENTHESES_CHECK (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 2)
	#define IDD_AUTOCBRACKET_CHECK (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 3)
	#define IDD_AUTOCCURLYBRACKET_CHECK (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 4)
	#define IDD_AUTOC_DOUBLEQUOTESCHECK (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 5)
	#define IDD_AUTOC_QUOTESCHECK (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 6)
	#define IDD_AUTOCTAG_CHECK (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 7)
	#define IDC_MACHEDPAIROPEN_STATIC (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 8)
	#define IDC_MACHEDPAIRCLOSE_STATIC (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 9)
	#define IDC_MACHEDPAIR_STATIC1 (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 10)
	#define IDC_MACHEDPAIROPEN_EDIT1 (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 11)
	#define IDC_MACHEDPAIRCLOSE_EDIT1 (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 12)
	#define IDC_MACHEDPAIR_STATIC2 (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 13)
	#define IDC_MACHEDPAIROPEN_EDIT2 (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 14)
	#define IDC_MACHEDPAIRCLOSE_EDIT2 (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 15)
	#define IDC_MACHEDPAIR_STATIC3 (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 16)
	#define IDC_MACHEDPAIROPEN_EDIT3 (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 17)
	#define IDC_MACHEDPAIRCLOSE_EDIT3 (IDD_PREFERENCE_SUB_AUTOCOMPLETION + 18)

#define IDD_PREFERENCE_SUB_SEARCHING 6900 //(IDD_PREFERENCE_BOX + 900)
	#define IDC_CHECK_STOPFILLINGFINDFIELD (IDD_PREFERENCE_SUB_SEARCHING + 1)
	#define IDC_CHECK_MONOSPACEDFONT_FINDDLG (IDD_PREFERENCE_SUB_SEARCHING + 2)
	#define IDC_CHECK_FINDDLG_ALWAYS_VISIBLE (IDD_PREFERENCE_SUB_SEARCHING + 3)
	#define IDC_CHECK_CONFIRMREPLOPENDOCS (IDD_PREFERENCE_SUB_SEARCHING + 4)
	#define IDC_CHECK_REPLACEANDSTOP (IDD_PREFERENCE_SUB_SEARCHING + 5)

#define IDD_PREFERENCE_SUB_DARKMODE 7100 //(IDD_PREFERENCE_BOX + 1100)
	#define IDC_CHECK_DARKMODE_ENABLE      (IDD_PREFERENCE_SUB_DARKMODE + 1)
	#define IDC_RADIO_DARKMODE_BLACK       (IDD_PREFERENCE_SUB_DARKMODE + 2)
	#define IDC_RADIO_DARKMODE_RED         (IDD_PREFERENCE_SUB_DARKMODE + 3)
	#define IDC_RADIO_DARKMODE_GREEN       (IDD_PREFERENCE_SUB_DARKMODE + 4)
	#define IDC_RADIO_DARKMODE_BLUE        (IDD_PREFERENCE_SUB_DARKMODE + 5)
	//#define IDC_STATIC_DARKMODE_WARNING (IDD_PREFERENCE_SUB_DARKMODE + 6)
	#define IDC_RADIO_DARKMODE_PURPLE      (IDD_PREFERENCE_SUB_DARKMODE + 7)
	#define IDC_RADIO_DARKMODE_CYAN        (IDD_PREFERENCE_SUB_DARKMODE + 8)
	#define IDC_RADIO_DARKMODE_OLIVE       (IDD_PREFERENCE_SUB_DARKMODE + 9)
	#define IDC_RADIO_DARKMODE_CUSTOMIZED  (IDD_PREFERENCE_SUB_DARKMODE + 15)
	#define IDD_CUSTOMIZED_COLOR1_STATIC   (IDD_PREFERENCE_SUB_DARKMODE + 16)
	#define IDD_CUSTOMIZED_COLOR2_STATIC   (IDD_PREFERENCE_SUB_DARKMODE + 17)
	#define IDD_CUSTOMIZED_COLOR3_STATIC   (IDD_PREFERENCE_SUB_DARKMODE + 18)
	#define IDD_CUSTOMIZED_COLOR4_STATIC   (IDD_PREFERENCE_SUB_DARKMODE + 19)
	#define IDD_CUSTOMIZED_COLOR5_STATIC   (IDD_PREFERENCE_SUB_DARKMODE + 20)
	#define IDD_CUSTOMIZED_COLOR6_STATIC   (IDD_PREFERENCE_SUB_DARKMODE + 21)
	#define IDD_CUSTOMIZED_COLOR7_STATIC   (IDD_PREFERENCE_SUB_DARKMODE + 22)
	#define IDD_CUSTOMIZED_COLOR8_STATIC   (IDD_PREFERENCE_SUB_DARKMODE + 23)
	#define IDD_CUSTOMIZED_COLOR9_STATIC   (IDD_PREFERENCE_SUB_DARKMODE + 24)
	#define IDD_CUSTOMIZED_COLOR10_STATIC  (IDD_PREFERENCE_SUB_DARKMODE + 25)
	#define IDD_CUSTOMIZED_RESET_BUTTON    (IDD_PREFERENCE_SUB_DARKMODE + 30)
//
//#include "ProjectPanel_rc.h"
#define IDD_PROJECTPANEL                3100

#define IDD_PROJECTPANEL_MENU           (IDD_PROJECTPANEL + 10)
  #define IDM_PROJECT_RENAME       (IDD_PROJECTPANEL_MENU + 1)
  #define IDM_PROJECT_NEWFOLDER    (IDD_PROJECTPANEL_MENU + 2)
  #define IDM_PROJECT_ADDFILES     (IDD_PROJECTPANEL_MENU + 3)
  #define IDM_PROJECT_DELETEFOLDER (IDD_PROJECTPANEL_MENU + 4)
  #define IDM_PROJECT_DELETEFILE   (IDD_PROJECTPANEL_MENU + 5)
  #define IDM_PROJECT_MODIFYFILEPATH   (IDD_PROJECTPANEL_MENU + 6)
  #define IDM_PROJECT_ADDFILESRECUSIVELY   (IDD_PROJECTPANEL_MENU + 7)
  #define IDM_PROJECT_MOVEUP       (IDD_PROJECTPANEL_MENU + 8)
  #define IDM_PROJECT_MOVEDOWN     (IDD_PROJECTPANEL_MENU + 9)

#define IDD_PROJECTPANEL_MENUWS (IDD_PROJECTPANEL + 20)
  #define IDM_PROJECT_NEWPROJECT    (IDD_PROJECTPANEL_MENUWS + 1)
  #define IDM_PROJECT_NEWWS         (IDD_PROJECTPANEL_MENUWS + 2)
  #define IDM_PROJECT_OPENWS        (IDD_PROJECTPANEL_MENUWS + 3)
  #define IDM_PROJECT_RELOADWS      (IDD_PROJECTPANEL_MENUWS + 4)
  #define IDM_PROJECT_SAVEWS        (IDD_PROJECTPANEL_MENUWS + 5)
  #define IDM_PROJECT_SAVEASWS      (IDD_PROJECTPANEL_MENUWS + 6)
  #define IDM_PROJECT_SAVEACOPYASWS (IDD_PROJECTPANEL_MENUWS + 7)
  #define IDM_PROJECT_FINDINPROJECTSWS (IDD_PROJECTPANEL_MENUWS + 8)

#define IDD_PROJECTPANEL_CTRL           (IDD_PROJECTPANEL + 30)
  #define       ID_PROJECTTREEVIEW    (IDD_PROJECTPANEL_CTRL + 1)
  #define       IDB_PROJECT_BTN       (IDD_PROJECTPANEL_CTRL + 2)
  #define       IDB_EDIT_BTN          (IDD_PROJECTPANEL_CTRL + 3)

#define IDD_FILERELOCALIZER_DIALOG  3200
  #define       IDC_EDIT_FILEFULLPATHNAME  (IDD_FILERELOCALIZER_DIALOG + 1)
//
//#include "regExtDlgRc.h"
#define	IDD_REGEXT_BOX 4000
	#define IDC_REGEXT_LANG_LIST             (IDD_REGEXT_BOX + 1)
	#define IDC_REGEXT_LANGEXT_LIST          (IDD_REGEXT_BOX + 2)
	#define IDC_REGEXT_REGISTEREDEXTS_LIST   (IDD_REGEXT_BOX + 3)
	#define IDC_ADDFROMLANGEXT_BUTTON        (IDD_REGEXT_BOX + 4)
	#define IDI_POUPELLE_ICON                (IDD_REGEXT_BOX + 5)
	#define IDC_CUSTOMEXT_EDIT               (IDD_REGEXT_BOX + 6)
	#define IDC_REMOVEEXT_BUTTON             (IDD_REGEXT_BOX + 7)
	#define IDC_ADMINMUSTBEONMSG_STATIC      (IDD_REGEXT_BOX + 8)
	#define IDC_SUPPORTEDEXTS_STATIC         (IDD_REGEXT_BOX + 9)
	#define IDC_REGISTEREDEXTS_STATIC        (IDD_REGEXT_BOX + 10)
//
//#include "RunDlg_rc.h"
#define	IDD_RUN_DLG      1900
#define	IDC_BUTTON_FILE_BROWSER	(IDD_RUN_DLG + 1)
#define	IDC_COMBO_RUN_PATH		(IDD_RUN_DLG + 2)
#define	IDC_MAINTEXT_STATIC     (IDD_RUN_DLG + 3)
#define	IDC_BUTTON_SAVE         (IDD_RUN_DLG + 4)
//
//#include "RunMacroDlg_rc.h"
#define IDD_RUN_MACRO_DLG					8000
#define IDC_M_RUN_MULTI						8001
#define IDC_M_RUN_EOF						8002
#define IDC_M_RUN_TIMES						8003
#define IDC_MACRO_COMBO						8004
//#define IDC_MACROGROUP_STATIC				8005
#define IDC_TIMES_STATIC					8005
#define IDC_MACRO2RUN_STATIC				8006
//
//#include "shortcutRc.h"
#define IDD_SHORTCUT_DLG      5000

#define IDD_SHORTCUTSCINT_DLG 5001

#define IDC_CTRL_CHECK       (IDD_SHORTCUT_DLG + 1)
#define IDC_ALT_CHECK        (IDD_SHORTCUT_DLG + 2)
#define IDC_SHIFT_CHECK      (IDD_SHORTCUT_DLG + 3)
#define IDC_KEY_COMBO        (IDD_SHORTCUT_DLG + 4)
#define IDC_NAME_EDIT        (IDD_SHORTCUT_DLG + 5)
#define IDC_NAME_STATIC      (IDD_SHORTCUT_DLG + 6)
#define IDC_WARNING_STATIC   (IDD_SHORTCUT_DLG + 7)
#define IDC_BUTTON_ADD       (IDD_SHORTCUT_DLG + 8)
#define IDC_BUTTON_RMVE      (IDD_SHORTCUT_DLG + 9)
#define IDC_BUTTON_APPLY     (IDD_SHORTCUT_DLG + 10)
#define IDC_LIST_KEYS        (IDD_SHORTCUT_DLG + 11)
#define IDC_CONFLICT_STATIC  (IDD_SHORTCUT_DLG + 12)
//
//#include "ShortcutMapper_rc.h"
#define IDD_SHORTCUTMAPPER_DLG      2600
#define IDD_BABYGRID_ID1      (IDD_SHORTCUTMAPPER_DLG + 1)
#define IDM_BABYGRID_MODIFY   (IDD_SHORTCUTMAPPER_DLG + 2)
#define IDM_BABYGRID_DELETE   (IDD_SHORTCUTMAPPER_DLG + 3)
#define IDC_BABYGRID_TABBAR   (IDD_SHORTCUTMAPPER_DLG + 4)
#define IDC_BABYGRID_INFO     (IDD_SHORTCUTMAPPER_DLG + 5)
#define IDM_BABYGRID_CLEAR    (IDD_SHORTCUTMAPPER_DLG + 6)
#define IDC_BABYGRID_STATIC   (IDD_SHORTCUTMAPPER_DLG + 7)
#define IDC_BABYGRID_FILTER   (IDD_SHORTCUTMAPPER_DLG + 8)
//
//#include "TaskListDlg_rc.h"
#define IDD_TASKLIST_DLG    2450
	#define ID_PICKEDUP     (IDD_TASKLIST_DLG + 1)
//
//#include "VerticalFileSwitcher_rc.h"
#define IDD_DOCLIST             3000
#define IDC_LIST_DOCLIST    (IDD_DOCLIST + 1)
//
//#include "WindowsDlgRc.h"
#ifdef __GNUC__
#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif
	#ifndef LVS_OWNERDATA
		#define LVS_OWNERDATA 4096
	#endif
#endif

#define IDD_WINDOWS 7000
	#define IDC_WINDOWS_LIST (IDD_WINDOWS + 1)
	#define IDC_WINDOWS_SAVE (IDD_WINDOWS + 2)
	#define IDC_WINDOWS_CLOSE (IDD_WINDOWS + 3)
	#define IDC_WINDOWS_SORT (IDD_WINDOWS + 4)

#define IDR_WINDOWS_MENU 11000
	#define  IDM_WINDOW_WINDOWS   (IDR_WINDOWS_MENU + 1)
	#define  IDM_WINDOW_MRU_FIRST (IDR_WINDOWS_MENU + 20)
	#define  IDM_WINDOW_MRU_LIMIT (IDR_WINDOWS_MENU + 29)
	#define  IDM_WINDOW_COPY_NAME (IDM_WINDOW_MRU_LIMIT + 1)
	#define  IDM_WINDOW_COPY_PATH (IDM_WINDOW_MRU_LIMIT + 2)
//
//#include "WordStyleDlgRes.h"
#define IDD_STYLER_DLG	2200
    //#define IDC_STYLETYPE_COMBO	(IDD_STYLER_DLG + 1)
    #define IDC_FONT_COMBO		(IDD_STYLER_DLG + 2)
    #define IDC_FONTSIZE_COMBO	(IDD_STYLER_DLG + 3)
    #define IDC_BOLD_CHECK		(IDD_STYLER_DLG + 4)
    #define IDC_ITALIC_CHECK		(IDD_STYLER_DLG + 5)
    #define IDC_FG_STATIC			(IDD_STYLER_DLG + 6)
    #define IDC_BG_STATIC			(IDD_STYLER_DLG + 7)
    #define IDC_FONTNAME_STATIC      (IDD_STYLER_DLG + 8) 
    #define IDC_FONTSIZE_STATIC       (IDD_STYLER_DLG + 9)
    //#define IDC_STYLEDEFAULT_WARNING_STATIC (IDD_STYLER_DLG + 10)  for the sake of compablity of traslation xml files, this number (2210) don't be use anymore by Notepad++
    #define IDC_STYLEDESC_STATIC  (IDD_STYLER_DLG + 11)
    #define IDC_COLOURGROUP_STATIC  (IDD_STYLER_DLG + 12)
    #define IDC_FONTGROUP_STATIC  (IDD_STYLER_DLG + 13)
	
	#define IDC_DEF_EXT_STATIC				(IDD_STYLER_DLG + 14)
    #define IDC_DEF_EXT_EDIT					(IDD_STYLER_DLG + 15)
	#define IDC_USER_EXT_STATIC				(IDD_STYLER_DLG + 16)
	#define IDC_USER_EXT_EDIT					(IDD_STYLER_DLG + 17)
	#define IDC_UNDERLINE_CHECK			(IDD_STYLER_DLG + 18)
	#define IDC_DEF_KEYWORDS_STATIC	(IDD_STYLER_DLG + 19)
	#define IDC_DEF_KEYWORDS_EDIT		(IDD_STYLER_DLG + 20)
	#define IDC_USER_KEYWORDS_STATIC	(IDD_STYLER_DLG + 21)
	#define IDC_USER_KEYWORDS_EDIT		(IDD_STYLER_DLG + 22)
	#define IDC_PLUSSYMBOL_STATIC 		(IDD_STYLER_DLG + 23)
	#define IDC_PLUSSYMBOL2_STATIC 		(IDD_STYLER_DLG + 24)
	#define IDC_LANGDESC_STATIC  (IDD_STYLER_DLG + 25)
	
	#define IDC_GLOBAL_FG_CHECK		(IDD_STYLER_DLG + 26)
	#define IDC_GLOBAL_BG_CHECK		(IDD_STYLER_DLG + 27)
	#define IDC_GLOBAL_FONT_CHECK		(IDD_STYLER_DLG + 28)
	#define IDC_GLOBAL_FONTSIZE_CHECK		(IDD_STYLER_DLG + 29)
	#define IDC_GLOBAL_BOLD_CHECK		(IDD_STYLER_DLG + 30)
	#define IDC_GLOBAL_ITALIC_CHECK		(IDD_STYLER_DLG + 31)
	#define IDC_GLOBAL_UNDERLINE_CHECK		(IDD_STYLER_DLG + 32)
	#define IDC_STYLEDESCRIPTION_STATIC	(IDD_STYLER_DLG + 33)
	                                                    
# define IDD_GLOBAL_STYLER_DLG	2300
    #define IDC_SAVECLOSE_BUTTON	(IDD_GLOBAL_STYLER_DLG + 1)
    #define IDC_SC_PERCENTAGE_SLIDER     (IDD_GLOBAL_STYLER_DLG + 2)
	#define IDC_SC_TRANSPARENT_CHECK    (IDD_GLOBAL_STYLER_DLG + 3)
	#define IDC_LANGUAGES_LIST   (IDD_GLOBAL_STYLER_DLG + 4)
	#define IDC_STYLES_LIST       (IDD_GLOBAL_STYLER_DLG + 5)
	#define IDC_SWITCH2THEME_STATIC       (IDD_GLOBAL_STYLER_DLG + 6)
	#define IDC_SWITCH2THEME_COMBO       (IDD_GLOBAL_STYLER_DLG + 7)
//
//#include "columnEditor_rc.h"
#define IDD_COLUMNEDIT   2020
	#define IDC_COL_INITNUM_EDIT            (IDD_COLUMNEDIT + 1)
	#define IDC_COL_INCREASENUM_EDIT        (IDD_COLUMNEDIT + 2)
	#define IDC_COL_TEXT_RADIO                      (IDD_COLUMNEDIT + 3)
	#define IDC_COL_DEC_RADIO                       (IDD_COLUMNEDIT + 4)
	#define IDC_COL_OCT_RADIO                       (IDD_COLUMNEDIT + 5)
	#define IDC_COL_HEX_RADIO                       (IDD_COLUMNEDIT + 6)
	#define IDC_COL_BIN_RADIO                       (IDD_COLUMNEDIT + 7)
	#define IDC_COL_TEXT_GRP_STATIC         (IDD_COLUMNEDIT + 8)
	#define IDC_COL_NUM_GRP_STATIC          (IDD_COLUMNEDIT + 9)
	#define IDC_COL_INITNUM_STATIC          (IDD_COLUMNEDIT + 10)
	#define IDC_COL_INCRNUM_STATIC          (IDD_COLUMNEDIT + 11)
	#define IDC_COL_FORMAT_GRP_STATIC       (IDD_COLUMNEDIT + 12)
	#define IDC_COL_NUM_RADIO                       (IDD_COLUMNEDIT + 13)
	#define IDC_COL_TEXT_EDIT                       (IDD_COLUMNEDIT + 14)
	#define IDC_COL_LEADZERO_CHECK          (IDD_COLUMNEDIT + 15)
	#define IDC_COL_REPEATNUM_STATIC        (IDD_COLUMNEDIT + 16)
	#define IDC_COL_REPEATNUM_EDIT          (IDD_COLUMNEDIT + 17)
//
//#include "FindReplaceDlg_rc.h"
#define IDD_FIND_REPLACE_DLG                    1600
#define IDFINDWHAT                                              1601
#define IDREPLACEWITH                                   1602
#define IDWHOLEWORD                                             1603
#define IDF_WHOLEWORD   1
#define IDMATCHCASE                                             1604
#define IDF_MATCHCASE   2

#define IDREGEXP                                                1605

#define IDWRAP                                                  1606
#define IDF_WRAP        256
#define IDUNSLASH                                               1607
#define IDREPLACE                                               1608
#define IDREPLACEALL                                    1609
#define ID_STATICTEXT_REPLACE                   1611
//#define	IDDIRECTIONUP					1612
//#define	IDDIRECTIONDOWN                                 1613
#define IDF_WHICH_DIRECTION     512
#define IDCCOUNTALL                                             1614
#define IDCMARKALL                                              1615
#define IDC_MARKLINE_CHECK                              1616
#define IDF_MARKLINE_CHECK      16
//#define	IDC_STYLEFOUND_CHECK			1617
#define IDF_STYLEFOUND_CHECK    8
#define IDC_PURGE_CHECK                                 1618
#define IDF_PURGE_CHECK 4
#define IDC_FINDALL_STATIC                              1619
#define IDFINDWHAT_STATIC                               1620
//#define	IDC_DIR_STATIC					1621

#define IDC_PERCENTAGE_SLIDER                   1622
#define IDC_TRANSPARENT_GRPBOX                  1623

#define IDC_MODE_STATIC                                 1624
#define IDNORMAL                                                1625
#define IDEXTENDED                                              1626

#define IDC_FIND_IN_STATIC                              1628
//#define	IDC_CURRENT_FILE_RADIO		1629
//#define	IDC_OPENED_FILES_RADIO		1630
//#define	IDC_FILES_RADIO				1631
#define IDC_IN_SELECTION_CHECK                  1632
#define IDF_IN_SELECTION_CHECK  128
#define IDC_CLEAR_ALL                           1633
#define IDC_REPLACEINSELECTION                  1634
#define IDC_REPLACE_OPENEDFILES                 1635
#define IDC_FINDALL_OPENEDFILES                 1636
//#define	IDC_FINDINFILES  1637
#define IDC_FINDINFILES_LAUNCH                  1638
#define IDC_GETCURRENTDOCTYPE                   1639
#define IDC_FINDALL_CURRENTFILE                 1641
//#define	IDSWITCH  1640

#define IDD_FINDINFILES_DLG                             1650
#define IDD_FINDINFILES_BROWSE_BUTTON   1651
#define IDD_FINDINFILES_FILTERS_COMBO   1652
#define IDD_FINDINFILES_DIR_COMBO               1653
#define IDD_FINDINFILES_FILTERS_STATIC  1654
#define IDD_FINDINFILES_DIR_STATIC              1655
#define IDD_FINDINFILES_FIND_BUTTON             1656
#define IDD_FINDINFILES_RECURSIVE_CHECK         1658
#define IDF_FINDINFILES_RECURSIVE_CHECK 32
#define IDD_FINDINFILES_INHIDDENDIR_CHECK       1659
#define IDF_FINDINFILES_INHIDDENDIR_CHECK       64
#define IDD_FINDINFILES_REPLACEINFILES  1660
#define IDD_FINDINFILES_FOLDERFOLLOWSDOC_CHECK  1661
#define IDD_FINDINFILES_PROJECT1_CHECK  1662
#define IDF_FINDINFILES_PROJECT1_CHECK  128
#define IDD_FINDINFILES_PROJECT2_CHECK  1663
#define IDF_FINDINFILES_PROJECT2_CHECK  256
#define IDD_FINDINFILES_PROJECT3_CHECK  1664
#define IDF_FINDINFILES_PROJECT3_CHECK  512
#define IDD_FINDINFILES_REPLACEINPROJECTS       1665
#define IDD_FINDINFILES_FINDINPROJECTS  1666

#define IDD_FINDRESULT                                  1670

#define IDD_INCREMENT_FIND                              1680
#define IDC_INCSTATIC                                   1681
#define IDC_INCFINDTEXT                                 1682
#define IDC_INCFINDPREVOK                               1683
#define IDC_INCFINDNXTOK                                1684
#define IDC_INCFINDMATCHCASE                    1685
#define IDC_TRANSPARENT_CHECK                           1686
#define IDC_TRANSPARENT_LOSSFOCUS_RADIO         1687
#define IDC_TRANSPARENT_ALWAYS_RADIO            1688
#define IDC_INCFINDSTATUS                               1689
#define IDC_INCFINDHILITEALL                    1690

#define IDB_INCREMENTAL_BG                              1691

#define IDC_FRCOMMAND_INIT                              1700
#define IDC_FRCOMMAND_EXEC                              1701
#define IDC_FRCOMMAND_BOOLEANS                  1702

#define IDREDOTMATCHNL                                  1703
#define IDF_REDOTMATCHNL        1024

#define IDD_FINDINFINDER_DLG                    1710
#define IDFINDWHAT_STATIC_FIFOLDER              1711
#define IDFINDWHAT_FIFOLDER                             1712
#define IDC_MATCHLINENUM_CHECK_FIFOLDER 1713
#define IDWHOLEWORD_FIFOLDER                    1714
#define IDMATCHCASE_FIFOLDER                    1715
#define IDC_MODE_STATIC_FIFOLDER                1716
#define IDNORMAL_FIFOLDER                               1717
#define IDEXTENDED_FIFOLDER                             1718
#define IDREGEXP_FIFOLDER                               1719
#define IDREDOTMATCHNL_FIFOLDER                 1720
#define IDC_FINDPREV                                    1721
#define IDC_BACKWARDDIRECTION                   1722
#define IDC_FINDNEXT                                    1723
#define IDC_2_BUTTONS_MODE                              1724

#define IDC_COPY_MARKED_TEXT                    1725
//
//#include "UserDefineResource.h"
#define    IDD_GLOBAL_USERDEFINE_DLG 20000
    #define    IDC_DOCK_BUTTON                              (IDD_GLOBAL_USERDEFINE_DLG + 1 )
    #define    IDC_RENAME_BUTTON                            (IDD_GLOBAL_USERDEFINE_DLG + 2 )
    #define    IDC_ADDNEW_BUTTON                            (IDD_GLOBAL_USERDEFINE_DLG + 3 )
    #define    IDC_REMOVELANG_BUTTON                        (IDD_GLOBAL_USERDEFINE_DLG + 4 )
    #define    IDC_SAVEAS_BUTTON                            (IDD_GLOBAL_USERDEFINE_DLG + 5 )
    #define    IDC_LANGNAME_COMBO                           (IDD_GLOBAL_USERDEFINE_DLG + 6 )
    #define    IDC_LANGNAME_STATIC                          (IDD_GLOBAL_USERDEFINE_DLG + 7 )
    #define    IDC_EXT_EDIT                                 (IDD_GLOBAL_USERDEFINE_DLG + 8 )
    #define    IDC_EXT_STATIC                               (IDD_GLOBAL_USERDEFINE_DLG + 9 )

    #define    IDC_UD_PERCENTAGE_SLIDER                     (IDD_GLOBAL_USERDEFINE_DLG + 10)
    #define    IDC_UD_TRANSPARENT_CHECK                     (IDD_GLOBAL_USERDEFINE_DLG + 11)
    #define    IDC_LANGNAME_IGNORECASE_CHECK                (IDD_GLOBAL_USERDEFINE_DLG + 12)
    #define    IDC_AUTOCOMPLET_EDIT                         (IDD_GLOBAL_USERDEFINE_DLG + 13)
    #define    IDC_AUTOCOMPLET_STATIC                       (IDD_GLOBAL_USERDEFINE_DLG + 14)
    #define    IDC_IMPORT_BUTTON                            (IDD_GLOBAL_USERDEFINE_DLG + 15)
    #define    IDC_EXPORT_BUTTON                            (IDD_GLOBAL_USERDEFINE_DLG + 16)

#define    IDD_FOLDER_STYLE_DLG   21000 // IDD_GLOBAL_USERDEFINE_DLG + 1000
     #define    IDC_DEFAULT                                 (IDD_FOLDER_STYLE_DLG + 100)
	#define    IDC_DEFAULT_DESCGROUP_STATIC             (IDC_DEFAULT + 1)
	#define    IDC_DEFAULT_STYLER                       (IDC_DEFAULT + 2)
	#define    IDC_WEB_HELP_LINK                        (IDC_DEFAULT + 3)
	#define    IDC_WEB_HELP_STATIC                      (IDC_DEFAULT + 4)
	#define    IDC_WEB_HELP_DESCGROUP_STATIC            (IDC_DEFAULT + 5)
	#define    IDC_FOLDER_FOLD_COMPACT                  (IDC_DEFAULT + 6)

    #define    IDC_FOLDER_IN_CODE1                          (IDD_FOLDER_STYLE_DLG + 200)
	#define IDC_FOLDER_IN_CODE1_DESCGROUP_STATIC            (IDC_FOLDER_IN_CODE1 + 20)
	#define IDC_FOLDER_IN_CODE1_OPEN_EDIT                   (IDC_FOLDER_IN_CODE1 + 21)
	#define IDC_FOLDER_IN_CODE1_MIDDLE_EDIT                 (IDC_FOLDER_IN_CODE1 + 22)
	#define IDC_FOLDER_IN_CODE1_CLOSE_EDIT                  (IDC_FOLDER_IN_CODE1 + 23)
	#define IDC_FOLDER_IN_CODE1_OPEN_STATIC                 (IDC_FOLDER_IN_CODE1 + 24)
	#define IDC_FOLDER_IN_CODE1_MIDDLE_STATIC               (IDC_FOLDER_IN_CODE1 + 25)
	#define IDC_FOLDER_IN_CODE1_CLOSE_STATIC                (IDC_FOLDER_IN_CODE1 + 26)
	#define IDC_FOLDER_IN_CODE1_STYLER                      (IDC_FOLDER_IN_CODE1 + 27)

    #define    IDC_FOLDER_IN_CODE2                          (IDD_FOLDER_STYLE_DLG + 300)
	#define IDC_FOLDER_IN_CODE2_DESCGROUP_STATIC            (IDC_FOLDER_IN_CODE2 + 20)
	#define IDC_FOLDER_IN_CODE2_OPEN_EDIT                   (IDC_FOLDER_IN_CODE2 + 21)
	#define IDC_FOLDER_IN_CODE2_MIDDLE_EDIT                 (IDC_FOLDER_IN_CODE2 + 22)
	#define IDC_FOLDER_IN_CODE2_CLOSE_EDIT                  (IDC_FOLDER_IN_CODE2 + 23)
	#define IDC_FOLDER_IN_CODE2_OPEN_STATIC                 (IDC_FOLDER_IN_CODE2 + 24)
	#define IDC_FOLDER_IN_CODE2_MIDDLE_STATIC               (IDC_FOLDER_IN_CODE2 + 25)
	#define IDC_FOLDER_IN_CODE2_CLOSE_STATIC                (IDC_FOLDER_IN_CODE2 + 26)
	#define IDC_FOLDER_IN_CODE2_STYLER                      (IDC_FOLDER_IN_CODE2 + 27)

    #define    IDC_FOLDER_IN_COMMENT                         (IDD_FOLDER_STYLE_DLG + 400)
	#define IDC_FOLDER_IN_COMMENT_DESCGROUP_STATIC          (IDC_FOLDER_IN_COMMENT + 20)
	#define IDC_FOLDER_IN_COMMENT_OPEN_EDIT                 (IDC_FOLDER_IN_COMMENT + 21)
	#define IDC_FOLDER_IN_COMMENT_MIDDLE_EDIT               (IDC_FOLDER_IN_COMMENT + 22)
	#define IDC_FOLDER_IN_COMMENT_CLOSE_EDIT                (IDC_FOLDER_IN_COMMENT + 23)
	#define IDC_FOLDER_IN_COMMENT_OPEN_STATIC               (IDC_FOLDER_IN_COMMENT + 24)
	#define IDC_FOLDER_IN_COMMENT_MIDDLE_STATIC             (IDC_FOLDER_IN_COMMENT + 25)
	#define IDC_FOLDER_IN_COMMENT_CLOSE_STATIC              (IDC_FOLDER_IN_COMMENT + 26)
	#define IDC_FOLDER_IN_COMMENT_STYLER                    (IDC_FOLDER_IN_COMMENT + 27)

#define    IDD_KEYWORD_STYLE_DLG   22000 //(IDD_GLOBAL_USERDEFINE_DLG + 2000)
    #define    IDC_KEYWORD1                                 (IDD_KEYWORD_STYLE_DLG + 100)
	#define    IDC_KEYWORD1_DESCGROUP_STATIC            (IDC_KEYWORD1 + 1 )
	#define    IDC_KEYWORD1_EDIT                        (IDC_KEYWORD1 + 20)
	#define    IDC_KEYWORD1_PREFIX_CHECK                (IDC_KEYWORD1 + 21)
	#define    IDC_KEYWORD1_STYLER                      (IDC_KEYWORD1 + 22)

    #define    IDC_KEYWORD2                                 (IDD_KEYWORD_STYLE_DLG + 200)
	#define    IDC_KEYWORD2_DESCGROUP_STATIC            (IDC_KEYWORD2 + 1 )
	#define    IDC_KEYWORD2_EDIT                        (IDC_KEYWORD2 + 20)
	#define    IDC_KEYWORD2_PREFIX_CHECK                (IDC_KEYWORD2 + 21)
	#define    IDC_KEYWORD2_STYLER                      (IDC_KEYWORD2 + 22)

    #define    IDC_KEYWORD3                                 (IDD_KEYWORD_STYLE_DLG + 300)
	#define    IDC_KEYWORD3_DESCGROUP_STATIC            (IDC_KEYWORD3 + 1 )
	#define    IDC_KEYWORD3_EDIT                        (IDC_KEYWORD3 + 20)
	#define    IDC_KEYWORD3_PREFIX_CHECK                (IDC_KEYWORD3 + 21)
	#define    IDC_KEYWORD3_STYLER                      (IDC_KEYWORD3 + 22)

    #define    IDC_KEYWORD4                                 (IDD_KEYWORD_STYLE_DLG + 400)
	#define    IDC_KEYWORD4_DESCGROUP_STATIC            (IDC_KEYWORD4 + 1 )
	#define    IDC_KEYWORD4_EDIT                        (IDC_KEYWORD4 + 20)
	#define    IDC_KEYWORD4_PREFIX_CHECK                (IDC_KEYWORD4 + 21)
	#define    IDC_KEYWORD4_STYLER                      (IDC_KEYWORD4 + 22)

    #define    IDC_KEYWORD5                                 (IDD_KEYWORD_STYLE_DLG + 450)
	#define    IDC_KEYWORD5_DESCGROUP_STATIC            (IDC_KEYWORD5 + 1 )
	#define    IDC_KEYWORD5_EDIT                        (IDC_KEYWORD5 + 20)
	#define    IDC_KEYWORD5_PREFIX_CHECK                (IDC_KEYWORD5 + 21)
	#define    IDC_KEYWORD5_STYLER                      (IDC_KEYWORD5 + 22)

    #define    IDC_KEYWORD6                                 (IDD_KEYWORD_STYLE_DLG + 500)
	#define    IDC_KEYWORD6_DESCGROUP_STATIC            (IDC_KEYWORD6 + 1 )
	#define    IDC_KEYWORD6_EDIT                        (IDC_KEYWORD6 + 20)
	#define    IDC_KEYWORD6_PREFIX_CHECK                (IDC_KEYWORD6 + 21)
	#define    IDC_KEYWORD6_STYLER                      (IDC_KEYWORD6 + 22)

    #define    IDC_KEYWORD7                                 (IDD_KEYWORD_STYLE_DLG + 550)
	#define    IDC_KEYWORD7_DESCGROUP_STATIC            (IDC_KEYWORD7 + 1 )
	#define    IDC_KEYWORD7_EDIT                        (IDC_KEYWORD7 + 20)
	#define    IDC_KEYWORD7_PREFIX_CHECK                (IDC_KEYWORD7 + 21)
	#define    IDC_KEYWORD7_STYLER                      (IDC_KEYWORD7 + 22)

    #define    IDC_KEYWORD8                                 (IDD_KEYWORD_STYLE_DLG + 600)
	#define    IDC_KEYWORD8_DESCGROUP_STATIC            (IDC_KEYWORD8 + 1 )
	#define    IDC_KEYWORD8_EDIT                        (IDC_KEYWORD8 + 20)
	#define    IDC_KEYWORD8_PREFIX_CHECK                (IDC_KEYWORD8 + 21)
	#define    IDC_KEYWORD8_STYLER                      (IDC_KEYWORD8 + 22)

#define    IDD_COMMENT_STYLE_DLG 23000 //(IDD_GLOBAL_USERDEFINE_DLG + 3000)
    #define IDC_FOLDING_OF_COMMENTS             (IDD_COMMENT_STYLE_DLG + 1)
    #define IDC_COMMENTLINE_POSITION_STATIC     (IDD_COMMENT_STYLE_DLG + 3)
    #define IDC_ALLOW_ANYWHERE                  (IDD_COMMENT_STYLE_DLG + 4)
    #define IDC_FORCE_AT_BOL                    (IDD_COMMENT_STYLE_DLG + 5)
    #define IDC_ALLOW_WHITESPACE                (IDD_COMMENT_STYLE_DLG + 6)

    #define    IDC_COMMENT                                  (IDD_COMMENT_STYLE_DLG + 100)
	#define    IDC_COMMENT_DESCGROUP_STATIC             (IDC_COMMENT + 1 )
	#define    IDC_COMMENT_OPEN_EDIT                    (IDC_COMMENT + 20)
	#define    IDC_COMMENT_CLOSE_EDIT                   (IDC_COMMENT + 21)
	#define    IDC_COMMENT_OPEN_STATIC                  (IDC_COMMENT + 22)
	#define    IDC_COMMENT_CLOSE_STATIC                 (IDC_COMMENT + 23)
	#define    IDC_COMMENT_STYLER                       (IDC_COMMENT + 24)

    #define    IDC_NUMBER                                   (IDD_COMMENT_STYLE_DLG + 200)
	#define    IDC_NUMBER_DESCGROUP_STATIC              (IDC_NUMBER + 1 )
	#define    IDC_NUMBER_STYLER                        (IDC_NUMBER + 20)
	#define    IDC_NUMBER_PREFIX1_STATIC                (IDC_NUMBER + 30)
	#define    IDC_NUMBER_PREFIX1_EDIT                  (IDC_NUMBER + 31)
	#define    IDC_NUMBER_PREFIX2_STATIC                (IDC_NUMBER + 32)
	#define    IDC_NUMBER_PREFIX2_EDIT                  (IDC_NUMBER + 33)
	#define    IDC_NUMBER_EXTRAS1_STATIC                (IDC_NUMBER + 34)
	#define    IDC_NUMBER_EXTRAS1_EDIT                  (IDC_NUMBER + 35)
	#define    IDC_NUMBER_EXTRAS2_STATIC                (IDC_NUMBER + 36)
	#define    IDC_NUMBER_EXTRAS2_EDIT                  (IDC_NUMBER + 37)
	#define    IDC_NUMBER_SUFFIX1_STATIC                (IDC_NUMBER + 38)
	#define    IDC_NUMBER_SUFFIX1_EDIT                  (IDC_NUMBER + 39)
	#define    IDC_NUMBER_SUFFIX2_STATIC                (IDC_NUMBER + 40)
	#define    IDC_NUMBER_SUFFIX2_EDIT                  (IDC_NUMBER + 41)
	#define    IDC_NUMBER_RANGE_STATIC                  (IDC_NUMBER + 42)
	#define    IDC_NUMBER_RANGE_EDIT                    (IDC_NUMBER + 43)
	#define    IDC_DECIMAL_SEPARATOR_STATIC             (IDC_NUMBER + 44)
	#define    IDC_DOT_RADIO                            (IDC_NUMBER + 45)
	#define    IDC_COMMA_RADIO                          (IDC_NUMBER + 46)
	#define    IDC_BOTH_RADIO                           (IDC_NUMBER + 47)

    #define    IDC_COMMENTLINE                              (IDD_COMMENT_STYLE_DLG + 300)
	#define    IDC_COMMENTLINE_DESCGROUP_STATIC         (IDC_COMMENTLINE + 1 )
	#define    IDC_COMMENTLINE_OPEN_EDIT                (IDC_COMMENTLINE + 20)
	#define    IDC_COMMENTLINE_CONTINUE_EDIT            (IDC_COMMENTLINE + 21)
	#define    IDC_COMMENTLINE_CLOSE_EDIT               (IDC_COMMENTLINE + 22)
	#define    IDC_COMMENTLINE_OPEN_STATIC              (IDC_COMMENTLINE + 23)
	#define    IDC_COMMENTLINE_CONTINUE_STATIC          (IDC_COMMENTLINE + 24)
	#define    IDC_COMMENTLINE_CLOSE_STATIC             (IDC_COMMENTLINE + 25)
	#define    IDC_COMMENTLINE_STYLER                   (IDC_COMMENTLINE + 26)

#define    IDD_SYMBOL_STYLE_DLG   24000   //IDD_GLOBAL_USERDEFINE_DLG + 4000
    #define    IDC_OPERATOR                             (IDD_SYMBOL_STYLE_DLG + 100)
	#define    IDC_OPERATOR_DESCGROUP_STATIC            (IDC_OPERATOR + 1 )
	#define    IDC_OPERATOR_STYLER                      (IDC_OPERATOR + 13)
	#define    IDC_OPERATOR1_EDIT                       (IDC_OPERATOR + 14)
	#define    IDC_OPERATOR2_EDIT                       (IDC_OPERATOR + 15)
	#define    IDC_OPERATOR1_STATIC                     (IDC_OPERATOR + 16)
	#define    IDC_OPERATOR2_STATIC                     (IDC_OPERATOR + 17)

    #define    IDC_DELIMITER1                               (IDD_SYMBOL_STYLE_DLG + 200)
	#define    IDC_DELIMITER1_DESCGROUP_STATIC          (IDC_DELIMITER1 + 1 )
	#define    IDC_DELIMITER1_BOUNDARYOPEN_EDIT         (IDC_DELIMITER1 + 17)
	#define    IDC_DELIMITER1_ESCAPE_EDIT               (IDC_DELIMITER1 + 18)
	#define    IDC_DELIMITER1_BOUNDARYCLOSE_EDIT        (IDC_DELIMITER1 + 19)
	#define    IDC_DELIMITER1_BOUNDARYOPEN_STATIC       (IDC_DELIMITER1 + 20)
	#define    IDC_DELIMITER1_ESCAPE_STATIC             (IDC_DELIMITER1 + 21)
	#define    IDC_DELIMITER1_BOUNDARYCLOSE_STATIC      (IDC_DELIMITER1 + 22)
	#define    IDC_DELIMITER1_STYLER                    (IDC_DELIMITER1 + 23)

    #define    IDC_DELIMITER2                               (IDD_SYMBOL_STYLE_DLG + 300)
	#define    IDC_DELIMITER2_DESCGROUP_STATIC          (IDC_DELIMITER2 + 1 )
	#define    IDC_DELIMITER2_BOUNDARYOPEN_EDIT         (IDC_DELIMITER2 + 17)
	#define    IDC_DELIMITER2_ESCAPE_EDIT               (IDC_DELIMITER2 + 18)
	#define    IDC_DELIMITER2_BOUNDARYCLOSE_EDIT        (IDC_DELIMITER2 + 19)
	#define    IDC_DELIMITER2_BOUNDARYOPEN_STATIC       (IDC_DELIMITER2 + 20)
	#define    IDC_DELIMITER2_ESCAPE_STATIC             (IDC_DELIMITER2 + 21)
	#define    IDC_DELIMITER2_BOUNDARYCLOSE_STATIC      (IDC_DELIMITER2 + 22)
	#define    IDC_DELIMITER2_STYLER                    (IDC_DELIMITER2 + 23)

    #define    IDC_DELIMITER3                               (IDD_SYMBOL_STYLE_DLG + 400)
	#define    IDC_DELIMITER3_DESCGROUP_STATIC          (IDC_DELIMITER3 + 1 )
	#define    IDC_DELIMITER3_BOUNDARYOPEN_EDIT         (IDC_DELIMITER3 + 17)
	#define    IDC_DELIMITER3_ESCAPE_EDIT               (IDC_DELIMITER3 + 18)
	#define    IDC_DELIMITER3_BOUNDARYCLOSE_EDIT        (IDC_DELIMITER3 + 19)
	#define    IDC_DELIMITER3_BOUNDARYOPEN_STATIC       (IDC_DELIMITER3 + 20)
	#define    IDC_DELIMITER3_ESCAPE_STATIC             (IDC_DELIMITER3 + 21)
	#define    IDC_DELIMITER3_BOUNDARYCLOSE_STATIC      (IDC_DELIMITER3 + 22)
	#define    IDC_DELIMITER3_STYLER                    (IDC_DELIMITER3 + 23)

    #define    IDC_DELIMITER4                               (IDD_SYMBOL_STYLE_DLG + 450)
	#define    IDC_DELIMITER4_DESCGROUP_STATIC          (IDC_DELIMITER4 + 1 )
	#define    IDC_DELIMITER4_BOUNDARYOPEN_EDIT         (IDC_DELIMITER4 + 17)
	#define    IDC_DELIMITER4_ESCAPE_EDIT               (IDC_DELIMITER4 + 18)
	#define    IDC_DELIMITER4_BOUNDARYCLOSE_EDIT        (IDC_DELIMITER4 + 19)
	#define    IDC_DELIMITER4_BOUNDARYOPEN_STATIC       (IDC_DELIMITER4 + 20)
	#define    IDC_DELIMITER4_ESCAPE_STATIC             (IDC_DELIMITER4 + 21)
	#define    IDC_DELIMITER4_BOUNDARYCLOSE_STATIC      (IDC_DELIMITER4 + 22)
	#define    IDC_DELIMITER4_STYLER                    (IDC_DELIMITER4 + 23)

    #define    IDC_DELIMITER5                               (IDD_SYMBOL_STYLE_DLG + 500)
	#define    IDC_DELIMITER5_DESCGROUP_STATIC          (IDC_DELIMITER5 + 1 )
	#define    IDC_DELIMITER5_BOUNDARYOPEN_EDIT         (IDC_DELIMITER5 + 17)
	#define    IDC_DELIMITER5_ESCAPE_EDIT               (IDC_DELIMITER5 + 18)
	#define    IDC_DELIMITER5_BOUNDARYCLOSE_EDIT        (IDC_DELIMITER5 + 19)
	#define    IDC_DELIMITER5_BOUNDARYOPEN_STATIC       (IDC_DELIMITER5 + 20)
	#define    IDC_DELIMITER5_ESCAPE_STATIC             (IDC_DELIMITER5 + 21)
	#define    IDC_DELIMITER5_BOUNDARYCLOSE_STATIC      (IDC_DELIMITER5 + 22)
	#define    IDC_DELIMITER5_STYLER                    (IDC_DELIMITER5 + 23)

    #define    IDC_DELIMITER6                               (IDD_SYMBOL_STYLE_DLG + 550)
	#define    IDC_DELIMITER6_DESCGROUP_STATIC          (IDC_DELIMITER6 + 1 )
	#define    IDC_DELIMITER6_BOUNDARYOPEN_EDIT         (IDC_DELIMITER6 + 17)
	#define    IDC_DELIMITER6_ESCAPE_EDIT               (IDC_DELIMITER6 + 18)
	#define    IDC_DELIMITER6_BOUNDARYCLOSE_EDIT        (IDC_DELIMITER6 + 19)
	#define    IDC_DELIMITER6_BOUNDARYOPEN_STATIC       (IDC_DELIMITER6 + 20)
	#define    IDC_DELIMITER6_ESCAPE_STATIC             (IDC_DELIMITER6 + 21)
	#define    IDC_DELIMITER6_BOUNDARYCLOSE_STATIC      (IDC_DELIMITER6 + 22)
	#define    IDC_DELIMITER6_STYLER                    (IDC_DELIMITER6 + 23)

    #define    IDC_DELIMITER7                               (IDD_SYMBOL_STYLE_DLG + 600)
	#define    IDC_DELIMITER7_DESCGROUP_STATIC          (IDC_DELIMITER7 + 1 )
	#define    IDC_DELIMITER7_BOUNDARYOPEN_EDIT         (IDC_DELIMITER7 + 17)
	#define    IDC_DELIMITER7_ESCAPE_EDIT               (IDC_DELIMITER7 + 18)
	#define    IDC_DELIMITER7_BOUNDARYCLOSE_EDIT        (IDC_DELIMITER7 + 19)
	#define    IDC_DELIMITER7_BOUNDARYOPEN_STATIC       (IDC_DELIMITER7 + 20)
	#define    IDC_DELIMITER7_ESCAPE_STATIC             (IDC_DELIMITER7 + 21)
	#define    IDC_DELIMITER7_BOUNDARYCLOSE_STATIC      (IDC_DELIMITER7 + 22)
	#define    IDC_DELIMITER7_STYLER                    (IDC_DELIMITER7 + 23)

    #define    IDC_DELIMITER8                               (IDD_SYMBOL_STYLE_DLG + 650)
	#define    IDC_DELIMITER8_DESCGROUP_STATIC          (IDC_DELIMITER8 + 1 )
	#define    IDC_DELIMITER8_BOUNDARYOPEN_EDIT         (IDC_DELIMITER8 + 17)
	#define    IDC_DELIMITER8_ESCAPE_EDIT               (IDC_DELIMITER8 + 18)
	#define    IDC_DELIMITER8_BOUNDARYCLOSE_EDIT        (IDC_DELIMITER8 + 19)
	#define    IDC_DELIMITER8_BOUNDARYOPEN_STATIC       (IDC_DELIMITER8 + 20)
	#define    IDC_DELIMITER8_ESCAPE_STATIC             (IDC_DELIMITER8 + 21)
	#define    IDC_DELIMITER8_BOUNDARYCLOSE_STATIC      (IDC_DELIMITER8 + 22)
	#define    IDC_DELIMITER8_STYLER                    (IDC_DELIMITER8 + 23)

#define    IDD_STYLER_POPUP_DLG   25000   //IDD_GLOBAL_USERDEFINE_DLG + 5000
    #define IDC_STYLER_CHECK_BOLD                           (IDD_STYLER_POPUP_DLG + 1 )
    #define IDC_STYLER_CHECK_ITALIC                         (IDD_STYLER_POPUP_DLG + 2 )
    #define IDC_STYLER_CHECK_UNDERLINE                      (IDD_STYLER_POPUP_DLG + 3 )
    #define IDC_STYLER_COMBO_FONT_NAME                      (IDD_STYLER_POPUP_DLG + 4 )
    #define IDC_STYLER_COMBO_FONT_SIZE                      (IDD_STYLER_POPUP_DLG + 5 )
    #define IDC_STYLER_FG_STATIC                            (IDD_STYLER_POPUP_DLG + 6 )
    #define IDC_STYLER_BG_STATIC                            (IDD_STYLER_POPUP_DLG + 7 )
    #define IDC_STYLER_CHECK_NESTING_DELIMITER1             (IDD_STYLER_POPUP_DLG + 8 )
    #define IDC_STYLER_CHECK_NESTING_DELIMITER2             (IDD_STYLER_POPUP_DLG + 9 )
    #define IDC_STYLER_CHECK_NESTING_DELIMITER3             (IDD_STYLER_POPUP_DLG + 10)
    #define IDC_STYLER_CHECK_NESTING_DELIMITER4             (IDD_STYLER_POPUP_DLG + 11)
    #define IDC_STYLER_CHECK_NESTING_DELIMITER5             (IDD_STYLER_POPUP_DLG + 12)
    #define IDC_STYLER_CHECK_NESTING_DELIMITER6             (IDD_STYLER_POPUP_DLG + 13)
    #define IDC_STYLER_CHECK_NESTING_DELIMITER7             (IDD_STYLER_POPUP_DLG + 14)
    #define IDC_STYLER_CHECK_NESTING_DELIMITER8             (IDD_STYLER_POPUP_DLG + 15)
    #define IDC_STYLER_CHECK_NESTING_COMMENT                (IDD_STYLER_POPUP_DLG + 16)
    #define IDC_STYLER_CHECK_NESTING_COMMENT_LINE           (IDD_STYLER_POPUP_DLG + 17)
    #define IDC_STYLER_CHECK_NESTING_KEYWORD1               (IDD_STYLER_POPUP_DLG + 18)
    #define IDC_STYLER_CHECK_NESTING_KEYWORD2               (IDD_STYLER_POPUP_DLG + 19)
    #define IDC_STYLER_CHECK_NESTING_KEYWORD3               (IDD_STYLER_POPUP_DLG + 20)
    #define IDC_STYLER_CHECK_NESTING_KEYWORD4               (IDD_STYLER_POPUP_DLG + 21)
    #define IDC_STYLER_CHECK_NESTING_KEYWORD5               (IDD_STYLER_POPUP_DLG + 22)
    #define IDC_STYLER_CHECK_NESTING_KEYWORD6               (IDD_STYLER_POPUP_DLG + 23)
    #define IDC_STYLER_CHECK_NESTING_KEYWORD7               (IDD_STYLER_POPUP_DLG + 24)
    #define IDC_STYLER_CHECK_NESTING_KEYWORD8               (IDD_STYLER_POPUP_DLG + 25)
    #define IDC_STYLER_CHECK_NESTING_OPERATORS1             (IDD_STYLER_POPUP_DLG + 26)
    #define IDC_STYLER_CHECK_NESTING_OPERATORS2             (IDD_STYLER_POPUP_DLG + 27)
    #define IDC_STYLER_CHECK_NESTING_NUMBERS                (IDD_STYLER_POPUP_DLG + 28)
    #define IDC_STYLER_STATIC_NESTING_GROUP                 (IDD_STYLER_POPUP_DLG + 29)
    #define IDC_STYLER_STATIC_FONT_OPTIONS                  (IDD_STYLER_POPUP_DLG + 30)
    #define IDC_STYLER_NAME_STATIC                          (IDD_STYLER_POPUP_DLG + 31)
    #define IDC_STYLER_SIZE_STATIC                          (IDD_STYLER_POPUP_DLG + 32)

#define    IDD_STRING_DLG   26000   //IDD_GLOBAL_USERDEFINE_DLG + 6000
    #define    IDC_STRING_STATIC                            (IDD_STRING_DLG + 1)
    #define    IDC_STRING_EDIT                              (IDD_STRING_DLG + 2)
//
#endif // } __NPP_RESOURCE_H
