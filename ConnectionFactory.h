#ifndef __CONNECTION_FACTORY_H__
#define __CONNECTION_FACTORY_H__

#include "IoHandler.h"
#include "MultiPlexer.h"

class IConnectionFactory
{
public:
    virtual IIoHandler *create(int sock, int type, IMultiPlexer *multi_plexer) = 0;
};

#endif //__CONNECTION_FACTORY_H__
