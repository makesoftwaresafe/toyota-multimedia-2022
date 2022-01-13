# enable if you want to use font information
# from InspiriumExtension
# DEFINES += USE_FJIB_FONT_INFO

!win32|contains(QT_CONFIG, freetype):!mac {
    include($$PWD/basic/basic.pri)
}

unix:!mac {
    CONFIG += qpa/genericunixfontdatabase
    include($$PWD/genericunix/genericunix.pri)
    contains(QT_CONFIG,fontconfig) {
        include($$PWD/fontconfig/fontconfig.pri)
    }
}

mac {
    include($$PWD/mac/coretext.pri)
}

