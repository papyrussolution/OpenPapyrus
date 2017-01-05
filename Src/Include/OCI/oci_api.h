/* ------------------------------------------------------------------------ *
 *
 * oci_api.h - Part of OCILIB Project
 *
 * Oracle OCI API Protypes declaration
 *
 * OCILIB : ISO C Oracle Call Interface (OCI) Encapsulation
 *
 * Copyright (C) 2007 Vincent ROGIER <vince_rogier@yahoo.fr>
 *
 * URL : http://orclib.sourceforge.net
 *
 * LICENSE :
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ *
 * $Id: oci_api.h, v 1.5.0 10:24 04/01/2008 Vince $
 * ------------------------------------------------------------------------ */

#ifndef OCILIB_OCI_API_H_INCLUDED
#define OCILIB_OCI_API_H_INCLUDED

#include "oci_def.h"

//
// oci api protoptypes
//
typedef sword (*OCIENVCREATE)(
    OCIEnv **envhpp,
    ub4 mode,
    void *ctxp,
    void *(*malocfp)(void *ctxp, size_t size),
    void *(*ralocfp)(void *ctxp, void *memptr, size_t newsize),
    void  (*mfreefp)(void *ctxp, void *memptr),
    size_t xtramem_sz,
    void **usrmempp);
typedef sword (*OCIHANDLEALLOC)(
    const void *parenth,
    void **hndlpp,
    const ub4 type,
    const size_t xtramem_sz,
    void **usrmempp);
typedef sword (*OCIHANDLEFREE)(
    void *hndlp,
    const ub4 type);
typedef sword (*OCIDESCRIPTORALLOC)(
    const void *parenth,
    void **descpp,
    const ub4 type,
    const size_t xtramem_sz,
    void **usrmempp);
typedef sword (*OCIDESCRIPTORFREE)(
    void *descp,
    const ub4 type);
typedef sword (*OCIENVINIT)(
    OCIEnv **envp, ub4 mode,
    size_t xtramem_sz,
    void **usrmempp);
typedef sword (*OCISERVERATTACH)(
    OCIServer *srvhp,
    OCIError *errhp,
    const OraText *dblink,
    sb4 dblink_len,
    ub4 mode);
typedef sword (*OCISERVERDETACH)(
    OCIServer *srvhp,
    OCIError *errhp,
    ub4 mode);
typedef sword (*OCISESSIONBEGIN)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCISession *usrhp,
    ub4 credt,
    ub4 mode);
typedef sword (*OCISESSIONEND)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCISession *usrhp,
    ub4 mode);
typedef sword (*OCISTMTPREPARE)(
    OCIStmt *stmtp,
    OCIError *errhp,
    const OraText *stmt,
    ub4 stmt_len,
    ub4 language,
    ub4 mode);
typedef sword (*OCIBINDBYPOS)(
    OCIStmt *stmtp,
    OCIBind **bindp,
    OCIError *errhp,
    ub4 position,
    void *valuep,
    sb4 value_sz,
    ub2 dty,
    void *indp,
    ub2 *alenp,
    ub2 *rcodep,
    ub4 maxarr_len,
    ub4 *curelep,
    ub4 mode);
typedef sword (*OCIBINDBYNAME)(
    OCIStmt *stmtp,
    OCIBind **bindp,
    OCIError *errhp,
    const OraText *placeholder,
    sb4 placeh_len,
    void *valuep,
    sb4 value_sz,
    ub2 dty,
    void *indp,
    ub2 *alenp,
    ub2 *rcodep,
    ub4 maxarr_len,
    ub4 *curelep,
    ub4 mode);
typedef sword (*OCISTMTGETPIECEINFO)(
    OCIStmt *stmtp,
    OCIError *errhp,
    void **hndlpp,
    ub4 *typep,
    ub1 *in_outp,
    ub4 *iterp,
    ub4 *idxp,
    ub1 *piecep);
typedef sword (*OCISTMTSETPIECEINFO)(
    void *hndlp,
    ub4 type,
    OCIError *errhp,
    const void *bufp,
    ub4 *alenp,
    ub1 piece,
    const void *indp,
    ub2 *rcodep);
