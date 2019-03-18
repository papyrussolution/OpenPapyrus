/** @file */

#ifndef ABSTRACT_EVENT_HANDLER_H
#define ABSTRACT_EVENT_HANDLER_H

namespace TED
{

//! Абстрактный обработчик событий
/*!
 */
class AbstractEventHandler
{
public:
    virtual ~AbstractEventHandler()
    {
    }

    //! Callback-функция обработки события
    /*!
       В данном методе обработчик должен получить данные у переданного устройства, а также удалить их, если они больше не нужны.
       \param p Указатель на объект драйвера устройств ввода, у которого есть необработанное событие
       \return Результат обработки
     */
    virtual int on_data_event(void *p = 0) = 0;

};

//! Обработчик событий сканера (для дККТ)
class AbstractScannerEventHandler
{
public:
    virtual ~AbstractScannerEventHandler()
    {
    }

    //! Callback-функция обработки события
    /*!
       \param data Данные
       \param size Размер данных
       \return Результат обработки
     */
    virtual int on_data_event(const unsigned char *data, int size) = 0;

};

}

#endif /* ABSTRACT_EVENT_HANDLER_H */
