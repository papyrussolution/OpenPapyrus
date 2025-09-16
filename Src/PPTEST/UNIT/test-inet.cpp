// TEST-INET.CPP
// Copyright (c) A.Sobolev 2023, 2024, 2025
//
#include <pp.h>
#pragma hdrstop

int SMailMessage::DebugOutput(SString & rBuf) const
{
	SString temp_buf;
	if(GetS(HFP.FromP, temp_buf))
		rBuf.CatEq("from", temp_buf).CR();
	if(GetS(HFP.ToP, temp_buf))
		rBuf.CatEq("to", temp_buf).CR();
	if(GetS(HFP.CcP, temp_buf))
		rBuf.CatEq("cc", temp_buf).CR();
	if(GetS(HFP.SubjP, temp_buf))
		rBuf.CatEq("subj", temp_buf).CR();
	if(GetS(HFP.MailerP, temp_buf))
		rBuf.CatEq("mailer", temp_buf).CR();
	if(GetS(HFP.MsgIdP, temp_buf))
		rBuf.CatEq("msgid", temp_buf).CR();
	if(GetS(HFP.UserAgentP, temp_buf))
		rBuf.CatEq("useragent", temp_buf).CR();
	if(GetS(HFP.OrganizationP, temp_buf))
		rBuf.CatEq("organization", temp_buf).CR();
	if(GetS(HFP.ReturnPathP, temp_buf))
		rBuf.CatEq("returnpath", temp_buf).CR();
	if(GetS(HFP.DeliveredToP, temp_buf))
		rBuf.CatEq("deliveredto", temp_buf).CR();
	if(GetS(HFP.ReplyToP, temp_buf))
		rBuf.CatEq("replyto", temp_buf).CR();
	if(GetS(HFP.ContentLangP, temp_buf))
		rBuf.CatEq("contentlang", temp_buf).CR();
	if(GetS(HFP.AcceptLangP, temp_buf))
		rBuf.CatEq("acceptlang", temp_buf).CR();
	if(GetS(HFP.XOrgIpP, temp_buf))
		rBuf.CatEq("xorgip", temp_buf).CR();
	temp_buf.Z().Cat(HFP.Dtm, DATF_ISO8601CENT, 0);
	rBuf.CatEq("date", temp_buf).CR();
	HFP.MimeVer.ToStr(temp_buf.Z());
	rBuf.CatEq("mimever", temp_buf).CR();
	if(ReceivedChainL.getCount()) {
		for(uint i = 0; i < ReceivedChainL.getCount(); i++) {
			GetS(ReceivedChainL.at(i), temp_buf);
			rBuf.CatEq("received", temp_buf).CR();
		}
	}
	DebugOutput_Boundary(B, 1, rBuf);
	return 1;
}