typedef sword (*OCISTMTEXECUTE)(
    OCISvcCtx *svchp,
    OCIStmt *stmtp,
    OCIError *errhp,
    ub4 iters,
    ub4 rowoff,
    const OCISnapshot *snap_in,
    OCISnapshot *snap_out,
    ub4 mode);
typedef sword (*OCIDEFINEBYPOS)(
    OCIStmt *stmtp,
    OCIDefine **defnp,
    OCIError *errhp,
    ub4 position,
    void *valuep,
    sb4 value_sz,
    ub2 dty,
    void *indp,
    ub2 *rlenp,
    ub2 *rcodep,
    ub4 mode);
typedef sword (*OCISTMTFETCH )(
    OCIStmt *stmtp,
    OCIError *errhp,
    ub4 nrows,
    ub2 orientation,
    ub4 mode);
typedef sword (*OCIPARAMGET)(
    const void *hndlp,
    ub4 htype,
    OCIError *errhp,
    void **parmdpp,
    ub4 pos);
typedef sword (*OCIPARAMSET)(
    void *hdlp,
    ub4 htyp,
    OCIError *errhp,
    const void *dscp,
    ub4 dtyp,
    ub4 pos);
typedef sword (*OCITRANSSTART)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    uword timeout,
    ub4 flags);
typedef sword (*OCITRANSDETACH)(
    OCISvcCtx    *svchp,
    OCIError     *errhp,
    ub4          flags);
typedef sword (*OCITRANSPREPARE)(
    OCISvcCtx    *svchp,
    OCIError     *errhp,
    ub4          flags);
typedef sword (*OCITRANSFORGET)(
    OCISvcCtx     *svchp,
    OCIError      *errhp,
    ub4           flags);
typedef sword (*OCITRANSCOMMIT)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    ub4 flags);
typedef sword (*OCITRANSROLLBACK)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    ub4 flags);
typedef sword (*OCIERRORGET)(
    void *hndlp,
    ub4 recordno,
    OraText *sqlstate,
    sb4 *errcodep,
    OraText *bufp,
    ub4 bufsiz,
    ub4 type);
typedef sword (*OCILOBCREATETEMPORARY)(
    OCISvcCtx          *svchp,
    OCIError           *errhp,
    OCILobLocator      *locp,
    ub2                 csid,
    ub1                 csfrm,
    ub1                 lobtype,
    boolean             cache,
    OCIDuration         duration);
typedef sword (*OCILOBFREETEMPORARY)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *locp);
typedef sword (*OCILOBISTEMPORARY)(
    OCIEnv *envp,
    OCIError *errhp,
    OCILobLocator *locp,
    boolean *is_temporary);
typedef sword (*OCILOBAPPEND)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *dst_locp,
    OCILobLocator *src_locp);
typedef sword (*OCILOBCOPY)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *dst_locp,
    OCILobLocator *src_locp,
    ub4 amount,
    ub4 dst_offset,
    ub4 src_offset);
typedef sword (*OCILOBREAD)  (
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *locp,
    ub4 *amtp,
    ub4 offset,
    void *bufp,
    ub4 bufl,
    void *ctxp,
    sb4 (*cbfp)(void *ctxp, const void *bufp, ub4 len, ub1 piece),
    ub2 csid,
    ub1 csfrm);
typedef sword (*OCILOBTRIM)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *locp,
    ub4 newlen);
typedef sword (*OCILOBERASE)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *locp,
    ub4 *amount,
    ub4 offset);
typedef sword (*OCILOBWRITE)  (
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *locp,
    ub4 *amtp,
    ub4 offset,
    void *bufp,
    ub4 buflen,
    ub1 piece,
    void *ctxp,
    sb4 (*cbfp)(void *ctxp, void *bufp, ub4 *len,  ub1 *piece),
    ub2 csid,
    ub1 csfrm);
typedef sword (*OCILOBGETLENGTH)  (
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *locp,
    ub4 *lenp);
