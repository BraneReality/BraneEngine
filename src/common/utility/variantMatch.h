#ifndef BRANEENGINE_VARIANTMATCH_H
#define BRANEENGINE_VARIANTMATCH_H

#include <variant>

template<class... Ts>
struct overloads : Ts...
{
    using Ts::operator()...;
};

#define MATCHV(variant, ...) std::visit(overloads{__VA_ARGS__}, variant)


#endif // BRANEENGINE_VARIANTMATCH_H
