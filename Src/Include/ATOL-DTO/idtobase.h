/** @file */

#ifndef IDTOBASE_H
#define IDTOBASE_H

#if defined(_WIN32) || defined(_WIN32_WCE)
#  if defined(DTO_LIBRARY)
#    define DTOSHARED_EXPORT __declspec(dllexport)
#  else
#    define DTOSHARED_EXPORT __declspec(dllimport)
#  endif
#  define DTOSHARED_CCA __cdecl
#elif defined(__linux__) || defined(ANDROID)
#  define DTOSHARED_EXPORT
#  define DTOSHARED_CCA
#else
#  define DTOSHARED_EXPORT
#  define DTOSHARED_CCA __attribute__ ((cdecl))
#endif

#include "abstract_error_handler.h"


namespace TED
{

//! Базовый интерфейс для работы с драйверами
class IDTOBase
{
public:
    virtual ~IDTOBase()
    {
    }

    //! Устанавливает Context
    /*!
       Сохраняет указатель на контекст Android-приложения в статическую глобальную переменну для дальнейшего использования в специфичных для Android случаях.
       Android-обертки ДТО самостоятельно вызывают данный метод и передают в него контекст, полученный ими в конструкторе.
       \param context Указатель на контекст android-приложения
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_AndroidContext(void *context) = 0;

    //! Возвращает строку с версией драйвера
    /*!
       Версия драйвера представляет собой строку вида MAJOR.MINOR.PATCH.BUILD
       \param bfr Буфер для строки
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_Version(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает строку с названием драйвера
    /*!
       Формирует специфичную для каждого драйвера строку с описанием.
       \param bfr Буфер для строки
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_DriverName(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает логическое состояние драйвера
    /*!
       Если драйвер был активирован (put_DeviceEnabled()), возвращается 1 (true), иначе 0 (false).
       Не сбрасывается, если связь с устройством была разорвана без вызова put_DeviceEnabled().
       \param value Состояние драйвера
       \retval 0 Успех
       \sa put_DeviceEnabled()
     */
    virtual int DTOSHARED_CCA get_DeviceEnabled(int *value) = 0;

    //! Устанавливает логическое состояние драйвера
    /*!
       При активации драйвера (value = 1 (true)) происходит попытка установки связи с устройством (если это требуется).
       При деактивации драйвера (value = 0 (false)) происходит разрыв связи с устройством (если требуется).
       При неудачной попытке логическое состояние не изменяется. Для получения текущего состояния используется get_DeviceEnabled().
       \param value 1 (true) - активировать драйвер, 0 (false) - деактивировать драйвер
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_DeviceEnabled()
     */
    virtual int DTOSHARED_CCA put_DeviceEnabled(int value) = 0;

