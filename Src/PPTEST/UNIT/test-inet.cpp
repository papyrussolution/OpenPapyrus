// TEST-INET.CPP
// Copyright (c) A.Sobolev 2023
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
	rBuf.Tab(tab).Cat("boundary").Space().CatEq("lines", temp_buf).CR();
	GetS(rB.Ct.MimeP, temp_buf);
	rBuf.Tab(tab+1).CatEq("ct:mime", temp_buf).CR();
	GetS(rB.Ct.TypeP, temp_buf);
	rBuf.Tab(tab+1).CatEq("ct:type", temp_buf).CR();
	GetS(rB.Ct.NameP, temp_buf);
	rBuf.Tab(tab+1).CatEq("ct:name", temp_buf).CR();
	GetS(rB.Ct.BoundaryP, temp_buf);
	rBuf.Tab(tab+1).CatEq("ct:boundaryid", temp_buf).CR();
	rB.Ct.Cp.ToStr(SCodepageIdent::fmtDefault, temp_buf);
	rBuf.Tab(tab+1).CatEq("ct:cp", temp_buf).CR();
	//
	rBuf.Tab(tab+1).CatEq("cd:type", (long)rB.Cd.Type).CR();
	GetS(rB.Cd.NameP, temp_buf);
	rBuf.Tab(tab+1).CatEq("cd:name", temp_buf).CR();
	GetS(rB.Cd.FileNameP, temp_buf);
	rBuf.Tab(tab+1).CatEq("cd:filename", temp_buf).CR();
	if(rB.Cd.Size != 0)
		rBuf.Tab(tab+1).CatEq("cd:size", rB.Cd.Size).CR();
	if(!!rB.Cd.CrDtm) {
		temp_buf.Z().Cat(rB.Cd.CrDtm, DATF_ISO8601CENT, 0);
		rBuf.Tab(tab+1).CatEq("cd:crdtm", rB.Cd.Size).CR();
	}
	if(!!rB.Cd.ModifDtm) {
		temp_buf.Z().Cat(rB.Cd.ModifDtm, DATF_ISO8601CENT, 0);
		rBuf.Tab(tab+1).CatEq("cd:modifdtm", rB.Cd.Size).CR();
	}
	if(!!rB.Cd.RdDtm) {
		temp_buf.Z().Cat(rB.Cd.RdDtm, DATF_ISO8601CENT, 0);
		rBuf.Tab(tab+1).CatEq("cd:readdtm", rB.Cd.Size).CR();
	}
	rBuf.Tab(tab+1).CatEq("contenttypeenc", (long)rB.ContentTransfEnc).CR();
	rBuf.Tab(tab+1).CatEq("datasize", rB.Data.GetAvailableSize()).CR();
	for(uint i = 0; i < rB.Children.getCount(); i++) {
		const Boundary * p_b = rB.Children.at(i);
		if(p_b) {
			DebugOutput_Boundary(*p_b, tab+1, rBuf); // @recursion
		}
		else {
			rBuf.Tab(tab+2).Cat("zeroptr child boundary").CR();
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
				SPathStruc ps(f_in.GetName());
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
	temp_buf.Z().Cat("Соболев Антон").Space().CatChar('<').Cat("soobolev@yandex.ru").CatChar('>').Transf(CTRANSF_OUTER_TO_UTF8);
	rMsg.SetField(rMsg.fldFrom, temp_buf);
	temp_buf.Z().Cat("Соболев Антон").Space().CatChar('<').Cat("sobolev@petroglif.ru").CatChar('>');
	temp_buf.Comma().Cat("Папирус Сольюшн").Space().CatChar('<').Cat("papyrussolution@gmail.com").CatChar('>');
	// @v10.4.5 temp_buf.Transf(CTRANSF_OUTER_TO_UTF8);
	rMsg.SetField(rMsg.fldTo, temp_buf);
	temp_buf.Z().Cat("Тестовое письмо для проверки правильности формирования сообщения. Вот!")/* @v10.4.5 .Transf(CTRANSF_OUTER_TO_UTF8)*/;
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
			url.Composite(0, temp_buf);
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
	SString out_buf, temp_buf;
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
				url.Composite(url.stAll, temp_buf);
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
