/*
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "StackBounds.h"

/**/
// 
const size_t StackBounds::s_defaultAvailabilityDelta = 512 * 1024;
/**/

#if OS(DARWIN)

#include <mach/task.h>
#include <mach/thread_act.h>
#include <pthread.h>

#elif OS(WINDOWS)

#include <windows.h>

#elif OS(SOLARIS)

#include <thread.h>

#elif OS(UNIX)

#include <pthread.h>
#if HAVE(PTHREAD_NP_H)
#include <pthread_np.h>
#endif

#endif

namespace WTF {

// Bug 26276 - Need a mechanism to determine stack extent
//
// These platforms should now be working correctly:
//     DARWIN, OPENBSD, QNX, SOLARIS, UNIX
// These platforms are not:
//     WINDOWS, WINCE
//
// FIXME: remove this! - this code unsafely guesses at stack sizes!
#if OS(WINCE)
// Based on the current limit used by the JSC parser, guess the stack size.
static const ptrdiff_t estimatedStackSize = 128 * sizeof(void*) * 1024;
// This method assumes the stack is growing downwards.
static void* estimateStackBound(void* origin)
{
    return static_cast<char*>(origin) - estimatedStackSize;
}
#endif
#if OS(DARWIN)

void StackBounds::initialize()
{
    pthread_t thread = pthread_self();
    m_origin = pthread_get_stackaddr_np(thread);
    rlim_t size = 0;
    if (pthread_main_np()) {
        // FIXME: <rdar://problem/13741204>
        // pthread_get_size lies to us when we're the main thread, use get_rlimit instead
        rlimit limit;
        getrlimit(RLIMIT_STACK, &limit);
        size = limit.rlim_cur;
    } else
        size = pthread_get_stacksize_np(thread);

    m_bound = static_cast<char*>(m_origin) - size;
}

#elif OS(SOLARIS)

void StackBounds::initialize()
{
    stack_t s;
    thr_stksegment(&s);
    m_origin = s.ss_sp;
    m_bound = static_cast<char*>(m_origin) - s.ss_size;
}

#elif OS(OPENBSD)

void StackBounds::initialize()
{
    pthread_t thread = pthread_self();
    stack_t stack;
    pthread_stackseg_np(thread, &stack);
    m_origin = stack.ss_sp;
#if CPU(HPPA)
    m_bound = static_cast<char*>(m_origin) + stack.ss_size;
#else
    m_bound = static_cast<char*>(m_origin) - stack.ss_size;
#endif
}

#elif OS(UNIX)

void StackBounds::initialize()
{
    void* stackBase = 0;
    size_t stackSize = 0;

    pthread_t thread = pthread_self();
    pthread_attr_t sattr;
    pthread_attr_init(&sattr);
#if HAVE(PTHREAD_NP_H) || OS(NETBSD)
    // e.g. on FreeBSD 5.4, neundorf@kde.org
    pthread_attr_get_np(thread, &sattr);
#else
    // FIXME: this function is non-portable; other POSIX systems may have different np alternatives
    pthread_getattr_np(thread, &sattr);
#endif
    int rc = pthread_attr_getstack(&sattr, &stackBase, &stackSize);
    (void)rc; // FIXME: Deal with error code somehow? Seems fatal.
    ASSERT(stackBase);
    pthread_attr_destroy(&sattr);
    m_bound = stackBase;
    m_origin = static_cast<char*>(stackBase) + stackSize;
}
/**/
#elif OS(WINCE)
static bool detectGrowingDownward(void* previousFrame)
{
    // Find the address of this stack frame by taking the address of a local variable.
    int thisFrame;
    return previousFrame > &thisFrame;
}

static inline bool isPageWritable(void* page)
{
    MEMORY_BASIC_INFORMATION memoryInformation;
    DWORD result = VirtualQuery(page, &memoryInformation, sizeof(memoryInformation));

    // return false on error, including ptr outside memory
    if (result != sizeof(memoryInformation))
        return false;

    DWORD protect = memoryInformation.Protect & ~(PAGE_GUARD | PAGE_NOCACHE);
    return protect == PAGE_READWRITE
        || protect == PAGE_WRITECOPY
        || protect == PAGE_EXECUTE_READWRITE
        || protect == PAGE_EXECUTE_WRITECOPY;
}

static inline void* getLowerStackBound(char* currentPage, DWORD pageSize)
{
    while (currentPage > 0) {
        // check for underflow
        if (currentPage >= reinterpret_cast<char*>(pageSize))
            currentPage -= pageSize;
        else
            currentPage = 0;

        if (!isPageWritable(currentPage))
            return currentPage + pageSize;
    }

    return 0;
}

static inline void* getUpperStackBound(char* currentPage, DWORD pageSize)
{
    do {
        // guaranteed to complete because isPageWritable returns false at end of memory
        currentPage += pageSize;
    } while (isPageWritable(currentPage));

    return currentPage - pageSize;
}

void StackBounds::initialize()
{
    // find the address of this stack frame by taking the address of a local variable
    void* thisFrame = &thisFrame;
    bool isGrowingDownward = detectGrowingDownward(thisFrame);

    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    DWORD pageSize = systemInfo.dwPageSize;

    // scan all of memory starting from this frame, and return the last writeable page found
    char* currentPage = reinterpret_cast<char*>(reinterpret_cast<DWORD>(thisFrame) & ~(pageSize - 1));
    void* lowerStackBound = getLowerStackBound(currentPage, pageSize);
    void* upperStackBound = getUpperStackBound(currentPage, pageSize);

    m_origin = isGrowingDownward ? upperStackBound : lowerStackBound;
   // m_bound = isGrowingDownward ? lowerStackBound : upperStackBound;
	m_bound = estimateStackBound(m_origin);
}
/**/

#elif OS(WINDOWS)

void StackBounds::initialize()
{
    MEMORY_BASIC_INFORMATION stackOrigin;
    VirtualQuery(&stackOrigin, &stackOrigin, sizeof(stackOrigin));
    // stackOrigin.AllocationBase points to the reserved stack memory base address.

    m_origin = static_cast<char*>(stackOrigin.BaseAddress) + stackOrigin.RegionSize;
#if OS(WINCE)
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    DWORD pageSize = systemInfo.dwPageSize;

    MEMORY_BASIC_INFORMATION stackMemory;
    VirtualQuery(m_origin, &stackMemory, sizeof(stackMemory));

    m_bound = static_cast<char*>(m_origin) - stackMemory.RegionSize + pageSize;
#else
    // The stack on Windows consists out of three parts (uncommitted memory, a guard page and present
    // committed memory). The 3 regions have different BaseAddresses but all have the same AllocationBase
    // since they are all from the same VirtualAlloc. The 3 regions are laid out in memory (from high to
    // low) as follows:
    //
    //    High |-------------------|  -----
    //         | committedMemory   |    ^
    //         |-------------------|    |
    //         | guardPage         | reserved memory for the stack
    //         |-------------------|    |
    //         | uncommittedMemory |    v
    //    Low  |-------------------|  ----- <--- stackOrigin.AllocationBase
    //
    // See http://msdn.microsoft.com/en-us/library/ms686774%28VS.85%29.aspx for more information.

    MEMORY_BASIC_INFORMATION uncommittedMemory;
    VirtualQuery(stackOrigin.AllocationBase, &uncommittedMemory, sizeof(uncommittedMemory));
    ASSERT(uncommittedMemory.State == MEM_RESERVE);

    MEMORY_BASIC_INFORMATION guardPage;
    VirtualQuery(static_cast<char*>(uncommittedMemory.BaseAddress) + uncommittedMemory.RegionSize, &guardPage, sizeof(guardPage));
    ASSERT(guardPage.Protect & PAGE_GUARD);

    void* endOfStack = stackOrigin.AllocationBase;

#ifndef NDEBUG
    MEMORY_BASIC_INFORMATION committedMemory;
    VirtualQuery(static_cast<char*>(guardPage.BaseAddress) + guardPage.RegionSize, &committedMemory, sizeof(committedMemory));
    ASSERT(committedMemory.State == MEM_COMMIT);

    void* computedEnd = static_cast<char*>(m_origin) - (uncommittedMemory.RegionSize + guardPage.RegionSize + committedMemory.RegionSize);

    ASSERT(stackOrigin.AllocationBase == uncommittedMemory.AllocationBase);
    ASSERT(stackOrigin.AllocationBase == guardPage.AllocationBase);
    ASSERT(stackOrigin.AllocationBase == committedMemory.AllocationBase);
    ASSERT(stackOrigin.AllocationBase == uncommittedMemory.BaseAddress);
    ASSERT(endOfStack == computedEnd);
#endif // NDEBUG
    m_bound = static_cast<char*>(endOfStack) + guardPage.RegionSize;
#endif // OS(WINCE)
}

#else
#error Need a way to get the stack bounds on this platform
#endif

} // namespace WTF
