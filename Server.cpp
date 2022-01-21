/*=============================================================================
 * TarotClub - Server.cpp
 *=============================================================================
 * Management of raw network protocol
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

#include <cctype>
#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include "Util.h"
#include "Server.h"
#include "System.h"
#include "Base64Util.h"

using namespace boost;

PeerSession::PeerSession(asio::ip::tcp::socket socket, std::shared_ptr<Lobby> lobby, asio::io_context &io_context)
    : socket_(std::move(socket))
    , mLobby(lobby)
    , read(io_context)
{
}

void PeerSession::Start()
{
    TLogInfo("[SERVER] New peer");
    uuid = mLobby->AddUser(shared_from_this());
    ReadHeader();
}

void PeerSession::Deliver(const std::string &data)
{
    // On génère une trame uniquement pour ce client, chiffrée avec ses clés
    // La source est toujours le lobby, et la destination notre peer
    DoWrite(mProto.Build(Protocol::LOBBY_UID, uuid, data));
}

void PeerSession::ReadHeader()
{
    socket_.async_receive(asio::buffer(mProto.Data(), PROTO_HEADER_SIZE),asio::bind_executor(read,
          [&] (boost::system::error_code error, std::size_t /*length*/)
     {
          if ((asio::error::eof == error) || (asio::error::connection_reset == error))
          {
              boost::system::error_code ec;
              socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
              if (ec)
              {
                  TLogError("[SERVER] Close error");
              }
              TLogNetwork("[SERVER] Peer disconnected");
              mLobby->RemoveUser(uuid);
          }
          else if (mProto.ParseHeader(h))
          {
              ReadBody();
          }
          else
          {
              ReadHeader();
          }
     }));
}

void PeerSession::ReadBody()
{
    socket_.async_receive(asio::buffer(mProto.Body(), h.BodyLength()),asio::bind_executor(read,
          [&] (std::error_code error, std::size_t /*length*/)
     {
         if(!error)
         {
            HandleBody();
            ReadHeader();
         }
     }));
}

void PeerSession::HandleBody()
{
    if (mIsPending)
    {
        mProto.ParsePrefix(h);
        // 2. set player security key
        // prefix contains webId
        // à l'aide de cette information, on va récupérer la clé associée à ce joueur
        if (mLobby->GetSecurity(h.prefix, sec))
        {
            mProto.SetSecurity(sec.gek);
        }
        else
        {
            TLogError("[SESSION] Security not found!");
        }
    }

    Request req;
    if (mProto.DecryptPayload(req.arg, h))
    {
//        TLogNetwork("[SESSION] Found one packet with data: " + req.arg);
        req.src_uuid = h.src_uid;
        req.dest_uuid = h.dst_uid;

        if (mIsPending)
        {
            if (req.arg == sec.passPhrase)
            {
                mIsPending = false;

                // Ask player to log in lobby with identity
                JsonObject json;
                json.AddValue("cmd", "RequestLogin");
                json.AddValue("uuid", uuid);

                Deliver(json.ToString());
            }
            else
            {
                TLogNetwork("[SERVER] Bad pass phrase, expected: " + sec.passPhrase + " decoded: " + req.arg);
                mLobby->RemoveUser(uuid);
            }
        }
        else
        {
            req.src_uuid = h.src_uid;
            req.dest_uuid = h.dst_uid;
            mLobby->Deliver(req);
        }
    }
    else
    {
        TLogNetwork("[SERVER] Decrypt problem");
        mLobby->RemoveUser(uuid);
    }
}

void PeerSession::DoWrite(const std::string &d)
{
    asio::async_write(socket_, asio::buffer(d.c_str(), d.size()),
                      [](std::error_code ec, std::size_t /*length*/)
    {
        if (!ec)
        {
        }
    });
}

Server::Server(asio::io_context &io_context, ServerOptions &options)
    : mOptions(options)
    , mLobby(std::make_shared<Lobby>())
    , acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), options.game_tcp_port))
    , socket_(io_context)
    , context(io_context)
{
    mLobby->CreateTable("Local game");
    Accept();
}

Server::~Server()
{
    for (auto & s : mServices)
    {
        s->Stop();
    }
}

void Server::AddClient(const std::string &webId, const std::string &gek, const std::string &passPhrase)
{
    mLobby->AddAllowedClient(webId, gek, passPhrase);
}

void Server::AddService(std::shared_ptr<IService> svc)
{
    mServices.push_back(svc);
    svc->Initialize(shared_from_this(), mLobby);
}

void Server::Accept()
{
    acceptor_.async_accept(socket_,
                           [this](std::error_code ec)
    {
        if (!ec)
        {
            std::make_shared<PeerSession>(std::move(socket_), mLobby, context)->Start();
        }

        Accept();
    });
}


//=============================================================================
// End of file Server.cpp
//=============================================================================
