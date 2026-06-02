#include "config.h"
#include "utils/crc32.h"

#include <iostream>

using namespace std;

void __print_intrin()
{
#ifdef USE_X64
    cout << "x64" << endl;
#endif

#ifdef USE_AVX2
    cout << "AVX2" << endl;
#endif

#ifdef USE_AVX
    cout << "AVX" << endl;
#endif

#ifdef USE_SSE42
    cout << "SSE42" << endl;
#endif

#ifdef USE_SSE41
    cout << "SSE41" << endl;
#endif

#ifdef USE_SSSE3
    cout << "SSSE3" << endl;
#endif

#ifdef USE_SSE2
    cout << "SSE2" << endl;
#endif
}