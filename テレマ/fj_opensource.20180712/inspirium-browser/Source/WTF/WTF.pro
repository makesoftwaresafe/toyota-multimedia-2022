# -------------------------------------------------------------------
# Project file for WTF
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------
TEMPLATE = lib
TARGET = WTF

include(WTF.pri)

!win32-* {
    CONFIG += staticlib
}

VPATH += $$PWD/wtf

INCLUDEPATH += $$PWD/wtf

#
# for fjib_log()
INCLUDEPATH += $${ROOT_WEBKIT_DIR}/webkit-thirdparty/InspiriumExtension/include
LIBS += -L$${ROOT_BUILD_DIR}/lib -lInspiriumExtension
#

wince* {
    # for mt19937ar.c
    INCLUDEPATH += $${ROOT_WEBKIT_DIR}/Source/ThirdParty
}

HEADERS += \
    ASCIICType.h \
    Assertions.h \
    Atomics.h \
    AVLTree.h \
    Bag.h \
    BagToHashMap.h \
    Bitmap.h \
    BitVector.h \
    BloomFilter.h \
    BoundsCheckedPointer.h \
    BumpPointerAllocator.h \
    ByteOrder.h \
    CheckedArithmetic.h \
    Compiler.h \
    Compression.h \
    CryptographicUtilities.h \
    CryptographicallyRandomNumber.h \
    CurrentTime.h \
    DateMath.h \
    DecimalNumber.h \
    Decoder.h \
    DataLog.h \ 
    Deque.h \
    DisallowCType.h \
    dtoa.h \
    dtoa/bignum-dtoa.h \
    dtoa/bignum.h \
    dtoa/cached-powers.h \
    dtoa/diy-fp.h \
    dtoa/double-conversion.h \
    dtoa/double.h \
    dtoa/fast-dtoa.h \
    dtoa/fixed-dtoa.h \
    dtoa/strtod.h \
    dtoa/utils.h \
    DynamicAnnotations.h \
    Encoder.h \
    ExportMacros.h \
    FastMalloc.h \
    FeatureDefines.h \
    FilePrintStream.h \
    Forward.h \
    FunctionDispatcher.h \
    Functional.h \
    GetPtr.h \
    GregorianDateTime.h \
    HashCountedSet.h \
    HashFunctions.h \
    HashIterators.h \
    HashMap.h \
    HashSet.h \
    HashTable.h \
    HashTraits.h \
    HexNumber.h \
    IteratorAdaptors.h \
    IteratorRange.h \
    ListHashSet.h \
    Locker.h \
    MainThread.h \
    MallocPtr.h \
    MathExtras.h \
    MD5.h \
    MediaTime.h \
    MessageQueue.h \
    MetaAllocator.h \
    MetaAllocatorHandle.h \
    Ref.h \
    Noncopyable.h \
    NumberOfCores.h \
    RAMSize.h \
    OSAllocator.h \
    OSRandomSource.h \
    OwnPtr.h \
    OwnPtrCommon.h \
    PackedIntVector.h \
    PageAllocation.h \
    PageAllocationAligned.h \
    PageBlock.h \
    PageReservation.h \
    ParallelJobs.h \
    ParallelJobsGeneric.h \
    ParallelJobsLibdispatch.h \
    ParallelJobsOpenMP.h \
    PassOwnPtr.h \
    PassRef.h \
    PassRefPtr.h \
    Platform.h \
    PossiblyNull.h \
    PrintStream.h \
    ProcessID.h \
    RandomNumber.h \
    RandomNumberSeed.h \
    RawPointer.h \
    RedBlackTree.h \
    RefCounted.h \
    RefCountedLeakCounter.h \
    RefPtr.h \
    RefPtrHashMap.h \
    RetainPtr.h \
    RunLoop.h \
    SHA1.h \
    SaturatedArithmetic.h \
    Spectrum.h \
    StackBounds.h \
    StaticConstructors.h \
    StdLibExtras.h \
    StringExtras.h \
    StringHasher.h \
    StringPrintStream.h \
    TCPackedCache.h \
    TCSpinLock.h \
    TCSystemAlloc.h \
    text/ASCIIFastPath.h \
    text/AtomicString.h \
    text/AtomicStringHash.h \
    text/AtomicStringImpl.h \
    text/AtomicStringTable.h \
    text/Base64.h \
    text/CString.h \
    text/IntegerToStringConversion.h \
    text/LChar.h \ 
    text/StringBuffer.h \
    text/StringBuilder.h \
    text/StringConcatenate.h \
    text/StringHash.h \
    text/StringImpl.h \
    text/StringOperators.h \
    text/StringView.h \
    text/TextPosition.h \
    text/WTFString.h \
    threads/BinarySemaphore.h \
    Threading.h \
    ThreadingPrimitives.h \
    ThreadRestrictionVerifier.h \
    ThreadSafeRefCounted.h \
    ThreadSpecific.h \
    unicode/CharacterNames.h \
    unicode/Collator.h \
    unicode/UTF8.h \
    ValueCheck.h \
    Vector.h \
    VectorTraits.h \
    VMTags.h \
    WTFThreadData.h \
    WorkQueue.h \
    WeakPtr.h

