#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>

#include "utils.hpp"

using namespace std;

// #define TEXBIN_VERBOSE

// because for some reason, texbin decided to implement lz77 *slightly differently*
// from every other place konami games use it. smh.
// This differs from texbintools in that it JUST compresses, and doesn't add
// or parse the length headers
vector<uint8_t> texbin_lz77_compress(const vector<uint8_t> &data);
// max_len is a soft clamp, you may get a few extra bytes
vector<uint8_t> texbin_lz77_decompress(const vector<uint8_t> &comp_with_hdr, size_t max_len = 0);

class ImageEntryParsed {
    public:
    vector<uint8_t> tex;

    ImageEntryParsed(vector<uint8_t> tex)
        : tex(tex)
    {}
    ImageEntryParsed()
        : tex(vector<uint8_t>())
    {}

    // w/h
    pair<uint16_t, uint16_t> peek_dimensions();
    // data, w, h
    optional<tuple<vector<uint8_t>, uint16_t, uint16_t>> tex_to_argb8888();
    // used to determine what image formats to write impls for, by opening
    // every .bin file in my Bemani folder
    string tex_type_str();
};

class RectEntryParsed {
    public:
    string parent_name;
    uint16_t x, y, w, h;
    // if the image has been replaced, we defer until save-time so we replace
    // all images in the same base image at once, for better performance
    optional<vector<uint8_t>> dirty_data = nullopt;

    inline uint16_t x2() {return x + w;};
    inline uint16_t y2() {return y + h;};
};

class Texbin {
    public:

    // real .bin files don't seem to care about the order of names, so neither
    // will we

    // name -> entry
    map<string, ImageEntryParsed, CaseInsensitiveCompare> images;
    // name -> entry. Don't need to maintain a list of source rects, as we don't
    // support packing a new texture into an existing rect (please let this
    // remain a never-needed usecase)
    map<string, RectEntryParsed, CaseInsensitiveCompare> rects;

    Texbin(
        map<string, ImageEntryParsed, CaseInsensitiveCompare> images,
        map<string, RectEntryParsed, CaseInsensitiveCompare> rects
    )
        : images(images)
        , rects(rects)
    {}

    Texbin() = default;

    static optional<Texbin> from_path(const char *path);
    static optional<Texbin> from_stream(istream &f);
    bool add_or_replace_image(const char *image_name, const char *png_path);
    bool save(const char *dest);
    void debug();

    private:
    void process_dirty_rects();
};
