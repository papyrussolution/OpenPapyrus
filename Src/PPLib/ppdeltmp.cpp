// PPDELTMP.CPP
// Copyright (c) A.Osolotkin, A.Sobolev 2001-2003, 2005, 2007, 2013, 2015, 2016, 2017, 2018
// Удаление временных файлов
//
#include <pp.h>
#pragma hdrstop

SLAPI DeleteTmpFilesParam::DeleteTmpFilesParam() : Flags(0), InDays(0), OutDays(0)
{
}

static void FASTCALL RemoveTempDataFiles(PPID pathID)
{
	SString src_path, src_file_name;
	PPGetPath(pathID, src_path);
	src_path.SetLastSlash();
	(src_file_name = src_path).Cat("tmp?????.btr");
	SDirEntry sde;
	for(SDirec sd(src_file_name); sd.Next(&sde) > 0;) {
		SFile::Remove((src_file_name = src_path).Cat(sde.FileName));
	}
}

static void SLAPI _RemoveTempFiles(PPID pathID, const char * pExt, const char * pFilePart/* = 0*/)
{
	SString src_path, src_file_name;
	if(PPGetPath(pathID, src_path) > 0) {
		src_path.SetLastSlash();
		src_file_name = src_path;
		if(pFilePart)
			src_file_name.Cat(pFilePart);
		src_file_name.Cat("*.").Cat(pExt);
		SDirEntry sde;
		for(SDirec sd(src_file_name); sd.Next(&sde) > 0;) {
			SFile::Remove((src_file_name = src_path).Cat(sde.FileName));
		}
	}
}

static void FASTCALL RemoveInOutFiles(long diff_dt, PPID pathID)
{
	SString src_path, src_file_name;
	LDATE  cur_dt = getcurdate_();
	PPGetPath(pathID, src_path);
	src_path.SetLastSlash();
	(src_file_name = src_path).Cat("*.pps");
	SDirEntry sde;
	for(SDirec sd(src_file_name); sd.Next(&sde) > 0;)
		if(diffdate(cur_dt, sde.WriteTime.d) >= diff_dt)
			SFile::Remove((src_file_name = src_path).Cat(sde.FileName));
}

