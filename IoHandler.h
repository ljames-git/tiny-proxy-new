#ifndef __IO_HANDLER_H__
#define __IO_HANDLER_H__

class IIoHandler
{
public:
    virtual int handle_read(IIoHandler **h = NULL) = 0;
    virtual int handle_write(IIoHandler **h = NULL) = 0;
    virtual int handle_error() = 0;

public:
    virtual ~IIoHandler() {}
};

#endif //__IO_HANDLER_H__