typedef sword (*OCILOBOPEN)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *locp,
    ub1 mode);
typedef sword (*OCILOBCLOSE)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *locp);
typedef sword (*OCILOBFILEOPEN)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *filep,
    ub1 mode);
typedef sword (*OCILOBFILECLOSE)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *filep);
typedef sword (*OCILOBFILECLOSEALL)(
    OCISvcCtx *svchp,
    OCIError *errhp);
typedef sword (*OCILOBFILEISOPEN)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *filep,
    boolean *flag);
typedef sword (*OCILOBFILEEXISTS)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *filep,
    boolean *flag);
typedef sword (*OCILOBFIELGETNAME)(
    OCIEnv *envhp,
    OCIError *errhp,
    CONST OCILobLocator *filep,
    OraText *dir_alias,
    ub2 *d_length,
    OraText *filename,
    ub2 *f_length);
typedef sword (*OCILOBFILESETNAME)(
    OCIEnv *envhp, OCIError *errhp,
    OCILobLocator **filepp,
    CONST OraText *dir_alias,
    ub2 d_length,
    CONST OraText *filename,
    ub2 f_length);
typedef sword (*OCILOBLOADFROMFILE)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *dst_locp,
    OCILobLocator *src_filep,
    ub4 amount,
    ub4 dst_offset,
    ub4 src_offset);
typedef sword (*OCILOBWRITEAPPEND)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    OCILobLocator *lobp,
    ub4 *amtp,
    dvoid *bufp,
    ub4 bufl,
    ub1 piece,
    dvoid *ctxp,
    sb4 (*cbfp)(void *ctxp, void *bufp, ub4 *len,  ub1 *piece),
    ub2 csid,
    ub1 csfrm);
typedef sword (*OCILOBISEQUAL)(
    OCIEnv *envhp,
    CONST OCILobLocator *x,
    CONST OCILobLocator *y,
    boolean *is_equal);
typedef sword (*OCISERVERVERSION)  (
    void *hndlp,
    OCIError *errhp,
    OraText *bufp,
    ub4 bufsz,
    ub1 hndltype);
typedef sword (*OCIATTRGET)(
    const void *trgthndlp,
    ub4 trghndltyp,
    void *attributep,
    ub4 *sizep, ub4 attrtype,
    OCIError *errhp);
typedef sword (*OCIATTRSET)(
    void *trgthndlp,
    ub4 trghndltyp,
    void *attributep,
    ub4 size,
    ub4 attrtype,
    OCIError *errhp);
typedef sword (*OCIDATEASSIGN)(
    OCIError *err,
    CONST OCIDate *from,
    OCIDate *to);
typedef sword (*OCIDATETOTEXT)(
    OCIError *err,
    CONST OCIDate *date,
    CONST text *fmt,
    ub1 fmt_length,
    CONST text *lang_name,
    ub4 lang_length,
    ub4 *buf_size,
    text *buf);
typedef sword (*OCIDATEFROMTEXT)(
    OCIError *err,
    CONST text *date_str,
    ub4 d_str_length,
    CONST text *fmt,
    ub1 fmt_length,
    CONST text *lang_name,
    ub4 lang_length,
    OCIDate *date);
typedef sword (*OCIDATECOMPARE)(
    OCIError *err,
    CONST OCIDate *date1,
    CONST OCIDate *date2,
    sword *result);
typedef sword (*OCIDATEADDMONTHS)(
    OCIError *err,
    CONST OCIDate *date,
    sb4 num_months,
    OCIDate *result);
typedef sword (*OCIDATEADDDAYS)(
    OCIError *err,
    CONST OCIDate *date,
    sb4 num_days,
    OCIDate *result);
typedef sword (*OCIDATELASTDAY)(
    OCIError *err,
    CONST OCIDate *date,
    OCIDate *last_day);
typedef sword (*OCIDATEDAYSBETWEEN)(
    OCIError *err,
    CONST OCIDate *date1,
    CONST OCIDate *date2,
    sb4 *num_days);
