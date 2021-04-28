// STYLOQ.CPP
// Copyright (c) A.Sobolev 2021
// @construction
//
#include <pp.h>
#pragma hdrstop
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
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
// Descr: Класс, реализующий механизм обмена пакетами.
//   Модуль - пока в интенсивной разработке. Но этот класс получается довольно сложным потому
//   сразу внесу пояснения.
//   Проблема, которую я пытаюсь решить, заключается в том, что сейчас я точно не знаю какими
//   низкоуровневыми механизмами транспорта буду пользоваться. На первом этапе рассматривается 
//   брокер сообщений RabbitMQ (поскольку я уже умею с ним работать). Кроме того, для тестов будет
//   использоваться обмен в памяти текущего процесса. Далее, предполагаются варианты иных брокеров
//   сообщений а так же прямые tcp-соединения равно как и http. В общем, венегрет знатный, потому
//   и этот класс.
//
//   Итак, _RoundTripPool является контейнером абстрактных элементов _RoundTripPool::Entry
//   каждый из которых держит состояние одной round-trip-транзакции. Имеется в виду многофазного обмена.
//   Например, SRP-авторизация занимает (в зависимости от фантазии) 4-6 фаз. Согласование ключей обмена - 
//   тоже более 2 фаз и так далее. Все это в состоянии ненадежности сети и иных потенциальных траблов.
//   Так вот, _RoundTripPool::Entry содержит виртуальные методы Send и Poll (насчет Send я не уверен,
//   что он здесь нужен). Метод Poll опрашивает транспортный источник есть ли для объекта Entry что-либо
//   от визави. При этом, учитывая то, что природа round-trip-транзакций может быть самой разной, то
//   конкретный класс Entry самостоятельно определяет нужна ему какая-то посылка или нет, а так же, что
//   с ней делать.
//
//   Пошли далее: элементов Entry может быть много (не думаю, что очень, но черт его знает). Транспортное соединение -
//   штука дорогая (установить соединение с брокером или даже с tcp-узлом не быстрое дело). Большинство элементов
//   будут транспортироваться через одного брокера, но это - не точно. Потому в описываемом классе
//   есть встроенный объект Carrier (транспортер). Этих транспортеров может быть несколько. При этом, каждый 
//   элемент Entry имеет собственный (абстрактный) набор параметров транспортировки TransportBlock.
//   Все в сборе выглядит так: 
//      -- Метод Entry::Poll вызывает Entry::GetCarrier(_RoundTripPool *) который возвращает
//      транспортера из родительского класса в зависимости от параметров TransportBlock. Реализация
//      этого возложена на метод _RoundTripPool::GetCarrier(Entry::TransportBlock *),
//      который находит уже готового транспортера с параметрами Entry::TransportBlock или создает нового.
//      -- Если GetCarrier() вернула !0 то мы его используем для того, что б получить PPStyloQInterchange::TransportPacket
//      Если получить такой пакет удалось, то далее метод Poll уже разбирается что с ним делать.  
//
//   При обмене посредством RabbitMQ: 
//      Прием инициирующих сообщений сервисом: Exchange=StyloQ; Queue=MIME64(public-ident)
//      Прием ответов на инициирующие сообщения: Exchange=StyloQ; Queue=round-trip-ident
//
//
// @construction
//
class PPStyloQInterchange {
public:
	// 1 - native service, 2 - foreign service, 3 - client, 4 - client-session, 5 - service-session
	enum {
		kUndef = 0,
		kNativeService = 1, // Собственная идентификация. Используется для любого узла, включая клиентские, которые никогда не будут сервисами //
		kForeignService,
		kClient,
		kSession,
	};
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
		enum {
			opUndef = 0,
			opHandshake, // Обмен ключами для установки соединения //
			opSession,   // Установка соединения по идентификатору сессии
			opSrpAuth,   // SRP-авторизация //
		};
		enum {
			pfPlain = 1,
			pfZ,
		};
		TransportPacket() : Prefix(0), Op(opUndef)
		{
		}
		int    IsValid() const
		{
			return (oneof2(Prefix, pfPlain, pfZ) && oneof3(Op, opHandshake, opSession, opSrpAuth));
		}
		int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
		{
			int    ok = 1;
			THROW(pSCtx->Serialize(dir, Prefix, rBuf));
			THROW(pSCtx->Serialize(dir, Op, rBuf));
			THROW(IsValid());
			THROW(P.Serialize(dir, rBuf, pSCtx));
			THROW(pSCtx->Serialize(dir, Data, rBuf));
			CATCHZOK
			return ok;
		}
		uint32 Prefix;
		uint32 Op;
		SSecretTagPool P;
		SBinaryChunk Data;
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
	int    SetupPeerInstance(PPID * pID, int use_ta);
	//
	// Descr: Публикация приглашения сервисом
	//
	int    MakeInvitation(const Invitation & rSource, SString & rInvitationData);

	enum {
		fsksError = 0,
		fsksNewSession,
		fsksSessionById,
		fsksSessionByCliId,
		fsksSessionBySvcId
	};

