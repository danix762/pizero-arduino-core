#include "Arduino.h"
#include "usb/usb_core.h"
#include "cache.h"
#include "irq.h"
#include "usb_bg.h"

extern "C" {
    typedef void (*ctor_fn)(void);
    extern ctor_fn __preinit_array_start[];
    extern ctor_fn __preinit_array_end[];
    extern ctor_fn __init_array_start[];
    extern ctor_fn __init_array_end[];
}

static void run_global_constructors(void) {
    for (ctor_fn *f = __preinit_array_start; f < __preinit_array_end; f++) (*f)();
    for (ctor_fn *f = __init_array_start;    f < __init_array_end;    f++) (*f)();
}

int main(void) {
    icache_enable();
    mmu_enable();
    irq_init();                   // vector table, fault handler, interrupt controller
    run_global_constructors();    // global C++ objects
    usb_start();
    usb_bg_start();               // no-op unless USB_BG_ENABLE is 1
    setup();
    while (1) {
        usb_service_main();       // no-op if USB_MAIN_SERVICE is 0
        loop();
    }
    return 0;
}
