#include "Network.h"
#include "Log.h"


static const Ack gAckList[] = {
    {"AckJoinTable", Engine::WAIT_FOR_PLAYERS},
    {"Ready", Engine::WAIT_FOR_READY },
    {"NewDeal", Engine::WAIT_FOR_CARDS },
    {"ShowBid", Engine::WAIT_FOR_SHOW_BID },
    {"AllPassed", Engine::WAIT_FOR_ALL_PASSED },
    {"ShowKingCall", Engine::WAIT_FOR_SHOW_KING_CALL },
    {"ShowDog", Engine::WAIT_FOR_SHOW_DOG },
    {"StartDeal", Engine::WAIT_FOR_START_DEAL },
    {"ShowHandle", Engine::WAIT_FOR_SHOW_HANDLE },
    {"Card", Engine::WAIT_FOR_SHOW_CARD },
    {"EndOfTrick", Engine::WAIT_FOR_END_OF_TRICK },
    {"EndOfDeal", Engine::WAIT_FOR_END_OF_DEAL }
};

static const std::uint32_t gAckListSize = sizeof(gAckList) / sizeof(gAckList[0]);

Engine::Sequence Ack::FromString(const std::string &step)
{
    Engine::Sequence seq = Engine::BAD_STEP;
    for (std::uint32_t i = 0U; i < gAckListSize; i++)
    {
        if (gAckList[i].step == step)
        {
            seq = gAckList[i].sequence;
            break;
        }
    }
    return seq;
}

std::string Ack::ToString(Engine::Sequence sequence)
{
    std::string step = "BadSeq";
    for (std::uint32_t i = 0U; i < gAckListSize; i++)
    {
        if (gAckList[i].sequence == sequence)
        {
            step = gAckList[i].step;
            break;
        }
    }
    return step;
}
