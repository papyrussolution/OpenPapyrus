/** @file */

#ifndef DTO_ERRORS_H
#define DTO_ERRORS_H

//! \name Возвращаемые значения диалогов
//! @{

//! Ошибка дилога
#define EC_DIALOG_ERROR             (-1)
//! Пользователь нажал "Отмена"
#define EC_DIALOG_CANCEL            (0)
//! Пользователь нажал "OK"
#define EC_DIALOG_OK                (1)

//! @}

//! Возвращаемое значение функции при ошибке
#define EC_ERR                      (-1)

//! \name Стандартные коды ошибок
//! @{

//! База стандартных ошибок
#define ERROR_BASE_COMMON           (0)
//! Нет ошибки
#define EC_OK                       (ERROR_BASE_COMMON - 0)
//! Нет связи
#define EC_NOT_CONNECT              (ERROR_BASE_COMMON - 1)
//! Порт недоступен
#define EC_COM_BLOCKED              (ERROR_BASE_COMMON - 3)
//! Ключ защиты не найден
#define EC_KEY_NOT_FOUND            (ERROR_BASE_COMMON - 4)
//! Работа драйвера прервана пользователем
#define EC_BREAK_BY_USER            (ERROR_BASE_COMMON - 5)
//! Недопустимое значение
#define EC_INVALID_PARAM            (ERROR_BASE_COMMON - 6)
//! Невозможно добавить устройство
#define EC_CANT_ADD_DEVICE          (ERROR_BASE_COMMON - 7)
//! Невозможно удалить устройство
#define EC_CANT_DELETE_DEVICE       (ERROR_BASE_COMMON - 8)
//! Устройство не найдено
#define EC_DEVICE_NOT_FOUND         (ERROR_BASE_COMMON - 9)
//! Неверная последовательность команд
#define EC_INCORRECT_COMMAND        (ERROR_BASE_COMMON - 10)
//! Устройство не включено
#define EC_DEVICE_DISABLED          (ERROR_BASE_COMMON - 11)
//! Не поддерживает в данной версии оборудования
#define EC_NOT_SUPPORTED            (ERROR_BASE_COMMON - 12)
//! Драйвер не смог загрузить необходимые модули
#define EC_NOTLOADED                (ERROR_BASE_COMMON - 13)
//! Порт занят
#define EC_INTERNAL_BUSY            (ERROR_BASE_COMMON - 14)
//! Некорректные данные из устройства
#define EC_INCORRECT_DATA           (ERROR_BASE_COMMON - 15)
//! Не поддерживается в данном режиме устройства
#define EC_INVALID_MODE             (ERROR_BASE_COMMON - 16)
//! Нет больше элементов отчета
#define EC_END_OF_REPORT            (ERROR_BASE_COMMON - 17)
//! Устройство с данными параметрами не найдено
#define EC_DEVICE_PARAM_NOT_FOUND   (ERROR_BASE_COMMON - 18)
//! Нет ответа от устройства
#define EC_NO_REPLY                 (ERROR_BASE_COMMON - 20)
//! Соединение разорвано
#define EC_CONNECTION_LOST          (ERROR_BASE_COMMON - 21)
//! Ошибка выделения памяти
#define EC_MALLOC_FAILED            (ERROR_BASE_COMMON - 22)
//! Ошибка инициализации
#define EC_INIT_FAILED              (ERROR_BASE_COMMON - 23)
//! Некорректные настройки
#define EC_INVALID_SETTINGS         (ERROR_BASE_COMMON - 24)
//! Ошибка работы с файлом
#define EC_FILE_ERROR               (ERROR_BASE_COMMON - 25)
//! Неизвестная ошибка
#define EC_UNKNOWN                  (ERROR_BASE_COMMON - 199)
//! Минимальный код ошибки
#define EC_LESSER_COMMON_ERROR      EC_UNKNOWN

//! @}


//! Коды ошибок весов
//! @{

//! База кодов ошибок весов
#define ERROR_BASE_SCALE            (-2700)
//! Ошибка протокола
#define EC_SCALE_PROTOCOL_ERROR     (ERROR_BASE_SCALE - 0)
//! Отрицательный вес
#define EC_SCALE_NEG_WEIGHT         (ERROR_BASE_SCALE - 1)
//! Неверный диапазон
#define EC_SCALE_INVALID_RANGE      (ERROR_BASE_SCALE - 2)
//! Нестабильный вес
#define EC_SCALE_NON_STABLE         (ERROR_BASE_SCALE - 3)
//! Вес больше максимального
#define EC_SCALE_BIG_WEIGHT         (ERROR_BASE_SCALE - 4)
//! Неверный вес тары
#define EC_SCALE_BAD_TARE_WEIGHT    (ERROR_BASE_SCALE - 5)

//! Специфические ошибки весов Штрих
//! Неверная длина данных команды
#define EC_SCALE_SHTRIH_INCORRECT_CMD_LENGTH                    (ERROR_BASE_SCALE - 101)
//! Неверный пароль
#define EC_SCALE_SHTRIH_INCORRECT_PASSWORD                      (ERROR_BASE_SCALE - 102)
//! Команда не реализуется в данном режиме
#define EC_SCALE_SHTRIH_UNEXPECTED_CMD                          (ERROR_BASE_SCALE - 103)
//! Неверное значение параметра
#define EC_SCALE_SHTRIH_INCORRECT_PARAM                         (ERROR_BASE_SCALE - 104)
//! Ошибка при попытки установки нуля
#define EC_SCALE_SHTRIH_ZEROSCALE_ERROR                         (ERROR_BASE_SCALE - 105)
//! Ошибка при установке тары
#define EC_SCALE_SHTRIH_TARE_ERROR                              (ERROR_BASE_SCALE - 106)
//! Сбой энергонезависимой памяти
#define EC_SCALE_SHTRIH_MEMORY_FAILURE                          (ERROR_BASE_SCALE - 107)
//! Команда не реализуется интерфейсом
#define EC_SCALE_SHTRIH_CMD_INTERFACE_ISNOT_IMPLEMENTED         (ERROR_BASE_SCALE - 108)
//! Исчерпан лимит попыток обращения с неверным паролем
#define EC_SCALE_SHTRIH_ATTEMPTS_LIMIT                          (ERROR_BASE_SCALE - 109)
//! Режим градуировки блокирован градуировочным переключателем
#define EC_SCALE_SHTRIH_DISABLED_CALIBRATION_MODE               (ERROR_BASE_SCALE - 110)
//! Клавиатура заблокирована
#define EC_SCALE_SHTRIH_KEYBOARD_IS_BLOCKED                     (ERROR_BASE_SCALE - 111)
//! Нельзя поменять тип текущего канала
#define EC_SCALE_SHTRIH_CANNOT_CHANGE_CHANNEL_CODE              (ERROR_BASE_SCALE - 112)
//! Нельзя выключить текущий канал
#define EC_SCALE_SHTRIH_CANNOT_TURNOFF_CURRENT_CHANNEL          (ERROR_BASE_SCALE - 113)
//! С данным каналом ничего нельзя делать
#define EC_SCALE_SHTRIH_WITH_THIS_CHANNEL_NOTHING_CAN_BE_DONE   (ERROR_BASE_SCALE - 114)
//! Неверный номер канала
#define EC_SCALE_SHTRIH_INCORRECT_CHANNEL_NUMBER                (ERROR_BASE_SCALE - 115)
//! Нет ответа от АЦП
#define EC_SCALE_SHTRIH_ADC_NO_RESPONSE                         (ERROR_BASE_SCALE - 116)
//! Минимальный код ошибки
#define EC_LESSER_SCALE_ERROR       EC_SCALE_SHTRIH_ADC_NO_RESPONSE

//! @}


//! \name Коды ошибок ККТ
//! @{

