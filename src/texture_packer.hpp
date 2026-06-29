#pragma once

#define MAX_TEXTURE 4096

#include <vector>

#include "utils.hpp"

struct Bitmap {
    istring name;
    int width;
    int height;
    int packX;
    int packY;
    Bitmap(istring name, int width, int height)
        : name(std::move(name))
        , width(width)
        , height(height) {}
};

struct Packer {
    int width;
    int height;

    std::vector<Bitmap> bitmaps;

    Packer(int max_size);
    void Pack(std::vector<Bitmap>& bitmaps);
};

bool pack_textures(std::vector<Bitmap>& textures, std::vector<Packer>& packed_textures);
