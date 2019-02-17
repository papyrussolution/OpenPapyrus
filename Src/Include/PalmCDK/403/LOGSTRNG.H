/*****************************************************************************
 *
 * Copyright (c) 1998-1999 Palm Computing, Inc. or its subsidiaries.  
 * All rights reserved.
 *
 ****************************************************************************/
/////////////////////////////////////////////////////////////////////////////
//
//      File:                   LogStrng.h
//
//      Description:    Contains the string codes for the log messages.
//
/////////////////////////////////////////////////////////////////////////////

#define  IDSTR_FIRSTLOG           3300

#define  IDSTR_DATEBOOK           (IDSTR_FIRSTLOG + 1)
#define  IDSTR_ADDRESSBOOK        (IDSTR_FIRSTLOG + 2)
#define  IDSTR_MEMOPAD            (IDSTR_FIRSTLOG + 3)
#define  IDSTR_TODO               (IDSTR_FIRSTLOG + 4)
#define  IDSTR_DUE                (IDSTR_FIRSTLOG + 5)
#define  IDSTR_NO_DUE_DATE        (IDSTR_FIRSTLOG + 6)
#define  IDSTR_PRIORITY           (IDSTR_FIRSTLOG + 7)
#define  IDSTR_AM                 (IDSTR_FIRSTLOG + 8)
#define  IDSTR_PM                 (IDSTR_FIRSTLOG + 9)
#define  IDSTR_UNTIMED            (IDSTR_FIRSTLOG + 10)
#define  IDSTR_CONNECTION         (IDSTR_FIRSTLOG + 11)
#define  IDSTR_DEVICE_FULL        (IDSTR_FIRSTLOG + 12)
#define  IDSTR_DESKTOP_FULL       (IDSTR_FIRSTLOG + 13)
#define  IDSTR_DESKTOP            (IDSTR_FIRSTLOG + 14)
#define  IDSTR_HANDHELD			  (IDSTR_FIRSTLOG + 15)
#define  IDSTR_BAD_ADD_REC        (IDSTR_FIRSTLOG + 16)
#define  IDSTR_BAD_PURGE          (IDSTR_FIRSTLOG + 17)
#define  IDSTR_BAD_DEL_REC        (IDSTR_FIRSTLOG + 18)
#define  IDSTR_BAD_CHANGE_REC     (IDSTR_FIRSTLOG + 19)
#define  IDSTR_BAD_LOCALADD_REC   (IDSTR_FIRSTLOG + 20)
#define  IDSTR_BAD_CHANGE_CAT1    (IDSTR_FIRSTLOG + 21)
#define  IDSTR_BAD_CHANGE_CAT2    (IDSTR_FIRSTLOG + 22)
#define  IDSTR_BAD_READ_RECORD    (IDSTR_FIRSTLOG + 23)
#define  IDSTR_TOOMANY_CATS1      (IDSTR_FIRSTLOG + 24)
#define  IDSTR_TOOMANY_CATS2      (IDSTR_FIRSTLOG + 25)
#define  IDSTR_CATEGORY_DEL1      (IDSTR_FIRSTLOG + 26)
#define  IDSTR_CATEGORY_DEL2      (IDSTR_FIRSTLOG + 27)
#define  IDSTR_CATEGORY_DEL3      (IDSTR_FIRSTLOG + 28)
#define  IDSTR_DOUBLE_MODIFY1     (IDSTR_FIRSTLOG + 29)
#define  IDSTR_DOUBLE_MODIFY2     (IDSTR_FIRSTLOG + 30)
#define  IDSTR_DOUBLE_MODIFY3     (IDSTR_FIRSTLOG + 31)
#define  IDSTR_ARCH_DOUBLE_MOD1   (IDSTR_FIRSTLOG + 32)
#define  IDSTR_ARCH_DOUBLE_MOD2   (IDSTR_FIRSTLOG + 33)
#define  IDSTR_ARCH_DOUBLE_MOD3   (IDSTR_FIRSTLOG + 34)
#define  IDSTR_REVERSE_DEL1       (IDSTR_FIRSTLOG + 35)
#define  IDSTR_REVERSE_DEL2       (IDSTR_FIRSTLOG + 36)
#define  IDSTR_DATE_CHANGED1      (IDSTR_FIRSTLOG + 37)
#define  IDSTR_DATE_CHANGED2      (IDSTR_FIRSTLOG + 38)
#define  IDSTR_RECCOUNT_DESKTOP1  (IDSTR_FIRSTLOG + 39)
#define  IDSTR_RECCOUNT_DESKTOP2  (IDSTR_FIRSTLOG + 40)
#define  IDSTR_RECCOUNT_DESKTOP3  (IDSTR_FIRSTLOG + 41)
#define  IDSTR_RECCOUNT_PILOT1    (IDSTR_FIRSTLOG + 42)
#define  IDSTR_RECCOUNT_PILOT2    (IDSTR_FIRSTLOG + 43)
#define  IDSTR_CUSTOM_LABEL1      (IDSTR_FIRSTLOG + 44)
#define  IDSTR_CUSTOM_LABEL2      (IDSTR_FIRSTLOG + 45)
#define  IDSTR_BAD_XMAP           (IDSTR_FIRSTLOG + 46)
#define  IDSTR_BAD_ARCHIVE_ERR    (IDSTR_FIRSTLOG + 47)
#define  IDSTR_BAD_LOCAL_SAVE     (IDSTR_FIRSTLOG + 48)
#define  IDSTR_BAD_RESET_FLAGS    (IDSTR_FIRSTLOG + 49)
#define  IDSTR_INSTALL            (IDSTR_FIRSTLOG + 50)
#define  IDSTR_BACKUP             (IDSTR_FIRSTLOG + 51)
#define  IDSTR_INSTALL_SENT       (IDSTR_FIRSTLOG + 52)
#define  IDSTR_INSTALL_DELETE     (IDSTR_FIRSTLOG + 53)
#define  IDSTR_RESTORE            (IDSTR_FIRSTLOG + 54)
#define  IDSTR_INSTALL_FAIL       (IDSTR_FIRSTLOG + 55)
#define  IDSTR_INSTALL_DUPLICATE  (IDSTR_FIRSTLOG + 56)
#define  IDSTR_TODO_DATE_CHANGED1 (IDSTR_FIRSTLOG + 57)
#define  IDSTR_TODO_DATE_CHANGED2 (IDSTR_FIRSTLOG + 58)
#define  IDSTR_DOUBLE_MODIFY_SUBSC1     (IDSTR_FIRSTLOG + 59)
#define  IDSTR_DOUBLE_MODIFY_SUBSC2     (IDSTR_FIRSTLOG + 60)
#define  IDSTR_FILELINK_SUCCESSFUL_UPDATE     (IDSTR_FIRSTLOG + 61)
#define  IDSTR_FILELINK_FAILED_UPDATE     (IDSTR_FIRSTLOG + 62)
#define  IDSTR_FILELINK_DETAILS     (IDSTR_FIRSTLOG + 63)
#define  IDSTR_FILELINK_DELETED     (IDSTR_FIRSTLOG + 64)
#define  IDSTR_FILELINK_RENAMED     (IDSTR_FIRSTLOG + 65)
#define  IDSTR_MULTIPLE_MEMOS1     (IDSTR_FIRSTLOG + 66)
#define  IDSTR_MULTIPLE_MEMOS2     (IDSTR_FIRSTLOG + 67)
#define  IDSTR_FILELINK_FAILED_UPDATE2     (IDSTR_FIRSTLOG + 68)
#define IDS_DEVICE_FULL_30              (IDSTR_FIRSTLOG + 69)
#define IDSTR_SYNC_DONOTHING            (IDSTR_FIRSTLOG + 70)
#define IDS_CUSTOM_LABEL_OVERWRITE      (IDSTR_FIRSTLOG + 71)
#define IDS_TOOMANY_CATS_LOG            (IDSTR_FIRSTLOG + 72)
#define IDS_BAD_CHANGE_CAT              (IDSTR_FIRSTLOG + 73)
#define IDS_CATEGORY_DEL                (IDSTR_FIRSTLOG + 74)
#define IDS_DOUBLE_MODIFY               (IDSTR_FIRSTLOG + 75)
#define IDS_ARCH_DOUBLE_MOD             (IDSTR_FIRSTLOG + 76)
#define IDS_REVERSE_DEL                 (IDSTR_FIRSTLOG + 77)
#define IDS_DATE_CHANGED                (IDSTR_FIRSTLOG + 78)
#define IDS_RECCOUNT_DESKTOP            (IDSTR_FIRSTLOG + 79)
#define IDS_RECCOUNT_PILOT              (IDSTR_FIRSTLOG + 80)
#define IDS_TODO_DATE_CHANGED           (IDSTR_FIRSTLOG + 81)
#define IDS_DOUBLE_MODIFY_SUBSC         (IDSTR_FIRSTLOG + 82)

