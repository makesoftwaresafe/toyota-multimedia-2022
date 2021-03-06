/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in 
** accordance with the Qt Commercial License Agreement provided with
** the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qgeocoordinate.h"
#include "qlocationutils_p.h"

#include <QDateTime>
#include <QHash>
#include <QDataStream>
#include <QDebug>
#include <qnumeric.h>

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const double qgeocoordinate_EARTH_MEAN_RADIUS = 6371.0072;

inline static double qgeocoordinate_degToRad(double deg)
{
    return deg * M_PI / 180;
}
inline static double qgeocoordinate_radToDeg(double rad)
{
    return rad * 180 / M_PI;
}


class QGeoCoordinatePrivate
{
public:
    double lat;
    double lng;
    double alt;

    QGeoCoordinatePrivate() {
        lat = qQNaN();
        lng = qQNaN();
        alt = qQNaN();
    }
};


/*!
    \class QGeoCoordinate
    \brief The QGeoCoordinate class defines a geographical position on the surface of the Earth.

    \inmodule QtLocation
    
    \ingroup location

    A QGeoCoordinate is defined by latitude, longitude, and optionally, altitude.

    Use type() to determine whether a coordinate is a 2D coordinate (has
    latitude and longitude only) or 3D coordinate (has latitude, longitude
    and altitude). Use distanceTo() and azimuthTo() to calculate the distance
    and bearing between coordinates.

    The coordinate values should be specified using the WGS84 datum.
*/

/*!
    \enum QGeoCoordinate::CoordinateType
    Defines the types of a coordinate.

    \value InvalidCoordinate An invalid coordinate. A coordinate is invalid if its latitude or longitude values are invalid.
    \value Coordinate2D A coordinate with valid latitude and longitude values.
    \value Coordinate3D A coordinate with valid latitude and longitude values, and also an altitude value.
*/

/*!
    \enum QGeoCoordinate::CoordinateFormat
    Defines the possible formatting options for toString().

    \value Degrees Returns a string representation of the coordinates in decimal degrees format.
    \value DegreesWithHemisphere Returns a string representation of the coordinates in decimal degrees format, using 'N', 'S', 'E' or 'W' to indicate the hemispheres of the coordinates.
    \value DegreesMinutes Returns a string representation of the coordinates in degrees-minutes format.
    \value DegreesMinutesWithHemisphere Returns a string representation of the coordinates in degrees-minutes format, using 'N', 'S', 'E' or 'W' to indicate the hemispheres of the coordinates.
    \value DegreesMinutesSeconds Returns a string representation of the coordinates in degrees-minutes-seconds format.
    \value DegreesMinutesSecondsWithHemisphere Returns a string representation of the coordinates in degrees-minutes-seconds format, using 'N', 'S', 'E' or 'W' to indicate the hemispheres of the coordinates.

    \sa toString()
*/


/*!
    Constructs a coordinate. The coordinate will be invalid until
    setLatitude() and setLongitude() have been called.
*/
QGeoCoordinate::QGeoCoordinate()
        : d(new QGeoCoordinatePrivate)
{
}

/*!
    Constructs a coordinate with the given \a latitude and \a longitude.

    If the latitude is not between -90 to 90 inclusive, or the longitude
    is not between -180 to 180 inclusive, none of the values are set and
    the type() will be QGeoCoordinate::InvalidCoordinate.

    \sa isValid()
*/
QGeoCoordinate::QGeoCoordinate(double latitude, double longitude)
        : d(new QGeoCoordinatePrivate)
{
    if (QLocationUtils::isValidLat(latitude) && QLocationUtils::isValidLong(longitude)) {
        d->lat = latitude;
        d->lng = longitude;
    }
}

/*!
    Constructs a coordinate with the given \a latitude, \a longitude
    and \a altitude.

    If the latitude is not between -90 to 90 inclusive, or the longitude
    is not between -180 to 180 inclusive, none of the values are set and
    the type() will be QGeoCoordinate::InvalidCoordinate.

    Note that \a altitude specifies the metres above sea level.

    \sa isValid()
*/
QGeoCoordinate::QGeoCoordinate(double latitude, double longitude, double altitude)
        : d(new QGeoCoordinatePrivate)
{
    if (QLocationUtils::isValidLat(latitude) && QLocationUtils::isValidLong(longitude)) {
        d->lat = latitude;
        d->lng = longitude;
        d->alt = altitude;
    }
}

/*!
    Constructs a coordinate from the contents of \a other.
*/
QGeoCoordinate::QGeoCoordinate(const QGeoCoordinate &other)
        : d(new QGeoCoordinatePrivate)
{
    operator=(other);
}

/*!
    Destroys the coordinate object.
*/
QGeoCoordinate::~QGeoCoordinate()
{
    delete d;
}

