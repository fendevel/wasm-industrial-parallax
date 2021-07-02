#ifndef IMPORTS
#define IMPORTS
#include "wasmdefs.hpp"
#include "math.hpp"

constexpr size_t strlen(char const *str)
{
    size_t res = 0;

    while(str[res])
    {
        res += 1;
    }

    return res;
}

/*relies on clang to decompose into primitives*/
class string_param
{
    char const *ptr;
    size_t len;
public:
    constexpr string_param(char const *str) : ptr(str), len(strlen(str)) {}
};

template <typename A, typename B>
struct pair
{
    A first;
    B second;
};

struct image { u32 *data; i32 w, h; };

[[clang::import_name("is_focused")]] vec2i is_focused();

[[clang::import_name("request_image")]] i32 request_image(string_param uri);
[[clang::import_name("image_ready")]] bool image_ready(i32 id);
[[clang::import_name("get_image")]] image get_image(i32 id);

[[clang::import_name("request_audio")]] i32 request_audio(string_param uri);
[[clang::import_name("audio_play")]] i32 audio_play(i32 id, bool loop = false);
[[clang::import_name("audio_pause")]] i32 audio_pause(i32 id);
[[clang::import_name("audio_resume")]] i32 audio_resume(i32 id);
[[clang::import_name("audio_set_volume")]] void audio_set_volume(i32 id, f32 level);

[[clang::import_name("cursor_inside")]] bool cursor_inside();
[[clang::import_name("cursor_xy")]] vec2i cursor_xy();
[[clang::import_name("get_keystate_buffer")]] pair<bool const *, u32> get_keystate_buffer();
[[clang::import_name("get_buttonstate_buffer")]] pair<bool const *, u32> get_buttonstate_buffer();
[[clang::import_name("get_screen_buffer")]] pair<u32 *, u32> get_screen_buffer();

[[clang::import_name("print_i32")]] void print(i32 val);
[[clang::import_name("print_i32_array")]] void print(u32 len, i32 const *vals);
[[clang::import_name("print_f32_array")]] void print(u32 len, f32 const *vals);
[[clang::import_name("print_u32")]] void print(u32 val);
[[clang::import_name("print_f32")]] void print(f32 val);
[[clang::import_name("print_str")]] void print(string_param str);
[[clang::import_name("console_clear")]] void console_clear();


#endif /* IMPORTS */
