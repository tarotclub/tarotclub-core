#include "TarotContext.h"

static const std::string TAROT_CONTEXT_VERSION  = "3";

TarotContext::TarotContext()
    : mNbPlayers(4U)
{

}
/*****************************************************************************/
void TarotContext::Initialize()
{
    mBid.Initialize();
    mDiscard.Clear();
    mAttackHandle.Clear();
    mDefenseHandle.Clear();

    for (std::uint32_t i = 0U; i < 24U; i++)
    {
        mTricks[i].Clear();
    }
    mTricksWon = 0;
    mStatsAttack.Reset();
}
/*****************************************************************************/
void TarotContext::SetHandle(const Deck &handle, Place p)
{
    if ((p == mBid.taker) || (p == mBid.partner))
    {
        mAttackHandle = handle;
    }
    else
    {
        mDefenseHandle = handle;
    }
}
/*****************************************************************************/
bool TarotContext::ManageDogAfterBid()
{
    bool showDog = true;
    if ((mBid.contract == Contract::GUARD_WITHOUT) || (mBid.contract == Contract::GUARD_AGAINST))
    {
        // No discard is made, set the owner of the dog
        if (mBid.contract != Contract(Contract::GUARD_AGAINST))
        {
            mDiscard = mDog;
            mDiscard.SetOwner(Team(Team::ATTACK));
        }
        else
        {
            // Guard _against_, the dog belongs to the defense
            mDiscard = mDog;
            mDiscard.SetOwner(Team(Team::DEFENSE));
        }

        // We do not display the dog and start the deal immediatly
        showDog = false;
    }
    return showDog;
}
/*****************************************************************************/
Contract TarotContext::SetBid(Contract c, bool slam, Place p)
{
    if (c > mBid.contract)
    {
        mBid.contract = c;
        mBid.taker = p;
        mBid.slam = slam;
    }
    else
    {
        c = Contract::PASS;
    }

    return c;
}
/*****************************************************************************/
bool TarotContext::HasDecimal(float f)
{
    return (f - static_cast<int>(f) != 0);
}
/*****************************************************************************/
/**
 * @brief Engine::GetTrick
 * @param turn
 * @return
 */
Deck TarotContext::GetTrick(std::uint8_t turn) const
{
    if (turn >= Tarot::NumberOfCardsInHand(mNbPlayers))
    {
        turn = 0U;
    }
    return mTricks[turn];
}
/*****************************************************************************/
Place TarotContext::GetWinner(std::uint8_t turn) const
{
    if (turn >= Tarot::NumberOfCardsInHand(mNbPlayers))
    {
        turn = 0U;
    }
    return mWinner[turn];
}
/*****************************************************************************/
bool TarotContext::CheckKingCall(const Card &c, Deck::Statistics &stats) const
{
    // On l'appelle King mais en fait le preneur peut appeler une dame (ou cavalier, etc) s'il
    // possède 4 rois ou 4 dames. On va vérifier cela.
    // On analyse toujours le jeu de l'attaquant avant de continuer
    bool cardIsValid = true;
    if (c.GetValue() != Card::KING)
    {
        if (stats.kings == 4)
        {
            if (c.GetValue() != Card::QUEEN)
            {
                if (stats.queens == 4)
                {
                    if (c.GetValue() != Card::KNIGHT)
                    {
                        if (stats.knights == 4)
                        {
                            if (c.GetValue() != Card::JACK)
                            {
                                if (stats.jacks == 4)
                                {
                                    cardIsValid = true;
                                }
                                else
                                {
                                    cardIsValid = false; // nah, il n'a pas 4 valets
                                }
                            }
                        }
                        else
                        {
                            cardIsValid = false; // nah, il n'a pas 4 cavaliers
                        }
                    }
                }
                else
                {
                    cardIsValid = false; // nah, il n'a pas 4 dames
                }
            }
        }
        else
        {
            cardIsValid = false; // nah, il n'a pas 4 rois
        }
    }

    return cardIsValid;
}
/*****************************************************************************/
Place TarotContext::GetOwner(Place firstPlayer,const Card &card, int turn) const
{
    Place p = firstPlayer;
    std::uint8_t numberOfPlayers = mTricks[turn].Size();

    for (const auto &c : mTricks[turn])
    {
        if (card == c)
        {
            break;
        }
        p = p.Next(numberOfPlayers); // max before rollover is the number of players
    }
    return p;
}
/*****************************************************************************/
/**
 * @brief Generate a file with all played cards of the deal
 */
