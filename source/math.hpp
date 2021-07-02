#ifndef MATH
#define MATH
#include "wasmdefs.hpp"
#include "type_traits.hpp"
#include <wasm_simd128.h>

struct vec2i 
{ 
    i32 x, y; 
    inline i32 len() const;
};
struct vec2f 
{ 
    f32 x, y;
    inline f32 len() const;
};
struct vec3f 
{ 
    f32 x, y, z;
    inline f32 len() const;
};

namespace math
{
    constexpr float pi = 3.14159f;
    constexpr float tau = 2.f*pi;
    
    template <typename T>
    concept signed_number = std::is_signed<T>::value;

    template <typename T>
    concept number = std::is_signed<T>::value || std::is_unsigned<T>::value;

    [[clang::import_name("cosf")]] f32 cos(f32 val);
    [[clang::import_name("sinf")]] f32 sin(f32 val);
    [[clang::import_name("acosf")]] f32 acos(f32 val);
    [[clang::import_name("asinf")]] f32 asin(f32 val);
    [[clang::import_name("sqrtf")]] f32 sqrt(f32 val);
    [[clang::import_name("sqrti")]] f32 sqrt(i32 val);
    [[clang::import_name("floor")]] f32 floor(f32 val);
    [[clang::import_name("ceil")]] f32 ceil(f32 val);
    [[clang::import_name("mod")]] f32 mod(f32 val, f32 b);

    inline number auto min(number auto a, number auto b)
    {
        return a < b ? a : b;
    }

    inline number auto max(number auto a, number auto b)
    {
        return a > b ? a : b;
    }

    inline vec2f min(vec2f a, vec2f b) { return { min(a.x, b.x), min(a.y, b.y) }; }
    inline vec2f max(vec2f a, vec2f b) { return { max(a.x, b.x), max(a.y, b.y) }; }

    inline vec2i min(vec2i a, vec2i b) { return { min(a.x, b.x), min(a.y, b.y) }; }
    inline vec2i max(vec2i a, vec2i b) { return { max(a.x, b.x), max(a.y, b.y) }; }


    template <typename T, size_t size>
    inline auto min(T const (&vals)[size]) requires (requires (T t) { min(t, t); } && size > 0)
    {
        auto res = vals[0];

        for (size_t i = 1; i < size; ++i)
        {
            res = min(res, vals[i]);
        }

        return res;
    }

    template <typename T, size_t size>
    inline auto max(T const (&vals)[size]) requires (requires (T t) { max(t, t); } && size > 0)
    {
        auto res = vals[0];

        for (size_t i = 1; i < size; ++i)
        {
            res = max(res, vals[i]);
        }

        return res;
    }

    inline number auto abs(number auto val)
    {
        return val < 0 ? -val : val;
    }

    inline f32 lerp(f32 a, f32 b, f32 t)
    {
        return a + t * (b - a);
    }
}

f32 vec2f::len() const{ return math::sqrt(math::abs(x * x + y * y)); }
f32 vec3f::len() const{ return math::sqrt(math::abs(x * x + y * y + z * z)); }
i32 vec2i::len() const{ return math::sqrt(math::abs(x * x + y * y)); }

inline vec2f operator+(vec2f a, vec2f b) { return { a.x + b.x, a.y + b.y }; }
inline vec2f operator-(vec2f a, vec2f b) { return { a.x - b.x, a.y - b.y }; }
inline vec2f operator*(vec2f v,   f32 s) { return { v.x * s, v.y * s }; }
inline vec2f operator/(vec2f v,   f32 s) { return { v.x / s, v.y / s }; }

inline f32 distance(vec2f a, vec2f b) { return (b - a).len(); }

inline vec2i operator+(vec2i a, vec2i b) { return { a.x + b.x, a.y + b.y }; }
inline vec2i operator-(vec2i a, vec2i b) { return { a.x - b.x, a.y - b.y }; }
inline vec2i operator*(vec2i v, vec2i b) { return { v.x * b.x, v.y * b.y }; }
inline vec2i operator*(vec2i v,   i32 s) { return { v.x * s, v.y * s }; }
inline vec2i operator/(vec2i v,   i32 s) { return { v.x / s, v.y / s }; }
inline vec2i operator+(vec2i v,   i32 s) { return { v.x + s, v.y + s }; }
inline vec2i operator&(vec2i v,   i32 s) { return { v.x & s, v.y & s }; }

inline i32 distance(vec2i a, vec2i b) { return (b - a).len(); }

struct mat2f
{ 
    vec2f x, y; 
    inline vec2f &operator[](i32 i)
    {
        switch (i) {
            case 0: return x;
            case 1: return y;
            default: throw "bad index!";
        }
    }

    inline vec2f operator[](i32 i) const
    {
        switch (i) {
            case 0: return x;
            case 1: return y;
            default: throw "bad index!";
        }
    }

    inline vec2f operator*(vec2f v) const
    {
        return vec2f{
            v.x*x.x+v.y*x.y,
            v.x*x.y+v.y*x.x,
        };
    }
};

inline vec2f operator /(vec2f a, vec2f b)
{
    return vec2f{
        a.x / b.x,
        a.y / b.y,
    };
}

#endif /* MATH */
