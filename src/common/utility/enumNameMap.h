//
// Created by eli on 9/1/2022.
//

#ifndef BRANEENGINE_ENUMNAMEMAP_H
#define BRANEENGINE_ENUMNAMEMAP_H

#include <string>
#include <vector>
#include <unordered_map>

template<typename Enum>
class EnumNameMap
{
    std::unordered_map<Enum, std::string> _enumToString;
    std::unordered_map<std::string, Enum> _stringToEnum;

  public:
    EnumNameMap(std::vector<std::pair<Enum, std::string>>&& map)
    {
        for(auto& pair : map)
        {
            _stringToEnum.emplace(pair.second, pair.first);
            _enumToString.emplace(std::move(pair));
        }
    }

    const std::string& toString(Enum e) const
    {
        assert(_enumToString.count(e));
        return _enumToString.at(e);
    }

    Enum toEnum(const std::string& s) const
    {
        assert(_stringToEnum.count(s));
        return _stringToEnum.at(s);
    }
};

#endif // BRANEENGINE_ENUMNAMEMAP_H