	int    FetchSessionKeys(SSecretTagPool & rSessCtx, const void * pForeignIdent, size_t foreignIdentLen)
	{
		const  int ec_curve_name_id = NID_X9_62_prime256v1;
		int    status = fsksError;
		bool   do_generate_keys = true;
		BN_CTX * p_bn_ctx = 0;
		EC_KEY * p_key = 0;
		StoragePacket pack;
		StoragePacket corr_pack;
		SBinaryChunk public_key;
		SBinaryChunk private_key;
		SBinaryChunk sess_secret;
		if(SearchGlobalIdentEntry(pForeignIdent, foreignIdentLen, &pack) > 0) {
			if(pack.Rec.Kind == kSession) {
				if(ExtractSessionFromPacket(pack, rSessCtx)) {
					status = fsksSessionById;
					do_generate_keys = false;
				}
			}
			else if(pack.Rec.Kind == kForeignService) {
				if(pack.Rec.CorrespondID && GetPeerEntry(pack.Rec.CorrespondID, &corr_pack) > 0) {
					if(corr_pack.Rec.Kind == kSession) {
						if(ExtractSessionFromPacket(pack, rSessCtx)) {
							status = fsksSessionBySvcId;
							do_generate_keys = false;
						}
					}
					else {
						// @error Корреспондирующей записью может быть только сессия
					}
				}
			}
			else if(pack.Rec.Kind == kClient) {
				if(pack.Rec.CorrespondID && GetPeerEntry(pack.Rec.CorrespondID, &corr_pack) > 0) {
					if(corr_pack.Rec.Kind == kSession) {
						if(ExtractSessionFromPacket(pack, rSessCtx)) {
							status = fsksSessionByCliId;
							do_generate_keys = false;
						}
					}
					else {
						// @error Корреспондирующей записью может быть только сессия
					}
				}
			}
		}
		if(do_generate_keys) {
			p_bn_ctx = BN_CTX_new();
			THROW(p_key = EC_KEY_new_by_curve_name(ec_curve_name_id)); // Failed to create key curve
			THROW(EC_KEY_generate_key(p_key) == 1); // Failed to generate key
			{
				const EC_GROUP * p_ecg = EC_KEY_get0_group(p_key);
				const EC_POINT * p_public_ecpt = EC_KEY_get0_public_key(p_key);
				const BIGNUM   * p_private_bn = EC_KEY_get0_private_key(p_key);
				THROW(p_private_bn);
				const size_t octlen = EC_POINT_point2oct(p_ecg, p_public_ecpt, POINT_CONVERSION_UNCOMPRESSED, NULL, 0, p_bn_ctx);
				THROW(public_key.Ensure(octlen));
				EC_POINT_point2oct(p_ecg, p_public_ecpt, POINT_CONVERSION_UNCOMPRESSED, static_cast<uchar *>(public_key.Ptr()), public_key.Len(), p_bn_ctx);
				int bn_len = BN_num_bytes(p_private_bn);
				THROW(private_key.Ensure(bn_len));
				BN_bn2bin(p_private_bn, static_cast<uchar *>(private_key.Ptr()));
			}
			rSessCtx.Put(SSecretTagPool::tagSessionPrivateKey, private_key);
			rSessCtx.Put(SSecretTagPool::tagSessionPublicKey, public_key);
			status = fsksNewSession;
		}
		CATCH
			status = fsksError;
		ENDCATCH
		EC_KEY_free(p_key);
		BN_CTX_free(p_bn_ctx);
		return status;
	}
	int    KexClientInit(SSecretTagPool & rSessCtx, const SSecretTagPool & rSvc)
	{
		int    ok = 1;
		SString temp_buf;
		SBinaryChunk svc_acsp;
		SBinaryChunk svc_ident;
		SBinaryChunk sess_pub_key;
		SBuffer transport_buffer;
		SSerializeContext sctx;
		PPMqbClient * p_mqbc = new PPMqbClient();
		THROW(rSvc.Get(SSecretTagPool::tagSvcAccessPoint, &svc_acsp));
		THROW(rSvc.Get(SSecretTagPool::tagSvcIdent, &svc_ident));
		{
			temp_buf.Z().CatN(static_cast<const char *>(svc_acsp.PtrC()), svc_acsp.Len());
			InetUrl url(temp_buf);
			SString host;
			THROW(url.GetComponent(InetUrl::cHost, 0, host));
			if(oneof2(url.GetProtocol(), InetUrl::protAMQP, InetUrl::protAMQPS)) {
				PPMqbClient::LoginParam mqblp;
				mqblp.Auth = "styloq-user";
				mqblp.Secret = "styloq-def-secret";
				mqblp.VHost = "styloq";
				THROW(p_mqbc->Connect(host, NZOR(url.GetPort(), InetUrl::GetDefProtocolPort(InetUrl::protAMQP)/*5672*/)));
				THROW(p_mqbc->Login(mqblp));
				{
					StoragePacket own_pack;
					SBinaryChunk own_ident;
					int  fsks = FetchSessionKeys(rSessCtx, svc_ident.PtrC(), svc_ident.Len());
					THROW(GetOwnPeerEntry(&own_pack) > 0);
					THROW(own_pack.Pool.Get(SSecretTagPool::tagPublicIdent, &own_ident));
					if(fsks == fsksNewSession) {
						TransportPacket tp;
						PPMqbClient::RoutingParamEntry rpe;
						PPMqbClient::MessageProperties props;
						THROW(rSessCtx.Get(SSecretTagPool::tagSessionPublicKey, &sess_pub_key));
						tp.Op = TransportPacket::opHandshake;
						tp.Prefix = TransportPacket::pfPlain;
						tp.P.Put(SSecretTagPool::tagClientIdent, own_ident);
						tp.P.Put(SSecretTagPool::tagSessionPublicKey, sess_pub_key);
						rpe.SetupReserved(PPMqbClient::rtrsrvStyloQRpc, svc_ident.Mime64(temp_buf), 0, 0);
						{
							transport_buffer.Z();
							THROW(tp.Serialize(+1, transport_buffer, &sctx));
							THROW(p_mqbc->Publish(rpe.ExchangeName, rpe.RoutingKey, &props, transport_buffer.constptr(), transport_buffer.GetAvailableSize()));
						}
					}
					else if(fsks == fsksSessionBySvcId) {
					}
				}
			}
		}
		CATCHZOK
		return ok;
	}
	//
	// Descr: Акцепт приглашения сервиса клиентом
	//
	int    AcceptInvitation(const char * pInvitationData, Invitation & rInv);
	int    SendPacket(void * pState, TransportPacket & rPack);
	int    AcceptPacket(void * pState, TransportPacket & rPack);

