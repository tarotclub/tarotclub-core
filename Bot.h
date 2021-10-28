/*=============================================================================
 * TarotClub - Bot.h
 *=============================================================================
 * Bot class player. Uses a Script Engine to execute IA scripts
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

#ifndef BOT_H
#define BOT_H

// Tarot files
#include "Network.h"
#include "PlayerContext.h"

// ICL files
#include "JSEngine.h"
#include "Log.h"

/*****************************************************************************/
class Bot
{

public:
    Bot(INetClient &net);
    ~Bot();

    bool Deliver(const Request &req);

    std::uint32_t GetUuid() { return mCtx.mMyself.uuid; }
    std::uint32_t GetCurrentTable() { return mCtx.mMyself.tableId; }
    std::string GetDeck() { return mCtx.mDeck.ToString(); }
    Place GetPlace() { return mCtx.mMyself.place; }

    void SetTimeBeforeSend(std::uint16_t t);
    void ChangeNickname(const std::string &nickname, std::vector<Reply> &out);
    void SetAiScript(const std::string &path);
    void SetTableToJoin(std::uint32_t table) { mCtx.mTableToJoin = table; }
    void SetUuid(std::uint32_t uuid) { mCtx.mMyself.uuid = uuid; }
    void SetIdentity(const Identity &identity);

private:
    INetClient &mNet;
    PlayerContext mCtx;
    std::uint16_t  mTimeBeforeSend;
    JSEngine mBotEngine;
    std::string mScriptPath;
    std::vector<Reply> mNetReplies;

    bool InitializeScriptContext();
    void StartDeal();
    void RequestBid(std::vector<Reply> &out);
    void AskForHandle(std::vector<Reply> &out);
    void ShowHandle();
    void BuildDiscard(std::vector<Reply> &out);
    void NewGame();
    void ShowCard();
    void PlayCard(std::vector<Reply> &out);
    void HandleRequest(const Request &req);
};

#endif // BOT_H

//=============================================================================
// End of file Bot.h
//=============================================================================
