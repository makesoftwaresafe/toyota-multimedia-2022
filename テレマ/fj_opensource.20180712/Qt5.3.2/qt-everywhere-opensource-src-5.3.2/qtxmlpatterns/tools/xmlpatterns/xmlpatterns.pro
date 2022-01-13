QT = core xmlpatterns-private

# Note that qcoloroutput.cpp and qcoloringmessagehandler.cpp are also used internally
# in libQtXmlPatterns. See src/xmlpatterns/api/api.pri.
SOURCES = main.cpp                          \
          qapplicationargument.cpp          \
          qapplicationargumentparser.cpp


HEADERS = main.h                            \
          qapplicationargument.cpp          \
          qapplicationargumentparser.cpp

load(qt_tool)
LIBS+=-Wl,-rpath,${CUSTOM_SYSROOT2}/additional/lib
# with c++11 / __STRICT_ANSI__ mingw.org stdio.h doesn't declare e.g. _fileno
win32-g++*: QMAKE_CXXFLAGS_CXX11 = -std=gnu++0x
