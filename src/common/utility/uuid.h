
#ifndef H_BRANE_UUID4
#define H_BRANE_UUID4

#include <cstdint>
#include <string>
#include "result.h"
#include "serializedData.h"
#include <string_view>

// Referenced from https://gist.github.com/kvelakur/9069c9896577c3040030

struct UUID
{
    union
    {
        struct
        {
            uint32_t time_low;
            uint16_t time_mid;
            uint16_t time_hi_and_version;
            uint8_t clk_seq_hi_res;
            uint8_t clk_seq_low;
            uint8_t node[6];
        };

        uint8_t buffer[16];
    } data;

    UUID();
    /// MUST be a 16 element array
    UUID(uint8_t* raw);
    static Result<UUID> generate();
    static Result<UUID> fromString(std::string_view text);

    bool operator==(const UUID&) const;
    bool operator!=(const UUID&) const;

    // UUID& operator=(const UUID&);

    bool empty() const;

    std::string toString() const;

    friend std::ostream& operator<<(std::ostream& os, const UUID& id);
};

template<>
struct std::hash<UUID>
{
    std::size_t operator()(const UUID& id) const
    {
        return std::hash<std::string_view>()(std::string_view((char*)id.data.buffer, 38));
    }
};

template<>
struct Serializer<UUID>
{
    static Result<void, SerializerError> read(InputSerializer& s, UUID& value)
    {
        return s.read(value.data.buffer, 16);
    }

    static Result<void, SerializerError> write(OutputSerializer& s, const UUID& value)
    {
        return s.write(value.data.buffer, 16);
    }
};

#endif // H_BRANE_UUID4
