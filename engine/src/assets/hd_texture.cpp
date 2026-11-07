#include "assets/hd_texture.hpp"

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

#include <algorithm>

#include <SDL3/SDL.h>

#include <cstring>
#include <vector>

#ifdef CR_HAVE_ZLIB
#include <zlib.h>
#endif

namespace cr {

namespace {

void put32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back(uint8_t(v >> (8 * i)));
}

uint32_t get32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }

} // namespace

// An upscale's see-through pixels' colour: drawn filtered (the originals nearest), a cut-out's
// edge shows the colour under its part-transparent pixels - which the networks leave dark (a
// black fringe round hair and ornaments). Each pixel short of opaque takes the nearest opaque
// one's colour (a breadth-first fill from them), its alpha kept.
void fillSeeThrough(Image& img) {
    const int w = img.width, h = img.height;
    if (w <= 0 || h <= 0 || img.alphaOnly) return;
    std::vector<uint8_t>& px = img.rgba;
    constexpr uint8_t kOpaque = 250;
    std::vector<uint8_t> done(size_t(w) * size_t(h), 0);
    std::vector<int> queue;
    queue.reserve(size_t(w) * size_t(h) / 4);
    size_t solid = 0;
    for (int i = 0; i < w * h; ++i)
        if (px[size_t(i) * 4 + 3] >= kOpaque) done[size_t(i)] = 1, ++solid;
    if (solid == 0 || solid == size_t(w) * size_t(h)) return;
    for (int y = 0; y < h; ++y) // (the opaque pixels at the edge: where the fill starts)
        for (int x = 0; x < w; ++x) {
            const int i = y * w + x;
            if (!done[size_t(i)]) continue;
            if ((x > 0 && !done[size_t(i - 1)]) || (x + 1 < w && !done[size_t(i + 1)]) || (y > 0 && !done[size_t(i - w)]) ||
                (y + 1 < h && !done[size_t(i + w)]))
                queue.push_back(i);
        }
    for (size_t q = 0; q < queue.size(); ++q) {
        const int i = queue[q], x = i % w, y = i / w;
        const int next[4] = {x > 0 ? i - 1 : -1, x + 1 < w ? i + 1 : -1, y > 0 ? i - w : -1, y + 1 < h ? i + w : -1};
        for (int n : next) {
            if (n < 0 || done[size_t(n)]) continue;
            done[size_t(n)] = 1;
            for (int c = 0; c < 3; ++c) px[size_t(n) * 4 + size_t(c)] = px[size_t(i) * 4 + size_t(c)];
            queue.push_back(n);
        }
    }
}

uint64_t imageHash(const Image& img) {
    uint64_t h = 1469598103934665603ull;
    auto mix = [&](const uint8_t* p, size_t n) {
        for (size_t i = 0; i < n; ++i) {
            h ^= p[i];
            h *= 1099511628211ull;
        }
    };
    const uint32_t head[3] = {uint32_t(img.width), uint32_t(img.height), img.alphaOnly ? 1u : 0u};
    mix(reinterpret_cast<const uint8_t*>(head), sizeof head);
    mix(img.rgba.data(), img.rgba.size());
    return h;
}

std::string hashName(uint64_t hash) {
    char s[17];
    SDL_snprintf(s, sizeof s, "%016llx", static_cast<unsigned long long>(hash));
    return s;
}

bool saveRawImage(const std::string& path, const Image& img) {
    std::vector<uint8_t> out = {'C', 'R', 'R', 'A', 'W', '1'};
    put32(out, uint32_t(img.width));
    put32(out, uint32_t(img.height));
    put32(out, (img.alphaOnly ? 1u : 0u) | uint32_t(img.storage) << 1);
    out.insert(out.end(), img.rgba.begin(), img.rgba.end());
    return SDL_SaveFile(path.c_str(), out.data(), out.size());
}