int SMailMessage::DebugOutput_Boundary(const Boundary & rB, uint tab, SString & rBuf) const
{
	SString temp_buf;
	temp_buf.Z().Cat(rB.LineNo_Start).Dot().Dot().Cat(rB.LineNo_Finish);
	rBuf.Tab_(tab).Cat("boundary").Space().CatEq("lines", temp_buf).CR();
	GetS(rB.Ct.MimeP, temp_buf);
	rBuf.Tab_(tab+1).CatEq("ct:mime", temp_buf).CR();
	GetS(rB.Ct.TypeP, temp_buf);
	rBuf.Tab_(tab+1).CatEq("ct:type", temp_buf).CR();
	GetS(rB.Ct.NameP, temp_buf);
	rBuf.Tab_(tab+1).CatEq("ct:name", temp_buf).CR();
	GetS(rB.Ct.BoundaryP, temp_buf);
	rBuf.Tab_(tab+1).CatEq("ct:boundaryid", temp_buf).CR();
	rB.Ct.Cp.ToStr(SCodepageIdent::fmtDefault, temp_buf);
	rBuf.Tab_(tab+1).CatEq("ct:cp", temp_buf).CR();
	//
	rBuf.Tab_(tab+1).CatEq("cd:type", (long)rB.Cd.Type).CR();
	GetS(rB.Cd.NameP, temp_buf);
	rBuf.Tab_(tab+1).CatEq("cd:name", temp_buf).CR();
	GetS(rB.Cd.FileNameP, temp_buf);
	rBuf.Tab_(tab+1).CatEq("cd:filename", temp_buf).CR();
	if(rB.Cd.Size != 0)
		rBuf.Tab_(tab+1).CatEq("cd:size", rB.Cd.Size).CR();
	if(!!rB.Cd.CrDtm) {
		temp_buf.Z().Cat(rB.Cd.CrDtm, DATF_ISO8601CENT, 0);
		rBuf.Tab_(tab+1).CatEq("cd:crdtm", rB.Cd.Size).CR();
	}
	if(!!rB.Cd.ModifDtm) {
		temp_buf.Z().Cat(rB.Cd.ModifDtm, DATF_ISO8601CENT, 0);
		rBuf.Tab_(tab+1).CatEq("cd:modifdtm", rB.Cd.Size).CR();
	}
	if(!!rB.Cd.RdDtm) {
		temp_buf.Z().Cat(rB.Cd.RdDtm, DATF_ISO8601CENT, 0);
		rBuf.Tab_(tab+1).CatEq("cd:readdtm", rB.Cd.Size).CR();
	}
	rBuf.Tab_(tab+1).CatEq("contenttypeenc", (long)rB.ContentTransfEnc).CR();
	rBuf.Tab_(tab+1).CatEq("datasize", rB.Data.GetAvailableSize()).CR();
	for(uint i = 0; i < rB.Children.getCount(); i++) {
		const Boundary * p_b = rB.Children.at(i);
		if(p_b) {
			DebugOutput_Boundary(*p_b, tab+1, rBuf); // @recursion
		}
		else {
			rBuf.Tab_(tab+2).Cat("zeroptr child boundary").CR();
		}
	}
	return 1;
}

static void Test_MailMsg_ReadFromFile()
{
	const char * src_file_name_list[] = {
		"test-01.eml",
		"test-02.eml",
		"test-03.eml",
		"test-04.eml",
		"test-05.eml"
	};

	SString temp_buf;
	const SString test_mailmsg_path("d:/papyrus/src/pptest/data/email/");
	for(uint i = 0; i < SIZEOFARRAY(src_file_name_list); i++) {
		(temp_buf = test_mailmsg_path).SetLastDSlash().Cat(src_file_name_list[i]);
		SFile f_in(temp_buf, SFile::mRead);
		if(f_in.IsValid()) {
			SMailMessage msg;
			msg.ReadFromFile(f_in);
			{
				SString out_file_name;
				SString out_buf;
				SFsPath ps(f_in.GetName());
				ps.Nam.Cat("-testoutput");
				ps.Ext = "out";
				ps.Merge(out_file_name);
				msg.DebugOutput(out_buf);
				SFile f_out(out_file_name, SFile::mWrite);
				f_out.WriteLine(out_buf);
			}
			{
				const uint attcount = msg.GetAttachmentCount();
				for(uint j = 0; j < attcount; j++) {
					(temp_buf = test_mailmsg_path).SetLastDSlash().Cat("attachment");
					msg.SaveAttachmentTo(j, temp_buf, 0);
				}
			}
		}
	}
}

