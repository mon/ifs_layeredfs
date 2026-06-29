#include <fstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "config.hpp"
#include "hook.hpp"
#include "imagefs.hpp"
#include "avs_standalone.hpp"
#include "modpath_handler.hpp"
#include "ramfs_demangler.hpp"

#include "3rd_party/rapidxml_print.hpp"
#include "src/3rd_party/rapidxml.hpp"
#include "src/utils.hpp"
#include "arc.hpp"

using ::testing::Contains;
using ::testing::Optional;

#define FOREACH_EXTRA_FUNC(X)                                                                                                                              \
    X("XCgsqzn000004d", void, avs_fs_umount_by_desc, uint32_t desc) \


#define AVS_FUNC_PTR(obfus_name, ret_type, name, ...) ret_type (*name)(__VA_ARGS__);
FOREACH_EXTRA_FUNC(AVS_FUNC_PTR)

#define LOAD_FUNC(obfus_name, ret_type, name, ...)                 \
   ASSERT_TRUE((name = (decltype(name))GetProcAddress(avs, obfus_name))); \

class LogTestName : public testing::EmptyTestEventListener {
    void OnTestStart(const testing::TestInfo& info) override {
        log_misc("--------- Running {}.{}", info.test_suite_name(), info.name());
    }
};

class Environment : public ::testing::Environment {
    public:
     ~Environment() override {}

     // Override this to define how to set up the environment.
     void SetUp() override {
        testing::UnitTest::GetInstance()->listeners().Append(new LogTestName);

        char exe_path[MAX_PATH];
        ASSERT_TRUE(GetModuleFileNameA(NULL, exe_path, MAX_PATH));
        dll_time = file_time(exe_path);

        ASSERT_TRUE(avs_standalone::boot(false));

        auto avs = GetModuleHandleA("avs2-core.dll");
        ASSERT_TRUE(avs);
        FOREACH_EXTRA_FUNC(LOAD_FUNC);

        ASSERT_EQ(init(), 0);

        config.set_mod_folder("./testcases_data_mods");
        config.verbose_logs = true;
        print_config();
        cache_mods();

        ASSERT_THAT(available_mods(), Contains(config.get_mod_folder() / "empty"));
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
   EXPECT_THAT(find_first_modfile("OhNo/oWo"), Optional(config.get_mod_folder() / "Case_Sensitive/OhNO/oWo"));
   EXPECT_THAT(find_first_modfile("ohno/owo"), Optional(config.get_mod_folder() / "Case_Sensitive/OhNO/oWo"));
}

TEST_P(DevModeOnOff, CaseInsensitiveFolders) {
   EXPECT_THAT(find_first_modfolder("OhNO"), Optional(config.get_mod_folder() / "Case_Sensitive/OhNO/"));
   EXPECT_THAT(find_first_modfolder("ohno"), Optional(config.get_mod_folder() / "Case_Sensitive/OhNO/"));
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
      if(!norm) return istring();

      log_info("Lookup {} norm {}", path, *norm);
      TestHookFile file(std::move(path), std::move(*norm));
      auto lookup = lookup_png_from_md5(file);
      EXPECT_NE(lookup, std::nullopt);
      if(!lookup) return istring();
      auto &[png, tex] = *lookup;

      return png;
   };

   auto lookup_afp = [&](std::string folder, std::string fname) {
      auto hash = MD5()(fname);
      auto path = mount + "/" + folder + "/" + hash;
      auto norm = normalise_path(path);
      EXPECT_NE(norm, std::nullopt);
      if(!norm) return istring();

      log_info("Lookup {} norm {}", path, *norm);
      TestHookFile file(std::move(path), std::move(*norm));
      auto lookup = lookup_afp_from_md5(file);
      EXPECT_NE(lookup, std::nullopt);
      if(!lookup) return istring();

      return *lookup;
   };

