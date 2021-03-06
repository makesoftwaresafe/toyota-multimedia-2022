/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2008 Holger Hans Peter Freyther
    Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/

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
#ifndef FontPlatformData_h
#define FontPlatformData_h

#include "FontDescription.h"
#include "FontOrientation.h"
#include <QFont>
#include <QHash>
#include <QRawFont>
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class FontPlatformDataPrivate : public RefCounted<FontPlatformDataPrivate> {
    WTF_MAKE_NONCOPYABLE(FontPlatformDataPrivate); WTF_MAKE_FAST_ALLOCATED;
public:
    FontPlatformDataPrivate()
/**/
#if !HAVE(QT5)
        : size(font.pixelSize())
        , bold(font.bold())
#else
        : size(0)
        , bold(false)
#endif
/**/
        , oblique(false)
        , isDeletedValue(false)
    { }
    FontPlatformDataPrivate(const float size, const bool bold, const bool oblique)
        : size(size)
        , bold(bold)
        , oblique(oblique)
        , isDeletedValue(false)
    {
/**/
#if HAVE(QT5)
/**/
// This is necessary for SVG Fonts, which are only supported when using QRawFont.
// It is used to construct the appropriate platform data to use as a fallback.
        QFont font;
        font.setBold(bold);
        font.setItalic(oblique);
        rawFont = QRawFont::fromFont(font, QFontDatabase::Any);
        rawFont.setPixelSize(size);
/**/
# endif
/**/
    }

/**/
#if !HAVE(QT5)
    FontPlatformDataPrivate(const QFont & font)
        : font(font)
        , size(font.pixelSize())
        , bold(font.bold())
        , isDeletedValue(false)
    {}
#else
/**/
    FontPlatformDataPrivate(const QRawFont& rawFont)
        : rawFont(rawFont)
        , size(rawFont.pixelSize())
        , bold(rawFont.weight() >= QFont::Bold)
        , oblique(false)
        , isDeletedValue(false)
    { }
/**/
#endif
/**/

    FontPlatformDataPrivate(WTF::HashTableDeletedValueType)
        : isDeletedValue(true)
    { }

/**/
#if !HAVE(QT5)
    QFont font;
#else
    QRawFont rawFont;
#endif
/**/
    float size;
    bool bold : 1;
    bool oblique : 1;
    bool isDeletedValue : 1;
};

class FontPlatformData {
    WTF_MAKE_FAST_ALLOCATED;
public:
    FontPlatformData(float size, bool bold, bool oblique);
    FontPlatformData(const FontDescription&, const AtomicString& familyName, int wordSpacing = 0, int letterSpacing = 0);
    FontPlatformData(const FontPlatformData&, float size);
/**/
#if !HAVE(QT5)
    FontPlatformData(const QFont& font)
        : m_data(adoptRef(new FontPlatformDataPrivate(font)))
    {}
#else
/**/
    FontPlatformData(const QRawFont& rawFont)
        : m_data(adoptRef(new FontPlatformDataPrivate(rawFont)))
    { }
/**/
#endif
/**/
    FontPlatformData(WTF::HashTableDeletedValueType)
        : m_data(adoptRef(new FontPlatformDataPrivate()))
    {
        m_data->isDeletedValue = true;
    }

    bool operator==(const FontPlatformData&) const;

    bool isHashTableDeletedValue() const
    {
        return m_data && m_data->isDeletedValue;
    }

/**/
#if !HAVE(QT5)
    QFont font() const
    {
        return m_data->font;
    }
#else
/**/
    QRawFont rawFont() const
    {
        Q_ASSERT(!isHashTableDeletedValue());
        if (!m_data)
            return QRawFont();
        return m_data->rawFont;
    }
/**/
#endif
/**/
    float size() const
    {
        Q_ASSERT(!isHashTableDeletedValue());
        if (!m_data)
            return 0;
        return m_data->size;
    }

    FontOrientation orientation() const { return Horizontal; } // FIXME: Implement.
    void setOrientation(FontOrientation) { } // FIXME: Implement.

    unsigned hash() const;

#ifndef NDEBUG
    String description() const;
#endif
private:
    RefPtr<FontPlatformDataPrivate> m_data;
};

} // namespace WebCore

#endif // FontPlatformData_h
