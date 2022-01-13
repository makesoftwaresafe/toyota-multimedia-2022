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

#include "fhilhintegration.h"
#include "fhilhbackingstore.h"
#include "fhilhscreen.h"

#include <qpa/qplatformwindow.h>
#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtGui/QScreen>

QT_BEGIN_NAMESPACE

static const char debugBackingStoreEnvironmentVariable[] = "QT_DEBUG_BACKINGSTORE";

static inline unsigned parseOptions(const QStringList &paramList)
{
    unsigned options = 0;
    foreach (const QString &param, paramList) {
        if (param == QLatin1String("enable_fonts"))
            options |= FhiLhIntegration::EnableFonts;
    }
    return options;
}

FhiLhIntegration::FhiLhIntegration(const QStringList &parameters)
    : m_dummyFontDatabase(new QGenericUnixFontDatabase())
    , m_options(parseOptions(parameters))
{
    if (qEnvironmentVariableIsSet(debugBackingStoreEnvironmentVariable)
        && qgetenv(debugBackingStoreEnvironmentVariable).toInt() > 0) {
        m_options |= DebugBackingStore | EnableFonts;
    }

    FhiLhScreen *m_primary_screen = new FhiLhScreen();
    screenAdded(m_primary_screen);
}

FhiLhIntegration::~FhiLhIntegration()
{
    delete m_dummyFontDatabase;
}

bool FhiLhIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case MultipleWindows: return true;
    case OpenGL         : return true;
    case ThreadedOpenGL : return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *FhiLhIntegration::createPlatformWindow(QWindow *window) const
{
    QPlatformWindow *w = new QPlatformWindow(window);
	window->setSurfaceType(QSurface::OpenGLSurface);
    w->requestActivateWindow();
    return w;
}

QPlatformBackingStore *FhiLhIntegration::createPlatformBackingStore(QWindow *window) const
{
	return new FhiLhBackingStore(window);
}

QAbstractEventDispatcher *FhiLhIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformFontDatabase *FhiLhIntegration::fontDatabase() const
{
    return m_dummyFontDatabase;
}

QPlatformOpenGLContext *FhiLhIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
	FhiLhScreen *pltScreen = (FhiLhScreen *)context->screen()->handle();
	QPlatformOpenGLContext *glContext = pltScreen->createAndSetPlatformContext();
	return glContext;
}


QT_END_NAMESPACE
