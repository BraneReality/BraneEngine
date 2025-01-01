// Asset server
#include "assets/assetManager.h"
#include "assetServer.h"
#include "networking/networking.h"
#include "utility/assert.h"
#include <common/config/config.h>
#include <database/database.h>
#include <ecs/entity.h>
#include <fileManager/fileManager.h>
#include <runtime/runtime.h>

std::string replaceAll(const std::string& input, const std::string& from, const std::string& to)
{
    std::string result;
    result.reserve(input.length()); // Pre-allocate for performance

    size_t start = 0;
    size_t pos;
    while((pos = input.find(from, start)) != std::string::npos)
    {
        result.append(input, start, pos - start);
        result.append(to);
        start = pos + from.length();
    }
    result.append(input, start);
    return result;
}

int main()
{
    std::string assertContext = "yo! I'm a runtime value";
    if(!(2 + 2 == 5))
    {
        std::string message = std::format("a runtime string: {}\n and a runtime number: {}", assertContext, 42);
        std::string description = std::format(
            "condition '{}' failed\nline: {}\nof: {}\nwith message: '{}'", "2 + 2 == 5", __LINE__, __FILE__, message);
        description = replaceAll(description, "\"", "‛‛");
        description = replaceAll(description, "'", "‛");
        std::cout << "attemtping to print: " << description << std::endl;
        tinyfd_messageBox("ASSERTION FAILED", description.c_str(), "ok", "error", 1);
        throw std::runtime_error("asertion failed!");
    }
    // ASSERT(2 + 2 == 5, "a runtime string: {}\n and a runtime number: {}", assertContext, 42);
    Runtime::init();
    Config::loadConfig();

    Timeline& tl = Runtime::timeline();
    tl.addBlock("asset management");
    tl.addBlock("networking");
    tl.addBlock("before main");
    tl.addBlock("main");
    tl.addBlock("draw");
    Runtime::addModule<FileManager>();
    Runtime::addModule<NetworkManager>();
    Runtime::addModule<EntityManager>();
    Runtime::addModule<AssetManager>();
    Runtime::addModule<Database>();
    Runtime::addModule<AssetServer>();

    Runtime::setTickRate(30);
    Runtime::run();
    Runtime::cleanup();
    return 0;
}
