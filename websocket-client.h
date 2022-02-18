#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <cstdlib>
#include <cstring>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "openssl/ssl3.h"

class WebSocketClient
{
public:
    class IReadHandler
    {
    public:
        ~IReadHandler() {}
        virtual void OnWsData(const std::string &data) = 0;
    };

    enum State {
        STATE_NO_ERROR,
        STATE_NO_SESSION,
        STATE_CONNECT,
        STATE_READ,
        STATE_RESOLVE,
        STATE_WRITE,
        STATE_SSL,
        STATE_WS_HANDSHAKE,
        STATE_CLOSE,
        STATE_UNKNOWN,
    };

    WebSocketClient(IReadHandler &handler);

    std::string Run(const std::string &host, const std::string &port);
    void Send(const std::string &message);
    void Close();
    bool IsConnected();
    State GetState();
private:
    IReadHandler &mReadHandler;

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // Sends a WebSocket message and prints the response
    class session : public std::enable_shared_from_this<session>
    {
        boost::asio::ip::tcp::resolver resolver_;
        boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> ws_;
        boost::beast::flat_buffer buffer_;
        std::string host_;
        std::string port_;

    public:
        // Resolver and socket require an io_context
        explicit session(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, IReadHandler &handler);
        // Start the asynchronous operation
        void Run(const std::string &host, const std::string &port);
        void Close();
        void Send(const std::string &message);
        bool IsConnected() const { return mConnected; }
        State GetState() { return mState; }

    private:
        IReadHandler &mReadHandler;
        bool mConnected = false;
        State mState = STATE_NO_ERROR;

        void on_resolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results);
        void on_connect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type ep);
        void on_ssl_handshake(boost::beast::error_code ec);
        void on_handshake(boost::beast::error_code ec);
        void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
        void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
        void on_close(boost::beast::error_code ec);
        void on_failure(boost::beast::error_code ec, State error);
        void still_connected();

    };

    std::shared_ptr<session> mSession;

};

#endif // WEBSOCKET_CLIENT_H
