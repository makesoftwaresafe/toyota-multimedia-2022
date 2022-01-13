/*
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "JSNavigator.h"

#include "Dictionary.h"
#include "ExceptionCode.h"

#if ENABLE(MEDIA_STREAM)
#include "JSNavigatorUserMediaErrorCallback.h"
#include "JSNavigatorUserMediaSuccessCallback.h"
#include "NavigatorMediaStream.h"
#endif /* ENABLE(MEDIA_STREAM) */

#if ENABLE(FJIB_VIAPI)
#include "NavigatorVehicle.h"
#include "JSVehicle.h"
#endif /* ENABLE(FJIB_VIAPI) */

#if ENABLE(FJIB_AMB)
#include "NavigatorFjIbAMB.h"
#include "JSAMBInterface.h"
#endif /* ENABLE(FJIB_AMB) */

using namespace JSC;

namespace WebCore {

#if ENABLE(MEDIA_STREAM)
JSValue JSNavigator::webkitGetUserMedia(ExecState* exec)
{
    if (exec->argumentCount() < 2) {
        throwVMError(exec, createNotEnoughArgumentsError(exec));
        return jsUndefined();
    }

    Dictionary options(exec, exec->argument(0));
    if (exec->hadException())
        return jsUndefined();

    if (!options.isObject()) {
        throwVMError(exec, createTypeError(exec, "First argument of webkitGetUserMedia must be a valid Dictionary"));
        return jsUndefined();
    }

    if (!exec->argument(1).isFunction()) {
        throwVMError(exec, createTypeError(exec, "Argument 2 ('successCallback') to Navigator.webkitGetUserMedia must be a function"));
        return jsUndefined();
    }

    JSNavigator* castedThis = jsDynamicCast<JSNavigator*>(exec->hostThisValue());
    RefPtr<NavigatorUserMediaErrorCallback> errorCallback;
    if (!exec->argument(2).isUndefinedOrNull()) {
        if (!exec->argument(2).isFunction()) {
            throwVMError(exec, createTypeError(exec, "Argument 3 ('errorCallback') to Navigator.webkitGetUserMedia must be a function"));
            return jsUndefined();
        }
        errorCallback = JSNavigatorUserMediaErrorCallback::create(asObject(exec->argument(2)), castedThis->globalObject());
    }

    RefPtr<NavigatorUserMediaSuccessCallback> successCallback = JSNavigatorUserMediaSuccessCallback::create(asObject(exec->argument(1)), castedThis->globalObject());
    Navigator& impl = castedThis->impl();
    ExceptionCode ec = 0;
    NavigatorMediaStream::webkitGetUserMedia(&impl, options, successCallback, errorCallback, ec);
    setDOMException(exec, ec);
    return jsUndefined();
}
#endif /* ENABLE(MEDIA_STREAM) */

#if ENABLE(FJIB_VIAPI)
JSValue JSNavigator::vehicle(ExecState* exec) const {
	Navigator& navigator = impl();

	PassRefPtr<Vehicle> vehicle = NavigatorVehicle::vehicle(&navigator);

	JSValue result;
	if(vehicle) {
		result = toJS(exec, globalObject(), WTF::getPtr(vehicle));
	}
	else {
		result = jsUndefined();
	}
	return result;
}
#endif /* ENABLE(FJIB_VIAPI) */

#if ENABLE(FJIB_AMB)
JSValue JSNavigator::fjibamb(ExecState* exec) const {
	Navigator& navigator = impl();

	PassRefPtr<AMBInterface> ambInterface = NavigatorFjIbAMB::fjibamb(&navigator);

	JSValue result;
	if(ambInterface) {
		result = toJS(exec, globalObject(), WTF::getPtr(ambInterface));
	}
	else {
		result = jsUndefined();
	}
	return result;
}
#endif /* ENABLE(FJIB_AMB) */

} // namespace WebCore


