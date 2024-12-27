//
// Created by eli on 8/27/2022.
//

#include "testing.h"
#include "utility/jsonVersioner.h"

TEST(VersionedJSON, Path)
{
    EXPECT_EQ(Json::getPathComponent("array/1/contained value", 0), "array");
    EXPECT_EQ(Json::getPathComponent("array/1/contained value", 1), "1");
    EXPECT_EQ(Json::getPathComponent("array/1/contained value", 2), "contained value");

    std::stringstream initJson(R"({"array":["value1",{"contained value":"hello world"}]})");
    Json::Value root;
    initJson >> root;

    EXPECT_STREQ(Json::resolvePath("array/1/contained value", root)
                     .

                 asCString(),

                 "hello world");
}

TEST(VersionedJSON, Change)
{
    JsonVersionTracker tracker;
    VersionedJson json(tracker);

    std::stringstream initJson(R"({"value":"hello"})");
    initJson >> json.

                data();

    EXPECT_STREQ(json["value"].

                 asCString(),

                 "hello");

    json.changeValue("value", "world");
    EXPECT_STREQ(json["value"].

                 asCString(),

                 "world");

    tracker.

        undo();

    EXPECT_STREQ(json["value"].

                 asCString(),

                 "hello");

    tracker.

        redo();

    EXPECT_STREQ(json["value"].

                 asCString(),

                 "world");

    tracker.

        undo();

    json.changeValue("value", "there"); // Test overwrite
    EXPECT_STREQ(json["value"].

                 asCString(),

                 "there");

    tracker.

        redo(); // Test boundaries
    tracker.

        redo();

    tracker.

        redo();

    EXPECT_STREQ(json["value"].

                 asCString(),

                 "there");

    tracker.

        undo();

    tracker.

        undo();

    tracker.

        undo();

    EXPECT_STREQ(json["value"].

                 asCString(),

                 "hello");
}