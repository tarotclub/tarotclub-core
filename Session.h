#ifndef SESSION_H
#define SESSION_H

#include "Network.h"
#include "ThreadQueue.h"
#include <boost/asio.hpp>
#include "Protocol.h"

class Session
{
public:
    enum Command
    {
        NO_CMD,
        START,
        EXIT
    };

    explicit Session(INetClientEvent &client);

    void Initialize(const std::string &webId, const std::string &key, const std::string &passPhrase);
    std::string BuildConnectionPacket();
    void Send(uint32_t my_uid, const std::vector<Reply> &out);
    bool IsConnected();
    void Disconnect();
    void ConnectToHost(const std::string &hostName, std::uint16_t port);
    void Close();
private:
    INetClientEvent &mListener;

    std::thread mThread;
    bool        mInitialized;
    std::string mHostName;
    std::uint16_t mTcpPort;
    Protocol mProto;
    bool m_isConnected{false};

    Protocol::Header h;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ip::tcp::socket socket;


    std::string mWebId;
    std::string mPassPhrase;

    void SendToHost(const std::string &data);
    void Run();
    void ReadHeader();
    void ReadBody();
};


#endif // SESSION_H
