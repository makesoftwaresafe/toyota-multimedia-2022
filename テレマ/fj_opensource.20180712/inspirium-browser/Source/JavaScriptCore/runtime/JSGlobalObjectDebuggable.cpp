/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "JSGlobalObjectDebuggable.h"

#if ENABLE(REMOTE_INSPECTOR)

#include "APIShims.h"
#include "InspectorAgentBase.h"
#include "InspectorFrontendChannel.h"
#include "JSGlobalObject.h"
#include "RemoteInspector.h"

using namespace Inspector;

namespace JSC {

JSGlobalObjectDebuggable::JSGlobalObjectDebuggable(JSGlobalObject& globalObject)
    : m_globalObject(globalObject)
{
}

JSGlobalObjectDebuggable::~JSGlobalObjectDebuggable()
{
    if (m_inspectorController)
        disconnectInternal(InspectorDisconnectReason::InspectedTargetDestroyed);
}

String JSGlobalObjectDebuggable::name() const
{
    String name = m_globalObject.name();
    return name.isEmpty() ? ASCIILiteral("JSContext") : name;
}

void JSGlobalObjectDebuggable::connect(InspectorFrontendChannel* frontendChannel)
{
    APIEntryShim entryShim(&m_globalObject.vm());

    ASSERT(!m_inspectorController);
    m_inspectorController = std::make_unique<Inspector::JSGlobalObjectInspectorController>(m_globalObject);
    m_inspectorController->connectFrontend(frontendChannel);
}

void JSGlobalObjectDebuggable::disconnect()
{
    disconnectInternal(InspectorDisconnectReason::InspectorDestroyed);
}

void JSGlobalObjectDebuggable::disconnectInternal(InspectorDisconnectReason reason)
{
    APIEntryShim entryShim(&m_globalObject.vm());

    m_inspectorController->disconnectFrontend(reason);
    m_inspectorController = nullptr;
}

void JSGlobalObjectDebuggable::dispatchMessageFromRemoteFrontend(const String& message)
{
    APIEntryShim entryShim(&m_globalObject.vm());

    m_inspectorController->dispatchMessageFromFrontend(message);
}

} // namespace JSC

#endif // ENABLE(REMOTE_INSPECTOR)