static void Test_MakeEmailMessage(SMailMessage & rMsg)
{
	SString path;
	SString temp_buf;
	SBuffer data_buf;
	temp_buf.Z().Cat("������� �����").Space().CatChar('<').Cat("soobolev@yandex.ru").CatChar('>').Transf(CTRANSF_OUTER_TO_UTF8);
	rMsg.SetField(rMsg.fldFrom, temp_buf);
	temp_buf.Z().Cat("������� �����").Space().CatChar('<').Cat("sobolev@petroglif.ru").CatChar('>');
	temp_buf.Comma().Cat("������� �������").Space().CatChar('<').Cat("papyrussolution@gmail.com").CatChar('>');
	// @v10.4.5 temp_buf.Transf(CTRANSF_OUTER_TO_UTF8);
	rMsg.SetField(rMsg.fldTo, temp_buf);
	temp_buf.Z().Cat("�������� ������ ��� �������� ������������ ������������ ���������. ���!")/* @v10.4.5 .Transf(CTRANSF_OUTER_TO_UTF8)*/;
	rMsg.SetField(rMsg.fldSubj, temp_buf);
	{
		{
			SLS.QueryPath("testroot", path);
			path.SetLastSlash().Cat("data").SetLastSlash().Cat("rustext.txt");
			SFile f_in(path, SFile::mRead);
			if(f_in.IsValid()) {
				data_buf.Z();
				while(f_in.ReadLine(temp_buf)) {
					temp_buf.Transf(CTRANSF_OUTER_TO_UTF8);
					data_buf.Write(temp_buf, temp_buf.Len());
				}
				rMsg.AttachContent(0, SFileFormat::Txt, cpUTF8, data_buf.GetBuf(data_buf.GetRdOffs()), data_buf.GetAvailableSize());
			}
		}
		{
			SLS.QueryPath("testroot", path);
			path.SetLastSlash().Cat("data").SetLastSlash().Cat("test-gif.gif");
			rMsg.AttachFile(0, SFileFormat::Unkn, path);
		}
	}
}

static void Test_MakeEmailMessage()
{
	SMailMessage msg;
	Test_MakeEmailMessage(msg);
	{
		SString path;
		SBuffer data_buf;
		SLS.QueryPath("testroot", path);
		path.SetLastSlash().Cat("data/email").SetLastSlash().Cat("mkmsgtest.eml");
		SFile f_out(path, SFile::mWrite|SFile::mBinary);

		SMailMessage::WriterBlock wb(msg);
		data_buf.Z();
		while(wb.Read(2048, data_buf) > 0) {
			f_out.Write(data_buf.GetBuf(data_buf.GetRdOffs()), data_buf.GetAvailableSize());
			data_buf.Z();
		}
	}
}

SLTEST_R(ScURL_Mail_SMTP)
{
	int    ok = 1;
	SString temp_buf;
	SString url_text;
	InetUrl url;
	{
		uint   arg_no = 0;
		if(EnumArg(&arg_no, temp_buf)) {
			url.Parse(temp_buf);
			if(EnumArg(&arg_no, temp_buf)) {
				url.SetComponent(url.cUserName, temp_buf);
				if(EnumArg(&arg_no, temp_buf))
					url.SetComponent(url.cPassword, temp_buf);
			}
		}
	}
	{
		LAssocArray mail_list;
		ScURL curl;
		SString output_path;
		SMailMessage msg;
		Test_MakeEmailMessage(msg);
		THROW(SLCHECK_NZ(curl.SmtpSend(url, ScURL::mfDontVerifySslPeer, msg)));
	}
	CATCH
		CurrentStatus = 0;
		ok = 0;
	ENDCATCH
	return CurrentStatus;
}

