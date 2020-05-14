#include "server/server.h"

#include "gtest/gtest.h"

using namespace tgnews;

TEST(ServerTest, Sample)
{
  static constexpr uint32_t kPort = 300;
  Server server(kPort);
  EXPECT_EQ(server.Port(), kPort);
}