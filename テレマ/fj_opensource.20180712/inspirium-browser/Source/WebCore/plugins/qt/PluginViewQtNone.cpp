/*
 * Copyright (C) 2008 Kevin Ollivier <kevino@theolliviers.com> All Rights Reserved.
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

#include "config.h"
#include "PluginView.h"
#include "FrameView.h"

#if USE(JSC)
#include "BridgeJSC.h"
#include <runtime/JSObject.h>
//#include <runtime/ScopeChain.h>
#endif

using namespace WTF;

namespace WebCore {

bool PluginView::s_isRunningUnderDRT = false;

void PluginView::setFocus(bool focused)
{
#if !HAVE(QT5) // Windowed mode is not supported with Qt5 yet (so platformPluginWidget() is always null). 
	if (platformPluginWidget()) { 
		if (focused) 
			static_cast<QWidget*>(platformPluginWidget())->setFocus(Qt::OtherFocusReason); 
		} else 
#endif 
	Widget::setFocus(focused);
}

void PluginView::show()
{

}

void PluginView::hide()
{

}

void PluginView::paint(GraphicsContext* context, const IntRect& rect)
{
    if (!m_isStarted) {
/**/
        context->save();
        context->fillRect(frameRect(),Color("gray"),ColorSpaceDeviceRGB);
        context->restore();
/**/
        paintMissingPluginIcon(context, rect);
        return;
    }
}

void PluginView::handleKeyboardEvent(KeyboardEvent*)
{

}

void PluginView::handleMouseEvent(MouseEvent*)
{

}

void PluginView::setParent(ScrollView* parent)
{
    Widget::setParent(parent);

    if (parent) {
        init();
	}
}

void PluginView::setNPWindowRect(const IntRect&)
{

}

#if ENABLE(NETSCAPE_PLUGIN_API)
NPError PluginView::handlePostReadFile(Vector<char>& buffer, uint32_t len, const char* buf)
{
    String filename(buf, len);

    if (filename.startsWith("file:///"))
        filename = filename.substring(8);

    long long size;
    if (!getFileSize(filename, size))
        return NPERR_FILE_NOT_FOUND;

    FILE* fileHandle = fopen((filename.utf8()).data(), "r");
    if (!fileHandle)
        return NPERR_FILE_NOT_FOUND;

    buffer.resize(size);
    int bytesRead = fread(buffer.data(), 1, size, fileHandle);

    fclose(fileHandle);

    if (bytesRead <= 0)
        return NPERR_FILE_NOT_FOUND;

    return NPERR_NO_ERROR;
}

bool PluginView::platformGetValue(NPNVariable variable, void* value, NPError* result)
{
    return false;
}

bool PluginView::platformGetValueStatic(NPNVariable variable, void* value, NPError* result)
{
    switch (variable) {
    case NPNVToolkit:
        *static_cast<uint32_t*>(value) = 0;
        *result = NPERR_NO_ERROR;
        return true;

    case NPNVjavascriptEnabledBool:
        *static_cast<NPBool*>(value) = true;
        *result = NPERR_NO_ERROR;
        return true;

    default:
        return false;
    }
}

void PluginView::invalidateRect(NPRect* rect)
{

}
#endif

void PluginView::invalidateRect(const IntRect&)
{

}

#if ENABLE(NETSCAPE_PLUGIN_API)
void PluginView::invalidateRegion(NPRegion)
{

}
#endif

void PluginView::forceRedraw()
{

}

bool PluginView::platformStart()
{
    return true;
}

void PluginView::platformDestroy()
{

}

void PluginView::setParentVisible(bool)
{

}

void PluginView::updatePluginWidget()
{
    if (!parent())
        return;

    ASSERT(parent()->isFrameView());
    FrameView* frameView = toFrameView(parent());

    IntRect oldWindowRect = m_windowRect;
    IntRect oldClipRect = m_clipRect;

    m_windowRect = IntRect(frameView->contentsToWindow(frameRect().location()), frameRect().size());
    m_clipRect = windowClipRect();
    m_clipRect.move(-m_windowRect.x(), -m_windowRect.y());

    if (m_windowRect == oldWindowRect && m_clipRect == oldClipRect)
        return;

    // The plugin had a zero width or height before but was resized, we need to show it again.
    if (oldWindowRect.isEmpty())
        show();

    // do not call setNPWindowIfNeeded immediately, will be called on paint()
    m_hasPendingGeometryChange = true;

    invalidate();
}

#if 0 //
void PluginView::halt()
{

}

void PluginView::restart()
{

}
#endif

#if defined(XP_UNIX) && ENABLE(NETSCAPE_PLUGIN_API)
void PluginView::handleFocusInEvent()
{

}

void PluginView::handleFocusOutEvent()
{

}
#endif

#if 0
PlatformLayer* PluginView::platformLayer() const
{
    return m_platformLayer ? m_platformLayer.get() : NULL;
}
#endif

} // namespace WebCore

