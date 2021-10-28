/*=============================================================================
 * TarotClub - Lobby.h
 *=============================================================================
 * Central meeting point of a server to chat and join game tables
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

#ifndef LOBBY_H
#define LOBBY_H

#include <memory>
#include <vector>

// Tarot files
#include "Protocol.h"
#include "PlayingTable.h"
#include "Users.h"
#include "Network.h"

// ICL files
#include "Observer.h"

/*****************************************************************************/
class Peer
{
public:
  virtual ~Peer() {}
  virtual void Deliver(const std::string &data) = 0;
};

typedef std::shared_ptr<Peer> PeerPtr;

/*****************************************************************************/
class Lobby
{

public:
    struct Security {
        std::string webId;
        std::string gek;
        std::string passPhrase;
    };

    static const std::uint32_t cErrorFull           = 0U;
    static const std::uint32_t cErrorNickNameUsed   = 1U;
    static const std::uint32_t cErrorTableIdUnknown = 2U;

    Lobby(bool adminMode = false);
    ~Lobby();

    void Initialize(const std::string &name, const std::vector<std::string> &tables);
    std::string GetName() { return mName; }
    void RegisterListener(Observer<JsonValue> &obs);

    // Users management
    std::uint32_t GetNumberOfPlayers();
    std::uint32_t GetNumberOfTables();
    void RemoveAllUsers();

    // Tables management
    std::uint32_t CreateTable(const std::string &tableName, const Tarot::Game &game = Tarot::Game());
    bool DestroyTable(std::uint32_t id);
    void DeleteTables();

    bool Deliver(const Request &req);
    std::uint32_t AddUser(PeerPtr peer);
    void RemoveUser(std::uint32_t uuid);


    void AddAllowedClient(const std::string &webId, const std::string &gek, const std::string &passPhrase)
    {
        std::scoped_lock<std::mutex> lock(mNetMutex);
        Security sec;
        sec.webId = webId;
        sec.gek = gek;
        sec.passPhrase = passPhrase;
        mAllowedClients[webId] = sec;
    }

    bool GetSecurity(const std::string &playerId, Security &sec)
    {
        std::scoped_lock<std::mutex> lock(mNetMutex);
        bool success = false;
        if (mAllowedClients.count(playerId) > 0)
        {
            TLogNetwork("Found webId");
            sec = mAllowedClients[playerId];
            success = true;
        }
        return success;
    }

private:
    bool mInitialized;
    std::vector<std::unique_ptr<PlayingTable>> mTables;
    UniqueId    mTableIds;
    UniqueId    mUserIds;

    Users       mUsers;
    std::string mName;
    bool mAdminMode;
    std::uint32_t mEvCounter;
    Subject<JsonValue> mSubject;
    std::mutex  mNetMutex;

    std::map<std::uint32_t, PeerPtr> mPeers; // uuid <--> GameSession

    // key = playerId
    std::map<std::string, Security> mAllowedClients; // allowed peers on this server

    std::string GetTableName(const std::uint32_t tableId);
    void RemovePlayerFromTable(std::uint32_t uuid, std::uint32_t tableId, std::vector<Reply> &out);
    void Error(std::uint32_t error, std::uint32_t dest_uuid, std::vector<Reply> &out);
    JsonObject PlayerStatus(std::uint32_t uuid);
    void SendPlayerEvent(std::uint32_t uuid, const std::string &event, std::vector<Reply> &out);
    void Send(const std::vector<Reply> &out);
};

#endif // LOBBY_H
