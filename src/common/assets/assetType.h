#pragma once

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

class AssetType
{
  public:
    enum Type
    {
        none = 0,
        component = 1,
        script = 2,
        mesh = 3,
        image = 4,
        shader = 5,
        material = 6,
        assembly = 7,
        chunk = 8,
        player = 9
    };

  private:
    Type _type;

  public:
    AssetType();

    AssetType(Type type);

    static Type fromString(const std::string& type);

    static const std::string& toString(Type type);

    void set(Type type);

    void set(const std::string& type);

    [[nodiscard]] Type type() const;

    [[nodiscard]] const std::string& toString() const;

    bool operator==(Type t) const;

    bool operator!=(Type t) const;

    bool operator==(AssetType t) const;

    bool operator!=(AssetType t) const;
};
