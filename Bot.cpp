/*=============================================================================
 * TarotClub - Bot.cpp
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

#include <sstream>
#include <thread>
#include <chrono>
#include "Bot.h"
#include "Log.h"
#include "Util.h"
#include "System.h"
#include "JsonReader.h"
#include "Zip.h"

/*****************************************************************************/
Bot::Bot(INetClient &net)
    : mNet(net)
    , mTimeBeforeSend(0U)
{

}
/*****************************************************************************/
Bot::~Bot()
{

}
/*****************************************************************************/
bool Bot::Deliver(const Request &req)
{
    bool ret = true;
    HandleRequest(req);
    return ret;
}
/*****************************************************************************/
// Callback qui provien du décodeur
// Il ajoute des réponses à envoyer au network si besoin
void Bot::HandleRequest(const Request &req)
{
    JsonObject json;
    std::string ev;

    if (mCtx.Decode(req, json))
    {
        ev = json.GetValue("cmd").GetString();
    }

    if (!mCtx.mMyself.IsConnected())
    {
        if (ev == "RequestLogin")
        {
            mCtx.DecodeRequestLogin(json);
            mCtx.BuildReplyLogin(mNetReplies);
        }
    }
    else if (ev == "AccessGranted")
    {
        mCtx.DecodeAccessGranted(json);
        // As soon as we have entered into the lobby, join the assigned table
        mCtx.BuildJoinTable(Protocol::TABLES_UID, mNetReplies);
    }
    else if (ev == "ReplyJoinTable")
    {
        mCtx.DecodeReplyJoinTable(json);
        mCtx.Sync(Engine::WAIT_FOR_PLAYERS, mNetReplies);
    }
    else if (ev == "NewDeal")
    {
        mCtx.DecodeNewDeal(json);
        TLogInfo("Received cards: " + mCtx.mDeck.ToString());
        JSEngine::StringList args;
        args.push_back(mCtx.mDeck.ToString());
        mBotEngine.Call("ReceiveCards", args);
        mCtx.Sync(Engine::WAIT_FOR_CARDS, mNetReplies);
    }
    else if (ev == "RequestBid")
    {
        mCtx.DecodeRequestBid(json);
        // Only reply a bid if it is our place to anwser
        if (mCtx.mCurrentPlayer == mCtx.mMyself.place)
        {
            Contract highestBid = mCtx.mBid.contract;
            mCtx.mMyBid.contract = mCtx.CalculateBid(); // propose our algorithm if the user's one failed
            // only bid over previous one is allowed
            if (mCtx.mMyBid.contract <= highestBid)
            {
                mCtx.mMyBid.contract = Contract::PASS;
            }
            RequestBid(mNetReplies);
//            std::this_thread::sleep_for(std::chrono::seconds(1)); // Simulate thinking
        }
    }
    else if (ev == "RequestKingCall")
    {
        mCtx.Sync(Engine::WAIT_FOR_KING_CALL, mNetReplies);
    }
    else if (ev == "ShowKingCall")
    {
        mCtx.Sync(Engine::WAIT_FOR_SHOW_KING_CALL, mNetReplies);
    }
    else if (ev == "ShowBid")
    {
        mCtx.DecodeShowBid(json);
        mCtx.Sync(Engine::WAIT_FOR_SHOW_BID, mNetReplies);
    }
    else if (ev == "BuildDiscard")
    {
        BuildDiscard(mNetReplies);
        mCtx.Sync(Engine::WAIT_FOR_DISCARD, mNetReplies);
    }
    else if (ev == "ShowDog")
    {
        mCtx.DecodeShowDog(json);
        mCtx.Sync(Engine::WAIT_FOR_SHOW_DOG, mNetReplies);
    }
    else if (ev == "StartDeal")
    {
        mCtx.DecodeStartDeal(json);
        mCtx.Sync(Engine::WAIT_FOR_START_DEAL, mNetReplies);
    }
    else if (ev == "ShowHandle")
    {
        mCtx.DecodeShowHandle(json);
        mCtx.Sync(Engine::WAIT_FOR_SHOW_HANDLE, mNetReplies);
    }
    else if (ev == "NewGame")
    {
        mCtx.DecodeNewGame(json);
        mCtx.Sync(Engine::WAIT_FOR_READY, mNetReplies);
    }
    else if (ev == "ShowCard")
    {
        mCtx.DecodeShowCard(json);
        mCtx.Sync(Engine::WAIT_FOR_SHOW_CARD, mNetReplies);
    }
    else if (ev == "PlayCard")
    {
        mCtx.DecodePlayCard(json);
        // Only reply a bid if it is our place to anwser
        if (mCtx.IsMyTurn())
        {
            Card c = mCtx.ChooseRandomCard();
            mCtx.mDeck.Remove(c);
            mCtx.BuildSendCard(c, mNetReplies);
        }
    }
    else if (ev == "AskForHandle")
    {
        mCtx.BuildHandle(Deck(), mNetReplies);
    }
    else if (ev == "EndOfTrick")
    {
        mCtx.DecodeEndOfTrick(json);
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//            ClearBoard();
        mCtx.mCurrentTrick.Clear();
        mCtx.Sync(Engine::WAIT_FOR_END_OF_TRICK, mNetReplies);
    }
    else if (ev == "EndOfGame")
    {
        mCtx.DecodeEndOfGame(json);
//            ClearBoard();
        mCtx.Sync(Engine::WAIT_FOR_READY, mNetReplies);
    }
    else if (ev == "AllPassed")
    {
//        std::this_thread::sleep_for(std::chrono::seconds(1));
        mCtx.Sync(Engine::WAIT_FOR_ALL_PASSED, mNetReplies);
    }
    else if (ev == "EndOfDeal")
    {
        mCtx.DecodeEndOfDeal(json);
        mCtx.Sync(Engine::WAIT_FOR_END_OF_DEAL, mNetReplies);
    }
    else if (ev == "ChatMessage")
    {
        mCtx.DecodeChatMessage(json);
        // l'affichage s'occupe de tout, on ne fait rien après le décodage
    }
    else if (ev == "LobbyEvent")
    {
        // nada
    }
    else if (ev == "PlayerList")
    {
        // nada
    }
    else
    {
        TLogError("Unmanaged event: " + ev);
    }

    mNet.Send(mCtx.mMyself.uuid, mNetReplies);
    mNetReplies.clear();

/*
    bool ret = true;

    // Generic client decoder, fill the context and the client structure
    BasicClient::Event event = mCtx.Decode(ctx, src_uuid, dest_uuid, arg);

    switch (event)
    {
    case BasicClient::ACCESS_GRANTED:
    {
        JsonObject obj;
        obj.AddValue("cmd", "ReplyLogin");
        ToJson(mCtx.mMyself.identity, obj);
        out.push_back(Reply(Protocol::LOBBY_UID, obj));

        // As soon as we have entered into the lobby, join the assigned table
        mCtx.BuildJoinTable(mCtx.mTableToJoin, out);
        break;
    }
    case BasicClient::NEW_DEAL:
    {
        JSEngine::StringList args;
        args.push_back(mCtx.mDeck.ToString());
        mBotEngine.Call("ReceiveCards", args);
        mCtx.Sync(Engine::WAIT_FOR_CARDS, out);
        break;
    }
    case BasicClient::REQ_BID:
    {
        // Only reply a bid if it is our place to anwser
        if (mCtx.mCurrentBid.taker == mCtx.mMyself.place)
        {
            TLogNetwork("Bot " + mCtx.mMyself.place.ToString() + " is bidding");
            RequestBid(out);
        }
        break;
    }
    case BasicClient::SHOW_BID:
    {
        mCtx.Sync(Engine::WAIT_FOR_SHOW_BID, out);
        break;
    }
    case BasicClient::BUILD_DISCARD:
    {
        BuildDiscard(out);
        break;
    }
    case BasicClient::SHOW_DOG:
    {
        mCtx.Sync(Engine::WAIT_FOR_SHOW_DOG, out);
        break;
    }
    case BasicClient::START_DEAL:
    {
        StartDeal();
        mCtx.Sync(Engine::WAIT_FOR_START_DEAL, out);
        break;
    }
    case BasicClient::SHOW_HANDLE:
    {
        ShowHandle();
        mCtx.Sync(Engine::WAIT_FOR_SHOW_HANDLE, out);
        break;
    }
    case BasicClient::NEW_GAME:
    {
        NewGame();
        mCtx.Sync(Engine::WAIT_FOR_READY, out);
        break;
    }
    case BasicClient::SHOW_CARD:
    {
        ShowCard();
        mCtx.Sync(Engine::WAIT_FOR_SHOW_CARD, out);
        break;
    }
    case BasicClient::PLAY_CARD:
    {
        // Only reply a bid if it is our place to answer
        if (mCtx.IsMyTurn())
        {
            PlayCard(out);
        }
        break;
    }
    case BasicClient::ASK_FOR_HANDLE:
    {
        AskForHandle(out);
        break;
    }
    case BasicClient::END_OF_TRICK:
    {
        mCtx.mCurrentTrick.Clear();
        mCtx.Sync(Engine::WAIT_FOR_END_OF_TRICK, out);
        break;
    }
    case BasicClient::END_OF_GAME:
    {
        mCtx.Sync(Engine::WAIT_FOR_READY, out);
        break;
    }
    case BasicClient::END_OF_DEAL:
    {
        mCtx.Sync(Engine::WAIT_FOR_END_OF_DEAL, out);
        break;
    }
    case BasicClient::JOIN_TABLE:
    {
        mCtx.Sync(Engine::WAIT_FOR_PLAYERS, out);
        break;
    }
    case BasicClient::ALL_PASSED:
    {
        mCtx.Sync(Engine::WAIT_FOR_ALL_PASSED, out);
        break;
    }

    case BasicClient::JSON_ERROR:
    case BasicClient::BAD_EVENT:
    case BasicClient::REQ_LOGIN:
    case BasicClient::MESSAGE:
    case BasicClient::PLAYER_LIST:
    case BasicClient::QUIT_TABLE:
    case BasicClient::SYNC:
    {
        // Nothing to do for that event
        break;
    }

    default:
        ret = false;
        break;
    }

    return ret;
    */
}
/*****************************************************************************/
void Bot::RequestBid(std::vector<Reply> &out)
{
    JSEngine::StringList args;
    Contract highestBid = mCtx.mBid.contract;

    args.push_back(highestBid.ToString()); // Send the highest bid as argument: FIXME: not necessary, the Bot can remember it, to be deleted
    Value result = mBotEngine.Call("AnnounceBid", args);

    if (!result.IsValid())
    {
        TLogError("Invalid script answer, requested string");
    }

    mCtx.mMyBid.contract = Contract(result.GetString());

    // security test
    if ((mCtx.mMyBid.contract >= Contract(Contract::PASS)) && (mCtx.mMyBid.contract <= Contract(Contract::GUARD_AGAINST)))
    {
        // Ask to the bot if a slam has been announced
        args.clear();
        result = mBotEngine.Call("AnnounceSlam", args);
        if (result.IsValid())
        {
            mCtx.mMyBid.slam = result.GetBool();
        }
        else
        {
            TLogError("Invalid script answer, requested boolean");
        }
    }
    else
    {
        mCtx.mMyBid.contract = mCtx.CalculateBid(); // propose our algorithm if the user's one failed
    }

    // only bid over previous one is allowed
    if (mCtx.mMyBid.contract <= highestBid)
    {
        mCtx.mMyBid.contract = Contract::PASS;
    }

    mCtx.BuildReplyBid(out);
}
/*****************************************************************************/
void Bot::StartDeal()
{
    // FIXME: pass the game type to the script
    // FIXME: pass the slam declared bolean to the script
    JSEngine::StringList args;
    args.push_back(mCtx.mBid.taker.ToString());
    args.push_back(mCtx.mBid.contract.ToString());
    mBotEngine.Call("StartDeal", args);
}
/*****************************************************************************/
void Bot::AskForHandle(std::vector<Reply> &out)
{
    bool valid = false;
    JSEngine::StringList args;

    TLogInfo("Ask for handle");
    Value ret = mBotEngine.Call("AskForHandle", args);
    Deck handle; // empty by default

    if (ret.IsValid())
    {
        if (ret.GetType() == Value::STRING)
        {
            std::string cards = ret.GetString();
            if (cards.size() > 0)
            {
                std::uint8_t count = handle.SetCards(cards);
                if (count > 0U)
                {
                    valid = mCtx.mDeck.TestHandle(handle);
                }
                else
                {
                    TLogError("Unknown cards in the handle");
                }
            }
            else
            {
                // Empty string means no handle to declare, it is valid!
                valid = true;
            }
        }
        else
        {
            TLogError("Bad format, requested a string");
        }
    }
    else
    {
        TLogError("Invalid script answer");
    }

    if (!valid)
    {
        TLogInfo("Invalid handle is: " + handle.ToString());
        handle.Clear();
    }

    TLogInfo(std::string("Sending handle") + handle.ToString());
    mCtx.BuildHandle(handle, out);
}
/*****************************************************************************/
void Bot::ShowHandle()
{
    JSEngine::StringList args;

    // Send the handle to the bot
    args.push_back(mCtx.mHandle.ToString());
    if (mCtx.mHandle.GetOwner() == Team::ATTACK)
    {
        args.push_back("0");
    }
    else
    {
        args.push_back("1");
    }
    mBotEngine.Call("ShowHandle", args);
}
/*****************************************************************************/
void Bot::BuildDiscard(std::vector<Reply> &out)
{
    bool valid = false;
    JSEngine::StringList args;
    Deck discard;

    args.push_back(mCtx.mDog.ToString());
    Value ret = mBotEngine.Call("BuildDiscard", args);

    if (ret.IsValid())
    {
        if (ret.GetType() == Value::STRING)
        {
            std::uint8_t count = discard.SetCards(ret.GetString());
            if (count == Tarot::NumberOfDogCards(mCtx.mGameState.mNbPlayers))
            {
                valid = mCtx.TestDiscard(discard);
            }
            else
            {
                TLogError("Unknown cards in the discard");
            }
        }
        else
        {
            TLogError("Bad format, requested a string");
        }
    }
    else
    {
        TLogError("Invalid script answer");
    }

    if (valid)
    {
        mCtx.mDeck += mCtx.mDog;
        mCtx.mDeck.RemoveDuplicates(discard);

        TLogInfo("Player deck: " + mCtx.mDeck.ToString());
        TLogInfo("Discard: " + discard.ToString());
    }
    else
    {
        TLogInfo("Invalid discard is: " + discard.ToString());

        discard = mCtx.AutoDiscard(); // build a random valid deck
    }

    mCtx.BuildDiscard(discard, out);
}
/*****************************************************************************/
void Bot::NewGame()
{
    // (re)inititialize script context
    if (InitializeScriptContext() == true)
    {
        JSEngine::StringList args;
        args.push_back(mCtx.mMyself.place.ToString());
        Tarot::Game game = mCtx.mGame;
        std::string modeString;
        if (game.mode == Tarot::Game::cQuickDeal)
        {
            modeString = "one_deal";
        }
        else
        {
            modeString = "simple_tournament";
        }
        args.push_back(modeString);
        mBotEngine.Call("EnterGame", args);
    }
    else
    {
        TLogError("Cannot initialize bot context");
    }
}
/*****************************************************************************/
void Bot::PlayCard(std::vector<Reply> &out)
{
    Card c;

    // Wait some time before playing
    std::this_thread::sleep_for(std::chrono::milliseconds(mTimeBeforeSend));

    JSEngine::StringList args;
    Value ret = mBotEngine.Call("PlayCard", args);

    if (!ret.IsValid())
    {
        TLogError("Invalid script answer");
    }

    // Test validity of card
    c = mCtx.mDeck.GetCard(ret.GetString());
    if (c.IsValid())
    {
        if (!mCtx.IsValid(c))
        {
            std::stringstream message;
            message << mCtx.mMyself.place.ToString() << " played a non-valid card: " << ret.GetString() << "Deck is: " << mCtx.mDeck.ToString();
            TLogError(message.str());
            // The show must go on, play a random card
            c = mCtx.ChooseRandomCard();

            if (!c.IsValid())
            {
                TLogError("Panic!");
            }
        }
    }
    else
    {
        std::stringstream message;
        message << mCtx.mMyself.place.ToString() << " played an unknown card: " << ret.GetString()
                << " Client deck is: " << mCtx.mDeck.ToString();

        // The show must go on, play a random card
        c = mCtx.ChooseRandomCard();

        if (c.IsValid())
        {
            message << " Randomly chosen card is: " << c.ToString();
            TLogInfo(message.str());
        }
        else
        {
            TLogError("Panic!");
        }
    }

    mCtx.mDeck.Remove(c);
    if (!c.IsValid())
    {
        TLogError("Invalid card!");
    }

    mCtx.BuildSendCard(c, out);
}
/*****************************************************************************/
void Bot::ShowCard()
{
    JSEngine::StringList args;
    args.push_back(mCtx.mCurrentTrick.Last().ToString());
    args.push_back(mCtx.mCurrentPlayer.ToString());
    mBotEngine.Call("PlayedCard", args);
}
/*****************************************************************************/
void Bot::SetTimeBeforeSend(std::uint16_t t)
{
    mTimeBeforeSend = t;
}
/*****************************************************************************/
void Bot::ChangeNickname(const std::string &nickname, std::vector<Reply> &out)
{
    mCtx.mMyself.identity.nickname = nickname;
    mCtx.BuildChangeNickname(out);
}
/*****************************************************************************/
void Bot::SetAiScript(const std::string &path)
{
    mScriptPath = path;
}
/*****************************************************************************/
void Bot::SetIdentity(const Identity &identity)
{
    mCtx.mMyself.identity = identity;
}
/*****************************************************************************/
bool Bot::InitializeScriptContext()
{
    bool retCode = true;
    Zip zip;
    bool useZip = false;

    if (Util::FileExists(mScriptPath))
    {
        // Seems to be an archive file
        if (!zip.Open(mScriptPath, true))
        {
            TLogError("Invalid AI Zip file.");
            retCode = false;
        }
        useZip = true;
    }
    else
    {
        if (Util::FolderExists(mScriptPath))
        {
            TLogInfo("Using script root path: " + mScriptPath);
        }
        else
        {
            // Last try, maybe a zipped memory buffer
            if (!zip.Open(mScriptPath, false))
            {
                TLogError("Invalid AI Zip buffer.");
                retCode = false;
            }
            useZip = true;
        }
    }

    if (retCode)
    {
        // Open the configuration file to find the scripts
        JsonValue json;

        if (useZip)
        {
            std::string package;
            if (zip.GetFile("package.json", package))
            {
                retCode = JsonReader::ParseString(json, package);
            }
            else
            {
                retCode = false;
            }
        }
        else
        {
            retCode = JsonReader::ParseFile(json, mScriptPath + Util::DIR_SEPARATOR + "package.json");
        }

        if (retCode)
        {
            JsonValue files = json.FindValue("files");

            mBotEngine.Initialize();

            // Load all Javascript files
            for (JsonArray::iterator iter = files.GetArray().begin(); iter != files.GetArray().end(); ++iter)
            {
                if (iter->IsValid() && (iter->IsString()))
                {
                    if (useZip)
                    {
                        std::string script;
                        if (zip.GetFile(iter->GetString(), script))
                        {
                            std::string output;
                            if (!mBotEngine.EvaluateString(script, output))
                            {
                                TLogError("Script error: " + output);
                                retCode = false;
                            }
                        }
                        else
                        {
                            retCode = false;
                        }
                    }
                    else
                    {
                        std::string fileName = mScriptPath + iter->GetString();

        #ifdef USE_WINDOWS_OS
                        // Correct the path if needed
     //                   Util::ReplaceCharacter(fileName, "/", "\\");
        #endif

                        if (!mBotEngine.EvaluateFile(fileName))
                        {
                            std::stringstream message;
                            message << "Script error: could not open program file: " << fileName;
                            TLogError(message.str());
                            retCode = false;
                        }
                    }
                }
                else
                {
                    TLogError("Bad Json value in the array");
                    retCode = false;
                }
            }
        }
        else
        {
            TLogError("Cannot open Json configuration file");
            retCode = false;
        }
    }

    return retCode;
}


//=============================================================================
// End of file Bot.cpp
//=============================================================================