//! База кодов ошибок ККТ
#define ERROR_BASE_FPTR             (-3800)
//! В ККТ нет денег для выплаты
#define EC_3800                     (ERROR_BASE_FPTR - 0)
//! Чек закрыт - операция невозможна
#define EC_3801                     (ERROR_BASE_FPTR - 1)
//! Чек открыт - операция невозможна
#define EC_3802                     (ERROR_BASE_FPTR - 2)
//! Неверная цена (сумма)
#define EC_3803                     (ERROR_BASE_FPTR - 3)
//! Неверное количество
#define EC_3804                     (ERROR_BASE_FPTR - 4)
//! Нулевая цена
#define EC_3805                     (ERROR_BASE_FPTR - 5)
//! Нет бумаги
#define EC_3807                     (ERROR_BASE_FPTR - 7)
//! Процесс ввода пароля
#define EC_3808                     (ERROR_BASE_FPTR - 8)
//! Недопустимый ИНН
#define EC_3809                     (ERROR_BASE_FPTR - 9)
//! Накопление меньше суммы возврата или аннулирования
#define EC_3810                     (ERROR_BASE_FPTR - 10)
//! Производится печать
#define EC_3811                     (ERROR_BASE_FPTR - 11)
//! Неверная величина скидки/надбавки
#define EC_3813                     (ERROR_BASE_FPTR - 13)
//! Операция после скидки/надбавки невозможна
#define EC_3814                     (ERROR_BASE_FPTR - 14)
//! Неверная секция
#define EC_3815                     (ERROR_BASE_FPTR - 15)
//! Неверный вид оплаты
#define EC_3816                     (ERROR_BASE_FPTR - 16)
//! Переполнение при умножении
#define EC_3817                     (ERROR_BASE_FPTR - 17)
//! Операция запрещена в таблице настроек
#define EC_3818                     (ERROR_BASE_FPTR - 18)
//! Переполнение итога чека
#define EC_3819                     (ERROR_BASE_FPTR - 19)
//! Переполнение контрольной ленты
#define EC_3820                     (ERROR_BASE_FPTR - 20)
//! Открыт чек возврата - операция невозможна
#define EC_3821                     (ERROR_BASE_FPTR - 21)
//! Смена превысила 24 часа
#define EC_3822                     (ERROR_BASE_FPTR - 22)
//! Скидка запрещена в таблице
#define EC_3823                     (ERROR_BASE_FPTR - 23)
//! Аннулирование и возврат в одном чеке
#define EC_3824                     (ERROR_BASE_FPTR - 24)
//! Неверный пароль
#define EC_3825                     (ERROR_BASE_FPTR - 25)
//! Не переполнен буфер контрольной ленты
#define EC_3826                     (ERROR_BASE_FPTR - 26)
//! Смена закрыта - операция невозможна
#define EC_3828                     (ERROR_BASE_FPTR - 28)
//! Идет печать отчета
#define EC_3829                     (ERROR_BASE_FPTR - 29)
//! Неверная дата
#define EC_3830                     (ERROR_BASE_FPTR - 30)
//! Неверное время
#define EC_3831                     (ERROR_BASE_FPTR - 31)
//! Сумма чека меньше суммы сторно
#define EC_3832                     (ERROR_BASE_FPTR - 32)
//! ???
#define EC_3833                     (ERROR_BASE_FPTR - 33)
//! Недопустимый РНМ
#define EC_3834                     (ERROR_BASE_FPTR - 34)
//! Вносимая сумма меньше суммы чека
#define EC_3835                     (ERROR_BASE_FPTR - 35)
//! Начисление сдачи невозможно
#define EC_3836                     (ERROR_BASE_FPTR - 36)
//! Смена открыта, операция невозможна
#define EC_3837                     (ERROR_BASE_FPTR - 37)
//! Неверный номер таблицы
#define EC_3839                     (ERROR_BASE_FPTR - 39)
//! Неверный номер ряда
#define EC_3840                     (ERROR_BASE_FPTR - 40)
//! Неверный номер поля
#define EC_3841                     (ERROR_BASE_FPTR - 41)
//! ККТ заблокирована и ждет ввода пароля налогового инспектора
#define EC_3842                     (ERROR_BASE_FPTR - 42)
//! Заводской номер / MAC-адрес уже задан
#define EC_3843                     (ERROR_BASE_FPTR - 43)
//! Исчерпан лимит перерегистраций
#define EC_3844                     (ERROR_BASE_FPTR - 44)
//! Ошибка фискальной памяти
#define EC_3845                     (ERROR_BASE_FPTR - 45)
//! Запись фискальной памяти повреждена
#define EC_3846                     (ERROR_BASE_FPTR - 46)
//! Неверная смена
#define EC_3847                     (ERROR_BASE_FPTR - 47)
//! Неверный тип отчета
#define EC_3848                     (ERROR_BASE_FPTR - 48)
//! Недопустимый заводской номер ККТ
#define EC_3850                     (ERROR_BASE_FPTR - 50)
//! ККТ не фискализирована
#define EC_3851                     (ERROR_BASE_FPTR - 51)
//! Не задан заводской номер
#define EC_3852                     (ERROR_BASE_FPTR - 52)
//! Нет отчетов
#define EC_3853                     (ERROR_BASE_FPTR - 53)
//! Режим не активизирован
#define EC_3854                     (ERROR_BASE_FPTR - 54)
//! Нет указанного чека в КЛ/ЭЖ
#define EC_3855                     (ERROR_BASE_FPTR - 55)
//! Некорректный код или номер кода защиты
#define EC_3857                     (ERROR_BASE_FPTR - 57)
//! Код защиты не введен
#define EC_3859                     (ERROR_BASE_FPTR - 59)
//! Режим не поддерживается
#define EC_3860                     (ERROR_BASE_FPTR - 60)
//! Повторная скидка/надбавка на операцию невозможна
#define EC_3862                     (ERROR_BASE_FPTR - 62)
//! Скидка/надбавка на предыдущую операцию невозможна
#define EC_3863                     (ERROR_BASE_FPTR - 63)
//! Открыт чек аннулирования - операция невозможна
#define EC_3864                     (ERROR_BASE_FPTR - 64)
//! Открыт чек продажи - операция невозможна
#define EC_3865                     (ERROR_BASE_FPTR - 65)
//! Требуется выполнение общего гашения
#define EC_3866                     (ERROR_BASE_FPTR - 66)
//! Нет устройства, обрабатывающего команду
#define EC_3867                     (ERROR_BASE_FPTR - 67)
//! Нет связи с внешним устройством
#define EC_3868                     (ERROR_BASE_FPTR - 68)
//! Неверный диапазон записей
#define EC_3869                     (ERROR_BASE_FPTR - 69)
//! Команда не разрешена введенными кодами защиты
#define EC_3870                     (ERROR_BASE_FPTR - 70)
//! Невозможна отмена скидки/надбавки
#define EC_3871                     (ERROR_BASE_FPTR - 71)
//! Невозможно открыть чек данным типом оплаты
#define EC_3872                     (ERROR_BASE_FPTR - 72)
//! Нет связи с принтером чеков
#define EC_3873                     (ERROR_BASE_FPTR - 73)
//! Неверный номер маршрута
#define EC_3877                     (ERROR_BASE_FPTR - 77)
//! Неверный номер начальной зоны
#define EC_3878                     (ERROR_BASE_FPTR - 78)
//! Неверный номер конечной зоны
#define EC_3879                     (ERROR_BASE_FPTR - 79)
//! Неверный тип тарифа
#define EC_3880                     (ERROR_BASE_FPTR - 80)
//! Неверный тариф
#define EC_3881                     (ERROR_BASE_FPTR - 81)
//! Нет заказа с таким номером
#define EC_3882                     (ERROR_BASE_FPTR - 82)
//! Снятие отчета прервалось
#define EC_3883                     (ERROR_BASE_FPTR - 83)
//! Неверный штрихкод товара
#define EC_3887                     (ERROR_BASE_FPTR - 87)
//! ККТ заблокирована после попытки ввода даты
#define EC_3892                     (ERROR_BASE_FPTR - 92)
//! Требуется подтверждение ввода даты
#define EC_3893                     (ERROR_BASE_FPTR - 93)
//! Отчет с гашением прерван. Устраните неисправность и повторите печать отчета
#define EC_3894                     (ERROR_BASE_FPTR - 94)
//! Неверная длина
#define EC_3895                     (ERROR_BASE_FPTR - 95)
//! Сумма не наличных оплат превышает сумму чека
#define EC_3896                     (ERROR_BASE_FPTR - 96)
//! Чек оплачен не полностью
#define EC_3897                     (ERROR_BASE_FPTR - 97)
//! Неверный номер картинки/штрихкода
#define EC_3898                     (ERROR_BASE_FPTR - 98)
//! Невозможно открыть файл или формат некорректный
#define EC_3899                     (ERROR_BASE_FPTR - 99)
//! Сумма платежей меньше суммы сторно
#define EC_3900                     (ERROR_BASE_FPTR - 100)
//! Неверный номер регистра
#define EC_3901                     (ERROR_BASE_FPTR - 101)
//! Недопустимое целевое значение
#define EC_3902                     (ERROR_BASE_FPTR - 102)
//! Неверный параметр регистра
#define EC_3903                     (ERROR_BASE_FPTR - 103)
//! Недостаточно памяти
#define EC_3904                     (ERROR_BASE_FPTR - 104)
//! Неверный формат или значение
#define EC_3905                     (ERROR_BASE_FPTR - 105)
//! Переполнение суммы платежей
#define EC_3906                     (ERROR_BASE_FPTR - 106)
//! Ошибка обмена с фискальным модулем
#define EC_3907                     (ERROR_BASE_FPTR - 107)
//! Переполнение ФП
#define EC_3908                     (ERROR_BASE_FPTR - 108)
//! Ошибка печатающего устройства
#define EC_3909                     (ERROR_BASE_FPTR - 109)
//! Ошибка интерфейса ЭКЛЗ/ФН
#define EC_3910                     (ERROR_BASE_FPTR - 110)
//! Ошибка формата передачи параметров ЭКЛЗ/ФН
#define EC_3911                     (ERROR_BASE_FPTR - 111)
//! Неверное состояние ЭКЛЗ/ФН
#define EC_3912                     (ERROR_BASE_FPTR - 112)
//! Неисправимая ошибка ЭКЛЗ/ФН
#define EC_3913                     (ERROR_BASE_FPTR - 113)
//! Авария крипто-процессора ЭКЛЗ/Ошибка КС ФН
#define EC_3914                     (ERROR_BASE_FPTR - 114)
//! Исчерпан временной ресурс ЭКЛЗ/Закончен срок эксплуатации ФН
#define EC_3915                     (ERROR_BASE_FPTR - 115)
//! ЭКЛЗ переполнена/Архив ФН переполнен
#define EC_3916                     (ERROR_BASE_FPTR - 116)
//! В ЭКЛЗ/ФН переданы неверные дата и время
#define EC_3917                     (ERROR_BASE_FPTR - 117)
//! В ЭКЛЗ/ФН нет запрошенных данных
#define EC_3918                     (ERROR_BASE_FPTR - 118)
//! Переполнение итога чека в ЭКЛЗ/ФН
#define EC_3919                     (ERROR_BASE_FPTR - 119)
//! Исчерпан лимит активаций
#define EC_3920                     (ERROR_BASE_FPTR - 120)
//! Проверьте дату и время
#define EC_3921                     (ERROR_BASE_FPTR - 121)
//! Дата и/или время в ККТ меньше чем в ЭКЛЗ/ФН
#define EC_3922                     (ERROR_BASE_FPTR - 122)
//! Невозможно закрыть архив ЭКЛЗ/ФН
#define EC_3923                     (ERROR_BASE_FPTR - 123)
//! Необходимо провести профилактические работы
#define EC_3924                     (ERROR_BASE_FPTR - 124)
//! Неверный номер ЭКЛЗ/ФН
#define EC_3925                     (ERROR_BASE_FPTR - 125)
//! Предыдущая операция не завершена
#define EC_3926                     (ERROR_BASE_FPTR - 126)
//! Переполнение сменного итога
#define EC_3927                     (ERROR_BASE_FPTR - 127)
//! Активация данной ЭКЛЗ/ФН в составе данной ККТ невозможна
#define EC_3928                     (ERROR_BASE_FPTR - 128)
//! Переполнение счетчика наличности
#define EC_3929                     (ERROR_BASE_FPTR - 129)
//! Переполнение буфера чека
#define EC_3930                     (ERROR_BASE_FPTR - 130)
//! Размер картинки слишком большой
#define EC_3931                     (ERROR_BASE_FPTR - 131)
//! Неверный тип чека
#define EC_3932                     (ERROR_BASE_FPTR - 132)
//! Вход в режим заблокирован
#define EC_3933                     (ERROR_BASE_FPTR - 133)
//! Неверные номер смен в ККТ и ЭКЛЗ/ФН
#define EC_3934                     (ERROR_BASE_FPTR - 134)
//! ЭКЛЗ/ФН отсутствует
#define EC_3935                     (ERROR_BASE_FPTR - 135)
//! Итоги чека ККТ и ЭКЛЗ/ФН не совпадают
#define EC_3936                     (ERROR_BASE_FPTR - 136)
//! ККТ находится в режиме ввода даты или времени
#define EC_3937                     (ERROR_BASE_FPTR - 137)
//! Переполнение буфера отложенного документа
#define EC_3938                     (ERROR_BASE_FPTR - 138)
//! Невозможно напечатать второй фискальный оттиск
#define EC_3939                     (ERROR_BASE_FPTR - 139)
//! Буфер переполнен
#define EC_3940                     (ERROR_BASE_FPTR - 140)
//! Требуется гашение ЭЖ
#define EC_3941                     (ERROR_BASE_FPTR - 141)
//! Перегрев головки принтера
#define EC_3942                     (ERROR_BASE_FPTR - 142)
//! Ошибка отрезчика
#define EC_3943                     (ERROR_BASE_FPTR - 143)
//! Буфер принтера ПД пустой
#define EC_3944                     (ERROR_BASE_FPTR - 144)
//! Буфер принтера ПД переполнен
#define EC_3945                     (ERROR_BASE_FPTR - 145)
//! Карта физически отсутствует в картоприемнике
#define EC_3946                     (ERROR_BASE_FPTR - 146)
//! Неверный PIN код
#define EC_3947                     (ERROR_BASE_FPTR - 147)
//! Ячейка защищена от записи
#define EC_3948                     (ERROR_BASE_FPTR - 148)
//! Невозможно сторно последней позиции
#define EC_3949                     (ERROR_BASE_FPTR - 149)
//! Сторно по коду невозможно
#define EC_3950                     (ERROR_BASE_FPTR - 150)
//! Невозможен повтор последней операции
#define EC_3951                     (ERROR_BASE_FPTR - 151)
//! Неверный код товара
#define EC_3952                     (ERROR_BASE_FPTR - 152)
//! Нет подтверждения или отмены продажи
#define EC_3953                     (ERROR_BASE_FPTR - 153)
//! Отключение контроля наличности невозможно
#define EC_3954                     (ERROR_BASE_FPTR - 154)
//! Товар не найден
#define EC_3955                     (ERROR_BASE_FPTR - 155)
//! Весовой штрихкод с кол-вом != 1.000
#define EC_3956                     (ERROR_BASE_FPTR - 156)
//! Переполнение буфера чека
#define EC_3957                     (ERROR_BASE_FPTR - 157)
//! Недостаточное кол-во товара
#define EC_3958                     (ERROR_BASE_FPTR - 158)
//! Сторнируемое количество больше проданного
#define EC_3959                     (ERROR_BASE_FPTR - 159)
//! Заблокированный товар не найден в буфере чека
#define EC_3960                     (ERROR_BASE_FPTR - 160)
//! Данный товар не продавался в чеке, сторно невозможно
#define EC_3961                     (ERROR_BASE_FPTR - 161)
//! Memo Plus 3 заблокировано с ПК
#define EC_3962                     (ERROR_BASE_FPTR - 162)
//! Ошибка контрольной суммы таблицы настроек Memo Plus 3
#define EC_3963                     (ERROR_BASE_FPTR - 163)
//! Идет обработка контрольной ленты
#define EC_3964                     (ERROR_BASE_FPTR - 164)
//! Недопустимый пароль
#define EC_3965                     (ERROR_BASE_FPTR - 165)
//! Ошибочное состояния ТРК
#define EC_3966                     (ERROR_BASE_FPTR - 166)
//! Невозможно напечатать вторую фискальную копию
#define EC_3967                     (ERROR_BASE_FPTR - 167)
//! Ошибка питания
#define EC_3968                     (ERROR_BASE_FPTR - 168)
//! Сумма налога больше суммы регистраций по чеку и/или итога
#define EC_3969                     (ERROR_BASE_FPTR - 169)
//! Начисление налога на последнюю операцию невозможно
#define EC_3970                     (ERROR_BASE_FPTR - 170)
//! Операция невозможна, недостаточно питания
#define EC_3971                     (ERROR_BASE_FPTR - 171)
#define EC_PRINTER_BUSY             (ERROR_BASE_FPTR - 172)
//! Ошибка записи в накопитель фискальной памяти
#define EC_3974                     (ERROR_BASE_FPTR - 174)
//! Некорректное значение параметров команды ФН
#define EC_3975                     (ERROR_BASE_FPTR - 175)
//! Превышение размеров TLV данных ФН
#define EC_3976                     (ERROR_BASE_FPTR - 176)
//! Нет транспортного соединения ФН
#define EC_3977                     (ERROR_BASE_FPTR - 177)
//! Исчерпан ресурс КС ФН
#define EC_3978                     (ERROR_BASE_FPTR - 178)
//! Исчерпан ресурс хранения ФН
#define EC_3979                     (ERROR_BASE_FPTR - 179)
//! Сообщение от ОФД не может быть принято ФН
#define EC_3980                     (ERROR_BASE_FPTR - 180)
//! В ФН есть не отправленные ФД
#define EC_3981                     (ERROR_BASE_FPTR - 181)
//! Несовпадение контрольной суммы накопителя фискальной памяти
#define EC_3994                     (ERROR_BASE_FPTR - 194)
//! Исчерпан ресурс ожидания передачи сообщения в ФН
#define EC_3995                     (ERROR_BASE_FPTR - 195)
//! Продолжительность смены ФН более 24 часов
#define EC_3996                     (ERROR_BASE_FPTR - 196)
//! Неверная разница во времени между двумя операциями ФН
#define EC_3997                     (ERROR_BASE_FPTR - 197)
//! Ошибка SD
#define EC_4012                     (ERROR_BASE_FPTR - 212)