   EXPECT_EQ(lookup_tex("tex", "inner"),               config.get_mod_folder() + "/md5_lookup/test_ifs/tex/inner.png");
   EXPECT_EQ(lookup_tex("tex", "outer"),               config.get_mod_folder() + "/md5_lookup/test_ifs/outer.png");
   EXPECT_EQ(lookup_afp("afp", "confirm_all"),         config.get_mod_folder() + "/md5_lookup/test_ifs/afp/confirm_all");
   EXPECT_EQ(lookup_afp("afp/bsi", "confirm_all"),     config.get_mod_folder() + "/md5_lookup/test_ifs/afp/bsi/confirm_all");
   EXPECT_EQ(lookup_afp("geo", "confirm_all_shape5"),  config.get_mod_folder() + "/md5_lookup/test_ifs/geo/confirm_all_shape5");
   EXPECT_EQ(lookup_afp("geo", "confirm_all_shape11"), config.get_mod_folder() + "/md5_lookup/test_ifs/geo/confirm_all_shape11");

   avs_fs_umount_by_desc(desc);
}

TEST(Xml, MergingWorks) {
   modpath_debug_add_folder("data2"); // also test data2 handling for good measure
   auto f = hook_avs_fs_open("data2/subfolder/base.xml", avs_open_mode_read(), 420);
   ASSERT_GT(f, 0);

   rapidxml::xml_document<> xml_doc;
   bool res = rapidxml_from_avs_file(f, xml_doc, xml_doc);
   ASSERT_TRUE(res);

   auto node = xml_doc.first_node();
   ASSERT_NE(node, nullptr);
   ASSERT_EQ(node->type(), rapidxml::node_type::node_declaration);
   node = node->next_sibling();
   ASSERT_NE(node, nullptr);
   EXPECT_STREQ(node->name(), "afplist");


   node = node->first_node();
   ASSERT_NE(node, nullptr);
   EXPECT_STREQ(node->name(), "afp");

   auto attr = node->first_attribute("name");
   ASSERT_NE(attr, nullptr);
   EXPECT_STREQ(attr->value(), "hare");

   auto geo = node->first_node("geo");
   ASSERT_NE(geo, nullptr);
   EXPECT_STREQ(geo->value(), "5 10 15");


   node = node->next_sibling();
   ASSERT_NE(node, nullptr);
   EXPECT_STREQ(node->name(), "afp");

   attr = node->first_attribute("name");
   ASSERT_NE(attr, nullptr);
   EXPECT_STREQ(attr->value(), "hare2");

   geo = node->first_node("geo");
   ASSERT_NE(geo, nullptr);
   EXPECT_STREQ(geo->value(), "20 25 30");


   node = node->next_sibling();
   ASSERT_EQ(node, nullptr);
}

TEST(LongPath, AvsFsOpenOver128Chars) {
   std::string subpath = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.bin";

   std::string avs_path = "/data/" + subpath;
   ASSERT_GT(avs_path.size(), 128u);
   auto f = hook_avs_fs_open(avs_path.c_str(), avs_open_mode_read(), 420);
   EXPECT_GT(f, 0);
   if(f > 0)
      avs_fs_close(f);
}

TEST(RamFs, DemanglingWorks) {
    AVS_FILE fake_handle = 9999;
    uint8_t fake_buffer[1];

    ramfs_demangler_on_fs_open("./data/test.ifs", fake_handle);

    ramfs_demangler_on_fs_read(fake_handle, fake_buffer);
    std::string flags = std::format("base={:#x}", (uintptr_t)fake_buffer);
    ramfs_demangler_on_fs_mount("/sd0", "test.ifs", "ramfs", flags);

    ramfs_demangler_on_fs_mount("/game/test", "/sd0/test.ifs", "imagefs", std::nullopt);

    std::string path = "/game/test/somefile";
    ramfs_demangler_demangle_if_possible(path);
    EXPECT_EQ(path, "./data/test.ifs/somefile");

    // re-open triggers cleanup, not explicit unmount
    ramfs_demangler_on_fs_open("./data/test.ifs", fake_handle);

    path = "/game/test/somefile";
    ramfs_demangler_demangle_if_possible(path);
    EXPECT_EQ(path, "/game/test/somefile");
}

