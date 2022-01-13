TEMPLATE = subdirs
SUBDIRS = \
    dds \
    icns \
    tga \
    wbmp

IGN = jp2 webp mng tiff

wince:SUBDIRS -= jp2

winrt {
    SUBDIRS -= tiff \
               tga
}

winrt|android: SUBDIRS -= webp