SLTEST_R(ScURL_Mail)
{
	int    ok = 1;
	SString temp_buf;
	SString url_text;
	InetUrl url;
	{
		uint   arg_no = 0;
		if(EnumArg(&arg_no, temp_buf)) {
			url.Parse(temp_buf);
			if(EnumArg(&arg_no, temp_buf)) {
				url.SetComponent(url.cUserName, temp_buf);
				if(EnumArg(&arg_no, temp_buf))
					url.SetComponent(url.cPassword, temp_buf);
			}
		}
	}
	{
		LAssocArray mail_list;
		ScURL curl;
		SString output_path;
		{
			//uint msg_count = 0;
			//uint64 msg_size = 0;
			//THROW(SLCHECK_NZ(curl.Pop3Stat(url, ScURL::mfDontVerifySslPeer, &msg_count, &msg_size)));
			THROW(SLCHECK_NZ(curl.Pop3List(url, ScURL::mfDontVerifySslPeer, mail_list)));
		}
		{
			SUniformFileTransmParam uftp;
			uftp.DestPath = MakeOutputFilePath("");
			temp_buf.EncodeUrl("/*.zip", 0);
			url.SetComponent(InetUrl::cPath, temp_buf);
			temp_buf.EncodeUrl("from=it@tdadvent.ru", 0);
			url.SetComponent(InetUrl::cQuery, temp_buf);
			url.Compose(0, temp_buf);
			uftp.SrcPath = temp_buf;
			//uftp.Format = SFileFormat::Jpeg;
			THROW(SLCHECK_NZ(uftp.Run(0, 0)));
			temp_buf = uftp.Reply;
		}
		{
			for(uint i = 0; i < mail_list.getCount(); i++) {
				SMailMessage msg;
				if(curl.Pop3Top(url, ScURL::mfDontVerifySslPeer, mail_list.at(i).Key, 0, msg)) {
					output_path = MakeOutputFilePath(temp_buf.Z().Cat(i).DotCat("output"));
					msg.DebugOutput(temp_buf.Z());
					SFile f_output(output_path, SFile::mWrite);
					if(f_output.IsValid())
						f_output.WriteLine(temp_buf);
				}
			}
		}
		/*
		{
			for(uint i = 0; i < mail_list.getCount(); i++) {
				SMailMessage msg;
				//THROW(SLCHECK_NZ(curl.Pop3Get(url, ScURL::mfDontVerifySslPeer, mail_list.at(i).Key, msg)));
				if(curl.Pop3Get(url, ScURL::mfDontVerifySslPeer, mail_list.at(i).Key, msg, 0, 0)) {
					output_path = MakeOutputFilePath(temp_buf.Z().Cat(i).DotCat("output"));
					msg.DebugOutput(temp_buf.Z());
					SFile f_output(output_path, SFile::mWrite);
					if(f_output.IsValid())
						f_output.WriteLine(temp_buf);
					{
						const uint attcount = msg.GetAttachmentCount();
						for(uint j = 0; j < attcount; j++) {
							output_path = MakeOutputFilePath("attachment");
							msg.SaveAttachmentTo(j, output_path, 0);
						}
					}
				}
			}
		}
		*/
	}
	CATCH
		CurrentStatus = 0;
		ok = 0;
	ENDCATCH
	return CurrentStatus;
}

