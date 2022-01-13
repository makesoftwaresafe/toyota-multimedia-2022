# -------------------------------------------------------------------
# Target file for the WebKit1 static library
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = lib
TARGET = WebKit1

include(WebKit1.pri)

use?(v8) {
WEBKIT += wtf webcore
} else {
WEBKIT += wtf javascriptcore webcore
}
QT += gui

# This is relied upon by our export macros and seems not to be properly
# defined by the logic in qt_module.prf as it should
DEFINES += QT_BUILD_WEBKIT_LIB

!win32-* {
    CONFIG += staticlib
}

SOURCES += \
    $$PWD/qt/Api/qhttpheader.cpp \
    $$PWD/qt/Api/qwebdatabase.cpp \
    $$PWD/qt/Api/qwebelement.cpp \
    $$PWD/qt/Api/qwebhistory.cpp \
    $$PWD/qt/Api/qwebhistoryinterface.cpp \
    $$PWD/qt/Api/qwebappcachedb.cpp \
    $$PWD/qt/Api/qwebicondb.cpp \
    $$PWD/qt/Api/qweblocalstoragedb.cpp \
    $$PWD/qt/Api/qwebkitglobal.cpp \
    $$PWD/qt/Api/qwebplugindatabase.cpp \
    $$PWD/qt/Api/qwebpluginfactory.cpp \
    $$PWD/qt/Api/qwebsecurityorigin.cpp \
    $$PWD/qt/Api/qwebsettings.cpp \
    $$PWD/qt/Api/qwebscriptworld.cpp \
    $$PWD/qt/WebCoreSupport/ChromeClientQt.cpp \
    $$PWD/qt/WebCoreSupport/ContextMenuClientQt.cpp \
    $$PWD/qt/WebCoreSupport/DragClientQt.cpp \
#    $$PWD/qt/WebCoreSupport/DumpRenderTreeSupportQt.cpp \
    $$PWD/qt/WebCoreSupport/EditorClientQt.cpp \
    $$PWD/qt/WebCoreSupport/FrameLoaderClientQt.cpp \
    $$PWD/qt/WebCoreSupport/FrameNetworkingContextQt.cpp \
    $$PWD/qt/WebCoreSupport/GeolocationPermissionClientQt.cpp \
    $$PWD/qt/WebCoreSupport/InitWebCoreQt.cpp \
    $$PWD/qt/WebCoreSupport/InspectorClientQt.cpp \
    $$PWD/qt/WebCoreSupport/InspectorServerQt.cpp \
    $$PWD/qt/WebCoreSupport/NotificationPresenterClientQt.cpp \
    $$PWD/qt/WebCoreSupport/PlatformStrategiesQt.cpp \
    $$PWD/qt/WebCoreSupport/ProgressTrackerClientQt.cpp \
    $$PWD/qt/WebCoreSupport/PopupMenuQt.cpp \
    $$PWD/qt/WebCoreSupport/QtPlatformPlugin.cpp \
    $$PWD/qt/WebCoreSupport/QtPluginWidgetAdapter.cpp \
    $$PWD/qt/WebCoreSupport/QtPrintContext.cpp \
    $$PWD/qt/WebCoreSupport/QWebFrameAdapter.cpp \
    $$PWD/qt/WebCoreSupport/QWebPageAdapter.cpp \
    $$PWD/qt/WebCoreSupport/SearchPopupMenuQt.cpp \
    $$PWD/qt/WebCoreSupport/TextCheckerClientQt.cpp \
    $$PWD/qt/WebCoreSupport/TextureMapperLayerClientQt.cpp \
    $$PWD/qt/WebCoreSupport/UndoStepQt.cpp \
    $$PWD/qt/WebCoreSupport/WebEventConversion.cpp

