/*=============================================================================
 * TarotClub - PlayerContext.cpp
 *=============================================================================
 * Moteur de jeu principal + serveur de jeu réseau
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
// Tarot files
#include "PlayerContext.h"
#include "Protocol.h"

// ICL files
#include "Log.h"
#include "JsonReader.h"
#include "Protocol.h"

/*****************************************************************************/
PlayerContext::PlayerContext()
    : mNbPlayers(4U)
    , mTableToJoin(0U)
{

}
/*****************************************************************************/
void PlayerContext::Clear()
{
    mTables.clear();
    mMessages.clear();
    mUsers.Clear();
    mMyself.Clear();
}
/*****************************************************************************/
bool PlayerContext::TestDiscard(const Deck &discard)
{
    return mDeck.TestDiscard(discard, mDog, mNbPlayers);
}
/*****************************************************************************/
Contract PlayerContext::CalculateBid()
{
    int total = 0;
    Contract cont;

    UpdateStatistics();

    // Set points according to the card values
    if (mStats.bigTrump == true)
    {
        total += 9;
    }
    if (mStats.fool == true)
    {
        total += 7;
    }
    if (mStats.littleTrump == true)
    {
        if (mStats.trumps == 5)
        {
            total += 5;
        }
        else if (mStats.trumps == 6 || mStats.trumps == 7)
        {
            total += 7;
        }
        else if (mStats.trumps > 7)
        {
            total += 8;
        }
    }

    // Each trump is 1 point
    // Each major trump is 1 more point
    total += mStats.trumps * 2;
    total += mStats.majorTrumps * 2;
    total += mStats.kings * 6;
    total += mStats.queens * 3;
    total += mStats.knights * 2;
    total += mStats.jacks;
    total += mStats.weddings;
    total += mStats.longSuits * 5;
    total += mStats.cuts * 5;
    total += mStats.singletons * 3;
    total += mStats.sequences * 4;

    // We can decide the bid
    if (total <= 35)
    {
        cont = Contract::PASS;
    }
    else if (total >= 36  && total <= 50)
    {
        cont = Contract::TAKE;
    }
    else if (total >= 51  && total <= 65)
    {
        cont = Contract::GUARD;
    }
    else if (total >= 66  && total <= 75)
    {
        cont = Contract::GUARD_WITHOUT;
    }
    else
    {
        cont = Contract::GUARD_AGAINST;
    }
    return cont;
}
/*****************************************************************************/
void PlayerContext::UpdateStatistics()
{
    mStats.Reset();
    mDeck.AnalyzeTrumps(mStats);
    mDeck.AnalyzeSuits(mStats);
}
/*****************************************************************************/
Card PlayerContext::ChooseRandomCard()
{
    Card card;

//    std::cout << ">>>>> RANDOM WITH: "  << mDeck.ToString() << ", " << mCurrentTrick.ToString() << std::endl;

    for (const auto &c : mDeck)
    {
        if (IsValid(c) == true)
        {
            card = c;
            break;
        }
    }
    return card;
}
/*****************************************************************************/
bool PlayerContext::IsValid(const Card &c)
{
    return mDeck.CanPlayCard(c, mCurrentTrick);
}
/*****************************************************************************/
Deck PlayerContext::AutoDiscard()
{
    Deck discard = mDeck.AutoDiscard(mDog, mNbPlayers);
    mDeck.RemoveDuplicates(discard);
    return discard;
}
/*****************************************************************************/
int PlayerContext::AttackPoints()
{
    return mPoints.GetPoints(Team(Team::ATTACK), mBid, mNbPlayers);
}
/*****************************************************************************/
int PlayerContext::DefensePoints()
{
    return mPoints.GetPoints(Team(Team::DEFENSE), mBid, mNbPlayers);
}
/*****************************************************************************/
void PlayerContext::Update()
{
    mEndOfTrickTimer.Update();
}
/*****************************************************************************/
void PlayerContext::BuildReplyLogin(std::vector<Reply> &out)
{
    JsonObject obj;

    obj.AddValue("cmd", "ReplyLogin");
    ToJson(mMyself.identity, obj);

    out.push_back(Reply(Protocol::LOBBY_UID, obj));
}
/*****************************************************************************/
void PlayerContext::BuildReplyBid(std::vector<Reply> &out)
{
    JsonObject obj;

    obj.AddValue("cmd", "ReplyBid");
    obj.AddValue("contract", mMyBid.contract.ToString());
    obj.AddValue("slam", mMyBid.slam);

    out.push_back(Reply(mMyself.tableId, obj));
}
/*****************************************************************************/
// TODO: gérer les messages privés, notamment en changeant la target
void PlayerContext::BuildChatMessage(const std::string &message, std::vector<Reply> &out)
{
    JsonObject obj;

    obj.AddValue("cmd", "ChatMessage");
    obj.AddValue("message", message);
    obj.AddValue("source", mMyself.uuid);
    obj.AddValue("target", Protocol::LOBBY_UID);

    out.push_back(Reply(Protocol::LOBBY_UID, obj));
}
/*****************************************************************************/
void PlayerContext::BuildHandle(const Deck &handle, std::vector<Reply> &out)
{
    JsonObject obj;

    obj.AddValue("cmd", "Handle");
    obj.AddValue("handle", handle.ToString());

    out.push_back(Reply(mMyself.tableId, obj));
}
/*****************************************************************************/
void PlayerContext::BuildDiscard(const Deck &discard, std::vector<Reply> &out)
{
    JsonObject obj;

    obj.AddValue("cmd", "Discard");
    obj.AddValue("discard", discard.ToString());

    out.push_back(Reply(mMyself.tableId, obj));
}
/*****************************************************************************/
void PlayerContext::BuildSendCard(Card c, std::vector<Reply> &out)
{
    JsonObject obj;

    mDeck.Remove(c);
    obj.AddValue("cmd", "Card");
    obj.AddValue("card", c.ToString());

    out.push_back(Reply(mMyself.tableId, obj));

    // On bloque tous les événements
    mMode = TABLE_MODE_BLOCKED;
}
/*****************************************************************************/
void PlayerContext::BuildQuitTable(std::uint32_t tableId, std::vector<Reply> &out)
{
    JsonObject obj;

    obj.AddValue("cmd", "RequestQuitTable");
    obj.AddValue("table_id", tableId);

    out.push_back(Reply(mMyself.tableId, obj));
}
/*****************************************************************************/
void PlayerContext::BuildChangeNickname(std::vector<Reply> &out)
{
    JsonObject obj;

    obj.AddValue("cmd", "RequestChangeNickname");
    obj.AddValue("nickname", mMyself.identity.nickname);

    out.push_back(Reply(Protocol::LOBBY_UID, obj));
}
/*****************************************************************************/
void PlayerContext::BuildJoinTable(uint32_t tableId, std::vector<Reply> &out)
{
    JsonObject obj;

    obj.AddValue("cmd", "RequestJoinTable");
    obj.AddValue("table_id", tableId);

    out.push_back(Reply(Protocol::LOBBY_UID, obj));
}
/*****************************************************************************/
void PlayerContext::BuildNewGame(std::vector<Reply> &out)
{
    JsonObject obj;

    obj.AddValue("cmd", "RequestNewGame");
    obj.AddValue("mode", mGame.Get());

    JsonArray array;
    for (auto &deal : mGame.deals)
    {
        JsonObject dealObj;

        ToJson(deal, dealObj);
        array.AddValue(dealObj);
    }

    obj.AddValue("deals", array);

    out.push_back(Reply(mMyself.tableId, obj));
}
/*****************************************************************************/
void PlayerContext::Sync(Engine::Sequence sequence, std::vector<Reply> &out)
{
    JsonObject obj;

    obj.AddValue("cmd", "Ack");
    obj.AddValue("step", Ack::ToString(sequence));
    out.push_back(Reply(mMyself.tableId, obj));
}
/*****************************************************************************/
void PlayerContext::UserFromJson(Users::Entry &member, JsonObject &player)
{
    FromJson(member.identity, player);

    member.uuid = static_cast<std::uint32_t>(player.GetValue("uuid").GetInteger());
    member.tableId = static_cast<std::uint32_t>(player.GetValue("table").GetInteger());
    member.place = Place(player.GetValue("place").GetString());
}
/*****************************************************************************/
/*
bool PlayerContext::PlayRandom(IContext &ctx, uint32_t src_uuid, uint32_t dest_uuid, const std::string &arg, std::vector<Reply> &out)
{
    bool ret = true;

    // Generic client decoder, fill the context and the client structure
    PlayerContext::Event event = Decode(ctx, src_uuid, dest_uuid, arg);

    switch (event)
    {
    case PlayerContext::ACCESS_GRANTED:
    {
        JsonObject obj;
        obj.AddValue("cmd", "ReplyLogin");
        ToJson(mMyself.identity, obj);
        out.push_back(Reply(Protocol::LOBBY_UID, obj));

        // As soon as we have entered into the lobby, join the assigned table
        BuildJoinTable(mTableToJoin, out);
        break;
    }
    case PlayerContext::NEW_DEAL:
    {
        Sync(Engine::WAIT_FOR_CARDS, out);
        break;
    }
    case PlayerContext::REQ_BID:
    {
        // Only reply a bid if it is our place to anwser
        if (mCurrentBid.taker == mMyself.place)
        {
            TLogNetwork("Bot " + mMyself.place.ToString() + " is bidding");
            Contract highestBid = mCurrentBid.contract;
            Contract botContract = CalculateBid(); // propose our algorithm if the user's one failed

            // only bid over previous one is allowed
            if (botContract <= highestBid)
            {
                botContract = Contract::PASS;
            }

            BuildBid(botContract, false, out);
        }
        break;
    }
    case PlayerContext::REQ_CALL_KING:
    {
        if (mBid.taker == mMyself.place)
        {
            JsonObject obj;

            obj.AddValue("cmd", "ReplyKingCall");
            obj.AddValue("card", "14-C"); // dans cette version on choisit toujours la même carte, le roi de coeur

            out.push_back(Reply(mMyself.tableId, obj));
        }
        break;
    }
    case PlayerContext::REQ_SHOW_CALLED_KING:
    {
        Sync(Engine::WAIT_FOR_SHOW_KING_CALL, out);
        break;
    }
    case PlayerContext::SHOW_BID:
    {
        Sync(Engine::WAIT_FOR_SHOW_BID, out);
        break;
    }
    case PlayerContext::BUILD_DISCARD:
    {
        Deck discard = AutoDiscard(); // build a random valid deck
        BuildDiscard(discard, out);
        break;
    }
    case PlayerContext::SHOW_DOG:
    {
        Sync(Engine::WAIT_FOR_SHOW_DOG, out);
        break;
    }
    case PlayerContext::START_DEAL:
    {
        Sync(Engine::WAIT_FOR_START_DEAL, out);
        break;
    }
    case PlayerContext::SHOW_HANDLE:
    {
        Sync(Engine::WAIT_FOR_SHOW_HANDLE, out);
        break;
    }
    case PlayerContext::NEW_GAME:
    {
        Sync(Engine::WAIT_FOR_READY, out);
        break;
    }
    case PlayerContext::SHOW_CARD:
    {
        Sync(Engine::WAIT_FOR_SHOW_CARD, out);
        break;
    }
    case PlayerContext::PLAY_CARD:
    {
        // Only reply a bid if it is our place to answer
        if (IsMyTurn())
        {
            Card c = ChooseRandomCard();
            mDeck.Remove(c);
            if (!c.IsValid())
            {
                TLogError("Invalid card!");
            }
            BuildSendCard(c, out);
        }
        break;
    }
    case PlayerContext::ASK_FOR_HANDLE:
    {
        Deck handle;
        BuildHandle(handle, out);
        break;
    }
    case PlayerContext::END_OF_TRICK:
    {
        mCurrentTrick.Clear();
        Sync(Engine::WAIT_FOR_END_OF_TRICK, out);
        break;
    }
    case PlayerContext::END_OF_GAME:
    {
        Sync(Engine::WAIT_FOR_READY, out);
        break;
    }
    case PlayerContext::END_OF_DEAL:
    {
        Sync(Engine::WAIT_FOR_END_OF_DEAL, out);
        break;
    }
    case PlayerContext::JOIN_TABLE:
    {
        Sync(Engine::WAIT_FOR_PLAYERS, out);
        break;
    }
    case PlayerContext::ALL_PASSED:
    {
        Sync(Engine::WAIT_FOR_ALL_PASSED, out);
        break;
    }

    case PlayerContext::JSON_ERROR:
    case PlayerContext::BAD_EVENT:
    case PlayerContext::REQ_LOGIN:
    case PlayerContext::MESSAGE:
    case PlayerContext::PLAYER_LIST:
    case PlayerContext::QUIT_TABLE:
    case PlayerContext::SYNC:
    {
        // Nothing to do for that event
        break;
    }

    default:
        ret = false;
        break;
    }

    return ret;
}
*/
/*****************************************************************************/
void PlayerContext::DecodeRequestLogin(const JsonValue &json)
{
    mMyself.uuid = static_cast<std::uint32_t>(json.FindValue("uuid").GetInteger());
}
/*****************************************************************************/
void PlayerContext::DecodeAccessGranted(const JsonValue &json)
{
    JsonArray tablesArray = json.FindValue("tables").GetArray();
    mTables.clear();
    for (std::uint32_t i = 0U; i < tablesArray.Size(); i++)
    {
       JsonObject tableObj = tablesArray.GetEntry(i).GetObj();
       uint32_t uuid = static_cast<std::uint32_t>(tableObj.GetValue("uuid").GetInteger());
       mTables[uuid] = tableObj.GetValue("name").GetString();
    }

    JsonArray players = json.FindValue("players").GetArray();
    mUsers.Clear();

    for (std::uint32_t i = 0U; i < players.Size(); i++)
    {
        Users::Entry member;
        JsonObject player = players.GetEntry(i).GetObj();

        UserFromJson(member, player);
        mUsers.AddEntry(member);
    }
}
/*****************************************************************************/
void PlayerContext::DecodeChatMessage(const JsonValue &json)
{
    Message msg;
    msg.src = static_cast<std::uint32_t>(json.FindValue("source").GetInteger());
    msg.dst = static_cast<std::uint32_t>(json.FindValue("target").GetInteger());
    msg.msg = json.FindValue("message").GetString();

    mMessages.push_back(msg);
}
/*****************************************************************************/
void PlayerContext::DecodeLobbyEvent(const JsonValue &json)
{
// {"cmd":"LobbyEvent","counter":8,"player":{"avatar":"Humain","gender":"Invalid","nickname":"Humain","place":"West","table":1000,"uuid":13},"type":"Join"}

    JsonObject player = json.FindValue("player").GetObj();
    std::string type = json.FindValue("type").GetString();

    Users::Entry member;
    UserFromJson(member, player);

    if (type == "JoinTable")
    {
        // Player has join a table
        mUsers.UpdateLocation(member.uuid, member.tableId, member.place);
    }

}
/*****************************************************************************/
void PlayerContext::DecodeReplyJoinTable(const JsonValue &json)
{
    mMyself.place = Place(json.FindValue("place").GetString());
    mMyself.tableId = static_cast<std::uint32_t>(json.FindValue("table_id").GetInteger());
    mNbPlayers = static_cast<std::uint32_t>(json.FindValue("size").GetInteger());
}
/*****************************************************************************/
void PlayerContext::DecodeNewGame(const JsonValue &json)
{
    mGame.Set(json.FindValue("mode").GetString());
}
/*****************************************************************************/
void PlayerContext::DecodeNewDeal(const JsonValue &json)
{
    mDeck.SetCards(json.FindValue("cards").GetString());

    mMode = TABLE_MODE_BID;

    // Initi context stuff
    mMyBid.Initialize();
}
/*****************************************************************************/
void PlayerContext::DecodeRequestBid(const JsonValue &json)
{
    mCurrentPlayer = Place(json.FindValue("place").GetString());
    mCurrentBid.contract = Contract(json.FindValue("contract").GetString());
}
/*****************************************************************************/
void PlayerContext::DecodeShowBid(const JsonValue &json)
{
    Place p(json.FindValue("place").GetString());
    mSits[p.Value()].slam = json.FindValue("slam").GetBool();
    mSits[p.Value()].contract = Contract(json.FindValue("contract").GetString());

    mBid.taker = Place(json.FindValue("taker_place").GetString());
    mBid.contract = Contract(json.FindValue("taker_contract").GetString());
    mBid.slam = json.FindValue("taker_slam").GetBool();
}
/*****************************************************************************/
void PlayerContext::DecodeShowDog(const JsonValue &json)
{
    mDog.SetCards(json.FindValue("dog").GetString());
}
/*****************************************************************************/
void PlayerContext::DecodeStartDeal(const JsonValue &json)
{
    mBid.taker = Place(json.FindValue("taker").GetString());
    mBid.contract = Contract(json.FindValue("contract").GetString());
    mBid.slam = json.FindValue("slam").GetBool();

    mMode = TABLE_MODE_PLAY;

    UpdateStatistics();
    mCurrentTrick.Clear();
}
/*****************************************************************************/
void PlayerContext::DecodeShowHandle(const JsonValue &json)
{
    Place place = Place(json.FindValue("place").GetString());
    mHandle.SetCards(json.FindValue("handle").GetString());

    Team team(Team::DEFENSE);
    if (place == mBid.taker)
    {
        team = Team::ATTACK;
    }
    mHandle.SetOwner(team);
}
/*****************************************************************************/
void PlayerContext::DecodeShowCard(const JsonValue &json)
{
    mCurrentPlayer = Place(json.FindValue("place").GetString());
    Card card = Card(json.FindValue("card").GetString());
    mCurrentTrick.Append(card);
}
/*****************************************************************************/
void PlayerContext::DecodePlayCard(const JsonValue &json)
{
    mCurrentPlayer = Place(json.FindValue("place").GetString());

    if (mCurrentTrick.Size() == 0)
    {
        if (mOptions.autoPlay)
        {
            mMode = TABLE_MODE_BLOCKED;
        }
        else
        {
            mMode = TABLE_MODE_PLAY;
        }

        mFirstPlayer = mCurrentPlayer;
    }
}
/*****************************************************************************/
void PlayerContext::DecodeEndOfTrick(const JsonValue &json)
{
    // Le gagnant du tour
    mCurrentPlayer = Place(json.FindValue("place").GetString());
}
/*****************************************************************************/
void PlayerContext::DecodeEndOfDeal(const JsonValue &json)
{
    mPoints.pointsAttack = json.FindValue("points").GetInteger();
    mPoints.oudlers = json.FindValue("oudlers").GetInteger();
    mPoints.littleEndianOwner = static_cast<std::uint8_t>(json.FindValue("little_bonus").GetInteger());
    mPoints.handlePoints = json.FindValue("handle_bonus").GetInteger();
    mPoints.slamDone = json.FindValue("slam_bonus").GetBool();

    mDealResult = json.FindValue("deal").GetObj();
}
/*****************************************************************************/
void PlayerContext::DecodeEndOfGame(const JsonValue &json)
{
    mCurrentPlayer = Place(json.FindValue("winner").GetString());
    mMode = PlayerContext::TABLE_MODE_SHOW_RESULTS;
}
/*****************************************************************************/
bool PlayerContext::Decode(const Request &req, JsonObject &json)
{
    bool success = true;

    JsonReader reader;
    JsonValue jsonVal;

    if (!reader.ParseString(jsonVal, req.arg))
    {
        TLogError("Not a JSON data" + req.arg);
        success = false;
    }
    else
    {
        json = jsonVal.GetObj();
    }
    return success;
}
/*****************************************************************************/
void PlayerContext::UpdateMember(Users::Entry &member, const std::string &event)
{
    if (event == "Quit")
    {
        mUsers.Remove(member.uuid);
    }
    else
    {
        if (!mUsers.IsHere(member.uuid))
        {
            if (event == "New")
            {
                // Create user if not exists
                if (!mUsers.AddEntry(member))
                {
                    TLogError("AddEntry should not fail, the list is managed by the server");
                }
            }
            else
            {
                TLogError("User uuid should be in the list");
            }
        }
        else
        {
            if (event == "Nick")
            {
                mUsers.ChangeNickName(member.uuid, member.identity.nickname);
            }
            else if ((event == "Join") ||
                     (event == "Leave"))
            {
                mUsers.UpdateLocation(member.uuid, member.tableId, member.place);
            }
        }
    }
}

