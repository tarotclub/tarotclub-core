#ifndef I_SERVER_H
#define I_SERVER_H

#include <string>
#include "ServerConfig.h"

class IServer
{

public:
    virtual ~IServer() { /* Nothing to do */ }

    virtual void AddClient(const std::string &webId, const std::string &gek, const std::string &passPhrase) = 0;
    virtual ServerOptions &GetOptions() = 0;
};

#endif // I_SERVER_H
