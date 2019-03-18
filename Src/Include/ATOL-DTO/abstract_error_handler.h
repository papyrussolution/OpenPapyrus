/** @file */

#ifndef ABSTRACT_ERROR_HANDLER
#define ABSTRACT_ERROR_HANDLER

namespace TED
{

class AbstractErrorHandler
{
public:
    virtual ~AbstractErrorHandler()
    {
    }
    virtual int showWarning(const wchar_t *title, const wchar_t *text) = 0;
    virtual int onErrorOccurred(int errorCode) = 0;
};

}

#endif // ABSTRACT_ERROR_HANDLER
