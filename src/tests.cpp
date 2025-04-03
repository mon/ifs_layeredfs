#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "config.hpp"
#include "hook.h"
#include "avs_standalone.hpp"
#include "modpath_handler.h"

using ::testing::Contains;
using ::testing::Optional;

class Environment : public ::testing::Environment {
    public:
     ~Environment() override {}

     // Override this to define how to set up the environment.
     void SetUp() override {
        ASSERT_TRUE(avs_standalone::boot(false));
        ASSERT_EQ(init(), 0);

        config.mod_folder = "./testcases_data_mods";
        print_config();
        cache_mods();

        ASSERT_THAT(available_mods(), Contains(config.mod_folder + "/empty"));
     }

     // Override this to define how to tear down the environment.
     void TearDown() override {
        avs_standalone::shutdown();
     }
};

testing::Environment* const foo_env =
    testing::AddGlobalTestEnvironment(new Environment);

class DevModeOnOff : public testing::TestWithParam<bool> {
   void SetUp() override {
      config.developer_mode = GetParam();
   }
   void TearDown() override {
      config.developer_mode = false;
   }
};
INSTANTIATE_TEST_SUITE_P(DevModeOnOffInstance, DevModeOnOff, testing::Bool());


TEST_P(DevModeOnOff, MissingFileNullopt) {
   ASSERT_EQ(find_first_modfile("doesn't exist"), std::nullopt);
}

TEST_P(DevModeOnOff, CaseInsensitiveFiles) {
   EXPECT_THAT(find_first_modfile("OhNo/oWo"), Optional(config.mod_folder + "/Case_Sensitive/OhNO/oWo"));
   EXPECT_THAT(find_first_modfile("ohno/owo"), Optional(config.mod_folder + "/Case_Sensitive/OhNO/oWo"));
}

TEST_P(DevModeOnOff, CaseInsensitiveFolders) {
   EXPECT_THAT(find_first_modfolder("OhNO"), Optional(config.mod_folder + "/Case_Sensitive/OhNO/"));
   EXPECT_THAT(find_first_modfolder("ohno"), Optional(config.mod_folder + "/Case_Sensitive/OhNO/"));
}
