#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <json/json.h>

class Config {
    const static char* configFileName;
    static Json::Value root;

  public:
    static void loadConfig();

    static void save();

    static Json::Value& json();
};