int SLAPI PPDeleteTmpFiles(DeleteTmpFilesParam * pDelParam)
{
	int    ok = 1;
	SString _temp_path;
	SString tmp_dir, tmp_sub_dir;
	SString tmp_path, tmp_file;
	SDirEntry sde;
	PPGetPath(PPPATH_TEMP, _temp_path);
	if(_temp_path.NotEmptyS())
		_temp_path.SetLastSlash();
	if(pDelParam->Flags & DeleteTmpFilesParam::fRmvTempData) {
		RemoveTempDataFiles(PPPATH_DAT);
		RemoveTempDataFiles(PPPATH_TEMP);
	}
	if(pDelParam->Flags & DeleteTmpFilesParam::fRmvTempPrns) {
		if(_temp_path.NotEmpty()) {
			tmp_dir = _temp_path;
			(tmp_sub_dir = tmp_dir).Cat("*.*");
			for(SDirec sd(tmp_sub_dir, 1); sd.Next(&sde) > 0;) {
				if(sde.IsFolder() && !sde.IsSelf() && !sde.IsUpFolder()) {
					int    need_remove = 1;
					for(const char * p = sde.FileName; *p; p++)
						if(!isdec(*p)) {
							need_remove = 0;
							break;
						}
					if(need_remove) {
						::RemoveDir((tmp_path = tmp_dir).Cat(sde.FileName).SetLastSlash());
					}
				}
			}
		}
	}
	if(pDelParam->Flags & DeleteTmpFilesParam::fRmvTempCharry) {
		_RemoveTempFiles(PPPATH_IN,  "CHY", 0);
		_RemoveTempFiles(PPPATH_OUT, "CHY", 0);
	}
	if(pDelParam->Flags & DeleteTmpFilesParam::fRmvTempEmail) {
		_RemoveTempFiles(PPPATH_IN,   "PML", 0);
		_RemoveTempFiles(PPPATH_OUT,  "PML", 0);
		_RemoveTempFiles(PPPATH_TEMP, "MSG", 0);
	}
	// @v9.9.9 {
	if(pDelParam->Flags & DeleteTmpFilesParam::fRmvTempQrCodes) {
		_RemoveTempFiles(PPPATH_TEMP, "png", "fccqr");
	}
	// } @v9.9.9 
	if(pDelParam->Flags & DeleteTmpFilesParam::fRmvBHTDataFiles) {
		_RemoveTempFiles(PPPATH_IN,  "DAT", "BHT");
		_RemoveTempFiles(PPPATH_IN,  "DBF", "BHT");
	}
	if(pDelParam->Flags & DeleteTmpFilesParam::fRmvInTransm)
		RemoveInOutFiles(pDelParam->InDays, PPPATH_IN);
	if(pDelParam->Flags & DeleteTmpFilesParam::fRmvOutTransm)
		RemoveInOutFiles(pDelParam->OutDays, PPPATH_OUT);
	{
		//
		// Безусловное удаление некоторых файлов, которые существуют более 3-x суток
		//
		StringSet etc_to_rmv_files;
		const LDATETIME ct = getcurdatetime_();
		if(_temp_path.NotEmpty()) {
			{
				//
				// Временные файлы изображений
				//
				(tmp_sub_dir = _temp_path).Cat("oimg????.*");
				for(SDirec sd(tmp_sub_dir, 0); sd.Next(&sde) > 0;) {
					if(sde.IsFile() && diffdatetimesec(ct, sde.WriteTime) > (3600*24*3))
						etc_to_rmv_files.add((tmp_path = _temp_path).Cat(sde.FileName));
				}
			}
			{
				//
				// Временные файлы post-изображений (подкаталог img)
				//
				(tmp_dir = _temp_path).Cat("img").SetLastSlash();
				(tmp_sub_dir = tmp_dir).Cat("pst?????.*");
				for(SDirec sd(tmp_sub_dir, 0); sd.Next(&sde) > 0;) {
					if(sde.IsFile() && diffdatetimesec(ct, sde.WriteTime) > (3600*24*3))
						etc_to_rmv_files.add((tmp_path = tmp_dir).Cat(sde.FileName));
				}
			}
			{
				//
				// Временные xml-файлы
				//
				(tmp_sub_dir = _temp_path).Cat("*.xml");
				for(SDirec sd(tmp_sub_dir, 0); sd.Next(&sde) > 0;) {
					if(sde.IsFile() && diffdatetimesec(ct, sde.WriteTime) > (3600*24*3))
						etc_to_rmv_files.add((tmp_path = _temp_path).Cat(sde.FileName));
				}
			}
			{
				//
				// Временные файлы рабочих книг
				//
				(tmp_sub_dir = _temp_path).Cat("wb*.*");
				for(SDirec sd(tmp_sub_dir, 0); sd.Next(&sde) > 0;) {
					if(sde.IsFile() && diffdatetimesec(ct, sde.WriteTime) > (3600*24*3))
						etc_to_rmv_files.add((tmp_path = _temp_path).Cat(sde.FileName));
				}
			}
			for(uint sp = 0; etc_to_rmv_files.get(&sp, tmp_path);) {
				SFile::Remove(tmp_path);
			}
		}
	}
	PPMsgLog::RemoveTempFiles();
	//CATCHZOK
	return ok;
}

