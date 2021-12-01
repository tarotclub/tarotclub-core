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

    // Modifiers
    Contract SetBid(Contract c, bool slam, Place p);
    Place SetTrick(const Deck &trick, std::uint8_t trickCounter);
    bool LoadFromJson(const JsonValue &json);
    bool ManageDogAfterBid();
    void AnalyzeGame(Points &points);
    void SetHandle(const Deck &handle, Place p);

    // Getters
    void SaveToJson(JsonObject &json) const;
    Place GetOwner(Place firstPlayer, const Card &card, int turn) const;
    Deck GetTrick(std::uint8_t turn) const;
    Place GetWinner(std::uint8_t turn) const;
    bool CheckKingCall(const Card &c, Deck::Statistics &stats) const;
private:
    bool HasDecimal(float f);
};

#endif // TAROTCONTEXT_H
