// MNCSHAPI.H
// (C) Harry V. Nikolayev, Micro Lana Ltd, 1995, 1996.
// Modified by A.Sobolev 1997
//
#ifndef __MNCSHAPI_H
#define __MNCSHAPI_H

#ifndef __SLIB_H
#	include <slib.h>
#endif

typedef struct {
	ulong l;                   /* младшее слово */
	ulong h;                   /* старшее слово */
} LONG64;
/*
	Таблица запроса товара/продажной транзакции
*/
typedef struct {
	short GI_bank;             /* # кассы */
	uchar GI_user;             /* # кассира */
	LONG64 GI_code;            /* код товара */
	/* == 0 - свободная цена; != 0 - номенклатуpный # */
	LONG64 GI_weight;          /* вес товара */
	/* > 0 - пpодажа/пpобивка в секцию; < 0 - возвpат/стоpно по секции */
	LONG64 GI_price;           /* цена товара (всегда > 0) */
	LONG64 GI_rate;            /* курс валюты на момент продажи */
	uchar GI_sign;             /* хар-ка товара */
	uchar GI_division;         /* отдел */
	uchar GI_taxes;            /* # НДС */
	uchar GI_group;            /* группа товара */
	uchar GI_buflen;           /* длина буфера имени */
	uchar GI_name[52];         /* наименование товара */
} GOODSINFO;

