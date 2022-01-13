/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GeolocationClientQt_h
#define GeolocationClientQt_h

#include "GeolocationClient.h"

#include <QObject>
#include <wtf/RefPtr.h>

/**/
#if QT_VERSION <= QT_VERSION_CHECK(5, 0, 0)

#ifdef USE_QTMOBILITY
#include <QGeoPositionInfo>
namespace QtMobility {
class QGeoPositionInfoSource;
};
using namespace QtMobility;
#else
class QWebView;
#include "GeolocationPositionSource.h"
#include "qgeopositioninfo.h"
#endif

#elif QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
#include <QtPositioning/QGeoPositionInfo>
#include "GeolocationPositionSource.h"

#elif QT_VERSION > QT_VERSION_CHECK(5, 0, 0)
#include <QtLocation/QGeoPositionInfo>
class QGeoPositionInfoSource;
#endif
/**/


class QWebPageAdapter;

namespace WebCore {

// This class provides an implementation of a GeolocationClient for QtWebkit.
class GeolocationClientQt : public QObject, public GeolocationClient {
    Q_OBJECT

public:
    GeolocationClientQt(const QWebPageAdapter*);
    virtual ~GeolocationClientQt();

    virtual void geolocationDestroyed();
    virtual void startUpdating();
    virtual void stopUpdating();

    void setEnableHighAccuracy(bool);
    virtual GeolocationPosition* lastPosition() { return m_lastPosition.get(); }

    virtual void requestPermission(Geolocation*);
    virtual void cancelPermissionRequest(Geolocation*);

private Q_SLOTS:
    void positionUpdated(const QGeoPositionInfo&);

private:
    const QWebPageAdapter* m_webPage;
    RefPtr<GeolocationPosition> m_lastPosition;
 
/**/   
#ifdef USE_QTMOBILITY
    QGeoPositionInfoSource* m_location;
#else
    GeolocationPositionSource* m_location;
#endif
/**/
};

} // namespace WebCore

#endif // GeolocationClientQt_h
