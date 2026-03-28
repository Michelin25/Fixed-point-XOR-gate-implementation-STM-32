#include <stdint.h>
#include <stdio.h>

#include "adc.h"
#include "led.h"
#include "sysinit.h"
#include "systick.h"
#include "usart.h"

#include "nn.h"

#include "stm32f042k6.h"


// Redirects printf output to USART2.
int __io_putchar(int data) {
  usart2_tx((uint8_t)data);
  return data;
}

// SysTick callback registration target.
static volatile uint16_t sys_tick = 0;   // SysTick tick counter.
volatile bool run_main = false;          // Main-loop execution flag.

// Handles periodic SysTick events.
void systick_callback_function(void) {
    gpio_pin_set(GPIOA, gpio_pin_3);

    run_main = true;                     // Schedules one main-loop execution.

    if ((sys_tick % 5) == 0) {           // Toggle heartbeat every five ticks.
        led_toggle(LED_USER);
    }
    sys_tick++;
    gpio_pin_reset(GPIOA, gpio_pin_3);
}

// USART receive event indicator set by interrupt callback.
volatile bool keypressed = false;
void usart2_rx_callback_function(uint8_t rx_data) {
    keypressed = true;
}

// ADC conversion callback:
// - Stores CH0 result and starts CH1 conversion.
// - Stores CH1 result and flags prediction execution.
volatile static uint16_t ch0; // Latest CH0 conversion result.
volatile static uint16_t ch1; // Latest CH1 conversion result.
static volatile bool run_prediction = false; // Prediction request flag.
void adc_callback_function(ADC_CHANNEL_t channel, uint16_t data) {
    gpio_pin_set(GPIOA, gpio_pin_4);
    switch(channel) {
        case ADC_CH0:
            ch0 = data;
            adc_convert(ADC_CH1);
            break;
        case ADC_CH1:
            ch1 = data;
            run_prediction = true;
            break;
        default:
            // No action required for unsupported channel IDs.
    }
    gpio_pin_reset(GPIOA, gpio_pin_4);
}

int main(void) {

    // Qm.n input vector passed to NN_qpredict().
    int8_t qinputs[NN_INPUTS];
    // Qm.n prediction returned from NN_qpredict().
    int8_t qresult;

    // Initializes clocks, peripherals, and pin multiplexing.
    sys_init();

    // Starts SysTick (2 Hz) and registers callback handler.
    systick_init(systick_callback_function);


    // Configures USART2: 115200 baud, 8 data bits, no parity, 1 stop bit.
    usart2_init(usart2_rx_callback_function);

    // Enables and configures ADC operation.
    adc_init(adc_callback_function);

    // Enables global interrupt handling in the Cortex-M core.
    __asm("cpsie i");

    // Prints startup banner.
    printf("Lab 4: Quantized NN - press any key:\n");

    while(1) {
        if(run_main) {

            // Starts ADC sampling on CH0; callback chains CH1 conversion.
            adc_convert(ADC_CH0);


            // Scales 12-bit ADC values (0..4095) into Q4.3-compatible integer inputs.
            // Integer multiplication is performed before division to preserve precision.
            volatile uint16_t ch01 = (ch0 * 8)/4095; // Multiply-first scaling path.
            volatile uint16_t ch11 = (ch1 * 8)/4095;

            // Narrows normalized values into int8_t Qm.n representation.
            qinputs[0] = (int8_t)ch01;
            qinputs[1] = (int8_t)ch11;

            // Executes quantized neural-network inference.
            qresult = NN_qpredict(qinputs);


            // Converts fixed-point output to percent scale using integer arithmetic only.
            int16_t result = ((int16_t)qresult * 100) / 8;

            // Logs scaled inputs and inference result.
            printf("count: %d,in[0]: %d, in[1]: %d, result: %d\n",sys_tick, ch01, ch11, result);

            // Clears scheduler flag after one loop iteration.
            run_main = false;
        }
    }
}