typedef sword (*OCIDATEZONETOZONE)(
    OCIError *err,
    CONST OCIDate *date1,
    CONST text *zon1,
    ub4 zon1_length,
    CONST text *zon2,
    ub4 zon2_length,
    OCIDate *date2);
typedef sword (*OCIDATENEXTDAY)(
    OCIError *err,
    CONST OCIDate *date,
    CONST text *day_p,
    ub4 day_length,
    OCIDate *next_day);
typedef sword (*OCIDATECHECK)(
    OCIError *err,
    CONST OCIDate *date,
    uword *valid);
typedef sword (*OCIDATESYSDATE)(
    OCIError *err,
    OCIDate *sys_date);
typedef sword (*OCIDESCRIBEANY)(
    OCISvcCtx *svchp,
    OCIError *errhp,
    dvoid *objptr,
    ub4 objnm_len,
    ub1 objptr_typ,
    ub1 info_level,
    ub1 objtyp,
    OCIDescribe *dschp);
typedef sword (*OCIINTERVALASSIGN)(
    dvoid *hndl,
    OCIError *err,
    CONST OCIInterval *inpinter,
    OCIInterval *outinter);
typedef sword (*OCIINTERVALCHECK)(
    dvoid *hndl,
    OCIError *err,
    CONST OCIInterval *interval,
    ub4 *valid);
typedef sword (*OCIINTERVALCOMPARE)(
    dvoid *hndl,
    OCIError *err,
    OCIInterval *inter1,
    OCIInterval *inter2,
    sword *result);
typedef sword (*OCIINTERVALTOTEXT)(
    dvoid *hndl,
    OCIError *err,
    CONST OCIInterval *interval,
    ub1 lfprec,
    ub1 fsprec,
    OraText *buffer,
    size_t buflen,
    size_t *resultlen);
typedef sword (*OCIINTERVALFROMTEXT)(
    dvoid *hndl,
    OCIError *err,
    CONST OraText *inpstring,
    size_t str_len,
    OCIInterval *result);
typedef sword (*OCIINTERVALFROMTZ)(
    dvoid *hndl,
    OCIError *err,
    CONST oratext *inpstring,
    size_t str_len,
    OCIInterval *result);
typedef sword (*OCIINTERVALGETDAYSECOND)(
    dvoid *hndl,
    OCIError *err,
    sb4 *dy,
    sb4 *hr,
    sb4 *mm,
    sb4 *ss,
    sb4 *fsec,
    CONST OCIInterval *interval);
typedef sword (*OCIINTERVALGETYEARMONTH)(
    dvoid *hndl,
    OCIError *err,
    sb4 *yr,
    sb4 *mnth,
    CONST OCIInterval *interval);
typedef sword (*OCIINTERVALSETDAYSECOND)(
    dvoid *hndl,
    OCIError *err,
    sb4 dy,
    sb4 hr,
    sb4 mm,
    sb4 ss,
    sb4 fsec,
    OCIInterval *result);
typedef sword (*OCIINTERVALSETYEARMONTH)(
    dvoid *hndl,
    OCIError *err,
    sb4 yr,
    sb4 mnth,
    OCIInterval *result);
typedef sword (*OCIINTERVALADD)(
    dvoid *hndl,
    OCIError *err,
    OCIInterval *addend1,
    OCIInterval *addend2,
    OCIInterval *result);
typedef sword (*OCIINTERVALSUBTRACT)(
    dvoid *hndl,
    OCIError *err,
    OCIInterval *minuend,
    OCIInterval *subtrahend,
    OCIInterval *result);
typedef sword (*OCIDATETIMEASSIGN)(
    dvoid *hndl,
    OCIError *err,
    CONST OCIDateTime *from,
    OCIDateTime *to);
typedef sword (*OCIDATETIMECHECK)(
    dvoid *hndl,
    OCIError *err,
    CONST OCIDateTime *date,
    ub4 *valid);
typedef sword (*OCIDATETIMECOMPARE)(
    dvoid *hndl,
    OCIError *err,
    CONST OCIDateTime *date1,
    CONST OCIDateTime *date2,
    sword *result);
