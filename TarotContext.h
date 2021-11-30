#ifndef TAROTCONTEXT_H
#define TAROTCONTEXT_H

#include "Deck.h"
#include "Common.h"
#include "JsonValue.h"
#include "Score.h"

struct TarotContext
{
public:
    TarotContext();

    void Initialize();

    std::uint8_t mNbPlayers;
    Tarot::Bid mBid;
    Deck mDiscard;
    Deck mDog;
    Deck mAttackHandle;
    Deck mDefenseHandle;
    Place mFirstPlayer;
    Deck mTricks[24];    // 24 tricks max with 3 players
    Place mWinner[24];
    int mTricksWon;
    Deck::Statistics mStatsAttack;
    Card mKingCalled;

    Contract SetBid(Contract c, bool slam, Place p);
    void SaveToJson(JsonObject &json);
    Place SetTrick(const Deck &trick, std::uint8_t trickCounter);
    Place GetOwner(Place firstPlayer, const Card &card, int turn);
    bool LoadFromJson(const JsonValue &json);
    bool ShowDogAfterBid();
    void AnalyzeGame(Points &points, std::uint8_t numberOfPlayers);
    Deck GetTrick(std::uint8_t turn, std::uint8_t numberOfPlayers);
    Place GetWinner(std::uint8_t turn, std::uint8_t numberOfPlayers);
    void SetHandle(const Deck &handle, Team team);
    bool SetKingCall(const Card &c);
private:
    bool HasDecimal(float f);
};

#endif // TAROTCONTEXT_H