//! Недопустимое кол-во регистраций
#define EC_4020                     (ERROR_BASE_FPTR - 220)
//! Некорректная СНО
#define EC_4021                     (ERROR_BASE_FPTR - 221)
//! Недопустимый номер ставки налога
#define EC_4022                     (ERROR_BASE_FPTR - 222)
//! Недопустимый тип кода товара
#define EC_4023                     (ERROR_BASE_FPTR - 223)
//! Недопустима регистрация подакцизного товара
#define EC_4024                     (ERROR_BASE_FPTR - 224)
//! Ошибка программирования реквизита
#define EC_4025                     (ERROR_BASE_FPTR - 225)
//! Невозможно начислить скидку
#define EC_4026                     (ERROR_BASE_FPTR - 226)
//! Отчет о регистрации ККТ прерван. Устраните неисправность и повторите печать отчета
#define EC_4027                     (ERROR_BASE_FPTR - 227)
//! Отчет о закрытии ФН. Устраните неисправность и повторите печать отчета
#define EC_4028                     (ERROR_BASE_FPTR - 228)
//! Открытие смены прервано. Устраните неисправность и повторите открытие смены
#define EC_4029                     (ERROR_BASE_FPTR - 229)
//! Отчет о состоянии расчетов прерван. Устраните неисправность и повторите печать отчета
#define EC_4030                     (ERROR_BASE_FPTR - 230)
//! Закрытие чека прервано. Устраните неисправность
#define EC_4031                     (ERROR_BASE_FPTR - 231)
//! Получение документа из ФН прервано
#define EC_4032                     (ERROR_BASE_FPTR - 232)
//! Ошибка работы с ЭЖ
#define EC_4033                     (ERROR_BASE_FPTR - 233)
//! Некорректная версия ФФД
#define EC_4034                     (ERROR_BASE_FPTR - 234)
//! Внутренняя ошибка ККТ
#define EC_4035                     (ERROR_BASE_FPTR - 235)
//! Параметр доступен только для чтения
#define EC_4036                     (ERROR_BASE_FPTR - 236)
//! Превышен максимальный размер чека
#define EC_4037                     (ERROR_BASE_FPTR - 237)
//! Некорректный признак способа расчета
#define EC_4038                     (ERROR_BASE_FPTR - 238)
//! Некорректный признак предмета расчета
#define EC_4039                     (ERROR_BASE_FPTR - 239)

