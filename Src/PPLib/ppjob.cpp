// PPJOB.CPP
// Copyright (c) A.Sobolev 2005, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop
#include <comdef.h>		// COM для WMI
#include <wbemidl.h>	// WMI для удаленного запуска процессов
#include <charry.h>

#define JOB_FACTORY_PRFX JFF_

int FASTCALL WriteParam(SBuffer & rBuf, const void * pParam, size_t paramSize)
{
	return rBuf.Write(pParam, paramSize) ? 1 : PPSetErrorSLib();
}

int FASTCALL ReadParam(SBuffer & rBuf, void * pParam, size_t paramSize)
{
	int    ok = -1;
	if(rBuf.GetAvailableSize())
		if(rBuf.Read(pParam, paramSize))
			ok = 1;
		else
			ok = PPSetErrorSLib();
	return ok;
}
//
//
PPJobHandler * PPJobMngr::CreateInstance(PPID jobID, const PPJobDescr * pDescr)
{
	PPJobHandler * p_jh = 0;
	PPJobDescr jd;
	SString ffn;
	if(!RVALUEPTR(jd, pDescr))
		THROW(LoadResource(jobID, &jd));
	PPSetAddedMsgString(jd.Text);
	FN_JOB_FACTORY f = reinterpret_cast<FN_JOB_FACTORY>(GetProcAddress(SLS.GetHInst(), jd.GetFactoryFuncName(ffn)));
	THROW(f);
	THROW(p_jh = f(&jd));
	CATCH
		PPSetError(PPERR_JOBUNIMPL);
		p_jh = 0;
	ENDCATCH
	return p_jh;
}

int PPJobMngr::GetXmlPoolDir(SString &rXmlPoolPath)
{
	int    ok = 1;
	THROW(PPGetFilePath(PPPATH_WORKSPACE, "srvjobpool", rXmlPoolPath));  // получаем путь к workspace
	THROW(SFile::CreateDir(rXmlPoolPath));
	CATCHZOK
	return ok;
}

//int PPJobMngr::DoJob(PPID jobID, const PPJob & rJob, SBuffer * pParam)
int PPJobMngr::DoJob(PPJob & rJob)
{
	PPJobHandler * p_jobh = CreateInstance(rJob.Descr.CmdID, 0);
	if(p_jobh) {
		p_jobh->SetJobDbSymb(rJob.DbSymb);
		return p_jobh->Run(&rJob.Param, 0);
	}
	else
		return 0;
}

int PPJobMngr::EditJobParam(PPJob & rJob)
{
	PPJobHandler * p_jobh = CreateInstance(rJob.Descr.CmdID, 0);
	if(p_jobh) {
		p_jobh->SetJobDbSymb(rJob.DbSymb); // @v11.0.0
		return p_jobh->EditParam(&rJob.Param, 0);
	}
	else
		return 0;
}
//
//
//
PPJobMngr::PPJobMngr() : LckH(0), LastId(0), /*P_F(0),*/LastLoading(ZERODATETIME)
{
	SString name;
	P_Rez = new TVRez(makeExecPathFileName("pp", "res", name), 1);
	// @v12.3.1 (@obsolete) PPGetFilePath(PPPATH_BIN, PPFILNAM_JOBPOOL, FilePath);
//@erik v10.7.4
	GetXmlPoolDir(XmlFilePath);
	Sync.Init(XmlFilePath);
	XmlFilePath.SetLastSlash().Cat("ppjobpool").DotCat("xml");
// } @erik
}

PPJobMngr::~PPJobMngr()
{
	CloseFile();
	delete P_Rez;
}

long PPJobMngr::AcquireNewId()
{
	return ++LastId;
}

void FASTCALL PPJobMngr::UpdateLastId(long id)
{
	assert(id >= LastId);
	LastId = id;
}

void PPJobMngr::CloseFile()
{
	/* @v12.3.1 (@obsolete) 
	if(P_F) {
		if(LckH) {
			P_F->Unlock(LckH);
			LckH = 0;
		}
		ZDELETE(P_F);
	}*/
	if(Sync.IsMyLock(LConfig.SessionID, PPCFGOBJ_JOBPOOL, 1)) {
		Sync.ReleaseMutex(PPCFGOBJ_JOBPOOL, 1);
	}
}

/* @v12.3.1 (@obsolete) int PPJobMngr::ConvertBinToXml()
{
	int    ok = 1;
	PPJobPool pool(this, 0, 0);
	THROW(LoadPool(0, &pool, 1));
	THROW(Sync.CreateMutex_(LConfig.SessionID, PPCFGOBJ_JOBPOOL, 1, 0, 0));
	if(!SavePool2(&pool))
		ok = 0;
	Sync.ReleaseMutex(PPCFGOBJ_JOBPOOL, 1);
	CATCHZOK
	return ok;
}*/

int PPJobMngr::LoadResource(PPID jobID, PPJobDescr * pJob)
{
	int    ok = 1;
	pJob->CmdID = jobID;
	if(P_Rez) {
		THROW_PP(P_Rez->findResource(static_cast<uint>(jobID), PP_RCDECLJOB), PPERR_RESFAULT);
		P_Rez->getString(pJob->Symb, 2);
		P_Rez->getString(pJob->Text = 0, 2);
		SLS.ExpandString(pJob->Text, CTRANSF_UTF8_TO_INNER);
		pJob->Flags = static_cast<long>(P_Rez->getUINT());
	}
	CATCHZOK
	return ok;
}

int PPJobMngr::GetResourceList(int loadText, StrAssocArray & rList)
{
	int    ok = 1;
	rList.Z();
	if(P_Rez) {
		ulong pos = 0;
		for(uint   rsc_id = 0; P_Rez->enumResources(PP_RCDECLJOB, &rsc_id, &pos) > 0;) {
			PPJobDescr job;
			THROW(LoadResource(rsc_id, &job));
			THROW_SL(rList.Add(job.CmdID, loadText ? job.Text : job.Symb));
		}
	}
	CATCHZOK
	return ok;
}

#define JOBSTRGSIGN 'SJPP'

struct JobStrgHeader { // @persistent @size=64
	long   Signature;      // const=JOBSTRGSIGN
	uint32 Locking;        // @v7.7.9 Отрезок, блокируемый для предотвращения множественного доступа на изменение
	SVerT  Ver;            // @anchor Версия сессии, создавшей файл
	uint32 Count;          // Количество задач
	int32  LastId;         // @v7.7.9
	char   Reserve2[44];
};

/* @v12.3.1 (@obsolete) int PPJobMngr::Helper_ReadHeader(SFile & rF, void * pHdr, int lockMode)
{
	int    ok = 1;
	uint32 locking_stub = 0;
	JobStrgHeader * p_hdr = static_cast<JobStrgHeader *>(pHdr);
	p_hdr->Locking = 0;
	THROW_SL(rF.Seek(0, SEEK_SET));
	THROW_SL(rF.Read(&p_hdr->Signature, sizeof(p_hdr->Signature)));
	THROW_PP(p_hdr->Signature == JOBSTRGSIGN, PPERR_JOBSTRGCORRUPTED);
	//
	// 4 байта, отведенные под блокировку не читаем (они могут быть заблокированы), но перемещеам курсор
	// на эти 4 байта вперед (SEEK_CUR)
	//
	THROW_SL(rF.Seek(sizeof(p_hdr->Locking), SEEK_CUR));
	THROW_SL(rF.Read(&p_hdr->Ver, sizeof(*p_hdr)-offsetof(JobStrgHeader, Ver)));
	if(lockMode > 0) {
		LckH = rF.Lock(offsetof(JobStrgHeader, Locking), sizeof(p_hdr->Locking));
		if(LckH == 0) {
			if(SLibError == SLERR_FLOCKFAULT) {
				CALLEXCEPT_PP(PPERR_JOBPOOLLOCKED);
			}
			else {
				CALLEXCEPT_PP(PPERR_SLIB);
			}
		}
		else
			p_hdr->Locking = 1;
	}
	//
	// Блокировка могла переместить (и, скорее всего, так и сделала) текущую позицию файла:
	// устанавливаем ее на следующий байт за заголовком.
	//
	THROW_SL(rF.Seek(sizeof(*p_hdr), SEEK_SET));
	CATCHZOK
	return ok;
}*/

/* @v12.3.1 (@obsolete) int PPJobMngr::LoadPool(const char * pDbSymb, PPJobPool * pPool, int readOnly)
{
	int    ok = 1;
	assert(!P_F || LckH);
	CloseFile();
	pPool->Flags &= ~PPJobPool::fReadOnly;
	pPool->freeAll();
	if(!fileExists(FilePath)) {
		THROW(CreatePool());
	}
	{
		SFile  f;
		JobStrgHeader hdr;
		SBuffer buf;
		SFile * p_file = 0;
		int    sfmode = SFile::mBinary|SFile::mNoStd;
		if(readOnly) {
			p_file = &f;
			sfmode |= SFile::mRead;
		}
		else {
			THROW_MEM(P_F = new SFile());
			p_file = P_F;
			sfmode |= SFile::mReadWrite;
		}
		THROW_SL(p_file->Open(FilePath, sfmode));
		THROW(Helper_ReadHeader(*p_file, &hdr, readOnly ? 0 : 1));
		LastId = hdr.LastId;
		for(uint i = 0; i < hdr.Count; i++) {
			PPID   id = 0;
			PPJob  job;
			THROW_SL(p_file->Read(buf.Z()));
			THROW(job.Read(buf));
			id = job.ID;
			THROW(pPool->PutJobItem(&id, &job));
		}
	}
	LastLoading = getcurdatetime_();
	pPool->DbSymb = pDbSymb;
	CATCH
		CloseFile();
		ok = 0;
	ENDCATCH
	SETFLAG(pPool->Flags, PPJobPool::fReadOnly, readOnly);
	return ok;
}*/

