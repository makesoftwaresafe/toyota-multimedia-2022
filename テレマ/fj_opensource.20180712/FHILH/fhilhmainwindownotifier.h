/*** COPYRIGHT FUJITSU LIMITED 2017 ***/

#ifndef FHILHMAINWINDOWNOTIFIER_H
#define FHILHMAINWINDOWNOTIFIER_H

#include <QObject>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE

class FhiLhMainWindowNotifier : public QObject
{
	Q_OBJECT

public:
	explicit FhiLhMainWindowNotifier();
	~FhiLhMainWindowNotifier();

public slots:
	void onHideWindow();
	void notifyDrawing();

private:
	bool mIsFirstDrawn;
    QTcpSocket mTcpSocket; 
};

#endif // FHILHMAINWINDOWNOTIFIER_H
