/*=============================================================================
 * TarotClub - ServerConfig.cpp
 *=============================================================================
 * Network server parameters
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

#include <sstream>
#include "JsonReader.h"
#include "JsonWriter.h"
#include "ServerConfig.h"
#include "Log.h"
#include "System.h"

static const std::string SERVER_CONFIG_VERSION  = "8"; // increase the version to force any incompatible update in the file structure
const std::string ServerConfig::DEFAULT_SERVER_CONFIG_FILE  = "tcds.json";
const std::string ServerConfig::DEFAULT_SERVER_NAME = "server1";


/*


                // Load bot configuration
                for (std::uint32_t i = 1U; i < 4U; i++)
                {
                    Place p(i);
                    std::string botPos = p.ToString();
                    if (json.GetValue(botPos + ":name", stringval))
                    {
                        mOptions.bots[p].identity.nickname = stringval;
                    }
                    if (json.GetValue(botPos + ":avatar", stringval))
                    {
                        mOptions.bots[p].identity.avatar = stringval;
                    }
                    if (json.GetValue(botPos + ":gender", stringval))
                    {
                        if (stringval == "female")
                        {
                            mOptions.bots[p].identity.gender = Identity::cGenderFemale;
                        }
                        else
                        {
                            mOptions.bots[p].identity.gender = Identity::cGenderMale;
                        }

                    }

                    if (json.GetValue(botPos + ":bot_file_path", stringval))
                    {
                        mOptions.bots[p].scriptFilePath = stringval;
                    }
                }

                JsonValue servers = json.FindValue("servers");
                mOptions.serverList.clear();
                for (JsonArray::iterator iter = servers.GetArray().begin(); iter != servers.GetArray().end(); ++iter)
                {
                    if (iter->IsObject())
                    {
                        ServerInfo srv;

                        bool ret = iter->GetValue("address", srv.address);
                        if (ret) { ret = iter->GetValue("game_port", srv.game_tcp_port); }

                        if (ret)
                        {
                            mOptions.serverList.push_back(srv);
                        }
                    }
                }



                //----------------------------   SAVE

    // List of bots
    for (std::uint32_t i = 1U; i < 4U; i++)
    {
        Place p(i);
        JsonObject botObj;
        botObj.AddValue("name", mOptions.bots[p].identity.nickname);

        // Make sure to use unix path separator
        Util::ReplaceCharacter(mOptions.bots[p].identity.avatar, "\\", "/");

        botObj.AddValue("avatar", mOptions.bots[p].identity.avatar);
        std::string text;
        if (mOptions.bots[p].identity.gender == Identity::cGenderMale)
        {
            text = "male";
        }
        else
        {
            text = "female";
        }

        botObj.AddValue("gender", text);

        // Make sure to use unix path separator
        Util::ReplaceCharacter(mOptions.bots[p].scriptFilePath, "\\", "/");
        botObj.AddValue("bot_file_path", mOptions.bots[p].scriptFilePath);
        json.AddValue(p.ToString(), botObj);
    }

    // List of servers
    JsonArray servers; // = json.CreateArrayPair("servers");
    for (std::uint32_t i = 0U; i < mOptions.serverList.size(); i++)
    {
        ServerInfo inf =  mOptions.serverList[i];
        JsonObject srv; // = servers->CreateObject();
        srv.AddValue("address", inf.address);
        srv.AddValue("game_port", inf.game_tcp_port);
        servers.AddValue(srv);
    }
    json.AddValue("servers", servers);





    //----------------------------  default

    const ServerInfo DefaultServers[1U] = {
        {"tarotclub.fr", 4269 }
    };

    const std::uint32_t NumberOfServers = (sizeof(DefaultServers) / sizeof(ServerInfo));


    for (std::uint32_t i = 0U; i < NumberOfServers; i++)
    {
        opt.serverList.push_back(DefaultServers[i]);
    }

    opt.bots[Place(Place::WEST)].identity.nickname = "Leela";
    opt.bots[Place(Place::WEST)].identity.avatar   = ":/avatars/FD05.png";
    opt.bots[Place(Place::WEST)].identity.gender   = Identity::cGenderFemale;
    opt.bots[Place(Place::WEST)].scriptFilePath    = System::ScriptPath();

    opt.bots[Place(Place::NORTH)].identity.nickname = "Bender";
    opt.bots[Place(Place::NORTH)].identity.avatar  = ":/avatars/N03.png";
    opt.bots[Place(Place::NORTH)].identity.gender  = Identity::cGenderMale;
    opt.bots[Place(Place::NORTH)].scriptFilePath   = System::ScriptPath();

    opt.bots[Place(Place::EAST)].identity.nickname = "Amy";
    opt.bots[Place(Place::EAST)].identity.avatar   = ":/avatars/FE02.png";
    opt.bots[Place(Place::EAST)].identity.gender   = Identity::cGenderFemale;
    opt.bots[Place(Place::EAST)].scriptFilePath    = System::ScriptPath();



*/


