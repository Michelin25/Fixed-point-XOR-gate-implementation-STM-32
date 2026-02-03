#include "systick.h"

static void(*systick_cbfn)(void) = 0;

// Implement a 2 Hz SysTick timer event
void systick_init(void(*cbfn)(void)) {
    // register the callback function
    systick_cbfn = cbfn;

    // Altered to 600 000, this is given by (48MHz/8)*0.1, we are counting number of ticks needed
    // -1 becaue counting starts at 0
    SYSTICK->RVR = 600000-1;
    // Switch to the "external clock source" (HCLK/8 = 6 MHz), 
    // enable the counter,
    // and enable an exception request when the counter reaches 0
    SYSTICK->CSR = (SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT);
}

void SysTick_Handler(void) {
    if( systick_cbfn) 
        systick_cbfn();
}