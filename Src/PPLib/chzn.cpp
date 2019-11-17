// CHZN.CPP
// Copyright (c) A.Sobolev 2019
// @codepage UTF-8
// Реализация интерфейса к сервисам честный знак
//
#include <pp.h>
#pragma hdrstop

class ChZnInterface {
public:
	SLAPI  ChZnInterface()
	{
	}
	SLAPI ~ChZnInterface()
	{
	}
	int    SLAPI Connect(PPID guaID)
	{
		int    ok = 1;
		json_t * p_json_req = 0;
		SString temp_buf;
		//SString addr = "https://api.stage.mdlp.crpt.ru/api/v1/auth/"; // Test
		//SString addr = "http://185.196.171.27/api/v1/auth/";
		//SString addr = "https://api.sb.mdlp.crpt.ru"; // SandBox
		SString addr = "https://api.mdlp.crpt.ru"; // Productive
		SString req_buf;
		SString cli_ident;
		SString cli_accskey;
		SString cli_secret;
		PPObjGlobalUserAcc gua_obj;
		PPGlobalUserAccPacket gua_pack;
		THROW(gua_obj.GetPacket(guaID, &gua_pack) > 0);
		gua_pack.TagL.GetItemStr(PPTAG_GUA_ACCESSKEY, cli_accskey);
		gua_pack.TagL.GetItemStr(PPTAG_GUA_SECRET, cli_secret);
		gua_pack.TagL.GetItemStr(PPTAG_GUA_LOGIN, cli_ident); // Отпечаток открытого ключа 
			// Сертификат в реестре находится в "сертификаты-текущий пользователь/личное/реестр/сертификаты". 
			// Требуется в доверенные еще внести сертификат Крипто-Про (в инструкции по быстрому старту про это есть). 
		{
			ScURL c;
			InetUrl url(addr);
			StrStrAssocArray hdr_flds;
			SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, "application/json;charset=UTF-8");
			{
				p_json_req = new json_t(json_t::tOBJECT);
				p_json_req->Insert("client_id", json_new_string(cli_accskey));
				p_json_req->Insert("client_secret", json_new_string(cli_secret));
				p_json_req->Insert("user_id", json_new_string(cli_ident));
				p_json_req->Insert("auth_type", json_new_string("PASSWORD"));
				THROW_SL(json_tree_to_string(p_json_req, req_buf));
			}
			{
				SBuffer ack_buf;
				SFile wr_stream(ack_buf, SFile::mWrite);
				THROW_SL(c.SetupDefaultSslOptions(SSystem::sslTLS_v10, 0)); //CURLOPT_SSLVERSION значением CURL_SSLVERSION_TLSv1_0
				THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
				{
					SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
					if(p_ack_buf) {
						const int avl_size = (int)p_ack_buf->GetAvailableSize();
						temp_buf.Z().CatN((const char *)p_ack_buf->GetBuf(), avl_size);
						//f_out_test.WriteLine(temp_buf.CR().CR());
					}
				}
			}
		}
		CATCHZOK
		json_free_value(&p_json_req);
		return ok;
	}
private:
};

class ChZnPrcssr {
public:
	struct Param {
		Param() : GuaID(0)
		{
		}
		PPID   GuaID;
	};
	int SLAPI EditParam(Param * pParam)
	{
		class ChZnPrcssrParamDialog : public TDialog {
			DECL_DIALOG_DATA(ChZnPrcssr::Param);
		public:
			ChZnPrcssrParamDialog() : TDialog(DLG_CHZNIX)
			{
			}
			DECL_DIALOG_SETDTS()
			{
				RVALUEPTR(Data, pData);
				SetupPPObjCombo(this, CTLSEL_CHZNIX_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, OLW_CANINSERT, 0);
				return 1;
			}
			DECL_DIALOG_GETDTS()
			{
				int    ok = 1;
				getCtrlData(CTLSEL_CHZNIX_GUA, &Data.GuaID);
				ASSIGN_PTR(pData, Data);
				return ok;
			}
		};
		DIALOG_PROC_BODY(ChZnPrcssrParamDialog, pParam);
	}
};

int SLAPI TestChZn()
{
	int    ok = 1;
	ChZnPrcssr prcssr;
	ChZnPrcssr::Param param;
	if(prcssr.EditParam(&param) > 0) {
		ChZnInterface ifc;
		ifc.Connect(param.GuaID);
	}
	return ok;
}