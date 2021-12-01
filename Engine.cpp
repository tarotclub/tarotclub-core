/*=============================================================================
 * TarotClub - Engine.cpp
 *=============================================================================
 * Main Tarot engine
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

#include <chrono>
#include <random>
#include <sstream>
#include <iostream>

#include "Engine.h"
#include "DealGenerator.h"
#include "Identity.h"
#include "Util.h"
#include "Log.h"
#include "System.h"
#include "JsonReader.h"

/*****************************************************************************/
Engine::Engine()
    : mSequence(STOPPED)
    , mPosition(0U)
    , mTrickCounter(0U)
{
    std::chrono::system_clock::rep seed = std::chrono::system_clock::now().time_since_epoch().count(); // rep is long long
    mSeed = static_cast<std::uint32_t>(seed);

    mCtx.Initialize();

    for (std::uint8_t i = 0U; i < 5U; i++)
    {
        mHandleAsked[i] = false;
    }
}
/*****************************************************************************/
Engine::~Engine()
{

}
/*****************************************************************************/
/**
 * @brief Engine::Initialize
 * Call this method before clients connections
 */
void Engine::Initialize()
{
    mSequence = STOPPED;
}
/*****************************************************************************/
void Engine::CreateTable(std::uint8_t nbPlayers)
{
    // Save parameters
    mCtx.mNbPlayers = nbPlayers;

    // 1. Initialize internal states
    mCtx.Initialize();

    // Choose the dealer
    mDealer = DealGenerator::RandomPlace(mCtx.mNbPlayers);

    // Wait for ready
    mSequence = WAIT_FOR_PLAYERS;
}
/*****************************************************************************/
void Engine::NewGame()
{
    mSequence = WAIT_FOR_READY;
}
/*****************************************************************************/
Tarot::Distribution Engine::NewDeal(const Tarot::Distribution &shuffle)
{
    Tarot::Distribution shReturned = shuffle;

    // 1. Initialize internal states
    mCtx.Initialize();

    mPosition = 0U;
    currentTrick.Clear();

    // 2. Choose the dealer and the first player to start the bid
    mDealer = mDealer.Next(mCtx.mNbPlayers);
    mCurrentPlayer = mDealer.Next(mCtx.mNbPlayers); // The first player on the dealer's right begins the bid

    // 3. Give cards to all players
    CreateDeal(shReturned);

    // 4. Prepare the wait for ack
    mSequence = WAIT_FOR_CARDS;

    return shReturned;
}
/*****************************************************************************/
/**
 * @brief Engine::StartDeal
 * @return The first player to play
 */
Place Engine::StartDeal()
{
    mTrickCounter = 0U;
    mPosition = 0U;

    for (std::uint8_t i = 0U; i < 5U; i++)
    {
        mHandleAsked[i] = false;
    }

    // In case of slam, the first player to play is the taker.
    // Otherwise, it is the player on the right of the dealer
    if (mCtx.mBid.slam == true)
    {
        mCurrentPlayer = mCtx.mBid.taker;
    }
    else
    {
        mCurrentPlayer = mDealer.Next(mCtx.mNbPlayers); // The first player on the dealer's right
    }

    std::stringstream ss;
    ss << "Taker: " << mCtx.mBid.taker.ToString() << " / ";
    ss << "Contract: " << mCtx.mBid.contract.ToString();
    TLogInfo(ss.str());

    mCtx.mFirstPlayer = mCurrentPlayer;
    return mCurrentPlayer;
}
/*****************************************************************************/
bool Engine::SetDiscard(const Deck &discard)
{
    bool valid = mPlayers[mCtx.mBid.taker.Value()].TestDiscard(discard, mCtx.mDog, mCtx.mNbPlayers);

    if (valid)
    {
        // Add the dog to the player's deck, and then filter the discard
        mPlayers[mCtx.mBid.taker.Value()] += mCtx.mDog;
        mPlayers[mCtx.mBid.taker.Value()].RemoveDuplicates(discard);
        mCtx.mDiscard = discard;
        mCtx.mDiscard.SetOwner(Team(Team::ATTACK));

        std::stringstream ss;
        ss << "Received discard: " << discard.ToString() << " / ";
        ss << "Taker's deck after the discard: " << mPlayers[mCtx.mBid.taker.Value()].ToString();
        TLogInfo(ss.str());
        mSequence = WAIT_FOR_START_DEAL;
    }
    return valid;
}
/*****************************************************************************/
/**
 * @brief Engine::SetHandle
 * @param handle
 * @param p
 * @return  true if the handle is valid, otherwise false
 */
