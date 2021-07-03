#ifndef WASMDEFS
#define WASMDEFS

using size_t = __SIZE_TYPE__;
using uintptr_t = __UINTPTR_TYPE__;
using uint8_t = __UINT8_TYPE__;

using u64 = unsigned long long;
using i64 = long long;
using u32 = unsigned int;
using i32 = int;
using f32 = float;
using f64 = double;

static void* memset(void *dst, i32 val, u64 size)
{
#if __has_builtin(__builtin_memset)
    return __builtin_memset(dst, val, size);
#else
#error "__builtin_memset not supported!"
#endif
}

static void* memcpy(void *dst, void const *src, u64 size)
{
#if __has_builtin(__builtin_memcpy)
    return __builtin_memcpy(dst, src, size);
#else
#error "__builtin_memcpy not supported!"
#endif
}

#endif /* WASMDEFS */
