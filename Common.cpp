/*=============================================================================
 * TarotClub - Common.cpp
 *=============================================================================
 * Common Tarot related classes
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

#include "Common.h"
#include "Log.h"

/*****************************************************************************/
const std::uint8_t Team::ATTACK     = 0U;
const std::uint8_t Team::DEFENSE    = 1U;
const std::uint8_t Team::NO_TEAM    = 0xFFU;

/*****************************************************************************/
const std::string Place::STR_SOUTH      = "South";
const std::string Place::STR_EAST       = "East";
const std::string Place::STR_NORTH      = "North";
const std::string Place::STR_WEST       = "West";
const std::string Place::STR_FIFTH      = "Fifth";
const std::string Place::STR_NOWHERE    = "Nowhere";

const std::uint8_t Place::SOUTH     = 0;
const std::uint8_t Place::EAST      = 1;
const std::uint8_t Place::NORTH     = 2;
const std::uint8_t Place::WEST      = 3;
const std::uint8_t Place::FIFTH     = 4;
const std::uint8_t Place::NOWHERE   = 5;

std::vector<std::string> Place::mStrings = Place::Initialize();
/*****************************************************************************/

const std::string Contract::STR_NO_ANY         = "";
const std::string Contract::STR_PASS           = "Pass";
const std::string Contract::STR_TAKE           = "Take";
const std::string Contract::STR_GUARD          = "Guard";
const std::string Contract::STR_GUARD_WITHOUT  = "Guard without";
const std::string Contract::STR_GUARD_AGAINST  = "Guard against";

const std::uint8_t Contract::NO_ANY        = 0;
const std::uint8_t Contract::PASS          = 1;
const std::uint8_t Contract::TAKE          = 2;
const std::uint8_t Contract::GUARD         = 3;
const std::uint8_t Contract::GUARD_WITHOUT = 4;
const std::uint8_t Contract::GUARD_AGAINST = 5;