TEST(RamFs, DemanglingWorksNabla) {
   // Nabla has an extra indirection:
   // M:opening /data/graphics/ver07/logo.ifs mode 1 flags 420
   // M:mounting image.bin to /mnt/bm2d/rmp_89_2714881 with type ramfs and args base=0x00000000d2435060,size=0x8984f0,mode=ro
   // M:ramfs mount mapped to /data/graphics/ver07/logo.ifs
   // M:mounting /mnt/bm2d/rmp_89_2714881/image.bin to /mnt/bm2d/rfs88r89/logo.ifs with type link and args
   // M:mounting /mnt/bm2d/rfs88r89/logo.ifs to /mnt/bm2d/ngp88/logo.ifs with type imagefs and args (null)
   // M:imagefs mount mapped to /mnt/bm2d/rfs88r89/logo.ifs
   // M:opening /mnt/bm2d/rfs88r89/logo.ifs mode 1 flags 420

   AVS_FILE fake_handle = 9999;
   uint8_t fake_buffer[1];

   ramfs_demangler_on_fs_open("/data/graphics/ver07/logo.ifs", fake_handle);

   ramfs_demangler_on_fs_read(fake_handle, fake_buffer);
   std::string flags = std::format("base={:#x}", (uintptr_t)fake_buffer);
   ramfs_demangler_on_fs_mount("/mnt/bm2d/rmp_89_2714881", "image.bin", "ramfs", flags);

   ramfs_demangler_on_fs_mount("/mnt/bm2d/rfs88r89/logo.ifs", "/mnt/bm2d/rmp_89_2714881/image.bin", "link", "");
   ramfs_demangler_on_fs_mount("/mnt/bm2d/ngp88/logo.ifs", "/mnt/bm2d/rfs88r89/logo.ifs", "imagefs", std::nullopt);

   ramfs_demangler_on_fs_open("/mnt/bm2d/rfs88r89/logo.ifs", 9001);

   std::string path = "/mnt/bm2d/ngp88/logo.ifs/tex/texturelist.xml";
   ramfs_demangler_demangle_if_possible(path);
   EXPECT_EQ(path, "/data/graphics/ver07/logo.ifs/tex/texturelist.xml");
}

static std::vector<uint8_t> read_arc_file(istring const& arc_path, istring const& name) {
    std::ifstream f(arc_path.c_str(), std::ios::binary);
    if (!f) return {};
    auto arc = ArcArchive::from_stream(f);
    if (!arc) return {};
    auto it = arc->files.find(name);
    if (it == arc->files.end()) return {};
    return it->second;
}

TEST(ArcArchive, ScratchFromTwoMods) {
    AvsOpenHookFile file("scratch.arc", "scratch.arc", avs_open_mode_read(), 420);
    handle_arc(file);
    ASSERT_TRUE(file.mod_path.has_value());

    std::vector<uint8_t> expected_a = {'h','e','l','l','o','_','a'};
    std::vector<uint8_t> expected_b = {'h','e','l','l','o','_','b'};
    EXPECT_EQ(read_arc_file(*file.mod_path, "file_a"), expected_a);
    EXPECT_EQ(read_arc_file(*file.mod_path, "file_b"), expected_b);
}

TEST(ArcArchive, MergedXmlInsideArc) {
    AvsOpenHookFile file("xml_merge.arc", "xml_merge.arc", avs_open_mode_read(), 42);
    // note: only two tests where we have an original .arc so we need a tiny bit of
    // copied extra work from handle_file
    auto norm_copy = file.norm_path;
    file.mod_path = find_first_modfile(norm_copy);
    handle_arc(file);
    ASSERT_TRUE(file.mod_path.has_value());

    auto base_xml = read_arc_file(*file.mod_path, "merged/base.xml");
    ASSERT_FALSE(base_xml.empty());
    EXPECT_TRUE(read_arc_file(*file.mod_path, "merged/base.merged.xml").empty());

    base_xml.push_back('\0');
    rapidxml::xml_document<> xml_doc;
    xml_doc.parse<rapidxml::parse_no_utf8>((char*)base_xml.data());

    auto node = xml_doc.first_node();
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(node->name(), "afplist");

    node = node->first_node();
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(node->name(), "afp");
    auto attr = node->first_attribute("name");
    ASSERT_NE(attr, nullptr);
    EXPECT_STREQ(attr->value(), "hare");

    node = node->next_sibling();
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(node->name(), "afp");
    attr = node->first_attribute("name");
    ASSERT_NE(attr, nullptr);
    EXPECT_STREQ(attr->value(), "hare2");

    EXPECT_EQ(node->next_sibling(), nullptr);
}

