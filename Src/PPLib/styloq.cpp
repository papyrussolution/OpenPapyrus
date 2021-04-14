// STYLOQ.CPP
// Copyright (c) A.Sobolev 2021
//
#include <pp.h>
#pragma hdrstop
/*
	Обмен данными осуществляется посредством брокера сообщений Rabbit-MQ или прямым соединением с сервером,
	предоставляющими сервис.

	Термины:
	Публичный идентификатор сервиса (SVCPID) - идентификатор, публикуемый сервисом для того, чтобы его могли узнавать клиенты
	Публичный идентификатор клиента, ассоциированный с сервисом (CLSPID) - идентификатор, передаваемый клиентом сервису для того, чтобы
		тот мог узнать клиента. Идентификаторы одного клиента для разных сервисов различаются.
	Лик (CLFACE) - представление клиента перед сервисом. Клиент может иметь произвольное число ликов, котоыми представляется перед
		сервисами. Однако, отношение CLSPID->CLFACE определяется как 1:1, то есть, в любой момент времени сервис соотносит лишь
		один лик клиента с его идентификатором. 
		Лики обязательно идентифицируются уникальным образом лишь на клиенте: клиент не обязан передавать никакой технически уникальной информации,
		ассоциированной с ликом, сервису.
		С другой стороны, если клиент принимает решение передать сервису свои социальные публичные реквизиты, он делает это добровольно
		(возможно, по специальному требованию сервиса) с осознанием того, что с момента передачи этих данных два или более различных сервисов
		технически способны сопоставить его публичные идентфикаторы (CLSPID) и понять, что они соответствуют одному и тому же актору.

		Каждый лик принадлежит одной из трех категорий: 
		1. Формально анонимный: никаких явных обозначений лика, кроме случайно сгенерированного имени, клиент сервису не передает,
			при этом клиент извещает сервис о том, что лик анонимный
		2. Формально верифицируемый: клиент передает сервису информацию, позволяющую с высокой вероятностью идентифицировать
			конкретное лицо. Например, сочетание имени, отчества, фамилии и даты рождения, или номер телефона. В любом случае,
			лик, относящийся к этой категории таковым и объявляется. Другими словами, клиент, приняв решение передать сервису
			формально верифицируемый лик явно сообщает об этом сервису и сам осознает то, что делает.
		3. Произвольный: клиент не объявляет свой лик ни как анонимный, ни как формально верифицируемый. Таким образом, сервис
			теоретически не способен понять с кем именно имеет дело и никто не способен сопоставить данные этого клиента на разных
			сервисах.
	Сервисная команда (SVCCMD) - действие, инициируемое клиентом и исполняемое сервисом. Команда имеет текстовый заголовок, который
		понятен и клиенту и сервису. Параметры команды определяются набором строковых тегов в виде key-value, передавамым посредством
		формализованного формата (json или xml).

	Сервис публикует собственный публичный идентификатор, по которому его узнают клиенты
	Клиент при регистрации на сервисе предоставляет собственный публичный идентификатор, ассоциированный только с этим сервисом.
	Клиент не использует один и тот же идентификатор для взаимодействия с разными сервисами.
	Клиент передает сервису публичный идентфикатор вместе с ликом.

	Сценарии:
	I. Работа по QR-приглашению
	1.1 Сервис публикует приглашение (QR-code), содержащее публичный ИД сервиса, адрес сервера обмена и возможности обмена и команду,
		которую может выполнить клиент.
	1.2 Если клиент не имеет регистрации сервиса и опции приглашения допускают регистрацию, то клиент автоматически регистрируется 
		(процесс регистрации будет описан позже)
	1.3 Если клиент зарегистрирован в сервисе, то принимает приглашение, устанавливает соединение и инициирует выполнение 
		опубликованной команды сервисом.
	II. Выполнение команды сервисом, инициируемое зарегистрированным клиентом
	2.1 Клиент устанавливает соединение с сервисом по данным, которые были сохранены при последнем контакте. 
		Предполагается один из следующих вариантов:
		2.1.1 Срок действия параметров последней сессии не истек - в этом случае клиент соединяется по этим параметрам
		2.1.2 Параметры последней сессии утратили валидность - клиент пытается инициировать новую сессию с сервисом
		После установки соединения клиент инициирует одну из команд, опубликованную сервисом
	III. Информирование сервисом зарегистрированного клиента. Сервис имеет возможность передать какую-либо информацию 
		зарегистрированному клиенту. При этом клиент имеет возможность блокировать получение информации от сервиса.
	IV. Поиск сервисов клиентом. Клиент имеет возможность отправить широковещательный формальный запрос, который будет
		виден всем сервисам независимо от того, имеет клиент у них регистрацию или нет. Любой сервис имеет возможность
		ответить на широковещательный запрос клиента. При этом, как запрос клиента, так и ответы сервисом проходят
		модерацию специальным сервисом-посредником с целью нормализации и фильтрации запросов, а так же ранжирования 
		ответов.

	Предполагается, что обмен данными реализуется по открытому каналу и сервисы не способны эффективно защитить данные,
	которыми они располагают.

	Регистрация и авторизация:
		Процесс регистрации и авторизации реализуется посредством протокола SRP. Это исключает необходимость посредничества
		и обеспечивает необходимый уровень защиты (на мой взгляд).
	Обмен данными между сервисом и клиентом:
		Обмен осуществляется посредством зашифрованных блоков. Применяется шифрование с открытым ключем (RSA). Закрытый и публичный
		ключи генерируются при формировании сессии обмена. Время жизни сессии ограничено.
	Каждый сервис и каждый клиент располагают собственными постоянными приватными ключами, которые применяются ситуативно
	для выполнения вспомогательных функций. Получение такого ключа злоумышленником в распоряжение не должно нанести какого-либо
	значительного массового ущерба.

	Цикл сообщений клиент->сервис:
	1. Клиент посылает сообщение, содержащее:
	    -- идентификатор сервиса
		-- публичный идентификатор сопоставленный сервису
*/
//
// @construction
//
class PPStyloQInterchange {
public:
	struct Service {
		PPID   ID;
		SString Appellation;
		S_GUID PublicIdent;
	};
	struct Account {
		PPID   ID;
		PPID   PersonID;
		long   Flags;
		S_GUID PublicIdent;
	};
	struct Command {
		
	};
	struct Invitation {
		enum {
			capRegistrationAllowed = 0x0001,
			capVerifiableFaceOnly  = 0x0002,
			capAnonymFaceDisabled  = 0x0004
		};
		Invitation() : Capabitities(0)
		{
		}
		Invitation & Z()
		{
			SvcPublicId.Z();
			Capabitities = 0;
			AccessPoint.Z();
			Command.Z();
			CommandParam.Z();
			return *this;
		}
		int    FASTCALL IsEqual(const Invitation & rS) const
		{
			return (SvcPublicId == rS.SvcPublicId && Capabitities == rS.Capabitities && AccessPoint == rS.AccessPoint && 
				Command == rS.Command && CommandParam == rS.CommandParam);
		}
		S_GUID SvcPublicId;
		uint32 Capabitities; // capXXX
		SString AccessPoint;
		SString Command;
		SString CommandParam;
	};
	struct TransportPacket {
		SSecretTagPool P;
	};
	class Client {
	public:
		Client()
		{
		}
		~Client()
		{
		}
	};
	class Server {
	public:
		Server()
		{
		}
		~Server()
		{
		}
	};
	PPStyloQInterchange();
	~PPStyloQInterchange();
	//
	// Descr: Функция реализует первоначальную генерацию необходимых ключей
	//   и значений с сохранением их в базе данных.
	// Returns:
	//   >0 - функция успешно выполнила первоначальную инициализацию
	//   <0 - инсталляция клиента/сервера уже инициализирована
	//    0 - ошибка
	//
	int    SetupPeerInstance();
	//
	// Descr: Публикация приглашения сервисом
	//
	int    MakeInvitation(const Invitation & rSource, SString & rInvitationData);
	//
	// Descr: Акцепт приглашения сервиса клиентом
	//
	int    AcceptInvitation(const char * pInvitationData, Invitation & rInv);
	int    SendPacket(void * pState, TransportPacket & rPack);
	int    AcceptPacket(void * pState, TransportPacket & rPack);

