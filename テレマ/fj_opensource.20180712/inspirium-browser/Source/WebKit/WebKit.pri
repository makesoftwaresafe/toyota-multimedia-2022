SOURCES += \
    $$PWD/qt/Api/qwebframe.cpp \
    $$PWD/qt/Api/qgraphicswebview.cpp \
    $$PWD/qt/Api/qwebpage.cpp \
    $$PWD/qt/Api/qwebview.cpp \
    $$PWD/qt/Api/qwebelement.cpp \
    $$PWD/qt/Api/qwebhistory.cpp \
    $$PWD/qt/Api/qwebsettings.cpp \
    $$PWD/qt/Api/qwebhistoryinterface.cpp \
    $$PWD/qt/Api/qwebplugindatabase.cpp \
    $$PWD/qt/Api/qwebpluginfactory.cpp \
    $$PWD/qt/Api/qwebsecurityorigin.cpp \
    $$PWD/qt/Api/qwebscriptworld.cpp \
    $$PWD/qt/Api/qwebdatabase.cpp \
    $$PWD/qt/Api/qwebinspector.cpp \
    $$PWD/qt/Api/qwebkitversion.cpp \
#
    $$PWD/qt/Api/qhttpheader.cpp \
    $$PWD/qt/Api/qdiskcache.cpp
#

HEADERS += \
    $$PWD/qt/Api/qwebframe.h \
    $$PWD/qt/Api/qwebframe_p.h \
    $$PWD/qt/Api/qgraphicswebview.h \
    $$PWD/qt/Api/qwebkitglobal.h \
    $$PWD/qt/Api/qwebkitplatformplugin.h \
    $$PWD/qt/Api/qwebpage.h \
    $$PWD/qt/Api/qwebview.h \
    $$PWD/qt/Api/qwebsettings.h \
    $$PWD/qt/Api/qwebhistoryinterface.h \
    $$PWD/qt/Api/qwebdatabase.h \
    $$PWD/qt/Api/qwebsecurityorigin.h \
    $$PWD/qt/Api/qwebelement.h \
    $$PWD/qt/Api/qwebelement_p.h \
    $$PWD/qt/Api/qwebpluginfactory.h \
    $$PWD/qt/Api/qwebhistory.h \
    $$PWD/qt/Api/qwebinspector.h \
    $$PWD/qt/Api/qwebkitversion.h \
    $$PWD/qt/Api/qwebplugindatabase_p.h \
#
    $$PWD/qt/Api/qhttpheader_p.h\
    $$PWD/qt/Api/qdiskcache.h
#

enable?(FJIB_VIAPI) {
SOURCES += \
    $$PWD/qt/Api/qvehicle.cpp \
    $$PWD/qt/Api/qtrip.cpp
    
HEADERS += \
    $$PWD/qt/Api/qvehicle.h \
    $$PWD/qt/Api/qtrip.h
}

enable?(NETWORK_INFO) {
HEADERS += \
	$$PWD/qt/Api/qnetworkinfo.h

SOURCES += \
	$$PWD/qt/Api/qnetworkinfo.cpp
}

enable?(SCRIPTED_SPEECH) {
	HEADERS += \
		$$PWD/qt/Api/qspeechrecognition.h \
		$$PWD/qt/Api/qspeechrecognition_p.h \
		$$PWD/qt/Api/qspeechgrammarlist.h \
		$$PWD/qt/Api/qspeechgrammar.h \
		$$PWD/qt/Api/qspeechrecognitionresult.h \
		$$PWD/qt/Api/qspeechrecognitionalternative.h \
		$$PWD/qt/Api/qspeechrecognitionerror.h
	SOURCES += \
		$$PWD/qt/Api/qspeechrecognition.cpp \
		$$PWD/qt/Api/qspeechgrammarlist.cpp \
		$$PWD/qt/Api/qspeechgrammar.cpp \
		$$PWD/qt/Api/qspeechrecognitionresult.cpp \
		$$PWD/qt/Api/qspeechrecognitionalternative.cpp \
		$$PWD/qt/Api/qspeechrecognitionerror.cpp
}

enable?(SPEECH_SYNTHESIS) {
	HEADERS += \
		$$PWD/qt/Api/qspeechsynthesizer.h \
		$$PWD/qt/Api/qspeechsynthesisutterance.h \
		$$PWD/qt/Api/qspeechsynthesisvoice.h
	SOURCES += \
		$$PWD/qt/Api/qspeechsynthesisutterance.cpp \
		$$PWD/qt/Api/qspeechsynthesizer.cpp \
		$$PWD/qt/Api/qspeechsynthesisvoice.cpp
}

#
haveQt(5): contains(CONFIG, accessibility) {
#
    SOURCES += $$PWD/qt/Api/qwebviewaccessible.cpp
    HEADERS += $$PWD/qt/Api/qwebviewaccessible_p.h
}