void TarotContext::SaveToJson(JsonObject &json) const
{
    json.AddValue("version", TAROT_CONTEXT_VERSION);

    // ========================== Game information ==========================
    JsonObject dealInfo;

    // Players are sorted from south to north-west, anti-clockwise (see Place class)
    dealInfo.AddValue("number_of_players", mNbPlayers);
    dealInfo.AddValue("taker", mBid.taker.ToString());
    dealInfo.AddValue("contract", mBid.contract.ToString());
    dealInfo.AddValue("slam", mBid.slam);
    dealInfo.AddValue("first_trick_lead", mFirstPlayer.ToString());
    dealInfo.AddValue("dog", mDog.ToString());
    dealInfo.AddValue("attack_handle", mAttackHandle.ToString());
    dealInfo.AddValue("defense_handle", mDefenseHandle.ToString());
    json.AddValue("deal_info", dealInfo);

    // ========================== Played cards ==========================
    JsonArray tricks;

    for (std::uint32_t i = 0U; i < Tarot::NumberOfCardsInHand(mNbPlayers); i++)
    {
        tricks.AddValue(mTricks[i].ToString());
    }
    json.AddValue("tricks", tricks);
}
/*****************************************************************************/
bool TarotContext::LoadFromJson(const JsonValue &json)
{
    bool ret = true;

    Initialize();

    std::uint32_t numberOfPlayers;
    JsonValue players = json.FindValue("deal_info:number_of_players");
    numberOfPlayers = static_cast<std::uint32_t>(players.GetInteger());
    if ((numberOfPlayers == 3U) ||
            (numberOfPlayers == 4U) ||
            (numberOfPlayers == 5U))
    {
        Tarot::Bid bid;

        std::string str_value;
        if (json.GetValue("deal_info:taker", str_value))
        {
            bid.taker = str_value;
        }
        else
        {
            ret = false;
        }
        if (json.GetValue("deal_info:contract", str_value))
        {
            bid.contract = str_value;
        }
        else
        {
            ret = false;
        }
        bool slam;
        if (json.GetValue("deal_info:slam", slam))
        {
            bid.slam = slam;
        }
        else
        {
            ret = false;
        }

        if (json.GetValue("deal_info:dog", str_value))
        {
            mDog.SetCards(str_value);
        }
        else
        {
            ret = false;
        }

        if (json.GetValue("deal_info:attack_handle", str_value))
        {
            mAttackHandle.SetCards(str_value);
        }
        else
        {
            ret = false;
        }

        if (json.GetValue("deal_info:defense_handle", str_value))
        {
            mDefenseHandle.SetCards(str_value);
        }
        else
        {
            ret = false;
        }

        if (!json.GetValue("deal_info:first_trick_lead", str_value))
        {
            ret = false;
        }

        if (ret)
        {
#ifdef UNIT_TEST
            std::cout << "First player: " << str_value << std::endl;
#endif
            mFirstPlayer = Place(str_value);
            mBid = bid;

            // Load played cards
            JsonValue tricks = json.FindValue("tricks");
            if (tricks.GetArray().Size() == Tarot::NumberOfCardsInHand(numberOfPlayers))
            {
                std::uint8_t trickCounter = 1U;
                mDiscard.CreateTarotDeck();

                for (JsonArray::iterator iter = tricks.GetArray().begin(); iter != tricks.GetArray().end(); ++iter)
                {
                    Deck trick(iter->GetString());

                    if (trick.Size() == numberOfPlayers)
                    {
                        Place winner = SetTrick(trick, trickCounter);
#ifdef UNIT_TEST
                        (void) winner;
            //            std::cout << "Trick: " << (int)trickCounter << ", Cards: " << trick.ToString() << ", Winner: " << winner.ToString() << std::endl;
#else
                        (void) winner;
#endif
                        // Remove played cards from this deck
                        if (mDiscard.RemoveDuplicates(trick) != numberOfPlayers)
                        {
                            std::stringstream msg;
                            msg << "Bad deal contents, trick: " << (int)trickCounter;
                            TLogError(msg.str());
                            ret = false;
                        }
                        trickCounter++;
                    }
                    else
                    {
                        std::stringstream msg;
                        msg << "Bad deal contents at trick: " << (int)trickCounter;
                        TLogError(msg.str());
                        ret = false;
                    }
                }

                // Now that we have removed all the played cards from the mDiscard deck,
                // it should contains only the discard cards
                if (mDiscard.Size() == Tarot::NumberOfDogCards(numberOfPlayers))
                {
#ifdef UNIT_TEST
                    std::cout << "Discard: " << mDiscard.ToString() << std::endl;
#endif

                    // Give the cards to the right team owner
                    if (bid.contract == Contract::GUARD_AGAINST)
                    {
                        mDiscard.SetOwner(Team(Team::DEFENSE));
                    }
                    else
                    {
                        mDiscard.SetOwner(Team(Team::ATTACK));
                    }
                }
                else
                {
                    std::stringstream msg;
                    msg << "Bad discard size: " << (int)mDiscard.Size() << ", contents: " << mDiscard.ToString();
                    TLogError(msg.str());
                    ret = false;
                }
            }
            else
            {
                TLogError("Bad deal contents");
                ret = false;
            }
        }
    }
    else
    {
        TLogError("Bad number of players in the array");
        ret = false;
    }

    return ret;
}

