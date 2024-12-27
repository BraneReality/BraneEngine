// Asset server
#include "assets/assetManager.h"
#include "assetServer.h"
#include "networking/networking.h"
#include <common/config/config.h>
#include <database/database.h>
#include <ecs/entity.h>
#include <fileManager/fileManager.h>
#include <runtime/runtime.h>

int main()
{
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