bool Engine::SetHandle(const Deck &handle, Place p)
{
    bool valid = mPlayers[p.Value()].TestHandle(handle);

    if (valid)
    {
        mCtx.SetHandle(handle, p);

        mSequence = WAIT_FOR_SHOW_HANDLE;
    }
    return valid;
}
/*****************************************************************************/
bool Engine::SetCard(const Card &c, Place p)
{
    bool ret = false;

    if (mPlayers[p.Value()].CanPlayCard(c, currentTrick))
    {
        currentTrick.Append(c);
        mPlayers[p.Value()].Remove(c);

        std::stringstream ss;
        ss << "Turn: " << (int)mTrickCounter + 1 << ", Tick: " << currentTrick.ToString() <<  ", Player " << p.ToString() << " played " << c.ToString() << " Engine player deck is: " << mPlayers[p.Value()].ToString();
        TLogInfo(ss.str());

        // ------- PREPARE NEXT ONE
        mPosition++; // done for this player
        mCurrentPlayer = mCurrentPlayer.Next(mCtx.mNbPlayers); // next player!
        mSequence = WAIT_FOR_SHOW_CARD;
        ret = true;
    }
    else
    {
        std::stringstream ss;
        ss << "The player " << p.ToString() << " cannot play the card: " << c.ToString()
           << " on turn " << (int)mTrickCounter + 1 << " Engine deck is: " << mPlayers[p.Value()].ToString();
        TLogError(ss.str());
    }
    return ret;
}
/*****************************************************************************/
Contract Engine::SetBid(Contract c, bool slam, Place p)
{
    c = mCtx.SetBid(c, slam, p);

    // ------- PREPARE NEXT ONE
    mPosition++; // done for this player
    mCurrentPlayer = mCurrentPlayer.Next(mCtx.mNbPlayers); // next player!
    mSequence = WAIT_FOR_SHOW_BID;
    return c;
}
/*****************************************************************************/
bool Engine::SetKingCalled(const Card &c)
{
    bool success = false;
    Deck::Statistics stats;
    mPlayers[mCtx.mBid.taker.Value()].AnalyzeSuits(stats);

    if (mCtx.CheckKingCall(c, stats))
    {

        // Appel au roi dans le chien ou dans le deck du preneur ?
        if (mCtx.mDog.HasCard(c) || mPlayers[mCtx.mBid.taker.Value()].HasCard(c))
        {
            // Il est tout seul car il a appelé une carte à lui ou au chien
            mCtx.mBid.partner = mCtx.mBid.taker;
            success = true;
        }
        else
        {
            // On recherche son partenaire
            Place partner = mCtx.mBid.taker.Next(5);
            for (uint32_t i = 0; i < 4; i++)
            {
                if (mPlayers[partner.Value()].HasCard(c))
                {
                    mCtx.mBid.partner = partner;
                    success = true;
                    break;
                }
                partner = partner.Next(5);
            }
        }

        mCtx.mKingCalled = c; // sauvegarde du roi appelé

        mSequence = Engine::WAIT_FOR_SHOW_KING_CALL;
    }

    return success;
}
/*****************************************************************************/
void Engine::Stop()
{
    mSequence = STOPPED;
}
/*****************************************************************************/
Deck Engine::GetDeck(Place p)
{
    Deck deck;

    if (p < Place(Place::NOWHERE))
    {
        deck = mPlayers[p.Value()];
    }

    return deck;
}
/*****************************************************************************/
Points Engine::GetCurrentGamePoints()
{
    return mCurrentPoints;
}
/*****************************************************************************/
/**
 * @brief Engine::GameSequence
 * @return true if the trick is finished
 */
void Engine::GameSequence()
{
    // If end of trick, prepare next one
    if (IsEndOfTrick())
    {
        TLogInfo("----------------------------------------------------\n");

        // The current trick winner will begin the next trick
        mCurrentPlayer = mCtx.SetTrick(currentTrick, mTrickCounter);
        currentTrick.Clear();
        mSequence = WAIT_FOR_END_OF_TRICK;
    }
    // Special case of first round: players can declare a handle
    else if ((mTrickCounter == 0U) &&
             (!mHandleAsked[mCurrentPlayer.Value()]))
    {
        mHandleAsked[mCurrentPlayer.Value()] = true;
        mSequence = WAIT_FOR_HANDLE;
    }
    else
    {
        std::stringstream message;

        message << "Turn: " << (std::uint32_t)mTrickCounter << " player: " << mCurrentPlayer.ToString();
        TLogInfo(message.str());

        mSequence = WAIT_FOR_PLAYED_CARD;
    }
}
/*****************************************************************************/
void Engine::EndOfDeal(JsonObject &json)
{
    mCurrentPoints.Clear();
    mCtx.AnalyzeGame(mCurrentPoints);
    mCtx.SaveToJson(json);

    mSequence = WAIT_FOR_END_OF_DEAL;
}
/*****************************************************************************/
void Engine::ManageAfterBidSequence()
{
    if (mCtx.ManageDogAfterBid())
    {
        // Show the dog to all the players
        mSequence = WAIT_FOR_SHOW_DOG;
    }
    else
    {
        // We do not display the dog and start the deal immediatly
        mSequence = WAIT_FOR_START_DEAL;
    }
}
/*****************************************************************************/

