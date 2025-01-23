#include "jsonSerializer.h"

std::string JsonSerializerError::toString() const
{
    switch(type)
    {
        case MissingKey:
            return std::format("Missing Key: {}", message);
        case WrongType:
            return std::format("Wrong Type: {}", message);
        case WrongStringFormat:
            return std::format("Wrong String Format: {}", message);
        case ParserError:
            return std::format("Parser Error: {}", message);
        default:
            return "Unknown error type";
    }
}
