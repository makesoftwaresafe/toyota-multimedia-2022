/*
 * Copyright (C) 2008, 2009, 2010 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Jan Michael Alonzo
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
#include "AccessibilityController.h"

#if HAVE(ACCESSIBILITY)

#include "AccessibilityCallbacks.h"
#include "AccessibilityUIElement.h"
#include "DumpRenderTree.h"
#include "DumpRenderTreeChrome.h"
#include "WebCoreSupport/DumpRenderTreeSupportEfl.h"

#include <atk/atk.h>
#include <wtf/gobject/GUniquePtr.h>

AccessibilityUIElement AccessibilityController::focusedElement()
{
    AtkObject* accessible =  DumpRenderTreeSupportEfl::focusedAccessibleElement(browser->mainFrame());
    if (!accessible)
        return 0;

    return AccessibilityUIElement(accessible);
}

AccessibilityUIElement AccessibilityController::rootElement()
{
    AtkObject* accessible = DumpRenderTreeSupportEfl::rootAccessibleElement(browser->mainFrame());
    if (!accessible)
        return 0;

    return AccessibilityUIElement(accessible);
}

AccessibilityUIElement AccessibilityController::accessibleElementById(JSStringRef id)
{
    AtkObject* root = DumpRenderTreeSupportEfl::rootAccessibleElement(browser->mainFrame());
    if (!root)
        return 0;

    size_t bufferSize = JSStringGetMaximumUTF8CStringSize(id);
    GUniquePtr<gchar> idBuffer(static_cast<gchar*>(g_malloc(bufferSize)));
    JSStringGetUTF8CString(id, idBuffer.get(), bufferSize);

    AtkObject* result = childElementById(root, idBuffer.get());
    if (ATK_IS_OBJECT(result))
        return AccessibilityUIElement(result);

    return 0;
}

#endif
