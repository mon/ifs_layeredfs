#pragma once

#define MAX_TEXTURE 4096

#include <string>
#include <vector>

struct Bitmap
{
    std::string name;
    int width;
    int height;
    int packX;
    int packY;
    Bitmap(const std::string& name, int width, int height)
    : name(name), width(width), height(height)
    {}
};

struct Packer
{
    int width;
    int height;

    std::vector<Bitmap*> bitmaps;

    Packer(int max_size);
    void Pack(std::vector<Bitmap*> &bitmaps);
};

bool pack_textures(std::vector<Bitmap*> &textures, std::vector<Packer*> &packed_textures);