	int    Registration_ClientInit(void * pState, TransportPacket & rPack);
	int    Registration_ServiceReply(void * pState, TransportPacket & rPack);
	int    Registration_ClientSendVerification(void * pState, TransportPacket & rPack);
	int    Registration_ServiceAckVerification(void * pState, TransportPacket & rPack);

	int    Auth_ClientInit(void * pState, TransportPacket & rPack); // User -> Host: (username, bytes_A) 
	int    Auth_ServiceChallenge(void * pState, TransportPacket & rPack); // Host -> User: (bytes_s, bytes_B) 
	int    Auth_ClientVerification(void * pState, TransportPacket & rPack); // User -> Host: (bytes_M) 
	int    Auth_ServiceAck(void * pState, TransportPacket & rPack); // Host -> User: (HAMK) 

	int    Session_ClientInit(void * pState, TransportPacket & rPack);
	int    Session_ServiceReply(void * pState, TransportPacket & rPack);

private:
	int    GetClientIdentForService(const void * pSvcId, size_t svcIdLen, SSecretTagPool & rPool);
	StyloQSecTbl T;
};

int PPStyloQInterchange::SetupPeerInstance()
{
	int    ok = -1;
	return ok;
}

int PPStyloQInterchange::GetClientIdentForService(const void * pSvcId, size_t svcIdLen, SSecretTagPool & rPool)
{
	int    ok = 1;

	return ok;
}
//
// Registration {
//
struct PPStyloQInterchange_RegistrationContext {
};

