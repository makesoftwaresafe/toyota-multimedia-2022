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

#ifndef FHILHBACKINGSTORE_H
#define FHILHBACKINGSTORE_H

#include <qpa/qplatformbackingstore.h>
#include "fhilhevnethandler.h"

QT_BEGIN_NAMESPACE

class QOpenGLContext;
class QOpenGLPaintDevice;
class FhiLhMainWindowNotifier;
class FhiLhBackingStore : public QPlatformBackingStore
{
public:
    FhiLhBackingStore(QWindow *window);
    ~FhiLhBackingStore();

    QPaintDevice *paintDevice();
    void flush(QWindow *window, const QRegion &region, const QPoint &offset);
    void resize(const QSize &size, const QRegion &staticContents);
    void beginPaint(const QRegion &);
    void endPaint();

private:
    bool m_windowInitialized;   /* 描画ウィンドウの準備ができているかを示す */

    QOpenGLContext* m_context;
    QOpenGLPaintDevice* m_device;
    static void touchListener(int type, int x1, int y1, int x2, int y2);
    void sendErrorEvent(void *openglinfo );
    FhiLhMainWindowNotifier* m_mainWindowNotifier;

	FhiLhEventHandler eventHandler;
};

QT_END_NAMESPACE

#endif