unix: HEADERS += ThreadIdentifierDataPthreads.h

SOURCES += \
    Assertions.cpp \
    Atomics.cpp \
    BitVector.cpp \
    CompilationThread.cpp \
    Compression.cpp \
    CryptographicUtilities.cpp \
    CryptographicallyRandomNumber.cpp \
    CurrentTime.cpp \
    DateMath.cpp \
    DataLog.cpp \
    DecimalNumber.cpp \
    dtoa.cpp \
    dtoa/bignum-dtoa.cc \
    dtoa/bignum.cc \
    dtoa/cached-powers.cc \
    dtoa/diy-fp.cc \
    dtoa/double-conversion.cc \
    dtoa/fast-dtoa.cc \
    dtoa/fixed-dtoa.cc \
    dtoa/strtod.cc \
    FastBitVector.cpp \
    FastMalloc.cpp \
    FilePrintStream.cpp \
    FunctionDispatcher.cpp \
    GregorianDateTime.cpp \
    gobject/GOwnPtr.cpp \
    gobject/GRefPtr.cpp \
    HashTable.cpp \
    MD5.cpp \
    MainThread.cpp \
    MediaTime.cpp \
    MetaAllocator.cpp \
    NumberOfCores.cpp \
    RAMSize.cpp \
    OSRandomSource.cpp \
    qt/MainThreadQt.cpp \
    qt/RunLoopQt.cpp \
    qt/StringQt.cpp \
    qt/WorkQueueQt.cpp \
    PageAllocationAligned.cpp \
    PageBlock.cpp \
    ParallelJobsGeneric.cpp \
    PrintStream.cpp \
    RandomNumber.cpp \
    RefCountedLeakCounter.cpp \
    RunLoop.cpp \
    SHA1.cpp \
    SixCharacterHash.cpp \
    StackBounds.cpp \
    StringPrintStream.cpp \
    TCSystemAlloc.cpp \
    Threading.cpp \
    WTFThreadData.cpp \
    WorkQueue.cpp \
    text/AtomicString.cpp \
    text/AtomicStringTable.cpp \
    text/Base64.cpp \
    text/CString.cpp \
    text/StringBuilder.cpp \
    text/StringImpl.cpp \
    text/StringStatics.cpp \
    text/WTFString.cpp \
    unicode/CollatorDefault.cpp \
    unicode/icu/CollatorICU.cpp \
    unicode/UTF8.cpp

unix: SOURCES += \
    OSAllocatorPosix.cpp \
    ThreadIdentifierDataPthreads.cpp \
    ThreadingPthreads.cpp

win*|wince*: SOURCES += \
    OSAllocatorWin.cpp \
    ThreadSpecificWin.cpp \
    ThreadingWin.cpp

win32 {
    SOURCES += \
        threads/win/BinarySemaphoreWin.cpp
    INCLUDEPATH += $$PWD/wtf/threads
} else {
    SOURCES += \
        threads/BinarySemaphore.cpp
}

use?(v8) {
    SOURCES += \
        v8/ArrayBuffer.cpp \
        v8/ArrayBufferView.cpp
}

QT += core
QT -= gui

*-g++*:QMAKE_CXXFLAGS_RELEASE -= -O2
*-g++*:QMAKE_CXXFLAGS_RELEASE += -O3

*sh4* {
    QMAKE_CXXFLAGS += -mieee -w
    QMAKE_CFLAGS   += -mieee -w
}

#*-g++*:lessThan(QT_GCC_MAJOR_VERSION, 5):lessThan(QT_GCC_MINOR_VERSION, 6) {
#    # For GCC 4.5 and before we disable C++0x mode in JSC for if enabled in Qt's mkspec
#    QMAKE_CXXFLAGS -= -std=c++0x -std=gnu++0x -std=c++11 -std=gnu++11
#}

win32 { 
    !exists($$toSystemPath($${ROOT_BUILD_DIR}/lib)) {
        QMAKE_POST_LINK = mkdir -p $$toSystemPath($${ROOT_BUILD_DIR}/lib) && copy /y $$toSystemPath($${ROOT_BUILD_DIR}/Source/WTF/release/*.lib) $$toSystemPath($${ROOT_BUILD_DIR}/lib) 
    } else {
        QMAKE_POST_LINK = copy /y $$toSystemPath($${ROOT_BUILD_DIR}/Source/WTF/release/*.lib) $$toSystemPath($${ROOT_BUILD_DIR}/lib) 
    }
}