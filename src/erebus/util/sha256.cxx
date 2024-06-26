#include <erebus/util/sha256.hxx>

#include <iomanip>
#include <sstream>

namespace Er
{
    
namespace Util
{
    
constexpr std::array<uint32_t, 64> Sha256::K;

Sha256::Sha256() noexcept
    : m_data{}
    , m_blocklen(0)
    , m_bitlen(0)
{
    m_state[0] = 0x6a09e667;
    m_state[1] = 0xbb67ae85;
    m_state[2] = 0x3c6ef372;
    m_state[3] = 0xa54ff53a;
    m_state[4] = 0x510e527f;
    m_state[5] = 0x9b05688c;
    m_state[6] = 0x1f83d9ab;
    m_state[7] = 0x5be0cd19;
}

void Sha256::update(const uint8_t* data, size_t length) noexcept
{
    for (size_t i = 0; i < length; i++) 
    {
        m_data[m_blocklen++] = data[i];
        if (m_blocklen == 64) 
        {
            transform();

            // End of the block
            m_bitlen += 512;
            m_blocklen = 0;
        }
    }
}

void Sha256::update(const std::string& data) noexcept 
{
    update(reinterpret_cast<const uint8_t*> (data.c_str()), data.size());
}

void Sha256::update(std::string_view data) noexcept
{
    update(reinterpret_cast<const uint8_t*> (data.data()), data.size());
}

Sha256::Hash Sha256::digest() noexcept 
{
    Hash hash;

    pad();
    revert(hash);

    return hash;
}

uint32_t Sha256::rotr(uint32_t x, uint32_t n) noexcept 
{
    return (x >> n) | (x << (32 - n));
}

uint32_t Sha256::choose(uint32_t e, uint32_t f, uint32_t g) noexcept 
{
    return (e & f) ^ (~e & g);
}

uint32_t Sha256::majority(uint32_t a, uint32_t b, uint32_t c) noexcept 
{
    return (a & (b | c)) | (b & c);
}

uint32_t Sha256::sig0(uint32_t x) noexcept 
{
    return Sha256::rotr(x, 7) ^ Sha256::rotr(x, 18) ^ (x >> 3);
}

uint32_t Sha256::sig1(uint32_t x) noexcept 
{
    return Sha256::rotr(x, 17) ^ Sha256::rotr(x, 19) ^ (x >> 10);
}

void Sha256::transform() noexcept 
{
    uint32_t maj, xorA, ch, xorE, sum, newA, newE, m[64];
    uint32_t state[8];

    for (uint8_t i = 0, j = 0; i < 16; i++, j += 4) 
    { 
        // Split data in 32 bit blocks for the 16 first words
        m[i] = (m_data[j] << 24) | (m_data[j + 1] << 16) | (m_data[j + 2] << 8) | (m_data[j + 3]);
    }

    for (uint8_t k = 16; k < 64; k++) 
    { 
        // Remaining 48 blocks
        m[k] = Sha256::sig1(m[k - 2]) + m[k - 7] + Sha256::sig0(m[k - 15]) + m[k - 16];
    }

    for (uint8_t i = 0; i < 8; i++) 
    {
        state[i] = m_state[i];
    }

    for (uint8_t i = 0; i < 64; i++) 
    {
        maj = Sha256::majority(state[0], state[1], state[2]);
        xorA = Sha256::rotr(state[0], 2) ^ Sha256::rotr(state[0], 13) ^ Sha256::rotr(state[0], 22);

        ch = choose(state[4], state[5], state[6]);

        xorE = Sha256::rotr(state[4], 6) ^ Sha256::rotr(state[4], 11) ^ Sha256::rotr(state[4], 25);

        sum = m[i] + K[i] + state[7] + ch + xorE;
        newA = xorA + maj + sum;
        newE = state[3] + sum;

        state[7] = state[6];
        state[6] = state[5];
        state[5] = state[4];
        state[4] = newE;
        state[3] = state[2];
        state[2] = state[1];
        state[1] = state[0];
        state[0] = newA;
    }

    for (uint8_t i = 0; i < 8; i++) 
    {
        m_state[i] += state[i];
    }
}

void Sha256::pad() noexcept 
{

    uint64_t i = m_blocklen;
    uint8_t end = m_blocklen < 56 ? 56 : 64;

    m_data[i++] = 0x80; // Append a bit 1
    while (i < end) 
    {
        m_data[i++] = 0x00; // Pad with zeros
    }

    if (m_blocklen >= 56) 
    {
        transform();
        std::memset(m_data, 0, 56);
    }

    // Append to the padding the total message's length in bits and transform.
    m_bitlen += m_blocklen * 8;
    m_data[63] = m_bitlen;
    m_data[62] = m_bitlen >> 8;
    m_data[61] = m_bitlen >> 16;
    m_data[60] = m_bitlen >> 24;
    m_data[59] = m_bitlen >> 32;
    m_data[58] = m_bitlen >> 40;
    m_data[57] = m_bitlen >> 48;
    m_data[56] = m_bitlen >> 56;
    transform();
}

void Sha256::revert(Hash& hash) noexcept 
{
    // SHA uses big endian byte ordering
    // Revert all bytes
    for (uint8_t i = 0; i < 4; i++) 
    {
        for (uint8_t j = 0; j < 8; j++) 
        {
            hash[i + (j * 4)] = (m_state[j] >> (24 - i * 8)) & 0x000000ff;
        }
    }
}

std::string Sha256::str(const Hash& digest) 
{
    std::stringstream s;
    s << std::setfill('0') << std::hex;

    for (uint8_t i = 0; i < 32; i++) 
    {
        s << std::setw(2) << (unsigned int)digest[i];
    }

    return s.str();
}


} // namespace Util {}
    
} // namespace Er {}