namespace {

// A replacement left out: why ("<hash>.png: ..."), and false

uint16_t dds16(const uint8_t* p) { return uint16_t(p[0]) | uint16_t(p[1]) << 8; }
uint8_t ddsExpand(uint32_t v, int n) { const uint32_t m=(1u<<n)-1; return uint8_t((v*255u+m/2)/m); }
void dds565(uint16_t v,uint8_t* o) { o[0]=ddsExpand((v>>11)&31,5); o[1]=ddsExpand((v>>5)&63,6); o[2]=ddsExpand(v&31,5); o[3]=255; }
void ddsColor(const uint8_t* b,uint8_t c[4][4],bool four) {
 uint16_t a=dds16(b),d=dds16(b+2);dds565(a,c[0]);dds565(d,c[1]);
 if(a>d||four){for(int k=0;k<3;k++){c[2][k]=uint8_t((2*c[0][k]+c[1][k]+1)/3);c[3][k]=uint8_t((c[0][k]+2*c[1][k]+1)/3);}c[2][3]=c[3][3]=255;}
 else{for(int k=0;k<3;k++)c[2][k]=uint8_t((c[0][k]+c[1][k])/2);c[2][3]=255;c[3][0]=c[3][1]=c[3][2]=c[3][3]=0;}
}
void ddsAlpha(const uint8_t* b,uint8_t v[16]){
 uint8_t p[8]={b[0],b[1]};
 if(b[0]>b[1])for(int i=1;i<=6;i++)p[i+1]=uint8_t(((7-i)*b[0]+i*b[1]+3)/7);
 else{for(int i=1;i<=4;i++)p[i+1]=uint8_t(((5-i)*b[0]+i*b[1]+2)/5);p[6]=0;p[7]=255;}
 uint64_t bits=0;for(int i=0;i<6;i++)bits|=uint64_t(b[i+2])<<(8*i);for(int i=0;i<16;i++)v[i]=p[(bits>>(3*i))&7];
}
bool decodeDds(const uint8_t* p,size_t n,Image& out){
 if(!p||n<128||std::memcmp(p,"DDS ",4)||get32(p+4)!=124||get32(p+76)!=32)return false;
 uint32_t h=get32(p+12),w=get32(p+16),flags=get32(p+80),fcc=get32(p+84);
 if(!w||!h||w>8192||h>8192)return false;
 enum K{BC1,BC2,BC3,BC4,BC5} k;size_t at=128;
 if(flags&4u){
  if(fcc==0x31545844u)k=BC1;else if(fcc==0x33545844u)k=BC2;else if(fcc==0x35545844u)k=BC3;
  else if(fcc==0x55344342u||fcc==0x31495441u)k=BC4;else if(fcc==0x55354342u||fcc==0x32495441u)k=BC5;
  else if(fcc==0x30315844u){if(n<148||get32(p+132)!=3||(get32(p+136)&4u)||get32(p+140)!=1)return false;at=148;uint32_t z=get32(p+128);
   if(z==71||z==72)k=BC1;else if(z==74||z==75)k=BC2;else if(z==77||z==78)k=BC3;else if(z==80)k=BC4;else if(z==83)k=BC5;else return false;
  }else return false;
 }else return false;
 if(get32(p+112)&0x0020fe00u)return false;
 size_t bs=(k==BC1||k==BC4)?8:16;uint32_t nx=(w+3)/4,ny=(h+3)/4;size_t count=size_t(nx)*ny;if(at>n||count>(n-at)/bs)return false;
 out.width=int(w);out.height=int(h);out.rgba.assign(size_t(w)*h*4,0);
 for(uint32_t by=0;by<ny;by++)for(uint32_t bx=0;bx<nx;bx++){
  const uint8_t* b=p+at+(size_t(by)*nx+bx)*bs;uint8_t c[4][4]{},a[16]{},r[16]{},g[16]{};const uint8_t* col=b;bool four=false;
  if(k==BC2||k==BC3){col+=8;four=true;if(k==BC2)for(int i=0;i<16;i++)a[i]=uint8_t(((b[i/2]>>((i&1)*4))&15)*17);else ddsAlpha(b,a);}
  else if(k==BC4)ddsAlpha(b,r);else if(k==BC5){ddsAlpha(b,r);ddsAlpha(b+8,g);}
  if(k<=BC3)ddsColor(col,c,four);uint32_t ix=get32(col+4);
  for(uint32_t y=0;y<4;y++)for(uint32_t x=0;x<4;x++){uint32_t X=bx*4+x,Y=by*4+y;if(X>=w||Y>=h)continue;int q=int(y*4+x);uint8_t* d=out.rgba.data()+(size_t(Y)*w+X)*4;
   if(k<=BC3){std::memcpy(d,c[(ix>>(2*q))&3],4);if(k!=BC1)d[3]=a[q];}
   else if(k==BC4){d[0]=r[q];d[1]=d[2]=0;d[3]=255;}else{d[0]=r[q];d[1]=g[q];d[2]=255;d[3]=255;}
  }
 }
 return true;
}

bool leftOut(std::string* why, const std::string& file, const std::string& what) {
    if (why) *why = file + ": " + what;
    return false;
}

std::string sizesText(int w, int h, int originalW, int originalH) {
    return std::to_string(w) + "x" + std::to_string(h) + " for a " + std::to_string(originalW) + "x" + std::to_string(originalH) +
           " picture (its shape, at least its size)";
}

// PNG's chunk check (CRC-32, the IEEE polynomial)
uint32_t crc32Of(const uint8_t* p, size_t n) {
    uint32_t c = 0xffffffffu;
    for (size_t i = 0; i < n; ++i) {
        c ^= p[i];
        for (int k = 0; k < 8; ++k) c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1;
    }
    return ~c;
}

void putBe32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 3; i >= 0; --i) out.push_back(uint8_t(v >> (8 * i)));
}

} // namespace

