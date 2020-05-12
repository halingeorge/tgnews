#include "gtest/gtest.h"
#include "server/server.h"

using namespace tgnews;

TEST(ServerTest, Sample) {
  static constexpr uint32_t port = 300;
  Server server(port);
  EXPECT_EQ(server.Port(), port);
}