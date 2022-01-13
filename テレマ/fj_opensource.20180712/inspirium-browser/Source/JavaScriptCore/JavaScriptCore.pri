# -------------------------------------------------------------------
# This file contains shared rules used both when building
# JavaScriptCore itself, and by targets that use JavaScriptCore.
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

SOURCE_DIR = $${ROOT_WEBKIT_DIR}/Source/JavaScriptCore

JAVASCRIPTCORE_GENERATED_SOURCES_DIR = $${ROOT_BUILD_DIR}/Source/JavaScriptCore/$${GENERATED_SOURCES_DESTDIR}

INCLUDEPATH += \
    $$SOURCE_DIR \
    $$SOURCE_DIR/.. \
    $$SOURCE_DIR/../WTF \
    $$SOURCE_DIR/assembler \
    $$SOURCE_DIR/bindings \
    $$SOURCE_DIR/bytecode \
    $$SOURCE_DIR/bytecompiler \
    $$SOURCE_DIR/heap \
    $$SOURCE_DIR/dfg \
    $$SOURCE_DIR/debugger \
    $$SOURCE_DIR/disassembler \
    $$SOURCE_DIR/ftl \
    $$SOURCE_DIR/inspector \
    $$SOURCE_DIR/interpreter \
    $$SOURCE_DIR/jit \
    $$SOURCE_DIR/llint \
    $$SOURCE_DIR/parser \
    $$SOURCE_DIR/profiler \
    $$SOURCE_DIR/runtime \
    $$SOURCE_DIR/tools \
    $$SOURCE_DIR/yarr \
    $$SOURCE_DIR/API \
    $$SOURCE_DIR/ForwardingHeaders \
    $$JAVASCRIPTCORE_GENERATED_SOURCES_DIR

# Pick up the right version of LLIntAssembly.h
macx: INCLUDEPATH += $$JAVASCRIPTCORE_GENERATED_SOURCES_DIR/$$targetSubDir()

win32-*: LIBS += -lwinmm

wince* {
    INCLUDEPATH += $$SOURCE_DIR/os-win32
}

*linux-*:if(isEqual(QT_ARCH, "i386")|isEqual(QT_ARCH, "x86_64")) {
    INCLUDEPATH += $$SOURCE_DIR/disassembler/udis86
}

#
# for fjib_log()
INCLUDEPATH += $${ROOT_WEBKIT_DIR}/webkit-thirdparty/InspiriumExtension/include
LIBS += -L$${ROOT_WEBKIT_DIR}/webkit-thirdparty/InspiriumExtension -lInspiriumExtension
#

win32-* { 
    QMAKE_POST_LINK = copy /y $$toSystemPath($${ROOT_BUILD_DIR}/Source/JavaScriptCore/release/*.lib) $$toSystemPath($${ROOT_BUILD_DIR}/lib) 
}
