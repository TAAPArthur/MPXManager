#include "../bindings.h"
#include "../devices.h"
#include "../globals.h"
#include "../windows.h"
#include "../xutil/test-functions.h"
#include "test-event-helper.h"
#include "test-wm-helper.h"
#include "tester.h"
#include <stdlib.h>



SCUTEST_SET_ENV(onSimpleStartup, simpleCleanup);
SCUTEST_ITER(test_accessible_settings, 2) {
    for(int i = 0; i < getBindings()->size; i++) {
        const Binding* binding = getElement(getBindings(), i);
        if (binding->flags.noShortCircuit)
            continue;
        BindingEvent event = {binding->mod, binding->detail, binding->flags.mask};
        for(int j = i + 1; j < getBindings()->size; j++) {
            assert(!matches( getElement(getBindings(), j), &event));
        }
    }
}
