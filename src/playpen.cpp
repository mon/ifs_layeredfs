// The layeredfs playpen is for making sure shit works without having to boot
// the game a million times. BYO copy of AVS 2.17.x

#include <windows.h>

#include "hook.h"
#include "log.hpp"
#include "3rd_party/lodepng.h"

#include <fstream>

#include "texbin.hpp"
#include "avs_standalone.hpp"

#define log_assert(cond) if(!(cond)) {log_fatal("Assertion failed:" #cond);}

// decompressed_length MUST be set and will be updated on finish
unsigned char* lz_decompress(unsigned char* input, size_t length, size_t *decompressed_length) {
    auto compressor = cstream_create(AVS_DECOMPRESS_AVSLZ);
    if (!compressor) {
        log_warning("Couldn't create");
        return NULL;
    }
    compressor->input_buffer = input;
    compressor->input_size = (uint32_t)length; // may be -1 to auto-finish
    auto decompress_buffer = (unsigned char*)malloc(*decompressed_length);
    compressor->output_buffer = decompress_buffer;
    compressor->output_size = (uint32_t)*decompressed_length;

    cstream_operate(compressor);
    compressor->input_buffer = NULL;
    compressor->input_size = -1;
    bool ret = cstream_operate(compressor);
    if (!ret) {
        log_warning("Couldn't operate");
        return NULL;
    }
    if (cstream_finish(compressor)) {
        log_warning("Couldn't finish");
        return NULL;
    }
    *decompressed_length = *decompressed_length - compressor->output_size;
    cstream_destroy(compressor);
    return decompress_buffer;
}

void lz_unfuck(uint8_t *buf, size_t len) {
    int repl = 0;
    uint8_t *end = buf + len;
    while(buf < end) {
        uint8_t flag = *buf++;
        for(auto i = 0; i < 8; i++) {
            if(!(flag & 1) && (buf+1) < end) {
                uint8_t hi = buf[0];
                uint8_t lo = buf[1];

                *buf++ = (hi >> 4) | (lo & 0xF0);
                *buf++ = (hi << 4) | (lo & 0x0F);
                repl++;
            } else {
                buf++;
            }

            flag >>= 1;
        }
    }

    log_info("unfucked %d bytes", repl);
}

optional<std::vector<uint8_t>> readFile(const char* filename)
{
    // open the file:
    std::streampos fileSize;
    std::ifstream file(filename, std::ios::binary);

    if(!file) {
        log_warning("Couldn't open %s", filename);
        return nullopt;
    }

    // get its size:
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // read the data:
    std::vector<uint8_t> fileData((size_t)fileSize);
    file.read((char*) &fileData[0], fileSize);
    return fileData;
}

