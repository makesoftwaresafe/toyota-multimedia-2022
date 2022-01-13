#ifndef GeolocationPositionSource_h
#define GeolocationPositionSource_h
#include <QObject>
#include <QString>
#include "qgeopositioninfo.h"

/**/
class QWebPageAdapter;
/**/

class GeolocationPositionSource: public QObject
{
	Q_OBJECT
public:
/**/
	static GeolocationPositionSource* createDefaultSource(const QWebPageAdapter *);
/**/
	~GeolocationPositionSource(){}

	void startUpdates();
	void stopUpdates();

public slots:
	void geolocationPositionUpdated(const QString&, 
		const QString&, 
		const QString&, 
		const QString&, 
		const QString&, 
		const QString&, 
		const QString&);

signals:
	void getGeolocationPosition();
	void positionUpdated(const QGeoPositionInfo&);

private:
/**/
	GeolocationPositionSource(const QWebPageAdapter *);
	QWebPageAdapter* m_page;
/**/
	QGeoPositionInfo m_info;
};

#endif	/* GeolocationPositionSource_h */