/*!
    Assigns \a other to this coordinate and returns a reference to this
    coordinate.
*/
QGeoCoordinate &QGeoCoordinate::operator=(const QGeoCoordinate & other)
{
    if (this == &other)
        return *this;

    d->lat = other.d->lat;
    d->lng = other.d->lng;
    d->alt = other.d->alt;

    return *this;
}

/*!
    Returns true if the latitude, longitude and altitude of this
    coordinate are the same as those of \a other.

    The longitude will be ignored if the latitude is +/- 90 degrees.
*/
bool QGeoCoordinate::operator==(const QGeoCoordinate &other) const
{
    bool latEqual = (qIsNaN(d->lat) && qIsNaN(other.d->lat))
                        || qFuzzyCompare(d->lat, other.d->lat);
    bool lngEqual = (qIsNaN(d->lng) && qIsNaN(other.d->lng))
                        || qFuzzyCompare(d->lng, other.d->lng);
    bool altEqual = (qIsNaN(d->alt) && qIsNaN(other.d->alt))
                        || qFuzzyCompare(d->alt, other.d->alt);

    if (!qIsNaN(d->lat) && ((d->lat == 90.0) || (d->lat == -90.0)))
        lngEqual = true;

    return (latEqual && lngEqual && altEqual);
}

/*!
    \fn bool QGeoCoordinate::operator!=(const QGeoCoordinate &other) const;

    Returns true if the latitude, longitude or altitude of this
    coordinate are not the same as those of \a other.
*/

/*!
    Returns true if the type() is Coordinate2D or Coordinate3D.
*/
bool QGeoCoordinate::isValid() const
{
    CoordinateType t = type();
    return t == Coordinate2D || t == Coordinate3D;
}

/*!
    Returns the type of this coordinate.
*/
QGeoCoordinate::CoordinateType QGeoCoordinate::type() const
{
    if (QLocationUtils::isValidLat(d->lat)
            && QLocationUtils::isValidLong(d->lng)) {
        if (qIsNaN(d->alt))
            return Coordinate2D;
        return Coordinate3D;
    }
    return InvalidCoordinate;
}


/*!
    Returns the latitude, in decimal degrees. The return value is undefined
    if the latitude has not been set.

    A positive latitude indicates the Northern Hemisphere, and a negative
    latitude indicates the Southern Hemisphere.

    \sa setLatitude(), type()
*/
double QGeoCoordinate::latitude() const
{
    return d->lat;
}

/*!
    Sets the latitude (in decimal degrees) to \a latitude. The value should
    be in the WGS84 datum.

    To be valid, the latitude must be between -90 to 90 inclusive.

    \sa latitude()
*/
void QGeoCoordinate::setLatitude(double latitude)
{
    d->lat = latitude;
}

/*!
    Returns the longitude, in decimal degrees. The return value is undefined
    if the longitude has not been set.

    A positive longitude indicates the Eastern Hemisphere, and a negative
    longitude indicates the Western Hemisphere.

    \sa setLongitude(), type()
*/
double QGeoCoordinate::longitude() const
{
    return d->lng;
}

/*!
    Sets the longitude (in decimal degrees) to \a longitude. The value should
    be in the WGS84 datum.

    To be valid, the longitude must be between -180 to 180 inclusive.

    \sa longitude()
*/
void QGeoCoordinate::setLongitude(double longitude)
{
    d->lng = longitude;
}

/*!
    Returns the altitude (meters above sea level).

    The return value is undefined if the altitude has not been set.

    \sa setAltitude(), type()
*/
double QGeoCoordinate::altitude() const
{
    return d->alt;
}

/*!
    Sets the altitude (meters above sea level) to \a altitude.

    \sa altitude()
*/
void QGeoCoordinate::setAltitude(double altitude)
{
    d->alt = altitude;
}

/*!
    Returns the distance (in meters) from this coordinate to the coordinate
    specified by \a other. Altitude is not used in the calculation.

    This calculation returns the great-circle distance between the two
    coordinates, with an assumption that the Earth is spherical for the
    purpose of this calculation.

    Returns 0 if the type of this coordinate or the type of \a other is
    QGeoCoordinate::InvalidCoordinate.
*/
qreal QGeoCoordinate::distanceTo(const QGeoCoordinate &other) const
{
    if (type() == QGeoCoordinate::InvalidCoordinate
            || other.type() == QGeoCoordinate::InvalidCoordinate) {
        return 0;
    }

    // Haversine formula
    double dlat = qgeocoordinate_degToRad(other.d->lat - d->lat);
    double dlon = qgeocoordinate_degToRad(other.d->lng - d->lng);
    double y = sin(dlat / 2.0) * sin(dlat / 2.0)
               + cos(qgeocoordinate_degToRad(d->lat))
               * cos(qgeocoordinate_degToRad(other.d->lat))
               * sin(dlon / 2.0) * sin(dlon / 2.0);
    double x = 2 * asin(sqrt(y));
    return qreal(x * qgeocoordinate_EARTH_MEAN_RADIUS * 1000);
}

