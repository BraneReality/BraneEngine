
#include "uuid.h"
#include <chrono>
#include "assert.h"
#include "openssl/rand.h"

UUID::UUID()
{
    for(size_t i = 0; i < 16; ++i)
        data.buffer[i] = 0;
}

UUID::UUID(uint8_t* raw)
{
    std::memcpy(data.buffer, raw, 16);
}

Result<UUID> UUID::generate()
{
    UUID res;

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    res.data.time_low = (uint32_t)(ms >> 16);
    res.data.time_mid = (uint16_t)(ms & 0xFFFF);

    uint8_t random_bytes[10];
    int rc = RAND_bytes(random_bytes, sizeof(random_bytes));
    if(rc != 1)
        return Err(std::string("failed to generate uuid"));

    res.data.time_hi_and_version = (random_bytes[0] << 8 | random_bytes[1]) & 0x0FFF;
    res.data.time_hi_and_version |= (0x7 << 12);

    res.data.clk_seq_hi_res = (random_bytes[2] & 0x3F) | 0x80;
    res.data.clk_seq_low = random_bytes[3];
    std::memcpy(res.data.node, &random_bytes[4], 6);

    return Ok<UUID>(std::move(res));
}

Result<UUID> UUID::fromString(std::string_view text)
{
    if(text.size() != 36)
    {
        return Err(std::format("UUID must be parsed from 36 byte string matching the format 8-4-4-4-12 but the "
                               "constructor was passed '{}' "
                               "with len {}",
                               text,
                               text.size()));
    }
    char buffer[37];
    for(size_t i = 0; i < 37; ++i)
        buffer[i] = '\0';
    std::memcpy(buffer, text.data(), 37);
    UUID ret;
    size_t res = std::sscanf(buffer,
                             "%08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
                             &ret.data.time_low,
                             &ret.data.time_mid,
                             &ret.data.time_hi_and_version,
                             &ret.data.clk_seq_hi_res,
                             &ret.data.clk_seq_low,
                             &ret.data.node[0],
                             &ret.data.node[1],
                             &ret.data.node[2],
                             &ret.data.node[3],
                             &ret.data.node[4],
                             &ret.data.node[5]);
    if(res != 11)
        return Err(std::string("failed to parse uuid"));
    return Ok<UUID>(std::move(ret));
}

bool UUID::operator==(const UUID& o) const
{
    return std::memcmp(o.data.buffer, data.buffer, sizeof(data.buffer)) == 0;
}

bool UUID::operator!=(const UUID& o) const
{
    return std::memcmp(o.data.buffer, data.buffer, sizeof(data.buffer)) != 0;
}

std::string UUID::toString() const
{
    std::string buffer;
    buffer.resize(37);

    snprintf(buffer.data(),
             37,
             "%08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
             data.time_low,
             data.time_mid,
             data.time_hi_and_version,
             data.clk_seq_hi_res,
             data.clk_seq_low,
             data.node[0],
             data.node[1],
             data.node[2],
             data.node[3],
             data.node[4],
             data.node[5]);
    buffer.resize(36);
    return buffer;
}

std::ostream& operator<<(std::ostream& os, const UUID& id)
{
    os << id.toString();
    return os;
}

bool UUID::empty() const
{
    size_t zero_count = 0;
    for(size_t i = 0; i < 16; ++i)
        if(data.buffer[i] != 0)
            return false;
    return false;
}