/**
 * @brief Engine::BidSequence
 * @return The next sequence to go
 */
void Engine::BidSequence()
{
    // If a slam has been announced, we start immediately the deal
    if (IsEndOfTrick() || mCtx.mBid.slam)
    {
        if (mCtx.mBid.contract == Contract::PASS)
        {
            // All the players have passed, deal again new cards
            mSequence = WAIT_FOR_ALL_PASSED;
        }
        // On a terminé les enchères, on bascule sur le choix du roi
        // dans le cas du jeu à 5 joueurs
        else if ((mCtx.mNbPlayers == 5) && (mSequence == WAIT_FOR_SHOW_BID))
        {
            mCtx.mBid.partner = mCtx.mBid.taker; // par défaut, le partenaire est le preneur (cas à 5 joueurs que l'on tente
            // de généraliser pour tous les autres modes de jeu
            mSequence = WAIT_FOR_KING_CALL;
        }
        else
        {
            ManageAfterBidSequence();
        }
    }
    else
    {
        mSequence = WAIT_FOR_BID;
    }
}
/*****************************************************************************/
void Engine::DiscardSequence()
{
    mSequence = WAIT_FOR_DISCARD;
}
/*****************************************************************************/
bool Engine::IsEndOfTrick()
{
    bool endOfTrick = false;
    if (mPosition >= mCtx.mNbPlayers)
    {
        // Trick as ended, all the players have played
        mPosition = 0U;
        mTrickCounter++;
        endOfTrick = true;
    }
    return endOfTrick;
}
/*****************************************************************************/
void Engine::CreateDeal(Tarot::Distribution &shuffle)
{
    DealGenerator editor;
    bool random = true;

    if (shuffle.mType == Tarot::Distribution::CUSTOM_DEAL)
    {
        std::string fullPath;

        // If not an absolute path, then it is a path relative to the home directory
        if (!Util::FileExists(shuffle.mFile))
        {
            fullPath = System::HomePath() + shuffle.mFile;
        }
        else
        {
            fullPath = shuffle.mFile;
        }

        if (!editor.LoadFile(fullPath))
        {
            // Fall back to default mode
            TLogError("Cannot load custom deal file: " + fullPath);
        }
        else if (editor.IsValid(mCtx.mNbPlayers))
        {
            random = false;
            // Override the current player
            mCurrentPlayer = editor.GetFirstPlayer();
            mDealer = mCurrentPlayer.Previous(mCtx.mNbPlayers);
        }
        else
        {
            // Fall back to default mode
            TLogError("Invalid deal file");
        }
    }

    if (random)
    {
        bool valid = true;
        do
        {
            if (shuffle.mType == Tarot::Distribution::NUMBERED_DEAL)
            {
                valid = editor.CreateRandomDeal(mCtx.mNbPlayers, shuffle.mSeed);
                if (!valid)
                {
                    // The provided seed does dot generate a valid deal, so switch to a random one
                    shuffle.mType = Tarot::Distribution::RANDOM_DEAL;
                }
            }
            else
            {
                valid = editor.CreateRandomDeal(mCtx.mNbPlayers);
            }
        }
        while (!valid);

        // Save the seed
        shuffle.mSeed = editor.GetSeed();
    }

#ifdef UNIT_TEST
    editor.SaveFile("unit_test_current_deal.json");
#endif

    // Copy deal editor cards to engine
    for (std::uint32_t i = 0U; i < mCtx.mNbPlayers; i++)
    {
        Place p(i);
        mPlayers[i].Clear();
        mPlayers[i].Append(editor.GetPlayerDeck(p));

        TLogInfo( "Player " + p.ToString() + " deck: " + mPlayers[i].ToString());

#ifdef UNIT_TEST
    std::cout << "Player " + p.ToString() + " deck: " + mPlayers[i].ToString() << std::endl;
#endif
    }
    mCtx.mDog = editor.GetDogDeck();

    TLogInfo("Dog deck: " + editor.GetDogDeck().ToString());
}
/*****************************************************************************/
bool Engine::LoadGameDealLog(const std::string &fileName)
{
    bool ret = false;
    JsonValue json;

#ifdef TAROT_DEBUG
    std::cout << "File: " << fileName << std::endl;
#endif
    if (JsonReader::ParseFile(json, fileName))
    {
        ret = mCtx.LoadFromJson(json);
    }
    else
    {
        TLogError("Cannot open Json deal file");
    }
    return ret;
}
/*****************************************************************************/
bool Engine::LoadGameDeal(const std::string &buffer)
{
    bool ret = false;
    JsonValue json;

    if (JsonReader::ParseString(json, buffer))
    {
        ret = mCtx.LoadFromJson(json);
    }
    else
    {
        TLogError("Cannot analyze JSON buffer");
    }
    return ret;
}

//=============================================================================
// End of file Engine.cpp
//=============================================================================