//! Буфер пуст
#define EC_4101                     (ERROR_BASE_FPTR - 301)
//! Ошибка GSM-модуля
#define EC_4102                     (ERROR_BASE_FPTR - 302)
//! Модем выключен
#define EC_MODEM_OFFLINE            (ERROR_BASE_FPTR - 303)
//! Ошибка GSM-модуля
#define EC_MODEM_NOT_READY          (ERROR_BASE_FPTR - 304)
//! Ошибка GSM-модуля
#define EC_UNABLE_TO_CONNECT        (ERROR_BASE_FPTR - 305)
//! Минимальный код ошибки
#define EC_LESSER_FPTR_ERROR        EC_PRINTER_BUSY
//! @}


//! \name Коды ошибок принтера чеков
//! @{

//! База кодов ошибок ПЧ
#define ERROR_BASE_RCPPRN                   (-6000)
//! Нет бумаги
#define EC_PRN_NOPAPER                      (ERROR_BASE_RCPPRN - 1)
//! Ошибка в механике или незакрыт корпус
#define EC_PRN_MECHANICAL                   (ERROR_BASE_RCPPRN - 2)
//! Ошибка автоотрезчика
#define EC_PRN_AUTOCUTTER                   (ERROR_BASE_RCPPRN - 3)
//! Перегрев печатающей головки
#define EC_PRN_HOTPRINTHEAD                 (ERROR_BASE_RCPPRN - 4)
//! Буфер переполнен
#define EC_PRN_BUFFER_OVERFLOW              (ERROR_BASE_RCPPRN - 5)
//! Принтер находится в режиме OffLine
#define EC_PRN_OFFLINE                      (ERROR_BASE_RCPPRN - 6)
//! Ошибка в параметрах принтера
#define EC_PRN_PARAMETER                    (ERROR_BASE_RCPPRN - 7)
//! Принтер занят выполнением другой операции
#define EC_PRN_BUSY                         (ERROR_BASE_RCPPRN - 8)
//! Принтер вернул неизвестный формат команды
#define EC_PRN_UNKNOWN                      (ERROR_BASE_RCPPRN - 9)
//! Механическая ошибка ПД принтера
#define EC_PRN_SLIPMECHANICAL               (ERROR_BASE_RCPPRN - 10)
//! Невозможно записать данные в буфер принтера
#define EC_PRN_BUFFER_NOTEMPTY              (ERROR_BASE_RCPPRN - 11)
//! Сработал датчик конца бумаги
#define EC_PRN_PAPERNEAREND                 (ERROR_BASE_RCPPRN - 12)
//! ШК не вмещается на ленте
#define EC_PRN_BARCODEWIDTH                 (ERROR_BASE_RCPPRN - 13)

//! Операция невозможна: принтер находится в состоянии ошибка или остановлен
#define EC_PRN_ERRORSTATE                   (ERROR_BASE_RCPPRN - 20)
//! Нет данных для печати задания
#define EC_PRN_PRINT_NOTASK                 (ERROR_BASE_RCPPRN - 21)

//! Печать текста не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_ADDTEXT_UNSP           (ERROR_BASE_RCPPRN - 40)
//! Печать картинки не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_ADDPICTURE_UNSP        (ERROR_BASE_RCPPRN - 41)
//! Отрезка ленты не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_ADDCUTPAPER_UNSP       (ERROR_BASE_RCPPRN - 42)
//! Промотка ленты не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_ADDFEEDPAPER_UNSP      (ERROR_BASE_RCPPRN - 43)
//! Генерация звукового сигнала не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_ADDBEEP_UNSP           (ERROR_BASE_RCPPRN - 44)
//! Работа с денежным ящиком не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_ADDDRAWER_UNSP         (ERROR_BASE_RCPPRN - 45)
//! Работа с ПД принтером не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_ADDSLIP_UNSP           (ERROR_BASE_RCPPRN - 46)
//! Печать Штрихкода не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_ADDBARCODE_UNSP        (ERROR_BASE_RCPPRN - 47)
//! Печать картинки из памяти не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_ADDPICTUREMEM_UNSP     (ERROR_BASE_RCPPRN - 48)

//! Неверное значение в поле StatusErrorIndex
#define EC_PRN_VALUE_STATUSERRORINDEX       (ERROR_BASE_RCPPRN - 50)
//! Неверное значение в поле Model
#define EC_PRN_VALUE_MODEL                  (ERROR_BASE_RCPPRN - 51)
//! Неверное значение в поле CodePage
#define EC_PRN_VALUE_CODEPAGE               (ERROR_BASE_RCPPRN - 52)
//! Значение в поле CodePage не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_CODEPAGE_UNSP          (ERROR_BASE_RCPPRN - 53)
//! Неверное значение в поле CharSet
#define EC_PRN_VALUE_CHARSET                (ERROR_BASE_RCPPRN - 54)
//! Значение в поле CharSet не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_CHARSET_UNSP           (ERROR_BASE_RCPPRN - 55)
//! Неверное значение в поле FontIndex
#define EC_PRN_VALUE_FONTINDEX              (ERROR_BASE_RCPPRN - 56)
//! Значение в поле FontIndex не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_FONTINDEX_UNSP         (ERROR_BASE_RCPPRN - 57)
//! Неверное значение в поле PortNumber
#define EC_PRN_VALUE_PORTNUMBER             (ERROR_BASE_RCPPRN - 58)
//! Значение в поле PortNumber не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_PORTNUMBER_UNSP        (ERROR_BASE_RCPPRN - 59)
//! Неверное значение в поле BaudRate
#define EC_PRN_VALUE_BAUDRATE               (ERROR_BASE_RCPPRN - 60)
//! Значение в поле BaudRate не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_BAUDRATE_UNSP          (ERROR_BASE_RCPPRN - 61)
//! Неверное значение в поле Copies
#define EC_PRN_VALUE_COPYCOUNT              (ERROR_BASE_RCPPRN - 62)
//! Значение в поле Copies не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_COPYCOUNT_UNSP         (ERROR_BASE_RCPPRN - 63)
//! Значение в поле FontDblWidth не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_FONTDBLWIDTH_UNSP      (ERROR_BASE_RCPPRN - 64)
//! Значение в поле FontDblHeight не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_FONTDBLHEIGHT_UNSP     (ERROR_BASE_RCPPRN - 65)
//! Значение в поле FontOverLine не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_FONTOVERLINE_UNSP      (ERROR_BASE_RCPPRN - 66)
//! Значение в поле FontUnderLine не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_FONTUNDERLINE_UNSP     (ERROR_BASE_RCPPRN - 67)
//! Значение в поле FontNegative не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_FONTNEGATIVE_UNSP      (ERROR_BASE_RCPPRN - 68)
//! Значение в поле TextUpSideDown не поддерживается в данной модели
//! оборудования
#define EC_PRN_VALUE_TEXTUPSIDEDOWN_UNSP    (ERROR_BASE_RCPPRN - 69)
//! Значение в поле FontBold не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_FONTBOLD_UNSP          (ERROR_BASE_RCPPRN - 70)
//! Значение в поле FontItalic не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_FONTITALIC_UNSP        (ERROR_BASE_RCPPRN - 71)
//! Неверное значение в поле InitMode
#define EC_PRN_VALUE_INITMODE               (ERROR_BASE_RCPPRN - 72)
//! Значение в поле InitMode не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_INITMODE_UNSP          (ERROR_BASE_RCPPRN - 73)
//! Неверное значение в поле SlipValue
#define EC_PRN_VALUE_SLIPVALUE              (ERROR_BASE_RCPPRN - 74)
//! Значение в поле SlipValue не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_SLIPVALUE_UNSP         (ERROR_BASE_RCPPRN - 75)
//! Значение в поле ZeroSlashed не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_ZEROSLASHED_UNSP       (ERROR_BASE_RCPPRN - 76)
//! Неверное значение в поле CharRotation
#define EC_PRN_VALUE_CHARROTATION           (ERROR_BASE_RCPPRN - 77)
//! Значение в поле CharRotation не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_CHARROTATION_UNSP      (ERROR_BASE_RCPPRN - 78)
//! Неверное значение в поле Color
#define EC_PRN_VALUE_COLOR                  (ERROR_BASE_RCPPRN - 79)
//! Значение в поле Color не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_COLOR_UNSP             (ERROR_BASE_RCPPRN - 80)
//! Неверное значение в поле Aligment
#define EC_PRN_VALUE_ALIGNMENT              (ERROR_BASE_RCPPRN - 81)
//! Значение в поле Alignment не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_ALIGNMENT_UNSP         (ERROR_BASE_RCPPRN - 82)
//! Неверное значение в поле CutValue
#define EC_PRN_VALUE_CUTVALUE               (ERROR_BASE_RCPPRN - 83)
//! Значение в поле CutValue не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_CUTVALUE_UNSP          (ERROR_BASE_RCPPRN - 84)
//! Неверное значение в поле FeedValue
#define EC_PRN_VALUE_FEEDVALUE              (ERROR_BASE_RCPPRN - 85)
//! Значение в поле FeedValue не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_FEEDVALUE_UNSP         (ERROR_BASE_RCPPRN - 86)
//! Неверное значение в поле BeepValue
#define EC_PRN_VALUE_BEEPVALUE              (ERROR_BASE_RCPPRN - 87)
//! Неверное значение в поле LineSpacing
#define EC_PRN_VALUE_LINESPACING            (ERROR_BASE_RCPPRN - 88)
//! Значение в поле LineSpacing не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_LINESPACING_UNSP       (ERROR_BASE_RCPPRN - 89)
//! Невозможно открыть файл
#define EC_PRN_VALUE_FILENAME_OPEN          (ERROR_BASE_RCPPRN - 90)
//! Невозможно сохранить файл
#define EC_PRN_VALUE_FILENAME_SAVE          (ERROR_BASE_RCPPRN - 91)
//! Некорректный формат файла
#define EC_PRN_VALUE_FILENAME_FORMAT        (ERROR_BASE_RCPPRN - 92)
//! Некорректный размер рисунка
#define EC_PRN_VALUE_PICTURE_SIZE           (ERROR_BASE_RCPPRN - 93)
//! Неверное значение в поле ActiveStations
#define EC_PRN_VALUE_ACTIVESTATIONS         (ERROR_BASE_RCPPRN - 94)
//! Значение в поле ActiveStations не поддерживается в данной модели
//! оборудования
#define EC_PRN_VALUE_ACTIVESTATIONS_UNSP    (ERROR_BASE_RCPPRN - 95)
//! Неверное значение в поле Rotation
#define EC_PRN_VALUE_ROTATION               (ERROR_BASE_RCPPRN - 96)
//! Значение в поле Rotation не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_ROTATION_UNSP          (ERROR_BASE_RCPPRN - 97)
//! Неверное значение в поле DrawerValue
#define EC_PRN_VALUE_DRAWERVALUE            (ERROR_BASE_RCPPRN - 98)
//! Значение в поле DrawerValue не поддерживается в данной модели оборудования
#define EC_PRN_VALUE_DRAWERVALUE_UNSP       (ERROR_BASE_RCPPRN - 99)
//! Неверное значение в поле BarcodeHeight
#define EC_PRN_VALUE_BARCODEHEIGHT          (ERROR_BASE_RCPPRN - 100)
//! Неверное значение в поле BarcodeScale
#define EC_PRN_VALUE_BARCODESCALE           (ERROR_BASE_RCPPRN - 101)
//! Неверное значение в поле BarcodeType
#define EC_PRN_VALUE_BARCODETYPE            (ERROR_BASE_RCPPRN - 102)

