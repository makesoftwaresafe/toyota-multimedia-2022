/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "BlockAllocator.h"

#include "CopiedBlock.h"
#include "CopyWorkList.h"
#include "MarkedBlock.h"
#include "WeakBlock.h"
#include <wtf/CurrentTime.h>

namespace JSC {

inline ThreadIdentifier createBlockFreeingThread(BlockAllocator* allocator)
{
    if (!GCActivityCallback::s_shouldCreateGCTimer)
        return 0; // No block freeing thread.
    ThreadIdentifier identifier = createThread(allocator->blockFreeingThreadStartFunc, allocator, "JavaScriptCore::BlockFree");
    RELEASE_ASSERT(identifier);
    return identifier;
}

/*  add : begin */
double BlockAllocator::currentTime_MONOTONIC()
{
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return static_cast<double>(now.tv_sec) + static_cast<double>(now.tv_nsec / 1000000.0);
}

void BlockAllocator::conditionVariable_init()
{
	pthread_mutex_init(&m_pEmptyRegionConditionMutex, NULL);

	//pthread_cond_init(&m_pEmptyRegionCondition, NULL);
	pthread_condattr_t condattr;
    pthread_condattr_init(&condattr);
    pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
    pthread_cond_init(&m_pEmptyRegionCondition, &condattr);
    pthread_condattr_destroy(&condattr);
}

void BlockAllocator::conditionVariable_finalize()
{
	pthread_cond_destroy(&m_pEmptyRegionCondition);
	pthread_mutex_destroy(&m_pEmptyRegionConditionMutex);
}

void BlockAllocator::conditionVariable_wait()
{
	pthread_cond_wait(&m_pEmptyRegionCondition, &m_pEmptyRegionConditionMutex);
}

void BlockAllocator::conditionVariable_notifyOne()
{
	pthread_cond_signal(&m_pEmptyRegionCondition);
}

void BlockAllocator::conditionVariable_notifyAll()
{
    pthread_cond_broadcast(&m_pEmptyRegionCondition);
}

void BlockAllocator::conditionVariable_waitFor(std::chrono::milliseconds duration)
{
	double currentTime = currentTime_MONOTONIC();
	double durationTime = duration.count() / 1000;
	double absoluteTime = currentTime + durationTime;

	int timeSeconds = static_cast<int>(absoluteTime);
    int timeNanoseconds = static_cast<int>((absoluteTime - timeSeconds) * 1E9);

    timespec targetTime;
    targetTime.tv_sec = timeSeconds;
    targetTime.tv_nsec = timeNanoseconds;

	pthread_cond_timedwait(&m_pEmptyRegionCondition, &m_pEmptyRegionConditionMutex, &targetTime);
}
/*  add : end */

BlockAllocator::BlockAllocator()
    : m_superRegion()
    , m_copiedRegionSet(CopiedBlock::blockSize)
    , m_markedRegionSet(MarkedBlock::blockSize)
    , m_fourKBBlockRegionSet(WeakBlock::blockSize)
    , m_workListRegionSet(CopyWorkListSegment::blockSize)
    , m_numberOfEmptyRegions(0)
    , m_isCurrentlyAllocating(false)
    , m_blockFreeingThreadShouldQuit(false)
    //, m_blockFreeingThread(createBlockFreeingThread(this))
{
    m_regionLock.Init();
	conditionVariable_init();
	m_blockFreeingThread = createBlockFreeingThread(this);
}

BlockAllocator::~BlockAllocator()
{
    releaseFreeRegions();
    {
        //std::lock_guard<std::mutex> lock(m_emptyRegionConditionMutex);
		pthread_mutex_lock(&m_pEmptyRegionConditionMutex);
        m_blockFreeingThreadShouldQuit = true;
        //m_emptyRegionCondition.notify_all();
		conditionVariable_notifyAll();
		pthread_mutex_unlock(&m_pEmptyRegionConditionMutex);
    }
    if (m_blockFreeingThread)
        waitForThreadCompletion(m_blockFreeingThread);
    ASSERT(allRegionSetsAreEmpty());
    ASSERT(m_emptyRegions.isEmpty());
	conditionVariable_finalize();
}

bool BlockAllocator::allRegionSetsAreEmpty() const
{
    return m_copiedRegionSet.isEmpty()
        && m_markedRegionSet.isEmpty()
        && m_fourKBBlockRegionSet.isEmpty()
        && m_workListRegionSet.isEmpty();
}

void BlockAllocator::releaseFreeRegions()
{
    while (true) {
        Region* region;
        {
            SpinLockHolder locker(&m_regionLock);
            if (!m_numberOfEmptyRegions)
                region = 0;
            else {
                region = m_emptyRegions.removeHead();
                RELEASE_ASSERT(region);
                m_numberOfEmptyRegions--;
            }
        }
        
        if (!region)
            break;

        region->destroy();
    }
}

void BlockAllocator::waitForDuration(std::chrono::milliseconds duration)
{
    //std::unique_lock<std::mutex> lock(m_emptyRegionConditionMutex);
	pthread_mutex_lock(&m_pEmptyRegionConditionMutex);

    // If this returns early, that's fine, so long as it doesn't do it too
    // frequently. It would only be a bug if this function failed to return
    // when it was asked to do so.
    if (m_blockFreeingThreadShouldQuit)
        return;

    //m_emptyRegionCondition.wait_for(lock, duration);
	conditionVariable_waitFor(duration);
	pthread_mutex_unlock(&m_pEmptyRegionConditionMutex);
}

void BlockAllocator::blockFreeingThreadStartFunc(void* blockAllocator)
{
    static_cast<BlockAllocator*>(blockAllocator)->blockFreeingThreadMain();
}

void BlockAllocator::blockFreeingThreadMain()
{
    size_t currentNumberOfEmptyRegions;
    while (!m_blockFreeingThreadShouldQuit) {
        // Generally wait for one second before scavenging free blocks. This
        // may return early, particularly when we're being asked to quit.
        waitForDuration(std::chrono::seconds(1));
        if (m_blockFreeingThreadShouldQuit)
            break;
        
        if (m_isCurrentlyAllocating) {
            m_isCurrentlyAllocating = false;
            continue;
        }

        // Sleep until there is actually work to do rather than waking up every second to check.
        {
            //std::unique_lock<std::mutex> lock(m_emptyRegionConditionMutex);
			pthread_mutex_lock(&m_pEmptyRegionConditionMutex);
            SpinLockHolder regionLocker(&m_regionLock);
            while (!m_numberOfEmptyRegions && !m_blockFreeingThreadShouldQuit) {
                m_regionLock.Unlock();
                //m_emptyRegionCondition.wait(lock);
				conditionVariable_wait();
                m_regionLock.Lock();
            }
            currentNumberOfEmptyRegions = m_numberOfEmptyRegions;
			pthread_mutex_unlock(&m_pEmptyRegionConditionMutex);
        }
        
        size_t desiredNumberOfEmptyRegions = currentNumberOfEmptyRegions / 2;
        
        while (!m_blockFreeingThreadShouldQuit) {
            Region* region;
            {
                SpinLockHolder locker(&m_regionLock);
                if (m_numberOfEmptyRegions <= desiredNumberOfEmptyRegions)
                    region = 0;
                else {
                    region = m_emptyRegions.removeHead();
                    RELEASE_ASSERT(region);
                    m_numberOfEmptyRegions--;
                }
            }
            
            if (!region)
                break;
            
            region->destroy();
        }
    }
}

} // namespace JSC