std::vector<std::string> Contract::mStrings = Contract::Initialize();
/*****************************************************************************/
const std::string Tarot::Game::cQuickDealTxt          = "QuickDeal";
const std::string Tarot::Game::cSimpleTournamentTxt   = "SimpleTournament";
const std::string Tarot::Game::cCustomTxt             = "Custom";
/*****************************************************************************/
const std::string Tarot::Distribution::cRandomTxt      = "Random";
const std::string Tarot::Distribution::cNumberedTxt    = "Numbered";
const std::string Tarot::Distribution::cCustomTxt      = "Custom";
/*****************************************************************************/
Place::Place()
    : mPlace(NOWHERE)
{

}
/*****************************************************************************/
Place::Place(std::uint32_t p)
{
    *this = p;
}
/*****************************************************************************/
Place::Place(std::uint8_t p)
{
    *this = p;
}
/*****************************************************************************/
Place::Place(std::string p)
{
    *this = p;
}
/*****************************************************************************/
Place::Place(int p)
    : Place(static_cast<std::uint32_t>(p))
{

}
/*****************************************************************************/
std::string Place::ToString() const
{
    return mStrings[mPlace];
}
/*****************************************************************************/
std::uint8_t Place::Value() const
{
    return mPlace;
}
/*****************************************************************************/
Place Place::Next(std::uint8_t numberOfPlayers)
{
    std::uint8_t place = mPlace;
    place++;

    if ((place >= numberOfPlayers) ||
            (place >= NOWHERE))
    {
        place = Place::SOUTH;
    }
    return Place(place);
}
/*****************************************************************************/
Place Place::Previous(std::uint8_t numberOfPlayers)
{
    std::uint8_t place = mPlace;

    if (place == Place::SOUTH)
    {
        place = numberOfPlayers - 1U;
    }
    else
    {
        place--;
    }
    return Place(place);
}
/*****************************************************************************/
bool Place::IsValid() const
{
    return (mPlace < NOWHERE);
}
/*****************************************************************************/
std::vector<std::string> Place::Initialize()
{
    std::vector<std::string> stringList;

    stringList.push_back(STR_SOUTH);
    stringList.push_back(STR_EAST);
    stringList.push_back(STR_NORTH);
    stringList.push_back(STR_WEST);
    stringList.push_back(STR_FIFTH);
    stringList.push_back(STR_NOWHERE);

    return stringList;
}
/*****************************************************************************/
Contract::Contract()
    : mContract(PASS)
{

}
/*****************************************************************************/
Contract::Contract(std::uint32_t c)
{
    *this = c;
}
/*****************************************************************************/
Contract::Contract(std::uint8_t c)
{
    *this = c;
}
/*****************************************************************************/
Contract::Contract(std::string c)
{
    *this = c;
}
/*****************************************************************************/
std::string Contract::ToString() const
{
    return mStrings[mContract];
}
/*****************************************************************************/
std::uint8_t Contract::Value() const
{
    return mContract;
}
/*****************************************************************************/
std::vector<std::string> Contract::Initialize()
{
    std::vector<std::string> stringList;

    stringList.push_back(STR_NO_ANY);
    stringList.push_back(STR_PASS);
    stringList.push_back(STR_TAKE);
    stringList.push_back(STR_GUARD);
    stringList.push_back(STR_GUARD_WITHOUT);
    stringList.push_back(STR_GUARD_AGAINST);

    return stringList;
}
/*****************************************************************************/
std::uint8_t Tarot::NumberOfCardsInHand(std::uint8_t numberOfPlayers)
{
    if (numberOfPlayers == 3U)
    {
        return 24U;
    }
    if (numberOfPlayers == 5U)
    {
        return 15U;
    }
    else
    {
        // 4 players
        return 18U;
    }
}
/*****************************************************************************/
std::uint8_t Tarot::NumberOfDogCards(std::uint8_t numberOfPlayers)
{
    return (78U - (NumberOfCardsInHand(numberOfPlayers) * numberOfPlayers));
}
/*****************************************************************************/
bool Tarot::IsDealFinished(std::uint8_t trickCounter, std::uint8_t numberOfPlayers)
{
    if (trickCounter >= NumberOfCardsInHand(numberOfPlayers))
    {
        return true;
    }
    else
    {
        return false;
    }
}
/*****************************************************************************/
int Tarot::GetHandlePoints(uint32_t nbPlayers, Tarot::Handle handle)
{
    int points = 0;

    if (nbPlayers == 4)
    {
        if (handle == Tarot::SIMPLE_HANDLE)
        {
            points = 20;
        }
        else if (handle == Tarot::DOUBLE_HANDLE)
        {
            points = 30;
        }
        else  if (handle == Tarot::TRIPLE_HANDLE)
        {
            points = 40;
        }
        else
        {
            points = 0;
        }
    }
    else if (nbPlayers == 3)
    {
        if (handle == Tarot::SIMPLE_HANDLE)
        {
            points = 13;
        }
        else if (handle == Tarot::DOUBLE_HANDLE)
        {
            points = 15;
        }
        else  if (handle == Tarot::TRIPLE_HANDLE)
        {
            points = 18;
        }
        else
        {
            points = 0;
        }
    }
    else if (nbPlayers == 5)
    {
        if (handle == Tarot::SIMPLE_HANDLE)
        {
            points = 8;
        }
        else if (handle == Tarot::DOUBLE_HANDLE)
        {
            points = 10;
        }
        else  if (handle == Tarot::TRIPLE_HANDLE)
        {
            points = 13;
        }
        else
        {
            points = 0;
        }
    }

    return points;
}
/*****************************************************************************/
Tarot::Handle Tarot::GetHandleType(std::uint32_t size)
{
    Tarot::Handle type;

    if (size == 10U)
    {
        type = Tarot::SIMPLE_HANDLE;
    }
    else if (size == 13U)
    {
        type = Tarot::DOUBLE_HANDLE;
    }
    else if (size == 15U)
    {
        type = Tarot::TRIPLE_HANDLE;
    }
    else
    {
        type = Tarot::NO_HANDLE;
    }
    return type;
}
/*****************************************************************************/
std::int32_t Tarot::PointsToDo(std::uint8_t numberOfOudlers)
{
    std::uint32_t pointsToDo;

    if (numberOfOudlers == 0U)
    {
        pointsToDo = 56;
    }
    else if (numberOfOudlers == 1U)
    {
        pointsToDo = 51;
    }
    else if (numberOfOudlers == 2U)
    {
        pointsToDo = 41;
    }
    else // 3 oudlers
    {
        pointsToDo = 36;
    }

    return pointsToDo;
}
/*****************************************************************************/
std::uint32_t Tarot::GetMultiplier(Contract c)
{
    std::uint32_t multiplier;

    if (c == Contract::TAKE)
    {
        multiplier = 1U;
    }
    else if (c == Contract::GUARD)
    {
        multiplier = 2U;
    }
    else if (c == Contract::GUARD_WITHOUT)
    {
        multiplier = 4U;
    }
    else if (c == Contract::GUARD_AGAINST)
    {
        multiplier = 6U;
    }
    else     // GUARD_AGAINST
    {
        multiplier = 0U;
    }
    return multiplier;
}

//=============================================================================
// End of file Common.cpp
//=============================================================================
