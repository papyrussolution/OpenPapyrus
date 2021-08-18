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
const long __DefMqbConsumeTimeout = 10000;
/*
	RESERVED COMMANDS:
	
	register
	test
	getface
	getcmdlist
	login
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
StyloQFace::StyloQFace() : Id(0), Flags(0)
{
}

StyloQFace::~StyloQFace()
{
}

StyloQFace & StyloQFace::Z()
{
	Id = 0;
	Flags = 0;
	L.Z();
	return *this;
}

static const SIntToSymbTabEntry StyloQFaceTagNameList[] = {
	{ StyloQFace::tagModifTime,      "modtime" },
	{ StyloQFace::tagVerifiable,     "verifialbe" },
	{ StyloQFace::tagCommonName,     "cn" },
	{ StyloQFace::tagName,           "name" },
	{ StyloQFace::tagSurName,        "surname" },
	{ StyloQFace::tagPatronymic,     "patronymic" },
	{ StyloQFace::tagDOB,            "dob" },
	{ StyloQFace::tagPhone,          "phone" },
	{ StyloQFace::tagGLN,            "gln" },
	{ StyloQFace::tagCountryIsoSymb, "countryisosymb" },
	{ StyloQFace::tagCountryIsoCode, "countryisocode" },
	{ StyloQFace::tagCountryName,    "country" },
	{ StyloQFace::tagZIP,            "zip" },
	{ StyloQFace::tagCityName,       "city" },
	{ StyloQFace::tagStreet,         "street" },
	{ StyloQFace::tagAddress,        "address" },
	{ StyloQFace::tagLatitude,       "lat" },
	{ StyloQFace::tagLongitude,      "lon" },
	{ StyloQFace::tagDescr,          "descr" },
	{ StyloQFace::tagImage,          "image" },
	{ StyloQFace::tagRuINN,          "ruinn" },
	{ StyloQFace::tagRuKPP,          "rukpp" },
	{ StyloQFace::tagRuSnils,        "rusnils" },
};

int StyloQFace::FromJson(const char * pJsonText)
{
	int    ok = 0;
	Z();
	if(!isempty(pJsonText)) {
		SJson * p_js = 0;
		if(json_parse_document(&p_js, pJsonText)) {
			assert(p_js);
			if(p_js->Type == SJson::tOBJECT) {
				SString tag_symb;
				SString lang_symb;
				SString temp_buf;
				SJson * p_next = 0;
				for(SJson * p_cur = p_js->P_Child; p_cur; p_cur = p_next) {
					p_next = p_cur->P_Next;
					if(p_cur->P_Child && p_cur->P_Child->Text.NotEmpty()) {
						p_cur->Text.Divide('.', tag_symb, lang_symb);
						int tag_id = SIntToSymbTab_GetId(StyloQFaceTagNameList, SIZEOFARRAY(StyloQFaceTagNameList), tag_symb);
						if(tag_id) {
							int lang_id = 0;
							if(IsTagLangDependent(tag_id) && lang_symb.NotEmptyS()) {
								lang_id = RecognizeLinguaSymb(lang_symb, 1);
								SETIFZ(lang_id, -1);
							}
							if(tag_id > 0 && lang_id >= 0) {
								if(Set(tag_id, lang_id, p_cur->P_Child->Text) > 0)
									ok = 1;
							}
						}
					}
				}			
			}
		}
	}
	else
		ok = -1;
	return ok;
}

int StyloQFace::ToJson(SString & rResult) const
{
	int    ok = 0;
	rResult.Z();
	if(L.getCount()) {
		const long zero = 0L;
		LongArray lang_list;
		SString temp_buf;
		SString lang_code;
		SString tag_value;
		GetLanguageList(lang_list);
		lang_list.atInsert(0, &zero);
		SJson * p_js = new SJson(SJson::tOBJECT);
		for(uint i = 0; i < SIZEOFARRAY(StyloQFaceTagNameList); i++) {
			const SIntToSymbTabEntry & r_idx_entry = StyloQFaceTagNameList[i];
			if(IsTagLangDependent(r_idx_entry.Id)) {
				for(uint li = 0; li < lang_list.getCount(); li++) {
					const int lang = lang_list.get(li);
					if(GetExactly(r_idx_entry.Id, lang, tag_value)) {
						if(!lang) {
							p_js->InsertString(r_idx_entry.P_Symb, tag_value);
						}
						else if(GetLinguaCode(lang, lang_code)) {
							(temp_buf = r_idx_entry.P_Symb).Dot().Cat(lang_code);
							p_js->InsertString(temp_buf, tag_value);
						}
						else {
							; // invalid language
						}
					}
				}
			}
			else {
				if(GetExactly(r_idx_entry.Id, 0, tag_value)) {
					p_js->InsertString(r_idx_entry.P_Symb, tag_value);
				}
			}
		}
		if(json_tree_to_string(p_js, rResult))
			ok = 1;
		ZDELETE(p_js);
	}
	else
		ok = -1;
	return ok;
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

int   StyloQFace::SetDob(LDATE dt)
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

int   StyloQFace::SetVerifiable(bool v)
{
	SString & r_temp_buf = SLS.AcquireRvlStr();
	return Set(tagVerifiable, 0, v ? "true" : "false");
}

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

int StyloQFace::Get(int tag, int lang, SString & rResult) const
{
	rResult.Z();
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
				rResult = L.at_WithoutParent(pos).Txt;
				ok = (eff_tag >> 16);
				SETIFZ(ok, slangMeta);
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
					int sr = L.Search(eff_tag, &pos);
					assert(sr); // не может быть, что мы не нашли элемент - что-то мы сделали не так и надо проверять код!
					if(sr) {
						rResult = L.at_WithoutParent(pos).Txt;
						ok = (eff_tag >> 16);
						SETIFZ(ok, slangMeta);					
					}
					break;
				}
			}
		}
	}
	else {
		uint pos = 0;
		if(L.Search(tag, &pos)) {
			rResult = L.at_WithoutParent(pos).Txt;
			ok = 1;
		}
	}
	return ok;
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

bool  StyloQFace::IsVerifiable() const
{
	bool result = false;
	SString & r_temp_buf = SLS.AcquireRvlStr();
	if(Get(tagVerifiable, 0, r_temp_buf)) {
		if(r_temp_buf.IsEqiAscii("true"))
			result = true;
	}
	return result;
}

int StyloQFace::SetImage(const SImageBuffer * pImg)
{
	int    ok = 0;
	return ok;
}

int StyloQFace::GetImage(SImageBuffer * pImg) const
{
	return 0;
}

int StyloQFace::GetRepresentation(int lang, SString & rBuf) const
{
	rBuf.Z();
	SString temp_buf;
	if(Get(tagCommonName, lang, temp_buf)) {
		rBuf.Cat(temp_buf);
	}
	else {
		if(Get(tagSurName, lang, temp_buf))  {
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
class StyloQCommandList { // @construction
public:
	struct Item { 
		Item();
		enum {
			sqbcEmpty       = 0,
			sqbcRegister    = 1,
			sqbcLogin       = 2,
			sqbcPersonEvent = 3,
			sqbcReport      = 4
		};
		int32  Ver;                 //
		int32  BaseCmdId;           //
		int32  Flags;               //
		int32  ViewId;              //
		S_GUID Uuid;                //   
		int32  ObjTypeRestriction;  //
		int32  ObjGroupRestriction; //
		SString DbSymb;             // 
		SString Name;               // utf8
		SString ViewSymb;           //
		SString Description;        // utf8 Подробное описание команды
		SString Image;              // Ссылка на изображение, ассоциированное с командой
		SBuffer Param;              // Фильтр для ViewSymb и(или) ViewId 
		PPNamedFilt::ViewDefinition Vd;
	};
	StyloQCommandList();
	~StyloQCommandList();
	Item * CreateNewItem(uint * pIdx);
	uint   GetCount() const;
	Item * Get(uint idx);
	const  Item * GetC(uint idx) const;
	int    Set(uint idx, const Item * pItem);
	int    Store();
	int    Load();
	StyloQCommandList * CreateSubListByContext(PPObjID oid) const;
private:
	TSCollection <Item> L;
};

StyloQCommandList::Item::Item() : Ver(0), BaseCmdId(sqbcEmpty), Flags(0), ViewId(0), ObjTypeRestriction(0), ObjGroupRestriction(0)
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
	if(p_new_item) {
		p_new_item->Uuid.Generate();
	}
	return p_new_item;
}

uint StyloQCommandList::GetCount() const
{
	return L.getCount();
}

StyloQCommandList::Item * StyloQCommandList::Get(uint idx)
{
	return (idx < L.getCount()) ? L.at(idx) : 0;
}
	
const StyloQCommandList::Item * StyloQCommandList::GetC(uint idx) const
{
	return (idx < L.getCount()) ? L.at(idx) : 0;
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

int StyloQCommandList::Store()
{
	return 0;
}

int StyloQCommandList::Load()
{
	return 0;
}

StyloQCommandList * StyloQCommandList::CreateSubListByContext(PPObjID oid) const
{
	return 0;
}

int FASTCALL StyloQCore::StoragePacket::IsEqual(const StyloQCore::StoragePacket & rS) const
{
	#define FE(f) if(Rec.f != rS.Rec.f) return 0;
	FE(ID);
	FE(Kind);
	FE(CorrespondID);
	FE(SessExpiration);
	FE(LinkObjType);
	FE(LinkObjID);
	#undef FE
	if(memcmp(Rec.BI, rS.Rec.BI, sizeof(Rec.BI)) != 0)
		return 0;
	if(!Pool.IsEqual(rS.Pool))
		return 0;
	return 1;
}

StyloQCore::StyloQCore() : StyloQSecTbl()
{
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
	CATCHZOK
	return ok;
}

int StyloQCore::GetOwnPeerEntry(StoragePacket * pPack)
{
	int    ok = -1;
	StyloQSecTbl::Key2 k2;
	MEMSZERO(k2);
	k2.Kind = kNativeService;
	if(search(2, &k2, spGe) && data.Kind == kNativeService) {
		THROW(ReadCurrentPacket(pPack));
		ok = 1;		
	}
	CATCHZOK
	return ok;
}

int StyloQCore::SearchGlobalIdentEntry(const SBinaryChunk & rIdent, StyloQCore::StoragePacket * pPack)
{
	assert(rIdent.Len() > 0 && rIdent.Len() <= sizeof(StyloQSecTbl::Key1));
	int    ok = -1;
	StyloQSecTbl::Key1 k1;
	THROW(rIdent.Len() > 0 && rIdent.Len() <= sizeof(StyloQSecTbl::Key1));
	MEMSZERO(k1);
	memcpy(k1.BI, rIdent.PtrC(), rIdent.Len());
	if(search(1, &k1, spEq)) {
		THROW(ReadCurrentPacket(pPack));
		ok = 1;			
	}
	CATCHZOK
	return ok;
}

int StyloQCore::PutPeerEntry(PPID * pID, StoragePacket * pPack, int use_ta)
{
	int    ok = 1;
	const  PPID outer_id = pID ? *pID : 0;
	if(pPack) {
		assert(oneof5(pPack->Rec.Kind, kNativeService, kForeignService, kClient, kSession, kFace)); // @v11.1.8 kFace
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
				if(pPack->IsEqual(preserve_pack)) {
					ok = -1;
				}
				else {
					pPack->Rec.ID = outer_id;
					THROW_DB(rereadForUpdate(0, 0));
					copyBufFrom(&pPack->Rec);
					THROW_SL(pPack->Pool.Serialize(+1, cbuf, &sctx));
					THROW(writeLobData(VT, cbuf.GetBuf(0), cbuf.GetAvailableSize()));
					THROW_DB(updateRec());
					do_destroy_lob = true;
				}
			}
			else {
				THROW_DB(rereadForUpdate(0, 0));
				THROW_DB(deleteRec());
			}
		}
		else if(pPack) {
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
			THROW_DB(insertRec(0, pID));		
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
		obj_type_list.addzlist(PPOBJ_USR, PPOBJ_PERSON, PPOBJ_DBDIV, PPOBJ_CASHNODE, 0L);
		SetupObjListCombo(this, CTLSEL_STQCLIMATCH_OT, Data.Oid.Obj, &obj_type_list);
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
			clearEvent(event);
		}
		else if(event.isCbSelected(CTLSEL_STQCLIMATCH_OG)) {
			if(Data.Oid.Obj) {
				long obj_group = getCtrlLong(CTLSEL_STQCLIMATCH_OG);
				SetupPPObjCombo(this, CTLSEL_STQCLIMATCH_OBJ, Data.Oid.Obj, Data.Oid.Id, 0, reinterpret_cast<void *>(obj_group));
			}
			clearEvent(event);
		}
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
				if(P_Tbl->PutPeerEntry(&id, &pack, 1)) {
					// @v11.1.8 {
					if(pack.Rec.LinkObjType == PPOBJ_USR && pack.Rec.LinkObjID != 0) {
						DbProvider * p_dict = CurDict;
						if(p_dict) {
							SString db_symb;
							p_dict->GetDbSymb(db_symb);
							if(db_symb.NotEmpty()) {
								bool found = false;
								StringSet dbsymb_list;
								SString temp_buf;
								SString file_name;
								PPGetFilePath(PPPATH_WORKSPACE, "styloq-la", file_name);
								{
									SFile f(file_name, SFile::mRead);
									if(f.IsValid()) {
										while(f.ReadLine(temp_buf)) {
											temp_buf.Chomp().Strip();
											dbsymb_list.add(temp_buf);
											if(temp_buf.IsEqiAscii(db_symb))
												found = true;
										}
									}
								}
								if(!found) {
									SFile f(file_name, SFile::mWrite);
									if(f.IsValid()) {
										dbsymb_list.add(db_symb);
										dbsymb_list.sortAndUndup();
										for(uint ssp = 0; dbsymb_list.get(&ssp, temp_buf);) {
											temp_buf.Strip().CR();
											f.WriteLine(temp_buf);
										}
										f.Close();
									}
								}
							}
						}
					}
					// } @v11.1.8
					ok = 1;
				}
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
	public:
		StyloQFaceDialog() : TDialog(DLG_STQFACE)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			RVALUEPTR(Data, pData);
			SetupPage();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			ok = GetPage();
			if(ok) {
				ASSIGN_PTR(pData, Data);
			}
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
		}
		void SetupPage()
		{
			SString temp_buf;
			long lang = getCtrlLong(CTLSEL_STQFACE_LANG);
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
		}
		int GetPage()
		{
			int    ok = 1;
			SString temp_buf;
			long lang = getCtrlLong(CTLSEL_STQFACE_LANG);
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
			SBinaryChunk face_chunk;
			bool    ex_face_got = false;
			uint32  tag_id = 0;
			if(oneof2(pack.Rec.Kind, StyloQCore::kClient, StyloQCore::kForeignService))
				tag_id = SSecretTagPool::tagFace;
			else 
				tag_id = SSecretTagPool::tagSelfyFace;
			if(pack.Pool.Get(tag_id, &face_chunk)) {
				temp_buf.Z().CatN(static_cast<const char *>(face_chunk.PtrC()), face_chunk.Len());
				if(face_pack.FromJson(temp_buf))
					ex_face_got = true;
			}
			if(EditStyloQFace(face_pack) > 0) {
				THROW(face_pack.ToJson(temp_buf));
				face_chunk.Put(temp_buf, temp_buf.Len());
				pack.Pool.Put(tag_id, face_chunk);
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

PPStyloQInterchange::Invitation::Invitation() : ServerParamBase()
{
}

PPStyloQInterchange::Invitation::Invitation(const RunServerParam & rRsP) : ServerParamBase(rRsP)
{
}
		
PPStyloQInterchange::Invitation & PPStyloQInterchange::Invitation::Z()
{
	ServerParamBase::Z();
	//Capabilities = 0;
	//SvcIdent.Z();
	//AccessPoint.Z();
	CommandJson.Z();
	return *this;
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
		THROW_PP(H.DataLen == (src_size + sizeof(H)), PPERR_SQPROT_INVHDR);
		THROW_PP((H.Flags & hfEncrypted) || H.Padding == 0, PPERR_SQPROT_INVHDR);
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
			THROW(pack_dest.P.IsEqual(pack_src.P));
		}
	}
	CATCHZOK
	return ok;
}

PPStyloQInterchange::RoundTripBlock::RoundTripBlock() : P_Mqbc(0), P_MqbRpe(0), InnerSvcID(0), InnerSessID(0), InnerCliID(0), State(0)
{
}
		
PPStyloQInterchange::RoundTripBlock::RoundTripBlock(const void * pSvcIdent, size_t svcIdentLen, const char * pSvcAccsPoint) : P_Mqbc(0), P_MqbRpe(0), InnerSessID(0), InnerCliID(0), State(0)
{
	if(pSvcIdent && svcIdentLen)
		Other.Put(SSecretTagPool::tagSvcIdent, pSvcIdent, svcIdentLen);
	if(!isempty(pSvcAccsPoint)) {
		Other.Put(SSecretTagPool::tagSvcAccessPoint, pSvcAccsPoint, strlen(pSvcAccsPoint)+1);
	}
}
		
PPStyloQInterchange::RoundTripBlock::~RoundTripBlock()
{
	delete P_Mqbc;
	delete P_MqbRpe;
}

int    FASTCALL PPStyloQInterchange::Invitation::IsEqual(const PPStyloQInterchange::Invitation & rS) const
{
	return (Capabilities == rS.Capabilities && SvcIdent == rS.SvcIdent && AccessPoint == rS.AccessPoint && 
		CommandJson == rS.CommandJson);
}

int PPStyloQInterchange::AcceptInvitation(const char * pInvitationData, Invitation & rInv)
{
	int    ok = 1;
	rInv.Z();
	THROW_PP(!isempty(pInvitationData), PPERR_SQ_INVITATPARSEFAULT_EMPTY);
	{
		STempBuffer temp_binary(4096);
		StringSet ss;
		SString temp_buf(pInvitationData);
		temp_buf.Tokenize("&", ss);
		uint   tokn = 0;
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			tokn++;
			size_t actual_size = 0;
			if(tokn == 1) { // prefix and svcid
				const char first_symb = temp_buf.C(0);
				THROW_PP_S(first_symb == 'A', PPERR_SQ_INVITATPARSEFAULT, pInvitationData); // @error invalid invitation prefix
				temp_buf.ShiftLeft();
				THROW_SL(rInv.SvcIdent.FromMime64(temp_buf));
				//THROW(rInv.SvcIdent.Len() == 20); // @error invalid service public id
			}
			else if(tokn == 2) { // capabilities
				temp_buf.DecodeMime64(temp_binary, temp_binary.GetSize(), &actual_size);
				THROW_PP_S(actual_size == sizeof(rInv.Capabilities), PPERR_SQ_INVITATPARSEFAULT, pInvitationData); // @error invalid capabilities
				memcpy(&rInv.Capabilities, temp_binary, actual_size);
			}
			else if(tokn == 3) { // access point
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
			else if(tokn == 4) { // command
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

int PPStyloQInterchange::GeneratePublicIdent(const SSecretTagPool & rOwnPool, const SBinaryChunk & rSvcIdent, uint resultIdentTag, long flags, SSecretTagPool & rPool)
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

int PPStyloQInterchange::GetOwnPeerEntry(StyloQCore::StoragePacket * pPack)
{
	return P_T ? P_T->GetOwnPeerEntry(pPack) : 0;
}

int PPStyloQInterchange::SearchGlobalIdentEntry(const SBinaryChunk & rIdent, StyloQCore::StoragePacket * pPack)
{
	return P_T ? P_T->SearchGlobalIdentEntry(rIdent, pPack) : 0;
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
				THROW(P_T->SearchGlobalIdentEntry(cli_ident, &correspond_pack) > 0); // должна существовать запись соответствующего клиента
				correspond_type = StyloQCore::kClient;
			}
			else if(svc_ident.Len()) {
				THROW(P_T->SearchGlobalIdentEntry(svc_ident, &correspond_pack) > 0); // должна существовать запись соответствующего сервиса
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
	StyloQProtocol tp;
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
		SBuffer b;
		PPMqbClient::MessageProperties props;
		PPMqbClient::Envelope env;
		tp.StartWriting(PPSCMD_SQ_SRPREGISTER, StyloQProtocol::psubtypeForward);
		tp.P.Put(SSecretTagPool::tagClientIdent, cli_ident);
		/*
			@todo Здесь еще надо вставить лик клиента
		*/
		tp.P.Put(SSecretTagPool::tagSrpVerifier, __v);
		tp.P.Put(SSecretTagPool::tagSrpVerifierSalt, __s);
		assert(tp.P.IsValid());
		THROW(tp.FinishWriting(&sess_secret));
		THROW_PP(rB.P_Mqbc && rB.P_MqbRpe, PPERR_SQ_INVALIDMQBVALUES);
		const long cmto = PPMqbClient::SetupMessageTtl(__DefMqbConsumeTimeout, &props);
		THROW(rB.P_Mqbc->Publish(rB.P_MqbRpe->ExchangeName, rB.P_MqbRpe->RoutingKey, &props, tp.constptr(), tp.GetAvailableSize()));
		{
			const int cmr = rB.P_Mqbc->ConsumeMessage(env, cmto);
			THROW(cmr);
			THROW_PP_S(cmr > 0, PPERR_SQ_NOSVCRESPTOREGQUERY, cmto);
		}
		rB.P_Mqbc->Ack(env.DeliveryTag, 0);
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
		ok = 1;
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
		SBinaryChunk debug_cli_secret; // @debug do remove after debugging!
		SBinaryChunk other_face_chunk;
		SBinaryChunk selfy_face_chunk;
		SString temp_buf;
		THROW(rB.Other.Get(SSecretTagPool::tagClientIdent, &cli_ident_other_for_test));
		THROW_PP(rPack.P.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_UNDEFCLIID);
		assert(cli_ident_other_for_test == cli_ident);
		THROW(rPack.P.Get(SSecretTagPool::tagSrpVerifier, &srp_v));
		THROW(rPack.P.Get(SSecretTagPool::tagSrpVerifierSalt, &srp_s));
		rPack.P.Get(SSecretTagPool::tagFace, &other_face_chunk);
		rPack.P.Get(SSecretTagPool::tagSecret, &debug_cli_secret);
		if(P_T->SearchGlobalIdentEntry(cli_ident, &ex_storage_pack) > 0) {
			if(ex_storage_pack.Rec.Kind == StyloQCore::kClient) {
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
					if(debug_cli_secret.Len()) {
						new_storage_pack.Pool.Put(SSecretTagPool::tagSecret, debug_cli_secret);
					}
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
				// Этот случай - из области фантастики: фактически речь идет о неуникальности идентификатора cli_ident.
				ok = 0;
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
			if(debug_cli_secret.Len()) {
				new_storage_pack.Pool.Put(SSecretTagPool::tagSecret, debug_cli_secret);
			}
			// } @debug do remove after debugging!
			THROW(P_T->PutPeerEntry(&id, &new_storage_pack, 1));
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
	SBinaryChunk svc_acsp;
	SBinaryChunk svc_ident;
	PPMqbClient * p_mqbc = 0;
	THROW_PP(rB.Other.Get(SSecretTagPool::tagSvcAccessPoint, &svc_acsp), PPERR_SQ_UNDEFSVCACCSPOINT);
	THROW_PP(rB.Other.Get(SSecretTagPool::tagSvcIdent, &svc_ident), PPERR_SQ_UNDEFSVCID);
	{
		temp_buf.Z().CatN(static_cast<const char *>(svc_acsp.PtrC()), svc_acsp.Len());
		InetUrl url(temp_buf);
		SString host;
		THROW(url.GetComponent(InetUrl::cHost, 0, host));
		if(oneof2(url.GetProtocol(), InetUrl::protAMQP, InetUrl::protAMQPS)) {
			SBinaryChunk sess_pub_key;
			SBinaryChunk own_ident;
			SBinaryChunk other_sess_public;
			StyloQProtocol tp;
			PPMqbClient::RoutingParamEntry rpe; // маршрут до очереди, в которой сервис ждет новых клиентов
			PPMqbClient::RoutingParamEntry rpe_regular; // маршрут до очереди, в которой сервис будет с нами общаться
			PPMqbClient::MessageProperties props;
			PPMqbClient::Envelope env;
			PPMqbClient::InitParam mqip;
			THROW(PPMqbClient::SetupInitParam(mqip, "styloq", 0));
			int  fsks = FetchSessionKeys(rB.Sess, svc_ident);
			THROW(fsks);
			THROW(KexGenerateKeys(rB.Sess, 0, 0));
			THROW_PP(rB.Sess.Get(SSecretTagPool::tagClientIdent, &own_ident), PPERR_SQ_UNDEFCLIID); // !
			//if(fsks == fsksNewSession) 
			THROW(rB.Sess.Get(SSecretTagPool::tagSessionPublicKey, &sess_pub_key));
			tp.StartWriting(PPSCMD_SQ_ACQUAINTANCE, StyloQProtocol::psubtypeForward);
			tp.P.Put(SSecretTagPool::tagClientIdent, own_ident);
			tp.P.Put(SSecretTagPool::tagSessionPublicKey, sess_pub_key);
			THROW(rpe.SetupStyloQRpc(sess_pub_key, svc_ident, mqip.ConsumeParamList.CreateNewItem()));
			THROW(p_mqbc = PPMqbClient::CreateInstance(mqip));
			THROW(tp.FinishWriting(0));
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
	SBinaryChunk svc_acsp;
	SBinaryChunk sess_pub_key;
	SBinaryChunk other_sess_public;
	SBinaryChunk own_ident;
	PPMqbClient * p_mqbc = 0;
	THROW_PP(rB.Other.Get(SSecretTagPool::tagSvcAccessPoint, &svc_acsp), PPERR_SQ_UNDEFSVCACCSPOINT);
	THROW_PP(rB.Other.Get(SSecretTagPool::tagSvcIdent, &svc_ident), PPERR_SQ_UNDEFSVCID);
	THROW_PP(rB.Sess.Get(SSecretTagPool::tagSessionPublicKey, &sess_pub_key), PPERR_SQ_UNDEFSESSPUBKEY);
	THROW_PP(rB.Sess.Get(SSecretTagPool::tagSessionPublicKeyOther, &other_sess_public), PPERR_SQ_UNDEFOTHERPUBKEY);
	THROW_PP(rB.Sess.Get(SSecretTagPool::tagClientIdent, &own_ident), PPERR_SQ_UNDEFCLIID); // !
	{
		temp_buf.Z().CatN(static_cast<const char *>(svc_acsp.PtrC()), svc_acsp.Len());
		InetUrl url(temp_buf);
		SString host;
		THROW(url.GetComponent(InetUrl::cHost, 0, host));
		if(oneof2(url.GetProtocol(), InetUrl::protAMQP, InetUrl::protAMQPS)) {
			StyloQProtocol tp;
			PPMqbClient::RoutingParamEntry rpe; // маршрут до очереди, в которой сервис ждет новых клиентов
			PPMqbClient::RoutingParamEntry rpe_regular; // маршрут до очереди, в которой сервис будет с нами общаться
			PPMqbClient::MessageProperties props;
			PPMqbClient::Envelope env;
			PPMqbClient::InitParam mqip;
			THROW(PPMqbClient::SetupInitParam(mqip, "styloq", 0));
			tp.StartWriting(PPSCMD_SQ_SESSION, StyloQProtocol::psubtypeForward);
			tp.P.Put(SSecretTagPool::tagSessionPublicKey, sess_pub_key);
			tp.P.Put(SSecretTagPool::tagClientIdent, own_ident);
			THROW(rpe.SetupStyloQRpc(sess_pub_key, svc_ident, mqip.ConsumeParamList.CreateNewItem()));
			THROW(p_mqbc = PPMqbClient::CreateInstance(mqip));
			THROW(tp.FinishWriting(0));
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

int PPStyloQInterchange::Command_ClientRequest(RoundTripBlock & rB, const char * pCmdJson, SString & rReply)
{
	rReply.Z();
	int    ok = 1;
	SBinaryChunk sess_secret;
	THROW_PP(rB.Sess.Get(SSecretTagPool::tagSessionSecret, &sess_secret), PPERR_SQ_UNDEFSESSSECRET_INNER);
	THROW_PP(!isempty(pCmdJson), PPERR_SQ_UNDEFSVCCOMMAND);
	{
		PPMqbClient::MessageProperties props;
		PPMqbClient::Envelope env;
		StyloQProtocol tp;
		StyloQProtocol tp_reply;
		SBinaryChunk temp_bch;
		THROW(tp.StartWriting(PPSCMD_SQ_COMMAND, StyloQProtocol::psubtypeForward));
		temp_bch.Put(pCmdJson, sstrlen(pCmdJson));
		tp.P.Put(SSecretTagPool::tagRawData, temp_bch);
		THROW(tp.FinishWriting(&sess_secret));
		THROW_PP(rB.P_Mqbc && rB.P_MqbRpe, PPERR_SQ_INVALIDMQBVALUES);
		const long cmto = PPMqbClient::SetupMessageTtl(__DefMqbConsumeTimeout, &props);
		THROW(rB.P_Mqbc->Publish(rB.P_MqbRpe->ExchangeName, rB.P_MqbRpe->RoutingKey, &props, tp.constptr(), tp.GetAvailableSize()));
		{
			const int cmr = rB.P_Mqbc->ConsumeMessage(env, cmto);
			THROW(cmr);
			THROW_PP_S(cmr > 0, PPERR_SQ_NOSVCRESPTOCMD, cmto);
		}
		rB.P_Mqbc->Ack(env.DeliveryTag, 0);
		THROW(tp_reply.Read(env.Msg, &sess_secret));
		THROW(tp_reply.CheckRepError()); // Сервис вернул ошибку: можно уходить
		THROW(tp_reply.GetH().Type == PPSCMD_SQ_COMMAND && tp_reply.GetH().Flags & tp.hfAck);
		if(tp_reply.P.Get(SSecretTagPool::tagRawData, &temp_bch)) {
			rReply.CatN(static_cast<const char *>(temp_bch.PtrC()), temp_bch.Len());
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

int PPStyloQInterchange::Verification_ClientRequest(RoundTripBlock & rB)
{
	//
	// Это - инициирующий запрос и одновременно с авторизацией устанавливает ключи обмена.
	// Все фазы SRP-авторизации осуществляются без шифрования! 
	//
	int    ok = 1;
	SString temp_buf;
	StyloQCore::StoragePacket svc_pack;
	SBinaryChunk svc_ident;
	PPMqbClient * p_mqbc = 0;
	PPMqbClient::RoutingParamEntry rpe; // маршрут до очереди, в которой сервис ждет новых клиентов
	PPMqbClient::RoutingParamEntry * p_rpe = 0;
	PPMqbClient::RoutingParamEntry * p_rpe_init = 0; // if !0 then the first call to the service will be made through it, else through p_rpe
	bool   kex_generated = false;
	SBinaryChunk cli_ident;
	SBinaryChunk cli_secret;
	SBinaryChunk sess_pub_key;
	SBinaryChunk other_sess_public;
	SBinaryChunk _sess_secret;
	SBinaryChunk * p_sess_secret = 0;
	assert((rB.P_Mqbc && rB.P_MqbRpe) || (!rB.P_Mqbc && !rB.P_MqbRpe));
	if(rB.P_Mqbc && rB.P_MqbRpe) {
		p_mqbc = rB.P_Mqbc;
		p_rpe = rB.P_MqbRpe;
		p_rpe_init = p_rpe;
	}
	THROW(rB.Other.Get(SSecretTagPool::tagSvcIdent, &svc_ident));
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
	if(!p_mqbc) {
		SBinaryChunk svc_acsp;
		THROW_PP(rB.Other.Get(SSecretTagPool::tagSvcAccessPoint, &svc_acsp), PPERR_SQ_UNDEFSVCACCSPOINT);
		temp_buf.Z().CatN(static_cast<const char *>(svc_acsp.PtrC()), svc_acsp.Len());
		InetUrl url(temp_buf);
		SString host;
		THROW(url.GetComponent(InetUrl::cHost, 0, host));
		if(oneof2(url.GetProtocol(), InetUrl::protAMQP, InetUrl::protAMQPS)) {
			PPMqbClient::InitParam mqip;
			THROW(PPMqbClient::SetupInitParam(mqip, "styloq", 0));
			if(rpe.SetupStyloQRpc(sess_pub_key, svc_ident, mqip.ConsumeParamList.CreateNewItem())) {
				THROW(p_mqbc = PPMqbClient::CreateInstance(mqip));
				p_rpe_init = &rpe;
			}
		}
	}
	{
		int    srp_protocol_fault = 0; // Если !0 то возникла ошибка в верификации. Такие ошибки обрабатываются специальным образом.
		StyloQProtocol tp;
		SBinaryChunk temp_chunk;
		SBinaryChunk __m; // M
		SBinaryChunk srp_s;
		PPMqbClient::RoutingParamEntry rpe_regular; // маршрут до очереди, в которой сервис будет с нами общаться
		PPMqbClient::MessageProperties props;
		PPMqbClient::Envelope env;
		char * p_auth_ident = 0;
		temp_buf.EncodeMime64(cli_ident.PtrC(), cli_ident.Len());
		SlSRP::User usr(SlSRP::SRP_SHA1, SlSRP::SRP_NG_8192, temp_buf, cli_secret.PtrC(), cli_secret.Len(), /*n_hex*/0, /*g_hex*/0);
		{
			temp_chunk.Z(); // В этом блоке temp_chunk работает как фактор A
			usr.StartAuthentication(&p_auth_ident, temp_chunk);
			// User -> Host: (ident, __a) 
			tp.StartWriting(PPSCMD_SQ_SRPAUTH, 0);
			//tp.P.Put(SSecretTagPool::tag)
			if(kex_generated) {
				assert(sess_pub_key.Len()); // Мы его только что сгенерировали (see above)
				tp.P.Put(SSecretTagPool::tagSessionPublicKey, sess_pub_key);
			}
			tp.P.Put(SSecretTagPool::tagSrpA, temp_chunk);
			tp.P.Put(SSecretTagPool::tagClientIdent, cli_ident);
			assert(!p_sess_secret || p_sess_secret->Len());
			assert(p_sess_secret || tp.P.Get(SSecretTagPool::tagSessionPublicKey, 0));
			tp.FinishWriting(p_sess_secret);
		}
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
			usr.ProcessChallenge(srp_s, temp_chunk, __m);
		}
		THROW(rpe_regular.SetupStyloQRpc(sess_pub_key, other_sess_public, 0));
		if(!__m.Len()) { // @error User SRP-6a safety check violation
			srp_protocol_fault = 1;
			tp.StartWriting(PPSCMD_SQ_SRPAUTH_S2, StyloQProtocol::psubtypeForwardError);
			// Здесь текст и код ошибки нужны
		}
		else {
			// User -> Host: (bytes_M) 
			tp.StartWriting(PPSCMD_SQ_SRPAUTH_S2, 0);
			tp.P.Put(SSecretTagPool::tagSrpM, __m);
			tp.P.Put(SSecretTagPool::tagClientIdent, cli_ident);
		}
		THROW(tp.FinishWriting(0)); // несмотря на то, что у нас есть теперь ключ шифрования, этот roundtrip завершаем без шифровки пакетов
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
			THROW(temp_chunk.Len() == usr.GetSessionKeyLength());
			usr.VerifySession(static_cast<const uchar *>(temp_chunk.PtrC()));
			if(!usr.IsAuthenticated()) { // @error Server authentication failed
				srp_protocol_fault = 1;
				tp.StartWriting(PPSCMD_SQ_SRPAUTH_ACK, StyloQProtocol::psubtypeForwardError);
				// Здесь текст и код ошибки нужны
			}
			else {
				tp.StartWriting(PPSCMD_SQ_SRPAUTH_ACK, StyloQProtocol::psubtypeForward);
			}
			tp.FinishWriting(0); // несмотря на то, что у нас есть теперь ключ шифрования, этот roundtrip завершаем без шифровки пакетов
		}
		THROW(p_mqbc->Publish(rpe_regular.ExchangeName, rpe_regular.RoutingKey, &props, tp.constptr(), tp.GetAvailableSize()));
		// Это было завершающее сообщение. Если все OK то сервис ждет от нас команды, если нет, то можно уходить - свадьбы не будет
		THROW(!srp_protocol_fault);
		// Устанавливаем факторы RoundTrimBlock, которые нам понадобяться при последующих обменах
		if(SetRoundTripBlockReplyValues(rB, p_mqbc, rpe_regular)) {
			p_mqbc = 0;
		}
		{
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
			THROW(StoreSession(&sess_id, &sess_pack, 1));
		}
	}
	CATCHZOK
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
						Cat(temp_buf).Tab().Cat(spack.Rec.SessExpiration, DATF_ISO8601|DATF_CENTURY, 0);
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

int PPStyloQInterchange::SearchSession(const SBinaryChunk & rOtherPublic, StyloQCore::StoragePacket * pPack)
{
	int    ok = -1;
	if(P_T) {
		const binary160 bi = SlHash::Sha1(0, rOtherPublic.PtrC(), rOtherPublic.Len());
		StyloQSecTbl::Key1 k1;
		MEMSZERO(k1);
		assert(sizeof(bi) <= sizeof(k1.BI));
		memcpy(&k1.BI, &bi, sizeof(bi));
		if(P_T->search(1, &k1, spEq)) {
			THROW(P_T->ReadCurrentPacket(pPack));
			ok = 1;			
		}
		else if(BTRNFOUND) {
			// PPERR_SQ_SESSNFOUND  Транспортная сессия по публичному ключу не найдена
			ok = -1;
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::SetupPeerInstance(PPID * pID, int use_ta)
{
	int    ok = -1;
	BN_CTX * p_bn_ctx = 0;
	BIGNUM * p_rn = 0;
	if(P_T) {
		PPID   id = 0;
		SString temp_buf;
		StyloQCore::StoragePacket ex_pack;
		StyloQCore::StoragePacket new_pack;
		StyloQCore::StoragePacket * p_pack_to_export = 0;
		SBinaryChunk public_ident;
		if(P_T->GetOwnPeerEntry(&ex_pack) > 0) {
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
			THROW(P_T->PutPeerEntry(&id, &new_pack, 1));
			{
				//
				// Тестирование результатов
				// 
				SBinaryChunk c1;
				SBinaryChunk c2;
				const uint32 test_tag_list[] = { SSecretTagPool::tagPrimaryRN, SSecretTagPool::tagSvcIdent, SSecretTagPool::tagAG, SSecretTagPool::tagFPI };
				{
					StyloQCore::StoragePacket test_pack;
					if(P_T->GetPeerEntry(id, &test_pack) > 0) {
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
					if(P_T->GetOwnPeerEntry(&test_pack) > 0) {
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
	}
	CATCHZOK
	BN_free(p_rn);
	BN_CTX_free(p_bn_ctx);
	return ok;
}
//
// Registration {
//
//
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
	
int PPStyloQInterchange::MakeInvitation(const Invitation & rInv, SString & rInvitationData)
{
	int    ok = 1;
	SString temp_buf;
	rInvitationData.Z();
	// prefix SVCPID SVCCAP URL [SVCCMD [SVCCMDPARAM]]
	THROW_PP(rInv.SvcIdent.Len(), PPERR_SQ_UNDEFSVCID);
	THROW_PP(rInv.AccessPoint.NotEmpty(), PPERR_SQ_UNDEFSVCACCSPOINT);
	assert(rInv.CommandJson.NotEmpty());
	THROW_PP(rInv.CommandJson.NotEmpty(), PPERR_SQ_UNDEFSVCCOMMAND);
	rInvitationData.CatChar('A'); // prefix
	rInv.SvcIdent.Mime64(temp_buf);
	rInvitationData.Cat(temp_buf);
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

int PPStyloQInterchange::FetchSessionKeys(SSecretTagPool & rSessCtx, const SBinaryChunk & rForeignIdent)
{
	//const  int ec_curve_name_id = NID_X9_62_prime256v1;
	int    status = fsksError;
	if(P_T) {
		StyloQCore::StoragePacket pack;
		StyloQCore::StoragePacket corr_pack;
		const int sr = P_T->SearchGlobalIdentEntry(rForeignIdent, &pack);
		THROW(sr);
		if(sr > 0) {
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
						THROW(ExtractSessionFromPacket(corr_pack, rSessCtx));
						status = fsksSessionByCliId;
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
		SBinaryChunk svc_ident;
		SBinaryChunk temp_bch;
		StyloQCore::StoragePacket svc_pack;
		THROW(P_T->GetOwnPeerEntry(&rB.StP) > 0);
		THROW_PP(rB.Other.Get(SSecretTagPool::tagSvcIdent, &svc_ident), PPERR_SQ_UNDEFSVCID);
		if(P_T->SearchGlobalIdentEntry(svc_ident, &svc_pack) > 0) {
			THROW_PP(svc_pack.Rec.Kind == StyloQCore::kForeignService, PPERR_SQ_WRONGDBITEMKIND); // Что-то не так с базой данных или с программой: такого быть не должно!
			rB.InnerSvcID = svc_pack.Rec.ID;
			if(svc_pack.Rec.CorrespondID) {
				StyloQCore::StoragePacket corr_pack;
				if(P_T->GetPeerEntry(svc_pack.Rec.CorrespondID, &corr_pack) > 0) {
					THROW_PP(corr_pack.Rec.Kind == StyloQCore::kSession, PPERR_SQ_WRONGDBITEMKIND); // Что-то не так с базой данных или с программой: такого быть не должно!
					const LDATETIME _now = getcurdatetime_();
					if(!corr_pack.Rec.SessExpiration || cmp(corr_pack.Rec.SessExpiration, _now) > 0) {
						LongArray cid_list;
						svc_pack.Pool.Get(SSecretTagPool::tagClientIdent, &temp_bch);
						rB.Sess.Put(SSecretTagPool::tagClientIdent, temp_bch);
						cid_list.addzlist(SSecretTagPool::tagSessionPrivateKey, SSecretTagPool::tagSessionPublicKey, SSecretTagPool::tagSessionSecret, 
							SSecretTagPool::tagSvcIdent, SSecretTagPool::tagSessionPublicKeyOther, 0);
						THROW_SL(rB.Sess.CopyFrom(corr_pack.Pool, cid_list, true));
						rB.InnerSessID = corr_pack.Rec.ID;
					}
				}
			}
			else {
				THROW(GeneratePublicIdent(rB.StP.Pool, svc_ident, SSecretTagPool::tagClientIdent, gcisfMakeSecret, rB.Sess));
			}
		}				
		else {
			THROW(GeneratePublicIdent(rB.StP.Pool, svc_ident, SSecretTagPool::tagClientIdent, gcisfMakeSecret, rB.Sess));
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::KexServiceReply(SSecretTagPool & rSessCtx, SSecretTagPool & rCli, BIGNUM * pDebugPubX, BIGNUM * pDebugPubY)
{
	int    ok = 1;
	//SBinaryChunk cli_acsp;
	SBinaryChunk cli_ident;
	//rCli.Get(SSecretTagPool::tagSvcAccessPoint, &cli_acsp);
	THROW_PP(rCli.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_UNDEFCLIID);
	int  fsks = FetchSessionKeys(rSessCtx, cli_ident);
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
//
//static int _EcdhCryptModelling();

int Test_PPStyloQInterchange_Invitation()
{
	int    ok = 1;
	int    debug_flag = 0;
	PPID   own_peer_id = 0;
	SString temp_buf;
	PPStyloQInterchange ic;
	PPStyloQInterchange::RunServerParam rsparam;
	StyloQCore::StoragePacket sp;
	/*{
		int local_result = StyloQProtocol::Test();
		assert(local_result);
		local_result = _EcdhCryptModelling();
		assert(local_result);
	}*/
	int    spir = ic.SetupPeerInstance(&own_peer_id, 1);
	THROW(spir);
	THROW(ic.GetOwnPeerEntry(&sp) > 0);
	{
		sp.Pool.Get(SSecretTagPool::tagSvcIdent, &rsparam.SvcIdent);
		THROW(PPMqbClient::SetupInitParam(rsparam.MqbInitParam, "styloq", 0));
		{
			InetUrl url;
			url.SetProtocol(InetUrl::protAMQP);
			url.SetComponent(InetUrl::cHost, rsparam.MqbInitParam.Host);
			url.SetPort_(rsparam.MqbInitParam.Port);
			url.SetComponent(InetUrl::cUserName, rsparam.MqbInitParam.Auth);
			url.SetComponent(InetUrl::cPassword, rsparam.MqbInitParam.Secret);
			url.Composite(InetUrl::stAll, rsparam.AccessPoint);
		}
		{
			PPMqbClient::RoutingParamEntry rpe;
			if(rpe.SetupStyloQRpcListener(rsparam.SvcIdent)) {
				PPMqbClient::RoutingParamEntry * p_new_entry = rsparam.MqbInitParam.ConsumeParamList.CreateNewItem();
				ASSIGN_PTR(p_new_entry, rpe);
			}
		}
		if(ic.RunStyloQServer(rsparam)) {
			PPStyloQInterchange::Invitation inv(rsparam);
			{
				SJson * p_js = new SJson(SJson::tOBJECT);
				p_js->Insert("cmd", json_new_string("REGISTER"));
				json_tree_to_string(p_js, inv.CommandJson);
			}
			ic.ExecuteInvitationDialog(inv);
		}
	}
	//
	if(0) {
		PPIniFile ini_file;
		temp_buf.Z();
		ini_file.Get(PPINISECT_CONFIG, "styloqtestside", temp_buf);
		ic.Dump(); // @debug
		if(temp_buf.IsEqiAscii("server")) {
			SBinaryChunk _ident;
			sp.Pool.Get(SSecretTagPool::tagSvcIdent, &_ident);
			THROW(PPMqbClient::SetupInitParam(rsparam.MqbInitParam, "styloq", 0));
			{
				PPMqbClient::RoutingParamEntry rpe;
				if(rpe.SetupStyloQRpcListener(_ident)) {
					PPMqbClient::RoutingParamEntry * p_new_entry = rsparam.MqbInitParam.ConsumeParamList.CreateNewItem();
					ASSIGN_PTR(p_new_entry, rpe);
				}
			}
			ic.RunStyloQServer(rsparam);
		}
		else if(temp_buf.IsEqiAscii("client")) {
			const char * p_svc_ident_mime = "Lkekoviu1J2nw1O7/R66LYvpAtA="; // pft
			SBinaryChunk svc_ident;
			if(svc_ident.FromMime64(p_svc_ident_mime)) {
				//SSecretTagPool svc_pool;
				//SSecretTagPool sess_pool;
				PPStyloQInterchange::RoundTripBlock rtb(svc_ident.PtrC(), svc_ident.Len(), "amqp://213.166.70.221");
				if(ic.InitRoundTripBlock(rtb)) {
					//svc_pool.Put(SSecretTagPool::tagSvcIdent, svc_ident);
					//const char * p_accsp = "amqp://213.166.70.221";
					//svc_pool.Put(SSecretTagPool::tagSvcAccessPoint, p_accsp, strlen(p_accsp)+1);
					/* (рабочий фрагмент, но нам надо отладить верификацию) */
					if(rtb.InnerSessID) {
						if(ic.Session_ClientRequest(rtb) > 0) {
							SString cmd_buf;
							SString cmd_reply_buf;
							SJson * p_query = new SJson(SJson::tOBJECT);
							SJson * p_reply = 0;
							p_query->Insert("cmd", json_new_string("ECHO"));
							p_query->Insert("arg1", json_new_string("arg1-value"));
							p_query->Insert("arg2", json_new_string("arg2-value"));
							json_tree_to_string(p_query, cmd_buf);
							if(ic.Command_ClientRequest(rtb, cmd_buf, cmd_reply_buf)) {
								SString svc_result;
								int  reply_is_ok = 0;
								if(json_parse_document(&p_reply, cmd_reply_buf.cptr()) == JSON_OK) {
									for(SJson * p_cur = p_reply; p_cur; p_cur = p_cur->P_Next) {
										if(p_cur->Type == SJson::tOBJECT) {								
											for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
												if(p_obj->Text.IsEqiAscii("reply")) {
													if(p_obj->P_Child->Text == "ECHO")
														reply_is_ok++;
												}
												else if(p_obj->Text.IsEqiAscii("arg1")) {
													if(p_obj->P_Child->Text == "arg1-value")
														reply_is_ok++;
												}
												else if(p_obj->Text.IsEqiAscii("arg2")) {
													if(p_obj->P_Child->Text == "arg2-value")
														reply_is_ok++;
												}
												else if(p_obj->Text.IsEqiAscii("result")) {
													svc_result = p_obj->P_Child->Text;
												}
											}
										}
									}
								}
								debug_flag = 1;
							}
							delete p_query;
							delete p_reply;
						}
					}
					else if(rtb.InnerSvcID) {
						if(ic.Verification_ClientRequest(rtb) > 0) {
							debug_flag = 1;
						}
					}
					else {
						if(ic.KexClientRequest(rtb) > 0) {
							ic.Registration_ClientRequest(rtb);
						}
					}
				}
			}
		}
	}
#if 0 // @model {
	//__KeyGenerationEc();
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
	CATCHZOK
	return ok;
}

class StyloQServerSession : public PPThread {
	PPStyloQInterchange * P_Ic;
public:
	StyloQServerSession(const DbLoginBlock & rLB, const PPStyloQInterchange::RunServerParam & rP, const StyloQProtocol & rTPack) : 
		PPThread(kStyloQSession, 0, 0), LB(rLB), StartUp_Param(rP), TPack(rTPack), P_Ic(0)
	{
		StartUp_Param.MqbInitParam.ConsumeParamList.freeAll();
	}
	~StyloQServerSession()
	{
		delete P_Ic;
	}
	int SrpAuth()
	{
		int    ok = 1;
		int    debug_mark = 0;
		PPMqbClient * p_mqbc = 0;
		PPMqbClient::MessageProperties props;
		PPMqbClient::RoutingParamEntry rpe;
		StyloQProtocol tp;
		StyloQProtocol reply_tp;
		PPMqbClient::Envelope env;
		PPID   id = 0; // Внутренний идентификатор записи клиента в DBMS
		SString temp_buf;
		SString user_name_text;
		StyloQCore::StoragePacket storage_pack;
		SBinaryChunk other_public;
		SBinaryChunk cli_ident;
		SBinaryChunk my_public;
		SBinaryChunk srp_s;
		SBinaryChunk srp_v;
		SBinaryChunk __a; // A
		SBinaryChunk __b; // B
		SBinaryChunk __m; // M
		SBinaryChunk temp_bc;
		SBinaryChunk debug_cli_secret; // @debug do remove after debugging!
		THROW(TPack.P.Get(SSecretTagPool::tagSessionPublicKey, &other_public));
		THROW_PP(TPack.P.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_UNDEFCLIID);
		THROW(Login()); // @error (Inner service error: unable to login
		THROW(SETIFZ(P_Ic, new PPStyloQInterchange));
		B.Other.Put(SSecretTagPool::tagClientIdent, cli_ident);
		THROW(P_Ic->KexServiceReply(B.Sess, TPack.P, 0, 0));
		B.Sess.Get(SSecretTagPool::tagSessionPublicKey, &my_public);
		assert(my_public.Len());
		THROW(TPack.P.Get(SSecretTagPool::tagSrpA, &__a));
		THROW_PP(TPack.P.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_UNDEFCLIID); // @redundant
		THROW(P_Ic->SearchGlobalIdentEntry(cli_ident, &storage_pack) > 0);
		THROW(storage_pack.Rec.Kind == StyloQCore::kClient);
		THROW(storage_pack.Pool.Get(SSecretTagPool::tagClientIdent, &temp_bc) && temp_bc == cli_ident); // @error (I don't know you)
		THROW(storage_pack.Pool.Get(SSecretTagPool::tagSrpVerifier, &srp_v)); // @error (I havn't got your registration data)
		THROW(storage_pack.Pool.Get(SSecretTagPool::tagSrpVerifierSalt, &srp_s)); // @error (I havn't got your registration data)
		storage_pack.Pool.Get(SSecretTagPool::tagSecret, &debug_cli_secret); // @debug do remove after debugging!
		user_name_text.EncodeMime64(cli_ident.PtrC(), cli_ident.Len());
		{
			SlSRP::Verifier srp_ver(SlSRP::SRP_SHA1, SlSRP::SRP_NG_8192, user_name_text, srp_s, srp_v, __a, __b, /*n_hex*/0, /*g_hex*/0);
			const uchar * p_bytes_HAMK = 0;
			THROW(__b.Len()); // @error (Verifier SRP-6a safety check violated)
			// Host -> User: (bytes_s, bytes_B) 
			reply_tp.StartWriting(PPSCMD_SQ_SRPAUTH, StyloQProtocol::psubtypeReplyOk);
			reply_tp.P.Put(SSecretTagPool::tagSrpB, __b);
			reply_tp.P.Put(SSecretTagPool::tagSrpVerifierSalt, srp_s);
			//}
			reply_tp.P.Put(SSecretTagPool::tagSessionPublicKey, my_public);
			THROW(reply_tp.FinishWriting(0));
			props.Z();
			assert(StartUp_Param.MqbInitParam.ConsumeParamList.getCount() == 0);
			THROW(rpe.SetupStyloQRpc(my_public, other_public, StartUp_Param.MqbInitParam.ConsumeParamList.CreateNewItem()));
			THROW(p_mqbc = PPMqbClient::CreateInstance(StartUp_Param.MqbInitParam));
			{
				const long cmto = PPMqbClient::SetupMessageTtl(__DefMqbConsumeTimeout, &props);
				THROW(p_mqbc->Publish(rpe.ExchangeName, rpe.RoutingKey, &props, reply_tp.constptr(), reply_tp.GetAvailableSize()));
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
			}
			THROW(tp.Read(env.Msg, 0));
			THROW(tp.CheckRepError());
			THROW(tp.GetH().Type == PPSCMD_SQ_SRPAUTH_S2);
			THROW(tp.P.Get(SSecretTagPool::tagSrpM, &__m));
			// @debug {
			/*
			bool debug_is_ok = false;
			{
				char * p_auth_username = 0;
				SBinaryChunk debug__a; // A
				SBinaryChunk debug__b; // B
				SBinaryChunk debug__s(srp_s);
				SBinaryChunk debug__v(srp_v);
				SBinaryChunk debug__m; // m
				const uchar * p_debug_bytes_HAMK = 0;
				SlSRP::User debug_usr(SlSRP::SRP_SHA1, SlSRP::SRP_NG_8192, user_name_text, debug_cli_secret.PtrC(), debug_cli_secret.Len(), 0, 0);
				debug_usr.StartAuthentication(&p_auth_username, debug__a);
				// User -> Host: (username, bytes_A) 
				SlSRP::Verifier ver(SlSRP::SRP_SHA1, SlSRP::SRP_NG_8192, user_name_text, debug__s, debug__v, debug__a, debug__b, 0, 0);
				if(debug__b.Len()) {
					// Host -> User: (bytes_s, bytes_B) 
					debug_usr.ProcessChallenge(debug__s, debug__b, debug__m);
					if(debug__m.Len()) {
						// User -> Host: (bytes_M) 
						ver.VerifySession(static_cast<const uchar *>(debug__m.PtrC()), &p_debug_bytes_HAMK);
						if(p_debug_bytes_HAMK) {
							// Host -> User: (HAMK) 
							debug_usr.VerifySession(p_debug_bytes_HAMK);
							if(debug_usr.IsAuthenticated())
								debug_is_ok = true;
						}
					}
				}
			}
			*/
			// } @debug 
			srp_ver.VerifySession(static_cast<const uchar *>(__m.PtrC()), &p_bytes_HAMK);
			THROW(p_bytes_HAMK); // @error User authentication failed!
			{
				// Host -> User: (HAMK) 
				const SBinaryChunk srp_hamk(p_bytes_HAMK, srp_ver.GetSessionKeyLength());
				reply_tp.StartWriting(PPSCMD_SQ_SRPAUTH_S2, StyloQProtocol::psubtypeReplyOk);
				reply_tp.P.Put(SSecretTagPool::tagSrpHAMK, srp_hamk);
				reply_tp.FinishWriting(0);
				{
					const long cmto = PPMqbClient::SetupMessageTtl(__DefMqbConsumeTimeout, &props);
					THROW(p_mqbc->Publish(rpe.ExchangeName, rpe.RoutingKey, &props, reply_tp.constptr(), reply_tp.GetAvailableSize()));
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
					PPID   sess_id = 0;
					SBinaryChunk temp_chunk;
					SBinaryChunk _sess_secret;
					StyloQCore::StoragePacket sess_pack;
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
					sess_pack.Pool.Put(SSecretTagPool::tagClientIdent, cli_ident);
					THROW(P_Ic->StoreSession(&sess_id, &sess_pack, 1));
				}
				if(P_Ic->SetRoundTripBlockReplyValues(B, p_mqbc, rpe)) {
					p_mqbc = 0;
				}
			}
		}
		CATCHZOK
		delete p_mqbc;
		return ok;
	}
	int Acquaintance()
	{
		int    ok = 1;
		PPMqbClient * p_mqbc = 0;
		SBinaryChunk other_public;
		SBinaryChunk cli_ident;
		SBinaryChunk my_public;
		SBinaryChunk sess_secret; // @debug
		//SString debug_ecp_x;
		//SString debug_ecp_y;
		StyloQProtocol reply_tp;
		PPMqbClient::RoutingParamEntry rpe;
		PPMqbClient::MessageProperties props;
		//BIGNUM * p_debug_pub_ecp_x = BN_new(); // @debug
		//BIGNUM * p_debug_pub_ecp_y = BN_new(); // @debug
		THROW_PP(TPack.P.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_UNDEFCLIID);
		assert(cli_ident.Len());
		THROW_PP(TPack.P.Get(SSecretTagPool::tagSessionPublicKey, &other_public), PPERR_SQ_UNDEFSESSPUBKEY);
		assert(other_public.Len());
		THROW(Login());
		assert(P_Ic == 0);
		THROW_SL(P_Ic = new PPStyloQInterchange);
		B.Other.Put(SSecretTagPool::tagClientIdent, cli_ident);
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
			// @debug {
			/*{
				{
					char * p_dbuf = BN_bn2dec(p_debug_pub_ecp_x);
					debug_ecp_x = p_dbuf;
					OPENSSL_free(p_dbuf);
				}
				{
					char * p_dbuf = BN_bn2dec(p_debug_pub_ecp_y);
					debug_ecp_y = p_dbuf;
					OPENSSL_free(p_dbuf);
				}
				B.Sess.Get(SSecretTagPool::tagSessionSecret, &sess_secret); 
			}*/
			// } @debug
			assert(StartUp_Param.MqbInitParam.ConsumeParamList.getCount() == 0);
			THROW(rpe.SetupStyloQRpc(my_public, other_public, StartUp_Param.MqbInitParam.ConsumeParamList.CreateNewItem()));
			THROW(reply_tp.FinishWriting(0));
			THROW(p_mqbc = PPMqbClient::CreateInstance(StartUp_Param.MqbInitParam));
			THROW(p_mqbc->Publish(rpe.ExchangeName, rpe.RoutingKey, &props, reply_tp.constptr(), reply_tp.GetAvailableSize()));
			if(P_Ic->SetRoundTripBlockReplyValues(B, p_mqbc, rpe)) {
				p_mqbc = 0;
			}
		}
		CATCHZOK
		//BN_free(p_debug_pub_ecp_x); // @debug
		//BN_free(p_debug_pub_ecp_y); // @debug
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
		SBinaryChunk cli_ident;
		SBinaryChunk sess_secret;
		SBinaryChunk * p_sess_secret = 0;
		StyloQProtocol reply_tp;
		PPMqbClient::RoutingParamEntry rpe;
		PPMqbClient::MessageProperties props;
		StyloQCore::StoragePacket pack;
		THROW_PP(TPack.P.Get(SSecretTagPool::tagSessionPublicKey, &other_public), PPERR_SQ_INPHASNTSESSPUBKEY); // PPERR_SQ_INPHASNTSESSPUBKEY Входящий запрос сессии не содержит публичного ключа
		THROW_PP(TPack.P.Get(SSecretTagPool::tagClientIdent, &cli_ident), PPERR_SQ_INPHASNTCLIIDENT);           // PPERR_SQ_INPHASNTCLIIDENT   Входящий запрос сессии не содержит идентификатора контрагента
		assert(other_public.Len());
		assert(cli_ident.Len());
		THROW(Login());
		assert(P_Ic == 0);
		THROW_SL(P_Ic = new PPStyloQInterchange);
		THROW(P_Ic->SearchSession(other_public, &pack) > 0); 
		THROW_PP(pack.Pool.Get(SSecretTagPool::tagSessionPublicKey, &my_public), PPERR_SQ_INRSESSHASNTPUBKEY); // PPERR_SQ_INRSESSHASNTPUBKEY      Сохраненная сессия не содержит публичного ключа
		THROW_PP(pack.Pool.Get(SSecretTagPool::tagSessionPublicKeyOther, &temp_chunk), PPERR_SQ_INRSESSHASNTOTHERPUBKEY); // PPERR_SQ_INRSESSHASNTOTHERPUBKEY Сохраненная сессия не содержит публичного ключа контрагента
		THROW_PP(temp_chunk == other_public, PPERR_SQ_INRSESPUBKEYNEQTOOTR);         // PPERR_SQ_INRSESPUBKEYNEQTOOTR   Публичный ключ сохраненной сессии не равен полученному от контрагента
		THROW_PP(pack.Pool.Get(SSecretTagPool::tagClientIdent, &temp_chunk), PPERR_SQ_INRSESSHASNTCLIIDENT); // PPERR_SQ_INRSESSHASNTCLIIDENT   Сохраненная сессия не содержит идентификатора контрагента
		THROW_PP(temp_chunk == cli_ident, PPERR_SQ_INRSESCLIIDENTNEQTOOTR);          // PPERR_SQ_INRSESCLIIDENTNEQTOOTR Идентификатора контрагента сохраненной сессии не равен полученному от контрагента
		THROW_PP(pack.Pool.Get(SSecretTagPool::tagSessionSecret, &sess_secret), PPERR_SQ_INRSESSHASNTSECRET); // PPERR_SQ_INRSESSHASNTSECRET     Сохраненная сессия не содержит секрета 
		B.Sess.Put(SSecretTagPool::tagSessionSecret, sess_secret);
		p_sess_secret = &sess_secret;
		B.Other.Put(SSecretTagPool::tagClientIdent, cli_ident);
		THROW(rpe.SetupStyloQRpc(my_public, other_public, StartUp_Param.MqbInitParam.ConsumeParamList.CreateNewItem()));
		reply_tp.StartWriting(PPSCMD_SQ_SESSION, StyloQProtocol::psubtypeReplyOk);
		THROW(reply_tp.FinishWriting(0));
		THROW(p_mqbc = PPMqbClient::CreateInstance(StartUp_Param.MqbInitParam));
		THROW(p_mqbc->Publish(rpe.ExchangeName, rpe.RoutingKey, &props, reply_tp.constptr(), reply_tp.GetAvailableSize()));
		if(P_Ic->SetRoundTripBlockReplyValues(B, p_mqbc, rpe)) {
			p_mqbc = 0;
		}
		CATCHZOK
		ZDELETE(p_mqbc);
		return ok;
	}
	int    ProcessCommand()
	{
		int    ok = 1;
		return ok;
	}
	virtual void Run()
	{
		//PPMqbClient * p_mqbc = 0;
		bool do_process_loop = false;
		SString temp_buf;
		StyloQProtocol tp;
		StyloQProtocol reply_tp;
		PPMqbClient::MessageProperties props;
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
					const long live_silence_period = 5 * 60 * 1000;
					clock_t clk_start = clock();
					while(do_process_loop) {
						assert(B.P_Mqbc && B.P_MqbRpe);
						if(B.P_Mqbc->ConsumeMessage(env, 200) > 0) {
							clk_start = clock();
							B.P_Mqbc->Ack(env.DeliveryTag, 0);
							if(tp.Read(env.Msg, &sess_secret)) {
								if(tp.GetH().Type == PPSCMD_SQ_SRPREGISTER) {
									int32 reply_status = 0;
									SString reply_status_text;
									reply_tp.Z();
									if(P_Ic->Registration_ServiceReply(B, tp)) {
										reply_tp.StartWriting(PPSCMD_SQ_SRPREGISTER, StyloQProtocol::psubtypeReplyOk);
										reply_status_text = "Wellcome!";
									}
									else {
										reply_tp.StartWriting(PPSCMD_SQ_SRPREGISTER, StyloQProtocol::psubtypeReplyError);
										reply_status_text = "Something went wrong";
									}
									{
										// @v11.1.8 {
										{
											SBinaryChunk selfy_chunk;
											if(B.StP.Pool.Get(SSecretTagPool::tagSelfyFace, &selfy_chunk) && selfy_chunk.Len()) {
												StyloQFace face_pack;
												temp_buf.Z().CatN(static_cast<const char *>(selfy_chunk.PtrC()), selfy_chunk.Len());
												if(face_pack.FromJson(temp_buf))
													reply_tp.P.Put(SSecretTagPool::tagFace, selfy_chunk);
											}
										}
										// } @v11.1.8 
										reply_tp.P.Put(SSecretTagPool::tagReplyStatus, &reply_status, sizeof(reply_status));
										if(reply_status_text.NotEmpty()) 
											reply_tp.P.Put(SSecretTagPool::tagReplyStatusText, reply_status_text.cptr(), reply_status_text.Len()+1);
									}
									reply_tp.FinishWriting(&sess_secret);
									props.Z();
									int pr = B.P_Mqbc->Publish(B.P_MqbRpe->ExchangeName, B.P_MqbRpe->RoutingKey, &props, reply_tp.constptr(), reply_tp.GetAvailableSize());
								}
								else if(tp.GetH().Type == PPSCMD_SQ_COMMAND) {
									SBinaryChunk cmd_bch;
									SString cmd_buf;
									if(tp.P.Get(SSecretTagPool::tagRawData, &cmd_bch)) {
										SJson * p_js_cmd = 0;
										SJson * p_js_reply = 0;
										SString command;
										cmd_buf.CatN(static_cast<const char *>(cmd_bch.PtrC()), cmd_bch.Len());
										p_js_reply = new SJson(SJson::tOBJECT);
										if(json_parse_document(&p_js_cmd, cmd_buf.cptr()) == JSON_OK) {
											for(SJson * p_cur = p_js_cmd; p_cur; p_cur = p_cur->P_Next) {
												if(p_cur->Type == SJson::tOBJECT) {								
													for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
														if(p_obj->Text.IsEqiAscii("cmd")) {
															command = p_obj->P_Child->Text;
															//p_js_reply->InsertString("reply", command);
														}
														else {
															if(p_obj->Text.NotEmpty() && p_obj->P_Child && p_obj->P_Child->Text.NotEmpty()) {
																//p_js_reply->InsertString(p_obj->Text, p_obj->P_Child->Text);
															}
														}
													}
												}
											}
											if(command.NotEmptyS()) {
												if(command.IsEqiAscii("register")) {
													// Регистрация //
													p_js_reply->InsertString("reply", "hello");
												}
												else if(command.IsEqiAscii("quit") || command.IsEqiAscii("bye")) {
													// Завершение сеанса
													p_js_reply->InsertString("reply", "bye");
												}
												else if(command.IsEqiAscii("dtlogin")) {
													// Десктоп-логин (экспериментальная функция)
													//if(B.)
													p_js_reply->InsertString("reply", "bad");
												}
												else {
													// ProcessCommand
												}
											}
											p_js_reply->InsertString("result", "OK");
										}
										json_tree_to_string(p_js_reply, cmd_buf);
										ZDELETE(p_js_cmd);
										ZDELETE(p_js_reply);
									}
									else 
										cmd_buf = "Error";
									reply_tp.Z();
									reply_tp.StartWriting(PPSCMD_SQ_COMMAND, StyloQProtocol::psubtypeReplyOk);
									cmd_bch.Put(cmd_buf.cptr(), cmd_buf.Len());
									reply_tp.P.Put(SSecretTagPool::tagRawData, cmd_bch);
									reply_tp.FinishWriting(&sess_secret);
									props.Z();
									int pr = B.P_Mqbc->Publish(B.P_MqbRpe->ExchangeName, B.P_MqbRpe->RoutingKey, &props, reply_tp.constptr(), reply_tp.GetAvailableSize());
									// @debug {
									if(pr) {
										props.Z();
									}
									// } @debug
								}
							}
						}
						else {
							const clock_t silence_period = (clock() - clk_start);
							if(silence_period >= live_silence_period) {
								(temp_buf = "Service Stylo-Q roundtrip is finished after").Space().Cat(silence_period).Cat("ms of silence");
								PPLogMessage(PPFILNAM_INFO_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_COMP);
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
	DbLoginBlock LB;
	PPStyloQInterchange::RunServerParam StartUp_Param;
	StyloQProtocol TPack;
	PPStyloQInterchange::RoundTripBlock B;
};

class StyloQServer : public PPThread {
	PPStyloQInterchange::RunServerParam P;
	DbLoginBlock LB;
public:
	StyloQServer(const DbLoginBlock & rLB, const PPStyloQInterchange::RunServerParam & rP) : PPThread(kStyloQServer, 0, 0), P(rP), LB(rLB)
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
		PPMqbClient * p_mqb_cli = PPMqbClient::CreateInstance(P.MqbInitParam); // @v11.0.9
		if(p_mqb_cli) {
			PPMqbClient::Envelope mqb_envelop;
			const long __cycle_hs = (p_mqb_cli ? 37 : 293); // Период таймера в сотых долях секунды (37)
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
						{
							if(pt_purge.IsTime()) {
								;
							}
							if(p_mqb_cli && pt_mqc.IsTime()) {
								while(p_mqb_cli->ConsumeMessage(mqb_envelop, 200) > 0) {
									p_mqb_cli->Ack(mqb_envelop.DeliveryTag, 0);
									if(tpack.Read(mqb_envelop.Msg, 0)) {
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

int PPStyloQInterchange::RunStyloQServer(RunServerParam & rP, const DbLoginBlock & rDlb)
{
	int    ok = 1;
	PPID   own_peer_id = 0;
	SString temp_buf;
	LongArray ex_list;
	DS.GetThreadListByKind(PPThread::kStyloQServer, ex_list);
	if(!ex_list.getCount()) {
		StyloQServer * p_srv = new StyloQServer(rDlb, rP);
		CALLPTRMEMB(p_srv, Start(0));
	}
	else
		ok = -1;
	return ok;
}

int PPStyloQInterchange::RunStyloQServer(RunServerParam & rP)
{
	int    ok = 1;
	PPID   own_peer_id = 0;
	DbProvider * p_dict = CurDict;
	if(p_dict) {
		SString temp_buf;
		LongArray ex_list;
		DS.GetThreadListByKind(PPThread::kStyloQServer, ex_list);
		if(!ex_list.getCount()) {
			DbLoginBlock dlb;
			PPIniFile ini_file;
			PPDbEntrySet2 dbes;
			int    db_id = 0;
			p_dict->GetDbSymb(temp_buf);
			THROW(ini_file.IsValid());
			THROW(dbes.ReadFromProfile(&ini_file, 0));
			THROW_SL(db_id = dbes.GetBySymb(temp_buf, &dlb));
			{
				StyloQServer * p_srv = new StyloQServer(dlb, rP);
				CALLPTRMEMB(p_srv, Start(0));
			}
		}
		else
			ok = -1;
	}
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
	//unsigned char * get_secret(EC_KEY *key, const EC_POINT *peer_pub_key, size_t *secret_len)
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

int PPStyloQInterchange::ExecuteInvitationDialog(Invitation & rData)
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
	//Invitation inv;
	StyloQCore::StoragePacket sp;
	SString inv_code;
	PPID   own_peer_id = 0;
	int    spir = SetupPeerInstance(&own_peer_id, 1);
	if(spir) {
		//SBinaryChunk _ident;
		//PPMqbClient::InitParam mqb_init_param;
		//InetUrl url;
		//THROW(GetOwnPeerEntry(&sp) > 0);
		//sp.Pool.Get(SSecretTagPool::tagSvcIdent, &_ident);
		//THROW(PPMqbClient::SetupInitParam(mqb_init_param, "styloq", 0));
		//url.SetProtocol(InetUrl::protAMQP);
		//url.SetComponent(InetUrl::cHost, mqb_init_param.Host);
		//url.SetPort_(mqb_init_param.Port);
		//url.SetComponent(InetUrl::cUserName, mqb_init_param.Auth);
		//url.SetComponent(InetUrl::cPassword, mqb_init_param.Secret);
		//url.Composite(InetUrl::stAll, inv.AccessPoint);
		//THROW(sp.Pool.Get(SSecretTagPool::tagSvcIdent, &inv.SvcIdent));
		THROW(MakeInvitation(rData, inv_code));
		StQInvDialog * dlg = new StQInvDialog(inv_code);
		if(CheckDialogPtr(&dlg)) {
			ExecViewAndDestroy(dlg);
		}
	}
	CATCHZOKPPERR
	return ok;
}