/*****************************************************************************/
ServerConfig::ServerConfig()
    : mOptions(GetDefault())
    , mLoaded(false)
{

}
/*****************************************************************************/
ServerOptions &ServerConfig::GetOptions()
{
    if (!mLoaded)
    {
        mOptions = GetDefault();
    }
    return mOptions;
}
/*****************************************************************************/
void ServerConfig::SetOptions(const ServerOptions &newOptions)
{
    mOptions = newOptions;
}
/*****************************************************************************/
bool ServerConfig::Load(const std::string &fileName)
{
    JsonValue json;

    bool ret = JsonReader::ParseFile(json, fileName);
    if (ret)
    {
        TLogInfo("[SERVER] Opening configuration file: " + fileName);
        std::string stringVal;
        if (json.GetValue("version", stringVal))
        {
            if (stringVal == SERVER_CONFIG_VERSION)
            {
                // The general strategy is to be tolerant on the values.
                // If they are not in the acceptable range, we set the default value
                // without throwing any error
                std::uint32_t unsignedVal;
                bool boolVal;
                if (json.GetValue("game_tcp_port", unsignedVal))
                {
                    mOptions.game_tcp_port = unsignedVal;
                }

                if (json.GetValue("websocket_tcp_port", unsignedVal))
                {
                    mOptions.websocket_tcp_port = unsignedVal;
                }

                if (json.GetValue("console_tcp_port", unsignedVal))
                {
                    mOptions.console_tcp_port = unsignedVal;
                }

                if (json.GetValue("lobby_max_conn", unsignedVal))
                {
                    mOptions.lobby_max_conn = unsignedVal;
                }

                if (json.GetValue("local_host_only", boolVal))
                {
                    mOptions.localHostOnly = boolVal;
                }

                if (json.GetValue("name", stringVal))
                {
                    mOptions.name = stringVal;
                }

                if (json.GetValue("token", stringVal))
                {
                    mOptions.token = stringVal;
                }

                mOptions.tables.clear();
                JsonValue tables = json.FindValue("tables");

                for (JsonArray::iterator iter = tables.GetArray().begin(); iter != tables.GetArray().end(); ++iter)
                {
                    if (iter->IsString())
                    {
                        mOptions.tables.push_back(iter->GetString());
                    }
                }
            }
            else
            {
                TLogError("Wrong server configuration file version");
                ret = false;
            }
        }
        else
        {
            TLogError("Cannot read server configuration file version");
            ret = false;
        }
    }
    else
    {
        TLogError("Cannot open server configuration file" + fileName);
    }

    if (!ret)
    {
        // Overwrite old file with default value
        mOptions = GetDefault();
        ret = Save(fileName);
    }

    mLoaded = true;
    return ret;
}
/*****************************************************************************/
bool ServerConfig::Save(const std::string &fileName)
{
    bool ret = true;

    JsonObject json;

    json.AddValue("version", SERVER_CONFIG_VERSION);
    json.AddValue("game_tcp_port", mOptions.game_tcp_port);
    json.AddValue("websocket_tcp_port", mOptions.websocket_tcp_port);
    json.AddValue("console_tcp_port", mOptions.console_tcp_port);
    json.AddValue("lobby_max_conn", mOptions.lobby_max_conn);
    json.AddValue("local_host_only", mOptions.localHostOnly);
    json.AddValue("name", mOptions.name);
    json.AddValue("token", mOptions.token);

    JsonArray tables;
    for (std::vector<std::string>::iterator iter =  mOptions.tables.begin(); iter !=  mOptions.tables.end(); ++iter)
    {
        tables.AddValue(*iter);
    }
    json.AddValue("tables", tables);

    if (!JsonWriter::SaveToFile(json, fileName))
    {
        ret = false;
        TLogError("Saving server's configuration failed.");
    }
    return ret;
}
/*****************************************************************************/
ServerOptions ServerConfig::GetDefault()
{
    ServerOptions opt;

    opt.game_tcp_port       = DEFAULT_GAME_TCP_PORT;
    opt.console_tcp_port    = DEFAULT_CONSOLE_TCP_PORT;
    opt.websocket_tcp_port  = DEFAULT_WEBSOCKET_TCP_PORT;
    opt.lobby_max_conn      = DEFAULT_LOBBY_MAX_CONN;
    opt.localHostOnly       = false;
    opt.name                = DEFAULT_SERVER_NAME;
    opt.tables.push_back("Table 1"); // default table name (one table minimum)
    opt.token               = Util::GenerateRandomString(20);

    return opt;
}

//=============================================================================
// End of file ServerConfig.cpp
//=============================================================================