TEST(ArcArchive, MergedXmlInsideArcWithOriginalInsideArc) {
    AvsOpenHookFile file("xml_merge.arc", "xml_merge.arc", avs_open_mode_read(), 42);
    // note: only two tests where we have an original .arc so we need a tiny bit of
    // copied extra work from handle_file
    auto norm_copy = file.norm_path;
    file.mod_path = find_first_modfile(norm_copy);
    handle_arc(file);
    ASSERT_TRUE(file.mod_path.has_value());

    auto base_xml = read_arc_file(*file.mod_path, "merged_only/base.xml");
    ASSERT_FALSE(base_xml.empty());
    EXPECT_TRUE(read_arc_file(*file.mod_path, "merged_only/base.merged.xml").empty());

    base_xml.push_back('\0');
    rapidxml::xml_document<> xml_doc;
    xml_doc.parse<rapidxml::parse_no_utf8>((char*)base_xml.data());

    auto node = xml_doc.first_node();
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(node->name(), "afplist");

    node = node->first_node();
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(node->name(), "afp");
    auto attr = node->first_attribute("name");
    ASSERT_NE(attr, nullptr);
    EXPECT_STREQ(attr->value(), "hare");

    node = node->next_sibling();
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(node->name(), "afp");
    attr = node->first_attribute("name");
    ASSERT_NE(attr, nullptr);
    EXPECT_STREQ(attr->value(), "hare2");

    EXPECT_EQ(node->next_sibling(), nullptr);
}

// Drives the demangler mount chain end-to-end after handle_arc has registered
// the inner-ifs basename: simulates the ramfs/imagefs mount sequence and
// asserts that a path inside the inner ifs's imagefs mountpoint demangles back
// to "<arc_path with .arc->_arc>/inner.ifs/<file>".
static void exercise_inner_ifs_demangle(NormPath const& arc_path) {
    // The buffer pointer doesn't matter — basename lookup is the fallback path
    uint8_t fake_buffer[1];
    std::string flags = std::format("base={:#x}", (uintptr_t)fake_buffer);
    ramfs_demangler_on_fs_mount("/sd9", "inner.ifs", "ramfs", flags);
    ramfs_demangler_on_fs_mount("/game/inner_test", "/sd9/inner.ifs", "imagefs", std::nullopt);

    std::string p = "/game/inner_test/some_subfile";
    ramfs_demangler_demangle_if_possible(p);
    NormPath expected_arc = arc_path;
    expected_arc.replace_suffix_foreach_path_component(".arc", "_arc");
    EXPECT_EQ(p, fmt::format("data/{}/inner.ifs/some_subfile", expected_arc));
}

TEST(ArcArchive, IfsOnlySubtreeSkipsRepack) {
    // Mod folder has only an _ifs subtree (no per-entry overrides). No repack
    // should happen — the original arc is left alone — but the inner-ifs
    // basename still gets registered with the demangler.
    AvsOpenHookFile file("ifs_only_repack.arc", "ifs_only_repack.arc", avs_open_mode_read(), 42);
    handle_arc(file);
    EXPECT_FALSE(file.mod_path.has_value());

    exercise_inner_ifs_demangle(file.norm_path);
}