//! Неверное значение в поле PageHeight
#define EC_PRN_VALUE_PAGEHEIGHT             (ERROR_BASE_RCPPRN - 106)
//! Неверное значение в поле FlowControl
#define EC_PRN_VALUE_FLOWCONTROL            (ERROR_BASE_RCPPRN - 107)
//! Минимальный код ошибки
#define EC_LESSER_RCPPRN_ERROR              EC_PRN_VALUE_FLOWCONTROL
//! @}

//! \name Коды параметров ККТ
//! @{

#define BC_BASE_PARAM               (0)
#define BC_OK                       (BC_BASE_PARAM - 0)
#define BC_VALUE                    (BC_BASE_PARAM - 1)
#define BC_VALUEPURPOSE             (BC_BASE_PARAM - 2)
#define BC_CAPTION                  (BC_BASE_PARAM - 3)
#define BC_MODE                     (BC_BASE_PARAM - 4)
#define BC_PRICE                    (BC_BASE_PARAM - 5)
#define BC_NAME                     (BC_BASE_PARAM - 6)
#define BC_QUANTITY                 (BC_BASE_PARAM - 7)
#define BC_DEPARTMENT               (BC_BASE_PARAM - 8)
#define BC_SUMM                     (BC_BASE_PARAM - 9)
#define BC_TYPECLOSE                (BC_BASE_PARAM - 10)
#define BC_PERCENTS                 (BC_BASE_PARAM - 11)
#define BC_SECOND                   (BC_BASE_PARAM - 12)
#define BC_MINUTE                   (BC_BASE_PARAM - 13)
#define BC_HOUR                     (BC_BASE_PARAM - 14)
#define BC_DAY                      (BC_BASE_PARAM - 15)
#define BC_MONTH                    (BC_BASE_PARAM - 16)
#define BC_YEAR                     (BC_BASE_PARAM - 17)
#define BC_PRICE_QUANTITY           (BC_BASE_PARAM - 18)
#define BC_PASSWORD                 (BC_BASE_PARAM - 19)
#define BC_REPORTTYPE               (BC_BASE_PARAM - 20)
#define BC_CAPTIONPURPOSE           (BC_BASE_PARAM - 21)
#define BC_PLUNUMBER                (BC_BASE_PARAM - 22)
#define BC_BAUDRATE                 (BC_BASE_PARAM - 23)
#define BC_PORTNUMBER               (BC_BASE_PARAM - 24)
#define BC_CURRENTDEVICENAME        (BC_BASE_PARAM - 25)
#define BC_ENDDAY                   (BC_BASE_PARAM - 26)
#define BC_ENDMONTH                 (BC_BASE_PARAM - 27)
#define BC_ENDYEAR                  (BC_BASE_PARAM - 28)
#define BC_INN                      (BC_BASE_PARAM - 29)
#define BC_SESSION                  (BC_BASE_PARAM - 30)
#define BC_ENDSESSION               (BC_BASE_PARAM - 31)
#define BC_SERIALNUMBER             (BC_BASE_PARAM - 32)
#define BC_MACHINENUMBER            (BC_BASE_PARAM - 33)
#define BC_DESTINATION              (BC_BASE_PARAM - 34)
#define BC_LICENSE                  (BC_BASE_PARAM - 35)
#define BC_TIMEOUT                  (BC_BASE_PARAM - 36)
#define BC_CURRENTDEVICEINDEX       (BC_BASE_PARAM - 37)
#define BC_CURRENTDEVICENUMBER      (BC_BASE_PARAM - 38)
#define BC_UMODE                    (BC_BASE_PARAM - 39)
#define BC_TAX                      (BC_BASE_PARAM - 40)
#define BC_BARCODE                  (BC_BASE_PARAM - 41)
#define BC_TABLE                    (BC_BASE_PARAM - 42)
#define BC_ROW                      (BC_BASE_PARAM - 43)
#define BC_FIELD                    (BC_BASE_PARAM - 44)
#define BC_FIELDTYPE                (BC_BASE_PARAM - 45)
#define BC_POINTPOSITION            (BC_BASE_PARAM - 46)
#define BC_FACTOR                   (BC_BASE_PARAM - 47)
#define BC_ORDERNUMBER              (BC_BASE_PARAM - 48)
#define BC_RECORDTYPE               (BC_BASE_PARAM - 49)
#define BC_ORDERCODE                (BC_BASE_PARAM - 50)
#define BC_CHECKNUMBER              (BC_BASE_PARAM - 51)
#define BC_ROUTENUMBER              (BC_BASE_PARAM - 52)
#define BC_RATETYPE                 (BC_BASE_PARAM - 53)
#define BC_FIRSTZONE                (BC_BASE_PARAM - 54)
#define BC_LASTZONE                 (BC_BASE_PARAM - 55)
#define BC_FIRSTRECORD              (BC_BASE_PARAM - 56)
#define BC_LASTRECORD               (BC_BASE_PARAM - 57)
#define BC_ROUTECODE                (BC_BASE_PARAM - 58)
#define BC_TESTMODE                 (BC_BASE_PARAM - 59)
#define BC_ACCESSPASSWORD           (BC_BASE_PARAM - 60)
#define BC_OUTBOUNDSTREAM           (BC_BASE_PARAM - 61)
#define BC_DRAWERONTIMEOUT          (BC_BASE_PARAM - 62)
#define BC_DRAWEROFFTIMEOUT         (BC_BASE_PARAM - 63)
#define BC_DRAWERONQUANTITY         (BC_BASE_PARAM - 64)
#define BC_PRINTPURPOSE             (BC_BASE_PARAM - 65)
#define BC_LINENUMBER               (BC_BASE_PARAM - 66)
#define BC_PROTOCOL                 (BC_BASE_PARAM - 67)
#define BC_TAX1                     (BC_BASE_PARAM - 68)
#define BC_TAX2                     (BC_BASE_PARAM - 69)
#define BC_TAX3                     (BC_BASE_PARAM - 70)
#define BC_TAX4                     (BC_BASE_PARAM - 71)
#define BC_Frequency                (BC_BASE_PARAM - 72)
#define BC_Duration                 (BC_BASE_PARAM - 73)
#define BC_Count                    (BC_BASE_PARAM - 74)
#define BC_RecFont                  (BC_BASE_PARAM - 75)
#define BC_RecBrightness            (BC_BASE_PARAM - 76)
#define BC_RecFontHeight            (BC_BASE_PARAM - 77)
#define BC_Height                   (BC_BASE_PARAM - 78)
#define BC_RecLineSpacing           (BC_BASE_PARAM - 79)
#define BC_CheckType                (BC_BASE_PARAM - 80)
#define BC_WORKSHOP                 (BC_BASE_PARAM - 81)
#define BC_PictureNumber            (BC_BASE_PARAM - 82)
#define BC_LeftMargin               (BC_BASE_PARAM - 83)
#define BC_JrnBrightness            (BC_BASE_PARAM - 86)
#define BC_TextWrap                 (BC_BASE_PARAM - 87)
#define BC_JrnFont                  (BC_BASE_PARAM - 88)
#define BC_JrnFontHeight            (BC_BASE_PARAM - 89)
#define BC_JrnLineSpacing           (BC_BASE_PARAM - 90)
#define BC_Width                    (BC_BASE_PARAM - 92)
#define BC_RegisterNumber           (BC_BASE_PARAM - 93)
#define BC_UnitType                 (BC_BASE_PARAM - 94)
#define BC_StreamFormat             (BC_BASE_PARAM - 95)
#define BC_BarcodeType              (BC_BASE_PARAM - 96)
#define BC_EKLZKPKNumber            (BC_BASE_PARAM - 97)
#define BC_SlipLineSpacing          (BC_BASE_PARAM - 98)
#define BC_Alignment                (BC_BASE_PARAM - 99)
#define BC_SCALE                    (BC_BASE_PARAM - 100)
#define BC_SlipDocCharLineLength    (BC_BASE_PARAM - 101)
#define BC_SlipDocCopyCountHorz     (BC_BASE_PARAM - 102)
#define BC_SlipDocCopyCountVert     (BC_BASE_PARAM - 103)
#define BC_SlipDocCopyShiftHorz     (BC_BASE_PARAM - 104)
#define BC_SlipDocCopyShiftVert     (BC_BASE_PARAM - 105)
#define BC_SlipDocOrientation       (BC_BASE_PARAM - 106)
#define BC_SlipDocLeftMargin        (BC_BASE_PARAM - 107)
#define BC_SlipDocTopMargin         (BC_BASE_PARAM - 108)
#define BC_OPERATIONTYPE            (BC_BASE_PARAM - 109)
#define BC_MODEL                    (BC_BASE_PARAM - 110)
#define BC_DISCOUNTTYPE             (BC_BASE_PARAM - 111)
#define BC_DISCOUNTVALUE            (BC_BASE_PARAM - 112)
#define BC_DISCOUNTTYPENUMBER       (BC_BASE_PARAM - 113)
#define BC_CAPTIONTABLE             (BC_BASE_PARAM - 117)
#define BC_COMMANDBUFFER            (BC_BASE_PARAM - 118)
#define BC_FILEOPENTYPE             (BC_BASE_PARAM - 119)
#define BC_FILEOPENMODE             (BC_BASE_PARAM - 120)
#define BC_SETTINGSTRING            (BC_BASE_PARAM - 200)
#define BC_TABLETYPE                (BC_BASE_PARAM - 201)
#define BC_WARETYPE                 (BC_BASE_PARAM - 202)
#define BC_LICENSENUMBER            (BC_BASE_PARAM - 203)
#define BC_MODEMIPADDRESS           (BC_BASE_PARAM - 204)
#define BC_MODEMIPPORT              (BC_BASE_PARAM - 205)
#define BC_MODEMMODE                (BC_BASE_PARAM - 206)
#define BC_PINPADMODE               (BC_BASE_PARAM - 207)
#define BC_SYSTEM_OPERATION_DATA    (BC_BASE_PARAM - 208)
#define BC_LESSER_FPTR_PARAM        BC_SYSTEM_OPERATION_DATA
//! @}

