#include "server/server.h"

#include "gtest/gtest.h"

using namespace tgnews;

TEST(ServerTest, Sample)
{
  static constexpr uint32_t kPort = 12345;
  std::experimental::thread_pool pool(1);
  Server server(kPort, std::make_unique<FileManager>(pool), pool);
  EXPECT_EQ(server.Port(), kPort);
}
