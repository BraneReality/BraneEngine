
#ifndef H_BRANE_ASSERT
#define H_BRANE_ASSERT

#include <format>
#include "tinyfiledialogs/tinyfiledialogs.h"


// value to assert, fmt string, fmt args
#define BRANE_ASSERT_AT(line, file, condition, fmt, ...)                                                               \
    if(!(condition))                                                                                                   \
    {                                                                                                                  \
        std::string message = std::format(fmt __VA_OPT__(, ) __VA_ARGS__);                                             \
        std::string description = std::format(                                                                         \
            "condition \"{}\" failed\nline: {}\nof: {}\nwith message: \"{}\"", #condition, line, file, message);       \
        for(size_t i = 0; i < description.size(); ++i)                                                                 \
        {                                                                                                              \
            if(description[i] == '\"' || description[i] == '\'' || description[i] == '`')                              \
                description[i] = '|';                                                                                  \
        }                                                                                                              \
        tinyfd_messageBox("ASSERTION FAILED", description.c_str(), "ok", "error", 1);                                  \
        throw std::runtime_error("asertion failed!");                                                                  \
    }

#ifndef NDEBUG
#define BRANE_ASSERT(condition, fmt, ...) BRANE_ASSERT_AT(__LINE__, __FILE__, condition, fmt __VA_OPT__(, ) __VA_ARGS__)
#else
#define ASSERT(condition, fmt, ...) ((void)0)
#endif

#endif
