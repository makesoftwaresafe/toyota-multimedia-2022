/*** COPYRIGHT FUJITSU LIMITED 2017 ***/

#ifndef FHILHEVENTHANDLER_H
#define FHILHEVENTHANDLER_H

#include <QObject>
#include <QSocketNotifier>

class FhiLhEventHandler : public QObject
{
	Q_OBJECT
public:
	explicit FhiLhEventHandler(QObject *parent = 0);
	~FhiLhEventHandler();
	void setUiEventHandler(int a_fd, void(*eventHandler)(void*), void(*surfaceDestructor)(void*), void* a_data) { m_uiEventData = a_data; m_uiEventHandler = eventHandler; m_surfaceDestructor = surfaceDestructor; m_uiEventNotifier = new QSocketNotifier(a_fd, QSocketNotifier::Read, this);connect(m_uiEventNotifier, SIGNAL(activated(int)), this, SLOT(handleUiEvent()));}

private slots:
	void handleUiEvent();

private:


	void *m_uiEventData;
	void (*m_uiEventHandler)(void *);
	QSocketNotifier *m_uiEventNotifier;
	void (*m_surfaceDestructor)(void *);
};

#endif