/*!
    Returns the azimuth (or bearing) in degrees from this coordinate to the
    coordinate specified by \a other. Altitude is not used in the calculation.

    The bearing returned is the bearing from the origin to \a other along the
    great-circle between the two coordinates. There is an assumption that the
    Earth is spherical for the purpose of this calculation.

    Returns 0 if the type of this coordinate or the type of \a other is
    QGeoCoordinate::InvalidCoordinate.
*/
qreal QGeoCoordinate::azimuthTo(const QGeoCoordinate &other) const
{
    if (type() == QGeoCoordinate::InvalidCoordinate
            || other.type() == QGeoCoordinate::InvalidCoordinate) {
        return 0;
    }

    double dlon = qgeocoordinate_degToRad(other.d->lng - d->lng);
    double lat1Rad = qgeocoordinate_degToRad(d->lat);
    double lat2Rad = qgeocoordinate_degToRad(other.d->lat);

    double y = sin(dlon) * cos(lat2Rad);
    double x = cos(lat1Rad) * sin(lat2Rad) - sin(lat1Rad) * cos(lat2Rad) * cos(dlon);

    double whole;
    double fraction = modf(qgeocoordinate_radToDeg(atan2(y, x)), &whole);
    return qreal((int(whole + 360) % 360) + fraction);
}