SLTEST_R(ScURL_Ftp)
{
	int    ok = 1;
	SString temp_buf;
	SString url_text;
	InetUrl url;
	{
		uint   arg_no = 0;
		if(EnumArg(&arg_no, temp_buf)) {
			url.Parse(temp_buf);
			if(EnumArg(&arg_no, temp_buf)) {
				url.SetComponent(url.cUserName, temp_buf);
				if(EnumArg(&arg_no, temp_buf))
					url.SetComponent(url.cPassword, temp_buf);
			}
		}
	}
	SFileEntryPool pool;
	{
		{
			SUniformFileTransmParam uftp;
			uftp.DestPath = "http://posttestserver.com/post.php";
			uftp.SrcPath = MakeInputFilePath("test11.jpg");
			uftp.Format = SFileFormat::Jpeg;
			THROW(SLCHECK_NZ(uftp.Run(0, 0)));
			temp_buf = uftp.Reply;
		}
		{
			SUniformFileTransmParam uftp;
			uftp.DestPath = "https://posttestserver.com/post.php";
			uftp.SrcPath = MakeInputFilePath("test10.jpg");
			uftp.Format = SFileFormat::Jpeg;
			THROW(SLCHECK_NZ(uftp.Run(0, 0)));
			temp_buf = uftp.Reply;
		}
	}
	{
		{
			SUniformFileTransmParam uftp;
			uftp.DestPath = MakeOutputFilePath("remote-image.png");
			uftp.SrcPath = "https://www.nasa.gov/sites/default/files/thumbnails/image/pia21775.png";
			THROW(SLCHECK_NZ(uftp.Run(0, 0)));
		}
	}
	{
		ScURL curl;
		THROW(SLCHECK_NZ(curl.FtpList(url, ScURL::mfVerbose, pool)));
	}
	{
		ScURL curl;
		url.SetComponent(InetUrl::cPath, "test/");
		THROW(SLCHECK_NZ(curl.FtpChangeDir(url, ScURL::mfVerbose)));
	}
	{
		ScURL curl;
		url.SetComponent(InetUrl::cPath, "test/abc/");
		THROW(SLCHECK_NZ(curl.FtpCreateDir(url, ScURL::mfVerbose)));
	}
	{
		ScURL curl;
		url.SetComponent(InetUrl::cPath, "test/abc");
		THROW(SLCHECK_NZ(curl.FtpDeleteDir(url, ScURL::mfVerbose)));
	}
	{
		ScURL curl;
		url.SetComponent(InetUrl::cPath, "test/subdir01/subdir02/");
		THROW(SLCHECK_NZ(curl.FtpChangeDir(url, ScURL::mfVerbose)));
	}
	{
		ScURL curl;
		THROW(SLCHECK_NZ(curl.FtpPut(url, ScURL::mfVerbose, 0, MakeInputFilePath("binfile"), 0)));
	}
	{
		ScURL curl;
		url.GetComponent(InetUrl::cPath, 0, temp_buf);
		temp_buf.SetLastDSlash().Cat("binfile");
		url.SetComponent(InetUrl::cPath, temp_buf);
		THROW(SLCHECK_NZ(curl.FtpGet(url, 0, MakeOutputFilePath("binfile-from-ftp"), 0, 0)));
		//THROW(SLCHECK_NZ(curl.FtpDelete(url, 0)));
	}
	{
		ScURL curl;
		//url.GetComponent(InetUrl::cPath, temp_buf);
		//temp_buf.SetLastDSlash().Cat("binfile");
		//url.SetComponent(InetUrl::cPath, temp_buf);
		THROW(SLCHECK_NZ(curl.FtpDelete(url, 0)));
	}
	CATCH
		CurrentStatus = 0;
		ok = 0;
	ENDCATCH
	return CurrentStatus;
}

