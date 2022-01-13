# -------------------------------------------------------------------
# This file contains shared rules used both when building ANGLE
# itself, and by targets that use ANGLE.
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

SOURCE_DIR = $${ROOT_WEBKIT_DIR}/Source/ThirdParty/ANGLE

*clang: QT_CONFIG -= c++11

INCLUDEPATH += \
    $$SOURCE_DIR/include/GLSLANG \
    $$SOURCE_DIR/include/KHR

win32-* { 
    QMAKE_POST_LINK = copy /y $$toSystemPath($${ROOT_BUILD_DIR}/Source/ThirdParty/ANGLE/release/*.lib) $$toSystemPath($${ROOT_BUILD_DIR}/lib) 
} 