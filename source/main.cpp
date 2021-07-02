#include "wasmdefs.hpp"
#include "keycodes.hpp"
#include "math.hpp"
#include "imports.hpp"

static constexpr u32 align(u32 val, u32 alignment)
{
    return ((val + (alignment - 1)) & ~(alignment - 1));
}

extern "C" {
    void* malloc(size_t size);
    void free(void *ptr);
}

[[clang::export_name("zalloc")]]
void *zalloc(size_t size)
{
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

template <typename T, u32 size>
static constexpr u32 length_of(T const (& bla)[size])
{
    return size;
}

static constexpr u32 rgba(u32 r, u32 g, u32 b, u32 a)
{
    return 
        ((r <<  0)) |
        ((g <<  8)) |
        ((b << 16)) |
        ((a << 24));
}

struct rgba8_type 
{
    u32 r, g, b, a;
};

static constexpr rgba8_type unpack_rgba8(u32 abgr)
{
    return {
        ((abgr >>  0) & 255),
        ((abgr >>  8) & 255),
        ((abgr >> 16) & 255),
        ((abgr >> 24) & 255)
    };
}

static f32 to_rotation(vec2f v)
{
    float const res = math::acos(v.x / v.len());
    if (v.y > 0.f)
    {
        return math::tau - res;
    }

    return res;
}

static i32 screen_width;
static i32 screen_height;
static u32 screen_buffer_len;
static u32* screen_buffer;

static void test_animation()
{
    static u32 n = 0;
    i32 const val0 = ((math::sin((n % 256) / 255.f * math::pi * 2.f) + 1) * .5f) * 255;
    i32 const val1 = ((math::cos((n % 256) / 255.f * math::pi * 2.f) + 1) * .5) * 255;
    
    n += 1;
    u32 const col = rgba(val0, 0, val1, 255);

    for (i32 i = 0; i < screen_buffer_len; i += 4)
    {
        screen_buffer[i + 0] = col;
        screen_buffer[i + 1] = col;
        screen_buffer[i + 2] = col;
        screen_buffer[i + 3] = col;
    }
}

static u32 blend(u32 src0, u32 src1)
{
    auto const src0v = unpack_rgba8(src0);
    auto const src1v = unpack_rgba8(src1);

    auto const nAlpha = src0v.a;

    if( 0 == nAlpha )
    {
        return src1;
    }

    if( 255 == nAlpha )
    {
        return src0;
    }

    auto nInvAlpha = 255-nAlpha;

    auto nSrcRed   = src0v.r; 
    auto nSrcGreen = src0v.g; 
    auto nSrcBlue  = src0v.b; 

    auto nDestRed   = src1v.r; 
    auto nDestGreen = src1v.g; 
    auto nDestBlue  = src1v.b;  

    auto nRed  = ( nSrcRed   * nAlpha + nDestRed * nInvAlpha   )>>8;
    auto nGreen= ( nSrcGreen * nAlpha + nDestGreen * nInvAlpha )>>8;
    auto nBlue = ( nSrcBlue  * nAlpha + nDestBlue * nInvAlpha  )>>8;
    
    return rgba(nRed, nGreen, nBlue, 0xff);
}

static v128_t blend_simd(v128_t src0, v128_t src1)
{
    v128_t const src_r128 = wasm_v128_and(wasm_i32x4_shr(src0,  0), wasm_i32x4_splat(0xff));
    v128_t const src_g128 = wasm_v128_and(wasm_i32x4_shr(src0,  8), wasm_i32x4_splat(0xff));
    v128_t const src_b128 = wasm_v128_and(wasm_i32x4_shr(src0, 16), wasm_i32x4_splat(0xff));
    
    v128_t const src_a128 = wasm_v128_and(wasm_i32x4_shr(src0, 24), wasm_i32x4_splat(0xff));

    v128_t const dst_r128 = wasm_v128_and(wasm_i32x4_shr(src1,  0), wasm_i32x4_splat(0xff));
    v128_t const dst_g128 = wasm_v128_and(wasm_i32x4_shr(src1,  8), wasm_i32x4_splat(0xff));
    v128_t const dst_b128 = wasm_v128_and(wasm_i32x4_shr(src1, 16), wasm_i32x4_splat(0xff));

    v128_t const inv_a128 = wasm_i32x4_sub(wasm_i32x4_splat(255), src_a128);

    v128_t const nr128 = wasm_i32x4_shr(wasm_i32x4_add(wasm_i32x4_mul(src_r128, src_a128), wasm_i32x4_mul(dst_r128, inv_a128)), 8);
    v128_t const ng128 = wasm_i32x4_shr(wasm_i32x4_add(wasm_i32x4_mul(src_g128, src_a128), wasm_i32x4_mul(dst_g128, inv_a128)), 8);
    v128_t const nb128 = wasm_i32x4_shr(wasm_i32x4_add(wasm_i32x4_mul(src_b128, src_a128), wasm_i32x4_mul(dst_b128, inv_a128)), 8);

    v128_t const res_r128 = wasm_i32x4_shl(nr128,  0);
    v128_t const res_g128 = wasm_i32x4_shl(ng128,  8);
    v128_t const res_b128 = wasm_i32x4_shl(nb128, 16);
    v128_t const res_a128 = wasm_i32x4_splat(0xff << 24);

    return wasm_v128_or(res_r128, wasm_v128_or(res_g128, wasm_v128_or(res_b128, res_a128)));
}

static constexpr v128_t wasm_u32x4_const(uint32_t __c0, uint32_t __c1, uint32_t __c2, uint32_t __c3)
{
    return (v128_t)(__u32x4){__c0, __c1, __c2, __c3};
}

static v128_t wasm_u32x4_make(uint32_t __c0, uint32_t __c1, uint32_t __c2, uint32_t __c3) {
    return (v128_t)(__u32x4){__c0, __c1, __c2, __c3};
}

static v128_t  wasm_u8x16_splat(uint8_t __a) {
  return (v128_t)(__u8x16){__a, __a, __a, __a, __a, __a, __a, __a,
                           __a, __a, __a, __a, __a, __a, __a, __a};
}

static v128_t wasm_u8x16_make(uint8_t __c0, uint8_t __c1, uint8_t __c2, uint8_t __c3, uint8_t __c4,
                uint8_t __c5, uint8_t __c6, uint8_t __c7, uint8_t __c8, uint8_t __c9,
                uint8_t __c10, uint8_t __c11, uint8_t __c12, uint8_t __c13,
                uint8_t __c14, uint8_t __c15) {
  return (v128_t)(__u8x16){__c0,  __c1,  __c2,  __c3, __c4,  __c5,
                           __c6,  __c7,  __c8,  __c9, __c10, __c11,
                           __c12, __c13, __c14, __c15};
}

static v128_t
wasm_u16x8_extend_low_u8x16(v128_t __a) {
  return (v128_t) __builtin_convertvector(
      (__u8x8){((__u8x16)__a)[0], ((__u8x16)__a)[1], ((__u8x16)__a)[2],
               ((__u8x16)__a)[3], ((__u8x16)__a)[4], ((__u8x16)__a)[5],
               ((__u8x16)__a)[6], ((__u8x16)__a)[7]},
      __u16x8);
}

static v128_t wasm_u32x4_extend_low_u16x8(v128_t __a) {
  return (v128_t) __builtin_convertvector(
      (__u16x4){((__u16x8)__a)[0], ((__u16x8)__a)[1], ((__u16x8)__a)[2],
                ((__u16x8)__a)[3]},
      __u32x4);
}

static void draw_line(u32 dstw, u32 dsth, u32 dst_len, u32 *dst, i32 x0, i32 y0, i32 x1, i32 y1, u32 rgba)
{
    x1 -= x0;
    y1 -= y0;
    i32 g = 0;

    if (math::abs(x1) >= math::abs(y1)) 
    {
        g = math::abs(x1);
        if (0 != g) 
        {
            y1 = 65536 * y1 / g;
        }
        x1 = 0 <= x1 ? 65536 : -65536;
    } 
    else 
    {
        g = math::abs(y1);
        if (0 != g)
        {
            x1 = 65536 * x1 / g;
        }
        y1 = 0 <= y1 ? 65536 : -65536;
    }
    
    x0 = 65536 * x0 + 32768;
    y0 = 65536 * y0 + 32768;

    while (0 <= g)
    {
        i32 const x = (x0 >> 16);
        i32 const y = (y0 >> 16);

        if ((x >= 0 && x < dstw) &&
            (y >= 0 && y < dsth))
        {
            i32 pixel_pos = y * dstw + x;
            dst[pixel_pos] = blend(rgba, dst[pixel_pos]);
        }

        g -= 1;
        x0 += x1;
        y0 += y1;
    }
}

static void draw_rect(u32 dstw, u32 dsth, u32 dst_len, u32 *dst, i32 x, i32 y, i32 w, i32 h, u32 rgba)
{
    w -= 1;
    h -= 1;

    draw_line(screen_width, screen_height, screen_buffer_len, screen_buffer, x, y, x + w, y, rgba);
    draw_line(screen_width, screen_height, screen_buffer_len, screen_buffer, x, y + h, x + w, y + h, rgba);
    draw_line(screen_width, screen_height, screen_buffer_len, screen_buffer, x, y, x, y + h, rgba);
    draw_line(screen_width, screen_height, screen_buffer_len, screen_buffer, x + w, y, x + w, y + h, rgba);
}

static void draw_sprite(u32 dstw, u32 dsth, u32 *dst, image const &src, vec2i offset, i32 upscale)
{
    for (i32 j = 0; j < src.h*upscale; ++j)
    {
        for (i32 i = 0; i < src.w*upscale; ++i)
        {
            i32 const src_index = j/upscale * src.w + i/upscale;
            
            i32 const dstx = i + offset.x;
            i32 const dsty = j + offset.y;
            i32 const dst_index = dsty * dstw + dstx;

            if (dstx < 0 || dstx > dstw - 1 || dsty < 0 || dsty > dsth - 1)
            {
                continue;
            }

            u32 const res = blend(src.data[src_index], dst[dst_index]);
            dst[dst_index] = res;
        }
    }
}

struct image_load
{
    i32 id = -1;
    image image;
    bool loaded;
};

static bool image_loaded(image_load &img)
{
    if (!img.loaded)
    {
        if (!image_ready(img.id))
        {
            print("waiting on image!");
            return false;
        }
        else
        {
            print("image ready!");
            img.loaded = true;

            img.image = get_image(img.id);

            print(img.image.w);
            print(img.image.h);
            return true;
        }
    }

    return true;
}

static image_load music_off_icon {.id{-1}};
static image_load music_on_icon {.id{-1}};
static image_load parallax_industrial[4]{
    {.id{-1}},
    {.id{-1}},
    {.id{-1}},
    {.id{-1}}
};

static i32 audio_track = -1;

static bool const *keystate_ptr;
static u32 keystate_len;

static bool const *buttonstate_ptr;
static bool *buttonstate_old;
static u32 buttonstate_len;


static void clear_screen(u32 col)
{
    v128_t const col128 = wasm_i32x4_splat(col);
    for (i32 j = 0; j < screen_height; ++j)
    {
        for (i32 i = 0; i < screen_width; i += 8)
        {
            i32 const dst_offset = j * screen_width + i;
            wasm_v128_store(screen_buffer + dst_offset+0, col128);
            wasm_v128_store(screen_buffer + dst_offset+4, col128);
        }
    }
}

static bool music_playing = false;

[[clang::export_name("on_frame")]] i32 on_frame()
{
    if (!image_loaded(music_off_icon) ||
        !image_loaded(music_on_icon)
        )
    {
        return 1;
    }

    for (auto &parallax : parallax_industrial)
    {
        if (!image_loaded(parallax))
        {
            return 1;
        }
    }

    if (buttonstate_ptr[buttoncode_left] && !buttonstate_old[buttoncode_left])
    {
        music_playing = !music_playing;

        if (music_playing)
        {
            audio_play(audio_track, true);
        }
        else 
        {
            audio_pause(audio_track);
        }
    }

    vec2i const mouse = cursor_xy();

    clear_screen(rgba(25, 40, 31, 255));
    
    static i32 s{};

    for (i32 i = 0; i < length_of(parallax_industrial); ++i)
    {
        i32 const amount = ((s/2)*i);
        auto const &parallax = parallax_industrial[i];

        for (i32 j = 0; j < 3; ++j)
        {
            draw_sprite(screen_width, screen_height, screen_buffer, parallax.image, {screen_width - ((amount + parallax.image.w*3*j)%(screen_width + parallax.image.w*3)), screen_height - parallax.image.h*3}, 3);
        }
    }

    if (music_playing)
    {
        draw_sprite(screen_width, screen_height, screen_buffer, music_on_icon.image, {screen_width - music_on_icon.image.w, 0}, 1);
    }
    else
    {
        draw_sprite(screen_width, screen_height, screen_buffer, music_off_icon.image, {screen_width - music_off_icon.image.w, 0}, 1);
    }

    s += 1;

    vec2f const vertices[]{
        {f32(mouse.x), f32(mouse.y)}, {0, f32(screen_height)}, {f32(screen_width), f32(screen_height)}
    };

    if (keystate_ptr[keycode_Q])
    {
        audio_play(audio_track, true);
    }

    if (keystate_ptr[keycode_E])
    {
        audio_pause(audio_track);
    }

    memcpy(buttonstate_old, buttonstate_ptr, buttonstate_len);

    return 0;
}

[[clang::export_name("main")]] i32 entry(i32 w, i32 h)
{
    screen_width = w;
    screen_height = h;

    music_off_icon.id = request_image("./image/outline_volume_off_white_24dp.png");
    music_on_icon.id = request_image("./image/outline_volume_up_white_24dp.png");

    parallax_industrial[0].id = request_image("./image/bg.png");
    parallax_industrial[1].id = request_image("./image/far-buildings.png");
    parallax_industrial[2].id = request_image("./image/buildings.png");
    parallax_industrial[3].id = request_image("./image/skill-foreground.png");

    audio_track = request_audio("./audio/industrial.wav");

    {
        auto const[ptr, len] = get_keystate_buffer();
        keystate_ptr = ptr;
        keystate_len = len;
    }

    {
        auto const[ptr, len] = get_buttonstate_buffer();
        buttonstate_ptr = ptr;
        buttonstate_len = len;
    }

    buttonstate_old = reinterpret_cast<bool*>(zalloc(buttonstate_len));

    {
        auto const[ptr, len] = get_screen_buffer();
        screen_buffer = ptr;
        screen_buffer_len = len;
    }

    audio_set_volume(audio_track, .25f);

    return 1;
}