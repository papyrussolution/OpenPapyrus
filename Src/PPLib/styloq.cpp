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

const int ec_curve_name_id = NID_X9_62_prime256v1;
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
			opError = 1,      // Ошибка (ответ)
			opHandshake,      // Обмен ключами для установки соединения //
			opHandshakeReply, // Ответ на инициативу обмена ключами для установки соединения //
			opSession,        // Установка соединения по идентификатору сессии
			opSrpAuth,        // SRP-авторизация //
		};
		enum {
			pfPlain = 1,
			pfZ,
		};
		TransportPacket() : Prefix(0), Op(opUndef)
		{
		}
		TransportPacket & Z()
		{
			Prefix = 0;
			Op = 0;
			P.Z();
			Data.Z();
			return *this;
		}
		int    IsValid() const
		{
			return (oneof2(Prefix, pfPlain, pfZ) && oneof5(Op, opError, opHandshake, opHandshakeReply, opSession, opSrpAuth));
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
	static int RunStyloQServer();
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
	int    KexGenerageSecret(SSecretTagPool & rSessCtx, const SSecretTagPool & rOtherCtx)
	{
		int    ok = 1;
		SBinaryChunk other_public;
		SBinaryChunk my_public;
		SBinaryChunk my_private;
		BN_CTX * p_bn_ctx = 0;
		EC_POINT * p_public_other = 0;
		EC_KEY * p_private_my = 0;
		BIGNUM * p_private_bn = 0;//EC_KEY_get0_private_key(p_key);
		void * p_secret = 0;
		THROW(rOtherCtx.Get(SSecretTagPool::tagSessionPublicKey, &other_public));
		THROW(rSessCtx.Get(SSecretTagPool::tagSessionPublicKey, &my_public));
		THROW(rSessCtx.Get(SSecretTagPool::tagSessionPrivateKey, &my_private));
		assert(other_public.Len() && my_public.Len() && my_private.Len());
		{
			p_bn_ctx = BN_CTX_new();
			p_private_my = EC_KEY_new_by_curve_name(ec_curve_name_id);
			const EC_GROUP * p_ecg2 = EC_KEY_get0_group(p_private_my);
			p_public_other = EC_POINT_new(p_ecg2);
			p_private_bn = BN_new();//EC_KEY_get0_private_key(p_key);
			EC_POINT_oct2point(p_ecg2, p_public_other, static_cast<uchar *>(other_public.Ptr()), other_public.Len(), p_bn_ctx);
			BN_bin2bn(static_cast<const uchar *>(my_private.PtrC()), my_private.Len(), p_private_bn);
			EC_KEY_set_private_key(p_private_my, p_private_bn);
			const int field_size = EC_GROUP_get_degree(EC_KEY_get0_group(p_private_my));
			size_t secret_len = (field_size + 7) / 8;
			THROW(p_secret = (uchar *)OPENSSL_malloc(secret_len)); // Failed to allocate memory for secret
			secret_len = ECDH_compute_key(p_secret, secret_len, p_public_other, p_private_my, NULL);
			THROW(secret_len > 0);
			rSessCtx.Put(SSecretTagPool::tagSessionSecret, p_secret, secret_len);
		}
		CATCHZOK
		BN_free(p_private_bn);
		EC_POINT_free(p_public_other);
		EC_KEY_free(p_private_my);
		BN_CTX_free(p_bn_ctx);
		OPENSSL_free(p_secret);
		return ok;
	}
	int    KexServiceReply(SSecretTagPool & rSessCtx, SSecretTagPool & rCli)
	{
		int    ok = 1;
		SBinaryChunk cli_acsp;
		SBinaryChunk cli_ident;
		rCli.Get(SSecretTagPool::tagSvcAccessPoint, &cli_acsp);
		THROW(rCli.Get(SSecretTagPool::tagClientIdent, &cli_ident));
		int  fsks = FetchSessionKeys(rSessCtx, cli_ident.PtrC(), cli_ident.Len());
		if(fsks == fsksNewSession) {
			SBinaryChunk my_public;
			rSessCtx.Get(SSecretTagPool::tagSessionPublicKey, &my_public);
			THROW(my_public.Len());
			THROW(KexGenerageSecret(rSessCtx, rCli));
		}
		CATCHZOK
		return ok;
	}
	int    KexClientInit(SSecretTagPool & rSessCtx, const SSecretTagPool & rSvc)
	{
		int    ok = -1;
		SString temp_buf;
		SBinaryChunk svc_acsp;
		SBinaryChunk svc_ident;
		SBinaryChunk sess_pub_key;
		SBuffer transport_buffer;
		SSerializeContext sctx;
		PPMqbClient * p_mqbc = 0;
		THROW(rSvc.Get(SSecretTagPool::tagSvcAccessPoint, &svc_acsp));
		THROW(rSvc.Get(SSecretTagPool::tagSvcIdent, &svc_ident));
		{
			temp_buf.Z().CatN(static_cast<const char *>(svc_acsp.PtrC()), svc_acsp.Len());
			InetUrl url(temp_buf);
			SString host;
			THROW(url.GetComponent(InetUrl::cHost, 0, host));
			if(oneof2(url.GetProtocol(), InetUrl::protAMQP, InetUrl::protAMQPS)) {
				//PPMqbClient::LoginParam mqblp;
				PPMqbClient::InitParam mqip;
				THROW(PPMqbClient::SetupInitParam(mqip, 0));
				//mqip.Host = host;
				//mqip.Port = url.GetPort();
				//mqip.Auth = "styloq-user";
				//mqip.Secret = "styloq-def-secret";
				mqip.VHost = "styloq";
				//THROW(p_mqbc->Connect(host, NZOR(url.GetPort(), InetUrl::GetDefProtocolPort(InetUrl::protAMQP)/*5672*/)));
				//THROW(p_mqbc->Login(mqblp));
				{
					StoragePacket own_pack;
					SBinaryChunk own_ident;
					int  fsks = FetchSessionKeys(rSessCtx, svc_ident.PtrC(), svc_ident.Len());
					THROW(GetOwnPeerEntry(&own_pack) > 0);
					THROW(own_pack.Pool.Get(SSecretTagPool::tagPublicIdent, &own_ident));
					if(fsks == fsksNewSession) {
						TransportPacket tp;
						PPMqbClient::RoutingParamEntry * p_rpe = 0;
						PPMqbClient::MessageProperties props;
						THROW(rSessCtx.Get(SSecretTagPool::tagSessionPublicKey, &sess_pub_key));
						tp.Op = TransportPacket::opHandshake;
						tp.Prefix = TransportPacket::pfPlain;
						tp.P.Put(SSecretTagPool::tagClientIdent, own_ident);
						tp.P.Put(SSecretTagPool::tagSessionPublicKey, sess_pub_key);
						p_rpe = mqip.ConsumeParamList.CreateNewItem();
						if(p_rpe && p_rpe->SetupStyloQRpc(sess_pub_key, svc_ident)) {
							THROW(p_mqbc = PPMqbClient::CreateInstance(mqip));
							transport_buffer.Z();
							THROW(tp.Serialize(+1, transport_buffer, &sctx));
							THROW(p_mqbc->Publish(p_rpe->ExchangeName, p_rpe->RoutingKey, &props, transport_buffer.constptr(), transport_buffer.GetAvailableSize()));
							{
								SString consume_tag;
								PPMqbClient::Envelope env;
								THROW(p_mqbc->Consume(p_rpe->RpcReplyQueueName, &consume_tag, 0));
								const clock_t _c = clock();
								bool  trigger = false;
								do {
									if(p_mqbc->ConsumeMessage(env, 0) > 0) {
										trigger = true;
										ok = 1;
										tp.Z();
										if(tp.Serialize(-1, env.Msg.Body, &sctx)) {
											if(tp.Op == TransportPacket::opError) {
												;
											}
											else if(tp.Op == TransportPacket::opHandshakeReply) {
												THROW(KexGenerageSecret(rSessCtx, tp.P));
											}
										}
									}
									else
										SDelay(10);
								} while(!trigger && (clock() - _c) < 60000);
							}
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
	//uint8  prv_key[256];
	//uint8  pub_key[256];
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
	PPIniFile ini_file;
	PPID   own_peer_id = 0;
	int    spir = ic.SetupPeerInstance(&own_peer_id, 1);
	temp_buf.Z();
	ini_file.Get(PPINISECT_CONFIG, "styloqtestside", temp_buf);
	if(temp_buf.IsEqiAscii("server")) {
		PPStyloQInterchange::RunStyloQServer();
	}
	else if(temp_buf.IsEqiAscii("client")) {
		const char * p_svc_ident_mime = "BPYrrHS3xd1GEiaI98RKUIYWq5k="; // pft
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
#if 1 // {
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
#endif // } 0
	return ok;
}

class StyloQServerSession : public PPThread {
public:
	StyloQServerSession(const DbLoginBlock & rLB, const PPMqbClient::InitParam & rMqbParam, const PPStyloQInterchange::TransportPacket & rTPack) : 
		PPThread(kStyloQSession, 0, 0), LB(rLB), StartUp_MqbParam(rMqbParam), TPack(rTPack)
	{
		StartUp_MqbParam.ConsumeParamList.freeAll();
	}
	virtual void Run()
	{
		PPMqbClient * p_mqbc = 0;
		bool do_process_loop = false;
		SSerializeContext sctx;
		SBuffer transport_buffer;
		switch(TPack.Op) {
			case PPStyloQInterchange::TransportPacket::opHandshake:
				{
					SBinaryChunk pk;
					SBinaryChunk ci;
					TPack.P.Get(SSecretTagPool::tagSessionPublicKey, &pk);
					TPack.P.Get(SSecretTagPool::tagClientIdent, &ci);
					if(pk.Len() && ci.Len()) {
						if(Login()) {
							PPStyloQInterchange ic;
							if(ic.KexServiceReply(Session, TPack.P)) {
								SBinaryChunk my_public;
								PPStyloQInterchange::TransportPacket reply_tp;
								if(Session.Get(SSecretTagPool::tagSessionPublicKey, &my_public)) {
									assert(my_public.Len());
									reply_tp.P.Put(SSecretTagPool::tagSessionPublicKey, my_public);
									reply_tp.Op = PPStyloQInterchange::TransportPacket::opHandshakeReply;
									reply_tp.Prefix = PPStyloQInterchange::TransportPacket::pfPlain;
									//
									PPMqbClient::MessageProperties props;
									assert(StartUp_MqbParam.ConsumeParamList.getCount() == 0);
									PPMqbClient::RoutingParamEntry * p_rpe = StartUp_MqbParam.ConsumeParamList.CreateNewItem();
									if(p_rpe && p_rpe->SetupStyloQRpc(my_public, pk)) {
										p_rpe->PreprocessFlags |= PPMqbClient::RoutingParamEntry::ppfSkipQueueDeclaration;
										if(reply_tp.Serialize(+1, transport_buffer, &sctx)) {
											p_mqbc = PPMqbClient::CreateInstance(StartUp_MqbParam);
											if(p_mqbc) {
												int pr = p_mqbc->Publish(p_rpe->ExchangeName, p_rpe->RoutingKey, &props, transport_buffer.constptr(), transport_buffer.GetAvailableSize());
												//SString consume_tag;
												if(pr/* && p_mqbc->Consume(p_rpe->RpcReplyQueueName, &consume_tag, 0)*/) {
													do_process_loop = true;
													/*PPMqbClient::Envelope env;
													if(p_mqbc->ConsumeMessage(env, 1000) > 0) {
									
													}*/
												}
											}
										}

									}
								}
							}
						}
					}
				}
				break;
			case PPStyloQInterchange::TransportPacket::opSession:
				{
				}
				break;
			case PPStyloQInterchange::TransportPacket::opSrpAuth:
				break;
		}
		delete p_mqbc;
	}
private:
	int    Login()
	{
		int    ok = 1;
		char   secret[64];
		SString db_symb;
		if(LB.GetAttr(DbLoginBlock::attrDbSymb, db_symb)) {
			PPVersionInfo vi = DS.GetVersionInfo();
			THROW(vi.GetSecret(secret, sizeof(secret)));
			THROW(DS.Login(db_symb, PPSession::P_JobLogin, secret, PPSession::loginfSkipLicChecking));
			memzero(secret, sizeof(secret));
		}
		CATCHZOK
		return ok;
	}
	void   Logout()
	{
		DS.Logout();
	}
	DbLoginBlock LB;
	PPMqbClient::InitParam StartUp_MqbParam;
	PPStyloQInterchange::TransportPacket TPack;
	SSecretTagPool Session;
};

class StyloQServer : public PPThread {
	PPMqbClient::InitParam StartUp_MqbParam; 
	DbLoginBlock LB;
public:
	StyloQServer(const DbLoginBlock & rLB, const PPMqbClient::InitParam & rMqbParam) : PPThread(kStyloQServer, 0, 0), StartUp_MqbParam(rMqbParam), LB(rLB)
	{
	}
	virtual void Run()
	{
		const int   do_debug_log = 0; // @debug
		const long  pollperiod_mqc = 500;
		EvPollTiming pt_mqc(pollperiod_mqc, false);
		EvPollTiming pt_purge(3600000, true); // этот тайминг не надо исполнять при запуске. Потому registerImmediate = 1
		const int  use_sj_scan_alg2 = 0;
		SString msg_buf, temp_buf;
		PPMqbClient * p_mqb_cli = PPMqbClient::CreateInstance(StartUp_MqbParam); // @v11.0.9
		if(p_mqb_cli) {
			PPMqbClient::Envelope mqb_envelop;
			SSerializeContext sctx;
			const long __cycle_hs = (p_mqb_cli ? 37 : 293); // Период таймера в сотых долях секунды (37)
			int    queue_stat_flags_inited = 0;
			PPStyloQInterchange::TransportPacket tpack;
			Evnt   stop_event(SLS.GetStopEventName(temp_buf), Evnt::modeOpen);
			for(int stop = 0; !stop;) {
				uint   h_count = 0;
				HANDLE h_list[32];
				h_list[h_count++] = stop_event;
				//
				STimer __timer;  // Таймер для отмера времени до следующего опроса источников событий
				__timer.Set(getcurdatetime_().addhs(__cycle_hs), 0);
				h_list[h_count++] = __timer;
				uint   r = ::WaitForMultipleObjects(h_count, h_list, 0, /*CycleMs*//*INFINITE*/60000);
				switch(r) {
					case (WAIT_OBJECT_0 + 0): // stop event
						if(do_debug_log) {
							PPLogMessage(PPFILNAM_DEBUG_AEQ_LOG, "StopEvent", LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
						}
						stop = 1; // quit loop
						break;
					case WAIT_TIMEOUT:
						// Если по каким-то причинам сработал таймаут, то перезаряжаем цикл по-новой
						// Предполагается, что это событие крайне маловероятно!
						if(do_debug_log) {
							PPLogMessage(PPFILNAM_DEBUG_AEQ_LOG, "TimeOut", LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
						}
						break;
					case (WAIT_OBJECT_0 + 1): // __timer event
						{
							if(pt_purge.IsTime()) {
								;
							}
							if(p_mqb_cli && pt_mqc.IsTime()) {
								while(p_mqb_cli->ConsumeMessage(mqb_envelop, 200) > 0) {
									p_mqb_cli->Ack(mqb_envelop.DeliveryTag, 0);
									tpack.Z();
									if(tpack.Serialize(-1, mqb_envelop.Msg.Body, &sctx)) {
										StyloQServerSession * p_new_sess = new StyloQServerSession(LB, StartUp_MqbParam, tpack);
										if(p_new_sess)
											p_new_sess->Start(0);
									}
								}
							}
						}
						break;
					case WAIT_FAILED:
						if(do_debug_log) {
							PPLogMessage(PPFILNAM_DEBUG_AEQ_LOG, "QueueFailed", LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
						}
						// error
						break;
				}
			}
		}
		delete p_mqb_cli;
		DBS.CloseDictionary();
	}
};

/*static*/int PPStyloQInterchange::RunStyloQServer()
{
	int    ok = 1;
	StyloQServer * p_srv = 0;
	PPID   own_peer_id = 0;
	DbProvider * p_dict = CurDict;
	if(p_dict) {
		SString temp_buf;
		PPStyloQInterchange ic;
		PPStyloQInterchange::StoragePacket sp;
		THROW(ic.SetupPeerInstance(&own_peer_id, 1));
		THROW(ic.GetOwnPeerEntry(&sp) > 0);
		{
			SBinaryChunk _ident;
			SString public_ident_text;
			//SSecretTagPool svc_pool;
			SSecretTagPool sess_pool;
			sp.Pool.Get(SSecretTagPool::tagPublicIdent, &_ident);
			_ident.Mime64(public_ident_text);
			PPMqbClient::InitParam mqb_init_param;
			THROW(PPMqbClient::SetupInitParam(mqb_init_param, 0));
			mqb_init_param.VHost = "styloq";
			{
				PPMqbClient::RoutingParamEntry rpe;
				if(rpe.SetupStyloQRpcListener(_ident)) {
					PPMqbClient::RoutingParamEntry * p_new_entry = mqb_init_param.ConsumeParamList.CreateNewItem();
					ASSIGN_PTR(p_new_entry, rpe);
				}
			}
			{
				DbLoginBlock dlb;
				PPIniFile ini_file;
				PPDbEntrySet2 dbes;
				int    db_id = 0;
				p_dict->GetDbSymb(temp_buf);
				THROW(ini_file.IsValid());
				THROW(dbes.ReadFromProfile(&ini_file, 0));
				THROW_SL(db_id = dbes.GetBySymb(temp_buf, &dlb));
				{
					p_srv = new StyloQServer(dlb, mqb_init_param);
					if(p_srv) {
						p_srv->Start(0);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
