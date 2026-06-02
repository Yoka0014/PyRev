#pragma once

#if defined(_M_X64) | defined(__x86_64) | defined(__x86_64__)
#define USE_X64
#endif

#ifdef __AVX2__
#define USE_AVX2
#elif defined(__AVX__)
#define USE_AVX
#elif defined(__SSE4_2__)
#define USE_SSE42
#elif defined(__SSE4_1__)
#define USE_SSE41
#elif defined(__SSSE3__)
#define USE_SSSE3
#elif defined(__SSE2__)
#define USE_SSE2
#endif


// 使用可能な命令セットよりも古い命令セットを全てdefine
#ifdef USE_AVX2
#define USE_AVX
#endif

#ifdef USE_AVX
#define USE_SSE42
#endif

#ifdef USE_SSE42
#define USE_SSE41
#endif

#ifdef USE_SSE41
#define USE_SSSE3
#endif

#ifdef USE_SSSE3
#define USE_SSE2
#endif

void __print_intrin();

// 強制インライン化に関するマクロ
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define FORCE_INLINE __forceinline
#elif defined(__INTEL_COMPILER)
#define FORCE_INLINE inline
#elif defined(__GNUC__)
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

