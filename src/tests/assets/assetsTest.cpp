#include "assets/assetManager.h"
#include "testing.h"
#include <assets/asset.h>

// Edit this function if we need to "load" any assets for testing
AsyncData<Asset*> AssetManager::fetchAssetInternal(const AssetID& id, bool incremental)
{
    AsyncData<Asset*> asset;
    asset.setData(nullptr);
    return asset;
}

TEST(assets, AssetIDTest)
{
    AssetID aa("server.ip.goes.here/1234A");
    EXPECT_EQ(aa.

              address(),

              "server.ip.goes.here");
    EXPECT_EQ(aa.

              id(),

              0x1234A);
    EXPECT_EQ(aa.

              string(),

              "server.ip.goes.here/1234A");
    EXPECT_TRUE(aa == aa);
    EXPECT_FALSE(aa.

                 null()

    );
    aa.

        setNull();

    EXPECT_TRUE(aa.

                null()

    );

    AssetID null;
    EXPECT_TRUE(null.

                null()

    );
    EXPECT_EQ(aa, null);
}