//@erik v10.7.4
int PPJobMngr::LoadPool2(const char * pDbSymb, PPJobPool * pPool, bool readOnly)
{
	int    ok = 1;
	xmlParserCtxt * p_xml_parser = 0;
	xmlDoc  * p_doc = 0;
	JobStrgHeader hdr;
	SString temp_buf;
	const long session_id = LConfig.SessionID;
	pPool->Flags &= ~PPJobPool::fReadOnly;
	pPool->freeAll();
	/* @v12.3.1 @obsolete
	if(!fileExists(XmlFilePath)) {
		SString fmt_buf;
		SString msg_buf;
		// PPTXT_LOG_TRYTOCONVERTJOBPOOL       "xml-файл пула задач '%s' не найден - попытка найти и сконвертировать пул в старом формате '%s'"
		PPLoadText(PPTXT_LOG_TRYTOCONVERTJOBPOOL, fmt_buf);
		msg_buf.Printf(fmt_buf, XmlFilePath.cptr(), FilePath.cptr());
		PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_COMP);
		THROW(ConvertBinToXml());
	}*/
	if(!readOnly) {
		THROW(Sync.CreateMutex_(session_id, PPCFGOBJ_JOBPOOL, 1, 0, 0));
	}
	if(readOnly || Sync.IsMyLock(session_id, PPCFGOBJ_JOBPOOL, 1) > 0) {
		THROW_SL(fileExists(XmlFilePath));
		THROW(p_xml_parser = xmlNewParserCtxt());
		THROW_LXML(p_doc = xmlCtxtReadFile(p_xml_parser, XmlFilePath, 0, XML_PARSE_NOENT), p_xml_parser);
		for(const xmlNode * p_root = p_doc->children; p_root; p_root = p_root->next) {
			if(SXml::IsName(p_root, "JobPool")) {
				for(const xmlNode * p_node = p_root->children; p_node; p_node = p_node->next) {
					if(SXml::IsName(p_node, "JobStrgHeader")) {
						for(const xmlNode * p_hdr_node = p_node->children; p_hdr_node; p_hdr_node = p_hdr_node->next) {
							if(SXml::GetContentByName(p_hdr_node, "Count", temp_buf))
								hdr.Count = temp_buf.ToULong();
							else if(SXml::GetContentByName(p_hdr_node, "Locking", temp_buf))
								hdr.Locking = temp_buf.ToULong();
							else if(SXml::GetContentByName(p_hdr_node, "LastID", temp_buf))
								hdr.LastId = temp_buf.ToLong();
							else if(SXml::GetContentByName(p_hdr_node, "PpyVersion", temp_buf)) {
								hdr.Ver.FromStr(temp_buf);
							}
						}
						hdr.Signature = JOBSTRGSIGN;
						LastId = hdr.LastId;
					}
					else if(SXml::IsName(p_node, "PPJob")) {
						PPID   id = 0;
						PPJob  job;
						THROW(job.Read2(p_node));
						id = job.ID;
						THROW(pPool->PutJobItem(&id, &job));
					}
				}
			}
		}
		LastLoading = getcurdatetime_();
		pPool->DbSymb = pDbSymb;
	}
	else {
		CALLEXCEPT_PP(PPERR_JOBPOOLLOCKED);
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
	xmlFreeParserCtxt(p_xml_parser);
	SETFLAG(pPool->Flags, PPJobPool::fReadOnly, readOnly);
	return ok;
}

DirChangeNotification * PPJobMngr::CreateDcn()
{
	SString path;
	return PPGetFilePath(PPPATH_WORKSPACE, "srvjobpool", path)? new DirChangeNotification(path, 0, FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_FILE_NAME) : 0;
}

int PPJobMngr::IsPoolChanged() const
{
	int    ok = 0;
	if(LastLoading.d) {
		SFile::Stat fs;
		// @v11.8.11 {
		if(/* @v12.3.1 (obsolete) SFile::GetStat(FilePath, 0, &fs, 0) &&*/SFile::GetStat(XmlFilePath, 0, &fs, 0)) {
			LDATETIME fs_dtm;
			if(cmp(fs_dtm.SetNs100_AdjToTimezone(fs.ModTm_), LastLoading) > 0) { // @v12.3.1 SetNs100-->SetNs100_AdjToTimezone
				ok = 1; // Файл был модифицирован
			}
		}
		else {
			ok = 1; // Файл был удален, либо не появился
		}
		// } @v11.8.11 
		/* @v11.8.11 {
		if(!SFile::GetStat(FilePath, 0, &fs, 0) ||!SFile::GetStat(XmlFilePath, 0, &fs, 0) || cmp(fs.ModTime, LastLoading) > 0) { // @erik v10.7.4 add ||!SFile::GetStat(XmlFilePath, &fs)
			//
			// Файл был удален, либо не появился, либо был модифирован.
			//
			ok = 1;
		}*/
	}
	else
		ok = -1;
	return ok;
}

/*@v12.3.1 (@obsolete) int PPJobMngr::CreatePool()
{
	int    ok = 1;
	if(!fileExists(FilePath)) {
		SFile f;
		JobStrgHeader hdr;
		SBuffer buf;
		MEMSZERO(hdr);
		hdr.Signature = JOBSTRGSIGN;
		hdr.Count = 0;
		hdr.LastId = LastId;
		hdr.Ver = DS.GetVersion();
		int    sfmode = SFile::mBinary|SFile::mNoStd|SFile::mWrite;
		THROW_SL(f.Open(FilePath, sfmode));
		THROW_SL(f.Seek(0, SEEK_SET));
		THROW_SL(f.Write(&hdr, sizeof(hdr)))
	}
	else {
		ok = -1;
	}		
	CATCHZOK
	return ok;
}*/

/*@v12.3.1 (@obsolete) int PPJobMngr::SavePool(const PPJobPool * pPool)
{
	int ok = 1;
	assert(!P_F || LckH);
	JobStrgHeader hdr;
	SBuffer buf;
	THROW_PP(P_F && LckH, PPERR_JOBPOOLNOPENWR);
	MEMSZERO(hdr);
	hdr.Signature = JOBSTRGSIGN;
	hdr.Count = pPool->getCount();
	hdr.LastId = LastId;
	hdr.Ver = DS.GetVersion();
	{
		//
		// Снимаем блокировку непосредственно перед записью.
		// Ненулевое значение LckH уже проверено выше.
		//
		THROW_SL(P_F->Unlock(LckH));
		LckH = 0;
	}
	THROW_SL(P_F->Seek(0, SEEK_SET));
	THROW_SL(P_F->Write(&hdr, sizeof(hdr)));
	for(uint i = 0; i < hdr.Count; i++) {
		buf.Z();
		PPJob * p_job = pPool->at(i);
		THROW(p_job->Write(buf));
		THROW_SL(P_F->Write(buf));
	}
	//
	// Сразу после записи снова блокируем файл (быть может клиент захочет снова что-то поменять и сохранить)
	//
	THROW(Helper_ReadHeader(*P_F, &hdr, 1));
	CATCHZOK
	return ok;
}*/

//@erik v10.7.0
int PPJobMngr::SavePool2(const PPJobPool * pPool)
{
	int    ok = 1;
	JobStrgHeader hdr;
	xmlTextWriter * p_xml_writer = 0;
	SString temp_buf;
	MEMSZERO(hdr);
	hdr.Count = pPool->getCount();
	hdr.LastId = LastId;
	hdr.Ver = DS.GetVersion();
	if(Sync.IsMyLock(LConfig.SessionID, PPCFGOBJ_JOBPOOL, 1) > 0) {
		THROW_SL(p_xml_writer = xmlNewTextWriterFilename(XmlFilePath, 0));
		xmlTextWriterSetIndent(p_xml_writer, 1);
		xmlTextWriterSetIndentTab(p_xml_writer);
		SXml::WDoc _doc(p_xml_writer, cpUTF8);
		{
			SXml::WNode root_node(p_xml_writer, "JobPool");
			{
				SXml::WNode xml_job_pool_hdr(p_xml_writer, "JobStrgHeader");
				xml_job_pool_hdr.PutInner("Count", temp_buf.Z().Cat(hdr.Count));
				xml_job_pool_hdr.PutInner("LastID", temp_buf.Z().Cat(hdr.LastId));
				xml_job_pool_hdr.PutInner("PpyVersion", hdr.Ver.ToStr(temp_buf.Z()));
			}
			for(uint i = 0; i < hdr.Count; i++) {
				const PPJob * p_job = pPool->at(i);
				p_job->Write2(p_xml_writer);
			}
		}
	}	
	CATCHZOK
	xmlFreeTextWriter(p_xml_writer);
	return ok;
}

//
//
//
const char * PPJobDescr::P_FactoryPrfx = "JFF_";

PPJobDescr::PPJobDescr()
{
	THISZERO();
}

SString & FASTCALL PPJobDescr::GetFactoryFuncName(SString & rBuf) const
{
	(rBuf = P_FactoryPrfx).Cat(Symb).ToUpper();
	return rBuf;
}

int FASTCALL PPJobDescr::Write(SBuffer & rBuf) const
{
	if(rBuf.Write(CmdID) &&
		rBuf.Write(Flags) &&
		rBuf.Write(Reserve, sizeof(Reserve)) &&
		rBuf.Write(Symb) &&
		rBuf.Write(Text))
		return 1;
	else
		return PPSetErrorSLib();
}

int FASTCALL PPJobDescr::Read(SBuffer & rBuf)
{
	if(rBuf.Read(CmdID) &&
		rBuf.Read(Flags) &&
		rBuf.Read(Reserve, sizeof(Reserve)) &&
		rBuf.Read(Symb) &&
		rBuf.Read(Text))
		return 1;
	else
		return PPSetErrorSLib();
}

int FASTCALL PPJobDescr::Write2(xmlTextWriter * pXmlWriter) const //@erik v10.7.1
{
	int ok = 1;
	SString temp_buf;
	assert(pXmlWriter);
	THROW(pXmlWriter);
	{
		SXml::WNode pp_job_node(pXmlWriter, "PPJobDescr");
		pp_job_node.PutInner("CmdID", temp_buf.Z().Cat(CmdID));
		pp_job_node.PutInner("Flags", temp_buf.Z().Cat(Flags));
		XMLReplaceSpecSymb(temp_buf.Z().Cat(Symb), "&<>\'");
		temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
		pp_job_node.PutInner("Symb", temp_buf);
		XMLReplaceSpecSymb(temp_buf.Z().Cat(Text), "&<>\'");
		temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
		pp_job_node.PutInner("Text", temp_buf);
	}
	CATCHZOK;
	return ok;
}

int FASTCALL PPJobDescr::Read2(const xmlNode * pParentNode) //@erik v10.7.1
{
	int    ok = 1;
	SString temp_buf;
	{
		for(const xmlNode * p_node = pParentNode->children; p_node; p_node = p_node->next) {
			if(SXml::GetContentByName(p_node, "CmdID", temp_buf)!=0) {
				CmdID = temp_buf.ToLong();
			}
			else if(SXml::GetContentByName(p_node, "Flags", temp_buf)) {
				Flags = temp_buf.ToLong();
			}
			else if(SXml::GetContentByName(p_node, "Symb", temp_buf)) {
				Symb = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			}
			else if(SXml::GetContentByName(p_node, "Text", temp_buf)) {
				Text = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			}
		}
	}
	return ok;
}
//
//
//
PPJob::PPJob() : ID(0), Flags(0), EstimatedTime(0), NextJobID(0), EmailAccID(0), ScheduleBeforeTime(ZEROTIME), LastRunningTime(ZERODATETIME)
{
	MEMSZERO(Dtr);
	Ver = 1;
	memzero(Symb, sizeof(Symb));
	memzero(Reserve, sizeof(Reserve));
}

PPJob::PPJob(const PPJob & rS) : PPExtStrContainer(rS), ID(rS.ID), Flags(rS.Flags), EstimatedTime(rS.EstimatedTime), NextJobID(rS.NextJobID), 
	EmailAccID(rS.EmailAccID), ScheduleBeforeTime(rS.ScheduleBeforeTime), LastRunningTime(rS.LastRunningTime),
	Dtr(rS.Dtr), Ver(rS.Ver), Name(rS.Name), Descr(rS.Descr), DbSymb(rS.DbSymb), Param(rS.Param)
{
	STRNSCPY(Symb, rS.Symb);
	memzero(Reserve, sizeof(Reserve));
}

PPJob & FASTCALL PPJob::operator = (const PPJob & s)
{
	ID      = s.ID;
	Name    = s.Name;
	Descr   = s.Descr;
	DbSymb  = s.DbSymb;
	Dtr     = s.Dtr;
	Flags   = s.Flags;
	EstimatedTime   = s.EstimatedTime;
	LastRunningTime = s.LastRunningTime;
	NextJobID       = s.NextJobID;
	Ver     = s.Ver;
	EmailAccID      = s.EmailAccID;
	ScheduleBeforeTime = s.ScheduleBeforeTime;
	STRNSCPY(Symb, s.Symb);
	ExtString = s.ExtString;
	Param = s.Param;
	return *this;
}

/*@v12.3.1 (@obsolete) int FASTCALL PPJob::Write(SBuffer & rBuf)
{
	int    ok = 1;
	long   flags = (Flags|fV579);
	Ver = 1;
	THROW(rBuf.Write(ID));
	THROW(rBuf.Write(Name));
	THROW(rBuf.Write(DbSymb));
	THROW(Descr.Write(rBuf));
	THROW(rBuf.Write(&Dtr, sizeof(Dtr)));
	THROW(rBuf.Write(flags));
	THROW(rBuf.Write(EstimatedTime));
	THROW(rBuf.Write(LastRunningTime));
	THROW(rBuf.Write(Ver));
	THROW(rBuf.Write(NextJobID));
	THROW(rBuf.Write(Symb, sizeof(Symb)));
	THROW(rBuf.Write(EmailAccID));
	THROW(rBuf.Write(ScheduleBeforeTime));
	THROW(rBuf.Write(Reserve, sizeof(Reserve)));
	THROW(rBuf.Write(ExtString));
	THROW(rBuf.Write(Param));
	CATCH
		ok = PPSetErrorSLib();
	ENDCATCH
	return ok;
}*/

/*@v12.3.1 (@obsolete) int FASTCALL PPJob::Read(SBuffer & rBuf)
{
	int    ok = 1;
	THROW(rBuf.Read(ID));
	THROW(rBuf.Read(Name));
	THROW(rBuf.Read(DbSymb));
	THROW(Descr.Read(rBuf));
	THROW(rBuf.Read(&Dtr, sizeof(Dtr)));
	THROW(rBuf.Read(Flags));
	THROW(rBuf.Read(EstimatedTime));
	THROW(rBuf.Read(LastRunningTime));
	if(Flags & fV579) {
		THROW(rBuf.Read(Ver));
		THROW(rBuf.Read(NextJobID));
		THROW(rBuf.Read(Symb, sizeof(Symb)));
		THROW(rBuf.Read(EmailAccID));
		THROW(rBuf.Read(ScheduleBeforeTime));
		THROW(rBuf.Read(Reserve, sizeof(Reserve)));
	}
	if(Ver >= 1) {
		THROW(rBuf.Read(ExtString));
	}
	THROW(rBuf.Read(Param))
	CATCH
		ok = PPSetErrorSLib();
	ENDCATCH
	return ok;
}*/

int PPJob::Write2(xmlTextWriter * pXmlWriter) const //@erik v10.7.4
{
	int    ok = 1;
	SString temp_buf;
	SBuffer buf;
	assert(pXmlWriter);
	THROW(pXmlWriter);
	{
		long   flags = (Flags|fV579);
		SXml::WNode pp_job_node(pXmlWriter, "PPJob");
		Descr.Write2(pXmlWriter);
		pp_job_node.PutInner("ID", temp_buf.Z().Cat(ID));
		XMLReplaceSpecSymb(temp_buf.Z().Cat(Name), "&<>\'");
		temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
		pp_job_node.PutInner("Name", temp_buf);
		XMLReplaceSpecSymb(temp_buf.Z().Cat(DbSymb), "&<>\'");
		temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
		pp_job_node.PutInner("DbSymb", temp_buf);
		XMLReplaceSpecSymb(temp_buf.Z().Cat(Symb), "&<>\'");
		temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
		pp_job_node.PutInner("Symb", temp_buf);
		XMLReplaceSpecSymb(temp_buf.Z().Cat(ExtString), "&<>\'");
		temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
		pp_job_node.PutInner("ExtString", temp_buf);
		pp_job_node.PutInner("Flags", temp_buf.Z().Cat(flags));
		pp_job_node.PutInner("EstimatedTime", temp_buf.Z().Cat(EstimatedTime));
		if(!!LastRunningTime) // @v11.3.9
			pp_job_node.PutInner("LastRunningTime", temp_buf.Z().Cat(LastRunningTime, DATF_ISO8601CENT, 0));
		pp_job_node.PutInner("Ver", temp_buf.Z().Cat(Ver));
		pp_job_node.PutInner("NextJobID", temp_buf.Z().Cat(NextJobID));
		pp_job_node.PutInner("EmailAccID", temp_buf.Z().Cat(EmailAccID));
		pp_job_node.PutInner("ScheduleBeforeTime", temp_buf.Z().Cat(ScheduleBeforeTime));
		temp_buf.Z().EncodeMime64(static_cast<const char *>(Param.GetBuf(Param.GetRdOffs())), Param.GetAvailableSize());
		pp_job_node.PutInner("Param", temp_buf);
		buf.Write(&Dtr, sizeof(Dtr));
		temp_buf.Z().EncodeMime64(static_cast<const char *>(buf.GetBuf(buf.GetRdOffs())), buf.GetAvailableSize());
		pp_job_node.PutInner("Dtr", temp_buf);
	}
	CATCHZOK
	return ok;
}

int PPJob::Read2(const xmlNode * pParentNode) //@erik v10.7.1
{
	int    ok = 1;
	SString temp_buf;
	assert(pParentNode);
	{
		for(const xmlNode * p_node = pParentNode->children; p_node; p_node = p_node->next) {
			if(SXml::GetContentByName(p_node, "ID", temp_buf))
				ID = temp_buf.ToLong();
			else if(SXml::GetContentByName(p_node, "Name", temp_buf))
				Name = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			else if(SXml::GetContentByName(p_node, "DbSymb", temp_buf))
				DbSymb = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			else if(SXml::GetContentByName(p_node, "Symb", temp_buf))
				temp_buf.Transf(CTRANSF_UTF8_TO_INNER).CopyTo(Symb, sizeof(Symb));
			else if(SXml::GetContentByName(p_node, "ExtString", temp_buf))
				ExtString = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			else if(SXml::GetContentByName(p_node, "Flags", temp_buf))
				Flags = temp_buf.ToLong();
			else if(SXml::GetContentByName(p_node, "EstimatedTime", temp_buf))
				EstimatedTime = temp_buf.ToLong();
			else if(SXml::GetContentByName(p_node, "Ver", temp_buf))
				Ver = temp_buf.ToLong();
			else if(SXml::GetContentByName(p_node, "NextJobID", temp_buf))
				NextJobID = temp_buf.ToLong();
			else if(SXml::GetContentByName(p_node, "EmailAccID", temp_buf))
				EmailAccID = temp_buf.ToLong();
			else if(SXml::GetContentByName(p_node, "LastRunningTime", temp_buf))
				LastRunningTime.Set(temp_buf, DATF_DMY, TIMF_HMS);
			else if(SXml::GetContentByName(p_node, "ScheduleBeforeTime", temp_buf))
				strtotime(temp_buf, TIMF_HMS, &ScheduleBeforeTime);
			else if(SXml::GetContentByName(p_node, "Param", temp_buf)) {
				STempBuffer bin_buf(temp_buf.Len()*3);
				size_t actual_len = 0;
				temp_buf.DecodeMime64(bin_buf, bin_buf.GetSize(), &actual_len);
				Param.Write(bin_buf, actual_len);
			}
			else if(SXml::GetContentByName(p_node, "Dtr", temp_buf)) {
				SBuffer s_buf;
				STempBuffer bin_buf(temp_buf.Len()*3);
				size_t actual_len = 0;
				temp_buf.DecodeMime64(bin_buf, bin_buf.GetSize(), &actual_len);
				s_buf.Write(bin_buf, actual_len);
				s_buf.Read(&Dtr, sizeof(Dtr));
			}
			else if(SXml::IsName(p_node, "PPJobDescr")) {
				Descr.Read2(p_node);
			}
		}
	}
	//CATCHZOK
	return ok;
}
//
//
//
PPJobPool::PPJobPool(PPJobMngr * pMngr, const char * pDbSymb, int readOnly) : TSCollection <PPJob> (), P_Mngr(pMngr), DbSymb(pDbSymb), Flags(0)
{
	SETFLAG(Flags, fReadOnly, readOnly);
}

const SString & PPJobPool::GetDbSymb() const { return DbSymb; }

bool FASTCALL PPJobPool::IsJobSuited(const PPJob * pJob) const
{
	bool   ok = false;
	if(pJob) {
		if(DbSymb.IsEmpty())
			ok = true;
		else if(DbSymb.CmpNC(pJob->DbSymb) == 0)
			ok = true;
		else {
			ok = LOGIC(PPSetError(PPERR_JOBSTRNGFORPOOL)); // Задачи, которые относятся к другой базе данных в общем случае менять нельзя, но возможны исключения:
			if(pJob->EmailAccID == 0) { // Аккаунт точно ссылается на запись в базе данных - ничего не поделать - задача не наша
				if(pJob->Descr.CmdID == PPJOB_BACKUP) // Резервное копирование не оперирует данными внутри базы данных - с ней можно работать
					ok = true;
				else if(pJob->Descr.Flags & PPJobDescr::fNoLogin) // Задача не требует авторизации
					ok = true;
				else if(pJob->Param.GetAvailableSize() == 0) // Параметры задачи пустые - значит она не ссылается на данные внутри чужой базы данных
					ok = true;
			}
		}
	}
	return ok;
}

uint PPJobPool::GetCount() const
{
	uint   c = 0;
	if(DbSymb.NotEmpty()) {
		for(uint i = 0; i < getCount(); i++)
			if(DbSymb.CmpNC(at(i)->DbSymb) == 0)
				c++;
	}
	else
		c = getCount();
	return c;
}

int PPJobPool::CheckUniqueJob(const PPJob * pJob) const
{
	int    ok = 1;
	for(uint i = 0; i < getCount(); i++) {
		const PPJob * p_job = at(i);
		if(p_job->ID != pJob->ID) {
			THROW_PP_S(p_job->Name.CmpNC(pJob->Name) != 0, PPERR_DUPJOBNAME, p_job->Name);
			if(pJob->Symb[0])
				THROW_PP_S(stricmp(p_job->Symb, pJob->Symb) != 0, PPERR_DUPJOBSYMB, p_job->Symb);
		}
	}
	CATCHZOK
	return ok;
}

int PPJobPool::Enum(PPID * pID, PPJob * pJob, bool ignoreDbSymb) const
{
	for(uint i = 0; i < getCount(); i++) {
		const PPJob * p_job = at(i);
		if((ignoreDbSymb || IsJobSuited(p_job)) && (p_job->ID > *pID)) {
			ASSIGN_PTR(pJob, *p_job);
			(*pID) = p_job->ID;
			return 1;
		}
	}
	return 0;
}

const PPJob * PPJobPool::GetJobItem(PPID id, bool ignoreDbSymb) const
{
	if(id) {
		for(uint i = 0; i < getCount(); i++) {
			const PPJob * p_job = at(i);
			if(p_job->ID == id)
				return (ignoreDbSymb || IsJobSuited(p_job)) ? p_job : 0;
		}
	}
	PPSetError(PPERR_JOBBYIDNFOUND, id);
	return 0;
}

int PPJobPool::PutJobItem(PPID * pID, const PPJob * pJob)
{
	int    ok = -1;
	uint   i = 0;
	PPID   max_id = 0;
	//PPID   potential_id = P_Mngr->AcquireNewId(); // @erik v10.7.5
	THROW_PP((Flags & fReadOnly) == 0, PPERR_JOBPOOLISREADONLY);
	THROW(!pJob || IsJobSuited(pJob));
	if(!pJob && *pID) {
		//
		// Проверка на предмет запрета удаления задачи, на которую ссылаются другие задачи.
		//
		for(i = 0; i < getCount(); i++) {
			const PPJob * p_job = at(i);
			if(p_job->ID != *pID) {
				THROW_PP_S(p_job->NextJobID != *pID, PPERR_RMVREFEDJOB, p_job->Name);
			}
		}
	}
	{
		//
		// Порядок перебора обратный потому, что в цикле может быть удален элемент массива
		//
		i = getCount();
		if(i) do {
			PPJob * p_job = at(--i);
			if(p_job->ID == *pID) {
				THROW(!pJob || IsJobSuited(p_job));
				if(pJob)
					*p_job = *pJob;
				else
					atFree(i);
				ok = 1;
			}
			else if(p_job->ID > max_id)
				max_id = p_job->ID;
		} while(i);
	}
	if(ok < 0 && pJob) {
		PPJob * p_job = new PPJob;
		THROW_MEM(p_job);
		*p_job = *pJob;
		if(*pID)
			p_job->ID = *pID;
		else {
			assert(P_Mngr);
			PPID   potential_id = P_Mngr->AcquireNewId(); // @erik v10.7.5
			//
			// Цикл, призванный гарантировать уникальность нового идентификатора
			//
			for(i = 0; i < getCount();) {
				const PPJob * p_job = at(i);
				if(potential_id == p_job->ID) {
					//
					// Авария: потенциальный идентификатор уже встречается в пуле.
					// Увеличиваем значение potential_id и начинаем все снова (i = 0)
					//
					potential_id++;
					i = 0;
				}
				else
					i++;
			}
			//
			P_Mngr->UpdateLastId(potential_id);
			p_job->ID = potential_id;
		}
		THROW_SL(insert(p_job));
		ASSIGN_PTR(pID, p_job->ID);
		ok = 1;
	}
	CATCHZOK
	return ok;
}
//
//
//
PPJobHandler::PPJobHandler(const PPJobDescr * pJob)
{
	RVALUEPTR(D, pJob);
}

PPJobHandler::~PPJobHandler()
{
}

int PPJobHandler::EditParam(SBuffer * pParam, void * extraPtr) { return CheckParamBuf(pParam, 0); }
int PPJobHandler::Run(SBuffer * pParam, void * extraPtr) { return 1; }
void PPJobHandler::SetJobDbSymb(const char * pJobDbSymb) { (JobDbSymb = pJobDbSymb).Strip(); }

int PPJobHandler::CheckParamBuf(const SBuffer * pBuf, size_t neededSize) const
{
	int    ok = 1;
	if(pBuf == 0)
		ok = PPSetError(PPERR_INVJOBPARAMBUF);
	else {
		size_t avl_size = pBuf->GetAvailableSize();
		if(avl_size < neededSize)
			ok = PPSetError(PPERR_INVJOBPARAMBUF);
	}
	return ok;
}

const PPJobDescr & PPJobHandler::GetDescr() { return D; }
//
//
// prototype @defined(filtrnsm.cpp)
int EditObjReceiveParam(ObjReceiveParam * pParam, int editOptions);

class JOB_HDL_CLS(OBJRECV) : public PPJobHandler {
public:
	JOB_HDL_CLS(OBJRECV)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1, r;
		ObjReceiveParam param;
		param.Init();
		const size_t sav_offs = pParam->GetRdOffs();
		if((r = param.Read(*pParam, 0)) != 0) {
			ok = EditObjReceiveParam(&param, 1);
			if(ok > 0) {
				param.Write(pParam->Z(), 0);
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		ObjReceiveParam param;
		THROW(param.Read(*pParam, 0));
		param.Flags |= ObjReceiveParam::fNonInteractive;
		THROW(PPObjectTransmit::ReceivePackets(&param));
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(OBJRECV);
//
//
//
class JOB_HDL_CLS(DUMMY) : public PPJobHandler {
public:
	JOB_HDL_CLS(DUMMY)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		PPLogMessage(PPFILNAM_INFO_LOG, "Job DUMMY completed", LOGMSGF_TIME);
		return 1;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(DUMMY);
//
//
//
class JOB_HDL_CLS(MAINTAINPRJTASK) : public PPJobHandler {
public:
	JOB_HDL_CLS(MAINTAINPRJTASK)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		PPObjPrjTask todo_obj;
		return BIN(todo_obj.Maintain());
	}
};

IMPLEMENT_JOB_HDL_FACTORY(MAINTAINPRJTASK);
//
//
//
class PPBackupParam {
public:
	PPBackupParam()
	{
	}
	PPBackupParam & Z()
	{
		DBSymb_Unused.Z();
		BuScen.Z();
		return *this;
	}
	int Read(SBuffer & rBuf, long)
	{
		int    ok = -1;
		if(rBuf.GetAvailableSize()) {
			if(rBuf.Read(DBSymb_Unused) && rBuf.Read(&BuScen, sizeof(BuScen)))
				ok = 1;
			else
				ok = PPSetErrorSLib();
		}
		return ok;
	}
	int Write(SBuffer & rBuf, long)
	{
		if(rBuf.Write(DBSymb_Unused) && rBuf.Write(&BuScen, sizeof(BuScen)))
			return 1;
		else
			return PPSetErrorSLib();
	}
	SString DBSymb_Unused; // @v11.0.0
	PPBackupScen BuScen;
};

int DoServerBackup(const SString & rDBSymb, PPBackupScen * pScen); // @prototype(ppdbutil.cpp)

class JOB_HDL_CLS(BACKUP) : public PPJobHandler {
public:
	JOB_HDL_CLS(BACKUP)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1, r = 0;
		PPBackupParam param;
		const size_t sav_offs = pParam->GetRdOffs();
		if((r = param.Read(*pParam, 0)) != 0) {
			SString temp_db_symb = JobDbSymb; // @v11.0.0
			ok = EditJobBackupParam(/*param.DBSymb*/temp_db_symb, &param.BuScen);
			if(ok > 0) {
				param.Write(pParam->Z(), 0);
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		PPBackupParam param;
		THROW(param.Read(*pParam, 0));
		{
			SString temp_db_symb = JobDbSymb; // @v11.0.0
			THROW(DoServerBackup(/*param.DBSymb*/temp_db_symb, &param.BuScen));
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(BACKUP);
//
//
//
class JOB_HDL_CLS(PALMIMPEXP) : public PPJobHandler {
public:
	JOB_HDL_CLS(PALMIMPEXP) (PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		int    r = 0;
		PalmPaneData param;
		const size_t sav_offs = pParam->GetRdOffs();
		param.LocID = param.LocID ? param.LocID : LConfig.Location;
		if((r = ReadParam(*pParam, &param, sizeof(param))) != 0) {
			ok = PPObjStyloPalm::EditImpExpData(&param);
			if(ok > 0) {
				WriteParam(pParam->Z(), &param, sizeof(param));
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		PalmPaneData param;
		THROW(ReadParam(*pParam, &param, sizeof(param)));
		THROW(PPObjStyloPalm::ImpExp(&param));
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(PALMIMPEXP);
//
//
//
class JOB_HDL_CLS(SCARDDISCRECALC) : public PPJobHandler {
public:
	JOB_HDL_CLS(SCARDDISCRECALC) (PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		SCardChargeRule param;
		const size_t sav_offs = pParam->GetRdOffs();
		int    r = ReadParam(*pParam, &param, sizeof(param));
		if(r) {
			ok = PPObjSCardSeries::SelectRule(&param);
			if(ok > 0)
				WriteParam(pParam->Z(), &param, sizeof(param));
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		SCardChargeRule param;
		THROW(ReadParam(*pParam, &param, sizeof(param)));
		THROW(PPObjSCardSeries::SetSCardsByRule(&param));
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(SCARDDISCRECALC);
//
//
//
extern int ScalePrepDialog(uint rezID, PPID * pScaleID, long * pFlags); // @v5.0.0

struct ScalePrepParam {
	ScalePrepParam() : ScaleID(0), Flags(0)
	{
	}
	PPID   ScaleID;
	long   Flags;
};

class JOB_HDL_CLS(LOADSCALE) : public PPJobHandler {
public:
	JOB_HDL_CLS(LOADSCALE) (PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1, r = 0;
		ScalePrepParam param;
		const size_t sav_offs = pParam->GetRdOffs();
		if((r = ReadParam(*pParam, &param, sizeof(param))) != 0) {
			ok = ScalePrepDialog(DLG_SCALETRAN, &param.ScaleID, &param.Flags);
			if(ok > 0) {
				WriteParam(pParam->Z(), &param, sizeof(param));
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		ScalePrepParam param;
		PPObjScale sobj;
		THROW(ReadParam(*pParam, &param, sizeof(param)));
		if(param.ScaleID == 0) {
			while(sobj.EnumItems(&param.ScaleID) > 0) {
				sobj.TransmitData(param.ScaleID, param.Flags | PPObjScale::fTrSkipListing, 0);
				// @todo Выводить информацию об ошибках в журнал
			}
		}
		else {
			THROW(sobj.TransmitData(param.ScaleID, param.Flags | PPObjScale::fTrSkipListing, 0));
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(LOADSCALE);
//
//
//
#define CASHN_UPDATEONLY 0x00000001L

class JOB_HDL_CLS(LOADASYNCPOS) : public PPJobHandler {
public:
	JOB_HDL_CLS(LOADASYNCPOS) (PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		int    r = 0;
		AsyncPosPrepParam param;
		const size_t sav_offs = pParam->GetRdOffs();
		if((r = ReadParam(*pParam, &param, sizeof(param))) != 0) {
			ok = AsyncCashnPrepDialog(&param);
			if(ok > 0) {
				WriteParam(pParam->Z(), &param, sizeof(param));
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		AsyncPosPrepParam param;
		PPCashMachine * p_cm = 0;
		THROW(ReadParam(*pParam, &param, sizeof(param)));
		THROW(!PPObjCashNode::IsLocked(param.CashNodeID));
		THROW(p_cm = PPCashMachine::CreateInstance(param.CashNodeID));
		THROW(p_cm->AsyncOpenSession(BIN(param.Flags & AsyncPosPrepParam::fUpdateOnly), 0));
		CATCHZOK
		delete p_cm;
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(LOADASYNCPOS);
//
//
//
int GetLastTransmit(const ObjIdListFilt * pDBDivList, LDATETIME * pSince)
{
	int    ok = -1;
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	if(p_sj && pDBDivList) {
		const PPIDArray & rary = pDBDivList->Get();
		LDATETIME since = ZERODATETIME;
		LDATETIME min_since;
		for(uint i = 0; i < rary.getCount(); i++) {
			PPID db_div_id = rary.at(i);
			while(ok < 0 && p_sj->GetLastEvent(PPACN_TRANSMOD, 0/*extraVal*/, &since, 7) > 0)
				if(!db_div_id || p_sj->data.ObjID == db_div_id)
					ok = 1;
			if(since.d && (min_since.d == 0 || cmp(since, min_since) < 0))
				min_since = since;
		}
		ASSIGN_PTR(pSince, min_since);
	}
	return ok;
}

class JOB_HDL_CLS(TRANSMITMODIF) : public PPJobHandler {
public:
	JOB_HDL_CLS(TRANSMITMODIF) (PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1, r = 0;
		ObjTransmitParam param;
		const size_t sav_offs = pParam->GetRdOffs();
		param.Init();
		if((r = param.Read(*pParam, 0)) != 0) {
			ok = ObjTransmDialog(DLG_MODTRANSM, &param);
			if(ok > 0) {
				param.Write(pParam->Z(), 0);
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		ObjTransmitParam param;
		param.Init();
		THROW(param.Read(*pParam, 0));
		THROW(PPObjectTransmit::TransmitModificationsByDBDivList(&param));
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(TRANSMITMODIF);
//
//
//
class JOB_HDL_CLS(DBMAINTAIN) : public PPJobHandler {
public:
	JOB_HDL_CLS(DBMAINTAIN) (PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1, r = 0;
		DBMaintainParam param;
		const size_t sav_offs = pParam->GetRdOffs();
		MEMSZERO(param);
		if((r = param.Read(*pParam, 0)) != 0) {
			ok = DBMaintainDialog(&param);
			if(ok > 0) {
				param.Write(pParam->Z(), 0);
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		DBMaintainParam param;
		MEMSZERO(param);
		THROW(param.Read(*pParam, 0));
		THROW(DoDBMaintain(&param));
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(DBMAINTAIN);
//
// Закрытие кассовых сессий
//
struct CashNodeParam {
	CashNodeParam() : CashNodeID(0)
	{
		Period.Z();
	}
	CashNodeParam & Z()
	{
		CashNodeID = 0;
		Period.Z();
		return *this;
	}
	int Read(SBuffer & rBuf, long)
	{
		int    ok = -1;
		if(rBuf.GetAvailableSize()) {
			long upp = 0, low = 0;
			if(rBuf.Read(CashNodeID) && rBuf.Read(upp) && rBuf.Read(low)) {
				Period.upp.v = upp;
				Period.low.v = low;
				ok = 1;
			}
			else
				ok = PPSetErrorSLib();
		}
		return ok;
	}
	int Write(SBuffer & rBuf, long)
	{
		if(rBuf.Write(CashNodeID) && rBuf.Write(Period.upp.v) && rBuf.Write(Period.low.v))
			return 1;
		else
			return PPSetErrorSLib();
	}
	PPID   CashNodeID;
	DateRange Period;
};

class CashNodeDialog : public TDialog {
	DECL_DIALOG_DATA(CashNodeParam);
public:
	CashNodeDialog() : TDialog(DLG_SELCNODE)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		int    r = 1;
		PPID   id = 0;
		StrAssocArray node_list;
		PPCashNode node;
		PPObjCashNode cn_obj;
		if(!RVALUEPTR(Data, pData))
			Data.Z();
		for(id = 0; (r = cn_obj.EnumItems(&id, &node)) > 0;) {
			if(PPCashMachine::IsAsyncCMT(node.CashType)) {
				THROW_SL(node_list.Add(id, node.Name));
			}
		}
		THROW(r);
		SetupStrAssocCombo(this, CTLSEL_SELCNODE_CNODE, node_list, Data.CashNodeID, 0);
		SetPeriod();
		CATCHZOKPPERR
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTLSEL_SELCNODE_CNODE, &Data.CashNodeID);
		THROW_PP(Data.CashNodeID, PPERR_INVCASHNODEID);
		THROW_PP(Data.CashNodeID != PPCMT_OKA500, PPERR_OKA500NOTSUPPORTED);
		GetPeriod();
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERR
		return ok;
	}
private:
	int    GetPeriod();
	int    SetPeriod();
};

int CashNodeDialog::SetPeriod()
{
	int    ok = -1;
	SString ss;
	LDATE   dt  = Data.Period.low;
	LDATE   upp = Data.Period.upp;
	while(ok < 0) {
		int16  lw;
		if(dt.year() == (int)0x8000) {
			lw = (int16)dt.v;
			if(lw != (int16)0x8000) {
				ss.CatChar('@');
				if(lw)
					ss.CatChar((lw > 0) ? '+' : '-').Cat(abs(lw));
			}
		}
		else
			ss.Cat(dt);
		if(dt != upp) {
			ss.CatCharN('.', 2);
			dt = upp;
		}
		else
			ok = 1;
	}
	setCtrlString(CTL_SELCNODE_PERIOD, ss);
	return ok;
}

int CashNodeDialog::GetPeriod()
{
	int    ok = -1;
	char   buf[64];
	long   upp = 0, low = 0;
	getCtrlData(CTL_SELCNODE_PERIOD, buf);
	char * d = strstr(buf, "..");
	if(d) {
		char  low_buf[64], upp_buf[64];
		low_buf[0] = upp_buf[0] = 0;
		char * b = low_buf;
		for(char * p = buf; p < d;)
			*b++ = *p++;
		*b = 0;
		STRNSCPY(upp_buf, d+2);
		ok = BIN(ParseBound(low_buf, &low) && ParseBound(upp_buf, &upp));
	}
	else
		ok = ParseBound(buf, &low) ? (upp = low, 1) : 0;
	if(ok > 0) {
		Data.Period.upp.v = upp;
		Data.Period.low.v = low;
	}
	return ok;
}

static int EditSessionParam(CashNodeParam * pParam) { DIALOG_PROC_BODY(CashNodeDialog, pParam); }

class JOB_HDL_CLS(CSESSCLOSE) : public PPJobHandler {
public:
	JOB_HDL_CLS(CSESSCLOSE) (PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1, r = 0;
		CashNodeParam param;
		const size_t sav_offs = pParam->GetRdOffs();
		MEMSZERO(param);
		if((r = param.Read(*pParam, 0)) != 0) {
			if((ok = EditSessionParam(&param)) > 0) {
				param.Write(pParam->Z(), 0);
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		CashNodeParam param;
		THROW(param.Read(*pParam, 0));
		THROW(!PPObjCashNode::IsLocked(param.CashNodeID));
		param.Period.Actualize(ZERODATE);
		THROW(PPCashMachine::AsyncCloseSession2(param.CashNodeID, &param.Period));
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(CSESSCLOSE);
//
//
//
class JOB_HDL_CLS(FILLSALESTABLE) : public PPJobHandler {
public:
	JOB_HDL_CLS(FILLSALESTABLE) (PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1, r = 0;
		PrcssrPrediction::Param param;
		PrcssrPrediction prcssr;
		MEMSZERO(param);
		const size_t sav_offs = pParam->GetRdOffs();
		if((r = ReadParam(*pParam, &param, sizeof(param))) != 0) {
			if((ok = prcssr.EditParam(&param)) > 0) {
				WriteParam(pParam->Z(), &param, sizeof(param));
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		PrcssrPrediction::Param param;
		PrcssrPrediction prcssr;
		THROW(ReadParam(*pParam, &param, sizeof(param)));
		THROW(prcssr.Init(&param));
		THROW(prcssr.Run());
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(FILLSALESTABLE);
//
//
//
class JOB_HDL_CLS(TESTSALESTABLE) : public PPJobHandler {
public:
	JOB_HDL_CLS(TESTSALESTABLE) (PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1, r = 0;
		PrcssrPrediction::Param param;
		PrcssrPrediction prcssr;
		MEMSZERO(param);
		const size_t sav_offs = pParam->GetRdOffs();
		if((r = ReadParam(*pParam, &param, sizeof(param))) != 0) {
			param.Process |= param.prcsTest;
			if((ok = prcssr.EditParam(&param)) > 0) {
				WriteParam(pParam->Z(), &param, sizeof(param));
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		PrcssrPrediction::Param param;
		PrcssrPrediction prcssr;
		THROW(ReadParam(*pParam, &param, sizeof(param)));
		param.Process |= param.prcsTest;
		THROW(prcssr.Init(&param));
		THROW(prcssr.Run());
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(TESTSALESTABLE);
//
//
//
class JOB_HDL_CLS(CHARRYIMPORT) : public PPJobHandler {
public:
	JOB_HDL_CLS(CHARRYIMPORT) (PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1, r = 0;
		RcvCharryParam param;
		const size_t sav_offs = pParam->GetRdOffs();
		if((r = ReadParam(*pParam, &param, sizeof(param))) != 0) {
			if((ok = param.Edit()) > 0) {
				WriteParam(pParam->Z(), &param, sizeof(param));
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		RcvCharryParam param;
		THROW(ReadParam(*pParam, &param, sizeof(param)));
		THROW(ReceiveCharryObjects(&param));
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(CHARRYIMPORT);

class JOB_HDL_CLS(REMOVETEMPFILES) : public PPJobHandler {
public:
	JOB_HDL_CLS(REMOVETEMPFILES) (PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1, r = 0;
		DeleteTmpFilesParam param;
		const size_t sav_offs = pParam->GetRdOffs();
		if((r = ReadParam(*pParam, &param, sizeof(param))) != 0) {
			if((ok = DeleteTmpFilesDlg(&param)) > 0) {
				WriteParam(pParam->Z(), &param, sizeof(param));
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		DeleteTmpFilesParam param;
		THROW(ReadParam(*pParam, &param, sizeof(param)));
		THROW(PPDeleteTmpFiles(&param));
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(REMOVETEMPFILES);

class JOB_HDL_CLS(TESTCREATEFILES) : public PPJobHandler {
public:
	JOB_HDL_CLS(TESTCREATEFILES) (PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		char   path[MAX_PATH];
		const  size_t sav_offs = pParam->GetRdOffs();
		memzero(path, sizeof(path));
		int    r = ReadParam(*pParam, path, sizeof(path));
		if(r != 0) {
			SString s_path;
			s_path.CopyFrom(path);
			PPInputStringDialogParam isd_param; // empty param
			if((ok = InputStringDialog(isd_param, s_path)) > 0) { // @v12.4.1 InputStringDialog(0, s_path)-->InputStringDialog(&isd_param, s_path)
				WriteParam(pParam->Z(), s_path.cptr(), s_path.Len()); // @badcast
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		char   path[MAX_PATH];
		SString s_path, wait_file;
		PPLogger logger;
		memzero(path, sizeof(path));
		THROW(ReadParam(*pParam, path, sizeof(path)));
		(s_path = path).SetLastSlash().Cat("18.txt");
		(wait_file = path).SetLastSlash().Cat("cs_wait");
		if(fileExists(wait_file))
			SFile::Remove(wait_file);
		if(fileExists(s_path)) {
			FILE * p_f = fopen(wait_file, "w");
			if(p_f) {
				SFile::ZClose(&p_f);
			}
			SFile::Remove(s_path);
			SFile::Remove(wait_file);
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(TESTCREATEFILES);
//
// LaunchApp
//
class LaunchAppParam {
public:
	LaunchAppParam();
	int    Write(SBuffer & rBuf, long) const;
	int    Read(SBuffer & rBuf, long);

	enum {
		fWait	= 0x0001,
		fRemote	= 0x0002
	};
	long   Signature;
	long   Ver;
	int32  Flags;
	uint8  Reserve[32];
	SString App;
	SString Arg;
	SString WmiServer;
	SString UserLogin;
	SString UserPassword;
};

LaunchAppParam::LaunchAppParam() : Signature(PPConst::Signature_LaunchAppParam), Ver(0), Flags(0)
{
	memzero(Reserve, sizeof(Reserve));
}

int LaunchAppParam::Write(SBuffer & rBuf, long) const
{
	int    ok = 1;
	const  long signature = PPConst::Signature_LaunchAppParam;
	THROW_SL(rBuf.Write(signature));
	THROW_SL(rBuf.Write(Ver));
	THROW_SL(rBuf.Write(Flags));
	THROW_SL(rBuf.Write(Reserve, sizeof(Reserve)));
	THROW_SL(rBuf.Write(App));
	THROW_SL(rBuf.Write(Arg));
	THROW_SL(rBuf.Write(WmiServer));
	THROW_SL(rBuf.Write(UserLogin));
	THROW_SL(rBuf.Write(UserPassword));
	CATCHZOK
	return ok;
}

int LaunchAppParam::Read(SBuffer & rBuf, long)
{
	int    ok = -1;
	if(rBuf.GetAvailableSize()) {
		THROW_SL(rBuf.Read(Signature));
		THROW_PP_S(Signature == PPConst::Signature_LaunchAppParam, PPERR_INVSIGNONOBJSRLZRD, "LaunchAppParam");
		THROW_SL(rBuf.Read(Ver));
		THROW_SL(rBuf.Read(Flags));
		THROW_SL(rBuf.Read(Reserve, sizeof(Reserve)));
		THROW_SL(rBuf.Read(App));
		THROW_SL(rBuf.Read(Arg));
		THROW_SL(rBuf.Read(WmiServer));
		THROW_SL(rBuf.Read(UserLogin));
		THROW_SL(rBuf.Read(UserPassword));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

class LaunchAppDialog : public TDialog {
	DECL_DIALOG_DATA(LaunchAppParam);
	enum {
		ctlgroupBrowse = 1
	};
public:
	LaunchAppDialog() : TDialog(DLG_LAUNCHAPP)
	{
		FileBrowseCtrlGroup::Setup(this, CTLBRW_LAUNCHAPP_APP, CTL_LAUNCHAPP_APP, ctlgroupBrowse, PPTXT_TITLE_SELAPPFILE, 0, FileBrowseCtrlGroup::fbcgfFile);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		setCtrlString(CTL_LAUNCHAPP_APP, Data.App);
		setCtrlString(CTL_LAUNCHAPP_PARAMS, Data.Arg);
		AddClusterAssoc(CTL_LAUNCHAPP_FLAGS, 0, LaunchAppParam::fWait);
		AddClusterAssoc(CTL_LAUNCHAPP_FLAGS, 1, LaunchAppParam::fRemote);
		SetClusterData(CTL_LAUNCHAPP_FLAGS, Data.Flags);
		setCtrlString(CTL_LAUNCHAPP_SERVER, Data.WmiServer);
		setCtrlString(CTL_LAUNCHAPP_USER,   Data.UserLogin);
		//
		char   pw_buf[256];
		size_t ret_len = 0;
		Data.UserPassword.DecodeMime64(pw_buf, sizeof(pw_buf), &ret_len);
		IdeaDecrypt(0, pw_buf, ret_len);
		setCtrlData(CTL_LAUNCHAPP_PWD, pw_buf);
		memzero(pw_buf, sizeof(pw_buf));
		disableCtrls((Data.Flags & LaunchAppParam::fRemote) ? 0 : 1, CTL_LAUNCHAPP_SERVER, CTL_LAUNCHAPP_USER, CTL_LAUNCHAPP_PWD, 0);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		getCtrlString(sel = CTL_LAUNCHAPP_APP, Data.App);
		THROW_PP(Data.App.Len(), PPERR_USERINPUT);
		getCtrlString(CTL_LAUNCHAPP_PARAMS, Data.Arg);
		GetClusterData(CTL_LAUNCHAPP_FLAGS, &Data.Flags);
		getCtrlString(CTL_LAUNCHAPP_SERVER, Data.WmiServer);
		getCtrlString(CTL_LAUNCHAPP_USER,   Data.UserLogin);

		char   pw_buf[256];
		getCtrlData(CTL_LAUNCHAPP_PWD, pw_buf);
		IdeaEncrypt(0, pw_buf, sizeof(pw_buf));
		Data.UserPassword.EncodeMime64(pw_buf, sizeof(pw_buf));
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_LAUNCHAPP_FLAGS)) {
			GetClusterData(CTL_LAUNCHAPP_FLAGS, &Data.Flags);
			disableCtrls((Data.Flags & LaunchAppParam::fRemote) ? 0 : 1, CTL_LAUNCHAPP_SERVER, CTL_LAUNCHAPP_USER, CTL_LAUNCHAPP_PWD, 0);
			clearEvent(event);
		}
	}
};

int EditLaunchAppParam(LaunchAppParam * pData) { DIALOG_PROC_BODYERR(LaunchAppDialog, pData); }

class JOB_HDL_CLS(LAUNCHAPP) : public PPJobHandler {
public:
	JOB_HDL_CLS(LAUNCHAPP) (PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		LaunchAppParam param;
		const size_t sav_offs = pParam->GetRdOffs();
		int    r = param.Read(*pParam, 0);
		if(r) {
			if(EditLaunchAppParam(&param) > 0) {
				ok = BIN(param.Write(pParam->Z(), 0));
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		else
			ok = 0;
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		LaunchAppParam param;
		SString temp_buf;
		THROW(param.Read(*pParam, 0) > 0);
		if(param.Flags & LaunchAppParam::fRemote) {
			SWmi   wmi;
			char   pw_buff[256];
			size_t ret_len = 0;
			temp_buf = param.App;
			if(param.Arg.NotEmptyS())
				temp_buf.Space().Cat(param.Arg);
			param.UserPassword.DecodeMime64(pw_buff, sizeof(pw_buff), &ret_len);
			IdeaDecrypt(0, pw_buff, ret_len);
			THROW_SL(wmi.Connect(param.WmiServer, param.UserLogin, pw_buff));
			memzero(pw_buff, sizeof(pw_buff));
			THROW_SL(wmi.Method_CreateProcess(temp_buf));
		}
		else {
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			MEMSZERO(si);
			si.cb = sizeof(si);
			MEMSZERO(pi);
			temp_buf.Z().CatQStr(param.App);
			if(param.Arg.NotEmptyS())
				temp_buf.Space().Cat(param.Arg);
			STempBuffer cmd_line((temp_buf.Len() + 32) * sizeof(TCHAR));
			strnzcpy(static_cast<TCHAR *>(cmd_line.vptr()), SUcSwitch(temp_buf), cmd_line.GetSize()/sizeof(TCHAR));
			int    r = ::CreateProcess(0, static_cast<TCHAR *>(cmd_line.vptr()), 0, 0, FALSE, 0, 0, 0, &si, &pi); // @unicodeproblem
			if(!r) {
				SLS.SetOsError(0, 0);
				PPSetErrorSLib();
				PPError();
			}
		}
#if 0 // {
		else if(param.Arg.NotEmptyS()) {
			temp_buf.Z().Space().Cat(param.Arg);
			_spawnl((param.Flags & LaunchAppParam::fWait) ? _P_WAIT : _P_NOWAIT, param.App, (const char *)temp_buf, 0);
		}
		else {
			_spawnl(((param.Flags % 2) & LaunchAppParam::fWait) ? _P_WAIT : _P_NOWAIT, param.App, param.App, 0);
		}
#endif // } 0
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(LAUNCHAPP);
//
//
//
class JOB_HDL_CLS(CSESSCRDRAFT) : public PPJobHandler {
public:
	JOB_HDL_CLS(CSESSCRDRAFT)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		CSessCrDraftParam param;
		const size_t sav_offs = pParam->GetRdOffs();
		int    r = param.Read(*pParam, 0);
		if(r) {
			if(PPViewCSess::EditCreateDraftParam(&param) > 0) {
				ok = BIN(param.Write(pParam->Z(), 0));
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		else
			ok = 0;
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		CSessCrDraftParam param;
		THROW(param.Read(*pParam, 0));
		THROW(PrcssrBillAutoCreate::CreateDraftByCSessRule(&param));
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(CSESSCRDRAFT);
//
//
//
class JOB_HDL_CLS(DEBTRATE) : public PPJobHandler {
public:
	JOB_HDL_CLS(DEBTRATE)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		PrcssrDebtRate::Param param;
		PrcssrDebtRate prc;
		const size_t sav_offs = pParam->GetRdOffs();
		int    r = param.Read(*pParam, 0);
		if(r) {
			if(r < 0)
				prc.InitParam(&param);
			if(prc.EditParam(&param) > 0) {
				ok = BIN(param.Write(pParam->Z(), 0));
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		else
			ok = 0;
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		PrcssrDebtRate prc;
		PrcssrDebtRate::Param param;
		THROW(param.Read(*pParam, 0));
		THROW(prc.Init(&param));
		THROW(prc.Run());
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(DEBTRATE);
//
//
//
class JOB_HDL_CLS(BIZSCORE) : public PPJobHandler {
public:
	JOB_HDL_CLS(BIZSCORE)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		PrcssrBizScore::Param param;
		PrcssrBizScore prc;
		const size_t sav_offs = pParam->GetRdOffs();
		int    r = param.Read(*pParam, 0);
		if(r) {
			if(r < 0)
				prc.InitParam(&param);
			if(prc.EditParam(&param) > 0) {
				ok = BIN(param.Write(pParam->Z(), 0));
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		else
			ok = 0;
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		PrcssrBizScore prc;
		PrcssrBizScore::Param param;
		THROW(param.Read(*pParam, 0));
		THROW(prc.Init(&param));
		THROW(prc.Run());
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(BIZSCORE);
//
//
//
class JOB_HDL_CLS(EXPORTBALTIKA) : public PPJobHandler {
public:
	JOB_HDL_CLS(EXPORTBALTIKA)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrSupplInterchange prc;
			SupplInterchangeFilt filt;
			const size_t preserve_offs = pParam->GetRdOffs();
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			else
				pParam->SetRdOffs(preserve_offs);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		if(pParam) {
			SupplInterchangeFilt filt;
			if(filt.Read(*pParam, 0)) {
				PrcssrSupplInterchange prc;
				THROW(prc.Init(&filt));
				THROW(prc.Run());
			}
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(EXPORTBALTIKA);
//
//
//
class JOB_HDL_CLS(TRANSMITBILLBYFILT) : public PPJobHandler {
public:
	JOB_HDL_CLS(TRANSMITBILLBYFILT)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1, r = 1;
		uint   val = 0;
		BillFilt filt;
		ObjTransmitParam tr_param;
		const size_t sav_offs = pParam->GetRdOffs();
		filt.SetupBrowseBillsType(filt.Bbt = bbtUndef);
		if(pParam->GetAvailableSize() == 0) {
			if((r = SelectorDialog(DLG_BBTSEL, CTL_BBTSEL_TYPE, &val)) > 0)
				filt.SetupBrowseBillsType(filt.Bbt = static_cast<BrowseBillsType>(val));
		}
		else {
			filt.Read(*pParam, 0);
			tr_param.Read(*pParam, 0);
		}
		if(r > 0 && ObjTransmDialogExt(DLG_OBJTRANSM, PPVIEW_BILL, &tr_param, &filt) > 0) {
			if((ok = BIN(filt.Write(pParam->Z(), 0))) > 0)
				ok = BIN(tr_param.Write(*pParam, 0) > 0);
		}
		else
			pParam->SetRdOffs(sav_offs);
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		BillFilt filt;
		ObjTransmitParam tr_param;
		THROW(filt.Read(*pParam, 0));
		THROW(tr_param.Read(*pParam, 0));
		THROW(PPViewBill::TransmitByFilt(&filt, &tr_param));
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(TRANSMITBILLBYFILT);
//
//
//
class ExportBillsFiltDialog : public TDialog {
public:
	static int GetParamsByName(const char * pBillParamName, const char * pBRowParamName, PPBillImpExpParam * pBillParam, PPBillImpExpParam * pBRowParam)
	{
		int    ok = 0;
		SString ini_file_name, sect;
		THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
		if(pBillParamName && pBRowParamName && pBillParam && pBRowParam) {
			PPIniFile ini_file(ini_file_name, 0, 1, 1);
			THROW(LoadSdRecord(PPREC_BILL, &pBillParam->InrRec));
			THROW(LoadSdRecord(PPREC_BROW, &pBRowParam->InrRec));
			pBillParam->Direction = 0;
			pBRowParam->Direction = 0;
			pBillParam->ProcessName(1, (sect = pBillParamName));
			THROW_PP_S(pBillParam->ReadIni(&ini_file, sect, 0) > 0, PPERR_INVBILLEXPCFG, pBillParamName);
			// @vmiller {
			// Если имя pBillParamName соответствует одной из строк перечисления PPTXT_EDIEXPCMD, то экспорт
			// происходит через dll, более того, это режим EDI
			{
				SString buf;
				StringSet ss(';', PPLoadTextS(PPTXT_EDIEXPCMD, buf));
				for(uint i = 0, f_exit = 0; ss.get(&i, buf) && !f_exit;) {
					uint j = 0;
					StringSet ss1(',', buf);
					ss1.get(&j, buf.Z());
					ss1.get(&j, buf.Z());
					ss1.get(&j, buf.Z());
					if(buf.CmpNC(pBillParamName) == 0) {
						pBillParam->BaseFlags |= PPImpExpParam::bfDLL;
						f_exit = 1;
					}
				}
			}
			// } @vmiller
			// @vmiller {
			pBillParam->ProcessName(2, sect);
			if(sect.HasPrefixIAscii("DLL_"))
				pBillParam->BaseFlags |= PPImpExpParam::bfDLL;
			// } @vmiller
			pBRowParam->ProcessName(1, (sect = pBRowParamName));
			THROW_PP_S(pBRowParam->ReadIni(&ini_file, sect, 0) > 0, PPERR_INVBILLEXPCFG, pBRowParamName);
			pBillParam->ProcessName(2, (sect = pBillParamName));
			pBillParam->Name = sect;
			pBRowParam->ProcessName(2, (sect = pBRowParamName));
			pBRowParam->Name = sect;
			ok = 1;
		}
		CATCHZOK
		return ok;
	}
	struct ExpBillsFilt {
		enum {
			fEdi = 1 // @vmiller
		};
		void Init()
		{
			Filt.Init(1, 0);
			BillParam.Z();
			BRowParam.Z();
			Flags = 0;
		}
		int Read(SBuffer & rBuf, long)
		{
			int    ok = 1;
			THROW(rBuf.GetAvailableSize());
			THROW(Filt.Read(rBuf, 0));
			SETFLAG(Flags, fEdi, BIN(Filt.Flags & BillFilt::fExportEDI));
			THROW_SL(rBuf.Read(BillParam));
			THROW_SL(rBuf.Read(BRowParam));
			// THROW_SL(rBuf.Read(Flags)); // @vmiller @added --> rBuf.Read(Flags)
			CATCHZOK
			return ok;
		}
		int Write(SBuffer & rBuf, long)
		{
			int    ok = 1;
			SETFLAG(Filt.Flags, BillFilt::fExportEDI, BIN(Flags & fEdi));
			THROW(Filt.Write(rBuf, 0));
			THROW_SL(rBuf.Write(BillParam));
			THROW_SL(rBuf.Write(BRowParam));
			// THROW_SL(rBuf.Write(Flags)); // @vmiller @added --> rBuf.Write(Flags)
			CATCHZOK
			return ok;
		}
		SString BillParam;
		SString BRowParam;
		BillFilt Filt;
		long   Flags; // @transient @vmiller
	};
	ExportBillsFiltDialog() : TDialog(DLG_BILLEXPFILT)
	{
		PPBillImpExpParam param;
		GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_BILL, &param, &HdrList, 1);
		GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_BROW, &param, &LineList, 1);
	}
	int    setDTS(const ExpBillsFilt *);
	int    getDTS(ExpBillsFilt *);
private:
	DECL_HANDLE_EVENT;
	ExpBillsFilt Data;
	StrAssocArray HdrList, LineList;
	PPViewBill View;
};

IMPL_HANDLE_EVENT(ExportBillsFiltDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(TVCMD == cmBillFilt)
			View.EditBaseFilt(&Data.Filt);
		else if(event.isClusterClk(CTL_BILLEXPFILT_BILLTYP)) {
			long v = 0;
			GetClusterData(CTL_BILLEXPFILT_BILLTYP, &v);
			Data.Filt.SetupBrowseBillsType(Data.Filt.Bbt = static_cast<BrowseBillsType>(v));
		}
		else if(event.isCbSelected(CTLSEL_BILLEXPFILT_CFG)) {
			long   hdr_id = getCtrlLong(CTLSEL_BILLEXPFILT_CFG);
			SString sect;
			HdrList.GetText(hdr_id, sect);
			if(sect.NotEmpty()) {
				uint p = 0;
				if(LineList.SearchByTextNc(sect, &p) > 0)
					setCtrlLong(CTLSEL_BILLEXPFILT_RCFG, LineList.Get(p).Id);
			}
		}
		else if(event.isCbSelected(CTLSEL_BILLEXPFILT_RCFG)) {
			long row_id = getCtrlLong(CTLSEL_BILLEXPFILT_RCFG);
			if(!getCtrlLong(CTLSEL_BILLEXPFILT_CFG)) {
				SString sect;
				LineList.GetText(row_id, sect);
				if(sect.NotEmpty()) {
					uint p = 0;
					if(HdrList.SearchByTextNc(sect, &p) > 0)
						setCtrlLong(CTLSEL_BILLEXPFILT_CFG, HdrList.Get(p).Id);
				}
			}
		}
		// @vmiller {
		else if(event.isClusterClk(CTL_BILLEXPFILT_FLAGS)) {
			long   id = 0;
			uint   p = 0;
			PPBillImpExpParam bill_param, brow_param;
			GetClusterData(CTL_BILLEXPFILT_FLAGS, &Data.Flags);
			disableCtrls((Data.Flags & ExpBillsFilt::fEdi), CTLSEL_IEBILLSEL_BROW, 0);
			Data.BRowParam = 0;
			if(Data.Flags & ExpBillsFilt::fEdi) {
				// Заполняем списком типов документов
				HdrList.Z();
				SString buf;
				StringSet ss(';', PPLoadTextS(PPTXT_EDIEXPCMD, buf));
				for(uint i = 0; ss.get(&i, buf);) {
					uint j = 0;
					StringSet ss1(',', buf);
					ss1.get(&j, buf.Z());
					id = buf.ToLong();
					ss1.get(&j, buf.Z());
					ss1.get(&j, buf.Z());
					HdrList.Add(id, buf);
				}
				GetParamsByName(Data.BillParam, Data.BRowParam, &bill_param, &brow_param);
				id = (HdrList.SearchByTextNc(bill_param.Name, &(p = 0)) > 0) ? HdrList.Get(p).Id : 0;
				SetupStrAssocCombo(this, CTLSEL_BILLEXPFILT_CFG, HdrList, id, 0);
			}
			else {
				HdrList.Z();
				GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_BILL, &bill_param, &HdrList, 1);
				bill_param.Init();
				GetParamsByName(Data.BillParam, Data.BRowParam, &bill_param, &brow_param);
				id = (HdrList.SearchByTextNc(bill_param.Name, &(p = 0)) > 0) ? HdrList.Get(p).Id : 0;
				SetupStrAssocCombo(this, CTLSEL_BILLEXPFILT_CFG, HdrList, id, 0);
				id = (LineList.SearchByTextNc(brow_param.Name, &(p = 0)) > 0) ? LineList.Get(p).Id : 0;
				SetupStrAssocCombo(this, CTLSEL_BILLEXPFILT_RCFG, LineList, id, 0);
			}
		}
		// } @vmiller
		else
			return;
		clearEvent(event);
	}
}

int ExportBillsFiltDialog::setDTS(const ExpBillsFilt * pData)
{
	int    ok = 1;
	if(!RVALUEPTR(Data, pData))
		Data.Init();
	disableCtrls(0, CTLSEL_BILLEXPFILT_RCFG, 0L); // @vmiller
	if(!(Data.Flags & ExpBillsFilt::fEdi)) { // @vmiller
		if(!oneof2(Data.Filt.Bbt, bbtGoodsBills, bbtDraftBills))
			Data.Filt.SetupBrowseBillsType(Data.Filt.Bbt = bbtGoodsBills);
		{
			uint    p  = 0;
			uint    id = 0;
			SString sect;
			PPBillImpExpParam bill_param, brow_param;
			GetParamsByName(Data.BillParam, Data.BRowParam, &bill_param, &brow_param);
			id = (HdrList.SearchByTextNc(bill_param.Name, &p) > 0) ? (uint)HdrList.Get(p).Id : 0;
			SetupStrAssocCombo(this, CTLSEL_BILLEXPFILT_CFG, HdrList, (long)id, 0);
			id = (LineList.SearchByTextNc(brow_param.Name, &p) > 0) ? (uint)LineList.Get(p).Id : 0;
			SetupStrAssocCombo(this, CTLSEL_BILLEXPFILT_RCFG, LineList, (long)id, 0);
		}
	}//@vmiller
	// @vmiller {
	else if(Data.Flags & ExpBillsFilt::fEdi) {
		uint id = 0, p = 0;
		PPBillImpExpParam bill_param, brow_param;
		disableCtrls((Data.Flags & ExpBillsFilt::fEdi), CTLSEL_BILLEXPFILT_RCFG, 0L);
		// Заполняем списком типов документов
		HdrList.Z();
		SString buf;
		StringSet ss(';', PPLoadTextS(PPTXT_EDIEXPCMD, buf));
		for(uint i = 0; ss.get(&i, buf);) {
			uint j = 0;
			StringSet ss1(',', buf);
			ss1.get(&j, buf.Z());
			id = buf.ToLong();
			ss1.get(&j, buf.Z());
			ss1.get(&j, buf.Z());
			HdrList.Add(id, buf);
		}
		GetParamsByName(Data.BillParam, Data.BRowParam, &bill_param, &brow_param);
		id = (HdrList.SearchByTextNc(bill_param.Name, &(p = 0)) > 0) ? (uint)HdrList.Get(p).Id : 0;
		SetupStrAssocCombo(this, CTLSEL_BILLEXPFILT_CFG, HdrList, (long)id, 0);
	}
	// } @vmiller
	AddClusterAssocDef(CTL_BILLEXPFILT_BILLTYP, 0, bbtGoodsBills);
	AddClusterAssoc(CTL_BILLEXPFILT_BILLTYP,  1, bbtDraftBills);
	SetClusterData(CTL_BILLEXPFILT_BILLTYP,  (long)Data.Filt.Bbt);

	AddClusterAssoc(CTL_BILLEXPFILT_FLAGS, 0, ExpBillsFilt::fEdi); // @vmiller
	SetClusterData(CTL_BILLEXPFILT_FLAGS, Data.Flags); // @vmiller
	return ok;
}

int ExportBillsFiltDialog::getDTS(ExpBillsFilt * pData)
{
	int     ok = 1;
	uint    id = 0, sel = 0;
	long    v  = 0;
	SString sect;
	getCtrlData(sel = CTLSEL_BILLEXPFILT_CFG, &id);
	THROW_PP(id, PPERR_INVBILLIMPEXPCFG);
	HdrList.GetText(id, sect);
	Data.BillParam = sect;
	if(!(Data.Flags & ExpBillsFilt::fEdi)) { // @vmiller
		if(!sect.HasPrefixIAscii("DLL_")) { // @vmiller
			getCtrlData(sel = CTLSEL_BILLEXPFILT_RCFG, &id);
			THROW_PP(id, PPERR_INVBILLIMPEXPCFG);
			LineList.GetText(id, sect);
			Data.BRowParam = sect;
		} // @vmiller
	} // @vmiller
	ASSIGN_PTR(pData, Data);
	CATCH
		selectCtrl(sel);
		ok = 0;
	ENDCATCH
	return ok;
}

class JOB_HDL_CLS(EXPORTBILLS) : public PPJobHandler {
public:
	JOB_HDL_CLS(EXPORTBILLS)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		int    valid_data = 0;
		ExportBillsFiltDialog::ExpBillsFilt filt;
		ExportBillsFiltDialog * p_dlg = new ExportBillsFiltDialog;
		PPViewBill view;
		filt.Init(); // @vmiller
		const size_t sav_offs = pParam->GetRdOffs();
		THROW_INVARG(pParam);
		THROW(CheckDialogPtr(&p_dlg));
		filt.Filt.SetupBrowseBillsType(filt.Filt.Bbt = bbtUndef);
		if(pParam->GetAvailableSize() != 0)
			filt.Read(*pParam, 0);
		p_dlg->setDTS(&filt);
		for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
			if(p_dlg->getDTS(&filt) > 0)
				ok = valid_data = 1;
			else
				PPError();
		}
		if(ok > 0) {
			THROW(filt.Write(pParam->Z(), 0));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		delete p_dlg;
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		const  PPCommConfig & r_ccfg = CConfig;
		SString fmt_buf, msg_buf;
		ExportBillsFiltDialog::ExpBillsFilt filt;
		CoInitialize(NULL);
		THROW(filt.Read(*pParam, 0) > 0);
		if(r_ccfg.Flags & CCFLG_DEBUG) {
			PPLogMessage(PPFILNAM_DEBUG_LOG, PPLoadTextS(PPTXT_LOG_JOBEXPBILL_READFILT, msg_buf), LOGMSGF_USER|LOGMSGF_TIME);
		}
		{
			PPViewBill view;
			PPBillImpExpParam bill_param, brow_param;
			THROW(ExportBillsFiltDialog::GetParamsByName(filt.BillParam, filt.BRowParam, &bill_param, &brow_param));
			if(r_ccfg.Flags & CCFLG_DEBUG) {
				const char * p1 = filt.BillParam.NotEmpty() ? filt.BillParam.cptr() : "";
				const char * p2 = filt.BRowParam.NotEmpty() ? filt.BRowParam.cptr() : "";
				PPFormatT(PPTXT_LOG_JOBEXPBILL_GETPARAM, &msg_buf, p1, p2);
				PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
			}
			THROW(view.Init_(&filt.Filt));
			ok = view.ExportGoodsBill(&bill_param, &brow_param);
			if(r_ccfg.Flags & CCFLG_DEBUG) {
				PPFormatT(PPTXT_LOG_JOBEXPBILL_DONE, &msg_buf, ok);
				PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
			}
		}
		CATCHZOK
		CoUninitialize();
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(EXPORTBILLS);
//
//
//
class JOB_HDL_CLS(EXPORTGOODS) : public PPJobHandler {
public:
	JOB_HDL_CLS(EXPORTGOODS)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		ExportGoodsParam param;
		const size_t preserve_offs = pParam ? pParam->GetRdOffs() : 0;
		THROW_INVARG(pParam);
		if(pParam->GetAvailableSize() != 0)
			param.Read(*pParam, 0);
		if(ExportGoodsParam::Edit(param) > 0) {
			THROW(param.Write(pParam->Z(), 0));
			ok = 1;
		}
		else {
			pParam->SetRdOffs(preserve_offs);
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(preserve_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		ExportGoodsParam param;
		CoInitialize(NULL);
		if(param.Read(*pParam, 0) > 0) {
			PPViewGoods view;
			PPGoodsImpExpParam ie_param;
			THROW(ExportGoodsParam::GetExportParamByName(param.ExpCfg, &ie_param));
			THROW(view.Init_(&param.Filt));
			ie_param.LocID = param.LocID;
			ok = view.Export(&ie_param);
		}
		CATCHZOK
		CoUninitialize();
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(EXPORTGOODS);
//
//
//
class JOB_HDL_CLS(AUTOSUPPLORDER) : public PPJobHandler {
public:
	JOB_HDL_CLS(AUTOSUPPLORDER)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		SStatFilt * p_filt = 0;
		PPViewSStat view;
		THROW_INVARG(pParam);
		sav_offs = pParam->GetRdOffs();
		THROW_MEM(p_filt = static_cast<SStatFilt *>(view.CreateFilt(reinterpret_cast<void *>(1))));
		if(pParam->GetAvailableSize() != 0)
			p_filt->Read(*pParam, 0);
		if(view.EditBaseFilt(p_filt) > 0) {
			ok = 1;
			THROW(p_filt->Write(pParam->Z(), 0));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		ZDELETE(p_filt);
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		SStatFilt filt;
		THROW(filt.Read(*pParam, 0));
		THROW(PrcssrBillAutoCreate::CreateDraftBySupplOrders(&filt));
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(AUTOSUPPLORDER);
//
//
//
PPObjRFIDDevice::PPObjRFIDDevice(void * extraPtr) : PPObjReference(PPOBJ_RFIDDEVICE, extraPtr)
{
}

class RFIDDeviceDialog : public TDialog {
	DECL_DIALOG_DATA(PPRFIDDevice);
	enum {
		ctlgroupGoods = 1
	};
public:
	RFIDDeviceDialog() : TDialog(DLG_RFIDDEV)
	{
		addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_RFIDDEV_GGRP, CTLSEL_RFIDDEV_GOODS));
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		SETIFZ(Data.Cpp.ByteSize, 8);
		SETIFZ(Data.Cpp.Cbr, cbr57600);
		setCtrlData(CTL_RFIDDEV_ID,  &Data.ID);
		setCtrlData(CTL_RFIDDEV_NUMBER, &Data.DeviceNumber);
		setCtrlData(CTL_RFIDDEV_NAME, Data.Name);
		setCtrlData(CTL_RFIDDEV_PORT, Data.Port);
		SetupStringCombo(this, CTLSEL_RFIDDEV_CBR,      PPTXT_COMBAUDRATE, static_cast<long>(Data.Cpp.Cbr));
		SetupStringCombo(this, CTLSEL_RFIDDEV_STOPBITS, PPTXT_COMSTOPBITS, static_cast<long>(Data.Cpp.StopBits + 1));
		SetupStringCombo(this, CTLSEL_RFIDDEV_DATABITS, PPTXT_COMBYTESIZE, static_cast<long>(Data.Cpp.ByteSize));
		SetupStringCombo(this, CTLSEL_RFIDDEV_PARITY,   PPTXT_COMPARITY,   static_cast<long>(Data.Cpp.Parity));
		setCtrlData(CTL_RFIDDEV_GETTRIES,   &Data.Get_NumTries);
		setCtrlData(CTL_RFIDDEV_GETTRIES,   &Data.Put_NumTries);
		setCtrlData(CTL_RFIDDEV_GETTIMEOUT, &Data.Get_Timeout);
		setCtrlData(CTL_RFIDDEV_RELECOUNT,  &Data.ReleCount);
		{
			GoodsCtrlGroup::Rec grp_rec(0, Data.GoodsID, GoodsCtrlGroup::disableEmptyGoods|GoodsCtrlGroup::enableInsertGoods);
			setGroupData(ctlgroupGoods, &grp_rec);
		}
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   ctl_id = 0;
		long   temp_val = 0L;
		char   port[16];
		memzero(port, sizeof(port));
		getCtrlData(CTL_RFIDDEV_ID, &Data.ID);
		getCtrlData(CTL_RFIDDEV_NUMBER, &Data.DeviceNumber);
		getCtrlData(ctl_id = CTL_RFIDDEV_NAME, Data.Name);
		THROW_PP(sstrlen(Data.Name) > 0, PPERR_NAMENEEDED);
		getCtrlData(ctl_id = CTL_RFIDDEV_PORT, port);
		STRNSCPY(Data.Port, port);
		THROW(GetPort(Data.Port, 0));
		Data.Cpp.Cbr = (int16)getCtrlLong(CTLSEL_RFIDDEV_CBR);
		getCtrlData(CTLSEL_RFIDDEV_STOPBITS, &temp_val);
		Data.Cpp.StopBits = (uint8)(temp_val - 1);
		Data.Cpp.ByteSize = (int8)getCtrlLong(CTLSEL_RFIDDEV_DATABITS);
		Data.Cpp.Parity = (int8)getCtrlLong(CTLSEL_RFIDDEV_PARITY);
		getCtrlData(CTL_RFIDDEV_GETTRIES,   &Data.Get_NumTries);
		getCtrlData(CTL_RFIDDEV_GETTRIES,   &Data.Put_NumTries);
		getCtrlData(CTL_RFIDDEV_GETTIMEOUT, &Data.Get_Timeout);
		getCtrlData(CTL_RFIDDEV_RELECOUNT,  &Data.ReleCount);
		THROW_PP(Data.ReleCount > 0 && Data.ReleCount <= 255, PPERR_INVRELECOUNT);
		{
			GoodsCtrlGroup::Rec grp_rec;
			getGroupData(ctlgroupGoods, &grp_rec);
			Data.GoodsID = grp_rec.GoodsID;
			THROW_PP(Data.GoodsID, PPERR_GOODSNEEDED);
		}
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = PPErrorByDialog(this, ctl_id);
		ENDCATCH
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmTest)) {
			if(getDTS(0)) {
				SString reply_buf;
				selectCtrl(CTL_RFIDDEV_SYMB);
				RdObj.Test(Data, reply_buf);
				setStaticText(CTL_RFIDDEV_ST_INFO, reply_buf);
			}
			clearEvent(event);
		}
	}
	PPObjRFIDDevice RdObj;
};

#if 0 // {
// @vmiller
int PPObjRFIDDevice::ExecOper(PPAbstractDevice * pDvc, int cmd, StrAssocArray & rIn, StrAssocArray & rOut)
{
	int    ok = 1;
	if(pDvc) {
		if((ok = pDvc->RunCmd__(cmd, rIn, rOut)) != 1) {
			SString err_msg;
			rOut.Get(0, err_msg);
			if(pDvc->RunCmd__(DVCCMD_GETLASTERRORTEXT, rIn.Clear(), rOut.Clear()))
				rOut.Get(0, err_msg);
			if(err_msg.NotEmpty())
				PPSetError(PPERR_READER, err_msg.ToOem());
			ok = 0;
		}
	}
	return ok;
}
#endif // }

int PPObjRFIDDevice::Test(const PPRFIDDevice & rRec, SString & rRetBuf)
{
	rRetBuf.Z();
#if 0 // {
	int    ok = -1;

	PPAbstractDevice * p_dvc = 0;
	SString temp_buf;
	StrAssocArray in_params, out_params;
	PPAbstractDevice::ConnectionParam cp;
	cp.DeviceNo = rRec.DeviceNumber;
	cp.Address = rRec.Port;
	cp.Cpp = rRec.Cpp;
	THROW(PPAbstractDevice::CreateInstance("PPDevice_Leader", &p_dvc));
	if(p_dvc->OpenConnection(cp) > 0) {
		//
		in_params.Clear();
		THROW(p_dvc->RunCmd(DVCCMD_PING, in_params, out_params));
		if(out_params.Get(DVCCMDPAR_CARD, temp_buf)) {
			rRetBuf = temp_buf;
			//in_params.Clear().Add(DVCCMDPAR_TEXT, "ОТЛИЧНО КАРТА СЧИТАНА!");
			//THROW(p_dvc->RunCmd(DVCCMD_SETTEXT, in_params, out_params));
			in_params.Clear().Add(DVCCMDPAR_TEXT, (temp_buf = "ОТЛИЧНО ВКЛЮЧАЕМ!").Transf(CTRANSF_UTF8_TO_INNER));
			in_params.Add(DVCCMDPAR_COUNT, temp_buf.Z().Cat(3)); // 3 щелчка
			THROW(p_dvc->RunCmd(DVCCMD_TOGGLE, in_params, out_params));
		}
		else {
			rRetBuf = "OK";
			in_params.Clear().Add(DVCCMDPAR_TEXT, (temp_buf = "КАРТУ ДАВАЙ! ДА?").Transf(CTRANSF_UTF8_TO_INNER));
			THROW(p_dvc->RunCmd(DVCCMD_SETTEXT, in_params, out_params));
		}
		ok = 1;
	}
#endif // }

#if 0 // {
	// @vmiller {
	int    ok = -1, port_no = -1;
	PPAbstractDevice * P_AbstrDvc = 0;
	P_AbstrDvc = new PPAbstractDevice(0);
	SString temp_buf;
	StrAssocArray in_params, out_params;

	PPAbstractDevice::ConnectionParam cp;
	cp.DeviceNo = rRec.DeviceNumber;
	cp.Address = rRec.Port;
	cp.Cpp = rRec.Cpp;

	if(IsComDvcSymb(rRec.Port, &port_no) == comdvcsCom)
		if(port_no > 0)
			port_no--;

	P_AbstrDvc->PCpb.Cls = DVCCLS_READER;
	P_AbstrDvc->GetDllName(DVCCLS_READER, /*rRec.ID*/1, P_AbstrDvc->PCpb.DllName); // @vmiller Пока напишем номер утсройства в списке - 1, в списке устройств, перечисленных в ppdrv.ini. Хотя ИД здесь и не подойдет, наверное...
	P_AbstrDvc->IdentifyDevice(P_AbstrDvc->PCpb.DllName);
	// инициализируем
	THROW(ExecOper(P_AbstrDvc, DVCCMD_INIT, in_params, out_params.Clear()));
	// соединяемся
	in_params.Add(DVCPARAM_PORT, temp_buf.Z().Cat(/*port_no*/7)); // @vmiller Пока напишем порт устройства - 7 (ибо все равно через эмулятор)
	THROW(ExecOper(P_AbstrDvc, DVCCMD_CONNECT, in_params, out_params.Clear()));
	// читаем с устройства
	THROW(ExecOper(P_AbstrDvc, DVCCMD_LISTEN, in_params.Clear(), out_params.Clear()));
	out_params.Get(0, temp_buf);
	PPOutputMessage(temp_buf, mfOK);
	// } @vmiller
#endif // }

	int    port_no = -1, ok = 1;
	SCommPort cp;
	CommPortTimeouts cpt;
	if(IsComDvcSymb(rRec.Port, &port_no) == comdvcsCom)
		if(port_no > 0)
			port_no--;
	cp.SetParams(&rRec.Cpp);
	cp.SetReadCyclingParams(10, 10);

	cpt.Get_NumTries = rRec.Get_NumTries;
	cpt.Get_Delay    = rRec.Get_Timeout;
	cpt.Put_NumTries = rRec.Put_NumTries;
	cpt.Put_Delay    = rRec.Put_Timeout;
	cpt.W_Get_Delay  = rRec.Get_Timeout;
	cp.SetTimeouts(&cpt);
	THROW_PP_S(cp.InitPort(port_no, 0, 0), PPERR_SLIB, rRec.Port);
	{
		char   reply_buf[512];
		memzero(reply_buf, sizeof(reply_buf));
		size_t reply_size = 0;
		int    chr;
		for(LDATETIME dtm = getcurdatetime_(); reply_buf[0] == 0 && diffdatetimesec(getcurdatetime_(), dtm) < 10;) {
			while(cp.GetChr(&chr)) {
				reply_buf[reply_size++] = (int8)chr;
				rRetBuf.CatHex((uint8)chr);
			}
		}
		rRetBuf.SetIfEmpty("no reply");
	}
	/*
	//
	// PING CMD
	//
	{
		int8   b;
		int8   cs = 0;
		THROW_SL(cp.PutChr(170));
		b = (int8)rRec.DeviceNumber;
		cs = b;
		THROW_SL(cp.PutChr(b));
		b = 1; // cmd (PING)
		cs ^= b;
		THROW_SL(cp.PutChr(b));
		THROW_SL(cp.PutChr(cs));
		//
		//
		//
		{
			char   reply_buf[512];
			memzero(reply_buf, sizeof(reply_buf));
			size_t reply_size = 0;
			int    chr;
			while(cp.GetChr(&chr)) {
				reply_buf[reply_size++] = (int8)chr;
				rRetBuf.CatHex((uint8)chr);
			}
			if(rRetBuf.Empty())
				rRetBuf = "no reply";
		}
	}
	*/
	CATCH
		ok = PPErrorZ();
		rRetBuf = "Error";
	ENDCATCH
#if 0 // {
	// @vmiller {
	if(P_AbstrDvc) {
		ExecOper(P_AbstrDvc, DVCCMD_DISCONNECT, in_params.Clear(), out_params.Clear());
		ExecOper(P_AbstrDvc, DVCCMD_RELEASE, in_params.Clear(), out_params.Clear());
		delete P_AbstrDvc;
	}
	// } @vmiller
#endif // }
	// delete p_dvc;
	return ok;
}

/*virtual*/int PPObjRFIDDevice::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	int    ta = 0;
	PPRFIDDevice rec;
	RFIDDeviceDialog * p_dlg = 0;
	if(pID && *pID) {
		THROW(Search(*pID, &rec) > 0);
	}
	else
		MEMSZERO(rec);
	THROW(CheckDialogPtr(&(p_dlg = new RFIDDeviceDialog())));
	p_dlg->setDTS(&rec);
	for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
		if(p_dlg->getDTS(&rec) > 0) {
			THROW(StoreItem(PPOBJ_RFIDDEVICE, *pID, &rec, 1));
			*pID = rec.ID;
			valid_data = 1;
			ok = cmOK;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

/*virtual*/int  PPObjRFIDDevice::Browse(void * extraPtr) { return RefObjView(this, PPDS_CRRRFIDDEVICE, 0); }

#define DISPLAY_ROW_SIZE 16

class RFIDDevPrcssr {
public:
	RFIDDevPrcssr();
	~RFIDDevPrcssr();
	int Add(PPRFIDDevice & rRec);
	int Run();
private:
	int AddToBadList(const PPAbstractDevice::ConnectionParam * pParam);
	int ProcessBadComList();
	int IsWait(uint devPos);

	SString   BeginPlay;
	SString   CantPlay;
	SString   GetCard;
	SString   CardNotFound;
	SString   PriceNotDef;
	SString   InternalErr;
	LAssocArray WaitList;
	TSCollection<PPAbstractDevice::ConnectionParam> BadComList;
	TSCollection<PPAbstractDevice> DevList;
	PPObjSCard ScObj;
	PPObjGoods GObj;
};

RFIDDevPrcssr::RFIDDevPrcssr()
{
	SString temp_buf, scard_rest, word;

	PPLoadText(PPTXT_SCARDREST, scard_rest);
	PPLoadText(PPTXT_NOTENOUGHTMONEY, temp_buf);
	CantPlay.CatN(temp_buf, DISPLAY_ROW_SIZE);
	CantPlay.CatCharN(' ', DISPLAY_ROW_SIZE - CantPlay.Len());
	CantPlay.Cat(scard_rest);

	PPLoadText(PPTXT_BEGINPLAY, temp_buf.Z());
	BeginPlay.CatN(temp_buf, DISPLAY_ROW_SIZE);
	BeginPlay.CatCharN(' ', DISPLAY_ROW_SIZE - BeginPlay.Len());
	BeginPlay.Cat(scard_rest);

	PPLoadText(PPTXT_GETSCARD, temp_buf.Z());
	GetCard.CatN(temp_buf, DISPLAY_ROW_SIZE);

	PPLoadText(PPTXT_CARD, temp_buf.Z());
	PPGetWord(PPWORD_NOTFOUND, 0, word.Z());
	CardNotFound.CatN(word, DISPLAY_ROW_SIZE);
	CardNotFound.CatCharN(' ', DISPLAY_ROW_SIZE - CardNotFound.Len());
	CardNotFound.CatN(temp_buf, DISPLAY_ROW_SIZE);

	PPLoadText(PPTXT_PRICENOTDEF, temp_buf.Z());
	PriceNotDef.CatN(temp_buf, DISPLAY_ROW_SIZE);
	PriceNotDef.CatCharN(' ', DISPLAY_ROW_SIZE - PriceNotDef.Len());

	PPLoadString("err_internal", temp_buf);
	InternalErr.CatN(temp_buf, DISPLAY_ROW_SIZE);

	CantPlay.Transf(CTRANSF_INNER_TO_OUTER);
	BeginPlay.Transf(CTRANSF_INNER_TO_OUTER);
	GetCard.Transf(CTRANSF_INNER_TO_OUTER);
	CardNotFound.Transf(CTRANSF_INNER_TO_OUTER);
	PriceNotDef.Transf(CTRANSF_INNER_TO_OUTER);
	InternalErr.Transf(CTRANSF_INNER_TO_OUTER);
}

RFIDDevPrcssr::~RFIDDevPrcssr()
{
}

int RFIDDevPrcssr::AddToBadList(const PPAbstractDevice::ConnectionParam * pParam)
{
	if(pParam) {
		PPAbstractDevice::ConnectionParam * p_cp = new PPAbstractDevice::ConnectionParam;
		*p_cp = *pParam;
		BadComList.insert(p_cp);
	}
	return 1;
}

int RFIDDevPrcssr::Add(PPRFIDDevice & rRec)
{
	int    ok = 1;
	uint   dev_count = DevList.getCount();
	double price = 0;
	PPAbstractDevice * p_dvc = 0;
	PPAbstractDevice::ConnectionParam cp, cp2;

	cp.DeviceNo = rRec.DeviceNumber;
	cp.Address  = rRec.Port;
	cp.Cpp      = rRec.Cpp;
	cp.Cpt.Get_NumTries = rRec.Get_NumTries;
	cp.Cpt.Get_Delay    = rRec.Get_Timeout;
	cp.Cpt.Put_NumTries = rRec.Put_NumTries;
	cp.Cpt.Put_Delay    = rRec.Put_Timeout;
	cp.ReleCount        = rRec.ReleCount;
	cp.GoodsID  = rRec.GoodsID;
	cp.DeviceName       = rRec.Name;
	THROW(PPAbstractDevice::CreateInstance("PPDevice_Leader", &p_dvc));
	//THROW_PP_S(p_dvc->GetSessionPrice(&GObj, &price) > 0, PPERR_INVDEVQUOT, rRec.Name);
	for(uint i = 0; i < dev_count; i++) {
		PPAbstractDevice * p_prmr_dvc = DevList.at(i);
		if(p_prmr_dvc && p_prmr_dvc->GetConnParam(&cp2) && cp2.IsEqualAddr(cp)) {
			cp.NotOwned = 1;
			cp.P_Conn = cp2.P_Conn;
			break;
		}
	}
	DevList.insert(p_dvc);
	if(!p_dvc->OpenConnection(cp)) {
		AddToBadList(&cp);
		THROW(1);
	}
	CATCH
		ZDELETE(p_dvc);
		ok = PPError();
		ok = 0;
	ENDCATCH
	return ok;
}

IMPL_CMPFUNC(ConnectionParam, i1, i2)
{
	const PPAbstractDevice::ConnectionParam * p_i1 = static_cast<const PPAbstractDevice::ConnectionParam *>(i1);
	const PPAbstractDevice::ConnectionParam * p_i2 = static_cast<const PPAbstractDevice::ConnectionParam *>(i2);
	if(p_i1->IsEqualAddr(*p_i2))
		return 0;
	else
		return -1;
}

int RFIDDevPrcssr::Run()
{
	int    ok = 1;
	uint   dev_count = DevList.getCount();
	PPAbstractDevice::ConnectionParam cp;
	for(uint i = 0; i < dev_count; i++) {
		PPAbstractDevice * p_dvc = DevList.at(i);
		if(p_dvc) {
			p_dvc->GetConnParam(&cp);
			if(!BadComList.lsearch(&cp, 0, PTR_CMPFUNC(ConnectionParam)) && IsWait(i) == 0) {
				StrAssocArray in_params, out_params;
				in_params.Z();
				if(p_dvc->RunCmd(DVCCMD_PING, in_params, out_params)) {
					int cmd = 0;
					SString temp_buf;
					if(out_params.GetText(DVCCMDPAR_CARD, temp_buf)) {
						int8   rele_count = 0;
						double price = 0;
						SString msg;
						SCardTbl::Rec sc_rec;
						cmd = DVCCMD_TOGGLE;
						if(ScObj.SearchCode(0, temp_buf, &sc_rec) > 0) {
							if(p_dvc->GetSessionPrice(&GObj, &price) > 0) {
								double amount = 0;
								ScObj.P_Tbl->GetRest(sc_rec.ID, ZERODATE, &amount);
								if((amount - price) > 0.0) {
									TSVector <SCardCore::UpdateRestNotifyEntry> urn_list;
									SCardCore::OpBlock op_blk;
									op_blk.SCardID = sc_rec.ID;
									op_blk.PrevRest = amount;
									op_blk.Amount   = -price;
									op_blk.Flags |= SCardCore::OpBlock::fEdit;
									getcurdatetime(&op_blk.Dtm);
									amount -= price;
									if(ScObj.P_Tbl->PutOpBlk(op_blk, &urn_list, 1) > 0) {
										ScObj.FinishSCardUpdNotifyList(urn_list);
										msg.Printf(BeginPlay.cptr(), amount);
										rele_count = cp.ReleCount;
									}
									else {
										msg = InternalErr;
										PPError();
									}
								}
								else
									msg.Printf(CantPlay.cptr(), amount);
							}
							else {
								msg = PriceNotDef;
								PPError(PPERR_INVDEVQUOT, cp.DeviceName);
							}
						}
						else
							msg.Printf(CardNotFound.cptr(), temp_buf.cptr());
						in_params.Z().Add(DVCCMDPAR_TEXT, msg.cptr());
						in_params.Add(DVCCMDPAR_COUNT, temp_buf.Z().Cat(rele_count));

						/*
						LTIME tm;
						getcurtime(&tm);
						WaitList.AddUnique(i, tm.v, 0);
						*/
					}
					else {
						cmd = DVCCMD_SETTEXT;
						in_params.Z().Add(DVCCMDPAR_TEXT, (const char *)GetCard);
					}
					if(!p_dvc->RunCmd(cmd, in_params, out_params))
						AddToBadList(&cp);
				}
				else
					AddToBadList(&cp);
			}
		}
		SDelay(500);
	}
	ProcessBadComList();
	return ok;
}

int RFIDDevPrcssr::ProcessBadComList()
{
	uint bad_com_count = BadComList.getCount();
	uint dev_count = DevList.getCount();
	for(long b = bad_com_count - 1; b >= 0; b--) {
		int connected = 1;
		PPAbstractDevice::ConnectionParam cp;
		cp = *BadComList.at(b);
		cp.NotOwned = 0;
		cp.P_Conn = 0;
		for(uint j = 0; j < dev_count; j++) {
			PPAbstractDevice::ConnectionParam cp2;
			PPAbstractDevice * p_dvc = DevList.at(j);
			if(p_dvc && p_dvc->GetConnParam(&cp2) > 0 && cp.IsEqualAddr(cp2)) {
				cp2.P_Conn   = cp.P_Conn;
				cp2.NotOwned = cp.NotOwned;
				p_dvc->CloseConnection();
				if(p_dvc->OpenConnection(cp2) > 0) {
					p_dvc->GetConnParam(&cp2);
					cp.P_Conn   = cp2.P_Conn;
					cp.NotOwned = 1;
				}
				else
					connected = 0;
			}
		}
		if(connected)
			BadComList.atFree(b);
	}
	return 1;
}

int RFIDDevPrcssr::IsWait(uint devPos)
{
	int    r = 0;
	long   v = 0;
	uint   pos = 0;
	LTIME  cur_tm = getcurtime_();
	if(WaitList.Search(devPos, &v, &pos)) {
		LTIME tm;
		tm.v = v;
		if(labs(DiffTime(cur_tm, tm, 3)) >= 15)
			WaitList.atFree(pos);
		else
			r = 1;
	}
	return r;
}

class JOB_HDL_CLS(RFIDDEVICEPRCSSR) : public PPJobHandler {
public:
	JOB_HDL_CLS(RFIDDEVICEPRCSSR)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		PPIDArray dev_list;
		ListToListData l2_data(PPOBJ_RFIDDEVICE, 0, &dev_list);
		THROW_INVARG(pParam);
		sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize() != 0)
			pParam->Read(&dev_list, 0);
		if(ListToListDialog(&l2_data) > 0) {
			ok = 1;
			THROW(pParam->Z().Write(&dev_list, 0));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1, run = 0;
		SString err_msg;
		PPIDArray idlist;
		PPObjRFIDDevice rfid_obj;
		RFIDDevPrcssr prcssr;
		THROW(pParam->Read(&idlist, 0));
		for(uint i = 0; i < idlist.getCount(); i++) {
			PPRFIDDevice rfid_rec;
			if(rfid_obj.Search(idlist.at(i), &rfid_rec) > 0) {
				prcssr.Add(rfid_rec);
				run = 1;
			}
		}
		while(run) {
			prcssr.Run();
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(RFIDDEVICEPRCSSR);
//
//
//
class JOB_HDL_CLS(SCARDSELECTIONPRCSSR) : public PPJobHandler {
public:
	JOB_HDL_CLS(SCARDSELECTIONPRCSSR)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		SCardSelPrcssrParam param;
		PPViewSCard view;
		SCardSelPrcssrDialog * p_dlg = new SCardSelPrcssrDialog(&view, 1);
		THROW_INVARG(pParam);
		THROW(CheckDialogPtr(&p_dlg));
		sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize() != 0)
			param.Read(*pParam, 0);
		p_dlg->setDTS(&param);
		for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
			if(p_dlg->getDTS(&param) > 0) {
				ok = 1;
				valid_data = 1;
			}
			else
				PPError();
		}
		if(ok > 0) {
		   	THROW(param.Write(pParam->Z(), 0));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		delete p_dlg;
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		SCardSelPrcssrParam param;
		PPViewSCard view;
		THROW(param.Read(*pParam, 0));
		THROW(view.ProcessSelection(&param, 0));
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(SCARDSELECTIONPRCSSR);
//
//
//
class JOB_HDL_CLS(UPDATEQUOTS) : public PPJobHandler {
public:
	JOB_HDL_CLS(UPDATEQUOTS)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		QuotUpdFilt filt;
		THROW_INVARG(pParam);
		sav_offs = pParam->GetRdOffs();
		if(!pParam->GetAvailableSize()) {
			filt.LocList.Add(LConfig.Location);
			filt.Flags |= QuotUpdFilt::fExistOnly;
			filt.Flags &= ~QuotUpdFilt::fWarnExistsAbsQuot;
		}
		else
			filt.Read(*pParam, 0);
		if(EditQuotUpdDialog(&filt) > 0) {
			THROW(filt.Write(pParam->Z(), 0));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		QuotUpdFilt filt;
		THROW_INVARG(pParam);
		THROW(filt.Read(*pParam, 0));
		filt.Flags &= ~QuotUpdFilt::fWarnExistsAbsQuot;
		THROW(UpdateQuots(&filt));
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(UPDATEQUOTS);
//
//
//
int Helper_ExecuteNF_WithSerializedParam(SBuffer * pParam); // @prototype (ppcmd.cpp)

class JOB_HDL_CLS(EXPORTVIEW) : public PPJobHandler {
public:
	JOB_HDL_CLS(EXPORTVIEW)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		PPView::ExecNfViewParam param;
		THROW_INVARG(pParam);
		const size_t sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize()) {
			THROW(param.Read(*pParam, 0));
		}
		if(PPView::EditExecNfViewParam(param) > 0) {
			THROW(param.Write(pParam->Z(), 0));
			ok = 1;
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		return Helper_ExecuteNF_WithSerializedParam(pParam);
	}
};

IMPLEMENT_JOB_HDL_FACTORY(EXPORTVIEW);
//
//
//
// @Muxa {
int UhttIndexGoods(uint setSz, const SString & rOutPath);
int UhttIndexRandomGoods(uint setSz, const SString & rOutPath);
int UhttIndexChangedGoods(const SString & rOutPath);
int UhttIndexBrands(const SString & rOutPath);
int UhttIndexStores(const SString & rOutPath);
int UhttIndexPersons(uint setSz, PPID personKindID, const SString & rOutPath);

class JOB_HDL_CLS(EXPORTOBJLISTTOHTML) : public PPJobHandler {
public:
	struct Param {
		enum {
			fExpAllGoods = 0x0001,
			fExpRndGoods = 0x0002,
			fExpChgGoods = 0x0004
		};
		enum {
			fExpAllBrands = 0x0001
		};
		enum {
			fExpAllStores = 0x0001
		};
		Param() : Flags(0), PersonKindForExport(0), PersonSetSz(0), GoodsFlags(0), GoodsAllSetSz(0), GoodsRndSetSz(0), BrandsFlags(0), StoresFlags(0)
		{
			memzero(ReserveStart, sizeof(ReserveStart));
		}
		int    Write(SBuffer & rBuf, long)
		{
			int    ok = 1;
			THROW_SL(rBuf.Write(ReserveStart, sizeof(ReserveStart)));
			THROW_SL(rBuf.Write(PersonKindForExport));
			THROW_SL(rBuf.Write(PersonSetSz));
			THROW_SL(rBuf.Write(Flags));
			THROW_SL(rBuf.Write(OutPath));
			THROW_SL(rBuf.Write(GoodsFlags));
			THROW_SL(rBuf.Write(GoodsAllSetSz));
			THROW_SL(rBuf.Write(GoodsRndSetSz));
			THROW_SL(rBuf.Write(BrandsFlags));
			THROW_SL(rBuf.Write(StoresFlags));
			CATCHZOK
			return ok;
		}
		int    Read(SBuffer & rBuf, long)
		{
			int    ok = 1;
			THROW_SL(rBuf.ReadV(ReserveStart, sizeof(ReserveStart)));
			THROW_SL(rBuf.Read(PersonKindForExport));
			THROW_SL(rBuf.Read(PersonSetSz));
			THROW_SL(rBuf.Read(Flags));
			THROW_SL(rBuf.Read(OutPath));
			THROW_SL(rBuf.Read(GoodsFlags));
			THROW_SL(rBuf.Read(GoodsAllSetSz));
			THROW_SL(rBuf.Read(GoodsRndSetSz));
			THROW_SL(rBuf.Read(BrandsFlags));
			THROW_SL(rBuf.Read(StoresFlags));
			CATCHZOK
			return ok;
		}
		uint8   ReserveStart[24];
		long    PersonKindForExport;
		long    PersonSetSz;
		long    Flags;
		SString OutPath;
		long    GoodsFlags;
		long    GoodsAllSetSz;
		long    GoodsRndSetSz;
		long    BrandsFlags;
		long    StoresFlags;      // @v7.7.5 @muxa
	};
	JOB_HDL_CLS(EXPORTOBJLISTTOHTML)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		size_t sav_offs = 0;
		Param  param;
		TDialog * dlg = 0;
		SString temp_buf;
		THROW_INVARG(pParam);
		sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize())
			THROW(param.Read(*pParam, 0));
		dlg = new TDialog(DLG_JOB_EXPOBJHTM);
		THROW(CheckDialogPtrErr(&dlg));
		{
			FileBrowseCtrlGroup::Setup(dlg, CTLBRW_JOB_EXPOBJHTM_OUT, CTL_JOB_EXPOBJHTM_OUT, 1, 0, 0,
				FileBrowseCtrlGroup::fbcgfPath|FileBrowseCtrlGroup::fbcgfAllowNExists);
			dlg->setCtrlString(CTL_JOB_EXPOBJHTM_OUT, param.OutPath);
			dlg->setCtrlLong(CTL_JOB_EXPOBJHTM_GFLAGS, param.GoodsFlags);
			dlg->setCtrlLong(CTL_JOB_EXPOBJHTM_GASETS, param.GoodsAllSetSz);
			dlg->setCtrlLong(CTL_JOB_EXPOBJHTM_GRSETS, param.GoodsRndSetSz);
			dlg->setCtrlLong(CTL_JOB_EXPOBJHTM_BFLAGS, param.BrandsFlags);
			dlg->setCtrlLong(CTL_JOB_EXPOBJHTM_SFLAGS, param.StoresFlags);

			SetupPPObjCombo(dlg, CTLSEL_JOB_EXPOBJHTM_PK, PPOBJ_PERSONKIND, param.PersonKindForExport, 0);
			dlg->setCtrlLong(CTL_JOB_EXPOBJHTM_PSETS, param.PersonSetSz);

			if(ExecView(dlg) == cmOK) {
				dlg->getCtrlString(CTL_JOB_EXPOBJHTM_OUT, param.OutPath);
				param.GoodsFlags = dlg->getCtrlLong(CTL_JOB_EXPOBJHTM_GFLAGS);
				param.GoodsAllSetSz = dlg->getCtrlLong(CTL_JOB_EXPOBJHTM_GASETS);
				param.GoodsRndSetSz = dlg->getCtrlLong(CTL_JOB_EXPOBJHTM_GRSETS);
				param.BrandsFlags = dlg->getCtrlLong(CTL_JOB_EXPOBJHTM_BFLAGS);
				param.StoresFlags = dlg->getCtrlLong(CTL_JOB_EXPOBJHTM_SFLAGS);
				param.PersonKindForExport = dlg->getCtrlLong(CTLSEL_JOB_EXPOBJHTM_PK);
				param.PersonSetSz = dlg->getCtrlLong(CTL_JOB_EXPOBJHTM_PSETS);
				THROW(param.Write(pParam->Z(), 0));
			}
			else
				pParam->SetRdOffs(sav_offs);
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = PPErrorZ();
		ENDCATCH
		delete dlg;
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		Param  param;
		THROW_INVARG(pParam);
		THROW(param.Read(*pParam, 0));
		if(param.OutPath.NotEmpty()) {
			if(param.GoodsFlags & param.fExpAllGoods)
				UhttIndexGoods(param.GoodsAllSetSz, param.OutPath);
			if(param.GoodsFlags & param.fExpRndGoods)
				UhttIndexRandomGoods(param.GoodsRndSetSz, param.OutPath);
			if(param.GoodsFlags & param.fExpChgGoods)
				UhttIndexChangedGoods(param.OutPath);
			if(param.BrandsFlags & param.fExpAllBrands)
				UhttIndexBrands(param.OutPath);
			if(param.StoresFlags & param.fExpAllStores)
				UhttIndexStores(param.OutPath);
			if(param.PersonKindForExport)
				UhttIndexPersons(param.PersonSetSz, param.PersonKindForExport, param.OutPath);
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(EXPORTOBJLISTTOHTML);
// } @Muxa
//
//
//
class JOB_HDL_CLS(UHTTEXPORTGOODSREST) : public PPJobHandler {
public:
	JOB_HDL_CLS(UHTTEXPORTGOODSREST)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1, r = 1;
		uint   val = 0;
		PPViewGoodsRest gr_view;
		GoodsRestFilt filt;
		size_t sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize() == 0) {
			;
		}
		else {
			filt.Read(*pParam, 0);
		}
		if(r > 0 && gr_view.EditBaseFilt(&filt) > 0) {
			if(filt.Write(pParam->Z(), 0))
				ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		if(pParam && pParam->GetAvailableSize()) {
			PPViewGoodsRest gr_view;
			GoodsRestFilt filt;
			THROW(filt.Read(*pParam, 0));
			THROW(gr_view.Init_(&filt));
			THROW(gr_view.ExportUhtt(1));
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(UHTTEXPORTGOODSREST);
//
//
//
class JOB_HDL_CLS(IMPORTBILLS) : public PPJobHandler {
public:
	JOB_HDL_CLS(IMPORTBILLS)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		int    r = 1;
		uint   val = 0;
		SSerializeContext sctx;
		PPBillImpExpBaseProcessBlock blk;
		const size_t sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize() == 0) {
			;
		}
		else {
			THROW(blk.SerializeParam(-1, *pParam, &sctx));
		}
		if(blk.Select(1) > 0) {
			THROW(blk.SerializeParam(+1, pParam->Z(), &sctx));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCHZOKPPERR
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		SSerializeContext sctx;
		PPBillImporter prcssr;
		if(pParam->GetAvailableSize()) {
			prcssr.Init();
			THROW(prcssr.SerializeParam(-1, *pParam, &sctx));
			THROW(prcssr.LoadConfig(1));
			THROW(prcssr.Run());
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(IMPORTBILLS);
//
// 
//
class JOB_HDL_CLS(IMPORTCCHECKS) : public PPJobHandler {
public:
	JOB_HDL_CLS(IMPORTCCHECKS)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1, r = 1;
		uint   val = 0;
		SSerializeContext sctx;
		PPCCheckImporter importer;
		PPCCheckImpExpParam param;
		const size_t sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize() == 0) {
			;
		}
		else {
			THROW(importer.SerializeParam(-1, *pParam, &sctx));
		}
		if(importer.Select(&param, 1, false/*interactive*/) > 0) {
			THROW(importer.SerializeParam(+1, pParam->Z(), &sctx));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCHZOKPPERR
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		SSerializeContext sctx;
		PPCCheckImporter prcssr;
		if(pParam->GetAvailableSize()) {
			prcssr.Init(0, true/*nonInteractive*/);
			THROW(prcssr.SerializeParam(-1, *pParam, &sctx));
			THROW(prcssr.LoadConfig(1));
			THROW(prcssr.Run());
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(IMPORTCCHECKS);
//
//
//
class JOB_HDL_CLS(PROCESSOBJTEXT) : public PPJobHandler {
public:
	JOB_HDL_CLS(PROCESSOBJTEXT)(const PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrObjText prc;
			PrcssrObjTextFilt filt;
			const size_t preserve_offs = pParam->GetRdOffs();
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			else
				pParam->SetRdOffs(preserve_offs);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		if(pParam) {
			PrcssrObjTextFilt filt;
			if(filt.Read(*pParam, 0)) {
				PrcssrObjText prc;
				THROW(prc.Init(&filt));
				THROW(prc.Run());
			}
			else {
				THROW(DoProcessObjText(0));
			}
		}
		else {
			THROW(DoProcessObjText(0));
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(PROCESSOBJTEXT);
//
//
//
class JOB_HDL_CLS(PERSONEVENTBYREADER) : public PPJobHandler {
public:
	JOB_HDL_CLS(PERSONEVENTBYREADER)(const PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		AddPersonEventFilt filt;
		THROW_INVARG(pParam);
        sav_offs = pParam->GetRdOffs();
		filt.Read(*pParam, 0);
		if(filt.Edit() > 0) {
			THROW(filt.Write(pParam->Z(), 0));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		if(pParam) {
			PPObjPersonEvent pe_obj;
			AddPersonEventFilt filt;
			THROW(filt.Read(*pParam, 0));
			// (Функция интерактивная, из-за нее падает сервер) pe_obj.ProcessDeviceInput(filt);
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(PERSONEVENTBYREADER);
//
//
//
// @vmiller
class TSessAutoSmsFiltDialog : public TDialog {
public:
	TSessAutoSmsFiltDialog() : TDialog(DLG_JTSASMSFILT)
	{
		SetupCalPeriod(CTLCAL_JTSASMSFILT_STPERIOD, CTL_JTSASMSFILT_STPRD);
		SetupCalPeriod(CTLCAL_JTSASMSFILT_FNPERIOD, CTL_JTSASMSFILT_FNPRD);
	}
	int setDTS(const TSessionFilt *);
	int getDTS(TSessionFilt *);
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(TVCOMMAND) {
			clearEvent(event);
		}
	}
	TSessionFilt Data;
	PPViewTSession View;
};

int TSessAutoSmsFiltDialog::getDTS(TSessionFilt * pData)
{
	PrcTechCtrlGroup::Rec ptcg_rec;
	if(Data.Flags & TSessionFilt::fManufPlan) {
		getCtrlData(CTLSEL_JTSASMSFILT_PRC, &Data.PrcID);
	}
	else {
		getGroupData(2, &ptcg_rec);
		Data.PrcID  = ptcg_rec.PrcID;
		Data.TechID = ptcg_rec.TechID;
		Data.ArID   = ptcg_rec.ArID;
	}
	GetClusterData(CTL_JTSASMSFILT_STATUS, &Data.StatusFlags);
	GetClusterData(CTL_JTSASMSFILT_FLAGS, &Data.Flags);
	GetPeriodInput(this, CTL_JTSASMSFILT_STPRD, &Data.StPeriod);
	getCtrlData(CTL_JTSASMSFILT_STTIME, &Data.StTime);
	if(!Data.StPeriod.low)
		Data.StTime = ZEROTIME;
	GetPeriodInput(this, CTL_JTSASMSFILT_FNPRD, &Data.FnPeriod);
	getCtrlData(CTL_JTSASMSFILT_FNTIME, &Data.FnTime);
	if(!Data.FnPeriod.upp)
		Data.FnTime = ZEROTIME;
	ASSIGN_PTR(pData, Data);
	return 1;
}

int TSessAutoSmsFiltDialog::setDTS(const TSessionFilt * pData)
{
	const uint grp_prctech = 2;
	RVALUEPTR(Data, pData);
	PrcTechCtrlGroup::Rec ptcg_rec;
	ptcg_rec.PrcID = Data.PrcID;
	if(Data.Flags & TSessionFilt::fManufPlan) {
 		addGroup(grp_prctech, new PrcTechCtrlGroup(CTLSEL_JTSASMSFILT_PRC, 0, 0, 0, 0, 0));
		if(Data.PrcID == 0)
			ptcg_rec.PrcParentID = PRCEXDF_GROUP;
 		setGroupData(grp_prctech, &ptcg_rec);
	}
	else {
 		addGroup(grp_prctech, new PrcTechCtrlGroup(CTLSEL_JTSASMSFILT_PRC, CTLSEL_JTSASMSFILT_TECH,
 			CTL_JTSASMSFILT_ST_GOODS, CTLSEL_JTSASMSFILT_AR, 0, cmSelTechByGoods));
		ptcg_rec.TechID = Data.TechID;
		ptcg_rec.ArID   = Data.ArID;
		ptcg_rec.Ar2ID  = 0;
		ptcg_rec.IdleStatus = false;
 		setGroupData(grp_prctech, &ptcg_rec);
	}
	AddClusterAssocDef(CTL_JTSASMSFILT_STATUS, 0, (1 << TSESST_PLANNED));
	AddClusterAssoc(CTL_JTSASMSFILT_STATUS, 1, (1 << TSESST_PENDING));
	AddClusterAssoc(CTL_JTSASMSFILT_STATUS, 2, (1 << TSESST_INPROCESS));
	AddClusterAssoc(CTL_JTSASMSFILT_STATUS, 3, (1 << TSESST_CLOSED));
	AddClusterAssoc(CTL_JTSASMSFILT_STATUS, 4, (1 << TSESST_CANCELED));
	SetClusterData(CTL_JTSASMSFILT_STATUS, Data.StatusFlags);

	AddClusterAssoc(CTL_JTSASMSFILT_FLAGS, 0, TSessionFilt::fSuperSessOnly);
	SetClusterData(CTL_JTSASMSFILT_FLAGS, Data.Flags);

	SetPeriodInput(this, CTL_JTSASMSFILT_STPRD, Data.StPeriod);
	setCtrlData(CTL_JTSASMSFILT_STTIME, &Data.StTime);
	SetPeriodInput(this, CTL_JTSASMSFILT_FNPRD, Data.FnPeriod);
	setCtrlData(CTL_JTSASMSFILT_FNTIME, &Data.FnTime);
	return 1;
}

class JOB_HDL_CLS(TSESSAUTOSMS) : public PPJobHandler {
public:
	JOB_HDL_CLS(TSESSAUTOSMS)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		int    valid_data = 0;
		TSessionFilt filt;
		TSessAutoSmsFiltDialog * p_dlg = new TSessAutoSmsFiltDialog;
		const size_t sav_offs = pParam ? pParam->GetRdOffs() : 0;
		THROW_INVARG(pParam);
		THROW(CheckDialogPtr(&p_dlg));
		if(pParam->GetAvailableSize() != 0)
			filt.Read(*pParam, 0);
		p_dlg->setDTS(&filt);
		for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
			if(p_dlg->getDTS(&filt) > 0)
				ok = valid_data = 1;
			else
				PPError();
		}
		if(ok > 0) {
			THROW(filt.Write(pParam->Z(), 0));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		delete p_dlg;
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		const  PPCommConfig & r_ccfg = CConfig;
		SString fmt_buf;
		SString msg_buf;
		TSessionFilt filt;
		CoInitialize(NULL);
		THROW(filt.Read(*pParam, 0) > 0);
		if(r_ccfg.Flags & CCFLG_DEBUG) {
			PPLoadText(PPTXT_LOG_JOBTSESSAUTOSMS_READFILT, msg_buf);
			PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
		}
		{
			PPViewTSession view;
			THROW(view.Init_(&filt));
			ok = view.SendAutoSms();
			if(r_ccfg.Flags & CCFLG_DEBUG) {
				PPFormatT(PPTXT_LOG_JOBTSESSAUTOSMS_DONE, &msg_buf, ok);
				PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
			}
		}
		CATCHZOK
		CoUninitialize();
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(TSESSAUTOSMS);
//
//
//
class JOB_HDL_CLS(SENDNOTIFIESBYRECENTSCOPS) : public PPJobHandler {
public:
	struct Param {
		Param() : Ver(1), Since(ZERODATETIME)
		{
			memzero(Reserve, sizeof(Reserve));
		}
		int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
		{
			int    ok = 1;
			THROW_SL(pSCtx->Serialize(dir, Ver, rBuf));
			THROW_SL(pSCtx->Serialize(dir, Since, rBuf));
			THROW_SL(pSCtx->SerializeBlock(dir, sizeof(Reserve), Reserve, rBuf, 1));
			CATCHZOK
			return ok;
		}
		uint32 Ver;
		LDATETIME Since;
		uint8  Reserve[32];
	};
	JOB_HDL_CLS(SENDNOTIFIESBYRECENTSCOPS)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		TDialog * dlg = new TDialog(DLG_JOB_SNRSCO);
		Param  filt;
		SSerializeContext sctx;
		const size_t sav_offs = pParam->GetRdOffs();
		THROW_INVARG(pParam);
		if(pParam->GetAvailableSize() != 0)
			THROW(filt.Serialize(-1, *pParam, &sctx));
		THROW(CheckDialogPtr(&dlg));
		dlg->setCtrlDatetime(CTL_JOB_SNRSCO_DT, CTL_JOB_SNRSCO_TM, filt.Since);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlDatetime(CTL_JOB_SNRSCO_DT, CTL_JOB_SNRSCO_TM, filt.Since);
			THROW(filt.Serialize(+1, pParam->Z(), &sctx));
			ok = 1;
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		delete dlg;
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		Param  filt;
		SSerializeContext sctx;
		THROW(filt.Serialize(-1, *pParam, &sctx));
		{
			LDATETIME since;
			since.d = filt.Since.d.getactual(ZERODATE);
			since.t = filt.Since.t;
			if(checkdate(since.d)) {
				long dd = diffdate(getcurdate_(), since.d);
				if(dd >= 0 && dd < 7) {
					PPObjSCard sc_obj;
					THROW(sc_obj.NotifyAboutRecentOps(since));
				}
			}
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(SENDNOTIFIESBYRECENTSCOPS);
//
//
//
class JOB_HDL_CLS(EXPORTDBTBLTRANSFER) : public PPJobHandler {
public:
	JOB_HDL_CLS(EXPORTDBTBLTRANSFER)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		PPDbTableXmlExportParam_TrfrBill filt;
		SSerializeContext sctx;
		const size_t sav_offs = pParam->GetRdOffs();
		THROW_INVARG(pParam);
		if(pParam->GetAvailableSize() != 0)
			THROW(filt.Serialize(-1, *pParam, &sctx));
		if(PPDbTableXmlExportParam_TrfrBill::Edit(&filt) > 0) {
			THROW(filt.Serialize(+1, pParam->Z(), &sctx));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		PPDbTableXmlExportParam_TrfrBill filt;
		SSerializeContext sctx;
		THROW(filt.Serialize(-1, *pParam, &sctx));
		{
			PPDbTableXmlExporter_Transfer prc(filt);
            THROW(prc.Run(filt.FileName));
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(EXPORTDBTBLTRANSFER);
//
//
//
class JOB_HDL_CLS(EXPORTDBTBLBILL) : public JOB_HDL_CLS(EXPORTDBTBLTRANSFER) {
public:
	JOB_HDL_CLS(EXPORTDBTBLBILL)(PPJobDescr * pDescr) : JOB_HDL_CLS(EXPORTDBTBLTRANSFER)(pDescr)
	{
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		PPDbTableXmlExportParam_TrfrBill filt;
		SSerializeContext sctx;
		THROW(filt.Serialize(-1, *pParam, &sctx));
		{
			PPDbTableXmlExporter_Bill prc(filt);
            THROW(prc.Run(filt.FileName));
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(EXPORTDBTBLBILL);
//
//
//
class JOB_HDL_CLS(PROCESSEDI) : public PPJobHandler {
public:
	JOB_HDL_CLS(PROCESSEDI)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr) { return -1; }
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		return -1;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(PROCESSEDI);
//
//
//
class JOB_HDL_CLS(PROCESSTSESSION) : public PPJobHandler {
public:
	JOB_HDL_CLS(PROCESSTSESSION)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1, r = 0;
		PrcssrTSessMaintenance::Param param;
		PrcssrTSessMaintenance prcssr;
		const size_t sav_offs = pParam->GetRdOffs();
		if((r = ReadParam(*pParam, &param, sizeof(param))) != 0) {
			if((ok = prcssr.EditParam(&param)) > 0) {
				WriteParam(pParam->Z(), &param, sizeof(param));
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		PrcssrTSessMaintenance::Param param;
		PrcssrTSessMaintenance prcssr;
		THROW(ReadParam(*pParam, &param, sizeof(param)));
		THROW(prcssr.Init(&param));
		THROW(prcssr.Run());
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(PROCESSTSESSION);
//
//
//
class JOB_HDL_CLS(QUERYEGAIS) : public PPJobHandler {
public:
	JOB_HDL_CLS(QUERYEGAIS)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		PPEgaisProcessor prc(PPEgaisProcessor::cfUseVerByConfig, 0, 0); // @instantiation(PPEgaisProcessor)
		PPEgaisProcessor::QueryParam filt;
		SSerializeContext sctx;
		const size_t sav_offs = pParam->GetRdOffs();
		THROW_INVARG(pParam);
		if(pParam->GetAvailableSize() != 0)
			THROW(filt.Serialize(-1, *pParam, &sctx));
		if(prc.EditQueryParam(&filt)) {
			THROW(filt.Serialize(+1, pParam->Z(), &sctx));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		PPEgaisProcessor::QueryParam filt;
		SSerializeContext sctx;
		THROW(filt.Serialize(-1, *pParam, &sctx));
		{
			PPEgaisProcessor prc(PPEgaisProcessor::cfUseVerByConfig, 0, 0); // @instantiation(PPEgaisProcessor)
            THROW(prc.ImplementQuery(filt));
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(QUERYEGAIS);
//
//
//
class JOB_HDL_CLS(VETISINTERCHANGE) : public PPJobHandler {
public:
	JOB_HDL_CLS(VETISINTERCHANGE)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		VetisDocumentFilt filt;
		SSerializeContext sctx;
		const size_t sav_offs = pParam->GetRdOffs();
		THROW_INVARG(pParam);
		if(pParam->GetAvailableSize())
			THROW(filt.Serialize(-1, *pParam, &sctx));
		if(PPViewVetisDocument::EditInterchangeParam(&filt) > 0) {
			THROW(filt.Serialize(+1, pParam->Z(), &sctx));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		SSerializeContext sctx;
		VetisDocumentFilt filt;
		THROW(filt.Serialize(-1, *pParam, &sctx));
		THROW(PPViewVetisDocument::RunInterchangeProcess(&filt));
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(VETISINTERCHANGE);
//
//
//
class JOB_HDL_CLS(TIMESERIESSA) : public PPJobHandler {
public:
	JOB_HDL_CLS(TIMESERIESSA)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrTsStrategyAnalyze prc;
			PrcssrTsStrategyAnalyzeFilt filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		PrcssrTsStrategyAnalyze prc;
		PrcssrTsStrategyAnalyzeFilt filt;
		THROW(pParam && filt.Read(*pParam, 0));
		THROW(prc.Init(&filt));
		THROW(prc.Run());
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(TIMESERIESSA);
//
//
// ExportPrjTasks
class JOB_HDL_CLS(EXPORTPRJTASKS) : public PPJobHandler {
public:
	JOB_HDL_CLS(EXPORTPRJTASKS)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PPViewPrjTask view;
			PrjTaskFilt filt;
			if(filt.Read(*pParam, 0)) {
				;
			}
			if(view.EditBaseFilt(&filt) > 0) {
				if(filt.Write(*pParam, 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		PPViewPrjTask view;
		PrjTaskFilt filt;
		THROW(pParam && filt.Read(*pParam, 0));
		THROW(view.Init_(&filt));
		THROW(view.Transmit(0, 2));
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(EXPORTPRJTASKS);
//
//
//
class JOB_HDL_CLS(FTSINDEXING) : public PPJobHandler {
public:
	JOB_HDL_CLS(FTSINDEXING)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr) { return -1; }
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = -1;
		{
			//
			// Индексация данных Stylo-Q (собственно, ради нее задача и была сделана)
			//
			// @v11.4.3 @debug {
			/*{
				SLS.SaturateRvlStrPool(512);
				SLS.SaturateRvlStrUPool(512);
			}*/
			// } @v11.4.3 @debug
			SString temp_buf;
			StyloQCore::SvcDbSymbMap dbmap;
			if(StyloQCore::GetDbMap(dbmap)) {
				TSCollection <SBinaryChunk> reckoned_svc_id_list;
				for(uint i = 0; i < dbmap.getCount(); i++) {
					const StyloQCore::SvcDbSymbMapEntry * p_map_entry = dbmap.at(i);
					if(p_map_entry && p_map_entry->DbSymb.NotEmpty() && p_map_entry->SvcIdent.Len()) {
						PPSession::LimitedDatabaseBlock * p_ldb = DS.LimitedOpenDatabase(p_map_entry->DbSymb, PPSession::lodfReference|PPSession::lodfStyloQCore|PPSession::lodfSysJournal);
						if(p_ldb && p_ldb->P_Sqc) {
							ok = p_ldb->P_Sqc->IndexingContent();
						}
						ZDELETE(p_ldb); // @v11.4.3 @fix
						ok = 1;
					}
				}
			}
		}
		//CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(FTSINDEXING);
//
//
//
class JOB_HDL_CLS(STYLOQSENDINDEXINGCONTENT) : public PPJobHandler {
public:
	JOB_HDL_CLS(STYLOQSENDINDEXINGCONTENT)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr) { return -1; }
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		return PPStyloQInterchange::ExecuteIndexingRequest(false);
	}
};

IMPLEMENT_JOB_HDL_FACTORY(STYLOQSENDINDEXINGCONTENT);
//
//
//
class JOB_HDL_CLS(STYLOQPREPAREAHEAD) : public PPJobHandler {
public:
	JOB_HDL_CLS(STYLOQPREPAREAHEAD)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr) { return -1; }
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		return PPStyloQInterchange::PrepareAhed(false);
	}
};

IMPLEMENT_JOB_HDL_FACTORY(STYLOQPREPAREAHEAD);
//
//
//
class JOB_HDL_CLS(APTEKARUINTERCHANGE) : public PPJobHandler {
public:
	JOB_HDL_CLS(APTEKARUINTERCHANGE)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr) 
	{ 
		int    ok = -1;
		if(pParam) {
			PrcssrAptekaRu prc;
			PrcssrAptekaRuFilt filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		PrcssrAptekaRu prc;
		PrcssrAptekaRuFilt filt;
		THROW(pParam);
		THROW(filt.Read(*pParam, 0));
		THROW(prc.Init(&filt));
		THROW(prc.Run());
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(APTEKARUINTERCHANGE);
//
//
//
class JOB_HDL_CLS(MARKETPLACEINTERCHANGE) : public PPJobHandler {
public:
	JOB_HDL_CLS(MARKETPLACEINTERCHANGE)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr) 
	{ 
		int    ok = -1;
		if(pParam) {
			PrcssrMarketplaceInterchange prc;
			MarketplaceInterchangeFilt filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		PrcssrMarketplaceInterchange prc;
		MarketplaceInterchangeFilt filt;
		THROW(pParam);
		THROW(filt.Read(*pParam, 0));
		THROW(prc.Init(&filt));
		THROW(prc.Run());
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(MARKETPLACEINTERCHANGE);
//
//
//
class JOB_HDL_CLS(GATHERCLIENTACTIVITYSTAT) : public PPJobHandler {
public:
	JOB_HDL_CLS(GATHERCLIENTACTIVITYSTAT)(PPJobDescr * pDescr) : PPJobHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, void * extraPtr) 
	{ 
		int    ok = -1;
		if(pParam) {
			PrcssrClientActivityStatistics prc;
			PrcssrClientActivityStatisticsFilt filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, void * extraPtr)
	{
		int    ok = 1;
		PrcssrClientActivityStatistics prc;
		PrcssrClientActivityStatisticsFilt filt;
		THROW(pParam);
		THROW(filt.Read(*pParam, 0));
		THROW(prc.Init(&filt));
		THROW(prc.Run());
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_JOB_HDL_FACTORY(GATHERCLIENTACTIVITYSTAT);