HEADERS += \
    $$PWD/qt/Api/qhttpheader_p.h \
    $$PWD/qt/Api/qwebdatabase.h \
    $$PWD/qt/Api/qwebelement.h \
    $$PWD/qt/Api/qwebelement_p.h \
    $$PWD/qt/Api/qwebhistory.h \
    $$PWD/qt/Api/qwebhistory_p.h \
    $$PWD/qt/Api/qwebhistoryinterface.h \
    $$PWD/qt/Api/qwebappcachedb.h \
    $$PWD/qt/Api/qwebicondb.h \
    $$PWD/qt/Api/qweblocalstoragedb.h \
    $$PWD/qt/Api/qwebplugindatabase_p.h \
    $$PWD/qt/Api/qwebpluginfactory.h \
    $$PWD/qt/Api/qwebsecurityorigin.h \
    $$PWD/qt/Api/qwebsettings.h \
    $$PWD/qt/Api/qwebscriptworld_p.h \
    $$PWD/qt/Api/qwebkitplatformplugin.h \
    $$PWD/qt/WebCoreSupport/ChromeClientQt.h \
    $$PWD/qt/WebCoreSupport/ContextMenuClientQt.h \
    $$PWD/qt/WebCoreSupport/DragClientQt.h \
    $$PWD/qt/WebCoreSupport/EditorClientQt.h \
    $$PWD/qt/WebCoreSupport/FrameLoaderClientQt.h \
    $$PWD/qt/WebCoreSupport/FrameNetworkingContextQt.h \
    $$PWD/qt/WebCoreSupport/GeolocationPermissionClientQt.h \
    $$PWD/qt/WebCoreSupport/InitWebCoreQt.h \
    $$PWD/qt/WebCoreSupport/InspectorClientQt.h \
    $$PWD/qt/WebCoreSupport/InspectorServerQt.h \
    $$PWD/qt/WebCoreSupport/NotificationPresenterClientQt.h \
    $$PWD/qt/WebCoreSupport/PlatformStrategiesQt.h \
    $$PWD/qt/WebCoreSupport/ProgressTrackerClientQt.h \
    $$PWD/qt/WebCoreSupport/PopupMenuQt.h \
    $$PWD/qt/WebCoreSupport/QtPlatformPlugin.h \
    $$PWD/qt/WebCoreSupport/QtPluginWidgetAdapter.h \
    $$PWD/qt/WebCoreSupport/QtPrintContext.h \
    $$PWD/qt/WebCoreSupport/QWebFrameAdapter.h \
    $$PWD/qt/WebCoreSupport/QWebPageAdapter.h \
    $$PWD/qt/WebCoreSupport/SearchPopupMenuQt.h \
    $$PWD/qt/WebCoreSupport/TextCheckerClientQt.h \
    $$PWD/qt/WebCoreSupport/TextureMapperLayerClientQt.h \
    $$PWD/qt/WebCoreSupport/UndoStepQt.h \
    $$PWD/qt/WebCoreSupport/WebEventConversion.h

use?(v8) {
    HEADERS += \
        $$PWD/qt/Api/v8features.h
}


INCLUDEPATH += \
    $$PWD/qt/WebCoreSupport

use?(3D_GRAPHICS): WEBKIT += angle
#
enable?(GEOLOCATION) {
#
     HEADERS += \
#
        $$PWD/qt/WebCoreSupport/GeolocationClientQt.h \
        $$PWD/qt/WebCoreSupport/GeolocationPositionSource.h \
        $$PWD/qt/WebCoreSupport/qgeocoordinate.h \
        $$PWD/qt/WebCoreSupport/qgeopositioninfo.h \
        $$PWD/qt/WebCoreSupport/qmobilityglobal.h \
        $$PWD/qt/WebCoreSupport/qlocationutils_p.h
     SOURCES += \
        $$PWD/qt/WebCoreSupport/GeolocationClientQt.cpp \
        $$PWD/qt/WebCoreSupport/GeolocationPositionSource.cpp \
        $$PWD/qt/WebCoreSupport/qgeocoordinate.cpp \
        $$PWD/qt/WebCoreSupport/qgeopositioninfo.cpp
#
}

enable?(FJIB_VIAPI) {
	HEADERS += \
		$$PWD/qt/WebCoreSupport/VehicleClientQt.h \
		$$PWD/qt/Api/qvehicle.h \
		$$PWD/qt/Api/qtrip.h \
		$$PWD/qt/Api/qvehicleinterface.h \
		$$PWD/qt/Api/qvehiclehistory.h \
		$$PWD/qt/Api/qvehicledata.h \
		$$PWD/qt/Api/qtroublecode.h \
		$$PWD/qt/Api/qvehiclebutton.h \
		$$PWD/qt/Api/qfuelconfiguration.h

	SOURCES += \
		$$PWD/qt/WebCoreSupport/VehicleClientQt.cpp \
		$$PWD/qt/Api/qvehicle.cpp \
		$$PWD/qt/Api/qtrip.cpp \
		$$PWD/qt/Api/qvehicleinterface.cpp \
		$$PWD/qt/Api/qvehiclehistory.cpp \
		$$PWD/qt/Api/qvehicledata.cpp \
		$$PWD/qt/Api/qtroublecode.cpp \
		$$PWD/qt/Api/qvehiclebutton.cpp \
		$$PWD/qt/Api/qfuelconfiguration.cpp

	INCLUDEPATH += \
		$$ROOT_WEBKIT_DIR/Source/WebCore/Modules/vehicle
}

