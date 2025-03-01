// 2017-2019 Rotten Visions, LLC. https://www.rottenvisions.com

/*
*/
#ifndef OURO_MEMORY_MALLOC_H
#define OURO_MEMORY_MALLOC_H

#ifdef USE_JEMALLOC
#include "jemalloc/jemalloc.h"
#endif

//#define USE_OURO_MALLOC				
//#define OURO_MALLOC_ALIGN			16

#ifdef USE_OURO_MALLOC
#include "nedmalloc/nedmalloc.h"

#if OURO_PLATFORM == PLATFORM_WIN32
#ifdef _DEBUG
#pragma comment (lib, "nedmalloc_d.lib")
#else
#pragma comment (lib, "nedmalloc.lib")
#endif
#endif

inline void* operator new(size_t size)
{
#if OURO_MALLOC_ALIGN == 0
    return nedalloc::nedpmalloc(0, size);
#else
    return nedalloc::nedpmemalign(0, OURO_MALLOC_ALIGN, size);
#endif
}

inline void* operator new[](size_t size)
{
#if OURO_MALLOC_ALIGN == 0
    return nedalloc::nedpmalloc(0, size);
#else
    return nedalloc::nedpmemalign(0, OURO_MALLOC_ALIGN, size);
#endif
}

inline void  operator delete(void* p)
{
    nedalloc::nedpfree(0, p);
}

inline void  operator delete[](void* p)
{
    nedalloc::nedpfree(0, p);
}

#endif
#endif