bool hdShapeFits(int w, int h, int originalW, int originalH) {
    if (originalW <= 0 || originalH <= 0 || w < originalW || h < originalH || w > 8192 || h > 8192) return false;
    // (one scale s for both: w = originalW s + a, h = originalH s + b, each rounded, |a|, |b| <= 1/2)
    const int64_t off = int64_t(w) * originalH - int64_t(h) * originalW;
    return 2 * (off < 0 ? -off : off) <= int64_t(originalW) + originalH;
}

bool savePngImage(const std::string& path, const Image& img) {
    const size_t n = size_t(std::max(img.width, 0)) * size_t(std::max(img.height, 0)) * 4;
    if (!n || img.rgba.size() < n) return false;
    std::vector<uint8_t> rows(img.rgba.begin(), img.rgba.begin() + std::ptrdiff_t(n));
    std::vector<uint8_t> png;
    stbi_write_png_to_func(
        [](void* to, void* data, int size) {
            auto* v = static_cast<std::vector<uint8_t>*>(to);
            v->insert(v->end(), static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + size);
        },
        &png, img.width, img.height, 4, rows.data(), img.width * 4);
    if (png.size() < 33 || std::memcmp(png.data() + 12, "IHDR", 4) != 0) return false;
    // the .crraw's flags for tools/hd_textures.py (which upscaler), after IHDR
    const std::string text = std::string("cr-flags") + '\0' + std::to_string((img.alphaOnly ? 1u : 0u) | uint32_t(img.storage) << 1);
    std::vector<uint8_t> chunk;
    putBe32(chunk, uint32_t(text.size()));
    chunk.insert(chunk.end(), {'t', 'E', 'X', 't'});
    chunk.insert(chunk.end(), text.begin(), text.end());
    putBe32(chunk, crc32Of(chunk.data() + 4, chunk.size() - 4));
    png.insert(png.begin() + 33, chunk.begin(), chunk.end());
    return SDL_SaveFile(path.c_str(), png.data(), png.size());
}

bool loadHdImage(const std::string& dir, const Image& original, Image& out, std::string* why) {
    if (why) why->clear();
    if (dir.empty() || original.width <= 0) return false;
    const std::string name = hashName(imageHash(original));
    size_t size = 0;
#ifdef CR_HAVE_ZLIB
    if (void* data = SDL_LoadFile((dir + name + ".crhd").c_str(), &size)) {
        const uint8_t* p = static_cast<const uint8_t*>(data);
        std::string what;
        if (size <= 26 || std::memcmp(p, "CRHD1", 6) != 0) what = "not a .crhd";
        const uint32_t w = what.empty() ? get32(p + 6) : 0, h = what.empty() ? get32(p + 10) : 0;
        if (what.empty() && (get32(p + 14) != uint32_t(original.width) || get32(p + 18) != uint32_t(original.height)))
            what = "made for a picture of another size";
        if (what.empty() && !hdShapeFits(int(std::min(w, 65536u)), int(std::min(h, 65536u)), original.width, original.height))
            what = sizesText(int(std::min(w, 65536u)), int(std::min(h, 65536u)), original.width, original.height);
        if (what.empty()) {
            out.width = int(w);
            out.height = int(h);
            out.alphaOnly = (get32(p + 22) & 1) != 0;
            out.rgba.resize(size_t(w) * h * 4);
            uLongf len = uLongf(out.rgba.size());
            if (uncompress(out.rgba.data(), &len, p + 26, uLong(size - 26)) != Z_OK || len != out.rgba.size()) what = "its pixels don't unpack";
        }
        SDL_free(data);
        return what.empty() || leftOut(why, name + ".crhd", what);
    }
#endif
    if (void* data = SDL_LoadFile((dir + name + ".png").c_str(), &size)) { // (any upscale or edit: the original's shape)
        int w = 0, h = 0, channels = 0;
        stbi_uc* px = stbi_load_from_memory(static_cast<const stbi_uc*>(data), int(std::min(size, size_t(1) << 30)), &w, &h, &channels, 4);
        SDL_free(data);
        if (!px) return leftOut(why, name + ".png", "not a PNG this reads");
        if (!hdShapeFits(w, h, original.width, original.height)) {
            stbi_image_free(px);
            return leftOut(why, name + ".png", sizesText(w, h, original.width, original.height));
        }
        out.width = w;
        out.height = h;
        out.alphaOnly = original.alphaOnly;
        out.rgba.assign(px, px + size_t(w) * size_t(h) * 4);
        stbi_image_free(px);
        return true;
    }
    if (void* data = SDL_LoadFile((dir + name + ".dds").c_str(), &size)) {
        const bool ok = decodeDds(static_cast<const uint8_t*>(data), size, out);
        SDL_free(data);
        if (!ok) return leftOut(why, name + ".dds", "unsupported DDS (DXT1/3/5 or BC4/5 UNORM expected)");
        if (!hdShapeFits(out.width, out.height, original.width, original.height))
            return leftOut(why, name + ".dds", sizesText(out.width, out.height, original.width, original.height));
        out.alphaOnly = original.alphaOnly;
        return true;
    }
    return false;
}

} // namespace cr
