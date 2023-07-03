// STYLOQ.CPP
// Copyright (c) A.Sobolev 2021, 2022, 2023
// @codepage UTF-8
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
	additional nginx config params:
	http {
		client_max_body_size  8m;
		client_body_buffer_size  4m;
	}
*/
const int ec_curve_name_id = NID_X9_62_prime256v1;
const long __DefMqbConsumeTimeout = 5000; // @v11.4.3 5000-->10000 // @v11.4.5 10000-->5000
/*
	RESERVED COMMANDS:
		test
		//
		register
		quit
		advert
		getconfig
		getface
		setface - передается от клиента сервису для обновления собственного лика
		getcommandlist
		pushindexingcontent
		getforeignconfig
		echo
		search
		requestblobinfolist
		storeblob
		getblob
		dtlogin
		postdocument
		requestdocumentstatuslist // Запрос статусов документов
		onsrchr // Отправляется сервису при выборе пользователем результата глобального поиска

	DISPLAY METHODS (displaymethod)
		attendanceprereq
		orderprereq
		grid
		search
		//
		detailsvc         // детальная информация о сервисе
		detailware        // детальная информация о товаре
		detailprc         // детальная информация о процессоре
		detailgoodsgroup  // детальная информация о товарной группе
		detailbrand       // детальная информация о бренде
		detailcustomer    // детальная информация о клиенте
		detaildlvrloc     // детальная информация об адресе доставки
*/
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
	//
	//
	//
	Статусы документов заказа клиент-->сервис

	DRAFT              драфт
	WAITFORAPPROREXEC  ждет одобрения или исполнения от сервиса
	APPROVED           одобрен сервисом
	CORRECTED          скорректирован сервисом (от сервиса поступает корректирующий документ, который привязывается к оригиналу)
	CORRECTIONACCEPTED корректировка сервиса принята клиентом
	CORRECTIONREJECTED корректировка сервиса отклонена клиентом (документ полностью отменяется и цикл документа завершается)
	REJECTED           отклонен сервисом (цикл документа завершается)
	MODIFIED           изменен клиентом (от клиента поступает измененная версия документа, которая привязывается к оригиналу)
	CANCELLED          отменен клиентом (цикл документа завершается)
	EXECUTED           исполнен сервисом
	EXECUTIONACCEPTED  подтверждение от клиента исполнения документа сервисом (цикл документа завершается)
	EXECUTIONCORRECTED корректировка от клиента исполнения документа сервисом (от клиента поступает документ согласования)
	EXECORRECTIONACCEPTED согласие сервиса с документом согласования клиента
	EXECORRECTIONREJECTED отказ сервиса от документа согласования клиента - тупиковая ситуация, которая должна быть
				разрешена посредством дополнительных механизмов (escrow счета, полный возврат с отменой платежей и т.д.)
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
static const SIntToSymbTabEntry StyloQConfigTagNameList[] = {
	{ StyloQConfig::tagUrl,             "url" },
	{ StyloQConfig::tagMqbAuth,         "mqbauth" },
	{ StyloQConfig::tagMqbSecret,       "mqbsecret" },
	{ StyloQConfig::tagLoclUrl,         "loclurl" },
	{ StyloQConfig::tagLoclMqbAuth,     "loclmqbauth" },
	{ StyloQConfig::tagLoclMqbSecret,   "loclmqbsecret" }, 
	{ StyloQConfig::tagFeatures,        "features" }, 
	{ StyloQConfig::tagExpiryPeriodSec, "expiryperiodsec" },
	{ StyloQConfig::tagExpiryEpochSec,  "expiryepochsec" },
	{ StyloQConfig::tagPrefLanguage,    "preflang" }, // @v11.2.5 (private config) Предпочтительный язык
	{ StyloQConfig::tagDefFace,         "defface"  }, // @v11.2.5 (private config) Лик, используемый клиентом по умолчанию
	{ StyloQConfig::tagRole,            "role"  }, // @v11.2.8 
	{ StyloQConfig::tagCliFlags,        "cliflags" }, // @v11.6.0
	{ StyloQConfig::tagNotificationActualDays, "notificationactualdays" }, // @v11.7.4
};

/*static*/SJson * StyloQConfig::MakeTransmissionJson_(const char * pSrcJson, const void * pClientPacket)
{
	SJson * p_result = 0;
	const StyloQCore::StoragePacket * p_cli_pack = static_cast<const StyloQCore::StoragePacket *>(pClientPacket); // @v11.6.0
	SString temp_buf(pSrcJson);
	THROW(temp_buf.NotEmptyS());
	{
		StyloQConfig cfg_pack;
		THROW(cfg_pack.FromJson(temp_buf));
		// Здесь удаляем те компоненты конфигурации, которые передавать клиенту не следует
		cfg_pack.Set(StyloQConfig::tagLoclUrl, 0);
		cfg_pack.Set(StyloQConfig::tagLoclMqbAuth, 0);
		cfg_pack.Set(StyloQConfig::tagLoclMqbSecret, 0);
		cfg_pack.Set(StyloQConfig::tagExpiryEpochSec, 0);
		//
		{
			cfg_pack.Get(StyloQConfig::tagExpiryPeriodSec, temp_buf);
			long ep = temp_buf.ToLong();
			if(ep <= 0) {
				ep = 3600; // Значение по умолчанию. Для отладки небольшое, в реальности должно быть сутки или более.
				cfg_pack.Set(StyloQConfig::tagExpiryPeriodSec, temp_buf.Z().Cat(ep));
			}
		}
		// @v11.6.0 {
		if(p_cli_pack && p_cli_pack->IsValid()) {
			SBinaryChunk bch_cli_cfg;
			if(p_cli_pack->Pool.Get(SSecretTagPool::tagPrivateConfig, &bch_cli_cfg)) {
				StyloQConfig cli_cfg_pack;
				if(cli_cfg_pack.FromJson(bch_cli_cfg.ToRawStr(temp_buf)) && cli_cfg_pack.Get(StyloQConfig::tagCliFlags, temp_buf)) {
					const long cli_flags = temp_buf.ToLong();
					if(cli_flags & (clifSvcGPS|clifPsnAdrGPS)) {
						temp_buf.Z().Cat(cli_flags & (clifSvcGPS|clifPsnAdrGPS));
						cfg_pack.Set(StyloQConfig::tagCliFlags, temp_buf);
					}
				}
			}
		}
		// } @v11.6.0 
		p_result = cfg_pack.ToJson();
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

/*static*/int StyloQConfig::MakeTransmissionJson(const char * pSrcJson, const void * pClientPacket, SString & rTransmissionJson)
{
	int    ok = 0;
	SJson * p_js = StyloQConfig::MakeTransmissionJson_(pSrcJson, pClientPacket);
	if(p_js) {
		p_js->ToStr(rTransmissionJson);
		ZDELETE(p_js);
		ok = 1;
	}
	else
		rTransmissionJson.Z();
	return ok;
}

StyloQConfig::StyloQConfig()
{
}

bool FASTCALL StyloQConfig::IsEq(const StyloQConfig & rS) const
{
	return L.IsEq(rS.L);
}

StyloQConfig & StyloQConfig::Z()
{
	L.Z();
	return *this;
}

int StyloQConfig::Set(int tag, const char * pText)
{
	int    ok = isempty(pText) ? L.Remove(tag) : L.Add(tag, pText, 1);
	if(!ok)
		PPSetErrorSLib();
	return ok;
}

int StyloQConfig::Get(int tag, SString & rResult) const
{
	rResult.Z();
	int    ok = 0;
	uint   pos = 0;
	if(L.Search(tag, &pos)) {
		rResult = L.at_WithoutParent(pos).Txt;
		ok = 1;
	}
	return ok;
}

int  StyloQConfig::SetRole(uint role)
{
	int    ok = 1;
	if(oneof5(role, roleUndef, roleClient, rolePublicService, rolePrivateService, roleDedicatedMediator)) {
		const uint64 f = GetFeatures();
		ok = role ? L.Add(tagRole, FormatUInt(role, SLS.AcquireRvlStr()), 1) : L.Remove(tagRole);
		if(ok) {
			uint64 nf = f;
			if(oneof2(role, roleUndef, roleClient))
				nf &= ~featrfMediator;
			else if(role == roleDedicatedMediator)
				nf |= featrfMediator;
			if(nf != f)
				SetFeatures(nf);
		}
		else
			PPSetErrorSLib();
	}
	else
		ok = 0;
	return ok;
}

uint StyloQConfig::GetRole() const
{
	uint   pos = 0;
	return L.Search(tagRole, &pos) ? static_cast<uint>(satoi(L.at_WithoutParent(pos).Txt)) : 0U;
}

int StyloQConfig::SetFeatures(uint64 ff)
{
	int    ok = 1;
	const  uint role = GetRole();
	THROW(!oneof2(role, roleUndef, roleClient) || !(ff & featrfMediator));
	THROW(role != roleDedicatedMediator || ff & featrfMediator);
	THROW_SL(ff ? L.Add(tagFeatures, FormatUInt64(ff, SLS.AcquireRvlStr()), 1) : L.Remove(tagFeatures));
	CATCHZOK
	return ok;
}

uint64 StyloQConfig::GetFeatures() const
{
	uint   pos = 0;
	const  uint role = GetRole();
	uint64 f = L.Search(tagFeatures, &pos) ? static_cast<uint64>(satoi64(L.at_WithoutParent(pos).Txt)) : 0ULL;
	if(oneof2(role, roleUndef, roleClient))
		return (f & ~featrfMediator);
	else if(role == roleDedicatedMediator)
		return (f | featrfMediator);
	else
		return f;
}

int StyloQConfig::FromJsonObject(const SJson * pJsObj)
{
	int    ok = 0;
	Z();
	if(pJsObj && pJsObj->Type == SJson::tOBJECT) {
		const SJson * p_next = 0;
		for(const SJson * p_cur = pJsObj->P_Child; p_cur; p_cur = p_next) {
			p_next = p_cur->P_Next;
			if(p_cur->P_Child && p_cur->P_Child->Text.NotEmpty()) {
				int tag_id = SIntToSymbTab_GetId(StyloQConfigTagNameList, SIZEOFARRAY(StyloQConfigTagNameList), p_cur->Text);
				if(tag_id && Set(tag_id, SJson::Unescape(p_cur->P_Child->Text)) > 0)
					ok = 1;
			}
		}			
	}
	if(!ok)
		PPSetError(PPERR_SQ_JSONTOCFGFAULT);
	return ok;
}

int StyloQConfig::FromJson(const char * pJsonText)
{
	int    ok = 1;
	SJson * p_js = 0;
	Z();
	if(!isempty(pJsonText)) {
		THROW_SL(p_js = SJson::Parse(pJsonText));
		THROW(FromJsonObject(p_js));
	}
	else
		ok = -1;
	CATCHZOK
	delete p_js; // @v11.3.11 @fix
	return ok;
}

SJson * StyloQConfig::ToJson() const
{
	SJson * p_result = 0;
	if(L.getCount()) {
		const long zero = 0L;
		SString tag_value;
		THROW_SL(p_result = SJson::CreateObj());
		for(uint i = 0; i < SIZEOFARRAY(StyloQConfigTagNameList); i++) {
			const SIntToSymbTabEntry & r_idx_entry = StyloQConfigTagNameList[i];
			if(Get(r_idx_entry.Id, tag_value)) {
				THROW_SL(p_result->InsertString(r_idx_entry.P_Symb, tag_value.Escape())); // @v11.4.2 Escape
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

int StyloQConfig::ToJson(SString & rResult) const
{
	int    ok = 0;
	rResult.Z();
	if(L.getCount()) {
		SJson * p_json = ToJson();
		if(p_json) {
			if(p_json->ToStr(rResult))
				ok = 1;
			ZDELETE(p_json);
		}
	}
	else
		ok = -1;
	return ok;
}
//
//
//
StyloQFace::StyloQFace() : Id(0)/*, Flags(0)*/
{
}

StyloQFace::~StyloQFace()
{
}

bool FASTCALL StyloQFace::IsEq(const StyloQFace & rS) const
{
	return (Id == rS.Id && /*Flags == rS.Flags &&*/L.IsEq(rS.L));
}

StyloQFace & StyloQFace::Z()
{
	Id = 0;
	//Flags = 0;
	L.Z();
	return *this;
}

static const SIntToSymbTabEntry StyloQFaceTagNameList[] = {
	{ StyloQFace::tagModifTime,       "modtime" },
	{ StyloQFace::tagVerifiable_Depricated, "verifialbe" },
	{ StyloQFace::tagCommonName,      "cn" },
	{ StyloQFace::tagName,            "name" },
	{ StyloQFace::tagSurName,         "surname" },
	{ StyloQFace::tagPatronymic,      "patronymic" },
	{ StyloQFace::tagDOB,             "dob" },
	{ StyloQFace::tagPhone,           "phone" },
	{ StyloQFace::tagGLN,             "gln" },
	{ StyloQFace::tagCountryIsoSymb,  "countryisosymb" },
	{ StyloQFace::tagCountryIsoCode,  "countryisocode" },
	{ StyloQFace::tagCountryName,     "country" },
	{ StyloQFace::tagZIP,             "zip" },
	{ StyloQFace::tagCityName,        "city" },
	{ StyloQFace::tagStreet,          "street" },
	{ StyloQFace::tagAddress,         "address" },
	{ StyloQFace::tagLatitude,        "lat" },
	{ StyloQFace::tagLongitude,       "lon" },
	{ StyloQFace::tagDescr,           "descr" },
	{ StyloQFace::tagImage,           "image" },
	{ StyloQFace::tagRuINN,           "ruinn" },
	{ StyloQFace::tagRuKPP,           "rukpp" },
	{ StyloQFace::tagRuSnils,         "rusnils" },
	{ StyloQFace::tagExpiryPeriodSec, "expiryperiodsec" },
	{ StyloQFace::tagExpiryEpochSec,  "expiryepochsec" },
	{ StyloQFace::tagEMail,           "email" }, // @v11.3.2
	{ StyloQFace::tagVerifiability,   "verifiability" }, // @v11.3.2
	{ StyloQFace::tagStatus,          "status" }, // @v11.3.6
	{ StyloQFace::tagImageBlobSignature, "imgblobs" }, // @v11.3.8
};

int StyloQFace::FromJsonObject(const SJson * pJsObj)
{
	int    ok = 0;
	Z();
	if(pJsObj && pJsObj->Type == SJson::tOBJECT) {
		SString tag_symb;
		SString lang_symb;
		SString temp_buf;
		const SJson * p_next = 0;
		for(const SJson * p_cur = pJsObj->P_Child; p_cur; p_cur = p_next) {
			p_next = p_cur->P_Next;
			if(p_cur->P_Child && p_cur->P_Child->Text.NotEmpty()) {
				p_cur->Text.Divide('.', tag_symb, lang_symb);
				const int tag_id = SIntToSymbTab_GetId(StyloQFaceTagNameList, SIZEOFARRAY(StyloQFaceTagNameList), tag_symb);
				if(tag_id) {
					int lang_id = 0;
					if(IsTagLangDependent(tag_id) && lang_symb.NotEmptyS()) {
						lang_id = RecognizeLinguaSymb(lang_symb, 1);
						SETIFZQ(lang_id, -1);
					}
					if(tag_id > 0 && lang_id >= 0) {
						if(Set(tag_id, lang_id, SJson::Unescape(p_cur->P_Child->Text)) > 0)
							ok = 1;
					}
				}
			}
		}			
	}
	if(!ok)
		PPSetError(PPERR_SQ_JSONTOFACEFAULT);
	return ok;
}

int StyloQFace::FromJson(const char * pJsonText)
{
	int    ok = 1;
	SJson * p_js = 0;
	Z();
	if(!isempty(pJsonText)) {
		THROW_SL(p_js = SJson::Parse(pJsonText));
		THROW(FromJsonObject(p_js));
	}
	else
		ok = -1;
	CATCHZOK
	delete p_js;
	return ok;
}

SJson * StyloQFace::ToJsonObject(bool forTransmission) const
{
	SJson * p_result = 0;
	if(L.getCount()) {
		const long zero = 0L;
		LongArray lang_list;
		SString temp_buf;
		SString lang_code;
		SString tag_value;
		GetLanguageList(lang_list);
		lang_list.atInsert(0, &zero);
		p_result = SJson::CreateObj();
		for(uint i = 0; i < SIZEOFARRAY(StyloQFaceTagNameList); i++) {
			const SIntToSymbTabEntry & r_idx_entry = StyloQFaceTagNameList[i];
			if(IsTagLangDependent(r_idx_entry.Id)) {
				for(uint li = 0; li < lang_list.getCount(); li++) {
					const int lang = lang_list.get(li);
					if(GetExactly(r_idx_entry.Id, lang, tag_value)) {
						if(!lang) {
							p_result->InsertString(r_idx_entry.P_Symb, tag_value.Escape()); // @v11.4.2 Escape()
						}
						else if(GetLinguaCode(lang, lang_code)) {
							(temp_buf = r_idx_entry.P_Symb).Dot().Cat(lang_code);
							p_result->InsertString(temp_buf, tag_value.Escape()); // @v11.4.2 Escape()
						}
						else {
							; // invalid language
						}
					}
				}
			}
			else {
				if(forTransmission && r_idx_entry.Id == tagImage) {
					if(GetExactly(r_idx_entry.Id, 0, tag_value)) {
					}
				}
				else if(GetExactly(r_idx_entry.Id, 0, tag_value)) {
					p_result->InsertString(r_idx_entry.P_Symb, tag_value.Escape()); // @v11.4.2 Escape()
				}
			}
		}
	}
	return p_result;
}

int StyloQFace::ToJson(bool forTransmission, SString & rResult) const
{
	int    ok = 0;
	SJson * p_js = 0;
	rResult.Z();
	if(L.getCount()) {
		p_js = ToJsonObject(forTransmission);
		ok = BIN(p_js && p_js->ToStr(rResult));
	}
	else
		ok = -1;
	delete p_js;
	return ok;
}

/*static*/int StyloQFace::MakeTransmissionJson(PPID id, const SBinaryChunk & rOwnIdent, const char * pSrcJson, SString & rTransmissionJson)
{
	int    ok = 0;
	SJson * p_js = StyloQFace::MakeTransmissionJson(id, rOwnIdent, pSrcJson);
	if(p_js) {
		p_js->ToStr(rTransmissionJson);
		ZDELETE(p_js);
		ok = 1;
	}
	else
		rTransmissionJson.Z();
	return ok;
}

/*static*/SJson * StyloQFace::MakeTransmissionJson(PPID id, const SBinaryChunk & rOwnIdent, const char * pSrcJson)
{
	SJson * p_result = 0;
	SString temp_buf(pSrcJson);
	THROW(temp_buf.NotEmptyS());
	{
		StyloQFace face_pack;
		StyloQFace face_copy_for_transmission;
		StyloQFace * p_result_pack = 0;
		THROW(face_pack.FromJson(temp_buf));
		if(face_pack.Has(tagImage, 0)) {
			SString img_signature;
			for(uint i = 0; i < face_pack.L.getCount(); i++) {
				StrAssocArray::Item item = face_pack.L.Get(i);
				if(item.Id != tagImage) {
					face_copy_for_transmission.L.Add(item.Id, item.Txt);
				}
				else { 
					SString fmt_buf;
					SString base64_buf;
					temp_buf = item.Txt;
					if(temp_buf.Divide(':', fmt_buf, base64_buf) > 0) {
						fmt_buf.Strip();
						SFileFormat ff;
						if(ff.IdentifyMime(fmt_buf) && SImageBuffer::IsSupportedFormat(ff))
							PPStyloQInterchange::MakeBlobSignature(rOwnIdent, PPObjID(PPOBJ_STYLOQBINDERY, id), PPStyloQInterchange::InnerBlobN_Face, img_signature);
					}						
				}
			}
			if(img_signature.NotEmpty()) {
				face_copy_for_transmission.L.Add(StyloQFace::tagImageBlobSignature, img_signature);
			}
			p_result_pack = &face_copy_for_transmission;
		}
		else {
			p_result_pack = &face_pack;
		}
		if(p_result_pack) {
			p_result_pack->Get(StyloQFace::tagExpiryPeriodSec, 0, temp_buf);
			long ep = temp_buf.ToLong();
			if(ep <= 0) {
				ep = 3600; // Значение по умолчанию. Для отладки небольшое, в реальности должно быть сутки или более.
				p_result_pack->Set(StyloQFace::tagExpiryPeriodSec, 0, temp_buf.Z().Cat(ep));
			}
			p_result = p_result_pack->ToJsonObject(true);
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

int StyloQFace::GetLanguageList(LongArray & rList) const
{
	rList.Z();
	int    ok = -1;
	for(uint i = 0; i < L.getCount(); i++) {
		StrAssocArray::Item item = L.at_WithoutParent(i);
		if(item.Id & 0xffff0000) {
			long lang = (item.Id >> 16);
			rList.add(lang);
		}
	}
	if(rList.getCount()) {
		rList.sortAndUndup();
		ok = 1;
	}
	return ok;
}

int StyloQFace::Set(int tag, int lang, const char * pText)
{
	long   eff_tag = (lang && IsTagLangDependent(tag)) ? (tag | (lang << 16)) : tag;
	int    ok = isempty(pText) ? L.Remove(eff_tag) : L.Add(eff_tag, pText, 1);
	if(!ok)
		PPSetErrorSLib();
	return ok;
}

int StyloQFace::SetStatus(int status)
{
	int    ok = 1;
	const char * p_tag_val = 0;
	switch(status) {
		case statusPrvMale: p_tag_val = "male"; break;
		case statusPrvFemale: p_tag_val = "female"; break;
		case statusPrvGenderQuestioning: p_tag_val = "private"; break;
		case statusEnterprise: p_tag_val = "enterprise"; break;
		default:
			assert(0);
			ok = 0;
			break;
	}
	if(p_tag_val) {
		assert(ok);
		ok = Set(tagStatus, 0, p_tag_val);
	}
	return ok;
}

int StyloQFace::GetStatus() const
{
	int    result = statusUndef;
	SString & r_temp_buf = SLS.AcquireRvlStr();
	if(Get(tagStatus, 0, r_temp_buf)) {
		if(r_temp_buf.IsEqiAscii("male"))
			result = statusPrvMale;
		else if(r_temp_buf.IsEqiAscii("female"))
			result = statusPrvFemale;
		else if(r_temp_buf.IsEqiAscii("private"))
			result = statusPrvGenderQuestioning;
		else if(r_temp_buf.IsEqiAscii("enterprise"))
			result = statusEnterprise;
		else if(r_temp_buf.IsEqiAscii("undef"))
			result = statusUndef;
		else
			result = -1;	
	}
	return result;
}

int StyloQFace::SetDob(LDATE dt)
{
	int    ok = 1;
	if(!dt) 
		ok = Set(tagDOB, 0, 0);
	else if(checkdate(dt)) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		r_temp_buf.Cat(dt, DATF_ISO8601CENT);
		ok = Set(tagDOB, 0, r_temp_buf);
	}
	else
		ok = PPSetErrorSLib();
	return ok;
}

int StyloQFace::SetVerifiability(int v)
{
	int    ok = 1;
	const char * p_tag_val = 0;
	switch(v) {
		case vArbitrary:
			p_tag_val = "arbitrary";
			break;
		case vVerifiable:
			p_tag_val = "verifiable";
			break;
		case vAnonymous:
			p_tag_val = "anonymous";
			break;
		default:
			assert(0);
			ok = 0;
			break;
	}
	if(p_tag_val) {
		assert(ok);
		ok = Set(tagVerifiability, 0, p_tag_val);
	}
	return ok;
}
/*int   StyloQFace::SetVerifiable(bool v)
{
	SString & r_temp_buf = SLS.AcquireRvlStr();
	return Set(tagVerifiable, 0, v ? "true" : "false");
}*/

int StyloQFace::GetExactly(int tag, int lang, SString & rResult) const
{
	rResult.Z();
	int    ok = 0;
	const long eff_tag = (tag | (lang << 16));
	uint pos = 0;
	if(L.Search(eff_tag, &pos)) {
		rResult = L.at_WithoutParent(pos).Txt;
		ok = 1;
	}
	return ok;
}

int StyloQFace::Implement_Get(int tag, int lang, SString * pResult) const
{
	CALLPTRMEMB(pResult, Z());
	int    ok = 0;
	if(lang && IsTagLangDependent(tag)) {
		LongArray eff_tag_order;
		//
		// Сначала пытаемся найти значение, ассоциированное с языком в следующем порядке: lang, 0/*nolang*/, en, ru
		//
		eff_tag_order.add(tag | (lang << 16));
		eff_tag_order.add(tag);
		if(lang != slangEN)
			eff_tag_order.add(tag | (slangEN << 16));
		if(lang != slangRU)
			eff_tag_order.add(tag | (slangRU << 16));
		for(uint i = 0; !ok && i < eff_tag_order.getCount(); i++) {
			const long eff_tag = eff_tag_order.get(i);
			uint pos = 0;
			if(L.Search(eff_tag, &pos)) {
				ASSIGN_PTR(pResult, L.at_WithoutParent(pos).Txt);
				ok = (eff_tag >> 16);
				SETIFZQ(ok, slangMeta);
			}
		}
		if(!ok) {
			//
			// Если попытка найти значение тега с указанным языком не увенчалась успехом, то ищем 
			// значение тега, ассоциированное с любым языком. 
			//
			L.GetIdList(eff_tag_order);
			for(uint j = 0; !ok && j < eff_tag_order.getCount(); j++) {
				const long eff_tag = eff_tag_order.get(j);
				if((eff_tag & 0xffff) == tag) {
					uint pos = 0;
					const int sr = L.Search(eff_tag, &pos);
					assert(sr); // не может быть, что мы не нашли элемент - что-то мы сделали не так и надо проверять код!
					if(sr) {
						ASSIGN_PTR(pResult, L.at_WithoutParent(pos).Txt);
						ok = (eff_tag >> 16);
						SETIFZQ(ok, slangMeta);					
					}
					break;
				}
			}
		}
	}
	else {
		uint pos = 0;
		if(L.Search(tag, &pos)) {
			ASSIGN_PTR(pResult, L.at_WithoutParent(pos).Txt);
			ok = 1;
		}
	}
	return ok;
}

bool StyloQFace::Has(int tag, int lang) const
{
	return (Implement_Get(tag, lang, 0) > 0);
}

int StyloQFace::Get(int tag, int lang, SString & rResult) const
{
	return Implement_Get(tag, lang, &rResult);
}

LDATE StyloQFace::GetDob() const
{
	LDATE dob = ZERODATE;
	SString & r_temp_buf = SLS.AcquireRvlStr();
	if(Get(tagDOB, 0, r_temp_buf)) {
		dob = strtodate_(r_temp_buf, DATF_ISO8601CENT);
	}
	return dob;
}

int StyloQFace::SetGeoLoc(const SGeoPosLL & rPos)
{
	int    ok = -1;
	if(rPos.IsValid()) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		Set(tagLatitude, 0, r_temp_buf.Z().Cat(rPos.Lat, MKSFMTD(0, 7, NMBF_NOTRAILZ)));
		Set(tagLongitude, 0, r_temp_buf.Z().Cat(rPos.Lon, MKSFMTD(0, 7, NMBF_NOTRAILZ)));
		ok = 1;
	}
	return ok;
}

int StyloQFace::GetGeoLoc(SGeoPosLL & rPos) const
{
	int    ok = -1;
	SString & r_temp_lat_buf = SLS.AcquireRvlStr();
	SString & r_temp_lon_buf = SLS.AcquireRvlStr();
	if(Get(tagLatitude, 0, r_temp_lat_buf) && Get(tagLongitude, 0, r_temp_lon_buf)) {
		rPos.Lat = r_temp_lat_buf.ToReal();
		rPos.Lon = r_temp_lon_buf.ToReal();
		if(rPos.IsValid()) {
			ok = 1;
		}
		else {
			rPos.Z();
			ok = 0;
		}
	}
	return ok;
}

int StyloQFace::GetVerifiability() const
{
	int    result = 0;
	SString & r_temp_buf = SLS.AcquireRvlStr();
	if(Get(tagVerifiability, 0, r_temp_buf)) {
		if(r_temp_buf.IsEqiAscii("arbitrary"))
			result = vArbitrary;
		else if(r_temp_buf.IsEqiAscii("verifiable"))
			result = vVerifiable;
		else if(r_temp_buf.IsEqiAscii("anonymous"))
			result = vAnonymous;
	}
	return result;
	/*
	const int32 v = CHKXORFLAGS(flags, fVerifiable, fAnonym);
	switch(v) {
		case 0: return vArbitrary;
		case fVerifiable: return vVerifiable;
		case fAnonym: return vAnonymous;
		default: assert(0); return 0;
	}*/
}

/*bool  StyloQFace::IsVerifiable() const
{
	bool result = false;
	SString & r_temp_buf = SLS.AcquireRvlStr();
	if(Get(tagVerifiable, 0, r_temp_buf)) {
		if(r_temp_buf.IsEqiAscii("true"))
			result = true;
	}
	return result;
}*/

int StyloQFace::SetImage(int imgFormat/*SFileFormat::XXX*/, const SImageBuffer * pImg)
{
	int    ok = 1;
	if(pImg == 0) {
		THROW(Set(tagImage, 0, 0));
	}
	else {
		SString buf_to_store;
		SETIFZ(imgFormat, SFileFormat::Webp);
		SFileFormat ff(imgFormat);
		THROW(ff.GetMimeType() == SFileFormat::mtImage);
		THROW_SL(ff.GetMime(imgFormat, buf_to_store));
		{
			SString base64_buf;
			SImageBuffer::StoreParam sp(imgFormat);
			sp.Quality = 100;
			THROW_SL(pImg->StoreMime_Base64(sp, base64_buf));
			buf_to_store.CatDiv(':', 0).Cat(base64_buf);
			THROW(Set(tagImage, 0, buf_to_store));
		}
	}
	CATCHZOK
	return ok;
}

int StyloQFace::GetImage(SImageBuffer * pImg, int * pImgFormat) const
{
	int    ok = -1;
	int    img_format = 0;
	SString store_buf;
	if(Get(tagImage, 0, store_buf) > 0) {
		SString fmt_buf;
		SString base64_buf;
		if(store_buf.Divide(':', fmt_buf, base64_buf) > 0) {
			fmt_buf.Strip();
			SFileFormat ff;
			ff.IdentifyMime(fmt_buf);
			img_format = ff;
			if(pImg) {
				THROW_SL(pImg->LoadMime_Base64(fmt_buf, base64_buf.Strip()))
			}
			ok = 1;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pImgFormat, img_format);
	return ok;
}

int StyloQFace::GetRepresentation(int lang, SString & rBuf) const
{
	rBuf.Z();
	SString temp_buf;
	if(Get(tagCommonName, lang, temp_buf)) {
		rBuf.Cat(temp_buf);
	}
	else {
		if(Get(tagSurName, lang, temp_buf)) {
			rBuf.Cat(temp_buf);
		}
		if(Get(tagName, lang, temp_buf)) {
			rBuf.CatDivIfNotEmpty(' ', 0).Cat(temp_buf);
		}
		if(Get(tagPatronymic, lang, temp_buf)) {
			rBuf.CatDivIfNotEmpty(' ', 0).Cat(temp_buf);
		}
	}
	if(Get(tagPhone, lang, temp_buf)) {
		rBuf.CatDivIfNotEmpty(' ', 0).Cat(temp_buf);
	}
	return rBuf.NotEmpty();
}
//
//
//
/*static*/int StyloQCommandList::GetCanonicalFileName(SString & rFileName)
{
	rFileName.Z();
	int    ok = 1;
	PPGetFilePath(PPPATH_WORKSPACE, "styloqcommands", rFileName);
	if(::IsDirectory(rFileName) || ::createDir(rFileName)) {
		rFileName.SetLastSlash().Cat("stqc").DotCat("xml");
	}
	else
		ok = 0;
	return ok;
}

/*static*/SString & StyloQCommandList::GetBaseCommandName(int cmdId, SString & rBuf)
{
	rBuf.Z();
	const char * p_sign = 0;
	switch(cmdId) {
		case sqbcPersonEvent: p_sign = "styloqbasecmd_personevent"; break;
		case sqbcReport: p_sign = "styloqbasecmd_report"; break;
		case sqbcRsrvOrderPrereq: p_sign = "styloqbasecmd_rsrvorderprereq"; break; // @v11.2.6
		case sqbcRsrvAttendancePrereq: p_sign = "styloqbasecmd_rsrvattendanceprereq"; break; // @v11.3.2
		case sqbcRsrvPushIndexContent: p_sign = "styloqbasecmd_rsrvpushindexcontent"; break; // @v11.3.4
		case sqbcRsrvIndoorSvcPrereq: p_sign = "styloqbasecmd_rsrvindoorsvcprereq"; break; // @v11.4.4
		case sqbcGoodsInfo: p_sign = "styloqbasecmd_goodsinfo"; break; // @v11.4.4
		case sqbcLocalBarcodeSearch: p_sign = "styloqbasecmd_localbarcodesearch"; break; // @v11.4.5
		case sqbcIncomingListOrder: p_sign = "styloqbasecmd_incominglistorder"; break; // @v11.4.6
		case sqbcIncomingListCCheck: p_sign = "styloqbasecmd_incominglistccheck"; break; // @v11.4.7
		case sqbcIncomingListTSess: p_sign = "styloqbasecmd_incominglisttsess"; break; // @v11.4.7
		case sqbcIncomingListTodo: p_sign = "styloqbasecmd_incominglisttodo"; break; // @v11.4.7
		case sqbcDebtList: p_sign = "styloqbasecmd_debtlist"; break; // @v11.5.4
	}
	if(p_sign)
		PPLoadString(p_sign, rBuf);
	return rBuf;
}

StyloQCommandList::Item::Item() : Ver(0), BaseCmdId(sqbcEmpty), Flags(0), ObjTypeRestriction(0), ObjGroupRestriction(0), ObjIdRestriction(0), ResultExpiryTimeSec(0)
{
}

bool FASTCALL StyloQCommandList::Item::IsEq(const Item & rS) const
{
	#define F(f) if(f != rS.f) return false;
	// (Поле Ver сравнивать не будем поскольку оно формируется автоматически) F(Ver);
	F(BaseCmdId);
	F(Flags);
	F(Uuid);
	F(ObjTypeRestriction);
	F(ObjGroupRestriction);
	F(ObjIdRestriction); // @v11.4.6
	F(ResultExpiryTimeSec);
	F(DbSymb);
	F(Name);
	F(ViewSymb);
	F(Description);
	F(Image);
	#undef F
	if(!Param.IsEq(rS.Param))
		return false;
	else if(!Vd.IsEq(rS.Vd))
		return false;
	return true;
}

class StyloQCommandList_CachedFileEntity : public SCachedFileEntity {
public:
	static uint GloIdx;

	static StyloQCommandList_CachedFileEntity * GetGlobalObj()
	{
		StyloQCommandList_CachedFileEntity * p_result = 0;
		if(!GloIdx) {
			ENTER_CRITICAL_SECTION
			if(!GloIdx) {
				TSClassWrapper <StyloQCommandList_CachedFileEntity> cls;
				GloIdx = SLS.CreateGlobalObject(cls);
				p_result = static_cast<StyloQCommandList_CachedFileEntity *>(SLS.GetGlobalObject(GloIdx));
				if(p_result) {
					SString file_name;
					if(!StyloQCommandList::GetCanonicalFileName(file_name))
						p_result = 0;
					else if(!p_result->Init(file_name))
						p_result = 0;
				}
			}
			LEAVE_CRITICAL_SECTION
		}
		if(GloIdx && !p_result) {
			p_result = static_cast<StyloQCommandList_CachedFileEntity *>(SLS.GetGlobalObject(GloIdx));
			if(p_result) { 
				ENTER_CRITICAL_SECTION
				if(isempty(p_result->GetFilePath())) {
					SString file_name;
					if(!StyloQCommandList::GetCanonicalFileName(file_name))
						p_result = 0;
					else if(!p_result->Init(file_name))
						p_result = 0;
				}
				LEAVE_CRITICAL_SECTION
			}
		}
		return p_result;
	}
	StyloQCommandList_CachedFileEntity() : SCachedFileEntity()
	{
	}
	virtual bool InitEntity(void * extraPtr)
	{
		Data.Z();
		return Data.Load(0, GetFilePath());
	}
	virtual void DestroyEntity()
	{
		Data.Z();
	}
	void Get(StyloQCommandList & rC) const
	{
		Lck.Lock();
		rC = Data;
		Lck.Unlock();
	}
	void Get(const char * pDbSymb, StyloQCommandList & rC) const
	{
		assert(!isempty(pDbSymb));
		Lck.Lock();
		Data.GetSubListByDbSymb(pDbSymb, 0, rC);
		Lck.Unlock();
	}
private:
	StyloQCommandList Data;
};

/*static*/uint StyloQCommandList_CachedFileEntity::GloIdx = 0;

/*static*/bool StyloQCommandList::GetFullList(const char * pDbSymb, StyloQCommandList & rList)
{
	
	rList.Z();
	bool   ok = false;
	StyloQCommandList_CachedFileEntity * p_obj = StyloQCommandList_CachedFileEntity::GetGlobalObj();
	if(p_obj) {
		if(p_obj->Reload(false, 0)) {
			if(isempty(pDbSymb))
				p_obj->Get(rList);
			else
				p_obj->Get(pDbSymb, rList);
			ok = true;
		}
	}
	return ok;
}

StyloQCommandList::StyloQCommandList()
{
}

StyloQCommandList::StyloQCommandList(const StyloQCommandList & rS)
{
	Copy(rS);
}

StyloQCommandList::~StyloQCommandList()
{
}

StyloQCommandList & FASTCALL StyloQCommandList::operator = (const StyloQCommandList & rS)
{
	Copy(rS);
	return *this;
}

bool FASTCALL StyloQCommandList::Copy(const StyloQCommandList & rS)
{
	TSCollection_Copy(L, rS.L);
	DbSymbRestriction = rS.DbSymbRestriction;
	assert(IsEq(rS));
	return true;
}

bool FASTCALL StyloQCommandList::IsEq(const StyloQCommandList & rS) const
{
	bool   eq = true;
	if(DbSymbRestriction != rS.DbSymbRestriction)
		eq = false;
	else {
		const uint _c = L.getCount();
		if(_c != rS.L.getCount())
			eq = false;
		else {
			for(uint i = 0; eq && i < _c; i++) {
				const Item * p_i1 = L.at(i);
				const Item * p_i2 = rS.L.at(i);
				if(p_i1 == p_i2)
					;
				else if(!p_i1 || !p_i2)
					eq = false;
				else if(!p_i1->IsEq(*p_i2))
					eq = false;
			}
		}
	}
	return eq;
}

StyloQCommandList & StyloQCommandList::Z()
{
	L.freeAll();
	DbSymbRestriction.Z();
	return *this;
}

StyloQCommandList::Item * StyloQCommandList::CreateNewItem(uint * pIdx)
{
	Item * p_new_item = L.CreateNewItem(pIdx);
	if(p_new_item)
		p_new_item->Uuid.Generate();
	return p_new_item;
}

uint StyloQCommandList::GetCount() const { return L.getCount(); }
StyloQCommandList::Item * StyloQCommandList::Get(uint idx) { return (idx < L.getCount()) ? L.at(idx) : 0; }
const StyloQCommandList::Item * StyloQCommandList::GetC(uint idx) const { return (idx < L.getCount()) ? L.at(idx) : 0; }

const StyloQCommandList::Item * StyloQCommandList::GetByUuid(const S_GUID & rUuid) const
{
	const StyloQCommandList::Item * p_result = 0;
	if(!rUuid.IsZero()) {
		for(uint i = 0; !p_result && i < L.getCount(); i++) {
			const Item * p_item = L.at(i);
			if(p_item && p_item->Uuid == rUuid)
				p_result = p_item;
		}
		if(!p_result) {
			SString & r_temp_buf = SLS.AcquireRvlStr();
			r_temp_buf.Cat(rUuid);
			PPSetError(PPERR_SQ_UNKNCMD, r_temp_buf);
		}
	}
	else
		PPSetError(PPERR_SQ_ZEROCMDUUID);
	return p_result;
}

int StyloQCommandList::Set(uint idx, const Item * pItem)
{
	int    ok = 1;
	if(idx < L.getCount()) {
		if(pItem) {
			*L.at(idx) = *pItem;
		}
		else {
			L.atFree(idx);	
		}
	}
	else
		ok = 0;
	return ok;
}

int StyloQCommandList::Validate(const Item * pSelectedItem) const
{
	//StyloQCommandList::sqbcRsrvPushIndexContent
	int    ok = 1;
	bool   is_selected_item_member_of_list = false;
	uint   indexing_cmd_count = 0;
	SString selected_item_name;
	if(pSelectedItem)
		(selected_item_name = pSelectedItem->Name).Transf(CTRANSF_UTF8_TO_INNER);
	for(uint i = 0; i < L.getCount(); i++) {
		const Item * p_iter_item = L.at(i);
		THROW_PP(p_iter_item, PPERR_SQ_CMDITEM_ZEROPTR);
		if(p_iter_item == pSelectedItem)
			is_selected_item_member_of_list = true;
		if(p_iter_item->BaseCmdId == StyloQCommandList::sqbcRsrvPushIndexContent)
			indexing_cmd_count++;
		THROW_PP(p_iter_item->Name.NotEmpty(), PPERR_SQ_CMDITEM_NAME_EMPTY);
		THROW_PP(p_iter_item->Name.IsLegalUtf8(), PPERR_SQ_CMDITEM_NAME_INVUTF8);
		THROW_PP(p_iter_item->BaseCmdId > 0, PPERR_SQ_CMDITEM_UNDEFBASE);
		THROW_PP(!!p_iter_item->Uuid, PPERR_SQ_CMDITEM_ZEROUUID);
		THROW_PP(p_iter_item->Description.IsLegalUtf8(), PPERR_SQ_CMDITEM_DESCR_INVUTF8);
	}
	THROW_PP(indexing_cmd_count <= 1, PPERR_SQ_CMDLIST_DUPIDXNGCMD);
	THROW_PP_S(!pSelectedItem || is_selected_item_member_of_list, PPERR_SQ_CMDLIST_SELISNTINLIST, selected_item_name);
	CATCHZOK
	return ok;
}

int StyloQCommandList::Store(const char * pFileName) const
{
	int    ok = 1;
	xmlTextWriter * p_writer = 0;
	SString temp_buf;
	if(DbSymbRestriction.NotEmpty()) {
		//
		// Если сохраняемый список ограничен символом базы данных, то загружаем полный список (p_full_list),
		// удаляем из него команды, относящиеся к символу DbSymbRestriction, включаем туда
		// команды из нашего списка и сохраняем уже p_full_list
		//
		StyloQCommandList full_list;
		THROW(full_list.Load(0, pFileName));
		uint i = full_list.GetCount(); 
		if(i) do {
			Item * p_item = full_list.Get(--i);
			if(!p_item || p_item->DbSymb.IsEqiAscii(DbSymbRestriction)) {
				full_list.Set(i, 0);
			}
		} while(i);
		for(i = 0; i < GetCount(); i++) {
			const Item * p_item = GetC(i);
			if(p_item) {
				Item * p_new_item = new Item(*p_item);
				THROW_SL(p_new_item);
				p_new_item->DbSymb = DbSymbRestriction;
				THROW_SL(full_list.L.insert(p_new_item));
			}
		}
		assert(full_list.DbSymbRestriction.IsEmpty());
		THROW(full_list.Store(pFileName));
	}
	else {
		THROW_LXML(p_writer = xmlNewTextWriterFilename(pFileName, 0), 0);  // создание writerA
		{
			xmlTextWriterSetIndent(p_writer, 1);
			xmlTextWriterSetIndentTab(p_writer);
			SXml::WDoc _doc(p_writer, cpUTF8);
			{
				SXml::WNode n_list(p_writer, "StyloQCommandList");
				for(uint i = 0; i < GetCount(); i++) {
					const Item * p_item = GetC(i);
					if(p_item) {
						SXml::WNode n_item(p_writer, "StyloQCommand");
						n_item.PutInner("Uuid", temp_buf.Z().Cat(p_item->Uuid));
						n_item.PutInner("Name", p_item->Name);
						n_item.PutInner("BaseCommandID", temp_buf.Z().Cat(p_item->BaseCmdId));
						n_item.PutInner("Flags", temp_buf.Z().Cat(p_item->Flags));
						n_item.PutInnerSkipEmpty("DbSymb", p_item->DbSymb);
						n_item.PutInnerSkipEmpty("ViewSymb", p_item->ViewSymb);
						n_item.PutInnerSkipEmpty("Description", p_item->Description);
						if(p_item->ObjTypeRestriction) {
							n_item.PutInner("ObjTypeRestriction", temp_buf.Z().Cat(p_item->ObjTypeRestriction));
							if(p_item->ObjGroupRestriction)
								n_item.PutInner("ObjGroupRestriction", temp_buf.Z().Cat(p_item->ObjGroupRestriction));
							if(p_item->ObjIdRestriction) // @v11.4.6
								n_item.PutInner("ObjIdRestriction", temp_buf.Z().Cat(p_item->ObjIdRestriction));
						}
						{
							const size_t param_len = p_item->Param.GetAvailableSize();
							if(param_len) {
								temp_buf.Z().EncodeMime64(static_cast<const char *>(p_item->Param.GetBuf(p_item->Param.GetRdOffs())), param_len);
								n_item.PutInner("Param", temp_buf);						
							}
						}
						// @v11.2.5 {
						if(p_item->ResultExpiryTimeSec > 0) {
							n_item.PutInner("ResultExpiryTimeSec", temp_buf.Z().Cat(p_item->ResultExpiryTimeSec));
						}
						// } @v11.2.5 
						if(p_item->Vd.GetCount()) {
							THROW(p_item->Vd.XmlWrite(p_writer));
						}
					}
				}
			}
		}
	}
	CATCHZOK
	xmlFreeTextWriter(p_writer);
	return ok;
}

bool StyloQCommandList::Load(const char * pDbSymb, const char * pFileName)
{
	bool   ok = true;
	const  SString preserve_dbsymb_restriction(DbSymbRestriction);
	SString temp_buf;
	SString file_name(pFileName);
	xmlParserCtxt * p_parser = 0;
	xmlDoc * p_doc = 0;
	Item * p_new_item = 0; // Указатель, распределяемый для каждого нового элемента. После вставки в контейнер указатель обнуляется.
	if(file_name.NotEmptyS()) {
		THROW_SL(fileExists(file_name));
	}
	else {
		GetCanonicalFileName(file_name);
	}
	THROW(p_parser = xmlNewParserCtxt());
	THROW_LXML(p_doc = xmlCtxtReadFile(p_parser, file_name, 0, XML_PARSE_NOENT), p_parser);
	(DbSymbRestriction = pDbSymb).Strip();
	L.freeAll();
	{
		const xmlNode * p_root = xmlDocGetRootElement(p_doc);
		if(p_root && SXml::IsName(p_root, "StyloQCommandList")) {
			for(const xmlNode * p_node = p_root->children; p_node; p_node = p_node->next) {
				if(SXml::IsName(p_node, "StyloQCommand")) {
					THROW_SL(p_new_item = new Item);
					for(const xmlNode * p_cn = p_node->children; p_cn; p_cn = p_cn->next) {
						if(SXml::GetContentByName(p_cn, "Uuid", temp_buf))
							p_new_item->Uuid.FromStr(temp_buf);
						if(SXml::GetContentByName(p_cn, "Name", temp_buf))
							p_new_item->Name = temp_buf;
						else if(SXml::GetContentByName(p_cn, "BaseCommandID", temp_buf))
							p_new_item->BaseCmdId = temp_buf.ToLong();
						else if(SXml::GetContentByName(p_cn, "Flags", temp_buf))
							p_new_item->Flags = temp_buf.ToLong();
						else if(SXml::GetContentByName(p_cn, "DbSymb", temp_buf))
							p_new_item->DbSymb = temp_buf;
						else if(SXml::GetContentByName(p_cn, "ViewSymb", temp_buf))
							p_new_item->ViewSymb = temp_buf;
						else if(SXml::GetContentByName(p_cn, "Description", temp_buf))
							p_new_item->Description = temp_buf;
						else if(SXml::GetContentByName(p_cn, "ObjTypeRestriction", temp_buf))
							p_new_item->ObjTypeRestriction = temp_buf.ToLong();
						else if(SXml::GetContentByName(p_cn, "ObjGroupRestriction", temp_buf))
							p_new_item->ObjGroupRestriction = temp_buf.ToLong();
						else if(SXml::GetContentByName(p_cn, "ObjIdRestriction", temp_buf)) // @v11.4.6
							p_new_item->ObjIdRestriction = temp_buf.ToLong();
						else if(SXml::GetContentByName(p_cn, "ResultExpiryTimeSec", temp_buf)) // @v11.2.5
							p_new_item->ResultExpiryTimeSec = temp_buf.ToLong();
						else if(SXml::GetContentByName(p_cn, "Param", temp_buf)) {
							STempBuffer bin_buf(temp_buf.Len() * 3);
							size_t actual_len = 0;
							temp_buf.DecodeMime64(bin_buf, bin_buf.GetSize(), &actual_len);
							THROW(p_new_item->Param.Write(bin_buf, actual_len));
						}
						else if(SXml::IsName(p_cn, "VD")) {
							THROW(p_new_item->Vd.XmlRead(p_cn));
						}
					}
					if(isempty(pDbSymb) || p_new_item->DbSymb.IsEqiAscii(pDbSymb)) {
						L.insert(p_new_item);
					}
					else {
						delete p_new_item;
					}
					p_new_item = 0;
				}
			}
		}
	}
	CATCH
		DbSymbRestriction = preserve_dbsymb_restriction;
		ok = false;
	ENDCATCH
	delete p_new_item;
	xmlFreeDoc(p_doc);
	xmlFreeParserCtxt(p_parser);
	return ok;
}

/*static*/SJson * StyloQCommandList::CreateJsonForClient(const StyloQCore::StoragePacket & rOwnPack, const StyloQCommandList * pSelf, SJson * pParent, const char * pName, long expirationSec)
{
	assert(!pParent || !isempty(pName));
	SString temp_buf;
	SJson * p_result = SJson::CreateObj();
	//LDATETIME dtm_now = getcurdatetime_();
	//temp_buf.Z().Cat(dtm_now, DATF_ISO8601CENT, 0);
	p_result->InsertString("doctype", "commandlist");
	p_result->InsertString("time", temp_buf.Z().Cat(time(0)));
	if(expirationSec > 0) {
		p_result->Insert("expir_time_sec", json_new_number(temp_buf.Z().Cat(expirationSec)));
	}
	{
		SJson * p_array = SJson::CreateArr();
		if(pSelf) {
			for(uint i = 0; i < pSelf->L.getCount(); i++) {
				const Item * p_item = pSelf->L.at(i);
				if(p_item) {
					SJson * p_jitem = SJson::CreateObj();
					p_jitem->InsertString("uuid", temp_buf.Z().Cat(p_item->Uuid));
					p_jitem->InsertString("basecmdid", temp_buf.Z().Cat(p_item->BaseCmdId)); // @v11.2.9
					p_jitem->InsertString("name", (temp_buf = p_item->Name).Escape());
					// @v11.5.9 {
					{
						temp_buf.Z();
						if(p_item->Flags & Item::fNotify_ObjNew)
							temp_buf.CatDivIfNotEmpty(',', 0).Cat("objnew");
						if(p_item->Flags & Item::fNotify_ObjUpd)
							temp_buf.CatDivIfNotEmpty(',', 0).Cat("objupd");
						if(p_item->Flags & Item::fNotify_ObjStatus)
							temp_buf.CatDivIfNotEmpty(',', 0).Cat("objstatus");
						p_jitem->InsertStringNe("notify", temp_buf.Escape());
					}
					// } @v11.5.9 
					if(p_item->Description.NotEmpty()) {
						p_jitem->InsertString("descr", (temp_buf = p_item->Description).Escape());
					}
					// @v11.2.5 {
					if(p_item->ResultExpiryTimeSec > 0)
						p_jitem->InsertInt("result_expir_time_sec", p_item->ResultExpiryTimeSec);
					// } @v11.2.5 
					if(p_item->BaseCmdId == StyloQCommandList::sqbcPersonEvent) { // @v11.6.5
						if(p_item->Param.GetAvailableSize()) {
							StyloQPersonEventParam param;
							SBuffer temp_non_const_buf(p_item->Param);
							SSerializeContext sctx;
							if(param.Serialize(-1, temp_non_const_buf, &sctx) && param.MaxGeoDistance > 0.0) {
								SBinaryChunk bc_own_face;
								StyloQFace own_face;
								if(rOwnPack.Pool.Get(SSecretTagPool::tagSelfyFace, &bc_own_face)) {
									SGeoPosLL geopos;
									if(own_face.FromJson(bc_own_face.ToRawStr(temp_buf)) && own_face.GetGeoLoc(geopos) > 0) {
										SJson * p_js_dist = new SJson(SJson::tOBJECT);
										p_js_dist->InsertDouble("dist", param.MaxGeoDistance, MKSFMTD(0, 2, NMBF_NOTRAILZ));
										p_js_dist->InsertDouble("lat", geopos.Lat, MKSFMTD(0, 12, NMBF_NOTRAILZ));
										p_js_dist->InsertDouble("lon", geopos.Lon, MKSFMTD(0, 12, NMBF_NOTRAILZ));
										p_jitem->Insert("maxdistto", p_js_dist);
										p_js_dist = 0;
									}
								}
							}
						}
					}
					// @todo transmit image
					json_insert_child(p_array, p_jitem);
				}
			}
		}
		p_result->Insert("item_list", p_array);
	}
	if(pParent && pName) {
		pParent->InsertNz(pName, p_result);
	}
	return p_result;
}

bool StyloQCommandList::Item::CanApplyNotifyFlag(long f) const
{
	bool   result = false;
	if(BaseCmdId == StyloQCommandList::sqbcIncomingListOrder) {
		result = oneof3(f, fNotify_ObjNew, fNotify_ObjUpd, fNotify_ObjStatus);
	}
	else if(BaseCmdId == StyloQCommandList::sqbcIncomingListCCheck) {
		result = oneof3(f, fNotify_ObjNew, fNotify_ObjUpd, fNotify_ObjStatus);
	}
	else if(BaseCmdId == StyloQCommandList::sqbcIncomingListTSess) { // @v11.6.6
		result = oneof3(f, fNotify_ObjNew, fNotify_ObjUpd, fNotify_ObjStatus);
	}
	return result;
}

bool StyloQCommandList::Item::CanApplyPrepareAheadOption() const
{
	bool   result = false;
	if(BaseCmdId == StyloQCommandList::sqbcReport) {
		// @todo Здесь довольно сложно: надо считать фильтр и идентифицировать наличие в нем критериев со значением ROBJID_CONTEXT.
		// Так как в общем случае тип фильтра нам не известен, то придется создавать виртуальную функцию в PPBaseFilt которая //
		// вернет ответ на вопрос содержится ли в фильтре такой критерий.
	}
	else if(BaseCmdId == StyloQCommandList::sqbcRsrvOrderPrereq) {
		PPID   param_id = 0;
		SBuffer rb(Param);
		/* @v11.5.0 if(!rb.Read(param_id) || param_id != ROBJID_CONTEXT) { 
			result = true;
		}*/
		// @v11.5.0 {
		StyloQDocumentPrereqParam * p_filt = StyloQDocumentPrereqParam::Read(rb);
		if(p_filt) {
			if(p_filt->PalmID != ROBJID_CONTEXT)
				result = true;
			ZDELETE(p_filt);
		}
		else
			result = true;
		// } @v11.5.0 
	}
	else if(BaseCmdId == StyloQCommandList::sqbcRsrvIndoorSvcPrereq) {
		PPID   param_id = 0;
		SBuffer rb(Param);
		if(!rb.Read(param_id) || param_id != ROBJID_CONTEXT) {
			result = true;
		}
	}		
	return result;
}

#if 0 // {
int StyloQCommandList::Item::GetAttendanceParam(StyloQAttendancePrereqParam & rP) const
{
	rP.Init(0, 0);
	int    ok = 1;
	PPBaseFilt * p_base_filt = 0;
	SBuffer _param = Param; // Функция должна быть const и дабы не менять состояние Param при чтении просто сделаем его дубликат.
	const StyloQAttendancePrereqParam pattern_filt;
	if(_param.GetAvailableSize()) {
		THROW(PPView::ReadFiltPtr(_param, &p_base_filt));
		if(p_base_filt) {
			THROW(p_base_filt->GetSignature() == pattern_filt.GetSignature());
			rP = *static_cast<StyloQAttendancePrereqParam *>(p_base_filt);
		}
	}
	else
		ok = -1;
	CATCHZOK
	delete p_base_filt;
	return ok;
}

int StyloQCommandList::Item::GetIncomingListParam(StyloQIncomingListParam & rP) const
{
	rP.Init(0, 0);
	int    ok = 1;
	PPBaseFilt * p_base_filt = 0;
	SBuffer _param = Param; // Функция должна быть const и дабы не менять состояние Param при чтении просто сделаем его дубликат.
	const StyloQIncomingListParam pattern_filt;
	if(_param.GetAvailableSize()) {
		THROW(PPView::ReadFiltPtr(_param, &p_base_filt));
		if(p_base_filt) {
			THROW(p_base_filt->GetSignature() == pattern_filt.GetSignature());
			rP = *static_cast<StyloQIncomingListParam *>(p_base_filt);
		}
	}
	else
		ok = -1;
	CATCHZOK
	delete p_base_filt;
	return ok;
}
#endif // } 0

bool StyloQCommandList::GetSubListByDbSymb(const char * pDbSymb, int baseCmdId, StyloQCommandList & rList) const
{
	rList.Z();
	bool   ok = true;
	if(!isempty(pDbSymb))
		rList.DbSymbRestriction = pDbSymb;
	for(uint i = 0; i < L.getCount(); i++) {
		const Item * p_item = L.at(i);
		if(p_item && p_item->DbSymb.IsEqiAscii(pDbSymb)) {
			if(!baseCmdId || baseCmdId == p_item->BaseCmdId) {
				Item * p_new_item = 0;
				THROW_SL(p_new_item = rList.CreateNewItem(0));
				*p_new_item = *p_item;
			}
		}
	}
	CATCHZOK
	return ok;
}

StyloQCommandList * StyloQCommandList::CreateSubListByDbSymb(const char * pDbSymb, int baseCmdId) const
{
	StyloQCommandList * p_result = 0;
	for(uint i = 0; i < L.getCount(); i++) {
		const Item * p_item = L.at(i);
		if(p_item && p_item->DbSymb.IsEqiAscii(pDbSymb)) {
			if(!baseCmdId || baseCmdId == p_item->BaseCmdId) {
				Item * p_new_item = 0;
				THROW_SL(SETIFZ(p_result, new StyloQCommandList));
				THROW_SL(p_new_item = p_result->CreateNewItem(0));
				*p_new_item = *p_item;
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

StyloQCommandList * StyloQCommandList::CreateSubListByContext(PPObjID oid, int baseCmdId, bool skipInternalCommands) const
{
	StyloQCommandList * p_result = 0;
	PPObjPerson psn_obj;
	PPObjSecur usr_obj(PPOBJ_USR, 0);
	for(uint i = 0; i < L.getCount(); i++) {
		const Item * p_item = L.at(i);
		if(p_item && (!baseCmdId || p_item->BaseCmdId == baseCmdId)) { // @v11.4.5 (&& (!baseCmdId || p_item->BaseCmdId == baseCmdId))
			// rList.Z().addzlist(PPOBJ_USR, PPOBJ_PERSON, PPOBJ_DBDIV, PPOBJ_CASHNODE, 0L);
			bool suited = false;
			if(!skipInternalCommands || !oneof2(p_item->BaseCmdId, StyloQCommandList::sqbcRsrvPushIndexContent, StyloQCommandList::sqbcGoodsInfo)) {
				if(!p_item->ObjTypeRestriction)
					suited = true;
				else if(p_item->ObjTypeRestriction == PPOBJ_UNASSIGNED && oid.Obj == 0) // @v11.4.5
					suited = true;
				else if(p_item->ObjTypeRestriction == oid.Obj) {
					if(!p_item->ObjGroupRestriction)
						suited = true;
					else if(oid.Obj == PPOBJ_PERSON && psn_obj.P_Tbl->IsBelongToKind(oid.Id, p_item->ObjGroupRestriction))
						suited = true;
				}
				else if(oid.Obj == PPOBJ_USR) {
					PPSecur2 sec_rec;
					if(p_item->ObjTypeRestriction == PPOBJ_PERSON && usr_obj.Search(oid.Id, &sec_rec) > 0 && sec_rec.PersonID) {
						if(!p_item->ObjGroupRestriction) 
							suited = true;
						else if(psn_obj.P_Tbl->IsBelongToKind(sec_rec.PersonID, p_item->ObjGroupRestriction))
							suited = true;
					}
				}
				// @todo Необходимо закончить набор условий для проверки соответствия команды запрашивающему клиенту
			}
			if(suited) {
				THROW_SL(SETIFZ(p_result, new StyloQCommandList));
				Item * p_new_item = p_result->CreateNewItem(0);
				THROW_SL(p_new_item);
				*p_new_item = *p_item;
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

StyloQCore::StoragePacket::StoragePacket() : Signature(PPConst::Signature_StyloQStoragePacket)
{
}

StyloQCore::StoragePacket::StoragePacket(const StoragePacket & rS) : Signature(PPConst::Signature_StyloQStoragePacket), Rec(rS.Rec), Pool(rS.Pool)
{
}

StyloQCore::StoragePacket & FASTCALL StyloQCore::StoragePacket::operator = (const StoragePacket & rS)
{
	Rec = rS.Rec;
	Pool = rS.Pool;
	return *this;
}

bool FASTCALL StyloQCore::StoragePacket::IsEq(const StyloQCore::StoragePacket & rS) const
{
	#define FE(f) if(Rec.f != rS.Rec.f) return false;
	FE(ID);
	FE(Kind);
	FE(CorrespondID);
	FE(Expiration);
	FE(LinkObjType);
	FE(LinkObjID);
	#undef FE
	if(memcmp(Rec.BI, rS.Rec.BI, sizeof(Rec.BI)) != 0)
		return false;
	if(!Pool.IsEq(rS.Pool))
		return false;
	return true;
}

bool StyloQCore::StoragePacket::IsValid() const
{
	bool   ok = true;
	THROW(Signature == PPConst::Signature_StyloQStoragePacket); // @v11.6.0
	THROW(oneof8(Rec.Kind, kNativeService, kForeignService, kDocIncoming, kDocOutcoming, kClient, kSession, kFace, kCounter))
	THROW(oneof2(Rec.Kind, kNativeService, kForeignService) || !(Rec.Flags & styloqfMediator));
	THROW(oneof2(Rec.Kind, kDocIncoming, kDocOutcoming) || !(Rec.Flags & styloqfDocStatusFlags));
	if(oneof2(Rec.Kind, kDocIncoming, kDocOutcoming)) {
		//THROW(!(Rec.Flags & styloqfDocFinished)      || !(Rec.Flags & (styloqfDocWaitForOrdrsp|styloqfDocWaitForDesadv|styloqfDocDraft)));
		//THROW(!(Rec.Flags & styloqfDocWaitForOrdrsp) || !(Rec.Flags & (styloqfDocFinished|styloqfDocDraft)));
		//THROW(!(Rec.Flags & styloqfDocWaitForDesadv) || !(Rec.Flags & (styloqfDocFinished|styloqfDocDraft)));
		//THROW(!(Rec.Flags & styloqfDocDraft)         || !(Rec.Flags & (styloqfDocFinished|styloqfDocWaitForOrdrsp|styloqfDocWaitForDesadv)));
	}
	CATCHZOK
	return ok;
}

bool StyloQCore::StoragePacket::SetDocStatus(int styloqDocStatus)
{
	bool   ok = true;
	THROW(oneof2(Rec.Kind, kDocIncoming, kDocOutcoming));
	THROW(((styloqDocStatus << 1) & ~styloqfDocStatusFlags) == 0); // Проверяем чтоб за пределами битовой зоны статусов ничего не было.
	//THROW(!(Rec.Flags & (styloqfDocFinished|styloqfDocWaitForOrdrsp|styloqfDocWaitForDesadv)));
	//Rec.Flags |= styloqfDocDraft;
	Rec.Flags &= ~styloqfDocStatusFlags;
	Rec.Flags |= ((styloqDocStatus << 1) & styloqfDocStatusFlags);
	CATCHZOK
	return ok;	
}

int StyloQCore::StoragePacket::GetDocStatus() const
{
	int    result = 0;
	if(oneof2(Rec.Kind, kDocIncoming, kDocOutcoming)) {
		result = ((Rec.Flags >> 1) & styloqfDocStatusFlags);
	}
	return result;
}

/*
bool StyloQCore::StoragePacket::SetDocStatus_Draft()
{
	bool   ok = true;
	THROW(oneof2(Rec.Kind, kDocIncoming, kDocOutcominig));
	THROW(!(Rec.Flags & (styloqfDocFinished|styloqfDocWaitForOrdrsp|styloqfDocWaitForDesadv)));
	Rec.Flags |= styloqfDocDraft;
	CATCHZOK
	return ok;
}

bool StyloQCore::StoragePacket::SetDocStatus_Finished()
{
	bool   ok = true;
	THROW(oneof2(Rec.Kind, kDocIncoming, kDocOutcominig));
	Rec.Flags &= ~(styloqfDocWaitForOrdrsp|styloqfDocWaitForDesadv|styloqfDocDraft);
	Rec.Flags |= styloqfDocFinished;
	CATCHZOK
	return ok;
}

bool StyloQCore::StoragePacket::SetDocStatus_Intermediate(int flags)
{
	bool   ok = true;
	THROW(flags & (styloqfDocWaitForOrdrsp|styloqfDocWaitForDesadv));
	THROW(!(flags & ~(styloqfDocWaitForOrdrsp|styloqfDocWaitForDesadv)));
	THROW(oneof2(Rec.Kind, kDocIncoming, kDocOutcominig));
	THROW(!(Rec.Flags & styloqfDocFinished));
	Rec.Flags |= (flags & (styloqfDocWaitForOrdrsp|styloqfDocWaitForDesadv));
	Rec.Flags &= ~(styloqfDocDraft);
	CATCHZOK
	return ok;
}
*/

int StyloQCore::StoragePacket::GetFace(int tag, StyloQFace & rF) const
{
	rF.Z();
	int    ok = -1;
	if(oneof2(tag, SSecretTagPool::tagFace, SSecretTagPool::tagSelfyFace)) {
		SBinaryChunk bytes;
		if(Pool.Get(tag, &bytes) && bytes.Len()) {
			SString temp_buf;
			if(rF.FromJson(bytes.ToRawStr(temp_buf)))
				ok = 1;
		}
	}
	return ok;
}

/*static*/ReadWriteLock StyloQCore::_SvcDbMapRwl; // Блокировка для защиты _SvcDbMap
/*static*/StyloQCore::SvcDbSymbMap StyloQCore::_SvcDbMap;

/*static*/bool StyloQCore::GetDbMapBySvcIdent(const SBinaryChunk & rIdent, SString * pDbSymb, uint * pFlags)
{
	bool result = false;
	{
		SRWLOCKER(_SvcDbMapRwl, SReadWriteLocker::Read);
		if(_SvcDbMap.IsLoaded()) {
			result = _SvcDbMap.FindSvcIdent(rIdent, pDbSymb, pFlags);
		}
		else {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(_SvcDbMap.Read(0, 0)) {
				result = _SvcDbMap.FindSvcIdent(rIdent, pDbSymb, pFlags);
			}
		}
	}
	return result;
}

/*static*/bool StyloQCore::GetDbMap(SvcDbSymbMap & rMap)
{
	bool result = false;
	{
		SRWLOCKER(_SvcDbMapRwl, SReadWriteLocker::Read);
		if(_SvcDbMap.IsLoaded()) {
			TSCollection_Copy(rMap, _SvcDbMap);
			result = true;
		}
		else {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(_SvcDbMap.Read(0, 0)) {
				TSCollection_Copy(rMap, _SvcDbMap);
				result = true;
			}
		}
	}
	return result;
}

/*static*/bool StyloQCore::IsDocStatusFinished(int status)
{
	return oneof6(status, styloqdocstCANCELLED, styloqdocstREJECTED, styloqdocstEXECUTIONACCEPTED,
		styloqdocstCANCELLEDDRAFT, styloqdocstFINISHED_SUCC, styloqdocstFINISHED_FAIL);
}

/*static*/void StyloQCore::MakeDocExtDescriptionText(long docType, long flags, SString & rBuf)
{
	rBuf.Z();
	switch(docType) {
		case doctypCommandList:
			rBuf = "CommandList";
			break;
		case doctypOrderPrereq:
			rBuf = "OrderPrereq";
			break;
		case doctypReport:
			rBuf = "Report";
			break;
		case doctypGeneric:
			rBuf = "Generic";
			break;
		case doctypIndexingContent:
			rBuf = "IndexingContent";
			if(flags & fUnprocessedDoc_Misplaced)
				rBuf.Space().CatParStr("unprocessed");
			else
				rBuf.Space().CatParStr("processed");
			break;
		case doctypIndoorSvcPrereq:
			rBuf = "IndoorSvcPrereq";
			break;
		case doctypIncomingList: // @v11.4.8
			rBuf = "IncomingList";
			break;
		case doctypDebtList: // @v11.5.4
			rBuf = "DebtList";
			break;
	}
}

/*static*/bool StyloQCore::ValidateStatusTransition(int status, int newStatus)
{
	bool ok = false;
	switch(status) {
		case styloqdocstUNDEF:
			ok = (newStatus == styloqdocstDRAFT);
			break;
		case styloqdocstDRAFT:
			ok = (newStatus == styloqdocstWAITFORAPPROREXEC);
			break;
		case styloqdocstWAITFORAPPROREXEC:
			ok = oneof5(newStatus, styloqdocstAPPROVED, styloqdocstCORRECTED, styloqdocstREJECTED, styloqdocstMODIFIED, styloqdocstCANCELLED);
			break;
		case styloqdocstAPPROVED:
			ok = oneof2(newStatus, styloqdocstCANCELLED, styloqdocstEXECUTED);
			break;
		case styloqdocstCORRECTED:
			break;
		case styloqdocstCORRECTIONACCEPTED:
			break;
		case styloqdocstCORRECTIONREJECTED:
			break;
		case styloqdocstREJECTED:
			break;
		case styloqdocstMODIFIED:
			break;
		case styloqdocstCANCELLED:
			break;
		case styloqdocstEXECUTED:
			ok = oneof2(newStatus, styloqdocstEXECUTIONACCEPTED, styloqdocstEXECUTIONCORRECTED);
			break;
		case styloqdocstEXECUTIONACCEPTED:
			ok = (newStatus == styloqdocstFINISHED_SUCC);
			break;
		case styloqdocstEXECUTIONCORRECTED:
			break;
		case styloqdocstEXECORRECTIONACCEPTED:
			break;
		case styloqdocstEXECORRECTIONREJECTED:
			break;
		case styloqdocstFINISHED_SUCC:
			break;
		case styloqdocstFINISHED_FAIL:
			break;
	}
	return ok;
}

StyloQCore::StyloQCore() : StyloQSecTbl()
{
}

int StyloQCore::MakeTextHashForCounterparty(const StoragePacket & rOtherPack, uint len, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	if(len > 0 && len <= 20) {
		SBinaryChunk cli_ident;
		SBinaryChunk svc_ident;
		THROW(oneof2(rOtherPack.Rec.Kind, kClient, kForeignService));
		rOtherPack.Pool.Get(SSecretTagPool::tagClientIdent, &cli_ident);
		if(rOtherPack.Rec.Kind == kForeignService) {
			rOtherPack.Pool.Get(SSecretTagPool::tagSvcIdent, &svc_ident);
		}
		if(cli_ident.Len() > 0 && svc_ident.Len() > 0) {
			SBinaryChunk hashed_chunk;
			hashed_chunk.Cat(svc_ident);
			hashed_chunk.Cat(cli_ident);
			binary160 hash = SlHash::Sha1(0, hashed_chunk.PtrC(), hashed_chunk.Len());
			for(uint i = 0; rBuf.Len() < len && i < sizeof(hash); i++) {
				rBuf.CatChar((char)('A' + ((255 + hash.D[i]) % 26)));
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int StyloQCore::ReadCurrentPacket(StoragePacket * pPack)
{
	int    ok = -1;
	if(pPack) {
		SBuffer sbuf;
		SSerializeContext sctx;
		copyBufTo(&pPack->Rec);
		readLobData(VT, sbuf);
		destroyLobData(VT);
		THROW_SL(pPack->Pool.Serialize(-1, sbuf, &sctx));
	}
	CATCHZOK
	return ok;
}

int StyloQCore::GetPeerEntry(PPID id, StoragePacket * pPack)
{
	int    ok = -1;
	StyloQSecTbl::Key0 k0;
	k0.ID = id;
	if(search(0, &k0, spEq)) {
		THROW(ReadCurrentPacket(pPack));
		ok = 1;
	}
	else {
		PPSetError(PPERR_SQ_ENTRYNFOUND, id);
	}
	destroyLobData(VT); // @v11.4.9 @fix
	CATCHZOK
	return ok;
}

int StyloQCore::SearchSession(const SBinaryChunk & rOtherPublic, StoragePacket * pPack)
{
	int    ok = -1;
	const binary160 bi = SlHash::Sha1(0, rOtherPublic.PtrC(), rOtherPublic.Len());
	StyloQSecTbl::Key1 k1;
	MEMSZERO(k1);
	assert(sizeof(bi) <= sizeof(k1.BI));
	k1.Kind = StyloQCore::kSession;
	memcpy(&k1.BI, &bi, sizeof(bi));
	if(search(1, &k1, spGe) && data.Kind == kSession && memcmp(&bi, data.BI, sizeof(bi)) == 0) {
		THROW(ReadCurrentPacket(pPack));
		ok = 1;			
	}
	else if(BTRNFOUND) {
		PPSetError(PPERR_SQPROT_SESSNFOUND);
		ok = -1;
	}
	destroyLobData(VT); // @v11.4.9 @fix
	CATCHZOK
	return ok;
}

int StyloQCore::GetOwnPeerEntry(StoragePacket * pPack)
{
	int    ok = -1;
	StyloQSecTbl::Key1 k1;
	MEMSZERO(k1);
	k1.Kind = kNativeService;
	if(search(1, &k1, spGe) && data.Kind == kNativeService) {
		THROW(ReadCurrentPacket(pPack));
		ok = 1;		
	}
	else {
		PPSetError(PPERR_SQ_OWNENTRYNFOUND);
	}
	destroyLobData(VT); // @v11.4.9 @fix
	CATCHZOK
	return ok;
}

int StyloQCore::SearchGlobalIdentEntry(int kind, const SBinaryChunk & rIdent, StyloQCore::StoragePacket * pPack)
{
	assert(rIdent.Len() > 0 && rIdent.Len() <= sizeof(StyloQSecTbl::Key1));
	int    ok = -1;
	StyloQSecTbl::Key1 k1;
	THROW(rIdent.Len() > 0 && rIdent.Len() <= sizeof(StyloQSecTbl::Key1));
	MEMSZERO(k1);
	k1.Kind = kind;
	memcpy(k1.BI, rIdent.PtrC(), rIdent.Len());
	if(search(1, &k1, spGe) && data.Kind == kind && rIdent.IsEq(data.BI, rIdent.Len())) {
		THROW(ReadCurrentPacket(pPack));
		ok = 1;			
	}
	else {
		PPSetError(PPERR_SQ_ENTRYIDENTNFOUND, rIdent.Mime64(SLS.AcquireRvlStr()));
	}
	destroyLobData(VT); // @v11.4.9 @fix
	CATCHZOK
	return ok;
}

int StyloQCore::MakeDocumentStorageIdent(const SBinaryChunk & rOtherIdent, const S_GUID & rCmdUuid, SBinaryChunk & rDocIdent) const
{
	rDocIdent.Z();
	int    ok = 1;
	THROW(rOtherIdent.Len() > 0);
	if(!rCmdUuid.IsZero()) {
		SlHash::State st;
		SlHash::Sha1(&st, rOtherIdent.PtrC(), rOtherIdent.Len());
		SlHash::Sha1(&st, &rCmdUuid, sizeof(rCmdUuid));
		binary160 digest = SlHash::Sha1(&st, 0, 0); // finalize
		rDocIdent.Put(&digest, sizeof(digest));
	}
	else {
		rDocIdent = rOtherIdent;
	}
	assert(rDocIdent.Len() == 20);
	CATCHZOK
	return ok;
}

int StyloQCore::Helper_PutDocument(PPID * pID, StyloQSecTbl::Rec * pResultRec, const SBinaryChunk & rContractorIdent, int direction, int docType, const SBinaryChunk & rIdent, const SSecretTagPool & rPool, int use_ta)
{
	int    ok = -1;
	PPID   new_id = 0;
	SJson * p_js = 0;
	SString temp_buf;
	SBinaryChunk raw_doc;
	SBinaryChunk raw_doc_decl;
	time_t doc_time = 0;
	long   doc_expiry = 0;
	SString raw_doc_type;
	StoragePacket pack;
	StoragePacket other_pack;
	bool  skip_parsing_doc = false; // Если к документу прилагается декларация, то сам документ парсить не надо
	const time_t tm_now = time(0);
	THROW(rIdent.Len() > 0 && rIdent.Len() <= sizeof(pack.Rec.BI));
	THROW(rContractorIdent.Len() > 0 && rContractorIdent.Len() <= sizeof(pack.Rec.BI));
	const int sgi_as_cli_r = SearchGlobalIdentEntry(kClient, rContractorIdent, &other_pack);
	const int sgi_as_svc_r = (sgi_as_cli_r > 0) ? -1 : SearchGlobalIdentEntry(kForeignService, rContractorIdent, &other_pack);
	const int sgi_as_ownsvc_r = (sgi_as_cli_r > 0 || sgi_as_svc_r > 0) ? -1 : SearchGlobalIdentEntry(kNativeService, rContractorIdent, &other_pack); // @v11.4.8
	THROW(sgi_as_cli_r > 0 || sgi_as_svc_r > 0 || sgi_as_ownsvc_r > 0);
	THROW(oneof3(other_pack.Rec.Kind, kForeignService, kClient, kNativeService));
	THROW(rPool.Get(SSecretTagPool::tagRawData, &raw_doc));
	// @v11.4.8 {
	if(rPool.Get(SSecretTagPool::tagDocDeclaration, &raw_doc_decl)) {
		//MakeDocDeclareJs
		THROW_SL(p_js = SJson::Parse(raw_doc_decl.ToRawStr(temp_buf)));
		for(const SJson * p_cur = p_js; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Type == SJson::tOBJECT) {								
				for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
					if(p_obj->Text.IsEqiAscii("type"))
						raw_doc_type = SJson::Unescape(p_obj->P_Child->Text);
					else if(p_obj->Text.IsEqiAscii("time"))
						doc_time = static_cast<time_t>(p_obj->P_Child->Text.ToInt64());
					else if(p_obj->Text.IsEqiAscii("resultexpirytimesec"))
						doc_expiry = p_obj->P_Child->Text.ToLong();
				}
				skip_parsing_doc = true;
			}
		}
		ZDELETE(p_js);
	}
	// } @v11.4.8 
	if(!skip_parsing_doc) {
		THROW_SL(p_js = SJson::Parse(raw_doc.ToRawStr(temp_buf)));
		{
			for(const SJson * p_cur = p_js; p_cur; p_cur = p_cur->P_Next) {
				if(p_cur->Type == SJson::tOBJECT) {								
					for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
						if(p_obj->Text.IsEqiAscii("doctype"))
							raw_doc_type = SJson::Unescape(p_obj->P_Child->Text);
						else if(p_obj->Text.IsEqiAscii("time"))
							doc_time = static_cast<time_t>(p_obj->P_Child->Text.ToInt64());
						else if(p_obj->Text.IsEqiAscii("expir_time_sec"))
							doc_expiry = p_obj->P_Child->Text.ToLong();
					}
				}
			}
		}
	}
	{
		assert(other_pack.Rec.ID > 0); // Мы все проверили выше: не может быть чтоб этот ид был неопределен.
		LongArray ex_id_list;
		if(direction > 0)
			pack.Rec.Kind = kDocOutcoming;
		else if(direction < 0)
			pack.Rec.Kind = kDocIncoming;
		pack.Rec.DocType = docType;
		pack.Rec.CorrespondID = other_pack.Rec.ID;
		pack.Rec.TimeStamp = tm_now * 1000;
		memcpy(pack.Rec.BI, rIdent.PtrC(), rIdent.Len());
		if(doc_expiry > 0) {
			// Если контрагент указал период истечения срока действия документа, то отсчитываем его от своего текущего времени
			// дабы избежать ошибки, связанной с разным значением текущего времени у нас и у контрагента.
			pack.Rec.Expiration.SetTimeT(tm_now + doc_expiry);
		}
		pack.Pool = rPool;
		if(docType == doctypGeneric) {
			long _ex_doc_id_from_db = 0;
			// Документ такого типа может быть только один в комбинации {direction; rIdent}
			{
				PPTransaction tra(use_ta);
				THROW(tra);
				//
				// @v11.4.2 Выяснилась неприятная вещь: индексы таблицы, содержащие Timestamp немодифицируемые.
				// Наверное, делать их таковыми было опрометчиво, но пока для сглаживания проблемы будем удалять
				// предшествующую запись и затем вставлять новую.
				//
				if(GetDocIdListByType(direction, docType, &rIdent, ex_id_list) > 0) {
					for(uint i = 0; i < ex_id_list.getCount(); i++)	{
						if(i == ex_id_list.getCount()-1) {
							// @v11.4.2 _ex_doc_id_from_db = ex_id_list.get(i);
							PPID _id_to_remove = ex_id_list.get(i); // @v11.4.2 
							THROW(PutPeerEntry(&_id_to_remove, 0, 0)); // @v11.4.2 
						}
						else {
							PPID _id_to_remove = ex_id_list.get(i);
							THROW(PutPeerEntry(&_id_to_remove, 0, 0));
						}
					}
				}
				// @v11.4.2 {
				if(_ex_doc_id_from_db) {
					pack.Rec.ID = _ex_doc_id_from_db;
					// @! pack.Rec.TimeStamp
				}
				// } @v11.4.2
				THROW(PutPeerEntry(&_ex_doc_id_from_db, &pack, 0));
				THROW(tra.Commit());
				new_id = _ex_doc_id_from_db;
				ok = 1;
			}
		}
		else if(oneof4(docType, doctypCommandList, doctypIndexingContent, doctypOrderPrereq, doctypIndoorSvcPrereq)) {
			if(docType == doctypIndexingContent) {
				pack.Rec.Flags |= fUnprocessedDoc_Misplaced;
			}
			else if(docType == doctypCommandList) {
				THROW(raw_doc_type.IsEqiAscii("commandlist"));
			}
			// Документ такого типа может быть только один в комбинации {direction; rIdent}
			{
				PPTransaction tra(use_ta);
				THROW(tra);
				if(GetDocIdListByType(direction, docType, &rIdent, ex_id_list) > 0) {
					for(uint i = 0; i < ex_id_list.getCount(); i++)	{
						PPID _id_to_remove = ex_id_list.get(i);
						THROW(PutPeerEntry(&_id_to_remove, 0, 0));
					}
				}
				{
					assert(new_id == 0);
					THROW(PutPeerEntry(&new_id, &pack, 0));
				}
				THROW(tra.Commit());
				ok = 1;
			}
		}
	}
	CATCHZOK
	delete p_js; // @v11.3.11 @fix
	// @v11.4.8 {
	if(ok > 0) {
		ASSIGN_PTR(pResultRec, pack.Rec);
	}
	// } @v11.4.8 
	ASSIGN_PTR(pID, new_id);
	return ok;
}

int StyloQCore::PutDocument(PPID * pID, const SBinaryChunk & rContractorIdent, int direction, int docType, const SBinaryChunk & rIdent, const SSecretTagPool & rPool, int use_ta)
{
	return Helper_PutDocument(pID, 0, rContractorIdent, direction, docType, rIdent, rPool, use_ta);
}

int StyloQCore::PutDocument(PPID * pID, StyloQSecTbl::Rec * pResultRec, const SBinaryChunk & rContractorIdent, int direction, int docType, const SBinaryChunk & rIdent, const SSecretTagPool & rPool, int use_ta)
{
	return Helper_PutDocument(pID, pResultRec, rContractorIdent, direction, docType, rIdent, rPool, use_ta);
}

int StyloQCore::GetDocByType(int direction, int docType, const SBinaryChunk * pIdent, StoragePacket * pPack)
{
	int    ok = -1;
	StyloQSecTbl::Key3 k3;
	if(!pIdent || (pIdent->Len() && pIdent->Len() <= sizeof(k3.BI))) {
		time_t least_tm = 0;
		PPID   least_id = 0;
		MEMSZERO(k3);
		k3.DocType = docType;
		if(pIdent) {
			assert(pIdent->Len() && pIdent->Len() <= sizeof(k3.BI)); // Мы выше проверили это утверждение
			memcpy(k3.BI, pIdent->PtrC(), pIdent->Len());
		}
		if(search(3, &k3, spGe) && data.DocType == docType) do {
			if((direction == 0 && oneof2(data.Kind, kDocOutcoming, kDocIncoming)) || (direction > 0 && data.Kind == kDocOutcoming) || (direction < 0 && data.Kind == kDocIncoming)) {
				if(!pIdent || memcmp(data.BI, pIdent->PtrC(), pIdent->Len()) == 0) {
					if(!least_id || data.TimeStamp > least_tm) {
						least_tm = static_cast<time_t>(data.TimeStamp);
						least_id = data.ID;
					}
				}
			}
			destroyLobData(VT); // @v11.4.9 @fix
		} while(search(3, &k3, spNext) && data.DocType == docType);
		destroyLobData(VT); // @v11.4.9 @fix
		if(least_id) {
			ok = pPack ? GetPeerEntry(least_id, pPack) : 1;
		}
	}
	return ok;
}

int StyloQCore::GetDocIdListByType(int direction, int docType, const SBinaryChunk * pIdent, LongArray & rIdList)
{
	rIdList.Z();
	int    ok = -1;
	StyloQSecTbl::Key3 k3;
	if(!pIdent || (pIdent->Len() && pIdent->Len() <= sizeof(k3.BI))) {
		MEMSZERO(k3);
		k3.DocType = docType;
		if(pIdent) {
			assert(pIdent->Len() && pIdent->Len() <= sizeof(k3.BI)); // Мы выше проверили это утверждение
			memcpy(k3.BI, pIdent->PtrC(), pIdent->Len());
		}
		if(search(3, &k3, spGe) && data.DocType == docType) do {
			if(direction == 0 || (direction > 0 && data.Kind == kDocOutcoming) || (direction < 0 && data.Kind == kDocIncoming)) {
				if(!pIdent || memcmp(data.BI, pIdent->PtrC(), pIdent->Len()) == 0) {
					rIdList.add(data.ID);
					ok = 1;
				}
			}
			destroyLobData(VT); // @v11.4.9 @fix
		} while(search(3, &k3, spNext) && data.DocType == docType);
		destroyLobData(VT); // @v11.4.9 @fix
	}
	return ok;
}

int StyloQCore::GetUnprocessedIndexindDocIdList(LongArray & rIdList)
{
	rIdList.Z();
	int    ok = -1;
	StyloQSecTbl::Key3 k3;
	MEMSZERO(k3);
	k3.DocType = doctypIndexingContent;
	{
		if(search(3, &k3, spGe) && data.DocType == doctypIndexingContent) do {
			if(data.Flags & fUnprocessedDoc_Misplaced) {
				rIdList.add(data.ID);
				ok = 1;
			}
			destroyLobData(VT); // @v11.4.9 @fix
		} while(search(3, &k3, spNext) && data.DocType == doctypIndexingContent);
		destroyLobData(VT); // @v11.4.9 @fix
	}
	return ok;
}

int StyloQCore::GetMediatorIdList(LongArray & rIdList)
{
	rIdList.Z();
	int    ok = -1;
	StyloQSecTbl::Key1 k1;
	MEMSZERO(k1);
	k1.Kind = kForeignService;
	BExtQuery q(this, 1);
	q.select(this->ID, this->Flags, this->BI, 0).where(this->Kind == static_cast<long>(kForeignService));
	for(q.initIteration(false, &k1, spFirst); q.nextIteration() > 0;) {
		if(data.Flags & styloqfMediator) {
			rIdList.add(data.ID);
			ok = 1;
		}
	}
	return ok;
}

int StyloQCore::PutPeerEntry(PPID * pID, StoragePacket * pPack, int use_ta)
{
	int    ok = 1;
	const  PPID outer_id = pID ? *pID : 0;
	if(pPack) {
		assert(oneof7(pPack->Rec.Kind, kNativeService, kForeignService, kClient, kSession, kFace, kDocIncoming, kDocOutcoming)); // @v11.1.8 kFace
		if(oneof3(pPack->Rec.Kind, kNativeService, kForeignService, kClient)) {
			// Для обозначенных видов записи timestamp нулевой с целью обеспечения уникальности индекса {Kind, BI, TimeStamp}
			pPack->Rec.TimeStamp = 0;
		}
	}
	SBuffer cbuf;
	SSerializeContext sctx;
	bool   do_destroy_lob = false;
	const  RECORDSIZE fix_rec_size = getRecSize();
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(outer_id) {
			StyloQCore::StoragePacket preserve_pack;
			THROW(GetPeerEntry(outer_id, &preserve_pack) > 0);
			if(pPack) {
				assert(memcmp(preserve_pack.Rec.BI, pPack->Rec.BI, sizeof(pPack->Rec.BI)) == 0); // @v11.4.2
				if(pPack->Rec.Kind == kFace) {
					if(ismemzero(pPack->Rec.BI, sizeof(pPack->Rec.BI))) {
						S_GUID guid(SCtrGenerate_);
						memcpy(pPack->Rec.BI, &guid, sizeof(guid));
					}
				}
				if(pPack->IsEq(preserve_pack)) {
					ok = -1;
				}
				else {
					pPack->Rec.ID = outer_id;
					// @v11.3.8 THROW_DB(rereadForUpdate(0, 0));
					// (Этот вызов почему-то ломает внутреннюю структуру LOB-поля) copyBufFrom(&pPack->Rec);
					copyBufFrom(&pPack->Rec, fix_rec_size); // @v11.3.4 @fix
					THROW_SL(pPack->Pool.Serialize(+1, cbuf, &sctx));
					THROW(writeLobData(VT, cbuf.GetBuf(0), cbuf.GetAvailableSize()));
					THROW_DB(updateRec());
					DS.LogAction(PPACN_OBJUPD, PPOBJ_STYLOQBINDERY, pPack->Rec.ID, 0, 0); // @v11.2.12
					do_destroy_lob = true;
				}
			}
			else {
				THROW_DB(rereadForUpdate(0, 0));
				THROW_DB(deleteRec());
				DS.LogAction(PPACN_OBJRMV, PPOBJ_STYLOQBINDERY, outer_id, 0, 0); // @v11.2.12
			}
		}
		else if(pPack) {
			PPID   new_id = 0;
			if(pPack->Rec.Kind == kNativeService) {
				// В таблице может быть не более одной записи вида kNativeService
				THROW(GetOwnPeerEntry(0) < 0); // @error Попытка вставить вторую запись вида native-service
			}
			else if(pPack->Rec.Kind == kFace) {
				if(ismemzero(pPack->Rec.BI, sizeof(pPack->Rec.BI))) {
					const S_GUID guid(SCtrGenerate_);
					memcpy(pPack->Rec.BI, &guid, sizeof(guid));
				}
			}
			pPack->Rec.ID = 0;
			copyBufFrom(&pPack->Rec, fix_rec_size);
			THROW_SL(pPack->Pool.Serialize(+1, cbuf, &sctx));
			THROW(writeLobData(VT, cbuf.GetBuf(0), cbuf.GetAvailableSize()));
			THROW_DB(insertRec(0, &new_id));
			ASSIGN_PTR(pID, new_id);
			pPack->Rec.ID = new_id;
			DS.LogAction(PPACN_OBJADD, PPOBJ_STYLOQBINDERY, new_id, 0, 0); // @v11.2.12
			do_destroy_lob = true;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	if(do_destroy_lob)
		destroyLobData(VT);
	return ok;
}

TLP_IMPL(PPObjStyloQBindery, StyloQCore, P_Tbl);

PPObjStyloQBindery::PPObjStyloQBindery(void * extraPtr) : PPObject(PPOBJ_STYLOQBINDERY), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	ImplementFlags |= (implStrAssocMakeList);
}

PPObjStyloQBindery::~PPObjStyloQBindery()
{
	TLP_CLOSE(P_Tbl);
}

/*virtual*/int PPObjStyloQBindery::Edit(PPID * pID, void * extraPtr)
{
	return -1;
}

/*virtual*/int PPObjStyloQBindery::Browse(void * extraPtr)
{
	return -1;
}

/*virtual*/int PPObjStyloQBindery::Search(PPID id, void * b)
{
	return -1;
}

/*static*/PPIDArray & StyloQCore::MakeLinkObjTypeList(bool addUnaddignedObj, PPIDArray & rList)
{
	rList.Z().addzlist(PPOBJ_USR, PPOBJ_PERSON, PPOBJ_DBDIV, PPOBJ_CASHNODE, PPOBJ_PROCESSOR, 0L); // @v11.6.5 PPOBJ_PROCESSOR
	// @v11.4.5 {
	if(addUnaddignedObj)
		rList.add(PPOBJ_UNASSIGNED);
	// } @v11.4.5 
	return rList;
}
//
//
//
static int EditStyloQConfig(StyloQConfig & rData)
{
	class StyloQConfigDialog : public TDialog {
		DECL_DIALOG_DATA(StyloQConfig);
		PPAlbatrossConfig ACfg;
		bool   IsThereAlbatrossMqbParams;
		uint8  Reserve[3]; // @alignment
	public:
		StyloQConfigDialog() : TDialog(DLG_STQCFG), IsThereAlbatrossMqbParams(false)
		{
			if(CheckCfgRights(PPCFGOBJ_ALBATROS, PPR_READ, 0)) {
				if(PPAlbatrosCfgMngr::Get(&ACfg)) {
					SString temp_buf;
					ACfg.GetExtStrData(ALBATROSEXSTR_MQC_HOST, temp_buf);
					if(temp_buf.NotEmptyS())
						IsThereAlbatrossMqbParams = true;
				}
			}
			enableCommand(cmImport, IsThereAlbatrossMqbParams);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			RVALUEPTR(Data, pData);
			Data.Get(StyloQConfig::tagUrl, temp_buf);
			setCtrlString(CTL_STQCFG_URL, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQConfig::tagMqbAuth, temp_buf);
			setCtrlString(CTL_STQCFG_MQBAUTH, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQConfig::tagMqbSecret, temp_buf);
			setCtrlString(CTL_STQCFG_MQBSECR, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			// @v11.2.2 {
			Data.Get(StyloQConfig::tagLoclUrl, temp_buf);
			setCtrlString(CTL_STQCFG_LOCLURL, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQConfig::tagLoclMqbAuth, temp_buf);
			setCtrlString(CTL_STQCFG_LOCLMQBAUTH, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQConfig::tagLoclMqbSecret, temp_buf);
			setCtrlString(CTL_STQCFG_LOCLMQBSECR, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			//
			AddClusterAssoc(CTL_STQCFG_ROLE, 0, StyloQConfig::roleClient);
			AddClusterAssocDef(CTL_STQCFG_ROLE, 1, StyloQConfig::rolePublicService);
			AddClusterAssoc(CTL_STQCFG_ROLE, 2, StyloQConfig::rolePrivateService);
			AddClusterAssoc(CTL_STQCFG_ROLE, 3, StyloQConfig::roleDedicatedMediator); // @v11.2.12
			SetClusterData(CTL_STQCFG_ROLE, Data.GetRole());
			AddClusterAssoc(CTL_STQCFG_FLAGS, 0, StyloQConfig::featrfMediator);
			SetClusterData(CTL_STQCFG_FLAGS, static_cast<long>(Data.GetFeatures()));
			//
			//AddClusterAssoc(CTL_STQCFG_FEATURES, 0, StyloQConfig::featrfMediator);
			//SetClusterData(CTL_STQCFG_FEATURES, static_cast<long>(Data.GetFeatures()));
			// } @v11.2.2
			// @v11.2.3 {
			{
				Data.Get(StyloQConfig::tagExpiryPeriodSec, temp_buf);
				setCtrlLong(CTL_STQCFG_EXPIRYP, temp_buf.ToLong());
			}
			// } @v11.2.3 
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			getCtrlString(CTL_STQCFG_URL, temp_buf.Z());
			Data.Set(StyloQConfig::tagUrl, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQCFG_MQBAUTH, temp_buf.Z());
			Data.Set(StyloQConfig::tagMqbAuth, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQCFG_MQBSECR, temp_buf.Z());
			Data.Set(StyloQConfig::tagMqbSecret, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			// @v11.2.2 {
			getCtrlString(CTL_STQCFG_LOCLURL, temp_buf.Z());
			Data.Set(StyloQConfig::tagLoclUrl, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQCFG_LOCLMQBAUTH, temp_buf.Z());
			Data.Set(StyloQConfig::tagLoclMqbAuth, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQCFG_LOCLMQBSECR, temp_buf.Z());
			Data.Set(StyloQConfig::tagLoclMqbSecret, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			Data.SetRole(GetClusterData(CTL_STQCFG_ROLE)); // @v11.2.8
			{
				long flags = static_cast<long>(Data.GetFeatures());
				GetClusterData(CTL_STQCFG_FLAGS, &flags);
				Data.SetFeatures(flags);
			}
			// @v11.2.8 Data.SetFeatures(static_cast<uint64>(GetClusterData(CTL_STQCFG_FEATURES)));
			// } @v11.2.2 
			// @v11.2.3 {
			{
				long p = getCtrlLong(CTL_STQCFG_EXPIRYP);
				temp_buf.Z();
				if(p > 0)
					temp_buf.Cat(p);
				Data.Set(StyloQConfig::tagExpiryPeriodSec, temp_buf);
			}
			// } @v11.2.3 
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmClusterClk)) {
				if(event.isCtlEvent(CTL_STQCFG_ROLE)) {
					long new_role = GetClusterData(CTL_STQCFG_ROLE);
					Data.SetRole(new_role);
					const long org_f = GetClusterData(CTL_STQCFG_FLAGS);
					long new_f = static_cast<long>(Data.GetFeatures());
					if(new_f != org_f)
						SetClusterData(CTL_STQCFG_FLAGS, new_f);
				}
				else if(event.isCtlEvent(CTL_STQCFG_FLAGS)) {
					const long f = GetClusterData(CTL_STQCFG_FLAGS);
					Data.SetFeatures(static_cast<uint64>(f));
					long new_f = static_cast<long>(Data.GetFeatures());
					if(new_f != f)
						SetClusterData(CTL_STQCFG_FLAGS, new_f);
				}
			}
			else if(event.isCmd(cmImport)) {
				if(IsThereAlbatrossMqbParams) {
					SString temp_buf;
					ACfg.GetExtStrData(ALBATROSEXSTR_MQC_HOST, temp_buf);
					if(temp_buf.NotEmptyS()) {
						InetUrl url(temp_buf);
						if(url.GetProtocol() == 0)
							url.SetProtocol(InetUrl::protAMQP);
						url.Composite(0, temp_buf);
						setCtrlString(CTL_STQCFG_LOCLURL, temp_buf);
						ACfg.GetExtStrData(ALBATROSEXSTR_MQC_USER, temp_buf);
						setCtrlString(CTL_STQCFG_LOCLMQBAUTH, temp_buf);
						ACfg.GetPassword(ALBATROSEXSTR_MQC_SECRET, temp_buf);
						setCtrlString(CTL_STQCFG_LOCLMQBSECR, temp_buf);
					}
				}
			}
			else if(TVKEYDOWN) {
				if(TVKEY == kbF2) {
					if(isCurrCtlID(CTL_STQCFG_URL)) {
						if(IsThereAlbatrossMqbParams) {
							SString temp_buf;
							ACfg.GetExtStrData(ALBATROSEXSTR_MQC_HOST, temp_buf);
							if(temp_buf.NotEmptyS()) {
								InetUrl url(temp_buf);
								if(url.GetProtocol() == 0)
									url.SetProtocol(InetUrl::protAMQP);
								url.Composite(0, temp_buf);
								setCtrlString(CTL_STQCFG_URL, temp_buf);
								ACfg.GetExtStrData(ALBATROSEXSTR_MQC_USER, temp_buf);
								setCtrlString(CTL_STQCFG_MQBAUTH, temp_buf);
								ACfg.GetPassword(ALBATROSEXSTR_MQC_SECRET, temp_buf);
								setCtrlString(CTL_STQCFG_MQBSECR, temp_buf);
							}
						}
					}
				}
			}
		}
	};
	DIALOG_PROC_BODY(StyloQConfigDialog, &rData);
}

int PPObjStyloQBindery::EditConfig(PPID id)
{
	int    ok = -1;
	StyloQCore::StoragePacket pack;
	SString temp_buf;
	if(id && P_Tbl->GetPeerEntry(id, &pack) > 0) {
		const int _kind = pack.Rec.Kind;
		if(oneof2(_kind, StyloQCore::kNativeService, StyloQCore::kForeignService)) {
			bool    ex_item_got = false;
			uint32  tag_id = SSecretTagPool::tagConfig;
			StyloQConfig cfg_pack;
			SBinaryChunk bin_chunk;
			if(pack.Pool.Get(tag_id, &bin_chunk)) {
				if(cfg_pack.FromJson(bin_chunk.ToRawStr(temp_buf)))
					ex_item_got = true;
			}
			if(EditStyloQConfig(cfg_pack) > 0) {
				if(_kind == StyloQCore::kNativeService) {
					THROW(cfg_pack.ToJson(temp_buf));
					bin_chunk.Put(temp_buf, temp_buf.Len());
					pack.Pool.Put(tag_id, bin_chunk);
					THROW(P_Tbl->PutPeerEntry(&id, &pack, 1));
					ok = 1;
				}
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
struct StyloQAssignObjParam {
	StyloQAssignObjParam() : StqID(0), AutoMatched(false), CliFlags(0)
	{
	}
	PPID   StqID;
	PPObjID Oid;
	bool   AutoMatched;
	uint8  Reserve[3]; // @alignment
	long   CliFlags;
	SString EntryInfo;
};

class StyloQAssignObjDialog : public TDialog {
	DECL_DIALOG_DATA(StyloQAssignObjParam);
public:
	StyloQAssignObjDialog() : TDialog(DLG_STQCLIMATCH)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		SString temp_buf;
		PPIDArray obj_type_list;
		SetupObjListCombo(this, CTLSEL_STQCLIMATCH_OT, Data.Oid.Obj, &StyloQCore::MakeLinkObjTypeList(false, obj_type_list));
		SetupObjGroupCombo(Data.Oid.Obj);
		setCtrlString(CTL_STQCLIMATCH_INFO, Data.EntryInfo);
		// @v11.6.0 {
		AddClusterAssoc(CTL_STQCLIMATCH_CFLAGS, 0, StyloQConfig::clifSvcGPS);
		AddClusterAssoc(CTL_STQCLIMATCH_CFLAGS, 1, StyloQConfig::clifPsnAdrGPS);
		AddClusterAssoc(CTL_STQCLIMATCH_CFLAGS, 2, StyloQConfig::clifDisableFaceSelfModifying); // @v11.6.6
		SetClusterData(CTL_STQCLIMATCH_CFLAGS, Data.CliFlags);
		// } @v11.6.0 
		// @v11.5.10 {
		{
			if(Data.AutoMatched)
				PPLoadText(PPTXT_STQ_AUTOMATCHED, temp_buf);
			else
				temp_buf.Z();
			setCtrlString(CTL_STQCLIMATCH_AUTO, temp_buf);
		}
		// } @v11.5.10 
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTLSEL_STQCLIMATCH_OT, &Data.Oid.Obj);
		getCtrlData(CTLSEL_STQCLIMATCH_OBJ, &Data.Oid.Id);
		GetClusterData(CTL_STQCLIMATCH_CFLAGS, &Data.CliFlags); // @v11.6.0
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_STQCLIMATCH_OT)) {
			PPID new_obj_type = getCtrlLong(CTLSEL_STQCLIMATCH_OT);
			if(new_obj_type != Data.Oid.Obj) {
				Data.Oid.Obj = new_obj_type;
				Data.Oid.Id = 0;
				SetupObjGroupCombo(new_obj_type);
			}
		}
		else if(event.isCbSelected(CTLSEL_STQCLIMATCH_OG)) {
			if(Data.Oid.Obj) {
				long obj_group = getCtrlLong(CTLSEL_STQCLIMATCH_OG);
				SetupPPObjCombo(this, CTLSEL_STQCLIMATCH_OBJ, Data.Oid.Obj, Data.Oid.Id, 0, reinterpret_cast<void *>(obj_group));
			}
		}
		else
			return;
		clearEvent(event);
	}
	void   SetupObjGroupCombo(PPID objType)
	{
		if(objType == PPOBJ_PERSON) {
			disableCtrl(CTLSEL_STQCLIMATCH_OG, 0);
			SetupPPObjCombo(this, CTLSEL_STQCLIMATCH_OG, PPOBJ_PERSONKIND, 0, 0);
		}
		else {
			disableCtrl(CTLSEL_STQCLIMATCH_OG, 1);
		}
		SetupPPObjCombo(this, CTLSEL_STQCLIMATCH_OBJ, Data.Oid.Obj, Data.Oid.Id, 0);
	}
};

StyloQCore::SvcDbSymbMapEntry::SvcDbSymbMapEntry() : Flags(0)
{
}

int StyloQCore::SvcDbSymbMapEntry::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, SvcIdent, rBuf));
	THROW_SL(pSCtx->Serialize(dir, DbSymb, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Flags, rBuf));
	CATCHZOK
	return ok;
}

StyloQCore::SvcDbSymbMap::SvcDbSymbMap() : TSCollection <StyloQCore::SvcDbSymbMapEntry>(), LoadTime(0)
{
}

int FASTCALL StyloQCore::SvcDbSymbMap::Copy(const SvcDbSymbMap & rS)
{
	return TSCollection_Copy(*this, rS);
}

StyloQCore::SvcDbSymbMap::SvcDbSymbMap(const SvcDbSymbMap & rS)
{
	Copy(rS);
}

StyloQCore::SvcDbSymbMap & FASTCALL StyloQCore::SvcDbSymbMap::operator = (const SvcDbSymbMap & rS)
{
	Copy(rS);
	return *this;
}

bool StyloQCore::SvcDbSymbMap::HasDbUserAssocEntries(const char * pDbSymb) const
{
	bool ok = false;
	if(!isempty(pDbSymb)) {
		for(uint i = 0; i < getCount(); i++) {
			const StyloQCore::SvcDbSymbMapEntry * p_entry = at(i);
			if(p_entry && p_entry->DbSymb.IsEqiAscii(pDbSymb)) {
				if(p_entry->Flags & StyloQCore::SvcDbSymbMapEntry::fHasUserAssocEntries)
					ok = true;
				break;
			}
		}
	}
	return ok;
}

bool StyloQCore::SvcDbSymbMap::FindSvcIdent(const SBinaryChunk & rIdent, SString * pDbSymb, uint * pFlags) const
{
	bool ok = false;
	for(uint i = 0; !ok && i < getCount(); i++) {
		const StyloQCore::SvcDbSymbMapEntry * p_entry = at(i);
		if(p_entry && p_entry->SvcIdent == rIdent) {
			ASSIGN_PTR(pDbSymb, p_entry->DbSymb);
			ASSIGN_PTR(pFlags, p_entry->Flags);
			ok = true;
		}
	}
	return ok;
}

SString & StyloQCore::SvcDbSymbMap::InitFilePath(const char * pOuterPath, SString & rResultBuf)
{
	(rResultBuf = pOuterPath).Strip();
	if(rResultBuf.IsEmpty()) {
		PPGetFilePath(PPPATH_WORKSPACE, "styloq-sidsmap", rResultBuf);
	}
	return rResultBuf;
}

int StyloQCore::SvcDbSymbMap::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	const SVerT cur_ver = DS.GetVersion();
	if(dir > 0) {
		THROW_SL(rBuf.Write(&PPConst::Signature_StqDbSymbToSvcIdMap, sizeof(PPConst::Signature_StqDbSymbToSvcIdMap)));
		THROW_SL(rBuf.Write(&cur_ver, sizeof(cur_ver)));
	}
	else if(dir < 0) {
		SVerT ver;
		uint8 signature[sizeof(PPConst::Signature_StqDbSymbToSvcIdMap)];
		THROW_SL(rBuf.Read(signature, sizeof(signature)));
		THROW(memcmp(signature, &PPConst::Signature_StqDbSymbToSvcIdMap, sizeof(PPConst::Signature_StqDbSymbToSvcIdMap)) == 0);
		THROW_SL(rBuf.Read(&ver, sizeof(ver)));
	}
	THROW_SL(TSCollection_Serialize(*this, dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int StyloQCore::SvcDbSymbMap::Store(const char * pFilePath)
{
	int    ok = 1;
	SString file_path;
	InitFilePath(pFilePath, file_path);
	SSerializeContext sctx;
	SBuffer sbuf;
	THROW(Serialize(+1, sbuf, &sctx));
	{
		SFile f(file_path, SFile::mWrite|SFile::mBinary);
		THROW_SL(f.IsValid());
		THROW_SL(f.Write(sbuf.constptr(), sbuf.GetAvailableSize()));
	}
	CATCHZOK
	return ok;
}

int StyloQCore::SvcDbSymbMap::Read(const char * pFilePath, int loadTimeUsage)
{
	freeAll();
	int    ok = 1;
	if(loadTimeUsage > 0 && LoadTime > 0) {
		ok = -1;
	}
	else {
		SString file_path;
		InitFilePath(pFilePath, file_path);
		if(fileExists(file_path)) {
			SFileUtil::Stat fs;
			if(loadTimeUsage < 0 && LoadTime > 0 && SFileUtil::GetStat(file_path, &fs) && fs.ModTime.GetTimeT() < LoadTime) {
				ok = -1;
			}
			else {
				SSerializeContext sctx;
				SBuffer sbuf;
				SFile f(file_path, SFile::mRead|SFile::mBinary);
				THROW_SL(f.IsValid());
				{
					uint8  interm_buf[1024];
					size_t actual_size = 0;
					while(f.Read(interm_buf, sizeof(interm_buf), &actual_size) > 0) {
						if(actual_size) {
							THROW_SL(sbuf.Write(interm_buf, actual_size));
						}
						else
							break;
					}
				}
				THROW(Serialize(-1, sbuf, &sctx));
			}
		}
		else
			ok = -1;
		LoadTime = time(0);
	}
	CATCHZOK
	return ok;
}

/*static*/int StyloQCore::SvcDbSymbMap::Dump(const char * pInputFileName, const char * pDumpFileName)
{
	int    ok = 1;
	SString temp_buf;
	StyloQCore::SvcDbSymbMap map;
	map.InitFilePath(pInputFileName, temp_buf);
	THROW(map.Read(temp_buf, 0));
	(temp_buf = pDumpFileName).Strip();
	if(temp_buf.IsEmpty()) {
		PPGetFilePath(PPPATH_OUT, "styloq-sidsmap.dump", temp_buf);
	}
	{
		SFile f_dump(temp_buf, SFile::mWrite);
		SString mime_buf;
		THROW_SL(f_dump.IsValid());
		for(uint i = 0; i < map.getCount(); i++) {
			const StyloQCore::SvcDbSymbMapEntry * p_entry = map.at(i);
			if(p_entry) {
				mime_buf.EncodeMime64(p_entry->SvcIdent.PtrC(), p_entry->SvcIdent.Len());
				temp_buf.Z().Cat(p_entry->DbSymb).Tab().Cat(mime_buf).Tab().CatHex(p_entry->Flags).CR();
				f_dump.WriteLine(temp_buf);
			}
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int StyloQCore::ReadIgnitionServerList(TSCollection <IgnitionServerEntry> & rList)
{
	int    ok = -1;
	SString temp_buf;
	PPGetFilePath(PPPATH_BIN, "styloq_ignition_servers", temp_buf);
	if(fileExists(temp_buf)) {
		SFile  f_in(temp_buf, SFile::mRead);
		SString ident_buf;
		SString url_buf;
		SBinaryChunk bch;
		InetUrl url;
		THROW_SL(f_in.IsValid());
		while(f_in.ReadLine(temp_buf, SFile::rlfChomp|SFile::rlfStrip)) {
			if(temp_buf.Divide(' ', ident_buf, url_buf) > 0 && ident_buf.NotEmptyS() && url_buf.NotEmptyS()) {
				if(bch.FromMime64(ident_buf) && url.Parse(url_buf) && oneof2(url.GetProtocol(), InetUrl::protHttp, InetUrl::protHttps)) {
					IgnitionServerEntry * p_new_entry = rList.CreateNewItem();
					THROW_SL(p_new_entry);
					p_new_entry->Ident = bch;
					p_new_entry->Url = url_buf;
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int StyloQCore::BuildSvcDbSymbMap()
{
	int    ok = 1;
	SString db_symb;
	SString msg_buf;
	SBinaryChunk bc;
	SSerializeContext sctx;
	PPDbEntrySet2 dbes;
	DbLoginBlock dlb;
	PPSession::LimitedDatabaseBlock * p_ldb = 0;
	StyloQCore::SvcDbSymbMap map;
	PPLogMessage(PPFILNAM_INFO_LOG, PPLoadTextS(PPTXT_LOG_BUILDSVCDBSYMBMAPSTARTING, msg_buf), LOGMSGF_TIME|LOGMSGF_COMP);
	{
		PPIniFile ini_file;
		THROW(dbes.ReadFromProfile(&ini_file, 1, 1));
	}
	for(uint i = 0; i < dbes.GetCount(); i++) {
		if(dbes.GetByPos(i, &dlb)) {
			if(dlb.GetAttr(DbLoginBlock::attrDbSymb, db_symb) && db_symb.NotEmpty()) {
				p_ldb = DS.LimitedOpenDatabase(db_symb, PPSession::lodfReference|PPSession::lodfStyloQCore);
				if(!p_ldb)
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_COMP);
				else if(p_ldb->P_Sqc) {
					StyloQCore::StoragePacket sp;
					if(p_ldb->P_Sqc->GetOwnPeerEntry(&sp) <= 0) {
						PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_COMP);
					}
					else if(!sp.Pool.Get(SSecretTagPool::tagSvcIdent, &bc)) {
						PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_COMP);
					}
					else {
						assert(bc.Len() > 0);
						StyloQCore::SvcDbSymbMapEntry * p_entry = map.CreateNewItem();
						THROW_SL(p_entry);
						p_entry->DbSymb = db_symb;
						p_entry->SvcIdent = bc;
						{
							//
							// Выясняем существует ли хоть одна клиентская запись, ассоциированная с пользователем.
							// Эта информация понадобиться для принятия решения при выборе базы данных пользователем:
							// нужно ли отображать QR-код для авторизации с помощью смартфона.
							//
							StyloQSecTbl::Key1 k1;
							MEMSZERO(k1);
							k1.Kind = StyloQCore::kClient;
							if(p_ldb->P_Sqc->search(1, &k1, spGe) && p_ldb->P_Sqc->data.Kind == StyloQCore::kClient) do {
								StyloQSecTbl::Rec rec;
								p_ldb->P_Sqc->copyBufTo(&rec);
								if(rec.Kind == StyloQCore::kClient && rec.LinkObjType == PPOBJ_USR && rec.LinkObjID > 0) {
									p_entry->Flags |= StyloQCore::SvcDbSymbMapEntry::fHasUserAssocEntries;
									break;
								}
							} while(p_ldb->P_Sqc->search(1, &k1, spNext) && p_ldb->P_Sqc->data.Kind == StyloQCore::kClient);
							p_ldb->P_Sqc->destroyLobData(p_ldb->P_Sqc->VT); // @v11.4.9 @fix
						}
					}
				}
				ZDELETE(p_ldb);
			}
		}
	}
	THROW(map.Store(0));
	StyloQCore::SvcDbSymbMap::Dump(0, 0);
	PPLogMessage(PPFILNAM_INFO_LOG, PPLoadTextS(PPTXT_LOG_BUILDSVCDBSYMBMAPFINISHED, msg_buf), LOGMSGF_TIME|LOGMSGF_COMP);
	CATCH
		ok = 0;
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_COMP);
	ENDCATCH
	delete p_ldb;
	return ok;
}

int PPObjStyloQBindery::AssignObjToClientEntry(PPID id)
{
	int    ok = -1;
	SString temp_buf;
	StyloQAssignObjParam param;
	StyloQCore::StoragePacket pack;
	if(id && P_Tbl->GetPeerEntry(id, &pack) > 0) {
		if(pack.Rec.Kind == StyloQCore::kClient) {
			const uint cfg_tag = SSecretTagPool::tagPrivateConfig;
			param.StqID = pack.Rec.ID;
			param.Oid.Set(pack.Rec.LinkObjType, pack.Rec.LinkObjID);
			param.EntryInfo.EncodeMime64(pack.Rec.BI, sizeof(pack.Rec.BI));
			param.AutoMatched = LOGIC(pack.Rec.Flags & StyloQCore::styloqfAutoObjMatching); // @v11.5.10

			StyloQConfig cfg_pack;
			SBinaryChunk bin_chunk;
			if(pack.Pool.Get(cfg_tag, &bin_chunk)) {
				if(cfg_pack.FromJson(bin_chunk.ToRawStr(temp_buf))) {
					if(cfg_pack.Get(StyloQConfig::tagCliFlags, temp_buf))
						param.CliFlags = temp_buf.ToLong();
				}
			}
			if(PPDialogProcBody <StyloQAssignObjDialog, StyloQAssignObjParam> (&param) > 0) {
				pack.Rec.LinkObjType = param.Oid.Obj;
				pack.Rec.LinkObjID = param.Oid.Id;
				{
					temp_buf.Z();
					if(param.CliFlags)
						temp_buf.Cat(param.CliFlags);
					cfg_pack.Set(StyloQConfig::tagCliFlags, temp_buf);
					if(cfg_pack.ToJson(temp_buf)) {
						bin_chunk.Put(temp_buf, temp_buf.Len());
						pack.Pool.Put(cfg_tag, bin_chunk);
					}
				}
				if(P_Tbl->PutPeerEntry(&id, &pack, 1))
					ok = 1;
				else
					ok = PPErrorZ();
			}
		}
	}
	return ok;
}

static int EditStyloQFace(StyloQFace & rData)
{
	class StyloQFaceDialog : public TDialog {
		DECL_DIALOG_DATA(StyloQFace);
		enum {
			ctlgroupIBG = 1
		};
	public:
		StyloQFaceDialog() : TDialog(DLG_STQFACE)
		{
			addGroup(ctlgroupIBG, new ImageBrowseCtrlGroup(CTL_STQFACE_IMAGE, cmAddImage, cmDelImage, 1));
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			RVALUEPTR(Data, pData);
			{
				ImageBrowseCtrlGroup::Rec rec;
				Data.GetImage(&rec.ImgBuf, 0);
				rec.Flags |= ImageBrowseCtrlGroup::Rec::fImageBuffer;
				//Data.LinkFiles.Init(PPOBJ_GOODS);
				//Data.LinkFiles.Load(Data.Rec.ID, 0L);
				//Data.LinkFiles.At(0, rec.Path);
				setGroupData(ctlgroupIBG, &rec);
			}
			SetupPage();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			ok = GetPage();
			if(ok) {
				{
					ImageBrowseCtrlGroup::Rec rec;
					getGroupData(ctlgroupIBG, &rec);
					if(rec.Flags & ImageBrowseCtrlGroup::Rec::fImageBuffer) {
						Data.SetImage(0, &rec.ImgBuf);
					}
				}
				ASSIGN_PTR(pData, Data);
			}
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
		}
		void SetGeoCoord()
		{
			SString temp_buf;
			SGeoPosLL pos;
			if(Data.GetGeoLoc(pos) > 0)
				pos.ToStr(temp_buf);
			setCtrlString(CTL_STQFACE_GEOLOC, temp_buf);
		}
		void GetGeoCoord()
		{
			SGeoPosLL pos;
			SString temp_buf;
			getCtrlString(CTL_STQFACE_GEOLOC, temp_buf);
			if(pos.FromStr(temp_buf) > 0) {
				Data.SetGeoLoc(pos);
			}
		}
		void SetupPage()
		{
			SString temp_buf;
			long lang = getCtrlLong(CTLSEL_STQFACE_LANG);
			// @v11.3.2 {
			{
				AddClusterAssocDef(CTL_STQFACE_VRF, 0, StyloQFace::vArbitrary);
				AddClusterAssoc(CTL_STQFACE_VRF, 1, StyloQFace::vAnonymous);
				AddClusterAssoc(CTL_STQFACE_VRF, 2, StyloQFace::vVerifiable);
				SetClusterData(CTL_STQFACE_VRF, Data.GetVerifiability());
			}
			// } @v11.3.2
			Data.Get(StyloQFace::tagCommonName, lang, temp_buf);
			setCtrlString(CTL_STQFACE_CN, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQFace::tagName, lang, temp_buf);
			setCtrlString(CTL_STQFACE_NAME, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQFace::tagPatronymic, lang, temp_buf);
			setCtrlString(CTL_STQFACE_PATRONYM, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQFace::tagSurName, lang, temp_buf);
			setCtrlString(CTL_STQFACE_SURNAME, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQFace::tagCountryName, lang, temp_buf);
			setCtrlString(CTL_STQFACE_COUNTRY, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQFace::tagZIP, lang, temp_buf);
			setCtrlString(CTL_STQFACE_ZIP, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQFace::tagCityName, lang, temp_buf);
			setCtrlString(CTL_STQFACE_CITY, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQFace::tagStreet, lang, temp_buf);
			setCtrlString(CTL_STQFACE_STREET, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQFace::tagAddress, lang, temp_buf);
			setCtrlString(CTL_STQFACE_ADDR, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQFace::tagPhone, lang, temp_buf);
			setCtrlString(CTL_STQFACE_PHONE, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQFace::tagGLN, lang, temp_buf);
			setCtrlString(CTL_STQFACE_GLN, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQFace::tagRuINN, lang, temp_buf);
			setCtrlString(CTL_STQFACE_RUINN, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQFace::tagRuKPP, lang, temp_buf);
			setCtrlString(CTL_STQFACE_RUKPP, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			Data.Get(StyloQFace::tagRuSnils, lang, temp_buf);
			setCtrlString(CTL_STQFACE_RUSNILS, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			LDATE dob = Data.GetDob();
			setCtrlDate(CTL_STQFACE_DOB, dob);
			SetGeoCoord();
			{
				Data.Get(StyloQFace::tagExpiryPeriodSec, 0, temp_buf);
				setCtrlLong(CTL_STQFACE_EXPIRYP, temp_buf.ToLong());
			}
		}
		int GetPage()
		{
			int    ok = 1;
			SString temp_buf;
			long lang = getCtrlLong(CTLSEL_STQFACE_LANG);
			Data.SetVerifiability(GetClusterData(CTL_STQFACE_VRF)); // @v11.3.2
			getCtrlString(CTL_STQFACE_CN, temp_buf.Z());
			Data.Set(StyloQFace::tagCommonName, lang, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQFACE_NAME, temp_buf.Z());
			Data.Set(StyloQFace::tagName, lang, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQFACE_PATRONYM, temp_buf.Z());
			Data.Set(StyloQFace::tagPatronymic, lang, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQFACE_SURNAME, temp_buf.Z());
			Data.Set(StyloQFace::tagSurName, lang, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQFACE_COUNTRY, temp_buf.Z());
			Data.Set(StyloQFace::tagCountryName, lang, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQFACE_ZIP, temp_buf.Z());
			Data.Set(StyloQFace::tagZIP, lang, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQFACE_CITY, temp_buf.Z());
			Data.Set(StyloQFace::tagCityName, lang, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQFACE_STREET, temp_buf.Z());
			Data.Set(StyloQFace::tagStreet, lang, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQFACE_ADDR, temp_buf.Z());
			Data.Set(StyloQFace::tagAddress, lang, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQFACE_PHONE, temp_buf.Z());
			Data.Set(StyloQFace::tagPhone, lang, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQFACE_GLN, temp_buf.Z());
			Data.Set(StyloQFace::tagGLN, lang, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQFACE_RUINN, temp_buf.Z());
			Data.Set(StyloQFace::tagRuINN, lang, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQFACE_RUKPP, temp_buf.Z());
			Data.Set(StyloQFace::tagRuKPP, lang, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			getCtrlString(CTL_STQFACE_RUSNILS, temp_buf.Z());
			Data.Set(StyloQFace::tagRuSnils, lang, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			LDATE dob = getCtrlDate(CTL_STQFACE_DOB);
			Data.SetDob(dob);
			GetGeoCoord();
			{
				const long p = getCtrlLong(CTL_STQFACE_EXPIRYP);
				temp_buf.Z();
				if(p > 0)
					temp_buf.Cat(p);
				Data.Set(StyloQFace::tagExpiryPeriodSec, 0, temp_buf);
			}
			return ok;
		}
	};
	DIALOG_PROC_BODY(StyloQFaceDialog, &rData);
}

int PPObjStyloQBindery::EditFace(PPID id)
{
	int    ok = -1;
	StyloQAssignObjParam param;
	StyloQCore::StoragePacket pack;
	SString temp_buf;
	if(id && P_Tbl->GetPeerEntry(id, &pack) > 0) {
		if(oneof4(pack.Rec.Kind, StyloQCore::kClient, StyloQCore::kNativeService, StyloQCore::kForeignService, StyloQCore::kFace)) {
			StyloQFace face_pack;
			SBinaryChunk bin_chunk;
			bool    ex_item_got = false;
			uint32  tag_id = 0;
			if(oneof2(pack.Rec.Kind, StyloQCore::kClient, StyloQCore::kForeignService))
				tag_id = SSecretTagPool::tagFace;
			else 
				tag_id = SSecretTagPool::tagSelfyFace;
			if(pack.Pool.Get(tag_id, &bin_chunk)) {
				if(face_pack.FromJson(bin_chunk.ToRawStr(temp_buf)))
					ex_item_got = true;
			}
			if(EditStyloQFace(face_pack) > 0) {
				SBinarySet::DeflateStrategy ds(512);
				THROW(face_pack.ToJson(false, temp_buf));
				bin_chunk.Put(temp_buf, temp_buf.Len());
				pack.Pool.Put(tag_id, bin_chunk, &ds);
				THROW(P_Tbl->PutPeerEntry(&id, &pack, 1));
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	return ok;	
}
//
//
//
PPStyloQInterchange::ServerParamBase::ServerParamBase() : Capabilities(0)
{
}

PPStyloQInterchange::ServerParamBase & PPStyloQInterchange::ServerParamBase::Z()
{
	Capabilities = 0;
	SvcIdent.Z();
	LoclAddendum.Z(); // @v11.2.3
	AccessPoint.Z();
	return *this;
}

PPStyloQInterchange::RunServerParam::RunServerParam() : ServerParamBase()
{
}

PPStyloQInterchange::RunServerParam & PPStyloQInterchange::RunServerParam::Z()
{
	ServerParamBase::Z();
	return *this;
}

uint PPStyloQInterchange::RunServerParam::MakeMqbQueueIdent(SBinaryChunk & rResult) const
{
	rResult.Z();
	if(SvcIdent.Len()) {
		rResult.Cat(SvcIdent.PtrC(), SvcIdent.Len());
		if(LoclAddendum.Len())
			rResult.Cat(LoclAddendum.PtrC(), LoclAddendum.Len());
	}
	return static_cast<uint>(rResult.Len());
}

PPStyloQInterchange::InterchangeParam::InterchangeParam() : ServerParamBase()
{
}

PPStyloQInterchange::InterchangeParam::InterchangeParam(const RunServerParam & rRsP) : ServerParamBase(rRsP)
{
}

void PPStyloQInterchange::InterchangeParam::ClearParam()
{
	CommandJson.Z();
	Blob.Z();	
}
		
PPStyloQInterchange::InterchangeParam & PPStyloQInterchange::InterchangeParam::Z()
{
	ServerParamBase::Z();
	ClearParam();
	return *this;
}

PPStyloQInterchange::DocumentDeclaration::DocumentDeclaration(const StyloQCommandList::Item * pCmdItem, const char * pDl600Symb) : 
	Dtm(ZERODATETIME), ActionFlags(0), ResultExpiryTimeSec(pCmdItem ? pCmdItem->ResultExpiryTimeSec : 0)
{
	if(pCmdItem) {
		switch(pCmdItem->BaseCmdId) {
			case StyloQCommandList::sqbcReport:
				Type = "view";
				DisplayMethSymb = "grid";
				break;
			case StyloQCommandList::sqbcSearch:
				Type = "search";
				DisplayMethSymb = "search";
				break;
			case StyloQCommandList::sqbcRsrvOrderPrereq:
				Type = "orderprereq";
				DisplayMethSymb = "orderprereq";
				break;
			case StyloQCommandList::sqbcRsrvAttendancePrereq:
				Type = "attendanceprereq";
				DisplayMethSymb = "attendanceprereq";
				break;
			case StyloQCommandList::sqbcRsrvIndoorSvcPrereq:
				Type = "indoorsvcprereq";
				DisplayMethSymb = "indoorsvcprereq";
				break;
			case StyloQCommandList::sqbcIncomingListOrder: // @v11.4.7
				Type = "incominglistorder";
				DisplayMethSymb = "incominglistorder";
				break;
			case StyloQCommandList::sqbcIncomingListCCheck: // @v11.4.7
				Type = "incominglistccheck";
				DisplayMethSymb = "incominglistccheck";
				break;
			case StyloQCommandList::sqbcIncomingListTSess: // @v11.4.7
				Type = "incominglisttsess";
				DisplayMethSymb = "incominglisttsess";
				break;
			case StyloQCommandList::sqbcIncomingListTodo: // @v11.4.7
				Type = "incominglisttodo";
 				DisplayMethSymb = "incominglisttodo";
				break;			
			case StyloQCommandList::sqbcDebtList: // @v11.5.4
				Type = "debtlist";
 				DisplayMethSymb = "debtlist";
				break;
		}	
		assert(Type.NotEmpty());
		assert(DisplayMethSymb.NotEmpty());
		if(pCmdItem->BaseCmdId == StyloQCommandList::sqbcReport) {
			ViewSymb = pCmdItem->ViewSymb;
			Dl600Symb = pDl600Symb;
		}
	}
}

PPStyloQInterchange::DocumentDeclaration & PPStyloQInterchange::DocumentDeclaration::Z()
{
	Dtm.Z();
	ActionFlags = 0;
	ResultExpiryTimeSec = 0;
	Type.Z();
	Format.Z();
	DisplayMethSymb.Z();
	ViewSymb.Z();
	Dl600Symb.Z();
	return *this;
}

int PPStyloQInterchange::DocumentDeclaration::FromJsonObject(const SJson * pJsObj)
{
	int    ok = 1;
	SString temp_buf;
	THROW(SJson::IsObject(pJsObj));
	for(const SJson * p_cur = pJsObj->P_Child; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->P_Child) {
			if(p_cur->Text.IsEqiAscii("type")) {
				(Type = p_cur->P_Child->Text).Unescape();
			}
			else if(p_cur->Text.IsEqiAscii("displaymethod")) {
				(DisplayMethSymb = p_cur->P_Child->Text).Unescape();
			}
			else if(p_cur->Text.IsEqiAscii("viewsymb")) {
				(ViewSymb = p_cur->P_Child->Text).Unescape();
			}
			else if(p_cur->Text.IsEqiAscii("dl600symb")) {
				(Dl600Symb = p_cur->P_Child->Text).Unescape();
			}
			else if(p_cur->Text.IsEqiAscii("format")) {
				(Format = p_cur->P_Child->Text).Unescape();
			}
			else if(p_cur->Text.IsEqiAscii("time")) {
				temp_buf = p_cur->P_Child->Text;
				Dtm.Set(temp_buf, DATF_ISO8601CENT, TIMF_HMS);
			}
			else if(p_cur->Text.IsEqiAscii("resultexpirytimesec")) {
				ResultExpiryTimeSec = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("actionflags")) {
				ActionFlags = Document::IncomingListActionsFromString(p_cur->P_Child->Text);
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::DocumentDeclaration::FromJson(const char * pJson)
{
	int    ok = 1;
	SJson * p_js = 0;
	Z();
	if(!isempty(pJson)) {
		THROW_SL(p_js = SJson::Parse(pJson));
		THROW(FromJsonObject(p_js));
	}
	else
		ok = -1;
	CATCHZOK
	delete p_js;
	return ok;
}

SJson * PPStyloQInterchange::DocumentDeclaration::ToJsonObject() const
{
	SJson * p_result = SJson::CreateObj();
	SString temp_buf;
	{
		p_result->InsertStringNe("type", Type);
		p_result->InsertStringNe("displaymethod", DisplayMethSymb);
		p_result->InsertStringNe("viewsymb", ViewSymb);
		p_result->InsertStringNe("dl600symb", Dl600Symb);
	}
	p_result->InsertString("format", Format.NotEmpty() ? Format.cptr() : "json");
	p_result->InsertString("time", temp_buf.Z().CatCurDateTime(DATF_ISO8601CENT, 0));
	if(ResultExpiryTimeSec > 0)
		p_result->InsertInt("resultexpirytimesec", ResultExpiryTimeSec);
	p_result->InsertStringNe("actionflags", Document::IncomingListActionsToString(ActionFlags, temp_buf));
	return p_result;
}

int PPStyloQInterchange::DocumentDeclaration::ToJson(SString & rResult) const
{
	int    ok = 1;
	SJson * p_js = ToJsonObject();
	if(p_js)
		p_js->ToStr(rResult);
	else {
		rResult.Z();
		ok = 0;
	}
	delete p_js;
	return ok;
}

bool FASTCALL PPStyloQInterchange::InterchangeParam::IsEq(const PPStyloQInterchange::InterchangeParam & rS) const
{
	return (Capabilities == rS.Capabilities && SvcIdent == rS.SvcIdent && LoclAddendum == rS.LoclAddendum && AccessPoint == rS.AccessPoint && 
		CommandJson == rS.CommandJson && Blob == rS.Blob);
}

PPStyloQInterchange::Document::LotExtCode::LotExtCode() : Flags(0), BoxRefN(0)
{
	Code[0] = 0;
}

PPStyloQInterchange::Document::ValuSet::ValuSet() : Qtty(0.0), Cost(0.0), Price(0.0), Discount(0.0)
{
}

PPStyloQInterchange::Document::__TransferItem::__TransferItem() : RowIdx(0), GoodsID(0), UnitID(0), Flags(0), CcQueue(0)
{
}

PPStyloQInterchange::Document::BookingItem::BookingItem() : RowIdx(0), PrcID(0), GoodsID(0), Flags(0), ReqTime(ZERODATETIME), EstimatedDurationSec(0)
{
}

/*static*/uint PPStyloQInterchange::Document::IncomingListActionsFromString(const char * pStr)
{
	uint   result = 0;
	if(!isempty(pStr)) {
		StringSet ss(',', pStr);
		SString temp_buf;
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			temp_buf.Strip();
			if(temp_buf.IsEqiAscii("DocStatus"))
				result |= StyloQIncomingListParam::actionDocStatus;
			else if(temp_buf.IsEqiAscii("DocAcceptance"))
				result |= StyloQIncomingListParam::actionDocAcceptance;
			else if(temp_buf.IsEqiAscii("DocAcceptanceMarks"))
				result |= StyloQIncomingListParam::actionDocAcceptanceMarks;
			else if(temp_buf.IsEqiAscii("DocSettingMarks"))
				result |= StyloQIncomingListParam::actionDocSettingMarks;
			else if(temp_buf.IsEqiAscii("DocInventory"))
				result |= StyloQIncomingListParam::actionDocInventory;
			else if(temp_buf.IsEqiAscii("GoodsItemCorrection"))
				result |= StyloQIncomingListParam::actionGoodsItemCorrection;
			// @v11.5.2 {
			else if(temp_buf.IsEqiAscii("CCheckCreat"))
				result |= StyloQIncomingListParam::actionCCheckCreat;
			else if(temp_buf.IsEqiAscii("CCheckMod"))
				result |= StyloQIncomingListParam::actionCCheckMod;
			else if(temp_buf.IsEqiAscii("CCheckRegPrint"))
				result |= StyloQIncomingListParam::actionCCheckRegPrint;
			// } @v11.5.2 
		}
	}
	return result;
}

/*static*/SString & PPStyloQInterchange::Document::IncomingListActionsToString(uint actionFlags, SString & rBuf)
{
	rBuf.Z();
	if(actionFlags & StyloQIncomingListParam::actionDocStatus)
		rBuf.CatDivIfNotEmpty(',', 0).Cat("DocStatus");
	if(actionFlags & StyloQIncomingListParam::actionDocAcceptance)
		rBuf.CatDivIfNotEmpty(',', 0).Cat("DocAcceptance");
	if(actionFlags & StyloQIncomingListParam::actionDocAcceptanceMarks)
		rBuf.CatDivIfNotEmpty(',', 0).Cat("DocAcceptanceMarks");
	if(actionFlags & StyloQIncomingListParam::actionDocSettingMarks)
		rBuf.CatDivIfNotEmpty(',', 0).Cat("DocSettingMarks");
	if(actionFlags & StyloQIncomingListParam::actionDocInventory)
		rBuf.CatDivIfNotEmpty(',', 0).Cat("DocInventory");
	if(actionFlags & StyloQIncomingListParam::actionGoodsItemCorrection)
		rBuf.CatDivIfNotEmpty(',', 0).Cat("GoodsItemCorrection");
	// @v11.5.2 {
	if(actionFlags & StyloQIncomingListParam::actionCCheckCreat)
		rBuf.CatDivIfNotEmpty(',', 0).Cat("CCheckCreat");
	if(actionFlags & StyloQIncomingListParam::actionCCheckMod)
		rBuf.CatDivIfNotEmpty(',', 0).Cat("CCheckMod");
	if(actionFlags & StyloQIncomingListParam::actionCCheckRegPrint)
		rBuf.CatDivIfNotEmpty(',', 0).Cat("CCheckRegPrint");
	// } @v11.5.2 
	// @v11.6.5 {
	if(actionFlags & StyloQIncomingListParam::actionTSessCreat)
		rBuf.CatDivIfNotEmpty(',', 0).Cat("TSessCreat");
	if(actionFlags & StyloQIncomingListParam::actionTSessTmSet)
		rBuf.CatDivIfNotEmpty(',', 0).Cat("TSessTmSet");
	// } @v11.6.5 
	return rBuf;
}

PPStyloQInterchange::Document::Document() : ID(0), CreationTime(ZERODATETIME), Time(ZERODATETIME), DueTime(ZERODATETIME), InterchangeOpID(0), SvcOpType(0), SvcOpID(0),
	ClientID(0), DlvrLocID(0), AgentID(0), QuotKindID(0), PosNodeID(0), StatusSurrId(0), Amount(0.0)
{
}

PPStyloQInterchange::Document & PPStyloQInterchange::Document::Z()
{
	ID = 0;
	CreationTime.Z();
	Time.Z();
	DueTime.Z();
	CreationGeoLoc.Z(); // @v11.6.2
	InterchangeOpID = 0;
	SvcOpType = 0; // @v11.7.2
	SvcOpID = 0;
	ClientID = 0;
	DlvrLocID = 0;
	AgentID = 0;
	QuotKindID = 0; // @v11.7.1
	PosNodeID = 0;
	StatusSurrId = 0; // @v11.5.1
	Amount = 0.0;
	Code.Z();
	BaseCurrencySymb.Z();
	SvcIdent.Z();
	Uuid.Z();
	OrgCmdUuid.Z();
	Memo.Z();
	TiList.freeAll();
	BkList.freeAll();
	VXcL.clear();
	return *this;
}

SJson * PPStyloQInterchange::Document::ToJsonObject() const
{
	SJson * p_result = SJson::CreateObj();
	SString temp_buf;
	p_result->InsertInt64("id", ID);
	if(!!Uuid) {
		temp_buf.Z().Cat(Uuid, S_GUID::fmtIDL);
		p_result->InsertString("uuid", temp_buf);
	}
	if(!!OrgCmdUuid) {
		temp_buf.Z().Cat(OrgCmdUuid, S_GUID::fmtIDL);
		p_result->InsertString("orgcmduuid", temp_buf);
	}
	if(Code.NotEmpty()) {
		p_result->InsertString("code", temp_buf.Z().Cat(Code).Escape());
	}
	if(SvcIdent.Len()) {
		p_result->InsertString("svcident", SvcIdent.Mime64(temp_buf).Escape());
	}
	if(BaseCurrencySymb.NotEmpty()) {
		p_result->InsertString("basecurrency", temp_buf.Z().Cat(BaseCurrencySymb).Escape());
	}
	if(!!CreationTime) {
		temp_buf.Z().Cat(CreationTime, DATF_ISO8601CENT, 0);
		p_result->InsertString("crtm", temp_buf);
	}
	if(!!Time) {
		temp_buf.Z().Cat(Time, DATF_ISO8601CENT, 0);
		p_result->InsertString("tm", temp_buf);
	}
	if(!!DueTime) {
		temp_buf.Z().Cat(DueTime, DATF_ISO8601CENT, 0);
		p_result->InsertString("duetm", temp_buf);
	}
	// @v11.6.2 {
	if(CreationGeoLoc.IsValid() && !CreationGeoLoc.IsZero()) {
		p_result->InsertDouble("cr_lat", CreationGeoLoc.Lat, MKSFMTD(0, 12, NMBF_NOTRAILZ));
		p_result->InsertDouble("cr_lon", CreationGeoLoc.Lon, MKSFMTD(0, 12, NMBF_NOTRAILZ));
	}
	// } @v11.6.2 
	p_result->InsertIntNz("svcoptype", SvcOpType); // @v11.7.2
	p_result->InsertIntNz("svcopid", SvcOpID);
	p_result->InsertIntNz("icopid", InterchangeOpID);
	p_result->InsertIntNz("posnodeid", PosNodeID); // @v11.4.6
	p_result->InsertIntNz("statussurrid", StatusSurrId); // @v11.5.1
	p_result->InsertIntNz("agentid", AgentID); // @v11.4.6
	p_result->InsertIntNz("cliid", ClientID);
	p_result->InsertIntNz("dlvrlocid", DlvrLocID);
	p_result->InsertIntNz("quotkindid", QuotKindID); // @v11.7.1
	p_result->InsertDouble("amount", Amount, MKSFMTD(0, 5, NMBF_NOTRAILZ|NMBF_OMITEPS));
	if(Memo.NotEmpty()) {
		p_result->InsertString("memo", temp_buf.Z().Cat(Memo).Escape());
	}
	if(TiList.getCount()) {
		SJson * p_js_list = SJson::CreateArr();
		bool is_list_empty = true;
		for(uint i = 0; i < TiList.getCount(); i++) {
			const __TransferItem * p_ti = TiList.at(i);
			if(p_ti) {
				SJson * p_js_item = SJson::CreateObj();
				p_js_item->InsertInt("rowidx", p_ti->RowIdx);
				if(p_ti->GoodsID > 0) {
					p_js_item->InsertInt("goodsid", p_ti->GoodsID);
					if(p_ti->UnitID > 0)
						p_js_item->InsertInt("unitid", p_ti->UnitID);
					p_js_item->InsertInt("flags", p_ti->Flags);
					{
						SJson * p_js_ti_set = SJson::CreateObj();
						bool is_empty = true;
						if(p_ti->Set.Qtty != 0.0) {
							p_js_ti_set->InsertDouble("qtty", p_ti->Set.Qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ));
							is_empty = false;
						}
						if(p_ti->Set.Cost != 0.0) {
							p_js_ti_set->InsertDouble("cost", p_ti->Set.Cost, MKSFMTD(0, 2, 0));
							is_empty = false;
						}
						if(p_ti->Set.Price != 0.0) {
							p_js_ti_set->InsertDouble("price", p_ti->Set.Price, MKSFMTD(0, 2, 0));
							is_empty = false;
						}
						if(p_ti->Set.Discount != 0.0) {
							p_js_ti_set->InsertDouble("discount", p_ti->Set.Discount, MKSFMTD(0, 5, NMBF_NOTRAILZ));
							is_empty = false;
						}
						if(is_empty) {
							ZDELETE(p_js_ti_set);
						}
						else
							p_js_item->Insert("set", p_js_ti_set);
					}
					// @v11.6.12 {
					{
						SJson * p_js_ti_set = SJson::CreateObj();
						bool is_empty = true;
						if(p_ti->SetAccepted.Qtty != 0.0) {
							p_js_ti_set->InsertDouble("qtty", p_ti->SetAccepted.Qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ));
							is_empty = false;
						}
						if(p_ti->SetAccepted.Cost != 0.0) {
							p_js_ti_set->InsertDouble("cost", p_ti->SetAccepted.Cost, MKSFMTD(0, 2, 0));
							is_empty = false;
						}
						if(p_ti->SetAccepted.Price != 0.0) {
							p_js_ti_set->InsertDouble("price", p_ti->SetAccepted.Price, MKSFMTD(0, 2, 0));
							is_empty = false;
						}
						if(p_ti->SetAccepted.Discount != 0.0) {
							p_js_ti_set->InsertDouble("discount", p_ti->SetAccepted.Discount, MKSFMTD(0, 5, NMBF_NOTRAILZ));
							is_empty = false;
						}
						if(is_empty) {
							ZDELETE(p_js_ti_set);
						}
						else
							p_js_item->Insert("setaccepted", p_js_ti_set);
					}
					// } @v11.6.12 
					if(p_ti->XcL.getCount()) {
						SJson * p_js_xcl = 0;
						for(uint xci = 0; xci < p_ti->XcL.getCount(); xci++) {
							const LotExtCode & r_xci = p_ti->XcL.at(xci);
							if(r_xci.Code[0]) {
								SJson * p_js_xci = SJson::CreateObj();
								p_js_xci->InsertString("cod", (temp_buf = r_xci.Code).Escape());
								p_js_xci->InsertIntNz("boxrefn", r_xci.BoxRefN);
								p_js_xci->InsertIntNz("flags", r_xci.Flags);
								SETIFZ(p_js_xcl, SJson::CreateArr());
								p_js_xcl->InsertChild(p_js_xci);
							}
						}
						p_js_item->InsertNz("xcl", p_js_xcl);
					}
					p_js_list->InsertChild(p_js_item);
					is_list_empty = false;
				}
			}
		}
		if(is_list_empty) {
			ZDELETE(p_js_list);
		}
		else {
			p_result->Insert("ti_list", p_js_list);
		}
		if(VXcL.getCount()) {
			SJson * p_js_xcl = 0;
			for(uint xci = 0; xci < VXcL.getCount(); xci++) {
				const LotExtCode & r_xci = VXcL.at(xci);
				if(r_xci.Code[0]) {
					SJson * p_js_xci = SJson::CreateObj();
					p_js_xci->InsertString("cod", (temp_buf = r_xci.Code).Escape());
					p_js_xci->InsertIntNz("boxrefn", r_xci.BoxRefN);
					p_js_xci->InsertIntNz("flags", r_xci.Flags);
					SETIFZ(p_js_xcl, SJson::CreateArr());
					p_js_xcl->InsertChild(p_js_xci);
				}
			}
			p_result->InsertNz("vxcl", p_js_xcl);
		}
	}
	if(BkList.getCount()) {
		SJson * p_js_list = SJson::CreateArr();
		bool is_list_empty = true;
		for(uint i = 0; i < BkList.getCount(); i++) {
			const BookingItem * p_bi = BkList.at(i);
			if(p_bi) {
				SJson * p_js_item = SJson::CreateObj();
				p_js_item->InsertInt("rowidx", p_bi->RowIdx);
				p_js_item->InsertInt("prcid", p_bi->PrcID);
				p_js_item->InsertInt("goodsid", p_bi->GoodsID);
				p_js_item->InsertInt("flags", p_bi->Flags);
				if(!!p_bi->ReqTime) {
					p_js_item->InsertString("reqtime", temp_buf.Z().Cat(p_bi->ReqTime, DATF_ISO8601CENT, 0));
				}
				if(p_bi->EstimatedDurationSec > 0) {
					p_js_item->InsertInt("estimateddurationsec", p_bi->EstimatedDurationSec);
				}
				{
					SJson * p_js_set = SJson::CreateObj();
					bool is_empty = true;
					if(p_bi->Set.Qtty > 0.0) {
						p_js_set->InsertDouble("qtty", p_bi->Set.Qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ));
						is_empty = false;
					}
					if(p_bi->Set.Cost > 0.0) {
						p_js_set->InsertDouble("cost", p_bi->Set.Cost, MKSFMTD(0, 2, 0));
						is_empty = false;
					}
					if(p_bi->Set.Price > 0.0) {
						p_js_set->InsertDouble("price", p_bi->Set.Price, MKSFMTD(0, 2, 0));
						is_empty = false;
					}
					if(is_empty) {
						ZDELETE(p_js_set);
					}
					else
						p_js_item->Insert("set", p_js_set);
				}
				p_js_item->InsertStringNe("memo", (temp_buf = p_bi->Memo).Strip().Escape());
				p_js_list->InsertChild(p_js_item);
				is_list_empty = false;
			}
		}
		if(is_list_empty) {
			ZDELETE(p_js_list);
		}
		else {
			p_result->Insert("bk_list", p_js_list);
		}
	}
	//CATCH
		//ZDELETE(p_result);
	//ENDCATCH
	return p_result;
}

int PPStyloQInterchange::Document::FromJsonObject(const SJson * pJsObj)
{
	int    ok = 1;
	bool   occured_crgeoloc_lat = false;
	bool   occured_crgeoloc_lon = false;
	SString temp_buf;
	THROW(SJson::IsObject(pJsObj));
	for(const SJson * p_cur = pJsObj->P_Child; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->P_Child) {
			if(p_cur->Text.IsEqiAscii("id")) {
				ID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("uuid"))
				Uuid.FromStr(p_cur->P_Child->Text);
			else if(p_cur->Text.IsEqiAscii("orgcmduuid")) // @v11.4.0
				OrgCmdUuid.FromStr(p_cur->P_Child->Text);
			else if(p_cur->Text.IsEqiAscii("code"))
				Code = SJson::Unescape(p_cur->P_Child->Text);
			else if(p_cur->Text.IsEqiAscii("svcident"))
				SvcIdent.FromMime64(SJson::Unescape(p_cur->P_Child->Text));
			else if(p_cur->Text.IsEqiAscii("basecurrency")) // @v11.3.12
				BaseCurrencySymb = SJson::Unescape(p_cur->P_Child->Text);
			else if(p_cur->Text.IsEqiAscii("crtm")) {
				CreationTime.Set(SJson::Unescape(p_cur->P_Child->Text), DATF_ISO8601CENT, 0);
			}
			else if(p_cur->Text.IsEqiAscii("tm")) {
				Time.Set(SJson::Unescape(p_cur->P_Child->Text), DATF_ISO8601CENT, 0);
			}
			else if(p_cur->Text.IsEqiAscii("duetm")) {
				DueTime.Set(SJson::Unescape(p_cur->P_Child->Text), DATF_ISO8601CENT, 0);
			}
			else if(p_cur->Text.IsEqiAscii("cr_lat")) {
				CreationGeoLoc.Lat = p_cur->P_Child->Text.ToReal();
				occured_crgeoloc_lat = true;
			}
			else if(p_cur->Text.IsEqiAscii("cr_lon")) {
				CreationGeoLoc.Lon = p_cur->P_Child->Text.ToReal();
				occured_crgeoloc_lon = true;
			}
			else if(p_cur->Text.IsEqiAscii("icopid")) {
				InterchangeOpID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("svcoptype")) { // @v11.7.2
				SvcOpType = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("svcopid")) {
				SvcOpID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("posnodeid")) { // @v11.4.6
				PosNodeID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("agentid")) { // @v11.4.6
				AgentID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("cliid")) {
				ClientID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("dlvrlocid")) {
				DlvrLocID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("quotkindid")) { // @v11.7.1
				QuotKindID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("statussurrid")) { // @v11.5.1
				StatusSurrId = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("amount")) {
				Amount = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("memo")) {
				Memo = SJson::Unescape(p_cur->P_Child->Text);
			}
			else if(p_cur->Text.IsEqiAscii("ti_list")) {
				if(SJson::IsArray(p_cur->P_Child)) {
					for(const SJson * p_js_ti = p_cur->P_Child->P_Child; p_js_ti; p_js_ti = p_js_ti->P_Next) {
						if(SJson::IsObject(p_js_ti)) {
							__TransferItem * p_new_item = TiList.CreateNewItem();
							THROW_SL(p_new_item);
							for(const SJson * p_ti_cur = p_js_ti->P_Child; p_ti_cur; p_ti_cur = p_ti_cur->P_Next) {
								if(p_ti_cur->Text.IsEqiAscii("rowidx")) {
									p_new_item->RowIdx = p_ti_cur->P_Child->Text.ToLong();
								}
								else if(p_ti_cur->Text.IsEqiAscii("goodsid")) {
									p_new_item->GoodsID = p_ti_cur->P_Child->Text.ToLong();
								}
								else if(p_ti_cur->Text.IsEqiAscii("unitid")) {
									p_new_item->UnitID = p_ti_cur->P_Child->Text.ToLong();
								}
								else if(p_ti_cur->Text.IsEqiAscii("flags")) {
									p_new_item->Flags = p_ti_cur->P_Child->Text.ToLong();
								}
								else if(p_ti_cur->Text.IsEqiAscii("set")) {
									if(SJson::IsObject(p_ti_cur->P_Child)) {
										for(const SJson * p_set_cur = p_ti_cur->P_Child->P_Child; p_set_cur; p_set_cur = p_set_cur->P_Next) {
											if(p_set_cur->Text.IsEqiAscii("qtty") || p_set_cur->Text.IsEqiAscii("qty"))
												p_new_item->Set.Qtty = p_set_cur->P_Child->Text.ToReal();
											else if(p_set_cur->Text.IsEqiAscii("cost"))
												p_new_item->Set.Cost = p_set_cur->P_Child->Text.ToReal();
											else if(p_set_cur->Text.IsEqiAscii("price"))
												p_new_item->Set.Price = p_set_cur->P_Child->Text.ToReal();
											else if(p_set_cur->Text.IsEqiAscii("discount"))
												p_new_item->Set.Discount = p_set_cur->P_Child->Text.ToReal();
										}
									}
								}
								else if(p_ti_cur->Text.IsEqiAscii("setaccepted")) {
									if(SJson::IsObject(p_ti_cur->P_Child)) {
										for(const SJson * p_set_cur = p_ti_cur->P_Child->P_Child; p_set_cur; p_set_cur = p_set_cur->P_Next) {
											if(p_set_cur->Text.IsEqiAscii("qtty") || p_set_cur->Text.IsEqiAscii("qty"))
												p_new_item->SetAccepted.Qtty = p_set_cur->P_Child->Text.ToReal();
											else if(p_set_cur->Text.IsEqiAscii("cost"))
												p_new_item->SetAccepted.Cost = p_set_cur->P_Child->Text.ToReal();
											else if(p_set_cur->Text.IsEqiAscii("price"))
												p_new_item->SetAccepted.Price = p_set_cur->P_Child->Text.ToReal();
											else if(p_set_cur->Text.IsEqiAscii("discount"))
												p_new_item->SetAccepted.Discount = p_set_cur->P_Child->Text.ToReal();
										}
									}
								}
								else if(p_ti_cur->Text.IsEqiAscii("xcl")) {
									if(SJson::IsArray(p_ti_cur->P_Child)) {
										for(const SJson * p_js_xci = p_ti_cur->P_Child->P_Child; p_js_xci; p_js_xci = p_js_xci->P_Next) {
											if(SJson::IsObject(p_js_xci)) {
												LotExtCode lec;
												for(const SJson * p_xc_cur = p_js_xci->P_Child; p_xc_cur; p_xc_cur = p_xc_cur->P_Next) {
													if(p_xc_cur->Text.IsEqiAscii("cod")) {
														temp_buf = SJson::Unescape(p_xc_cur->P_Child->Text);
														STRNSCPY(lec.Code, temp_buf);
													}
													else if(p_xc_cur->Text.IsEqiAscii("boxrefn"))
														lec.BoxRefN = p_xc_cur->P_Child->Text.ToLong();
													else if(p_xc_cur->Text.IsEqiAscii("flags"))
														lec.Flags = p_xc_cur->P_Child->Text.ToLong();
												}
												if(lec.Code[0]) {
													p_new_item->XcL.insert(&lec);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			else if(p_cur->Text.IsEqiAscii("vxcl")) {
				if(SJson::IsArray(p_cur->P_Child)) {				
					for(const SJson * p_js_xci = p_cur->P_Child->P_Child; p_js_xci; p_js_xci = p_js_xci->P_Next) {
						for(const SJson * p_xc_cur = p_js_xci->P_Child->P_Child; p_xc_cur; p_xc_cur = p_xc_cur->P_Next) {
							if(SJson::IsObject(p_js_xci)) {
								LotExtCode lec;
								for(const SJson * p_xc_cur = p_js_xci->P_Child; p_xc_cur; p_xc_cur = p_xc_cur->P_Next) {
									if(p_xc_cur->Text.IsEqiAscii("cod")) {
										temp_buf = SJson::Unescape(p_xc_cur->P_Child->Text);
										STRNSCPY(lec.Code, temp_buf);
									}
									else if(p_xc_cur->Text.IsEqiAscii("boxrefn")) {
										lec.BoxRefN = p_xc_cur->P_Child->Text.ToLong();
									}
									else if(p_xc_cur->Text.IsEqiAscii("flags")) {
										lec.Flags = p_xc_cur->P_Child->Text.ToLong();
									}
								}
								if(lec.Code[0])
									VXcL.insert(&lec);
							}
						}
					}
				}
			}
			else if(p_cur->Text.IsEqiAscii("bk_list")) {
				if(SJson::IsArray(p_cur->P_Child)) {
					for(const SJson * p_js_ti = p_cur->P_Child->P_Child; p_js_ti; p_js_ti = p_js_ti->P_Next) {
						if(SJson::IsObject(p_js_ti)) {
							BookingItem * p_new_item = BkList.CreateNewItem();
							THROW_SL(p_new_item);
							for(const SJson * p_ti_cur = p_js_ti->P_Child; p_ti_cur; p_ti_cur = p_ti_cur->P_Next) {
								if(p_ti_cur->Text.IsEqiAscii("rowidx")) {
									p_new_item->RowIdx = p_ti_cur->P_Child->Text.ToLong();
								}
								else if(p_ti_cur->Text.IsEqiAscii("goodsid")) {
									p_new_item->GoodsID = p_ti_cur->P_Child->Text.ToLong();
								}
								if(p_ti_cur->Text.IsEqiAscii("prcid")) {
									p_new_item->PrcID = p_ti_cur->P_Child->Text.ToLong();
								}
								else if(p_ti_cur->Text.IsEqiAscii("flags")) {
									p_new_item->Flags = p_ti_cur->P_Child->Text.ToLong();
								}
								else if(p_ti_cur->Text.IsEqiAscii("reqtime")) {
									p_new_item->ReqTime.Set(SJson::Unescape(p_ti_cur->P_Child->Text), DATF_ISO8601CENT, 0);
								}
								else if(p_ti_cur->Text.IsEqiAscii("estimateddurationsec")) {
									p_new_item->EstimatedDurationSec = p_ti_cur->P_Child->Text.ToLong();
								}
								else if(p_ti_cur->Text.IsEqiAscii("set")) {
									if(SJson::IsObject(p_ti_cur->P_Child)) {
										for(const SJson * p_set_cur = p_ti_cur->P_Child->P_Child; p_set_cur; p_set_cur = p_set_cur->P_Next) {
											if(p_set_cur->Text.IsEqiAscii("qtty") || p_set_cur->Text.IsEqiAscii("qty")) {
												p_new_item->Set.Qtty = p_set_cur->P_Child->Text.ToReal();
											}
											else if(p_set_cur->Text.IsEqiAscii("cost")) {
												p_new_item->Set.Cost = p_set_cur->P_Child->Text.ToReal();
											}
											else if(p_set_cur->Text.IsEqiAscii("price")) {
												p_new_item->Set.Price = p_set_cur->P_Child->Text.ToReal();
											}
											else if(p_set_cur->Text.IsEqiAscii("discount")) {
												p_new_item->Set.Discount = p_set_cur->P_Child->Text.ToReal();
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::Document::FromJson(const char * pJson)
{
	int    ok = 1;
	SJson * p_js = 0;
	Z();
	if(!isempty(pJson)) {
		THROW_SL(p_js = SJson::Parse(pJson));
		THROW(FromJsonObject(p_js));
	}
	else
		ok = -1;
	CATCHZOK
	delete p_js;
	return ok;
}

SString & PPStyloQInterchange::Document::MakeCodeString(SString & rBuf) const
{
	rBuf.Z();
	rBuf.Cat(Code);
	if(checkdate(Time.d))
		rBuf.CatDiv('-', 1).Cat(Time.d, DATF_DMY);
	else if(checkdate(CreationTime.d))
		rBuf.CatDiv('-', 1).Cat(CreationTime.d, DATF_DMY);
	return rBuf;
}

int PPStyloQInterchange::Document::ToJson(SString & rResult) const
{
	int    ok = 1;
	return ok;
}

StyloQProtocol::StyloQProtocol() : PPJobSrvProtocol()
{
}
	
StyloQProtocol & StyloQProtocol::Z()
{
	PPJobSrvProtocol::Z();
	P.Z();
	return *this;
}

int StyloQProtocol::StartWriting(int cmdId, int subtype)
{
	int    ok = 1;
	PPJobSrvProtocol::Z();
	H.ProtocolVer = CurrentProtocolVer;
	H.Type = cmdId;
	H.DataLen = sizeof(H);
	if(subtype == psubtypeForwardError)
		H.Flags |= hfRepError;
	else if(subtype == psubtypeReplyOk)
		H.Flags |= hfAck;
	else if(subtype == psubtypeReplyError)
		H.Flags |= (hfAck|hfRepError);
	else if(subtype == psubtypeIntermediateReply) // @v11.2.12
		H.Flags |= hfInformer;
	THROW_SL(Write(&H, sizeof(H)));
	State |= stStructured;
	State &= ~stReading;
	CATCHZOK
	return ok;
}

int StyloQProtocol::FinishWriting(const SBinaryChunk * pCryptoKey)
{
	int    ok = 1;
	bool   encrypted = false;
	uint8  padding = 0; // Размер "набивки" буфера до кратного блоку шифрования //
	assert(State & stStructured);
	assert(GetWrOffs() == sizeof(H));
	THROW_PP(State & stStructured, PPERR_SQPROT_WRUNSTRUCTURED);
	if(pCryptoKey) {
		const binary128 key_md5 = SlHash::Md5(0, pCryptoKey->PtrC(), pCryptoKey->Len());
		SlCrypto cryp(SlCrypto::algAes, SlCrypto::kbl128, SlCrypto::algmodEcb);
		SlCrypto::Key crypkey;
		const SlCrypto::CipherProperties & r_cp = cryp.GetCipherProperties();
		P.Pack();
		STempBuffer cbuf2(P.GetDataLen() + 64); // 64 ensurance
		size_t cryp_size = 0;
		THROW_SL(cbuf2.IsValid());
		THROW_SL(cryp.SetupKey(crypkey, &key_md5, sizeof(key_md5), 0, 0));
		{
			const size_t raw_data_len = P.GetDataLen();
			size_t padded_size = r_cp.BlockSize ? (raw_data_len ? ((((raw_data_len-1) / r_cp.BlockSize) + 1) * r_cp.BlockSize) : r_cp.BlockSize) : raw_data_len;
			assert(padded_size >= raw_data_len && (padded_size - raw_data_len) < 256);
			THROW_PP(padded_size >= raw_data_len && (padded_size - raw_data_len) < 256, PPERR_SQPROT_WRINNERSTATE);
			padding = static_cast<uint8>(padded_size - raw_data_len);
			THROW_SL(P.Ensure(padded_size));
			THROW_SL(cryp.Encrypt_(&crypkey, P.GetPtr(), padded_size, cbuf2.vptr(), cbuf2.GetSize(), &cryp_size));
			THROW_SL(SBuffer::Write(cbuf2.vcptr(), cryp_size));
			encrypted = true;
		}
	}
	else {
		THROW_SL(P.Serialize(+1, *this, 0/*!*/));
	}
	{
		Header01 * p_hdr = static_cast<Header01 *>(SBuffer::Ptr(SBuffer::GetRdOffs()));
		p_hdr->DataLen = static_cast<int32>(SBuffer::GetAvailableSize());
		SETFLAG(p_hdr->Flags, hfEncrypted, encrypted);
		p_hdr->Padding = padding;
	}
	CATCHZOK
	return ok;
}

int StyloQProtocol::Read(SBuffer & rMsgBuf, const SBinaryChunk * pCryptoKey)
{
	int    ok = 1;
	Z();
	//THROW(SBuffer::Copy(rMsgBuf));
	SBuffer::SetRdOffs(0);
	THROW_PP(rMsgBuf.Read(&H, sizeof(H)) == sizeof(H), PPERR_SQPROT_RDHDRSIZE);
	THROW_PP(H.ProtocolVer == CurrentProtocolVer, PPERR_SQPROT_RDVERSION);
	{
		const  size_t src_size = rMsgBuf.GetAvailableSize();
		// @debug THROW_PP(H.DataLen == (src_size + sizeof(H)), PPERR_SQPROT_INVHDR);
		// @debug THROW_PP((H.Flags & hfEncrypted) || H.Padding == 0, PPERR_SQPROT_INVHDR);
		// @debug {
		{
			if(H.DataLen != (src_size + sizeof(H))) {
				CALLEXCEPT_PP(PPERR_SQPROT_INVHDR);
			}
			if(!(H.Flags & hfEncrypted) && H.Padding != 0) {
				CALLEXCEPT_PP(PPERR_SQPROT_INVHDR);
			}
		}
		// } @debug 
		if(src_size) {
			if(H.Flags & hfEncrypted) {
				THROW_PP(pCryptoKey, PPERR_SQPROT_NOCRYPTOKEY);
				{
					const binary128 key_md5 = SlHash::Md5(0, pCryptoKey->PtrC(), pCryptoKey->Len());
					SlCrypto cryp(SlCrypto::algAes, SlCrypto::kbl128, SlCrypto::algmodEcb);
					SlCrypto::Key crypkey;
					const SlCrypto::CipherProperties & r_cp = cryp.GetCipherProperties();
					STempBuffer cbuf2(src_size + 64); // 64 ensurance
					size_t cryp_size = 0;
					THROW_SL(cbuf2.IsValid());
					THROW_SL(cryp.SetupKey(crypkey, &key_md5, sizeof(key_md5), 0, 0));
					THROW_PP(!r_cp.BlockSize || (src_size % r_cp.BlockSize) == 0, PPERR_SQPROT_DECRYPTFAULT);
					THROW_SL(cryp.Decrypt_(&crypkey, rMsgBuf.GetBufC(rMsgBuf.GetRdOffs()), src_size, cbuf2.vptr(), cbuf2.GetSize(), &cryp_size));
					{
						size_t effective_size = 0;
						THROW_PP(cryp_size > 0, PPERR_SQPROT_DECRYPTFAULT);
						if(cryp_size < src_size && (src_size - cryp_size) == H.Padding)
							effective_size = cryp_size;
						else {
							THROW_PP(cryp_size > H.Padding, PPERR_SQPROT_DECRYPTFAULT);
							effective_size = (cryp_size-H.Padding);
						}
						THROW_PP(effective_size, PPERR_SQPROT_DECRYPTFAULT);
						THROW_SL(P.SetOuterBuffer(cbuf2.vptr(), effective_size));
					}
				}
			}
			else {
				THROW(P.Serialize(-1, rMsgBuf, 0/*!*/));
			}
		}
	}
	CATCHZOK
	return ok;
}

int StyloQProtocol::Read(PPMqbClient::Message & rMsg, const SBinaryChunk * pCryptoKey)
{
	return Read(rMsg.Body, pCryptoKey);
}

int StyloQProtocol::ReadMime64(const SString & rSrcMime64, const SBinaryChunk * pCryptoKey)
{
	int    ok = 1;
	Z();
	if(rSrcMime64.Len()) {
		SBuffer temp_sbuf;
		size_t real_size = 0;
		STempBuffer tbuf((rSrcMime64.Len() * 3) / 2);
		THROW_SL(rSrcMime64.DecodeMime64(tbuf, tbuf.GetSize(), &real_size));
		THROW_SL(temp_sbuf.Write(tbuf.cptr(), real_size));
		THROW(Read(temp_sbuf, pCryptoKey));
	}
	else {
		ok = -1;
	}
	CATCHZOK
	return ok;
}

/*static*/int StyloQProtocol::Test()
{
	int    ok = 1;
	StyloQProtocol pack_src;
	StyloQProtocol pack_dest;
	const size_t crypto_key_size = 16;
	uint8 raw_crypto_key[128];
	for(uint roundidx = 0; roundidx < 100; roundidx++) {
		pack_src.Z();
		pack_dest.Z();
		SObfuscateBuffer(raw_crypto_key, crypto_key_size);
		const SBinaryChunk crypto_key(raw_crypto_key, crypto_key_size);
		{
			SBinaryChunk bc;
			pack_src.StartWriting(PPSCMD_SQ_SRPREGISTER, psubtypeForward);
			for(uint cidx = 0; cidx < 17; cidx++) {
				uint csz = SLS.GetTLA().Rg.GetUniformInt(8192);
				THROW(bc.Set(0xfe, csz));
				THROW(pack_src.P.Put(cidx+1, bc));
			}
			THROW(pack_src.FinishWriting(&crypto_key));
			//
			THROW(pack_dest.Read(pack_src, &crypto_key));
			THROW(pack_dest.P.IsEq(pack_src.P));
		}
	}
	CATCHZOK
	return ok;
}

PPStyloQInterchange::RoundTripBlock::RoundTripBlock() : 
	P_Mqbc(0), P_MqbRpe(0), P_SrpV(0), InnerSvcID(0), InnerSessID(0), InnerCliID(0), State(0), LastRcvCmd(0), LastSndCmd(0)
{
}

//PPStyloQInterchange::RoundTripBlock::RoundTripBlock(const void * pSvcIdent, size_t svcIdentLen, const char * pSvcAccsPoint) : 		
PPStyloQInterchange::RoundTripBlock::RoundTripBlock(const SBinaryChunk * pSvcIdent, const SBinaryChunk * pSvcLoclAddendum, const char * pSvcAccsPoint) :
	P_Mqbc(0), P_MqbRpe(0), P_SrpV(0), InnerSvcID(0), InnerSessID(0), InnerCliID(0), State(0), LastRcvCmd(0), LastSndCmd(0)
{
	if(pSvcIdent && pSvcIdent->Len())
		Other.Put(SSecretTagPool::tagSvcIdent, *pSvcIdent);
	// @v11.2.3 {
	if(pSvcLoclAddendum && pSvcLoclAddendum->Len())
		Other.Put(SSecretTagPool::tagSvcLoclAddendum, *pSvcLoclAddendum);
	// } @v11.2.3 
	if(!isempty(pSvcAccsPoint)) {
		Other.Put(SSecretTagPool::tagSvcAccessPoint, pSvcAccsPoint, sstrlen(pSvcAccsPoint));
	}
}
		
PPStyloQInterchange::RoundTripBlock::~RoundTripBlock()
{
	delete P_Mqbc;
	delete P_MqbRpe;
	delete P_SrpV;
}

int PPStyloQInterchange::AcceptInvitation(const char * pInvitationData, InterchangeParam & rInv)
{
	int    ok = 1;
	rInv.Z();
	THROW_PP(!isempty(pInvitationData), PPERR_SQ_INVITATPARSEFAULT_EMPTY);
	{
		STempBuffer temp_binary(4096);
		SString temp_buf(pInvitationData);
		//temp_buf.Tokenize("&", ss);
		StringSet ss('&', temp_buf);
		uint   tokn = 0;
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			tokn++;
			size_t actual_size = 0;
			if(tokn == 1) { // prefix and svcid
				const char first_symb = temp_buf.C(0);
				THROW_PP_S(first_symb == 'A', PPERR_SQ_INVITATPARSEFAULT, pInvitationData); // @error invalid invitation prefix
				temp_buf.ShiftLeft();
				THROW_PP(rInv.SvcIdent.FromMime64(temp_buf), PPERR_SQ_MALFORMEDSVCIDENTTEXT);
				//THROW(rInv.SvcIdent.Len() == 20); // @error invalid service public id
			}
			else if(tokn == 2) { // @v11.2.3 locl addendum
				if(temp_buf.Len()) {
					THROW_SL(rInv.LoclAddendum.FromMime64(temp_buf));
				}
				else
					rInv.LoclAddendum.Z();
			}
			else if(tokn == 3) { // capabilities // @v11.2.3 2-->3
				temp_buf.DecodeMime64(temp_binary, temp_binary.GetSize(), &actual_size);
				THROW_PP_S(actual_size == sizeof(rInv.Capabilities), PPERR_SQ_INVITATPARSEFAULT, pInvitationData); // @error invalid capabilities
				memcpy(&rInv.Capabilities, temp_binary, actual_size);
			}
			else if(tokn == 4) { // access point // @v11.2.3 3-->4
				temp_buf.DecodeMime64(temp_binary, temp_binary.GetSize(), &actual_size);
				temp_buf.Z().CatN(temp_binary, actual_size);
				InetUrl url(temp_buf);
				THROW_PP_S(url.GetProtocol(), PPERR_SQ_INVITATPARSEFAULT, pInvitationData);
				{
					SString host;
					url.GetComponent(InetUrl::cHost, 0, host); 
					THROW_PP_S(host.NotEmpty(), PPERR_SQ_INVITATPARSEFAULT, pInvitationData); // @error invalid url
				}
				rInv.AccessPoint = temp_buf;
			}
			else if(tokn == 5) { // command // @v11.2.3 4-->5
				temp_buf.DecodeMime64(temp_binary, temp_binary.GetSize(), &actual_size);
				temp_buf.Z().CatN(temp_binary, actual_size);
				rInv.CommandJson.CatN(temp_binary, actual_size);
			}
			else {
				CALLEXCEPT_PP_S(PPERR_SQ_INVITATPARSEFAULT, pInvitationData); // invalid token count
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::GetOwnPeerEntry(StyloQCore::StoragePacket * pPack)
{
	return P_T ? P_T->GetOwnPeerEntry(pPack) : 0;
}

int PPStyloQInterchange::SearchGlobalIdentEntry(int kind, const SBinaryChunk & rIdent, StyloQCore::StoragePacket * pPack)
{
	return P_T ? P_T->SearchGlobalIdentEntry(kind, rIdent, pPack) : 0;
}

int StyloQCore::GetMediatorList(TSCollection <StyloQCore::IgnitionServerEntry> & rList)
{
	rList.clear();
	int    ok = -1;
	PPIDArray mediator_id_list;
	GetMediatorIdList(mediator_id_list);
	ReadIgnitionServerList(rList);
	if(mediator_id_list.getCount() || rList.getCount()) {
		SString temp_buf;
		SBinaryChunk temp_bch;
		SString url_buf;
		SBinaryChunk svc_ident_from_pool;
		SSecretTagPool svc_reply;
		StyloQConfig svc_cfg;
		for(uint i = 0; i < mediator_id_list.getCount(); i++) {
			StyloQCore::StoragePacket ms_pack;
			if(GetPeerEntry(mediator_id_list.get(i), &ms_pack) > 0) {
				if(ms_pack.Pool.Get(SSecretTagPool::tagConfig, &temp_bch) && ms_pack.Pool.Get(SSecretTagPool::tagSvcIdent, &svc_ident_from_pool)) {
					assert(temp_bch.Len());
					assert(svc_ident_from_pool.Len());
					temp_bch.ToRawStr(temp_buf);
					if(!svc_ident_from_pool.IsEq(ms_pack.Rec.BI, svc_ident_from_pool.Len())) {
						; // @error
					}
					else {
						bool dup_found = false;
						for(uint islidx = 0; !dup_found && islidx < rList.getCount(); islidx++) {
							const StyloQCore::IgnitionServerEntry * p_isl_entry = rList.at(islidx);
							if(p_isl_entry && p_isl_entry->Ident == svc_ident_from_pool)
								dup_found = true;
						}
						if(!dup_found && svc_cfg.FromJson(temp_buf) /*&& svc_cfg.Get(StyloQConfig::tagExpiryEpochSec, temp_buf) && !IsExpired(temp_buf.ToInt64())*/) {
							if(svc_cfg.Get(StyloQConfig::tagUrl, url_buf)) {
								InetUrl url(url_buf);
								if(url.GetHostName().NotEmpty()) {
									int prot = url.GetProtocol();
									if(prot == 0 && !svc_cfg.Get(StyloQConfig::tagMqbAuth, temp_buf)) {
										prot = InetUrl::protHttp;
									}
									if(oneof2(prot, InetUrl::protHttp, InetUrl::protHttps)) {
										StyloQCore::IgnitionServerEntry * p_new_isl_entry = rList.CreateNewItem();
										if(p_new_isl_entry) {
											p_new_isl_entry->Url = url_buf;
											p_new_isl_entry->Ident = svc_ident_from_pool;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if(rList.getCount())
		ok = 1;
	return ok;
}

int PPStyloQInterchange::ServiceSelfregisterInMediator(const StyloQCore::StoragePacket & rOwnPack, SysJournal * pSjOuter)
{
	int    ok = 1;
	SString temp_buf;
	TSCollection <StyloQCore::IgnitionServerEntry> isl;
	SBinaryChunk temp_bch;
	SBinaryChunk own_ident;
	SString my_face_json_buf;
	SString my_transmission_cfg_json_buf;
	StyloQFace my_face;
	StyloQConfig my_transmission_cfg;
	SString own_ident_hex;
	SysJournal * p_sj = pSjOuter ? pSjOuter : DS.GetTLA().P_SysJ;
	bool done = false; // Если последняя регистрация была позже последнего изменения собственной записи, то done становится true - ничего делать не надо.
	THROW_PP(P_T, PPERR_SQ_UNDEFINNERSTQIDBTBL);
	if(p_sj) {
		LDATETIME last_self_reg_dtm = ZERODATETIME;
		SysJournalTbl::Rec last_self_reg_rec;
		if(p_sj->GetLastEvent(PPACN_STYLOQSVCSELFREG, 0, &last_self_reg_dtm, 30, &last_self_reg_rec) > 0) {
			int cr = 0;
			LDATETIME last_mod_dtm = ZERODATETIME;
			SysJournalTbl::Rec last_mod_rec;
			if(p_sj->GetLastObjModifEvent(PPOBJ_STYLOQBINDERY, rOwnPack.Rec.ID, &last_mod_dtm, &cr, &last_mod_rec) > 0) {
				if(cmp(last_self_reg_dtm, last_mod_dtm) > 0)
					done = true;
			}
		}
	}
	if(!done) {
		THROW_PP(rOwnPack.Pool.Get(SSecretTagPool::tagSvcIdent, &own_ident), PPERR_SQ_UNDEFOWNSVCID);
		assert(own_ident.Len() > 0);
		own_ident.Mime64(own_ident_hex);
		THROW_PP(rOwnPack.Pool.Get(SSecretTagPool::tagConfig, &temp_bch), PPERR_SQ_UNDEFOWNCFG);
		THROW(StyloQConfig::MakeTransmissionJson(temp_bch.ToRawStr(temp_buf), 0, my_transmission_cfg_json_buf)); // @todo @err
		//p_js_adv_cfg = StyloQConfig::MakeTransmissionJson(temp_buf);
		THROW(my_transmission_cfg.FromJson(my_transmission_cfg_json_buf)); // @todo @err
		if(rOwnPack.Pool.Get(SSecretTagPool::tagSelfyFace, &temp_bch)) {
			// @v11.3.8 temp_bch.ToRawStr(my_face_json_buf);
			StyloQFace::MakeTransmissionJson(rOwnPack.Rec.ID, own_ident, temp_bch.ToRawStr(temp_buf), my_face_json_buf); // @v11.3.8 
			//if(my_face.FromJson(my_face_json_buf))
				//p_js_adv_face = my_face.ToJson();
		}
		if(my_transmission_cfg.GetCount() && P_T->GetMediatorList(isl) > 0) {
			SSecretTagPool svc_reply;
			assert(isl.getCount());
			// Для равномерного распределения нагрузки между медиаторами выбираем медиатора из списка случайным образом 
			isl.shuffle(); 
			for(uint islidx = 0; islidx < isl.getCount(); islidx++) {
				const StyloQCore::IgnitionServerEntry * p_isl_entry = isl.at(islidx);
				if(p_isl_entry) {
					PPStyloQInterchange::InterchangeParam dip;
					dip.SvcIdent = p_isl_entry->Ident;
					dip.AccessPoint = p_isl_entry->Url;
					SJson query(SJson::tOBJECT);
					query.InsertString("cmd", "advert");
					query.InsertString("svcident", own_ident_hex);
					SJson * p_js_adv_cfg = SJson::Parse(my_transmission_cfg_json_buf);
					if(p_js_adv_cfg) {
						query.Insert("config", p_js_adv_cfg);
						p_js_adv_cfg = 0;
						//
						if(my_face_json_buf.NotEmpty()) {
							SJson * p_js_adv_face = SJson::Parse(my_face_json_buf);
							if(p_js_adv_face) {
								query.Insert("face", p_js_adv_face);
								p_js_adv_face = 0;
							}
						}
						query.ToStr(dip.CommandJson);
						if(DoInterchange(dip, svc_reply)) {
							if(p_sj)
								p_sj->LogEvent(PPACN_STYLOQSVCSELFREG, PPOBJ_STYLOQBINDERY, rOwnPack.Rec.ID, 0/*extData*/, 1/*use_ta*/);
							ok = 1;
							break;
						}
						else {
							SJson * p_js_reply = svc_reply.GetJson(SSecretTagPool::tagRawData);
							if(p_js_reply) {
								// do something...
								ZDELETE(p_js_reply); // @v11.3.11 @fix
							}
						}
					}
				}
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::StoreSession(PPID * pID, StyloQCore::StoragePacket * pPack, int use_ta)
{
	int    ok = 1;
	PPID   correspind_id = 0;
	StyloQCore::StoragePacket org_pack;
	StyloQCore::StoragePacket correspond_pack;
	if(P_T) {
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			int    correspond_type = 0;
			SBinaryChunk temp_chunk;
			SBinaryChunk cli_ident;
			SBinaryChunk svc_ident;
			THROW(pPack->Pool.Get(SSecretTagPool::tagSessionPublicKey, 0));
			THROW(pPack->Pool.Get(SSecretTagPool::tagSessionPrivateKey, 0));
			THROW(pPack->Pool.Get(SSecretTagPool::tagSessionSecret, 0));
			{
				pPack->Pool.Get(SSecretTagPool::tagClientIdent, &cli_ident);
				pPack->Pool.Get(SSecretTagPool::tagSvcIdent, &svc_ident);
				THROW(cli_ident.Len() || svc_ident.Len());
				THROW((cli_ident.Len() * svc_ident.Len()) == 0); // Не должно быть так, что заданы одновременно и клиентский и сервисных идентификаторы
				THROW(oneof2(cli_ident.Len(), 0, 20));
				THROW(oneof2(svc_ident.Len(), 0, 20));
			}
			if(cli_ident.Len()) {
				THROW(P_T->SearchGlobalIdentEntry(StyloQCore::kClient, cli_ident, &correspond_pack) > 0); // должна существовать запись соответствующего клиента
				correspond_type = StyloQCore::kClient;
			}
			else if(svc_ident.Len()) {
				THROW(P_T->SearchGlobalIdentEntry(StyloQCore::kForeignService, svc_ident, &correspond_pack) > 0); // должна существовать запись соответствующего сервиса
				correspond_type = StyloQCore::kForeignService;
			}
			else {
				assert(0); // Выше должны были проверить это условие
			}
			correspind_id = correspond_pack.Rec.ID;
			THROW(correspond_pack.Rec.Kind == correspond_type); // Корреспондирующий пакет определенного вида должен быть обязательно 
			THROW(pPack->Pool.Get(SSecretTagPool::tagSessionPublicKeyOther, &temp_chunk));
			const binary160 bi = SlHash::Sha1(0, temp_chunk.PtrC(), temp_chunk.Len());
			assert(sizeof(bi) <= sizeof(pPack->Rec.BI));
			pPack->Rec.Kind = StyloQCore::kSession;
			pPack->Rec.ID = 0;
			pPack->Rec.CorrespondID = correspond_pack.Rec.ID;
			memcpy(pPack->Rec.BI, &bi, sizeof(bi));
			if(!pID || !*pID) {
				// create session
				PPID   new_id = 0;
				if(correspond_pack.Rec.CorrespondID) {
					StyloQCore::StoragePacket prev_sess_pack;
					if(P_T->GetPeerEntry(correspond_pack.Rec.CorrespondID, &prev_sess_pack) > 0) {
						if(prev_sess_pack.Rec.Kind != StyloQCore::kSession) {
							// @problem Нарушена логическая непротиворечивость данных - основная запись ссылается через CorrespondID 
							// на запись, не являющуюся сессией.
							// Тем не менее, мы удалим ту запись, поскольку будем менять ссылку correspond_pack.Rec.CorrespondID
						}
						THROW(P_T->PutPeerEntry(&correspond_pack.Rec.CorrespondID, 0, 0));
					}
				}
				THROW(P_T->PutPeerEntry(&new_id, pPack, 0));
				correspond_pack.Rec.CorrespondID = new_id;
				assert(correspind_id > 0);
				THROW(P_T->PutPeerEntry(&correspind_id, &correspond_pack, 0));
				ASSIGN_PTR(pID, new_id);
			}
			else {
				// update session by id
				THROW(P_T->GetPeerEntry(*pID, &org_pack) > 0);
				//
				// Проверка идентичности некоторых параметров изменяемой записи 
				//
				THROW(org_pack.Rec.Kind == pPack->Rec.Kind);
				THROW(org_pack.Rec.CorrespondID == correspind_id);
				if(cli_ident.Len()) {
					THROW(org_pack.Pool.Get(SSecretTagPool::tagClientIdent, &temp_chunk) && temp_chunk == cli_ident);
				}
				else if(svc_ident.Len()) {
					THROW(org_pack.Pool.Get(SSecretTagPool::tagSvcIdent, &temp_chunk) && temp_chunk == svc_ident);
				}
				pPack->Rec.ID = org_pack.Rec.ID;
				THROW(P_T->PutPeerEntry(pID, pPack, 0));
			}
		}
		else {
			THROW(pID && *pID); // @error invalid arguments
			// remove session by id
			THROW(P_T->GetPeerEntry(*pID, &org_pack) > 0);
			correspind_id = org_pack.Rec.CorrespondID;
			if(correspind_id) {
				if(P_T->GetPeerEntry(correspind_id, &correspond_pack) > 0) {
					if(correspond_pack.Rec.CorrespondID == *pID) {
						// Обнуляем ссылку на удаляемую запись в коррерспондирующем пакете
						correspond_pack.Rec.CorrespondID = 0;
						THROW(P_T->PutPeerEntry(&correspind_id, &correspond_pack, 0));
					}
					else {
						// @problem (логическая целостность таблицы нарушена, но прерывать исполнение нельзя - мы все равно удаляем запись)
					}
				}
				else {
					// @problem (логическая целостность таблицы нарушена, но прерывать исполнение нельзя - мы все равно удаляем запись)
				}
			}
			THROW(P_T->PutPeerEntry(pID, 0, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::ExtractSessionFromPacket(const StyloQCore::StoragePacket & rPack, SSecretTagPool & rSessCtx)
{
	int    ok = 1;
	LongArray tag_list;
	tag_list.addzlist(SSecretTagPool::tagSessionPrivateKey, SSecretTagPool::tagSessionPublicKey, SSecretTagPool::tagSessionSecret, 0L);
	THROW_PP(rPack.Rec.Kind == StyloQCore::kSession, PPERR_SQ_WRONGDBITEMKIND);
	THROW_SL(rSessCtx.CopyFrom(rPack.Pool, tag_list, true /*errOnAbsenceAny*/));
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::Registration_ClientRequest(RoundTripBlock & rB)
{
	int    ok = 0;
	SString temp_buf;
	SBinaryChunk cli_ident;
	SBinaryChunk cli_secret;
	SBinaryChunk sess_secret;
	SBinaryChunk svc_ident;
	SBinaryChunk __s;
	SBinaryChunk __v;
	THROW_PP(rB.Other.Get(SSecretTagPool::tagSvcIdent, &svc_ident), PPERR_SQ_UNDEFSVCID);
	//if(GeneratePublicIdent(rB.StP.Pool, svc_ident, SSecretTagPool::tagClientIdent, gcisfMakeSecret, rB.Sess)) {
	THROW_PP(rB.Sess.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_UNDEFCLIID);
	THROW_PP(rB.Sess.Get(SSecretTagPool::tagSecret, &cli_secret), PPERR_SQ_UNDEFSESSSECRET_INNER);
	THROW_PP(rB.Sess.Get(SSecretTagPool::tagSessionSecret, &sess_secret), PPERR_SQ_UNDEFSESSSECRET);
	temp_buf.EncodeMime64(cli_ident.PtrC(), cli_ident.Len());
	SlSRP::CreateSaltedVerificationKey2(SlSRP::SRP_SHA1, SlSRP::SRP_NG_8192, temp_buf, PTR8C(cli_secret.PtrC()), cli_secret.Len(), __s, __v, 0, 0);
	{
		StyloQProtocol tp;
		tp.StartWriting(PPSCMD_SQ_SRPREGISTER, StyloQProtocol::psubtypeForward);
		tp.P.Put(SSecretTagPool::tagClientIdent, cli_ident);
		{
			SBinaryChunk my_face;
			if(rB.StP.Pool.Get(SSecretTagPool::tagFace, &my_face)) {
				tp.P.Put(SSecretTagPool::tagFace, my_face);
			}
		}
		tp.P.Put(SSecretTagPool::tagSrpVerifier, __v);
		tp.P.Put(SSecretTagPool::tagSrpVerifierSalt, __s);
		assert(tp.P.IsValid());
		THROW(tp.FinishWriting(&sess_secret));
		if(oneof2(rB.Url.GetProtocol(), InetUrl::protHttp, InetUrl::protHttps)) {
			THROW(SendHttpQuery(rB, tp, &sess_secret));
			ok = 1;
		}
		else if(oneof2(rB.Url.GetProtocol(), InetUrl::protAMQP, InetUrl::protAMQPS)) {
			PPMqbClient::MessageProperties props;
			PPMqbClient::Envelope env;
			THROW_PP(rB.P_Mqbc && rB.P_MqbRpe, PPERR_SQ_INVALIDMQBVALUES);
			const long cmto = PPMqbClient::SetupMessageTtl(__DefMqbConsumeTimeout, &props);
			THROW(rB.P_Mqbc->Publish(rB.P_MqbRpe->ExchangeName, rB.P_MqbRpe->RoutingKey, &props, tp.constptr(), tp.GetAvailableSize()));
			{
				const int cmr = rB.P_Mqbc->ConsumeMessage(env, cmto);
				THROW(cmr);
				THROW_PP_S(cmr > 0, PPERR_SQ_NOSVCRESPTOREGQUERY, cmto);
			}
			rB.P_Mqbc->Ack(env.DeliveryTag, 0);
			ok = 1;
		}
		if(ok) {
			PPID   id = 0;
			StyloQCore::StoragePacket new_storage_pack;
			THROW_PP(P_T, PPERR_SQ_UNDEFINNERSTQIDBTBL);
			new_storage_pack.Rec.Kind = StyloQCore::kForeignService;
			memcpy(new_storage_pack.Rec.BI, svc_ident.PtrC(), svc_ident.Len());
			new_storage_pack.Pool.Put(SSecretTagPool::tagSvcIdent, svc_ident);
			new_storage_pack.Pool.Put(SSecretTagPool::tagClientIdent, cli_ident);
			new_storage_pack.Pool.Put(SSecretTagPool::tagSrpVerifier, __v);
			new_storage_pack.Pool.Put(SSecretTagPool::tagSrpVerifierSalt, __s);
			THROW(P_T->PutPeerEntry(&id, &new_storage_pack, 1));
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::Registration_ServiceReply(const RoundTripBlock & rB, const StyloQProtocol & rPack, StyloQCore::StoragePacket * pCliPack)
{
	int    ok = 1;
	PPID   id = 0;
	StyloQCore::StoragePacket new_storage_pack;
	StyloQCore::StoragePacket ex_storage_pack;
	SBinaryChunk cli_ident;
	SBinaryChunk cli_ident_other_for_test;
	SBinaryChunk srp_s;
	SBinaryChunk srp_v;
	//SBinaryChunk debug_cli_secret; // @debug do remove after debugging!
	SBinaryChunk other_face_chunk;
	SBinaryChunk selfy_face_chunk;
	SString temp_buf;
	THROW_PP(P_T, PPERR_SQ_UNDEFINNERSTQIDBTBL);
	THROW(rB.Other.Get(SSecretTagPool::tagClientIdent, &cli_ident_other_for_test));
	THROW_PP(rPack.P.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_UNDEFCLIID);
	assert(cli_ident_other_for_test == cli_ident);
	THROW(rPack.P.Get(SSecretTagPool::tagSrpVerifier, &srp_v));
	THROW(rPack.P.Get(SSecretTagPool::tagSrpVerifierSalt, &srp_s));
	rPack.P.Get(SSecretTagPool::tagFace, &other_face_chunk);
	//rPack.P.Get(SSecretTagPool::tagSecret, &debug_cli_secret);
	if(P_T->SearchGlobalIdentEntry(StyloQCore::kClient, cli_ident, &ex_storage_pack) > 0) {
		assert(ex_storage_pack.Rec.Kind == StyloQCore::kClient);
		// На этапе отладки будем принимать новые значения. В дальнейшем это невозможно (уязвимостью самозванца)!
		if(1/*debug*/) {
			id = ex_storage_pack.Rec.ID;
			THROW(cli_ident.Len() <= sizeof(new_storage_pack.Rec.BI));
			new_storage_pack.Rec.Kind = StyloQCore::kClient;
			memcpy(new_storage_pack.Rec.BI, cli_ident.PtrC(), cli_ident.Len());
			new_storage_pack.Pool.Put(SSecretTagPool::tagClientIdent, cli_ident);
			new_storage_pack.Pool.Put(SSecretTagPool::tagSrpVerifier, srp_v);
			new_storage_pack.Pool.Put(SSecretTagPool::tagSrpVerifierSalt, srp_s);
			// @v11.1.8 {
			if(other_face_chunk.Len()) {
				new_storage_pack.Pool.Put(SSecretTagPool::tagFace, other_face_chunk);
			}
			// } @v11.1.8 
			// @debug do remove after debugging! {
			/*if(debug_cli_secret.Len()) {
				new_storage_pack.Pool.Put(SSecretTagPool::tagSecret, debug_cli_secret);
			}*/
			// } @debug do remove after debugging!
			THROW(P_T->PutPeerEntry(&id, &new_storage_pack, 1));
			ASSIGN_PTR(pCliPack, new_storage_pack); // @v11.6.0
		}
		else {
			// Попытка клиента повторно зарегистрироваться c другими параметрами авторизации: в общем случае это - ошибка. Клиент не может менять свои регистрационные данные.
			// В дальнейшем возможны варианты.
			//
			// Attention! Логическая ошибка: клиент сейча генерирует пару {s, v} каждый раз новую, потому сравнение с существующими значениями неверно!
			//
			SBinaryChunk temp_bc;
			ASSIGN_PTR(pCliPack, ex_storage_pack); // @v11.6.0
			if(!ex_storage_pack.Pool.Get(SSecretTagPool::tagClientIdent, &temp_bc) || !(temp_bc == cli_ident))
				ok = 0;
			else if(!ex_storage_pack.Pool.Get(SSecretTagPool::tagSrpVerifier, &temp_bc) || !(temp_bc == srp_v))
				ok = 0;
			else if(!ex_storage_pack.Pool.Get(SSecretTagPool::tagSrpVerifierSalt, &temp_bc) || !(temp_bc == srp_s))
				ok = 0;
			else {
				// Клиент мог прислать другие параметры лика.
				ok = 1;
			}
		}
	}
	else {
		THROW(cli_ident.Len() <= sizeof(new_storage_pack.Rec.BI));
		new_storage_pack.Rec.Kind = StyloQCore::kClient;
		memcpy(new_storage_pack.Rec.BI, cli_ident.PtrC(), cli_ident.Len());
		new_storage_pack.Pool.Put(SSecretTagPool::tagClientIdent, cli_ident);
		new_storage_pack.Pool.Put(SSecretTagPool::tagSrpVerifier, srp_v);
		new_storage_pack.Pool.Put(SSecretTagPool::tagSrpVerifierSalt, srp_s);
		// @v11.1.8 {
		if(other_face_chunk.Len()) {
			new_storage_pack.Pool.Put(SSecretTagPool::tagFace, other_face_chunk);
		}
		// } @v11.1.8 
		// @debug do remove after debugging! {
		/*if(debug_cli_secret.Len()) {
			new_storage_pack.Pool.Put(SSecretTagPool::tagSecret, debug_cli_secret);
		}*/
		// } @debug do remove after debugging!
		THROW(P_T->PutPeerEntry(&id, &new_storage_pack, 1));
		ASSIGN_PTR(pCliPack, new_storage_pack); // @v11.6.0
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::SendHttpQuery(RoundTripBlock & rB, StyloQProtocol & rPack, const SBinaryChunk * pCryptoKey)
{
	int    ok = 1;
	bool   test_ping = false; // Признак того, что команда является тестовым пингом сервера
	assert(oneof2(rB.Url.GetProtocol(), InetUrl::protHttp, InetUrl::protHttps));
	SString temp_buf;
	ScURL  c;
	SString content_buf;
	SBuffer reply_buf;
	SFile wr_stream(reply_buf, SFile::mWrite);
	StrStrAssocArray hdr_flds;
	THROW(oneof2(rB.Url.GetProtocol(), InetUrl::protHttp, InetUrl::protHttps));
	/*
		String _path = rtb.Url.getPath();
		if(_path.startsWith("/"))
			_path = _path.substring(1);
		if(Util.GetLen(_path) == 0) { // @temporary
			_path = "styloq";
		}
	*/
	rB.Url.GetComponent(InetUrl::cPath, 0, temp_buf);
	if(temp_buf.IsEmpty()) {
		rB.Url.SetComponent(InetUrl::cPath, "styloq");
	}
	rB.Url.SetQueryParam("rtsid", SLS.AcquireRvlStr().Cat(rB.Uuid, S_GUID::fmtIDL|S_GUID::fmtPlain|S_GUID::fmtLower));
	if(rPack.GetH().Type == PPSCMD_PING) {
		content_buf = "test-request-for-connection-checking";
		test_ping = true;
	}
	else {
		content_buf.Z().EncodeMime64(rPack.constptr(), rPack.GetAvailableSize());
	}
	SFileFormat::GetMime(SFileFormat::Unkn, temp_buf);
	SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, temp_buf);
	SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentLen, temp_buf.Z().Cat(content_buf.Len()));
	SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrAccept, temp_buf);
	SFileFormat::GetContentTransferEncName(/*P_Cb->ContentTransfEnc*/SFileFormat::cteBase64, temp_buf);
	THROW_SL(c.HttpPost(rB.Url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose|ScURL::mfTcpKeepAlive, &hdr_flds, content_buf, &wr_stream)); // @v11.4.5 ScURL::mfTcpKeepAlive
	{
		SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
		THROW(p_ack_buf); // @todo error
		temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
		if(test_ping) {
			THROW(temp_buf.IsEqiAscii("your-test-request-is-accepted"));
		}
		else {
			THROW(rPack.ReadMime64(temp_buf, pCryptoKey) > 0);
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::KexClientRequest(RoundTripBlock & rB)
{
	//
	// Инициирующий запрос: устанавливает ключи шифрования для дальнейшего диалога с сервисом.
	//
	int    ok = -1;
	SString temp_buf;
	SString consume_tag;
	SBinaryChunk svc_ident;
	PPMqbClient * p_mqbc = 0;
	THROW_PP(rB.Other.Get(SSecretTagPool::tagSvcIdent, &svc_ident), PPERR_SQ_UNDEFSVCID);
	{
		SBinaryChunk own_ident;
		SBinaryChunk sess_pub_key;
		SBinaryChunk other_sess_public;
		StyloQProtocol tp;
		int  fsks = FetchSessionKeys(StyloQCore::kForeignService, rB.Sess, svc_ident);
		THROW(fsks);
		THROW(KexGenerateKeys(rB.Sess, 0, 0));
		THROW_PP(rB.Sess.Get(SSecretTagPool::tagClientIdent, &own_ident), PPERR_SQ_UNDEFCLIID); // !
		THROW(rB.Sess.Get(SSecretTagPool::tagSessionPublicKey, &sess_pub_key));
		{
			tp.StartWriting(PPSCMD_SQ_ACQUAINTANCE, StyloQProtocol::psubtypeForward);
			// @v11.2.0 Необходимо передать ид сервиса для того, чтобы сервер, управляющий сервисом мог идентифицировать 
			// базу данных получателя (один сервер может обслуживать несколько сервисов).
			tp.P.Put(SSecretTagPool::tagSvcIdent, svc_ident); 
			tp.P.Put(SSecretTagPool::tagClientIdent, own_ident);
			tp.P.Put(SSecretTagPool::tagSessionPublicKey, sess_pub_key);
			THROW(tp.FinishWriting(0));
		}
		if(oneof2(rB.Url.GetProtocol(), InetUrl::protHttp, InetUrl::protHttps)) {
			THROW(SendHttpQuery(rB, tp, 0));
			if(!tp.CheckRepError()) {
				// @todo Инициализировать ошибку для информирования caller
				ok = 0;
			}
			else {
				THROW(tp.GetH().Type == PPSCMD_SQ_ACQUAINTANCE && tp.GetH().Flags & tp.hfAck);
				tp.P.Get(SSecretTagPool::tagSessionPublicKey, &other_sess_public);
				THROW(KexGenerageSecret(rB.Sess, tp.P));
				// Теперь ключ шифрования сессии есть и у нас и у сервиса!
				ok = 1;
			}
		}
		else if(oneof2(rB.Url.GetProtocol(), InetUrl::protAMQP, InetUrl::protAMQPS)) {
			PPMqbClient::RoutingParamEntry rpe; // маршрут до очереди, в которой сервис ждет новых клиентов
			PPMqbClient::RoutingParamEntry rpe_regular; // маршрут до очереди, в которой сервис будет с нами общаться
			PPMqbClient::MessageProperties props;
			PPMqbClient::Envelope env;
			PPMqbClient::InitParam mqip;
			THROW(PPMqbClient::SetupInitParam(mqip, "styloq", 0));
			//
			//if(fsks == fsksNewSession) 
			THROW(rpe.SetupStyloQRpc(sess_pub_key, svc_ident, mqip.ConsumeParamList.CreateNewItem()));
			THROW(p_mqbc = PPMqbClient::CreateInstance(mqip));
			const long cmto = PPMqbClient::SetupMessageTtl(__DefMqbConsumeTimeout, &props);
			THROW(p_mqbc->Publish(rpe.ExchangeName, rpe.RoutingKey, &props, tp.constptr(), tp.GetAvailableSize()));
			{
				const clock_t _c = clock();
				{
					const int cmr = p_mqbc->ConsumeMessage(env, cmto);
					THROW(cmr);
					THROW_PP_S(cmr > 0, PPERR_SQ_NOSVCRESPTOKEXQUERY, cmto);
				}
				p_mqbc->Ack(env.DeliveryTag, 0);
				THROW(tp.Read(env.Msg, 0));
				if(!tp.CheckRepError()) {
					// @todo Инициализировать ошибку для информирования caller
					ok = 0;
				}
				else {
					THROW(tp.GetH().Type == PPSCMD_SQ_ACQUAINTANCE && tp.GetH().Flags & tp.hfAck);
					tp.P.Get(SSecretTagPool::tagSessionPublicKey, &other_sess_public);
					THROW(KexGenerageSecret(rB.Sess, tp.P));
					// Теперь ключ шифрования сессии есть и у нас и у сервиса!
					THROW(rpe_regular.SetupStyloQRpc(sess_pub_key, other_sess_public, 0));
					// Устанавливаем факторы RoundTrimBlock, которые нам понадобяться при последующих обменах
					if(SetRoundTripBlockReplyValues(rB, p_mqbc, rpe_regular)) {
						p_mqbc = 0;
					}
					ok = 1;
				}
			}
			//else if(fsks == fsksSessionBySvcId) {
			//}
		}
	}
	CATCHZOK
	ZDELETE(p_mqbc);
	return ok;
}

int PPStyloQInterchange::Session_ClientRequest(RoundTripBlock & rB)
{
	int    ok = 1;
	SString temp_buf;
	SBinaryChunk svc_ident;
	SBinaryChunk sess_pub_key;
	SBinaryChunk other_sess_public;
	SBinaryChunk own_ident;
	PPMqbClient * p_mqbc = 0;
	THROW_PP(rB.Other.Get(SSecretTagPool::tagSvcIdent, &svc_ident), PPERR_SQ_UNDEFSVCID);
	THROW_PP(rB.Sess.Get(SSecretTagPool::tagSessionPublicKey, &sess_pub_key), PPERR_SQ_UNDEFSESSPUBKEY);
	THROW_PP(rB.Sess.Get(SSecretTagPool::tagSessionPublicKeyOther, &other_sess_public), PPERR_SQ_UNDEFOTHERPUBKEY);
	THROW_PP(rB.Sess.Get(SSecretTagPool::tagClientIdent, &own_ident), PPERR_SQ_UNDEFCLIID); // !
	{
		StyloQProtocol tp;
		tp.StartWriting(PPSCMD_SQ_SESSION, StyloQProtocol::psubtypeForward);
		tp.P.Put(SSecretTagPool::tagSessionPublicKey, sess_pub_key);
		tp.P.Put(SSecretTagPool::tagClientIdent, own_ident);
		tp.P.Put(SSecretTagPool::tagSvcIdent, svc_ident);
		THROW(tp.FinishWriting(0));
		if(oneof2(rB.Url.GetProtocol(), InetUrl::protHttp, InetUrl::protHttps)) {
			THROW(SendHttpQuery(rB, tp, 0));
			THROW(tp.CheckRepError());
			THROW(tp.GetH().Type == PPSCMD_SQ_SESSION && tp.GetH().Flags & tp.hfAck);
		}
		else if(oneof2(rB.Url.GetProtocol(), InetUrl::protAMQP, InetUrl::protAMQPS)) {
			PPMqbClient::RoutingParamEntry rpe; // маршрут до очереди, в которой сервис ждет новых клиентов
			PPMqbClient::RoutingParamEntry rpe_regular; // маршрут до очереди, в которой сервис будет с нами общаться
			PPMqbClient::MessageProperties props;
			PPMqbClient::Envelope env;
			PPMqbClient::InitParam mqip;
			THROW(PPMqbClient::SetupInitParam(mqip, "styloq", 0));
			THROW(rpe.SetupStyloQRpc(sess_pub_key, svc_ident, mqip.ConsumeParamList.CreateNewItem()));
			THROW(p_mqbc = PPMqbClient::CreateInstance(mqip));
			const long cmto = PPMqbClient::SetupMessageTtl(__DefMqbConsumeTimeout, &props);
			THROW(p_mqbc->Publish(rpe.ExchangeName, rpe.RoutingKey, &props, tp.constptr(), tp.GetAvailableSize()));				
			{
				const int cmr = p_mqbc->ConsumeMessage(env, cmto);
				THROW(cmr);
				THROW_PP_S(cmr > 0, PPERR_SQ_NOSVCRESPTOSESSQUERY, cmto);
			}
			p_mqbc->Ack(env.DeliveryTag, 0);
			THROW(tp.Read(env.Msg, 0));
			THROW(tp.CheckRepError());
			THROW(tp.GetH().Type == PPSCMD_SQ_SESSION && tp.GetH().Flags & tp.hfAck);
			// Ключ шифрования сессии есть и у нас и у сервиса!
			THROW(rpe_regular.SetupStyloQRpc(sess_pub_key, other_sess_public, 0));
			// Устанавливаем факторы RoundTrimBlock, которые нам понадобяться при последующих обменах
			if(SetRoundTripBlockReplyValues(rB, p_mqbc, rpe_regular)) {
				p_mqbc = 0;
			}
		}
	}
	CATCHZOK
	ZDELETE(p_mqbc);
	return ok;
}

int PPStyloQInterchange::Command_ClientRequest(RoundTripBlock & rB, const char * pCmdJson, const SBinaryChunk * pBlob, SSecretTagPool & rReply)
{
	rReply.Z();
	int    ok = 1;
	int    crer = -1; // Результат последнего вызова CheckRepError
	int    shttpqr = -1; // Результат последнего вызова SendHttpQuery
	SBinaryChunk sess_secret;
	THROW_PP(rB.Sess.Get(SSecretTagPool::tagSessionSecret, &sess_secret), PPERR_SQ_UNDEFSESSSECRET_INNER);
	THROW_PP(!isempty(pCmdJson), PPERR_SQ_UNDEFSVCCOMMAND);
	{
		StyloQProtocol tp;
		SBinaryChunk temp_bch;
		const SBinarySet::DeflateStrategy ds(256);
		THROW(tp.StartWriting(PPSCMD_SQ_COMMAND, StyloQProtocol::psubtypeForward));
		temp_bch.Put(pCmdJson, sstrlen(pCmdJson));
		tp.P.Put(SSecretTagPool::tagRawData, temp_bch, &ds);
		if(pBlob && pBlob->Len()) {
			// @todo В следующем вызове параметр ds не следует применять, если blob представляет собой изображение (jpeg, webp, png etc)
			// поскольку изображения и так хорошо сжаты.
			// Вероятнее всего придется усложнить метод Put с целья определения формата на-ходу
			THROW(tp.P.Put(SSecretTagPool::tagBlob, *pBlob, &ds)); 
		}
		THROW(tp.FinishWriting(&sess_secret));
		if(oneof2(rB.Url.GetProtocol(), InetUrl::protHttp, InetUrl::protHttps)) {
			THROW(shttpqr = SendHttpQuery(rB, tp, &sess_secret));
			rReply = tp.P;
			THROW(crer = tp.CheckRepError());
			THROW(tp.GetH().Type == PPSCMD_SQ_COMMAND && tp.GetH().Flags & tp.hfAck);
		}
		else if(oneof2(rB.Url.GetProtocol(), InetUrl::protAMQP, InetUrl::protAMQPS)) {
			PPMqbClient::MessageProperties props;
			PPMqbClient::Envelope env;
			THROW_PP(rB.P_Mqbc && rB.P_MqbRpe, PPERR_SQ_INVALIDMQBVALUES);
			const long cmto = PPMqbClient::SetupMessageTtl(__DefMqbConsumeTimeout, &props);
			THROW(rB.P_Mqbc->Publish(rB.P_MqbRpe->ExchangeName, rB.P_MqbRpe->RoutingKey, &props, tp.constptr(), tp.GetAvailableSize()));
			{
				const int cmr = rB.P_Mqbc->ConsumeMessage(env, cmto);
				THROW(cmr);
				THROW_PP_S(cmr > 0, PPERR_SQ_NOSVCRESPTOCMD, cmto);
			}
			rB.P_Mqbc->Ack(env.DeliveryTag, 0);
			THROW(tp.Read(env.Msg, &sess_secret));
			rReply = tp.P;
			THROW(crer = tp.CheckRepError()); // Сервис вернул ошибку: можно уходить
			THROW(tp.GetH().Type == PPSCMD_SQ_COMMAND && tp.GetH().Flags & tp.hfAck);
			/*if(tp.P.Get(SSecretTagPool::tagRawData, &temp_bch)) {
				rReply.CatN(static_cast<const char *>(temp_bch.PtrC()), temp_bch.Len());
			}*/
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::SetRoundTripBlockReplyValues(RoundTripBlock & rB, PPMqbClient * pMqbc, const PPMqbClient::RoutingParamEntry & rRpe)
{
	int    ok = 1;
	if(pMqbc && rRpe.ExchangeName.NotEmpty() && rRpe.RoutingKey.NotEmpty()) {
		if(rB.P_Mqbc != pMqbc)
			ZDELETE(rB.P_Mqbc);
		rB.P_Mqbc = pMqbc;
		ZDELETE(rB.P_MqbRpe);
		rB.P_MqbRpe = new PPMqbClient::RoutingParamEntry(rRpe);
		//p_mqbc = 0;
	}
	else
		ok = 0;
	return ok;
}

SlSRP::User * PPStyloQInterchange::InitSrpAuth(const SBinaryChunk & rCliIdent, const SBinaryChunk & rCliSecret) const
{
	SlSRP::User * p_srp_usr = 0;
	SString temp_buf;
	temp_buf.EncodeMime64(rCliIdent.PtrC(), rCliIdent.Len());
	p_srp_usr = new SlSRP::User(SlSRP::SRP_SHA1, SlSRP::SRP_NG_8192, temp_buf, rCliSecret.PtrC(), rCliSecret.Len(), /*n_hex*/0, /*g_hex*/0);
	return p_srp_usr;
}

SlSRP::Verifier * PPStyloQInterchange::InitSrpVerifier(const SBinaryChunk & rCliIdent, const SBinaryChunk & rSrpS,
	const SBinaryChunk & rSrpV, const SBinaryChunk & rA, SBinaryChunk & rResultB) const
{
	SlSRP::Verifier * p_vrf = 0;
	SString temp_buf;
	temp_buf.EncodeMime64(rCliIdent.PtrC(), rCliIdent.Len());
	p_vrf = new SlSRP::Verifier(SlSRP::SRP_SHA1, SlSRP::SRP_NG_8192, temp_buf, rSrpS, rSrpV, rA, rResultB, /*n_hex*/0, /*g_hex*/0);
	return p_vrf;
}

SlSRP::Verifier * PPStyloQInterchange::CreateSrpPacket_Svc_Auth(const SBinaryChunk & rMyPub, const SBinaryChunk & rCliIdent, const SBinaryChunk & rSrpS, 
	const SBinaryChunk & rSrpV, const SBinaryChunk & rA, StyloQProtocol & rP)
{
	rP.Z();
	SBinaryChunk __b;
	SlSRP::Verifier * p_vrf = InitSrpVerifier(rCliIdent, rSrpS, rSrpV, rA, __b);
	//SlSRP::Verifier srp_ver(SlSRP::SRP_SHA1, SlSRP::SRP_NG_8192, user_name_text, srp_s, srp_v, __a, __b, /*n_hex*/0, /*g_hex*/0);
	//const uchar * p_bytes_HAMK = 0;
	if(__b.Len() == 0) { // @error (Verifier SRP-6a safety check violated)
		rP.StartWriting(PPSCMD_SQ_SRPAUTH, StyloQProtocol::psubtypeReplyError);
		CALLEXCEPT();
	}
	else {
		// Host -> User: (bytes_s, bytes_B) 
		rP.StartWriting(PPSCMD_SQ_SRPAUTH, StyloQProtocol::psubtypeReplyOk);
		rP.P.Put(SSecretTagPool::tagSrpB, __b);
		rP.P.Put(SSecretTagPool::tagSrpVerifierSalt, rSrpS);
		//}
		rP.P.Put(SSecretTagPool::tagSessionPublicKey, rMyPub);
	}
	THROW(rP.FinishWriting(0));
	CATCH
		ZDELETE(p_vrf);
	ENDCATCH
	return p_vrf;
}

int PPStyloQInterchange::CreateSrpPacket_Cli_Auth(SlSRP::User * pU, const SBinaryChunk & rSvcIdent, const SBinaryChunk & rCliIdent, 
	const SBinaryChunk & rSessPubKey, const SBinaryChunk * pSessSecret, StyloQProtocol & rP)
{
	int    ok = 1;
	char * p_auth_ident = 0;
	SBinaryChunk temp_chunk; // В этом блоке temp_chunk работает как фактор A
	pU->StartAuthentication(&p_auth_ident, temp_chunk);
	// User -> Host: (ident, __a) 
	rP.StartWriting(PPSCMD_SQ_SRPAUTH, 0);
	//tp.P.Put(SSecretTagPool::tag)
	if(/*kex_generated*/rSessPubKey.Len()) {
		//assert(rSessPubKey.Len()); // Мы его только что сгенерировали (see above)
		rP.P.Put(SSecretTagPool::tagSessionPublicKey, rSessPubKey);
	}
	rP.P.Put(SSecretTagPool::tagSrpA, temp_chunk);
	rP.P.Put(SSecretTagPool::tagClientIdent, rCliIdent);
	rP.P.Put(SSecretTagPool::tagSvcIdent, rSvcIdent);
	assert(!pSessSecret || pSessSecret->Len());
	assert(pSessSecret || rP.P.Get(SSecretTagPool::tagSessionPublicKey, 0));
	THROW(rP.FinishWriting(pSessSecret));
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::CreateSrpPacket_Cli_Auth2(const SBinaryChunk & rM, const SBinaryChunk & rCliIdent, StyloQProtocol & rP, int * pSrpProtocolFault)
{
	int    ok = 1;
	if(!rM.Len()) { // @error User SRP-6a safety check violation
		ASSIGN_PTR(pSrpProtocolFault, 1);
		rP.StartWriting(PPSCMD_SQ_SRPAUTH_S2, StyloQProtocol::psubtypeForwardError);
		// Здесь текст и код ошибки нужны
	}
	else {
		// User -> Host: (bytes_M) 
		rP.StartWriting(PPSCMD_SQ_SRPAUTH_S2, 0);
		rP.P.Put(SSecretTagPool::tagSrpM, rM);
		rP.P.Put(SSecretTagPool::tagClientIdent, rCliIdent);
	}
	THROW(rP.FinishWriting(0)); // несмотря на то, что у нас есть теперь ключ шифрования, этот roundtrip завершаем без шифровки пакетов
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::CreateSrpPacket_Cli_HAMK(SlSRP::User * pU, const SBinaryChunk & rHamk, StyloQProtocol & rP, int * pSrpProtocolFault)
{
	int    ok = 1;
	THROW(rHamk.Len() == pU->GetSessionKeyLength());
	pU->VerifySession(static_cast<const uchar *>(rHamk.PtrC()));
	if(!pU->IsAuthenticated()) { // @error Server authentication failed
		ASSIGN_PTR(pSrpProtocolFault, 1);
		rP.StartWriting(PPSCMD_SQ_SRPAUTH_ACK, StyloQProtocol::psubtypeForwardError);
		// Здесь текст и код ошибки нужны
	}
	else {
		//
		// Это - последнее послание сервису при верфикции, потому здесь вставляем время жизни сессии
		//
		const uint32 sess_expiry_period = GetNominalSessionLifeTimeSec();
		rP.StartWriting(PPSCMD_SQ_SRPAUTH_ACK, StyloQProtocol::psubtypeForward);
		rP.P.Put(SSecretTagPool::tagSessionExpirPeriodSec, &sess_expiry_period, sizeof(sess_expiry_period));
	}
	THROW(rP.FinishWriting(0)); // несмотря на то, что у нас есть теперь ключ шифрования, этот roundtrip завершаем без шифровки пакетов
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::Verification_ClientRequest(RoundTripBlock & rB)
{
	//
	// Это - инициирующий запрос и одновременно с авторизацией устанавливает ключи обмена.
	// Все фазы SRP-авторизации осуществляются без шифрования! 
	//
	int    ok = 1;
	bool   do_register_session = false;
	int    srp_protocol_fault = 0; // Если !0 то возникла ошибка в верификации. Такие ошибки обрабатываются специальным образом.
	uint32 cli_session_expiry_period = 0;
	uint32 svc_session_expiry_period = 0;
	SlSRP::User * p_srp_usr = 0;
	SString temp_buf;
	StyloQCore::StoragePacket svc_pack;
	SBinaryChunk svc_ident;
	PPMqbClient * p_mqbc = 0;
	PPMqbClient::RoutingParamEntry rpe; // маршрут до очереди, в которой сервис ждет новых клиентов
	PPMqbClient::RoutingParamEntry * p_rpe = 0;
	PPMqbClient::RoutingParamEntry * p_rpe_init = 0; // if !0 then the first call to the service will be made through it, else through p_rpe
	bool   kex_generated = false;
	SBinaryChunk temp_chunk;
	SBinaryChunk cli_ident;
	SBinaryChunk cli_secret;
	SBinaryChunk sess_pub_key;
	SBinaryChunk other_sess_public;
	SBinaryChunk _sess_secret;
	SBinaryChunk * p_sess_secret = 0;
	THROW_PP(rB.Other.Get(SSecretTagPool::tagSvcIdent, &svc_ident), PPERR_SQ_UNDEFSVCID);
	if(rB.Sess.Get(SSecretTagPool::tagSessionSecret, &_sess_secret)) {
		p_sess_secret = &_sess_secret;
	}
	else {
		THROW(KexGenerateKeys(rB.Sess, 0, 0));
		THROW_PP(rB.Sess.Get(SSecretTagPool::tagSessionPublicKey, &sess_pub_key), PPERR_SQ_UNDEFSESSPUBKEY_INNER); // Если KexGenerateKeys отработал верно, 
			// то ошибки здесь быть не может. Если все же возникла, то у нас серьезные проблемы с реализацией.
		kex_generated = true;
	}
	THROW_PP(rB.Sess.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_UNDEFCLIID);
	THROW_PP(rB.Sess.Get(SSecretTagPool::tagSecret, &cli_secret), PPERR_SQ_UNDEFSESSSECRET_INNER);
	{
		StyloQProtocol tp;
		SBinaryChunk __m; // M
		SBinaryChunk srp_s;
		p_srp_usr = InitSrpAuth(cli_ident, cli_secret);
		THROW(CreateSrpPacket_Cli_Auth(p_srp_usr, svc_ident, cli_ident, sess_pub_key, p_sess_secret, tp));
		if(oneof2(rB.Url.GetProtocol(), InetUrl::protHttp, InetUrl::protHttps)) {
			THROW(SendHttpQuery(rB, tp, p_sess_secret));
			THROW(tp.CheckRepError()); // Сервис вернул ошибку: можно уходить - верификация не пройдена
			THROW(tp.GetH().Type == PPSCMD_SQ_SRPAUTH && tp.GetH().Flags & tp.hfAck);
			//
			tp.P.Get(SSecretTagPool::tagSessionPublicKey, &other_sess_public);
			THROW(KexGenerageSecret(rB.Sess, tp.P));
			THROW(rB.Sess.Get(SSecretTagPool::tagSessionSecret, &_sess_secret));
			// Теперь ключ шифрования сессии есть и у нас и у сервиса!
			{
				temp_chunk.Z(); // // В этом блоке temp_chunk работает как фактор B
				tp.P.Get(SSecretTagPool::tagSrpB, &temp_chunk);
				tp.P.Get(SSecretTagPool::tagSrpVerifierSalt, &srp_s);
				p_srp_usr->ProcessChallenge(srp_s, temp_chunk, __m);
			}
			THROW(CreateSrpPacket_Cli_Auth2(__m, cli_ident, tp, &srp_protocol_fault));
			THROW(SendHttpQuery(rB, tp, p_sess_secret));
			//
			THROW(tp.CheckRepError()); // Сервис вернул ошибку: можно уходить - верификация не пройдена
			THROW(tp.GetH().Type == PPSCMD_SQ_SRPAUTH_S2 && tp.GetH().Flags & tp.hfAck);
			{
				temp_chunk.Z(); // В этом блоке temp_chunk работает как фактор HAMK
				THROW(tp.P.Get(SSecretTagPool::tagSrpHAMK, &temp_chunk));
				THROW(CreateSrpPacket_Cli_HAMK(p_srp_usr, temp_chunk, tp, &srp_protocol_fault));
			}
			//
			// CreateSrpPacket_Cli_HAMK должна была затолкать в пакет наш вариант времени жизни сессии
			//
			if(tp.P.Get(SSecretTagPool::tagSessionExpirPeriodSec, &temp_chunk) && temp_chunk.Len() == sizeof(uint32)) {
				cli_session_expiry_period = PTR32C(temp_chunk.PtrC())[0];
			}
			THROW(SendHttpQuery(rB, tp, p_sess_secret));
			{
				THROW(tp.CheckRepError()); // Сервис вернул ошибку: можно уходить - верификация пройдена, но что-то в последний момент пошло не так
				THROW(tp.GetH().Type == PPSCMD_SQ_SRPAUTH_ACK && tp.GetH().Flags & tp.hfAck); // То же, что и выше
				if(tp.P.Get(SSecretTagPool::tagSessionExpirPeriodSec, &temp_chunk) && temp_chunk.Len() == sizeof(uint32)) {
					svc_session_expiry_period = PTR32C(temp_chunk.PtrC())[0];
				}
			}
			// Это было завершающее сообщение. Если все OK то сервис ждет от нас команды, если нет, то можно уходить - свадьбы не будет
			THROW(!srp_protocol_fault);
			do_register_session = true;
		}
		else if(oneof2(rB.Url.GetProtocol(), InetUrl::protAMQP, InetUrl::protAMQPS)) {
			assert((rB.P_Mqbc && rB.P_MqbRpe) || (!rB.P_Mqbc && !rB.P_MqbRpe));
			if(rB.P_Mqbc && rB.P_MqbRpe) {
				p_mqbc = rB.P_Mqbc;
				p_rpe = rB.P_MqbRpe;
				p_rpe_init = p_rpe;
			}
			if(!p_mqbc) {
				PPMqbClient::InitParam mqip;
				THROW(PPMqbClient::SetupInitParam(mqip, "styloq", 0));
				if(rpe.SetupStyloQRpc(sess_pub_key, svc_ident, mqip.ConsumeParamList.CreateNewItem())) {
					THROW(p_mqbc = PPMqbClient::CreateInstance(mqip));
					p_rpe_init = &rpe;
				}
			}
			{
				PPMqbClient::RoutingParamEntry rpe_regular; // маршрут до очереди, в которой сервис будет с нами общаться
				PPMqbClient::MessageProperties props;
				PPMqbClient::Envelope env;
				{
					const long cmto = PPMqbClient::SetupMessageTtl(__DefMqbConsumeTimeout, &props);
					THROW(p_mqbc->Publish(p_rpe_init->ExchangeName, p_rpe_init->RoutingKey, &props, tp.constptr(), tp.GetAvailableSize()));
					{
						const int cmr = p_mqbc->ConsumeMessage(env, cmto);
						THROW(cmr);
						THROW_PP_S(cmr > 0, PPERR_SQ_NOSVCRESPTOSRPAUTH1, cmto);
					}
					p_mqbc->Ack(env.DeliveryTag, 0);
				}
				THROW(tp.Read(env.Msg, 0));
				THROW(tp.CheckRepError()); // Сервис вернул ошибку: можно уходить - верификация не пройдена
				THROW(tp.GetH().Type == PPSCMD_SQ_SRPAUTH && tp.GetH().Flags & tp.hfAck);
				//
				tp.P.Get(SSecretTagPool::tagSessionPublicKey, &other_sess_public);
				THROW(KexGenerageSecret(rB.Sess, tp.P));
				THROW(rB.Sess.Get(SSecretTagPool::tagSessionSecret, &_sess_secret));
				// Теперь ключ шифрования сессии есть и у нас и у сервиса!
				{
					temp_chunk.Z(); // // В этом блоке temp_chunk работает как фактор B
					tp.P.Get(SSecretTagPool::tagSrpB, &temp_chunk);
					tp.P.Get(SSecretTagPool::tagSrpVerifierSalt, &srp_s);
					p_srp_usr->ProcessChallenge(srp_s, temp_chunk, __m);
				}
				THROW(rpe_regular.SetupStyloQRpc(sess_pub_key, other_sess_public, 0));
				THROW(CreateSrpPacket_Cli_Auth2(__m, cli_ident, tp, &srp_protocol_fault));
				{
					const long cmto = PPMqbClient::SetupMessageTtl(__DefMqbConsumeTimeout, &props);
					THROW(p_mqbc->Publish(rpe_regular.ExchangeName, rpe_regular.RoutingKey, &props, tp.constptr(), tp.GetAvailableSize()));
					THROW(!srp_protocol_fault);
					{
						const int cmr = p_mqbc->ConsumeMessage(env, cmto);
						THROW(cmr);
						THROW_PP_S(cmr > 0, PPERR_SQ_NOSVCRESPTOSRPAUTH2, cmto);
					}
					p_mqbc->Ack(env.DeliveryTag, 0);
				}
				THROW(tp.Read(env.Msg, 0));
				THROW(tp.CheckRepError()); // Сервис вернул ошибку: можно уходить - верификация не пройдена
				THROW(tp.GetH().Type == PPSCMD_SQ_SRPAUTH_S2 && tp.GetH().Flags & tp.hfAck);
				{
					temp_chunk.Z(); // В этом блоке temp_chunk работает как фактор HAMK
					THROW(tp.P.Get(SSecretTagPool::tagSrpHAMK, &temp_chunk));
					THROW(CreateSrpPacket_Cli_HAMK(p_srp_usr, temp_chunk, tp, &srp_protocol_fault));
					//
					// CreateSrpPacket_Cli_HAMK должна была затолкать в пакет наш вариант времени жизни сессии
					//
					if(tp.P.Get(SSecretTagPool::tagSessionExpirPeriodSec, &temp_chunk)) {
						if(temp_chunk.Len() == sizeof(uint32))
							cli_session_expiry_period = PTR32C(temp_chunk.PtrC())[0];
					}
				}
				{
					const long cmto = PPMqbClient::SetupMessageTtl(__DefMqbConsumeTimeout, &props);
					THROW(p_mqbc->Publish(rpe_regular.ExchangeName, rpe_regular.RoutingKey, &props, tp.constptr(), tp.GetAvailableSize()));
					// Это было завершающее сообщение. Если все OK то сервис ждет от нас команды, если нет, то можно уходить - свадьбы не будет
					THROW(!srp_protocol_fault);
					{
						const int cmr = p_mqbc->ConsumeMessage(env, cmto);
						THROW(cmr);
						THROW_PP_S(cmr > 0, PPERR_SQ_NOSVCRESPTOSRPHAMK, cmto);
					}
					p_mqbc->Ack(env.DeliveryTag, 0);
					if(tp.P.Get(SSecretTagPool::tagSessionExpirPeriodSec, &temp_chunk) && temp_chunk.Len() == sizeof(uint32))
						svc_session_expiry_period = PTR32C(temp_chunk.PtrC())[0];
				}
				// Устанавливаем факторы RoundTrimBlock, которые нам понадобяться при последующих обменах
				if(SetRoundTripBlockReplyValues(rB, p_mqbc, rpe_regular)) {
					p_mqbc = 0;
				}
				do_register_session = true;
			}
		}
	}
	if(do_register_session) {
		PPID   sess_id = 0;
		StyloQCore::StoragePacket sess_pack;
		// Теперь надо сохранить параметры сессии дабы в следующий раз не проделывать столь сложную процедуру
		//
		// Проверки assert'ами (не THROW) реализуются из-за того, что не должно возникнуть ситуации, когда мы
		// попали в этот участок кода с невыполненными условиями (то есть при необходимости THROW должны были быть вызваны выше).
		assert(rB.Sess.Get(SSecretTagPool::tagSessionSecret, &temp_chunk));
		assert(temp_chunk == _sess_secret);
		assert(sess_pub_key.Len());
		assert(other_sess_public.Len());
		assert(rB.Sess.Get(SSecretTagPool::tagSessionPrivateKey, 0));
		sess_pack.Pool.Put(SSecretTagPool::tagSessionPublicKey, sess_pub_key);
		{
			rB.Sess.Get(SSecretTagPool::tagSessionPrivateKey, &temp_chunk);
			sess_pack.Pool.Put(SSecretTagPool::tagSessionPrivateKey, temp_chunk);
		}
		sess_pack.Pool.Put(SSecretTagPool::tagSessionPublicKeyOther, other_sess_public);
		sess_pack.Pool.Put(SSecretTagPool::tagSessionSecret, _sess_secret);
		sess_pack.Pool.Put(SSecretTagPool::tagSvcIdent, svc_ident);
		if(cli_session_expiry_period || svc_session_expiry_period) {
			uint32 sep = 0;
			if(!cli_session_expiry_period || !svc_session_expiry_period)
				sep = MAX(cli_session_expiry_period, svc_session_expiry_period);
			else
				sep = MIN(cli_session_expiry_period, svc_session_expiry_period);
			if(sep)
				sess_pack.Rec.Expiration.SetTimeT(time(0) + sep);
		}
		THROW(StoreSession(&sess_id, &sess_pack, 1));
	}
	CATCHZOK
	delete p_srp_usr;
	return ok;
}

int PPStyloQInterchange::Dump()
{
	int    ok = 1;
	SString temp_buf;
	SString out_buf;
	StyloQSecTbl::Key0 k0;
	StyloQCore::StoragePacket spack;
	SBinaryChunk chunk;
	PPGetFilePath(PPPATH_OUT, "styloqsec.dump", temp_buf);
	SFile f_out(temp_buf, SFile::mWrite);
	THROW_SL(f_out.IsValid());
	THROW_PP(P_T, PPERR_SQ_UNDEFINNERSTQIDBTBL);
	MEMSZERO(k0);
	out_buf.Z().Cat("ID").Tab().Cat("Kind").Tab().Cat("CorrespondID").Tab().Cat("BI").Tab().Cat("SessExpiration");
	f_out.WriteLine(out_buf.CR());
	if(P_T->search(0, &k0, spFirst)) do {
		if(P_T->ReadCurrentPacket(&spack)) {
			temp_buf.Z().EncodeMime64(spack.Rec.BI, sizeof(spack.Rec.BI));
			out_buf.Z().Cat(spack.Rec.ID).Tab().Cat(spack.Rec.Kind).Tab().Cat(spack.Rec.CorrespondID).Tab().
				Cat(temp_buf).Tab().Cat(spack.Rec.Expiration, DATF_ISO8601CENT, 0);
			f_out.WriteLine(out_buf.CR());
			uint32 cid = 0;
			out_buf.Z();
			for(size_t pp = 0; spack.Pool.Enum(&pp, &cid, &chunk) > 0;) {
				chunk.Mime64(temp_buf);
				out_buf.Tab().Cat(cid).CatChar('=').Cat(temp_buf).CR();
			}
			f_out.WriteLine(out_buf.CR());
		}
	} while(P_T->search(0, &k0, spNext));
	P_T->destroyLobData(P_T->VT); // @fix @v11.4.9
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::TestDatabase()
{
	int    ok = 1;
	class InnerBlock {
	public:
		InnerBlock() : Timestamp(time(0) * 1000), Expiration(plusdatetime(getcurdatetime_(), 3 * 24 * 3600, 3)), Kind(StyloQCore::kClient), Id(0)
		{
			Ident.Randomize(sizeof(((StyloQCore::StoragePacket *)0)->Rec.BI));
		}
		int    CreateDocument(int direction, int docType, StyloQCore::StoragePacket & rPack)
		{
			int    ok = 1;
			SString temp_buf;
			assert((direction > 0) || (direction < 0));
			assert(Ident.Len() == sizeof(rPack.Rec.BI));
			memcpy(rPack.Rec.BI, Ident.PtrC(), sizeof(rPack.Rec.BI));
			rPack.Rec.DocType = docType;
			rPack.Rec.Expiration = Expiration;
			rPack.Rec.Kind = (direction > 0) ? StyloQCore::kDocOutcoming : StyloQCore::kDocIncoming;
			rPack.Rec.TimeStamp = Timestamp;
			{
				//"doctype"
				SJson js(SJson::tOBJECT);
				if(docType == StyloQCore::doctypCommandList)
					js.InsertString("doctype", "commandlist");
				js.InsertString("time", temp_buf.Z().Cat(Timestamp));
				js.Insert("expir_time_sec", json_new_number(temp_buf.Z().Cat(3 * 24 * 3600)));
				{
					SJson * p_array = SJson::CreateArr();
					{
						SJson * p_jitem = SJson::CreateObj();
						p_jitem->InsertString("uuid", temp_buf.Z().Cat(S_GUID(SCtrGenerate_)));
						p_jitem->InsertString("name", "command #1");
						p_jitem->InsertString("descr", "command #1 description");
						// @todo transmit image
						json_insert_child(p_array, p_jitem);
					}
					{
						SJson * p_jitem = SJson::CreateObj();
						p_jitem->InsertString("uuid", temp_buf.Z().Cat(S_GUID(SCtrGenerate_)));
						p_jitem->InsertString("name", "command #2");
						p_jitem->InsertString("descr", "command #2 description");
						// @todo transmit image
						json_insert_child(p_array, p_jitem);
					}
					{
						SJson * p_jitem = SJson::CreateObj();
						p_jitem->InsertString("uuid", temp_buf.Z().Cat(S_GUID(SCtrGenerate_)));
						p_jitem->InsertString("name", "command #3");
						p_jitem->InsertString("descr", "command #3 description");
						// @todo transmit image
						json_insert_child(p_array, p_jitem);
					}
					js.Insert("item_list", p_array);
				}
				THROW_SL(js.ToStr(temp_buf));
				THROW_SL(rPack.Pool.Put(SSecretTagPool::tagRawData, temp_buf.cptr(), temp_buf.Len()));
				THROW_SL(rPack.Pool.Put((direction > 0) ? SSecretTagPool::tagClientIdent : SSecretTagPool::tagSvcIdent, Ident));
			}
			CATCHZOK
			return ok;
		}
		int    CreatePacket(StyloQCore::StoragePacket & rPack)
		{
			int    ok = 1;
			SString temp_buf;
			assert(Ident.Len() == sizeof(rPack.Rec.BI));
			{
				SJson js(SJson::tOBJECT);
				js.InsertString("key1", "val1");
				js.InsertString("key2", "val2");
				js.InsertString("key3", "val3");
				THROW_SL(js.ToStr(temp_buf));
				{
					const SJson * p_c = 0;
					p_c = js.FindChildByKey("key1");
					THROW(p_c && p_c->Text.IsEqiAscii("val1"));
					p_c = js.FindChildByKey("key2");
					THROW(p_c && p_c->Text.IsEqiAscii("val2"));
					p_c = js.FindChildByKey("key3");
					THROW(p_c && p_c->Text.IsEqiAscii("val3"));
				}
				THROW_SL(rPack.Pool.Put(SSecretTagPool::tagRawData, temp_buf.cptr(), temp_buf.Len()));
			}
			THROW_SL(rPack.Pool.Put(SSecretTagPool::tagClientIdent, Ident));
			memcpy(rPack.Rec.BI, Ident.PtrC(), Ident.Len());
			rPack.Rec.Kind = Kind;
			rPack.Rec.Expiration = Expiration;
			rPack.Rec.TimeStamp = Timestamp;
			CATCHZOK
			return ok;
		}
		int    VerifyPacket(const StyloQCore::StoragePacket & rReadPack)
		{
			int    ok = 1;
			SJson * p_js = 0;
			SString temp_buf;
			SBinaryChunk bc_temp;
			THROW(rReadPack.Rec.ID == Id);
			THROW(rReadPack.Rec.Kind == Kind);
			//THROW(rReadPack.Rec.Expiration == Expiration);
			//THROW(rReadPack.Rec.TimeStamp == Timestamp);
			THROW(Ident.IsEq(rReadPack.Rec.BI, sizeof(rReadPack.Rec.BI)));
			THROW_SL(rReadPack.Pool.Get(SSecretTagPool::tagClientIdent, &bc_temp));
			THROW(bc_temp.IsEq(rReadPack.Rec.BI, sizeof(rReadPack.Rec.BI)));
			THROW_SL(p_js = rReadPack.Pool.GetJson(SSecretTagPool::tagRawData));
			{
				const SJson * p_c = 0;
				p_c = p_js->FindChildByKey("key1");
				THROW(p_c && p_c->Text.IsEqiAscii("val1"));
				p_c = p_js->FindChildByKey("key2");
				THROW(p_c && p_c->Text.IsEqiAscii("val2"));
				p_c = p_js->FindChildByKey("key3");
				THROW(p_c && p_c->Text.IsEqiAscii("val3"));
			}
			CATCHZOK
			ZDELETE(p_js);
			return ok;
		}
		const  time_t Timestamp;
		const  LDATETIME Expiration;
		const  int Kind;
		PPID   Id;
		SBinaryChunk Ident;
	};
	InnerBlock __tb;
	SString temp_buf;
	SJson * p_js = 0;
	PPTransaction tra(1);
	THROW(tra);
	{
		StyloQCore::StoragePacket pack;
		THROW(__tb.CreatePacket(pack));
		THROW(P_T->PutPeerEntry(&__tb.Id, &pack, 0));
	}
	{
		StyloQCore::StoragePacket rpack;
		THROW(P_T->GetPeerEntry(__tb.Id, &rpack) > 0);
		THROW(__tb.VerifyPacket(rpack));
	}
	{
		StyloQCore::StoragePacket rpack;
		THROW(P_T->SearchGlobalIdentEntry(__tb.Kind, __tb.Ident, &rpack) > 0);
		THROW(__tb.VerifyPacket(rpack));
	}
	{
		PPID  id1 = 0;
		PPID  id2 = 0;
		const int doc_direction = -1;
		const int doc_type = StyloQCore::doctypCommandList;
		StyloQCore::StoragePacket doc_pack;
		THROW(__tb.CreateDocument(doc_direction, doc_type, doc_pack));
		THROW(P_T->PutDocument(&id1, __tb.Ident, doc_direction, doc_pack.Rec.DocType, __tb.Ident, doc_pack.Pool, 0));
		{
			PPIDArray id_list;
			THROW(P_T->GetDocIdListByType(doc_direction, doc_pack.Rec.DocType, &__tb.Ident, id_list) > 0);
			THROW(id_list.getCount() == 1);
			THROW(id_list.get(0) == id1);
		}
		//
		// Пытаемся добавить еще один экземпляр того же документа.
		// Для типа документа StyloQCore::doctypCommandList функция должна удалить те, что есть и создать новый.
		//
		THROW(P_T->PutDocument(&id2, __tb.Ident, doc_direction, doc_pack.Rec.DocType, __tb.Ident, doc_pack.Pool, 0));
		{
			PPIDArray id_list;
			THROW(id2 > id1);
			THROW(P_T->GetDocIdListByType(doc_direction, doc_pack.Rec.DocType, &__tb.Ident, id_list) > 0);
			THROW(id_list.getCount() == 1);
			THROW(id_list.get(0) == id2);
			{
				StyloQCore::StoragePacket rpack;
				THROW(P_T->GetPeerEntry(id2, &rpack) > 0);
				THROW(rpack.Rec.Kind == doc_pack.Rec.Kind);
				THROW(rpack.Rec.DocType == doc_pack.Rec.DocType);
				// Функция StyloQCore::PutDocument самостоятельно инициализирует Expiration и TimeStamp на основе тела документа
				// возможно, какие-то нюансы такого поведения будут изменены в дальнейшем.
				//THROW(rpack.Rec.Expiration == doc_pack.Rec.Expiration);
				//THROW(rpack.Rec.TimeStamp == doc_pack.Rec.TimeStamp);
				THROW(rpack.Rec.LinkObjType == 0);
				THROW(rpack.Rec.LinkObjID == 0);
				THROW(__tb.Ident.IsEq(rpack.Rec.BI, sizeof(rpack.Rec.BI)));
				{
					SBinaryChunk bc_temp;
					const int _other_id_tag = (doc_direction > 0) ? SSecretTagPool::tagClientIdent : SSecretTagPool::tagSvcIdent;
					THROW_SL(rpack.Pool.Get(_other_id_tag, &bc_temp));
					THROW(bc_temp.IsEq(rpack.Rec.BI, sizeof(rpack.Rec.BI)));
					//
					THROW_SL(p_js = rpack.Pool.GetJson(SSecretTagPool::tagRawData));
					THROW(p_js->IsObject());
					{
						const SJson * p_c = 0;
						p_c = p_js->FindChildByKey("doctype");
						if(doc_type == StyloQCore::doctypCommandList) {
							THROW(p_c && p_c->Text.IsEqiAscii("commandlist"));
						}
						p_c = p_js->FindChildByKey("time");
						THROW(p_c);
						p_c = p_js->FindChildByKey("expir_time_sec");
						THROW(p_c);
						p_c = p_js->FindChildByKey("item_list");
						THROW(p_c && p_c->IsArray());
						{
							for(const SJson * p_ai = p_c->P_Child->P_Child; p_ai; p_ai = p_ai->P_Next) {
								THROW(p_ai->IsObject());
								const SJson * p_i = 0;
								p_i = p_ai->FindChildByKey("uuid");
								THROW(p_i && p_i->IsString());
								p_i = p_ai->FindChildByKey("name");
								THROW(p_i && p_i->IsString());
								p_i = p_ai->FindChildByKey("descr");
								THROW(p_i && p_i->IsString());
							}
						}
					}
				}
			}
			{
				THROW(P_T->PutPeerEntry(&id2, 0, 0)); // delete document entry
				THROW(P_T->GetDocIdListByType(doc_direction, doc_pack.Rec.DocType, &__tb.Ident, id_list) < 0);
				THROW(id_list.getCount() == 0);
			}
		}
	}
	{
		StyloQCore::StoragePacket rpack;
		THROW(P_T->PutPeerEntry(&__tb.Id, 0, 0)); // delete entry
		THROW(P_T->GetPeerEntry(__tb.Id, &rpack) < 0 && PPErrCode == PPERR_SQ_ENTRYNFOUND);
		THROW(P_T->SearchGlobalIdentEntry(__tb.Kind, __tb.Ident, &rpack) < 0 && PPErrCode == PPERR_SQ_ENTRYIDENTNFOUND);
	}
	THROW(tra.Commit());
	CATCHZOK
	delete p_js;
	return ok;
}

int PPStyloQInterchange::SearchSession(const SBinaryChunk & rOtherPublic, StyloQCore::StoragePacket * pPack)
{
	return P_T ? P_T->SearchSession(rOtherPublic, pPack) : -1;
}

int StyloQCore::GeneratePublicIdent(const SSecretTagPool & rOwnPool, const SBinaryChunk & rSvcIdent, uint resultIdentTag, long flags, SSecretTagPool & rPool)
{
	assert(oneof2(resultIdentTag, SSecretTagPool::tagSvcIdent, SSecretTagPool::tagClientIdent));
	int    ok = 1;
	SBinaryChunk prmrn;
	SBinaryChunk ag;
	THROW_PP(rOwnPool.Get(SSecretTagPool::tagPrimaryRN, &prmrn), PPERR_SQ_UNDEFINNERPRNTAG);
	THROW_PP(rOwnPool.Get(SSecretTagPool::tagAG, &ag), PPERR_SQ_UNDEFINNERAGTAG);
	prmrn.Cat(rSvcIdent.PtrC(), rSvcIdent.Len());
	prmrn.Cat(ag.PtrC(), ag.Len());
	{
		binary160 hash = SlHash::Sha1(0, prmrn.PtrC(), prmrn.Len());
		assert(sizeof(hash) == 20);
		rPool.Put(resultIdentTag, &hash, sizeof(hash));
		if(flags & gcisfMakeSecret) {
			SBinaryChunk secret;
			secret.Cat(ag.PtrC(), ag.Len());
			secret.Cat(rSvcIdent.PtrC(), rSvcIdent.Len());
			binary160 secret_hash = SlHash::Sha1(0, secret.PtrC(), secret.Len());
			assert(sizeof(secret_hash) == 20);
			rPool.Put(SSecretTagPool::tagSecret, &secret_hash, sizeof(secret_hash));
		}
	}
	CATCHZOK
	return ok;
}

int StyloQCore::SetupPeerInstance(PPID * pID, int use_ta)
{
	int    ok = -1;
	BN_CTX * p_bn_ctx = 0;
	BIGNUM * p_rn = 0;
	PPID   id = 0;
	SString temp_buf;
	StyloQCore::StoragePacket ex_pack;
	StyloQCore::StoragePacket new_pack;
	StyloQCore::StoragePacket * p_pack_to_export = 0;
	SBinaryChunk public_ident;
	if(GetOwnPeerEntry(&ex_pack) > 0) {
		; // Запись уже существует
		p_pack_to_export = &ex_pack;
		ASSIGN_PTR(pID, ex_pack.Rec.ID);
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
			SObfuscateBuffer(temp, seed_size);
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
		{
			const SBinaryChunk fpi(temp, rn_len);
			new_pack.Pool.Put(SSecretTagPool::tagFPI, fpi);
			new_pack.Pool.Put(SSecretTagPool::tagAG, &S_GUID(SCtrGenerate_), sizeof(S_GUID)); // Autogeneration
			{
				public_ident.Z();
				THROW(GeneratePublicIdent(new_pack.Pool, fpi, SSecretTagPool::tagSvcIdent, 0, new_pack.Pool));
				const int r = new_pack.Pool.Get(SSecretTagPool::tagSvcIdent, &public_ident);
				assert(r);
				assert(public_ident.Len() <= sizeof(new_pack.Rec.BI));
				memcpy(new_pack.Rec.BI, public_ident.PtrC(), public_ident.Len());
				new_pack.Rec.Kind = StyloQCore::kNativeService;
			}
		}
		THROW(PutPeerEntry(&id, &new_pack, 1));
		{
			//
			// Тестирование результатов
			// 
			SBinaryChunk c1;
			SBinaryChunk c2;
			const uint32 test_tag_list[] = { SSecretTagPool::tagPrimaryRN, SSecretTagPool::tagSvcIdent, SSecretTagPool::tagAG, SSecretTagPool::tagFPI };
			{
				StyloQCore::StoragePacket test_pack;
				if(GetPeerEntry(id, &test_pack) > 0) {
					for(uint i = 0; i < SIZEOFARRAY(test_tag_list); i++) {
						uint32 _tag = test_tag_list[i];
						if(new_pack.Pool.Get(_tag, &c1) && test_pack.Pool.Get(_tag, &c2)) {
							assert(c1 == c2);
							if(_tag == SSecretTagPool::tagSvcIdent) {
								assert(memcmp(c1.PtrC(), test_pack.Rec.BI, c1.Len()) == 0);
							}
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
				StyloQCore::StoragePacket test_pack;		
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
			{
				//
				// Здесь проверяем инвариантность формирования клиентского идентификатора и секрета по одному и тому же 
				// идентификатору сервиса.
				//
				const char * p_svc_ident_mime = "Wn7M3JuxUaDpiCHlWiIStn+YYkQ="; // pft
				SBinaryChunk svc_ident_test;
				const int r = svc_ident_test.FromMime64(p_svc_ident_mime);
				SSecretTagPool last_test_pool;
				assert(r);
				for(uint gi = 0; gi < 10; gi++) {
					SSecretTagPool test_pool;
					THROW(GeneratePublicIdent(new_pack.Pool, svc_ident_test, SSecretTagPool::tagClientIdent, gcisfMakeSecret, test_pool));
					if(gi > 0) {
						SBinaryChunk bch_test;
						SBinaryChunk last_bch_test;
						test_pool.Get(SSecretTagPool::tagClientIdent, &bch_test);
						last_test_pool.Get(SSecretTagPool::tagClientIdent, &last_bch_test);
						assert(bch_test.Len() > 0);
						assert(last_bch_test.Len() > 0);
						assert(bch_test == last_bch_test);
						test_pool.Get(SSecretTagPool::tagSecret, &bch_test);
						last_test_pool.Get(SSecretTagPool::tagSecret, &last_bch_test);
						assert(bch_test.Len() > 0);
						assert(last_bch_test.Len() > 0);
						assert(bch_test == last_bch_test);
					}
					last_test_pool = test_pool;
				}
			}
		}
		p_pack_to_export = &new_pack;
		ASSIGN_PTR(pID, id);
		ok = 1;
	}
	if(p_pack_to_export) {
		PPGetFilePath(PPPATH_OUT, "styloq-instance.txt", temp_buf);
		SFile f_out(temp_buf, SFile::mWrite);
		public_ident.Z();
		p_pack_to_export->Pool.Get(SSecretTagPool::tagSvcIdent, &public_ident);
		public_ident.Mime64(temp_buf);
		f_out.WriteLine(temp_buf.CR());
	}
	CATCHZOK
	BN_free(p_rn);
	BN_CTX_free(p_bn_ctx);
	return ok;
}

int PPStyloQInterchange::SetupPeerInstance(PPID * pID, int use_ta)
{
	return P_T ? P_T->SetupPeerInstance(pID, use_ta) : 0;
}
//
// Registration {
//
//
/*static*/int64 PPStyloQInterchange::EvaluateExpiryTime(int expiryPeriodSec)
{
	int64 result = 0;
	if(expiryPeriodSec > 0) {
		const time_t now_ = time(0);
		result = now_ + expiryPeriodSec;
	}
	return result;
}

/*static*/bool PPStyloQInterchange::IsExpired(int64 expiration)
{
	bool result = false;
	if(expiration <= 0)
		result = true;
	else {
		const time_t now_ = time(0);
		// Эксперсс-проверка на валидность expiration (из-за ошибки на начальной фазе разработки)
		if(expiration > now_) {
			expiration = 0;
		}
		//
		if(expiration <= 0 || expiration <= now_)
			result = true;
	}
	return result;
}

StyloQBlobInfo::StyloQBlobInfo() : BlobN(0), HashAlg(0)
{
}

StyloQBlobInfo & StyloQBlobInfo::Z()
{
	Oid.Z();
	BlobN = 0;
	Ff.Clear();
	HashAlg = 0;
	Hash.Z();
	SrcPath.Z();
	Signature.Z();
	return *this;
}

PPStyloQInterchange::SvcNotification::SvcNotification() : EventOrgTime(ZERODATETIME), EventIssueTime(ZERODATETIME), ObjNominalTime(ZERODATETIME), EventId(0), MessageId(0)
{
}
		
int PPStyloQInterchange::SvcNotification::GenerateIdent()
{
	int    ok = 1;
	SBinaryChunk buf;
	buf.Cat(&EventOrgTime, sizeof(EventOrgTime));
	buf.Cat(&ObjNominalTime, sizeof(ObjNominalTime));
	buf.Cat(&EventId, sizeof(EventId));
	buf.Cat(&Oid, sizeof(Oid));
	Ident = SlHash::Sha1(0, buf.PtrC(), buf.Len());
	return ok;
}

SJson * PPStyloQInterchange::SvcNotification::ToJson() const
{
	SJson * p_result = new SJson(SJson::tOBJECT);
	if(p_result) {
		SString temp_buf;
		if(!Ident.IsZero()) {
			temp_buf.EncodeMime64(&Ident, sizeof(Ident));
			p_result->InsertString("ident", temp_buf);
		}
		p_result->InsertString("evnt_org_time", temp_buf.Z().Cat(EventOrgTime, DATF_ISO8601CENT, 0).Escape());
		p_result->InsertString("evnt_iss_time", temp_buf.Z().Cat(EventIssueTime, DATF_ISO8601CENT, 0).Escape());
		p_result->InsertString("obj_nominal_time", temp_buf.Z().Cat(ObjNominalTime, DATF_ISO8601CENT, 0).Escape());
		// @v11.5.11 {
		if(!!CmdUuid) {
			p_result->InsertString("orgcmduuid", temp_buf.Z().Cat(CmdUuid, S_GUID::fmtIDL).Escape());
		}
		// } @v11.5.11 
		p_result->InsertIntNz("evnt", EventId);
		if(Oid.Obj) {
			p_result->InsertInt("objtype", Oid.Obj);
			p_result->InsertIntNz("objid", Oid.Id);
		}
		p_result->InsertStringNe("msg", (temp_buf = Message).Escape());
		p_result->InsertIntNz("msgid", MessageId);
	}
	return p_result;
}

PPStyloQInterchange::PPStyloQInterchange() : State(0), P_T(new StyloQCore)
{
}

PPStyloQInterchange::PPStyloQInterchange(StyloQCore * pStQC) : State(0), P_T(pStQC)
{
	if(pStQC)
		State |= stOuterStqC;
}

PPStyloQInterchange::~PPStyloQInterchange()
{
	if(!(State & stOuterStqC)) {
		ZDELETE(P_T);
	}
}
	
int PPStyloQInterchange::MakeInvitation(const InterchangeParam & rInv, SString & rInvitationData)
{
	int    ok = 1;
	SString temp_buf;
	rInvitationData.Z();
	// prefix SVCPID LOCLADDENDUM SVCCAP URL [SVCCMD [SVCCMDPARAM]] // @v11.2.3 LOCLADDENDUM
	THROW_PP(rInv.SvcIdent.Len(), PPERR_SQ_UNDEFSVCID);
	THROW_PP(rInv.AccessPoint.NotEmpty(), PPERR_SQ_UNDEFSVCACCSPOINT);
	assert(rInv.CommandJson.NotEmpty());
	THROW_PP(rInv.CommandJson.NotEmpty(), PPERR_SQ_UNDEFSVCCOMMAND);
	rInvitationData.CatChar('A'); // prefix
	rInv.SvcIdent.Mime64(temp_buf);
	rInvitationData.Cat(temp_buf);
	// @v11.2.3 LoclAddendum {
	temp_buf.Z();
	if(rInv.LoclAddendum.Len())
		rInv.LoclAddendum.Mime64(temp_buf);
	rInvitationData.CatChar('&').Cat(temp_buf);
	// } @v11.2.3 LoclAddendum
	{
		temp_buf.Z().EncodeMime64(&rInv.Capabilities, sizeof(rInv.Capabilities));
		rInvitationData.CatChar('&').Cat(temp_buf);
	}
	temp_buf.Z().EncodeMime64(rInv.AccessPoint, rInv.AccessPoint.Len());
	rInvitationData.CatChar('&').Cat(temp_buf);
	temp_buf.Z().EncodeMime64(rInv.CommandJson, rInv.CommandJson.Len());
	rInvitationData.CatChar('&').Cat(temp_buf);
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::FetchSessionKeys(int kind, SSecretTagPool & rSessCtx, const SBinaryChunk & rForeignIdent)
{
	//const  int ec_curve_name_id = NID_X9_62_prime256v1;
	int    status = fsksError;
	StyloQCore::StoragePacket pack;
	StyloQCore::StoragePacket corr_pack;
	THROW_PP(P_T, PPERR_SQ_UNDEFINNERSTQIDBTBL);
	const int sr = P_T->SearchGlobalIdentEntry(kind, rForeignIdent, &pack);
	THROW(sr);
	if(sr > 0) {
		THROW(pack.Rec.Kind == kind);
		switch(pack.Rec.Kind) {
			case StyloQCore::kSession:
				THROW(ExtractSessionFromPacket(pack, rSessCtx));
				status = fsksSessionById;
				break;
			case StyloQCore::kForeignService:
				if(pack.Rec.CorrespondID && P_T->GetPeerEntry(pack.Rec.CorrespondID, &corr_pack) > 0) {
					THROW_PP(corr_pack.Rec.Kind == StyloQCore::kSession, PPERR_SQ_WRONGDBITEMKIND); // Корреспондирующей записью может быть только сессия
					THROW(ExtractSessionFromPacket(corr_pack, rSessCtx));
					status = fsksSessionBySvcId;
				}
				else
					status = fsksNewSession;
				break;
			case StyloQCore::kClient:
				if(pack.Rec.CorrespondID && P_T->GetPeerEntry(pack.Rec.CorrespondID, &corr_pack) > 0) {
					THROW_PP(corr_pack.Rec.Kind == StyloQCore::kSession, PPERR_SQ_WRONGDBITEMKIND); // Корреспондирующей записью может быть только сессия
					if(ExtractSessionFromPacket(corr_pack, rSessCtx)) {
						status = fsksSessionByCliId;
					}
					else {
						status = fsksNewSession; // Если что-то не так с сессией, то просто считаем ее отсутствующей и возвращаем статус fsksNewSession
					}
				}
				else
					status = fsksNewSession;
				break;
			default:
				CALLEXCEPT_PP(PPERR_SQ_WRONGDBITEMKIND);
				break;
		}
	}
	else
		status = fsksNewEntry;
	CATCH
		status = fsksError;
	ENDCATCH
	return status;
}

int PPStyloQInterchange::KexGenerateKeys(SSecretTagPool & rSessCtx, BIGNUM * pDebugPubX, BIGNUM * pDebugPubY)
{
	int    ok = 1;
	EC_KEY * p_key = 0;
	SBinaryChunk public_key;
	SBinaryChunk private_key;
	BN_CTX * p_bn_ctx = BN_CTX_new();
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
		// @debug {
		if(pDebugPubX && pDebugPubY) {
			EC_POINT_get_affine_coordinates(p_ecg, p_public_ecpt, pDebugPubX, pDebugPubY, p_bn_ctx);
		}
		// } @debug 
		int bn_len = BN_num_bytes(p_private_bn);
		THROW(private_key.Ensure(bn_len));
		BN_bn2bin(p_private_bn, static_cast<uchar *>(private_key.Ptr()));
	}
	rSessCtx.Put(SSecretTagPool::tagSessionPrivateKey, private_key);
	rSessCtx.Put(SSecretTagPool::tagSessionPublicKey, public_key);
	CATCHZOK
	EC_KEY_free(p_key);
	BN_CTX_free(p_bn_ctx);
	return ok;
}

int PPStyloQInterchange::KexGenerageSecret(SSecretTagPool & rSessCtx, const SSecretTagPool & rOtherCtx)
{
	int    ok = 1;
	SBinaryChunk my_public; // @debug
	SBinaryChunk other_public;
	SBinaryChunk my_private;
	SString temp_buf;
	BN_CTX * p_bn_ctx = 0;
	EC_POINT * p_public_other = 0;
	EC_KEY * p_private_my = 0;
	BIGNUM * p_private_bn = 0;//EC_KEY_get0_private_key(p_key);
	void * p_secret = 0;
	THROW_PP(rOtherCtx.Get(SSecretTagPool::tagSessionPublicKey, &other_public), PPERR_SQ_UNDEFOTHERPUBKEY);
	THROW_PP(rSessCtx.Get(SSecretTagPool::tagSessionPublicKey, &my_public), PPERR_SQ_UNDEFSESSPUBKEY_INNER); // @debug
	THROW_PP(rSessCtx.Get(SSecretTagPool::tagSessionPrivateKey, &my_private), PPERR_SQ_UNDEFPRIVKEY);
	assert(other_public.Len() && my_public.Len() && my_private.Len());
	{
		const size_t ecptdimsize = 32;
		const size_t opl_other = other_public.Len();
		other_public.Mime64(temp_buf);
		THROW_PP_S(opl_other == (1 + ecptdimsize * 2) && static_cast<const uchar *>(other_public.PtrC())[0] == 4, PPERR_SQ_INVOTHERPUBKEY, temp_buf);
	}
	{
		p_bn_ctx = BN_CTX_new();
		p_private_my = EC_KEY_new_by_curve_name(ec_curve_name_id);
		const EC_GROUP * p_ecg2 = EC_KEY_get0_group(p_private_my);
		p_public_other = EC_POINT_new(p_ecg2);
		p_private_bn = BN_new();//EC_KEY_get0_private_key(p_key);
		THROW(EC_POINT_oct2point(p_ecg2, p_public_other, static_cast<uchar *>(other_public.Ptr()), other_public.Len(), p_bn_ctx) == 1);
		THROW(BN_bin2bn(static_cast<const uchar *>(my_private.PtrC()), my_private.Len(), p_private_bn));
		THROW(EC_KEY_set_private_key(p_private_my, p_private_bn) == 1);
		const int field_size = EC_GROUP_get_degree(EC_KEY_get0_group(p_private_my));
		size_t secret_len = (field_size + 7) / 8;
		THROW(p_secret = (uchar *)OPENSSL_malloc(secret_len)); // Failed to allocate memory for secret
		memzero(p_secret, secret_len);
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

int PPStyloQInterchange::InitRoundTripBlock(RoundTripBlock & rB)
{
	int    ok = 1;
	bool    do_generate_public_ident = true;
	SString temp_buf;
	SBinaryChunk svc_ident;
	SBinaryChunk svc_acsp;
	SBinaryChunk temp_bch;
	StyloQCore::StoragePacket svc_pack;
	THROW_PP(P_T, PPERR_SQ_UNDEFINNERSTQIDBTBL);
	THROW_PP(rB.Other.Get(SSecretTagPool::tagSvcIdent, &svc_ident), PPERR_SQ_UNDEFSVCID);
	{
		SString acsp_url;
		rB.Url.Z();
		THROW_PP(rB.Other.Get(SSecretTagPool::tagSvcAccessPoint, &svc_acsp), PPERR_SQ_UNDEFSVCACCSPOINT);
		THROW_SL(rB.Url.Parse(svc_acsp.ToRawStr(acsp_url)));
		THROW_PP_S(rB.Url.GetComponent(InetUrl::cHost, 0, temp_buf), PPERR_SQ_UNDEFSVCACCSPOINTHOST, acsp_url);
		const int proto = rB.Url.GetProtocol();
		THROW_PP_S(oneof4(proto, InetUrl::protHttp, InetUrl::protHttp, InetUrl::protAMQP, InetUrl::protAMQPS), PPERR_SQ_INVSVCACCSPOINTPROT, acsp_url);
	}
	//
	THROW(P_T->GetOwnPeerEntry(&rB.StP) > 0);
	if(P_T->SearchGlobalIdentEntry(StyloQCore::kForeignService, svc_ident, &svc_pack) > 0) {
		THROW_PP(svc_pack.Rec.Kind == StyloQCore::kForeignService, PPERR_SQ_WRONGDBITEMKIND); // Что-то не так с базой данных или с программой: такого быть не должно!
		rB.InnerSvcID = svc_pack.Rec.ID;
		if(svc_pack.Rec.CorrespondID) {
			StyloQCore::StoragePacket corr_pack;
			if(P_T->GetPeerEntry(svc_pack.Rec.CorrespondID, &corr_pack) > 0) {
				THROW_PP(corr_pack.Rec.Kind == StyloQCore::kSession, PPERR_SQ_WRONGDBITEMKIND); // Что-то не так с базой данных или с программой: такого быть не должно!
				const LDATETIME _now = getcurdatetime_();
				//if(!corr_pack.Rec.Expiration || cmp(corr_pack.Rec.Expiration, _now) > 0) {
				if(!!corr_pack.Rec.Expiration && cmp(corr_pack.Rec.Expiration, _now) > 0) {
					LongArray cid_list;
					svc_pack.Pool.Get(SSecretTagPool::tagClientIdent, &temp_bch);
					rB.Sess.Put(SSecretTagPool::tagClientIdent, temp_bch);
					cid_list.addzlist(SSecretTagPool::tagSessionPrivateKey, SSecretTagPool::tagSessionPublicKey, SSecretTagPool::tagSessionSecret, 
						SSecretTagPool::tagSvcIdent, SSecretTagPool::tagSessionPublicKeyOther, 0);
					THROW_SL(rB.Sess.CopyFrom(corr_pack.Pool, cid_list, true));
					rB.InnerSessID = corr_pack.Rec.ID;
					do_generate_public_ident = false;
				}
			}
		}
	}				
	if(do_generate_public_ident) {
		THROW(P_T->GeneratePublicIdent(rB.StP.Pool, svc_ident, SSecretTagPool::tagClientIdent, StyloQCore::gcisfMakeSecret, rB.Sess));
	}
	// @v11.1.12 {
	{
		rB.Uuid.Generate();
		// Включение этого идентификатора в общий пул сомнительно. Дело в том, что
		// этот идентификатор обрабатывается на ранней фазе получения сообщения стороной диалога
		// когда общий пакет сообщения еще не распакован (не расшифрован и т.д.)
		THROW_SL(rB.Sess.Put(SSecretTagPool::tagRoundTripIdent, &rB.Uuid, sizeof(rB.Uuid))); // ? 
	}
	// } @v11.1.12 
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::KexServiceReply(SSecretTagPool & rSessCtx, const SSecretTagPool & rCli, BIGNUM * pDebugPubX, BIGNUM * pDebugPubY)
{
	int    ok = 1;
	SBinaryChunk cli_ident;
	SBinaryChunk my_public;
	THROW_PP(rCli.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_UNDEFCLIID);
	int  fsks = FetchSessionKeys(StyloQCore::kClient, rSessCtx, cli_ident);
	THROW(fsks);
	ok = fsks;
	THROW(KexGenerateKeys(rSessCtx, pDebugPubX, pDebugPubY));
	rSessCtx.Get(SSecretTagPool::tagSessionPublicKey, &my_public);
	THROW(my_public.Len());
	THROW(KexGenerageSecret(rSessCtx, rCli));
	CATCHZOK
	return ok;
}

/*static*/S_GUID PPStyloQInterchange::GetLocalAddendum(long flag/*smqbpfLocalMachine || smqbpfLocalSession*/)
{
	S_GUID result;
	assert(oneof2(flag, smqbpfLocalMachine, smqbpfLocalSession));
	if(flag == smqbpfLocalMachine) {
		S_GUID lmid;
		SString temp_buf;
		{
			WinRegKey reg_key_r(HKEY_CURRENT_USER, PPConst::WrKey_SysSettings, 1);
			reg_key_r.GetString(PPConst::WrParam_StyloQLoclMachineUuid, temp_buf);
			if(temp_buf.NotEmpty() && lmid.FromStr(temp_buf))
				result = lmid;
		}
		if(!result) {
			{
				lmid.Generate();
				WinRegKey reg_key_w(HKEY_CURRENT_USER, PPConst::WrKey_SysSettings, 0);
				temp_buf.Z().Cat(lmid, S_GUID::fmtLower|S_GUID::fmtPlain);
				reg_key_w.PutString(PPConst::WrParam_StyloQLoclMachineUuid, temp_buf);
			}
			{ // Так как нам очень важно, что ключ постоянно один для всех сеансов на этой машине, 
				// то мы не просто воспользуемся сгенерированным ключем, а для страховки извлечем его и реестра.
				WinRegKey reg_key_r(HKEY_CURRENT_USER, PPConst::WrKey_SysSettings, 1);
				reg_key_r.GetString(PPConst::WrParam_StyloQLoclMachineUuid, temp_buf);
				if(temp_buf.NotEmpty() && lmid.FromStr(temp_buf))
					result = lmid;
			}
		}
	}
	else if(flag == smqbpfLocalSession) {
		result = SLS.GetSessUuid();
	}
	return result;
}

/*static*/int PPStyloQInterchange::SetupMqbParam(const StyloQCore::StoragePacket & rOwnPack, long flags, PPStyloQInterchange::RunServerParam & rP)
{
	int    ok = -1;
	const char * p_vhost = "styloq";
	SString temp_buf;
	StyloQConfig cfg_pack;
	SBinaryChunk bc_cfg;
	bool own_cfg_used = false;
	THROW_PP(rOwnPack.Pool.Get(SSecretTagPool::tagSvcIdent, &rP.SvcIdent), PPERR_SQ_UNDEFOWNSVCID);
	THROW_PP(rOwnPack.Pool.Get(SSecretTagPool::tagConfig, &bc_cfg), PPERR_SQ_UNDEFOWNCFG);
	assert(bc_cfg.Len());
	THROW(cfg_pack.FromJson(bc_cfg.ToRawStr(temp_buf)));
	const int locl_accsp_tag_list[] = { StyloQConfig::tagLoclUrl, StyloQConfig::tagLoclMqbAuth, StyloQConfig::tagLoclMqbSecret };
	const int glob_accsp_tag_list[] = { StyloQConfig::tagUrl, StyloQConfig::tagMqbAuth, StyloQConfig::tagMqbSecret };
	const int * p_glob_accsp_tag_list = (flags & (smqbpfLocalMachine|smqbpfLocalSession)) ? locl_accsp_tag_list : glob_accsp_tag_list;
	//PPERR_SQ_UNDEFOWNCFGGLOBACSPNT      "В конфигурации собственного сервиса не определена глобальная точка доступа"
	//PPERR_SQ_UNDEFOWNCFGLOCACSPNT       "В конфигурации собственного сервиса не определена локальная точка доступа"
	const int acspntgetr = cfg_pack.Get(p_glob_accsp_tag_list[0], temp_buf);
	THROW_PP(acspntgetr, (flags & (smqbpfLocalMachine|smqbpfLocalSession)) ? PPERR_SQ_UNDEFOWNCFGLOCACSPNT : PPERR_SQ_UNDEFOWNCFGGLOBACSPNT);
	rP.MqbInitParam.Host = temp_buf.Strip();
	if(cfg_pack.Get(p_glob_accsp_tag_list[1], temp_buf))
		rP.MqbInitParam.Auth = temp_buf;
	if(cfg_pack.Get(p_glob_accsp_tag_list[2], temp_buf))
		rP.MqbInitParam.Secret = temp_buf;
	rP.MqbInitParam.VHost = p_vhost;
	rP.MqbInitParam.Method = 1;
	own_cfg_used = true;
	/* Не будем применять конфигурацию Albatross в случае дефолта!
	if(!own_cfg_used) {
		THROW(PPMqbClient::SetupInitParam(rP.MqbInitParam, p_vhost, 0));
	}*/
	THROW(own_cfg_used);
	//
	THROW_PP(rP.MqbInitParam.Host.NotEmpty(), (flags & (smqbpfLocalMachine|smqbpfLocalSession)) ? PPERR_SQ_UNDEFOWNCFGLOCACSPNT : PPERR_SQ_UNDEFOWNCFGGLOBACSPNT);
	if(flags & (smqbpfLocalMachine|smqbpfLocalSession)) {
		S_GUID locl_uuid = PPStyloQInterchange::GetLocalAddendum(flags & (smqbpfLocalMachine|smqbpfLocalSession));
		THROW(locl_uuid);
		rP.LoclAddendum.Put(&locl_uuid, sizeof(locl_uuid));
	}
	if(flags & smqbpfInitAccessPoint) {
		// @v11.2.12 {
		{
			SString host(rP.MqbInitParam.Host);
			InetUrl url(host);
			const  int prot = url.GetProtocol();
			THROW(oneof3(prot, 0, InetUrl::protAMQP, InetUrl::protAMQPS));
			if(prot == 0)
				url.SetProtocol(InetUrl::protAMQP);
			url.SetPort_(rP.MqbInitParam.Port);
			url.SetComponent(InetUrl::cUserName, rP.MqbInitParam.Auth);
			url.SetComponent(InetUrl::cPassword, rP.MqbInitParam.Secret);
			url.Composite(InetUrl::stAll, rP.AccessPoint);
		}
		// } @v11.2.12 
		/* @v11.2.12 {
			InetUrl url;
			url.SetProtocol(InetUrl::protAMQP);
			url.SetComponent(InetUrl::cHost, rP.MqbInitParam.Host); // @!
			url.SetPort_(rP.MqbInitParam.Port);
			url.SetComponent(InetUrl::cUserName, rP.MqbInitParam.Auth);
			url.SetComponent(InetUrl::cPassword, rP.MqbInitParam.Secret);
			url.Composite(InetUrl::stAll, rP.AccessPoint);
		}*/
	}
	ok = 1;
	CATCHZOK
	return ok;
}

int Test_StyloQInvitation()
{
	class InnerBlock {
	public:
		static int TestRound(PPStyloQInterchange & rIc, int roundNo, const SBinaryChunk & rLoclAddendum)
		{
			int    ok = 1;
			SString temp_buf;
			PPStyloQInterchange::InterchangeParam inv_source;
			PPStyloQInterchange::InterchangeParam inv_result;
			inv_source.SvcIdent.Randomize(20);
			inv_source.LoclAddendum = rLoclAddendum;
			inv_source.AccessPoint = "AMQP://192.168.0.1/test";
			inv_source.Capabilities = 0;
			{
				SJson js(SJson::tOBJECT);
				js.InsertString("cmd", "SomeCommand");
				js.InsertString("arg", "SomeArgument");
				js.ToStr(inv_source.CommandJson);
			}
			rIc.MakeInvitation(inv_source, temp_buf);
			if(!rIc.AcceptInvitation(temp_buf, inv_result))
				ok = 0;
			else if(!inv_result.IsEq(inv_source))
				ok = 0;
			{
				PPBarcode::BarcodeImageParam bip;
				bip.ColorFg = SClrBlack;
				bip.ColorBg = SClrWhite;
				bip.Code = temp_buf;
				bip.Std = BARCSTD_QR;
				//bip.Size.Set(600, 600);
				bip.OutputFormat = SFileFormat::Png;
				PPGetFilePath(PPPATH_OUT, SString("styloq-invitation").CatChar('-').Cat(roundNo).Dot().Cat("png"), bip.OutputFileName);
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
			return ok;
		}
	};
	int    ok = 0;
	SString temp_buf;
	PPStyloQInterchange ic;
	SBinaryChunk la;
	la.Randomize(16);
	if(InnerBlock::TestRound(ic, 1, la))
		ok++;
	la.Z();
	if(InnerBlock::TestRound(ic, 2, la))
		ok++;
	return ok;
}

int PPStyloQInterchange::QueryConfigIfNeeded(RoundTripBlock & rB)
{
	int    ok = -1;
	if(rB.InnerSvcID > 0) {
		SString temp_buf;
		SBinaryChunk face_bytes;
		SBinaryChunk cfg_bytes;
		bool do_query_cfg = true;
		bool do_query_face = true;
		StyloQCore::StoragePacket svc_pack;
		THROW(P_T->GetPeerEntry(rB.InnerSvcID, &svc_pack));
		THROW(svc_pack.Rec.Kind == StyloQCore::kForeignService); // @todo @err
		svc_pack.Pool.Get(SSecretTagPool::tagFace, &face_bytes);
		svc_pack.Pool.Get(SSecretTagPool::tagConfig, &cfg_bytes);
		if(face_bytes.Len()) {
			StyloQFace current_face;
			face_bytes.ToRawStr(temp_buf);
			if(current_face.FromJson(temp_buf) && current_face.Get(StyloQFace::tagExpiryEpochSec, 0, temp_buf) && !IsExpired(temp_buf.ToInt64()))
				do_query_face = false;
		}
		if(cfg_bytes.Len()) {
			StyloQConfig current_cfg;
			cfg_bytes.ToRawStr(temp_buf);
			if(current_cfg.FromJson(temp_buf) && current_cfg.Get(StyloQFace::tagExpiryEpochSec, temp_buf) && !IsExpired(temp_buf.ToInt64()))
				do_query_cfg = false;
		}
		if(do_query_face || do_query_cfg) {
			bool do_update_svc_pack = false;
			SSecretTagPool reply_pool;
			SJson js_query(SJson::tOBJECT);
			js_query.InsertString("cmd", "GetConfig");
			THROW_SL(js_query.ToStr(temp_buf));
			THROW(Command_ClientRequest(rB, temp_buf, 0, reply_pool));
			reply_pool.Get(SSecretTagPool::tagFace,   &face_bytes);
			reply_pool.Get(SSecretTagPool::tagConfig, &cfg_bytes);
			if(face_bytes.Len()) {
				StyloQFace other_face;
				if(other_face.FromJson(face_bytes.ToRawStr(temp_buf))) {
					// Необходимо модифицировать оригинальный face установкой
					// фактического времени истечения срока действия //
					other_face.Get(StyloQFace::tagExpiryPeriodSec, 0, temp_buf);
					const int64 ees = EvaluateExpiryTime(temp_buf.ToLong());
					if(ees > 0)
						other_face.Set(StyloQFace::tagExpiryEpochSec, 0, temp_buf.Z().Cat(ees));
					THROW_SL(other_face.ToJson(false, temp_buf));
					face_bytes.Z().Cat(temp_buf.cptr(), temp_buf.Len());
					svc_pack.Pool.Put(SSecretTagPool::tagFace, face_bytes);
					rB.Other.Put(SSecretTagPool::tagFace, face_bytes);
					do_update_svc_pack = true;
				}
			}
			if(cfg_bytes.Len()) {
				StyloQConfig cfg;
				if(cfg.FromJson(cfg_bytes.ToRawStr(temp_buf))) {
					// Необходимо модифицировать оригинальную кофигурацию установкой
					// фактического времени истечения срока действия //
					cfg.Get(StyloQConfig::tagExpiryPeriodSec, temp_buf);
					const int64 ees = EvaluateExpiryTime(temp_buf.ToLong());
					if(ees > 0)
						cfg.Set(StyloQConfig::tagExpiryEpochSec, temp_buf.Z().Cat(ees));
					THROW_SL(cfg.ToJson(temp_buf));
					cfg_bytes.Z().Cat(temp_buf.cptr(), temp_buf.Len());
					svc_pack.Pool.Put(SSecretTagPool::tagConfig, cfg_bytes); // !
					// @v11.2.12 {
					if(cfg.GetFeatures() & cfg.featrfMediator)
						svc_pack.Rec.Flags |= StyloQCore::styloqfMediator;
					// } @v11.2.12 
					do_update_svc_pack = true;
				}
			}
			if(do_update_svc_pack) {
				long id = rB.InnerSvcID;
				THROW(P_T->PutPeerEntry(&id, &svc_pack, 1));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int StyloQCore::IndexingContent_Json(PPFtsInterface::TransactionHandle * pFtsTra, PPTextAnalyzer * pTa, const char * pJsText)
{
	class InnerBlock {
	public:
		int Tokenize(PPTextAnalyzer * pTa, const SString & rText_, StringSet & rSet)
		{
			int    ok = -1;
			if(pTa && rText_.Len()) {
				TextBuf = rText_;
				if(TextBuf.Unescape().NotEmpty()) {
					uint   idx_first = 0;
					uint   idx_count = 0;
					TextBuf.Utf8ToLower();
					if(TextBuf.C(0) == '\"' && TextBuf.Last() == '\"') {
						TextBuf.TrimRightChr('\"');
						TextBuf.ShiftLeftChr('\"');
						TextBuf.ReplaceStr("\"\"", "\"", 0);
					}
					THROW_SL(pTa->Write(0, 0, TextBuf, TextBuf.Len()+1));
					THROW_SL(pTa->Run(&idx_first, &idx_count));
					for(uint j = 0; j < idx_count; j++) {
						if(pTa->Get(idx_first+j, TItem)) {
							if(TItem.Token == STokenizer::tokWord) {
								rSet.add(TItem.Text);
								ok = 1;
							}
						}
					}
				}
			}
			CATCHZOK
			return ok;
		}
	private:
		STokenizer::Item TItem;
		SString TextBuf;
	};
	int    ok = 1;
	InnerBlock ib_;
	SString temp_buf;
	SString text;
	StringSet ss;
	SString cmd;
	SString svc_ident;
	SString opaque_data; // utf8-текст, хранящийся как приложение к документу xapian для аннотации
	int64  svc_id = 0;
	SJson * p_js_doc = 0;
	THROW_PP(!isempty(pJsText), PPERR_SQ_EMPTYINDEXINGJS);
	THROW_SL(p_js_doc = SJson::Parse(pJsText));
	THROW_PP(p_js_doc->IsObject(), PPERR_SQ_MALFORMEDINDEXINGJS);
	{
		const SJson * p_itm = p_js_doc->FindChildByKey("service");
		if(p_itm) {
			const SJson * p_js_ident = p_itm->FindChildByKey("ident");
			if(p_js_ident) {
				(svc_ident = p_js_ident->Text).Unescape();
			}
		}
		p_itm = p_js_doc->FindChildByKey("cmd");
		if(p_itm)
			cmd = p_itm->Text;
		p_itm = p_js_doc->FindChildByKey("time");
		if(p_itm) {
			;
		}
		p_itm = p_js_doc->FindChildByKey("expir_time_sec");
		if(p_itm) {
			;
		}
	}
	THROW_PP(svc_ident.NotEmpty(), PPERR_SQ_INDEXINGJSHASNTSVCIDENT);
	THROW_PP(cmd.IsEqiAscii("pushindexingcontent"), PPERR_SQ_MALFORMEDINDEXINGJS);
	{
		SBinaryChunk bc_ident;
		StoragePacket svc_pack;
		THROW_PP(bc_ident.FromMime64(svc_ident), PPERR_SQ_MALFORMEDSVCIDENTTEXT);
		{
			int nsr = 0;
			int fsr = 0;
			THROW(fsr = SearchGlobalIdentEntry(kForeignService, bc_ident, &svc_pack));
			if(fsr < 0) {
				THROW(nsr = SearchGlobalIdentEntry(kNativeService, bc_ident, &svc_pack));
			}
			THROW(fsr > 0 || nsr > 0);
			svc_id = svc_pack.Rec.ID;
		}
	}
	for(const SJson * p_cur = p_js_doc->P_Child; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Text.IsEqiAscii("service")) {
			if(p_cur->P_Child && p_cur->P_Child->IsObject()) {
				ss.Z();
				opaque_data.Z();
				for(const SJson * p_inr = p_cur->P_Child->P_Child; p_inr; p_inr = p_inr->P_Next) {
					if(p_inr->Text.IsEqiAscii("nm")) {
						if(p_inr->P_Child) {
							(temp_buf = p_inr->P_Child->Text).Unescape();
							opaque_data = temp_buf;
							ib_.Tokenize(pTa, temp_buf, ss);
						}
					}
					else if(p_inr->Text.IsEqiAscii("descr")) {
						if(p_inr->P_Child) {
							ib_.Tokenize(pTa, (temp_buf = p_inr->P_Child->Text).Unescape(), ss);
						}
					}
					else if(p_inr->Text.IsEqiAscii("gln")) {
						if(p_inr->P_Child)
							ib_.Tokenize(pTa, p_inr->P_Child->Text, ss);
					}
					else if(p_inr->Text.IsEqiAscii("ruinn")) {
						if(p_inr->P_Child)
							ib_.Tokenize(pTa, p_inr->P_Child->Text, ss);
					}
					else if(p_inr->Text.IsEqiAscii("rukpp")) {
						if(p_inr->P_Child)
							ib_.Tokenize(pTa, p_inr->P_Child->Text, ss);
					}
					else if(p_inr->Text.IsEqiAscii("phone")) {
						if(p_inr->P_Child) 
							ib_.Tokenize(pTa, (temp_buf = p_inr->P_Child->Text).Unescape(), ss);
					}
					else if(p_inr->Text.IsEqiAscii("city")) {
						if(p_inr->P_Child)
							ib_.Tokenize(pTa, (temp_buf = p_inr->P_Child->Text).Unescape(), ss);
					}
					else if(p_inr->Text.IsEqiAscii("email")) {
						if(p_inr->P_Child)
							ib_.Tokenize(pTa, (temp_buf = p_inr->P_Child->Text).Unescape(), ss);
					}
				}
				if(pFtsTra && ss.getCount()) {
					PPFtsInterface::Entity entity;
					entity.Scope = PPFtsInterface::scopeStyloQSvc;
					entity.ScopeIdent = svc_ident;
					entity.ObjType = PPOBJ_STYLOQBINDERY;
					entity.ObjId = svc_id;
					uint64 result_doc_id = pFtsTra->PutEntity(entity, ss, opaque_data);
					THROW(result_doc_id);
				}
			}
		}
		else if(p_cur->Text.IsEqiAscii("goods_list")) {
			if(p_cur->P_Child && p_cur->P_Child->IsArray()) {
				for(const SJson * p_inr = p_cur->P_Child->P_Child; p_inr; p_inr = p_inr->P_Next) {
					if(p_inr->IsObject()) {
						ss.Z();
						opaque_data.Z();
						int64 _id = 0;
						for(const SJson * p_itm = p_inr->P_Child; p_itm; p_itm = p_itm->P_Next) {
							if(p_itm->Text.IsEqiAscii("id")) {
								if(p_itm->P_Child)
									_id = p_itm->P_Child->Text.ToInt64();
							}
							else if(p_itm->Text.IsEqiAscii("nm")) {
								if(p_itm->P_Child) {
									(temp_buf = p_itm->P_Child->Text).Unescape();
									opaque_data = temp_buf;
									ib_.Tokenize(pTa, temp_buf, ss);
								}
							}
							else if(p_itm->Text.IsEqiAscii("code_list")) {
								if(p_itm->P_Child && p_itm->P_Child->IsArray()) {
									for(const SJson * p_cod = p_itm->P_Child->P_Child; p_cod; p_cod = p_cod->P_Next) {
										if(p_cod->P_Child && p_cod->P_Child->Text.IsEqiAscii("cod") && p_cod->P_Child->P_Child) {
											ib_.Tokenize(pTa, (temp_buf = p_cod->P_Child->P_Child->Text).Unescape(), ss);
										}
									}
								}
							}
						}
						if(pFtsTra && _id && ss.getCount()) {
							PPFtsInterface::Entity entity;
							entity.Scope = PPFtsInterface::scopeStyloQSvc;
							entity.ScopeIdent = svc_ident;
							entity.ObjType = PPOBJ_GOODS;
							entity.ObjId = _id;
							uint64 result_doc_id = pFtsTra->PutEntity(entity, ss, opaque_data);
							THROW(result_doc_id);
						}
					}
				}
			}
		}
		else if(p_cur->Text.IsEqiAscii("goodsgroup_list")) {
			if(p_cur->P_Child && p_cur->P_Child->IsArray()) {
				for(const SJson * p_inr = p_cur->P_Child->P_Child; p_inr; p_inr = p_inr->P_Next) {
					if(p_inr->IsObject()) {
						ss.Z();
						opaque_data.Z();
						int64 _id = 0;
						for(const SJson * p_itm = p_inr->P_Child; p_itm; p_itm = p_itm->P_Next) {
							if(p_itm->Text.IsEqiAscii("id")) {
								if(p_itm->P_Child)
									_id = p_itm->P_Child->Text.ToInt64();
							}
							else if(p_itm->Text.IsEqiAscii("nm")) {
								if(p_itm->P_Child) {
									(temp_buf = p_itm->P_Child->Text).Unescape();
									opaque_data = temp_buf;
									ib_.Tokenize(pTa, temp_buf, ss);
								}
							}
						}
						if(pFtsTra && _id && ss.getCount()) {
							PPFtsInterface::Entity entity;
							entity.Scope = PPFtsInterface::scopeStyloQSvc;
							entity.ScopeIdent = svc_ident;
							entity.ObjType = PPOBJ_GOODSGROUP;
							entity.ObjId = _id;
							uint64 result_doc_id = pFtsTra->PutEntity(entity, ss, opaque_data);
							THROW(result_doc_id);
						}
					}
				}
			}
		}
		else if(p_cur->Text.IsEqiAscii("brand_list")) {
			if(p_cur->P_Child && p_cur->P_Child->IsArray()) {
				for(const SJson * p_inr = p_cur->P_Child->P_Child; p_inr; p_inr = p_inr->P_Next) {
					if(p_inr->IsObject()) {
						ss.Z();
						opaque_data.Z();
						int64 _id = 0;
						for(const SJson * p_itm = p_inr->P_Child; p_itm; p_itm = p_itm->P_Next) {
							if(p_itm->Text.IsEqiAscii("id")) {
								if(p_itm->P_Child)
									_id = p_itm->P_Child->Text.ToInt64();
							}
							else if(p_itm->Text.IsEqiAscii("nm")) {
								if(p_itm->P_Child) {
									(temp_buf = p_itm->P_Child->Text).Unescape();
									opaque_data = temp_buf;
									ib_.Tokenize(pTa, temp_buf, ss);
								}
							}
						}
						if(pFtsTra && _id && ss.getCount()) {
							PPFtsInterface::Entity entity;
							entity.Scope = PPFtsInterface::scopeStyloQSvc;
							entity.ScopeIdent = svc_ident;
							entity.ObjType = PPOBJ_BRAND;
							entity.ObjId = _id;
							uint64 result_doc_id = pFtsTra->PutEntity(entity, ss, opaque_data);
							THROW(result_doc_id);
						}
					}
				}
			}
		}
		else if(p_cur->Text.IsEqiAscii("processor_list")) {
			if(p_cur->P_Child && p_cur->P_Child->IsArray()) {
				for(const SJson * p_inr = p_cur->P_Child->P_Child; p_inr; p_inr = p_inr->P_Next) {
					if(p_inr->IsObject()) {
						ss.Z();
						opaque_data.Z();
						int64 _id = 0;
						for(const SJson * p_itm = p_inr->P_Child; p_itm; p_itm = p_itm->P_Next) {
							if(p_itm->Text.IsEqiAscii("id")) {
								if(p_itm->P_Child)
									_id = p_itm->P_Child->Text.ToInt64();
							}
							else if(p_itm->Text.IsEqiAscii("nm")) {
								if(p_itm->P_Child) {
									(temp_buf = p_itm->P_Child->Text).Unescape();
									opaque_data = temp_buf;
									ib_.Tokenize(pTa, temp_buf, ss);
								}
							}
						}
						if(pFtsTra && _id && ss.getCount()) {
							PPFtsInterface::Entity entity;
							entity.Scope = PPFtsInterface::scopeStyloQSvc;
							entity.ScopeIdent = svc_ident;
							entity.ObjType = PPOBJ_PROCESSOR;
							entity.ObjId = _id;
							uint64 result_doc_id = pFtsTra->PutEntity(entity, ss, opaque_data);
							THROW(result_doc_id);
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_doc;
	return ok;
}

SJson * PPStyloQInterchange::MakeObjJson_OwnFace(const StyloQCore::StoragePacket & rOwnPack, uint flags)
{
	SJson * p_result = SJson::CreateObj();
	SString temp_buf;
	SBinaryChunk own_svc_ident;
	THROW_SL(p_result);
	THROW(rOwnPack.Rec.Kind == StyloQCore::kNativeService); // @err
	THROW_PP(rOwnPack.Pool.Get(SSecretTagPool::tagSvcIdent, &own_svc_ident), PPERR_SQ_UNDEFOWNSVCID);
	{
		SString my_face_json_buf;
		SBinaryChunk temp_bch;
		p_result->InsertString("ident", own_svc_ident.Mime64(temp_buf).Escape());
		if(rOwnPack.Pool.Get(SSecretTagPool::tagSelfyFace, &temp_bch)) {
			StyloQFace my_face;
			if(my_face.FromJson(temp_bch.ToRawStr(my_face_json_buf))) {
				if(my_face.Get(StyloQFace::tagCommonName, 0, temp_buf))
					p_result->InsertString("nm", temp_buf.Escape());
				if(my_face.Get(StyloQFace::tagDescr, 0, temp_buf))
					p_result->InsertString("descr", temp_buf.Escape());
				if(my_face.Get(StyloQFace::tagGLN, 0, temp_buf))
					p_result->InsertString("gln", temp_buf.Escape());
				if(my_face.Get(StyloQFace::tagRuINN, 0, temp_buf))
					p_result->InsertString("ruinn", temp_buf.Escape());
				if(my_face.Get(StyloQFace::tagRuKPP, 0, temp_buf))
					p_result->InsertString("rukpp", temp_buf.Escape());
				if(my_face.Get(StyloQFace::tagPhone, 0, temp_buf))
					p_result->InsertString("phone", temp_buf.Escape());
				if(my_face.Get(StyloQFace::tagCityName, 0, temp_buf))
					p_result->InsertString("city", temp_buf.Escape());
				if(my_face.Get(StyloQFace::tagEMail, 0, temp_buf))
					p_result->InsertString("email", temp_buf.Escape());
			}
		}
		if(!(flags & mojfForIndexing)) {
			//StyloQBlobInfo blob_info;
			PPObjID oid(PPOBJ_STYLOQBINDERY, rOwnPack.Rec.ID);
			SString blob_signature;
			//if(GetBlobInfo(own_svc_ident, oid, 0, gbifSignatureOnly, blob_info, 0)) {
			if(FetchBlobSignature(own_svc_ident, oid, 0, blob_signature)) {
				//assert(blob_info.Signature.Len() && blob_info.Signature.IsAscii());
				assert(blob_signature.Len() && blob_signature.IsAscii());
				p_result->InsertString("imgblobs", blob_signature);
			}
		}
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

int PPStyloQInterchange::StqInsertIntoJs_BaseCurrency(SJson * pJs) // @v11.3.12
{
	int    ok = -1;
	if(pJs) {
		if(LConfig.BaseCurID > 0) {
			PPObjCurrency cur_obj;
			PPCurrency cur_rec;
			if(cur_obj.Fetch(LConfig.BaseCurID, &cur_rec) > 0 && cur_rec.Symb[0]) {
				SString temp_buf;
				(temp_buf = cur_rec.Symb).Strip();
				if(temp_buf.IsAscii()) {
					temp_buf.ToUpper();
					pJs->InsertString("basecurrency", temp_buf);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

SJson * PPStyloQInterchange::MakeObjJson_Goods(const SBinaryChunk & rOwnIdent, const PPGoodsPacket & rPack, 
	const InnerGoodsEntry * pInnerEntry, uint flags, const StyloQGoodsInfoParam * pGi, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	SJson * p_result = SJson::CreateObj();
	PPObjGoods * p_goods_obj = 0;
	PROFILE_START
	const PPObjID oid(PPOBJ_GOODS, rPack.Rec.ID);
	SString temp_buf;
	THROW_SL(p_result);
	p_result->InsertInt("id", rPack.Rec.ID);
	p_result->InsertString("nm", (temp_buf = rPack.Rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
	Stq_CmdStat_MakeRsrv_Response::RegisterOid(pStat, oid);
	if(!(flags & mojfForIndexing)) {
		//StyloQBlobInfo blob_info;
		SString blob_signature;
		if(rPack.Rec.ParentID) {
			p_result->InsertInt("parid", rPack.Rec.ParentID);
			if(pGi) {
				PPObjGoodsGroup gg_obj;
				Goods2Tbl::Rec gg_rec;
				if(gg_obj.Fetch(rPack.Rec.ParentID, &gg_rec) > 0) {
					(temp_buf = gg_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape();
					p_result->InsertString("parnm", temp_buf);
				}
			}
		}
		if(FetchBlobSignature(rOwnIdent, oid, 0, blob_signature)) {
			//assert(blob_info.Signature.Len() && blob_info.Signature.IsAscii());
			assert(blob_signature.Len() && blob_signature.IsAscii());
			p_result->InsertString("imgblobs", blob_signature);
			Stq_CmdStat_MakeRsrv_Response::RegisterBlobOid(pStat, oid);
		}
		if(rPack.Rec.UnitID) {
			p_result->InsertInt("uomid", rPack.Rec.UnitID);
		}
		if(/*pPack->Rec.Flags & PLMF_EXPBRAND &&*/rPack.Rec.BrandID) {
			p_result->InsertInt("brandid", rPack.Rec.BrandID);
			if(pGi) {
				PPObjBrand brand_obj;
				PPBrand brand_pack;
				if(brand_obj.Fetch(rPack.Rec.BrandID, &brand_pack) > 0) {
					(temp_buf = brand_pack.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape();
					p_result->InsertString("brandnm", temp_buf);
				}
			}
		}
		if(pInnerEntry) {
			if(pGi)
				StqInsertIntoJs_BaseCurrency(p_result);
			// @v11.5.0 {
			if(pInnerEntry->Flags & InnerGoodsEntry::fEgais)
				p_result->InsertBool("egais", true);
			if(pInnerEntry->ChZnMarkCat)
				p_result->InsertInt("chzncat", pInnerEntry->ChZnMarkCat);
			// } @v11.5.0 
			//
			if(pInnerEntry->UnitPerPack > 1.0)
				p_result->InsertDouble("upp", pInnerEntry->UnitPerPack, MKSFMTD(0, 3, NMBF_NOTRAILZ));
			//
			// Минимальный заказ
			//
			if(pInnerEntry->OrderQtyMult > 0.0)
				p_result->InsertDouble("ordqtymult", pInnerEntry->OrderQtyMult, MKSFMTD(0, 3, NMBF_NOTRAILZ));
			if(pInnerEntry->OrderMinQty > 0.0)
				p_result->InsertDouble("ordminqty", pInnerEntry->OrderMinQty, MKSFMTD(0, 3, NMBF_NOTRAILZ));
			//
			p_result->InsertDouble("price", pInnerEntry->Price, MKSFMTD(0, 2, NMBF_NOTRAILZ));
			if(checkdate(pInnerEntry->Expiry)) {
				p_result->InsertString("expirydate", temp_buf.Z().Cat(pInnerEntry->Expiry, DATF_ISO8601CENT).Escape());
			}
			if(pInnerEntry->Rest > 0.0)
				p_result->InsertDouble("stock", pInnerEntry->Rest, MKSFMTD(0, 3, NMBF_NOTRAILZ));
			// @v11.6.4 {
			if(pInnerEntry->Flags & InnerGoodsEntry::fDontShowRest)
				p_result->InsertBool("hidestock", true);
			// } @v11.6.4 
			if(pGi) {
				if(rPack.Rec.ManufID) {
					PPObjPerson psn_obj;
					PersonTbl::Rec psn_rec;
					if(psn_obj.Fetch(rPack.Rec.ManufID, &psn_rec) > 0) {
						p_result->InsertInt("manufid", psn_rec.ID);
						p_result->InsertString("manufnm", (temp_buf = psn_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
					}
				}
				if(checkdate(pInnerEntry->ManufDtm.d)) {
					temp_buf.Z().Cat(pInnerEntry->ManufDtm, DATF_ISO8601CENT, 0).Escape();
					p_result->InsertString("manuftm", temp_buf);
				}
			}
		}
		if(pGi) {
			SString ext_text;
			SString goods_ex_titles;
			bool   goods_ex_titles_inited = false;
			const int ext_text_ids[] = { GDSEXSTR_A, GDSEXSTR_B, GDSEXSTR_C, GDSEXSTR_D, GDSEXSTR_E };
			SJson * p_js_ext_text_array = 0;
			for(uint i = 0; i < SIZEOFARRAY(ext_text_ids); i++) {
				if(rPack.GetExtStrData(ext_text_ids[i], ext_text) > 0) {
					if(!goods_ex_titles_inited) {
						PPObjGoods::ReadGoodsExTitles(rPack.Rec.ParentID, goods_ex_titles);
						goods_ex_titles_inited = true;
					}
					SETIFZQ(p_js_ext_text_array, SJson::CreateArr());
					SJson * p_js_ext_text = SJson::CreateObj();
					if(PPGetExtStrData(ext_text_ids[i], goods_ex_titles, temp_buf) <= 0)
						(temp_buf = "exttext").CatLongZ(i, 2);
					p_js_ext_text->InsertString("title", temp_buf.Transf(CTRANSF_INNER_TO_UTF8).Escape());
					p_js_ext_text->InsertString("text", ext_text.Transf(CTRANSF_INNER_TO_UTF8).Escape());
					p_js_ext_text_array->InsertChild(p_js_ext_text);
				}
			}
			p_result->InsertNz("exttext_list", p_js_ext_text_array);
		}
	}
	if(rPack.Codes.getCount()) {
		SJson * p_js_codelist = SJson::CreateArr();
		for(uint bcidx = 0; bcidx < rPack.Codes.getCount(); bcidx++) {
			const BarcodeTbl::Rec & r_bc = rPack.Codes.at(bcidx);
			SJson * p_js_item = SJson::CreateObj();
			p_js_item->InsertString("cod", (temp_buf = r_bc.Code).Transf(CTRANSF_INNER_TO_UTF8).Escape());
			if(!(flags & mojfForIndexing) && r_bc.Qtty > 0.0) {
				p_js_item->InsertDouble("qty", r_bc.Qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ));
			}
			p_js_codelist->InsertChild(p_js_item);
		}
		p_result->Insert("code_list", p_js_codelist);
	}
	if(pStat)
		pStat->GoodsCount++;
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	PROFILE_END
	delete p_goods_obj;
	return p_result;
}

SJson * PPStyloQInterchange::MakeObjArrayJson_Uom(const SBinaryChunk & rOwnIdent, PPIDArray & rIdList, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	SJson * p_js_list = 0;
	if(rIdList.getCount()) {
		rIdList.sortAndUndup();
		SString temp_buf;
		PPObjUnit u_obj;
		PPUnit2 u_rec;
		for(uint i = 0; i < rIdList.getCount(); i++) {
			if(u_obj.Fetch(rIdList.get(i), &u_rec) > 0) {
				SETIFZ(p_js_list, SJson::CreateArr());
				SJson * p_jsobj = SJson::CreateObj();
				THROW_SL(p_jsobj);
				p_jsobj->InsertInt("id", u_rec.ID);
				if(u_rec.Fragmentation > 0) {
					p_jsobj->InsertInt("fragmentation", u_rec.Fragmentation);
				}
				else if(u_rec.Rounding_ > 0.0) {
					p_jsobj->InsertDouble("rounding", u_rec.Rounding_, MKSFMTD(0, 6, NMBF_NOTRAILZ));
				}
				else if(u_rec.Flags & PPUnit2::IntVal) {
					p_jsobj->InsertBool("int", true);
				}
				p_jsobj->InsertString("nm", (temp_buf = u_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
				p_js_list->InsertChild(p_jsobj);
			}
		}
		// "uom_list"
	}
	CATCH
		ZDELETE(p_js_list);
	ENDCATCH
	return p_js_list;
}

SJson * PPStyloQInterchange::MakeObjJson_GoodsGroup(const SBinaryChunk & rOwnIdent, const Goods2Tbl::Rec & rRec, uint flags, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	SJson * p_result = SJson::CreateObj();
	SString temp_buf;
	THROW_SL(p_result);
	p_result->InsertInt("id", rRec.ID);
	if(!(flags & mojfForIndexing))
		p_result->InsertInt("parid", rRec.ParentID);
	p_result->InsertString("nm", (temp_buf = rRec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
	if(pStat) {
		pStat->GoodsGroupCount++;
		Stq_CmdStat_MakeRsrv_Response::RegisterOid(pStat, PPObjID(PPOBJ_GOODSGROUP, rRec.ID));
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

SJson * PPStyloQInterchange::MakeObjArrayJson_GoodsGroup(const SBinaryChunk & rOwnIdent, PPIDArray & rIdList, bool hierarchical, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	SJson * p_js_list = 0;
	if(rIdList.getCount()) {
		rIdList.sortAndUndup();
		PPObjGoodsGroup gg_obj;
		Goods2Tbl::Rec gg_rec;
		if(hierarchical) {
			StrAssocArray * p_list = gg_obj.Implement_MakeStrAssocList(0, &rIdList);
			if(p_list) {
				p_list->SortByTextInTreeOrder();
				for(uint i = 0; i < p_list->getCount(); i++) {
					StrAssocArray::Item item = p_list->Get(i);
					if(gg_obj.Fetch(item.Id, &gg_rec) > 0) {
						SETIFZ(p_js_list, SJson::CreateArr());
						SJson * p_jsobj = MakeObjJson_GoodsGroup(rOwnIdent, gg_rec, mojfHierarchical, pStat);
						THROW(p_jsobj);
						p_js_list->InsertChild(p_jsobj);
					}
				}
				ZDELETE(p_list);
			}
		}
		else {
			for(uint i = 0; i < rIdList.getCount(); i++) {
				if(gg_obj.Fetch(rIdList.get(i), &gg_rec) > 0) {
					SETIFZ(p_js_list, SJson::CreateArr());
					SJson * p_jsobj = MakeObjJson_GoodsGroup(rOwnIdent, gg_rec, 0, pStat);
					THROW(p_jsobj);
					p_js_list->InsertChild(p_jsobj);
				}
			}
		}
	}
	CATCH
		ZDELETE(p_js_list);
	ENDCATCH
	return p_js_list;
}

SJson * PPStyloQInterchange::MakeObjArrayJson_Brand(const SBinaryChunk & rOwnIdent, PPIDArray & rIdList, uint flags, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	SJson * p_js_list = 0;
	if(rIdList.getCount()) {
		rIdList.sortAndUndup();
		PPObjBrand br_obj;
		PPBrandPacket brand_pack;
		for(uint i = 0; i < rIdList.getCount(); i++) {
			const PPObjID oid(PPOBJ_BRAND, rIdList.get(i));
			brand_pack.Init();
			if(br_obj.Get(oid.Id, &brand_pack) > 0) {
				SETIFZ(p_js_list, SJson::CreateArr());
				SJson * p_jsobj = MakeObjJson_Brand(rOwnIdent, brand_pack, flags, pStat);
				THROW(p_jsobj);
				p_js_list->InsertChild(p_jsobj);
			}
		}
	}
	CATCH
		ZDELETE(p_js_list);
	ENDCATCH
	return p_js_list;
}

SJson * PPStyloQInterchange::MakeObjJson_Brand(const SBinaryChunk & rOwnIdent, const PPBrandPacket & rPack, uint flags, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	SJson * p_result = SJson::CreateObj();
	SString temp_buf;
	const PPObjID oid(PPOBJ_BRAND, rPack.Rec.ID);
	THROW_SL(p_result);
	p_result->InsertInt("id", rPack.Rec.ID);
	p_result->InsertString("nm", (temp_buf = rPack.Rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
	Stq_CmdStat_MakeRsrv_Response::RegisterOid(pStat, oid);
	if(!(flags & mojfForIndexing)) {
		SString blob_signature;
		//StyloQBlobInfo blob_info;
		//if(GetBlobInfo(rOwnIdent, oid, 0, gbifSignatureOnly, blob_info, 0)) {
		if(FetchBlobSignature(rOwnIdent, oid, 0, blob_signature)) {
			//assert(blob_info.Signature.Len() && blob_info.Signature.IsAscii());
			assert(blob_signature.Len() && blob_signature.IsAscii());
			p_result->InsertString("imgblobs", blob_signature);
			Stq_CmdStat_MakeRsrv_Response::RegisterBlobOid(pStat, oid);
		}
	}
	if(pStat)
		pStat->BrandCount++;
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

SJson * PPStyloQInterchange::MakeObjJson_Prc(const SBinaryChunk & rOwnIdent, const ProcessorTbl::Rec & rRec, uint flags, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	SJson * p_result = SJson::CreateObj();
	SString temp_buf;
	THROW_SL(p_result);
	p_result->InsertInt("id", rRec.ID);
	p_result->InsertString("nm", (temp_buf = rRec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
	Stq_CmdStat_MakeRsrv_Response::RegisterOid(pStat, PPObjID(PPOBJ_PROCESSOR, rRec.ID));
	if(pStat)
		pStat->PrcCount++;
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

int PPStyloQInterchange::MakeIndexingRequestCommand(const StyloQCore::StoragePacket * pOwnPack, const StyloQCommandList::Item * pCmd, long expirationSec, 
	S_GUID & rDocUuid, SString & rResult, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	/*
		service { nm; descr; segment_list [ { nm } ] }
		goodsgroup_list [ { id; nm } ]
		brand_list [ { id; nm } ]
		goods_list [ { id; nm; code_list [ { cod } ] }
		processor_list [ { id; nm } ]
		person_list [ { id; nm; ruinn; rukpp; gln; phone_list [ { cod } ]; addr_list [ { zip; city; street } ] ]
	*/
	assert(pOwnPack);
	int    ok = 1;
	StyloQIndexingParam * p_param_ = 0;
	SString temp_buf;
	SBinaryChunk own_svc_ident;
	SJson  js(SJson::tOBJECT);
	if(!rDocUuid)
		rDocUuid.Generate();
	THROW(!pCmd || pCmd->BaseCmdId == StyloQCommandList::sqbcRsrvPushIndexContent); // @todo @err
	THROW(pOwnPack->Rec.Kind == StyloQCore::kNativeService); // @todo @err
	THROW_PP(pOwnPack->Pool.Get(SSecretTagPool::tagSvcIdent, &own_svc_ident), PPERR_SQ_UNDEFOWNSVCID)
	THROW(own_svc_ident.Len() <= sizeof(pOwnPack->Rec.BI) && own_svc_ident.IsEq(pOwnPack->Rec.BI, own_svc_ident.Len())); // @todo @err
	if(pCmd) {
		size_t sav_offs = 0;
		uint   pos = 0;
		{
			const StyloQIndexingParam pattern_filt;
			sav_offs = pCmd->Param.GetRdOffs();
			{
				SBuffer temp_filt_buf(pCmd->Param);
				if(temp_filt_buf.GetAvailableSize()) {
					PPBaseFilt * p_base_filt = 0;
					THROW(PPView::ReadFiltPtr(temp_filt_buf, &p_base_filt));
					if(p_base_filt) {
						if(p_base_filt->GetSignature() == pattern_filt.GetSignature())
							p_param_ = static_cast<StyloQIndexingParam *>(p_base_filt);
						else {
							assert(p_param_ == 0);
							// Путаница в фильтрах - убиваем считанный фильтр чтобы создать новый.
							ZDELETE(p_base_filt);
						}
					}
				}
			}
		}
	}
	//if(p_param && p_param->GoodsGroupID)
	js.InsertString("cmd", "pushindexingcontent");
	js.InsertString("time", temp_buf.Z().Cat(time(0)));
	if(expirationSec > 0) {
		js.Insert("expir_time_sec", json_new_number(temp_buf.Z().Cat(expirationSec)));
	}
	{
		SJson * p_js_svc = MakeObjJson_OwnFace(*pOwnPack, mojfForIndexing);
		THROW(p_js_svc);
		js.Insert("service", p_js_svc);
		Stq_CmdStat_MakeRsrv_Response::RegisterOid(pStat, PPObjID(PPOBJ_STYLOQBINDERY, pOwnPack->Rec.ID));
	}
	js.InsertString("uuid", temp_buf.Z().Cat(rDocUuid)); // @v11.4.5
	{
		PPIDArray goodsgrp_id_list;
		PPIDArray brand_id_list;
		PPObjGoods goods_obj;
		Goods2Tbl::Rec goods_rec;
		if(!p_param_ || p_param_->Flags & StyloQIndexingParam::fGoods) {
			SJson * p_js_goodslist = 0;
			GoodsFilt gf;
			PPGoodsPacket goods_pack;
			gf.Flags |= (GoodsFilt::fHideGeneric|GoodsFilt::fHidePassive);
			if(p_param_ && p_param_->GoodsGroupID)
				gf.GrpIDList.Add(p_param_->GoodsGroupID);
			//StrAssocArray goods_list;
			//GoodsIterator::GetListByFilt(&gf, &goods_list)
			for(GoodsIterator gi(&gf, 0); gi.Next(&goods_rec) > 0;) {
				if(goods_obj.GetPacket(goods_rec.ID, &goods_pack, PPObjGoods::gpoSkipQuot) > 0) {
					goodsgrp_id_list.addnz(goods_rec.ParentID);
					brand_id_list.addnz(goods_rec.BrandID);
					SJson * p_js_ware = MakeObjJson_Goods(own_svc_ident, goods_pack, 0, mojfForIndexing, 0, pStat);
					THROW(p_js_ware);
					SETIFZ(p_js_goodslist, SJson::CreateArr());
					p_js_goodslist->InsertChild(p_js_ware);
				}
			}
			js.InsertNz("goods_list", p_js_goodslist);
		}
		if(!p_param_ || p_param_->Flags & StyloQIndexingParam::fGoodsGroups) {
			js.InsertNz("goodsgroup_list", MakeObjArrayJson_GoodsGroup(own_svc_ident, goodsgrp_id_list, false, pStat));
		}
		if(!p_param_ || p_param_->Flags & StyloQIndexingParam::fBrands) {
			js.InsertNz("brand_list", MakeObjArrayJson_Brand(own_svc_ident, brand_id_list, mojfForIndexing, pStat));
		}
		if(!p_param_ || p_param_->Flags & StyloQIndexingParam::fProcessors) {
			PPObjProcessor prc_obj;
			ProcessorTbl::Rec prc_rec;
			SJson * p_js_prclist = 0;
			for(SEnum en = prc_obj.P_Tbl->Enum(PPPRCK_PROCESSOR, 0); en.Next(&prc_rec) > 0;) {
				bool   skip = false;
				if(p_param_ && p_param_->PrcList.GetCount() && !p_param_->PrcList.CheckID(prc_rec.ID)) {
					skip = true;
				}
				if(!skip) {
					SETIFZ(p_js_prclist, SJson::CreateArr());
					SJson * p_js_item = MakeObjJson_Prc(own_svc_ident, prc_rec, mojfForIndexing, pStat);
					THROW(p_js_item);
					p_js_prclist->InsertChild(p_js_item);
					p_js_item = 0;
				}
			}
			js.InsertNz("processor_list", p_js_prclist);
			p_js_prclist = 0;
		}
	}
	THROW(js.ToStr(rResult));
	// @debug {
	if(false) {
		{
			SString out_file_name;
			PPGetFilePath(PPPATH_OUT, "stq-indexingquery.json", out_file_name);
			SFile f_out(out_file_name, SFile::mWrite);
			f_out.WriteLine(rResult);
		}
		{
			P_T->IndexingContent_Json(0, 0, rResult);
		}
	}
	// } @debug
	CATCHZOK
	ZDELETE(p_param_);
	return ok;
}

int PPStyloQInterchange::QuerySvcConfig(const SBinaryChunk & rSvcIdent, StyloQConfig & rCfg)
{
	rCfg.Z();
	int    ok = -1;
	SJson * p_js_reply = 0;
	TSCollection <StyloQCore::IgnitionServerEntry> isl;
	if(StyloQCore::ReadIgnitionServerList(isl) > 0) {
		SSecretTagPool svc_reply;
		//
		// getforeignconfig
		//
		SString svc_ident_hex;
		rSvcIdent.Mime64(svc_ident_hex);
		for(uint islidx = 0; ok < 0 && islidx < isl.getCount(); islidx++) {
			const StyloQCore::IgnitionServerEntry * p_isl = isl.at(islidx);
			if(p_isl && p_isl->Ident.Len() && p_isl->Url.NotEmpty()) {
				PPStyloQInterchange::InterchangeParam dip;
				dip.SvcIdent = p_isl->Ident;
				dip.AccessPoint = p_isl->Url;
				SJson query(SJson::tOBJECT);
				query.InsertString("cmd", "getforeignconfig");
				query.InsertString("foreignsvcident", svc_ident_hex);
				query.ToStr(dip.CommandJson);
				if(Helper_DoInterchange(dip, svc_reply)) {
					StyloQFace __face;
					StyloQConfig __cfg;
					p_js_reply = svc_reply.GetJson(SSecretTagPool::tagRawData);
					if(p_js_reply && p_js_reply->IsObject()) {
						const SJson * p_js_face = p_js_reply->FindChildByKey("face");
						const SJson * p_js_cfg = p_js_reply->FindChildByKey("config");
						if(p_js_cfg && rCfg.FromJsonObject(p_js_cfg))
							ok = 1;
						/*if(p_js_face) {
							if(!__face.FromJsonObject(p_js_face))
								local_ok = false;
						}*/
					}
				}
			}
		}
	}
	delete p_js_reply;
	return ok;
}

int PPStyloQInterchange::Helper_DoInterchange(InterchangeParam & rParam, SSecretTagPool & rReply)
{
	int    ok = 1;
	THROW_PP(rParam.SvcIdent.Len(), PPERR_SQ_UNDEFSVCID);
	THROW_PP(rParam.AccessPoint.NotEmpty(), PPERR_SQ_UNDEFSVCACCSPOINT);
	{
		PPStyloQInterchange::RoundTripBlock rtb(&rParam.SvcIdent, &rParam.LoclAddendum, rParam.AccessPoint);
		THROW(InitRoundTripBlock(rtb));
		if(rtb.InnerSessID) {
			THROW(Session_ClientRequest(rtb) > 0);
			THROW(QueryConfigIfNeeded(rtb));
		}
		else if(rtb.InnerSvcID) {
			THROW(Verification_ClientRequest(rtb) > 0);
			THROW(QueryConfigIfNeeded(rtb));
		}
		else {
			THROW(KexClientRequest(rtb) > 0);
			THROW(Registration_ClientRequest(rtb));
		}
		if(rParam.CommandJson.NotEmpty()) {
			SString cmd_reply_buf;
			SString svc_result;
			THROW(Command_ClientRequest(rtb, rParam.CommandJson, &rParam.Blob, rReply));
			//int  reply_is_ok = 0;
			/*if(json_parse_document(&p_reply, cmd_reply_buf.cptr()) == JSON_OK) {
				for(const SJson * p_cur = p_reply; p_cur; p_cur = p_cur->P_Next) {
					if(p_cur->Type == SJson::tOBJECT) {								
						for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
							if(p_obj->Text.IsEqiAscii("reply")) { // @debug
								;
							}
						}
					}
				}
			}*/
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::DoInterchange(InterchangeParam & rParam, SSecretTagPool & rReply)
{
	rReply.Z();
	int    ok = 1;
	SString temp_buf;
	THROW_PP(rParam.SvcIdent.Len() > 0, PPERR_SQ_UNDEFSVCID);
	if(rParam.AccessPoint.IsEmpty()) {
		//
		// Если в параметрах не задан адрес сервера (param.AccsPoint),
		// то действуем по следующему плану:
		// 1. Если у нас уже есть запись сервиса, то извлекаем из нее точку доступа.
		// 2. Если на предыдущем шаге нас ждало разочарование, то обращаемся к медиаторам за информацией о конфигурации сервиса.
		//
		StyloQCore::StoragePacket svc_pack;
		StyloQConfig svc_cfg;
		if(QuerySvcConfig(rParam.SvcIdent, svc_cfg) > 0 && svc_cfg.Get(StyloQConfig::tagUrl, temp_buf)) {
			rParam.AccessPoint = temp_buf;
			svc_cfg.Get(StyloQConfig::tagMqbAuth, rParam.MqbAuth);
			svc_cfg.Get(StyloQConfig::tagMqbSecret, rParam.MqbSecret);
		}
		if(rParam.AccessPoint.IsEmpty()) {
			SBinaryChunk raw_svc_cfg;
			StyloQConfig svc_cfg;
			THROW(P_T->SearchGlobalIdentEntry(StyloQCore::kForeignService, rParam.SvcIdent, &svc_pack) > 0);
			THROW_PP_S(svc_pack.Pool.Get(SSecretTagPool::tagConfig, &raw_svc_cfg), PPERR_SQ_UNDEFSVCCFG, rParam.SvcIdent.Mime64(temp_buf));
			THROW(svc_cfg.FromJson(raw_svc_cfg.ToRawStr(temp_buf)) && svc_cfg.Get(StyloQConfig::tagUrl, temp_buf));
			rParam.AccessPoint = temp_buf;
			//rtb.Other.Put(SSecretTagPool::tagSvcAccessPoint, temp_buf.cptr(), temp_buf.Len());
			svc_cfg.Get(StyloQConfig::tagMqbAuth, rParam.MqbAuth);
			svc_cfg.Get(StyloQConfig::tagMqbSecret, rParam.MqbSecret);
		}
	}
	THROW(Helper_DoInterchange(rParam, rReply));
	CATCHZOK
	return ok;
}

//long CreateOnetimePass(PPID userID); // @v11.1.9
long OnetimePass(PPID userID); // @v11.1.9

/*static*/void PPStyloQInterchange::Stq_CmdStat_MakeRsrv_Response::RegisterOid(Stq_CmdStat_MakeRsrv_Response * pThis, PPObjID oid)
{
	if(pThis && oid.Obj && oid.Id)
		pThis->OidList.Add(oid.Obj, oid.Id);
}

/*static*/void PPStyloQInterchange::Stq_CmdStat_MakeRsrv_Response::RegisterBlobOid(Stq_CmdStat_MakeRsrv_Response * pThis, PPObjID oid)
{
	if(pThis && oid.Obj && oid.Id)
		pThis->BlobOidList.Add(oid.Obj, oid.Id);
}

PPStyloQInterchange::Stq_CmdStat_MakeRsrv_Response::Stq_CmdStat_MakeRsrv_Response() : 
	GoodsCount(0), GoodsGroupCount(0), BrandCount(0), ClientCount(0), DlvrLocCount(0), PrcCount(0)
{
}

PPStyloQInterchange::InnerGoodsEntry::InnerGoodsEntry(PPID goodsID) : GoodsID(goodsID), 
	Flags(0), Expiry(ZERODATE),  ManufDtm(ZERODATETIME), QuotKindUsedForPrice(0), QuotKindUsedForExtPrice(0),
	Rest(0.0), Cost(0.0), Price(0.0), ExtPrice(0.0), UnitPerPack(0.0), OrderQtyMult(0.0), OrderMinQty(0), ChZnMarkCat(GTCHZNPT_UNDEF)
{
}

PPStyloQInterchange::InnerGoodsEntry & PPStyloQInterchange::InnerGoodsEntry::Z()
{
	THISZERO();
	return *this;
}

int PPStyloQInterchange::MakeRsrvPriceListResponse_ExportClients(const SBinaryChunk & rOwnIdent, const StyloQDocumentPrereqParam & rParam, SJson * pJs, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	int    ok = 1;
	SString temp_buf;
	PPObjBill * p_bobj = BillObj;
	const  bool use_omt_paym_amt = LOGIC(CConfig.Flags2 & CCFLG2_USEOMTPAYMAMT);
	DebtTrnovrViewItem debt_item;
	PPViewDebtTrnovr * p_debt_view = 0; // @stub
	PPStyloPalmPacket palm_pack;
	if(rParam.PalmID) {
		PPObjStyloPalm sp_obj;
		THROW(sp_obj.GetPacket(rParam.PalmID, &palm_pack));
	}
	else {
		assert(palm_pack.Rec.ID == 0);
	}
	/*if(palmFlags & PLMF_EXPCLIDEBT && !(palmFlags & PLMF_BLOCKED)) {
		DebtTrnovrFilt debt_filt;
		debt_filt.AccSheetID = acsID;
		debt_filt.Flags |= DebtTrnovrFilt::fDebtOnly;
		THROW_MEM(SETIFZ(rBlk.P_DebtView, new PPViewDebtTrnovr));
		THROW(rBlk.P_DebtView->Init_(&debt_filt));
	}*/
	if(!rParam.PalmID || !(palm_pack.Rec.Flags & PLMF_BLOCKED)) {
		SJson * p_js_client_list = 0;
		SString wait_msg;
		SString addr;
		SString inn_buf;
		SString kpp_buf;
		PPIDArray dlvr_loc_list;
		PPObjLocation loc_obj;
		PPObjPerson psn_obj;
		PPObjArticle ar_obj;
		PPViewArticle ar_view;
		ArticleFilt ar_filt;
		ArticleViewItem ar_item;
		PPObjAccSheet acc_sheet_obj;
		PPAccSheet acs_rec;
		PPOprKind op_rec;
		//StyloQBlobInfo blob_info;
		SString blob_signature;
		PPID   op_id = 0;
		if(rParam.PalmID) {
			op_id = palm_pack.Rec.OrderOpID;
			if(!op_id) {
				PPAlbatrossConfig acfg;
				DS.FetchAlbatrosConfig(&acfg);
				op_id = acfg.Hdr.OpID;
			}
		}
		else {
			op_id = rParam.OpID;
		}
		PPID   acs_id = (GetOpData(op_id, &op_rec) > 0) ? op_rec.AccSheetID : 0;
		SETIFZ(acs_id, GetSellAccSheet());
		ar_filt.AccSheetID = acs_id;
		THROW(acc_sheet_obj.Fetch(acs_id, &acs_rec) > 0);
		PPLoadText(PPTXT_WAIT_PALMEXPCLI, wait_msg);
		THROW(ar_view.Init_(&ar_filt));
		for(ar_view.InitIteration(); ar_view.NextIteration(&ar_item) > 0;) {
			PPWaitPercent(ar_view.GetCounter(), wait_msg);
			if(!ar_item.Closed) { // Не будем выгружать пассивные статьи
				inn_buf.Z();
				long   _flags = 0;
				long   _def_due_period_hr = 0;
				PPID   quot_kind_id = 0;
				PPClientAgreement cli_agt;
				if(ar_obj.GetClientAgreement(ar_item.ID, cli_agt, 0) > 0) {
					quot_kind_id = cli_agt.DefQuotKindID;
					if(cli_agt.Flags & AGTF_DONTUSEMINSHIPMQTTY)
						_flags |= CLIENTF_DONTUSEMINSHIPMQTTY;
					// @v11.4.8 {
					if(cli_agt.DefDuePeriodHour > 0 && cli_agt.DefDuePeriodHour <= (180*24))
						_def_due_period_hr = cli_agt.DefDuePeriodHour;
					// } @v11.4.8 
				}
				if(acs_rec.Assoc == PPOBJ_PERSON) {
					psn_obj.GetRegNumber(ar_item.ObjID, PPREGT_TPID, inn_buf);
					psn_obj.GetRegNumber(ar_item.ObjID, PPREGT_KPP, kpp_buf);
				}
				if(rParam.PalmID && (palm_pack.Rec.Flags & PLMF_EXPSTOPFLAG) && (ar_item.Flags & ARTRF_STOPBILL)) {
					_flags |= CLIENTF_BLOCKED;
				}
				const bool valid_debt = (p_debt_view && p_debt_view->GetItem(ar_item.ID, 0L, 0L, &debt_item) > 0);
				//
				//if(GetBlobInfo(const SBinaryChunk & rOwnIdent, PPObjID oid, uint blobN, BlobInfo & rInfo) const;
				//
				SJson * p_js_obj = SJson::CreateObj();
				p_js_obj->InsertInt("id", ar_item.ID);
				p_js_obj->InsertString("nm", (temp_buf = ar_item.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
				if(inn_buf.NotEmpty()) {
					p_js_obj->InsertString("ruinn", inn_buf.Transf(CTRANSF_INNER_TO_UTF8).Escape());
					if(kpp_buf.NotEmpty())
						p_js_obj->InsertString("rukpp", kpp_buf.Transf(CTRANSF_INNER_TO_UTF8).Escape());
				}
				if(quot_kind_id) {
					p_js_obj->InsertInt("quotkind", quot_kind_id); // @v11.7.1 @fix "quotkid"-->"quotkind"
				}
				if(valid_debt) {
					p_js_obj->InsertDouble("debt", debt_item.Debt, MKSFMTD(0, 2, NMBF_NOTRAILZ));
				}
				// @v11.4.8 {
				if(_def_due_period_hr > 0)
					p_js_obj->InsertInt("dueperiodhr", _def_due_period_hr);
				// } @v11.4.8 
				if(_flags & CLIENTF_DONTUSEMINSHIPMQTTY)
					p_js_obj->InsertBool("dontuseminshipmqty", true);
				if(_flags & CLIENTF_BLOCKED)
					p_js_obj->InsertBool("blocked", true);
				if(acs_rec.Assoc == PPOBJ_PERSON) {
					{
						const PPObjID oid(PPOBJ_PERSON, ar_item.ObjID);
						Stq_CmdStat_MakeRsrv_Response::RegisterOid(pStat, oid);
						if(FetchBlobSignature(rOwnIdent, oid, 0, blob_signature)) {
							assert(blob_signature.Len() && blob_signature.IsAscii());
							p_js_obj->InsertString("imgblobs", blob_signature);
							Stq_CmdStat_MakeRsrv_Response::RegisterBlobOid(pStat, oid);
						}
					}
					SJson * p_js_dlvrloc_list = 0;
					dlvr_loc_list.clear();
					LocationTbl::Rec loc_rec;
					THROW(psn_obj.GetDlvrLocList(ar_item.ObjID, &dlvr_loc_list));
					for(uint i = 0; i < dlvr_loc_list.getCount(); i++) {
						const PPID dlvr_loc_id = dlvr_loc_list.at(i);
						if(loc_obj.Fetch(dlvr_loc_id, &loc_rec) > 0 && LocationCore::GetAddress(loc_rec, 0, addr) && addr.NotEmptyS()) {
							SJson * p_js_adr = SJson::CreateObj();
							p_js_adr->InsertInt("id", dlvr_loc_id);
							p_js_adr->InsertString("addr", (temp_buf = addr).Transf(CTRANSF_INNER_TO_UTF8).Escape());
							// @v11.6.1 {
							{
								SGeoPosLL geopos(loc_rec.Latitude, loc_rec.Longitude);
								if(geopos.IsValid() && !geopos.IsZero()) {
									p_js_adr->InsertDouble("lat", geopos.Lat, MKSFMTD(0, 12, NMBF_NOTRAILZ));
									p_js_adr->InsertDouble("lon", geopos.Lon, MKSFMTD(0, 12, NMBF_NOTRAILZ));
								}
							}
							// } @v11.6.1 
							SETIFZ(p_js_dlvrloc_list, SJson::CreateArr());
							p_js_dlvrloc_list->InsertChild(p_js_adr);
							if(pStat)
								pStat->DlvrLocCount++;
						}
					}
					p_js_obj->InsertNz("dlvrloc_list", p_js_dlvrloc_list);
				}
				if(p_debt_view) {
					SJson * p_js_debtlist = 0;
					PayableBillList pb_list;
					PayableBillListItem * p_pb_item;
					p_debt_view->GetPayableBillList(ar_item.ID, 0L, &pb_list);
					for(uint i = 0; pb_list.enumItems(&i, (void **)&p_pb_item);) {
						BillTbl::Rec bill_rec;
						if(p_bobj->Search(p_pb_item->ID, &bill_rec) > 0) {
							const double amt = BR2(bill_rec.Amount);
							double paym = 0.0;
							//
							// Извлечение примечания к долговому документу
							//
							/*
							{
								uint pos = 0;
								SString memos;
								StringSet ss(MemosDelim);
								BillObj->FetchExtMemo(p_pb_item->ID, memos);
								ss.setBuf(memos, prev_memos.Len());
								if(ss.search(PalmMemo, &pos, 0) > 0)
									ss.get(&pos, memo);
							}
							*/
							if(use_omt_paym_amt)
								paym = bill_rec.PaymAmount;
							else
								p_bobj->P_Tbl->CalcPayment(p_pb_item->ID, 0, 0, p_pb_item->CurID, &paym);
							const double debt = R2(amt - paym);
							if(debt > 0.0) {
								PPBillExt bill_ext;
								PPFreight freight;
								p_bobj->FetchExt(p_pb_item->ID, &bill_ext);
								SJson * p_js_debt_entry = SJson::CreateObj();
								p_js_debt_entry->InsertInt("id", p_pb_item->ID);
								p_js_debt_entry->InsertString("cod", (temp_buf = bill_rec.Code).Transf(CTRANSF_INNER_TO_UTF8).Escape());
								temp_buf.Z().Cat(bill_rec.Dt, DATF_ISO8601CENT);
								p_js_debt_entry->InsertString("dat", temp_buf);
								p_js_debt_entry->InsertDouble("amt", amt, MKSFMTD(0, 2, NMBF_NOTRAILZ));
								p_js_debt_entry->InsertDouble("debt", debt, MKSFMTD(0, 2, NMBF_NOTRAILZ));
								p_js_debt_entry->InsertInt("agentid", bill_ext.AgentID);
								SETIFZ(p_js_debtlist, SJson::CreateArr());
								p_js_debtlist->InsertChild(p_js_debt_entry);
							}
						}
					}
					p_js_obj->InsertNz("debt_list", p_js_debtlist);
				}
				SETIFZ(p_js_client_list, SJson::CreateArr());
				p_js_client_list->InsertChild(p_js_obj);
				if(pStat)
					pStat->ClientCount++;
			}
		}
		pJs->InsertNz("client_list", p_js_client_list);
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::MakeRsrvIndoorSvcPrereqResponse_ExportGoods(const SBinaryChunk & rOwnIdent, const PPSyncCashNode * pPack, 
	const PPIDArray * pAdditionalIdList, uint flags, SJson * pJs, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	/*
		quotkind_list [ { id; nm } ]
		goodsgroup_list [ { id; parid; nm } ]
		warehouse_list [ { id; nm } ]
		uom_list [ { id; nm; (fragmentation|rounding|int) } ]
		brand_list [ { id; nm } ]
		goods_list [ { id; nm; parid; uomid; code_list [ { cod; qty } ]; brandid; upp; price; stock; (ordqtymult|ordminqty); quot_list [ { id; val } ] ]
	*/
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	PPIDArray  grp_id_list;
	PPIDArray  brand_id_list;
	PPIDArray  unit_id_list;
	PPObjQuotKind qk_obj;
	SVector goods_list(sizeof(InnerGoodsEntry));
	PPIDArray temp_loc_list;
	StyloQBlobInfo blob_info;
	if(!(pPack->Flags & CASHFX_PASSIVE)) {
		CPosProcessor cpp(pPack->ID, 0, 0, CPosProcessor::ctrfForceInitGroupList, /*pDummy*/0);
		PPObjGoods goods_obj;
		const PPID single_loc_id = pPack->LocID;
		{
			PPIDArray local_goods_id_list;
			cpp.Backend_GetGoodsList(local_goods_id_list);
			if(local_goods_id_list.getCount()) {
				PPObjGoods goods_obj;
				Goods2Tbl::Rec goods_rec;
				long   ext_rgi_flags = 0;
				SJson * p_js_goods = SJson::CreateArr();
				for(uint i = 0; i < local_goods_id_list.getCount(); i++) {
					const PPID goods_id = local_goods_id_list.get(i);
					if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
						RetailGoodsInfo rgi;
						if(cpp.GetRgi(goods_id, 0.0, ext_rgi_flags, rgi)) {
							if(!(rgi.Flags & RetailGoodsInfo::fDisabledQuot) && (rgi.Price > 0.0)) {
								grp_id_list.addnz(goods_rec.ParentID);
								brand_id_list.addnz(goods_rec.BrandID);
								unit_id_list.addnz(goods_rec.UnitID);

								InnerGoodsEntry goods_entry(goods_id);
								goods_entry.Cost = rgi.Cost;
								goods_entry.Price = rgi.Price;
								goods_entry.ExtPrice = rgi.ExtPrice;
								goods_entry.Expiry = rgi.Expiry;
								goods_entry.ManufDtm = rgi.ManufDtm;
								goods_entry.Flags = rgi.Flags;
								goods_entry.QuotKindUsedForPrice = rgi.QuotKindUsedForPrice;
								goods_entry.QuotKindUsedForExtPrice = rgi.QuotKindUsedForExtPrice;
								goods_entry.ChZnMarkCat = rgi.ChZnMarkCat; // @v11.5.0
								if(false) {
									GoodsRestParam rp;
									rp.CalcMethod = GoodsRestParam::pcmMostRecent;
									rp.GoodsID = goods_id;
									rp.Date = ZERODATE;
									rp.LocID = cpp.GetCnLocID(goods_id);
									p_bobj->trfr->GetRest(rp);
									goods_entry.Rest = rp.Total.Rest;
								}
								goods_list.insert(&goods_entry);
							}
						}
					}
				}
			}
		}
		// (done by MakeObjArrayJson_GoodsGroup) grp_id_list.sortAndUndup();
		// (done by MakeObjArrayJson_Brand) brand_id_list.sortAndUndup();
		// (done by MakeObjArrayJson_Uom) unit_id_list.sortAndUndup();
		pJs->InsertNz("goodsgroup_list", MakeObjArrayJson_GoodsGroup(rOwnIdent, grp_id_list, false, pStat));
		pJs->InsertNz("uom_list", MakeObjArrayJson_Uom(rOwnIdent, unit_id_list, pStat));
		pJs->InsertNz("brand_list", MakeObjArrayJson_Brand(rOwnIdent, brand_id_list, 0, pStat));
		{
			SJson * p_goods_list = 0;
			PPGoodsPacket goods_pack;
			PROFILE_START
			for(uint glidx = 0; glidx < goods_list.getCount(); glidx++) {
				const InnerGoodsEntry & r_goods_entry = *static_cast<const InnerGoodsEntry *>(goods_list.at(glidx));
				const PPObjID oid(PPOBJ_GOODS, r_goods_entry.GoodsID);
				if(goods_obj.GetPacket(r_goods_entry.GoodsID, &goods_pack, PPObjGoods::gpoSkipQuot) > 0) {
					SJson * p_jsobj = MakeObjJson_Goods(rOwnIdent, goods_pack, &r_goods_entry, 0, 0, pStat);
					THROW(p_jsobj);
					SETIFZ(p_goods_list, SJson::CreateArr());
					p_goods_list->InsertChild(p_jsobj);
				}
			}
			PROFILE_END
			pJs->InsertNz("goods_list", p_goods_list);
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
class StyloQDialogWithPrcList : public TDialog {
protected:
	StyloQDialogWithPrcList(uint dlgId, uint ctlPrcList) : TDialog(dlgId), CtlPrcList(ctlPrcList), P_Data(0)
	{
		P_Box = static_cast<SmartListBox *>(getCtrlView(ctlPrcList));
		SetupStrListBox(P_Box);
	}
	void SetDataRef(ObjIdListFilt * pPrcList)
	{
		P_Data = pPrcList;
		SetupList();
	}
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmSelectProcessors)) {
			if(P_Data) {
				PPIDArray prc_list;
				P_Data->Get(prc_list);
				ListToListData data(PPOBJ_PROCESSOR, 0, &prc_list);
				data.Flags |= ListToListData::fIsTreeList;
				data.TitleStrID = PPTXT_SELPRCLIST;
				if(ListToListDialog(&data) > 0) {
					if(prc_list.getCount())
						P_Data->Set(&prc_list);
					else
						P_Data->Set(0);
					SetupList();
				}
			}
			clearEvent(event);
		}
	}
	void SetupList()
	{
		if(P_Box && P_Data) { 
			const long sav_pos = P_Box->P_Def ? P_Box->P_Def->_curItem() : 0;
			SString temp_buf;
			ProcessorTbl::Rec prc_rec;
			P_Box->freeAll();
			for(uint i = 0; i < P_Data->GetCount(); i++) {
				const PPID prc_id = P_Data->Get(i);
				if(PrcObj.Fetch(prc_id, &prc_rec) > 0)
					P_Box->addItem(prc_id, prc_rec.Name);
			}
			P_Box->Draw_();
			P_Box->focusItem(sav_pos);
		}
	}
	const uint CtlPrcList;
	ObjIdListFilt * P_Data;
	SmartListBox * P_Box;
	PPObjProcessor PrcObj;
};

IMPLEMENT_PPFILT_FACTORY_CLS(StyloQGoodsInfoParam);

StyloQGoodsInfoParam::StyloQGoodsInfoParam() : PPBaseFilt(PPFILT_STYLOQGOODSINFOPARAM, 0, 0)
{
	InitInstance();
}
	
StyloQGoodsInfoParam::StyloQGoodsInfoParam(const StyloQGoodsInfoParam & rS) : PPBaseFilt(PPFILT_STYLOQGOODSINFOPARAM, 0, 0)
{
	InitInstance();
	Copy(&rS, 1);
}
	
StyloQGoodsInfoParam & FASTCALL StyloQGoodsInfoParam::operator = (const StyloQGoodsInfoParam & rS)
{
	Copy(&rS, 1);
	return *this;
}

int StyloQGoodsInfoParam::InitInstance()
{
	SetFlatChunk(offsetof(StyloQGoodsInfoParam, ReserveStart), offsetof(StyloQGoodsInfoParam, Reserve)-offsetof(StyloQGoodsInfoParam, ReserveStart));
	return Init(1, 0);
}

/*static*/int PPStyloQInterchange::Edit_GoodsInfoParam(StyloQGoodsInfoParam & rParam)
{
	class GoodsInfoParamDialog : public TDialog {
		DECL_DIALOG_DATA(StyloQGoodsInfoParam);
	public:
		GoodsInfoParamDialog() : TDialog(DLG_STQGINFPARAM)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			AddClusterAssoc(CTL_STQGINFPARAM_FLAGS, 0, StyloQGoodsInfoParam::fShowRest);
			AddClusterAssoc(CTL_STQGINFPARAM_FLAGS, 1, StyloQGoodsInfoParam::fShowCost);
			AddClusterAssoc(CTL_STQGINFPARAM_FLAGS, 2, StyloQGoodsInfoParam::fShowPrice);
			AddClusterAssoc(CTL_STQGINFPARAM_FLAGS, 3, StyloQGoodsInfoParam::fShowDescriptions);
			AddClusterAssoc(CTL_STQGINFPARAM_FLAGS, 4, StyloQGoodsInfoParam::fShowCodes);
			SetClusterData(CTL_STQGINFPARAM_FLAGS, Data.Flags);
			{
				PPObjCashNode::SelFilt f;
				f.Flags |= PPObjCashNode::SelFilt::fSkipPassive;
				f.OnlyGroups = -1;
				f.SyncGroup = 1;
				SetupPPObjCombo(this, CTLSEL_STQGINFPARAM_POSN, PPOBJ_CASHNODE, Data.PosNodeID, 0, &f);
			}
			SetupPPObjCombo(this, CTLSEL_STQGINFPARAM_LOC, PPOBJ_LOCATION, Data.LocID, 0);
			SetupCtrls();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			GetClusterData(CTL_STQGINFPARAM_FLAGS, &Data.Flags);
			getCtrlData(CTLSEL_STQGINFPARAM_POSN, &Data.PosNodeID);
			getCtrlData(CTLSEL_STQGINFPARAM_LOC, &Data.LocID);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_STQGINFPARAM_POSN)) {
				SetupCtrls();
			}
			else
				return;
		}
		void   SetupCtrls()
		{
			getCtrlData(CTLSEL_STQGINFPARAM_POSN, &Data.PosNodeID);
			PPObjCashNode cn_obj;
			bool    disable_ctrl_loc = false;
			if(Data.PosNodeID) {
				PPSyncCashNode scn;
				if(cn_obj.GetSync(Data.PosNodeID, &scn) > 0) {
					if(scn.LocID) {
						Data.LocID = scn.LocID;
						setCtrlLong(CTLSEL_STQGINFPARAM_LOC, Data.LocID);
						disable_ctrl_loc = true;
					}
				}
			}
			disableCtrl(CTLSEL_STQGINFPARAM_LOC, disable_ctrl_loc);
		}
	};
	DIALOG_PROC_BODY(GoodsInfoParamDialog, &rParam);
}

IMPLEMENT_PPFILT_FACTORY_CLS(StyloQIndexingParam);

StyloQIndexingParam::StyloQIndexingParam() : PPBaseFilt(PPFILT_STYLOQINDEXINGPARAM, 0, 0)
{
	InitInstance();
}

StyloQIndexingParam::StyloQIndexingParam(const StyloQIndexingParam & rS) : PPBaseFilt(PPFILT_STYLOQINDEXINGPARAM, 0, 0)
{
	InitInstance();
	Copy(&rS, 1);
}

StyloQIndexingParam & FASTCALL StyloQIndexingParam::operator = (const StyloQIndexingParam & rS)
{
	Copy(&rS, 1);
	return *this;
}

int StyloQIndexingParam::InitInstance()
{
	SetFlatChunk(offsetof(StyloQIndexingParam, ReserveStart), offsetof(StyloQIndexingParam, PrcList)-offsetof(StyloQIndexingParam, ReserveStart));
	SetBranchObjIdListFilt(offsetof(StyloQIndexingParam, PrcList));
	return Init(1, 0);
}

/*static*/int PPStyloQInterchange::Edit_IndexingParam(StyloQIndexingParam & rParam)
{
	class IndexingParamDialog : public StyloQDialogWithPrcList {
		DECL_DIALOG_DATA(StyloQIndexingParam);
	public:
		IndexingParamDialog() : StyloQDialogWithPrcList(DLG_STQIDXCPARAM, CTL_STQIDXCPARAM_PRCL)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			//
			AddClusterAssoc(CTL_STQIDXCPARAM_FLAGS, 0, StyloQIndexingParam::fGoods);
			AddClusterAssoc(CTL_STQIDXCPARAM_FLAGS, 1, StyloQIndexingParam::fGoodsGroups);
			AddClusterAssoc(CTL_STQIDXCPARAM_FLAGS, 2, StyloQIndexingParam::fBrands);
			AddClusterAssoc(CTL_STQIDXCPARAM_FLAGS, 3, StyloQIndexingParam::fProcessors);
			SetClusterData(CTL_STQIDXCPARAM_FLAGS, Data.Flags);
			SetupPPObjCombo(this, CTLSEL_STQIDXCPARAM_GGRP, PPOBJ_GOODSGROUP, Data.GoodsGroupID, OLW_CANSELUPLEVEL);
			SetupPPObjCombo(this, CTLSEL_STQIDXCPARAM_PSNK, PPOBJ_PERSONKIND, Data.PersonKindID, 0);
			SetDataRef(&Data.PrcList);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			GetClusterData(CTL_STQIDXCPARAM_FLAGS, &Data.Flags);
			getCtrlData(CTLSEL_STQIDXCPARAM_GGRP, &Data.GoodsGroupID);
			getCtrlData(CTLSEL_STQIDXCPARAM_PSNK, &Data.PersonKindID);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	};
	DIALOG_PROC_BODY(IndexingParamDialog, &rParam);
}
//
//
//
StyloQPersonEventParam::StyloQPersonEventParam() : Signature(PPConst::Signature_StyloQPersonEventParam), Flags(0), MaxGeoDistance(0.0)
{
	MEMSZERO(Fb);
}

StyloQPersonEventParam::StyloQPersonEventParam(const StyloQPersonEventParam & rS) : Signature(rS.Signature), Flags(rS.Flags), 
	MaxGeoDistance(rS.MaxGeoDistance), PePack(rS.PePack)
{
	assert(Signature == PPConst::Signature_StyloQPersonEventParam);
	memcpy(&Fb, &rS.Fb, sizeof(Fb));
}
	
StyloQPersonEventParam & FASTCALL StyloQPersonEventParam::operator = (const StyloQPersonEventParam & rS)
{
	assert(Signature == PPConst::Signature_StyloQPersonEventParam);
	assert(rS.Signature == PPConst::Signature_StyloQPersonEventParam);
	Flags = rS.Flags;
	MaxGeoDistance = rS.MaxGeoDistance;
	PePack = rS.PePack;
	memcpy(&Fb, &rS.Fb, sizeof(Fb));
	return *this;
}

StyloQPersonEventParam & StyloQPersonEventParam::Z()
{
	assert(Signature == PPConst::Signature_StyloQPersonEventParam);
	Flags = 0;
	MaxGeoDistance = 0.0;
	PePack.Destroy();
	MEMSZERO(Fb);
	return *this;
}
	
int StyloQPersonEventParam::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	bool   done = false;
	const size_t preserve_offs = rBuf.GetRdOffs();
	assert(Signature == PPConst::Signature_StyloQPersonEventParam);
	PPObjPersonEvent pe_obj;
	PPObject::SerializeSignature pre_1162_signature(pe_obj.Obj);
	uint32 _signature = Signature;
	/*if(dir < 0 && pre_1162_signature.Read(rBuf)) { // Версия параметра до v11.6.2
		rBuf.SetRdOffs(preserve_offs);
		Z();
		THROW(pe_obj.SerializePacket(dir, &PePack, rBuf, pSCtx));
	}*/
	if(dir < 0) {
		if(pe_obj.SerializePacket(dir, &PePack, rBuf, pSCtx)) {
			Flags = 0;
			MaxGeoDistance = 0.0;
			MEMSZERO(Fb);
			done = true;
		}
		else
			rBuf.SetRdOffs(preserve_offs);
	}
	if(!done) {
		THROW(pSCtx->Serialize(dir, _signature, rBuf));
		THROW(dir > 0 || _signature == PPConst::Signature_StyloQPersonEventParam); // @todo @err
		THROW(pSCtx->Serialize(dir, Flags, rBuf));
		THROW(pSCtx->Serialize(dir, MaxGeoDistance, rBuf));
		THROW(pe_obj.SerializePacket(dir, &PePack, rBuf, pSCtx));
		THROW(pSCtx->SerializeBlock(dir, sizeof(Fb), &Fb, rBuf, 1));
	}
	CATCHZOK
	return ok;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY_CLS(StyloQDocumentPrereqParam);

StyloQDocumentPrereqParam::StyloQDocumentPrereqParam() : PPBaseFilt(PPFILT_STYLOQDOCUMENTPREREQPARAM, 0, 0)
{
	InitInstance();
}

StyloQDocumentPrereqParam::StyloQDocumentPrereqParam(const StyloQDocumentPrereqParam & rS) : PPBaseFilt(PPFILT_STYLOQDOCUMENTPREREQPARAM, 0, 0)
{
	InitInstance();
	Copy(&rS, 1);
}

StyloQDocumentPrereqParam & FASTCALL StyloQDocumentPrereqParam::operator = (const StyloQDocumentPrereqParam & rS)
{
	Copy(&rS, 1);
	return *this;
}

int StyloQDocumentPrereqParam::InitInstance()
{
	SetFlatChunk(offsetof(StyloQDocumentPrereqParam, ReserveStart), offsetof(StyloQDocumentPrereqParam, Reserve)+sizeof(Reserve)-offsetof(StyloQDocumentPrereqParam, ReserveStart));
	return Init(1, 0);
}

/*static*/StyloQDocumentPrereqParam * StyloQDocumentPrereqParam::Read(SBuffer & rBuf)
{
	StyloQDocumentPrereqParam * p_filt = 0;
	const  size_t sav_offs = rBuf.GetRdOffs();
	const size_t avls = rBuf.GetAvailableSize();
	if(avls) {
		if(avls == sizeof(PPID)) {
			PPID   stylopalm_id = 0;
			rBuf.ReadStatic(&stylopalm_id, sizeof(stylopalm_id));
			p_filt = new StyloQDocumentPrereqParam;
			p_filt->PalmID = stylopalm_id;
		}
		else {
			PPBaseFilt * p_base_filt = 0;
			THROW(PPView::ReadFiltPtr(rBuf, &p_base_filt));
			if(p_base_filt) {
				const StyloQDocumentPrereqParam pattern_filt;
				if(p_base_filt->GetSignature() == pattern_filt.GetSignature()) {
					p_filt = static_cast<StyloQDocumentPrereqParam *>(p_base_filt);
				}
				else {
					assert(p_filt == 0);
					// Путаница в фильтрах - убиваем считанный фильтр
					ZDELETE(p_base_filt);
				}
			}
		}
	}
	CATCH
		rBuf.SetRdOffs(sav_offs);
		ZDELETE(p_filt);
	ENDCATCH
	return p_filt;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY_CLS(StyloQAttendancePrereqParam);

StyloQAttendancePrereqParam::StyloQAttendancePrereqParam() : PPBaseFilt(PPFILT_STYLOQATTENDANCEPREREQPARAM, 0, 0)
{
	InitInstance();
}

StyloQAttendancePrereqParam::StyloQAttendancePrereqParam(const StyloQAttendancePrereqParam & rS) : PPBaseFilt(PPFILT_STYLOQATTENDANCEPREREQPARAM, 0, 0)
{
	InitInstance();
	Copy(&rS, 1);
}

StyloQAttendancePrereqParam & FASTCALL StyloQAttendancePrereqParam::operator = (const StyloQAttendancePrereqParam & rS)
{
	Copy(&rS, 1);
	return *this;
}

int StyloQAttendancePrereqParam::InitInstance()
{
	SetFlatChunk(offsetof(StyloQAttendancePrereqParam, ReserveStart), offsetof(StyloQAttendancePrereqParam, PrcList)-offsetof(StyloQAttendancePrereqParam, ReserveStart));
	SetBranchObjIdListFilt(offsetof(StyloQAttendancePrereqParam, PrcList));
	SetBranchSString(offsetof(StyloQAttendancePrereqParam, PrcTitle));
	return Init(1, 0);
}

/*static*/int PPStyloQInterchange::Edit_PersonEventParam(StyloQPersonEventParam & rParam)
{
	class StyloQPersonEventParamDialog : public TDialog {
		DECL_DIALOG_DATA(StyloQPersonEventParam);
	public:
		StyloQPersonEventParamDialog() : TDialog(DLG_STQPEVPARAM)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			setCtrlReal(CTL_STQPEVPARAM_MAXDIST, Data.MaxGeoDistance);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			Data.MaxGeoDistance = getCtrlReal(sel = CTL_STQPEVPARAM_MAXDIST);
			if(Data.MaxGeoDistance < 0.0 || Data.MaxGeoDistance > 100*1000) {
				ok = PPErrorByDialog(this, sel, PPERR_USERINPUT);
			}
			else {
				ASSIGN_PTR(pData, Data);
			}
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmPersonEventPacket)) {
				if(PeObj.EditPacket(&Data.PePack, true) > 0) {
					;
				}
			}
			else
				return;
			clearEvent(event);
		}
		PPObjPersonEvent PeObj;
	};

	DIALOG_PROC_BODY(StyloQPersonEventParamDialog, &rParam);
}

/*static*/int PPStyloQInterchange::Edit_RsrvDocumentPrereqParam(StyloQDocumentPrereqParam & rParam)
{
	class RsrvDocumentPrereqParamDialog : public TDialog {
		DECL_DIALOG_DATA(StyloQDocumentPrereqParam);
		enum {
			ctlgroupLoc = 1
		};
	public:
		RsrvDocumentPrereqParamDialog() : TDialog(DLG_STQDOCPARAM)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			//
			SetupPPObjCombo(this, CTLSEL_STQDOCPARAM_PALM, PPOBJ_STYLOPALM, Data.PalmID, OLW_CANINSERT|OLW_INSCONTEXTEDITEMS);
			PPIDArray op_types;
			op_types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSORDER, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, 0L);
			SetupOprKindCombo(this, CTLSEL_STQDOCPARAM_OP, Data.OpID, 0, &op_types, 0);
			SetupLocationCombo(this, CTLSEL_STQDOCPARAM_LOC, Data.LocID, 0, LOCTYP_WAREHOUSE, 0);
			SetupPPObjCombo(this, CTLSEL_STQDOCPARAM_QK, PPOBJ_QUOTKIND, Data.QuotKindID, 0);
			AddClusterAssoc(CTL_STQDOCPARAM_FLAGS, 0, StyloQDocumentPrereqParam::fUseBarcodeSearch);
			AddClusterAssoc(CTL_STQDOCPARAM_FLAGS, 1, StyloQDocumentPrereqParam::fUseBrands);
			AddClusterAssoc(CTL_STQDOCPARAM_FLAGS, 2, StyloQDocumentPrereqParam::fDlvrDateAsNominal); // @v11.6.1
			AddClusterAssoc(CTL_STQDOCPARAM_FLAGS, 3, StyloQDocumentPrereqParam::fUseHierarchGroups); // @v11.6.4
			AddClusterAssoc(CTL_STQDOCPARAM_FLAGS, 4, StyloQDocumentPrereqParam::fHideStock); // @v11.6.4
			SetClusterData(CTL_STQDOCPARAM_FLAGS, Data.Flags);
			SetupCtrls();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlData(CTLSEL_STQDOCPARAM_PALM, &Data.PalmID);
			getCtrlData(CTLSEL_STQDOCPARAM_OP, &Data.OpID);
			getCtrlData(CTLSEL_STQDOCPARAM_LOC, &Data.LocID);
			getCtrlData(CTLSEL_STQDOCPARAM_QK, &Data.QuotKindID);
			GetClusterData(CTL_STQDOCPARAM_FLAGS, &Data.Flags);
			//
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_STQDOCPARAM_PALM)) {
				SetupCtrls();
			}
			else
				return;
			clearEvent(event);
		}
		void   SetupCtrls()
		{
			getCtrlData(CTLSEL_STQDOCPARAM_PALM, &Data.PalmID);
			disableCtrls(Data.PalmID != 0, CTLSEL_STQDOCPARAM_OP, CTLSEL_STQDOCPARAM_LOC, CTLSEL_STQDOCPARAM_QK, 0);
			DisableClusterItem(CTL_STQDOCPARAM_FLAGS, 1, Data.PalmID != 0);
		}
	};

	DIALOG_PROC_BODY(RsrvDocumentPrereqParamDialog, &rParam);
}

/*static*/int PPStyloQInterchange::Edit_RsrvAttendancePrereqParam(StyloQAttendancePrereqParam & rParam)
{
	class RsrvAttendancePrereqParamDialog : public StyloQDialogWithPrcList {
		DECL_DIALOG_DATA(StyloQAttendancePrereqParam);
	public:
		RsrvAttendancePrereqParamDialog() : StyloQDialogWithPrcList(DLG_STQATTCPARAM, CTL_STQATTCPARAM_PRCLIST)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			setCtrlString(CTL_STQATTCPARAM_PRCTTL, Data.PrcTitle);
			setCtrlLong(CTL_STQATTCPARAM_MAXSCHD, Data.MaxScheduleDays); // @v11.3.10
			SetupLocationCombo(this, CTLSEL_STQATTCPARAM_LOC, Data.LocID, 0, LOCTYP_WAREHOUSE, 0); // @v11.3.12
			SetupPPObjCombo(this, CTLSEL_STQATTCPARAM_QK, PPOBJ_QUOTKIND, Data.QuotKindID, 0); // @v11.3.12
			SetDataRef(&Data.PrcList);
			// @v11.4.3 {
			AddClusterAssocDef(CTL_STQATTCPARAM_TSD, 0, 5);
			AddClusterAssoc(CTL_STQATTCPARAM_TSD, 1, 10);
			AddClusterAssoc(CTL_STQATTCPARAM_TSD, 2, 15);
			SetClusterData(CTL_STQATTCPARAM_TSD, Data.TimeSheetDiscreteness);
			// } @v11.4.3 
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlString(CTL_STQATTCPARAM_PRCTTL, Data.PrcTitle);
			Data.MaxScheduleDays = inrangeordefault(getCtrlLong(CTL_STQATTCPARAM_MAXSCHD), 1L, 365L, 7L); // @v11.3.10
			Data.LocID = getCtrlLong(CTLSEL_STQATTCPARAM_LOC); // @v11.3.12
			Data.QuotKindID = getCtrlLong(CTLSEL_STQATTCPARAM_QK); // @v11.3.12
			Data.TimeSheetDiscreteness = static_cast<uint16>(GetClusterData(CTL_STQATTCPARAM_TSD)); // @v11.4.3
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	};
	DIALOG_PROC_BODY(RsrvAttendancePrereqParamDialog, &rParam);
}

SJson * PPStyloQInterchange::MakeRsrvAttendancePrereqResponse_Prc(const SBinaryChunk & rOwnIdent, PPID prcID, PPID mainQuotKindID, PPObjTSession & rTSesObj, 
	long maxScheduleDays, LAssocArray * pGoodsToPrcList, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	SJson * p_result = 0;
	PPProcessorPacket prc_pack;
	if(rTSesObj.PrcObj.GetPacket(prcID, &prc_pack) > 0) {
		SString temp_buf;
		PPIDArray goods_id_list;
		PPIDArray tec_id_list;
		PPIDArray ar_list_by_prc;
		PPObjArticle ar_obj;
		ar_obj.GetByProcessor(0, prcID, &ar_list_by_prc);
		ar_list_by_prc.add(0L); // Список статей нужен для идентификации котировок. Добавим нулевую для итерацию без учета статьи.
			// Нулевой элемент должен быть в конце!
		rTSesObj.TecObj.GetGoodsListByPrc(prcID, &goods_id_list);
		rTSesObj.TecObj.GetListByPrc(prcID, &tec_id_list);
		if(goods_id_list.getCount()) {
			assert(tec_id_list.getCount());
			TechTbl::Rec tec_rec;
			PPObjPerson psn_obj;
			PPObjStaffCal scal_obj;
			goods_id_list.sortAndUndup();
			p_result = MakeObjJson_Prc(rOwnIdent, prc_pack.Rec, 0, pStat);
			// @todo Внести подробную информацию о связанном объекте
			if(prc_pack.Rec.LinkObjType == PPOBJ_PERSON) {
				PersonTbl::Rec psn_rec;
				if(psn_obj.Fetch(prc_pack.Rec.LinkObjID, &psn_rec) > 0) {
					const PPObjID oid(PPOBJ_PERSON, psn_rec.ID);
					{
						const PPID reg_cal_id = psn_obj.GetConfig().RegStaffCalID;
						if(reg_cal_id) {
							const LDATE curdt = getcurdate_();
							STimeChunkArray work_list;
							//STimeChunkArray collapse_list;
							DateRange period;
							assert(maxScheduleDays >= 0);
							period.Set(curdt, plusdate(curdt, maxScheduleDays));
							assert(checkdate(period.low) && checkdate(period.upp));
							ScObjAssoc scoa;
							scal_obj.InitScObjAssoc(reg_cal_id, 0, oid.Id, &scoa);
							if(scal_obj.CalcPeriod(scoa, period, 0, 0, 0, &work_list) && work_list.getCount()) {
								work_list.Sort();
								SJson * p_js_wl = SJson::CreateArr();
								for(uint wlidx = 0; wlidx < work_list.getCount(); wlidx++) {
									const STimeChunk * p_wl_item = (const STimeChunk *)work_list.at(wlidx);
									SJson * p_js_wli = SJson::CreateObj();
									p_js_wli->InsertString("low", temp_buf.Z().Cat(p_wl_item->Start, DATF_ISO8601CENT, 0));
									p_js_wli->InsertString("upp", temp_buf.Z().Cat(p_wl_item->Finish, DATF_ISO8601CENT, 0));
									p_js_wl->InsertChild(p_js_wli);
								}
								p_result->Insert("worktime", p_js_wl);
							}
						}
					}
					{
						SString blob_signature;
						if(FetchBlobSignature(rOwnIdent, oid, 0, blob_signature)) {
							assert(blob_signature.Len() && blob_signature.IsAscii());
							p_result->InsertString("imgblobs", blob_signature);
							Stq_CmdStat_MakeRsrv_Response::RegisterBlobOid(pStat, oid);
						}
					}
				}
			}
			{
				SJson * p_js_prcgoodslist = SJson::CreateArr();
				for(uint gidx = 0; gidx < goods_id_list.getCount(); gidx++) {
					const PPID goods_id = goods_id_list.get(gidx);
					SJson * p_js_goods_item = SJson::CreateObj();
					p_js_goods_item->InsertInt("id", goods_id);
					uint tec_time_sec = 0;
					for(uint tecidx = 0; tecidx < tec_id_list.getCount(); tecidx++) {
						const PPID tec_id = tec_id_list.get(tecidx);
						if(rTSesObj.TecObj.Fetch(tec_id, &tec_rec) > 0 && tec_rec.GoodsID == goods_id) {
							if(tec_rec.Capacity > 0.0) {
								tec_time_sec = R0i(1.0 / tec_rec.Capacity);
								break;
							}
						}
					}
					if(tec_time_sec > 0) {
						p_js_goods_item->InsertInt("duration", tec_time_sec);
					}
					{
						// price
						double price = 0.0;
						PPIDArray qk_list;
						qk_list.addnz(mainQuotKindID);
						qk_list.add(PPQUOTK_BASE);
						for(uint qkidx = 0; price == 0.0 && qkidx < qk_list.getCount(); qkidx++) {
							const PPID qk_id = qk_list.get(qkidx);
							assert(qk_id != 0);
							for(uint aridx = 0; price == 0.0 && aridx < ar_list_by_prc.getCount(); aridx++) {
								const PPID ar_id = ar_list_by_prc.get(aridx);
								QuotIdent qi(prc_pack.Rec.LocID, qk_id, 0/*curID*/, ar_id);
								double result = 0.0;
								if(rTSesObj.GObj.GetQuotExt(goods_id, qi, 0.0, 0.0, &result, 1) > 0) {
									if(result > 0.0)
										price = result;
								}
							}
						}
						if(price > 0.0) {
							p_js_goods_item->InsertDouble("price", price, MKSFMTD(0, 2, 0));
						}
					}
					p_js_prcgoodslist->InsertChild(p_js_goods_item);
					CALLPTRMEMB(pGoodsToPrcList, Add(goods_id, prcID));
				}
				p_result->Insert("goods_list", p_js_prcgoodslist);
			}
			{
				SJson * p_js_busylist = SJson::CreateArr();
				uint   busylist_count = 0;
				PrcBusyArray busy_list;
				STimeChunk observe_time_chunk(getcurdatetime_(), LDATETIME().SetFar());
				rTSesObj.P_Tbl->LoadBusyArray(prcID, 0, TSESK_SESSION, &observe_time_chunk, &busy_list);
				for(uint blidx = 0; blidx < busy_list.getCount(); blidx++) {
					const STimeChunk * p_busy_item = static_cast<const STimeChunk *>(busy_list.at(blidx));
					if(checkdate(p_busy_item->Start.d) && checkdate(p_busy_item->Finish.d)) {
						SJson * p_js_busy_entry = SJson::CreateObj();
						p_js_busy_entry->InsertString("low", temp_buf.Z().Cat(p_busy_item->Start, DATF_ISO8601CENT, 0));
						p_js_busy_entry->InsertString("upp", temp_buf.Z().Cat(p_busy_item->Finish, DATF_ISO8601CENT, 0));
						p_js_busylist->InsertChild(p_js_busy_entry);
						busylist_count++;
					}
				}
				if(busylist_count) {
					p_result->Insert("busy_list", p_js_busylist);
				}
				else
					ZDELETE(p_js_busylist);
			}
		}
	}
	return p_result;
}
//
//
//
/*static*/int PPStyloQInterchange::Document::CliStatus::GetFromBill(const TSVector <CliStatus> & rList, const PPBillPacket & rBp)
{
	int    surr_id = 0;
	for(uint i = 0; !surr_id && i < rList.getCount(); i++) {
		const CliStatus & r_entry = rList.at(i);
		if(r_entry.StatusID) {
			if(rBp.Rec.StatusID == r_entry.StatusID) {
				if(!(r_entry.Flags & CliStatus::fBillFlagDeclined) || (rBp.Rec.Flags2 & BILLF2_DECLINED))
					surr_id = r_entry.SurrogateId;
			}
		}
		else if((r_entry.Flags & CliStatus::fBillFlagDeclined) && (rBp.Rec.Flags2 & BILLF2_DECLINED))
			surr_id = r_entry.SurrogateId;
	}
	return surr_id;
}

/*static*/int PPStyloQInterchange::Document::CliStatus::SetToBill(const TSVector <CliStatus> & rList, int surrId, PPBillPacket & rBp)
{
	int    ok = -1;
	if(surrId > 0) {
		for(uint i = 0; ok < 0 && i < rList.getCount(); i++) {
			const CliStatus & r_entry = rList.at(i);
			if(r_entry.SurrogateId == surrId) {
				if(r_entry.StatusID) {
					PPObjBillStatus bs_obj;
					PPBillStatus bs_rec;
					THROW(bs_obj.Fetch(r_entry.StatusID, &bs_rec) > 0);
					rBp.Rec.StatusID = r_entry.StatusID;
				}
				if(r_entry.Flags & CliStatus::fBillFlagDeclined) {
					rBp.Rec.Flags2 |= BILLF2_DECLINED;
				}
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

PPStyloQInterchange::Document::CliStatus::CliStatus()
{
	THISZERO();
}

PPStyloQInterchange::Document::CliStatus & PPStyloQInterchange::Document::CliStatus::Z()
{
	THISZERO();
	return *this;
}

SJson * PPStyloQInterchange::Document::CliStatus::ToJsonObj() const
{
	SJson * p_result = 0;
	if(IsValuable()) {
		p_result = SJson::CreateObj();
		p_result->InsertIntNz("surrid", SurrogateId);
		p_result->InsertIntNz("statusid", StatusID);
		p_result->InsertIntNz("rsrvdcase", ReservedCase);
		p_result->InsertIntNz("flags", Flags & fBillFlagDeclined);
		p_result->InsertStringNe("nm", NameUtf8);
	}
	return p_result;
}

bool PPStyloQInterchange::Document::CliStatus::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
		//SurrogateId = pJs->Get
	if(pJs && pJs->Type == SJson::tOBJECT) {
		SString temp_buf;
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("surrid")) {
				SurrogateId = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("statusid")) {
				StatusID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("rsrvdcase")) {
				ReservedCase = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("flags")) {
				Flags = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("nm")) {
				STRNSCPY(NameUtf8, p_cur->P_Child->Text);
			}
		}
		if(IsValuable())
			ok = true;
	}
	return ok;
}

IMPLEMENT_PPFILT_FACTORY_CLS(StyloQIncomingListParam);

StyloQIncomingListParam::StyloQIncomingListParam() : PPBaseFilt(PPFILT_STYLOQINCOMINGLISTPARAM, 0, 2)
{
	InitInstance();
}
	
StyloQIncomingListParam::StyloQIncomingListParam(const StyloQIncomingListParam & rS) : PPBaseFilt(PPFILT_STYLOQINCOMINGLISTPARAM, 0, 2)
{
	InitInstance();
	Copy(&rS, 1);
}

void StyloQIncomingListParam::NormalizeStatusList()
{
	if(StatusList.getCount()) {
		long   iter_surr_id = 1;
		LongArray seen_surr_id_list;
		for(uint i = 0; i < StatusList.getCount(); i++) {
			PPStyloQInterchange::Document::CliStatus & r_item = StatusList.at(i);
			if(r_item.SurrogateId == 0) {
				r_item.SurrogateId = iter_surr_id;
			}
			else if(seen_surr_id_list.lsearch(r_item.SurrogateId)) {
				if(iter_surr_id == r_item.SurrogateId)
					iter_surr_id++;
				r_item.SurrogateId = iter_surr_id;
			}
			seen_surr_id_list.add(r_item.SurrogateId);
			if(iter_surr_id == r_item.SurrogateId)
				iter_surr_id = r_item.SurrogateId+1;
			else
				iter_surr_id++;
		}
	}
}

/*virtual*/int StyloQIncomingListParam::ReadPreviousVer(SBuffer & rBuf, int ver)
{
	int    ok = -1;
	if(ver == 0) {
		class StyloQIncomingListParam_v0 : public PPBaseFilt {
			int InitInstance()
			{
				P_BF = 0;
				P_TdF = 0;
				P_TsF = 0;
				P_CcF = 0;
				SetFlatChunk(offsetof(StyloQIncomingListParam, ReserveStart), offsetof(StyloQIncomingListParam, Reserve)+sizeof(Reserve)-offsetof(StyloQIncomingListParam, ReserveStart));
				SetBranchBaseFiltPtr(PPFILT_BILL, offsetof(StyloQIncomingListParam, P_BF));
				SetBranchBaseFiltPtr(PPFILT_PRJTASK, offsetof(StyloQIncomingListParam, P_TdF));
				SetBranchBaseFiltPtr(PPFILT_TSESSION, offsetof(StyloQIncomingListParam, P_TsF));
				SetBranchBaseFiltPtr(PPFILT_CCHECK, offsetof(StyloQIncomingListParam, P_CcF));
				return Init(1, 0);
			}
		public:
			StyloQIncomingListParam_v0() : PPBaseFilt(PPFILT_STYLOQINCOMINGLISTPARAM, 0, 0)
			{
				InitInstance();
			}
			uint8  ReserveStart[48];
			DateRange Period;  // Это - отладочный критерий. В реальности должен использоваться LookbackDays, но так как тестовые 
				// базы данных не меняются, то LookbackDays при разработке использовать затруднительно.
			uint32 Flags;
			uint32 ActionFlags;
			int    StQBaseCmdId;
			int    LookbackDays; // Количество дней с отсчетом назад от текущей системной даты. Данные захватываются за период,
				// начиная от now-LookbackDays до бесконечности (данные будущими числами так же захватываются).
			long   Reserve;
			//
			// Фильтры, представленные здесь не подлежать прямому интерактивному редактированию пользователем.
			// Наружу выводится лишь часть критериев этих фильтров, остальные параметры будут задаваться автоматически.
			//
			BillFilt * P_BF;
			PrjTaskFilt * P_TdF;
			TSessionFilt * P_TsF;
			CCheckFilt * P_CcF;
		};
		StyloQIncomingListParam_v0 fv0;
		THROW(fv0.Read(rBuf, 0));
		MEMSZERO(ReserveStart);
		Reserve = 0;
		#define CPYFLD(f) f = fv0.f
		CPYFLD(Period);
		CPYFLD(Flags);
		CPYFLD(ActionFlags);
		CPYFLD(StQBaseCmdId);
		CPYFLD(LookbackDays);
		#undef CPYFLD
		AssignPtr2Ptr(P_BF, fv0.P_BF);
		AssignPtr2Ptr(P_TdF, fv0.P_TdF);
		AssignPtr2Ptr(P_TsF, fv0.P_TsF);
		AssignPtr2Ptr(P_CcF, fv0.P_CcF);
		StatusList.clear();
		ok = 1;
	}
	else if(ver == 1) {
		// Изменилась структура DocStatus
		class StyloQIncomingListParam_v1 : public PPBaseFilt {
			int InitInstance()
			{
				P_BF = 0;
				P_TdF = 0;
				P_TsF = 0;
				P_CcF = 0;
				SetFlatChunk(offsetof(StyloQIncomingListParam_v1, ReserveStart), offsetof(StyloQIncomingListParam_v1, Reserve)+sizeof(Reserve)-offsetof(StyloQIncomingListParam_v1, ReserveStart));
				SetBranchBaseFiltPtr(PPFILT_BILL, offsetof(StyloQIncomingListParam_v1, P_BF));
				SetBranchBaseFiltPtr(PPFILT_PRJTASK, offsetof(StyloQIncomingListParam_v1, P_TdF));
				SetBranchBaseFiltPtr(PPFILT_TSESSION, offsetof(StyloQIncomingListParam_v1, P_TsF));
				SetBranchBaseFiltPtr(PPFILT_CCHECK, offsetof(StyloQIncomingListParam_v1, P_CcF));
				SetBranchSVector(offsetof(StyloQIncomingListParam_v1, StatusList));
				return Init(1, 0);
			}
		public:
			StyloQIncomingListParam_v1() : PPBaseFilt(PPFILT_STYLOQINCOMINGLISTPARAM, 0, 1)
			{
				InitInstance();
			}
			struct DocStatus_v1 {
				DocStatus_v1()
				{
					THISZERO();
				}
				PPID   StatusID;
				long   ReservedCase; // DocStatus::rXXX
				long   Flags;
				char   NameUtf8[128]; // utf8 Наименование для клиента 
			};
			uint8  ReserveStart[48];
			DateRange Period;  // Это - отладочный критерий. В реальности должен использоваться LookbackDays, но так как тестовые 
				// базы данных не меняются, то LookbackDays при разработке использовать затруднительно.
			uint32 Flags;
			uint32 ActionFlags;
			int    StQBaseCmdId;
			int    LookbackDays; // Количество дней с отсчетом назад от текущей системной даты. Данные захватываются за период,
				// начиная от now-LookbackDays до бесконечности (данные будущими числами так же захватываются).
			long   Reserve;
			//
			// Фильтры, представленные здесь не подлежать прямому интерактивному редактированию пользователем.
			// Наружу выводится лишь часть критериев этих фильтров, остальные параметры будут задаваться автоматически.
			//
			BillFilt * P_BF;
			PrjTaskFilt * P_TdF;
			TSessionFilt * P_TsF;
			CCheckFilt * P_CcF;
			TSVector <DocStatus_v1> StatusList; // @v11.4.12 Список правил установки статуса
		};
		StyloQIncomingListParam_v1 fv1;
		THROW(fv1.Read(rBuf, 0));
		MEMSZERO(ReserveStart);
		Reserve = 0;
		#define CPYFLD(f) f = fv1.f
		CPYFLD(Period);
		CPYFLD(Flags);
		CPYFLD(ActionFlags);
		CPYFLD(StQBaseCmdId);
		CPYFLD(LookbackDays);
		#undef CPYFLD
		AssignPtr2Ptr(P_BF, fv1.P_BF);
		AssignPtr2Ptr(P_TdF, fv1.P_TdF);
		AssignPtr2Ptr(P_TsF, fv1.P_TsF);
		AssignPtr2Ptr(P_CcF, fv1.P_CcF);
		StatusList.clear();
		for(uint i = 0; i < fv1.StatusList.getCount(); i++) {
			const StyloQIncomingListParam_v1::DocStatus_v1 & r_entry_v1 = fv1.StatusList.at(i);
			PPStyloQInterchange::Document::CliStatus new_entry;
			new_entry.StatusID = r_entry_v1.StatusID;
			new_entry.ReservedCase = r_entry_v1.ReservedCase;
			new_entry.Flags = r_entry_v1.Flags;
			STRNSCPY(new_entry.NameUtf8, r_entry_v1.NameUtf8);
			THROW_SL(StatusList.insert(&new_entry));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

StyloQIncomingListParam & FASTCALL StyloQIncomingListParam::operator = (const StyloQIncomingListParam & rS)
{
	Copy(&rS, 1);
	return *this;
}

int StyloQIncomingListParam::InitInstance()
{
	P_BF = 0;
	P_TdF = 0;
	P_TsF = 0;
	P_CcF = 0;
	SetFlatChunk(offsetof(StyloQIncomingListParam, ReserveStart), offsetof(StyloQIncomingListParam, Reserve)+sizeof(Reserve)-offsetof(StyloQIncomingListParam, ReserveStart));
	SetBranchBaseFiltPtr(PPFILT_BILL, offsetof(StyloQIncomingListParam, P_BF));
	SetBranchBaseFiltPtr(PPFILT_PRJTASK, offsetof(StyloQIncomingListParam, P_TdF));
	SetBranchBaseFiltPtr(PPFILT_TSESSION, offsetof(StyloQIncomingListParam, P_TsF));
	SetBranchBaseFiltPtr(PPFILT_CCHECK, offsetof(StyloQIncomingListParam, P_CcF));
	SetBranchSVector(offsetof(StyloQIncomingListParam, StatusList)); // @v11.4.12
	return Init(1, 0);
}

class IncomingListParam_Base_Dialog : public TDialog {
protected:
	enum {
		ctlgroupLoc       = 2,
		ctlgroupArticle   = 3,
		ctlgroupAgent     = 4
	};
	DECL_DIALOG_DATA(StyloQIncomingListParam);
	//
	int    EditStatusList()
	{
		// @todo
		class StatusListDialog : public PPListDialog {
			DECL_DIALOG_DATA(TSVector <PPStyloQInterchange::Document::CliStatus>);
		public:
			StatusListDialog() : PPListDialog(DLG_STQSTATUSL, CTL_STQSTATUSL_LIST)
			{
			}
			DECL_DIALOG_SETDTS()
			{
				int    ok = 1;
				RVALUEPTR(Data, pData);
				updateList(-1);
				//
				return ok;
			}
			DECL_DIALOG_GETDTS()
			{
				int    ok = 1;
				//
				ASSIGN_PTR(pData, Data);
				return ok;
			}
		private:
			int    EditStatusEntry(PPStyloQInterchange::Document::CliStatus & rData)
			{
				int    ok = -1;
				TDialog * dlg = new TDialog(DLG_STQSTATUSE);
				if(CheckDialogPtr(&dlg)) {
					SString temp_buf;
					SetupPPObjCombo(dlg, CTLSEL_STQSTATUSE_STATUS, PPOBJ_BILLSTATUS, rData.StatusID, 0);
					dlg->AddClusterAssocDef(CTL_STQSTATUSE_RSRVST, 0, PPStyloQInterchange::Document::CliStatus::rUndef);
					dlg->AddClusterAssoc(CTL_STQSTATUSE_RSRVST, 1, PPStyloQInterchange::Document::CliStatus::rAccepted);
					dlg->AddClusterAssoc(CTL_STQSTATUSE_RSRVST, 2, PPStyloQInterchange::Document::CliStatus::rRejected);
					dlg->SetClusterData(CTL_STQSTATUSE_RSRVST, rData.ReservedCase);
					dlg->AddClusterAssoc(CTL_STQSTATUSE_FLAGS, 0, PPStyloQInterchange::Document::CliStatus::fBillFlagDeclined);
					dlg->SetClusterData(CTL_STQSTATUSE_FLAGS, rData.Flags);
					dlg->setCtrlString(CTL_STQSTATUSE_NAME, (temp_buf = rData.NameUtf8).Transf(CTRANSF_UTF8_TO_INNER));
					while(ok < 0 && ExecView(dlg) == cmOK) {
						rData.StatusID = dlg->getCtrlLong(CTLSEL_STQSTATUSE_STATUS);
						dlg->GetClusterData(CTL_STQSTATUSE_RSRVST, &rData.ReservedCase);
						dlg->GetClusterData(CTL_STQSTATUSE_FLAGS, &rData.Flags);
						dlg->getCtrlString(CTL_STQSTATUSE_NAME, temp_buf);
						STRNSCPY(rData.NameUtf8, temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
						// @todo Проверить на ошибки
						ok = 1;
					}
				}
				delete dlg;
				return ok;
			}
			virtual int  setupList()
			{
				int    ok = 1;
				StringSet ss(SLBColumnDelim);
				SString temp_buf;
				PPObjBillStatus bs_obj;
				PPBillStatus bs_rec;
				for(uint i = 0; i < Data.getCount(); i++) {
					const PPStyloQInterchange::Document::CliStatus & r_item = Data.at(i);
					ss.Z();
					if(r_item.StatusID) {
						if(bs_obj.Fetch(r_item.StatusID, &bs_rec) > 0) {
							temp_buf = bs_rec.Name;
						}
						else {
							temp_buf.Z().CatChar('#').Cat(r_item.StatusID);
						}
					}
					else {
						temp_buf = "none";
					}
					ss.add(temp_buf);
					temp_buf.Z();
					if(r_item.ReservedCase == PPStyloQInterchange::Document::CliStatus::rAccepted)
						temp_buf = "accepted";
					else if(r_item.ReservedCase == PPStyloQInterchange::Document::CliStatus::rRejected)
						temp_buf = "rejected";
					ss.add(temp_buf);
					temp_buf.Z();
					if(r_item.Flags & PPStyloQInterchange::Document::CliStatus::fBillFlagDeclined)
						temp_buf = "set bill declined flag";
					ss.add(temp_buf);
					(temp_buf = r_item.NameUtf8).Transf(CTRANSF_UTF8_TO_INNER);
					ss.add(temp_buf);
					addStringToList(i+1, ss.getBuf());
				}
				return ok;
			}
			virtual int  addItem(long * pPos, long * pID)
			{
				int    ok = -1;
				PPStyloQInterchange::Document::CliStatus item;
				if(EditStatusEntry(item) > 0) {
					Data.insert(&item);
					ASSIGN_PTR(pPos, Data.getCount()-1);
					ASSIGN_PTR(pID, Data.getCount());
					ok = 1;
				}
				return ok;
			}
			virtual int  editItem(long pos, long id)
			{
				int    ok = -1;
				if(pos >= 0 && pos < Data.getCountI()) {
					if(EditStatusEntry(Data.at(pos)) > 0)
						ok = 1;
				}
				return ok;
			}
			virtual int  delItem(long pos, long id)
			{
				int    ok = -1;
				if(pos >= 0 && pos < Data.getCountI()) {
					Data.atFree(pos);
					ok = 1;
				}
				return ok;
			}
		};
		DIALOG_PROC_BODY(StatusListDialog, &Data.StatusList);
	}
public:
	IncomingListParam_Base_Dialog(uint dlgId) : TDialog(dlgId)
	{
		if(CConfig.Flags & CCFLG_DEBUG) {
			showCtrl(CTL_STQINLPARAM_PERIOD, 1);
		}
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		int    ok = 1;
		setCtrlData(CTL_STQINLPARAM_LBD, &Data.LookbackDays);
		if(CConfig.Flags & CCFLG_DEBUG) {
			SetPeriodInput(this, CTL_STQINLPARAM_PERIOD, &Data.Period);
		}
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTL_STQINLPARAM_LBD, &Data.LookbackDays);
		if(CConfig.Flags & CCFLG_DEBUG) {
			GetPeriodInput(this, CTL_STQINLPARAM_PERIOD, &Data.Period);
		}
		// (в базовом классе нельзя - финальное присвоение осуществляется порожденными классами) ASSIGN_PTR(pData, Data);
		return ok;
	}
};

class IncomingListParam_CCheck_Dialog : public IncomingListParam_Base_Dialog {
public:
	IncomingListParam_CCheck_Dialog() : IncomingListParam_Base_Dialog(DLG_STQINLCPARAM)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		IncomingListParam_Base_Dialog::setDTS(pData);
		if(!Data.P_CcF)
			Data.P_CcF = new CCheckFilt;
		//
		{
			PPID   posnode_id = Data.P_CcF->NodeList.GetSingle();
			PPObjCashNode::SelFilt f;
			f.SyncGroup = 1;
			SetupPPObjCombo(this, CTLSEL_STQINLPARAM_POS, PPOBJ_CASHNODE, posnode_id, 0, &f);
		}
		AddClusterAssoc(CTL_STQINLPARAM_ACTIONS, 0, StyloQIncomingListParam::actionCCheckCreat);
		AddClusterAssoc(CTL_STQINLPARAM_ACTIONS, 1, StyloQIncomingListParam::actionCCheckMod);
		AddClusterAssoc(CTL_STQINLPARAM_ACTIONS, 2, StyloQIncomingListParam::actionCCheckRegPrint);
		SetClusterData(CTL_STQINLPARAM_ACTIONS, Data.ActionFlags);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		if(!Data.P_CcF)
			Data.P_CcF = new CCheckFilt;
		IncomingListParam_Base_Dialog::getDTS(pData);
		{
			PPID   posnode_id = Data.P_CcF->NodeList.GetSingle();
			getCtrlData(CTLSEL_STQINLPARAM_POS, &posnode_id);
			Data.P_CcF->NodeList.SetSingle(posnode_id);
		}
		Data.ActionFlags = GetClusterData(CTL_STQINLPARAM_ACTIONS);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
};

class IncomingListParam_TSess_Dialog : public IncomingListParam_Base_Dialog {
public:
	IncomingListParam_TSess_Dialog() : IncomingListParam_Base_Dialog(DLG_STQINLSPARAM)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		IncomingListParam_Base_Dialog::setDTS(pData);
		if(!Data.P_TsF)
			Data.P_TsF = new TSessionFilt;
		//
		SetupPPObjCombo(this, CTLSEL_STQINLPARAM_PRC, PPOBJ_PROCESSOR, Data.P_TsF->PrcID, OLW_CANSELUPLEVEL|OLW_INSCONTEXTEDITEMS);
		AddClusterAssoc(CTL_STQINLPARAM_ACTIONS, 0, StyloQIncomingListParam::actionTSessCreat);
		AddClusterAssoc(CTL_STQINLPARAM_ACTIONS, 1, StyloQIncomingListParam::actionTSessTmSet);
		AddClusterAssoc(CTL_STQINLPARAM_ACTIONS, 2, StyloQIncomingListParam::actionDocStatus);
		SetClusterData(CTL_STQINLPARAM_ACTIONS, Data.ActionFlags);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		if(!Data.P_TsF)
			Data.P_TsF = new TSessionFilt;
		IncomingListParam_Base_Dialog::getDTS(pData);
		getCtrlData(CTLSEL_STQINLPARAM_PRC, &Data.P_TsF->PrcID);
		Data.ActionFlags = GetClusterData(CTL_STQINLPARAM_ACTIONS);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
};

class IncomingListParam_Doc_Dialog : public IncomingListParam_Base_Dialog {
public:
	IncomingListParam_Doc_Dialog() : IncomingListParam_Base_Dialog(DLG_STQINLOPARAM)
	{
		addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_STQINLPARAM_LOC, 0, 0, cmLocList, 0, 0, 0));
		addGroup(ctlgroupArticle, new ArticleCtrlGroup(0, CTLSEL_STQINLPARAM_OP, CTLSEL_STQINLPARAM_CLI, 0, 0, ArticleCtrlGroup::fUseByContextValue));
		addGroup(ctlgroupAgent, new ArticleCtrlGroup(0, 0, CTLSEL_STQINLPARAM_AGNT, 0, GetAgentAccSheet(), ArticleCtrlGroup::fUseByContextValue));
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		IncomingListParam_Base_Dialog::setDTS(pData);
		if(!Data.P_BF) {
			Data.P_BF = new BillFilt;
		}
		PPIDArray op_types;
		op_types.addzlist(PPOPT_ACCTURN, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSORDER,
			PPOPT_PAYMENT, PPOPT_GOODSRETURN, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_INVENTORY, 0L);
		SetupOprKindCombo(this, CTLSEL_STQINLPARAM_OP, Data.P_BF->OpID, 0, &op_types, 0);
		{
			ArticleCtrlGroup::Rec grp_rec(0, Data.P_BF->OpID, Data.P_BF->ObjectID);
			setGroupData(ctlgroupArticle, &grp_rec);
		}
		{
			ArticleCtrlGroup::Rec grp_rec(GetAgentAccSheet(), Data.P_BF->OpID, Data.P_BF->AgentID);
			setGroupData(ctlgroupAgent, &grp_rec);
		}
		{
			LocationCtrlGroup::Rec loc_rec(&Data.P_BF->LocList);
			setGroupData(ctlgroupLoc, &loc_rec);
		}
		AddClusterAssoc(CTL_STQINLPARAM_ACTIONS, 0, StyloQIncomingListParam::actionDocStatus);
		AddClusterAssoc(CTL_STQINLPARAM_ACTIONS, 1, StyloQIncomingListParam::actionDocAcceptance);
		AddClusterAssoc(CTL_STQINLPARAM_ACTIONS, 2, StyloQIncomingListParam::actionDocAcceptanceMarks);
		AddClusterAssoc(CTL_STQINLPARAM_ACTIONS, 3, StyloQIncomingListParam::actionDocSettingMarks);
		AddClusterAssoc(CTL_STQINLPARAM_ACTIONS, 4, StyloQIncomingListParam::actionDocInventory);
		AddClusterAssoc(CTL_STQINLPARAM_ACTIONS, 5, StyloQIncomingListParam::actionGoodsItemCorrection);
		SetClusterData(CTL_STQINLPARAM_ACTIONS, Data.ActionFlags);
		// @v11.4.8 {
		AddClusterAssoc(CTL_STQINLPARAM_FLAGS, 0, StyloQIncomingListParam::fBillWithMarksOnly);
		AddClusterAssoc(CTL_STQINLPARAM_FLAGS, 1, StyloQIncomingListParam::fBillWithMarkedGoodsOnly);
		SetClusterData(CTL_STQINLPARAM_FLAGS, Data.Flags);
		// } @v11.4.8 
		// @v11.7.0 {
		AddClusterAssoc(CTL_STQINLPARAM_ACCEPT, 0, StyloQIncomingListParam::acceptanceNone);
		AddClusterAssoc(CTL_STQINLPARAM_ACCEPT, 1, StyloQIncomingListParam::acceptanceTags);
		AddClusterAssoc(CTL_STQINLPARAM_ACCEPT, 2, StyloQIncomingListParam::acceptanceModifyOrgDoc);
		AddClusterAssoc(CTL_STQINLPARAM_ACCEPT, 3, StyloQIncomingListParam::acceptanceLinkDraft);
		SetClusterData(CTL_STQINLPARAM_ACCEPT, Data.Acceptance);
		// } @v11.7.0
		SetupOp();
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		if(!Data.P_BF) {
			Data.P_BF = new BillFilt;
		}
		IncomingListParam_Base_Dialog::getDTS(pData);
		{
			LocationCtrlGroup::Rec loc_rec;
			getGroupData(ctlgroupLoc, &loc_rec);
			Data.P_BF->LocList = loc_rec.LocList;
		}
		{
			ArticleCtrlGroup::Rec grp_rec;
			getGroupData(ctlgroupArticle, &grp_rec);
			Data.P_BF->OpID = grp_rec.OpID;
			Data.P_BF->ObjectID = grp_rec.ArList.GetSingle();
		}
		{
			ArticleCtrlGroup::Rec grp_rec;
			getGroupData(ctlgroupAgent, &grp_rec);
			Data.P_BF->AgentID = grp_rec.ArList.GetSingle();
		}
		Data.ActionFlags = static_cast<uint32>(GetClusterData(CTL_STQINLPARAM_ACTIONS));
		Data.Flags = static_cast<uint32>(GetClusterData(CTL_STQINLPARAM_FLAGS)); // @v11.4.8 
		GetClusterData(CTL_STQINLPARAM_ACCEPT, &Data.Acceptance); // @v11.7.0
		//
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		IncomingListParam_Base_Dialog::handleEvent(event);
		if(event.isCmd(cmStatusList)) {
			EditStatusList();
			Data.NormalizeStatusList();
		}
		else if(event.isCbSelected(CTLSEL_STQINLPARAM_OP)) {
			SetupOp();
		}
		else
			return;
		clearEvent(event);
	}
	void    SetupOp()
	{
		const PPID op_id = getCtrlLong(CTLSEL_STQINLPARAM_OP);
		const PPID op_type_id = GetOpType(op_id);
		DisableClusterItem(CTL_STQINLPARAM_ACTIONS, 1, !oneof4(op_type_id, PPOPT_GOODSRECEIPT, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_DRAFTEXPEND)); // StyloQIncomingListParam::actionDocAcceptance
		DisableClusterItem(CTL_STQINLPARAM_ACTIONS, 2, !oneof2(op_type_id, PPOPT_GOODSRECEIPT, PPOPT_GOODSRECEIPT)); // StyloQIncomingListParam::actionDocAcceptanceMarks
		DisableClusterItem(CTL_STQINLPARAM_ACTIONS, 3, !oneof2(op_type_id, PPOPT_GOODSEXPEND, PPOPT_DRAFTEXPEND)); // StyloQIncomingListParam::actionDocSettingMarks
		DisableClusterItem(CTL_STQINLPARAM_ACTIONS, 4, (op_type_id != PPOPT_INVENTORY)); // StyloQIncomingListParam::actionDocInventory
		DisableClusterItem(CTL_STQINLPARAM_ACTIONS, 5, (op_type_id != PPOPT_GOODSORDER)); //StyloQIncomingListParam::actionGoodsItemCorrection
	}
};

/*static*/int PPStyloQInterchange::Edit_IncomingListParam(StyloQIncomingListParam & rParam)
{
	int    ok = -1;
	if(rParam.StQBaseCmdId == StyloQCommandList::sqbcIncomingListOrder) {
		ok = PPDialogProcBody <IncomingListParam_Doc_Dialog, StyloQIncomingListParam> (&rParam);
	}
	else if(rParam.StQBaseCmdId == StyloQCommandList::sqbcIncomingListCCheck) { // @v11.5.2
		ok = PPDialogProcBody <IncomingListParam_CCheck_Dialog, StyloQIncomingListParam> (&rParam);
	}
	else if(rParam.StQBaseCmdId == StyloQCommandList::sqbcIncomingListTSess) { // @v11.6.5
		ok = PPDialogProcBody <IncomingListParam_TSess_Dialog, StyloQIncomingListParam> (&rParam);
	}
	return ok;
}
//
//
//
int PPStyloQInterchange::MakeDocDeclareJs(const StyloQCommandList::Item & rCmdItem, const char * pDl600Symb, SString & rDocDeclaration)
{
	return DocumentDeclaration(&rCmdItem, pDl600Symb).ToJson(rDocDeclaration);  // @v11.4.9
}

int PPStyloQInterchange::Document::FromTSessionPacket(const TSessionPacket & rS, PPIDArray * pGoodsIdList) // @v11.6.5
{
	Z();
	int     ok = 1;
	SString memo_buf;
	ID = rS.Rec.ID;
	if(rS.TagL.GetItemStr(PPTAG_TSESS_OUTERCODE, Code) > 0) {
		Code.Transf(CTRANSF_INNER_TO_UTF8);
	}
	else
		Code.Cat(rS.Rec.Num);
	rS.GetGuid(Uuid);
	rS.Ext.GetExtStrData(PRCEXSTR_MEMO, memo_buf);
	memo_buf.Transf(CTRANSF_INNER_TO_UTF8);
	{
		PPObjTSession tses_obj;
		Document::BookingItem * p_bi = BkList.CreateNewItem();
		p_bi->RowIdx = 1;
		p_bi->PrcID = rS.Rec.PrcID;
		if(checkdate(rS.Rec.StDt) && checktime(rS.Rec.StTm)) {
			long duration = 0;
			LDATETIME st_dtm;
			st_dtm.Set(rS.Rec.StDt, rS.Rec.StTm);
			p_bi->ReqTime = st_dtm;
			if(checkdate(rS.Rec.FinDt) && checktime(rS.Rec.FinTm)) {
				LDATETIME fin_dtm;
				fin_dtm.Set(rS.Rec.FinDt, rS.Rec.FinTm);
				duration = diffdatetimesec(fin_dtm, st_dtm);
			}
			else if(rS.Rec.PlannedTiming > 0) 
				duration = rS.Rec.PlannedTiming;
			p_bi->EstimatedDurationSec = duration;
		}
		if(rS.Rec.TechID) {
			TechTbl::Rec tec_rec;
			if(tses_obj.GetTech(rS.Rec.TechID, &tec_rec) > 0) {
				if(tec_rec.GoodsID > 0) {
					p_bi->GoodsID = tec_rec.GoodsID;
					CALLPTRMEMB(pGoodsIdList, add(tec_rec.GoodsID));
				}
			}
		}
		p_bi->Memo = memo_buf;
	}
	return ok;
}

int PPStyloQInterchange::Document::FromCCheckPacket(const CCheckPacket & rS, PPID posNodeID, PPIDArray * pGoodsIdList)
{
	Z();
	int    ok = 1;
	SString temp_buf;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	ID = rS.Rec.ID;
	Code.Cat(rS.Rec.Code);
	CreationTime = rS.Ext.CreationDtm;
	Time.Set(rS.Rec.Dt, rS.Rec.Tm);
	SvcOpType = 0; // @v11.7.2 Это - не документ. Типа операции нет.
	SvcOpID = 0; // Это - не документ. Вида операции нет.
	InterchangeOpID = StyloQCommandList::sqbdtCCheck; // @v11.5.8
	PosNodeID = posNodeID; // @v11.5.8
	AgentID = rS.Ext.SalerID;
	ClientID = 0; // ???
	DlvrLocID = 0; // ???
	DueTime.Z(); // ???
	Amount = MONEYTOLDBL(rS.Rec.Amount);
	rS.GetGuid(Uuid);
	rS.GetExtStrData(CCheckPacket::extssMemo, temp_buf);
	Memo = temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
	for(uint i = 0; i < rS.GetCount(); i++) {
		const CCheckLineTbl::Rec & r_line = rS.GetLineC(i);
		const PPID goods_id = labs(r_line.GoodsID);
		CCheckPacket::LineExt le;
		PPStyloQInterchange::Document::__TransferItem * p_doc_ti = TiList.CreateNewItem();
		THROW_SL(p_doc_ti);
		p_doc_ti->RowIdx = r_line.RByCheck;
		p_doc_ti->GoodsID = goods_id;
		if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
			p_doc_ti->UnitID = goods_rec.UnitID;
		}
		p_doc_ti->Set.Qtty = r_line.Quantity;
		p_doc_ti->Set.Price = intmnytodbl(r_line.Price);
		p_doc_ti->Set.Discount = r_line.Dscnt;
		CALLPTRMEMB(pGoodsIdList, add(goods_id));
		if(rS.GetLineExt(i+1, le) > 0) {
			p_doc_ti->CcQueue = le.Queue;
			SETFLAG(p_doc_ti->Flags, PPStyloQInterchange::Document::__TransferItem::fCcFixedPrice, le.Flags & CCheckPacket::LineExt::fFixedPrice);
			SETFLAG(p_doc_ti->Flags, PPStyloQInterchange::Document::__TransferItem::fCcGroup, le.Flags & CCheckPacket::LineExt::fGroup);
			SETFLAG(p_doc_ti->Flags, PPStyloQInterchange::Document::__TransferItem::fCcModifier, le.Flags & CCheckPacket::LineExt::fModifier);
			SETFLAG(p_doc_ti->Flags, PPStyloQInterchange::Document::__TransferItem::fCcPartOfComplex, le.Flags & CCheckPacket::LineExt::fPartOfComplex);
			SETFLAG(p_doc_ti->Flags, PPStyloQInterchange::Document::__TransferItem::fCcQuotedByGift, le.Flags & CCheckPacket::LineExt::fQuotedByGift);
		}
		rS.GetLineTextExt(i+1, CCheckPacket::lnextSerial, temp_buf);
		p_doc_ti->Serial = temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
		rS.GetLineTextExt(i+1, CCheckPacket::lnextEgaisMark, temp_buf);
		if(temp_buf.NotEmpty()) {
			temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
			PPStyloQInterchange::Document::LotExtCode lec;
			STRNSCPY(lec.Code, temp_buf);
			p_doc_ti->XcL.insert(&lec);
		}
		rS.GetLineTextExt(i+1, CCheckPacket::lnextChZnMark, temp_buf);
		if(temp_buf.NotEmpty()) {
			temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
			PPStyloQInterchange::Document::LotExtCode lec;
			STRNSCPY(lec.Code, temp_buf);
			p_doc_ti->XcL.insert(&lec);
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::Document::FromBillPacket(const PPBillPacket & rS, const TSVector <CliStatus> * pCliStatusList, PPIDArray * pGoodsIdList)
{
	Z();
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	ID = rS.Rec.ID;
	(Code = rS.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8);
	{
		const ObjTagItem * p_tagitem = rS.BTagL.GetItem(PPTAG_BILL_CREATEDTM);
		CALLPTRMEMB(p_tagitem, GetTimestamp(&CreationTime));
	}
	Time.d = rS.Rec.Dt;
	SvcOpType = rS.OpTypeID; // @v11.7.2
	SvcOpID = rS.Rec.OpID;
	AgentID = rS.Ext.AgentID;
	ClientID = rS.Rec.Object;
	DlvrLocID = rS.GetDlvrAddrID();
	DueTime.d = rS.Rec.DueDate;
	Amount = rS.Rec.Amount;
	if(!rS.GetGuid(Uuid)) {
		const S_GUID new_doc_uuid(SCtrGenerate_);
		if(p_bobj->PutGuid(rS.Rec.ID, &new_doc_uuid, 1)) {
			Uuid = new_doc_uuid;
		}
	}
	StatusSurrId = SVector::GetCount(pCliStatusList) ? CliStatus::GetFromBill(*pCliStatusList, rS) : 0; // @v11.5.1
	(Memo = rS.SMemo).Transf(CTRANSF_INNER_TO_UTF8);
	if(rS.OpTypeID == PPOPT_INVENTORY) {
		for(uint i = 0; i < rS.InvList.getCount(); i++) {
			const InventoryTbl::Rec & r_inv_item = rS.InvList.at(i);
			PPStyloQInterchange::Document::__TransferItem * p_doc_ti = TiList.CreateNewItem();
			THROW_SL(p_doc_ti);
			const PPID goods_id = labs(r_inv_item.GoodsID);
			p_doc_ti->RowIdx = r_inv_item.OprNo;
			p_doc_ti->GoodsID = goods_id;
			if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
				p_doc_ti->UnitID = goods_rec.UnitID;
			}
			p_doc_ti->Set.Qtty = fabs(r_inv_item.StockRest); // @v11.7.2 fabs
			p_doc_ti->Set.Price = r_inv_item.StockPrice;
			CALLPTRMEMB(pGoodsIdList, add(goods_id));
		}
	}
	else {
		PPLotExtCodeContainer::MarkSet lotxcode_set;
		for(uint i = 0; i < rS.GetTCount(); i++) {
			const PPTransferItem & r_ti = rS.ConstTI(i);
			PPStyloQInterchange::Document::__TransferItem * p_doc_ti = TiList.CreateNewItem();
			THROW_SL(p_doc_ti);
			const PPID goods_id = labs(r_ti.GoodsID);
			p_doc_ti->RowIdx = r_ti.RByBill;
			p_doc_ti->GoodsID = goods_id;
			if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
				p_doc_ti->UnitID = goods_rec.UnitID;
			}
			p_doc_ti->Set.Qtty = fabs(r_ti.Qtty()); // @v11.7.2 fabs
			p_doc_ti->Set.Price = r_ti.Price;
			p_doc_ti->Set.Discount = r_ti.Discount;
			if(true/*param.ActionFlags & (StyloQIncomingListParam::actionDocSettingMarks|StyloQIncomingListParam::actionDocAcceptanceMarks)*/) {
				// Здесь передаем марки, ассоциированные со строкой
				rS.XcL.Get(i+1, 0, lotxcode_set);
				for(uint xci = 0; xci < lotxcode_set.GetCount(); xci++) {
					PPLotExtCodeContainer::MarkSet::Entry xce;
					if(lotxcode_set.GetByIdx(xci, xce)) {
						PPStyloQInterchange::Document::LotExtCode lec;
						lec.BoxRefN = xce.BoxID;
						STRNSCPY(lec.Code, xce.Num);
						lec.Flags = xce.Flags;
						p_doc_ti->XcL.insert(&lec);
					}
				}
			}
			CALLPTRMEMB(pGoodsIdList, add(goods_id));
		}
		if(true/*param.ActionFlags & StyloQIncomingListParam::actionDocAcceptanceMarks*/) {
			// Здесь передаем валидирующие марки
			rS._VXcL.Get(-1, 0, lotxcode_set);
			for(uint xci = 0; xci < lotxcode_set.GetCount(); xci++) {
				PPLotExtCodeContainer::MarkSet::Entry xce;
				if(lotxcode_set.GetByIdx(xci, xce)) {
					PPStyloQInterchange::Document::LotExtCode lec;
					lec.BoxRefN = xce.BoxID;
					STRNSCPY(lec.Code, xce.Num);
					lec.Flags = xce.Flags;
					VXcL.insert(&lec);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::Debug_Command(const StyloQCommandList::Item * pCmd) // @debug
{
	int    ok = 1;
	if(pCmd) {
		SString result;
		SString decl;
		StyloQCore::StoragePacket fake_cli_pack;
		SGeoPosLL fake_geo_pos;
		switch(pCmd->BaseCmdId) {
			case StyloQCommandList::sqbcRsrvOrderPrereq:
				PPWait(1);
				THROW(ProcessCommand_RsrvOrderPrereq(*pCmd, fake_cli_pack, result, decl, prccmdfDebugOutput));
				break;
			case StyloQCommandList::sqbcRsrvAttendancePrereq:
				PPWait(1);
				THROW(ProcessCommand_RsrvAttendancePrereq(*pCmd, fake_cli_pack, fake_geo_pos, result, decl, prccmdfDebugOutput));
				break;
			case StyloQCommandList::sqbcRsrvPushIndexContent:
				{
					StyloQCore::StoragePacket own_pack;
					S_GUID doc_uuid;
					PPWait(1);
					THROW(GetOwnPeerEntry(&own_pack) > 0);
					THROW(MakeIndexingRequestCommand(&own_pack, pCmd, 3600*24, doc_uuid, result, 0));
				}
				break;
			case StyloQCommandList::sqbcReport:
				PPWait(1);
				THROW(ProcessCommand_Report(*pCmd, fake_cli_pack, fake_geo_pos, result, decl, prccmdfDebugOutput));
				break;
			case StyloQCommandList::sqbcRsrvIndoorSvcPrereq: // @v11.4.4
				THROW(ProcessCommand_RsrvIndoorSvcPrereq(*pCmd, fake_cli_pack, result, decl, prccmdfDebugOutput));
				break;
			case StyloQCommandList::sqbcGoodsInfo: // @v11.4.4
				// @todo
				break;
			case StyloQCommandList::sqbcIncomingListOrder: // @v11.4.7
				THROW(ProcessCommand_IncomingListOrder(*pCmd, fake_cli_pack, 0, result, decl, prccmdfDebugOutput));
				break;
			case StyloQCommandList::sqbcIncomingListCCheck: // @v11.4.7
				THROW(ProcessCommand_IncomingListCCheck(*pCmd, fake_cli_pack, 0, result, decl, prccmdfDebugOutput));
				break;
			case StyloQCommandList::sqbcIncomingListTSess: // @v11.4.7
				THROW(ProcessCommand_IncomingListTSess(*pCmd, fake_cli_pack, 0, result, decl, prccmdfDebugOutput)); // @v11.6.5
				break;
			case StyloQCommandList::sqbcIncomingListTodo: // @v11.4.7
				// @todo
				break;
			case StyloQCommandList::sqbcDebtList: // @v11.5.4
				// @todo
				break;
		}
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

int PPStyloQInterchange::FetchProcessorFromClientPacket(const StyloQCore::StoragePacket & rCliPack, PPID * pPrcID, bool logResult)
{
	int    ok = -1;
	PPID   prc_id = 0;
	PPObjProcessor prc_obj;
	ProcessorTbl::Rec prc_rec;
	if(rCliPack.Rec.LinkObjType == PPOBJ_PERSON) {
		if(rCliPack.Rec.LinkObjID) {
			if(prc_obj.SearchByLinkObj(PPOBJ_PERSON, rCliPack.Rec.LinkObjID, 0, &prc_rec) > 0) {
				prc_id = prc_rec.ID;
				ok = 1;
			}
		}
	}
	else if(rCliPack.Rec.LinkObjType == PPOBJ_USR) {
		PPSecur sec_rec;
		PPObjSecur sec_obj(PPOBJ_USR, 0);
		if(sec_obj.Search(rCliPack.Rec.LinkObjID, &sec_rec) > 0) {
			if(sec_rec.PersonID) {
				if(prc_obj.SearchByLinkObj(PPOBJ_PERSON, rCliPack.Rec.LinkObjID, 0, &prc_rec) > 0) {
					prc_id = prc_rec.ID;
					ok = 1;
				}
			}
		}
	}
	else if(rCliPack.Rec.LinkObjType == PPOBJ_PROCESSOR) {
		prc_id = rCliPack.Rec.LinkObjID;
		if(prc_obj.Fetch(prc_id, &prc_rec) > 0) {
			ok = 1;
		}
	}
	ASSIGN_PTR(pPrcID, prc_id);
	return ok;
}

int PPStyloQInterchange::FetchPersonFromClientPacket(const StyloQCore::StoragePacket & rCliPack, PPID * pPersonID, bool logResult)
{
	int    ok = -1;
	bool   auto_matching = false;
	PPID   person_id = 0;
	SString fmt_buf;
	SString msg_buf;
	SString temp_buf;
	PersonTbl::Rec psn_rec;
	if(rCliPack.Rec.LinkObjType == PPOBJ_PERSON) {
		person_id = rCliPack.Rec.LinkObjID;
		// @v11.5.7 {
		if(rCliPack.Rec.Flags & StyloQCore::styloqfAutoObjMatching)
			auto_matching = true;
		// } @v11.5.7 
		if(logResult) {
			//PPTXT_STQ_MAPCLI2PSN_DIRECT          "Идентифицирована персоналия, непосредственно ассоциированная с клиентом Stylo-Q: %s"
			PPLoadText(PPTXT_STQ_MAPCLI2PSN_DIRECT, fmt_buf);
			temp_buf.Z().EncodeMime64(rCliPack.Rec.BI, sizeof(rCliPack.Rec.BI)).Space().Cat("-->").Space().Cat(person_id);
			PPLogMessage(PPFILNAM_INFO_LOG, msg_buf.Printf(fmt_buf, temp_buf.cptr()), LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
		}
	}
	else if(rCliPack.Rec.LinkObjType == PPOBJ_USR) {
		PPSecur sec_rec;
		PPObjSecur sec_obj(PPOBJ_USR, 0);
		if(sec_obj.Search(rCliPack.Rec.LinkObjID, &sec_rec) > 0) {
			if(sec_rec.PersonID) {
				person_id = sec_rec.PersonID;
				if(logResult) {
					//PPTXT_STQ_MAPCLI2PSN_USER            "Идентифицирована персоналия, ассоциированная с клиентом Stylo-Q как пользователь: %s"
					PPLoadText(PPTXT_STQ_MAPCLI2PSN_USER, fmt_buf);
					temp_buf.Z().EncodeMime64(rCliPack.Rec.BI, sizeof(rCliPack.Rec.BI)).Space().Cat("-->").Space().Cat(person_id);
					PPLogMessage(PPFILNAM_INFO_LOG, msg_buf.Printf(fmt_buf, temp_buf.cptr()), LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
				}
			}
			else {
				//PPERR_STQ_MAPCLI2PSN_FAIL_USER       "Не удалось идентифицировать персоналию, ассоциированную с клиентом Stylo-Q сопоставленным с пользователем: %s"
				PPLoadString(PPSTR_ERROR, PPERR_STQ_MAPCLI2PSN_FAIL_USER, fmt_buf);
				temp_buf.Z().EncodeMime64(rCliPack.Rec.BI, sizeof(rCliPack.Rec.BI)).Space().Cat("-->").Space().Cat(sec_rec.ID);
				PPSetError(PPERR_STQ_MAPCLI2PSN_FAIL_USER, temp_buf);
				if(logResult) {
					msg_buf.Printf(fmt_buf, temp_buf.cptr());
					PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
				}
			}
		}
	}
	else if(rCliPack.Rec.LinkObjType == PPOBJ_PROCESSOR) { // @v11.6.5
		PPObjProcessor prc_obj;
		ProcessorTbl::Rec prc_rec;
		if(prc_obj.Fetch(rCliPack.Rec.LinkObjID, &prc_rec) > 0) {
			if(prc_rec.LinkObjType == PPOBJ_PERSON) {
				person_id = prc_rec.LinkObjID;
				if(logResult) {
				}
			}
			else {
				; // @todo @logmsg
			}
		}		
	}
	else {
		//PPERR_STQ_MAPCLI2PSN_FAIL_UNMATCHED  "Не удалось идентифицировать персоналию, ассоциированную с клиентом Stylo-Q, поскольку он не сопоставлен с объектами, допускающими такую идентификацию: %s"
		PPLoadString(PPSTR_ERROR, PPERR_STQ_MAPCLI2PSN_FAIL_UNMATCHED, fmt_buf);
		temp_buf.Z().EncodeMime64(rCliPack.Rec.BI, sizeof(rCliPack.Rec.BI));
		PPSetError(PPERR_STQ_MAPCLI2PSN_FAIL_UNMATCHED, temp_buf);
		if(logResult) {
			msg_buf.Printf(fmt_buf, temp_buf.cptr());
			PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
		}
	}
	ASSIGN_PTR(pPersonID, person_id);
	if(person_id)
		ok = auto_matching ? 2 : 1;
	return ok;
}

int PPStyloQInterchange::AcceptStyloQClientAsPerson(const StyloQCore::StoragePacket & rCliPack, PPID personKindID, PPID * pPersonID, int use_ta)
{
	int    ok = -1;
	SString temp_buf;
	PPID   person_id = 0;
	if(personKindID) {
		PPObjPerson psn_obj;
		PPPersonPacket psn_pack;
		bool is_person_associated_with_cli = false;
		bool can_modify_psn_pack = true;
		if(FetchPersonFromClientPacket(rCliPack, &person_id, true/*logResult*/) > 0) {
			if(psn_obj.GetPacket(person_id, &psn_pack, 0) > 0) {
				is_person_associated_with_cli = true;
				// @v11.6.6 {
				{
					SBinaryChunk bch_cli_cfg;
					if(rCliPack.Pool.Get(SSecretTagPool::tagPrivateConfig, &bch_cli_cfg)) {
						StyloQConfig cli_cfg_pack;
						if(cli_cfg_pack.FromJson(bch_cli_cfg.ToRawStr(temp_buf)) && cli_cfg_pack.Get(StyloQConfig::tagCliFlags, temp_buf)) {
							long cli_match_flags = temp_buf.ToLong();
							if(cli_match_flags & StyloQConfig::clifDisableFaceSelfModifying)
								can_modify_psn_pack = false;	
						}
					}
				}
				// } @v11.6.6 
				/*if(psn_obj.P_Tbl->IsBelongToKind(person_id, personKindID) > 0) {
					ok = 1;
				}
				else {
					THROW(psn_obj.P_Tbl->AddKind(person_id, personKindID, use_ta));
					ok = 1;
				}*/
			}
			else
				person_id = 0;
		}
		{
			bool face_is_valid = false;
			StyloQFace cli_face;
			SBinaryChunk face_bytes;
			if(rCliPack.Pool.Get(SSecretTagPool::tagFace, &face_bytes)) {
				if(face_bytes.Len()) {
					if(cli_face.FromJson(face_bytes.ToRawStr(temp_buf)))
						face_is_valid = true;
				}
			}
			{
				RegisterTbl::Rec new_reg_rec;
				if(face_is_valid) {
					STokenRecognizer tr;
					SNaturalTokenArray nta;
					SString nbuf; // Буфер нормализованного значения //
					if(can_modify_psn_pack) { // @v11.6.6
						if(cli_face.Get(StyloQFace::tagCommonName, 0/*lang*/, temp_buf)) {
							if(can_modify_psn_pack)
								STRNSCPY(psn_pack.Rec.Name, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
						}
						else {
							SString compound_name;
							if(cli_face.Get(StyloQFace::tagSurName, 0/*lang*/, temp_buf))
								compound_name.CatDivIfNotEmpty(' ', 0).Cat(temp_buf);
							if(cli_face.Get(StyloQFace::tagName, 0/*lang*/, temp_buf))
								compound_name.CatDivIfNotEmpty(' ', 0).Cat(temp_buf);
							if(cli_face.Get(StyloQFace::tagPatronymic, 0/*lang*/, temp_buf))
								compound_name.CatDivIfNotEmpty(' ', 0).Cat(temp_buf);
							if(compound_name.NotEmptyS())
								STRNSCPY(psn_pack.Rec.Name, compound_name.Transf(CTRANSF_UTF8_TO_INNER));
						}
					}
					if(cli_face.Get(StyloQFace::tagPhone, 0/*lang*/, temp_buf)) {
						tr.Run(temp_buf.ucptr(), temp_buf.Len(), nta, 0);
						if(nta.Has(SNTOK_PHONE)) {
							temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
							PPEAddr::Phone::NormalizeStr(temp_buf, 0, nbuf);
							if(!psn_pack.ELA.SearchByText(nbuf, 0))
								psn_pack.ELA.AddItem(PPELK_MOBILE, nbuf);
						}
					}
					if(cli_face.Get(StyloQFace::tagEMail, 0/*lang*/, temp_buf)) {
						tr.Run(temp_buf.ucptr(), temp_buf.Len(), nta, 0);
						temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						if(nta.Has(SNTOK_EMAIL) && !psn_pack.ELA.SearchByText(temp_buf, 0)) {
							psn_pack.ELA.AddItem(PPELK_EMAIL, temp_buf);
						}
					}
					if(cli_face.Get(StyloQFace::tagRuINN, 0/*lang*/, temp_buf)) {
						tr.Run(temp_buf.ucptr(), temp_buf.Len(), nta, 0);
						if(nta.Has(SNTOK_RU_INN)) {
							temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
							PPObjRegister::InitPacket(&new_reg_rec, PPREGT_TPID, PPObjID(PPOBJ_PERSON, 0), temp_buf);
							uint rpos = 0;
							if(psn_pack.Regs.GetRegister(PPREGT_TPID, &rpos, 0) > 0)
								;
							else
								psn_pack.Regs.insert(&new_reg_rec);
						}
					}
					if(cli_face.Get(StyloQFace::tagRuKPP, 0/*lang*/, temp_buf)) {
						tr.Run(temp_buf.ucptr(), temp_buf.Len(), nta, 0);
						if(nta.Has(SNTOK_RU_KPP)) {
							temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
							PPObjRegister::InitPacket(&new_reg_rec, PPREGT_KPP, PPObjID(PPOBJ_PERSON, 0), temp_buf);
							uint rpos = 0;
							if(psn_pack.Regs.GetRegister(PPREGT_KPP, &rpos, 0) > 0)
								;
							else
								psn_pack.Regs.insert(&new_reg_rec);
						}
					}
					if(cli_face.Get(StyloQFace::tagGLN, 0/*lang*/, temp_buf)) {
						tr.Run(temp_buf.ucptr(), temp_buf.Len(), nta, 0);
						if(nta.Has(SNTOK_EAN13)) {
							temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
							PPObjRegister::InitPacket(&new_reg_rec, PPREGT_GLN, PPObjID(PPOBJ_PERSON, 0), temp_buf);
							uint rpos = 0;
							if(psn_pack.Regs.GetRegister(PPREGT_GLN, &rpos, 0) > 0)
								;
							else
								psn_pack.Regs.insert(&new_reg_rec);
						}
					}
					{
						//tagCountryIsoSymb  =  9, // countryisosymb : string
						//tagCountryIsoCode  = 10, // countryisocode : string (numeric)
						//tagCountryName     = 11, // country : string with optional language shifted on 16 bits left
						//tagZIP             = 12, // zip : string
						//tagCityName        = 13, // city : string with optional language shifted on 16 bits left
						//tagStreet          = 14, // street : string with optional language shifted on 16 bits left
						//tagAddress         = 15, // address : string with optional language shifted on 16 bits left
						PPLocationPacket addr_pack;
						if(cli_face.Get(StyloQFace::tagCityName, 0/*lang*/, temp_buf)) {
							temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
							//STRNSCPY(addr_pack.Rec.
						}
					}
				}
				if(isempty(psn_pack.Rec.Name)) {
					P_T->MakeTextHashForCounterparty(rCliPack, 10, temp_buf);
					STRNSCPY(psn_pack.Rec.Name, temp_buf);					
				}
				psn_pack.Kinds.addUnique(personKindID);
				//assert(person_id == 0);
				//person_id = 0;
				{
					const bool is_new = (person_id == 0);
					PPTransaction tra(use_ta);
					THROW(tra);
					{
						PPID   temp_id = rCliPack.Rec.ID;
						assert(temp_id); // Если rCliPack.Rec.ID нулевой, то что-то выше по стеку пошло не так - нам подсунули пакет клиента не из базы данных!
						if(temp_id) {
							StyloQCore::StoragePacket cli_pack_to_update;
							THROW(P_T->GetPeerEntry(rCliPack.Rec.ID, &cli_pack_to_update) > 0);
							if(!is_new) {
								//clifDisableFaceSelfModifying
							}
							THROW(psn_obj.PutPacket(&person_id, &psn_pack, 0));
							if(!is_person_associated_with_cli) {
								if(rCliPack.Rec.LinkObjType == 0 && rCliPack.Rec.LinkObjID == 0) {
									cli_pack_to_update.Rec.LinkObjType = PPOBJ_PERSON;
									cli_pack_to_update.Rec.LinkObjID = person_id;
									// @v11.5.11 {
									if(is_new)
										cli_pack_to_update.Rec.Flags |= StyloQCore::styloqfAutoObjMatching;
									// } @v11.5.11 
									THROW(P_T->PutPeerEntry(&temp_id, &cli_pack_to_update, 0));
								}
								else {
									; // @todo Здесь надо что-то в лог написать!
								}
							}
						}
					}
					THROW(tra.Commit())
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pPersonID, person_id);
	return ok;
}

/*static*/int PPStyloQInterchange::GetBlobStoragePath(SString & rBuf)
{	
	rBuf.Z();
	int    ok = 0;
	PPGetPath(PPPATH_WORKSPACE, rBuf);
	if(rBuf.NotEmpty() && IsDirectory(rBuf)) {
		rBuf.SetLastSlash().Cat("blob");
		ok = 1;
	}
	else
		rBuf.Z();
	return ok;
}

PPStyloQInterchange::Stq_ReqBlobInfoEntry::Stq_ReqBlobInfoEntry() : StyloQBlobInfo(), Missing(false), RepDiffHash(false)
{
	memzero(Reserve, sizeof(Reserve));
}

PPStyloQInterchange::Stq_ReqBlobInfoEntry & PPStyloQInterchange::Stq_ReqBlobInfoEntry::Z()
{
	StyloQBlobInfo::Z();
	Missing = false;
	RepDiffHash = false;
	memzero(Reserve, sizeof(Reserve));
	return *this;
}

PPStyloQInterchange::Stq_ReqBlobInfoList::Stq_ReqBlobInfoList() : TSCollection <Stq_ReqBlobInfoEntry>()
{
}

bool PPStyloQInterchange::Stq_ReqBlobInfoList::SearchSignature(const char * pSignature, uint * pPos) const
{
	bool    ok = false;
	if(!isempty(pSignature)) {
		for(uint i = 0; !ok && i < getCount(); i++) {
			const Stq_ReqBlobInfoEntry * p_item = at(i);
			if(p_item && p_item->Signature == pSignature) {
				ASSIGN_PTR(pPos, i);
				ok = true;
			}
		}
	}
	return ok;
}

SJson * PPStyloQInterchange::Stq_ReqBlobInfoList::MakeRequestInfoListQuery() const
{
	SJson * p_result = 0;
	if(getCount()) {
		SString temp_buf;
		p_result = SJson::CreateObj();
		p_result->InsertString("cmd", "requestblobinfolist");
		SJson * p_js_list = SJson::CreateArr();
		for(uint i = 0; i < getCount(); i++) {
			const Stq_ReqBlobInfoEntry * p_req_blob_entry = at(i);
			assert(p_req_blob_entry);
			if(p_req_blob_entry) {
				SJson * p_js_blob = SJson::CreateObj();
				p_js_blob->InsertString("signature", (temp_buf = p_req_blob_entry->Signature).Escape());
				assert(p_req_blob_entry->HashAlg);
				SlHash::GetAlgorithmSymb(p_req_blob_entry->HashAlg, temp_buf);
				p_js_blob->InsertString("hashalg", temp_buf.Escape());
				p_js_blob->InsertString("hash", p_req_blob_entry->Hash.Mime64(temp_buf).Escape());
				p_js_list->InsertChild(p_js_blob);
			}
		}
		p_result->Insert("list", p_js_list);
	}
	return p_result;
}

int PPStyloQInterchange::StoreOidListWithBlob(const PPObjIDArray & rList)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPObjIDArray _list;
	bool   do_update = false;
	THROW(p_ref);
	THROW(GetOidListWithBlob(_list));
	for(uint i = 0; i < rList.getCount(); i++) {
		const PPObjID oid = rList.at(i);
		if(oid.Obj > 0 && oid.Id > 0) {
			if(!_list.Search(oid, 0)) {
				THROW(_list.Add(oid.Obj, oid.Id));
				do_update = true;
			}
		}
	}
	if(do_update) {
		SBuffer sbuf;
		if(_list.getCount()) {
			const bool use_compression = ((_list.getCount() * _list.getItemSize()) > 512) ? true : false;
			if(use_compression) {
				uint8 cs[32];
				size_t cs_size = SSerializeContext::GetCompressPrefix(cs);
				SCompressor compr(SCompressor::tZLib);
				SBuffer _compr_buf;
				THROW_SL(sbuf.Write(cs, cs_size));
				THROW_SL(_compr_buf.Write(&_list, SBuffer::ffAryCount32));
				THROW_SL(compr.CompressBlock(_compr_buf.GetBuf(0), _compr_buf.GetAvailableSize(), sbuf, 0, 0));
			}
			else {
				THROW_SL(sbuf.Write(&_list, SBuffer::ffAryCount32));
			}
		}
		THROW(p_ref->PutPropSBuffer(PPOBJ_CURRENTSTATE, 1, CSTATEPRP_STQOBJBLOBLIST, sbuf, 1));
		ok = 1;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::GetOidListWithBlob(PPObjIDArray & rList)
{
	rList.clear();
	int    ok = 1;
	Reference * p_ref = PPRef;
	SBuffer sbuf;
	THROW(p_ref);
	if(p_ref->GetPropSBuffer(PPOBJ_CURRENTSTATE, 1, CSTATEPRP_STQOBJBLOBLIST, sbuf) > 0) {
		const size_t actual_size = sbuf.GetAvailableSize();
		const size_t cs_size = SSerializeContext::GetCompressPrefix(0);
		if(actual_size > cs_size && SSerializeContext::IsCompressPrefix(sbuf.GetBuf(sbuf.GetRdOffs()))) {
			SCompressor compr(SCompressor::tZLib);
			SBuffer dbuf;
			int  inflr = compr.DecompressBlock(sbuf.GetBuf(sbuf.GetRdOffs()+cs_size), actual_size-cs_size, dbuf);
			THROW(inflr);
			THROW_SL(dbuf.Read(&rList, SBuffer::ffAryCount32));
		}
		else {
			THROW_SL(sbuf.Read(&rList, SBuffer::ffAryCount32));
		}
	}
	else
		ok = -1;
	CATCH
		rList.clear();
		ok = 0;
	ENDCATCH
	return ok;
}

/*static*/const uint PPStyloQInterchange::InnerBlobN_Face = 10000U;

bool PPStyloQInterchange::GetBlobInfo(const SBinaryChunk & rOwnIdent, PPObjID oid, uint blobN, uint flags, StyloQBlobInfo & rInfo, SBinaryChunk * pBlobBuf) const
{
	rInfo.Z();
	bool   ok = false;
	PROFILE_START
	SString temp_buf;
	if(oid.Obj == PPOBJ_STYLOQBINDERY) {
		StyloQCore::StoragePacket pack;
		if(P_T->GetPeerEntry(oid.Id, &pack) > 0) {
			int face_tag_id = 0;
			if(oneof2(pack.Rec.Kind, StyloQCore::kClient, StyloQCore::kForeignService))
				face_tag_id = SSecretTagPool::tagFace;
			else
				face_tag_id = SSecretTagPool::tagSelfyFace;
			StyloQFace face_pack;
			if(pack.GetFace(face_tag_id, face_pack) > 0) {
				int    img_format = 0;
				if(face_pack.Get(StyloQFace::tagImage, 0, temp_buf) > 0) {
					SString fmt_buf;
					SString base64_buf;
					if(temp_buf.Divide(':', fmt_buf, base64_buf) > 0) {
						fmt_buf.Strip();
						SFileFormat ff;
						if(ff.IdentifyMime(fmt_buf)) {
							img_format = ff;
							SBinaryChunk bc_img;
							if(bc_img.FromMime64(base64_buf)) {
								rInfo.Oid = oid;
								rInfo.BlobN = PPStyloQInterchange::InnerBlobN_Face;
								rInfo.Ff = ff;
								rInfo.HashAlg = SHASHF_SHA256;
								if(!(flags & gbifSignatureOnly))
									SlHash::CalcBufferHash(rInfo.HashAlg, bc_img.Ptr(), bc_img.Len(), rInfo.Hash);
								PPStyloQInterchange::MakeBlobSignature(rOwnIdent, oid, rInfo.BlobN, rInfo.Signature);
								ASSIGN_PTR(pBlobBuf, bc_img);
								ok = true;
							}
						}
					}
				}
			}
		}
	}
	else {
		ObjLinkFiles olf;
		olf.Init(oid.Obj);
		olf.Load(oid.Id, 0L);
		if(olf.GetCount()) {
			uint idx = 0;
			ObjLinkFiles::Fns fns;
			int atr = 0; // Результат вызова olf.At(idx)
			if(blobN == 0)
				atr = olf.At(idx, temp_buf);
			else {
				for(uint i = 0; !idx && i < olf.GetCount(); i++) {
					if(olf.At(i, temp_buf) > 0) {
						const uint n = (ObjLinkFiles::SplitInnerFileName(temp_buf, &fns) && fns.Cntr > 0) ? static_cast<uint>(fns.Cntr) : 0;
						if(n == blobN)
							idx = n;
					}
				}
				if(idx)
					atr = olf.At(idx, temp_buf);
			}
			if(atr > 0) {
				SPathStruc::NormalizePath(temp_buf, SPathStruc::npfCompensateDotDot, rInfo.SrcPath);
				const int fir = rInfo.Ff.Identify(rInfo.SrcPath, 0);
				if(oneof2(fir, 2, 3) && SImageBuffer::IsSupportedFormat(rInfo.Ff)) { // Принимаем только идентификацию по сигнатуре
					SFile f_in(rInfo.SrcPath, SFile::mRead|SFile::mBinary);
					rInfo.HashAlg = SHASHF_SHA256;
					if(f_in.IsValid() && ((flags & gbifSignatureOnly) || f_in.CalcHash(0, rInfo.HashAlg, rInfo.Hash))) {
						rInfo.Oid = oid;
						rInfo.BlobN = (ObjLinkFiles::SplitInnerFileName(rInfo.SrcPath, &fns) && fns.Cntr > 0) ? static_cast<uint32>(fns.Cntr) : 0;
						PPStyloQInterchange::MakeBlobSignature(rOwnIdent, oid, rInfo.BlobN, rInfo.Signature);
						ok = true;
						if(pBlobBuf) {
							STempBuffer blob_buf(SMEGABYTE(1));
							size_t actual_size = 0;
							if(f_in.ReadAll(blob_buf, 0, &actual_size)) {
								pBlobBuf->Put(blob_buf, actual_size);
							}
							else {
								ok = LOGIC(PPSetErrorSLib());
							}
						}
					}
				}
			}
		}
	}
	if(!ok)
		rInfo.Z();
	PROFILE_END
	return ok;
}

int PPStyloQInterchange::AddImgBlobToReqBlobInfoList(const SBinaryChunk & rOwnIdent, PPObjID oid, Stq_ReqBlobInfoList & rList)
{
	int    ok = -1;
	StyloQBlobInfo bi;
	if(GetBlobInfo(rOwnIdent, oid, 0, 0/*flags*/, bi, 0)) {
		Stq_ReqBlobInfoEntry * p_new_entry = rList.CreateNewItem();
		*static_cast <StyloQBlobInfo *>(p_new_entry) = bi;
		ok = 1;
	}
	return ok;
}

int PPStyloQInterchange::GetContextualOpAndLocForOrder(const StyloQDocumentPrereqParam & rParam, PPID * pOpID, PPIDArray * pLocList)
{
	int    ok = -1;
	PPID   op_id = 0;
	PPOprKind op_rec;
	CALLPTRMEMB(pLocList, Z());
	if(rParam.PalmID) {
		PPObjStyloPalm stp_obj;
		PPStyloPalmPacket stp_pack;
		if(stp_obj.GetPacket(rParam.PalmID, &stp_pack) > 0) {
			op_id = stp_pack.Rec.OrderOpID;
			if(op_id && GetOpData(op_id, &op_rec) > 0 && op_rec.OpTypeID == PPOPT_GOODSORDER) {
				;
			}
			else {
				PPAlbatrossConfig acfg;
				DS.FetchAlbatrosConfig(&acfg);
				op_id = acfg.Hdr.OpID;
				if(op_id && GetOpData(op_id, &op_rec) > 0 && op_rec.OpTypeID == PPOPT_GOODSORDER) {
					ok = 2;
				}
				else
					op_id = 0;
			}
			if(pLocList)
				stp_pack.LocList.Get(*pLocList);
			ok = 1;
		}
	}
	else {
		op_id = rParam.OpID;
		CALLPTRMEMB(pLocList, addnz(rParam.LocID));
		if(op_id && GetOpData(op_id, &op_rec) > 0) {
			ok = 1;
		}
	}
	ASSIGN_PTR(pOpID, op_id);
	return ok;
}

int PPStyloQInterchange::LoadViewSymbList()
{
	int    ok = -1;
	if(!(State & stViewSymbListLoaded)) {
		if(PPView::GetDescriptionList(true, &ViewSymbList, &ViewDescrList) > 0) {
			State |= stViewSymbListLoaded;
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

bool PPStyloQInterchange::AmIMediator(const char * pCommand)
{
	bool   ok = true;
	StyloQCore::StoragePacket own_pack;
	SBinaryChunk bc_cfg;
	SString temp_buf;
	StyloQConfig own_cfg;
	THROW(GetOwnPeerEntry(&own_pack) > 0);
	THROW_PP_S(own_pack.Pool.Get(SSecretTagPool::tagConfig, &bc_cfg), PPERR_SQ_CMDFAULT_IMNOTMEDIATOR, pCommand);
	THROW(own_cfg.FromJson(bc_cfg.ToRawStr(temp_buf)));
	THROW_PP_S(own_cfg.GetFeatures() & StyloQConfig::featrfMediator, PPERR_SQ_CMDFAULT_IMNOTMEDIATOR, pCommand);
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::GetAndStoreClientsFace(const StyloQProtocol & rRcvPack, const SBinaryChunk & rCliIdent)
{
	int    ok = -1;
	StyloQCore::StoragePacket cli_pack;
	SBinaryChunk bc_face;
	if(rRcvPack.P.Get(SSecretTagPool::tagFace, &bc_face)) {
		SString temp_buf;
		SString msg_buf;
		PPLoadText(PPTXT_SQ_LOG_CLIFACERCVD, msg_buf);
		msg_buf.CatDiv(':', 2).Cat(rCliIdent.Mime64(temp_buf));
		if(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0) {
			StyloQFace other_face;
			assert(bc_face.Len()); // rRcvPack.P.Get гарантирует!
			if(bc_face.Len() && other_face.FromJson(bc_face.ToRawStr(temp_buf))) {
				// Необходимо модифицировать оригинальный face установкой
				// фактического времени истечения срока действия //
				other_face.Get(StyloQFace::tagExpiryPeriodSec, 0, temp_buf);
				const int64 ees = EvaluateExpiryTime(temp_buf.ToLong());
				if(ees > 0)
					other_face.Set(StyloQFace::tagExpiryEpochSec, 0, temp_buf.Z().Cat(ees));
				if(other_face.ToJson(false, temp_buf)) {
					bc_face.Put(temp_buf.cptr(), temp_buf.Len());
					cli_pack.Pool.Put(SSecretTagPool::tagFace, bc_face);
					PPID   temp_id = cli_pack.Rec.ID;
					if(P_T->PutPeerEntry(&temp_id, &cli_pack, 1)) {
						rCliIdent.Mime64(temp_buf);
						msg_buf.Space().Cat("saved");
						ok = 1;
					}
				}
			}					
		}
		if(ok <= 0) {
			msg_buf.Space().Cat("not saved");
			PPGetLastErrorMessage(1, temp_buf);
			msg_buf.CatDiv('-', 1).Cat(temp_buf);
		}
		PPLogMessage(PPFILNAM_SERVER_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO);
	}
	return ok;
}

int PPStyloQInterchange::IntermediateReply(int waitPeriodMs, int pollIntervalMs, 
	const SBinaryChunk * pSessSecret, ProcessCmdCallbackProc intermediateReplyProc, void * pIntermediateReplyExtra)
{
	int    ok = 1;
	if(intermediateReplyProc && waitPeriodMs > 0) { 
		StyloQProtocol irp;
		irp.StartWriting(PPSCMD_SQ_COMMAND, StyloQProtocol::psubtypeIntermediateReply);
		{
			SBinaryChunk cmd_bch;
			SString cmd_buf;
			SJson ir_js(SJson::tOBJECT);
			ir_js.InsertInt("waitms", 5*60*1000);
			ir_js.InsertInt("pollintervalms", 1000);
			ir_js.ToStr(cmd_buf);
			cmd_bch.Put(cmd_buf.cptr(), cmd_buf.Len());
			irp.P.Put(SSecretTagPool::tagRawData, cmd_bch, 0);
		}
		irp.FinishWriting(pSessSecret);
		ok = intermediateReplyProc(irp, pIntermediateReplyExtra);
	}
	else
		ok = -1;
	return ok;
}

SJson * PPStyloQInterchange::ReplyGoodsInfo(const SBinaryChunk & rOwnIdent, const SBinaryChunk & rCliIdent, PPID goodsID, const char * pBarcode)
{
	SJson * p_result = 0;
	SJson * p_js_detail = 0;
	StyloQCommandList * p_target_cmd_list = 0;
	{
		DbProvider * p_dict = CurDict;
		SString db_symb;
		if(!p_dict || !p_dict->GetDbSymb(db_symb)) {
			PPSetError(PPERR_SESSNAUTH);
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
			PPSetError(PPERR_SQ_CMDSETLOADINGFAULT);
		}
		else {
			StyloQCommandList full_cmd_list;
			StyloQCore::StoragePacket cli_pack;
			SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack); // cli_pack может быть пустым!
			if(StyloQCommandList::GetFullList(db_symb, full_cmd_list)) {
				const PPObjID oid(cli_pack.Rec.LinkObjType, cli_pack.Rec.LinkObjID);
				p_target_cmd_list = full_cmd_list.CreateSubListByContext(oid, StyloQCommandList::sqbcGoodsInfo, false);
				assert(!p_target_cmd_list || p_target_cmd_list->GetCount() > 0);
			}
			p_js_detail = ProcessCommand_GetGoodsInfo(rOwnIdent, cli_pack, (p_target_cmd_list ? p_target_cmd_list->Get(0) : 0), goodsID, pBarcode);
		}
	}
	if(p_js_detail) {
		//SJson * p_js = MakeObjJson_Goods(own_ident, goods_pack, 0, 0, 0, 0);
		//THROW(p_js);
		p_result = SJson::CreateObj();
		p_result->InsertString("result", "ok");
		p_result->InsertInt("objtype", PPOBJ_GOODS);
		p_result->InsertInt("objid", goodsID);
		p_result->InsertString("displaymethod", "goodsinfo"); // @v11.4.5
		p_result->Insert("detail", p_js_detail);
		//cmd_reply_ok = true;
	}
	else {
		; // @err
	}
	delete p_target_cmd_list;
	return p_result;
}

bool PPStyloQInterchange::GetOwnIdent(SBinaryChunk & rOwnIdent, StyloQCore::StoragePacket * pOwnPack)
{
	rOwnIdent.Z();
	bool   ok = true;
	StyloQCore::StoragePacket own_pack;
	THROW(GetOwnPeerEntry(&own_pack) > 0);
	THROW_PP(own_pack.Pool.Get(SSecretTagPool::tagSvcIdent, &rOwnIdent), PPERR_SQ_UNDEFOWNSVCID);
	ASSIGN_PTR(pOwnPack, own_pack);
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::ProcessCmd(const StyloQProtocol & rRcvPack, const SBinaryChunk & rCliIdent, const SBinaryChunk * pSessSecret, 
	StyloQProtocol & rReplyPack, ProcessCmdCallbackProc intermediateReplyProc, void * pIntermediateReplyExtra)
{
	rReplyPack.Z();
	int    ok = 1;
	SString temp_buf;
	bool   cmd_reply_ok = false;
	bool   do_reply = false; // Если true, то следует ответить клиенту даже в случае ошибки, если false, то ответа не будет 
	SBinaryChunk cmd_bch;
	SBinaryChunk reply_config;
	SBinaryChunk reply_face;
	SBinaryChunk reply_doc;
	SBinaryChunk reply_doc_declaration;
	SBinaryChunk reply_blob; // @v11.3.8
	SBinaryChunk temp_bch;
	SString cmd_buf;
	SString command;
	SString reply_text_buf;
	SGeoPosLL geopos;
	SJson * p_js_reply = 0;
	SJson * p_js_cmd = 0;
	const SJson * p_in_doc = 0;
	const SJson * p_in_declaration = 0;
	LDATETIME _ifchangedsince = ZERODATETIME; // @v11.6.2 Если команда содержит критерий "ifchangedsince" с валидным значением LDATETIME,
		// то эта переменная получит это значение для дальнейшей передачи функциям, непосредственно готовящим данные.
	THROW_PP(rCliIdent.Len(), PPERR_SQ_UNDEFCLIID);
	do_reply = true;
	THROW_PP(rRcvPack.P.Get(SSecretTagPool::tagRawData, &cmd_bch), PPERR_SQ_UNDEFCMDBODY);
	//ProcessCommand(cmd_bch, sess_secret);
	cmd_bch.ToRawStr(cmd_buf);
	THROW_SL(p_js_cmd = SJson::Parse(cmd_buf));
	{
		for(const SJson * p_cur = p_js_cmd; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Type == SJson::tOBJECT) {								
				for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
					if(p_obj->Text.IsEqiAscii("cmd")) {
						command = SJson::Unescape(p_obj->P_Child->Text);
						//p_js_reply->InsertString("reply", command);
					}
					else if(p_obj->Text.IsEqiAscii("document")) {
						if(SJson::IsObject(p_obj->P_Child)) {
							p_in_doc = p_obj->P_Child;
						}
					}
					else if(p_obj->Text.IsEqiAscii("declaration")) {
						if(SJson::IsObject(p_obj->P_Child)) {
							p_in_declaration = p_obj->P_Child;
						}
					}
					else if(p_obj->Text.IsEqiAscii("latitude")) {
						geopos.Lat = p_obj->P_Child->Text.ToReal();
					}
					else if(p_obj->Text.IsEqiAscii("longitude")) {
						geopos.Lon = p_obj->P_Child->Text.ToReal();
					}
					else if(p_obj->Text.IsEqiAscii("ifchangedsince")) { // @v11.6.2
						LDATETIME dtm;
						if(strtodatetime(p_obj->P_Child->Text, &dtm, DATF_ISO8601, TIMF_HMS) && checkdate(dtm.d))
							_ifchangedsince = dtm;
					}
					else {
						if(p_obj->Text.NotEmpty() && p_obj->P_Child && p_obj->P_Child->Text.NotEmpty()) {
							//p_js_reply->InsertString(p_obj->Text, p_obj->P_Child->Text);
						}
					}
				}
			}
		}
	}
	if(command.NotEmptyS()) {
		SBinaryChunk own_ident;
		StyloQCore::StoragePacket own_pack;
		StyloQCore::StoragePacket cli_pack;
		StyloQCommandList full_cmd_list;
		DbProvider * p_dict = CurDict;
		THROW(GetOwnIdent(own_ident, &own_pack));
		if(command.IsEqiAscii("register")) {
			// Регистрация //
			//p_js_reply = SJson::CreateObj();
			//p_js_reply->InsertString("reply", "Hello");
			PPLoadText(PPTXT_SQ_HELLO, reply_text_buf);
			reply_text_buf.Transf(CTRANSF_INNER_TO_UTF8);
			cmd_reply_ok = true;
		}
		else if(command.IsEqiAscii("requestblobinfolist")) { // @v11.3.6 Запрос информации о списке blob'ов с целью выяснить какие из них следует передавать (получать), а какие-нет.
			// Запрос содержит массив пар {blob-ident; block-hash} в ответ на который сервис-медиатор возвращает подмножество этого списка, содержащее набор
			// blob'ов, которые у него отсутствуют либо отличаются по хэшу.
			if(AmIMediator(command)) {
				if(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0) {
					p_js_reply = ProcessCommand_ReqBlobInfoList(cli_pack, p_js_cmd);
					if(p_js_reply)
						cmd_reply_ok = true;
				}
			}
		}
		else if(command.IsEqiAscii("storeblob")) { // @v11.3.3
			// Команда отправляется от сервиса к сервису-медиатору для того, чтобы медиатор сохранил произвольный бинарный объект для доступа к нему клиентов
			if(AmIMediator(command)) {
				if(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0) {
					SBinaryChunk blob;
					THROW(rRcvPack.P.Get(SSecretTagPool::tagBlob, &blob)); // @err
					if(ProcessCommand_StoreBlob(cli_pack, p_js_cmd, blob, reply_text_buf) > 0)
						cmd_reply_ok = true;
				}
			}
		}
		else if(command.IsEqiAscii("getblob")) { // @v11.3.3
			// Команда отправляется от клиента к сервису-медиатору для того, чтобы получить произвольный бинарный объект по идентификатору, предоставленному другим сервисом
			if(AmIMediator(command)) {
				if(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0) {
					p_js_reply = ProcessCommand_GetBlob(cli_pack, p_js_cmd, reply_blob);
					if(p_js_reply) {
						cmd_reply_ok = true;
					}
				}
			}
		}
		else if(command.IsEqiAscii("pushindexingcontent")) { // @v11.3.4
			// Команда передачи сервису-медиатору данных для поисковой индексации
			if(AmIMediator(command)) {
				if(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0) {
					if(ProcessCommand_IndexingContent(cli_pack, p_js_cmd, reply_text_buf) > 0) {
						cmd_reply_ok = true;
					}
				}
			}
		}
		else if(command.IsEqiAscii("search")) { // @v11.3.4
			// Команда поискового запроса
			if(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0) {
				if(ProcessCommand_Search(cli_pack, p_js_cmd, reply_text_buf, temp_buf.Z()) > 0) {
					reply_doc.Z().Put(reply_text_buf.cptr(), reply_text_buf.Len());
					if(temp_buf.Len())
						reply_doc_declaration.Put(temp_buf, temp_buf.Len());
					cmd_reply_ok = true;
				}
			}
		}
		else if(command.IsEqiAscii("advert")) { // Команда отправляется от сервиса к сервису-медиатору для того, чтобы медиатор зарегистрировал конфигурацию
			// сервиса с целью последующей передаче интересующемуся клиенту
			if(AmIMediator(command)) {
				SBinaryChunk foreign_svc_ident;
				StyloQConfig foreign_cfg;
				StyloQFace foreign_face;
				int    local_err = 0;
				for(const SJson * p_cur = p_js_cmd; !local_err && p_cur; p_cur = p_cur->P_Next) {
					if(p_cur->Type == SJson::tOBJECT) {								
						for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
							if(p_obj->Text.IsEqiAscii("config")) {
								if(!foreign_cfg.FromJsonObject(p_obj->P_Child))
									local_err = 1;
							}
							else if(p_obj->Text.IsEqiAscii("face")) {
								if(!foreign_face.FromJsonObject(p_obj->P_Child))
									local_err = 1;
							}
							else if(p_obj->Text.IsEqiAscii("svcident")) {
								if(p_obj->P_Child && foreign_svc_ident.FromMime64(SJson::Unescape(p_obj->P_Child->Text)))
									;
								else {
									PPSetError(PPERR_SQ_MALFORMEDSVCIDENTTEXT);
									local_err = 1; // @todo err_code
								}
							}
						}
					}
				}
				if(!local_err) {
					if(!foreign_cfg.GetCount()) {
						PPSetError(PPERR_SQ_CMDFAULT_ADVHASNTCFG);
						local_err = 1; // @todo err_code
					}
					else if(foreign_svc_ident.Len() == 0) {
						PPSetError(PPERR_SQ_CMDFAULT_ADVHASNTIDENT);
						local_err = 1; // @todo err_code
					}
					else {
						PPID   id = 0;
						StyloQCore::StoragePacket pack;
						if(P_T->SearchGlobalIdentEntry(StyloQCore::kForeignService, foreign_svc_ident, &pack) > 0) {
							id = pack.Rec.ID;
						}
						else {
							pack.Rec.Kind = StyloQCore::kForeignService;
							memcpy(pack.Rec.BI, foreign_svc_ident.PtrC(), foreign_svc_ident.Len());
							pack.Pool.Put(SSecretTagPool::tagSvcIdent, foreign_svc_ident);
						}
						if(foreign_cfg.ToJson(temp_buf)) {
							temp_bch.Z();
							temp_bch.Put(temp_buf.cptr(), temp_buf.Len());
							pack.Pool.Put(SSecretTagPool::tagConfig, temp_bch);
							if(foreign_face.GetCount()) {
								if(foreign_face.ToJson(false, temp_buf)) {
									temp_bch.Put(temp_buf.cptr(), temp_buf.Len());
									pack.Pool.Put(SSecretTagPool::tagFace, temp_bch);
								}
							}
							THROW(P_T->PutPeerEntry(&id, &pack, 1));
							cmd_reply_ok = true;
						}
						else
							local_err = 1;
					}
				}
			}
		}
		else if(command.IsEqiAscii("getforeignconfig")) { // Команда от клиента или сервиса к сервису-медиатору для получения конфигурации другого
			// сервиса по его идентификатору.
			if(AmIMediator(command)) {
				SBinaryChunk foreign_svc_ident;
				StyloQCore::StoragePacket foreign_svc_pack;
				for(const SJson * p_cur = p_js_cmd; p_cur; p_cur = p_cur->P_Next) {
					if(p_cur->Type == SJson::tOBJECT) {								
						for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
							if(p_obj->Text.IsEqiAscii("foreignsvcident")) {
								// @v11.3.3 Unescape
								foreign_svc_ident.FromMime64(SJson::Unescape(p_obj->P_Child->Text));
							}
						}
					}
				}
				if(!foreign_svc_ident) {
					PPSetError(PPERR_SQ_CMDFAULT_GFCHASNTIDENT);
				}
				else if(P_T->SearchGlobalIdentEntry(StyloQCore::kForeignService, foreign_svc_ident, &foreign_svc_pack) > 0) {
					StyloQConfig foreign_cfg;
					StyloQFace foreign_face;
					if(foreign_svc_pack.Pool.Get(SSecretTagPool::tagConfig, &temp_bch)) {
						if(foreign_cfg.FromJson(temp_bch.ToRawStr(temp_buf))) {
							if(foreign_svc_pack.Pool.Get(SSecretTagPool::tagFace, &temp_bch)) {
								foreign_face.FromJson(temp_bch.ToRawStr(temp_buf));
								// Если здесь ошибка - не важно: достаточно конфигурации - лик клиент от самого сервиса получит
							}
							assert(foreign_cfg.GetCount());
							SJson * p_js_fcfg = foreign_cfg.ToJson();
							if(p_js_fcfg) {
								p_js_reply = SJson::CreateObj();
								if(p_js_reply) {
									p_js_reply->Insert("config", p_js_fcfg);
									p_js_fcfg = 0;
									//
									p_js_reply->InsertNz("face", foreign_face.ToJsonObject(true/*forTransmission*/));
									cmd_reply_ok = true;
								}
								else
									PPSetErrorSLib();
							}
						}
					}
					else {
						PPSetError(PPERR_SQ_CMDFAULT_GFCNOCONFIG);
					}
				}
			}
		}
		else if(command.IsEqiAscii("quit") || command.IsEqiAscii("bye")) {
			// Завершение сеанса
			//p_js_reply = SJson::CreateObj();
			//p_js_reply->InsertString("reply", "Bye");
			PPLoadText(PPTXT_SQ_GOODBYE, reply_text_buf);
			reply_text_buf.Transf(CTRANSF_INNER_TO_UTF8);
		}
		else if(command.IsEqiAscii("setface")) {
			if(GetAndStoreClientsFace(rRcvPack, rCliIdent) > 0) {
				cmd_reply_ok = true;
			}
		}
		else if(command.IsEqiAscii("getconfig") || command.IsEqiAscii("getface")) {
			THROW(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0); // @v11.6.0
			if(own_pack.Pool.Get(SSecretTagPool::tagConfig, &reply_config)) {
				assert(reply_config.Len());
				SString transmission_cfg_json;
				if(StyloQConfig::MakeTransmissionJson(reply_config.ToRawStr(temp_buf), &cli_pack, transmission_cfg_json)) {
					reply_config.Z().Put(transmission_cfg_json.cptr(), transmission_cfg_json.Len());
					cmd_reply_ok = true;
				}
				else
					reply_config.Z();
			}
			if(own_pack.Pool.Get(SSecretTagPool::tagSelfyFace, &reply_face)) {
				assert(reply_face.Len());
				//StyloQFace face_pack;
				// @v11.3.8 {
				SString my_face_json_buf;
				if(StyloQFace::MakeTransmissionJson(own_pack.Rec.ID, own_ident, reply_face.ToRawStr(temp_buf), my_face_json_buf)) { 
					reply_face.Put(my_face_json_buf.cptr(), my_face_json_buf.Len());
					cmd_reply_ok = true;
				}
				else
					reply_face.Z();
				// } @v11.3.8 
				/* @v11.3.8 if(face_pack.FromJson(reply_face.ToRawStr(temp_buf))) {
					cmd_reply_ok = true;
				}
				else
					reply_face.Z(); */
			}
			//
			// Возможно, в пакете с запросом конифигурации/лика клиент присал и собственный лик
			//
			GetAndStoreClientsFace(rRcvPack, rCliIdent);
		}
		else if(command.IsEqiAscii("getcommandlist")) {
			// Клиент запрашивает список доступных для него команд
			//SBinaryChunk cli_ident;
			SString db_symb;
			if(!p_dict || !p_dict->GetDbSymb(db_symb)) {
				PPSetError(PPERR_SESSNAUTH);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
				PPSetError(PPERR_SQ_CMDSETLOADINGFAULT);
			}
			else {
				StyloQCommandList * p_target_cmd_list = 0;
				THROW(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0);
				if(StyloQCommandList::GetFullList(db_symb, full_cmd_list)) {
					PPObjID oid(cli_pack.Rec.LinkObjType, cli_pack.Rec.LinkObjID);
					p_target_cmd_list = full_cmd_list.CreateSubListByContext(oid, 0, true);
				}
				p_js_reply = StyloQCommandList::CreateJsonForClient(own_pack, p_target_cmd_list, 0, 0, 4 * 3600);
				ZDELETE(p_target_cmd_list);
				cmd_reply_ok = true;
			}
		}
		else if(command.IsEqiAscii("dtlogin")) {
			// Десктоп-логин (экспериментальная функция)
			THROW(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0);
			if(cli_pack.Rec.Kind == StyloQCore::kClient && cli_pack.Rec.LinkObjType == PPOBJ_USR && cli_pack.Rec.LinkObjID) {
				PPObjSecur sec_obj(PPOBJ_USR, 0);
				PPSecur sec_rec;
				if(sec_obj.Search(cli_pack.Rec.LinkObjID, &sec_rec) > 0) {
					if(!sstreqi_ascii(sec_rec.Name, PPSession::P_JobLogin) && !sstreqi_ascii(sec_rec.Name, PPSession::P_EmptyBaseCreationLogin)) {
						if(OnetimePass(sec_rec.ID) > 0) {
							PPLoadText(PPTXT_SQ_CMDSUCCESS_DTLOGIN, reply_text_buf);
							reply_text_buf.Transf(CTRANSF_INNER_TO_UTF8);
							//p_js_reply = SJson::CreateObj();
							//p_js_reply->InsertString("reply", "Your request for login is accepted :)");
							cmd_reply_ok = true;
						}
					}
				}
			}
			if(!cmd_reply_ok) {
				PPSetError(PPERR_SQ_CMDFAULT_DTLOGIN);
				//p_js_reply = SJson::CreateObj();
				//p_js_reply->InsertString("reply", "Your request for login is not accepted :(");
			}
		}
		else if(command.IsEqiAscii("echo")) {
			p_js_reply = SJson::CreateObj();
			for(const SJson * p_cur = p_js_cmd; p_cur; p_cur = p_cur->P_Next) {
				if(p_cur->Type == SJson::tOBJECT) {								
					for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
						if(p_obj->P_Child->IsString())
							p_js_reply->InsertString(p_obj->Text, p_obj->P_Child->Text);
						else if(p_obj->P_Child->IsNumber())
							p_js_reply->InsertNumber(p_obj->Text, p_obj->P_Child->Text);
					}
				}
			}
			p_js_reply->InsertString("result", "ok");
			cmd_reply_ok = true;
		}
		else if(command.IsEqiAscii("setgeoloc")) { // @v11.6.1
			// @todo
			// svcident || locid
			SString svcident;
			SString locident;
			SGeoPosLL geoloc;
			for(const SJson * p_cur = p_js_cmd; p_cur; p_cur = p_cur->P_Next) {
				if(p_cur->Type == SJson::tOBJECT) {								
					for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
						if(p_obj->Text.IsEqiAscii("svcident"))
							svcident = p_obj->P_Child->Text;
						else if(p_obj->Text.IsEqiAscii("locid"))
							locident = p_obj->P_Child->Text;
						else if(p_obj->Text.IsEqiAscii("lat"))
							geoloc.Lat = p_obj->P_Child->Text.ToReal();
						else if(p_obj->Text.IsEqiAscii("lon"))
							geoloc.Lon = p_obj->P_Child->Text.ToReal();
					}
				}
			}
			THROW(geoloc.IsValid() && !geoloc.IsZero()); // @todo @err
			if(svcident.NotEmpty()) {
				SBinaryChunk bc_own_ident;
				THROW(locident.IsEmpty()); // @todo @err
				if(bc_own_ident.FromMime64(svcident)) {
					PPID   own_id = own_pack.Rec.ID;
					StyloQFace own_face;
					const int tag_id = SSecretTagPool::tagSelfyFace;
					THROW(own_id); // @todo @err
					THROW(bc_own_ident.IsEq(own_pack.Rec.BI, sizeof(own_pack.Rec.BI))); // @todo @err
					own_pack.GetFace(tag_id, own_face);
					THROW(own_face.SetGeoLoc(geoloc));
					{
						SJson * p_js_face = 0;
						SString js_face;
						SBinaryChunk face_chunk;
						SBinarySet::DeflateStrategy ds(512);
						THROW(own_face.ToJson(false, js_face));
						face_chunk.Put(js_face, js_face.Len());
						own_pack.Pool.Put(tag_id, face_chunk, &ds);
						THROW(P_T->PutPeerEntry(&own_id, &own_pack, 1));
						//
						THROW(p_js_face = own_face.ToJsonObject(true/*for_transmission*/)); // @todo @err
						//
						p_js_reply = SJson::CreateObj();
						p_js_reply->InsertString("result", "ok");
						p_js_reply->Insert("svcface", p_js_face);	
						p_js_face = 0;
						cmd_reply_ok = true;
					}
				}
			}
			else if(locident.NotEmpty()) {
				const PPID loc_id = locident.ToLong();
				PPObjLocation loc_obj;
				PPLocationPacket loc_pack;
				THROW(svcident.IsEmpty()); // @todo @err
				THROW(loc_id > 0);
				THROW(loc_obj.GetPacket(loc_id, &loc_pack) > 0);
				loc_pack.Latitude = geoloc.Lat;
				loc_pack.Longitude = geoloc.Lon;
				{
					PPID temp_loc_id = loc_id;
					THROW(loc_obj.PutPacket(&temp_loc_id, &loc_pack, 1));
					//
					p_js_reply = SJson::CreateObj();
					p_js_reply->InsertString("result", "ok");
					{
						SString addr;
						LocationCore::GetAddress(loc_pack, 0, addr);
						SJson * p_js_adr = SJson::CreateObj();
						p_js_adr->InsertInt("id", loc_id);
						p_js_adr->InsertString("addr", (temp_buf = addr).Transf(CTRANSF_INNER_TO_UTF8).Escape());
						{
							const SGeoPosLL geopos(loc_pack.Latitude, loc_pack.Longitude);
							if(geopos.IsValid() && !geopos.IsZero()) {
								p_js_adr->InsertDouble("lat", geopos.Lat, MKSFMTD(0, 12, NMBF_NOTRAILZ));
								p_js_adr->InsertDouble("lon", geopos.Lon, MKSFMTD(0, 12, NMBF_NOTRAILZ));
							}
						}
						p_js_reply->InsertNz("dlvrloc", p_js_adr);
					}
					cmd_reply_ok = true;
				}
			}
			else {
				CALLEXCEPT(); // @todo @err
			}
		}
		else if(command.IsEqiAscii("requestnotificationlist")) { // @v11.5.9 Запрос извещений по командам, для которых определены флаги извещений.
			//S_GUID org_cmd_uuid;
			struct RequestNotificationEntry {
				S_GUID OrgCmdUUID;
				char   NotifyList[256];
			};
			TSVector <RequestNotificationEntry> req_list;
			for(const SJson * p_cur = p_js_cmd; p_cur; p_cur = p_cur->P_Next) {
				if(p_cur->Type == SJson::tOBJECT) {								
					for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
						if(p_obj->Text.IsEqiAscii("cmd")) {
						}
						else if(p_obj->Text.IsEqiAscii("list")) {
							const SJson * p_js_list = p_obj->P_Child;
							if(p_js_list && p_js_list->Type == SJson::tARRAY) {
								for(const SJson * p_js_entry = p_js_list->P_Child; p_js_entry; p_js_entry = p_js_entry->P_Next) {
									if(p_js_entry->Type == SJson::tOBJECT) {
										RequestNotificationEntry req_entry;
										for(const SJson * p_js_fld = p_js_entry->P_Child; p_js_fld; p_js_fld = p_js_fld->P_Next) {
											if(p_js_fld->Text.IsEqiAscii("notify")) {
												STRNSCPY(req_entry.NotifyList, p_js_fld->P_Child->Text);
											}
											else if(p_js_fld->Text.IsEqiAscii("orgcmduuid")) {
												req_entry.OrgCmdUUID.FromStr(p_js_fld->P_Child->Text);
											}
										}
										if(!!req_entry.OrgCmdUUID) {
											req_list.insert(&req_entry);
										}
									}
								}
							}							
						}
					}
				}
			}
			if(req_list.getCount()) {
				SString db_symb;
				if(!p_dict || !p_dict->GetDbSymb(db_symb)) {
					PPSetError(PPERR_SESSNAUTH);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
					PPSetError(PPERR_SQ_CMDSETLOADINGFAULT);
				}
				else {
					if(StyloQCommandList::GetFullList(db_symb, full_cmd_list)) {
						THROW(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0);
						{
							SJson * p_js_evnt_list = new SJson(SJson::tARRAY);
							for(uint reqlistidx = 0; reqlistidx < req_list.getCount(); reqlistidx++) {
								const RequestNotificationEntry & r_req_entry = req_list.at(reqlistidx);
								if(!!r_req_entry.OrgCmdUUID) {
									const StyloQCommandList::Item * p_item = full_cmd_list.GetByUuid(r_req_entry.OrgCmdUUID);
									THROW(p_item);
									const PPObjID oid(cli_pack.Rec.LinkObjType, cli_pack.Rec.LinkObjID);
									StyloQCommandList * p_target_cmd_list = full_cmd_list.CreateSubListByContext(oid, 0, false);
									const StyloQCommandList::Item * p_targeted_item = p_target_cmd_list ? p_target_cmd_list->GetByUuid(r_req_entry.OrgCmdUUID) : 0;
									THROW_PP(p_targeted_item, PPERR_SQ_UNTARGETEDCMD);
									ProcessCommand_RequestNotificationList(*p_targeted_item, cli_pack, p_js_evnt_list);
								}
							}
							{
								assert(p_js_evnt_list != 0);
								p_js_reply = new SJson(SJson::tOBJECT);
								p_js_reply->Insert("evnt_list", p_js_evnt_list);
								cmd_reply_ok = true;
							}
						}
					}
				}
			}
		}
		else if(command.IsEqiAscii("requestdocumentstatuslist")) { // Запрос статусов документов. Отправляется от клиента сервису для взаимной синхронизации статусов
			// В ответ сервис может прислать как статус, так и собственную версию документа. Например, с корректировками или согасованиями.
			//UUID   DocUUID;
			//long   DocID;                    // Идентификатор документа в локальной базе данных
			//long   RemoveDocID;              // Идентификатор документа у контрагента
			//int    AfterTransmitStatusFlags; // Флаги статуса, которые должны быть установлены у документа после успешной передачи
			//int    RemoteStatus;             // Статус документа у контрагента				
			/*
			{
				orgcmduuid // GUID оригинальной (сервисной) команды, к которой привязан документ
				uuid       // GUID документа
				cid        // Клиентский ид документа
				sid        // Ид документа у сервиса
				cst        // Клиентский статус документа
				sst        // Статус документа у сервиса
			}
			*/
			THROW(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0);
			p_js_reply = ProcessCommand_RequestDocumentStatusList(own_ident, cli_pack, p_js_cmd);
			if(p_js_reply) {
				cmd_reply_ok = true;
			}
		}
		else if(command.IsEqiAscii("postdocument")) { // Отправка документа от клиента сервису
			// Передается document-declaration and document
			// document содержится как объект в json команды с тегом document
			// document-declaration находится в общем пакете под тегом SSecretTagPool::tagDocDeclaration
			// В ответ отправляется сигнал о успешной либо безуспешной обработке команды
			//
			PPID   result_obj_id = 0;
			THROW(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0);
			p_js_reply = ProcessCommand_PostDocument(own_ident, cli_pack, p_in_declaration, p_in_doc, &result_obj_id);
			if(p_js_reply) {
				cmd_reply_ok = true;
			}
		}
		else if(command.IsEqiAscii("getgoodsinfo")) {
			p_js_reply = SJson::CreateObj();
			SString goods_code;
			PPID   goods_id = 0;
			for(const SJson * p_cur = p_js_cmd; p_cur; p_cur = p_cur->P_Next) {
				if(p_cur->Type == SJson::tOBJECT) {								
					for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
						if(p_obj->Text.IsEqiAscii("code") || p_obj->Text.IsEqiAscii("cod"))
							goods_code = SJson::Unescape(p_obj->P_Child->Text);
						else if(p_obj->Text.IsEqiAscii("id"))
							goods_id = SJson::Unescape(p_obj->P_Child->Text).ToLong();
					}
				}
			}
			if(goods_id || goods_code.NotEmptyS()) {
				SString db_symb;
				if(!p_dict || !p_dict->GetDbSymb(db_symb)) {
					PPSetError(PPERR_SESSNAUTH);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
					PPSetError(PPERR_SQ_CMDSETLOADINGFAULT);
				}
				else {
					StyloQCommandList * p_target_cmd_list = 0;
					THROW(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0);
					if(StyloQCommandList::GetFullList(db_symb, full_cmd_list)) {
						const PPObjID oid(cli_pack.Rec.LinkObjType, cli_pack.Rec.LinkObjID);
						p_target_cmd_list = full_cmd_list.CreateSubListByContext(oid, StyloQCommandList::sqbcGoodsInfo, false);
						assert(!p_target_cmd_list || p_target_cmd_list->GetCount() > 0);
					}
					p_js_reply = ProcessCommand_GetGoodsInfo(own_ident, cli_pack, (p_target_cmd_list ? p_target_cmd_list->Get(0) : 0), goods_id, goods_code);
					ZDELETE(p_target_cmd_list);
				}
			}
			else {
				; // @err
			}
			//p_js_reply->InsertString("result", "ok");
			//cmd_reply_ok = true;
		}
		else if(command.IsEqiAscii("onsrchr")) { // Пользователь нажал на результат глобального поиска
			PPObjID oid;
			SString txt_objtype;
			SString txt_objid;
			for(const SJson * p_cur = p_js_cmd; p_cur; p_cur = p_cur->P_Next) {
				if(p_cur->Type == SJson::tOBJECT) {								
					for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
						if(p_obj->Text.IsEqiAscii("objtype"))
							txt_objtype = SJson::Unescape(p_obj->P_Child->Text);
						else if(p_obj->Text.IsEqiAscii("objid"))
							txt_objid = SJson::Unescape(p_obj->P_Child->Text);
					}
				}
			}
			if(PPObject::Identify(txt_objtype, txt_objid, &oid)) {
				/*
				{
					objtype
					objid
					ware || prc || svc
					id
					nm
					imgblobs
					commondescription
					spcdescription [
						{
							title
							descr
						}
					]
					ware {
								
					}
				}

			if(GetBlobInfo(rOwnIdent, oid, 0, blob_info, 0)) {
				assert(blob_info.Signature.Len() && blob_info.Signature.IsAscii());
				p_jsobj->InsertString("imgblobs", blob_info.Signature);
				Stq_CmdStat_MakeRsrv_Response::RegisterBlobOid(pStat, oid);
			}
				*/
				if(oid.Obj == PPOBJ_GOODS) {
					if(oid.Id) {
						p_js_reply = ReplyGoodsInfo(own_ident, rCliIdent, oid.Id, 0);
						if(p_js_reply)
							cmd_reply_ok = true;
					}
				}
				else if(oid.Obj == PPOBJ_PROCESSOR) {
					if(oid.Id) {
						PPObjProcessor prc_obj;
						PPProcessorPacket prc_pack;
						if(prc_obj.GetPacket(oid.Id, &prc_pack) > 0) {
							SJson * p_js = MakeObjJson_Prc(own_ident, prc_pack.Rec, 0, 0);
							THROW(p_js);
							p_js_reply = SJson::CreateObj();
							p_js_reply->InsertString("result", "ok");
							p_js_reply->InsertInt("objtype", oid.Obj);
							p_js_reply->InsertInt("objid", oid.Id);
							p_js_reply->Insert("detail", p_js);
							cmd_reply_ok = true;
						}
					}
				}
				else if(oid.Obj == PPOBJ_PERSON) {
					if(oid.Id) {
						PPObjPerson psn_obj;
						PPPersonPacket psn_pack;
						if(psn_obj.GetPacket(oid.Id, &psn_pack, 0) > 0) {
						}
					}
				}
				else if(oid.Obj == PPOBJ_STYLOQBINDERY) {
					if(oid.Id) {
						SJson * p_js = MakeObjJson_OwnFace(own_pack, 0);
						p_js_reply = SJson::CreateObj();
						p_js_reply->InsertString("result", "ok");
						p_js_reply->InsertInt("objtype", oid.Obj);
						p_js_reply->InsertInt("objid", oid.Id);
						p_js_reply->Insert("detail", p_js);
						cmd_reply_ok = true;
					}
				}
			}
			else {
				; // @err
			}
		}
		else {
			S_GUID cmd_uuid;
			if(cmd_uuid.FromStr(command) && SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0) {
				SString db_symb;
				if(!p_dict || !p_dict->GetDbSymb(db_symb)) {
					PPSetError(PPERR_SESSNAUTH);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
					PPSetError(PPERR_SQ_CMDSETLOADINGFAULT);
				}
				else if(StyloQCommandList::GetFullList(db_symb, full_cmd_list)) {
					const PPObjID oid(cli_pack.Rec.LinkObjType, cli_pack.Rec.LinkObjID);
					const StyloQCommandList::Item * p_item = full_cmd_list.GetByUuid(cmd_uuid);
					THROW(p_item);
					StyloQCommandList * p_target_cmd_list = full_cmd_list.CreateSubListByContext(oid, 0, false);
					const StyloQCommandList::Item * p_targeted_item = p_target_cmd_list ? p_target_cmd_list->GetByUuid(cmd_uuid) : 0;
					THROW_PP(p_targeted_item, PPERR_SQ_UNTARGETEDCMD);
					switch(p_targeted_item->BaseCmdId) {
						case StyloQCommandList::sqbcPersonEvent:
							if(ProcessCommand_PersonEvent(StyloQCommandList::Item(*p_targeted_item), cli_pack, p_js_cmd, geopos)) {
								PPLoadText(PPTXT_SQ_CMDSUCCESS_PSNEV, reply_text_buf);
								reply_text_buf.Transf(CTRANSF_INNER_TO_UTF8);
								cmd_reply_ok = true;
							}
							else {
								PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
								PPSetError(PPERR_SQ_CMDFAULT_PSNEV);
							}
							break;
						case StyloQCommandList::sqbcReport:
							IntermediateReply(5*60*1000, 1000, pSessSecret, intermediateReplyProc, pIntermediateReplyExtra); // @v11.4.1
							if(ProcessCommand_Report(StyloQCommandList::Item(*p_targeted_item), cli_pack, geopos, reply_text_buf, temp_buf.Z(), false)) {
								reply_doc.Put(reply_text_buf, reply_text_buf.Len());
								if(temp_buf.Len())
									reply_doc_declaration.Put(temp_buf, temp_buf.Len());
								cmd_reply_ok = true;
							}
							else {
								PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
								PPSetError(PPERR_SQ_CMDFAULT_REPORT);
							}
							break;
						case StyloQCommandList::sqbcRsrvOrderPrereq: // @v11.2.6
							{
								// @v11.4.8 {
								bool local_done = false;
								if(p_targeted_item->Flags & StyloQCommandList::Item::fPrepareAhead) {
									int   local_doc_type = StyloQCore::doctypOrderPrereq;
									SBinaryChunk local_doc_ident;
									StyloQCore::StoragePacket local_sp;
									bool  do_prepare = true;
									if(P_T->MakeDocumentStorageIdent(own_ident, p_targeted_item->Uuid, local_doc_ident)) {
										if(P_T->GetDocByType(+1, local_doc_type, &local_doc_ident, &local_sp) > 0) {
											SBinaryChunk local_raw_doc;
											SBinaryChunk local_raw_decl;
											if(local_sp.Pool.Get(SSecretTagPool::tagRawData, &local_raw_doc) && local_sp.Pool.Get(SSecretTagPool::tagDocDeclaration, &local_raw_decl)) {
												reply_doc = local_raw_doc;
												reply_doc_declaration = local_raw_decl;
												local_done = true;
												cmd_reply_ok = true;
											}
										}
									}
								}
								// } @v11.4.8 
								if(!local_done) {
									IntermediateReply(5*60*1000, 1000, pSessSecret, intermediateReplyProc, pIntermediateReplyExtra); // @v11.2.12
									if(ProcessCommand_RsrvOrderPrereq(StyloQCommandList::Item(*p_targeted_item), cli_pack, reply_text_buf, temp_buf.Z(), false)) {
										reply_doc.Put(reply_text_buf, reply_text_buf.Len());
										if(temp_buf.Len())
											reply_doc_declaration.Put(temp_buf, temp_buf.Len());
										cmd_reply_ok = true;
									}
									else {
										PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
										PPSetError(PPERR_SQ_CMDFAULT_ORDERPREREQ);
									}
								}
							}
							break;
						case StyloQCommandList::sqbcRsrvAttendancePrereq: // v11.3.2
							{
								IntermediateReply(5*60*1000, 1000, pSessSecret, intermediateReplyProc, pIntermediateReplyExtra);
								if(ProcessCommand_RsrvAttendancePrereq(StyloQCommandList::Item(*p_targeted_item), cli_pack, geopos, reply_text_buf, temp_buf.Z(), 0)) {
									reply_doc.Put(reply_text_buf, reply_text_buf.Len());
									if(temp_buf.Len())
										reply_doc_declaration.Put(temp_buf, temp_buf.Len());
									cmd_reply_ok = true;
								}
								else {
									PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
									PPSetError(PPERR_SQ_CMDFAULT_ATTENDANCEPREREQ);
								}
							}
							break;
						case StyloQCommandList::sqbcRsrvIndoorSvcPrereq: // @v11.4.4
							{
								// @v11.4.8 {
								bool local_done = false;
								if(p_targeted_item->Flags & StyloQCommandList::Item::fPrepareAhead) {
									int   local_doc_type = StyloQCore::doctypIndoorSvcPrereq;
									SBinaryChunk local_doc_ident;
									StyloQCore::StoragePacket local_sp;
									bool  do_prepare = true;
									if(P_T->MakeDocumentStorageIdent(own_ident, p_targeted_item->Uuid, local_doc_ident)) {
										if(P_T->GetDocByType(+1, local_doc_type, &local_doc_ident, &local_sp) > 0) {
											SBinaryChunk local_raw_doc;
											SBinaryChunk local_raw_decl;
											if(local_sp.Pool.Get(SSecretTagPool::tagRawData, &local_raw_doc) && local_sp.Pool.Get(SSecretTagPool::tagDocDeclaration, &local_raw_decl)) {
												reply_doc = local_raw_doc;
												reply_doc_declaration = local_raw_decl;
												local_done = true;
												cmd_reply_ok = true;
											}
										}
									}
								}
								// } @v11.4.8 
								if(!local_done) {
									IntermediateReply(3*60*1000, 1000, pSessSecret, intermediateReplyProc, pIntermediateReplyExtra);
									if(ProcessCommand_RsrvIndoorSvcPrereq(StyloQCommandList::Item(*p_targeted_item), cli_pack, reply_text_buf, temp_buf.Z(), 0)) {
										reply_doc.Put(reply_text_buf, reply_text_buf.Len());
										if(temp_buf.Len())
											reply_doc_declaration.Put(temp_buf, temp_buf.Len());
										cmd_reply_ok = true;									
									}
									else {
										PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
										PPSetError(PPERR_SQ_CMDFAULT_INDOORSVCPREREQ);
									}
								}
							}
							break;
						case StyloQCommandList::sqbcGoodsInfo: // @v11.4.4
							// @todo
							break;
						case StyloQCommandList::sqbcLocalBarcodeSearch: // @v11.4.5
							{
								SString barcode;
								SString barcodestd;
								for(const SJson * p_cur = p_js_cmd; p_cur; p_cur = p_cur->P_Next) {
									if(p_cur->Type == SJson::tOBJECT) {								
										for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
											if(p_obj->Text.IsEqiAscii("barcode"))
												barcode = SJson::Unescape(p_obj->P_Child->Text);
											else if(p_obj->Text.IsEqiAscii("barcodestd"))
												barcodestd = SJson::Unescape(p_obj->P_Child->Text);
										}
									}
								}
								if(barcode.NotEmptyS()) {
									p_js_reply = ReplyGoodsInfo(own_ident, rCliIdent, 0, barcode);
									if(p_js_reply)
										cmd_reply_ok = true;									
								}
							}
							break;
						case StyloQCommandList::sqbcIncomingListOrder: // @v11.4.6
							{
								IntermediateReply(3*60*1000, 1000, pSessSecret, intermediateReplyProc, pIntermediateReplyExtra);
								if(ProcessCommand_IncomingListOrder(StyloQCommandList::Item(*p_targeted_item), cli_pack, &_ifchangedsince, reply_text_buf, temp_buf.Z(), 0)) {
									reply_doc.Put(reply_text_buf, reply_text_buf.Len());
									if(temp_buf.Len())
										reply_doc_declaration.Put(temp_buf, temp_buf.Len());
									cmd_reply_ok = true;									
								}
								else {
									PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
									PPSetError(PPERR_SQ_CMDFAULT_INCOMINGLIST);
								}
							}
							break;
						case StyloQCommandList::sqbcIncomingListCCheck: // @v11.4.7
							{
								IntermediateReply(3*60*1000, 1000, pSessSecret, intermediateReplyProc, pIntermediateReplyExtra);
								if(ProcessCommand_IncomingListCCheck(StyloQCommandList::Item(*p_targeted_item), cli_pack, &_ifchangedsince, reply_text_buf, temp_buf.Z(), 0)) {
									reply_doc.Put(reply_text_buf, reply_text_buf.Len());
									if(temp_buf.Len())
										reply_doc_declaration.Put(temp_buf, temp_buf.Len());
									cmd_reply_ok = true;									
								}
								else {
									PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
									PPSetError(PPERR_SQ_CMDFAULT_INCOMINGLIST);
								}
							}
							break;
						case StyloQCommandList::sqbcIncomingListTSess: // @v11.6.5
							{
								IntermediateReply(3*60*1000, 1000, pSessSecret, intermediateReplyProc, pIntermediateReplyExtra);
								if(ProcessCommand_IncomingListTSess(StyloQCommandList::Item(*p_targeted_item), cli_pack, &_ifchangedsince, reply_text_buf, temp_buf.Z(), 0)) {
									reply_doc.Put(reply_text_buf, reply_text_buf.Len());
									if(temp_buf.Len())
										reply_doc_declaration.Put(temp_buf, temp_buf.Len());
									cmd_reply_ok = true;									
								}
								else {
									PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
									PPSetError(PPERR_SQ_CMDFAULT_INCOMINGLIST);
								}
							}
							break;
						case StyloQCommandList::sqbcIncomingListTodo: // @v11.4.7
							{
								// @todo
							}
							break;
						case StyloQCommandList::sqbcDebtList: // @v11.5.4
							{
								if(ProcessCommand_DebtList(StyloQCommandList::Item(*p_targeted_item), p_js_cmd, cli_pack, reply_text_buf, temp_buf.Z(), 0)) {
									reply_doc.Put(reply_text_buf, reply_text_buf.Len());
									if(temp_buf.Len())
										reply_doc_declaration.Put(temp_buf, temp_buf.Len());
									cmd_reply_ok = true;									
								}
								else {
									PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
									PPSetError(PPERR_SQ_CMDFAULT_DEBTLIST);
								}
							}
							break;
						default:
							PPSetError(PPERR_SQ_UNSUPPORTEDCMD);
							break;
					}
				}
				else {
					// Сохраняем детализированную ошибку в логе а клиенту передаем обобщенное сообщение об ошибке
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
					PPSetError(PPERR_SQ_CMDSETLOADINGFAULT);
				}
			}
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	if(do_reply) {
		//StyloQProtocol reply_tp;
		SBinarySet::DeflateStrategy ds(512);
		rReplyPack.StartWriting(PPSCMD_SQ_COMMAND, cmd_reply_ok ? StyloQProtocol::psubtypeReplyOk : StyloQProtocol::psubtypeReplyError);
		if(reply_config.Len())
			rReplyPack.P.Put(SSecretTagPool::tagConfig, reply_config);
		if(reply_face.Len())
			rReplyPack.P.Put(SSecretTagPool::tagFace, reply_face);
		if(reply_doc.Len()) {
			if(reply_doc_declaration.Len()) {
				rReplyPack.P.Put(SSecretTagPool::tagDocDeclaration, reply_doc_declaration);
			}
			rReplyPack.P.Put(SSecretTagPool::tagRawData, reply_doc, &ds);
		}
		else if(p_js_reply) {
			p_js_reply->ToStr(cmd_buf);
			cmd_bch.Put(cmd_buf.cptr(), cmd_buf.Len());
			rReplyPack.P.Put(SSecretTagPool::tagRawData, cmd_bch, &ds);
			if(reply_blob.Len()) {
				rReplyPack.P.Put(SSecretTagPool::tagBlob, reply_blob, &ds);
				reply_blob.Z();
			}
		}
		else {
			p_js_reply = SJson::CreateObj();
			p_js_reply->InsertString("result", cmd_reply_ok ? "ok" : "error");
			if(!cmd_reply_ok) {
				PPGetLastErrorMessage(1, temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
				p_js_reply->InsertInt("errcode", PPErrCode);
				p_js_reply->InsertString("errmsg", temp_buf.Escape());
			}
			if(reply_text_buf.NotEmpty()) {
				p_js_reply->InsertString("msg", reply_text_buf.Escape());
			}
			p_js_reply->ToStr(cmd_buf);
			cmd_bch.Put(cmd_buf.cptr(), cmd_buf.Len());
			rReplyPack.P.Put(SSecretTagPool::tagRawData, cmd_bch, &ds);
		}
		rReplyPack.FinishWriting(pSessSecret);
		/*{
			PPMqbClient::MessageProperties props;
			int pr = B.P_Mqbc->Publish(B.P_MqbRpe->ExchangeName, B.P_MqbRpe->RoutingKey, &props, reply_tp.constptr(), reply_tp.GetAvailableSize());
			// @debug {
			if(pr) {
				props.Z();
			}
			// } @debug
		}*/
		ok = 1;
	}
	else
		ok = 0;
	ZDELETE(p_js_cmd);
	ZDELETE(p_js_reply);
	return ok;
}

/*static*/void PPStyloQInterchange::SetupMqbReplyProps(/*const StyloQProtocol & rReqPack*/const RoundTripBlock & rB, PPMqbClient::MessageProperties & rProps)
{
	//SBinaryChunk bc;
	//if(rReqPack.P.Get(SSecretTagPool::tagRoundTripIdent, &bc) && bc.Len() == sizeof(S_GUID)) {
	if(!!rB.Uuid) {
		//S_GUID round_trip_uuid;
		//assert(bc.Len() == sizeof(round_trip_uuid)); // @paranoic
		//memcpy(&round_trip_uuid, bc.PtrC(), bc.Len());
		rProps.CorrelationId.Z().Cat(rB.Uuid, S_GUID::fmtIDL|S_GUID::fmtLower);
		rProps.Expiration.Z().Cat(120000);
	}
}

class StyloQServerSession : public PPThread {
	PPStyloQInterchange * P_Ic;
	DbLoginBlock LB;
	PPStyloQInterchange::RunServerParam StartUp_Param;
	const StyloQProtocol TPack;
	PPStyloQInterchange::RoundTripBlock B;
	SBinaryChunk CliIdent;
public:
	StyloQServerSession(const DbLoginBlock & rLB, const PPStyloQInterchange::RunServerParam & rP, const StyloQProtocol & rTPack) : 
		PPThread(kStyloQSession, 0, 0), LB(rLB), StartUp_Param(rP), TPack(rTPack), P_Ic(0)
	{
		StartUp_Param.MqbInitParam.ConsumeParamList.freeAll();
		TPack.P.Get(SSecretTagPool::tagClientIdent, &CliIdent);
	}
	~StyloQServerSession()
	{
		delete P_Ic;
	}
	int SrpAuth(const SBinaryChunk & rOtherPublic)
	{
		int    ok = 1;
		int    debug_mark = 0;
		SlSRP::Verifier * p_srp_vrf = 0;
		PPMqbClient * p_mqbc = 0;
		PPMqbClient::MessageProperties props;
		PPMqbClient::RoutingParamEntry rpe;
		StyloQProtocol tp;
		StyloQProtocol reply_tp;
		PPMqbClient::Envelope env;
		PPID   id = 0; // Внутренний идентификатор записи клиента в DBMS
		SString temp_buf;
		StyloQCore::StoragePacket storage_pack;
		// @v11.3.10 SBinaryChunk other_public;
		SBinaryChunk my_public;
		SBinaryChunk srp_s;
		SBinaryChunk srp_v;
		SBinaryChunk __a; // A
		SBinaryChunk __b; // B
		SBinaryChunk __m; // M
		SBinaryChunk temp_bc;
		SBinaryChunk debug_cli_secret; // @debug do remove after debugging!
		// @v11.3.10 THROW(TPack.P.Get(SSecretTagPool::tagSessionPublicKey, &other_public));
		//THROW_PP(TPack.P.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_UNDEFCLIID);
		THROW_PP(CliIdent.Len(), PPERR_SQ_UNDEFCLIID);
		assert(rOtherPublic.Len());
		THROW_PP(rOtherPublic.Len(), PPERR_SQ_UNDEFSESSPUBKEY); // @v11.3.10
		THROW(Login()); // @error (Inner service error: unable to login
		THROW(SETIFZ(P_Ic, new PPStyloQInterchange));
		B.Other.Put(SSecretTagPool::tagClientIdent, CliIdent);
		THROW(P_Ic->KexServiceReply(B.Sess, TPack.P, 0, 0));
		B.Sess.Get(SSecretTagPool::tagSessionPublicKey, &my_public);
		assert(my_public.Len());
		THROW(TPack.P.Get(SSecretTagPool::tagSrpA, &__a));
		//THROW_PP(TPack.P.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_UNDEFCLIID); // @redundant
		THROW(P_Ic->SearchGlobalIdentEntry(StyloQCore::kClient, CliIdent, &storage_pack) > 0);
		THROW(storage_pack.Rec.Kind == StyloQCore::kClient);
		THROW(storage_pack.Pool.Get(SSecretTagPool::tagClientIdent, &temp_bc) && temp_bc == CliIdent); // @error (I don't know you)
		THROW(storage_pack.Pool.Get(SSecretTagPool::tagSrpVerifier, &srp_v)); // @error (I havn't got your registration data)
		THROW(storage_pack.Pool.Get(SSecretTagPool::tagSrpVerifierSalt, &srp_s)); // @error (I havn't got your registration data)
		storage_pack.Pool.Get(SSecretTagPool::tagSecret, &debug_cli_secret); // @debug do remove after debugging!
		{
			const uchar * p_bytes_HAMK = 0;
			p_srp_vrf = P_Ic->CreateSrpPacket_Svc_Auth(my_public, CliIdent, srp_s, srp_v, __a, reply_tp);
			props.Z();
			assert(StartUp_Param.MqbInitParam.ConsumeParamList.getCount() == 0);
			THROW(rpe.SetupStyloQRpc(my_public, rOtherPublic, StartUp_Param.MqbInitParam.ConsumeParamList.CreateNewItem()));
			THROW(p_mqbc = PPMqbClient::CreateInstance(StartUp_Param.MqbInitParam));
			//
			const long cmto = PPMqbClient::SetupMessageTtl(__DefMqbConsumeTimeout, &props);
			PPStyloQInterchange::SetupMqbReplyProps(B, props); // @v11.3.1 
			THROW(p_mqbc->Publish(rpe.ExchangeName, rpe.RoutingKey, &props, reply_tp.constptr(), reply_tp.GetAvailableSize())); // @reply
			THROW(p_srp_vrf); // Отложенная обработка ошибки верификации
			//do_process_loop = true;
			{
				temp_buf = "ConsumeMessage";
				for(uint i = 0; i < StartUp_Param.MqbInitParam.ConsumeParamList.getCount(); i++) {
					const PPMqbClient::RoutingParamEntry * p_rpe = StartUp_Param.MqbInitParam.ConsumeParamList.at(i);
					temp_buf.Space().Cat(p_rpe->QueueName);
					//p_cli->Consume(p_rpe->QueueName, &consumer_tag.Z(), 0);
				}
				PPLogMessage(PPFILNAM_DEBUG_LOG, temp_buf, LOGMSGF_TIME);
				//
				int cmr = p_mqbc->ConsumeMessage(env, cmto);
				if(cmr < 0) {
					CALLEXCEPT_PP_S(PPERR_MQBC_CONSUMETIMEOUT, cmto);
				}
				else {
					THROW(cmr);
				}
			}
			p_mqbc->Ack(env.DeliveryTag, 0);
			//
			THROW(tp.Read(env.Msg, 0));
			THROW(tp.CheckRepError());
			THROW(tp.GetH().Type == PPSCMD_SQ_SRPAUTH_S2);
			THROW(tp.P.Get(SSecretTagPool::tagSrpM, &__m));
			p_srp_vrf->VerifySession(static_cast<const uchar *>(__m.PtrC()), &p_bytes_HAMK);
			THROW(p_bytes_HAMK); // @error User authentication failed!
			{
				// Host -> User: (HAMK) 
				const SBinaryChunk srp_hamk(p_bytes_HAMK, p_srp_vrf->GetSessionKeyLength());
				reply_tp.StartWriting(PPSCMD_SQ_SRPAUTH_S2, StyloQProtocol::psubtypeReplyOk);
				reply_tp.P.Put(SSecretTagPool::tagSrpHAMK, srp_hamk);
				reply_tp.FinishWriting(0);
				{
					const long cmto = PPMqbClient::SetupMessageTtl(__DefMqbConsumeTimeout, &props);
					PPStyloQInterchange::SetupMqbReplyProps(B, props); // @v11.3.1 
					THROW(p_mqbc->Publish(rpe.ExchangeName, rpe.RoutingKey, &props, reply_tp.constptr(), reply_tp.GetAvailableSize())); // @reply
					// Последнее сообщение от клиента: если он скажет OK - считаем верификацию завершенной
					THROW(p_mqbc->ConsumeMessage(env, cmto) > 0);
					p_mqbc->Ack(env.DeliveryTag, 0);
				}
				THROW(tp.Read(env.Msg, 0));
				THROW(tp.CheckRepError());
				THROW(tp.GetH().Type == PPSCMD_SQ_SRPAUTH_ACK);
				//
				// Теперь все - верификация завершена. Сохраняем сессию и дальше будем ждать полезных сообщений от клиента
				{
					uint32 cli_session_expiry_period = 0;
					uint32 svc_session_expiry_period = P_Ic->GetNominalSessionLifeTimeSec();
					PPID   sess_id = 0;
					SBinaryChunk temp_chunk;
					SBinaryChunk _sess_secret;
					StyloQCore::StoragePacket sess_pack;
					// @v11.2.2 {
					if(tp.P.Get(SSecretTagPool::tagSessionExpirPeriodSec, &temp_chunk) && temp_chunk.Len() == sizeof(uint32)) {
						cli_session_expiry_period = PTR32C(temp_chunk.PtrC())[0];
					}
					// } @v11.2.2 
					// Теперь надо сохранить параметры сессии дабы в следующий раз не проделывать столь сложную процедуру
					//
					// Проверки assert'ами (не THROW) реализуются из-за того, что не должно возникнуть ситуации, когда мы
					// попали в этот участок кода с невыполненными условиями (то есть при необходимости THROW должны были быть вызваны выше).
					B.Sess.Get(SSecretTagPool::tagSessionSecret, &_sess_secret);
					assert(_sess_secret.Len());
					assert(my_public.Len());
					// @v11.3.10 assert(rOtherPublic.Len());
					assert(B.Sess.Get(SSecretTagPool::tagSessionPrivateKey, 0));
					sess_pack.Pool.Put(SSecretTagPool::tagSessionPublicKey, my_public);
					{
						B.Sess.Get(SSecretTagPool::tagSessionPrivateKey, &temp_chunk);
						sess_pack.Pool.Put(SSecretTagPool::tagSessionPrivateKey, temp_chunk);
					}
					sess_pack.Pool.Put(SSecretTagPool::tagSessionPublicKeyOther, rOtherPublic);
					sess_pack.Pool.Put(SSecretTagPool::tagSessionSecret, _sess_secret);
					sess_pack.Pool.Put(SSecretTagPool::tagClientIdent, CliIdent);
					// @v11.2.2 {
					if(cli_session_expiry_period || svc_session_expiry_period) {
						uint32 sep = 0;
						if(!cli_session_expiry_period || !svc_session_expiry_period)
							sep = MAX(cli_session_expiry_period, svc_session_expiry_period);
						else
							sep = MIN(cli_session_expiry_period, svc_session_expiry_period);
						if(sep)
							sess_pack.Rec.Expiration.SetTimeT(time(0) + sep);
					}
					// } @v11.2.2 
					THROW(P_Ic->StoreSession(&sess_id, &sess_pack, 1));
					// @v11.2.2 {
					{
						reply_tp.StartWriting(PPSCMD_SQ_SRPAUTH_ACK, StyloQProtocol::psubtypeReplyOk);
						reply_tp.P.Put(SSecretTagPool::tagSessionPublicKey, my_public);
						if(svc_session_expiry_period)
							reply_tp.P.Put(SSecretTagPool::tagSessionExpirPeriodSec, &svc_session_expiry_period, sizeof(svc_session_expiry_period));
						THROW(reply_tp.FinishWriting(0));
						PPStyloQInterchange::SetupMqbReplyProps(B, props); // @v11.3.1 
						THROW(p_mqbc->Publish(rpe.ExchangeName, rpe.RoutingKey, &props, reply_tp.constptr(), reply_tp.GetAvailableSize())); // @reply
					}
					// } @v11.2.2 
				}
				if(P_Ic->SetRoundTripBlockReplyValues(B, p_mqbc, rpe)) {
					p_mqbc = 0;
				}
			}
		}
		CATCHZOK
		delete p_mqbc;
		delete p_srp_vrf;
		return ok;
	}
	int Acquaintance(const SBinaryChunk & rOtherPublic)
	{
		int    ok = 1;
		PPMqbClient * p_mqbc = 0;
		//SBinaryChunk other_public;
		SBinaryChunk my_public;
		//SBinaryChunk sess_secret; // @debug
		StyloQProtocol reply_tp;
		PPMqbClient::RoutingParamEntry rpe;
		PPMqbClient::MessageProperties props;
		THROW_PP(CliIdent.Len(), PPERR_SQ_UNDEFCLIID);
		// @v11.3.10 THROW_PP(TPack.P.Get(SSecretTagPool::tagSessionPublicKey, &other_public), PPERR_SQ_UNDEFSESSPUBKEY);
		assert(rOtherPublic.Len());
		THROW_PP(rOtherPublic.Len(), PPERR_SQ_UNDEFSESSPUBKEY); // @v11.3.10
		THROW(Login());
		assert(P_Ic == 0);
		THROW_SL(P_Ic = new PPStyloQInterchange);
		B.Other.Put(SSecretTagPool::tagClientIdent, CliIdent);
		const int fsks = P_Ic->KexServiceReply(B.Sess, TPack.P, /*p_debug_pub_ecp_x*/0, /*p_debug_pub_ecp_y*/0);
		THROW(fsks);
		{
			//
			// Неоднозначность: надо ли обрабатывать запрос на знакомство если мы уже знакомы (fsks != fsksNewEntry)
			//
			THROW(B.Sess.Get(SSecretTagPool::tagSessionPublicKey, &my_public));
			assert(my_public.Len());
			reply_tp.StartWriting(PPSCMD_SQ_ACQUAINTANCE, StyloQProtocol::psubtypeReplyOk);
			reply_tp.P.Put(SSecretTagPool::tagSessionPublicKey, my_public);
			props.Z();
			assert(StartUp_Param.MqbInitParam.ConsumeParamList.getCount() == 0);
			THROW(rpe.SetupStyloQRpc(my_public, rOtherPublic, StartUp_Param.MqbInitParam.ConsumeParamList.CreateNewItem()));
			THROW(reply_tp.FinishWriting(0));
			THROW(p_mqbc = PPMqbClient::CreateInstance(StartUp_Param.MqbInitParam));
			PPStyloQInterchange::SetupMqbReplyProps(B, props); // @v11.3.1 
			THROW(p_mqbc->Publish(rpe.ExchangeName, rpe.RoutingKey, &props, reply_tp.constptr(), reply_tp.GetAvailableSize())); // @reply
			if(P_Ic->SetRoundTripBlockReplyValues(B, p_mqbc, rpe)) {
				p_mqbc = 0;
			}
		}
		CATCHZOK
		delete p_mqbc;
		return ok;
	}
	int Session(const SBinaryChunk & rOtherPublic)
	{
		int    ok = 1;
		PPMqbClient * p_mqbc = 0;
		SBinaryChunk temp_chunk;
		//@v11.3.10 SBinaryChunk other_public;
		SBinaryChunk my_public;
		//SBinaryChunk cli_ident;
		SBinaryChunk sess_secret;
		SBinaryChunk * p_sess_secret = 0;
		StyloQProtocol reply_tp;
		PPMqbClient::RoutingParamEntry rpe;
		PPMqbClient::MessageProperties props;
		StyloQCore::StoragePacket pack;
		//@v11.3.10 THROW_PP(TPack.P.Get(SSecretTagPool::tagSessionPublicKey, &other_public), PPERR_SQ_INPHASNTSESSPUBKEY); // PPERR_SQ_INPHASNTSESSPUBKEY Входящий запрос сессии не содержит публичного ключа
		//THROW_PP(TPack.P.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_INPHASNTCLIIDENT); 
		THROW_PP(CliIdent.Len(), PPERR_SQ_INPHASNTCLIIDENT); // PPERR_SQ_INPHASNTCLIIDENT   Входящий запрос сессии не содержит идентификатора контрагента
		assert(rOtherPublic.Len());
		THROW_PP(rOtherPublic.Len(), PPERR_SQ_INPHASNTSESSPUBKEY);
		//assert(cli_ident.Len());
		THROW(Login());
		assert(P_Ic == 0);
		THROW_SL(P_Ic = new PPStyloQInterchange);
		THROW(P_Ic->SearchSession(rOtherPublic, &pack) > 0); 
		THROW_PP(pack.Pool.Get(SSecretTagPool::tagSessionPublicKey, &my_public), PPERR_SQ_INRSESSHASNTPUBKEY); // PPERR_SQ_INRSESSHASNTPUBKEY      Сохраненная сессия не содержит публичного ключа
		THROW_PP(pack.Pool.Get(SSecretTagPool::tagSessionPublicKeyOther, &temp_chunk), PPERR_SQ_INRSESSHASNTOTHERPUBKEY); // PPERR_SQ_INRSESSHASNTOTHERPUBKEY Сохраненная сессия не содержит публичного ключа контрагента
		THROW_PP(temp_chunk == rOtherPublic, PPERR_SQ_INRSESPUBKEYNEQTOOTR); // PPERR_SQ_INRSESPUBKEYNEQTOOTR   Публичный ключ сохраненной сессии не равен полученному от контрагента
		THROW_PP(pack.Pool.Get(SSecretTagPool::tagClientIdent, &temp_chunk), PPERR_SQ_INRSESSHASNTCLIIDENT); // PPERR_SQ_INRSESSHASNTCLIIDENT   Сохраненная сессия не содержит идентификатора контрагента
		THROW_PP(temp_chunk == CliIdent, PPERR_SQ_INRSESCLIIDENTNEQTOOTR); // PPERR_SQ_INRSESCLIIDENTNEQTOOTR Идентификатора контрагента сохраненной сессии не равен полученному от контрагента
		THROW_PP(pack.Pool.Get(SSecretTagPool::tagSessionSecret, &sess_secret), PPERR_SQ_INRSESSHASNTSECRET); // PPERR_SQ_INRSESSHASNTSECRET     Сохраненная сессия не содержит секрета 
		B.Sess.Put(SSecretTagPool::tagSessionSecret, sess_secret);
		p_sess_secret = &sess_secret;
		B.Other.Put(SSecretTagPool::tagClientIdent, CliIdent);
		THROW(rpe.SetupStyloQRpc(my_public, rOtherPublic, StartUp_Param.MqbInitParam.ConsumeParamList.CreateNewItem()));
		reply_tp.StartWriting(PPSCMD_SQ_SESSION, StyloQProtocol::psubtypeReplyOk);
		THROW(reply_tp.FinishWriting(0));
		THROW(p_mqbc = PPMqbClient::CreateInstance(StartUp_Param.MqbInitParam));
		PPStyloQInterchange::SetupMqbReplyProps(B, props); // @v11.3.1 
		THROW(p_mqbc->Publish(rpe.ExchangeName, rpe.RoutingKey, &props, reply_tp.constptr(), reply_tp.GetAvailableSize())); // @reply
		if(P_Ic->SetRoundTripBlockReplyValues(B, p_mqbc, rpe)) {
			p_mqbc = 0;
		}
		CATCHZOK
		ZDELETE(p_mqbc);
		return ok;
	}
	virtual void Run()
	{
		bool do_process_loop = false;
		SString temp_buf;
		SString log_buf;
		StyloQProtocol tp;
		StyloQProtocol reply_tp;
		SBinaryChunk other_public;
		PPMqbClient::MessageProperties props;
		THROW_PP(TPack.P.Get(SSecretTagPool::tagSessionPublicKey, &other_public), PPERR_SQ_INPHASNTSESSPUBKEY); // PPERR_SQ_INPHASNTSESSPUBKEY Входящий запрос сессии не содержит публичного ключа
		// @v11.3.1 {
		{
			SBinaryChunk bc_rti;
			if(TPack.P.Get(SSecretTagPool::tagRoundTripIdent, &bc_rti)) { 
				if(bc_rti.Len() == sizeof(S_GUID)) {
					memcpy(&B.Uuid, bc_rti.PtrC(), bc_rti.Len());
					{ // @debug
						log_buf.Z().Cat("mqb-sess-uuid-set").Space().Cat(GetThreadID()).Space().Cat(B.Uuid, S_GUID::fmtIDL|S_GUID::fmtLower);
						PPLogMessage(PPFILNAM_DEBUG_LOG, log_buf, LOGMSGF_TIME);
					}
				}
			}
		}
		// } @v11.3.1 
		//PPMqbClient::RoutingParamEntry rpe;
		//
		// Первая фаза обмена осуществляется в нешифрованном режиме. Это - этап взаимной идентификации и ключей шифрования у нас еще нет
		// либо мы не знаем какие использовать.
		// Здесь возможны команды: PPSCMD_SQ_ACQUAINTANCE PPSCMD_SQ_SRPAUTH PPSCMD_SQ_SESSION
		// Команда PPSCMD_SQ_SRPREGISTER здесь недопустима поскольку требует шифрования (передаются чувствительные данные).
		//
		switch(TPack.GetH().Type) {
			case PPSCMD_SQ_ACQUAINTANCE:
				if(Acquaintance(other_public)) {
					assert(B.Sess.Get(SSecretTagPool::tagSessionSecret, 0));
					do_process_loop = true; // OK
				}
				else {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO|LOGMSGF_THREADID);
				}
				break;
			case PPSCMD_SQ_SESSION: // Команда инициации соединения по значению сессии, которая была установлена на предыдущем сеансе обмена
				if(Session(other_public)) {
					assert(B.Sess.Get(SSecretTagPool::tagSessionSecret, 0));
					do_process_loop = true; // OK
				}
				else {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO|LOGMSGF_THREADID);
				}
				break;
			case PPSCMD_SQ_SRPAUTH: // Команда инициации соединения методом SRP-авторизации по параметрам, установленным ранее 
				if(SrpAuth(other_public)) {
					assert(B.Sess.Get(SSecretTagPool::tagSessionSecret, 0));
					do_process_loop = true; // OK
				}
				else {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO|LOGMSGF_THREADID);
				}
				break;
			default:
				// Недопустимая инициирующая команда
				break;
		}
		if(do_process_loop) {
			//
			// Если обмен по инициирующей команде прошел успешно, то сервис переходит в режим диалога с клиентом.
			// Этот режим длится пока клиент не перестанет присылать команды (факт того, что он "перестал" фиксируется по таймауту).
			// Завершение диалога приводит к ликвидации потока и связанных с ним ресурсов.
			//
			assert(P_Ic);
			if(P_Ic && P_Ic->GetOwnPeerEntry(&B.StP) > 0) {
				SBinaryChunk sess_secret;
				PPMqbClient::Envelope env;
				if(B.Sess.Get(SSecretTagPool::tagSessionSecret, &sess_secret)) {
					const long live_silence_period = (10 * 1000); // (2 * 60 * 1000)-->(15 * 1000)
					clock_t clk_start = clock();
					SString reply_status_text;
					while(do_process_loop) {
						assert(B.P_Mqbc && B.P_MqbRpe);
						bool is_there_my_message = false;
						if(B.P_Mqbc->ConsumeMessage(env, 1000) > 0) { // @v11.3.2 timeout 200-->1000
							if(!B.Uuid || S_GUID(env.Msg.Props.CorrelationId) == B.Uuid) {
								{ // @debug
									log_buf.Z().Cat("mqb-sess-got-own-msg").Space().Cat(env.Msg.Props.CorrelationId);
									PPLogMessage(PPFILNAM_DEBUG_LOG, log_buf, LOGMSGF_TIME|LOGMSGF_THREADID);
								}
								is_there_my_message = true;
							}
							else {
								{ // @debug
									log_buf.Z().Cat("mqb-sess-got-another's-msg").Space().Cat(env.Msg.Props.CorrelationId);
									PPLogMessage(PPFILNAM_DEBUG_LOG, log_buf, LOGMSGF_TIME|LOGMSGF_THREADID);
								}
								B.P_Mqbc->Reject(env.DeliveryTag, PPMqbClient::mqofRequeue);
								is_there_my_message = false;
							}
						}
						if(is_there_my_message) {
							clk_start = clock();
							B.P_Mqbc->Ack(env.DeliveryTag, 0);
							if(tp.Read(env.Msg, &sess_secret)) {
								if(tp.GetH().Type == PPSCMD_SQ_SRPREGISTER) {
									StyloQCore::StoragePacket cli_pack; // @v11.6.0
									int32 reply_status = 0;
									reply_status_text.Z();
									reply_tp.Z();
									if(P_Ic->Registration_ServiceReply(B, tp, &cli_pack)) {
										SBinaryChunk bc;
										SBinaryChunk own_ident;
										B.StP.Pool.Get(SSecretTagPool::tagSvcIdent, &own_ident); // @v11.3.8
										reply_tp.StartWriting(PPSCMD_SQ_SRPREGISTER, StyloQProtocol::psubtypeReplyOk);
										//
										// В случае успешной регистрации передаем клиенту наш лик и конфигурацию
										//
										if(B.StP.Pool.Get(SSecretTagPool::tagSelfyFace, &bc)) {
											assert(bc.Len());
											StyloQFace face_pack;
											SString my_face_json_buf;
											// @v11.3.8 {
											if(StyloQFace::MakeTransmissionJson(B.StP.Rec.ID, own_ident, bc.ToRawStr(temp_buf), my_face_json_buf)) { 
												bc.Put(my_face_json_buf.cptr(), my_face_json_buf.Len());
												reply_tp.P.Put(SSecretTagPool::tagFace, bc);
											}
											// } @v11.3.8 
											/* @v11.3.8 if(face_pack.FromJson(bc.ToRawStr(temp_buf)))
												reply_tp.P.Put(SSecretTagPool::tagFace, bc);*/
										}
										bc.Z();
										if(B.StP.Pool.Get(SSecretTagPool::tagConfig, &bc)) {
											assert(bc.Len());
											SString transmission_cfg_json;
											if(StyloQConfig::MakeTransmissionJson(bc.ToRawStr(temp_buf), &cli_pack, transmission_cfg_json)) {
												bc.Z().Put(transmission_cfg_json.cptr(), transmission_cfg_json.Len());
												reply_tp.P.Put(SSecretTagPool::tagConfig, bc);
											}
										}
										reply_status_text = "Wellcome!";
									}
									else {
										reply_tp.StartWriting(PPSCMD_SQ_SRPREGISTER, StyloQProtocol::psubtypeReplyError);
										reply_status_text = "Something went wrong";
									}
									{
										reply_tp.P.Put(SSecretTagPool::tagReplyStatus, &reply_status, sizeof(reply_status));
										if(reply_status_text.NotEmpty()) 
											reply_tp.P.Put(SSecretTagPool::tagReplyStatusText, reply_status_text.cptr(), reply_status_text.Len()+1);
									}
									reply_tp.FinishWriting(&sess_secret);
									props.Z();
									PPStyloQInterchange::SetupMqbReplyProps(B, props); // @v11.3.1 
									int pr = B.P_Mqbc->Publish(B.P_MqbRpe->ExchangeName, B.P_MqbRpe->RoutingKey, &props, reply_tp.constptr(), reply_tp.GetAvailableSize()); // @reply
								}
								else if(tp.GetH().Type == PPSCMD_SQ_COMMAND) {
									static const uint IntermediateReplyBlock_Signature = 0x19D93F2;
									class IntermediateReplyBlock {
									public:
										explicit IntermediateReplyBlock(PPStyloQInterchange::RoundTripBlock & rB) : Signature(IntermediateReplyBlock_Signature), R_B(rB)
										{
										}
										const uint Signature;
										PPStyloQInterchange::RoundTripBlock & R_B;
										//
										static int Proc(StyloQProtocol & rReplyPack, void * pExtra)
										{
											int ok = 1;
											IntermediateReplyBlock * p_this = static_cast<IntermediateReplyBlock *>(pExtra);
											if(p_this && p_this->Signature == IntermediateReplyBlock_Signature) {
												PPMqbClient::MessageProperties props;
												PPStyloQInterchange::SetupMqbReplyProps(p_this->R_B, props); // @v11.3.1 
												p_this->R_B.P_Mqbc->Publish(p_this->R_B.P_MqbRpe->ExchangeName, p_this->R_B.P_MqbRpe->RoutingKey, &props, rReplyPack.constptr(), rReplyPack.GetAvailableSize()); // @reply
											}
											else
												ok = 0;
											return ok;
										}
									};
									IntermediateReplyBlock irb(B);
									if(P_Ic->ProcessCmd(tp, CliIdent, &sess_secret, reply_tp, IntermediateReplyBlock::Proc, &irb)) {
										PPMqbClient::MessageProperties props;
										PPStyloQInterchange::SetupMqbReplyProps(B, props); // @v11.3.1 
										int pr = B.P_Mqbc->Publish(B.P_MqbRpe->ExchangeName, B.P_MqbRpe->RoutingKey, &props, reply_tp.constptr(), reply_tp.GetAvailableSize()); // @reply
										// @debug {
										if(pr) {
											props.Z();
										}
										// } @debug
									}
								}
							}
							else {
								PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_THREADID);
							}
							// @v11.4.4 {
							/*{
								(temp_buf = "Service Stylo-Q roundtrip is finished after getting of the command"); 
								PPLogMessage(PPFILNAM_INFO_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_THREADID);
								if(B.P_Mqbc) {
									B.P_Mqbc->Disconnect();
									ZDELETE(B.P_Mqbc);
								}
								break;
							}*/
							// } @v11.4.4 
						}
						else {
							const clock_t silence_period = (clock() - clk_start);
							if(silence_period >= live_silence_period) {
								(temp_buf = "Service Stylo-Q roundtrip is finished after").Space().Cat(silence_period).Cat("ms of silence");
								PPLogMessage(PPFILNAM_INFO_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_THREADID);
								// @v11.3.1 {
								if(B.P_Mqbc) {
									B.P_Mqbc->Disconnect();
									ZDELETE(B.P_Mqbc);
								}
								// } @v11.3.1 
								break;
							}
							SDelay(200);
						}
					}
				}
			}
		}
		CATCH
			;
		ENDCATCH
		// @v11.3.10 {
		if(B.P_Mqbc) {
			//B.P_Mqbc->QueueUnbind();
			B.P_Mqbc->Disconnect();
			ZDELETE(B.P_Mqbc);
		}
		// } @v11.3.10 
		ZDELETE(P_Ic);
		Logout();
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
			THROW(DS.Login(db_symb, PPSession::P_JobLogin, secret, PPSession::loginfSkipLicChecking|PPSession::loginfInternal));
			memzero(secret, sizeof(secret));
		}
		CATCHZOK
		return ok;
	}
	void   Logout()
	{
		DS.Logout();
	}
};
#if 1 // {

static int SetupCorrelationIdent(const PPMqbClient::Envelope & rMqbEnv, StyloQProtocol & rTPack)
{
	int    ok = 1;
	if(rMqbEnv.Msg.Props.CorrelationId.NotEmpty()) {
		S_GUID round_trip_uuid;
		SBinaryChunk bc_round_trip_ident;
		round_trip_uuid.FromStr(rMqbEnv.Msg.Props.CorrelationId);
		if(rTPack.P.Get(SSecretTagPool::tagRoundTripIdent, &bc_round_trip_ident)) {
			assert(bc_round_trip_ident.Len() > 0);
			bool round_trip_ident_match = true;
			if(bc_round_trip_ident.Len() > 0) {
				if(bc_round_trip_ident.Len() == sizeof(S_GUID)) {
					S_GUID round_trip_uuid2;
					memcpy(&round_trip_uuid2, bc_round_trip_ident.PtrC(), bc_round_trip_ident.Len());
					if(round_trip_uuid2 != round_trip_uuid)
						round_trip_ident_match = false;
				}
				else
					round_trip_ident_match = false;
			}
			if(!round_trip_ident_match)
				ok = 0;
		}
		else
			rTPack.P.Put(SSecretTagPool::tagRoundTripIdent, &round_trip_uuid, sizeof(round_trip_uuid));
	}
	else
		ok = -1;
	return ok;
}

int StyloQCore::IndexingContent()
{
	int    ok = -1;
	SString temp_buf;
	StyloQCore::StoragePacket sp;
	StyloQConfig cfg_pack;
	SBinaryChunk bin_chunk;
	THROW(GetOwnPeerEntry(&sp) > 0);
	THROW_PP(sp.Pool.Get(SSecretTagPool::tagConfig, &bin_chunk), PPERR_SQ_UNDEFOWNCFG);
	THROW(cfg_pack.FromJson(bin_chunk.ToRawStr(temp_buf)));
	if(cfg_pack.GetFeatures() & StyloQConfig::featrfMediator) {
		PPIDArray doc_id_list;
		if(GetUnprocessedIndexindDocIdList(doc_id_list) > 0) {
			assert(doc_id_list.getCount()); // because GetDocIdListByType() > 0 

			PPTextAnalyzer txa;
			STokenizer::Param txa_param;
			txa_param.Cp = cpUTF8;
			txa_param.Delim = " \t\n\r(){}[]<>,.:;-\\/&$#@!?*^\"+=%\xA0";
			txa_param.Flags |= (STokenizer::fDivAlNum|STokenizer::fEachDelim);
			txa.SetParam(&txa_param);

			PPFtsInterface fts_db(true/*forUpdate*/, 120000);
			THROW(fts_db);
			for(uint didx = 0; didx < doc_id_list.getCount(); didx++) {
				const PPID doc_id = doc_id_list.get(didx);
				StyloQCore::StoragePacket doc_pack;
				THROW(GetPeerEntry(doc_id, &doc_pack) > 0);
				if(doc_pack.Rec.Flags & StyloQCore::fUnprocessedDoc_Misplaced) {
					if(doc_pack.Pool.Get(SSecretTagPool::tagRawData, &bin_chunk)) {
						bin_chunk.ToRawStr(temp_buf);
						PPFtsInterface::TransactionHandle tra(fts_db);
						bool   is_local_fault = true;
						if(!!tra) {
							if(IndexingContent_Json(&tra, &txa, temp_buf)) {
								if(tra.Commit()) {
									doc_pack.Rec.Flags &= ~StyloQCore::fUnprocessedDoc_Misplaced;
									PPID temp_id = doc_id;
									THROW(PutPeerEntry(&temp_id, &doc_pack, 1));
									is_local_fault = false;
								}
							}
						}
						if(is_local_fault) {
							PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

//
// Descr: Вариант централизованного сервера, обслуживающего MQB-запросы. В этом варианте сервер может обслуживать запросы
//   к нескольким сервисам (базам данных), в отличии от локального Mqb-сервера, который может обслуживать локальные запросы
//   в пределах одной базы данных (или, даже, сеанса).
//
class StyloQServer2 : public PPThread {
public:
	struct LaunchEntry {
		PPStyloQInterchange::RunServerParam P;
		DbLoginBlock LB;
	};
private:
	TSCollection <LaunchEntry> Entries;
	class RunBlock {
	public:
		struct Entry {
			Entry(const LaunchEntry * pLe, uint connIdx) : /*P_Cli(0),*/ ConnIdx(connIdx)
			{
				if(pLe) {
					LB = pLe->LB;
					P = pLe->P;
				}
			}
			const uint ConnIdx; // Позиция [1..] MQB-клиента в списке RunBlock::ConnList
			PPStyloQInterchange::RunServerParam P;
			DbLoginBlock LB;
		};
		RunBlock & Z()
		{
			EntryList.freeAll();
			ConnList.freeAll();
			return *this;
		}
		TSCollection <Entry> EntryList;
		TSCollection <PPMqbClient> ConnList;
		LongArray DeadConnIdxList; // Список индексов ConnList, содержащих "мертвые" соединения - их надо пытаться перезапустить время от времени
	};
	int    InitializeConnections(RunBlock & rRunBlk, bool tryToReconnectOnly)
	{
		int    ok = 0;
		if(tryToReconnectOnly) {
			ok = 0;
			for(uint j = 0; j < rRunBlk.ConnList.getCount(); j++) {
				PPMqbClient * p_conn = rRunBlk.ConnList.at(j);
				if(p_conn && p_conn->GetExtStatusFlag(PPMqbClient::extsfTryToReconnect)) {
					for(uint eidx = 0; eidx < rRunBlk.EntryList.getCount(); eidx++) {
						RunBlock::Entry * p_e = rRunBlk.EntryList.at(eidx);
						if(p_e && p_e->ConnIdx == (j+1)) {
							PPMqbClient * p_new_conn = PPMqbClient::CreateInstance(p_e->P.MqbInitParam);
							if(p_new_conn) {
								ZDELETE(p_conn); // Attention! Разрушаем элемент rRunBlk.ConnList по индексу j - далее сразу заменяем его на новый!
								rRunBlk.ConnList.atPut(j, p_new_conn);
								//
								// Для всех точек входа, использующих это же соединение, делаем необходимые манипуляции 
								// 
								for(uint eidx2 = eidx+1; eidx2 < rRunBlk.EntryList.getCount(); eidx2++) {
									RunBlock::Entry * p_e2 = rRunBlk.EntryList.at(eidx2);
									if(p_e2 && p_e2->ConnIdx == (j+1)) {
										if(p_e2->P.MqbInitParam.ConsumeParamList.getCount()) {
											for(uint cplidx = 0; cplidx < p_e2->P.MqbInitParam.ConsumeParamList.getCount(); cplidx++) {
												PPMqbClient::RoutingParamEntry * p_rpe = p_e2->P.MqbInitParam.ConsumeParamList.at(cplidx);
												if(p_rpe) {
													p_new_conn->ApplyRoutingParamEntry(*p_rpe);
													p_new_conn->Consume(p_rpe->QueueName, &p_rpe->ConsumeTag.Z(), 0);
												}
											}
										}
									}
								}
								ok |= 1;
							}
							else {
								ok |= 2;
								PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP);
							}
							break; // necessarily!
						}
					}
				}
			}
		}
		else {
			rRunBlk.Z();
			// @v11.7.8 ok = 1;
			for(uint i = 0; i < Entries.getCount(); i++) {
				const LaunchEntry * p_le = Entries.at(i);
				if(p_le) {
					uint   conn_idx = 0;
					for(uint j = 0; !conn_idx && j < rRunBlk.ConnList.getCount(); j++) {
						const PPMqbClient * p_mqb_cli = rRunBlk.ConnList.at(j);
						if(p_mqb_cli && p_mqb_cli->IsHostEqual(p_le->P.MqbInitParam.Host, p_le->P.MqbInitParam.Port))
							conn_idx = j+1;							
					}
					if(conn_idx) {
						RunBlock::Entry * p_new_entry = new RunBlock::Entry(p_le, conn_idx);
						PPMqbClient * p_mqb_cli = rRunBlk.ConnList.at(conn_idx-1);
						if(p_le->P.MqbInitParam.ConsumeParamList.getCount()) {
							for(uint cplidx = 0; cplidx < p_le->P.MqbInitParam.ConsumeParamList.getCount(); cplidx++) {
								PPMqbClient::RoutingParamEntry * p_rpe = p_le->P.MqbInitParam.ConsumeParamList.at(cplidx);
								if(p_rpe) {
									p_mqb_cli->ApplyRoutingParamEntry(*p_rpe);
									p_mqb_cli->Consume(p_rpe->QueueName, &p_rpe->ConsumeTag.Z(), 0);
								}
							}
						}
						rRunBlk.EntryList.insert(p_new_entry);
						ok = 1; // @v11.7.8
					}
					else {
						// Инициализация MQB exchange
						//p_mqb_cli->ExchangeDeclare("styloqrpc", PPMqbClient::exgtDirect, 0);
						PPMqbClient * p_mqb_cli = PPMqbClient::CreateInstance(p_le->P.MqbInitParam);
						if(p_mqb_cli) {
							if(rRunBlk.ConnList.insert(p_mqb_cli)) {
								conn_idx = rRunBlk.ConnList.getCount();
								RunBlock::Entry * p_new_entry = new RunBlock::Entry(p_le, conn_idx);
								rRunBlk.EntryList.insert(p_new_entry);
								ok = 1;
							}
						}
						else {
							PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP);
						}
					}
				}
			}
		}
		return ok;
	}
public:
	StyloQServer2(const TSCollection <LaunchEntry> & rEntries) : PPThread(kStyloQServer, 0, 0)
	{
		TSCollection_Copy(Entries, rEntries);
	}
	virtual void Run()
	{
		const int   do_debug_log = 0; // @debug
		const long  pollperiod_mqc = 500;
		const long  try_to_reconnect_period = 60000; // Таймаут для попытки восствановить соединение (ms)
		clock_t last_reconnect_try_clock = 0;
		EvPollTiming pt_mqc(pollperiod_mqc, false);
		EvPollTiming pt_purge(3600000, true); // этот тайминг не надо исполнять при запуске. Потому registerImmediate = 1
		const int  use_sj_scan_alg2 = 0;
		SString msg_buf, temp_buf;
		RunBlock run_blk;
		if(InitializeConnections(run_blk, false) > 0) {
			assert(run_blk.EntryList.getCount());
			PPMqbClient::Envelope mqb_envelop;
			const long __cycle_hs = 37; // Период таймера в сотых долях секунды (37)
			int    queue_stat_flags_inited = 0;
			StyloQProtocol tpack;
			Evnt   stop_event(SLS.GetStopEventName(temp_buf), Evnt::modeOpen);
			for(int stop = 0; !stop;) {
				if(last_reconnect_try_clock && (clock() - last_reconnect_try_clock) >= try_to_reconnect_period) {
					const int icr = InitializeConnections(run_blk, true);
					if(icr & 2) { // Не все соединения удалось восстановить
						last_reconnect_try_clock = clock();
					}
					else {
						last_reconnect_try_clock = 0;
					}
				}
				uint   h_count = 0;
				HANDLE h_list[32];
				h_list[h_count++] = stop_event;
				h_list[h_count++] = EvLocalStop;
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
					case (WAIT_OBJECT_0 + 1): // local stop event
						if(do_debug_log) {
							PPLogMessage(PPFILNAM_DEBUG_AEQ_LOG, "LocalStopEvent", LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
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
					case (WAIT_OBJECT_0 + 2): // __timer event
						if(pt_purge.IsTime()) {
							;
						}
						if(pt_mqc.IsTime()) {
							for(uint connidx = 0; connidx < run_blk.ConnList.getCount(); connidx++) {
								PPMqbClient * p_mqb_cli = run_blk.ConnList.at(connidx);
								if(p_mqb_cli && !p_mqb_cli->GetExtStatusFlag(PPMqbClient::extsfTryToReconnect)) {
									bool local_mqb_err = false;
									for(bool consume_fault = false; !consume_fault;) {
										const int cmr = p_mqb_cli->ConsumeMessage(mqb_envelop, 200);
										if(cmr > 0) {
											p_mqb_cli->Ack(mqb_envelop.DeliveryTag, 0);
											if(tpack.Read(mqb_envelop.Msg, 0)) {
												bool local_error = false;
												SString a("debug"); // @debug
												SBinaryChunk msg_svc_ident; // Идент сервиса, переданный с прикладным сообщение
												SBinaryChunk evn_svc_ident; // Идент сервиса, полученный из mqb_envelop.RoutingKey 
												if(mqb_envelop.RoutingKey.NotEmpty()) {
													evn_svc_ident.FromMime64(mqb_envelop.RoutingKey);
												}
												// @v11.3.1 {
												if(!SetupCorrelationIdent(mqb_envelop, tpack))
													local_error = true;
												// } @v11.3.1 
												tpack.P.Get(SSecretTagPool::tagSvcIdent, &msg_svc_ident);
												const RunBlock::Entry * p_target_entry = 0;
												for(uint j = 0; !p_target_entry && j < run_blk.EntryList.getCount(); j++) {
													const RunBlock::Entry * p_entry = run_blk.EntryList.at(j);
													if(p_entry) {
														if(msg_svc_ident.Len()) {
															if(p_entry->P.SvcIdent == msg_svc_ident)
																p_target_entry = p_entry;
														}
														else if(evn_svc_ident.Len()) {
															if(p_entry->P.SvcIdent == evn_svc_ident)
																p_target_entry = p_entry;
														}
													}
												}
												if(p_target_entry) {
													StyloQServerSession * p_new_sess = new StyloQServerSession(p_target_entry->LB, p_target_entry->P, tpack);
													CALLPTRMEMB(p_new_sess, Start(0));													
												}
											}
											else
												PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP);
										}
										else {
											consume_fault = true;
											if(cmr == 0) {
												const int err_code = SLibError;
												PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP);
												local_mqb_err = true;
												// @v11.4.8 SLERR_AMQP_UNEXPECTED_STATE
												if(oneof4(err_code, SLERR_AMQP_SOCKET, SLERR_AMQP_CONNECTION_CLOSED, SLERR_AMQP_SOCKET_CLOSED, SLERR_AMQP_UNEXPECTED_STATE)) {
													p_mqb_cli->SetExtStatusFlag(PPMqbClient::extsfTryToReconnect);
													last_reconnect_try_clock = clock();
												}
												// AMQP_STATUS_SOCKET_ERROR-->SLERR_AMQP_SOCKET
												// AMQP_STATUS_CONNECTION_CLOSED-->SLERR_AMQP_CONNECTION_CLOSED 
												// AMQP_STATUS_SOCKET_CLOSED-->SLERR_AMQP_SOCKET_CLOSED 
												// AMQP_STATUS_UNEXPECTED_STATE-->SLERR_AMQP_UNEXPECTED_STATE
											}
										}
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
		else {
			PPLogMessage(PPFILNAM_SERVER_LOG, PPLoadTextS(PPTXT_STYLOQMQBSVR_NOTHINGTODO, SLS.AcquireRvlStr()), LOGMSGF_TIME);
		}
		//delete p_mqb_cli;
		DBS.CloseDictionary();
	}
};

void RunStyloQMqbServer()
{
	int    ok = 1;
	StyloQCore::SvcDbSymbMap dbmap;
	SBinaryChunk bc;
	SBinaryChunk cfg_bytes;
	SString temp_buf;
	SString url_buf;
	SString mqb_auth_buf;
	SString mqb_secr_buf;
	SString msg_buf;
	PPDbEntrySet2 dbes;
	PPLogMessage(PPFILNAM_SERVER_LOG, PPLoadTextS(PPTXT_LOG_STQMQBSERVERSTARTING, msg_buf), LOGMSGF_TIME|LOGMSGF_COMP);
	{
		PPIniFile ini_file;
		THROW(ini_file.IsValid());
		THROW(dbes.ReadFromProfile(&ini_file, 0));
	}
	if(StyloQCore::GetDbMap(dbmap)) {
		TSCollection <StyloQServer2::LaunchEntry> ll;
		// Так как среди точек входа в базу данных могут быть дубликаты (разные точки, но база одна), то
		// элиминируем эти дубликаты по значению service-ident. Следующий список как раз для этого и предназначен.
		TSCollection <SBinaryChunk> reckoned_svc_id_list;
		for(uint i = 0; i < dbmap.getCount(); i++) {
			const StyloQCore::SvcDbSymbMapEntry * p_map_entry = dbmap.at(i);
			if(p_map_entry && p_map_entry->DbSymb.NotEmpty() && p_map_entry->SvcIdent.Len()) {
				StyloQCore::StoragePacket sp;
				PPSession::LimitedDatabaseBlock * p_ldb = DS.LimitedOpenDatabase(p_map_entry->DbSymb, PPSession::lodfReference|PPSession::lodfStyloQCore|PPSession::lodfSysJournal);
				if(!p_ldb) {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_COMP|LOGMSGF_TIME);
				}
				else if(p_ldb->P_Sqc->GetOwnPeerEntry(&sp) <= 0) {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_COMP|LOGMSGF_TIME);
				}
				else if(!sp.Pool.Get(SSecretTagPool::tagSvcIdent, &bc)) {
					PPSetError(PPERR_SQ_UNDEFOWNSVCID);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_COMP|LOGMSGF_TIME);
				}
				else {
					bool svc_id_reckoned = false;
					for(uint rslidx = 0; !svc_id_reckoned && rslidx < reckoned_svc_id_list.getCount(); rslidx++) {
						if(*reckoned_svc_id_list.at(rslidx) == bc)
							svc_id_reckoned = true;
					}
					if(!svc_id_reckoned) {
						if(sp.Pool.Get(SSecretTagPool::tagConfig, &cfg_bytes) && cfg_bytes.Len()) {
							{
								//
								// Здесь запустим авторегистрацию сервисов у медиаторов независимо от того, какой протокол этот сервис использует
								// Эта точка кода выбрана потому, что здесь уже известны: символ базы данных, ограниченная сессия p_ldb, наличие
								//   конфигурации у сервиса.
								//
								PPStyloQInterchange ic(p_ldb->P_Sqc);
								ic.ServiceSelfregisterInMediator(sp, p_ldb->P_Sj);
							}
							StyloQConfig _cfg;
							if(_cfg.FromJson(cfg_bytes.ToRawStr(temp_buf))) {
								const long role = _cfg.GetRole();
								_cfg.Get(StyloQConfig::tagUrl, url_buf);
								_cfg.Get(StyloQConfig::tagMqbAuth, mqb_auth_buf);
								_cfg.Get(StyloQConfig::tagMqbSecret, mqb_secr_buf);
								if(oneof2(role, StyloQConfig::rolePublicService, StyloQConfig::rolePrivateService) && url_buf.NotEmptyS() && mqb_auth_buf.NotEmptyS()) {
									InetUrl url(url_buf);
									const int prot = url.GetProtocol();
									if(oneof2(prot, InetUrl::protAMQP, InetUrl::protAMQPS) || prot == 0) {
										DbLoginBlock local_dblb;
										const int db_id = dbes.GetBySymb(p_map_entry->DbSymb, &local_dblb);
										if(db_id > 0) {
											PPStyloQInterchange::RunServerParam rsp;
											PPMqbClient::RoutingParamEntry rpe;
											if(PPStyloQInterchange::SetupMqbParam(sp, PPStyloQInterchange::smqbpfInitAccessPoint, rsp)) {
												rsp.MakeMqbQueueIdent(bc);
												if(rpe.SetupStyloQRpcListener(/*rsparam.SvcIdent*/bc)) {
													PPMqbClient::RoutingParamEntry * p_rp_entry = rsp.MqbInitParam.ConsumeParamList.CreateNewItem();
													if(p_rp_entry) {
														*p_rp_entry = rpe;
														StyloQServer2::LaunchEntry * p_new_ll_entry = ll.CreateNewItem();
														p_new_ll_entry->LB = local_dblb;
														p_new_ll_entry->P = rsp;
														p_new_ll_entry->P.SvcIdent.Mime64(temp_buf);
														SBinaryChunk * p_reckoned_svc_id = reckoned_svc_id_list.CreateNewItem();
														ASSIGN_PTR(p_reckoned_svc_id, p_new_ll_entry->P.SvcIdent);
														// @v11.3.10 (msg_buf = "Stylo-Q MQB server's entry-point was created").CatDiv(':', 2).CatEq("db", p_map_entry->DbSymb).Space().
														// @v11.3.10 	CatEq("host", p_new_ll_entry->P.MqbInitParam.Host).Space().CatEq("svcid", temp_buf);
														PPLoadText(PPTXT_LOG_STQENTRYPOINTCREATED, msg_buf); // @v11.3.10 
														msg_buf.CatDiv(':', 2).CatEq("db", p_map_entry->DbSymb).Space().
															CatEq("host", p_new_ll_entry->P.MqbInitParam.Host).Space().CatEq("svcid", temp_buf); // @v11.3.10
														PPLogMessage(PPFILNAM_SERVER_LOG, msg_buf, LOGMSGF_TIME);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
				ZDELETE(p_ldb); // @v11.4.3 @fix
			}
		}
		if(ll.getCount()) {
			StyloQServer2 * p_srv = new StyloQServer2(ll);
			if(p_srv) {
				p_srv->Start(0);
				PPLogMessage(PPFILNAM_SERVER_LOG, PPLoadTextS(PPTXT_LOG_STQMQBSERVERSTARTED, msg_buf), LOGMSGF_TIME|LOGMSGF_COMP);
			}
		}
		else {
			// @v11.3.10 msg_buf = "No Stylo-Q MQB server's entry-points were found";
			PPLogMessage(PPFILNAM_SERVER_LOG, PPLoadTextS(PPTXT_LOG_STQMQBSVRENTPTNOTFOUND, msg_buf), LOGMSGF_TIME|LOGMSGF_COMP);
		}
	}
	CATCHZOK
}
#endif // 

/*static*/int PPStyloQInterchange::StopStyloQServer()
{
	int    ok = -1;
	LongArray ex_list;
	DS.GetThreadListByKind(PPThread::kStyloQServer, ex_list);
	if(ex_list.getCount()) {
		for(uint i = 0; i < ex_list.getCount(); i++) {
			const ThreadID tid = ex_list.get(i);
			DS.StopThread(tid);
			ok = 1;
		}
	}
	return ok;
}

uint PPStyloQInterchange::GetNominalSessionLifeTimeSec() const
{
	return 3600;
}

int PPStyloQInterchange::RunStyloQLocalMqbServer(RunServerParam & rP, const DbLoginBlock * pDlb)
{
	class StyloQServer : public PPThread {
		PPStyloQInterchange::RunServerParam P;
		DbLoginBlock LB;
	public:
		StyloQServer(const DbLoginBlock & rLB, const PPStyloQInterchange::RunServerParam & rP) : PPThread(kStyloQServer, 0, 0), P(rP), LB(rLB)
		{
		}
		virtual void Run()
		{
			const bool  do_debug_log = false; // @debug
			const long  pollperiod_mqc = 500;
			EvPollTiming pt_mqc(pollperiod_mqc, false);
			EvPollTiming pt_purge(3600000, true); // этот тайминг не надо исполнять при запуске. Потому registerImmediate = 1
			const int  use_sj_scan_alg2 = 0;
			SString msg_buf;
			SString temp_buf;
			PPMqbClient * p_mqb_cli = PPMqbClient::CreateInstance(P.MqbInitParam); // @v11.0.9
			if(p_mqb_cli) {
				PPMqbClient::Envelope mqb_envelop;
				const long __cycle_hs = (p_mqb_cli ? 37 : 293); // Период таймера в сотых долях секунды (37)
				StyloQProtocol tpack;
				Evnt   stop_event(SLS.GetStopEventName(temp_buf), Evnt::modeOpen);
				for(int stop = 0; !stop;) {
					uint   h_count = 0;
					HANDLE h_list[32];
					h_list[h_count++] = stop_event;
					h_list[h_count++] = EvLocalStop;
					//
					STimer __timer; // Таймер для отмера времени до следующего опроса источников событий
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
						case (WAIT_OBJECT_0 + 1): // local stop event
							if(do_debug_log) {
								PPLogMessage(PPFILNAM_DEBUG_AEQ_LOG, "LocalStopEvent", LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
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
						case (WAIT_OBJECT_0 + 2): // __timer event
							{
								if(pt_purge.IsTime()) {
									;
								}
								if(p_mqb_cli && pt_mqc.IsTime()) {
									while(p_mqb_cli->ConsumeMessage(mqb_envelop, 200) > 0) {
										p_mqb_cli->Ack(mqb_envelop.DeliveryTag, 0);
										if(tpack.Read(mqb_envelop.Msg, 0)) {
											// @v11.3.3 {
											if(!SetupCorrelationIdent(mqb_envelop, tpack)) {
												;//local_error = true;
											}
											// } @v11.3.3
											StyloQServerSession * p_new_sess = new StyloQServerSession(LB, P, tpack);
											CALLPTRMEMB(p_new_sess, Start(0));
										}
										else {
											PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP);
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
	int    ok = 1;
	PPID   own_peer_id = 0;
	SString temp_buf;
	LongArray ex_list;
	DbLoginBlock local_dlb;
	DS.GetThreadListByKind(PPThread::kStyloQServer, ex_list);
	if(!ex_list.getCount()) {
		if(!pDlb) {
			DbProvider * p_dict = CurDict;
			if(p_dict) {
				PPIniFile ini_file;
				PPDbEntrySet2 dbes;
				int    db_id = 0;
				p_dict->GetDbSymb(temp_buf);
				THROW(ini_file.IsValid());
				THROW(dbes.ReadFromProfile(&ini_file, 0));
				THROW_SL(db_id = dbes.GetBySymb(temp_buf, &local_dlb));
				pDlb = &local_dlb;
			}
		}
		if(pDlb) {
			StyloQServer * p_srv = new StyloQServer(*pDlb, rP);
			CALLPTRMEMB(p_srv, Start(0));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

#if 0 // @model {
static int _EcdhCryptModelling()
{
	//#define NISTP256 NID_X9_62_prime256v1
	//#define NISTP384 NID_secp384r1
	//#define NISTP521 NID_secp521r1
	int    ok = 1;
	BN_CTX * p_bn_ctx = BN_CTX_new();
	EC_KEY * p_key_cli = 0;
	EC_KEY * p_key_svc = 0;
	EC_KEY * p_key_cli2 = 0;
	EC_KEY * p_key_svc2 = 0;
	uchar * p_secret_cli = 0;
	uchar * p_secret_svc = 0;
	SBinaryChunk secret_cli2;
	SBinaryChunk secret_svc2;
	const EC_POINT * p_public_cli = 0;
	const EC_POINT * p_public_svc = 0;
	SBinaryChunk private_key_cli;
	SBinaryChunk public_key_cli;
	//SBinaryChunk public_key_bn_cli;
	SBinaryChunk private_key_svc;
	SBinaryChunk public_key_svc;
	//SBinaryChunk public_key_bn_svc;
	size_t secret_len_cli = 0;
	size_t secret_len_svc = 0;
	{
		THROW(p_key_cli = EC_KEY_new_by_curve_name(ec_curve_name_id)); // Failed to create key curve
		THROW(EC_KEY_generate_key(p_key_cli) == 1); // Failed to generate key
		{
			const EC_GROUP * p_ecg = EC_KEY_get0_group(p_key_cli);
			const BIGNUM   * p_private_bn = EC_KEY_get0_private_key(p_key_cli);
			THROW(p_private_bn);
			int bn_len = BN_num_bytes(p_private_bn);
			THROW(private_key_cli.Ensure(bn_len));
			BN_bn2bin(p_private_bn, static_cast<uchar *>(private_key_cli.Ptr()));
		}
	}	
	{
		THROW(p_key_svc = EC_KEY_new_by_curve_name(ec_curve_name_id)); // Failed to create key curve
		THROW(EC_KEY_generate_key(p_key_svc) == 1); // Failed to generate key
		{
			const EC_GROUP * p_ecg = EC_KEY_get0_group(p_key_svc);
			const BIGNUM   * p_private_bn = EC_KEY_get0_private_key(p_key_svc);
			THROW(p_private_bn);
			int bn_len = BN_num_bytes(p_private_bn);
			THROW(private_key_svc.Ensure(bn_len));
			BN_bn2bin(p_private_bn, static_cast<uchar *>(private_key_svc.Ptr()));
		}
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
	//uchar * get_secret(EC_KEY *key, const EC_POINT *peer_pub_key, size_t *secret_len)
	{
		EC_GROUP * p_ecg2 = EC_GROUP_new_by_curve_name(ec_curve_name_id);
		EC_POINT * p_public_ecpt = EC_POINT_new(p_ecg2);
		EC_POINT_oct2point(p_ecg2, p_public_ecpt, static_cast<uchar *>(public_key_svc.Ptr()), public_key_svc.Len(), p_bn_ctx);
		{
			const int field_size = EC_GROUP_get_degree(EC_KEY_get0_group(p_key_cli));
			secret_len_cli = (field_size + 7) / 8;
			THROW(p_secret_cli = (uchar *)OPENSSL_malloc(secret_len_cli)); // Failed to allocate memory for secret
			secret_len_cli = ECDH_compute_key(p_secret_cli, secret_len_cli, p_public_ecpt, p_key_cli, NULL);
			THROW(secret_len_cli > 0);
		}
		{
			//
			// Здесь тестируем генерацию секрета с использованием приватного ключа, полученного из бинарного представления private_key_cli (сформированного выше)
			//
			EC_KEY * p_synt_key = EC_KEY_new_by_curve_name(ec_curve_name_id);
			const EC_GROUP * p_synt_ecg2 = EC_KEY_get0_group(p_synt_key);
			BIGNUM * p_private_bn = BN_new();
			THROW(BN_bin2bn(static_cast<const uchar *>(private_key_cli.PtrC()), private_key_cli.Len(), p_private_bn));
			THROW(EC_KEY_set_private_key(p_synt_key, p_private_bn) == 1);
			const int field_size = EC_GROUP_get_degree(p_synt_ecg2);
			size_t sl = (field_size + 7) / 8;
			secret_cli2.Ensure(sl);
			sl = ECDH_compute_key(secret_cli2.Ptr(), secret_cli2.Len(), p_public_ecpt, p_synt_key, NULL);
			BN_free(p_private_bn);
			EC_KEY_free(p_synt_key);
		}
		EC_POINT_free(p_public_ecpt);
		EC_GROUP_free(p_ecg2);
		assert(secret_cli2.Len() == secret_len_cli);
		assert(memcmp(secret_cli2.PtrC(), p_secret_cli, secret_len_cli) == 0);
	}
	{
		EC_GROUP * p_ecg2 = EC_GROUP_new_by_curve_name(ec_curve_name_id);
		EC_POINT * p_public_ecpt = EC_POINT_new(p_ecg2);
		EC_POINT_oct2point(p_ecg2, p_public_ecpt, static_cast<uchar *>(public_key_cli.Ptr()), public_key_cli.Len(), p_bn_ctx);
		{
			const int field_size = EC_GROUP_get_degree(EC_KEY_get0_group(p_key_svc));
			secret_len_svc = (field_size + 7) / 8;
			THROW(p_secret_svc = (uchar *)OPENSSL_malloc(secret_len_svc)); // Failed to allocate memory for secret
			secret_len_svc = ECDH_compute_key(p_secret_svc, secret_len_svc, p_public_ecpt, p_key_svc, NULL);
			THROW(secret_len_svc > 0);
		}
		{
			//
			// Здесь тестируем генерацию секрета с использованием приватного ключа, полученного из бинарного представления private_key_svc (сформированного выше)
			//
			EC_KEY * p_synt_key = EC_KEY_new_by_curve_name(ec_curve_name_id);
			const EC_GROUP * p_synt_ecg2 = EC_KEY_get0_group(p_synt_key);
			BIGNUM * p_private_bn = BN_new();
			THROW(BN_bin2bn(static_cast<const uchar *>(private_key_svc.PtrC()), private_key_svc.Len(), p_private_bn));
			THROW(EC_KEY_set_private_key(p_synt_key, p_private_bn) == 1);
			const int field_size = EC_GROUP_get_degree(p_synt_ecg2);
			size_t sl = (field_size + 7) / 8;
			secret_svc2.Ensure(sl);
			sl = ECDH_compute_key(secret_svc2.Ptr(), secret_svc2.Len(), p_public_ecpt, p_synt_key, NULL);
			BN_free(p_private_bn);
			EC_KEY_free(p_synt_key);
		}
		EC_POINT_free(p_public_ecpt);
		EC_GROUP_free(p_ecg2);
		assert(secret_svc2.Len() == secret_len_svc);
		assert(memcmp(secret_svc2.PtrC(), p_secret_svc, secret_len_svc) == 0);
	}
	{
		assert(secret_len_cli == secret_len_svc);
		for(size_t i = 0; i < secret_len_cli; i++)
			assert(p_secret_cli[i] == p_secret_svc[i]);
	}
	CATCHZOK
	EC_KEY_free(p_key_cli);
	EC_KEY_free(p_key_svc);
	EC_KEY_free(p_key_cli2);
	EC_KEY_free(p_key_svc2);
	OPENSSL_free(p_secret_cli);
	OPENSSL_free(p_secret_svc);
	BN_CTX_free(p_bn_ctx);
	return ok;
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
#endif // } @model

int PPStyloQInterchange::ExecuteInvitationDialog(InterchangeParam & rData)
{
	class StQInvDialog : public TDialog {
	public:
		StQInvDialog(const char * pInvitation) : TDialog(DLG_STQINV), Invitation(pInvitation)
		{
			assert(Invitation.NotEmpty());
			TImageView * p_iv = static_cast<TImageView *>(getCtrlView(CTL_STQINV_QR));
			if(p_iv && p_iv->IsSubSign(TV_SUBSIGN_IMAGEVIEW)) {
				HWND h_iv = p_iv->getHandle();
				if(h_iv) {
					RECT img_rect;
					::GetClientRect(h_iv, &img_rect);
					PPBarcode::BarcodeImageParam bcip;
					bcip.Code = Invitation;
					bcip.Std = BARCSTD_QR;
					bcip.ColorFg = SClrBlack;
					bcip.ColorBg = SClrWhite;
					bcip.Size.Set(img_rect.right-img_rect.left, img_rect.bottom-img_rect.top);
					if(PPBarcode::CreateImage(bcip)) {
						SDrawFigure * p_fig = new SDrawImage(bcip.Buffer);
						p_iv->SetOuterFigure(p_fig);
					}
				}
			}
		}
	private:
		SString Invitation;
	};
	int    ok = -1;
	StyloQCore::StoragePacket sp;
	SString inv_code;
	PPID   own_peer_id = 0;
	const  int spir = SetupPeerInstance(&own_peer_id, 1);
	if(spir) {
		THROW(MakeInvitation(rData, inv_code));
		StQInvDialog * dlg = new StQInvDialog(inv_code);
		if(CheckDialogPtr(&dlg))
			ExecViewAndDestroy(dlg);
	}
	CATCHZOKPPERR
	return ok;
}

/*static*/SString & PPStyloQInterchange::MakeBlobSignature(const SBinaryChunk & rOwnIdent, PPObjID oid, uint itemNumber, SString & rBuf)
{
	rBuf.Z();
	const uint32 inner_file_number = static_cast<uint32>(itemNumber);
	SBinaryChunk bc_sign;
	bc_sign.Cat(rOwnIdent);
	bc_sign.Cat(&oid, sizeof(oid));
	bc_sign.Cat(&inner_file_number, sizeof(inner_file_number));
	const binary128 sign = SlHash::Md5(0, bc_sign.PtrC(), bc_sign.Len());
	Base32_Encode(reinterpret_cast<const uint8 *>(&sign), sizeof(sign), rBuf);
	while(rBuf.Last() == '=')
		rBuf.TrimRight();
	return rBuf;
}

/*static*/SString & PPStyloQInterchange::MakeBlobSignature(const SBinaryChunk & rOwnIdent, const char * pResourceName, SString & rBuf)
{
	rBuf.Z();
	SBinaryChunk bc_sign;
	bc_sign.Cat(rOwnIdent);
	bc_sign.Cat(pResourceName, sstrlen(pResourceName));
	const binary128 sign = SlHash::Md5(0, bc_sign.PtrC(), bc_sign.Len());
	Base32_Encode(reinterpret_cast<const uint8 *>(&sign), sizeof(sign), rBuf);
	while(rBuf.Last() == '=')
		rBuf.TrimRight();
	return rBuf;
}

SJson * PPStyloQInterchange::MakeQuery_StoreBlob(const void * pBlobBuf, size_t blobSize, const SString & rSignature)
{
	SJson * p_js_result = SJson::CreateObj();
	SString temp_buf;
	SBinaryChunk raw_data;
	THROW(pBlobBuf && blobSize);
	THROW(rSignature.NotEmpty());
	{
		SJson query(SJson::tOBJECT);
		p_js_result->InsertString("cmd", "storeblob");
		p_js_result->InsertString("time", temp_buf.Z().Cat(time(0)));
		SFileFormat ff;
		const int fir = ff.IdentifyBuffer(pBlobBuf, blobSize);
		if(fir == 2) {
			SFileFormat::GetMime(ff, temp_buf);
			if(temp_buf.NotEmpty())
				p_js_result->InsertString("contenttype", temp_buf);
		}
		p_js_result->InsertString("contentsize", temp_buf.Z().Cat(blobSize));
		SlHash::GetAlgorithmSymb(SHASHF_SHA256, temp_buf);
		assert(temp_buf.NotEmpty());
		p_js_result->InsertString("hashalg", temp_buf.Escape());
		const binary256 hash = SlHash::Sha256(0, pBlobBuf, blobSize);
		temp_buf.EncodeMime64(&hash, sizeof(hash));
		p_js_result->InsertString("hash", temp_buf.Escape());
		p_js_result->InsertString("signature", rSignature);
	}
	CATCH
		ZDELETE(p_js_result);
	ENDCATCH
	return p_js_result;
}

bool PPStyloQInterchange::Stq_ReqBlobInfoList::ParseRequestInfoListReply(const SJson * pJs)
{
	bool    ok = true;
	Stq_ReqBlobInfoEntry rep_entry;
	THROW(pJs);
	THROW(pJs->IsArray());
	for(const SJson * p_inr = pJs->P_Child; p_inr; p_inr = p_inr->P_Next) {
		if(p_inr->IsObject()) {
			rep_entry.Z();
			for(const SJson * p_itm = p_inr->P_Child; p_itm; p_itm = p_itm->P_Next) {
				if(p_itm->Text.IsEqiAscii("signature")) {
					if(p_itm->P_Child)
						(rep_entry.Signature = p_itm->P_Child->Text).Unescape();
				}
				else if(p_itm->Text.IsEqiAscii("hashalg")) {
					if(p_itm->P_Child)
						rep_entry.HashAlg = SlHash::IdentifyAlgorithmSymb(p_itm->P_Child->Text.Unescape());
				}
				else if(p_itm->Text.IsEqiAscii("hash")) {
					if(p_itm->P_Child)
						rep_entry.Hash.FromMime64(p_itm->P_Child->Text.Unescape());
				}
				else if(p_itm->Text.IsEqiAscii("missing")) {
					if(p_itm->P_Child && p_itm->P_Child->IsTrue())
						rep_entry.Missing = true;
				}
			}
			uint sp = 0;
			if(SearchSignature(rep_entry.Signature, &sp)) {
				Stq_ReqBlobInfoEntry * p_entry = at(sp);
				assert(p_entry);
				if(p_entry) {
					p_entry->Missing = rep_entry.Missing;
					p_entry->RepDiffHash = p_entry->Missing ? false : (rep_entry.Hash != p_entry->Hash);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::TestClientInteractive(PPID svcID)
{
	class StqClientTestDialog : public TDialog {
	public:
		enum {
			cmdNone = 0,
			cmdEcho,
			cmdIndexing,
			cmdSearch,
			cmdStoreBlob, // storeblob
			cmdGetBlob,   // getblob
			cmdRequestBlobInfoList // requestblobinfolist
		};
		StqClientTestDialog(PPStyloQInterchange * pIc, const StyloQCore::StoragePacket & rPack) : TDialog(DLG_STQCLITEST), P_Ic(pIc), Pack(rPack)
		{
			SString svc_title;
			SString temp_buf;
			SBinaryChunk bc_face;
			SBinaryChunk bc_ident;
			if(Pack.Pool.Get(SSecretTagPool::tagSvcIdent, &bc_ident)) {
				svc_title.Cat(bc_ident.Mime64(temp_buf));
			}
			if(Pack.Pool.Get(SSecretTagPool::tagFace, &bc_face)) {
				StyloQFace face_pack;
				if(face_pack.FromJson(bc_face.ToRawStr(temp_buf))) {
					if(face_pack.GetRepresentation(0, temp_buf))
						svc_title.CatDivIfNotEmpty(' ', 0).Cat(temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
				}
			}
			setCtrlString(CTL_STQCLITEST_SVC, svc_title);
			{
				StrAssocArray cmd_list;
				cmd_list.Add(cmdEcho, PPLoadTextS(PPTXT_STQCLITESTCMD_ECHO, temp_buf));
				cmd_list.Add(cmdIndexing, PPLoadTextS(PPTXT_STQCLITESTCMD_INDEXING, temp_buf));
				cmd_list.Add(cmdSearch, PPLoadTextS(PPTXT_STQCLITESTCMD_SEARCH, temp_buf));
				cmd_list.Add(cmdStoreBlob, PPLoadTextS(PPTXT_STQCLITESTCMD_STOREBLOB, temp_buf)); // @v11.3.6
				cmd_list.Add(cmdGetBlob, PPLoadTextS(PPTXT_STQCLITESTCMD_GETBLOB, temp_buf)); // @v11.3.7
				cmd_list.Add(cmdRequestBlobInfoList, PPLoadTextS(PPTXT_STQCLITESTCMD_REQBLOBINFOLIST, temp_buf)); // @v11.3.7
				SetupStrAssocCombo(this, CTLSEL_STQCLITEST_FUNC, cmd_list, 0, 0);
			}
		}
	private:
		DECL_HANDLE_EVENT
		{
			SString temp_buf;
			if(event.isCmd(cmOK)) {
				SString json_buf;
				setCtrlString(CTL_STQCLITEST_RESULT, json_buf);
				SSecretTagPool svc_reply;
				PPStyloQInterchange::InterchangeParam dip;
				Pack.Pool.Get(SSecretTagPool::tagSvcIdent, &dip.SvcIdent);
				const long cmd = getCtrlLong(CTLSEL_STQCLITEST_FUNC);
				if(cmd == cmdEcho) {
					//dip.AccessPoint = p_amq_server;
					SJson query(SJson::tOBJECT);
					query.InsertString("cmd", "echo");
					query.InsertString("arg1", "arg1-value");
					query.InsertString("arg2", "arg2-value");
					query.ToStr(dip.CommandJson);
					if(P_Ic->DoInterchange(dip, svc_reply)) {
						SBinaryChunk raw_data;
						if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
							SJson * p_js = SJson::Parse(raw_data.ToRawStr(json_buf));
							if(p_js) {
								setCtrlString(CTL_STQCLITEST_RESULT, json_buf);
								ZDELETE(p_js); // @v11.3.11 @fix
							}
							else
								PPError();
						}
					}
				}
				else if(cmd == cmdRequestBlobInfoList) {
					/*
						{
							cmd: "requestblobinfolist"
							list [
								{ signature, hashalg, hash }
								{ signature, hashalg, hash }
							]
						}
					*/
					SBinaryChunk bc_own_ident;
					if(P_Ic->GetOwnIdent(bc_own_ident, 0)) {
						GoodsFilt gf;
						gf.Flags |= (GoodsFilt::fHasImages|GoodsFilt::fHidePassive|GoodsFilt::fHideGeneric);
						Goods2Tbl::Rec goods_rec;
						Stq_ReqBlobInfoList req_blob_list;
						for(GoodsIterator gi(&gf, GoodsIterator::ordByDefault); gi.Next(&goods_rec, 0) > 0;) {
							P_Ic->AddImgBlobToReqBlobInfoList(bc_own_ident, PPObjID(PPOBJ_GOODS, goods_rec.ID), req_blob_list);
						}
						SJson * p_js_query = req_blob_list.MakeRequestInfoListQuery();
						if(p_js_query) {
							SJson * p_js = 0;
							SBinaryChunk raw_data;
							dip.ClearParam();
							p_js_query->ToStr(dip.CommandJson);
							ZDELETE(p_js_query);
							if(P_Ic->DoInterchange(dip, svc_reply)) {
								if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
									p_js = SJson::Parse(raw_data.ToRawStr(json_buf));
									if(!p_js) {
										PPSetErrorSLib();
										PPError();
									}
									else if(req_blob_list.ParseRequestInfoListReply(p_js)) {
										STempBuffer blob_buf(SMEGABYTE(2));
										for(uint i = 0; i < req_blob_list.getCount(); i++) {
											Stq_ReqBlobInfoEntry * p_entry = req_blob_list.at(i);
											assert(p_entry);
											if(p_entry && (p_entry->Missing || p_entry->RepDiffHash)) {
												SFile f_in(p_entry->SrcPath, SFile::mRead|SFile::mBinary);
												if(f_in.IsValid() /*&& f_in.CalcHash(0, hash_func, bc_hash)*/) {
													size_t actual_size = 0;
													f_in.ReadAll(blob_buf, 0, &actual_size);
													SJson * p_js_blob = P_Ic->MakeQuery_StoreBlob(blob_buf, actual_size, p_entry->Signature);
													if(p_js_blob) {
														dip.ClearParam();
														p_js_blob->ToStr(dip.CommandJson);
														ZDELETE(p_js_blob);
														dip.Blob.Put(blob_buf, actual_size);
														if(P_Ic->DoInterchange(dip, svc_reply)) {
															;
														}
														else {
															;
														}
													}
												}
											}
										}
										setCtrlString(CTL_STQCLITEST_RESULT, json_buf);
									}
								}
							}
							else {
								if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
									p_js = SJson::Parse(raw_data.ToRawStr(json_buf));
									if(p_js) {
										setCtrlString(CTL_STQCLITEST_RESULT, json_buf);
									}
									else {
										PPSetErrorSLib();
										PPError();
									}
								}
							}
							delete p_js;
						}
					}
				}
				else if(cmd == cmdGetBlob) {
					{
						const char * p_test_file_name = "test_webp.webp";
						PPGetPath(PPPATH_TESTROOT, temp_buf);
						if(temp_buf.NotEmpty()) {
							temp_buf.SetLastSlash().Cat("data").SetLastSlash().Cat(p_test_file_name);
							if(fileExists(temp_buf)) {
								SBinaryChunk raw_data;
								const char * p_signature = "gr2hkflnw7fug24ocmlsdh4a24"; // Сигнатура blob'а, отправленного тестовой командой (see cmdStoreBlock)
								SJson query(SJson::tOBJECT);
								SJson * p_js = 0;
								query.InsertString("cmd", "getblob");
								query.InsertString("signature", p_signature);
								dip.ClearParam();
								query.ToStr(dip.CommandJson);
								if(P_Ic->DoInterchange(dip, svc_reply)) {
									if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
										p_js = SJson::Parse(raw_data.ToRawStr(json_buf));
										if(p_js) {
											if(svc_reply.Get(SSecretTagPool::tagBlob, &raw_data)) {
												;
											}
											setCtrlString(CTL_STQCLITEST_RESULT, json_buf);
										}
										else {
											PPSetErrorSLib();
											PPError();
										}
									}
								}
								else {
									if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
										p_js = SJson::Parse(raw_data.ToRawStr(json_buf));
										if(p_js)
											setCtrlString(CTL_STQCLITEST_RESULT, json_buf);
										else {
											PPSetErrorSLib();
											PPError();
										}
									}
								}
								delete p_js;
							}
						}
					}
				}
				// "requestblobinfolist"
				else if(cmd == cmdStoreBlob) {
					{
						#if 0 // {
						//D:\Papyrus\Src\PPTEST\DATA\test_webp.webp
						const char * p_test_file_name = "test_webp.webp";
						PPGetPath(PPPATH_TESTROOT, temp_buf);
						if(temp_buf.NotEmpty()) {
							temp_buf.SetLastSlash().Cat("data").SetLastSlash().Cat(p_test_file_name);
							if(fileExists(temp_buf)) {
								SString content_type_buf;
								SFileFormat ff;
								int fir = ff.Identify(temp_buf, 0);
								if(oneof2(fir, 2, 3)) {
									SFileFormat::GetMime(ff, content_type_buf);
								}
								{
									STempBuffer blob_buf(SKILOBYTE(1));
									SFile f_in(temp_buf, SFile::mRead|SFile::mBinary);
									if(f_in.IsValid()) {
										size_t actual_size = 0;
										if(f_in.ReadAll(blob_buf, 0, &actual_size)) {
											StyloQCore::StoragePacket own_pack;
											SBinaryChunk bc_own_ident;
											if(P_Ic->GetOwnPeerEntry(&own_pack) > 0 && own_pack.Pool.Get(SSecretTagPool::tagSvcIdent, &bc_own_ident)) {
												PPStyloQInterchange::MakeBlobSignature(bc_own_ident, p_test_file_name, temp_buf);
												SJson * p_js_query = P_Ic->MakeQuery_StoreBlob(blob_buf, actual_size, temp_buf);
												if(p_js_query) {
													SJson * p_js = 0;
													SBinaryChunk raw_data;
													dip.ClearParam();
													p_js_query->ToStr(dip.CommandJson);
													dip.Blob.Put(blob_buf, actual_size);
													ZDELETE(p_js_query);
													if(P_Ic->DoInterchange(dip, svc_reply)) {
														if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
															p_js = SJson::Parse(raw_data.ToRawStr(json_buf));
															if(p_js) {
																setCtrlString(CTL_STQCLITEST_RESULT, json_buf);
															}
															else {
																PPSetErrorSLib();
																PPError();
															}
														}
													}
													else {
														if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
															p_js = SJson::Parse(raw_data.ToRawStr(json_buf));
															if(p_js) {
																setCtrlString(CTL_STQCLITEST_RESULT, json_buf);
															}
															else {
																PPSetErrorSLib();
																PPError();
															}
														}
													}
													delete p_js;
												}
												else
													PPError();
											}
										}
									}
								}
							}
						}
						#endif // } 0
					}
					{
						getCtrlString(CTL_STQCLITEST_INPUT, temp_buf);
						if(temp_buf.NotEmptyS()) {
							SString obj_type_symb;
							for(uint pi = 0; pi < temp_buf.Len(); pi++) {
								const char c = temp_buf.C(pi);
								if(isasciialpha(c))
									obj_type_symb.CatChar(c);
								else
									break;
							}
							temp_buf.ShiftLeft(obj_type_symb.Len()).Strip();
							if(obj_type_symb.NotEmptyS()) {
								PPID obj_type = GetObjectTypeBySymb(obj_type_symb, 0);
								if(oneof3(obj_type, PPOBJ_GOODS, PPOBJ_PERSON, PPOBJ_BRAND)) {
									temp_buf.ShiftLeftChr(':').Strip();
									PPID   obj_id = temp_buf.ToLong();
									if(obj_id > 0) {
										
									}
								}
								else if(obj_type == PPOBJ_STYLOQBINDERY) {
									
								}
							}
						}
					}
				}
				else if(cmd == cmdIndexing) {
					StyloQCore::StoragePacket own_pack;
					if(P_Ic->GetOwnPeerEntry(&own_pack) > 0) {
						const StyloQCommandList::Item * p_cmd_item = 0;
						StyloQCommandList * p_cmd_list = 0;
						StyloQCommandList full_cmd_list;
						SString db_symb;
						S_GUID doc_uuid;
						if(CurDict) {
							CurDict->GetDbSymb(db_symb);
						}
						if(db_symb.NotEmpty()) {
							if(StyloQCommandList::GetFullList(0, full_cmd_list)) {
								p_cmd_list = full_cmd_list.CreateSubListByDbSymb(db_symb, StyloQCommandList::sqbcRsrvPushIndexContent);
								if(p_cmd_list) {
									assert(p_cmd_list->GetCount());
									p_cmd_item = p_cmd_list->GetC(0);
								}
							}
						}
						dip.ClearParam();
						if(P_Ic->MakeIndexingRequestCommand(&own_pack, p_cmd_item, 3600*24, doc_uuid, dip.CommandJson, 0)) {
							// @v11.4.5 {
							assert(doc_uuid);
							{
								PPID   doc_id = 0;
								SBinaryChunk doc_ident;
								SSecretTagPool doc_pool;
								if(P_Ic->P_T->MakeDocumentStorageIdent(dip.SvcIdent, doc_uuid, doc_ident)) {
									SBinarySet::DeflateStrategy ds(256);
									doc_pool.Put(SSecretTagPool::tagRawData, dip.CommandJson, dip.CommandJson.Len(), &ds);
								}
								P_Ic->P_T->PutDocument(&doc_id, dip.SvcIdent, +1, StyloQCore::doctypIndexingContent, doc_ident, doc_pool, 1);
							}
							// } @v11.4.5 
							int ir = P_Ic->DoInterchange(dip, svc_reply);
							SBinaryChunk raw_data;
							if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
								SJson * p_js = SJson::Parse(raw_data.ToRawStr(json_buf));
								if(p_js) {
									setCtrlString(CTL_STQCLITEST_RESULT, json_buf);
								}
								else {
									PPSetErrorSLib();
									PPError();
								}
								delete p_js;
							}
						}
						ZDELETE(p_cmd_list);
					}
				}
				else if(cmd == cmdSearch) {
					getCtrlString(CTL_STQCLITEST_INPUT, temp_buf);
					if(temp_buf.NotEmptyS()) {
						temp_buf.Transf(CTRANSF_INNER_TO_UTF8).Escape();
						SJson query(SJson::tOBJECT);
						query.InsertString("cmd", "search");
						query.InsertString("plainquery", temp_buf);
						query.InsertInt("maxresultcount", 20);
						query.ToStr(dip.CommandJson);
						if(P_Ic->DoInterchange(dip, svc_reply)) {
							SBinaryChunk raw_data;
							if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
								SJson * p_js = SJson::Parse(raw_data.ToRawStr(json_buf));
								if(p_js) {
									json_buf.Transf(CTRANSF_UTF8_TO_INNER);
									setCtrlString(CTL_STQCLITEST_RESULT, json_buf);
								}
								else {
									PPSetErrorSLib();
									PPError();
								}
								delete p_js;
							}
						}
					}
				}
				else {
					setCtrlString(CTL_STQCLITEST_RESULT, SString("Commaind isn't defined"));
				}
			}
			else {
				TDialog::handleEvent(event);
			}
		}
		PPStyloQInterchange * P_Ic;
		const StyloQCore::StoragePacket Pack;
	};
	int    ok = -1;
	StqClientTestDialog * dlg = 0;
	StyloQCore::StoragePacket pack;
	THROW(P_T->GetPeerEntry(svcID, &pack) > 0);
	THROW(pack.Rec.Kind == StyloQCore::kForeignService);
	{
		dlg = new StqClientTestDialog(this, pack);
		THROW(CheckDialogPtr(&dlg));
		ExecViewAndDestroy(dlg);
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::Helper_ExecuteIndexingRequest(const StyloQCore::StoragePacket & rOwnPack, 
	const TSCollection <StyloQCore::IgnitionServerEntry> & rMediatorList, const StyloQCommandList::Item * pCmdItem)
{
	int    ok = 1;
	S_GUID doc_uuid;
	SSecretTagPool svc_reply;
	SString temp_buf;
	SString msg_buf;
	SString err_buf;
	SBinaryChunk own_ident;
	Stq_CmdStat_MakeRsrv_Response stat;
	PPStyloQInterchange::InterchangeParam dip_indexing;
	const StyloQCore::IgnitionServerEntry * p_mediator_entry = rMediatorList.getCount() ? rMediatorList.at(0) : 0;
	assert(p_mediator_entry);
	assert(pCmdItem);
	assert(pCmdItem->BaseCmdId == StyloQCommandList::sqbcRsrvPushIndexContent);
	THROW(p_mediator_entry);
	THROW(pCmdItem);
	THROW(pCmdItem->BaseCmdId == StyloQCommandList::sqbcRsrvPushIndexContent);
	THROW(DS.GetConstTLA().IsAuth());
	dip_indexing.SvcIdent = p_mediator_entry->Ident;
	dip_indexing.AccessPoint = p_mediator_entry->Url;
	THROW(MakeIndexingRequestCommand(&rOwnPack, pCmdItem, 3600*24, doc_uuid, dip_indexing.CommandJson, &stat));
	THROW_PP(rOwnPack.Pool.Get(SSecretTagPool::tagSvcIdent, &own_ident), PPERR_SQ_UNDEFOWNSVCID);
	// @v11.4.5 {
	assert(doc_uuid);
	{
		//
		// Перед передачей модератору мы сохраняем у себя в реестре копию документа как исходящий типа doctypIndexingContent
		//
		PPID   doc_id = 0;
		SBinaryChunk doc_ident;
		SSecretTagPool doc_pool;
		SBinarySet::DeflateStrategy ds(256);
		THROW(P_T->MakeDocumentStorageIdent(dip_indexing.SvcIdent, doc_uuid, doc_ident));
		THROW(doc_pool.Put(SSecretTagPool::tagRawData, dip_indexing.CommandJson, dip_indexing.CommandJson.Len(), &ds));
		THROW(P_T->PutDocument(&doc_id, dip_indexing.SvcIdent, +1, StyloQCore::doctypIndexingContent, doc_ident, doc_pool, 1));
	}
	// } @v11.4.5 
	{
		//
		// Сначала передаем blob'ы, сопоставленные с объектами, подлежащими индексации
		//
		Stq_ReqBlobInfoList req_blob_list;
		PPObjIDArray stored_oid_list;
		GetOidListWithBlob(stored_oid_list);
		if(stored_oid_list.getCount()) {
			stored_oid_list.SortAndUndup();
			for(uint i = 0; i < stored_oid_list.getCount(); i++) {
				PPObjID local_oid = stored_oid_list.at(i);
				Stq_CmdStat_MakeRsrv_Response::RegisterOid(&stat, local_oid);
			}
		}
		stat.OidList.SortAndUndup();
		for(uint oididx = 0; oididx < stat.OidList.getCount(); oididx++) {
			PPObjID oid = stat.OidList.at(oididx);
			AddImgBlobToReqBlobInfoList(own_ident, oid, req_blob_list);
		}
		SJson * p_js_query = req_blob_list.MakeRequestInfoListQuery();
		if(p_js_query) {
			SJson * p_js = 0;
			SBinaryChunk raw_data;
			PPStyloQInterchange::InterchangeParam dip_blob;
			dip_blob.SvcIdent = p_mediator_entry->Ident;
			dip_blob.AccessPoint = p_mediator_entry->Url;
			p_js_query->ToStr(dip_blob.CommandJson);
			ZDELETE(p_js_query);
			if(DoInterchange(dip_blob, svc_reply)) {
				if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
					THROW_SL(p_js = SJson::Parse(raw_data.ToRawStr(temp_buf)));
					if(req_blob_list.ParseRequestInfoListReply(p_js)) {
						SString fmt_buf_succ;
						SString fmt_buf_fail;
						PPLoadText(PPTXT_STQ_BLOBTRANSMTOMEDIATORSUCC, fmt_buf_succ); // "Stylo-Q: blob объекта передан медиатору (%s)"
						PPLoadText(PPTXT_STQ_BLOBTRANSMTOMEDIATORFAIL, fmt_buf_fail); // "Stylo-Q: ошибка передачи медиатору blob'а объекта (%s): %s"
						for(uint rblidx = 0; rblidx < req_blob_list.getCount(); rblidx++) {
							Stq_ReqBlobInfoEntry * p_entry = req_blob_list.at(rblidx);
							assert(p_entry);
							if(p_entry && (p_entry->Missing || p_entry->RepDiffHash)) {
								StyloQBlobInfo bi;
								dip_blob.ClearParam();
								if(GetBlobInfo(own_ident, p_entry->Oid, 0, 0/*flags*/, bi, &dip_blob.Blob)) {
									assert(bi.Signature == p_entry->Signature);
									assert(bi.Hash == p_entry->Hash);
									SJson * p_js_blob = MakeQuery_StoreBlob(dip_blob.Blob.PtrC(), dip_blob.Blob.Len(), p_entry->Signature);
									if(p_js_blob) {
										p_js_blob->ToStr(dip_blob.CommandJson);
										//dip_blob.Blob.Put(blob_buf, actual_size);
										ZDELETE(p_js_blob);
										//
										temp_buf.Z().CatChar('{').Cat(p_entry->Oid.Obj).Semicol().Cat(p_entry->Oid.Id).CatChar('}').Space().
											Cat(p_entry->Signature).Space().Cat("-->").Space().Cat(dip_blob.AccessPoint);
										if(DoInterchange(dip_blob, svc_reply)) {
											msg_buf.Printf(fmt_buf_succ.cptr(), temp_buf.cptr());
											PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_COMP);
										}
										else {
											PPGetMessage(mfError, PPErrCode, 0, 1, err_buf);
											msg_buf.Printf(fmt_buf_fail.cptr(), temp_buf.cptr(), err_buf.cptr());
											PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_COMP);
										}
										DS.SetThreadNotification(PPSession::stntMessage, msg_buf);
									}										
								}
							}
						}
					}
				}
			}
			else {
				if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
					p_js = SJson::Parse(raw_data.ToRawStr(temp_buf));
					if(p_js) {
						;
					}
				}
			}
			delete p_js;
		}
	}
	{
		const int ir = DoInterchange(dip_indexing, svc_reply);
		THROW(ir);
		{
			// @todo Как-то отреагировать на ответ сервиса
			SBinaryChunk raw_data;
			if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
				//raw_data.ToRawStr(json_buf);
				//SJson * p_js = SJson::Parse(json_buf);
				//if(p_js) {
					//;
				//}
			}
		}
		if(ir > 0) {
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			if(p_sj)
				p_sj->LogEvent(PPACN_STYLOQSVCIDXQUERY, PPOBJ_STYLOQBINDERY, rOwnPack.Rec.ID, 0/*extraData*/, 1/*use_ta*/);
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::Helper_PrepareAhed(const StyloQCommandList & rFullCmdList)
{
	int    ok = -1;
	const  LDATETIME dtm_now = getcurdatetime_();
	StyloQCore::StoragePacket fake_cli_pack;
	SString temp_buf;
	SString dtm_buf;
	SString result_buf;
	SString decl_buf;
	SString fmt_buf;
	SString msg_buf;
	SBinaryChunk own_ident;
	SString db_symb;
	THROW(DS.GetConstTLA().IsAuth());
	THROW(GetOwnIdent(own_ident, 0));
	{
		DbProvider * p_dict = CurDict;
		assert(p_dict); // Проверка DS.GetConstTLA().IsAuth() выше гарантирует исполнение условия!
		const int gdbsr = p_dict->GetDbSymb(db_symb); 
		assert(gdbsr);
		THROW(gdbsr);
		for(uint i = 0; i < rFullCmdList.GetCount(); i++) {
			const StyloQCommandList::Item * p_cmd_item = rFullCmdList.GetC(i);
			if(p_cmd_item->Flags & StyloQCommandList::Item::fPrepareAhead && p_cmd_item->DbSymb.IsEqNC(db_symb)) {
				if(p_cmd_item->CanApplyPrepareAheadOption()) {
					int    msg_to_log = 0; // 1 - nothing-to-do, 2 - done
					PPID   doc_id = 0;
					SBinaryChunk doc_ident;
					StyloQSecTbl::Rec result_rec;
					StyloQCore::StoragePacket sp;
					switch(p_cmd_item->BaseCmdId) {
						case StyloQCommandList::sqbcRsrvOrderPrereq:
							{
								const int doc_type = StyloQCore::doctypOrderPrereq;
								bool  do_prepare = true;
								THROW(P_T->MakeDocumentStorageIdent(own_ident, p_cmd_item->Uuid, doc_ident));
								if(P_T->GetDocByType(+1, doc_type, &doc_ident, &sp) > 0 && cmp(sp.Rec.Expiration, dtm_now) > 0) {
									msg_to_log = 1;
									do_prepare = false;
								}
								if(do_prepare && ProcessCommand_RsrvOrderPrereq(*p_cmd_item, fake_cli_pack, result_buf, decl_buf, false)) {
									SSecretTagPool doc_pool;
									{
										SBinarySet::DeflateStrategy ds(256);
										THROW_SL(doc_pool.Put(SSecretTagPool::tagRawData, result_buf, result_buf.Len(), &ds));
										if(decl_buf.Len()) {
											THROW_SL(doc_pool.Put(SSecretTagPool::tagDocDeclaration, decl_buf, decl_buf.Len(), &ds));
										}
									}
									THROW(P_T->PutDocument(&doc_id, &result_rec, own_ident, +1/*output*/, doc_type, doc_ident, doc_pool, 1/*use_ta*/));
									msg_to_log = 2;
									ok = 1;
								}
							}
							break;
						case StyloQCommandList::sqbcRsrvIndoorSvcPrereq:
							{
								const int doc_type = StyloQCore::doctypIndoorSvcPrereq;
								bool  do_prepare = true;
								THROW(P_T->MakeDocumentStorageIdent(own_ident, p_cmd_item->Uuid, doc_ident));
								if(P_T->GetDocByType(+1, doc_type, &doc_ident, &sp) > 0 && cmp(sp.Rec.Expiration,  dtm_now) > 0) {
									msg_to_log = 1;
									do_prepare = false;
								}
								if(do_prepare && ProcessCommand_RsrvIndoorSvcPrereq(*p_cmd_item, fake_cli_pack, result_buf, decl_buf, false)) {
									SSecretTagPool doc_pool;
									{
										SBinarySet::DeflateStrategy ds(256);
										THROW_SL(doc_pool.Put(SSecretTagPool::tagRawData, result_buf, result_buf.Len(), &ds));
										if(decl_buf.Len()) {
											THROW_SL(doc_pool.Put(SSecretTagPool::tagDocDeclaration, decl_buf, decl_buf.Len(), &ds));
										}
									}
									THROW(P_T->PutDocument(&doc_id, &result_rec, own_ident, +1/*output*/, doc_type, doc_ident, doc_pool, 1/*use_ta*/));
									msg_to_log = 2;
									ok = 1;
								}
							}
							break;
						case StyloQCommandList::sqbcReport:
							break;
					}
					if(msg_to_log == 1) {
						//PPTXT_LOG_STQPREPAREAHEAD_NTD    /!/ "Stylo-Q: PrepareAhead for the command '%s': existing data is still actual (%s)"
						PPLoadText(PPTXT_LOG_STQPREPAREAHEAD_NTD, fmt_buf);
						(temp_buf = p_cmd_item->Name).Transf(CTRANSF_UTF8_TO_INNER);
						dtm_buf.Z().Cat(sp.Rec.Expiration, DATF_ISO8601CENT, 0);
						msg_buf.Printf(fmt_buf, temp_buf.cptr(), dtm_buf.cptr());
						PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO);
					}
					else if(msg_to_log == 2) {
						//PPTXT_LOG_STQPREPAREAHEAD_DONE   /!/ "Stylo-Q: PrepareAhead for the command '%s': done. Data will be actual till (%s)"
						PPLoadText(PPTXT_LOG_STQPREPAREAHEAD_DONE, fmt_buf);
						(temp_buf = p_cmd_item->Name).Transf(CTRANSF_UTF8_TO_INNER);
						dtm_buf.Z().Cat(result_rec.Expiration, DATF_ISO8601CENT, 0);
						msg_buf.Printf(fmt_buf, temp_buf.cptr(), dtm_buf.cptr());
						PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int PPStyloQInterchange::PrepareAhed(bool useCurrentSession)
{
	int    ok = -1;
	StyloQCommandList full_cmd_list;
	char   secret[64];
	bool   is_logged_in = false;
	StringSet ss_db_symb; // Список символов DB в которых есть команды, требующие предварительной подготовки
	THROW(StyloQCommandList::GetFullList(0, full_cmd_list));
	if(useCurrentSession) {
		PPStyloQInterchange ic;
		ic.Helper_PrepareAhed(full_cmd_list);
	}
	else {
		{
			for(uint i = 0; i < full_cmd_list.GetCount(); i++) {
				const StyloQCommandList::Item * p_cmd_item = full_cmd_list.GetC(i);
				if(p_cmd_item->Flags & StyloQCommandList::Item::fPrepareAhead && p_cmd_item->DbSymb.NotEmpty()) {
					ss_db_symb.add(p_cmd_item->DbSymb);
				}
			}
			ss_db_symb.sortAndUndup();
		}
		{
			SString db_symb;
			{
				PPVersionInfo vi = DS.GetVersionInfo();
				THROW(vi.GetSecret(secret, sizeof(secret)));
			}
			for(uint ssp = 0; ss_db_symb.get(&ssp, db_symb);) {
				if(DS.Login(db_symb, PPSession::P_JobLogin, secret, PPSession::loginfSkipLicChecking)) {
					is_logged_in = true;
					{ // Здесь scope {} важна - ic должен разрушиться до вызова DS.Logout()
						PPStyloQInterchange ic;
						if(!ic.Helper_PrepareAhed(full_cmd_list)) {
							PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
						}
					}
					DS.Logout();
					is_logged_in = false;
				}
				else {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_COMP);
				}
			}
		}
	}
	CATCH
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP);
		ok = 0;
	ENDCATCH
	if(is_logged_in)
		DS.Logout();
	memzero(secret, sizeof(secret));
	return ok;
}

/*static*/int PPStyloQInterchange::ExecuteIndexingRequest(bool useCurrentSession)
{
	int    ok = 1;
	SBinaryChunk bc;
	SBinaryChunk cfg_bytes;
	SString url_buf;
	SString msg_buf;
	StyloQCommandList * p_cmd_list = 0;
	PPSession::LimitedDatabaseBlock * p_ldb = 0;
	TSCollection <StyloQCore::IgnitionServerEntry> mediator_list;
	StyloQCommandList full_cmd_list;
	char   secret[64];
	bool   is_logged_in = false;
	THROW(StyloQCommandList::GetFullList(0, full_cmd_list));
	if(useCurrentSession) {
		//MemLeakTracer mlt;
		THROW(DS.GetConstTLA().IsAuth());
		{
			PPStyloQInterchange ic;
			SString db_symb;
			DbProvider * p_dict = CurDict;
			assert(p_dict); // Проверка DS.GetConstTLA().IsAuth() выше гарантирует исполнение условия!
			const int gdbsr = p_dict->GetDbSymb(db_symb);
			assert(gdbsr);
			THROW(gdbsr);
			if(ic.P_T->GetMediatorList(mediator_list) > 0) {
				SysJournal * p_sj = DS.GetTLA().P_SysJ;
				StyloQCore::StoragePacket sp;
				mediator_list.shuffle();
				p_cmd_list = full_cmd_list.CreateSubListByDbSymb(db_symb, StyloQCommandList::sqbcRsrvPushIndexContent);
				THROW(ic.P_T->GetOwnPeerEntry(&sp) > 0);
				for(uint clidx = 0; clidx < p_cmd_list->GetCount(); clidx++) {
					const StyloQCommandList::Item * p_cmd_item = p_cmd_list->GetC(clidx);
					assert(p_cmd_item);
					assert(p_cmd_item->BaseCmdId == StyloQCommandList::sqbcRsrvPushIndexContent);
					bool is_expired = true;
					if(p_sj) {
						const long expiry_period = (p_cmd_item->ResultExpiryTimeSec > 0) ? p_cmd_item->ResultExpiryTimeSec : (24 * 3600);
						LDATETIME last_ev_dtm;
						SysJournalTbl::Rec last_ev_rec;
						is_expired = (p_sj->GetLastEvent(PPACN_STYLOQSVCIDXQUERY, 0, &last_ev_dtm, 7, &last_ev_rec) > 0) ? 
							(diffdatetimesec(getcurdatetime_(), last_ev_dtm) > expiry_period) : true;
					}
					if(is_expired) {
						if(!ic.Helper_ExecuteIndexingRequest(sp, mediator_list, p_cmd_item))
							PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP);
					}
				}
			}
		}
	}
	else {
		// Так как среди точек входа в базу данных могут быть дубликаты (разные точки, но база одна), то
		// элиминируем эти дубликаты по значению service-ident. Следующий список как раз для этого и предназначен.
		TSCollection <SBinaryChunk> reckoned_svc_id_list;
		StyloQCore::SvcDbSymbMap dbmap;
		PPDbEntrySet2 dbes;
		{
			PPIniFile ini_file;
			THROW(ini_file.IsValid());
			THROW(dbes.ReadFromProfile(&ini_file, 0));
		}
		{
			PPVersionInfo vi = DS.GetVersionInfo();
			THROW(vi.GetSecret(secret, sizeof(secret)));
		}
		THROW(StyloQCore::GetDbMap(dbmap));
		for(uint i = 0; i < dbmap.getCount(); i++) {
			const StyloQCore::SvcDbSymbMapEntry * p_map_entry = dbmap.at(i);
			if(p_map_entry && p_map_entry->DbSymb.NotEmpty() && p_map_entry->SvcIdent.Len()) {
				StyloQCore::StoragePacket sp;
				ZDELETE(p_ldb);
				p_ldb = DS.LimitedOpenDatabase(p_map_entry->DbSymb, PPSession::lodfReference|PPSession::lodfStyloQCore|PPSession::lodfSysJournal);
				if(!p_ldb) {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_COMP);
				}
				else if(p_ldb->P_Sqc->GetOwnPeerEntry(&sp) <= 0) {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_COMP);
				}
				else if(!sp.Pool.Get(SSecretTagPool::tagSvcIdent, &bc)) {
					PPSetError(PPERR_SQ_UNDEFOWNSVCID);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_COMP);
				}
				else {
					SysJournal * p_sj = p_ldb->P_Sj;
					if(p_ldb->P_Sqc->GetMediatorList(mediator_list) > 0) {
						assert(mediator_list.getCount());
						bool svc_id_reckoned = false;
						for(uint rslidx = 0; !svc_id_reckoned && rslidx < reckoned_svc_id_list.getCount(); rslidx++) {
							if(*reckoned_svc_id_list.at(rslidx) == bc)
								svc_id_reckoned = true;
						}
						if(!svc_id_reckoned) {
							ZDELETE(p_cmd_list);
							p_cmd_list = full_cmd_list.CreateSubListByDbSymb(p_map_entry->DbSymb, StyloQCommandList::sqbcRsrvPushIndexContent);
							if(p_cmd_list) {
								assert(p_cmd_list->GetCount()); // Если full_cmd_list.CreateSubListByDbSymb вернул ненулевой результат, то не должно быть так, что список пустой
								for(uint clidx = 0; clidx < p_cmd_list->GetCount(); clidx++) {
									const StyloQCommandList::Item * p_cmd_item = p_cmd_list->GetC(clidx);
									assert(p_cmd_item);
									assert(p_cmd_item->BaseCmdId == StyloQCommandList::sqbcRsrvPushIndexContent);
									bool is_expired = true;
									if(p_sj) {
										const long expiry_period = (p_cmd_item->ResultExpiryTimeSec > 0) ? p_cmd_item->ResultExpiryTimeSec : (24 * 3600);
										LDATETIME last_ev_dtm;
										SysJournalTbl::Rec last_ev_rec;
										is_expired = (p_sj->GetLastEvent(PPACN_STYLOQSVCIDXQUERY, 0, &last_ev_dtm, 7, &last_ev_rec) > 0) ? 
											(diffdatetimesec(getcurdatetime_(), last_ev_dtm) > expiry_period) : true;
									}
									if(is_expired) {
										//
										ZDELETE(p_ldb);
										p_sj = 0;
										//
										if(DS.Login(p_map_entry->DbSymb, PPSession::P_JobLogin, secret, PPSession::loginfSkipLicChecking)) {
											is_logged_in = true;
											{ // Здесь scope {} важна - ic должен разрушиться до вызова DS.Logout()
												PPStyloQInterchange ic;
												mediator_list.shuffle();
												if(!ic.Helper_ExecuteIndexingRequest(sp, mediator_list, p_cmd_item))
													PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP);
											}
											DS.Logout();
											is_logged_in = false;
										}
									}
								}
							}
							ZDELETE(p_cmd_list);
						}
					}
				}
				ZDELETE(p_ldb);
			}
		}
	}
	CATCH
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP);
		ok = 0;
	ENDCATCH
	if(is_logged_in)
		DS.Logout();
	memzero(secret, sizeof(secret));
	delete p_ldb;
	delete p_cmd_list;
	return ok;
}
//
//
//
PPSession::StyloQ_Cache::StyloQ_Cache()
{
}
	
PPSession::StyloQ_Cache::~StyloQ_Cache()
{
}

int PPSession::StyloQ_Cache::GetBlob(const SBinaryChunk & rOwnIdent, PPObjID oid, uint blobN, StyloQBlobInfo & rBi)
{
	rBi.Z();
	int    ok = -1;
	if(rOwnIdent.Len()) {
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		for(uint i = 0; i < L.getCount(); i++) {
			SvcEntry * p_entry = L.at(i);
			assert(p_entry);
			if(p_entry && p_entry->OwnIdent == rOwnIdent) {
				const TSVector <SvcEntry::BlobInnerEntry> & r_bl = p_entry->BlobList;
				uint   bidx = 0;
				if(r_bl.bsearch(&oid, &bidx, PTR_CMPFUNC(PPObjID))) {
					for(uint j = bidx; j < r_bl.getCount(); j++) {
						const SvcEntry::BlobInnerEntry & r_ble = r_bl.at(j);
						assert(j != bidx || r_ble.Oid == oid);
						if(r_ble.Oid == oid) {
							if(r_ble.BlobN == blobN) {
								rBi.BlobN = r_ble.BlobN;
								rBi.Ff = r_ble.Ff;
								rBi.Oid = r_ble.Oid;
								p_entry->GetS(r_ble.SignatureP, rBi.Signature);
								ok = 1;
								break; // У нас только один элемент в параметрах, потому, даже если один объект имеет несколько blob'ов, то все равно уходим.
							}
						}
						else
							break;
					}
				}
			}
		}
	}
	else
		ok = 0;
	return ok;
}

int PPSession::StyloQ_Cache::PutBlob(const SBinaryChunk & rOwnIdent, PPObjID oid, uint blobN, const StyloQBlobInfo & rBi)
{
	int    ok = -1;
	if(rOwnIdent.Len()) {
		assert(rBi.BlobN || rBi.Signature.IsEmpty());
		assert(!rBi.BlobN || !rBi.Signature.IsEmpty());
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		SvcEntry * p_target_entry = 0;
		for(uint i = 0; !p_target_entry && i < L.getCount(); i++) {
			SvcEntry * p_entry = L.at(i);
			if(p_entry && p_entry->OwnIdent == rOwnIdent) {
				p_target_entry = p_entry;
			}
		}
		if(!p_target_entry) {
			SvcEntry * p_new_entry = L.CreateNewItem();
			p_new_entry->OwnIdent = rOwnIdent;
			p_target_entry = p_new_entry;
		}
		assert(p_target_entry);
		if(p_target_entry) {
			TSVector <SvcEntry::BlobInnerEntry> & r_bl = p_target_entry->BlobList;
			uint   bidx = 0;
			bool   found = false;
			if(r_bl.bsearch(&oid, &bidx, PTR_CMPFUNC(PPObjID))) {
				SString s2;
				for(uint j = bidx; j < r_bl.getCount() && r_bl.at(j).Oid == oid; j++) {
					SvcEntry::BlobInnerEntry & r_ble = r_bl.at(j);
					if(r_ble.BlobN == rBi.BlobN) {
						p_target_entry->GetS(r_ble.SignatureP, s2);
						found = true;
						if(s2 != rBi.Signature) {
							p_target_entry->AddS(rBi.Signature, &r_ble.SignatureP);
							ok = 1;
						}
						if(r_ble.Ff != rBi.Ff) {
							r_ble.Ff = rBi.Ff;
							ok = 1;
						}
					}
				}
			}
			if(!found) {
				SvcEntry::BlobInnerEntry new_bie;
				new_bie.Oid = oid;
				new_bie.BlobN = rBi.BlobN;
				new_bie.Ff = rBi.Ff;
				p_target_entry->AddS(rBi.Signature, &new_bie.SignatureP);
				r_bl.ordInsert(&new_bie, 0, PTR_CMPFUNC(PPObjID));
				ok = 1;
			}
		}
	}
	return ok;
}

bool PPStyloQInterchange::FetchBlobSignature(const SBinaryChunk & rOwnIdent, PPObjID oid, uint blobN, SString & rSignature)
{
	rSignature.Z();
	StyloQBlobInfo bi;
	if(oid.Obj == PPOBJ_STYLOQBINDERY) {
		// blob'ы styloq-bindery хранятся внутри базы данных в блоке face и имеют зарезервированный номер
		blobN = PPStyloQInterchange::InnerBlobN_Face;
	}
	const int gbr = DS.Stq_GetBlob(rOwnIdent, oid, blobN, bi);
	if(gbr > 0) {
		rSignature = bi.Signature;
	}
	else if(gbr < 0) {
		const int gbir = GetBlobInfo(rOwnIdent, oid, blobN, gbifSignatureOnly, bi, 0);
		DS.Stq_PutBlob(rOwnIdent, oid, blobN, bi);
		rSignature = bi.Signature;
	}
	return rSignature.NotEmpty();
}
//
//
//
struct Settlement { // @construction
	S_GUID Uid;
	uint32 DonorId;
	uint32 AcptrId;
	uint32 AudtrId;
	uint64 Amt;
	uint64 TmstmpDonor;
	uint64 TmstmpAcptr;
	binary160 Hsh;
	binary160 BckHsh;
};
//

#if 0 // {

//
// Descr
//
table StyloQCmdSupp {
	autolong ID;
	raw    SvcID[20];   // ИД сервиса
	guid   OrgCmdUuid;
	long   Kind;        
	int64  TimeStamp;
	blob   VT[2048];
index:
	ID (unique);
	SvcID, Kind (unique);
	OrgCmdUuid, Kind (unique);
}

#endif // } 0
