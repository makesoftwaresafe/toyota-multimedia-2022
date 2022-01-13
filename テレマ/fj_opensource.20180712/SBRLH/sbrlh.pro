TARGET = qsbrlh

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = FhiLhIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

target{
DEFINES += MFTLH_PLATFORM_TARGET
#DEFINES += MFTLHDNFONT
DEFINES += _UNUSE_EGL_EXT_
} else {
#    DEFINES += MFTLHDEBUGTRACE
#    DEFINES += MFTLHFONTDEBUGTRACE
	DEFINES += MFTLH_PLATFORM_UBUNTU
	DESTDIR = /home/ivilinux/Browser/fbrws_customapp0_pc/qt/plugins/platforms

	QMAKE_CXXFLAGS+=-coverage
	QMAKE_CFLAGS+=-coverage
	QMAKE_LFLAGS+=-coverage
	QMAKE_CXXFLAGS+=-O0
	QMAKE_CXXFLAGS+=-fno-exceptions 
}

QT += core-private gui-private platformsupport-private network

CONFIG += egl qpa/genericunixfontdatabase

INCLUDEPATH += ./ \
		${CUSTOM_SYSROOT2}/usr/include/ilm

SOURCES =   fhilhmain.cpp \
            fhilhintegration.cpp \
            fhilhbackingstore.cpp \
            mftlhopengl.c \
            fhilheventhander.cpp \
            fhilhmainwindownotifier.cpp \
            fhilhscreen.cpp
HEADERS =   fhilhintegration.h \
            fhilhbackingstore.h \
            mftlhopengl.h \
            fhilhevnethandler.h \
            fhilhmainwindownotifier.h \
            fhilhscreen.h
target {
	LIBS += -L/home/ivilinux/root_env/x86a/lib
	LIBS += -lm -livi-application
	LIBS += -lEGL -lGLESv2
	LIBS += -lwayland-client -lwayland-egl
	LIBS += -lrt
} else {
	LIBS += -lEGL -lGLESv2
	LIBS += -lwayland-client -lwayland-egl
	LIBS += -lrt
}

OTHER_FILES += fhilh.json
