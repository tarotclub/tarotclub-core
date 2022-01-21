/*=============================================================================
 * TarotClub - Server.h
 *=============================================================================
 * Management of raw network protocol / ciphered layer
 *=============================================================================
 * TarotClub ( http://www.tarotclub.fr ) - This file is part of TarotClub
 * Copyright (C) 2003-2999 - Anthony Rabine
 * anthony@tarotclub.fr
 *
 * TarotClub is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TarotClub is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TarotClub.  If not, see <http://www.gnu.org/licenses/>.
 *
 *=============================================================================
 */

#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include "Lobby.h"
#include "Protocol.h"
#include <mutex>
#include <memory>
#include <boost/asio.hpp>
#include "IService.h"
#include "IServer.h"

using namespace boost;

class PeerSession : public Peer, public std::enable_shared_from_this<PeerSession>
{
public:
    PeerSession(boost::asio::ip::tcp::socket socket, std::shared_ptr<Lobby> lobby, boost::asio::io_context& io_context);

    void Start();
    virtual void Deliver(const std::string &data) override;

private:
    std::uint32_t uuid = 0;
    Protocol mProto;
    bool mIsPending = true;
    boost::asio::ip::tcp::socket socket_;
    std::shared_ptr<Lobby> mLobby;

    Lobby::Security sec;
    Protocol::Header h;
    boost::asio::io_context::strand read;

    void ReadHeader();
    void DoWrite(const std::string &d);
    void ReadBody();
    void HandleBody();
};


class Server : public std::enable_shared_from_this<IServer>, public IServer
{
public:
    Server(asio::io_context& io_context, ServerOptions &options);
    ~Server();

    // From IServer
    void AddClient(const std::string &webId, const std::string &gek, const std::string &passPhrase);
    ServerOptions &GetOptions() { return mOptions; }

    void AddService(std::shared_ptr<IService> svc);

private:
    ServerOptions &mOptions;
    std::shared_ptr<Lobby> mLobby;
    std::vector<std::shared_ptr<IService>> mServices;

    // Net stuff
    asio::ip::tcp::acceptor acceptor_;
    asio::ip::tcp::socket socket_;
    asio::io_context &context;

    void Accept();
};


#endif // SERVER_H

//=============================================================================
// End of file Server.h
//=============================================================================