SLTEST_R(Uri)
{
	int    ok = 1;
	SFile f_in(MakeInputFilePath("url-list.txt"), SFile::mRead);
	SFile f_out(MakeOutputFilePath("url-out.txt"), SFile::mWrite);
	InetUrl url(0);
	SString temp_buf;
	SString out_buf;
	SString line_buf;
	THROW(SLCHECK_NZ(f_in.IsValid()));
	THROW(SLCHECK_NZ(f_out.IsValid()));
	while(f_in.ReadLine(line_buf)) {
		line_buf.Chomp().Strip();
		out_buf = line_buf;
		int    r = url.Parse(line_buf);
		if(r > 0) {
			url.GetComponent(InetUrl::cScheme, 0, temp_buf);
			if(temp_buf.NotEmpty())
				out_buf.Tab().CatEq("scheme", temp_buf);

			url.GetComponent(InetUrl::cUserName, 0, temp_buf);
			if(temp_buf.NotEmpty())
				out_buf.Tab().CatEq("username", temp_buf);

			url.GetComponent(InetUrl::cPassword, 0, temp_buf);
			if(temp_buf.NotEmpty())
				out_buf.Tab().CatEq("password", temp_buf);

			url.GetComponent(InetUrl::cHost, 0, temp_buf);
			if(temp_buf.NotEmpty())
				out_buf.Tab().CatEq("host", temp_buf);

			url.GetComponent(InetUrl::cPort, 0, temp_buf);
			if(temp_buf.NotEmpty())
				out_buf.Tab().CatEq("port", temp_buf);

			url.GetComponent(InetUrl::cPath, 0, temp_buf);
			if(temp_buf.NotEmpty())
				out_buf.Tab().CatEq("path", temp_buf);

			url.GetComponent(InetUrl::cQuery, 0, temp_buf);
			if(temp_buf.NotEmpty())
				out_buf.Tab().CatEq("query", temp_buf);

			url.GetComponent(InetUrl::cRef, 0, temp_buf);
			if(temp_buf.NotEmpty())
				out_buf.Tab().CatEq("ref", temp_buf);
			{
				url.Compose(url.stAll, temp_buf);
				out_buf.Tab().CatEq("composition", temp_buf);
			}
			f_out.WriteLine(out_buf.CR());
		}
	}
	CATCH
		CurrentStatus = 0;
		ok = 0;
	ENDCATCH
	return CurrentStatus;
}
//
//
//
SLTEST_R(S_IPAddr)
{
	SString temp_buf;
	const char * pp_ip4_list[] = {
		"0.0.0.0", // @anchor
		"127.0.0.1",
		"255.255.255.0",
		"1.1.1.1",
		"217.77.50.185"
	};
	{
		S_IPAddr ia;
		for(uint i = 0; i < SIZEOFARRAY(pp_ip4_list); i++) {
			const char * p_ip4_addr = pp_ip4_list[i];
			SLCHECK_NZ(ia.FromStr(p_ip4_addr));
			if(i == 0) {
				SLCHECK_NZ(ia.IsZero());
				SLCHECK_NZ(!ia.IsIp4());
				SLCHECK_NZ(!ia.IsIp6());
			}
			else {
				SLCHECK_NZ(!ia.IsZero());
				SLCHECK_NZ(ia.IsIp4());
				SLCHECK_NZ(!ia.IsIp6());
			}
			ia.ToStr(0, temp_buf);
			SLCHECK_EQ(temp_buf, p_ip4_addr);
			{
				//char * STDCALL Sl_Curl_InetNtop(int af, const void * pSrc, char * pBuf, size_t bufSize)
				char    inetntop_buf[128];
				Sl_Curl_InetNtop(AF_INET, ia.D, inetntop_buf, sizeof(inetntop_buf));
				SLCHECK_EQ(temp_buf, inetntop_buf);
			}
		}
	}
	return CurrentStatus;
}

