#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "config.hpp"
#include "hook.h"
#include "imagefs.hpp"
#include "avs_standalone.hpp"
#include "modpath_handler.h"

using ::testing::Contains;
using ::testing::Optional;

#define FOREACH_EXTRA_FUNC(X)                                                                                                                              \
    X("XCgsqzn000004d", void, avs_fs_umount_by_desc, uint32_t desc) \


#define AVS_FUNC_PTR(obfus_name, ret_type, name, ...) ret_type (*name)(__VA_ARGS__);
FOREACH_EXTRA_FUNC(AVS_FUNC_PTR)

#define LOAD_FUNC(obfus_name, ret_type, name, ...)                 \
   ASSERT_TRUE((name = (decltype(name))GetProcAddress(avs, obfus_name))); \

class Environment : public ::testing::Environment {
    public:
     ~Environment() override {}

     // Override this to define how to set up the environment.
     void SetUp() override {
        ASSERT_TRUE(avs_standalone::boot(false));

        auto avs = GetModuleHandleA("avs2-core.dll");
        ASSERT_TRUE(avs);
        FOREACH_EXTRA_FUNC(LOAD_FUNC);

        ASSERT_EQ(init(), 0);

        config.mod_folder = "./testcases_data_mods";
        config.verbose_logs = true;
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

class TestHookFile final : public HookFile {
   using HookFile::HookFile;

   public:

   bool ramfs_demangle() override {return true;};

   uint32_t call_real() override {
       throw "Unimplemented";
   }

   std::optional<std::vector<uint8_t>> load_to_vec() override {
      throw "Unimplemented";
   }
};

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

TEST(ImageFs, MD5DemanglingWorks) {
   std::string mount = "/afp/data/mount/test.ifs";
   auto desc = hook_avs_fs_mount(mount.c_str(), "./data/test.ifs", "imagefs", NULL);
   ASSERT_GT(desc, 0);

   // load all the xml files to load md5 mappings
   auto check_load = [](std::string path) {
      auto f = hook_avs_fs_open(path.c_str(), avs_open_mode_read(), 420);
      EXPECT_GT(f, 0);
      if(f > 0)
         avs_fs_close(f);
   };
   check_load(mount + "/tex/texturelist.xml");
   check_load(mount + "/afp/afplist.xml");

   auto lookup_tex = [&](std::string folder, std::string fname) {
      MD5 md5;
      auto hash = md5(fname);
      auto path = mount + "/" + folder + "/" + hash;
      auto norm = normalise_path(path);
      EXPECT_NE(norm, std::nullopt);
      if(!norm) return std::string();

      TestHookFile file(path, *norm);
      log_info("Lookup %s norm %s", path.c_str(), norm->c_str());
      auto lookup = lookup_png_from_md5(file);
      EXPECT_NE(lookup, std::nullopt);
      if(!lookup) return std::string();
      auto &[png, tex] = *lookup;

      return png;
   };

   auto lookup_afp = [&](std::string folder, std::string fname) {
      auto hash = MD5()(fname);
      auto path = mount + "/" + folder + "/" + hash;
      auto norm = normalise_path(path);
      EXPECT_NE(norm, std::nullopt);
      if(!norm) return std::string();

      TestHookFile file(path, *norm);
      log_info("Lookup %s norm %s", path.c_str(), norm->c_str());
      auto lookup = lookup_afp_from_md5(file);
      EXPECT_NE(lookup, std::nullopt);
      if(!lookup) return std::string();

      return *lookup;
   };

   EXPECT_EQ(lookup_tex("tex", "inner"),               config.mod_folder + "/md5_lookup/test_ifs/tex/inner.png");
   EXPECT_EQ(lookup_tex("tex", "outer"),               config.mod_folder + "/md5_lookup/test_ifs/outer.png");
   EXPECT_EQ(lookup_afp("afp", "confirm_all"),         config.mod_folder + "/md5_lookup/test_ifs/afp/confirm_all");
   EXPECT_EQ(lookup_afp("afp/bsi", "confirm_all"),     config.mod_folder + "/md5_lookup/test_ifs/afp/bsi/confirm_all");
   EXPECT_EQ(lookup_afp("geo", "confirm_all_shape5"),  config.mod_folder + "/md5_lookup/test_ifs/geo/confirm_all_shape5");
   EXPECT_EQ(lookup_afp("geo", "confirm_all_shape11"), config.mod_folder + "/md5_lookup/test_ifs/geo/confirm_all_shape11");

   avs_fs_umount_by_desc(desc);
}