typedef sword (*OCIDATETIMECONSTRUCT)(
    dvoid *hndl,
    OCIError *err,
    OCIDateTime *datetime,
    sb2 year,
    ub1 month,
    ub1 day,
    ub1 hour,
    ub1 min,
    ub1 sec,
    ub4 fsec,
    OraText *timezone,
    size_t timezone_length);
typedef sword (*OCIDATETIMECONVERT)(
    dvoid *hndl,
    OCIError *err,
    OCIDateTime *indate,
    OCIDateTime *outdate);
typedef sword (*OCIDATETIMEFROMARRAY)(
    dvoid *hndl,
    OCIError *err,
    CONST ub1 *inarray,
    ub4 *len,
    ub1 type,
    OCIDateTime *datetime,
    CONST OCIInterval *reftz,
    ub1 fsprec);
typedef sword (*OCIDATETIMETOARRAY)(
    dvoid *hndl,
    OCIError *err,
    CONST OCIDateTime *datetime,
    CONST OCIInterval *reftz,
    ub1 *outarray,
    ub4 *len,
    ub1 fsprec);
typedef sword (*OCIDATETIMEFROMTEXT)(
    dvoid *hndl,
    OCIError *err,
    CONST OraText *date_str,
    size_t dstr_length,
    CONST OraText *fmt,
    ub1 fmt_length,
    CONST OraText *lang_name,
    size_t lang_length,
    OCIDateTime *datetime);
typedef sword (*OCIDATETIMETOTEXT)(
    dvoid *hndl,
    OCIError *err,
    CONST OCIDateTime *date,
    CONST OraText *fmt,
    ub1 fmt_length,
    ub1 fsprec,
    CONST OraText *lang_name,
    size_t lang_length,
    ub4 *buf_size,
    OraText *buf);
typedef sword (*OCIDATETIMEGETDATE)(
    dvoid *hndl,
    OCIError *err,
    CONST OCIDateTime *datetime,
    sb2 *year,
    ub1 *month,
    ub1 *day);
typedef sword (*OCIDATETIMEGETTIME)(
    dvoid *hndl,
    OCIError *err,
    OCIDateTime *datetime,
    ub1 *hour,
    ub1 *min,
    ub1 *sec,
    ub4 *fsec);
typedef sword (*OCIDATETIMEGETTIMEZONENAME)(
    dvoid *hndl,
    OCIError *err,
    CONST OCIDateTime *datetime,
    ub1 *buf,
    ub4 *buflen);
typedef sword (*OCIDATETIMEGETTIMEZONEOFFSET)(
    dvoid *hndl,
    OCIError *err,
    CONST OCIDateTime *datetime,
    sb1 *hour,
    sb1 *min);
typedef sword (*OCIDATETIMEINTERVALADD)(
    dvoid *hndl,
    OCIError *err,
    OCIDateTime *datetime,
    OCIInterval *inter,
    OCIDateTime *outdatetime);
typedef sword (*OCIDATETIMEINTERVALSUB)(
    dvoid *hndl,
    OCIError *err,
    OCIDateTime *datetime,
    OCIInterval *inter,
    OCIDateTime *outdatetime);
typedef sword (*OCIDATETIMESUBTRACT)(
    dvoid *hndl,
    OCIError *err,
    OCIDateTime *indate1,
    OCIDateTime *indate2,
    OCIInterval *inter);
typedef sword (*OCIDATETIMESYSTIMESTAMP)(
    dvoid *hndl,
    OCIError *err,
    OCIDateTime *sys_date);
/* Oracle 10g test */
typedef void (*OCICLIENTVERSION)(
    sword *major_version,
    sword *minor_version,
    sword *update_num,
    sword *patch_num,
    sword *port_update_num);
/* Oracle 11g test */
typedef sword (*OCIARRAYDESCRIPTORFREE)(
    void  **descp,
    const ub4 type);

#endif    /* OCILIB_OCI_API_H_INCLUDED */