int PPStyloQInterchange::Registration_ClientInit(void * pState, TransportPacket & rPack)
{
	int    ok = 0;
	PPStyloQInterchange_RegistrationContext * p_ctx = 0;
	SSecretTagPool tp;
	SString temp_buf;
	SBinaryChunk cli_ident;
	SBinaryChunk cli_secret;
	if(GetClientIdentForService(0, 0, tp)) {
		SBinaryChunk __s;
		SBinaryChunk __v;
		THROW(tp.Get(SSecretTagPool::tagPublicIdent, &cli_ident));
		THROW(tp.Get(SSecretTagPool::tagSecret, &cli_secret));
		temp_buf.EncodeMime64(cli_ident.PtrC(), cli_ident.Len());
		SlSRP::CreateSaltedVerificationKey2(SlSRP::SRP_SHA1, SlSRP::SRP_NG_8192, temp_buf, PTR8C(cli_secret.PtrC()), cli_secret.Len(), __s, __v, 0, 0);
		rPack.P.Put(SSecretTagPool::tagPublicIdent, cli_ident);
		rPack.P.Put(SSecretTagPool::tagSrpVerifierSalt, __s);
		rPack.P.Put(SSecretTagPool::tagSrpVerifier, __v);
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::Registration_ServiceReply(void * pState, TransportPacket & rPack)
{
	int    ok = 0;
	return ok;
}

int PPStyloQInterchange::Registration_ClientSendVerification(void * pState, TransportPacket & rPack)
{
	int    ok = 0;
	return ok;
}

int PPStyloQInterchange::Registration_ServiceAckVerification(void * pState, TransportPacket & rPack)
{
	int    ok = 0;
	return ok;
}
//
//
PPStyloQInterchange::PPStyloQInterchange()
{
}

PPStyloQInterchange::~PPStyloQInterchange()
{
}
	
int PPStyloQInterchange::MakeInvitation(const Invitation & rInv, SString & rInvitationData)
{
	int    ok = 1;
	SString temp_buf;
	rInvitationData.Z();
	// prefix SVCPID SVCCAP URL [SVCCMD [SVCCMDPARAM]]
	THROW(!rInv.SvcPublicId.IsZero() && rInv.AccessPoint.NotEmpty());
	assert(rInv.Command.NotEmpty() || rInv.CommandParam.IsEmpty());
	THROW(rInv.Command.NotEmpty() || rInv.CommandParam.IsEmpty());
	rInvitationData.CatChar('A'); // prefix
	temp_buf.EncodeMime64(&rInv.SvcPublicId, sizeof(rInv.SvcPublicId));
	rInvitationData.Cat(temp_buf);
	{
		temp_buf.Z().EncodeMime64(&rInv.Capabitities, sizeof(rInv.Capabitities));
		rInvitationData.CatChar('&').Cat(temp_buf);
	}
	temp_buf.Z().EncodeMime64(rInv.AccessPoint, rInv.AccessPoint.Len());
	rInvitationData.CatChar('&').Cat(temp_buf);
	if(rInv.Command.NotEmpty()) {
		temp_buf.Z().EncodeMime64(rInv.Command, rInv.Command.Len());
		rInvitationData.CatChar('&').Cat(temp_buf);
		if(rInv.CommandParam.NotEmpty()) {
			temp_buf.Z().EncodeMime64(rInv.CommandParam, rInv.CommandParam.Len());
			rInvitationData.CatChar('&').Cat(temp_buf);
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::AcceptInvitation(const char * pInvitationData, Invitation & rInv)
{
	int    ok = 1;
	rInv.Z();
	STempBuffer temp_binary(4096);
	StringSet ss;
	SString temp_buf(pInvitationData);
	temp_buf.Tokenize("&", ss);
	uint   tokn = 0;
	for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
		tokn++;
		if(tokn == 1) { // prefix and svcid
			const char first_symb = temp_buf.C(0);
			size_t actual_size = 0;
			THROW(first_symb == 'A'); // @error invalid invitation prefix
			temp_buf.ShiftLeft();
			temp_buf.DecodeMime64(temp_binary, temp_binary.GetSize(), &actual_size);
			THROW(actual_size == sizeof(rInv.SvcPublicId)); // @error invalid service public id
			memcpy(&rInv.SvcPublicId, temp_binary, actual_size);
		}
		else if(tokn == 2) { // capabilities
			size_t actual_size = 0;
			temp_buf.DecodeMime64(temp_binary, temp_binary.GetSize(), &actual_size);
			THROW(actual_size == sizeof(rInv.Capabitities)); // @error invalid capabilities
			memcpy(&rInv.Capabitities, temp_binary, actual_size);
		}
		else if(tokn == 3) { // access point
			size_t actual_size = 0;
			temp_buf.DecodeMime64(temp_binary, temp_binary.GetSize(), &actual_size);
			temp_buf.Z().CatN(temp_binary, actual_size);
			InetUrl url(temp_buf);
			THROW(url.GetProtocol());
			{
				SString host;
				url.GetComponent(InetUrl::cHost, 0, host); 
				THROW(host.NotEmpty()); // @error invalid url
			}
			rInv.AccessPoint = temp_buf;
		}
		else if(tokn == 4) { // command
			size_t actual_size = 0;
			temp_buf.DecodeMime64(temp_binary, temp_binary.GetSize(), &actual_size);
			temp_buf.Z().CatN(temp_binary, actual_size);
			rInv.Command.CatN(temp_binary, actual_size);
		}
		else if(tokn == 5) { // command params
			size_t actual_size = 0;
			temp_buf.DecodeMime64(temp_binary, temp_binary.GetSize(), &actual_size);
			temp_buf.Z().CatN(temp_binary, actual_size);
			rInv.CommandParam.CatN(temp_binary, actual_size);
		}
		else {
			CALLEXCEPT(); // invalid token count
		}
	}
	CATCHZOK
	return ok;
}

int Test_PPStyloQInterchange_Invitation()
{
	int    ok = 1;
	SString temp_buf;
	PPStyloQInterchange ic;
	{
		PPStyloQInterchange::Invitation inv_source;
		PPStyloQInterchange::Invitation inv_result;
		inv_source.SvcPublicId.Generate();
		inv_source.AccessPoint = "AMQP://192.168.0.1/test";
		inv_source.Capabitities = 0;
		inv_source.Command = "comein";
		inv_source.CommandParam = "yourpromo=100";
		ic.MakeInvitation(inv_source, temp_buf);
		if(!ic.AcceptInvitation(temp_buf, inv_result))
			ok = 0;
		else if(!inv_result.IsEqual(inv_source))
			ok = 0;
		{
			PPBarcode::BarcodeImageParam bip;
			bip.ColorFg = SClrBlack;
			bip.ColorBg = SClrWhite;
			bip.Code = temp_buf;
			bip.Std = BARCSTD_QR;
			bip.Size.Set(400, 400);
			bip.OutputFormat = SFileFormat::Png;
			PPGetFilePath(PPPATH_OUT, "styloq-invitation.png", bip.OutputFileName);
			if(PPBarcode::CreateImage(bip)) {
				//
				TSCollection <PPBarcode::Entry> bce_list;
				if(PPBarcode::RecognizeImage(bip.OutputFileName, bce_list)) {
					for(uint i = 0; i < bce_list.getCount(); i++) {
						temp_buf = bce_list.at(i)->Code;
					}
				}
			}
		}
	}
	{
		PPStyloQInterchange::Invitation inv_source;
		PPStyloQInterchange::Invitation inv_result;
		inv_source.SvcPublicId.Generate();
		inv_source.AccessPoint = "https://dummy-host.com";
		inv_source.Capabitities = 0;
		ic.MakeInvitation(inv_source, temp_buf);
		if(!ic.AcceptInvitation(temp_buf, inv_result))
			ok = 0;
		else if(!inv_result.IsEqual(inv_source))
			ok = 0;
	}
	{
		PPStyloQInterchange::Invitation inv_source;
		PPStyloQInterchange::Invitation inv_result;
		inv_source.SvcPublicId.Generate();
		inv_source.AccessPoint = "https://riptutorial.com/openssl/example/16738/save-private-key";
		inv_source.Capabitities = 0;
		ic.MakeInvitation(inv_source, temp_buf);
		if(!ic.AcceptInvitation(temp_buf, inv_result))
			ok = 0;
		else if(!inv_result.IsEqual(inv_source))
			ok = 0;
	}
	{
		PPStyloQInterchange::Invitation inv_source;
		PPStyloQInterchange::Invitation inv_result;
		inv_source.SvcPublicId.Generate();
		inv_source.AccessPoint = "https://riptutorial.com/openssl/example/16738/save-private-key";
		inv_source.Capabitities = 0;
		inv_source.Command = "some-command";
		inv_source.CommandParam = "xyz";
		ic.MakeInvitation(inv_source, temp_buf);
		if(!ic.AcceptInvitation(temp_buf, inv_result))
			ok = 0;
		else if(!inv_result.IsEqual(inv_source))
			ok = 0;
	}
	return ok;
}

