//
// Created by eli on 7/5/2022.
//

#ifndef BRANEENGINE_STRCASECOMPARE_H
#define BRANEENGINE_STRCASECOMPARE_H

#include <string>

template<typename T>
bool strCaseCompare(const T& a, const T& b)
{
    for(size_t i = 0; i < a.size() && i < b.size(); i++)
    {
        char ac = std::tolower(a[i]);
        char bc = std::tolower(b[i]);
        if(ac != bc)
            return ac < bc;
    }
    return a.size() < b.size();
}

#endif // BRANEENGINE_STRCASECOMPARE_H