SLTEST_R(MACAddr)
{
	const char * p_txt_macadr_list = "27-EF-D3-30-C0-D2,"
		"83-60-A3-2B-78-91,AA-97-A6-41-9D-D7,EA-FB-0D-55-CD-3F,FA-EA-AC-BF-71-2B,49-6E-82-2C-B0-4F,2E-0D-59-F5-B1-FB,"
		"04-3E-68-A7-FA-8F,FE-F8-DC-C8-F2-AF,2C-85-88-AB-96-A1,2E-5C-20-4A-FD-3E,AB-CC-E5-7E-4C-A3,FD-9C-B5-AD-9C-AE,"
		"E3-55-6A-F1-49-ED,BE-AC-DC-9B-E1-41,0A-5F-A8-B3-B9-21,FE-9D-F4-30-9B-B2,2A-B9-86-0B-E8-DA,9D-A1-B2-41-B4-E3,"
		"FD-7C-5B-CD-BA-6F,01-A0-6A-D0-3C-D4,4A-ED-ED-AD-EE-1D,B7-D3-E7-FF-AE-3D,D0-8F-BC-F8-F1-D4,9D-29-AC-2F-DA-5C,"
		"E4-37-8E-46-CA-AE,3F-EB-F8-50-A7-3C,67-BB-2A-5E-A4-B4,45-E1-1E-8C-AE-78,2F-FF-07-EE-7D-FB,B7-EC-AE-88-BB-EE,"
		"F5-48-A9-EC-B9-F2,F0-4B-F2-DD-CB-BB,0B-5D-02-6B-59-DF,2F-7D-4D-49-21-46,77-38-7E-EF-1A-6F,C0-A2-64-B0-71-B6,"
		"FD-10-DB-DC-1E-DA,1C-2C-A4-82-B8-4E,BC-CB-E9-A8-8C-DF,2E-DC-B8-28-CF-AA,B2-F4-B9-D7-CF-DD,8F-9D-E6-FE-C7-C4,"
		"1C-6D-6F-5B-DB-0E,7F-2B-D5-33-63-CF,08-61-12-1F-D3-E3,DB-AA-DD-BE-F4-EF,40-9F-AD-8C-D9-AD,41-FC-7A-95-CC-8C";
	MACAddrArray macadr_list;
	SString temp_buf;
	THROW(SLCHECK_NZ(GetMACAddrList(&macadr_list)));
	THROW(SLCHECK_NZ(macadr_list.getCount()));
	{
		MACAddr _macadr;
		SLCHECK_NZ(_macadr.IsZero());
		_macadr.Randomize();
		SLCHECK_Z(_macadr.IsZero());
		_macadr.Z();
		SLCHECK_NZ(_macadr.IsZero());
	}
	{
		StringSet ss(',', p_txt_macadr_list);
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			MACAddr macadr;
			SLCHECK_NZ(macadr.FromStr(temp_buf));
			SLCHECK_Z(macadr.IsZero());
			macadr_list.insert(&macadr);
		}
	}
	for(uint i = 0; i < macadr_list.getCount(); i++) {
		MACAddr temp_macadr;
		const MACAddr & r_macadr = macadr_list.at(i);
		r_macadr.ToStr(MACAddr::fmtDivColon, temp_buf);
		SLCHECK_NZ(temp_buf.HasChr(':'));
		SLCHECK_Z(temp_buf.HasChr('-'));
		SLCHECK_Z(temp_buf.HasChr(' '));
		SLCHECK_NZ(temp_macadr.FromStr(temp_buf));
		SLCHECK_NZ(temp_macadr == r_macadr);
		SLCHECK_NZ(temp_macadr.IsEq(r_macadr));
		//
		r_macadr.ToStr(0, temp_buf);
		SLCHECK_Z(temp_buf.HasChr(':'));
		SLCHECK_NZ(temp_buf.HasChr('-'));
		SLCHECK_Z(temp_buf.HasChr(' '));
		SLCHECK_NZ(temp_macadr.FromStr(temp_buf));
		SLCHECK_NZ(temp_macadr == r_macadr);
		SLCHECK_NZ(temp_macadr.IsEq(r_macadr));
		//
		r_macadr.ToStr(MACAddr::fmtPlain, temp_buf);
		SLCHECK_Z(temp_buf.HasChr(':'));
		SLCHECK_Z(temp_buf.HasChr('-'));
		SLCHECK_Z(temp_buf.HasChr(' '));
		{
			for(uint ci = 0; ci < temp_buf.Len(); ci++) {
				SLCHECK_Z(isasciilwr(temp_buf.C(ci)));
			}
		}
		SLCHECK_NZ(temp_macadr.FromStr(temp_buf));
		SLCHECK_NZ(temp_macadr == r_macadr);
		SLCHECK_NZ(temp_macadr.IsEq(r_macadr));
		//
		r_macadr.ToStr(MACAddr::fmtLower, temp_buf);
		SLCHECK_Z(temp_buf.HasChr(':'));
		SLCHECK_NZ(temp_buf.HasChr('-'));
		SLCHECK_Z(temp_buf.HasChr(' '));
		{
			for(uint ci = 0; ci < temp_buf.Len(); ci++) {
				SLCHECK_Z(isasciiupr(temp_buf.C(ci)));
			}
		}
		SLCHECK_NZ(temp_macadr.FromStr(temp_buf));
		SLCHECK_NZ(temp_macadr == r_macadr);
		SLCHECK_NZ(temp_macadr.IsEq(r_macadr));
		{
			uint64 ued = UED::SetRaw_MacAddr(r_macadr);
			SLCHECK_NZ(ued);
			SLCHECK_NZ(UED::GetRaw_MacAddr(ued, temp_macadr));
			SLCHECK_NZ(temp_macadr == r_macadr);
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}