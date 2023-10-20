// SLTESTAPP.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
// Тестовое приложение для отработки функций запуска и управления системными процессами
//
#include <pp.h>
#include <wsctl.h>

int main(int argc, char * argv[], char * envp[])
{
	int    result = 0;
	SString temp_buf;
	SStringU temp_buf_u;
	SString out_buf;
	SString policypath;
	SString report_file_name;
	WsCtl_ClientPolicy policy;
	PPGetFilePath(PPPATH_BIN, "sltestapp-report.txt", report_file_name);
	SFile f_rep(report_file_name, SFile::mWrite);
	(out_buf = "SlTestApp: тестовое приложение").Transf(CTRANSF_UTF8_TO_INNER).CR();
	slfprintf_stderr(out_buf);
	if(argc == 1) {
		slfprintf_stderr("There aren't cmdline args\n");
	}
	else {
		out_buf.Z().Cat("Cmdline args").Space().CatParStr(argc).Colon().CR();
		slfprintf_stderr(out_buf);
		for(int i = 1; i < argc; i++) {
			//temp_buf.CopyUtf8FromUnicode(argv[i], sstrlen(argv[i]), 0);
			temp_buf = argv[i];
			temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			out_buf.Z().Tab().Cat(temp_buf).CR();
			slfprintf_stderr(out_buf);
			if(temp_buf.IsEqiAscii("policypath") && (i+1) < argc) {
				policypath = argv[++i];
			}
		}
	}
	{
		//wchar_t user_name[128];
		//DWORD user_name_buf_len = SIZEOFARRAY(user_name);
		if(SSystem::GetUserName_(temp_buf)) {
		//if(::GetUserNameW(user_name, &user_name_buf_len)) {
			//temp_buf.CopyUtf8FromUnicode(user_name, sstrlen(user_name), 0);
			temp_buf.Transf(CTRANSF_UTF8_TO_INNER);			
			out_buf.Z().Cat("Current User").CatDiv(':', 2).Cat(temp_buf).CR();
			slfprintf_stderr(out_buf);
		}
	}
	{
		wchar_t curdir_u[1024];
		::GetCurrentDirectoryW(sizeof(curdir_u), curdir_u);
		temp_buf.CopyUtf8FromUnicode(curdir_u, sstrlen(curdir_u), 0);
		temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		out_buf.Z().Cat("Current Directory").CatDiv(':', 2).Cat(temp_buf).CR();
		slfprintf_stderr(out_buf);
	}
	if(!f_rep.IsValid()) {
		out_buf.Z().Cat("Report file").Space().CatParStr(report_file_name).Space().Cat("opening fault").CR();
		slfprintf_stderr(out_buf);
	}
	if(policypath.IsEmpty()) {
		policypath = "\\Papyrus\\Src\\PPTEST\\DATA";
	}
	if(policypath.NotEmpty()) {
		out_buf.Z().Cat("policy path").CatDiv(':', 2).Cat(policypath).CR();
		slfprintf_stderr(out_buf);
		//
		//winpolicy.json 
		temp_buf.Z().Cat(policypath).SetLastSlash().Cat(/*"winpolicy.json"*/"test-file-to-write");
		SFile f_policy(temp_buf, SFile::mWrite);
		if(f_policy.IsValid()) {
			out_buf.Z().Cat("File").Space().Cat(temp_buf).Space().Cat("opened successfully");
		}
		else {
			out_buf.Z().Cat("File").Space().Cat(temp_buf).Space().Cat("opening fault");
		}
		slfprintf_stderr(out_buf.CR());
	}
	{
		uint8 random_buffer[32];
		SLS.GetTLA().Rg.ObfuscateBuffer(random_buffer, sizeof(random_buffer));
		SString random_string;
		random_string.EncodeMime64(random_buffer, sizeof(random_buffer));
		{
			WinRegKey test_key(HKEY_CURRENT_USER, PPConst::WrKey_SlTestApp, 0);
			temp_buf.Z().Cat("CURRENT_USER").SetLastSlash().Cat(PPConst::WrKey_SlTestApp);
			if(test_key.IsValid()) {
				slfprintf_stderr((out_buf = ":) Test register key").Space().CatParStr(temp_buf).Space().Cat("is opened for writing successfully").CR());
				if(test_key.PutString("test-string", random_string)) {
					slfprintf_stderr((out_buf = ":) Writing to the test register key").Space().CatParStr(temp_buf).Space().Cat("is successful").CR());
				}
				else {
					slfprintf_stderr((out_buf = ":( Writing to the test register key").Space().CatParStr(temp_buf).Space().Cat("is fault").CR());
				}
			}
			else {
				slfprintf_stderr((out_buf = ":( Test register key").Space().CatParStr(temp_buf).Space().Cat("isn't opened for writing").CR());
			}
		}
		{
			WinRegKey test_key(HKEY_LOCAL_MACHINE, PPConst::WrKey_SlTestApp, 0);
			temp_buf.Z().Cat("MACHINE").SetLastSlash().Cat(PPConst::WrKey_SlTestApp);
			if(test_key.IsValid()) {
				slfprintf_stderr((out_buf = ":) Test register key").Space().CatParStr(temp_buf).Space().Cat("is opened for writing successfully").CR());
				if(test_key.PutString("test-string", random_string)) {
					slfprintf_stderr((out_buf = ":) Writing to the test register key").Space().CatParStr(temp_buf).Space().Cat("is successful").CR());
				}
				else {
					slfprintf_stderr((out_buf = ":( Writing to the test register key").Space().CatParStr(temp_buf).Space().Cat("is fault").CR());
				}
			}
			else {
				slfprintf_stderr((out_buf = ":( Test register key").Space().CatParStr(temp_buf).Space().Cat("isn't opened for writing").CR());
			}
		}
	}
	{
		//GetEnvironmentStrings()
		if(f_rep.IsValid()) {
			{
				f_rep.WriteLine((temp_buf = "Environment").CR());
				if(envp) {
					for(uint envidx = 0; envp[envidx]; envidx++) {
						temp_buf.Z().Tab().Cat(envp[envidx]).CR();
						f_rep.WriteLine(temp_buf);
					}
				}
			}
			{
				f_rep.WriteLine((temp_buf = "Known folders").CR());
				TSCollection <SKnownFolderEntry> known_folder_list;
				GetKnownFolderList(known_folder_list);
				for(uint i = 0; i < known_folder_list.getCount(); i++) {
					const SKnownFolderEntry * p_entry = known_folder_list.at(i);
					if(p_entry) {
						temp_buf.Z().CatHex(p_entry->UED).Space().Cat(p_entry->Guid, S_GUID::fmtIDL).Space().Cat(p_entry->Result).Space().Cat(p_entry->PathUtf8).CR();
						f_rep.WriteLine(temp_buf);
					}
				}
			}
		}
	}
	slfprintf_stderr("Press [Enter] to finish...\n");
	getchar();
	return result;
}
