#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

using namespace std;

//#define TEXBIN_VERBOSE

// because for some reason, texbin decided to implement lz77 *slightly differently*
// from every other place konami games use it. smh.
// This differs from texbintools in that it JUST compresses, and doesn't add
// or parse the length headers
vector<uint8_t> texbin_lz77_compress(const vector<uint8_t> &data);
vector<uint8_t> texbin_lz77_decompress(const vector<uint8_t> &comp_with_hdr);

class RectEntryParsed {
    public:
    string parent_name;
    uint16_t x, y, w, h;
};

class Texbin {
    public:

    string name;

    // real .bin files don't seem to care about the order of names, so neither
    // will we

    // name -> data
    unordered_map<string, vector<uint8_t>> images;
    // name -> entry. Don't need to maintain a list of source rects, as we don't
    // support packing a new texture into an existing rect (please let this
    // remain a never-needed usecase)
    unordered_map<string, RectEntryParsed> rects;

    Texbin(
        string name,
        unordered_map<string, vector<uint8_t>> images,
        unordered_map<string, RectEntryParsed> rects
    )
        : name(name)
        , images(images)
        , rects(rects)
    {}

    static optional<Texbin> from_path(const char *path);
    bool add_or_replace_image(const char *image_name, const char *png_path);
    bool save(const char *dest);
    void debug();
};
