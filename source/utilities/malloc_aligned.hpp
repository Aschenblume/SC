//  functions for aligned memory allocation
//  Copyright (C) 2009 Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#ifndef UTILITIES_MALLOC_ALIGNED_HPP
#define UTILITIES_MALLOC_ALIGNED_HPP

#include <cstdlib>
#include <cstring>

#ifdef HAVE_TBB
#include <tbb/cache_aligned_allocator.h>
#endif /* HAVE_TBB */

namespace nova
{

#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
/* we have posix_memalign */

/* memory alignment constraints:
 *
 * - 16 byte for SSE operations
 * - the cache lines size of modern x86 cpus is 64 bytes (pentium-m, pentium 4, core, k8)
 */
const int malloc_memory_alignment = 64;

inline void* malloc_aligned(std::size_t nbytes)
{
    void * ret;
    int status = posix_memalign(&ret, malloc_memory_alignment, nbytes);
    if (!status)
        return ret;
    else
        return 0;
}

inline void free_aligned(void *ptr)
{
    free(ptr);
}

#elif defined(HAVE_TBB)

inline void* malloc_aligned(std::size_t nbytes)
{
    tbb::cache_aligned_allocator<void*> ca_alloc;
    return static_cast<void*>(ca_alloc.allocate(nbytes));
}

inline void free_aligned(void *ptr)
{
    tbb::cache_aligned_allocator<void*> ca_alloc;
    ca_alloc.deallocate(static_cast<void**>(ptr), 0);
}

#else

/* on other systems, we use the aligned memory allocation taken
 * from thomas grill's implementation for pd */
#define VECTORALIGNMENT 128
inline void* malloc_aligned(std::size_t nbytes)
{
    void* vec = malloc(nbytes+ (VECTORALIGNMENT/8-1) + sizeof(void *));

    if (vec != NULL)
    {
        /* get alignment of first possible signal vector byte */
        long alignment = ((long)vec+sizeof(void *))&(VECTORALIGNMENT/8-1);
        /* calculate aligned pointer */
        void *ret = (unsigned char *)vec+sizeof(void *)+(alignment == 0?0:VECTORALIGNMENT/8-alignment);
        /* save original memory location */
        *(void **)((unsigned char *)ret-sizeof(void *)) = vec;
        return ret;
    }
    else
        return 0;
}

inline void free_aligned(void *ptr)
{
    /* get original memory location */
    void *ori = *(void **)((unsigned char *)ptr-sizeof(void *));
    free(ori);
}

#undef VECTORALIGNMENT

#endif

inline void * calloc_aligned(std::size_t nbytes)
{
    void * ret = malloc_aligned(nbytes);
    if (ret)
        std::memset(ret, 0, nbytes);
    return ret;
}

template <typename T>
T* malloc_aligned(std::size_t n)
{
    return static_cast<T*>(malloc_aligned(n * sizeof(T)));
}

template <typename T>
T* calloc_aligned(std::size_t n)
{
    return static_cast<T*>(calloc_aligned(n * sizeof(T)));
}

} /* namespace nova */

#endif /* UTILITIES_MALLOC_ALIGNED_HPP */
