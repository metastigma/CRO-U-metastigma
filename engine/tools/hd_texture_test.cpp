// HD textures: a replacement's shape checked; an exported PNG upright with its flags; a PNG replacement
// read back in the Image's own order, and one of the wrong shape left out with the reason
#include "assets/hd_texture.hpp"
#include "stb/stb_image.h"
#include <SDL3/SDL.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

int main() {
    namespace fs = std::filesystem;
    int problems = 0;
    auto check = [&](bool ok, const char* what) {
        if (!ok) ++problems, std::fprintf(stderr, "hd texture FAILED: %s\n", what);
    };
    check(cr::hdShapeFits(64, 32, 32, 16), "2x");
    check(cr::hdShapeFits(32, 16, 32, 16), "its own size");
    check(cr::hdShapeFits(13, 8, 5, 3), "2.5x, each side rounded");
    check(!cr::hdShapeFits(64, 64, 32, 16), "a square for a 2:1 picture");
    check(!cr::hdShapeFits(32, 64, 64, 32), "a picture turned on its side");
    check(!cr::hdShapeFits(16, 8, 32, 16), "smaller than the original");
    check(!cr::hdShapeFits(16384, 8192, 64, 32), "over 8192");
    const fs::path root = fs::temp_directory_path() /
                          ("cr-hd-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directories(root)) return std::fprintf(stderr, "hd texture: no folder\n"), 1;
    const std::string dir = root.string() + "/";
    cr::Image original;
    original.width = original.height = 2;
    original.rgba = {1, 2, 3, 255, 4, 5, 6, 255, 7, 8, 9, 255, 10, 11, 12, 255};
    cr::Image big; // (its rows numbered: row r's red r)
    big.width = big.height = 4;
    for (int r = 0; r < 4; ++r)
        for (int x = 0; x < 4; ++x) big.rgba.insert(big.rgba.end(), {uint8_t(r), uint8_t(x), 200, 255});
    const std::string name = cr::hashName(cr::imageHash(original));
    check(cr::savePngImage(dir + name + ".png", big), "a PNG written");
    std::vector<uint8_t> file;
    { // (closed before the file is written again: Windows won't replace an open file)
        std::ifstream in(dir + name + ".png", std::ios::binary);
        file.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
    check(std::string(file.begin(), file.end()).find(std::string("cr-flags") + '\0') != std::string::npos, "the flags in a tEXt chunk");
    int w = 0, h = 0, n = 0;
    stbi_uc* px = file.empty() ? nullptr : stbi_load_from_memory(file.data(), int(file.size()), &w, &h, &n, 4);
    check(px && w == 4 && h == 4 && px[0] == 0, "the PNG upright: the picture's top row first");
    if (px) stbi_image_free(px);
    cr::Image out;
    std::string why;
    check(cr::loadHdImage(dir, original, out, &why) && out.width == 4 && out.height == 4 && out.rgba == big.rgba,
          "a PNG replacement read in the Image's own order");
    cr::Image wide = big; // (4 x 2 for a 2 x 2 picture)
    wide.height = 2, wide.rgba.resize(4 * 2 * 4);
    check(cr::savePngImage(dir + name + ".png", wide), "a second PNG written");
    check(!cr::loadHdImage(dir, original, out, &why) && why.find("4x2") != std::string::npos, "the wrong shape left out, saying so");
    fs::remove(dir + name + ".png");
    std::vector<uint8_t> dds(136,0);
    auto put32le=[&](size_t at,uint32_t v){for(int k=0;k<4;k++)dds[at+size_t(k)]=uint8_t(v>>(8*k));};
    dds[0]='D';dds[1]='D';dds[2]='S';dds[3]=' ';put32le(4,124);put32le(8,0x81007);put32le(12,4);put32le(16,4);put32le(20,8);
    put32le(76,32);put32le(80,4);put32le(84,0x31545844);put32le(108,0x1000);dds[129]=0xf8;dds[130]=0xe0;dds[131]=7;
    check(SDL_SaveFile((dir+name+".dds").c_str(),dds.data(),dds.size()),"a DXT1 DDS written");
    check(cr::loadHdImage(dir,original,out,&why)&&out.width==4&&out.height==4&&out.rgba[0]==255&&out.rgba[1]==0&&out.rgba[2]==0,
          "a DXT1 DDS replacement decoded");
    std::error_code ec;
    fs::remove_all(root, ec);
    std::printf("hd texture: %d problems\n", problems);
    return problems ? 1 : 0;
}
