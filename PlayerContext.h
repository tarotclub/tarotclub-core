/*=============================================================================
 * TarotClub - PlayerContext.h
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

#ifndef PLAYER_CONTEXT_H
#define PLAYER_CONTEXT_H

#include "Network.h"
#include "Score.h"
#include "ClientConfig.h"
#include "DurationTimer.h"
/**
 * @brief The PlayerContext struct
 *
 * Store data and call events for any tarot based player
 */
struct PlayerContext
{
public:
    struct Message
    {
        Message()
            : src(Protocol::INVALID_UID)
            , dst(Protocol::INVALID_UID)
        {

        }

        std::uint32_t src;
        std::uint32_t dst;
        std::string msg;
    };

    struct Sit
    {
        std::string message;
        Contract    contract;  // contract announced
        bool        slam;

        Sit() {
            Clear();
        }

        void Clear() {
            message.clear();
            contract = Contract(Contract::NO_ANY);
            slam = false;
        }
    };

    enum TableMode
    {
        TABLE_MODE_BLOCKED,
        TABLE_MODE_BID,
        TABLE_MODE_PLAY,
        TABLE_MODE_WAIT_TIMER_END_OF_TRICK,
        TABLE_MODE_WAIT_CLICK_END_OF_TRICK,
        TABLE_MODE_SHOW_RESULTS,
    };

    PlayerContext();

    void Clear();
    bool TestDiscard(const Deck &discard);
    Contract CalculateBid();
    void UpdateStatistics();
    Card ChooseRandomCard();
    bool IsValid(const Card &c);
    Deck AutoDiscard();
    bool IsMyTurn() const { return mCurrentPlayer == mMyself.place; }
    bool IsMyBidTurn() const {
        return (mCurrentPlayer == mMyself.place) && !mMyBid.contract.IsValid();
    }

    int AttackPoints();
    int DefensePoints();

    void Update();

    // FIXME: ajouter le type de message (serveur, lobby, local ...)
    // ajuster src et dst en fonction
    void AddMessage(const std::string &text)
    {
        Message msg;
        msg.src = 0;
        msg.dst = 0;
        msg.msg = text;

        mMessages.push_back(msg);
    }

    // Network serializers
    void BuildReplyLogin(std::vector<Reply> &out);
    void BuildNewGame(std::vector<Reply> &out);
    void BuildChangeNickname(std::vector<Reply> &out);
    void BuildReplyBid(std::vector<Reply> &out);
    void BuildJoinTable(std::uint32_t tableId, std::vector<Reply> &out);
    void BuildHandle(const Deck &handle, std::vector<Reply> &out);
    void BuildDiscard(const Deck &discard, std::vector<Reply> &out);
    void BuildSendCard(Card c, std::vector<Reply> &out);
    void BuildQuitTable(std::uint32_t tableId, std::vector<Reply> &out);
    void BuildChatMessage(const std::string &message, std::vector<Reply> &out);

    void Sync(Engine::Sequence sequence, std::vector<Reply> &out);

    JsonObject mDealResult;
    Deck::Statistics   mStats;   // statistics on player's cards
    Tarot::Game mGame;
    Tarot::Bid mBid;
    Tarot::Bid mCurrentBid;
    Tarot::Bid mMyBid;
    Tarot::Distribution mShuffle;
    Points mPoints;
    Deck mDog;
    Deck mHandle;
    Deck mDeck;
    std::uint8_t mNbPlayers;
    Users::Entry mMyself;
    std::uint32_t mTableToJoin;
    Sit mSits[5];
    ClientOptions mOptions;

    DurationTimer mPlayTimer;
    DurationTimer mEndOfTrickTimer;

    // Current trick variables
    Deck mCurrentTrick;
    Place mCurrentPlayer;
    Place mFirstPlayer;

    TableMode mMode = TABLE_MODE_BLOCKED;

    // uuid --> name
    std::map<std::uint32_t, std::string> mTables;
    std::vector<Message> mMessages;
    Users mUsers;

    bool Decode(const Request &req, JsonObject &json);
//    bool PlayRandom(IContext &ctx, uint32_t src_uuid, uint32_t dest_uuid, const std::string &arg, std::vector<Reply> &out);

    void DecodeEndOfGame(const JsonValue &json);
    void DecodeEndOfDeal(const JsonValue &json);
    void DecodeEndOfTrick(const JsonValue &json);
    void DecodePlayCard(const JsonValue &json);
    void DecodeShowCard(const JsonValue &json);
    void DecodeShowHandle(const JsonValue &json);
    void DecodeStartDeal(const JsonValue &json);
    void DecodeShowDog(const JsonValue &json);
    void DecodeShowBid(const JsonValue &json);
    void DecodeRequestBid(const JsonValue &json);
    void DecodeNewDeal(const JsonValue &json);
    void DecodeNewGame(const JsonValue &json);
    void DecodeReplyJoinTable(const JsonValue &json);
    void DecodeLobbyEvent(const JsonValue &json);
    void DecodeChatMessage(const JsonValue &json);
    void DecodeAccessGranted(const JsonValue &json);
    void DecodeRequestLogin(const JsonValue &json);

private:
    void UserFromJson(Users::Entry &member, JsonObject &player);
    void UpdateMember(Users::Entry &member, const std::string &event);
};

#endif // PLAYER_CONTEXT_H