/*****************************************************************************/
/**
 * @brief SetTrick
 *
 * Store the current trick into memory and calculate the trick winner.
 *
 * This methods does not verify if the trick is consistent with the rules; this
 * is supposed to have been previously verified.
 *
 * @param[in] trickCounter Must begin at 1 (first trick of the deal)
 * @return The place of the winner of this trick
 */
Place TarotContext::SetTrick(const Deck &trick, std::uint8_t trickCounter)
{
    std::uint8_t turn = trickCounter - 1U;
    Place winner;
    std::uint8_t numberOfPlayers = trick.Size();
    Place firstPlayer;

    if (turn == 0U)
    {
        firstPlayer = mFirstPlayer; // First trick, we take the first player after the dealer
    }
    else
    {
        // trick counter 2 and beyond
        firstPlayer = mWinner[turn - 1U];
    }

    // Get the number of tricks we must play, it depends of the number of players
    int numberOfTricks = Tarot::NumberOfCardsInHand(numberOfPlayers);

    if (turn < 24U)
    {
        // Bonus: Fool
        bool foolSwap = false;  // true if the fool has been swaped of teams
        mTricks[turn] = trick;
        Card cLeader;

        // Each trick is won by the highest trump in it, or the highest card
        // of the suit led if no trumps were played.
        cLeader = trick.HighestTrump();
        if (!cLeader.IsValid())
        {
            // lead color is the first one, except if the first card is a fool. In that case, the second player plays the lead color
            cLeader = trick.HighestSuit();
        }

        if (!cLeader.IsValid())
        {
            TLogError("cLeader cannot be invalid!");
        }
        else
        {
            // The trick winner is the card leader owner
            winner = GetOwner(firstPlayer, cLeader, turn);

            if (mTricks[turn].HasFool())
            {
                Card cFool = mTricks[turn].GetCard("00-T");
                if (!cFool.IsValid())
                {
                    TLogError("Card cannot be invalid");
                }
                else
                {
                    Place foolPlace = GetOwner(firstPlayer, cFool, turn);
                    Team winnerTeam = ((winner == mBid.taker) || (winner == mBid.partner)) ? Team(Team::ATTACK) : Team(Team::DEFENSE);
                    Team foolTeam = (foolPlace == mBid.taker) ? Team(Team::ATTACK) : Team(Team::DEFENSE);

                    if (Tarot::IsDealFinished(trickCounter, numberOfPlayers))
                    {
                        // Special case of the fool: if played at last turn with a slam realized, it wins the trick
                        if ((mTricksWon == (numberOfTricks - 1)) &&
                                (foolPlace == mBid.taker))
                        {
                            winner = mBid.taker;
                        }
                        // Otherwise, the fool is _always_ lost if played at the last trick, even if the
                        // fool belongs to the same team than the winner of the trick.
                        else
                        {
                            if (winnerTeam == foolTeam)
                            {
                                foolSwap = true;
                            }
                        }
                    }
                    else
                    {
                        // In all other cases, the fool is kept by the owner. If the trick is won by a
                        // different team than the fool owner, they must exchange 1 low card with the fool.
                        if (winnerTeam != foolTeam)
                        {
                            foolSwap = true;
                        }
                    }
                }
            }
        }

        if ((winner == mBid.taker) || (winner == mBid.partner))
        {
            mTricks[turn].SetOwner(Team(Team::ATTACK));
            mTricks[turn].AnalyzeTrumps(mStatsAttack);
            mTricksWon++;

            if (foolSwap)
            {
                mStatsAttack.points -= 4; // defense keeps its points
                mStatsAttack.oudlers--; // attack looses an oudler! what a pity!
            }
        }
        else
        {
            mTricks[turn].SetOwner(Team(Team::DEFENSE));
            if (foolSwap)
            {
                mStatsAttack.points += 4; // get back the points
                mStatsAttack.oudlers++; // hey, it was MY oudler!
            }
        }

        // Save the current winner for the next turn
        mWinner[turn] = winner;
    }
    else
    {
        TLogError("Index out of scope!");
    }

    return winner;
}
/*****************************************************************************/
void TarotContext::AnalyzeGame(Points &points)
{
    std::uint8_t numberOfTricks = Tarot::NumberOfCardsInHand(mNbPlayers);
    std::uint8_t lastTrick = numberOfTricks - 1U;

    // 1. Slam detection
    if ((mTricksWon == numberOfTricks) || (mTricksWon == 0U))
    {
        points.slamDone = true;
    }
    else
    {
        points.slamDone = false;
    }

    // 2. Attacker points, we add the dog if needed
    if (mDiscard.GetOwner() == Team::ATTACK)
    {
        mDiscard.AnalyzeTrumps(mStatsAttack);
    }

    // 3. One of trumps in the last trick bonus detection
    if (points.slamDone)
    {
        // With a slam, the 1 of Trump bonus is valid if played
        // in the penultimate trick AND if the fool is played in the last trick
        if (mTricks[lastTrick].HasFool())
        {
            lastTrick--;
        }
    }
    if (mTricks[lastTrick].HasOneOfTrump())
    {
        points.littleEndianOwner = mTricks[lastTrick].GetOwner();
    }
    else
    {
        points.littleEndianOwner = Team::NO_TEAM;
    }

    // 4. The number of oudler(s) decides the points to do
    points.oudlers = mStatsAttack.oudlers;

    // 4.1 On sauvegarde les points bruts de l'attaque avant ajustements
    // Peut éventuellement servir pour analyser le jeu (ou le montrer)
    points.cardsPointsAttack = mStatsAttack.points;

    // 4.2 : à 3 ou 5 joueurs, le demi-point va au gagnant
    if (HasDecimal(mStatsAttack.points))
    {
        if (mStatsAttack.points >= Tarot::PointsToDo(points.oudlers))
        {
            // le demi-point lui-revient
            mStatsAttack.points += 0.5;
        }
        else
        {
            // il perd le demi-point
            mStatsAttack.points -= 0.5;
        }
    }

    // 5. We save the points done by the attacker
    points.pointsAttack = static_cast<int>(mStatsAttack.points); // voluntary ignore digits after the coma

    // 6. Handle bonus: Ces primes gardent la même valeur quel que soit le contrat.
    // La prime est acquise au camp vainqueur de la donne.
    points.handlePoints += Tarot::GetHandlePoints(mNbPlayers, Tarot::GetHandleType(mAttackHandle.Size()));
    points.handlePoints += Tarot::GetHandlePoints(mNbPlayers, Tarot::GetHandleType(mDefenseHandle.Size()));
}