enable?(FJIB_AMB) {
	HEADERS += \
		$$PWD/qt/WebCoreSupport/AMBClientQt.h \
		$$PWD/qt/Api/qamb.h \
		$$PWD/qt/Api/qambobject.h \
		$$PWD/qt/Api/qamberror.h \
		$$PWD/qt/Api/qambcontext.h \
		$$PWD/qt/Api/qambcontextprivate.h \
		$$PWD/qt/Api/qambinterface.h \
		$$PWD/qt/Api/qambinfo.h \

	SOURCES += \
		$$PWD/qt/WebCoreSupport/AMBClientQt.cpp \
		$$PWD/qt/Api/qambobject.cpp \
		$$PWD/qt/Api/qamberror.cpp \
		$$PWD/qt/Api/qambinterface.cpp \
		$$PWD/qt/Api/qambcontext.cpp \
		$$PWD/qt/Api/qambinfo.cpp \

	INCLUDEPATH += \
		$$ROOT_WEBKIT_DIR/Source/WebCore/Modules/fjibamb
}

enable?(ICONDATABASE) {
    HEADERS += \
        $$PWD/../WebCore/loader/icon/IconDatabaseClient.h \
        $$PWD/qt/WebCoreSupport/IconDatabaseClientQt.h

    SOURCES += \
        $$PWD/qt/WebCoreSupport/IconDatabaseClientQt.cpp
}

enable?(VIDEO) {
    use?(GSTREAMER) | use?(QT_MULTIMEDIA) {
        HEADERS += $$PWD/qt/WebCoreSupport/FullScreenVideoQt.h
        SOURCES += $$PWD/qt/WebCoreSupport/FullScreenVideoQt.cpp
    }
}

enable?(NETWORK_INFO) {
	HEADERS += \
		$$PWD/qt/WebCoreSupport/NetworkInfoClientQt.h \
		$$PWD/qt/Api/qnetworkinfo.h \
		$$PWD/qt/Api/qnetworkinfo_p.h

	SOURCES += \
		$$PWD/qt/WebCoreSupport/NetworkInfoClientQt.cpp \
		$$PWD/qt/Api/qnetworkinfo.cpp

	INCLUDEPATH += \
		$$ROOT_WEBKIT_DIR/Source/WebCore/Modules/networkinfo
}

enable?(SCRIPTED_SPEECH) {
	HEADERS += \
		$$PWD/qt/WebCoreSupport/SpeechRecognitionClientQt.h \
		$$PWD/qt/Api/qspeechrecognition.h \
		$$PWD/qt/Api/qspeechrecognition_p.h \
		$$PWD/qt/Api/qspeechgrammarlist.h \
		$$PWD/qt/Api/qspeechgrammar.h \
		$$PWD/qt/Api/qspeechrecognitionresult.h \
		$$PWD/qt/Api/qspeechrecognitionalternative.h \
		$$PWD/qt/Api/qspeechrecognitionerror.h \
		$$PWD/qt/Api/qspeechrecognition_p.h
	SOURCES += \
		$$PWD/qt/WebCoreSupport/SpeechRecognitionClientQt.cpp \
		$$PWD/qt/Api/qspeechrecognition.cpp \
		$$PWD/qt/Api/qspeechgrammarlist.cpp \
		$$PWD/qt/Api/qspeechgrammar.cpp \
		$$PWD/qt/Api/qspeechrecognitionresult.cpp \
		$$PWD/qt/Api/qspeechrecognitionalternative.cpp \
		$$PWD/qt/Api/qspeechrecognitionerror.cpp
	INCLUDEPATH += \
		$$ROOT_WEBKIT_DIR/Source/WebCore/Modules/speech
}

enable?(SPEECH_SYNTHESIS) {
	HEADERS += \
		$$PWD/qt/WebCoreSupport/SpeechSynthesizerControllerQt.h \
		$$PWD/qt/Api/qspeechsynthesisutterance.h \
		$$PWD/qt/Api/qspeechsynthesisvoice.h \
		$$PWD/qt/Api/qspeechsynthesizer.h \
		$$PWD/qt/Api/qspeechsynthesizer_p.h 
	SOURCES += \
		$$PWD/qt/WebCoreSupport/SpeechSynthesizerControllerQt.cpp \
		$$PWD/qt/Api/qspeechsynthesisutterance.cpp \
		$$PWD/qt/Api/qspeechsynthesisvoice.cpp \
		$$PWD/qt/Api/qspeechsynthesizer.cpp
	INCLUDEPATH += \
		$$ROOT_WEBKIT_DIR/Source/WebCore/Modules/speech \
		$$ROOT_WEBKIT_DIR/Source/WebCore/platform \
		$$ROOT_WEBKIT_DIR/Source/WebCore/platform/qt
}

win32 { 
    QMAKE_PRE_LINK = copy /y $$toSystemPath($${ROOT_BUILD_DIR}/Source/WebCore/release/*.lib) $$toSystemPath($${ROOT_BUILD_DIR}/lib) 
} 