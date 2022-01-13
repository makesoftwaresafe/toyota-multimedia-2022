/*** COPYRIGHT FUJITSU LIMITED 2015 ***/

/*  workaround for playcanvas */

#include "config.h"

#if ENABLE(GAMEPAD)

#include "JSGamepad.h"
#include "Gamepad.h"
#include <runtime/JSArray.h>
#include <runtime/JSString.h>
#include <wtf/GetPtr.h>

using namespace JSC;

namespace WebCore {

JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, Gamepad* impl)
{
    if (!impl)
        return jsUndefined();
    if (JSValue result = getExistingWrapper<JSGamepad>(exec, impl))
        return result;
#if COMPILER(CLANG)
    // If you hit this failure the interface definition has the ImplementationLacksVTable
    // attribute. You should remove that attribute. If the class has subclasses
    // that may be passed through this toJS() function you should use the SkipVTableValidation
    // attribute to Gamepad.
    COMPILE_ASSERT(!__is_polymorphic(Gamepad), Gamepad_is_polymorphic_but_idl_claims_not_to_be);
#endif
    ReportMemoryCost<Gamepad>::reportMemoryCost(exec, impl);
    return createNewWrapper<JSGamepad>(exec, globalObject, impl);
}

} // namespace WebCore

#endif /* ENABLE(GAMEPAD) */