typedef struct {
	short RB_bank;             /* # кассы */
	uchar RB_user;             /* # кассира */
	ulong RB_data;             /* asciz стpока баpкода */
} RAWBARCODE;
/*
	Таблица pаботы со скидками и надбавками
*/
typedef struct {
	short RI_bank;             /* # кассы */
	uchar RI_user;             /* # кассира */
	uchar RI_subtotal;         /* пpизнак <ПРОМ. ИТОГ> */
	LONG64 RI_code;            /* код товаpа */
	/* == 0 - над свободной ценой или <ПРОМ. ИТОГ>   */
	/* != 0 - над указанным товаpом                  */
	LONG64 RI_value;           /* значение скидки/надбавки */
} RAISEINFO;
/*
	Таблица инфоpмации по концу чека
*/
typedef struct {
	short TI_bank;             /* # кассы */
	uchar TI_user;             /* # кассира */
	uchar TI_operation;        /* тип опеpации */
	short TI_ticket;           /* # чека */
	LONG64 TI_payment0;        /* оплата 0 */
	LONG64 TI_payment1;        /* оплата 1 */
	LONG64 TI_payment2;        /* оплата 2 */
	LONG64 TI_payment3;        /* оплата 3 */
	LONG64 TI_return0;         /* возвpат 0 */
	LONG64 TI_return1;         /* возвpат 1 */
	LONG64 TI_return2;         /* возвpат 2 */
	LONG64 TI_return3;         /* возвpат 3 */
} TICKETINFO;
/*
	Cash device keyboard codes
*/
#define _KEY_DIGIT0   0x0000
#define _KEY_DIGIT1   0x0001
#define _KEY_DIGIT2   0x0002
#define _KEY_DIGIT3   0x0003
#define _KEY_DIGIT4   0x0004
#define _KEY_DIGIT5   0x0005
#define _KEY_DIGIT6   0x0006
#define _KEY_DIGIT7   0x0007
#define _KEY_DIGIT8   0x0008
#define _KEY_DIGIT9   0x0009
#define _KEY_DIGIT00  0x000A
#define _KEY_PERIOD   0x000B
#define _KEY_PAPERUP  0x000C
#define _KEY_RESET    0x000D
#define _KEY_TOTAL    0x000E
#define _KEY_I        0x000F
#define _KEY_II       0x0010
#define _KEY_III      0x0011
#define _KEY_MUL      0x0012
#define _KEY_REPEAT   0x0013
#define _KEY_STORNO   0x0014
#define _KEY_TAXES    0x0015
#define _KEY_HOUR     0x0016
#define _KEY_SUBTOTAL 0x0017
#define _KEY_ARTICLE  0x0018
#define _KEY_BANKNUM  0x0019
#define _KEY_PLUS     0x001A
#define _KEY_MINUS    0x001B
#define _KEY_SECT1    0x001C
#define _KEY_SECT2    0x001D
#define _KEY_SECT3    0x001E
#define _KEY_SECT4    0x001F
#define _KEY_SECT5    0x0020
#define _KEY_SECT6    0x0021
#define _KEY_SECT7    0x0022
#define _KEY_SECT8    0x0023
#define _KEY_SECT9    0x0024
#define _KEY_SECT10   0x0025
/*
	Cash device keyboard key position
*/
#define _KEY_WORK     0x0000
#define _KEY_READ     0x0100
#define _KEY_ERASE    0x0200
#define _KEY_PROGRAMM 0x0300
/*
	Состояние выполнения транзакции
*/
#define TR_UNKNOWN     0       /* Неопределено */
#define TR_RXREQUEST   1       /* Принят запрос на информацию по товару */
#define TR_SENTACK     2       /* Товар имеется в наличии */
#define TR_SENTNAK     3       /* Товар отсутствует */
#define TR_FINISHSALES 4       /* Транзакция продажи/возврата завершена */
#define TR_ENDOFTICKET 5       /* Конец чека */
/*
	Типы отчетов с гашением
*/
typedef enum {
	DaylyReport = 1,           /* Дневной финансовый */
	OperatorsReport = 2,       /* Отчет кассиров */
	WareReport = 3             /* Отчет по товарам */
} REPORT_TYPE;
/*
	Информация по драйверу MnCash
*/
typedef struct {
	uchar mi_pid;              /* Идентификатор процесса */
	uchar mi_major;            /* Номер версии драйвера */
	uchar mi_minor;            /* Номер коррекции драйвера */
	uchar mi_usedIRQ;          /* Кол-во используемых IRQ */
	uchar mi_maxIRQ;           /* Максимальное кол-во IRQ */
	uchar mi_usedChannels;     /* Кол-во используемых каналов */
	uchar mi_maxChannels;      /* Максимальное кол-во каналов */
	uchar mi_connectAPI;       /* Логика перехвата запросов подключена */
} MNCASH_INFO;
/*
	Структура описания физических характеристик канала
*/
typedef struct {
	ulong mp_baud;             /* Скорость обмена */
	uchar mp_mode;             /* Режим работы */
	uchar mp_sout;             /* Выходные сигналы */
	uchar mp_stxin;            /* Условие разрешения передачи */
} MNCASH_PCHANNEL;
/*
	Информация по текущему состоянию кассового аппарата
*/
typedef struct {
	uchar mb_drvState;         /* Состояние драйвера */
	uchar mb_cmdState;         /* Состояние выполняемой команды */
	uint  mb_bank;             /* Номер кассового аппарата */
	uchar mb_user;             /* Номер текущего кассира */
	uchar mb_section;          /* Номер текущей секции */
	ulong mb_rate;             /* Текущий курс валюты */
	ulong mb_goods;            /* Код последнего товара */
	long  mb_price;            /* Цена последнего товара */
	long  mb_weight;           /* Кол-во последнего товара */
	long  mb_raise;            /* Последняя скидка или надбавка */
	uint  mb_ticket;           /* Номер последнего закрытого чека */
	long  mb_payment[4];       /* Последние использованные оплаты */
	long  mb_surrender;        /* Последнее значение сдачи */
} MNCASH_BANKINFO;
/*
	Структура описателя параметров секции

	Все данные в этой структуре должны быть в виде терминальных ASCII-кодов
	(для строк - без завершающего нулевого символа в конце строки!).
	Для логических значений код '0' (30h) означает "Нет", а код "1" (31h)
	означает "Да". Все числовые значения должны быть также представлены
	в виде ASCII-строк, пустые позиции в левой части строки должны быть
	заполнены кодом "0" (30h). Например, строка "0123" (30h 31h 32h 33h)
	является экивалентом десятичного числа 123. Если число имеет
	десятичную точку (ее позиция всегда фиксирована), то она явно не
	ставится, а подразумевается в зависимости от типа поля. Так, поле
	количества (веса) товара всегда имеет три десятичных знака после
	точки, поэтому строка "123000" (31h 32h 33h 30h 30h 30h) означает
	либо 123 штуки, либо 123 килограмма товара.
*/
typedef struct {
	uchar s_taxes;             /* Признак запрета вычисления НДС */
	uchar s_singleTransaction; /* Признак разрешения единичной транзакции */
	uchar s_duty;              /* Выбор ставки НДС */
	uchar s_group;             /* Номер товарной группы */
	uchar s_name[12];          /* Наименование секции */
	uchar s_cash[8];           /* Свободная цена секции */
} SECTION;
/*
	Структура обязательной части параметров кассового аппарата

	Все данные в этой структуре должны быть в виде терминальных ASCII-кодов
	(для строк - без завершающего нулевого символа в конце строки!).
	Для логических значений код '0' (30h) означает "Нет", а код "1" (31h)
	означает "Да". Все числовые значения должны быть также представлены
	в виде ASCII-строк, пустые позиции в левой части строки должны быть
	заполнены кодом "0" (30h). Например, строка "0123" (30h 31h 32h 33h)
	является экивалентом десятичного числа 123. Если число имеет
	десятичную точку (ее позиция всегда фиксирована), то она явно не
	ставится, а подразумевается в зависимости от типа поля. Так, поле
	количества (веса) товара всегда имеет три десятичных знака после
	точки, поэтому строка "123000" (31h 32h 33h 30h 30h 30h) означает
	либо 123 штуки, либо 123 килограмма товара.
*/
typedef struct {
	ushort bp_actualSize;      /* Общий размер данных в байтах */
	uchar bp_p1;               /* Алфавит: "0" - рус, "1"-лат, "2" - смешанный */
	uchar bp_p2;               /* Фис. память включана ("1" = да) */
	uchar bp_p3;               /* Резерв (д.б. "0") */
	uchar bp_p4;               /* Указание цены с НДС ("1" = да) */
	uchar bp_p5;               /* Вычисление НДС для скидок и надбавок ("1" = да) */
	uchar bp_p6;               /* Полная оплата ("1" = да) */
	uchar bp_p7;               /* Обязателен ввод оплаченных денег ("1" = да) */
	uchar bp_p8;               /* Печать промежуточной суммы запрещена ("1" = да) */
	uchar bp_p9;               /* Скидки/надбавки над пром. суммой запрещены ("1" = да) */
	uchar bp_p10;              /* Контроль блокировки сейфа ("1" = да) */
	uchar bp_p11;              /* Печать товаров и отделов ("0" - имя, "1" - код) */
	uchar bp_p12;              /* Печать значений НДС после клише разрешена ("1" = да) */
	uchar bp_p13;              /* Тип интерфейса ("0" - автоном, "1" - ТП, "2" - RS232) */
	uchar bp_p14;              /* Режим связи с ЭВМ ("0" - для отчета, "1" - все время) */
	uchar bp_p15;              /* Печать вторичных чеков покупателя разрешена ("1" = да) */
	uchar bp_p16;              /* Печать клише на контрольной ленте разрешена ("1" = да) */
	uchar bp_p17;              /* Кол-во строк в клише (всегда "6") */
	uchar bp_p18;              /* Позиция десятичной точки в ценах ("1" = "x", "2" = "x.y" "3" = "x.yy") */
	uchar bp_p19;              /* Тип электронных весов */
	uchar bp_p20;              /* Считыватель бар-кодов подключен ("1" = да) */
	uchar bp_p21a[3];          /* Условие печати регистров отделов и товаров */
	uchar bp_p21b[8];          /* HALO - макс. свободная цена отдела */
	uchar bp_p22[3];           /* Работа с клавишей <СТ> */
	uchar bp_p23[23][2];       /* Работа с клавиатурой */
	uchar bp_p24[2];           /* Работа с кассирами */
	uchar bp_p25[3][4];        /* Ставки НДС */
	uchar bp_p26[2][4];        /* Процентная ставка скидок и надбавок */
	uchar bp_p27[3][4];        /* Валютный коэффициент */
	uchar bp_p28[4][4];        /* Коды кассиров */
	uchar bp_p29[8][8];        /* Часовые периоды */
	uchar bp_p30[6][18];       /* Текст клише */
	uchar bp_p31[4][8];        /* Имена кассиров */
	uchar bp_p32[3][12];       /* Наименования оплат I, II и III */
	uchar bp_xz[12];           /* "НАЛИЧНЫМИ   " */
	SECTION bp_p33[10];        /* Описатели секций */
} BANKPARM;
/*
	Описатели внутренних (программируемых) товаров

	Все данные в этой структуре должны быть в виде терминальных ASCII-кодов
	(для строк - без завершающего нулевого символа в конце строки!).
	Для логических значений код '0' (30h) означает "Нет", а код "1" (31h)
	означает "Да". Все числовые значения должны быть также представлены
	в виде ASCII-строк, пустые позиции в левой части строки должны быть
	заполнены кодом "0" (30h). Например, строка "0123" (30h 31h 32h 33h)
	является экивалентом десятичного числа 123. Если число имеет
	десятичную точку (ее позиция всегда фиксирована), то она явно не
	ставится, а подразумевается в зависимости от типа поля. Так, поле
	количества (веса) товара всегда имеет три десятичных знака после
	точки, поэтому строка "123000" (31h 32h 33h 30h 30h 30h) означает
	либо 123 штуки, либо 123 килограмма товара.
*/
typedef struct {
	uchar ig_code[4];          /* код товара ASCII */
	uchar ig_sign;             /* хаp-ка товаpа */
	uchar ig_division;         /* отдел */
	uchar ig_taxes;            /* N НДС */
	uchar ig_group;            /* гpуппа товаpа */
	uchar ig_name[12];         /* ASCII имя товаpа */
	uchar ig_price[8];         /* ASCII цена товаpа */
} INTERNALGOODS;
/*
	Паpаметpы генеpации дpайвеpа
*/
#define DPG_ONLY_PRINTER 0x0080 /* Только чековый пpинтеp */
#define DPG_NETWORK      0x0040 /* Сетевая веpсия */
#define DPG_SYSDRIVER    0x0008 /* Системный дpайвеp устpойства */
/*
	Определение прототипов функций
*/
#ifdef __cplusplus
extern "C" {
#endif

MNCASH_INFO* mcGetInfo(void);
int  mnUnload();
int  mcGetPhSettings(int cn,MNCASH_PCHANNEL far* p);
int  mcSetPhSettings(int cn,MNCASH_PCHANNEL far* p);
int  mcOpenChannel(int cn,ulong dollar);
int  mcCloseChannel(int cn);
int  mcRunChannel(int cn,int bank);
char far* mcCfgFile();
ulong mcGetCashIndex(int cn,uint* prec);
int  mcSetCashIndex(int cn,ulong dollar,uint prec);
int  mcGetBankInformation(int cn,MNCASH_BANKINFO* p);
int  mcGetBankState(int cn, uint* bnptr, uint* onptr, uint* tnptr, uint* dsptr, uint* tsptr);
int  mcGetCommandState(int cn);
int  mcSetDateAndTime(int cn);
int  mcReport(int cn, REPORT_TYPE report);
int  mcAccumulation(int cn,int sec,long cash);
int  mcPrintCheck(int cn,int ctype);
int  mcRequestSettings(int cn);
int  mcSetupParmBuffer(int cn, BANKPARM* buf, uint extra);
int  mcSetupBank(int cn);
void mcSetAlarm();
void mcResetAlarm();
int  mcSimulateKeyboard(int cn,int aKey);
int  mcConnectAPI();
int  mcDisconnectAPI();
int  mcWaitCmdFinished(int unit,ulong tmo);
int  mcSaleGoods(uint unit, ulong code, uint sec, long price, char* name);
int  mcIncasso(uint unit,long money,int type);
int  mcCloseTicket(uint unit, long payment0, long payment1, long payment2, long payment3);
void mcIdle(void);

#ifdef __cplusplus
}
#endif

#endif /* __MNCSHAPI_H */
