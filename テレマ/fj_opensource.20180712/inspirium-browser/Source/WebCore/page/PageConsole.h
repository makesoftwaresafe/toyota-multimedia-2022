/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PageConsole_h
#define PageConsole_h

#include <inspector/ConsoleTypes.h>
#include <inspector/ScriptCallStack.h>
#include <wtf/Forward.h>
#include <wtf/PassOwnPtr.h>

#if !USE(V8)
namespace JSC {
class ExecState;
}
#endif

namespace WebCore {

class Document;
class Page;
#if USE(V8)
class ScriptState;
#endif

class PageConsole {
public:
    PageConsole(Page&);
    ~PageConsole();

    static void printSourceURLAndPosition(const String& sourceURL, unsigned lineNumber, unsigned columnNumber = 0);
    static void printMessageSourceAndLevelPrefix(MessageSource, MessageLevel, bool showAsTrace = false);
#if USE(V8)
	void addMessage(MessageSource, MessageLevel, const String& message, const String& sourceURL, unsigned lineNumber, unsigned columnNumber, PassRefPtr<Inspector::ScriptCallStack> = 0, ScriptState* = 0, unsigned long requestIdentifier = 0);
#else
    void addMessage(MessageSource, MessageLevel, const String& message, const String& sourceURL, unsigned lineNumber, unsigned columnNumber, PassRefPtr<Inspector::ScriptCallStack> = 0, JSC::ExecState* = 0, unsigned long requestIdentifier = 0);
#endif
    void addMessage(MessageSource, MessageLevel, const String& message, PassRefPtr<Inspector::ScriptCallStack>);
    void addMessage(MessageSource, MessageLevel, const String& message, unsigned long requestIdentifier = 0, Document* = 0);

    static void mute();
    static void unmute();

    static bool shouldPrintExceptions();
    static void setShouldPrintExceptions(bool);

private:
    Page& m_page;
};

} // namespace WebCore

#endif // PageConsole_h