//! Коды ошибок ПС
//! @{

//! База кодов ошибок ПС
#define ERROR_BASE_PAYCARD_COMMON                       (-10000)

//! База ошибок параметров
#define EC_PAYCARD_BASE_BAD_PROPERTY                    (ERROR_BASE_PAYCARD_COMMON - 1000) //-11000

//! Ошибка сетевого соединения
#define EC_PAYCARD_CURL_ERROR                           (EC_PAYCARD_BASE_BAD_PROPERTY - 1)
//! Недопустимая сумма
#define EC_PAYCARD_BAD_SUM                              (EC_PAYCARD_BASE_BAD_PROPERTY - 2)
//! Недопустимый индекс оператора
#define EC_PAYCARD_BAD_OPERATOR                         (EC_PAYCARD_BASE_BAD_PROPERTY - 3)
//! Недопустимый номер телефона
#define EC_PAYCARD_BAD_PHONE_NUMBER                     (EC_PAYCARD_BASE_BAD_PROPERTY - 4)
//! Недопустимый тип авторизации
#define EC_PAYCARD_BAD_AUTH_TYPE                        (EC_PAYCARD_BASE_BAD_PROPERTY - 5)
//! Недопустимый протокол платежной системы
#define EC_PAYCARD_BAD_PROTOCOL                         (EC_PAYCARD_BASE_BAD_PROPERTY - 6)
//! Недопустимая последовательность команд отчета
#define EC_PAYCARD_BAD_REPORT_STATE                     (EC_PAYCARD_BASE_BAD_PROPERTY - 7)
//! Недопустимый тип транзакции
#define EC_PAYCARD_BAD_TRANS_TYPE                       (EC_PAYCARD_BASE_BAD_PROPERTY - 8)
//! Недопустимый тип операции
#define EC_PAYCARD_BAD_OPERATION_TYPE                   (EC_PAYCARD_BASE_BAD_PROPERTY - 9)
//! Недопустимый тип отчета
#define EC_PAYCARD_BAD_REPORT_TYPE                      (EC_PAYCARD_BASE_BAD_PROPERTY - 10)
//! Недопустимая директория обмена с сервером авторизации
#define EC_PAYCARD_BAD_PATH_AC                          (EC_PAYCARD_BASE_BAD_PROPERTY - 11)
//! Недопустимая директория внутренней базы данных
#define EC_PAYCARD_BAD_PATH_DB                          (EC_PAYCARD_BASE_BAD_PROPERTY - 12)
//! Недопустимое время ожидания ответа сервера авторизации
#define EC_PAYCARD_BAD_TIMEOUT                          (EC_PAYCARD_BASE_BAD_PROPERTY - 13)
//! Недопустимый адрес сервера авторизации
#define EC_PAYCARD_BAD_URL                              (EC_PAYCARD_BASE_BAD_PROPERTY - 14)
//! Недопустимый адрес прокси-сервера
#define EC_PAYCARD_BAD_PROXY_URL                        (EC_PAYCARD_BASE_BAD_PROPERTY - 15)
//! Недопустимый порт прокси-сервера
#define EC_PAYCARD_BAD_PROXY_PORT                       (EC_PAYCARD_BASE_BAD_PROPERTY - 16)
//! Недопустимый код операции
#define EC_PAYCARD_BAD_OPERATION_CODE                   (EC_PAYCARD_BASE_BAD_PROPERTY - 17)
//! Недопустимый тип параметра
#define EC_PAYCARD_BAD_PARAM_TYPE                       (EC_PAYCARD_BASE_BAD_PROPERTY - 18)
//! Недопустимый номер счета
#define EC_PAYCARD_BAD_ACCOUNT                          (EC_PAYCARD_BASE_BAD_PROPERTY - 19)
//! Недопустимый тип карты
#define EC_PAYCARD_BAD_CARD_TYPE                        (EC_PAYCARD_BASE_BAD_PROPERTY - 20)
//! Недопустимое имя
#define EC_PAYCARD_BAD_NAME                             (EC_PAYCARD_BASE_BAD_PROPERTY - 21)
//! Недопустимая сумма в отчете
#define EC_PAYCARD_BAD_REPORT_SUM                       (EC_PAYCARD_BASE_BAD_PROPERTY - 22)
//! Сумма недостаточна для проведения платежа
#define EC_PAYCARD_SUM_TO_SMALL                         (EC_PAYCARD_BASE_BAD_PROPERTY - 23)
//! Недопустимый тип служебной операции
#define EC_PAYCARD_BAD_SERVICE_OPERATION_TYPE           (EC_PAYCARD_BASE_BAD_PROPERTY - 24)
//! Недопустимый идентификатор терминала
#define EC_PAYCARD_BAD_TERMINAL_ID                      (EC_PAYCARD_BASE_BAD_PROPERTY - 25)
//! Недопустимый код валюты
#define EC_PAYCARD_BAD_CURRENCY_CODE                    (EC_PAYCARD_BASE_BAD_PROPERTY - 26)
//! Недопустимый номер чека ККТ
#define EC_PAYCARD_BAD_ECR_RECEIPT_NUMBER               (EC_PAYCARD_BASE_BAD_PROPERTY - 27)

