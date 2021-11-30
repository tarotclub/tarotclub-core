/*=============================================================================
 * TarotClub - Lobby.cpp
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

// C++ files
#include <sstream>
#include <memory>

// Tarot files
#include "Lobby.h"
#include "Network.h"
#include "PlayingTable.h"

// ICL files
#include "Log.h"
#include "JsonReader.h"
#include "JsonWriter.h"


/*****************************************************************************/
Lobby::Lobby(bool adminMode)
    : mInitialized(false)
    , mTableIds(Protocol::TABLES_UID, Protocol::TABLES_UID + Protocol::MAXIMUM_TABLES)
    , mUserIds(Protocol::USERS_UID, Protocol::MAXIMUM_USERS)
    , mAdminMode(adminMode)
    , mEvCounter(0U)
{

}
/*****************************************************************************/
Lobby::~Lobby()
{
    DeleteTables();
}
/*****************************************************************************/
void Lobby::DeleteTables()
{
    mTables.clear();
}
/*****************************************************************************/
void Lobby::Initialize(const std::string &name, const std::vector<std::string> &tables)
{
    mName = name;
    mEvCounter = 0U;

    for (std::uint32_t i = 0U; i < tables.size(); i++)
    {
        CreateTable(tables[i]);
    }
}
/*****************************************************************************/
void Lobby::RegisterListener(Observer<JsonValue> &obs)
{
    mSubject.Attach(obs);
}
/*****************************************************************************/
void Lobby::Send(const std::vector<Reply> &out)
{
    // Send all data
    for (std::uint32_t i = 0U; i < out.size(); i++)
    {
        std::string data = out[i].data.ToString();
        // To all indicated peers
        for (std::uint32_t j = 0U; j < out[i].dest.size(); j++)
        {
            std::uint32_t uuid = out[i].dest[j];
            if (mPeers.count(uuid) > 0)
            {
                mPeers[uuid]->Deliver(data);
            }
            else if (uuid == Protocol::LOBBY_UID)
            {
                // On broadcast à tout le monde

                for (auto & p : mPeers)
                {
                    p.second->Deliver(data);
                }
            }
            else
            {
                TLogError("[Server] Cannot find peer");
            }
        }
    }
}
/*****************************************************************************/
bool Lobby::Deliver(const Request &req)
{
    std::scoped_lock<std::mutex> lock(mNetMutex);
    std::vector<Reply> out;
    bool ret = true;
    JsonReader reader;
    JsonValue json;

    if (!reader.ParseString(json, req.arg))
    {
        TLogNetwork("Not a JSON data");
        return false;
    }

    // Warn every observer of that event
    mSubject.Notify(json);

    // Filter using the destination uuid (table or lobby?)
    if (mTableIds.IsTaken(req.dest_uuid))
    {
        // gets the table of the sender
        std::uint32_t tableId = mUsers.GetPlayerTable(req.src_uuid);
        if (tableId == req.dest_uuid)
        {
            // forward it to the suitable table PlayingTable
            for (auto &t : mTables)
            {
                if (t->GetId() == tableId)
                {
                    t->ExecuteRequest(req.src_uuid, req.dest_uuid, json, out);
                }
            }
        }
        else
        {
            ret = false;
            TLogNetwork("Packet received for an invalid table, or player is not connected to the table");
        }
    }
    else if (req.dest_uuid == Protocol::LOBBY_UID)
    {
        std::string cmd = json.FindValue("cmd").GetString();

        if (cmd == "ChatMessage")
        {
            // cmd, source, target, message
            std::uint32_t target = json.FindValue("target").GetInteger();
            out.push_back(Reply(target, json.GetObj()));
        }
        else if (cmd == "ReplyLogin")
        {
            Users::Entry entry;

            FromJson(entry.identity, json.GetObj());

            if (mUserIds.IsTaken(req.src_uuid))
            {
                // Ok, move the user into the main list
                // User belong to the lobby
                entry.uuid = req.src_uuid;
                entry.tableId = Protocol::LOBBY_UID;
                if (mUsers.AddEntry(entry))
                {
                    // Create a list of tables available on the server
                    JsonObject reply;
                    JsonArray tables;

                    for (const auto & t : mTables)
                    {
                        JsonObject table;
                        table.AddValue("name", t->GetName());
                        table.AddValue("uuid", t->GetId());
                        tables.AddValue(table);
                    }

                    reply.AddValue("cmd", "AccessGranted");
                    reply.AddValue("tables", tables);

                    // Add the list of players
                    std::vector<Users::Entry> users = mUsers.Get(Protocol::LOBBY_UID);
                    JsonArray array;

                    for (uint32_t i = 0U; i < users.size(); i++)
                    {
                        array.AddValue(PlayerStatus(users[i].uuid));
                    }
                    reply.AddValue("players", array);

                    // Send to the player the final step of the login process
                    out.push_back(Reply(req.src_uuid, reply));

                    // Send the information for all other users
                    SendPlayerEvent(req.src_uuid, "New", out);
                }
                else
                {
                    // Add failed, probably because of the nickname
                    // FIXME: manage the case: Lobby full
                    Error(cErrorNickNameUsed, req.src_uuid, out);
                }
            }
            else
            {
                TLogNetwork("Unknown uuid");
            }
        }
        else if (cmd == "RequestJoinTable")
        {
            std::uint32_t tableId = json.FindValue("table_id").GetInteger();

            // A user can join a table if he is _NOT_ already around a table
            if (mUsers.GetPlayerTable(req.src_uuid) == Protocol::LOBBY_UID)
            {
                Place assignedPlace;
                std::uint8_t nbPlayers = 0U;
                bool foundTable = false;

                // Forward it to the table PlayingTable
                JsonObject tableContext;
                for (auto &t : mTables)
                {
                    if (t->GetId() == tableId)
                    {
                        foundTable = true;
                        assignedPlace = t->AddPlayer(req.src_uuid, nbPlayers);
                        tableContext = t->GetContext();
                        break;
                    }
                }

                if (!foundTable)
                {
                    Error(cErrorTableIdUnknown, req.src_uuid, out);
                }
                else if (assignedPlace.IsValid())
                {
                    mUsers.SetPlayingTable(req.src_uuid, tableId, assignedPlace);

                    JsonObject reply;

                    reply.AddValue("cmd", "ReplyJoinTable");
                    reply.AddValue("table_id", tableId);
                    reply.AddValue("place", assignedPlace.ToString());
                    reply.AddValue("size", nbPlayers);
                    // On envoie toujours tout le contexte de la partie en cours de la table
                    reply.AddValue("context", tableContext);
                    out.push_back(Reply(req.src_uuid, reply));

                    SendPlayerEvent(req.src_uuid, "JoinTable", out);
                }
                else
                {
                    Error(cErrorFull, req.src_uuid, out);
                }
            }
        }
        else if (cmd == "RequestQuitTable")
        {
            std::uint32_t tableId = json.FindValue("table_id").GetInteger();

            if (mUsers.GetPlayerTable(req.src_uuid) == tableId)
            {
                RemovePlayerFromTable(req.src_uuid, tableId, out);
            }
        }
        else if (cmd == "RequestChangeNickname")
        {
            std::string nickname = json.FindValue("nickname").GetString();


            if (mUsers.ChangeNickName(req.src_uuid, nickname))
            {
                std::vector<std::uint32_t> peers;
                peers.push_back(req.src_uuid);

                // Send to all the list of players and the event
                SendPlayerEvent(req.src_uuid, "Nick", out);
            }
            else
            {
                Error(cErrorNickNameUsed, req.src_uuid, out);
            }
        }
        else
        {
            ret = false;
            TLogNetwork("Lobby received a bad packet");
        }
    }
    else
    {
        ret = false;
        std::stringstream ss;
        ss << "Packet destination must be the table or the lobby, nothing else; received UID: " << req.dest_uuid;
        TLogNetwork(ss.str());
    }

    Send(out);

    // Also send every output packet to listeners
    for (auto &reply : out)
    {
        mSubject.Notify(reply.data);
    }

    return ret;
}
/*****************************************************************************/
std::uint32_t Lobby::GetNumberOfPlayers()
{
    return mUsers.Size();
}
/*****************************************************************************/
uint32_t Lobby::GetNumberOfTables()
{
    return mTables.size();
}
/*****************************************************************************/
uint32_t Lobby::AddUser(PeerPtr peer)
{
    std::scoped_lock<std::mutex> lock(mNetMutex);
    std::uint32_t uuid = mUserIds.TakeId();
    mPeers[uuid] = peer;
    return uuid;
}
/*****************************************************************************/
void Lobby::RemoveUser(uint32_t uuid)
{
    std::scoped_lock<std::mutex> lock(mNetMutex);
    std::vector<Reply> out;
    std::uint32_t tableId = mUsers.GetPlayerTable(uuid);
    if (tableId != Protocol::LOBBY_UID)
    {
        // First, remove the player from the table
        RemovePlayerFromTable(uuid, tableId, out);
    }
//    // Generate qui event
//    SendPlayerEvent(uuid, "Quit", out);

    // Remove the player from the lobby list
    mUsers.Remove(uuid);
    // Free the ID
    mUserIds.ReleaseId(uuid);
}
/*****************************************************************************/
void Lobby::RemoveAllUsers()
{
    mUsers.Clear();
}
/*****************************************************************************/
std::uint32_t Lobby::CreateTable(const std::string &tableName, const Tarot::Game &game)
{
    std::uint32_t id = mTableIds.TakeId();

    if (id > 0U)
    {
        std::stringstream ss;
        ss << "Creating table \"" << tableName << "\": id=" << id << std::endl;
        TLogInfo(ss.str());

        auto table = std::make_unique<PlayingTable>();
        table->SetId(id);
        table->SetName(tableName);
        table->SetAdminMode(mAdminMode);
        table->SetupGame(game);
        table->Initialize();
        table->CreateTable(4U);
        mTables.push_back(std::move(table));
    }
    else
    {
        TLogError("Cannot create table: maximum number of tables reached.");
    }
    return id;
}
/*****************************************************************************/
bool Lobby::DestroyTable(std::uint32_t id)
{
    bool ret = false;

    auto it = find_if(mTables.begin(), mTables.end(), [&](std::unique_ptr<PlayingTable>& t){ return t->GetId() == id; });

    if (it != mTables.end())
    {
        ret = true;
        mTables.erase(it);
    }

    mTableIds.ReleaseId(id);
    return ret;
}
/*****************************************************************************/
void Lobby::Error(std::uint32_t error, std::uint32_t dest_uuid, std::vector<Reply> &out)
{
    static const char* errors[] { "Table is full", "Nickname already used", "Unknown table ID" };
    JsonObject reply;

    reply.AddValue("cmd", "Error");
    reply.AddValue("code", error);

    if (error < (sizeof(errors)/sizeof(errors[0])))
    {
        reply.AddValue("reason", errors[error]);

        out.push_back(Reply(dest_uuid, reply));
    }
}
/*****************************************************************************/
void Lobby::RemovePlayerFromTable(std::uint32_t uuid, std::uint32_t tableId, std::vector<Reply> &out)
{
    // Forward it to the table PlayingTable
    for (auto &t : mTables)
    {
        if (t->GetId() == tableId)
        {
            // Remove the player from the table, if we are in game, then all are removed
            t->RemovePlayer(uuid);
        }
    }

    // Warn one or more player that they are kicked from the table
    std::vector<std::uint32_t> peers;
    // Remove only one player
    peers.push_back(uuid);

    for (std::uint32_t i = 0U; i < peers.size(); i++)
    {
        if (mUsers.IsHere(peers[i]))
        {
            mUsers.SetPlayingTable(peers[i], 0U, Place(Place::NOWHERE)); // refresh lobby state
        }

        SendPlayerEvent(peers[i], "LeaveTable", out);
    }
}
/*****************************************************************************/
JsonObject Lobby::PlayerStatus(std::uint32_t uuid)
{
    JsonObject obj;
    Users::Entry entry;

    /**
    {
        "uuid": 37,
        "nickname": "Belegar",
        "avatar": "http://wwww.fdshjkfjds.com/moi.jpg",
        "gender": "Male",
        "table": 0,
        "place": "South"
    }
    */
    if (mUsers.GetEntry(uuid, entry))
    {
        obj.AddValue("uuid", uuid);
        obj.AddValue("table", entry.tableId);
        obj.AddValue("place", entry.place.ToString());
        ToJson(entry.identity, obj);
    }

    return obj;
}
/*****************************************************************************/
void Lobby::SendPlayerEvent(std::uint32_t uuid, const std::string &event, std::vector<Reply> &out)
{
    Users::Entry entry;
    if (mUsers.GetEntry(uuid, entry))
    {
        /**
            "cmd": "Event",
            "type": "Nick",
            "counter": 3590896,
            "player": { }
        */
        JsonObject obj;

        mEvCounter++;
        obj.AddValue("cmd", "LobbyEvent");
        obj.AddValue("type", event);
        obj.AddValue("counter", mEvCounter);
        obj.AddValue("player", PlayerStatus(uuid));

        out.push_back(Reply(mUsers.GetAll(), obj));
    }
    else
    {
        TLogError("Cannot find player, should be in the list!");
    }
}
/*****************************************************************************/
std::string Lobby::GetTableName(const std::uint32_t tableId)
{
    std::string name = "error_table_not_found";

    // Forward it to the table PlayingTable
    for (const auto &t : mTables)
    {
        if (t->GetId() == tableId)
        {
            name = t->GetName();
        }
    }

    return name;
}
