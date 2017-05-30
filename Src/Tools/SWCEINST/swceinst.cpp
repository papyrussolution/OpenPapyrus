// SWCEINST.CPP
//
//
#include <slib.h>
#include "rapi.h"
#include <stylopalm.h>

void LogMsg(SFile * pLogFile, const char * pBuf)
{
	printf(pBuf);
	if(pLogFile) {
		SString buf;
		LDATETIME dtm;
		getcurdatetime(&dtm);
		buf.Cat(dtm).Space().Cat(pBuf);
		pLogFile->WriteLine(buf);
	}
}

int CreateDirectory(const char * pDir)
{
	size_t wbuflen = 0;
	WCHAR * p_wbuf = 0;

	wbuflen = strlen(pDir)+ 1;
	p_wbuf = (WCHAR *)malloc(wbuflen * sizeof(WCHAR));
	p_wbuf[0] = 0;
	MultiByteToWideChar(CP_OEMCP, 0, pDir, (int)strlen(pDir), p_wbuf, (int)wbuflen);
	p_wbuf[wbuflen - 1] = 0;
	CeCreateDirectory(p_wbuf, 0);
	free(p_wbuf);
	return 1;
}

int CopyFileToPDA(const char * pSrcFile, const char * pDestFile, SFile * pLogFile, int copyIfNotExists)
{
	int ok = 1;
	char buf[16384];
	DWORD buf_size = sizeof(buf), read_bytes = 0, write_bytes = 0;
	SString msg_buf;
	HANDLE h_src = INVALID_HANDLE_VALUE, h_dest = INVALID_HANDLE_VALUE;

	memzero(buf, buf_size);
	h_src = CreateFile(pSrcFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(h_src == INVALID_HANDLE_VALUE) {
		LogMsg(pLogFile, msg_buf.Printf("Не удалось открыть файл «%s».", pSrcFile).CR().ToOem());
		ok = 0;
	}
	if(ok == 1) {
		size_t wbuflen = 0;
		SString dest_dir;
		SPathStruc sp;
		WCHAR * p_wbuf = 0;

		sp.Split(pDestFile);
		sp.Nam = 0;
		sp.Ext = 0;
		sp.Merge(dest_dir);

		wbuflen = dest_dir.Len() + 1;
		p_wbuf = (WCHAR *)malloc(wbuflen * sizeof(WCHAR));
		p_wbuf[0] = 0;
		MultiByteToWideChar(CP_OEMCP, 0, dest_dir, (int)dest_dir.Len(), p_wbuf, (int)wbuflen);
		p_wbuf[wbuflen - 1] = 0;
		CeCreateDirectory(p_wbuf, 0);
		free(p_wbuf);

		wbuflen = strlen(pDestFile) + 1;
		p_wbuf = (WCHAR *)malloc(wbuflen * sizeof(WCHAR));
		p_wbuf[0] = 0;
		MultiByteToWideChar(CP_OEMCP, 0, pDestFile, (int)strlen(pDestFile), p_wbuf, (int)wbuflen);
		p_wbuf[wbuflen - 1] = 0;
		if(copyIfNotExists) {
			CE_FIND_DATA find_data;
			memzero(&find_data, sizeof(find_data));
			HANDLE h = CeFindFirstFile(p_wbuf, &find_data);

			if(h != INVALID_HANDLE_VALUE)
				CeFindClose(h);
			if(find_data.nFileSizeLow != 0) {
				LogMsg(pLogFile, msg_buf.Printf("Файл %s уже существует на КПК.", pDestFile).CR().ToOem());
				ok = -1;
			}
		}
		if(ok > 0) {
			h_dest = CeCreateFile(p_wbuf, GENERIC_READ|GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			if(h_dest == INVALID_HANDLE_VALUE) {
				LogMsg(pLogFile, msg_buf.Printf("Не удалось создать файл «%s» на КПК.", pDestFile).CR().ToOem());
				ok = 0;
			}
			while(ok > 0 && ReadFile(h_src, buf, buf_size, &read_bytes, 0) && read_bytes > 0) {
				if(!CeWriteFile(h_dest, buf, read_bytes, &write_bytes, 0)) {
					LogMsg(pLogFile, msg_buf.Printf("Ошибка при копировании файла %s на КПК %s", pSrcFile, pDestFile).CR().ToOem());
					ok = 0;
				}
			}
		}
		free(p_wbuf);
	}
	if(h_src != INVALID_HANDLE_VALUE)
		CloseHandle(h_src);
	if(h_dest != INVALID_HANDLE_VALUE)
		CeCloseHandle(h_dest);
	if(ok > 0)
		LogMsg(pLogFile, msg_buf.Printf("Копирование файла %s на КПК %s прошло успешно", pSrcFile, pDestFile).CR().ToOem());
	return ok;
}

int CreateShortcut(const char * pShortcut, const char * pPath, SFile * pLog)
{
	int    ok = 1;
	size_t wbuflen = 0;
	SString msg_buf;
	WCHAR * p_wshortcut = 0, * p_wpath = 0;

	wbuflen = strlen(pShortcut)+ 1;
	p_wshortcut = (WCHAR *)malloc(wbuflen * sizeof(WCHAR));
	p_wshortcut[0] = 0;
	MultiByteToWideChar(CP_OEMCP, 0, pShortcut, (int)strlen(pShortcut), p_wshortcut, (int)wbuflen);
	p_wshortcut[wbuflen - 1] = 0;

	wbuflen = strlen(pPath)+ 1;
	p_wpath = (WCHAR *)malloc(wbuflen * sizeof(WCHAR));
	p_wpath[0] = 0;
	MultiByteToWideChar(CP_OEMCP, 0, pPath, (int)strlen(pPath), p_wpath, (int)wbuflen);
	p_wpath[wbuflen - 1] = 0;

	if(CeSHCreateShortcut(p_wshortcut,  p_wpath) == FALSE)
		LogMsg(pLog, msg_buf.Printf("Ошибка создания ярлыка %s", pShortcut).CR().ToOem());
	free(p_wshortcut);
	free(p_wpath);
	return ok;
}

struct ConnectionConfig {
	char  IP[4];
	char  Mask[4];
	long  Port;
	long  Timeout;
	char  UserName[48];
	char  Password[64];
};

struct AdvConfig {
	ConnectionConfig Conn;
	ConnectionConfig LocalConn;
	char DeviceName[16];
};

void main(int argc, char ** argv)
{
	const char * p_ini_sect        = "config";
	const char * p_ini_param       = "default_mgr";
	const char * p_reg_def_mgr     = "MUmgmt";
	const char * p_reg_new_mgr     = "SwceInst";
	const char * p_reg_guest       = "GuestOnly";
	const char * p_reg_autostart   = "SOFTWARE\\Microsoft\\Windows CE Services\\AutoStartOnConnect";
	const char * p_reg_wincesrvs   = "SOFTWARE\\Microsoft\\Windows CE Services\\";
	const char * p_wcecfg_file     = "swce.cfg";
	const char * p_wceadvcfg_file  = "swceadd.cfg";
	const char * p_stylowce_file   = "StyloWce.exe";
	const char * p_todayitem_file  = "TodayItem.dll";
	const char * p_stylobhtii_file = "StyloBhtII.exe";
	const char * p_device_path     = "\\Program Files\\Petroglif\\StyloWce";
	const char * p_petroglif_dir   = "\\Program Files\\Petroglif";
	const char * p_bht_path        = "\\Program Files\\Petroglif\\StyloBhtII";
	int ok = 0, conn_with_device = 1;
	SString buf, path, fname;
	SFile log_file;
	SPathStruc sp;

	sp.Split(argv[0]);
	sp.Nam = 0;
	sp.Ext = 0;
	sp.Merge(path);

	(fname = path).SetLastSlash().Cat("log.txt");
	log_file.Open(fname, SFile::mAppend);
	log_file.WriteLine((buf = "Start").CR());
	if(argc > 1) {
		SString cmdl_buf;
		cmdl_buf = argv[1];
		(fname = path).SetLastSlash().Cat("cfg.ini");
		if(cmdl_buf.CmpNC("install") == 0 || cmdl_buf.CmpNC("-install") == 0 || cmdl_buf.CmpNC("/install") == 0) {
			SIniFile ini_file(fname, (fileExists(fname)) ? 0 : 1, 0);
			if(ini_file.IsValid()) {
				char key_buf[MAXPATH];
				WinRegKey reg_key(HKEY_LOCAL_MACHINE, p_reg_autostart, 0);

				memzero(key_buf, sizeof(key_buf));
				reg_key.PutString(p_reg_new_mgr, argv[0]);
				reg_key.Close();
				reg_key.Open(HKEY_LOCAL_MACHINE, p_reg_autostart, 1);
				reg_key.GetString(p_reg_def_mgr, key_buf, sizeof(key_buf));
				reg_key.Close();
				if(strlen(key_buf) > 0)
					ini_file.AppendParam(p_ini_sect, p_ini_param, key_buf, 1);
				reg_key.DeleteValue(HKEY_LOCAL_MACHINE, p_reg_autostart, p_reg_def_mgr);
				reg_key.Open(HKEY_LOCAL_MACHINE, p_reg_wincesrvs, 0);
				reg_key.PutDWord(p_reg_guest, 1);
				conn_with_device = 0;
			}
			else
				LogMsg(&log_file, buf.Printf("Файл не найден %s", (const char*)fname).CR().ToOem());
		}
		else if(cmdl_buf.CmpNC("restore") == 0 || cmdl_buf.CmpNC("-restore") == 0 || cmdl_buf.CmpNC("/restore") == 0) {
			SIniFile ini_file(fname);
			if(ini_file.IsValid()) {
				WinRegKey reg_key(HKEY_LOCAL_MACHINE, p_reg_autostart, 0);
				ini_file.GetParam(p_ini_sect, p_ini_param, (fname = 0));
				reg_key.PutString(p_reg_def_mgr, fname);
				reg_key.Close();
				reg_key.DeleteValue(HKEY_LOCAL_MACHINE, p_reg_autostart, p_reg_new_mgr);
				reg_key.DeleteValue(HKEY_LOCAL_MACHINE, p_reg_wincesrvs, p_reg_guest);
				conn_with_device = 0;
			}
			else
				LogMsg(&log_file, buf.Printf("Файл не найден %s", (const char*)fname).CR().ToOem());
		}
		else if(cmdl_buf.CmpNC("help") == 0 || cmdl_buf.CmpNC("-help") == 0 || cmdl_buf.CmpNC("/help") == 0 || cmdl_buf.CmpNC("/?") == 0 || cmdl_buf.CmpNC("-?") == 0) {
			printf((buf = "swceinst.exe [/install][/restore][/help]").CR().ToOem());
			printf((buf = "/install - При подсоединении устройства обмен будет производится с программой swceinst").CR().ToOem());
			printf((buf = "/restore - Восстанавливает настройки ActiveSync по умолчанию").CR().ToOem());
			printf((buf = "/help - Помощь").CR().ToOem());
			conn_with_device = 0;
		}
	}
	if(conn_with_device == 1) {
		RAPIINIT ri;
		HRESULT h;
		DWORD timeout = 0;

		MEMSZERO(ri);
		ri.cbSize = sizeof(ri);
		h = CeRapiInitEx(&ri);
		if(FAILED(h)) {
			printf((buf = "Не удалось инициализировать систему связи с КПК").ToOem());
			log_file.WriteLine(buf.CR());
		}
		timeout = GetTickCount() + 30000;
		while(GetTickCount() < timeout) {
			DWORD ret = MsgWaitForMultipleObjects(1, &ri.heRapiInit, FALSE, timeout - GetTickCount(), QS_ALLINPUT);
			if(ret == WAIT_OBJECT_0) {
				if(SUCCEEDED(ri.hrRapiInit))
					ok = 1;
				else
					LogMsg(&log_file, buf.Printf("Не удалось установить соединение с КПК.", (const char*)fname).CR().ToOem());
				break;
			}
		}
		if(ok == 0)
			LogMsg(&log_file, buf.Printf("Таймаут связи с КПК.", (const char*)fname).CR().ToOem());
		else {
			CEOSVERSIONINFO os_info;
			SString dev_fname;

			memzero(&os_info, sizeof(os_info));
			os_info.dwOSVersionInfoSize = sizeof(CEOSVERSIONINFO);
			CeGetVersionEx(&os_info);
			CreateDirectory(p_petroglif_dir);
			//
			// Установка софта на КПК (StyloWce.exe и файлы конфигурации)
			//
			if(os_info.dwMajorVersion >= 5) {
				CopyFileToPDA((fname = path).SetLastSlash().Cat(p_stylowce_file), (dev_fname = p_device_path).SetLastSlash().Cat(p_stylowce_file), &log_file, 0);
				if(CopyFileToPDA((fname = path).SetLastSlash().Cat(p_todayitem_file), (dev_fname = p_device_path).SetLastSlash().Cat(p_todayitem_file), &log_file, 0) <= 0) {
					sp.Split(dev_fname);
					sp.Ext = "new";
					sp.Merge(dev_fname = 0);
					CopyFileToPDA((fname = path).SetLastSlash().Cat(p_todayitem_file), dev_fname, &log_file, 0);
				}
				{
					PalmConfig dev_cfg;
					(fname = path).SetLastSlash().Cat(p_wcecfg_file);
					dev_cfg.Read(fname);
					dev_cfg.TmLastXchg.SetZero();
					dev_cfg.TmClient.SetZero();
					dev_cfg.TmGoods.SetZero();
					dev_cfg.TmCliDebt.SetZero();
					dev_cfg.TmCliSell.SetZero();
					dev_cfg.TmToDo.SetZero();
					dev_cfg.InvCode        = 0;
					dev_cfg.OrdCode        = 0;
					dev_cfg.NumSellWeeks   = 0;
					dev_cfg.CardNo         = 0;
					dev_cfg.ActiveNetCfgID = 0;
					dev_cfg.Write(fname);
					(fname = path).SetLastSlash().Cat(p_wcecfg_file);
					CopyFileToPDA(fname, (dev_fname = p_device_path).SetLastSlash().Cat(p_wcecfg_file), &log_file, 1);
				}
				{
					SFile file;
					if(file.Open((fname = path).SetLastSlash().Cat(p_wceadvcfg_file), SFile::mReadWrite|SFile::mBinary) > 0) {
						AdvConfig adv_cfg;
						file.Read(&adv_cfg, sizeof(adv_cfg));
						memzero(adv_cfg.DeviceName, sizeof(adv_cfg.DeviceName));
						file.Seek(0, SEEK_SET);
						file.Write(&adv_cfg, sizeof(adv_cfg));
						file.Close();
					}
					CopyFileToPDA(fname, (dev_fname = p_device_path).SetLastSlash().Cat(p_wceadvcfg_file), &log_file, 1);
				}
			}
			//
			// Установка софта на терминал FALCON 4220
			//
			else {
				CopyFileToPDA((fname = path).SetLastSlash().Cat(p_stylobhtii_file), (dev_fname = p_bht_path).SetLastSlash().Cat(p_stylobhtii_file), &log_file, 0);
				// CreateShortcut("StyloBhtII", fname, &log_file);
			}
		}
	}
	CeRapiUninit();
	log_file.WriteLine((buf = "End").CR().CR());
}
