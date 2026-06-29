#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "utils.hpp"

// because for some reason, texbin decided to implement lz77 *slightly differently*
// from every other place konami games use it. smh.
// This differs from texbintools in that it JUST compresses, and doesn't add
// or parse the length headers
std::vector<uint8_t> texbin_lz77_compress(const std::vector<uint8_t>& data);
// max_len is a soft clamp, you may get a few extra bytes
std::vector<uint8_t> texbin_lz77_decompress(
    const std::vector<uint8_t>& comp_with_hdr, size_t max_len = 0, bool debug = true);

class ImageEntryParsed {
  public:
    std::vector<uint8_t> tex;

    ImageEntryParsed(std::vector<uint8_t> tex)
        : tex(tex) {}
    ImageEntryParsed()
        : tex(std::vector<uint8_t>()) {}

    // w/h
    std::pair<uint16_t, uint16_t> peek_dimensions();
    // data, w, h
    std::optional<std::tuple<std::vector<uint8_t>, uint16_t, uint16_t>> tex_to_argb8888();
    // used in debug output
    std::string tex_type_str(bool debug_lz77 = true);
};

class RectEntryParsed {
  public:
    istring parent_name;
    uint16_t x, y, w, h;
    // if the image has been replaced, we defer until save-time so we replace
    // all images in the same base image at once, for better performance
    std::optional<std::vector<uint8_t>> dirty_data = std::nullopt;

    inline uint16_t x2() { return x + w; };
    inline uint16_t y2() { return y + h; };
};

class Texbin {
  public:
    // real .bin files don't seem to care about the order of names, so neither
    // will we

    // name -> entry
    std::map<istring, ImageEntryParsed> images;
    // name -> entry. Don't need to maintain a list of source rects, as we don't
    // support packing a new texture into an existing rect (please let this
    // remain a never-needed usecase)
    std::map<istring, RectEntryParsed> rects;

    Texbin(std::map<istring, ImageEntryParsed> images, std::map<istring, RectEntryParsed> rects)
        : images(images)
        , rects(rects) {}

    Texbin() = default;

    static std::optional<Texbin> from_path(const char* path);
    static std::optional<Texbin> from_stream(std::istream& f);
    bool add_or_replace_image(const char* image_name, const char* png_path);
    bool save(const char* dest);
    void debug();

  private:
    void process_dirty_rects();
};
