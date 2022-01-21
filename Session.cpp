#include "Session.h"
#include "Log.h"
#include "Protocol.h"

using namespace boost;

/*****************************************************************************/
Session::Session(INetClientEvent &client)
    : mListener(client)
    , mInitialized(false)
    , mTcpPort(0U)
    , resolver(io_context)
    , socket(io_context)
{

}
/*****************************************************************************/
void Session::Initialize(const std::string &webId, const std::string &key, const std::string &passPhrase)
{
    if (!mInitialized)
    {
        mWebId = webId;
        mPassPhrase = passPhrase;
        mProto.SetSecurity(key);
        mThread = std::thread(&Session::Run, this);
        mInitialized = true;
    }
}
/*****************************************************************************/
std::string Session::BuildConnectionPacket()
{
    return mProto.Build(0, Protocol::LOBBY_UID, mPassPhrase, mWebId);
}
/*****************************************************************************/
void Session::Send(uint32_t my_uid, const std::vector<Reply> &out)
{
    // Send all messages
    for (std::uint32_t i = 0U; i < out.size(); i++)
    {
        // To all indicated peers
        for (std::uint32_t j = 0U; j < out[i].dest.size(); j++)
        {
            SendToHost(mProto.Build(my_uid, out[i].dest[j], out[i].data.ToString()));
        }
    }
}
/*****************************************************************************/
void Session::SendToHost(const std::string &data)
{
//    std::stringstream dbg;
//    dbg << "Client sending packet: 0x" << std::hex << (int)cmd;
///    TLogNetwork(dbg.str());

    if (socket.is_open())
    {
        asio::async_write(socket,
            asio::buffer(data.c_str(),
              data.length()),
            [this](std::error_code ec, std::size_t /*length*/)
            {
              if (!ec)
              {
                 // okay
              }
              else
              {
                socket.close();
              }
            });
    }
    else
    {
        TLogNetwork("WARNING! try to send packet without any connection.");
    }
}
/*****************************************************************************/
bool Session::IsConnected()
{
    return socket.is_open();
}
/*****************************************************************************/
void Session::Disconnect()
{
    boost::system::error_code ec;
    socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    if (ec)
    {
        TLogError("[SESSION] Close error");
    }
    // Not necessary to close the socket
    // With asio, it may throw an exception
}
/*****************************************************************************/
void Session::ConnectToHost(const std::string &hostName, std::uint16_t port)
{
    Disconnect();
    mHostName = hostName;
    mTcpPort = port;

    // tcp::resolver::results_type& endpoints
    auto endpoints = resolver.resolve(hostName.c_str(), std::to_string(port));
    asio::async_connect(socket, endpoints,
        [this](boost::system::error_code error, asio::ip::tcp::endpoint)
        {
            if ((asio::error::eof == error) || (asio::error::connection_reset == error))
            {
                Disconnect();
            }
            else if (!error)
            {
                TLogInfo("Client " + mWebId + " connected");
                SendToHost(BuildConnectionPacket());
                ReadHeader();
            }
        }
    );
}
/*****************************************************************************/
void Session::ReadHeader()
{
    asio::async_read(socket,
        asio::buffer(mProto.Data(), PROTO_HEADER_SIZE),
        [this](std::error_code ec, std::size_t /*length*/)
        {
            if (!ec && mProto.ParseHeader(h))
            {
                ReadBody();
            }
            else
            {
                socket.close();
            }
        }
    );
}
/*****************************************************************************/
void Session::ReadBody()
{
    asio::async_read(socket,
        asio::buffer(mProto.Body(), h.BodyLength()),
        [this](std::error_code ec, std::size_t /*length*/)
        {
            if (!ec)
            {
                Request req;
                if (mProto.DecryptPayload(req.arg, h))
                {
//                    TLogNetwork("[SESSION] Found one packet with data: " + req.arg);
                    req.src_uuid = h.src_uid;
                    req.dest_uuid = h.dst_uid;
                    (void) mListener.Deliver(req);
                }
                ReadHeader();
            }
            else
            {
                socket.close();
            }
        }
    );
}
/*****************************************************************************/
void Session::Close()
{
    asio::post(io_context, [this]() { socket.close(); });
    io_context.stop();
    if (mThread.joinable())
    {
        mThread.join();
    }
    mInitialized = false;
}
/*****************************************************************************/
void Session::Run()
{
    asio::executor_work_guard<decltype(io_context.get_executor())> work{io_context.get_executor()};

    TLogInfo("Client " + mWebId + " started");
    io_context.run();

    TLogInfo("Client " + mWebId + " ended");
}