//! Недопустимое имя оператора
#define EC_PAYCARD_BAD_OPERATOR_NAME                    (EC_PAYCARD_BASE_BAD_PROPERTY - 80)
//! Недопустимый код оператора
#define EC_PAYCARD_BAD_OPERATOR_CODE                    (EC_PAYCARD_BASE_BAD_PROPERTY - 81)
//! Недопустимое значение минимальной суммы платежа оператора
#define EC_PAYCARD_BAD_OPERATOR_MIN_SUM                 (EC_PAYCARD_BASE_BAD_PROPERTY - 82)
//! Недопустимое значение максимальной суммы платежа оператора
#define EC_PAYCARD_BAD_OPERATOR_MAX_SUM                 (EC_PAYCARD_BASE_BAD_PROPERTY - 83)
//! Недопустимое указание комиссии
#define EC_PAYCARD_BAD_OPERATOR_COMMISSION_TYPE         (EC_PAYCARD_BASE_BAD_PROPERTY - 84)
//! Недопустимое начисление комиссии
#define EC_PAYCARD_BAD_OPERATOR_COMMISSION_ADD          (EC_PAYCARD_BASE_BAD_PROPERTY - 85)
//! Недопустимое значение комиссии оператора
#define EC_PAYCARD_BAD_OPERATOR_COMMISSION              (EC_PAYCARD_BASE_BAD_PROPERTY - 86)
//! Недопустимое значение требующей подтверждения суммы
#define EC_PAYCARD_BAD_OPERATOR_ADD_REQ_SUM             (EC_PAYCARD_BASE_BAD_PROPERTY - 87)
//! Недопустимый дополнительный параметр №1
#define EC_PAYCARD_BAD_OPERATOR_ADDPARAM1               (EC_PAYCARD_BASE_BAD_PROPERTY - 88)
//! Недопустимая маска параметра №1
#define EC_PAYCARD_BAD_OPERATOR_MASK1                   (EC_PAYCARD_BASE_BAD_PROPERTY - 89)
//! Недопустимый дополнительный параметр №2
#define EC_PAYCARD_BAD_OPERATOR_ADDPARAM2               (EC_PAYCARD_BASE_BAD_PROPERTY - 90)
//! Недопустимая маска параметра №2
#define EC_PAYCARD_BAD_OPERATOR_MASK2                   (EC_PAYCARD_BASE_BAD_PROPERTY - 91)

//! Недопустимое время ожидания проверки статуса платежа
#define EC_CYBERPLAT_BAD_CHECK_TIMEOUT                  (EC_PAYCARD_BASE_BAD_PROPERTY - 92)
//! Недопустимый код дилера
#define EC_CYBERPLAT_BAD_DEALER_CODE                    (EC_PAYCARD_BASE_BAD_PROPERTY - 93)
//! Недопустимый код точки приема
#define EC_CYBERPLAT_BAD_POINT_CODE                     (EC_PAYCARD_BASE_BAD_PROPERTY - 94)
//! Недопустимый код оператора
#define EC_CYBERPLAT_BAD_OPERATOR_CODE                  (EC_PAYCARD_BASE_BAD_PROPERTY - 95)
//! Недопустимый пароль закрытого ключа
#define EC_CYBERPLAT_BAD_KEY_LOAD_PWD                   (EC_PAYCARD_BASE_BAD_PROPERTY - 96)
//! Недопустимый серийный номер ключа
#define EC_CYBERPLAT_BAD_SERIAL_KEY_NUMBER              (EC_PAYCARD_BASE_BAD_PROPERTY - 97)

//! Недопустимый код закрытия смены
#define EC_ARCUS2_BAD_OPCODE_CLOSE_DAY                  (EC_PAYCARD_BASE_BAD_PROPERTY - 98)
//! Недопустимый код печати журнала операций
#define EC_ARCUS2_BAD_OPCODE_PRINT_JOURNAL              (EC_PAYCARD_BASE_BAD_PROPERTY - 99)

//! База кодов недостающих параметров
#define EC_PAYCARD_BASE_NEED_PROPERTY                   (ERROR_BASE_PAYCARD_COMMON - 1900) //-11900

//! Требуется номер телефона
#define EC_PAYCARD_NEED_PHONE_NUMBER                    (EC_PAYCARD_BASE_NEED_PROPERTY - 1)
//! Требуется номер счета
#define EC_PAYCARD_NEED_ACCOUNT                         (EC_PAYCARD_BASE_NEED_PROPERTY - 2)
//! Требуется ссылочный номер
#define EC_PAYCARD_NEED_REFERENCE_NUMBER                (EC_PAYCARD_BASE_NEED_PROPERTY - 3)
//! Требуется код авторизации
#define EC_PAYCARD_NEED_AUTH_CODE                       (EC_PAYCARD_BASE_NEED_PROPERTY - 4)

//! База ошибок выполнения
#define EC_PAYCARD_EXECUTION_BASE                       (ERROR_BASE_PAYCARD_COMMON - 2000) //-12000

//! Ошибка диалогов
#define EC_PAYCARD_DIALOG_ERROR                         (EC_PAYCARD_EXECUTION_BASE - 1)
//! Операция не поддерживается платежной системой
#define EC_PAYCARD_UNSUPPORTED                          (EC_PAYCARD_EXECUTION_BASE - 2)

//! Ошибка электронной подписи
#define EC_CYBERPLAT_SIGN_LIBRARY_ERROR                 (EC_PAYCARD_EXECUTION_BASE - 50)
//! Ошибка Киберплат
#define EC_CYBERPLAT_RESPONSE_ERROR                     (EC_PAYCARD_EXECUTION_BASE - 51)
//! Ошибка загрузки публичного ключа
#define EC_CYBERPLAT_PUBLIC_KEY_LOAD_ERROR              (EC_PAYCARD_EXECUTION_BASE - 52)
//! Ошибка загрузки секретного ключа
#define EC_CYBERPLAT_SECRET_KEY_LOAD_ERROR              (EC_PAYCARD_EXECUTION_BASE - 53)

//! Ошибка запуска бинарного файла ARCOM ARCUS v.2
#define EC_ARCUS2_CASHREG_EXECUTE_ERROR                 (EC_PAYCARD_EXECUTION_BASE - 100)

//! Ошибка выполнения операции INPAS SmartSale
#define EC_INPAS_EXECUTE_ERROR                          (EC_PAYCARD_EXECUTION_BASE - 150)
//! Ошибка выполнения запроса результата INPAS SmartSale
#define EC_INPAS_GET_RESULT_ERROR                       (EC_PAYCARD_EXECUTION_BASE - 151)
//! Ошибка инициализации библиотеки INPAS SmartSale
#define EC_INPAS_INIT_RES_ERROR                         (EC_PAYCARD_EXECUTION_BASE - 152)


//! Ошибка запуска бинарного файла Сбербанка
#define EC_SBERBANK_PILOT_EXECUTE_ERROR                 (EC_PAYCARD_EXECUTION_BASE - 200)
//! Ошибка чтения результата Сбербанка
#define EC_SBERBANK_PILOT_READ_ERROR                    (EC_PAYCARD_EXECUTION_BASE - 201)
//! Ошибка чтения слипа Сбербанка
#define EC_SBERBANK_PILOT_CHEQUE_ERROR                  (EC_PAYCARD_EXECUTION_BASE - 202)

// Ошибки UCS
//! Ошибка "EFTPOS устройству требуется инкасация"
#define EC_UCS_COLLECTION_REQUIRED_ERROR                (EC_PAYCARD_EXECUTION_BASE - 301)
//! Ошибка "В EFTPOS устройстве закончилась бумага"
#define EC_UCS_OUT_OF_PEPER_ERROR                       (EC_PAYCARD_EXECUTION_BASE - 302)

//! База ошибок ПС
#define EC_PAYCARD_AC_ERROR                             ERROR_BASE_PAYCARD_COMMON //-10000

//! Ошибка разбора ответа сервера
#define EC_CYBERPLAT_RESPONSE_PARSE_ERROR               (EC_PAYCARD_AC_ERROR - 1)
//! Ошибка разбора запроса к серверу
#define EC_CYBERPLAT_REQUEST_PARSE_ERROR                (EC_PAYCARD_AC_ERROR - 2)
//! Ошибка выполнения запроса ARCOM ARCUS v.2
#define EC_ARCUS2_RESPONSE_ERROR                        (EC_PAYCARD_AC_ERROR - 10)
//! Ошибка выполнения запроса INPAS SmartSale
#define EC_INPAS_RESPONSE_ERROR                         (EC_PAYCARD_AC_ERROR - 20)
//! Ошибка выполнения запроса Сбербанка
#define EC_SBERBANK_RESPONSE_ERROR                      (EC_PAYCARD_AC_ERROR - 30)
//! Ошибка выполнения запроса Газпромбанка
#define EC_GAZPROMBANK_RESPONSE_ERROR                   (EC_PAYCARD_AC_ERROR - 40)
//! Ошибка выполнения запроса ICMP
#define EC_ICMP_RESPONSE_ERROR                          (EC_PAYCARD_AC_ERROR - 50)

