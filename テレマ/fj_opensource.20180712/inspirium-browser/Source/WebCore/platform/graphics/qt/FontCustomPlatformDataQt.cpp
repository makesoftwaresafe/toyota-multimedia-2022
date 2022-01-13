/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/
#include "config.h"
#include "FontCustomPlatformData.h"

#include "FontPlatformData.h"
#include "SharedBuffer.h"

/**/
#if !HAVE(QT5)
#include <QFontDatabase>
#endif

#if 1 //USE(ZLIB)
#include "WOFFFileFormat.h"
#endif
/**/
#include <QStringList>

namespace WebCore {

FontPlatformData FontCustomPlatformData::fontPlatformData(int size, bool bold, bool italic, FontOrientation, FontWidthVariant, FontRenderingMode)
{
/**/
#if !HAVE(QT5)
    QFont font;
    font.setFamily(QFontDatabase::applicationFontFamilies(m_handle)[0]);
    font.setPixelSize(size);
    if (bold) 
        font.setWeight(QFont::Bold);
    font.setItalic(italic);
    return FontPlatformData(font);
#else
    Q_ASSERT(m_rawFont.isValid());
    m_rawFont.setPixelSize(qreal(size));
    return FontPlatformData(m_rawFont);
#endif
/**/
}

std::unique_ptr<FontCustomPlatformData> createFontCustomPlatformData(SharedBuffer& sharedBuffer)
{
    SharedBuffer* buffer = &sharedBuffer;

/**/
#if 1 //USE(ZLIB)
/**/
    RefPtr<SharedBuffer> sfntBuffer;
    if (isWOFF(buffer)) {
        Vector<char> sfnt;
        if (!convertWOFFToSfnt(buffer, sfnt))
            return nullptr;

        sfntBuffer = SharedBuffer::adoptVector(sfnt);
        buffer = sfntBuffer.get();
    }
#endif // USE(ZLIB)

    const QByteArray fontData(buffer->data(), buffer->size());

/**/
#if !HAVE(QT5)
    int id = QFontDatabase::addApplicationFontFromData(fontData);
    if (id == -1)
       return nullptr;
    Q_ASSERT(QFontDataBase::applicationFontFamilies(id).size() > 0);
#endif
/**/

#if !USE(ZLIB)
    if (fontData.startsWith("wOFF")) {
        qWarning("WOFF support requires QtWebKit to be built with zlib support.");
        return nullptr;
    }
#endif // !USE(ZLIB)
    // Pixel size doesn't matter at this point, it is set in FontCustomPlatformData::fontPlatformData.
    QRawFont rawFont(fontData, /*pixelSize = */0, QFont::PreferDefaultHinting);
    if (!rawFont.isValid())
        return nullptr;

    std::unique_ptr<FontCustomPlatformData> data = std::make_unique<FontCustomPlatformData>();
/**/
#if !HAVE(QT5)
    data->m_handle = id;
#else
    data->m_rawFont = rawFont;
#endif
/**/
    return data;
}

bool FontCustomPlatformData::supportsFormat(const String& format)
{
    return equalIgnoringCase(format, "truetype") || equalIgnoringCase(format, "opentype")
/**/
#if 1 //USE(ZLIB)
/**/
            || equalIgnoringCase(format, "woff")
#endif
    ;
}

}