	int    Registration_ClientInit(void * pState, const void * pSvcIdent, size_t svcIdentLen, TransportPacket & rPack);
	int    Registration_ServiceReply(void * pState, TransportPacket & rPack);
	int    Registration_ClientSendVerification(void * pState, const void * pSvcIdent, size_t svcIdentLen, TransportPacket & rPack);
	int    Registration_ServiceAckVerification(void * pState, TransportPacket & rPack);

	int    Auth_ClientInit(void * pState, TransportPacket & rPack); // User -> Host: (username, bytes_A) 
	int    Auth_ServiceChallenge(void * pState, TransportPacket & rPack); // Host -> User: (bytes_s, bytes_B) 
	int    Auth_ClientVerification(void * pState, TransportPacket & rPack); // User -> Host: (bytes_M) 
	int    Auth_ServiceAck(void * pState, TransportPacket & rPack); // Host -> User: (HAMK) 

	int    Session_ClientInit(void * pState, TransportPacket & rPack);
	int    Session_ServiceReply(void * pState, TransportPacket & rPack);
private:
	struct StoragePacket {
		StyloQSecTbl::Rec Rec;
		SSecretTagPool Pool;
	};
	int    ReadCurrentPacket(StoragePacket * pPack);
	int    PutPeerEntry(PPID * pID, StoragePacket * pPack, int use_ta);
	int    GetPeerEntry(PPID id, StoragePacket * pPack);
	int    GetOwnPeerEntry(StoragePacket * pPack);
	int    SearchGlobalIdentEntry(const void * pIdent, size_t identLen, StoragePacket * pPack);
	int    ExtractSessionFromPacket(const StoragePacket & rPack, SSecretTagPool & rSessCtx)
	{
		int    ok = 0;
		if(rPack.Rec.Kind == kSession) {
			SBinaryChunk public_key;
			SBinaryChunk private_key;
			SBinaryChunk sess_secret;
			if(rPack.Pool.Get(SSecretTagPool::tagSessionPrivateKey, &private_key)) {
				if(rPack.Pool.Get(SSecretTagPool::tagSessionPublicKey, &public_key)) {
					if(rPack.Pool.Get(SSecretTagPool::tagSessionSecret, &sess_secret)) {
						rSessCtx.Put(SSecretTagPool::tagSessionPrivateKey, private_key);
						rSessCtx.Put(SSecretTagPool::tagSessionPublicKey, public_key);
						rSessCtx.Put(SSecretTagPool::tagSessionSecret, sess_secret);
						ok = 1;
					}
				}
			}
		}
		return ok;
	}
	enum {
		gcisfMakeSecret = 0x0001
	};
	int    GetClientIdentForService(const SSecretTagPool & rOwnPool, const void * pSvcId, size_t svcIdLen, long flags, SSecretTagPool & rPool);
	StyloQSecTbl T;
};

class _RoundTripPool {
public:
	class Entry {
	public:
		Entry() : TimeToPoll(0), TimeToExpiration(0)
		{
		}
		virtual ~Entry()
		{
		}
		virtual int Poll(_RoundTripPool * pMaster)
		{
			int    ok = 0;
			if(pMaster) {
				void * p_c_ = pMaster->GetCarrier(&Tb);
				if(p_c_) {
					_RoundTripPool::Carrier	* p_carrier = (_RoundTripPool::Carrier *)p_c_;
					PPStyloQInterchange::TransportPacket * p_pack = p_carrier->Read(pMaster, Tb);
					if(p_pack) {
						
					}
				}
			}
			return ok;
		}
		virtual int Send(_RoundTripPool * pMaster)
		{
			int    ok = 0;
			if(pMaster) {
				void * p_c_ = pMaster->GetCarrier(&Tb);
				if(p_c_) {
					_RoundTripPool::Carrier	* p_carrier = (_RoundTripPool::Carrier *)p_c_;

				}
			}
			return ok;
		}
		bool   IsExpired(uint64 t) const
		{
			return (TimeToExpiration && t > TimeToExpiration);
		}
		struct TransportBlock {
			TransportBlock() : P_MqbP(0), P_IpA(0)
			{
			}
			TransportBlock(const TransportBlock & rS) : P_MqbP(0), P_IpA(0)
			{
				if(rS.P_MqbP)
					P_MqbP = new PPMqbClient::InitParam(*rS.P_MqbP);
				if(rS.P_IpA)
					P_IpA = new InetAddr(*rS.P_IpA);
			}
			~TransportBlock()
			{
				delete P_MqbP;
				delete P_IpA;
			}
			TransportBlock & FASTCALL operator = (const TransportBlock & rS)
			{
				ZDELETE(P_MqbP);
				ZDELETE(P_IpA);				
				if(rS.P_MqbP)
					P_MqbP = new PPMqbClient::InitParam(*rS.P_MqbP);
				if(rS.P_IpA)
					P_IpA = new InetAddr(*rS.P_IpA);
				return *this;
			}
			TransportBlock & Z()
			{
				ZDELETE(P_MqbP);
				ZDELETE(P_IpA);
				return *this;
			}
			int    FASTCALL IsEqual(const TransportBlock & rS) const 
			{
				int    eq = 1;
				if(P_MqbP)
					eq = rS.P_MqbP ? P_MqbP->IsEqualConnection(*rS.P_MqbP) : 0;
				else if(rS.P_MqbP)
					eq = 0;
				if(P_IpA)
					eq = rS.P_IpA ? P_IpA->IsEqual(*rS.P_IpA) : 0;
				else if(rS.P_IpA)
					eq = 0;
				return eq;
			}

