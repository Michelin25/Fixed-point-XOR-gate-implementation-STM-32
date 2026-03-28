#include "systick.h"

static void(*systick_cbfn)(void) = 0;

// Configures SysTick to generate periodic interrupt callbacks.
void systick_init(void(*cbfn)(void)) {
    // Register callback function.
    systick_cbfn = cbfn;

    // Configure reload value for 0.1 s period using HCLK/8.
    SYSTICK->RVR = 600000 - 1;

    // Use HCLK/8 clock source, enable SysTick, and enable interrupt.
    SYSTICK->CSR = (SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT);
}

void SysTick_Handler(void) {
    if(systick_cbfn)
        systick_cbfn();
}
