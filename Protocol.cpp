/*=============================================================================
 * TarotClub - Protocol.cpp
 *=============================================================================
 * Network tarot game protocol
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

#include <iomanip>
#include <cstring>
#include <sstream>
#include "Protocol.h"
#include "Log.h"
#include "Util.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"
#include "mbedtls/gcm.h"
#include "mbedtls/aes.h"

// Specific static UUID
const std::uint32_t Protocol::INVALID_UID       = 0U;
const std::uint32_t Protocol::LOBBY_UID         = 1U;

const std::uint32_t Protocol::USERS_UID         = 10U;
const std::uint32_t Protocol::MAXIMUM_USERS     = 200U;
const std::uint32_t Protocol::TABLES_UID        = 1000U;
const std::uint32_t Protocol::MAXIMUM_TABLES    = 50U;


const std::uint32_t Protocol::cTagSize              = 16U;
const std::uint32_t Protocol::cIVSize               = 12U;


const std::uint16_t Protocol::cOptionClearData      = 0U;
const std::uint16_t Protocol::cOptionCypheredData   = 1U;

/**
 * \page protocol Protocol format
 * The aim of the protocol is to be simple and printable (all ASCII).
 * Room is reserved for future improvements such as encryption facilities
 *
 *     OO:SSSS:DDDD:LLLL:TTTT:<argument>
 *
 * OO protocol option byte, in HEX (ex: B4)
 * SSSS is always a 4 digits unsigned integer in HEX that indicates the source UUID (max: FFFF)
 * DDDD same format, indicates the destination UUID (max: FFFF)
 * LLLL: the length of the argument (can be zero), followed by the payload bytes <argument>, typically in JSON format that allow complex structures
 * TTTT: Packet type in ASCII
 */

