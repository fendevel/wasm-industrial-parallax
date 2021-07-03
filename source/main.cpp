#include "wasmdefs.hpp"
#include "keycodes.hpp"
#include "math.hpp"
#include "imports.hpp"

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

static constexpr rgba8_type unpack_rgba8(u32 col)
{
    return {
        ((col >>  0) & 255),
        ((col >>  8) & 255),
        ((col >> 16) & 255),
        ((col >> 24) & 255)
    };
}

static vec2i screen_size;
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

static image_load music_off_icon;
static image_load music_on_icon;
static image_load parallax_industrial[4];

static i32 audio_track = -1;

static bool const *keystate_ptr;
static u32 keystate_len;

static bool const *buttonstate_ptr;
static bool *buttonstate_old;
static u32 buttonstate_len;


static void clear_screen(u32 col)
{
    v128_t const col128 = wasm_i32x4_splat(col);
    for (i32 j = 0; j < screen_size.y; ++j)
    {
        for (i32 i = 0; i < screen_size.x; i += 8)
        {
            i32 const dst_offset = j * screen_size.x + i;
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
    
    static i32 scroll{};

    for (i32 i = 0; i < length_of(parallax_industrial); ++i)
    {
        i32 const amount = ((scroll/2)*i);
        auto const &parallax = parallax_industrial[i];

        for (i32 j = 0; j < 3; ++j)
        {
            draw_sprite(screen_size.x, screen_size.y, screen_buffer, parallax.image, {screen_size.x - ((amount + parallax.image.w*3*j)%(screen_size.x + parallax.image.w*3)), screen_size.y - parallax.image.h*3}, 3);
        }
    }

    if (music_playing)
    {
        draw_sprite(screen_size.x, screen_size.y, screen_buffer, music_on_icon.image, {screen_size.x - music_on_icon.image.w, 0}, 1);
    }
    else
    {
        draw_sprite(screen_size.x, screen_size.y, screen_buffer, music_off_icon.image, {screen_size.x - music_off_icon.image.w, 0}, 1);
    }

    scroll += 1;

    memcpy(buttonstate_old, buttonstate_ptr, buttonstate_len);

    return 0;
}

[[clang::export_name("entry")]] i32 entry(i32 w, i32 h)
{
    screen_size = {w, h};

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

    audio_set_volume(audio_track, .125f);

    return 1;
}