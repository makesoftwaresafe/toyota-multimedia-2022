/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "fhilhbackingstore.h"
#include "fhilhmainwindownotifier.h"
#include "mftlhopengl.h"
#include "fhilhevnethandler.h"
#include "fhilhscreen.h"

#include <private/qguiapplication_p.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qwidget.h>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QOpenGLContext>
#include <qpa/qplatformwindow.h>

QT_BEGIN_NAMESPACE

static QWindow *g_widget;

void FhiLhBackingStore::touchListener(int type, int x1, int y1, int x2, int y2)
{
	//TODO:
	//now this listner can handle 1 point touch only (may need 2 point touch).
	//and touch event convert to mouse event (left btn pressed and released).
	QPointF point(x1 * 1.f, y1 * 1.f);
	Qt::MouseButtons mMouseButtons=Qt::NoButton;
	if (Touch_Begin == type)
	{
		mMouseButtons=Qt::LeftButton;
	}
	else if (Touch_End == type)
    	{
		mMouseButtons=Qt::NoButton;
	}
	else
	{
		mMouseButtons=Qt::LeftButton;
	}

	QWindowSystemInterface::handleMouseEvent(g_widget, point, point, mMouseButtons);
}


FhiLhBackingStore::FhiLhBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_context(NULL)
    , m_device(NULL)
    , m_mainWindowNotifier(NULL)
{
    m_windowInitialized = false;
    g_widget = window;
}

FhiLhBackingStore::~FhiLhBackingStore()
{
	if (m_context != NULL)
	{
		delete m_context;
		m_context = NULL;
	}

	if (m_device != NULL)
	{
		delete m_device;
		m_device = NULL;
	}

	if (m_mainWindowNotifier != NULL)
	{
		delete m_mainWindowNotifier;
		m_mainWindowNotifier = NULL;
    }
}

QPaintDevice *FhiLhBackingStore::paintDevice()
{
    return m_device;
}

void FhiLhBackingStore::beginPaint(const QRegion &)
{
	QWindow *targetWindow = window();
    if (m_windowInitialized != true)
	{
		QScreen *screen = targetWindow->screen();
		FhiLhScreen *pltScreen = (FhiLhScreen *)screen->handle();
		
		/* スクリーンのサイズをウィンドウに合わせる。*/
		QRect winGeom = targetWindow->geometry();
		QRect scrGeom = pltScreen->geometry();
		if (winGeom != scrGeom)
		{
			pltScreen->setGeometry(winGeom);
		}
		
		/* OpenGLのコンテキストを生成する。*/
		QSurfaceFormat format = targetWindow->requestedFormat();
		m_context = new QOpenGLContext();
		m_context->setFormat(format);
		m_context->setScreen(screen);
		m_context->create();
		
		/* EGLのswapIntervalを最小にする。*/
		m_context->makeCurrent(targetWindow);
		pltScreen->setMinInterval();

		/* Waylandのイベントディスパッチャーを設定する。*/
		int eventfd = pltScreen->getEvnetFd();
		eventHandler.setUiEventHandler(
				eventfd, handleEvent, openglDestroySurface, pltScreen->getPlatformHandle());
		
		m_mainWindowNotifier = new FhiLhMainWindowNotifier();
		
		/* Waylandのタッチリスナーを設定する。*/
		pltScreen->setTouchListener(touchListener);

        m_windowInitialized = true;
	}
	else
	{
		m_context->makeCurrent(targetWindow);
	}

	/* PaintDeviceを生成する */
	if (m_device == NULL)
	{
		QSize size = window()->size();
		m_device = new QOpenGLPaintDevice(size);
	}
}

void FhiLhBackingStore::endPaint()
{
	/* PaintDeviceを破棄する */
	if (m_device != NULL)
	{
		delete m_device;
		m_device = NULL;
	}
}

void FhiLhBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
	Q_UNUSED(region);
	Q_UNUSED(offset);

    if ( false == m_windowInitialized )
    {
        qWarning() << "TLH window not initialized error";
        return;
    }

    /* 描画した内容をウィンドウシステムに転送。 */
    m_context->swapBuffers(window);
    
	QScreen *screen = window->screen();
	FhiLhScreen *pltScreen = (FhiLhScreen *)screen->handle();
    pltScreen->displayFlush();
    
    /* ブラウザに描画を通知 */
    m_mainWindowNotifier->notifyDrawing();
}

void FhiLhBackingStore::resize(const QSize &size, const QRegion &)
{
	Q_UNUSED(size);
}

void FhiLhBackingStore::sendErrorEvent( void *openglinfo )
{
	Q_UNUSED(openglinfo);
}

QT_END_NAMESPACE
