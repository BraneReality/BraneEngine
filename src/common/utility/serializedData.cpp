#include "serializedData.h"

std::string_view SerializerError::toString() const
{
    switch(type)
    {
        case NotEnoughData:
            return "Not Enough Data";
        case WrongFormat:
            return "Wrong Format";
    }

    return "unknown error";
}