void avs_playpen() {
    // auto texo = Texbin::from_path("gfdm issues/tex_gf_game2_orig.bin");
    // auto texn = Texbin::from_path("gfdm issues/tex_gf_game2_repack.bin");

    auto texo = Texbin::from_path("gfdm issues/tex_gf_common_orig.bin");
    texo->add_or_replace_image("COM_BACKPIC00", "gfdm issues/COM_BACKPIC00.png");
    texo->save("gfdm issues/gf_common_mypack.bin");
    auto texoo = Texbin::from_path("gfdm issues/gf_common_mypack.bin");
    // auto texn = Texbin::from_path("gfdm issues/tex_gf_common_repack.bin");

    // avs_fs_addfs(avs_filesys_ramfs());

    // auto f = hook_avs_fs_open("/data/graphic/gmframe29.ifs", avs_open_mode_read(), 420);
    // log_assert(f >= 0);

    // // avs_file_to_vec but using the hook_read so ramfs demangler catches it
    // avs_stat stat = {0};
    // avs_fs_fstat(f, &stat);
    // std::vector<uint8_t> contents;
    // contents.resize(stat.filesize);
    // hook_avs_fs_read(f, &contents[0], stat.filesize);
    // avs_fs_close(f);

    // char args[] = "base=0x0000000000000000,size=0x00000000,mode=ro";
    // snprintf(args, sizeof(args), "base=0x%p,size=0x%llx,mode=ro", &contents[0], contents.size());
    // log_assert(hook_avs_fs_mount("/afpr2318908", "image.bin", "ramfs", args) >= 0);
    // log_assert(hook_avs_fs_mount("/afp23/data/graphic/gmframe29.ifs", "/afpr2318908/image.bin", "imagefs", NULL) >= 0);
    // hook_avs_fs_open("/afp23/data/graphic/gmframe29.ifs/tex/texturelist.xml", avs_open_mode_read(), 420);
    // hook_avs_fs_open("/afp23/data/graphic/gmframe29.ifs/tex/643b3d20d1b19dfe98e9e23a59a72bba", avs_open_mode_read(), 420);

    // log_info("loading file");
    // auto _debug = readFile("debug.bin");
    // if(!_debug) {
    //     return;
    // }
    // auto debug = *_debug;
    // log_info("Loaded %d bytes", debug.size());

    // // lz_unfuck(&debug[8], comp_sz);
    // // auto decomp = lz_decompress(&debug[8], comp_sz, &decomp_sz);
    // auto decomp = texbin_lz77_decompress(debug);
    // log_info("Decomp to %u", decomp.size());
    // auto comp = texbin_lz77_compress(decomp);
    // log_info("Comp again to %u", comp.size());
    // auto decomp2 = texbin_lz77_decompress(comp);
    // log_info("Final decomp to %u", decomp2.size());

    // auto f = fopen("debug.out.bin", "wb");
    // fwrite(&decomp[0], 1, decomp.size(), f);
    // fclose(f);

    // return;


    // auto _tex = Texbin::from_path("tex_l44qb_smc_sm.bin");
    // auto tex = Texbin::from_path("tex_custom.bin");
    // auto tex = Texbin::from_path("tex_l44_paseli_info.bin");
    // if(!tex) {
    //     return;
    // }
    // tex->debug();

// #ifdef TEXBIN_VERBOSE
//     tex->debug();
// #endif

//     tex->add_or_replace_image("AAA_NEW_IMAGE", "new image test.png");
//     tex->add_or_replace_image("SSM_000_T1000", "replace image test.png");

//     tex->save("tex_custom_modified.bin");
    // tex.save("tex_l44qb_smc_sm_modified.bin");

    /*string path = "testcases.xml";
    void* prop_buffer = NULL;
    property_t prop = NULL;

    auto f = avs_fs_open(path.c_str(), avs_open_mode_read(), 420);
    if (f < 0)
        return;

    auto memsize = property_read_query_memsize(avs_fs_read, f, NULL, NULL);
    if (memsize < 0) {
        log_warning("Couldn't get memsize %08X", memsize);
        goto FAIL;
    }

    // who knows
    memsize *= 10;

    prop_buffer = malloc(memsize);
    prop = property_create(PROP_READ|PROP_WRITE|PROP_APPEND|PROP_CREATE|PROP_JSON, prop_buffer, memsize);
    if (!prop) {
        log_warning("Couldn't create prop");
        goto FAIL;
    }

    avs_fs_lseek(f, 0, SEEK_SET);
    property_insert_read(prop, 0, avs_fs_read, f);
    avs_fs_close(f);

    property_file_write(prop, "testcases.json");

FAIL:
    if (f)
        avs_fs_close(f);
    if (prop)
        property_destroy(prop);
    if (prop_buffer)
        free(prop_buffer);*/

    /*auto d = avs_fs_opendir(config.mod_folder.c_str());
    if (!d) {
        log_warning("couldn't d");
        return;
    }
    for (char* n = avs_fs_readdir(d); n; n = avs_fs_readdir(d))
        log_info("dir %s", n);
    avs_fs_closedir(d);*/
    //char name[64];
    //auto playpen = prop_from_file("playpen.xml");
    //auto node = property_search(playpen, NULL, "/");
    //auto start = property_node_traversal(node, TRAVERSE_FIRST_CHILD);
    //auto end = property_node_traversal(start, TRAVERSE_LAST_SIBLING);
    //print_node(node);
    //print_node(start);
    //print_node(end);
    /*for (int i = 0; i <= 8; i++) {
        if (i == 6 || i == 3) continue;
        log_info("Traverse: %d", i);
        auto node = property_search(playpen, NULL, "/root/t2");
        auto nnn = property_node_traversal(node, 8);
        auto nna = property_node_traversal(nnn, TRAVERSE_FIRST_ATTR);
        property_node_name(nna, name, 64);
        log_info("bloop %s", name);
        for (;node;node = property_node_traversal(node, i)) {
            if (!property_node_name(node, name, 64)) {
                log_info("    %s", name);
            }
        }
    }*/
    //prop_free(playpen);
}

// used to compare my results against texbintool, the "known good" impl
void textypes() {
    // if the image we get isn't great
    int i = 0;
    // find . -type f | grep .bin$ | grep -Ev '^./(Groove|CHUNITHM|taiko|Crossbeats)' | grep -v 'DIVA AFT' > bins.txt
    ifstream files("bins.txt");
    for (string line; getline(files, line); ) {
        auto f = line;
        auto bin = Texbin::from_path(f.c_str());
        if(!bin) {
            continue;
        }

        // printf("%s\n", f.c_str());
        for(auto &[name, image] : bin->images) {
            // printf("%s %s\n", _name.c_str(), image.tex_type_str().c_str());
            auto type = image.tex_type_str();

            if(type == "BGR_16BIT" && ++i == 14) {
                system(("\"gitadora-texbintool.exe --no-split \"" + f + "\"\"").c_str());
                printf("%s %s\n", f.c_str(), name.c_str());

                auto decode = image.tex_to_argb8888();
                if(!decode) {
                    printf("Bailing, decode failed (type %s)\n", type.c_str());
                    exit(0);
                }
                auto [data, width, height] = *decode;
                lodepng_encode32_file("test.png", &data[0], width, height);

                // auto ff = fopen("test.bin", "wb");
                // fwrite(&data[0], 1, data.size(), ff);
                // fclose(ff);

                exit(0);
            }
            // printf("%s\n", image.tex_type_str().c_str());
        }
    }

    exit(0);
}

int main(int argc, char** argv) {
    if(!avs_standalone::boot(false)) {
        log_fatal("avs_standalone boot failed");
        return 1;
    }

    init(); // this double-hooks some AVS funcs, don't care

    avs_playpen();

    avs_standalone::shutdown();

    return 0;
}
