#ifndef NETWORK_H
#define NETWORK_H

#include <map>
#include <vector>

#include "JsonValue.h"
#include "Common.h"
#include "Identity.h"
#include "Protocol.h"
#include "Users.h"
#include "Engine.h"

struct Reply
{
    std::vector<std::uint32_t> dest;
    JsonObject data;

    Reply(std::uint32_t d, const JsonObject &obj)
        : data(obj)
    {
        dest.push_back(d);
    }

    Reply(const std::vector<std::uint32_t> &dlist, const JsonObject &obj)
        : dest(dlist)
        , data(obj)
    {

    }
};


struct Request
{
    std::uint32_t src_uuid;
    std::uint32_t dest_uuid;
    std::string arg;

    Request()
        : src_uuid(Protocol::INVALID_UID)
        , dest_uuid(Protocol::INVALID_UID)
    {

    }
};

// Ask steps translation from/into integer
struct Ack
{
    std::string step; // Textual value
    Engine::Sequence sequence; // Engine sequence

    static Engine::Sequence FromString(const std::string &step);
    static std::string ToString(Engine::Sequence sequence);
};


inline bool FromJson(Tarot::Distribution &dist, const JsonObject &obj)
{
    bool ret = true;

    dist.mFile = obj.GetValue("file").GetString();
    dist.mSeed = obj.GetValue("seed").GetInteger();
    dist.TypeFromString(obj.GetValue("type").GetString());

    return ret;
}

inline void ToJson(const Tarot::Distribution &dist, JsonObject &obj)
{
    obj.AddValue("file", dist.mFile);
    obj.AddValue("seed", dist.mSeed);
    obj.AddValue("type", dist.TypeToString());
}

inline bool FromJson(Identity &ident, JsonObject &obj)
{
    bool ret = true;

    ident.nickname = obj.GetValue("nickname").GetString();
    ident.avatar = obj.GetValue("avatar").GetString();
    ident.GenderFromString(obj.GetValue("gender").GetString());

    return ret;
}

inline void ToJson(Identity &ident, JsonObject &obj)
{
    obj.AddValue("nickname", ident.nickname);
    obj.AddValue("avatar", ident.avatar);
    obj.AddValue("gender", ident.GenderToString());
}

class INetClientEvent
{
public:
    virtual ~INetClientEvent() { }

    virtual bool Deliver(const Request &req) = 0;
    virtual void Disconnected() = 0;
};

class INetClient
{
public:
    virtual ~INetClient() { }

    virtual void Send(uint32_t my_uid, const std::vector<Reply> &replies) = 0;
    virtual void ConnectToHost(const std::string &host, uint16_t tcp_port) = 0;
    virtual void Disconnect() = 0;
};

class INetServerEvent
{
public:
    static const std::uint32_t ErrDisconnectedFromServer    = 6000U;
    static const std::uint32_t ErrCannotConnectToServer     = 6001U;

    virtual ~INetServerEvent() { }

    virtual bool Deliver(const Request &req, std::vector<Reply> &out) = 0;
    virtual std::uint32_t AddUser() = 0;
    virtual void RemoveUser(std::uint32_t uuid, std::vector<Reply> &out) = 0;
    virtual std::uint32_t GetUuid() = 0;
};


#endif // NETWORK_H