/*****************************************************************************/
Protocol::Protocol()
{


}
/*****************************************************************************/
Protocol::~Protocol()
{

}
/*****************************************************************************/
void Protocol::Encrypt(const std::string &aad, const std::string &payload, const std::string &iv, std::string &output)
{
    uint8_t tag[cTagSize];
    uint8_t scratch[payload.size()];

    mbedtls_gcm_context ctx;
    mbedtls_gcm_init (&ctx);
    mbedtls_gcm_setkey (&ctx, MBEDTLS_CIPHER_ID_AES, reinterpret_cast<const unsigned char *>(mKey.data()), 128);

    mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT, payload.size(),
                            reinterpret_cast<const unsigned char *>(iv.data()), 12,
                            reinterpret_cast<const unsigned char *>(aad.data()), aad.size(),
                              reinterpret_cast<const unsigned char *>(payload.data()),
                            scratch, cTagSize,tag
                            );
    /*

    mbedtls_gcm_starts (&ctx, MBEDTLS_GCM_ENCRYPT,
                        reinterpret_cast<const unsigned char *>(iv.data()), 12,
                        reinterpret_cast<const unsigned char *>(aad.data()), aad.size());

    int bound_size = (payload.size() >> 4) * 16;
    int last_packet_size = payload.size() - bound_size;

    if (bound_size > 0)
    {
        mbedtls_gcm_update (&ctx, bound_size, reinterpret_cast<const unsigned char *>(payload.data()), scratch);
    }
    if (last_packet_size > 0)
    {
        mbedtls_gcm_update (&ctx, last_packet_size, reinterpret_cast<const unsigned char *>(payload.data() + bound_size) , scratch + bound_size);
    }

    // After the loop
    mbedtls_gcm_finish (&ctx, tag, cTagSize);
*/
    mbedtls_gcm_free (&ctx);

    output.append(iv);
    output.append(reinterpret_cast<char *>(scratch), sizeof(scratch));
    output.append(reinterpret_cast<char *>(tag), cTagSize);

}
/*****************************************************************************/
bool Protocol::Decrypt(const std::string_view &aad, uint8_t *ciphered, uint32_t size, std::string &output) const
{
    uint8_t tag_computed[cTagSize];
    uint8_t tag[cTagSize];
    uint8_t iv[cIVSize];

    uint8_t *payload = ciphered + cIVSize;

    memcpy(tag, ciphered + (size - cTagSize), cTagSize);
    memcpy(iv, ciphered , cIVSize);

    uint32_t plainTextSize = size - (cIVSize + cTagSize);
    uint8_t plainText[plainTextSize];

    mbedtls_gcm_context ctx;
    mbedtls_gcm_init (&ctx);
    mbedtls_gcm_setkey (&ctx, MBEDTLS_CIPHER_ID_AES, reinterpret_cast<const unsigned char *>(mKey.data()), 128);

    mbedtls_gcm_auth_decrypt(&ctx, plainTextSize,
                              reinterpret_cast<const unsigned char *>(iv), 12,
                              reinterpret_cast<const unsigned char *>(aad.data()), aad.size(),
                             reinterpret_cast<const unsigned char *>(tag), cTagSize,
                              reinterpret_cast<const unsigned char *>(payload),
                             reinterpret_cast<unsigned char *>(plainText)
                              );

    /*
    mbedtls_gcm_starts (&ctx, MBEDTLS_GCM_DECRYPT,
                        iv, cIVSize,
                        reinterpret_cast<const unsigned char *>(aad.data()), aad.size());

    int bound_size = (plainTextSize >> 4) * 16;
    int last_packet_size = plainTextSize - bound_size;

    if (bound_size > 0)
    {
        mbedtls_gcm_update (&ctx, bound_size, payload, plainText);
    }
    if (last_packet_size > 0)
    {
        mbedtls_gcm_update (&ctx, last_packet_size, payload + bound_size , plainText + bound_size);
    }

    // After the loop
    mbedtls_gcm_finish (&ctx, tag_computed, cTagSize);
*/

    mbedtls_gcm_free (&ctx);
    output.append(reinterpret_cast<char *>(plainText), sizeof(plainText));
    return std::memcmp(&tag_computed[0], tag, cTagSize) == 0;
}
/*****************************************************************************/
std::string Protocol::Build(std::uint32_t src, std::uint32_t dst, const std::string &clearMessage, const std::string &prefix)
{
    std::stringstream stream;
    static const std::uint16_t option = cOptionCypheredData;

    // Prédiction de la taille finale du payload
    uint32_t cipheredPayloadSize = (cIVSize + clearMessage.size() + cTagSize) * 2;

    stream  << std::setfill ('0') << std::setw(2) << std::hex << option << ":"
            << std::setfill ('0') << std::setw(4) << std::hex << src << ":"
            << std::setfill ('0') << std::setw(4) << std::hex << dst << ":"
            << std::setfill ('0') << std::setw(4) << std::hex << cipheredPayloadSize << ":"
            << std::setfill ('0') << std::setw(4) << std::hex << mTxFrameCounter << ":"
            << std::setfill ('0') << std::setw(4) << std::hex << prefix.size() << ":"
            << prefix  << ":";

    // On chiffre
    std::string payload; // IV + data + tag
    std::string iv = "000000000000"; // Util::GenerateRandomString(cIVSize)
    Encrypt(stream.str(), clearMessage, iv, payload); // l'AAD c'est tout l'en-tête + le prefix
    std::string payloadAscii = Util::ToHex(reinterpret_cast<const char *>(payload.data()), payload.size());

    stream << payloadAscii;

    mTxFrameCounter++;
    return stream.str();
}
/*****************************************************************************/
void Protocol::SetSecurity(const std::string &key)
{
    mKey = key;
    mRxFrameCounter = 0;
    mTxFrameCounter = 0;
}
/*****************************************************************************/
bool Protocol::ParseUint32(const char* data, std::uint32_t size, std::uint32_t &value) const
{
    bool ret = false;
    char *ptr;
    value = std::strtoul(data, &ptr, 16);
    std::uint32_t comp_size = static_cast<std::uint32_t>(ptr - data);
    if (size == comp_size)
    {
        ret = true;
    }
    return ret;
}
/*****************************************************************************/
bool Protocol::ParseUint16(const char* data, std::uint32_t size, std::uint16_t &value) const
{
    bool ret = false;
    char *ptr;
    value = std::strtoul(data, &ptr, 16);
    std::uint32_t comp_size = static_cast<std::uint16_t>(ptr - data);
    if (size == comp_size)
    {
        ret = true;
    }
    return ret;
}
/*****************************************************************************/
void Protocol::ParsePrefix(Header &h)
{
    if (h.prefix_size > 0)
    {
        if (h.prefix_size < PROTO_MAX_BODY_SIZE)
        {
            h.prefix = std::string(&mData[28], h.prefix_size);
        }
        else
        {
            TLogError("[PROTO] Bad prefix size!");
        }
    }
}
/*****************************************************************************/
// Format d'entrée : IV + cyphered data + Tag
// L'ensemble est transmis en ascii hex
// Le header est utilisé comme Additional Data (s'il est corrompu, on le détectera)
// Format de sortie : clear data
bool Protocol::DecryptPayload(std::string &output, const Header &h) const
{
    uint32_t cipheredPayloadSize = h.payload_size / 2;
    uint8_t ciphered[cipheredPayloadSize];

    // Transformation en ascii > décimal
    Util::HexStringToUint8(std::string_view(Payload(h), h.payload_size), ciphered);

    // l'Additional Data: tout l'en-tête + le prefix
    return Decrypt(std::string_view(Data(), PROTO_HEADER_SIZE + h.prefix_size + 1), ciphered, cipheredPayloadSize, output);
}
/*****************************************************************************/
bool Protocol::ParseHeader(Header &h) const
{
    bool ret = true;

    if ((mData[2] == ':') &&
        (mData[7] == ':') &&
        (mData[12] == ':') &&
        (mData[17] == ':') &&
        (mData[22] == ':') &&
        (mData[27] == ':'))
    {
        ret = ParseUint16(&mData[0], 2, h.option);
        ret = ret && ParseUint32(&mData[3], 4, h.src_uid);
        ret = ret && ParseUint32(&mData[8], 4, h.dst_uid);
        ret = ret && ParseUint32(&mData[13], 4, h.payload_size);
        ret = ret && ParseUint32(&mData[18], 4, h.frame_counter);
        ret = ret && ParseUint32(&mData[23], 4, h.prefix_size);

        if ((h.payload_size + h.prefix_size + 1) >= PROTO_MAX_BODY_SIZE)
        {
            TLogError("[PROTOCOL] Body size too large");
            abort();
        }
    }
    else
    {
        ret = false;
    }

    return ret;
}


//=============================================================================
// End of file Protocol.cpp
//=============================================================================