TEST(ArcArchive, IfsWithExtraOverrides) {
    ArcArchive orig;
    std::vector<uint8_t> inner_ifs_bytes(64, 'I');
    std::vector<uint8_t> other_bin_bytes = {'o','r','i','g','_','o','t','h','e','r'};
    orig.add_or_replace("inner.ifs", inner_ifs_bytes);
    orig.add_or_replace("other.bin", other_bin_bytes);

    auto tmp_path = config.get_mod_folder() / "_cache/ifs_with_extras_orig.arc";
    mkdir_p(config.get_mod_folder() / "_cache");
    ASSERT_TRUE(orig.save(tmp_path.c_str()));

    AvsOpenHookFile file(std::move(tmp_path), "ifs_with_extras.arc", avs_open_mode_read(), 42);
    handle_arc(file);
    ASSERT_TRUE(file.mod_path.has_value());

    // inner.ifs unchanged, other.bin replaced from the mod
    EXPECT_EQ(read_arc_file(*file.mod_path, "inner.ifs"), inner_ifs_bytes);
    std::vector<uint8_t> expected_override = {'o','v','e','r','r','i','d','d','e','n','_','o','t','h','e','r','_','d','a','t','a'};
    EXPECT_EQ(read_arc_file(*file.mod_path, "other.bin"), expected_override);

    exercise_inner_ifs_demangle(file.norm_path);
}

TEST(ArcArchive, OverlayWithTwoMods) {
    // Build original arc in memory: has "original_file" and "override_file"
    ArcArchive orig;
    orig.add_or_replace("original_file", {'o','r','i','g','i','n','a','l'});
    orig.add_or_replace("override_file", {'o','l','d'});

    auto tmp_path = config.get_mod_folder() / "_cache/overlay_test_orig.arc";
    mkdir_p(config.get_mod_folder() / "_cache");
    ASSERT_TRUE(orig.save(tmp_path.c_str()));

    AvsOpenHookFile file(std::move(tmp_path), "overlay_test.arc", avs_open_mode_read(), 42);
    handle_arc(file);
    ASSERT_TRUE(file.mod_path.has_value());

    std::vector<uint8_t> expected_orig    = {'o','r','i','g','i','n','a','l'};
    std::vector<uint8_t> expected_replace = {'r','e','p','l','a','c','e','d'};
    std::vector<uint8_t> expected_new     = {'n','e','w'};
    EXPECT_EQ(read_arc_file(*file.mod_path, "original_file"), expected_orig);
    EXPECT_EQ(read_arc_file(*file.mod_path, "override_file"), expected_replace);
    EXPECT_EQ(read_arc_file(*file.mod_path, "new_file"), expected_new);
}

TEST(Regression, BeatStreamAfpXml) {
   // M:mounting /data2/graphic/psd_result.ifs to /afp22/data2/graphic/psd_result.ifs with type imagefs and args (null)
   int mnt = hook_avs_fs_mount("/afp22/data2/graphic/psd_result.ifs", "/data2/graphic/psd_result.ifs", "imagefs", nullptr);
   ASSERT_GT(mnt, 0);

   auto f = hook_avs_fs_open("/afp22/data2/graphic/psd_result.ifs/afp/afplist.xml", avs_open_mode_read(), 420);
   ASSERT_GT(f, 0);
   rapidxml::xml_document<> xml_doc;
   bool res = rapidxml_from_avs_file(f, xml_doc, xml_doc);
   ASSERT_TRUE(res);

   auto node = xml_doc.first_node();
   ASSERT_NE(node, nullptr);
   ASSERT_EQ(node->type(), rapidxml::node_type::node_declaration);
   node = node->next_sibling();
   ASSERT_NE(node, nullptr);
   EXPECT_STREQ(node->name(), "afplist");

   node = node->last_node();
   ASSERT_NE(node, nullptr);
   EXPECT_STREQ(node->name(), "afp");

   auto attr = node->first_attribute("name");
   ASSERT_NE(attr, nullptr);
   EXPECT_STREQ(attr->value(), "hare");

   auto geo = node->first_node("geo");
   ASSERT_NE(geo, nullptr);
   EXPECT_STREQ(geo->value(), "5 10 15");
}
