// STYLOQ.CPP
// Copyright (c) A.Sobolev 2021, 2022
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
const long __DefMqbConsumeTimeout = 5000;
/*
	RESERVED COMMANDS:
	
	test
	//
	register
	quit
	advert
	getconfig
	getface
	setface - передается от клиента сервису для обноления собственного лика
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
};

/*static*/SJson * StyloQConfig::MakeTransmissionJson(const char * pSrcJson)
{
	SJson * p_result = 0;
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
		p_result = cfg_pack.ToJson();
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

/*static*/int StyloQConfig::MakeTransmissionJson(const char * pSrcJson, SString & rTransmissionJson)
{
	int    ok = 0;
	SJson * p_js = MakeTransmissionJson(pSrcJson);
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
	int    ok = 0;
	Z();
	if(!isempty(pJsonText)) {
		SJson * p_js = 0;
		if(json_parse_document(&p_js, pJsonText) == JSON_OK) { // @v11.2.10 @fix (== JSON_OK)
			assert(p_js);
			ok = FromJsonObject(p_js);
		}
	}
	else
		ok = -1;
	return ok;
}

SJson * StyloQConfig::ToJson() const
{
	SJson * p_result = 0;
	if(L.getCount()) {
		const long zero = 0L;
		SString tag_value;
		THROW_SL(p_result = new SJson(SJson::tOBJECT));
		for(uint i = 0; i < SIZEOFARRAY(StyloQConfigTagNameList); i++) {
			const SIntToSymbTabEntry & r_idx_entry = StyloQConfigTagNameList[i];
			if(Get(r_idx_entry.Id, tag_value)) {
				THROW_SL(p_result->InsertString(r_idx_entry.P_Symb, tag_value));
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
	int    ok = 0;
	Z();
	if(!isempty(pJsonText)) {
		SJson * p_js = 0;
		if(json_parse_document(&p_js, pJsonText) == JSON_OK) { // @v11.2.10 @fix (== JSON_OK)
			assert(p_js);
			ok = FromJsonObject(p_js);
		}
		ZDELETE(p_js);
	}
	else
		ok = -1;
	return ok;
}

SJson * StyloQFace::ToJson(bool forTransmission) const
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
		p_result = new SJson(SJson::tOBJECT);
		for(uint i = 0; i < SIZEOFARRAY(StyloQFaceTagNameList); i++) {
			const SIntToSymbTabEntry & r_idx_entry = StyloQFaceTagNameList[i];
			if(IsTagLangDependent(r_idx_entry.Id)) {
				for(uint li = 0; li < lang_list.getCount(); li++) {
					const int lang = lang_list.get(li);
					if(GetExactly(r_idx_entry.Id, lang, tag_value)) {
						if(!lang) {
							p_result->InsertString(r_idx_entry.P_Symb, tag_value);
						}
						else if(GetLinguaCode(lang, lang_code)) {
							(temp_buf = r_idx_entry.P_Symb).Dot().Cat(lang_code);
							p_result->InsertString(temp_buf, tag_value);
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
					p_result->InsertString(r_idx_entry.P_Symb, tag_value);
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
		p_js = ToJson(forTransmission);
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
	SJson * p_js = MakeTransmissionJson(id, rOwnIdent, pSrcJson);
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
			p_result = p_result_pack->ToJson(true);
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
		r_temp_buf.Cat(dt, DATF_ISO8601|DATF_CENTURY);
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
			//Flags &= ~(fVerifiable|fAnonym);
			p_tag_val = "arbitrary";
			break;
		case vVerifiable:
			//Flags |= fVerifiable;
			//Flags &= ~fAnonym;
			p_tag_val = "verifiable";
			break;
		case vAnonymous:
			//Flags |= fAnonym;
			//Flags &= ~fVerifiable;
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
		dob = strtodate_(r_temp_buf, DATF_ISO8601|DATF_CENTURY);
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
	}
	if(p_sign)
		PPLoadString(p_sign, rBuf);
	return rBuf;
}

StyloQCommandList::Item::Item() : Ver(0), BaseCmdId(sqbcEmpty), Flags(0), ObjTypeRestriction(0), ObjGroupRestriction(0), ResultExpiryTimeSec(0)
{
}

StyloQCommandList::StyloQCommandList()
{
}

StyloQCommandList::~StyloQCommandList()
{
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
	}
	if(!p_result) {
		PPSetError(PPERR_SQ_UNKNCMD);
	}
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
							if(p_item->ObjGroupRestriction) {
								n_item.PutInner("ObjGroupRestriction", temp_buf.Z().Cat(p_item->ObjGroupRestriction));
							}
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

int StyloQCommandList::Load(const char * pDbSymb, const char * pFileName)
{
	int    ok = 1;
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
		ok = 0;
	ENDCATCH
	delete p_new_item;
	xmlFreeDoc(p_doc);
	xmlFreeParserCtxt(p_parser);
	return ok;
}

/*static*/SJson * StyloQCommandList::CreateJsonForClient(const StyloQCommandList * pSelf, SJson * pParent, const char * pName, long expirationSec)
{
	assert(!pParent || !isempty(pName));
	SString temp_buf;
	SJson * p_result = new SJson(SJson::tOBJECT);
	//LDATETIME dtm_now = getcurdatetime_();
	//temp_buf.Z().Cat(dtm_now, DATF_ISO8601|DATF_CENTURY, 0);
	p_result->InsertString("doctype", "commandlist");
	p_result->InsertString("time", temp_buf.Z().Cat(time(0)));
	if(expirationSec > 0) {
		p_result->Insert("expir_time_sec", json_new_number(temp_buf.Z().Cat(expirationSec)));
	}
	{
		SJson * p_array = new SJson(SJson::tARRAY);
		if(pSelf) {
			for(uint i = 0; i < pSelf->L.getCount(); i++) {
				const Item * p_item = pSelf->L.at(i);
				if(p_item) {
					SJson * p_jitem = new SJson(SJson::tOBJECT);
					p_jitem->InsertString("uuid", temp_buf.Z().Cat(p_item->Uuid));
					p_jitem->InsertString("basecmdid", temp_buf.Z().Cat(p_item->BaseCmdId)); // @v11.2.9
					p_jitem->InsertString("name", (temp_buf = p_item->Name).Escape());
					if(p_item->Description.NotEmpty()) {
						p_jitem->InsertString("descr", (temp_buf = p_item->Description).Escape());
					}
					// @v11.2.5 {
					if(p_item->ResultExpiryTimeSec > 0)
						p_jitem->InsertInt("result_expir_time_sec", p_item->ResultExpiryTimeSec);
					// } @v11.2.5 
					// @todo transmit image
					json_insert_child(p_array, p_jitem);
				}
			}
		}
		p_result->Insert("item_list", p_array);
	}
	if(p_result && pParent && pName) {
		pParent->Insert(pName, p_result);
	}
	return p_result;
}

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

StyloQCommandList * StyloQCommandList::CreateSubListByContext(PPObjID oid) const
{
	StyloQCommandList * p_result = 0;
	PPObjPerson psn_obj;
	PPObjSecur usr_obj(PPOBJ_USR, 0);
	for(uint i = 0; i < L.getCount(); i++) {
		const Item * p_item = L.at(i);
		if(p_item) {
			bool suited = false;
			if(p_item->BaseCmdId != StyloQCommandList::sqbcRsrvPushIndexContent) { // @v11.3.4 Эту команду не надо посылать клиенту, она параметризует индексацию медиаторами
				if(!p_item->ObjTypeRestriction)
					suited = true;
					// rList.Z().addzlist(PPOBJ_USR, PPOBJ_PERSON, PPOBJ_DBDIV, PPOBJ_CASHNODE, 0L);
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
		// PPERR_SQ_SESSNFOUND  Транспортная сессия по публичному ключу не найдена
		ok = -1;
	}
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

int StyloQCore::PutDocument(PPID * pID, const SBinaryChunk & rContractorIdent, int direction, int docType, const SBinaryChunk & rIdent, SSecretTagPool & rPool, int use_ta)
{
	int    ok = -1;
	PPID   new_id = 0;
	SJson * p_js = 0;
	SString temp_buf;
	SBinaryChunk raw_doc;
	time_t doc_time = 0;
	long   doc_expiry = 0;
	SString raw_doc_type;
	StoragePacket pack;
	StoragePacket other_pack;
	const time_t tm_now = time(0);
	THROW(rIdent.Len() > 0 && rIdent.Len() <= sizeof(pack.Rec.BI));
	THROW(rContractorIdent.Len() > 0 && rContractorIdent.Len() <= sizeof(pack.Rec.BI));
	const int sgi_as_cli_r = SearchGlobalIdentEntry(kClient, rContractorIdent, &other_pack);
	const int sgi_as_svc_r = (sgi_as_cli_r > 0) ? -1 : SearchGlobalIdentEntry(kForeignService, rContractorIdent, &other_pack);
	THROW(sgi_as_cli_r > 0 || sgi_as_svc_r > 0);
	THROW(oneof2(other_pack.Rec.Kind, kForeignService, kClient));
	THROW(rPool.Get(SSecretTagPool::tagRawData, &raw_doc));
	THROW_SL(json_parse_document(&p_js, raw_doc.ToRawStr(temp_buf)) == JSON_OK);
	{
		for(const SJson * p_cur = p_js; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Type == SJson::tOBJECT) {								
				for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
					if(p_obj->Text.IsEqiAscii("doctype")) {
						raw_doc_type = SJson::Unescape(p_obj->P_Child->Text);
					}
					else if(p_obj->Text.IsEqiAscii("time")) {
						doc_time = static_cast<time_t>(p_obj->P_Child->Text.ToInt64());
					}
					else if(p_obj->Text.IsEqiAscii("expir_time_sec")) {
						doc_expiry = p_obj->P_Child->Text.ToLong();
					}
				}
			}
		}
	}
	{
		assert(other_pack.Rec.ID > 0); // Мы все проверили выше: не может быть чтоб этот ид был неопределен.
		if(direction > 0)
			pack.Rec.Kind = kDocOutcominig;
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
		if(docType == doctypIndexingContent) {
			pack.Rec.Flags |= fUnprocessedDoc;
			// Документ такого типа может быть только один в комбинации {direction; rIdent}
			LongArray ex_id_list;
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
		else if(docType == doctypCommandList) {
			THROW(raw_doc_type.IsEqiAscii("commandlist"));
			// Документ такого типа может быть только один в комбинации {direction; rIdent}
			LongArray ex_id_list;
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
	ASSIGN_PTR(pID, new_id);
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
			if(direction == 0 || (direction > 0 && data.Kind == kDocOutcominig) || (direction < 0 && data.Kind == kDocIncoming)) {
				if(!pIdent || memcmp(data.BI, pIdent->PtrC(), pIdent->Len()) == 0) {
					rIdList.add(data.ID);
					ok = 1;
				}
			}
		} while(search(3, &k3, spNext) && data.DocType == docType);
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
	if(search(3, &k3, spGe) && data.DocType == doctypIndexingContent) do {
		if(data.Flags & fUnprocessedDoc) {
			rIdList.add(data.ID);
			ok = 1;
		}
	} while(search(3, &k3, spNext) && data.DocType == doctypIndexingContent);
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
		assert(oneof7(pPack->Rec.Kind, kNativeService, kForeignService, kClient, kSession, kFace, kDocIncoming, kDocOutcominig)); // @v11.1.8 kFace
		if(oneof3(pPack->Rec.Kind, kNativeService, kForeignService, kClient)) {
			// Для обозначенных видов записи timestamp нулевой с целью обеспечения уникальности индекса {Kind, BI, TimeStamp}
			pPack->Rec.TimeStamp = 0;
		}
	}
	SBuffer cbuf;
	SSerializeContext sctx;
	bool   do_destroy_lob = false;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(outer_id) {
			StyloQCore::StoragePacket preserve_pack;
			THROW(GetPeerEntry(outer_id, &preserve_pack) > 0);
			if(pPack) {
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
					// @v11.3.8 copyBufFrom(&pPack->Rec); // @v11.3.4 @fix
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
					S_GUID guid(SCtrGenerate_);
					memcpy(pPack->Rec.BI, &guid, sizeof(guid));
				}
			}
			pPack->Rec.ID = 0;
			copyBufFrom(&pPack->Rec);
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

/*static*/PPIDArray & StyloQCore::MakeLinkObjTypeList(PPIDArray & rList)
{
	rList.Z().addzlist(PPOBJ_USR, PPOBJ_PERSON, PPOBJ_DBDIV, PPOBJ_CASHNODE, 0L);
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
	StyloQAssignObjParam() : StqID(0)
	{
	}
	PPID   StqID;
	PPObjID Oid;
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
		PPIDArray obj_type_list;
		SetupObjListCombo(this, CTLSEL_STQCLIMATCH_OT, Data.Oid.Obj, &StyloQCore::MakeLinkObjTypeList(obj_type_list));
		SetupObjGroupCombo(Data.Oid.Obj);
		setCtrlString(CTL_STQCLIMATCH_INFO, Data.EntryInfo);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTLSEL_STQCLIMATCH_OT, &Data.Oid.Obj);
		getCtrlData(CTLSEL_STQCLIMATCH_OBJ, &Data.Oid.Id);
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
		THROW_SL(rBuf.Write(&_PPConst.Signature_StqDbSymbToSvcIdMap, sizeof(_PPConst.Signature_StqDbSymbToSvcIdMap)));
		THROW_SL(rBuf.Write(&cur_ver, sizeof(cur_ver)));
	}
	else if(dir < 0) {
		SVerT ver;
		uint8 signature[sizeof(_PPConst.Signature_StqDbSymbToSvcIdMap)];
		THROW_SL(rBuf.Read(signature, sizeof(signature)));
		THROW(memcmp(signature, &_PPConst.Signature_StqDbSymbToSvcIdMap, sizeof(_PPConst.Signature_StqDbSymbToSvcIdMap)) == 0);
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
		while(f_in.ReadLine(temp_buf)) {
			if(temp_buf.Chomp().Strip().Divide(' ', ident_buf, url_buf) > 0 && ident_buf.NotEmptyS() && url_buf.NotEmptyS()) {
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
	SBinaryChunk bc;
	SSerializeContext sctx;
	PPDbEntrySet2 dbes;
	PPIniFile ini_file;
	DbLoginBlock dlb;
	PPSession::LimitedDatabaseBlock * p_ldb = 0;
	StyloQCore::SvcDbSymbMap map;
	dbes.ReadFromProfile(&ini_file, 1, 1);
	for(uint i = 0; i < dbes.GetCount(); i++) {
		if(dbes.GetByPos(i, &dlb)) {
			if(dlb.GetAttr(DbLoginBlock::attrDbSymb, db_symb) && db_symb.NotEmpty()) {
				p_ldb = DS.LimitedOpenDatabase(db_symb, PPSession::lodfReference|PPSession::lodfStyloQCore);
				if(p_ldb) {
					if(p_ldb->P_Sqc) {
						StyloQCore::StoragePacket sp;
						if(p_ldb->P_Sqc->GetOwnPeerEntry(&sp) > 0) {
							if(sp.Pool.Get(SSecretTagPool::tagSvcIdent, &bc)) {
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
								}
							}
						}
					}
					ZDELETE(p_ldb);
				}
			}
		}
	}
	THROW(map.Store(0));
	StyloQCore::SvcDbSymbMap::Dump(0, 0);
	CATCHZOK
	delete p_ldb;
	return ok;
}

int PPObjStyloQBindery::AssignObjToClientEntry(PPID id)
{
	int    ok = -1;
	StyloQAssignObjParam param;
	StyloQCore::StoragePacket pack;
	if(id && P_Tbl->GetPeerEntry(id, &pack) > 0) {
		if(pack.Rec.Kind == StyloQCore::kClient) {
			param.StqID = pack.Rec.ID;
			param.Oid.Set(pack.Rec.LinkObjType, pack.Rec.LinkObjID);
			param.EntryInfo.EncodeMime64(pack.Rec.BI, sizeof(pack.Rec.BI));
			if(PPDialogProcBody <StyloQAssignObjDialog, StyloQAssignObjParam> (&param) > 0) {
				pack.Rec.LinkObjType = param.Oid.Obj;
				pack.Rec.LinkObjID = param.Oid.Id;
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
				THROW(face_pack.ToJson(temp_buf));
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

bool FASTCALL PPStyloQInterchange::InterchangeParam::IsEq(const PPStyloQInterchange::InterchangeParam & rS) const
{
	return (Capabilities == rS.Capabilities && SvcIdent == rS.SvcIdent && LoclAddendum == rS.LoclAddendum && AccessPoint == rS.AccessPoint && 
		CommandJson == rS.CommandJson && Blob == rS.Blob);
}

PPStyloQInterchange::Document::LotExtCode::LotExtCode() : Flags(0), BoxRefN(0)
{
	PTR32(Code)[0] = 0;
}

PPStyloQInterchange::Document::ValuSet::ValuSet() : Qtty(0.0), Cost(0.0), Price(0.0)
{
}

PPStyloQInterchange::Document::TransferItem::TransferItem() : GoodsID(0), UnitID(0), Flags(0)
{
}

PPStyloQInterchange::Document::BookingItem::BookingItem() : PrcID(0), GoodsID(0), Flags(0), ReqTime(ZERODATETIME), EstimatedDurationSec(0)
{
}

PPStyloQInterchange::Document::Document() : ID(0), CreationTime(ZERODATETIME), Time(ZERODATETIME), DueTime(ZERODATETIME), OpID(0),
	ClientID(0), DlvrLocID(0)
{
}

int PPStyloQInterchange::Document::FromJsonObject(const SJson * pJsObj)
{
	int    ok = 1;
	THROW(SJson::IsObject(pJsObj));
	for(const SJson * p_cur = pJsObj->P_Child; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->P_Child) {
			if(p_cur->Text.IsEqiAscii("id")) {
				ID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("uuid")) {
				Uuid.FromStr(p_cur->P_Child->Text);
			}
			else if(p_cur->Text.IsEqiAscii("code")) {
				Code = SJson::Unescape(p_cur->P_Child->Text);
			}
			else if(p_cur->Text.IsEqiAscii("svcident")) {
				SvcIdent.FromMime64(SJson::Unescape(p_cur->P_Child->Text));
			}
			else if(p_cur->Text.IsEqiAscii("crtm")) {
				CreationTime.Set(SJson::Unescape(p_cur->P_Child->Text), DATF_ISO8601|DATF_CENTURY, 0);
			}
			else if(p_cur->Text.IsEqiAscii("tm")) {
				Time.Set(SJson::Unescape(p_cur->P_Child->Text), DATF_ISO8601|DATF_CENTURY, 0);
			}
			else if(p_cur->Text.IsEqiAscii("duetm")) {
				DueTime.Set(SJson::Unescape(p_cur->P_Child->Text), DATF_ISO8601|DATF_CENTURY, 0);
			}
			else if(p_cur->Text.IsEqiAscii("opid")) {
				OpID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("cliid")) {
				ClientID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("dlvrlocid")) {
				DlvrLocID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("memo")) {
				Memo = SJson::Unescape(p_cur->P_Child->Text);
			}
			else if(p_cur->Text.IsEqiAscii("ti_list")) {
				if(SJson::IsArray(p_cur->P_Child)) {
					for(const SJson * p_js_ti = p_cur->P_Child->P_Child; p_js_ti; p_js_ti = p_js_ti->P_Next) {
						if(SJson::IsObject(p_js_ti)) {
							TransferItem * p_new_item = TiList.CreateNewItem();
							THROW_SL(p_new_item);
							for(const SJson * p_ti_cur = p_js_ti->P_Child; p_ti_cur; p_ti_cur = p_ti_cur->P_Next) {
								if(p_ti_cur->Text.IsEqiAscii("goodsid")) {
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
											if(p_set_cur->Text.IsEqiAscii("qtty") || p_set_cur->Text.IsEqiAscii("qty")) {
												p_new_item->Set.Qtty = p_set_cur->P_Child->Text.ToReal();
											}
											else if(p_set_cur->Text.IsEqiAscii("cost")) {
												p_new_item->Set.Cost = p_set_cur->P_Child->Text.ToReal();
											}
											else if(p_set_cur->Text.IsEqiAscii("price")) {
												p_new_item->Set.Price = p_set_cur->P_Child->Text.ToReal();
											}
										}
									}
								}
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
								if(p_ti_cur->Text.IsEqiAscii("goodsid")) {
									p_new_item->GoodsID = p_ti_cur->P_Child->Text.ToLong();
								}
								if(p_ti_cur->Text.IsEqiAscii("prcid")) {
									p_new_item->PrcID = p_ti_cur->P_Child->Text.ToLong();
								}
								else if(p_ti_cur->Text.IsEqiAscii("flags")) {
									p_new_item->Flags = p_ti_cur->P_Child->Text.ToLong();
								}
								else if(p_ti_cur->Text.IsEqiAscii("reqtime")) {
									p_new_item->ReqTime.Set(SJson::Unescape(p_ti_cur->P_Child->Text), DATF_ISO8601|DATF_CENTURY, 0);
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
	return ok;
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
		Header * p_hdr = static_cast<Header *>(SBuffer::Ptr(SBuffer::GetRdOffs()));
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
	THROW(P_T);
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
		THROW_PP(rOwnPack.Pool.Get(SSecretTagPool::tagSvcIdent, &own_ident), PPERR_SQ_UNDEFSVCID);
		assert(own_ident.Len() > 0);
		own_ident.Mime64(own_ident_hex);
		THROW_PP(rOwnPack.Pool.Get(SSecretTagPool::tagConfig, &temp_bch), PPERR_SQ_UNDEFOWNCFG);
		THROW(StyloQConfig::MakeTransmissionJson(temp_bch.ToRawStr(temp_buf), my_transmission_cfg_json_buf)); // @todo @err
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
			if(P_T) {
				PPID   id = 0;
				StyloQCore::StoragePacket new_storage_pack;
				new_storage_pack.Rec.Kind = StyloQCore::kForeignService;
				memcpy(new_storage_pack.Rec.BI, svc_ident.PtrC(), svc_ident.Len());
				new_storage_pack.Pool.Put(SSecretTagPool::tagSvcIdent, svc_ident);
				new_storage_pack.Pool.Put(SSecretTagPool::tagClientIdent, cli_ident);
				new_storage_pack.Pool.Put(SSecretTagPool::tagSrpVerifier, __v);
				new_storage_pack.Pool.Put(SSecretTagPool::tagSrpVerifierSalt, __s);
				THROW(P_T->PutPeerEntry(&id, &new_storage_pack, 1));
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::Registration_ServiceReply(const RoundTripBlock & rB, const StyloQProtocol & rPack)
{
	int    ok = 1;
	if(P_T) {
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
			}
			else {
				// Попытка клиента повторно зарегистрироваться c другими параметрами авторизации: в общем случае это - ошибка. Клиент не может менять свои регистрационные данные.
				// В дальнейшем возможны варианты.
				//
				// Attention! Логическая ошибка: клиент сейча генерирует пару {s, v} каждый раз новую, потому сравнение с существующими значениями неверно!
				//
				SBinaryChunk temp_bc;
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
		}
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
	THROW_SL(c.HttpPost(rB.Url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, content_buf, &wr_stream));
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
			THROW(SendHttpQuery(rB, tp, &sess_secret));
			rReply = tp.P;
			THROW(tp.CheckRepError());
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
			THROW(tp.CheckRepError()); // Сервис вернул ошибку: можно уходить
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
	if(P_T) {
		SString temp_buf;
		SString out_buf;
		PPGetFilePath(PPPATH_OUT, "styloqsec.dump", temp_buf);
		SFile f_out(temp_buf, SFile::mWrite);
		if(f_out.IsValid()) {
			StyloQSecTbl::Key0 k0;
			StyloQCore::StoragePacket spack;
			SBinaryChunk chunk;
			MEMSZERO(k0);
			out_buf.Z().Cat("ID").Tab().Cat("Kind").Tab().Cat("CorrespondID").Tab().Cat("BI").Tab().Cat("SessExpiration");
			f_out.WriteLine(out_buf.CR());
			if(P_T->search(0, &k0, spFirst)) do {
				if(P_T->ReadCurrentPacket(&spack)) {
					temp_buf.Z().EncodeMime64(spack.Rec.BI, sizeof(spack.Rec.BI));
					out_buf.Z().Cat(spack.Rec.ID).Tab().Cat(spack.Rec.Kind).Tab().Cat(spack.Rec.CorrespondID).Tab().
						Cat(temp_buf).Tab().Cat(spack.Rec.Expiration, DATF_ISO8601|DATF_CENTURY, 0);
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
		}
	}
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
			rPack.Rec.Kind = (direction > 0) ? StyloQCore::kDocOutcominig : StyloQCore::kDocIncoming;
			rPack.Rec.TimeStamp = Timestamp;
			{
				//"doctype"
				SJson js(SJson::tOBJECT);
				if(docType == StyloQCore::doctypCommandList)
					js.InsertString("doctype", "commandlist");
				js.InsertString("time", temp_buf.Z().Cat(Timestamp));
				js.Insert("expir_time_sec", json_new_number(temp_buf.Z().Cat(3 * 24 * 3600)));
				{
					SJson * p_array = new SJson(SJson::tARRAY);
					{
						SJson * p_jitem = new SJson(SJson::tOBJECT);
						p_jitem->InsertString("uuid", temp_buf.Z().Cat(S_GUID(SCtrGenerate_)));
						p_jitem->InsertString("name", "command #1");
						p_jitem->InsertString("descr", "command #1 description");
						// @todo transmit image
						json_insert_child(p_array, p_jitem);
					}
					{
						SJson * p_jitem = new SJson(SJson::tOBJECT);
						p_jitem->InsertString("uuid", temp_buf.Z().Cat(S_GUID(SCtrGenerate_)));
						p_jitem->InsertString("name", "command #2");
						p_jitem->InsertString("descr", "command #2 description");
						// @todo transmit image
						json_insert_child(p_array, p_jitem);
					}
					{
						SJson * p_jitem = new SJson(SJson::tOBJECT);
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

PPStyloQInterchange::BlobInfo::BlobInfo() : BlobN(0), HashAlg(0)
{
}

PPStyloQInterchange::BlobInfo & PPStyloQInterchange::BlobInfo::Z()
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
	if(P_T) {
		StyloQCore::StoragePacket pack;
		StyloQCore::StoragePacket corr_pack;
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
	}
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
	if(P_T) {
		bool    do_generate_public_ident = true;
		SString temp_buf;
		SBinaryChunk svc_ident;
		SBinaryChunk svc_acsp;
		SBinaryChunk temp_bch;
		StyloQCore::StoragePacket svc_pack;
		//
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
			rB.Sess.Put(SSecretTagPool::tagRoundTripIdent, &rB.Uuid, sizeof(rB.Uuid)); // ? 
		}
		// } @v11.1.12 
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::KexServiceReply(SSecretTagPool & rSessCtx, const SSecretTagPool & rCli, BIGNUM * pDebugPubX, BIGNUM * pDebugPubY)
{
	int    ok = 1;
	//SBinaryChunk cli_acsp;
	SBinaryChunk cli_ident;
	//rCli.Get(SSecretTagPool::tagSvcAccessPoint, &cli_acsp);
	THROW_PP(rCli.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_UNDEFCLIID);
	int  fsks = FetchSessionKeys(StyloQCore::kClient, rSessCtx, cli_ident);
	THROW(fsks);
	ok = fsks;
	THROW(KexGenerateKeys(rSessCtx, pDebugPubX, pDebugPubY));
	//if(fsks == fsksNewSession) 
	{
		SBinaryChunk my_public;
		rSessCtx.Get(SSecretTagPool::tagSessionPublicKey, &my_public);
		THROW(my_public.Len());
		THROW(KexGenerageSecret(rSessCtx, rCli));
	}
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
			WinRegKey reg_key_r(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 1);
			reg_key_r.GetString(_PPConst.WrParam_StyloQLoclMachineUuid, temp_buf);
			if(temp_buf.NotEmpty() && lmid.FromStr(temp_buf))
				result = lmid;
		}
		if(!result) {
			{
				lmid.Generate();
				WinRegKey reg_key_w(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 0);
				temp_buf.Z().Cat(lmid, S_GUID::fmtLower|S_GUID::fmtPlain);
				reg_key_w.PutString(_PPConst.WrParam_StyloQLoclMachineUuid, temp_buf);
			}
			{ // Так как нам очень важно, что ключ постоянно один для всех сеансов на этой машине, 
				// то мы не просто воспользуемся сгенерированным ключем, а для страховки извлечем его и реестра.
				WinRegKey reg_key_r(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 1);
				reg_key_r.GetString(_PPConst.WrParam_StyloQLoclMachineUuid, temp_buf);
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
	rOwnPack.Pool.Get(SSecretTagPool::tagSvcIdent, &rP.SvcIdent);
	{
		SBinaryChunk bc_cfg;
		bool own_cfg_used = false;
		if(rOwnPack.Pool.Get(SSecretTagPool::tagConfig, &bc_cfg)) {
			assert(bc_cfg.Len());
			SString temp_buf;
			StyloQConfig cfg_pack;
			if(cfg_pack.FromJson(bc_cfg.ToRawStr(temp_buf))) {
				const int locl_accsp_tag_list[] = { StyloQConfig::tagLoclUrl, StyloQConfig::tagLoclMqbAuth, StyloQConfig::tagLoclMqbSecret };
				const int glob_accsp_tag_list[] = { StyloQConfig::tagUrl, StyloQConfig::tagMqbAuth, StyloQConfig::tagMqbSecret };
				const int * p_glob_accsp_tag_list = (flags & (smqbpfLocalMachine|smqbpfLocalSession)) ? locl_accsp_tag_list : glob_accsp_tag_list;
				if(cfg_pack.Get(p_glob_accsp_tag_list[0], temp_buf)) {
					rP.MqbInitParam.Host = temp_buf.Strip();
					if(cfg_pack.Get(p_glob_accsp_tag_list[1], temp_buf))
						rP.MqbInitParam.Auth = temp_buf;
					if(cfg_pack.Get(p_glob_accsp_tag_list[2], temp_buf))
						rP.MqbInitParam.Secret = temp_buf;
					rP.MqbInitParam.VHost = p_vhost;
					rP.MqbInitParam.Method = 1;
					own_cfg_used = true;
				}
			}
		}
		/* Не будем применять конфигурацию Albatross в случае дефолта!
		if(!own_cfg_used) {
			THROW(PPMqbClient::SetupInitParam(rP.MqbInitParam, p_vhost, 0));
		}*/
		THROW(own_cfg_used);
		//
		THROW(rP.MqbInitParam.Host.NotEmpty());
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
	}
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
					THROW_SL(other_face.ToJson(temp_buf));
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

int StyloQCore::IndexingContent_Json(PPFtsIterface::TransactionHandle * pFtsTra, PPTextAnalyzer * pTa, const char * pJsText)
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
	p_js_doc = SJson::Parse(pJsText);
	THROW_SL(p_js_doc);
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
					PPFtsIterface::Entity entity;
					entity.Scope = PPFtsIterface::scopeStyloQSvc;
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
							PPFtsIterface::Entity entity;
							entity.Scope = PPFtsIterface::scopeStyloQSvc;
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
							PPFtsIterface::Entity entity;
							entity.Scope = PPFtsIterface::scopeStyloQSvc;
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
							PPFtsIterface::Entity entity;
							entity.Scope = PPFtsIterface::scopeStyloQSvc;
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
							PPFtsIterface::Entity entity;
							entity.Scope = PPFtsIterface::scopeStyloQSvc;
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

int PPStyloQInterchange::MakeIndexingRequestCommand(const StyloQCore::StoragePacket * pOwnPack, const StyloQCommandList::Item * pCmd, long expirationSec, 
	PPObjIDArray & rOidList, SString & rResult)
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
						if(p_base_filt->GetSignature() == pattern_filt.GetSignature()) {
							p_param_ = static_cast<StyloQIndexingParam *>(p_base_filt);
						}
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
		SString my_face_json_buf;
		SBinaryChunk temp_bch;
		SJson * p_js_svc = new SJson(SJson::tOBJECT);
		p_js_svc->InsertString("ident", own_svc_ident.Mime64(temp_buf).Escape());
		if(pOwnPack->Pool.Get(SSecretTagPool::tagSelfyFace, &temp_bch)) {
			StyloQFace my_face;
			if(my_face.FromJson(temp_bch.ToRawStr(my_face_json_buf))) {
				if(my_face.Get(StyloQFace::tagCommonName, 0, temp_buf))
					p_js_svc->InsertString("nm", temp_buf.Escape());
				if(my_face.Get(StyloQFace::tagDescr, 0, temp_buf))
					p_js_svc->InsertString("descr", temp_buf.Escape());
				if(my_face.Get(StyloQFace::tagGLN, 0, temp_buf))
					p_js_svc->InsertString("gln", temp_buf.Escape());
				if(my_face.Get(StyloQFace::tagRuINN, 0, temp_buf))
					p_js_svc->InsertString("ruinn", temp_buf.Escape());
				if(my_face.Get(StyloQFace::tagRuKPP, 0, temp_buf))
					p_js_svc->InsertString("rukpp", temp_buf.Escape());
				if(my_face.Get(StyloQFace::tagPhone, 0, temp_buf))
					p_js_svc->InsertString("phone", temp_buf.Escape());
				if(my_face.Get(StyloQFace::tagCityName, 0, temp_buf))
					p_js_svc->InsertString("city", temp_buf.Escape());
				if(my_face.Get(StyloQFace::tagEMail, 0, temp_buf))
					p_js_svc->InsertString("email", temp_buf.Escape());
			}
		}
		js.Insert("service", p_js_svc);
		rOidList.Add(PPOBJ_STYLOQBINDERY, pOwnPack->Rec.ID);
	}
	{
		PPIDArray goodsgrp_id_list;
		PPIDArray brand_id_list;
		PPObjGoods goods_obj;
		Goods2Tbl::Rec goods_rec;
		if(!p_param_ || p_param_->Flags & StyloQIndexingParam::fGoods) {
			SJson * p_js_goodslist = 0;
			GoodsFilt gf;
			BarcodeArray bc_list;
			gf.Flags |= (GoodsFilt::fHideGeneric|GoodsFilt::fHidePassive);
			if(p_param_ && p_param_->GoodsGroupID)
				gf.GrpIDList.Add(p_param_->GoodsGroupID);
			//StrAssocArray goods_list;
			//GoodsIterator::GetListByFilt(&gf, &goods_list)
			for(GoodsIterator gi(&gf, 0); gi.Next(&goods_rec) > 0;) {
				goodsgrp_id_list.addnz(goods_rec.ParentID);
				brand_id_list.addnz(goods_rec.BrandID);
				SETIFZ(p_js_goodslist, new SJson(SJson::tARRAY));
				SJson * p_js_ware = new SJson(SJson::tOBJECT);
				p_js_ware->InsertInt("id", goods_rec.ID);
				p_js_ware->InsertString("nm", (temp_buf = goods_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
				if(goods_obj.P_Tbl->ReadBarcodes(goods_rec.ID, bc_list) > 0 && bc_list.getCount()) {
					SJson * p_js_codelist = new SJson(SJson::tARRAY);
					for(uint bcidx = 0; bcidx < bc_list.getCount(); bcidx++) {
						const BarcodeTbl::Rec & r_bc = bc_list.at(bcidx);
						SJson * p_js_item = new SJson(SJson::tOBJECT);
						p_js_item->InsertString("cod", (temp_buf = r_bc.Code).Transf(CTRANSF_INNER_TO_UTF8).Escape());
						p_js_codelist->InsertChild(p_js_item);
					}
					p_js_ware->Insert("code_list", p_js_codelist);
				}
				p_js_goodslist->InsertChild(p_js_ware);
				rOidList.Add(PPOBJ_GOODS, goods_rec.ID);
			}
			if(p_js_goodslist) {
				js.Insert("goods_list", p_js_goodslist);
			}
		}
		if(!p_param_ || p_param_->Flags & StyloQIndexingParam::fGoodsGroups) {
			if(goodsgrp_id_list.getCount()) {
				goodsgrp_id_list.sortAndUndup();
				SJson * p_js_grplist = new SJson(SJson::tARRAY);
				for(uint i = 0; i < goodsgrp_id_list.getCount(); i++) {
					const PPID item_id = goodsgrp_id_list.get(i);
					if(goods_obj.Fetch(item_id, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GROUP) {
						SJson * p_js_item = new SJson(SJson::tOBJECT);
						p_js_item->InsertInt("id", goods_rec.ID);
						p_js_item->InsertString("nm", (temp_buf = goods_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
						p_js_grplist->InsertChild(p_js_item);
						rOidList.Add(PPOBJ_GOODSGROUP, goods_rec.ID);
					}
				}
				js.Insert("goodsgroup_list", p_js_grplist);
			}
		}
		if(!p_param_ || p_param_->Flags & StyloQIndexingParam::fBrands) {
			if(brand_id_list.getCount()) {
				PPObjBrand brand_obj;
				PPBrandPacket brand_pack;
				brand_id_list.sortAndUndup();
				SJson * p_js_brandlist = new SJson(SJson::tARRAY);
				for(uint i = 0; i < brand_id_list.getCount(); i++) {
					const PPID item_id = brand_id_list.get(i);
					brand_pack.Init();
					if(brand_obj.Get(item_id, &brand_pack) > 0) {
						SJson * p_js_item = new SJson(SJson::tOBJECT);
						p_js_item->InsertInt("id", brand_pack.Rec.ID);
						p_js_item->InsertString("nm", (temp_buf = brand_pack.Rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
						p_js_brandlist->InsertChild(p_js_item);
						rOidList.Add(PPOBJ_BRAND, brand_pack.Rec.ID);
					}
				}
				js.Insert("brand_list", p_js_brandlist);
			}
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
					SETIFZ(p_js_prclist, new SJson(SJson::tARRAY));
					SJson * p_js_item = new SJson(SJson::tOBJECT);
					p_js_item->InsertInt("id", prc_rec.ID);
					p_js_item->InsertString("nm", (temp_buf = prc_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
					p_js_prclist->InsertChild(p_js_item);
					rOidList.Add(PPOBJ_PROCESSOR, prc_rec.ID);
				}
			}
			if(p_js_prclist) {
				js.Insert("processor_list", p_js_prclist);
			}
		}
	}
	THROW(js.ToStr(rResult));
	// @debug {
	{
		SString out_file_name;
		PPGetFilePath(PPPATH_OUT, "stq-indexingquery.json", out_file_name);
		SFile f_out(out_file_name, SFile::mWrite);
		f_out.WriteLine(rResult);
	}
	{
		P_T->IndexingContent_Json(0, 0, rResult);
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
		//if(sp.Pool.Get(SSecretTagPool::tagSvcIdent, &temp_bch)) {
		//assert(temp_bch.Len() > 0);
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
	THROW(rParam.SvcIdent.Len());
	THROW(rParam.AccessPoint.NotEmpty());
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
	SJson * p_reply = 0;
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
			if(P_T->SearchGlobalIdentEntry(StyloQCore::kForeignService, rParam.SvcIdent, &svc_pack) > 0) {
				SBinaryChunk raw_svc_cfg;
				if(svc_pack.Pool.Get(SSecretTagPool::tagConfig, &raw_svc_cfg)) {
					StyloQConfig svc_cfg;
					if(svc_cfg.FromJson(raw_svc_cfg.ToRawStr(temp_buf)) && svc_cfg.Get(StyloQConfig::tagUrl, temp_buf)) {
						rParam.AccessPoint = temp_buf;
						//rtb.Other.Put(SSecretTagPool::tagSvcAccessPoint, temp_buf.cptr(), temp_buf.Len());
						svc_cfg.Get(StyloQConfig::tagMqbAuth, rParam.MqbAuth);
						svc_cfg.Get(StyloQConfig::tagMqbSecret, rParam.MqbSecret);
					}					
				}
			}
		}
	}
	THROW(Helper_DoInterchange(rParam, rReply));
	CATCHZOK
	delete p_reply;
	return ok;
}

//long CreateOnetimePass(PPID userID); // @v11.1.9
long OnetimePass(PPID userID); // @v11.1.9

int PPStyloQInterchange::ProcessCommand_PersonEvent(StyloQCommandList::Item & rCmdItem, const StyloQCore::StoragePacket & rCliPack, const SGeoPosLL & rGeoPos)
{
	int    ok = 1;
	PPID   new_id = 0;
	assert(rCmdItem.BaseCmdId == StyloQCommandList::sqbcPersonEvent);
	{
		SSerializeContext sctx;
		const LDATETIME _now = getcurdatetime_();
		PPObjPersonEvent psnevobj;
		PPPsnEventPacket pe_pack;
		THROW(rCmdItem.Param.GetAvailableSize());
		THROW(psnevobj.SerializePacket(-1, &pe_pack, rCmdItem.Param, &sctx));
		pe_pack.Rec.Dt = _now.d;
		pe_pack.Rec.Tm = _now.t;
		THROW(pe_pack.Rec.OpID);
		if(pe_pack.Rec.PersonID == ROBJID_CONTEXT) {
			THROW(FetchPersonFromClientPacket(rCliPack, &pe_pack.Rec.PersonID) > 0);
		}
		if(pe_pack.Rec.SecondID == ROBJID_CONTEXT) {
			THROW(FetchPersonFromClientPacket(rCliPack, &pe_pack.Rec.SecondID) > 0);
		}
		THROW(psnevobj.PutPacket(&new_id, &pe_pack, 1));
	}
	CATCHZOK
	return ok;
}

PPStyloQInterchange::Stq_CmdStat_MakeRsrv_Response::Stq_CmdStat_MakeRsrv_Response() : 
	GoodsCount(0), GoodsGroupCount(0), BrandCount(0), ClientCount(0), DlvrLocCount(0), PrcCount(0)
{
}

int PPStyloQInterchange::MakeRsrvPriceListResponse_ExportClients(const SBinaryChunk & rOwnIdent, const PPStyloPalmPacket * pPack, SJson * pJs, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	int    ok = 1;
	SString temp_buf;
	PPObjBill * p_bobj = BillObj;
	const  int use_omt_paym_amt = BIN(CConfig.Flags2 & CCFLG2_USEOMTPAYMAMT);
	DebtTrnovrViewItem debt_item;
	PPViewDebtTrnovr * p_debt_view = 0; // @stub
	/*if(palmFlags & PLMF_EXPCLIDEBT && !(palmFlags & PLMF_BLOCKED)) {
		DebtTrnovrFilt debt_filt;
		debt_filt.AccSheetID = acsID;
		debt_filt.Flags |= DebtTrnovrFilt::fDebtOnly;
		THROW_MEM(SETIFZ(rBlk.P_DebtView, new PPViewDebtTrnovr));
		THROW(rBlk.P_DebtView->Init_(&debt_filt));
	}*/
	if(!(pPack->Rec.Flags & PLMF_BLOCKED)) {
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
		PPOprKind opk_rec;
		BlobInfo blob_info;
		PPID   acs_id = GetOpData(pPack->Rec.OrderOpID, &opk_rec) ? opk_rec.AccSheetID : 0;
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
				PPID   quot_kind_id = 0;
				PPClientAgreement cli_agt;
				if(ar_obj.GetClientAgreement(ar_item.ID, cli_agt, 0) > 0) {
					quot_kind_id = cli_agt.DefQuotKindID;
					if(cli_agt.Flags & AGTF_DONTUSEMINSHIPMQTTY)
						_flags |= CLIENTF_DONTUSEMINSHIPMQTTY;
				}
				if(acs_rec.Assoc == PPOBJ_PERSON) {
					psn_obj.GetRegNumber(ar_item.ObjID, PPREGT_TPID, inn_buf);
					psn_obj.GetRegNumber(ar_item.ObjID, PPREGT_KPP, kpp_buf);
				}
				if((pPack->Rec.Flags & PLMF_EXPSTOPFLAG) && (ar_item.Flags & ARTRF_STOPBILL)) {
					_flags |= CLIENTF_BLOCKED;
				}
				const bool valid_debt = (p_debt_view && p_debt_view->GetItem(ar_item.ID, 0L, 0L, &debt_item) > 0);
				//
				//if(GetBlobInfo(const SBinaryChunk & rOwnIdent, PPObjID oid, uint blobN, BlobInfo & rInfo) const;
				//
				SJson * p_js_obj = new SJson(SJson::tOBJECT);
				p_js_obj->InsertInt("id", ar_item.ID);
				p_js_obj->InsertString("nm", (temp_buf = ar_item.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
				if(inn_buf.NotEmpty()) {
					p_js_obj->InsertString("ruinn", inn_buf.Transf(CTRANSF_INNER_TO_UTF8).Escape());
					if(kpp_buf.NotEmpty())
						p_js_obj->InsertString("rukpp", kpp_buf.Transf(CTRANSF_INNER_TO_UTF8).Escape());
				}
				if(quot_kind_id) {
					p_js_obj->InsertInt("quotkid", quot_kind_id);
				}
				if(valid_debt) {
					p_js_obj->InsertDouble("debt", debt_item.Debt, MKSFMTD(0, 2, NMBF_NOTRAILZ));
				}
				if(_flags & CLIENTF_DONTUSEMINSHIPMQTTY)
					p_js_obj->InsertBool("dontuseminshipmqty", true);
				if(_flags & CLIENTF_BLOCKED)
					p_js_obj->InsertBool("blocked", true);
				if(acs_rec.Assoc == PPOBJ_PERSON) {
					{
						const PPObjID oid(PPOBJ_PERSON, ar_item.ObjID);
						if(GetBlobInfo(rOwnIdent, oid, 0, blob_info, 0)) {
							assert(blob_info.Signature.Len() && blob_info.Signature.IsAscii());
							p_js_obj->InsertString("imgblobs", blob_info.Signature);
							if(pStat)
								pStat->BlobOidList.Add(oid.Obj, oid.Id);
						}
					}
					SJson * p_js_dlvrloc_list = 0;
					dlvr_loc_list.clear();
					THROW(psn_obj.GetDlvrLocList(ar_item.ObjID, &dlvr_loc_list));
					for(uint i = 0; i < dlvr_loc_list.getCount(); i++) {
						const PPID dlvr_loc_id = dlvr_loc_list.at(i);
						loc_obj.GetAddress(dlvr_loc_id, 0, addr);
						if(addr.NotEmptyS()) {
							SJson * p_js_adr = new SJson(SJson::tOBJECT);
							p_js_adr->InsertInt("id", dlvr_loc_id);
							p_js_adr->InsertString("addr", (temp_buf = addr).Transf(CTRANSF_INNER_TO_UTF8).Escape());
							SETIFZ(p_js_dlvrloc_list, new SJson(SJson::tARRAY));
							p_js_dlvrloc_list->InsertChild(p_js_adr);
							if(pStat)
								pStat->DlvrLocCount++;
						}
					}
					if(p_js_dlvrloc_list)
						p_js_obj->Insert("dlvrloc_list", p_js_dlvrloc_list);
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
								SJson * p_js_debt_entry = new SJson(SJson::tOBJECT);
								p_js_debt_entry->InsertInt("id", p_pb_item->ID);
								p_js_debt_entry->InsertString("cod", (temp_buf = bill_rec.Code).Transf(CTRANSF_INNER_TO_UTF8).Escape());
								temp_buf.Z().Cat(bill_rec.Dt, DATF_ISO8601|DATF_CENTURY);
								p_js_debt_entry->InsertString("dat", temp_buf);
								p_js_debt_entry->InsertDouble("amt", amt, MKSFMTD(0, 2, NMBF_NOTRAILZ));
								p_js_debt_entry->InsertDouble("debt", debt, MKSFMTD(0, 2, NMBF_NOTRAILZ));
								p_js_debt_entry->InsertInt("agentid", bill_ext.AgentID);
								SETIFZ(p_js_debtlist, new SJson(SJson::tARRAY));
								p_js_debtlist->InsertChild(p_js_debt_entry);
							}
						}
					}
					if(p_js_debtlist)
						p_js_obj->Insert("debt_list", p_js_debtlist);
				}
				SETIFZ(p_js_client_list, new SJson(SJson::tARRAY));
				p_js_client_list->InsertChild(p_js_obj);
				if(pStat)
					pStat->ClientCount++;
			}
		}
		if(p_js_client_list)
			pJs->Insert("client_list", p_js_client_list);
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::MakeRsrvPriceListResponse_ExportGoods(const SBinaryChunk & rOwnIdent, const PPStyloPalmPacket * pPack, SJson * pJs, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	/*
		quotkind_list [ { id; nm } ]
		goodsgroup_list [ { id; parid; nm } ]
		warehouse_list [ { id; nm } ]
		uom_list [ { id; nm; (fragmentation|rounding|int) } ]
		brand_list [ { id; nm } ]
		goods_list [ { id; nm; parid; uomid; code_list [ { cod; qty } ]; brandid; upp; price; stock; (ordqtymult|ordminqty); quot_list [ { id; val } ] ]
	*/
	struct InnerGoodsEntry { // @flat
		InnerGoodsEntry(PPID goodsID) : GoodsID(goodsID), Rest(0.0), Cost(0.0), Price(0.0), UnitPerPack(0.0)
		{
		}
		PPID   GoodsID;
		double Rest;
		double Cost;
		double Price;
		double UnitPerPack;
	};	
	int    ok = 1;
	SString temp_buf;
	PPIDArray  grp_id_list;
	PPIDArray  brand_id_list;
	PPIDArray  unit_id_list;
	PPObjQuotKind qk_obj;
	Goods2Tbl::Rec goods_rec;
	SVector goods_list(sizeof(InnerGoodsEntry));
	PPIDArray temp_loc_list;
	BlobInfo blob_info;
	if(!(pPack->Rec.Flags & PLMF_BLOCKED)) {
		PPObjGoods goods_obj;
		const PPID single_loc_id = pPack->LocList.GetSingle();
		{
			SJson * p_js_list = 0;
			for(uint i = 0; i < pPack->QkList__.GetCount(); i++) {
				const PPID qk_id = pPack->QkList__.Get(i);
				PPQuotKind qk_rec;
				if(qk_obj.Fetch(qk_id, &qk_rec) > 0) {
					SETIFZ(p_js_list, new SJson(SJson::tARRAY));
					SJson * p_jsobj = new SJson(SJson::tOBJECT);
					p_jsobj->InsertInt("id", qk_rec.ID);
					p_jsobj->InsertString("nm", (temp_buf = qk_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
					p_js_list->InsertChild(p_jsobj);
				}
			}
			if(p_js_list)
				pJs->Insert("quotkind_list", p_js_list);
		}
		{
			PPIDArray gt_quasi_unlim_list;
			{
				PPObjGoodsType gt_obj;
				PPGoodsType gt_rec;
				for(SEnum en = gt_obj.Enum(0); en.Next(&gt_rec) > 0;) {
					if(gt_rec.Flags & (GTF_QUASIUNLIM|GTF_UNLIMITED)) 
						gt_quasi_unlim_list.add(gt_rec.ID);
				}
			}
			GoodsRestFilt gr_filt;
			PPViewGoodsRest gr_view;
			gr_filt.LocList = pPack->LocList;
			gr_filt.GoodsGrpID = pPack->Rec.GoodsGrpID;
			gr_filt.CalcMethod = GoodsRestParam::pcmLastLot;
			if(gt_quasi_unlim_list.getCount())
				gr_filt.Flags |= GoodsRestFilt::fNullRest;
			gr_filt.WaitMsgID  = PPTXT_WAIT_GOODSREST;
			THROW(gr_view.Init_(&gr_filt));
			GoodsRestViewItem gr_item;
			for(gr_view.InitIteration(); gr_view.NextIteration(&gr_item) > 0;) {
				if(goods_obj.Fetch(gr_item.GoodsID, &goods_rec) > 0) {
					if(gr_item.Rest > 0.0 || gt_quasi_unlim_list.lsearch(goods_rec.GoodsTypeID)) {
						InnerGoodsEntry goods_entry(gr_item.GoodsID);
						grp_id_list.addnz(goods_rec.ParentID);
						brand_id_list.addnz(goods_rec.BrandID);
						unit_id_list.addnz(goods_rec.UnitID);

						goods_entry.Rest = gr_item.Rest;
						goods_entry.Cost = gr_item.Cost;
						goods_entry.Price = gr_item.Price;
						goods_entry.UnitPerPack = gr_item.UnitPerPack;
						goods_list.insert(&goods_entry);
					}
				}
			}
		}
		grp_id_list.sortAndUndup();
		brand_id_list.sortAndUndup();
		unit_id_list.sortAndUndup();
		if(pPack->Rec.Flags & PLMF_EXPLOC) {
			//
			// Склады
			//
			SJson * p_js_list = 0;
			PPIDArray loc_list;
			PPObjLocation loc_obj;
			LocationTbl::Rec loc_rec;
			loc_obj.GetWarehouseList(&loc_list, 0);
			for(uint i = 0; i < loc_list.getCount(); i++) {
				const PPID loc_id = loc_list.get(i);
				if(pPack->LocList.CheckID(loc_id) && loc_obj.Fetch(loc_id, &loc_rec) > 0) {
					SETIFZ(p_js_list, new SJson(SJson::tARRAY));
					SJson * p_jsobj = new SJson(SJson::tOBJECT);
					p_jsobj->InsertInt("id", loc_rec.ID);
					p_jsobj->InsertString("nm", (temp_buf = loc_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
					p_js_list->InsertChild(p_jsobj);
				}
			}
			if(p_js_list)
				pJs->Insert("warehouse_list", p_js_list);
		}
		{
			//
			// Товарные группы
			//
			SJson * p_js_list = 0;
			PPObjGoodsGroup gg_obj;
			Goods2Tbl::Rec gg_rec;
			for(uint i = 0; i < grp_id_list.getCount(); i++) {
				if(gg_obj.Fetch(grp_id_list.get(i), &gg_rec) > 0) {
					SETIFZ(p_js_list, new SJson(SJson::tARRAY));
					SJson * p_jsobj = new SJson(SJson::tOBJECT);
					p_jsobj->InsertInt("id", gg_rec.ID);
					p_jsobj->InsertInt("parid", gg_rec.ParentID);
					p_jsobj->InsertString("nm", (temp_buf = gg_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
					p_js_list->InsertChild(p_jsobj);
					if(pStat)
						pStat->GoodsGroupCount++;
				}
			}
			if(p_js_list)
				pJs->Insert("goodsgroup_list", p_js_list);
		}
		{
			// Units
			SJson * p_js_list = 0;
			PPObjUnit u_obj;
			PPUnit2 u_rec;
			for(uint i = 0; i < unit_id_list.getCount(); i++) {
				if(u_obj.Fetch(unit_id_list.get(i), &u_rec) > 0) {
					SETIFZ(p_js_list, new SJson(SJson::tARRAY));
					SJson * p_jsobj = new SJson(SJson::tOBJECT);
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
			if(p_js_list)
				pJs->Insert("uom_list", p_js_list);
		}
		if(pPack->Rec.Flags & PLMF_EXPBRAND) {
			//
			// Brands
			//
			SJson * p_js_list = 0;
			PPObjBrand br_obj;
			for(uint i = 0; i < brand_id_list.getCount(); i++) {
				PPBrand brand_rec;
				const PPObjID oid(PPOBJ_BRAND, brand_id_list.get(i));
				if(br_obj.Fetch(oid.Id, &brand_rec) > 0) {
					SETIFZ(p_js_list, new SJson(SJson::tARRAY));
					SJson * p_jsobj = new SJson(SJson::tOBJECT);
					p_jsobj->InsertInt("id", brand_rec.ID);
					p_jsobj->InsertString("nm", (temp_buf = brand_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
					if(GetBlobInfo(rOwnIdent, oid, 0, blob_info, 0)) {
						assert(blob_info.Signature.Len() && blob_info.Signature.IsAscii());
						p_jsobj->InsertString("imgblobs", blob_info.Signature);
						if(pStat)
							pStat->BlobOidList.Add(oid.Obj, oid.Id);
					}
					p_js_list->InsertChild(p_jsobj);
					if(pStat)
						pStat->BrandCount++;
				}
			}
			if(p_js_list)
				pJs->Insert("brand_list", p_js_list);
		}
		{
			SJson * p_goods_list = 0;
			BarcodeArray bc_list;
			for(uint glidx = 0; glidx < goods_list.getCount(); glidx++) {
				const InnerGoodsEntry & r_goods_entry = *static_cast<const InnerGoodsEntry *>(goods_list.at(glidx));
				const PPObjID oid(PPOBJ_GOODS, r_goods_entry.GoodsID);
				if(goods_obj.Fetch(r_goods_entry.GoodsID, &goods_rec) > 0) {
					SJson * p_jsobj = new SJson(SJson::tOBJECT);
					p_jsobj->InsertInt("id", r_goods_entry.GoodsID);
					p_jsobj->InsertString("nm", (temp_buf = goods_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
					p_jsobj->InsertInt("parid", goods_rec.ParentID);
					if(GetBlobInfo(rOwnIdent, oid, 0, blob_info, 0)) {
						assert(blob_info.Signature.Len() && blob_info.Signature.IsAscii());
						p_jsobj->InsertString("imgblobs", blob_info.Signature);
						if(pStat)
							pStat->BlobOidList.Add(oid.Obj, oid.Id);
					}
					if(goods_rec.UnitID) {
						p_jsobj->InsertInt("uomid", goods_rec.UnitID);
					}
					{
						if(goods_obj.ReadBarcodes(goods_rec.ID, bc_list) > 0) {
							SJson * p_js_bcarray = 0;
							for(uint bcidx = 0; bcidx < bc_list.getCount(); bcidx++) {
								const BarcodeTbl::Rec & r_bc_rec = bc_list.at(bcidx);
								int   diag = 0;
								int   std = 0;
								if(PPObjGoods::DiagBarcode(r_bc_rec.Code, &diag, &std, &temp_buf) > 0) {
									SJson * p_bc_item = new SJson(SJson::tOBJECT);
									p_bc_item->InsertString("cod", temp_buf.Transf(CTRANSF_INNER_TO_UTF8).Escape());
									if(r_bc_rec.Qtty > 0.0) {
										p_bc_item->InsertDouble("qty", r_bc_rec.Qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ));
									}
									SETIFZ(p_js_bcarray, new SJson(SJson::tARRAY));
									p_js_bcarray->InsertChild(p_bc_item);
								}
							}
							if(p_js_bcarray)
								p_jsobj->Insert("code_list", p_js_bcarray);
						}
					}
					if(pPack->Rec.Flags & PLMF_EXPBRAND && goods_rec.BrandID) {
						p_jsobj->InsertInt("brandid", goods_rec.BrandID);
					}
					if(r_goods_entry.UnitPerPack > 1.0) {
						p_jsobj->InsertDouble("upp", r_goods_entry.UnitPerPack, MKSFMTD(0, 3, NMBF_NOTRAILZ));
					}
					p_jsobj->InsertDouble("price", r_goods_entry.Price, MKSFMTD(0, 2, NMBF_NOTRAILZ));
					if(r_goods_entry.Rest > 0.0) {
						p_jsobj->InsertDouble("stock", r_goods_entry.Rest, MKSFMTD(0, 3, NMBF_NOTRAILZ));
					}
					{
						//
						// Минимальный заказ
						//
						GoodsStockExt stock;
						if(goods_obj.GetStockExt(goods_rec.ID, &stock, 1) > 0) {
							if(stock.MinShippmQtty > 0.0) {
								if(stock.GseFlags & GoodsStockExt::fMultMinShipm)
									p_jsobj->InsertDouble("ordqtymult", stock.MinShippmQtty, MKSFMTD(0, 3, NMBF_NOTRAILZ));
								else
									p_jsobj->InsertDouble("ordminqty", stock.MinShippmQtty, MKSFMTD(0, 3, NMBF_NOTRAILZ));
							}
						}
					}
					{
						//
						// Котировки
						//
						SJson * p_js_quot_list = 0;
						const LDATE now_date = getcurdate_();
						Transfer * p_trfr = BillObj->trfr;
						for(uint i = 0; i < pPack->QkList__.GetCount(); i++) {
							const PPID qk_id = pPack->QkList__.Get(i);
							double quot = 0.0;
							QuotIdent qi(QIDATE(now_date), single_loc_id, qk_id);
							if(goods_obj.GetQuotExt(r_goods_entry.GoodsID, qi, r_goods_entry.Cost, r_goods_entry.Price, &quot, 1) > 0) {
								SJson * p_js_quot = new SJson(SJson::tOBJECT);
								p_js_quot->InsertInt("id", qk_id);
								p_js_quot->InsertDouble("val", quot, MKSFMTD(0, 2, NMBF_NOTRAILZ));
								SETIFZ(p_js_quot_list, new SJson(SJson::tARRAY));
								p_js_quot_list->InsertChild(p_js_quot);
							}
							else if(!single_loc_id && pPack->LocList.GetCount()) {
								GoodsRestParam grparam;
								grparam.LocList = pPack->LocList.Get();
								grparam.GoodsID = r_goods_entry.GoodsID;
								grparam.DiffParam = GoodsRestParam::_diffLoc;
								p_trfr->GetCurRest(grparam);
								temp_loc_list.clear();
								if(grparam.Total.Rest > 0.0) {
									for(uint grvidx = 0; grvidx < grparam.getCount(); grvidx++) {
										GoodsRestVal & r_grv = grparam.at(grvidx);
										if(r_grv.Rest > 0.0)
											temp_loc_list.add(r_grv.LocID);
									}
								}
								{
									const PPID target_loc_id = temp_loc_list.getCount() ? temp_loc_list.get(0) : pPack->LocList.Get(0);
									QuotIdent qi(QIDATE(now_date), target_loc_id, qk_id);
									if(goods_obj.GetQuotExt(r_goods_entry.GoodsID, qi, r_goods_entry.Cost, r_goods_entry.Price, &quot, 1) > 0) {
										SJson * p_js_quot = new SJson(SJson::tOBJECT);
										p_js_quot->InsertInt("id", qk_id);
										p_js_quot->InsertDouble("val", quot, MKSFMTD(0, 2, NMBF_NOTRAILZ));
										SETIFZ(p_js_quot_list, new SJson(SJson::tARRAY));
										p_js_quot_list->InsertChild(p_js_quot);
									}
								}
							}
						}
						if(p_js_quot_list)
							p_jsobj->Insert("quot_list", p_js_quot_list);
					}
					SETIFZ(p_goods_list, new SJson(SJson::tARRAY));
					p_goods_list->InsertChild(p_jsobj);
					if(pStat)
						pStat->GoodsCount++;
				}
			}
			if(p_goods_list)
				pJs->Insert("goods_list", p_goods_list);
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
			const long sav_pos = P_Box->def ? P_Box->def->_curItem() : 0;
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
			SetDataRef(&Data.PrcList);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlString(CTL_STQATTCPARAM_PRCTTL, Data.PrcTitle);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	};
	DIALOG_PROC_BODY(RsrvAttendancePrereqParamDialog, &rParam);
}

int PPStyloQInterchange::ProcessCommand_RsrvAttendancePrereq(const StyloQCommandList::Item & rCmdItem, const StyloQCore::StoragePacket & rCliPack, const SGeoPosLL & rGeoPos,
	SString & rResult, SString & rDocDeclaration)
{
	int    ok = 1;
	/*
		param

		quotkind_list
		warehouse_list
		goodsgroup_list [ { id; nm } ]
		goods_list
			ware
		processor_list [ { id; nm; goods_list [ {id} ] } ]
	*/
	SString temp_buf;
	PPObjTSession tses_obj;
	PPObjUnit unit_obj;
	PPObjPerson psn_obj;
	StyloQAttendancePrereqParam param;
	Stq_CmdStat_MakeRsrv_Response stat; // @v11.3.8
	BlobInfo blob_info; // @v11.3.8
	StyloQCore::StoragePacket own_pack;
	SBinaryChunk bc_own_ident;
	SJson js(SJson::tOBJECT);
	PPIDArray goods_id_list; // @reusable
	PPIDArray prc_id_list;
	LAssocArray goods_to_prc_list;
	THROW(GetOwnPeerEntry(&own_pack) > 0);
	THROW_PP(own_pack.Pool.Get(SSecretTagPool::tagSvcIdent, &bc_own_ident), PPERR_SQ_UNDEFOWNSVCID);
	{
		if(rCmdItem.GetAttendanceParam(param) > 0) {
			if(param.PrcTitle.NotEmpty()) {
				SJson * p_js_param = new SJson(SJson::tOBJECT);
				p_js_param->InsertString("prctitle", (temp_buf = param.PrcTitle).Transf(CTRANSF_INNER_TO_UTF8).Escape());
				js.Insert("param", p_js_param);
			}
		}
	}
	{
		ProcessorTbl::Rec prc_rec;
		for(SEnum en = tses_obj.PrcObj.P_Tbl->Enum(PPPRCK_PROCESSOR, 0); en.Next(&prc_rec) > 0;) {
			prc_id_list.add(prc_rec.ID);
		}
	}
	if(prc_id_list.getCount()) {
		prc_id_list.sortAndUndup();
		PrcBusyArray busy_list;
		SJson * p_js_prc_list = new SJson(SJson::tARRAY);
		for(uint prcidx = 0; prcidx < prc_id_list.getCount(); prcidx++) {
			const PPID prc_id = prc_id_list.get(prcidx);
			PPProcessorPacket prc_pack;
			if(tses_obj.PrcObj.GetPacket(prc_id, &prc_pack) > 0) {
				goods_id_list.Z();
				tses_obj.TecObj.GetGoodsListByPrc(prc_id, &goods_id_list);
				if(goods_id_list.getCount()) {
					goods_id_list.sortAndUndup();
					SJson * p_js_prc = new SJson(SJson::tOBJECT);
					p_js_prc->InsertInt("id", prc_pack.Rec.ID);
					p_js_prc->InsertString("nm", (temp_buf = prc_pack.Rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
					// @todo Внести подробную информацию о связанном объекте
					if(prc_pack.Rec.LinkObjType == PPOBJ_PERSON) {
						PersonTbl::Rec psn_rec;
						if(psn_obj.Fetch(prc_pack.Rec.LinkObjID, &psn_rec) > 0) {
							const PPObjID oid(PPOBJ_PERSON, psn_rec.ID);
							if(GetBlobInfo(bc_own_ident, oid, 0, blob_info, 0)) {
								assert(blob_info.Signature.Len() && blob_info.Signature.IsAscii());
								p_js_prc->InsertString("imgblobs", blob_info.Signature);
								stat.BlobOidList.Add(oid.Obj, oid.Id);
							}
						}
					}
					{
						SJson * p_js_prcgoodslist = new SJson(SJson::tARRAY);
						for(uint gidx = 0; gidx < goods_id_list.getCount(); gidx++) {
							const PPID goods_id = goods_id_list.get(gidx);
							SJson * p_js_goods_item = new SJson(SJson::tOBJECT);
							p_js_goods_item->InsertInt("id", goods_id);
							p_js_prcgoodslist->InsertChild(p_js_goods_item);
							goods_to_prc_list.Add(goods_id, prc_id);
						}
						p_js_prc->Insert("goods_list", p_js_prcgoodslist);
					}
					{
						SJson * p_js_busylist = new SJson(SJson::tARRAY);
						uint   busylist_count = 0;
						busy_list.clear();
						STimeChunk observe_time_chunk(getcurdatetime_(), LDATETIME().SetFar());
						tses_obj.P_Tbl->LoadBusyArray(prc_id, 0, TSESK_SESSION, &observe_time_chunk, &busy_list);
						for(uint blidx = 0; blidx < busy_list.getCount(); blidx++) {
							const STimeChunk * p_busy_item = static_cast<const STimeChunk *>(busy_list.at(blidx));
							if(checkdate(p_busy_item->Start.d) && checkdate(p_busy_item->Finish.d)) {
								SJson * p_js_busy_entry = new SJson(SJson::tOBJECT);
								p_js_busy_entry->InsertString("low", temp_buf.Z().Cat(p_busy_item->Start, DATF_ISO8601|DATF_CENTURY, 0));
								p_js_busy_entry->InsertString("upp", temp_buf.Z().Cat(p_busy_item->Finish, DATF_ISO8601|DATF_CENTURY, 0));
								p_js_busylist->InsertChild(p_js_busy_entry);
								busylist_count++;
							}
						}
						if(busylist_count) {
							p_js_prc->Insert("busy_list", p_js_busylist);
						}
						else
							ZDELETE(p_js_busylist);
					}
					p_js_prc_list->InsertChild(p_js_prc);
				}
			}
		}
		js.Insert("processor_list", p_js_prc_list);
		{
			PPIDArray goodsgrp_id_list;
			PPIDArray uom_id_list;
			Goods2Tbl::Rec goodsgroup_rec;
			PPUnit u_rec;
			goods_to_prc_list.SortByKeyVal();
			goods_to_prc_list.GetKeyList(goods_id_list);
			{
				Goods2Tbl::Rec goods_rec;
				for(uint gidx = 0; gidx < goods_id_list.getCount(); gidx++) {
					const PPID goods_id = goods_id_list.get(gidx);
					if(tses_obj.GObj.Search(goods_id, &goods_rec) > 0) {
						if(goods_rec.ParentID > 0 && tses_obj.GObj.Fetch(goods_rec.ParentID, &goodsgroup_rec) > 0 && goodsgroup_rec.Kind == PPGDSK_GROUP) {
							goodsgrp_id_list.add(goods_rec.ParentID);
						}
						if(goods_rec.UnitID > 0 && unit_obj.Fetch(goods_rec.UnitID, &u_rec) > 0) {
							uom_id_list.add(goods_rec.UnitID);
						}
					}
				}
				goodsgrp_id_list.sortAndUndup();
				uom_id_list.sortAndUndup();
			}
			if(goodsgrp_id_list.getCount()) {
				SJson * p_js_goodsgrplist = new SJson(SJson::tARRAY);
				for(uint ggidx = 0; ggidx < goodsgrp_id_list.getCount(); ggidx++) {
					int r = tses_obj.GObj.Fetch(goodsgrp_id_list.get(ggidx), &goodsgroup_rec);
					assert(r > 0);
					SJson * p_js_goodsgrp = new SJson(SJson::tOBJECT);
					p_js_goodsgrp->InsertInt("id", goodsgroup_rec.ID);
					p_js_goodsgrp->InsertString("nm", (temp_buf = goodsgroup_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
					p_js_goodsgrplist->InsertChild(p_js_goodsgrp);
				}
				js.Insert("goodsgroup_list", p_js_goodsgrplist);
			}
			if(uom_id_list.getCount()) {
				SJson * p_js_uomlist = new SJson(SJson::tARRAY);
				for(uint uidx = 0; uidx < uom_id_list.getCount(); uidx++) {
					int r = unit_obj.Fetch(uom_id_list.get(uidx), &u_rec);
					assert(r > 0);
					SJson * p_js_uom = new SJson(SJson::tOBJECT);
					p_js_uom->InsertInt("id", u_rec.ID);
					if(u_rec.Fragmentation > 0) {
						p_js_uom->InsertInt("fragmentation", u_rec.Fragmentation);
					}
					else if(u_rec.Rounding_ > 0.0) {
						p_js_uom->InsertDouble("rounding", u_rec.Rounding_, MKSFMTD(0, 6, NMBF_NOTRAILZ));
					}
					else if(u_rec.Flags & PPUnit2::IntVal) {
						p_js_uom->InsertBool("int", true);
					}
					p_js_uom->InsertString("nm", (temp_buf = u_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
					p_js_uomlist->InsertChild(p_js_uom);
				}
				js.Insert("uom_list", p_js_uomlist);
			}
			{
				SJson * p_js_goodslist = new SJson(SJson::tARRAY);
				PPGoodsPacket goods_pack;
				for(uint gidx = 0; gidx < goods_id_list.getCount(); gidx++) {
					const PPID goods_id = goods_id_list.get(gidx);
					if(tses_obj.GObj.GetPacket(goods_id, &goods_pack, 0) > 0) {
						SJson * p_js_ware = new SJson(SJson::tOBJECT);
						p_js_ware->InsertInt("id", goods_pack.Rec.ID);
						p_js_ware->InsertString("nm", (temp_buf = goods_pack.Rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
						if(goodsgrp_id_list.bsearch(goods_pack.Rec.ParentID))
							p_js_ware->InsertInt("parid", goods_pack.Rec.ParentID);
						if(goods_pack.Rec.UnitID && uom_id_list.bsearch(goods_pack.Rec.UnitID))
							p_js_ware->InsertInt("uomid", goods_pack.Rec.UnitID);
						{
							const PPObjID oid(PPOBJ_GOODS, goods_id);
							if(GetBlobInfo(bc_own_ident, oid, 0, blob_info, 0)) {
								assert(blob_info.Signature.Len() && blob_info.Signature.IsAscii());
								p_js_ware->InsertString("imgblobs", blob_info.Signature);
								stat.BlobOidList.Add(oid.Obj, oid.Id);
							}
						}
						if(goods_pack.Codes.getCount()) {
							SJson * p_js_bcarray = 0;
							for(uint bcidx = 0; bcidx < goods_pack.Codes.getCount(); bcidx++) {
								const BarcodeTbl::Rec & r_bc_rec = goods_pack.Codes.at(bcidx);
								int   diag = 0;
								int   std = 0;
								if(PPObjGoods::DiagBarcode(r_bc_rec.Code, &diag, &std, &temp_buf) > 0) {
									SJson * p_bc_item = new SJson(SJson::tOBJECT);
									p_bc_item->InsertString("cod", temp_buf.Transf(CTRANSF_INNER_TO_UTF8).Escape());
									if(r_bc_rec.Qtty > 0.0) {
										p_bc_item->InsertDouble("qty", r_bc_rec.Qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ));
									}
									SETIFZ(p_js_bcarray, new SJson(SJson::tARRAY));
									p_js_bcarray->InsertChild(p_bc_item);
								}
							}
							if(p_js_bcarray)
								p_js_ware->Insert("code_list", p_js_bcarray);
						}
						p_js_goodslist->InsertChild(p_js_ware);
					}
				}
				js.Insert("goods_list", p_js_goodslist);
			}
		}
	}
	THROW(js.ToStr(rResult));
	// @debug {
	{
		SString out_file_name;
		PPGetFilePath(PPPATH_OUT, "stq-rsrvattendanceprereq.json", out_file_name);
		SFile f_out(out_file_name, SFile::mWrite);
		f_out.WriteLine(rResult);
	}
	// } @debug
	{
		SJson js_decl(SJson::tOBJECT);
		js_decl.InsertString("type", "attendanceprereq");
		js_decl.InsertString("format", "json");
		js_decl.InsertString("time", temp_buf.Z().CatCurDateTime(DATF_ISO8601|DATF_CENTURY, 0));
		if(rCmdItem.ResultExpiryTimeSec > 0)
			js_decl.InsertInt("resultexpirytimesec", rCmdItem.ResultExpiryTimeSec);
		js_decl.InsertString("displaymethod", "attendanceprereq");
		THROW_SL(js_decl.ToStr(rDocDeclaration));
	}
	StoreOidListWithBlob(stat.BlobOidList.SortAndUndup());
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::ProcessCommand_RsrvOrderPrereq(const StyloQCommandList::Item & rCmdItem, const StyloQCore::StoragePacket & rCliPack, const SGeoPosLL & rGeoPos, 
	SString & rResult, SString & rDocDeclaration)
{
	//StoreOidListWithBlob
	int    ok = 1;
	PPObjIDArray bloboid_list;
	Stq_CmdStat_MakeRsrv_Response stat;
	PPUserFuncProfiler ufp(PPUPRF_STYLOQ_CMD_ORDERPREREQ);
	PPID   stylopalm_id = 0;
	SString temp_buf;
	PPObjStyloPalm stp_obj;
	PPStyloPalmPacket stp_pack;
	StyloQCore::StoragePacket own_pack;
	SBinaryChunk bc_own_ident;
	THROW(GetOwnPeerEntry(&own_pack) > 0);
	THROW_PP(own_pack.Pool.Get(SSecretTagPool::tagSvcIdent, &bc_own_ident), PPERR_SQ_UNDEFOWNSVCID);
	THROW(rCmdItem.Param.ReadStatic(&stylopalm_id, sizeof(stylopalm_id))); // @todo err
	if(stylopalm_id == ROBJID_CONTEXT) {
		PPID   local_person_id = 0;
		THROW(FetchPersonFromClientPacket(rCliPack, &local_person_id) > 0);
		if(local_person_id) {
			PPIDArray stp_id_list;
			if(stp_obj.GetListByPerson(local_person_id, stp_id_list) > 0) {
				stylopalm_id = stp_id_list.get(0);
			}
		}
	}
	THROW(stp_obj.GetPacket(stylopalm_id, &stp_pack) > 0);
	{
		SJson js(SJson::tOBJECT);
		THROW(MakeRsrvPriceListResponse_ExportGoods(bc_own_ident, &stp_pack, &js, &stat));
		if(rCmdItem.ObjTypeRestriction == PPOBJ_PERSON && rCmdItem.ObjGroupRestriction == PPPRK_AGENT) {
			THROW(MakeRsrvPriceListResponse_ExportClients(bc_own_ident, &stp_pack, &js, &stat));
		}
		THROW(js.ToStr(rResult));
		// @debug {
		{
			SString out_file_name;
			PPGetFilePath(PPPATH_OUT, "stq-rsrvorderprereq.json", out_file_name);
			SFile f_out(out_file_name, SFile::mWrite);
			f_out.WriteLine(rResult);
		}
		// } @debug
		{
			SJson js_decl(SJson::tOBJECT);
			js_decl.InsertString("type", "orderprereq");
			js_decl.InsertString("format", "json");
			js_decl.InsertString("time", temp_buf.Z().CatCurDateTime(DATF_ISO8601|DATF_CENTURY, 0));
			if(rCmdItem.ResultExpiryTimeSec > 0)
				js_decl.InsertInt("resultexpirytimesec", rCmdItem.ResultExpiryTimeSec);
			js_decl.InsertString("displaymethod", "orderprereq");
			THROW_SL(js_decl.ToStr(rDocDeclaration));
		}
	}
	StoreOidListWithBlob(stat.BlobOidList.SortAndUndup());
	ufp.SetFactor(0, static_cast<double>(stat.GoodsCount));
	ufp.SetFactor(1, static_cast<double>(stat.ClientCount));
	ufp.Commit();
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::ProcessCommand_Report(StyloQCommandList::Item & rCmdItem, const StyloQCore::StoragePacket & rCliPack, const SGeoPosLL & rGeoPos, 
	SString & rResult, SString & rDocDeclaration)
{
	int    ok = 1;
	SString temp_buf;
	SJson * p_js = 0;
	PPView * p_view = 0;
	PPBaseFilt * p_filt = 0;
	DlRtm  * p_rtm = 0;
	assert(rCmdItem.BaseCmdId == StyloQCommandList::sqbcReport);
	{
		// @debug {
			const PPThreadLocalArea & r_tla = DS.GetConstTLA();
			assert((&r_tla) != 0);
		// } @debug
		rResult.Z();
		SString dl600_name(rCmdItem.Vd.GetStrucSymb());
		uint p = 0;
		LoadViewSymbList();
		long   view_id = ViewSymbList.SearchByText(rCmdItem.ViewSymb, 1, &p) ? ViewSymbList.at_WithoutParent(p).Id : 0;
		THROW_PP_S(view_id, PPERR_NAMEDFILTUNDEFVIEWID, rCmdItem.ViewSymb);
		THROW(PPView::CreateInstance(view_id, &p_view));
		{
			if(rCmdItem.Param.GetAvailableSize()) {
				THROW(PPView::ReadFiltPtr(rCmdItem.Param, &p_filt));
			}
			else {
				THROW(p_filt = p_view->CreateFilt(0));
			}
			THROW(p_view->Init_(p_filt));
			if(!dl600_name.NotEmptyS()) {
				if(p_view->GetDefReportId()) {
					SReport rpt(p_view->GetDefReportId(), 0);
					THROW(rpt.IsValid());
					dl600_name = rpt.getDataName();
				}
			}
			{
				DlContext ctx;
				PPFilt f(p_view);
				DlRtm::ExportParam ep;
				THROW(ctx.InitSpecial(DlContext::ispcExpData));
				THROW(ctx.CreateDlRtmInstance(dl600_name, &p_rtm));
				ep.P_F = &f;
				ep.Sort = 0;
				ep.Flags |= (DlRtm::ExportParam::fIsView|DlRtm::ExportParam::fInheritedTblNames);
				ep.Flags &= ~DlRtm::ExportParam::fDiff_ID_ByScope;
				SETFLAG(ep.Flags, DlRtm::ExportParam::fCompressXml, /*pNf->Flags & PPNamedFilt::fCompressXml*/0);
				ep.Flags |= (DlRtm::ExportParam::fDontWriteXmlDTD|DlRtm::ExportParam::fDontWriteXmlTypes);
				/*
				if(pNf->VD.GetCount() > 0)
					ep.P_ViewDef = &pNf->VD;
				*/
				if(rCmdItem.Vd.GetCount()) {
					ep.P_ViewDef = &rCmdItem.Vd;
				}
				ep.Cp = cpUTF8;
				// format == SFileFormat::Json)
				{
					ep.Flags |= DlRtm::ExportParam::fJsonStQStyle; 
					THROW(p_js = p_rtm->ExportJson(ep));
					THROW_SL(p_js->ToStr(rResult));
				}
				{
					SJson js_decl(SJson::tOBJECT);
					js_decl.InsertString("type", "view");
					js_decl.InsertString("viewsymb", rCmdItem.ViewSymb);
					js_decl.InsertString("dl600symb", dl600_name);
					js_decl.InsertString("format", "json");
					js_decl.InsertString("time", temp_buf.Z().CatCurDateTime(DATF_ISO8601|DATF_CENTURY, 0));
					if(rCmdItem.ResultExpiryTimeSec > 0)
						js_decl.InsertInt("resultexpirytimesec", rCmdItem.ResultExpiryTimeSec);
					js_decl.InsertString("displaymethod", "grid");
					THROW_SL(js_decl.ToStr(rDocDeclaration));
				}
			}
		}
	}
	CATCHZOK
	delete p_js;
	delete p_rtm;
	delete p_filt;
	delete p_view;
	return ok;
}

void PPStyloQInterchange::Debug_Command(const StyloQCommandList::Item * pCmd) // @debug
{
	if(pCmd) {
		if(pCmd->BaseCmdId == StyloQCommandList::sqbcRsrvOrderPrereq) {
			SString result;
			SString decl;
			StyloQCore::StoragePacket fake_cli_pack;
			SGeoPosLL fake_geo_pos;
			ProcessCommand_RsrvOrderPrereq(*pCmd, fake_cli_pack, fake_geo_pos, result, decl);
		}
		else if(pCmd->BaseCmdId == StyloQCommandList::sqbcRsrvAttendancePrereq) {
			SString result;
			SString decl;
			StyloQCore::StoragePacket fake_cli_pack;
			SGeoPosLL fake_geo_pos;
			ProcessCommand_RsrvAttendancePrereq(*pCmd, fake_cli_pack, fake_geo_pos, result, decl);
		}
		else if(pCmd->BaseCmdId == StyloQCommandList::sqbcRsrvPushIndexContent) {
			StyloQCore::StoragePacket own_pack;
			SString result;
			if(GetOwnPeerEntry(&own_pack) > 0) {
				PPObjIDArray oid_list;
				MakeIndexingRequestCommand(&own_pack, pCmd, 3600*24, oid_list, result);
			}
		}
		else if(pCmd->BaseCmdId == StyloQCommandList::sqbcReport) {
			;
		}
	}
}

int PPStyloQInterchange::FetchPersonFromClientPacket(const StyloQCore::StoragePacket & rCliPack, PPID * pPersonID)
{
	int    ok = -1;
	PPID   person_id = 0;
	PersonTbl::Rec psn_rec;
	if(rCliPack.Rec.LinkObjType == PPOBJ_PERSON) {
		person_id = rCliPack.Rec.LinkObjID;
	}
	else if(rCliPack.Rec.LinkObjType == PPOBJ_USR) {
		PPSecur sec_rec;
		PPObjSecur sec_obj(PPOBJ_USR, 0);
		if(sec_obj.Search(rCliPack.Rec.LinkObjID, &sec_rec) > 0 && sec_rec.PersonID)
			person_id = sec_rec.PersonID;
	}
	ASSIGN_PTR(pPersonID, person_id);
	if(person_id)
		ok = 1;
	return ok;
}

int PPStyloQInterchange::AcceptStyloQClientAsPerson(const StyloQCore::StoragePacket & rCliPack, PPID personKindID, PPID * pPersonID, int use_ta)
{
	int    ok = -1;
	PPID   person_id = 0;
	if(personKindID) {
		PPObjPerson psn_obj;
		if(FetchPersonFromClientPacket(rCliPack, &person_id) > 0) {
			if(psn_obj.P_Tbl->IsBelongToKind(person_id, personKindID) > 0) {
				ok = 1;
			}
			else {
				THROW(psn_obj.P_Tbl->AddKind(person_id, personKindID, use_ta));
				ok = 1;
			}
		}
		else {
			bool face_is_valid = false;
			SString temp_buf;
			StyloQFace cli_face;
			SBinaryChunk face_bytes;
			if(rCliPack.Pool.Get(SSecretTagPool::tagFace, &face_bytes)) {
				if(face_bytes.Len()) {
					if(cli_face.FromJson(face_bytes.ToRawStr(temp_buf)))
						face_is_valid = true;
				}
			}
			{
				PPPersonPacket psn_pack;
				RegisterTbl::Rec new_reg_rec;
				if(face_is_valid) {
					if(cli_face.Get(StyloQFace::tagCommonName, 0/*lang*/, temp_buf)) {
						STRNSCPY(psn_pack.Rec.Name, temp_buf);
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
							STRNSCPY(psn_pack.Rec.Name, compound_name);
					}
					if(cli_face.Get(StyloQFace::tagPhone, 0/*lang*/, temp_buf))
						psn_pack.ELA.AddItem(PPELK_MOBILE, temp_buf);
					if(cli_face.Get(StyloQFace::tagEMail, 0/*lang*/, temp_buf))
						psn_pack.ELA.AddItem(PPELK_EMAIL, temp_buf);
					if(cli_face.Get(StyloQFace::tagRuINN, 0/*lang*/, temp_buf)) {
						PPObjRegister::InitPacket(&new_reg_rec, PPREGT_TPID, PPObjID(PPOBJ_PERSON, 0), temp_buf);
						psn_pack.Regs.insert(&new_reg_rec);
					}
					if(cli_face.Get(StyloQFace::tagRuKPP, 0/*lang*/, temp_buf)) {
						PPObjRegister::InitPacket(&new_reg_rec, PPREGT_KPP, PPObjID(PPOBJ_PERSON, 0), temp_buf);
						psn_pack.Regs.insert(&new_reg_rec);
					}
					if(cli_face.Get(StyloQFace::tagGLN, 0/*lang*/, temp_buf)) {
						PPObjRegister::InitPacket(&new_reg_rec, PPREGT_GLN, PPObjID(PPOBJ_PERSON, 0), temp_buf);
						psn_pack.Regs.insert(&new_reg_rec);
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
							//STRNSCPY(addr_pack.Rec.
						}
					}
				}
				if(isempty(psn_pack.Rec.Name)) {
					P_T->MakeTextHashForCounterparty(rCliPack, 10, temp_buf);
					STRNSCPY(psn_pack.Rec.Name, temp_buf);					
				}
				psn_pack.Kinds.addUnique(personKindID);
				assert(person_id == 0);
				person_id = 0;
				THROW(psn_obj.PutPacket(&person_id, &psn_pack, use_ta));
				ok = 1;
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pPersonID, person_id);
	return ok;
}

PPID PPStyloQInterchange::ProcessCommand_IndexingContent(const StyloQCore::StoragePacket & rCliPack, const SJson * pDocument, SString & rResult)
{
	rResult.Z();
	PPID   doc_id = 0;
	SString js_doc_text;
	SBinaryChunk doc_ident;
	SBinaryChunk cli_ident;
	SSecretTagPool doc_pool;
	THROW(pDocument != 0); // @todo @err
	THROW(rCliPack.Pool.Get(SSecretTagPool::tagClientIdent, &cli_ident)); // @todo @err
	THROW(P_T->MakeDocumentStorageIdent(cli_ident, ZEROGUID, doc_ident));
	{
		SBinarySet::DeflateStrategy ds(256);
		THROW_SL(pDocument->ToStr(js_doc_text));
		THROW_SL(doc_pool.Put(SSecretTagPool::tagRawData, js_doc_text, js_doc_text.Len(), &ds));
	}
	THROW(P_T->PutDocument(&doc_id, cli_ident, -1/*input*/, StyloQCore::doctypIndexingContent, doc_ident, doc_pool, 1/*use_ta*/));
	rResult.CatEq("stored-document-id", doc_id).CatDiv(';', 2).CatEq("raw-json-size", js_doc_text.Len()+1).CatDiv(';', 2).CatEq("result-pool-size", doc_pool.GetDataLen());
	CATCH
		doc_id = 0;
	ENDCATCH
	return doc_id;
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

SJson * PPStyloQInterchange::ProcessCommand_GetBlob(const StyloQCore::StoragePacket & rCliPack, const SJson * pDocument, SBinaryChunk & rBlob)
{
	rBlob.Z();
	int    ok = 1;
	SJson * p_js_result = new SJson(SJson::tOBJECT);
	SString blob_path;
	SString _signature;
	for(const SJson * p_cur = pDocument; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type == SJson::tOBJECT) {
			for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("cmd")) {
					THROW(p_obj->P_Child->Text.IsEqiAscii("getblob")); // @err
				}
				else if(p_obj->Text.IsEqiAscii("signature")) {
					_signature = p_obj->P_Child->Text.Unescape();
				}
			}
		}
	}
	THROW(_signature.NotEmpty()); // @err
	THROW(PPStyloQInterchange::GetBlobStoragePath(blob_path));
	{
		assert(blob_path.NotEmpty());
		//SBuffer content_buf; // Бинарное представление BLOB'а
		const size_t nominal_readbuf_size = SMEGABYTE(1);
		STempBuffer rb(nominal_readbuf_size+64/*insurance*/); // Временный буфер для чтения файла порциями
		THROW_SL(rb.IsValid());
		{
			{
				SFileStorage sfs(blob_path);
				int64   fs = 0; // file-size
				SHandle rh; // read-handle
				size_t  actual_size = 0;
				THROW_SL(sfs.IsValid());
				rh = sfs.GetFile(_signature, &fs);
				THROW_SL(rh);
				do {
					THROW_SL(sfs.Read(rh, rb, nominal_readbuf_size, &actual_size));
					THROW_SL(rBlob.Cat(rb, actual_size));
					//THROW_SL(content_buf.Write(rb, actual_size));
				} while(actual_size);
				{
					uint64 rd_bytes = sfs.GetTotalRdSize(rh);
					THROW_SL(rd_bytes);
					//assert(content_buf.GetAvailableSize() == rd_bytes);
					assert(rBlob.Len() == rd_bytes);
				}
			}
			{
				//const size_t preserve_rd_offs = content_buf.GetRdOffs();
				//const size_t _content_size = content_buf.GetAvailableSize();
				//binary256 _hash = SlHash::Sha256(0, content_buf.GetBufC(content_buf.GetRdOffs()), _content_size);
				binary256 _hash = SlHash::Sha256(0, rBlob.PtrC(), rBlob.Len());
				//assert(preserve_rd_offs == content_buf.GetRdOffs());
				{
					SString temp_buf;
					SFileFormat ff;
					const int fir = ff.IdentifyBuffer(rBlob.PtrC(), rBlob.Len());
					if(fir == 2) {
						SFileFormat::GetMime(ff, temp_buf);
						if(temp_buf.NotEmpty())
							p_js_result->InsertString("contenttype", temp_buf);
					}
					p_js_result->InsertInt64("contentsize", static_cast<int64>(rBlob.Len()));
					SlHash::GetAlgorithmSymb(SHASHF_SHA256, temp_buf);
					assert(temp_buf.NotEmpty());
					p_js_result->InsertString("hashalg", temp_buf.Escape());
					temp_buf.Z().EncodeMime64(&_hash, sizeof(_hash));
					p_js_result->InsertString("hash", temp_buf.Escape());
					//rBlob.Put(content_buf.GetBufC(content_buf.GetRdOffs()), _content_size); // @v11.3.8 
					// @v11.3.8 THROW_SL(temp_buf.EncodeMime64(content_buf.GetBufC(content_buf.GetRdOffs()), _content_size));
					// @v11.3.8 p_js_result->InsertString("content", temp_buf.Escape());
				}
			}
		}
	}
	// Все временные буферы разрушены - теперь сформируем результат
	//THROW_SL(js_reply.ToStr(rResult));
	CATCH
		ZDELETE(p_js_result);
	ENDCATCH
	return p_js_result;
}

PPStyloQInterchange::Stq_ReqBlobInfoEntry::Stq_ReqBlobInfoEntry() : BlobInfo(), Missing(false), RepDiffHash(false)
{
	memzero(Reserve, sizeof(Reserve));
}

PPStyloQInterchange::Stq_ReqBlobInfoEntry & PPStyloQInterchange::Stq_ReqBlobInfoEntry::Z()
{
	BlobInfo::Z();
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
		p_result = new SJson(SJson::tOBJECT);
		p_result->InsertString("cmd", "requestblobinfolist");
		SJson * p_js_list = new SJson(SJson::tARRAY);
		for(uint i = 0; i < getCount(); i++) {
			const Stq_ReqBlobInfoEntry * p_req_blob_entry = at(i);
			assert(p_req_blob_entry);
			if(p_req_blob_entry) {
				SJson * p_js_blob = new SJson(SJson::tOBJECT);
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

bool PPStyloQInterchange::GetBlobInfo(const SBinaryChunk & rOwnIdent, PPObjID oid, uint blobN, BlobInfo & rInfo, SBinaryChunk * pBlobBuf) const
{
	rInfo.Z();
	int    ok = false;
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
					if(f_in.IsValid() && f_in.CalcHash(0, rInfo.HashAlg, rInfo.Hash)) {
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
	return ok;
}

int PPStyloQInterchange::AddImgBlobToReqBlobInfoList(const SBinaryChunk & rOwnIdent, PPObjID oid, Stq_ReqBlobInfoList & rList)
{
	int    ok = -1;
	BlobInfo bi;
	if(GetBlobInfo(rOwnIdent, oid, 0, bi, 0)) {
		Stq_ReqBlobInfoEntry * p_new_entry = rList.CreateNewItem();
		*static_cast<BlobInfo *>(p_new_entry) = bi;
		ok = 1;
	}
	return ok;
}

SJson * PPStyloQInterchange::ProcessCommand_ReqBlobInfoList(const StyloQCore::StoragePacket & rCliPack, const SJson * pDocument)
{
	int    ok = 1;
	SJson * p_js_result = 0;
	SString temp_buf;
	Stq_ReqBlobInfoList req_list;
	for(const SJson * p_cur = pDocument; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type == SJson::tOBJECT) {
			for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("cmd")) {
					THROW(p_obj->P_Child->Text.IsEqiAscii("requestblobinfolist")); // @err
				}
				else if(p_obj->Text.IsEqiAscii("list")) {
					if(p_obj->P_Child && p_obj->P_Child->IsArray()) {
						for(const SJson * p_inr = p_obj->P_Child->P_Child; p_inr; p_inr = p_inr->P_Next) {
							if(p_inr->IsObject()) {
								Stq_ReqBlobInfoEntry * p_entry = req_list.CreateNewItem();
								for(const SJson * p_itm = p_inr->P_Child; p_itm; p_itm = p_itm->P_Next) {
									if(p_itm->Text.IsEqiAscii("signature")) {
										if(p_itm->P_Child)
											(p_entry->Signature = p_itm->P_Child->Text).Unescape();
									}
									else if(p_itm->Text.IsEqiAscii("hashalg")) {
										if(p_itm->P_Child) {
											p_entry->HashAlg = SlHash::IdentifyAlgorithmSymb(p_itm->P_Child->Text.Unescape());
										}
									}
									else if(p_itm->Text.IsEqiAscii("hash")) {
										if(p_itm->P_Child) {
											p_entry->Hash.FromMime64(p_itm->P_Child->Text.Unescape());
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
	{
		p_js_result = new SJson(SJson::tARRAY);
		if(req_list.getCount()) {
			SString blob_path;
			THROW(PPStyloQInterchange::GetBlobStoragePath(blob_path));
			{
				SBinaryChunk bc_my_hash;
				SFileStorage sfs(blob_path);
				SBuffer content_buf; // Бинарное представление BLOB'а
				const size_t nominal_readbuf_size = SMEGABYTE(1);
				STempBuffer rb(nominal_readbuf_size+64/*insurance*/); // Временный буфер для чтения файла порциями
				THROW_SL(sfs.IsValid());
				THROW_SL(rb.IsValid());
				for(uint i = 0; i < req_list.getCount(); i++) {
					const Stq_ReqBlobInfoEntry * p_entry = req_list.at(i);
					assert(p_entry);
					if(p_entry) {
						if(p_entry->Signature.NotEmpty()) {
							int64   fs = 0; // file-size
							size_t  actual_size = 0;
							content_buf.Z();
							SHandle rh = sfs.GetFile(p_entry->Signature, &fs); // read-handle
							if(rh) {
								do {
									THROW_SL(sfs.Read(rh, rb, nominal_readbuf_size, &actual_size));
									THROW_SL(content_buf.Write(rb, actual_size));
								} while(actual_size);
								{
									uint64 rd_bytes = sfs.GetTotalRdSize(rh);
									THROW_SL(rd_bytes);
									assert(content_buf.GetAvailableSize() == rd_bytes);
									THROW_SL(SlHash::CalcBufferHash(p_entry->HashAlg, content_buf.GetBufC(content_buf.GetRdOffs()), content_buf.GetAvailableSize(), bc_my_hash));
									if(bc_my_hash == p_entry->Hash) {
										//	
									}
									else {
										SJson * p_js_item = new SJson(SJson::tOBJECT);
										p_js_item->InsertString("signature", (temp_buf = p_entry->Signature).Escape());
										SlHash::GetAlgorithmSymb(p_entry->HashAlg, temp_buf);
										assert(temp_buf.NotEmpty());
										p_js_item->InsertString("hashalg", temp_buf.Escape());
										p_js_item->InsertString("hash", bc_my_hash.Mime64(temp_buf).Escape());
										p_js_result->InsertChild(p_js_item);
									}
								}
								sfs.CloseFile(rh);
							}
							else {
								SJson * p_js_item = new SJson(SJson::tOBJECT);
								p_js_item->InsertString("signature", (temp_buf = p_entry->Signature).Escape());
								p_js_item->InsertBool("missing", true);
								p_js_result->InsertChild(p_js_item);							
							}
						}
					}
				}
			}
		}
	}
	CATCH
		ZDELETE(p_js_result);
	ENDCATCH
	return p_js_result;
}

int PPStyloQInterchange::ProcessCommand_StoreBlob(const StyloQCore::StoragePacket & rCliPack, const SJson * pDocument, const SBinaryChunk & rBlob, SString & rResult)
{
	//SFileStorage
	// storeblob
	/*
		{
			cmd: storeblob
			time: epoch-secondes
			contenttype: mime-declaration
			contentsize: size-of-content (in raw format - not in encoded version)
			hashalg: hashing-algorithm (sha-256 or other)
			hash: hash-value (base64 encoded)
			signature: text-signature-for-storing-data
			content: content-data (base64 encoded)
		}
	*/
	rResult.Z();
	int    ok = 1;
	long   _time = 0;
	SString blob_path;
	SFileFormat _content_type;
	int64  _content_size = 0;
	int    _hash_alg = 0;
	//SBinaryChunk blob_data;
	SBinaryChunk _hash;
	SString _signature;
	THROW(rBlob.Len()); // @err
	//const char * p_content_b64 = 0;
	for(const SJson * p_cur = pDocument; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type == SJson::tOBJECT) {
			for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("cmd")) {
					THROW(p_obj->P_Child->Text.IsEqiAscii("storeblob")); // @err
				}
				else if(p_obj->Text.IsEqiAscii("time")) {
					_time = p_obj->P_Child->Text.ToLong();
				}
				else if(p_obj->Text.IsEqiAscii("contenttype")) {
					_content_type.IdentifyMime(p_obj->P_Child->Text);
				}
				else if(p_obj->Text.IsEqiAscii("contentsize")) {
					_content_size = p_obj->P_Child->Text.ToInt64();
				}
				else if(p_obj->Text.IsEqiAscii("hashalg")) {
					_hash_alg = SlHash::IdentifyAlgorithmSymb(p_obj->P_Child->Text);
				}
				else if(p_obj->Text.IsEqiAscii("hash")) {
					THROW_SL(_hash.FromMime64(p_obj->P_Child->Text.Unescape()));
				}
				else if(p_obj->Text.IsEqiAscii("signature")) {
					_signature = p_obj->P_Child->Text.Unescape();
				}
				/*else if(p_obj->Text.IsEqiAscii("content")) {
					p_content_b64 = p_obj->P_Child->Text.Unescape();
				}*/
			}
		}
	}
	//THROW(p_content_b64); // @err
	THROW(_signature.NotEmpty()); // @err
	THROW(_hash_alg); // @err
	THROW(_hash.Len()); // @err
	//THROW_SL(blob_data.FromMime64(p_content_b64));
	THROW_SL(SlHash::VerifyBuffer(_hash_alg, _hash.PtrC(), _hash.Len(), rBlob.PtrC(), rBlob.Len()));
	THROW(PPStyloQInterchange::GetBlobStoragePath(blob_path));
	{
		assert(blob_path.NotEmpty());
		SFileStorage sfs(blob_path);
		THROW_SL(sfs.IsValid());
		THROW_SL(sfs.PutFile(_signature, rBlob.PtrC(), rBlob.Len()));
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::ProcessCommand_Search(const StyloQCore::StoragePacket & rCliPack, const SJson * pDocument, SString & rResult, SString & rDocDeclaration)
{
	rResult.Z();
	int    ok = -1;
	uint   max_result_count = 0;
	SString temp_buf;
	SString plain_query;
	for(const SJson * p_cur = pDocument; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type == SJson::tOBJECT) {								
			for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("cmd")) {
					;
				}
				else if(p_obj->Text.IsEqiAscii("plainquery")) {
					if(SJson::IsString(p_obj->P_Child)) {
						(plain_query = p_obj->P_Child->Text).Unescape();
					}
				}
				else if(p_obj->Text.IsEqiAscii("maxresultcount")) {
					if(SJson::IsNumber(p_obj->P_Child))
						max_result_count = p_obj->P_Child->Text.ToULong();
				}
			}
		}
	}
	if(plain_query.NotEmptyS()) {
		PPFtsIterface fts_db(false/*forUpdate*/, 30000);
		TSCollection <PPFtsIterface::SearchResultEntry> result_list;
		THROW(fts_db);
		THROW(fts_db.Search(plain_query, max_result_count, result_list));
		/*if(result_list.getCount())*/ {
			/* json
				searchresult
				query
				scope_list
					{
						scope
						scopeident
						nm
					}
				result_list
					{
						scope
						scopeident
						objtype
						objid
						rank
						weight
						text
					}
			*/
			SJson js_reply(SJson::tOBJECT);
			js_reply.InsertString("query", (temp_buf = plain_query).Escape());
			{
				SJson * p_js_list = new SJson(SJson::tARRAY);
				SBinaryChunk bc_temp;
				PPFtsIterface::SurrogateScopeList sslist;
				for(uint i = 0; i < result_list.getCount(); i++) {
					const PPFtsIterface::SearchResultEntry * p_ri = result_list.at(i);
					if(p_ri) {
						p_ri->MakeSurrogateScopeIdent(bc_temp);
						if(!sslist.Search(bc_temp, 0)) {
							SJson * p_js_si = new SJson(SJson::tOBJECT);
							temp_buf.Z();
							switch(p_ri->Scope) {
								case PPFtsIterface::scopePPDb: temp_buf = "ppdb"; break;
								case PPFtsIterface::scopeStyloQSvc: temp_buf = "styloqsvc"; break;
								//case PPFtsIterface::scopeUndef: 
								default: temp_buf = "undef"; break;
							}
							p_js_si->InsertString("scope", temp_buf);
							p_js_si->InsertString("scopeident", (temp_buf = p_ri->ScopeIdent).Unescape());
							if(p_ri->Scope == PPFtsIterface::scopeStyloQSvc) {
								if(bc_temp.FromMime64(temp_buf)) {
									StyloQCore::StoragePacket sp;
									StyloQFace face_pack;
									temp_buf.Z();
									if(P_T->SearchGlobalIdentEntry(StyloQCore::kForeignService, bc_temp, &sp) > 0) {
										if(sp.GetFace(SSecretTagPool::tagFace, face_pack) > 0) {
											face_pack.GetRepresentation(0, temp_buf);
										}
									}
									else if(P_T->SearchGlobalIdentEntry(StyloQCore::kNativeService, bc_temp, &sp) > 0) {
										if(sp.GetFace(SSecretTagPool::tagSelfyFace, face_pack) > 0) {
											face_pack.GetRepresentation(0, temp_buf);
										}										
									}
									if(temp_buf.NotEmpty()) {
										p_js_si->InsertString("nm", temp_buf.Escape());
									}
								}
							}
							p_js_list->InsertChild(p_js_si);
							//
							sslist.insert(new SBinaryChunk(bc_temp));
						}
					}
				}
				js_reply.Insert("scope_list", p_js_list);
			}
			{
				SJson * p_js_list = new SJson(SJson::tARRAY);
				for(uint i = 0; i < result_list.getCount(); i++) {
					const PPFtsIterface::SearchResultEntry * p_ri = result_list.at(i);
					if(p_ri) {
						SJson * p_js_ri = new SJson(SJson::tOBJECT);
						temp_buf.Z();
						switch(p_ri->Scope) {
							case PPFtsIterface::scopePPDb: temp_buf = "ppdb"; break;
							case PPFtsIterface::scopeStyloQSvc: temp_buf = "styloqsvc"; break;
							//case PPFtsIterface::scopeUndef: 
							default: temp_buf = "undef"; break;
						}
						p_js_ri->InsertString("scope", temp_buf);
						p_js_ri->InsertString("scopeident", (temp_buf = p_ri->ScopeIdent).Unescape());
						if(DS.GetObjectTypeSymb(p_ri->ObjType, temp_buf)) {
							p_js_ri->InsertString("objtype", temp_buf);
							p_js_ri->InsertInt64("objid", p_ri->ObjId);
						}
						p_js_ri->InsertInt64("rank", p_ri->Rank);
						p_js_ri->InsertDouble("weight", p_ri->Weight, MKSFMTD(0, 6, NMBF_NOTRAILZ));
						if(p_ri->Text.NotEmpty()) {
							p_js_ri->InsertString("text", (temp_buf = p_ri->Text).Escape());
						}
						p_js_list->InsertChild(p_js_ri);
					}
				}
				js_reply.Insert("result_list", p_js_list);
			}
			js_reply.ToStr(rResult);
			{
				SJson js_decl(SJson::tOBJECT);
				js_decl.InsertString("type", "search");
				js_decl.InsertString("format", "json");
				js_decl.InsertString("time", temp_buf.Z().CatCurDateTime(DATF_ISO8601|DATF_CENTURY, 0));
				js_decl.InsertString("displaymethod", "search");
				THROW_SL(js_decl.ToStr(rDocDeclaration));
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::ProcessCommand_PostDocument(const StyloQCore::StoragePacket & rCliPack, const SJson * pDeclaration, const SJson * pDocument, PPID * pResultID)
{
	int    result_obj_type = 0;
	PPID   result_obj_id = 0;
	//PPID   result_bill_id = 0;
	//PPID   result_tses_id = 0;
	Document doc;
	if(pDeclaration) {
			
	}
	THROW(doc.FromJsonObject(pDocument));
	THROW(oneof2(doc.OpID, PPEDIOP_ORDER, StyloQCommandList::sqbdtSvcReq)); // Пока только документ заказа умеем принимать
	THROW(checkdate(doc.CreationTime.d) || checkdate(doc.Time.d));
	if(doc.OpID == StyloQCommandList::sqbdtSvcReq) {
		THROW(doc.BkList.getCount()); // @todo @err
		{
			PPObjGoods gobj;
			PPObjTSession tses_obj;
			PPObjArticle ar_obj;
			PPIDArray tec_candidate_list;
			for(uint i = 0; i < doc.BkList.getCount(); i++) {
				const Document::BookingItem * p_item = doc.BkList.at(i);
				Goods2Tbl::Rec goods_rec;
				ProcessorTbl::Rec prc_rec;
				TechTbl::Rec tec_rec;
				TSessionPacket tses_pack;
				PPID   person_id = 0;
				THROW(p_item);
				THROW(gobj.Search(p_item->GoodsID, &goods_rec) > 0);
				THROW(checkdate(p_item->ReqTime.d) && checktime(p_item->ReqTime.t));
				if(p_item->PrcID) {
					THROW(tses_obj.GetPrc(p_item->PrcID, &prc_rec, 1, 0) > 0);
					THROW(tses_obj.GetTechByGoods(p_item->GoodsID, p_item->PrcID, &tec_rec) > 0); // @todo @err
				}
				else {
					// Найти подходящую технологию и любой свободный процессор
					LAssocArray prc_tec_list;
					tec_candidate_list.Z();
					tses_obj.TecObj.GetListByGoods(p_item->GoodsID, &tec_candidate_list);
					THROW(tec_candidate_list.getCount()); // @err no tech for such a ware
					tec_candidate_list.sortAndUndup();
					for(uint j = 0; j < tec_candidate_list.getCount(); j++) {
						const PPID tec_id = tec_candidate_list.get(j);
						if(tses_obj.GetTech(tec_id, &tec_rec) > 0) {
							
						}
					}
				}
				tses_obj.InitPacket(&tses_pack, TSESK_SESSION, p_item->PrcID, 0, -1);
				tses_pack.Rec.TechID = tec_rec.ID;
				tses_pack.Rec.StDt = p_item->ReqTime.d;
				tses_pack.Rec.StTm = p_item->ReqTime.t;
				{
					double duration = 0.0;
					if(tec_rec.Capacity > 0.0) {
						duration = 1.0 / tec_rec.Capacity;
					}
					LDATETIME dtm_finish = p_item->ReqTime;
					if(duration == 0.0)
						duration = 3600.0;
					dtm_finish.addsec(R0i(duration));
					if(checkdate(dtm_finish.d) && checktime(dtm_finish.t)) {
						tses_pack.Rec.FinDt = dtm_finish.d;
						tses_pack.Rec.FinTm = dtm_finish.t;
					}
				}
				{
					PPTransaction tra(1);
					THROW(tra);
					if(prc_rec.WrOffOpID) {
						PPOprKind op_rec;
						if(GetOpData(prc_rec.WrOffOpID, &op_rec) > 0) {
							PPObjAccSheet acs_obj;
							PPAccSheet acs_rec;
							if(op_rec.AccSheetID && acs_obj.Fetch(op_rec.AccSheetID, &acs_rec) > 0) {
								if(AcceptStyloQClientAsPerson(rCliPack, acs_rec.ObjGroup, &person_id, 0) > 0) {
									PPID ar_id = 0;
									if(ar_obj.P_Tbl->PersonToArticle(person_id, acs_rec.ID, &ar_id) > 0) {
										tses_pack.Rec.ArID = ar_id;
									}
								}
							}
						}
					}
					THROW(tses_obj.PutPacket(&result_obj_id, &tses_pack, 0));
					THROW(tra.Commit());
					result_obj_type = PPOBJ_TSESSION;
				}
			}
		}
	}
	else {
		THROW(doc.TiList.getCount()); // @todo @err
		{
			PPObjGoods gobj;
			{
				for(uint i = 0; i < doc.TiList.getCount(); i++) {
					const Document::TransferItem * p_item = doc.TiList.at(i);
					Goods2Tbl::Rec goods_rec;
					THROW(p_item);
					THROW(gobj.Search(p_item->GoodsID, &goods_rec) > 0);
					THROW(p_item->Set.Qtty > 0.0);
				}
			}
			{
				PPObjBill * p_bobj = BillObj;
				PPObjPerson psn_obj;
				PPObjArticle ar_obj;
				PPAlbatrossConfig acfg;
				PPBillPacket bpack;
				PPOprKind op_rec;
				PPObjAccSheet acs_obj;
				PPAccSheet acs_rec;
				THROW(p_bobj);
				DS.FetchAlbatrosConfig(&acfg);
				THROW(GetOpData(acfg.Hdr.OpID, &op_rec) > 0);
				{
					PPTransaction tra(1);
					THROW(tra);
					THROW(bpack.CreateBlank_WithoutCode(acfg.Hdr.OpID, 0, 0/*locID*/, 0));
					STRNSCPY(bpack.Rec.Code, doc.Code);
					bpack.SetupEdiAttributes(PPEDIOP_ORDER, "STYLOQ", 0); // @!
					bpack.SetGuid(doc.Uuid);
					if(checkdate(doc.Time.d))
						bpack.Rec.Dt = doc.Time.d;
					else if(checkdate(doc.CreationTime.d))
						bpack.Rec.Dt = doc.CreationTime.d;
					if(checkdate(doc.DueTime.d)) 
						bpack.Rec.DueDate = doc.DueTime.d;
					{
						PPID  person_id = 0;
						if(doc.ClientID) {
							ArticleTbl::Rec ar_rec;
							THROW(ar_obj.Search(doc.ClientID, &ar_rec) > 0);
							THROW(ar_rec.AccSheetID == op_rec.AccSheetID);
							bpack.Rec.Object = ar_rec.ID;
							if(doc.DlvrLocID) {
								PPID cli_psn_id = ObjectToPerson(ar_rec.ID, 0);
								if(cli_psn_id) {
									PPIDArray dlvr_loc_list;
									psn_obj.GetDlvrLocList(cli_psn_id, &dlvr_loc_list);
									if(dlvr_loc_list.lsearch(doc.DlvrLocID))
										bpack.SetFreight_DlvrAddrOnly(doc.DlvrLocID);
								}
							}
							{
								const PPID agent_acs_id = GetAgentAccSheet();
								if(agent_acs_id && acs_obj.Fetch(agent_acs_id, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup) {
									if(AcceptStyloQClientAsPerson(rCliPack, acs_rec.ObjGroup, &person_id, 0) > 0) {
										PPID ar_id = 0;
										if(ar_obj.P_Tbl->PersonToArticle(person_id, acs_rec.ID, &ar_id) > 0)
											bpack.Ext.AgentID = ar_id;
									}
								}
							}
						}
						else if(op_rec.AccSheetID && acs_obj.Fetch(op_rec.AccSheetID, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup) {
							if(AcceptStyloQClientAsPerson(rCliPack, acs_rec.ObjGroup, &person_id, 0) > 0) {
								PPID ar_id = 0;
								if(ar_obj.P_Tbl->PersonToArticle(person_id, acs_rec.ID, &ar_id) > 0) {
									bpack.Rec.Object = ar_id;
								}
							}
						}
					}
					for(uint i = 0; i < doc.TiList.getCount(); i++) {
						const Document::TransferItem * p_item = doc.TiList.at(i);
						Goods2Tbl::Rec goods_rec;
						THROW(p_item);
						THROW(gobj.Search(p_item->GoodsID, &goods_rec) > 0);
						THROW(p_item->Set.Qtty > 0.0);
						{
							PPTransferItem ti(&bpack.Rec, TISIGN_UNDEF);
							ti.SetupGoods(p_item->GoodsID);
							ti.Cost  = p_item->Set.Cost;
							ti.Price = p_item->Set.Price;
							ti.Quantity_ = fabs(p_item->Set.Qtty);
							THROW(bpack.LoadTItem(&ti, 0, 0));
						}
					}
					THROW(p_bobj->__TurnPacket(&bpack, 0, 1, 0));
					THROW(tra.Commit());
					result_obj_type = PPOBJ_BILL;
					result_obj_id = bpack.Rec.ID;
				}
			}
		}
	}
	CATCH
		result_obj_type = 0;
		result_obj_id = 0;
	ENDCATCH
	ASSIGN_PTR(pResultID, result_obj_id);
	return result_obj_type;
}

int PPStyloQInterchange::LoadViewSymbList()
{
	int    ok = -1;
	if(!(State & stViewSymbListLoaded)) {
		PPNamedFiltMngr nf_mgr;
		if(nf_mgr.GetResourceLists(&ViewSymbList, &ViewDescrList) > 0) {
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
	StyloQCore::StoragePacket own_pack;
	SString cmd_buf;
	SString command;
	SString reply_text_buf;
	SGeoPosLL geopos;
	SJson * p_js_reply = 0;
	SJson * p_js_cmd = 0;
	const SJson * p_in_doc = 0;
	const SJson * p_in_declaration = 0;
	THROW_PP(rCliIdent.Len(), PPERR_SQ_UNDEFCLIID);
	do_reply = true;
	THROW_PP(rRcvPack.P.Get(SSecretTagPool::tagRawData, &cmd_bch), PPERR_SQ_UNDEFCMDBODY);
	//ProcessCommand(cmd_bch, sess_secret);
	cmd_bch.ToRawStr(cmd_buf);
	if(json_parse_document(&p_js_cmd, cmd_buf.cptr()) == JSON_OK) {
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
			if(command.IsEqiAscii("register")) {
				// Регистрация //
				//p_js_reply = new SJson(SJson::tOBJECT);
				//p_js_reply->InsertString("reply", "Hello");
				PPLoadText(PPTXT_SQ_HELLO, reply_text_buf);
				reply_text_buf.Transf(CTRANSF_INNER_TO_UTF8);
				cmd_reply_ok = true;
			}
			else if(command.IsEqiAscii("requestblobinfolist")) { // @v11.3.6 Запрос информации о списке blob'ов с целью выяснить какие из них следует передавать (получать), а какие-нет.
				// Запрос содержит массив пар {blob-ident; block-hash} в ответ на который сервис-медиатор возвращает подмножество этого списка, содержащее набор
				// blob'ов, которые у него отсутствуют либо отличаются по хэшу.
				if(AmIMediator(command)) {
					StyloQCore::StoragePacket cli_pack;
					if(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0) {
						p_js_reply = ProcessCommand_ReqBlobInfoList(cli_pack, p_js_cmd);
						if(p_js_reply) {
							cmd_reply_ok = true;
						}
					}
				}
			}
			else if(command.IsEqiAscii("storeblob")) { // @v11.3.3
				// Команда отправляется от сервиса к сервису-медиатору для того, чтобы медиатор сохранил произвольный бинарный объект для доступа к нему клиентов
				if(AmIMediator(command)) {
					StyloQCore::StoragePacket cli_pack;
					if(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0) {
						SBinaryChunk blob;
						THROW(rRcvPack.P.Get(SSecretTagPool::tagBlob, &blob)); // @err
						if(ProcessCommand_StoreBlob(cli_pack, p_js_cmd, blob, reply_text_buf) > 0) {
							cmd_reply_ok = true;
						}
					}
				}
			}
			else if(command.IsEqiAscii("getblob")) { // @v11.3.3
				// Команда отправляется от клиента к сервису-медиатору для того, чтобы получить произвольный бинарный объект по идентификатору, предоставленному другим сервисом
				if(AmIMediator(command)) {
					StyloQCore::StoragePacket cli_pack;
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
					StyloQCore::StoragePacket cli_pack;
					if(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0) {
						if(ProcessCommand_IndexingContent(cli_pack, p_js_cmd, reply_text_buf) > 0) {
							cmd_reply_ok = true;
						}
					}
				}
			}
			else if(command.IsEqiAscii("search")) { // @v11.3.4
				// Команда поискового запроса
				StyloQCore::StoragePacket cli_pack;
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
									if(foreign_face.ToJson(temp_buf)) {
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
									p_js_reply = new SJson(SJson::tOBJECT);
									if(p_js_reply) {
										p_js_reply->Insert("config", p_js_fcfg);
										p_js_fcfg = 0;
										//
										if(foreign_face.GetCount()) {
											SJson * p_js_fface = foreign_face.ToJson(true/*forTransmission*/);
											if(p_js_fface) {
												p_js_reply->Insert("face", p_js_fface);
												p_js_fface = 0;
											}
										}
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
				//p_js_reply = new SJson(SJson::tOBJECT);
				//p_js_reply->InsertString("reply", "Bye");
				PPLoadText(PPTXT_SQ_GOODBYE, reply_text_buf);
				reply_text_buf.Transf(CTRANSF_INNER_TO_UTF8);
			}
			else if(command.IsEqiAscii("setface")) {
				StyloQCore::StoragePacket cli_pack;
				SBinaryChunk bc_face;
				if(!rRcvPack.P.Get(SSecretTagPool::tagFace, &bc_face)) {
				}
				else if(!SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0) {
				}
				else {
					StyloQFace other_face;
					assert(bc_face.Len()); // rRcvPack.P.Get гарантирует!
					if(bc_face.Len() && other_face.FromJson(bc_face.ToRawStr(temp_buf))) {
						// Необходимо модифицировать оригинальный face установкой
						// фактического времени истечения срока действия //
						other_face.Get(StyloQFace::tagExpiryPeriodSec, 0, temp_buf);
						const int64 ees = EvaluateExpiryTime(temp_buf.ToLong());
						if(ees > 0)
							other_face.Set(StyloQFace::tagExpiryEpochSec, 0, temp_buf.Z().Cat(ees));
						if(other_face.ToJson(temp_buf)) {
							bc_face.Z().Cat(temp_buf.cptr(), temp_buf.Len());
							cli_pack.Pool.Put(SSecretTagPool::tagFace, bc_face);
							PPID   temp_id = cli_pack.Rec.ID;
							if(P_T->PutPeerEntry(&temp_id, &cli_pack, 1)) {
								cmd_reply_ok = true;
							}
						}
					}					
				}
			}
			else if(command.IsEqiAscii("getconfig") || command.IsEqiAscii("getface")) {
				if(GetOwnPeerEntry(&own_pack) > 0) {
					SBinaryChunk own_ident;
					own_pack.Pool.Get(SSecretTagPool::tagSvcIdent, &own_ident);
					if(own_pack.Pool.Get(SSecretTagPool::tagConfig, &reply_config)) {
						assert(reply_config.Len());
						SString transmission_cfg_json;
						if(StyloQConfig::MakeTransmissionJson(reply_config.ToRawStr(temp_buf), transmission_cfg_json)) {
							reply_config.Z().Put(transmission_cfg_json.cptr(), transmission_cfg_json.Len());
							cmd_reply_ok = true;
						}
						else
							reply_config.Z();
					}
					if(own_pack.Pool.Get(SSecretTagPool::tagSelfyFace, &reply_face)) {
						assert(reply_face.Len());
						StyloQFace face_pack;
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
				}
			}
			else if(command.IsEqiAscii("getcommandlist")) {
				// Клиент запрашивает список доступных для него команд
				//SBinaryChunk cli_ident;
				StyloQCore::StoragePacket cli_pack;
				StyloQCommandList full_cmd_list;
				StyloQCommandList * p_target_cmd_list = 0;

				DbProvider * p_dict = CurDict;
				SString db_symb;
				if(!p_dict || !p_dict->GetDbSymb(db_symb)) {
					PPSetError(PPERR_SESSNAUTH);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
					PPSetError(PPERR_SQ_CMDSETLOADINGFAULT);
				}
				else {
					THROW(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0);
					if(full_cmd_list.Load(db_symb, 0)) {
						PPObjID oid(cli_pack.Rec.LinkObjType, cli_pack.Rec.LinkObjID);
						p_target_cmd_list = full_cmd_list.CreateSubListByContext(oid);
					}
					p_js_reply = StyloQCommandList::CreateJsonForClient(p_target_cmd_list, 0, 0, 4 * 3600);
					ZDELETE(p_target_cmd_list);
					cmd_reply_ok = true;
				}
			}
			else if(command.IsEqiAscii("dtlogin")) {
				// Десктоп-логин (экспериментальная функция)
				StyloQCore::StoragePacket cli_pack;
				if(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0) {
					if(cli_pack.Rec.Kind == StyloQCore::kClient && cli_pack.Rec.LinkObjType == PPOBJ_USR && cli_pack.Rec.LinkObjID) {
						PPObjSecur sec_obj(PPOBJ_USR, 0);
						PPSecur sec_rec;
						if(sec_obj.Search(cli_pack.Rec.LinkObjID, &sec_rec) > 0) {
							if(!sstreqi_ascii(sec_rec.Name, PPSession::P_JobLogin) && !sstreqi_ascii(sec_rec.Name, PPSession::P_EmptyBaseCreationLogin)) {
								if(OnetimePass(sec_rec.ID) > 0) {
									PPLoadText(PPTXT_SQ_CMDSUCCESS_DTLOGIN, reply_text_buf);
									reply_text_buf.Transf(CTRANSF_INNER_TO_UTF8);
									//p_js_reply = new SJson(SJson::tOBJECT);
									//p_js_reply->InsertString("reply", "Your request for login is accepted :)");
									cmd_reply_ok = true;
								}
							}
						}
					}
				}
				if(!cmd_reply_ok) {
					PPSetError(PPERR_SQ_CMDFAULT_DTLOGIN);
					//p_js_reply = new SJson(SJson::tOBJECT);
					//p_js_reply->InsertString("reply", "Your request for login is not accepted :(");
				}
			}
			else if(command.IsEqiAscii("echo")) {
				p_js_reply = new SJson(SJson::tOBJECT);
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
			else if(command.IsEqiAscii("postdocument")) { // Отправка документа от клиента сервису
				// Передается document-declaration and document
				// document содержится как объект в json команды с тегом document
				// document-declaration находится в общем пакете под тегом SSecretTagPool::tagDocDeclaration
				// В ответ отправляется сигнал о успешной либо безуспешной обработке команды
				//
				StyloQCore::StoragePacket cli_pack;
				if(SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0) {
					PPID   result_obj_id = 0;
					PPID   result_obj_type = ProcessCommand_PostDocument(cli_pack, p_in_declaration, p_in_doc, &result_obj_id);
					if(result_obj_type == PPOBJ_BILL) {
						assert(result_obj_id);
						S_GUID _uuid;
						p_js_reply = new SJson(SJson::tOBJECT);
						p_js_reply->InsertInt("result-objtype", result_obj_type);
						p_js_reply->InsertInt("document-id", result_obj_id);
						PPRef->Ot.GetTagGuid(PPOBJ_BILL, result_obj_id, PPTAG_BILL_UUID, _uuid);
						if(!!_uuid) {
							temp_buf.Z().Cat(_uuid, S_GUID::fmtIDL);
							p_js_reply->InsertString("document-uuid", temp_buf);
						}
						p_js_reply->InsertString("result", "ok");
						cmd_reply_ok = true;
					}
					else if(result_obj_type == PPOBJ_TSESSION) {
						assert(result_obj_id);
						S_GUID _uuid;
						p_js_reply = new SJson(SJson::tOBJECT);
						p_js_reply->InsertInt("result-objtype", result_obj_type);
						p_js_reply->InsertInt("document-id", result_obj_id);
						PPRef->Ot.GetTagGuid(PPOBJ_TSESSION, result_obj_id, PPTAG_TSESS_UUID, _uuid);
						if(!!_uuid) {
							temp_buf.Z().Cat(_uuid, S_GUID::fmtIDL);
							p_js_reply->InsertString("document-uuid", temp_buf);
						}
						p_js_reply->InsertString("result", "ok");
						cmd_reply_ok = true;
					}
				}
			}
			else {
				S_GUID cmd_uuid;
				StyloQCore::StoragePacket cli_pack;
				if(cmd_uuid.FromStr(command) && SearchGlobalIdentEntry(StyloQCore::kClient, rCliIdent, &cli_pack) > 0) {
					StyloQCommandList full_cmd_list;
					DbProvider * p_dict = CurDict;
					SString db_symb;
					if(!p_dict || !p_dict->GetDbSymb(db_symb)) {
						PPSetError(PPERR_SESSNAUTH);
						PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
						PPSetError(PPERR_SQ_CMDSETLOADINGFAULT);
					}
					else if(full_cmd_list.Load(db_symb, 0)) {
						const PPObjID oid(cli_pack.Rec.LinkObjType, cli_pack.Rec.LinkObjID);
						const StyloQCommandList::Item * p_item = full_cmd_list.GetByUuid(cmd_uuid);
						THROW(p_item);
						StyloQCommandList * p_target_cmd_list = full_cmd_list.CreateSubListByContext(oid);
						const StyloQCommandList::Item * p_targeted_item = p_target_cmd_list ? p_target_cmd_list->GetByUuid(cmd_uuid) : 0;
						THROW_PP(p_targeted_item, PPERR_SQ_UNTARGETEDCMD);
						switch(p_targeted_item->BaseCmdId) {
							case StyloQCommandList::sqbcPersonEvent:
								if(ProcessCommand_PersonEvent(StyloQCommandList::Item(*p_targeted_item), cli_pack, geopos)) {
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
								if(ProcessCommand_Report(StyloQCommandList::Item(*p_targeted_item), cli_pack, geopos, reply_text_buf, temp_buf.Z())) {
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
									// @v11.2.12 {
									if(intermediateReplyProc) { 
										StyloQProtocol irp;
										irp.StartWriting(PPSCMD_SQ_COMMAND, StyloQProtocol::psubtypeIntermediateReply);
										{
											SJson ir_js(SJson::tOBJECT);
											ir_js.InsertInt("waitms", 5*60*1000);
											ir_js.InsertInt("pollintervalms", 1000);
											ir_js.ToStr(cmd_buf);
											cmd_bch.Put(cmd_buf.cptr(), cmd_buf.Len());
											irp.P.Put(SSecretTagPool::tagRawData, cmd_bch, 0);
										}
										irp.FinishWriting(pSessSecret);
										intermediateReplyProc(irp, pIntermediateReplyExtra);
									}
									// } @v11.2.12 
									if(ProcessCommand_RsrvOrderPrereq(StyloQCommandList::Item(*p_targeted_item), cli_pack, geopos, reply_text_buf, temp_buf.Z())) {
										reply_doc.Put(reply_text_buf, reply_text_buf.Len());
										if(temp_buf.Len())
											reply_doc_declaration.Put(temp_buf, temp_buf.Len());
										cmd_reply_ok = true;
									}
									else {
										PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
										PPSetError(PPERR_SQ_CMDFAULT_REPORT);
									}
								}
								break;
							case StyloQCommandList::sqbcRsrvAttendancePrereq: // v11.3.2
								{
									if(intermediateReplyProc) { 
										StyloQProtocol irp;
										irp.StartWriting(PPSCMD_SQ_COMMAND, StyloQProtocol::psubtypeIntermediateReply);
										{
											SJson ir_js(SJson::tOBJECT);
											ir_js.InsertInt("waitms", 5*60*1000);
											ir_js.InsertInt("pollintervalms", 1000);
											ir_js.ToStr(cmd_buf);
											cmd_bch.Put(cmd_buf.cptr(), cmd_buf.Len());
											irp.P.Put(SSecretTagPool::tagRawData, cmd_bch, 0);
										}
										irp.FinishWriting(pSessSecret);
										intermediateReplyProc(irp, pIntermediateReplyExtra);
									}
									if(ProcessCommand_RsrvAttendancePrereq(StyloQCommandList::Item(*p_targeted_item), cli_pack, geopos, reply_text_buf, temp_buf.Z())) {
										reply_doc.Put(reply_text_buf, reply_text_buf.Len());
										if(temp_buf.Len())
											reply_doc_declaration.Put(temp_buf, temp_buf.Len());
										cmd_reply_ok = true;
									}
									else {
										PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
										PPSetError(PPERR_SQ_CMDFAULT_REPORT);
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
			p_js_reply = new SJson(SJson::tOBJECT);
			p_js_reply->InsertString("result", cmd_reply_ok ? "ok" : "error");
			if(!cmd_reply_ok) {
				PPGetLastErrorMessage(1, temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
				p_js_reply->InsertInt("errcode", PPErrCode);
				p_js_reply->InsertString("errmsg", temp_buf);
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
	int SrpAuth()
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
		SBinaryChunk other_public;
		SBinaryChunk my_public;
		SBinaryChunk srp_s;
		SBinaryChunk srp_v;
		SBinaryChunk __a; // A
		SBinaryChunk __b; // B
		SBinaryChunk __m; // M
		SBinaryChunk temp_bc;
		SBinaryChunk debug_cli_secret; // @debug do remove after debugging!
		THROW(TPack.P.Get(SSecretTagPool::tagSessionPublicKey, &other_public));
		//THROW_PP(TPack.P.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_UNDEFCLIID);
		THROW_PP(CliIdent.Len(), PPERR_SQ_UNDEFCLIID);
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
			THROW(rpe.SetupStyloQRpc(my_public, other_public, StartUp_Param.MqbInitParam.ConsumeParamList.CreateNewItem()));
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
					assert(other_public.Len());
					assert(B.Sess.Get(SSecretTagPool::tagSessionPrivateKey, 0));
					sess_pack.Pool.Put(SSecretTagPool::tagSessionPublicKey, my_public);
					{
						B.Sess.Get(SSecretTagPool::tagSessionPrivateKey, &temp_chunk);
						sess_pack.Pool.Put(SSecretTagPool::tagSessionPrivateKey, temp_chunk);
					}
					sess_pack.Pool.Put(SSecretTagPool::tagSessionPublicKeyOther, other_public);
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
	int Acquaintance()
	{
		int    ok = 1;
		PPMqbClient * p_mqbc = 0;
		SBinaryChunk other_public;
		SBinaryChunk my_public;
		//SBinaryChunk sess_secret; // @debug
		StyloQProtocol reply_tp;
		PPMqbClient::RoutingParamEntry rpe;
		PPMqbClient::MessageProperties props;
		THROW_PP(CliIdent.Len(), PPERR_SQ_UNDEFCLIID);
		THROW_PP(TPack.P.Get(SSecretTagPool::tagSessionPublicKey, &other_public), PPERR_SQ_UNDEFSESSPUBKEY);
		assert(other_public.Len());
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
			THROW(rpe.SetupStyloQRpc(my_public, other_public, StartUp_Param.MqbInitParam.ConsumeParamList.CreateNewItem()));
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
	int Session()
	{
		int    ok = 1;
		PPMqbClient * p_mqbc = 0;
		SBinaryChunk temp_chunk;
		SBinaryChunk other_public;
		SBinaryChunk my_public;
		//SBinaryChunk cli_ident;
		SBinaryChunk sess_secret;
		SBinaryChunk * p_sess_secret = 0;
		StyloQProtocol reply_tp;
		PPMqbClient::RoutingParamEntry rpe;
		PPMqbClient::MessageProperties props;
		StyloQCore::StoragePacket pack;
		THROW_PP(TPack.P.Get(SSecretTagPool::tagSessionPublicKey, &other_public), PPERR_SQ_INPHASNTSESSPUBKEY); // PPERR_SQ_INPHASNTSESSPUBKEY Входящий запрос сессии не содержит публичного ключа
		//THROW_PP(TPack.P.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_INPHASNTCLIIDENT); 
		THROW_PP(CliIdent.Len(), PPERR_SQ_INPHASNTCLIIDENT); // PPERR_SQ_INPHASNTCLIIDENT   Входящий запрос сессии не содержит идентификатора контрагента
		assert(other_public.Len());
		//assert(cli_ident.Len());
		THROW(Login());
		assert(P_Ic == 0);
		THROW_SL(P_Ic = new PPStyloQInterchange);
		THROW(P_Ic->SearchSession(other_public, &pack) > 0); 
		THROW_PP(pack.Pool.Get(SSecretTagPool::tagSessionPublicKey, &my_public), PPERR_SQ_INRSESSHASNTPUBKEY); // PPERR_SQ_INRSESSHASNTPUBKEY      Сохраненная сессия не содержит публичного ключа
		THROW_PP(pack.Pool.Get(SSecretTagPool::tagSessionPublicKeyOther, &temp_chunk), PPERR_SQ_INRSESSHASNTOTHERPUBKEY); // PPERR_SQ_INRSESSHASNTOTHERPUBKEY Сохраненная сессия не содержит публичного ключа контрагента
		THROW_PP(temp_chunk == other_public, PPERR_SQ_INRSESPUBKEYNEQTOOTR); // PPERR_SQ_INRSESPUBKEYNEQTOOTR   Публичный ключ сохраненной сессии не равен полученному от контрагента
		THROW_PP(pack.Pool.Get(SSecretTagPool::tagClientIdent, &temp_chunk), PPERR_SQ_INRSESSHASNTCLIIDENT); // PPERR_SQ_INRSESSHASNTCLIIDENT   Сохраненная сессия не содержит идентификатора контрагента
		THROW_PP(temp_chunk == CliIdent, PPERR_SQ_INRSESCLIIDENTNEQTOOTR); // PPERR_SQ_INRSESCLIIDENTNEQTOOTR Идентификатора контрагента сохраненной сессии не равен полученному от контрагента
		THROW_PP(pack.Pool.Get(SSecretTagPool::tagSessionSecret, &sess_secret), PPERR_SQ_INRSESSHASNTSECRET); // PPERR_SQ_INRSESSHASNTSECRET     Сохраненная сессия не содержит секрета 
		B.Sess.Put(SSecretTagPool::tagSessionSecret, sess_secret);
		p_sess_secret = &sess_secret;
		B.Other.Put(SSecretTagPool::tagClientIdent, CliIdent);
		THROW(rpe.SetupStyloQRpc(my_public, other_public, StartUp_Param.MqbInitParam.ConsumeParamList.CreateNewItem()));
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
		//PPMqbClient * p_mqbc = 0;
		bool do_process_loop = false;
		SString temp_buf;
		SString log_buf;
		StyloQProtocol tp;
		StyloQProtocol reply_tp;
		PPMqbClient::MessageProperties props;
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
				if(Acquaintance()) {
					assert(B.Sess.Get(SSecretTagPool::tagSessionSecret, 0));
					do_process_loop = true; // OK
				}
				else {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
				}
				break;
			case PPSCMD_SQ_SESSION: // Команда инициации соединения по значению сессии, которая была установлена на предыдущем сеансе обмена
				if(Session()) {
					assert(B.Sess.Get(SSecretTagPool::tagSessionSecret, 0));
					do_process_loop = true; // OK
				}
				else {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
				}
				break;
			case PPSCMD_SQ_SRPAUTH: // Команда инициации соединения методом SRP-авторизации по параметрам, установленным ранее 
				if(SrpAuth()) {
					assert(B.Sess.Get(SSecretTagPool::tagSessionSecret, 0));
					do_process_loop = true; // OK
				}
				else {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
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
					const long live_silence_period = 2 * 60 * 1000;
					clock_t clk_start = clock();
					SString reply_status_text;
					while(do_process_loop) {
						assert(B.P_Mqbc && B.P_MqbRpe);
						bool is_there_my_message = false;
						if(B.P_Mqbc->ConsumeMessage(env, 1000) > 0) { // @v11.3.2 timeout 200-->1000
							if(!B.Uuid || S_GUID(env.Msg.Props.CorrelationId) == B.Uuid) {
								{ // @debug
									log_buf.Z().Cat("mqb-sess-got-own-msg").Space().Cat(GetThreadID()).Space().Cat(env.Msg.Props.CorrelationId);
									PPLogMessage(PPFILNAM_DEBUG_LOG, log_buf, LOGMSGF_TIME);
								}
								is_there_my_message = true;
							}
							else {
								{ // @debug
									log_buf.Z().Cat("mqb-sess-got-another's-msg").Space().Cat(GetThreadID()).Space().Cat(env.Msg.Props.CorrelationId);
									PPLogMessage(PPFILNAM_DEBUG_LOG, log_buf, LOGMSGF_TIME);
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
									int32 reply_status = 0;
									reply_status_text.Z();
									reply_tp.Z();
									if(P_Ic->Registration_ServiceReply(B, tp)) {
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
											if(StyloQConfig::MakeTransmissionJson(bc.ToRawStr(temp_buf), transmission_cfg_json)) {
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
						}
						else {
							const clock_t silence_period = (clock() - clk_start);
							if(silence_period >= live_silence_period) {
								(temp_buf = "Service Stylo-Q roundtrip is finished after").Space().Cat(silence_period).Cat("ms of silence");
								PPLogMessage(PPFILNAM_INFO_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_COMP);
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
	THROW(sp.Pool.Get(SSecretTagPool::tagConfig, &bin_chunk));
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

			PPFtsIterface fts_db(true/*forUpdate*/, 120000);
			THROW(fts_db);
			for(uint didx = 0; didx < doc_id_list.getCount(); didx++) {
				const PPID doc_id = doc_id_list.get(didx);
				StyloQCore::StoragePacket doc_pack;
				THROW(GetPeerEntry(doc_id, &doc_pack) > 0);
				if(doc_pack.Rec.Flags & StyloQCore::fUnprocessedDoc) {
					if(doc_pack.Pool.Get(SSecretTagPool::tagRawData, &bin_chunk)) {
						bin_chunk.ToRawStr(temp_buf);
						PPFtsIterface::TransactionHandle tra(fts_db);
						bool   is_local_fault = true;
						if(!!tra) {
							if(IndexingContent_Json(&tra, &txa, temp_buf)) {
								if(tra.Commit()) {
									doc_pack.Rec.Flags &= ~StyloQCore::fUnprocessedDoc;
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
		TSCollection <Entry> EntryList;
		TSCollection <PPMqbClient> ConnList;
	};
public:
	StyloQServer2(const TSCollection <LaunchEntry> & rEntries) : PPThread(kStyloQServer, 0, 0)
	{
		TSCollection_Copy(Entries, rEntries);
	}
	virtual void Run()
	{
		const int   do_debug_log = 0; // @debug
		const long  pollperiod_mqc = 500;
		EvPollTiming pt_mqc(pollperiod_mqc, false);
		EvPollTiming pt_purge(3600000, true); // этот тайминг не надо исполнять при запуске. Потому registerImmediate = 1
		const int  use_sj_scan_alg2 = 0;
		SString msg_buf, temp_buf;
		RunBlock run_blk;
		for(uint i = 0; i < Entries.getCount(); i++) {
			const LaunchEntry * p_le = Entries.at(i);
			if(p_le) {
				uint   conn_idx = 0;
				for(uint j = 0; !conn_idx && j < run_blk.ConnList.getCount(); j++) {
					const PPMqbClient * p_mqb_cli = run_blk.ConnList.at(j);
					if(p_mqb_cli && p_mqb_cli->IsHostEqual(p_le->P.MqbInitParam.Host, p_le->P.MqbInitParam.Port))
						conn_idx = j+1;							
				}
				if(conn_idx) {
					RunBlock::Entry * p_new_entry = new RunBlock::Entry(p_le, conn_idx);
					PPMqbClient * p_mqb_cli = run_blk.ConnList.at(conn_idx-1);
					if(p_le->P.MqbInitParam.ConsumeParamList.getCount()) {
						for(uint cplidx = 0; cplidx < p_le->P.MqbInitParam.ConsumeParamList.getCount(); cplidx++) {
							PPMqbClient::RoutingParamEntry * p_rpe = p_le->P.MqbInitParam.ConsumeParamList.at(cplidx);
							if(p_rpe) {
								p_mqb_cli->ApplyRoutingParamEntry(*p_rpe);
								p_mqb_cli->Consume(p_rpe->QueueName, &p_rpe->ConsumeTag.Z(), 0);
							}
						}
					}
					run_blk.EntryList.insert(p_new_entry);
				}
				else {
					// Инициализация MQB exchange
					//p_mqb_cli->ExchangeDeclare("styloqrpc", PPMqbClient::exgtDirect, 0);
					PPMqbClient * p_mqb_cli = PPMqbClient::CreateInstance(p_le->P.MqbInitParam);
					if(p_mqb_cli) {
						if(run_blk.ConnList.insert(p_mqb_cli)) {
							conn_idx = run_blk.ConnList.getCount();
							RunBlock::Entry * p_new_entry = new RunBlock::Entry(p_le, conn_idx);
							run_blk.EntryList.insert(p_new_entry);
						}
					}
					else
						PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP);
				}
			}
		}
		if(run_blk.EntryList.getCount()) {
			PPMqbClient::Envelope mqb_envelop;
			const long __cycle_hs = 37; // Период таймера в сотых долях секунды (37)
			int    queue_stat_flags_inited = 0;
			StyloQProtocol tpack;
			Evnt   stop_event(SLS.GetStopEventName(temp_buf), Evnt::modeOpen);
			for(int stop = 0; !stop;) {
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
								if(p_mqb_cli) {
									while(p_mqb_cli->ConsumeMessage(mqb_envelop, 200) > 0) {
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
	PPIniFile ini_file;
	PPDbEntrySet2 dbes;
	THROW(ini_file.IsValid());
	THROW(dbes.ReadFromProfile(&ini_file, 0));
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
				if(p_ldb && p_ldb->P_Sqc->GetOwnPeerEntry(&sp) > 0 && sp.Pool.Get(SSecretTagPool::tagSvcIdent, &bc)) {
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
														(msg_buf = "Stylo-Q MQB server's entry-point was created").CatDiv(':', 2).CatEq("db", p_map_entry->DbSymb).Space().
															CatEq("host", p_new_ll_entry->P.MqbInitParam.Host).Space().CatEq("svcid", temp_buf);
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
			}
		}
		if(ll.getCount()) {
			StyloQServer2 * p_srv = new StyloQServer2(ll);
			CALLPTRMEMB(p_srv, Start(0));
		}
		else {
			msg_buf = "No Stylo-Q MQB server's entry-points were found";
			PPLogMessage(PPFILNAM_SERVER_LOG, msg_buf, LOGMSGF_TIME);
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
			ThreadID tid = ex_list.get(i);
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
	int    spir = SetupPeerInstance(&own_peer_id, 1);
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
	SJson * p_js_result = new SJson(SJson::tOBJECT);
	//size_t actual_size = 0;
	//StyloQCore::StoragePacket own_pack;
	//SBinaryChunk bc_own_ident;
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
		//temp_buf.EncodeMime64(pBlobBuf, blobSize);
		//p_js_result->InsertString("content", temp_buf);
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
					if(p_itm->P_Child) {
						rep_entry.HashAlg = SlHash::IdentifyAlgorithmSymb(p_itm->P_Child->Text.Unescape());
					}
				}
				else if(p_itm->Text.IsEqiAscii("hash")) {
					if(p_itm->P_Child) {
						rep_entry.Hash.FromMime64(p_itm->P_Child->Text.Unescape());
					}
				}
				else if(p_itm->Text.IsEqiAscii("missing")) {
					if(p_itm->P_Child && p_itm->P_Child->IsTrue()) {
						rep_entry.Missing = true;
					}
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
							}
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
					SString temp_buf;
					StyloQCore::StoragePacket own_pack;
					SBinaryChunk bc_own_ident;
					if(P_Ic->GetOwnPeerEntry(&own_pack) > 0 && own_pack.Pool.Get(SSecretTagPool::tagSvcIdent, &bc_own_ident)) {
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
									if(req_blob_list.ParseRequestInfoListReply(p_js)) {
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
								}
							}
							delete p_js;
						}
					}
				}
				else if(cmd == cmdGetBlob) {
					SString temp_buf;
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
									}
								}
								else {
									if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
										p_js = SJson::Parse(raw_data.ToRawStr(json_buf));
										if(p_js) {
											setCtrlString(CTL_STQCLITEST_RESULT, json_buf);
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
					SString temp_buf;
					{
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
														}
													}
													else {
														if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
															p_js = SJson::Parse(raw_data.ToRawStr(json_buf));
															if(p_js) {
																setCtrlString(CTL_STQCLITEST_RESULT, json_buf);
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
						PPObjIDArray oid_list;
						if(CurDict) {
							CurDict->GetDbSymb(db_symb);
						}
						if(db_symb.NotEmpty()) {
							if(full_cmd_list.Load(0, 0)) {
								p_cmd_list = full_cmd_list.CreateSubListByDbSymb(db_symb, StyloQCommandList::sqbcRsrvPushIndexContent);
								if(p_cmd_list) {
									assert(p_cmd_list->GetCount());
									p_cmd_item = p_cmd_list->GetC(0);
								}
							}
						}
						dip.ClearParam();
						if(P_Ic->MakeIndexingRequestCommand(&own_pack, p_cmd_item, 3600*24, oid_list, dip.CommandJson)) {
							int ir = P_Ic->DoInterchange(dip, svc_reply);
							SBinaryChunk raw_data;
							if(svc_reply.Get(SSecretTagPool::tagRawData, &raw_data)) {
								SJson * p_js = SJson::Parse(raw_data.ToRawStr(json_buf));
								if(p_js) {
									setCtrlString(CTL_STQCLITEST_RESULT, json_buf);
								}
								delete p_js;
							}
						}
						ZDELETE(p_cmd_list);
					}
				}
				else if(cmd == cmdSearch) {
					SString temp_buf;
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
	PPObjIDArray oid_list;
	//mediator_list.shuffle();
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
	if(MakeIndexingRequestCommand(&rOwnPack, pCmdItem, 3600*24, oid_list, dip_indexing.CommandJson)) {
		SSecretTagPool svc_reply;
		SString temp_buf;
		SBinaryChunk own_ident;
		THROW_PP(rOwnPack.Pool.Get(SSecretTagPool::tagSvcIdent, &own_ident), PPERR_SQ_UNDEFOWNSVCID);
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
					oid_list.Add(local_oid.Obj, local_oid.Id);
				}
			}
			oid_list.SortAndUndup();
			for(uint oididx = 0; oididx < oid_list.getCount(); oididx++) {
				PPObjID oid = oid_list.at(oididx);
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
						p_js = SJson::Parse(raw_data.ToRawStr(temp_buf));
						if(req_blob_list.ParseRequestInfoListReply(p_js)) {
							STempBuffer blob_buf(SMEGABYTE(2));
							for(uint rblidx = 0; rblidx < req_blob_list.getCount(); rblidx++) {
								Stq_ReqBlobInfoEntry * p_entry = req_blob_list.at(rblidx);
								assert(p_entry);
								if(p_entry && (p_entry->Missing || p_entry->RepDiffHash)) {
									BlobInfo bi;
									dip_blob.ClearParam();
									if(GetBlobInfo(own_ident, p_entry->Oid, 0, bi, &dip_blob.Blob)) {
										assert(bi.Signature == p_entry->Signature);
										assert(bi.Hash == p_entry->Hash);
										SJson * p_js_blob = MakeQuery_StoreBlob(dip_blob.Blob.PtrC(), dip_blob.Blob.Len(), p_entry->Signature);
										if(p_js_blob) {
											p_js_blob->ToStr(dip_blob.CommandJson);
											//dip_blob.Blob.Put(blob_buf, actual_size);
											ZDELETE(p_js_blob);
											if(DoInterchange(dip_blob, svc_reply)) {
												; // Stylo-Q: blob объекта передан медиатору
											}
											else {
												; // Stylo-Q: ошибка передачи медиатору blob'а объекта 
											}
										}										
									}
									#if 0 // {
									if(p_entry->Oid.Obj == PPOBJ_STYLOQBINDERY) {

									}
									else {
										SFile f_in(p_entry->SrcPath, SFile::mRead|SFile::mBinary);
										if(f_in.IsValid() /*&& f_in.CalcHash(0, hash_func, bc_hash)*/) {
											size_t actual_size = 0;
											f_in.ReadAll(blob_buf, 0, &actual_size);
											SJson * p_js_blob = MakeQuery_StoreBlob(blob_buf, actual_size, p_entry->Signature);
											if(p_js_blob) {
												dip_blob.ClearParam();
												p_js_blob->ToStr(dip_blob.CommandJson);
												dip_blob.Blob.Put(blob_buf, actual_size);
												ZDELETE(p_js_blob);
												if(DoInterchange(dip_blob, svc_reply)) {
													; // Stylo-Q: blob объекта передан медиатору
												}
												else {
													; // Stylo-Q: ошибка передачи медиатору blob'а объекта 
												}
											}
										}
									}
									#endif // } 0
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
	}
	CATCHZOK
	return ok;
}

/*static*/int PPStyloQInterchange::ExecuteIndexingRequest()
{
	int    ok = 1;
	StyloQCore::SvcDbSymbMap dbmap;
	SBinaryChunk bc;
	SBinaryChunk cfg_bytes;
	SString url_buf;
	SString msg_buf;
	PPIniFile ini_file;
	PPDbEntrySet2 dbes;
	StyloQCommandList * p_cmd_list = 0;
	PPSession::LimitedDatabaseBlock * p_ldb = 0;
	char   secret[64];
	bool   is_logged_in = false;
	// Так как среди точек входа в базу данных могут быть дубликаты (разные точки, но база одна), то
	// элиминируем эти дубликаты по значению service-ident. Следующий список как раз для этого и предназначен.
	TSCollection <SBinaryChunk> reckoned_svc_id_list;
	StyloQCommandList full_cmd_list;
	//
	THROW(ini_file.IsValid());
	THROW(dbes.ReadFromProfile(&ini_file, 0));
	{
		PPVersionInfo vi = DS.GetVersionInfo();
		THROW(vi.GetSecret(secret, sizeof(secret)));
	}
	THROW(StyloQCore::GetDbMap(dbmap));
	THROW(full_cmd_list.Load(0, 0));
	for(uint i = 0; i < dbmap.getCount(); i++) {
		const StyloQCore::SvcDbSymbMapEntry * p_map_entry = dbmap.at(i);
		if(p_map_entry && p_map_entry->DbSymb.NotEmpty() && p_map_entry->SvcIdent.Len()) {
			StyloQCore::StoragePacket sp;
			ZDELETE(p_ldb);
			p_ldb = DS.LimitedOpenDatabase(p_map_entry->DbSymb, PPSession::lodfReference|PPSession::lodfStyloQCore|PPSession::lodfSysJournal);
			if(p_ldb && p_ldb->P_Sqc->GetOwnPeerEntry(&sp) > 0 && sp.Pool.Get(SSecretTagPool::tagSvcIdent, &bc)) {
				TSCollection <StyloQCore::IgnitionServerEntry> mediator_list;
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
											ic.Helper_ExecuteIndexingRequest(sp, mediator_list, p_cmd_item);
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
	CATCHZOK
	if(is_logged_in)
		DS.Logout();
	memzero(secret, sizeof(secret));
	delete p_ldb;
	delete p_cmd_list;
	return ok;
}
