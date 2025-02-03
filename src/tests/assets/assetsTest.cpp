#include "assets/assetManager.h"
#include "testing.h"
#include <assets/asset.h>
#include <gtest/gtest.h>

class TestAssetLoader : public AssetLoader
{
    AsyncData<Shared<Asset>> loadAsset(const AssetID& id, bool incremental) override
    {
        AsyncData<Shared<Asset>> asset;
        asset.setError("Not implemented");
        return asset;
    }
};

TEST(assets, BraneAssetIDTest)

{
    auto parseRes = BraneAssetID::parse("Brane://server.ip.goes.here/1ede064a-f742-45c3-aa3b-9688579fc502");
    EXPECT_TRUE(parseRes);
    uint8_t uuid[16] = {0x1e, 0xde, 0x06, 0x4a, 0xf7, 0x42, 0x45, 0xc3, 0xaa, 0x3b, 0x96, 0x88, 0x57, 0x9f, 0xc5, 0x02};

    auto aa = parseRes.ok();

    EXPECT_EQ(aa.uuid, UUID(uuid));
    EXPECT_EQ(aa.domain, "server.ip.goes.here");


    AssetID null;
    EXPECT_TRUE(null.empty());
}
