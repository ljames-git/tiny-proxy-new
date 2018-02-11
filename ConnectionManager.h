#ifndef __CONNECTION_MANAGER_H__
#define __CONNECTION_MANAGER_H__

class IConnectionManager
{
public:
    virtual int manage(int sock, int status) = 0;
};

#endif //__CONNECTION_MANAGER_H__
