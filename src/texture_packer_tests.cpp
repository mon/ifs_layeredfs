#include <gtest/gtest.h>

#include <set>

#include "3rd_party/GuillotineBinPack.h"

#include "texture_packer.hpp"

TEST(TexturePacker, DoesNotRotateIntoSidewaysOnlySpace) {
    // 48x22 is the size of the small difficulty numbers (e.g. list_a80s).
    // After the first image, only a 22x64 strip is available.
    std::vector<Bitmap> images{{"small", 48, 22}, {"large", 42, 64}};
    Packer canvas(64);
    canvas.Pack(images);
    ASSERT_EQ(canvas.bitmaps.size(), 1u);
    ASSERT_EQ(images.size(), 1u);
    EXPECT_EQ(images.back().name, "small");
    EXPECT_EQ(images.back().width, 48);
    EXPECT_EQ(images.back().height, 22);
}

TEST(TexturePacker, DoesNotRotateIntoPerfectSidewaysFit) {
    // Leave exactly 22x48 free; this exercises the perfect-fit shortcut.
    std::vector<Bitmap> images{{"small", 48, 22}, {"large", 42, 48}, {"top", 64, 16}};
    Packer canvas(64);
    canvas.Pack(images);
    ASSERT_EQ(canvas.bitmaps.size(), 2u);
    ASSERT_EQ(images.size(), 1u);
    EXPECT_EQ(images.back().name, "small");
}

TEST(TexturePacker, UprightPackingAndCanvasShrink) {
    std::vector<Bitmap> images{{"bottom", 32, 16}, {"top", 32, 16}};
    Packer canvas(64);
    canvas.Pack(images);
    EXPECT_TRUE(images.empty());
    ASSERT_EQ(canvas.bitmaps.size(), 2u);
    EXPECT_EQ(canvas.width, 32);
    EXPECT_EQ(canvas.height, 32);
    EXPECT_EQ(canvas.bitmaps[0].packX, 0);
    EXPECT_EQ(canvas.bitmaps[1].packX, 0);
    EXPECT_EQ(canvas.bitmaps[0].packY, 0);
    EXPECT_EQ(canvas.bitmaps[1].packY, 16);
    for (const auto& image : canvas.bitmaps) {
        EXPECT_EQ(image.width, 32);
        EXPECT_EQ(image.height, 16);
    }
}

TEST(TexturePacker, DefaultBinPackingStillAllowsRotation) {
    for (int height : {48, 64}) {
        rbp::GuillotineBinPack packer(22, height);
        const auto rect = packer.Insert(48, 22, false, rbp::GuillotineBinPack::RectBestAreaFit,
            rbp::GuillotineBinPack::SplitLongerAxis);
        EXPECT_EQ(rect.width, 22);
        EXPECT_EQ(rect.height, 48);
    }
}

TEST(TexturePacker, MixedImagesStayInBoundsAndDoNotOverlap) {
    std::vector<Bitmap> images;
    // Deterministic varied dimensions, including tall and wide rectangles.
    for (int i = 0; i < 300; ++i)
        images.emplace_back(std::to_string(i), 2 + (i * 137) % 998, 2 + (i * 71) % 510);
    const auto originals = images;
    std::vector<Packer> canvases;
    ASSERT_TRUE(pack_textures(images, canvases));
    EXPECT_TRUE(images.empty());
    EXPECT_GT(canvases.size(), 1u);
    std::set<istring> seen;
    for (const auto& canvas : canvases) {
        EXPECT_LE(canvas.width, MAX_TEXTURE);
        EXPECT_LE(canvas.height, MAX_TEXTURE);
        for (size_t i = 0; i < canvas.bitmaps.size(); ++i) {
            const auto& a = canvas.bitmaps[i];
            ASSERT_TRUE(seen.insert(a.name).second);
            const auto& original = originals.at(std::stoi(a.name.downcast_ref()));
            EXPECT_EQ(a.width, original.width);
            EXPECT_EQ(a.height, original.height);
            EXPECT_GE(a.packX, 0);
            EXPECT_GE(a.packY, 0);
            EXPECT_LE(a.packX + a.width, canvas.width);
            EXPECT_LE(a.packY + a.height, canvas.height);
            for (size_t j = 0; j < i; ++j) {
                const auto& b = canvas.bitmaps[j];
                EXPECT_TRUE(a.packX + a.width <= b.packX || b.packX + b.width <= a.packX ||
                            a.packY + a.height <= b.packY || b.packY + b.height <= a.packY);
            }
        }
    }
    EXPECT_EQ(seen.size(), originals.size());
}

TEST(TexturePacker, EmptyAndMaximumSizeInputs) {
    std::vector<Bitmap> images;
    std::vector<Packer> canvases;
    EXPECT_TRUE(pack_textures(images, canvases));
    EXPECT_TRUE(canvases.empty());
    images.emplace_back("full", MAX_TEXTURE, MAX_TEXTURE);
    EXPECT_TRUE(pack_textures(images, canvases));
    EXPECT_TRUE(images.empty());
    ASSERT_EQ(canvases.size(), 1u);
    EXPECT_EQ(canvases[0].width, MAX_TEXTURE);
    EXPECT_EQ(canvases[0].height, MAX_TEXTURE);
}
