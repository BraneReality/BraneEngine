//
// Created by eli on 1/31/2022.
//

#include "testing.h"
#include <networking/networking.h>
#include <utility/serializedData.h>

using namespace net;

TEST(networking, SerializedDataTest)
{
    SerializedData data;
    OutputSerializer oData(data);

    std::vector<uint32_t> first = {1, 2, 4, 8, 16};
    std::string second = "test string\n";
    uint64_t third = 42;

    oData << first << second << third;
    first.resize(0);
    second = "";
    third = 0;

    InputSerializer iData(data);

    iData >> first >> second >> third;

    EXPECT_EQ(first.

              size(),

              5);
    EXPECT_EQ(first[0], 1);
    EXPECT_EQ(first[1], 2);
    EXPECT_EQ(first[2], 4);
    EXPECT_EQ(first[3], 8);
    EXPECT_EQ(first[4], 16);
    EXPECT_TRUE(second == "test string\n");
    EXPECT_EQ(third, 42);

    EXPECT_THROW(iData >> first, std::runtime_error);
    EXPECT_THROW(iData >> second, std::runtime_error);
    EXPECT_THROW(iData >> third, std::runtime_error);
}