int SLAPI DeleteTmpFilesDlg(DeleteTmpFilesParam * pParam)
{
	class DeleteTmpFilesDialog : public TDialog {
	public:
		DeleteTmpFilesDialog() : TDialog(DLG_DELTMP)
		{
		}
		int    setDTS(const DeleteTmpFilesParam * pData)
		{
			ushort v = 0;
			Data = *pData;
			AddClusterAssoc(CTL_DELTMP_FILES, 0, DeleteTmpFilesParam::fRmvTempData);
			AddClusterAssoc(CTL_DELTMP_FILES, 1, DeleteTmpFilesParam::fRmvTempPrns);
			AddClusterAssoc(CTL_DELTMP_FILES, 2, DeleteTmpFilesParam::fRmvTempCharry);
			AddClusterAssoc(CTL_DELTMP_FILES, 3, DeleteTmpFilesParam::fRmvTempEmail);
			AddClusterAssoc(CTL_DELTMP_FILES, 4, DeleteTmpFilesParam::fRmvBHTDataFiles);
			AddClusterAssoc(CTL_DELTMP_FILES, 5, DeleteTmpFilesParam::fRmvTempQrCodes); // @v9.9.9
			SetClusterData(CTL_DELTMP_FILES, Data.Flags);

			v = BIN(Data.Flags & DeleteTmpFilesParam::fRmvInTransm);
			setCtrlData(CTL_DELTMP_INCHECK, &v);
			setCtrlData(CTL_DELTMP_INDAYS,  &Data.InDays);
			disableCtrl(CTL_DELTMP_INDAYS, !v);
			v = BIN(Data.Flags & DeleteTmpFilesParam::fRmvOutTransm);
			setCtrlData(CTL_DELTMP_OUTCHECK, &v);
			setCtrlData(CTL_DELTMP_OUTDAYS, &Data.OutDays);
			disableCtrl(CTL_DELTMP_OUTDAYS, !v);
			return 1;
		}
		int    getDTS(DeleteTmpFilesParam * pData)
		{
			ushort v = 0;
			int    r = 1;
			Data.Flags = 0;
			GetClusterData(CTL_DELTMP_FILES, &Data.Flags);
			getCtrlData(CTL_DELTMP_INCHECK, &v);
			SETFLAG(Data.Flags, DeleteTmpFilesParam::fRmvInTransm, v);
			if(v) {
				getCtrlData(CTL_DELTMP_INDAYS, &Data.InDays);
				if(Data.InDays < 0)
					r = PPSetError(PPERR_USERINPUT);
			}
			else
				Data.InDays = 0;
			getCtrlData(CTL_DELTMP_OUTCHECK, &v);
			SETFLAG(Data.Flags, DeleteTmpFilesParam::fRmvOutTransm, v);
			if(v) {
				getCtrlData(CTL_DELTMP_OUTDAYS, &Data.OutDays);
				if(Data.OutDays < 0)
					r = PPSetError(PPERR_USERINPUT);
			}
			else
				Data.OutDays = 0;
			ASSIGN_PTR(pData, Data);
			return r;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(TVCOMMAND && TVCMD == cmClusterClk) {
				disableCtrl(CTL_DELTMP_INDAYS,  !getCtrlUInt16(CTL_DELTMP_INCHECK));
				disableCtrl(CTL_DELTMP_OUTDAYS, !getCtrlUInt16(CTL_DELTMP_OUTCHECK));
				clearEvent(event);
			}
		}
		DeleteTmpFilesParam Data;
	};
	DIALOG_PROC_BODYERR(DeleteTmpFilesDialog, pParam);
}

int SLAPI DeleteTmpFiles()
{
	int    ok = -1;
	DeleteTmpFilesParam param;
	param.Flags |= (DeleteTmpFilesParam::fRmvTempData |
		DeleteTmpFilesParam::fRmvTempPrns | DeleteTmpFilesParam::fRmvTempCharry |
		DeleteTmpFilesParam::fRmvInTransm | DeleteTmpFilesParam::fRmvOutTransm |
		DeleteTmpFilesParam::fRmvTempEmail | DeleteTmpFilesParam::fRmvBHTDataFiles);
	if(DeleteTmpFilesDlg(&param) > 0) {
		PPWait(1);
		if(!PPDeleteTmpFiles(&param))
			PPError();
		else
			ok = 1;
		PPWait(0);
	}
	return ok;
}