//#ifdef BGS_EXPENSE /* (kanai@bowneglobal.co.jp) */
#define  IDSTR_EXPENSE               (IDSTR_FIRSTLOG + 83)
#define  IDSTR_EXPENSE_DATE_CHANGED1 (IDSTR_FIRSTLOG + 84)
#define  IDSTR_EXPENSE_DATE_CHANGED2 (IDSTR_FIRSTLOG + 85)
#define  IDSTR_CUSTOM_CURRENCY1      (IDSTR_FIRSTLOG + 86)
#define  IDSTR_CUSTOM_CURRENCY2      (IDSTR_FIRSTLOG + 87)
#define  IDS_EXPENSE_DATE_CHANGED    (IDSTR_FIRSTLOG + 88)
//#else /* not BGS_EXPENSE */
//#endif /* not BGS_EXPENSE */

#define  IDS_FLNK_ADDR_FILTER        (IDSTR_FIRSTLOG + 89)
#define  IDS_FLNK_MEMO_FILTER        (IDSTR_FIRSTLOG + 90)
#define IDS_DTRECCOUNT_LESS             (IDSTR_FIRSTLOG + 91)
#define IDS_HHRECCOUNT_LESS             (IDSTR_FIRSTLOG + 92)


#define  IDSTR_LASTLOG            3600


