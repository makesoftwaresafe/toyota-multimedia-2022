#include "config.h"
#include "GeolocationPositionSource.h"

#include <qnumeric.h>
#include "qgeocoordinate.h"
/**/
#include "QWebPageAdapter.h"

GeolocationPositionSource::GeolocationPositionSource(const QWebPageAdapter * page)
{
	m_page = const_cast<QWebPageAdapter*>(page);
	m_page->bindingGeolocationPositionSource(this);
}

GeolocationPositionSource* GeolocationPositionSource::createDefaultSource(const QWebPageAdapter *page)
{
	return new GeolocationPositionSource(page);
}
/**/

void GeolocationPositionSource::startUpdates()
{
    emit getGeolocationPosition();
}

void GeolocationPositionSource::stopUpdates()
{
	/* do nothing */
}

void GeolocationPositionSource::geolocationPositionUpdated(const QString& latitude, 
	const QString& longitude,
	const QString& altitude, 
	const QString& accuracy, 
	const QString& altitudeAccuracy, 
	const QString& heading, 
	const QString& speed)
{
	QDateTime dateTime = QDateTime::currentDateTime ();
	m_info.setTimestamp(dateTime);

    QGeoCoordinate coordinate(latitude.toDouble(), longitude.toDouble());
	double alt = altitude.isEmpty()? qQNaN():altitude.toDouble();
	coordinate.setAltitude(alt);

    m_info.setCoordinate(coordinate);
	
	m_info.setAttribute(QGeoPositionInfo::HorizontalAccuracy, accuracy.toDouble());
	
	if (!altitudeAccuracy.isEmpty()){
		m_info.setAttribute(QGeoPositionInfo::VerticalAccuracy, altitudeAccuracy.toDouble());
	}

	if (!heading.isEmpty()){
		m_info.setAttribute(QGeoPositionInfo::Direction, heading.toDouble());
	}

	if (!speed.isEmpty()){
		m_info.setAttribute(QGeoPositionInfo::GroundSpeed, speed.toDouble());
	}
	emit positionUpdated(m_info);
}

#include "moc_GeolocationPositionSource.cpp"