			PPMqbClient::InitParam * P_MqbP;
			InetAddr * P_IpA;
		};
		uint64 TimeToPoll;
		S_GUID Id; // Идентификатор транзкции обмена
	protected:
		uint64 TimeToExpiration;
		TransportBlock Tb;
	};
	_RoundTripPool()
	{
	}
	~_RoundTripPool()
	{
	}
	void * GetCarrier(Entry::TransportBlock * pTb)
	{
		void * p_result = 0;
		if(pTb) {
			for(uint i = 0; i < CL.getCount(); i++) {
				Carrier * p_c = CL.at(i);
				Entry::TransportBlock & r_tb = p_c->GetTb();
				if(p_c && r_tb.IsEqual(*pTb)) {
					p_result = p_c;
					if(r_tb.P_MqbP) {
						for(uint j = 0; j < pTb->P_MqbP->ConsumeParamList.getCount(); j++) {
							const PPMqbClient::RoutingParamEntry * p_rpe = pTb->P_MqbP->ConsumeParamList.at(j);
							uint k = 0;
							if(p_rpe && !r_tb.P_MqbP->SearchRoutingEntry(*p_rpe, &k)) {
								PPMqbClient::RoutingParamEntry * p_new_rpe = r_tb.P_MqbP->ConsumeParamList.CreateNewItem();
								if(p_new_rpe) {
									*p_new_rpe = *p_rpe;
								}
							}
						}
					}
				}
			}
			if(!p_result) {
				uint   pos = 0;
				Carrier * p_new_c = CL.CreateNewItem(&pos);
				if(p_new_c) {
					if(p_new_c->Init(*pTb)) {
						p_result = p_new_c;
					}
					else {
						CL.atFree(pos);
					}
				}
			}
		}
		return p_result;		
	}
	void Loop()
	{
		for(uint i = 0; i < L.getCount(); i++) {
			Entry * p_entry = L.at(i);
			if(p_entry) {
				__time64_t t;
				_time64(&t);
				if(p_entry->IsExpired(t)) {
					// destroy entry
				}
				else if(!p_entry->TimeToPoll || t > p_entry->TimeToPoll) {
					int pr = p_entry->Poll(this);
					if(pr > 0) {
						
					}
				}
			}
		}
	}
private:
	class Carrier {
	public:
		Carrier() : P_Mqbc(0), P_Soc(0)
		{
		}
		~Carrier()
		{
			delete P_Mqbc;
			delete P_Soc;
		}
		int    Init(const Entry::TransportBlock & rTb)
		{
			int    ok = 0;
			Tb = rTb;
			if(Tb.P_MqbP) {
				P_Mqbc = new PPMqbClient();
				if(P_Mqbc) {
					THROW(P_Mqbc->Connect(Tb.P_MqbP->Host, NZOR(Tb.P_MqbP->Port, InetUrl::GetDefProtocolPort(InetUrl::protAMQP)/*5672*/)));
					THROW(P_Mqbc->Login(*Tb.P_MqbP));
					if(Tb.P_MqbP->ConsumeParamList.getCount()) {
						for(uint i = 0; i < Tb.P_MqbP->ConsumeParamList.getCount(); i++) {
							const PPMqbClient::RoutingParamEntry * p_entry = Tb.P_MqbP->ConsumeParamList.at(i);
							if(p_entry) {
								THROW(P_Mqbc->ApplyRoutingParamEntry(*p_entry));
							}
						}
					}
				}
			}
			else if(Tb.P_IpA) {
				// @todo
			}
			CATCH
				ZDELETE(P_Mqbc);
				ZDELETE(P_Soc);
			ENDCATCH
			return ok;
		}
		PPStyloQInterchange::TransportPacket * Read(_RoundTripPool * pMaster, Entry::TransportBlock & rTb)
		{
			PPStyloQInterchange::TransportPacket * p_result = 0;
			if(pMaster) {
				if(P_Mqbc) {
					//P_Mqbc->ConsumeMessage()
					SString consumer_tag;
					SString queue_name;
					SSerializeContext sctx;
					assert(rTb.P_MqbP);
					assert(rTb.P_MqbP->IsEqualConnection(*Tb.P_MqbP));
					THROW(rTb.P_MqbP);
					THROW(rTb.P_MqbP->IsEqualConnection(*Tb.P_MqbP));
					for(uint i = 0; i < rTb.P_MqbP->ConsumeParamList.getCount(); i++) {
						const PPMqbClient::RoutingParamEntry * p_rpe = rTb.P_MqbP->ConsumeParamList.at(i);
						if(p_rpe) {
							if(P_Mqbc->Consume(p_rpe->QueueName, &consumer_tag, 0)) {
								PPMqbClient::Envelope env;
								int cmr = P_Mqbc->ConsumeMessage(env, 10);							
								if(cmr > 0) {
									p_result = new PPStyloQInterchange::TransportPacket;
									if(p_result) {
										THROW(p_result->Serialize(-1, env.Msg.Body, &sctx));
									}
								}
							}
						}
					}
				}
				else if(P_Soc) {
				}
			}
			CATCH
				ZDELETE(p_result);
			ENDCATCH
			return p_result;
		}
		Entry::TransportBlock & GetTb() { return Tb; }
	private:
		Entry::TransportBlock Tb;
		PPMqbClient * P_Mqbc;
		TcpSocket * P_Soc;
	};
	TSCollection <Carrier> CL;
	TSCollection <Entry> L;
};

