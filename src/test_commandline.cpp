#include <sstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "config.hpp"
#include "avs_standalone.hpp"

using ::testing::ElementsAre;

TEST(Cmdline, ConfigParsingWorks) {
   load_config();

   EXPECT_EQ(config.verbose_logs, 1);
   EXPECT_EQ(config.developer_mode, 1);
   EXPECT_EQ(config.disable, 1);
   EXPECT_STREQ(config.logfile, "some logfile.log");
   EXPECT_THAT(config.allowlist, ElementsAre("allowed", "these folders"));
   EXPECT_THAT(config.blocklist, ElementsAre("blocked", "these folders"));
   EXPECT_EQ(config.mod_folder, "./some modfolder");
}
