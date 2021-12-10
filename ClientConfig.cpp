/*=============================================================================
 * TarotClub - ClientConfig.cpp
 *=============================================================================
 * Option file parameters management for the client configuration
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
#include "ClientConfig.h"
#include "Log.h"
#include "System.h"

static const std::string CLIENT_CONFIG_VERSION  = "5"; // increase the version to force any incompatible update in the file structure
static const std::uint16_t  DEFAULT_DELAY               = 500U;     // in ms
static const bool           AVATARS_DEF                 = true;
static const std::uint16_t  CLIENT_TIMER_DEF            = 1500U;
static const std::string    DEFAULT_CLIENT_CONFIG_FILE  = "tarotclub.json";


ClientOptions::ClientOptions()
{
    SetDefault();
}

void ClientOptions::SetDefault()
{
    autoPlay = false;
    timer = DEFAULT_DELAY;
    showAvatars = AVATARS_DEF;
    backgroundColor = "#004f00";
    language = 0;
    deckFilePath = "default";
    cardsOrder = "TCHDS";
    delayBeforeCleaning = CLIENT_TIMER_DEF;
    clickToClean = true;

    identity.username = "Fry";
    identity.avatar = ":/avatars/A02.png";
    identity.gender = Identity::cGenderMale;
}

/*****************************************************************************/
bool ClientConfig::Load(ClientOptions &options, const std::string &fileName)
{
    JsonValue json;

    bool ret = JsonReader::ParseFile(json, fileName);
    if (ret)
    {
        std::string stringval;
        std::int32_t intval;
        std::uint32_t unsignedVal;

        if (json.GetValue("version", stringval))
        {
            if (stringval == CLIENT_CONFIG_VERSION)
            {
                bool boolval;
                // The general strategy is to be tolerant on the values.
                // If they are not in the acceptable range, we set the default value
                // without throwing any error

                if (json.GetValue("show_avatars", boolval))
                {
                    options.showAvatars = boolval;
                }

                if (json.GetValue("background_color", stringval))
                {
                    options.backgroundColor = stringval;
                }

                if (json.GetValue("language", intval))
                {
                    options.language = intval;
                }

                if (json.GetValue("delay_between_players", unsignedVal))
                {
                    if (unsignedVal > 9000U)
                    {
                        unsignedVal = DEFAULT_DELAY;
                    }
                    options.timer = unsignedVal;
                }

                if (json.GetValue("delay_before_cleaning", options.delayBeforeCleaning))
                {
                    if (options.delayBeforeCleaning > 5000)
                    {
                        options.delayBeforeCleaning = 5000;
                    }

                    if (options.delayBeforeCleaning < 0)
                    {
                        options.delayBeforeCleaning = 0;
                    }
                }

                if (json.GetValue("click_to_clean", boolval))
                {
                    options.clickToClean = boolval;
                }

                if (json.GetValue("cards_order", stringval))
                {
                    options.cardsOrder = stringval;
                }

                // Player's identity
                if (json.GetValue("identity:name", stringval))
                {
                    options.identity.username = stringval;
                }
                if (json.GetValue("identity:avatar", stringval))
                {
                    options.identity.avatar = stringval;
                }
                if (json.GetValue("identity:gender", stringval))
                {
                    if (stringval == "female")
                    {
                        options.identity.gender = Identity::cGenderFemale;
                    }
                    else
                    {
                        options.identity.gender = Identity::cGenderMale;
                    }
                }
            }
            else
            {
                TLogError("Wrong client configuration file version");
                ret = false;
            }
        }
        else
        {
            TLogError("Cannot read client configuration file version");
            ret = false;
        }
    }
    else
    {
        TLogError("Cannot open client configuration file");
    }

    if (!ret)
    {
        // Overwrite old file with default value
        options.SetDefault();
        ret = Save(options, fileName);
    }

    options.loaded = true;
    return ret;
}
/*****************************************************************************/
bool ClientConfig::Save(ClientOptions &options, const std::string &fileName)
{
    bool ret = true;

    JsonObject json;

    json.AddValue("version", CLIENT_CONFIG_VERSION);
    json.AddValue("show_avatars", options.showAvatars);
    json.AddValue("background_color", options.backgroundColor);
    json.AddValue("language", options.language);
    json.AddValue("delay_between_players", options.timer);
    json.AddValue("delay_before_cleaning", options.delayBeforeCleaning);
    json.AddValue("click_to_clean", options.clickToClean);
    json.AddValue("cards_order", options.cardsOrder);

    JsonObject identity;
    identity.AddValue("name", options.identity.username);

    // Make sure to use unix path separator
    Util::ReplaceCharacter(options.identity.avatar, "\\", "/");

    identity.AddValue("avatar", options.identity.avatar);
    std::string text;
    if (options.identity.gender == Identity::cGenderMale)
    {
        text = "male";
    }
    else
    {
        text = "female";
    }
    identity.AddValue("gender", text);
    json.AddValue("identity", identity);

    if (!JsonWriter::SaveToFile(json, fileName))
    {
        ret = false;
        TLogError("Saving client's configuration failed.");
    }
    return ret;
}

std::string ClientConfig::DefaultConfigFile()
{
    return DEFAULT_CLIENT_CONFIG_FILE;
}

//=============================================================================
// End of file ClientConfig.cpp
//=============================================================================