int PPStyloQInterchange::ReadCurrentPacket(StoragePacket * pPack)
{
	int    ok = -1;
	if(pPack) {
		SBuffer sbuf;
		SSerializeContext sctx;
		T.copyBufTo(&pPack->Rec);
		T.readLobData(T.VT, sbuf);
		T.destroyLobData(T.VT);
		THROW(pPack->Pool.Serialize(-1, sbuf, &sctx));
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::SearchGlobalIdentEntry(const void * pIdent, size_t identLen, StoragePacket * pPack)
{
	assert(identLen > 0 && identLen <= sizeof(StyloQSecTbl::Key1));
	int    ok = -1;
	StyloQSecTbl::Key1 k1;
	THROW(identLen > 0 && identLen <= sizeof(StyloQSecTbl::Key1));
	MEMSZERO(k1);
	memcpy(k1.BI, pIdent, identLen);
	if(T.search(1, &k1, spEq)) {
		THROW(ReadCurrentPacket(pPack));
		ok = 1;			
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::GetOwnPeerEntry(StoragePacket * pPack)
{
	int    ok = -1;
	StyloQSecTbl::Key2 k2;
	MEMSZERO(k2);
	k2.Kind = kNativeService;
	if(T.search(2, &k2, spGe) && T.data.Kind == kNativeService) {
		THROW(ReadCurrentPacket(pPack));
		ok = 1;		
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::GetPeerEntry(PPID id, StoragePacket * pPack)
{
	int    ok = -1;
	StyloQSecTbl::Key0 k0;
	k0.ID = id;
	if(T.search(0, &k0, spEq)) {
		THROW(ReadCurrentPacket(pPack));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::PutPeerEntry(PPID * pID, StoragePacket * pPack, int use_ta)
{
	int    ok = 1;
	const  PPID outer_id = pID ? *pID : 0;
	if(pPack) {
		assert(oneof4(pPack->Rec.Kind, kNativeService, kForeignService, kClient, kSession));
	}
	SBuffer cbuf;
	SSerializeContext sctx;
	bool   do_destroy_lob = false;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(outer_id) {
			StoragePacket preserve_pack;
			THROW(GetPeerEntry(outer_id, &preserve_pack) > 0);
			if(pPack) {
				pPack->Rec.ID = outer_id;
				T.copyBufFrom(&pPack->Rec);
				THROW_SL(pPack->Pool.Serialize(+1, cbuf, &sctx));
				THROW(T.writeLobData(T.VT, cbuf.GetBuf(0), cbuf.GetAvailableSize()));
				THROW_DB(T.rereadForUpdate(0, 0));
				THROW_DB(T.updateRec());
				do_destroy_lob = true;
			}
			else {
				THROW_DB(T.rereadForUpdate(0, 0));
				THROW_DB(T.deleteRec());
			}
		}
		else if(pPack) {
			if(pPack->Rec.Kind == kNativeService) {
				// В таблице может быть не более одной записи вида kNativeService
				THROW(GetOwnPeerEntry(0) < 0); // @error Попытка вставить вторую запись вида native-service
			}
			pPack->Rec.ID = 0;
			T.copyBufFrom(&pPack->Rec);
			THROW_SL(pPack->Pool.Serialize(+1, cbuf, &sctx));
			THROW(T.writeLobData(T.VT, cbuf.GetBuf(0), cbuf.GetAvailableSize()));
			THROW_DB(T.insertRec(0, pID));		
			do_destroy_lob = true;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	if(do_destroy_lob)
		T.destroyLobData(T.VT);
	return ok;
}

int PPStyloQInterchange::SetupPeerInstance(PPID * pID, int use_ta)
{
	int    ok = -1;
	PPID   id = 0;
	BN_CTX * p_bn_ctx = 0;
	BIGNUM * p_rn = 0;
	SString temp_buf;
	StoragePacket ex_pack;
	StoragePacket new_pack;
	StoragePacket * p_pack_to_export = 0;
	SBinaryChunk public_ident;
	if(GetOwnPeerEntry(&ex_pack) > 0) {
		; // Запись уже существует
		p_pack_to_export = &ex_pack;
	}
	else {
		//
		// Сгенерировать: 
		//   -- собственное большое случайное число (SSecretTagPool::tagPrimaryRN)
		//   -- GUID для дополнения SRN при генерации публичного идентификатора по идентификатору сервиса (SSecretTagPool::tagAG)
		//   -- Автономный фейковый идентификатор сервиса, для генерации собственного публичного идентификатора, не привязанного к сервису (SSecretTagPool::tagFPI)
		//
		const   int primary_rn_bits_width = 1024;
		uint8   temp[2048];
		p_bn_ctx = BN_CTX_new();
		p_rn = BN_new();
		{
			const size_t seed_size = 128;
			SLS.GetTLA().Rg.ObfuscateBuffer(temp, seed_size);
			RAND_seed(temp, seed_size);
		}
		int rn_len;
		do {
			BN_priv_rand(p_rn, primary_rn_bits_width, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);
			rn_len = BN_bn2bin(p_rn, temp);
			THROW(rn_len > 0);
		} while(rn_len < primary_rn_bits_width / 8);
		assert(rn_len == primary_rn_bits_width / 8);
		new_pack.Pool.Put(SSecretTagPool::tagPrimaryRN, temp, rn_len);
		//
		do {
			BN_priv_rand(p_rn, 160, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);
			rn_len = BN_bn2bin(p_rn, temp);
			THROW(rn_len > 0);
		} while(rn_len < 20);
		assert(rn_len == 20);
		new_pack.Pool.Put(SSecretTagPool::tagFPI, temp, rn_len);
		//
		{
			S_GUID ag;
			ag.Generate();
			new_pack.Pool.Put(SSecretTagPool::tagAG, &ag, sizeof(ag));
		}
		{
			public_ident.Z();
			THROW(GetClientIdentForService(new_pack.Pool, temp, rn_len, 0, new_pack.Pool));
			int  r = new_pack.Pool.Get(SSecretTagPool::tagPublicIdent, &public_ident);
			assert(r);
			assert(public_ident.Len() <= sizeof(new_pack.Rec.BI));
			memcpy(new_pack.Rec.BI, public_ident.PtrC(), public_ident.Len());
			new_pack.Rec.Kind = kNativeService;
		}
		THROW(PutPeerEntry(&id, &new_pack, 1));
		{
			SBinaryChunk c1;
			SBinaryChunk c2;
			const uint32 test_tag_list[] = { SSecretTagPool::tagPrimaryRN, SSecretTagPool::tagPublicIdent, SSecretTagPool::tagAG, SSecretTagPool::tagFPI };
			{
				StoragePacket test_pack;
				if(GetPeerEntry(id, &test_pack) > 0) {
					for(uint i = 0; i < SIZEOFARRAY(test_tag_list); i++) {
						uint32 _tag = test_tag_list[i];
						if(new_pack.Pool.Get(_tag, &c1) && test_pack.Pool.Get(_tag, &c2)) {
							assert(c1 == c2);
						}
						else {
							assert(0);
						}
					}
				}
				else {
					assert(0);
				}
			}
			{
				StoragePacket test_pack;		
				if(GetOwnPeerEntry(&test_pack) > 0) {
					for(uint i = 0; i < SIZEOFARRAY(test_tag_list); i++) {
						uint32 _tag = test_tag_list[i];
						if(new_pack.Pool.Get(_tag, &c1) && test_pack.Pool.Get(_tag, &c2)) {
							assert(c1 == c2);
						}
						else {
							assert(0);
						}
					}
				}
				else {
					assert(0);
				}
			}
		}
		p_pack_to_export = &new_pack;
		ok = 1;
	}
	if(p_pack_to_export) {
		PPGetFilePath(PPPATH_OUT, "styloq-instance.txt", temp_buf);
		SFile f_out(temp_buf, SFile::mWrite);
		public_ident.Z();
		p_pack_to_export->Pool.Get(SSecretTagPool::tagPublicIdent, &public_ident);
		public_ident.Mime64(temp_buf);
		f_out.WriteLine(temp_buf.CR());
	}
	CATCHZOK
	BN_free(p_rn);
	BN_CTX_free(p_bn_ctx);
	return ok;
}

int PPStyloQInterchange::GetClientIdentForService(const SSecretTagPool & rOwnPool, const void * pSvcId, size_t svcIdLen, long flags, SSecretTagPool & rPool)
{
	int    ok = 1;
	SBinaryChunk prmrn;
	SBinaryChunk ag;
	THROW(rOwnPool.Get(SSecretTagPool::tagPrimaryRN, &prmrn));
	THROW(rOwnPool.Get(SSecretTagPool::tagAG, &ag));
	prmrn.Cat(pSvcId, svcIdLen);
	prmrn.Cat(ag.PtrC(), ag.Len());
	{
		binary160 hash = SlHash::Sha1(0, prmrn.PtrC(), prmrn.Len());
		assert(sizeof(hash) == 20);
		rPool.Put(SSecretTagPool::tagPublicIdent, &hash, sizeof(hash));
		if(flags & gcisfMakeSecret) {
			SBinaryChunk secret;
			secret.Cat(ag.PtrC(), ag.Len());
			secret.Cat(pSvcId, svcIdLen);
			binary160 secret_hash = SlHash::Sha1(0, secret.PtrC(), secret.Len());
			assert(sizeof(secret_hash) == 20);
			rPool.Put(SSecretTagPool::tagSecret, &secret_hash, sizeof(secret_hash));
		}
	}
	CATCHZOK
	return ok;
}
//
// Registration {
//
struct PPStyloQInterchange_RegistrationContext {
};

class PPStyloQRoundTripState {
public:
	PPStyloQRoundTripState() : Phase(phaseUndef), State(0), P_Usr(0), P_Vrf(0), Expiry(ZERODATETIME)
	{
	}
	~PPStyloQRoundTripState()
	{
		delete P_Usr;
		delete P_Vrf;
	}
	enum {
		phaseUndef = 0,         // Неопределенная фаза 
		phaseClientSrpInit,     // Клиент послал сервису (ident, A) 
		phaseClientSrpM,        // Клиент послал сервису (M) or ERROR
		phaseClientSrpAck,      // Клиент послал сервису ACK or ERROR
		phaseSvcSrpChallenge,   // Сервис послал клиенту (salt, B) or ERROR
		phaseSvcSrpHamc,        // Сервис послал клиенту (HAMC) or ERROR
		// (phaseClientSrpInit) > (phaseSvcSrpChallenge) > (phaseClientSrpM) > (phaseSvcSrpHamc) > (phaseClientSrpAck)
	};
	enum {
		stError = 0x0001 // От контрагента получено сообщение об ошибке
	};
	int    Phase;
	uint   State;
	LDATETIME Expiry; // Момент времени, после которого ожидание ответа считается бесперспективным и экземпляр уничтожается //
	SlSRP::User * P_Usr;     // SRP Клиентская часть 
	SlSRP::Verifier * P_Vrf; // SRP Серверная часть
};

int PPStyloQInterchange::Registration_ClientInit(void * pState, const void * pSvcIdent, size_t svcIdentLen, TransportPacket & rPack)
{
	int    ok = 0;
	PPStyloQInterchange_RegistrationContext * p_ctx = 0;
	SSecretTagPool tp;
	SString temp_buf;
	SBinaryChunk cli_ident;
	SBinaryChunk cli_secret;
	StoragePacket own_peer;
	THROW(GetOwnPeerEntry(&own_peer) > 0);
	if(GetClientIdentForService(own_peer.Pool, pSvcIdent, svcIdentLen, gcisfMakeSecret, tp)) {
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

int PPStyloQInterchange::Registration_ClientSendVerification(void * pState, const void * pSvcIdent, size_t svcIdentLen, TransportPacket & rPack)
{
	int    ok = 0;
	SString temp_buf;
	StoragePacket svc_pack;
	if(SearchGlobalIdentEntry(pSvcIdent, svcIdentLen, &svc_pack) > 0) {
		SBinaryChunk cli_ident;
		SBinaryChunk cli_secret;		
		THROW(svc_pack.Pool.Get(SSecretTagPool::tagPublicIdent, &cli_ident));
		THROW(svc_pack.Pool.Get(SSecretTagPool::tagSecret, &cli_secret));
		{
			SBinaryChunk __a; // A
			char * p_auth_ident = 0;
			temp_buf.EncodeMime64(cli_ident.PtrC(), cli_ident.Len());
			SlSRP::User usr(SlSRP::SRP_SHA1, SlSRP::SRP_NG_8192, temp_buf, cli_secret.PtrC(), cli_secret.Len(), /*n_hex*/0, /*g_hex*/0);
			usr.StartAuthentication(&p_auth_ident, __a);
			// User -> Host: (ident, __a) 
		}
	}
	CATCHZOK
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

//

static void _EcdhCryptModelling()
{
	const int ec_curve_name_id = NID_X9_62_prime256v1;
	//#define NISTP256 NID_X9_62_prime256v1
	//#define NISTP384 NID_secp384r1
	//#define NISTP521 NID_secp521r1

	int    ok = 1;
	BN_CTX * p_bn_ctx = BN_CTX_new();
	EC_KEY * p_key_cli = 0;
	EC_KEY * p_key_svc = 0;
	uchar * p_secret_cli = 0;
	uchar * p_secret_svc = 0;
	const EC_POINT * p_public_cli = 0;
	const EC_POINT * p_public_svc = 0;
	SBinaryChunk public_key_cli;
	SBinaryChunk public_key_bn_cli;
	SBinaryChunk public_key_svc;
	SBinaryChunk public_key_bn_svc;
	size_t secret_len_cli = 0;
	size_t secret_len_svc = 0;
	{
		THROW(p_key_cli = EC_KEY_new_by_curve_name(ec_curve_name_id)); // Failed to create key curve
		THROW(EC_KEY_generate_key(p_key_cli) == 1); // Failed to generate key
	}	
	{
		THROW(p_key_svc = EC_KEY_new_by_curve_name(ec_curve_name_id)); // Failed to create key curve
		THROW(EC_KEY_generate_key(p_key_svc) == 1); // Failed to generate key
	}
	//
	{
		const EC_GROUP * p_ecg = EC_KEY_get0_group(p_key_cli);
		p_public_cli = EC_KEY_get0_public_key(p_key_cli);
		size_t octlen = EC_POINT_point2oct(p_ecg, p_public_cli, POINT_CONVERSION_UNCOMPRESSED, NULL, 0, p_bn_ctx);
		THROW(public_key_cli.Ensure(octlen));
		EC_POINT_point2oct(p_ecg, p_public_cli, POINT_CONVERSION_UNCOMPRESSED, static_cast<uchar *>(public_key_cli.Ptr()), public_key_cli.Len(), p_bn_ctx);
	}
	{
		const EC_GROUP * p_ecg = EC_KEY_get0_group(p_key_svc);
		p_public_svc = EC_KEY_get0_public_key(p_key_svc);
		size_t octlen = EC_POINT_point2oct(p_ecg, p_public_svc, POINT_CONVERSION_UNCOMPRESSED, NULL, 0, p_bn_ctx);
		THROW(public_key_svc.Ensure(octlen));
		EC_POINT_point2oct(p_ecg, p_public_svc, POINT_CONVERSION_UNCOMPRESSED, static_cast<uchar *>(public_key_svc.Ptr()), public_key_svc.Len(), p_bn_ctx);
	}
	//unsigned char * get_secret(EC_KEY *key, const EC_POINT *peer_pub_key, size_t *secret_len)
	{
		EC_GROUP * p_ecg2 = EC_GROUP_new_by_curve_name(ec_curve_name_id);
		EC_POINT * p_public_svc2 = EC_POINT_new(p_ecg2);
		EC_POINT_oct2point(p_ecg2, p_public_svc2, static_cast<uchar *>(public_key_svc.Ptr()), public_key_svc.Len(), p_bn_ctx);
		const int field_size = EC_GROUP_get_degree(EC_KEY_get0_group(p_key_cli));
		secret_len_cli = (field_size + 7) / 8;
		THROW(p_secret_cli = (uchar *)OPENSSL_malloc(secret_len_cli)); // Failed to allocate memory for secret
		secret_len_cli = ECDH_compute_key(p_secret_cli, secret_len_cli, p_public_svc2, p_key_cli, NULL);
		THROW(secret_len_cli > 0);
		EC_POINT_free(p_public_svc2);
		EC_GROUP_free(p_ecg2);
	}
	{
		EC_GROUP * p_ecg2 = EC_GROUP_new_by_curve_name(ec_curve_name_id);
		EC_POINT * p_public_cli2 = EC_POINT_new(p_ecg2);
		EC_POINT_oct2point(p_ecg2, p_public_cli2, static_cast<uchar *>(public_key_cli.Ptr()), public_key_cli.Len(), p_bn_ctx);
		const int field_size = EC_GROUP_get_degree(EC_KEY_get0_group(p_key_svc));
		secret_len_svc = (field_size + 7) / 8;
		THROW(p_secret_svc = (uchar *)OPENSSL_malloc(secret_len_svc)); // Failed to allocate memory for secret
		secret_len_svc = ECDH_compute_key(p_secret_svc, secret_len_svc, p_public_cli2, p_key_svc, NULL);
		THROW(secret_len_svc > 0);
		EC_POINT_free(p_public_cli2);
		EC_GROUP_free(p_ecg2);
	}
	{
		assert(secret_len_cli == secret_len_svc);
		for(size_t i = 0; i < secret_len_cli; i++)
			assert(p_secret_cli[i] == p_secret_svc[i]);
	}
	CATCHZOK
	EC_KEY_free(p_key_cli);
	EC_KEY_free(p_key_svc);
	OPENSSL_free(p_secret_cli);
	OPENSSL_free(p_secret_svc);
	BN_CTX_free(p_bn_ctx);
}

static int __KeyGenerationEc()
{
	int    ok = 1;
	uint8  prv_key[256];
	uint8  pub_key[256];
	size_t prv_key_size = 0;
	size_t pub_key_size = 0;
	BIO * outbio = NULL;
	EVP_PKEY * pkey   = NULL;
	_EcdhCryptModelling();
	{
	}
	// 
	// These function calls initialize openssl for correct work.
	// 
	OpenSSL_add_all_algorithms();
	ERR_load_BIO_strings();
	ERR_load_crypto_strings();
	// 
	// Create the Input/Output BIO's.
	// 
	outbio = BIO_new(BIO_s_file());
	outbio = BIO_new_fp(stdout, BIO_NOCLOSE);
	// 
	// Create a EC key sructure, setting the group type from NID
	// 
	int    eccgrp = OBJ_txt2nid("secp521r1");
	EC_KEY * p_ecc = EC_KEY_new_by_curve_name(eccgrp);
	// 
	// For cert signing, we use  the OPENSSL_EC_NAMED_CURVE flag*
	// 
	EC_KEY_set_asn1_flag(p_ecc, OPENSSL_EC_NAMED_CURVE);
	// 
	// Create the public/private EC key pair here
	// 
	THROW(EC_KEY_generate_key(p_ecc)); // Error generating the ECC key
	// 
	// Converting the EC key into a PKEY structure let us
	// handle the key just like any other key pair.
	// 
	pkey = EVP_PKEY_new();
	THROW(EVP_PKEY_assign_EC_KEY(pkey, p_ecc)); // Error assigning ECC key to EVP_PKEY structure
	//int grprkr = EVP_PKEY_get_raw_private_key(/*pkey*/p_ecc, prv_key, &prv_key_size);
	//int grpukr = EVP_PKEY_get_raw_public_key(pkey, pub_key, &pub_key_size);
	{
		// 
		// Now we show how to extract EC-specifics from the key
		// 
		p_ecc = EVP_PKEY_get1_EC_KEY(pkey);
		const EC_GROUP * ecgrp = EC_KEY_get0_group(p_ecc);
		// 
		// Here we print the key length, and extract the curve type.
		// 
		BIO_printf(outbio, "ECC Key size: %d bit\n", EVP_PKEY_bits(pkey));
		BIO_printf(outbio, "ECC Key type: %s\n", OBJ_nid2sn(EC_GROUP_get_curve_name(ecgrp)));
	}
	//
	// Here we print the private/public key data in PEM format
	// 
	THROW(PEM_write_bio_PrivateKey(outbio, pkey, NULL, NULL, 0, 0, NULL)); // Error writing private key data in PEM format
	THROW(PEM_write_bio_PUBKEY(outbio, pkey)); // Error writing public key data in PEM format
	CATCHZOK
	EVP_PKEY_free(pkey);
	EC_KEY_free(p_ecc);
	BIO_free_all(outbio);
	return ok;
}
//

int Test_PPStyloQInterchange_Invitation()
{
	int    ok = 1;
	SString temp_buf;
	PPStyloQInterchange ic;
	//
	PPID   own_peer_id = 0;
	int    spir = ic.SetupPeerInstance(&own_peer_id, 1);
	{
		const char * p_svc_ident_mime = "gBX7vDvf5z6VZ74kdxhDl1WGzEY=";
		SBinaryChunk svc_ident;
		if(svc_ident.FromMime64(p_svc_ident_mime)) {
			SSecretTagPool svc_pool;
			SSecretTagPool sess_pool;
			svc_pool.Put(SSecretTagPool::tagSvcIdent, svc_ident);
			const char * p_accsp = "amqp://213.166.69.241";
			svc_pool.Put(SSecretTagPool::tagSvcAccessPoint, p_accsp, strlen(p_accsp)+1);
			ic.KexClientInit(sess_pool, svc_pool);
		}
	}
	__KeyGenerationEc();
	//
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