/*!
    Returns this coordinate as a string in the specified \a format.

    For example, if this coordinate has a latitude of -27.46758, a longitude
    of 153.027892 and an altitude of 28.1, these are the strings
    returned depending on \a format:

    \table
    \header
        \o \a format value
        \o Returned string
    \row
        \o \l Degrees
        \o -27.46758\unicode{0xB0}, 153.02789\unicode{0xB0}, 28.1m
    \row
        \o \l DegreesWithHemisphere
        \o 27.46758\unicode{0xB0} S, 153.02789\unicode{0xB0} E, 28.1m
    \row
        \o \l DegreesMinutes
        \o -27\unicode{0xB0} 28.054', 153\unicode{0xB0} 1.673', 28.1m
    \row
        \o \l DegreesMinutesWithHemisphere
        \o 27\unicode{0xB0} 28.054 S', 153\unicode{0xB0} 1.673' E, 28.1m
    \row
        \o \l DegreesMinutesSeconds
        \o -27\unicode{0xB0} 28' 3.2", 153\unicode{0xB0} 1' 40.4", 28.1m
    \row
        \o \l DegreesMinutesSecondsWithHemisphere
        \o 27\unicode{0xB0} 28' 3.2" S, 153\unicode{0xB0} 1' 40.4" E, 28.1m
    \endtable

    The altitude field is omitted if no altitude is set.

    If the coordinate is invalid, an empty string is returned.
*/
QString QGeoCoordinate::toString(CoordinateFormat format) const
{
    if (type() == QGeoCoordinate::InvalidCoordinate)
        return QString();

    QString latStr;
    QString longStr;

    double absLat = qAbs(d->lat);
    double absLng = qAbs(d->lng);
    QChar symbol(0x00B0);   // degrees symbol

    switch (format) {
        case Degrees:
        case DegreesWithHemisphere: {
            latStr = QString::number(absLat, 'f', 5) + symbol;
            longStr = QString::number(absLng, 'f', 5) + symbol;
            break;
        }
        case DegreesMinutes:
        case DegreesMinutesWithHemisphere: {
            double latMin = (absLat - int(absLat)) * 60;
            double lngMin = (absLng - int(absLng)) * 60;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
            latStr = QString::fromLatin1("%1%2 %3'")
                     .arg(QString::number(int(absLat)))
                     .arg(symbol)
                     .arg(QString::number(latMin, 'f', 3));
            longStr = QString::fromLatin1("%1%2 %3'")
                      .arg(QString::number(int(absLng)))
                      .arg(symbol)
                      .arg(QString::number(lngMin, 'f', 3));
#else
            latStr = QString::fromAscii("%1%2 %3'")
                     .arg(QString::number(int(absLat)))
                     .arg(symbol)
                     .arg(QString::number(latMin, 'f', 3));
            longStr = QString::fromAscii("%1%2 %3'")
                      .arg(QString::number(int(absLng)))
                      .arg(symbol)
                      .arg(QString::number(lngMin, 'f', 3));
#endif
            break;
        }
        case DegreesMinutesSeconds:
        case DegreesMinutesSecondsWithHemisphere: {
            double latMin = (absLat - int(absLat)) * 60;
            double lngMin = (absLng - int(absLng)) * 60;
            double latSec = (latMin - int(latMin)) * 60;
            double lngSec = (lngMin - int(lngMin)) * 60;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
            latStr = QString::fromLatin1("%1%2 %3' %4\"")
                     .arg(QString::number(int(absLat)))
                     .arg(symbol)
                     .arg(QString::number(int(latMin)))
                     .arg(QString::number(latSec, 'f', 1));
            longStr = QString::fromLatin1("%1%2 %3' %4\"")
                      .arg(QString::number(int(absLng)))
                      .arg(symbol)
                      .arg(QString::number(int(lngMin)))
                      .arg(QString::number(lngSec, 'f', 1));
#else
            latStr = QString::fromAscii("%1%2 %3' %4\"")
                     .arg(QString::number(int(absLat)))
                     .arg(symbol)
                     .arg(QString::number(int(latMin)))
                     .arg(QString::number(latSec, 'f', 1));
            longStr = QString::fromAscii("%1%2 %3' %4\"")
                      .arg(QString::number(int(absLng)))
                      .arg(symbol)
                      .arg(QString::number(int(lngMin)))
                      .arg(QString::number(lngSec, 'f', 1));
#endif
            break;
        }
    }

    // now add the "-" to the start, or append the hemisphere char
    switch (format) {
        case Degrees:
        case DegreesMinutes:
        case DegreesMinutesSeconds: {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
			QString s = QString::fromLatin1("-");
#else
            QString s = QString::fromAscii("-");
#endif
            if (d->lat < 0)
                latStr.insert(0, s);
            if (d->lng < 0)
                longStr.insert(0, s);
            break;
        }
        case DegreesWithHemisphere:
        case DegreesMinutesWithHemisphere:
        case DegreesMinutesSecondsWithHemisphere: {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
            if (d->lat < 0)
                latStr.append(QString::fromLatin1(" S"));
            else if (d->lat > 0)
                latStr.append(QString::fromLatin1(" N"));
            if (d->lng < 0)
                longStr.append(QString::fromLatin1(" W"));
            else if (d->lng > 0)
                longStr.append(QString::fromLatin1(" E"));
            break;
#else
            if (d->lat < 0)
                latStr.append(QString::fromAscii(" S"));
            else if (d->lat > 0)
                latStr.append(QString::fromAscii(" N"));
            if (d->lng < 0)
                longStr.append(QString::fromAscii(" W"));
            else if (d->lng > 0)
                longStr.append(QString::fromAscii(" E"));
            break;
#endif
        }
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    if (qIsNaN(d->alt))
        return QString::fromLatin1("%1, %2").arg(latStr, longStr);
    return QString::fromLatin1("%1, %2, %3m").arg(latStr, longStr, QString::number(d->alt));
#else
    if (qIsNaN(d->alt))
        return QString::fromAscii("%1, %2").arg(latStr, longStr);
    return QString::fromAscii("%1, %2, %3m").arg(latStr, longStr, QString::number(d->alt));
#endif
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QGeoCoordinate &coord)
{
    double lat = coord.latitude();
    double lng = coord.longitude();

    dbg.nospace() << "QGeoCoordinate(";
    if (qIsNaN(lat))
        dbg.nospace() << '?';
    else
        dbg.nospace() << lat;
    dbg.nospace() << ", ";
    if (qIsNaN(lng))
        dbg.nospace() << '?';
    else
        dbg.nospace() << lng;
    if (coord.type() == QGeoCoordinate::Coordinate3D) {
        dbg.nospace() << ", ";
        dbg.nospace() << coord.altitude();
    }
    dbg.nospace() << ')';
    return dbg;
}
#endif

#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QGeoCoordinate &coordinate)

    \relates QGeoCoordinate

    Writes the given \a coordinate to the specified \a stream.

    \sa {Format of the QDataStream Operators}
*/

QDataStream &operator<<(QDataStream &stream, const QGeoCoordinate &coordinate)
{
    stream << coordinate.latitude();
    stream << coordinate.longitude();
    stream << coordinate.altitude();
    return stream;
}
#endif

#ifndef QT_NO_DATASTREAM
/*!
    \fn  QDataStream &operator>>(QDataStream &stream, QGeoCoordinate &coordinate)
    \relates QGeoCoordinate

    Reads a coordinate from the specified \a stream into the given
    \a coordinate.

    \sa {Format of the QDataStream Operators}
*/

QDataStream &operator>>(QDataStream &stream, QGeoCoordinate &coordinate)
{
    double value;
    stream >> value;
    coordinate.setLatitude(value);
    stream >> value;
    coordinate.setLongitude(value);
    stream >> value;
    coordinate.setAltitude(value);
    return stream;
}
#endif
