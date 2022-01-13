/*** COPYRIGHT FUJITSU LIMITED 2017 ***/

#include "fhilhevnethandler.h"

FhiLhEventHandler::FhiLhEventHandler(QObject *parent)
:QObject(parent)
{
	m_uiEventHandler = NULL;
	m_uiEventData = NULL;
	m_uiEventNotifier = NULL;
	m_surfaceDestructor = NULL;
}

FhiLhEventHandler::~FhiLhEventHandler()
{
	if (NULL != m_uiEventNotifier)
	{
		delete m_uiEventNotifier;
		m_uiEventNotifier = NULL;
	}
	m_uiEventHandler = NULL;
	m_uiEventData = NULL;
	m_surfaceDestructor = NULL;
}

void FhiLhEventHandler::handleUiEvent()
{
	if ( NULL != m_uiEventHandler )
	{
		(m_uiEventHandler)(m_uiEventData);
	}
}