//! Ошибка библиотеки EFTPOS: потеряна связь
#define EC_UCS_LOST_CONNECTION_ERROR                    (EC_PAYCARD_AC_ERROR - 301)
//! Ошибка библиотеки EFTPOS: нулевой указатель на управляющий объект
#define EC_UCS_NULLPTR_ERROR                            (EC_PAYCARD_AC_ERROR - 302)
//! Ошибка библиотеки EFTPOS: неизвестный код ошибки
#define EC_UCS_UNKNOWN_ERROR                            (EC_PAYCARD_AC_ERROR - 303)
//! Ошибка библиотеки EFTPOS: недопустимая команда
#define EC_UCS_INVALID_COMMAND_ERROR                    (EC_PAYCARD_AC_ERROR - 304)
//! Ошибка библиотеки EFTPOS: неразрешенная комбинация параметров
#define EC_UCS_UNRESOLVED_PARAMETERS_ERROR              (EC_PAYCARD_AC_ERROR - 305)
//! Ошибка библиотеки EFTPOS: незавершенная сессия с предыдущим TID
#define EC_UCS_INCOMPLETE_SESSION_ERROR                 (EC_PAYCARD_AC_ERROR - 306)
//! Ошибка библиотеки EFTPOS: сбой создания необходимых коммуникаций
#define EC_UCS_CREATE_COMMUNICATION_ERROR               (EC_PAYCARD_AC_ERROR - 307)
//! Ошибка библиотеки EFTPOS: параллельный вызов
#define EC_UCS_PARALLEL_CALL_ERROR                      (EC_PAYCARD_AC_ERROR - 308)
//! Ошибка библиотеки EFTPOS: сбой запуска
#define EC_UCS_START_FAILED_ERROR                       (EC_PAYCARD_AC_ERROR - 309)
//! Ошибка EFTPOS: Не поллучен пакет 6-0
#define EC_UCS_NO_PACK_60                               (EC_PAYCARD_AC_ERROR - 310)

//! @}

//! Ошибки динамических библиотек
//! @{

//! База кодов ошибок динамических библиотек
#define ERROR_BASE_LIBRARY                  (-5000)
//! Не удалось найти gui_engine.dll
#define EC_CANT_FIND_GUIENGINE              (ERROR_BASE_LIBRARY - 1)
//! Не удалось найти usbpd.dll
#define EC_CANT_FIND_USBPD                  (ERROR_BASE_LIBRARY - 2)
//! Не удалось найти input_gui.dll
#define EC_CANT_FIND_INPUTGUI               (ERROR_BASE_LIBRARY - 3)
//! Не удалось найти библиотеку cURL
#define EC_CANT_FIND_CURL                   (ERROR_BASE_LIBRARY - 4)
//! Не удалось найти библиотеку CyberPlat
#define EC_CANT_FIND_IPRIV                  (ERROR_BASE_LIBRARY - 5)
//! Не удалось найти библиотеку графического интерфейса драйвера ПС
#define EC_CANT_FIND_PAYCARD_GUI            (ERROR_BASE_LIBRARY - 6)
//! Не удалось найти библиотеку Arcom Arcus2
#define EC_CANT_FIND_ARCUS2                 (ERROR_BASE_LIBRARY - 7)
//! Не удалось найти библиотеку INPAS SmartSale
#define EC_CANT_FIND_INPAS                  (ERROR_BASE_LIBRARY - 8)
//! Не удалось найти библиотеку переводов
#define EC_CANT_FIND_GETTEXT                (ERROR_BASE_LIBRARY - 9)
//! Не удалось найти библиотеку Сбербанка
#define EC_CANT_FIND_SBERBANK               (ERROR_BASE_LIBRARY - 10)
//! Не удалось найти библиотеку Газпромбанка
#define EC_CANT_FIND_GAZPROMBANK            (ERROR_BASE_LIBRARY - 11)
//! Не удалось найти libusb.so
#define EC_CANT_FIND_LIBUSB                 (ERROR_BASE_LIBRARY - 12)
//! Не удалось найти библиотеку INPAS SmartSale
#define EC_CANT_FIND_INPAS_ATOL             (ERROR_BASE_LIBRARY - 13)
//! Не удалось найти библиотеку libudev
#define EC_CANT_FIND_UDEV                   (ERROR_BASE_LIBRARY - 14)
//! Не удалось найти библиотеку Arcom Arcus2
#define EC_CANT_FIND_ARCUS2_ATOL            (ERROR_BASE_LIBRARY - 15)
//! Не удалось найти библиотеку libfptr
#define EC_CANT_FIND_FPTR                   (ERROR_BASE_LIBRARY - 16)
//! Не удалось найти библиотеку libinput
#define EC_CANT_FIND_INPUT                  (ERROR_BASE_LIBRARY - 17)
//! Не удалось найти библиотеку libscale
#define EC_CANT_FIND_SCALE                  (ERROR_BASE_LIBRARY - 18)
//! Не удалось найти библиотеку libdisplay
#define EC_CANT_FIND_DISPLAY                (ERROR_BASE_LIBRARY - 19)
//! Не удалось найти библиотеку libpaycard
#define EC_CANT_FIND_PAYCARD                (ERROR_BASE_LIBRARY - 20)
//! Не удалось найти библиотеку Сбербанка (АТОЛ)
#define EC_CANT_FIND_SBERBANK_ATOL          (ERROR_BASE_LIBRARY - 21)
//! Не удалось найти библиотеку Сбербанка (ТТК)
#define EC_CANT_FIND_SBERBANK_TTK           (ERROR_BASE_LIBRARY - 22)
//! Не удалось найти библиотеку ОФД
#define EC_CANT_FIND_OFD                    (ERROR_BASE_LIBRARY - 23)
//! Не удалось найти библиотеку UCS
#define EC_CANT_FIND_UCS                    (ERROR_BASE_LIBRARY - 24)
//! Не удалось произвести инициализацию библиотеки
#define EC_CANT_INIT_LIBRARY                (ERROR_BASE_LIBRARY - 100)
//! @}

//! Коды ошибок дВПЭ
//! @{

//! База кодов ошибок дВПЭ
#define ERROR_BASE_LP               (-2000)
//! Неверный PLU
#define EC_LP_INCORRECT_PLU         (ERROR_BASE_LP - 1)
//! Весы не готовы к обмену информацией
#define EC_LP_SCALE_NOT_READY       (ERROR_BASE_LP - 10)
//! Ошибка при передаче данных
#define EC_LP_SCALE_EXCHANGE        (ERROR_BASE_LP - 11)
//! Недопустимое Сообщение
#define EC_LP_INVALID_MSG           (ERROR_BASE_LP - 21)
//! PLU вне диапазона
#define EC_LP_PLU_INVALID_RANGE     (ERROR_BASE_LP - 22)
//! Неверный Тип Товара
#define EC_LP_INVALID_WARE_TYPE     (ERROR_BASE_LP - 23)
//! Неверный Срок Годности
#define EC_LP_INVALID_LIFE          (ERROR_BASE_LP - 24)
//! Неверный Групповой Код
#define EC_LP_INVALID_GROUP_CODE    (ERROR_BASE_LP - 25)
//! Неверный Номер Сообщения
#define EC_LP_INVALID_MSG_NUMBER    (ERROR_BASE_LP - 26)
//! Тара вне диапазона
#define EC_LP_TARE_INVALID_RANGE    (ERROR_BASE_LP - 27)
//! Неверная Цена
#define EC_LP_INVALID_PRICE         (ERROR_BASE_LP - 28)
//! Неверный код товара
#define EC_LP_INVALID_WARE_CODE     (ERROR_BASE_LP - 29)
//! Неожиданное значение статусных байтов
#define EC_LP_INVALID_STATUS        (ERROR_BASE_LP - 30)
//! Неверный шрифт
#define EC_LP_INVALID_FONT          (ERROR_BASE_LP - 31)
//! Ошибка печатающего устройства
#define EC_LP_PRINT_DEVICE          (ERROR_BASE_LP - 32)
//! Вес нестабилен
#define EC_LP_NON_STABLE            (ERROR_BASE_LP - 33)
//! Ошибка установки даты
#define EC_LP_INVALID_DATE          (ERROR_BASE_LP - 34)
//! Ошибка установки времени
#define EC_LP_INVALID_TIME          (ERROR_BASE_LP - 35)
//! Ошибка в номере изображения
#define EC_LP_INVALID_PIC_NUMBER    (ERROR_BASE_LP - 36)
//! Ошибка памяти весов
#define EC_LP_MEMORY_FULL           (ERROR_BASE_LP - 37)
//! Неизвестная ошибка оборудования
#define EC_LP_UNKNOWN_ERROR         (ERROR_BASE_LP - 99)
#define EC_LESSER_LP_ERROR          EC_LP_UNKNOWN_ERROR
//! @}

#endif // DTO_ERRORS_H