    //! Возвращает результат последней операции
    /*!
       Возвращает результат последнего вызванного setter-а свойства или метода. Возможные результаты представлены в dto_errors.h.
       \param value Результат
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_ResultCode(int *value) = 0;

    //! Возвращает текстовое описание результата последней операции
    /*!
       \param bfr Буфер для описания результата
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa get_ResultCode()
     */
    virtual int DTOSHARED_CCA get_ResultDescription(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает код неверного параметра
    /*!
       Возвращает уточненный код ошибки для \a ResultCode = EC_INVALID_PARAM
       \param value Код параметра
       \retval 0 Успех
       \sa EC_INVALID_PARAM
     */
    virtual int DTOSHARED_CCA get_BadParam(int *value) = 0;

    //! Возвращает текстовое описание неверного параметра
    /*!
       \param bfr Буфер для описания результата
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa get_BadParam()
     */
    virtual int DTOSHARED_CCA get_BadParamDescription(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает дескриптор родительского приложения
    /*!
       \param value Дескриптор
       \retval 0 Успех
       \sa put_ApplicationHandle()
     */
    virtual int DTOSHARED_CCA get_ApplicationHandle(void **value) = 0;

    //! Устанавливает дескриптор родительского приложения
    /*!
       Дескриптор используется для привязки всплывающих диалогов к контексту родительского приложения.
       \warning На данный момент применяется только для Qt-приложений. Передача чего-либо, кроме указателя на QApplication, может привести к краху приложения!
       \param value Дескриптор
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ApplicationHandle()
     */
    virtual int DTOSHARED_CCA put_ApplicationHandle(void *value) = 0;

    //! Выводит диалог настройки драйвера
    /*!
       Использует libgui_engine.so (gui_engine.dll) для вывода диалога настройки
       \warning Работает только в Linux/Windows. Для Android используйте SettingsActivity или программную настройку (put_DeviceSettings(), put_DeviceSingleSetting(), ApplySingleSettings())
       \retval -1 Ошибка
       \retval 0 Был произведен выход из диалога без сохранения изменений
       \retval 1 Был произведен выход из диалога c сохранением изменений
     */
    virtual int DTOSHARED_CCA ShowProperties() = 0;

    //! Возвращает строку с текущими настройками драйвера
    /*!
       \param bfr Буфер для настроек
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_DeviceSettings()
     */
    virtual int DTOSHARED_CCA get_DeviceSettings(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает настройки драйвера
    /*!
       \param value Строка с настройками
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_DeviceSettings()
     */
    virtual int DTOSHARED_CCA put_DeviceSettings(const wchar_t *value) = 0;

    //! Возвращает значение настройки
    /*!
       Возвращает значение настройки \a name в виде строки
       \param name Название настройки
       \param bfr Буфер для значения
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_DeviceSingleSetting(const wchar_t *name, wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает значение настроки
    /*!
       Устанавливает строковое значение настройки \a name. Настройка применяется после вызова ApplySingleSettings()
       \param name Название настройки
       \param value Значение настройки
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_DeviceSingleSetting(const wchar_t *name, const wchar_t *value) = 0;

    //! Возвращает значение настройки
    /*!
       Возвращает значение настройки \a name в виде целого числа
       \param name Название настройки
       \param value Значение настройки
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_DeviceSingleSetting(const wchar_t *name, int *value) = 0;

    //! Устанавливает значение настроки
    /*!
       Устанавливает целочисленное значение настройки \a name. Настройка применяется после вызова ApplySingleSettings()
       \param name Название настройки
       \param value Значение настройки
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_DeviceSingleSetting(const wchar_t *name, const int value) = 0;

    //! Возвращает значение настройки
    /*!
       Возвращает значение настройки \a name в виде числа с точкой
       \param name Название настройки
       \param value Значение настройки
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_DeviceSingleSetting(const wchar_t *name, double *value) = 0;

    //! Устанавливает значение настроки
    /*!
       Устанавливает дробное значение настройки \a name. Настройка применяется после вызова ApplySingleSettings()
       \param name Название настройки
       \param value Значение настройки
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_DeviceSingleSetting(const wchar_t *name, const double value) = 0;

    //! Возвращает возможные значения настройки
    /*!
       Возвращает возможные значения настройки \a name в виде строки. Строка имеет вид значение1:описание1;значение2:описание2;...
       \param name Название настройки
       \param bfr Буфер для значения настройки
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_DeviceSingleSettingMapping(const wchar_t *name, wchar_t *bfr, int bfrSize) = 0;

    //! Применяет установленные настройки
    /*!
       Применяет настройки, заданные через put_DeviceSingleSetting(). Переустанавливает соединение с устройством заново.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ApplySingleSettings() = 0;

    //! Сбрасывает настройки до последних подтвержденных
    /*!
       Сбрасывает настройки, заданные через put_DeviceSingleSetting(), на текущие.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ResetSingleSettings() = 0;

    //! Возвращает тип лицензии
    /*!
       \warning Для внутреннего пользования
       \param value Тип лицензии
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_LicenseValid(int *value) = 0;

    //! Возвращает дату истечения лицензии
    /*!
       \warning Для внутреннего пользования
       \param bfr Буфер для даты
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_LicenseExpiredDate(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает обработчик ошибок
    /*!
       \warning На данный момент возвращает только ошибку EC_CONNECTION_LOST в случае потери связи с ККТ (дККТ) и некоторыми платежными терминалами (дПС).
       \param value Дескриптор
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_ErrorHandler(AbstractErrorHandler *value) = 0;

protected:
    IDTOBase()
    {
    }

private:
    IDTOBase& operator=(const IDTOBase&);
};


} // namespace TED

#endif /* IDTOBASE_H */
