#ifndef BOTMANAGER_H
#define BOTMANAGER_H

#include <memory>
#include "Session.h"
#include "Bot.h"
#include "UniqueId.h"
#include "Identity.h"

class BotManager
{
public:
    BotManager();
    ~BotManager();

    bool Initialize(uint32_t botId, const std::string &webId, const std::string &key, const std::string &passPhrase);
    std::uint32_t AddBot(std::uint32_t tableToJoin, const Identity &ident, std::uint16_t delay, const std::string &scriptFile);
    bool ConnectBot(uint32_t botId, const std::string &ip, std::uint16_t port);
    bool RemoveBot(std::uint32_t botid);
//    void ChangeBotIdentity(std::uint32_t uuid, const Identity &identity);
    void Close();
    void KillBots();
    bool JoinTable(std::uint32_t botId, std::uint32_t tableId);

private:
    class NetBot : private INetClient, private INetClientEvent
    {
    public:
        NetBot()
         : mSession(*this)
         , mBot(*this)
        {

        }

        ~NetBot() {
            mSession.Close();
        }


        Session mSession;
        Bot mBot;

    private:
        virtual bool Deliver(const Request &req) override {
            return mBot.Deliver(req);
        }

        virtual void Disconnected() override {

        }

        virtual void Send(uint32_t my_uid, const std::vector<Reply> &replies) override {
            mSession.Send(my_uid, replies);
        }

        virtual void ConnectToHost(const std::string &host, uint16_t tcp_port) override {
            mSession.ConnectToHost(host, tcp_port);
        }

        virtual void Disconnect() override {
            mSession.Disconnect();
        }

        virtual void Initialize(const std::string &webId, const std::string &key, const std::string &passPhrase)
        {
            mSession.Initialize(webId, key, passPhrase);
        }

    };

    // Bots management
    std::map<std::uint32_t, std::unique_ptr<NetBot>> mBots;
    std::mutex mMutex;
    UniqueId mBotsIds; ///< Internal management of bots only, not related to uuid used by the protocol
};

#endif // BOTMANAGER_H
