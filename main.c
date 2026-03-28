#include <stdint.h>
#include <stdio.h>

#include "adc.h"
#include "led.h"
#include "sysinit.h"
#include "systick.h"
#include "usart.h"

#include "nn.h"

#include "stm32f042k6.h"


// Send a character to the terminal window
int __io_putchar(int data) {
  usart2_tx((uint8_t)data);
  return data;
}

// Callback function for systick exceptions registered in systick_init()

static volatile uint16_t sys_tick = 0;   // SysTick counter
volatile bool run_main = false;          // main loop flag

void systick_callback_function(void) {   // SysTick callback
    gpio_pin_set(GPIOA, gpio_pin_3);    
    
    run_main = true;                     // trigger main loop

    if ((sys_tick % 5) == 0) {           // every 5 ticks
        led_toggle(LED_USER);        
    }
    sys_tick++;
    gpio_pin_reset(GPIOA, gpio_pin_3);   

}

// Callback function and global flag for USART receive data events,
// set the 'keypressed' flag indicating a key was pressed in the serial terminal
volatile bool keypressed = false;
void usart2_rx_callback_function(uint8_t rx_data) {
    keypressed = true;
}
// Callback function for ADC end of conversion events
// If the converted channel is channel 0, save the results 
// and start a conversion on channel 1.  If the conversion
// is channel 1, save the results and signal main() to 
// perform a prediction.
volatile static uint16_t ch0; // results of last channel 0 conversion
volatile static uint16_t ch1; // results of last channel 1 conversion
static volatile bool run_prediction = false; // flag for main()
void adc_callback_function(ADC_CHANNEL_t channel, uint16_t data) {
    gpio_pin_set(GPIOA, gpio_pin_4);    
        switch(channel) {
        //gpio_pin_reset(GPIOA, gpio_pin_4);
        case ADC_CH0:
            ch0 = data;
            adc_convert(ADC_CH1);
            break;
        case ADC_CH1:
            ch1 = data;
            run_prediction = true;
            //gpio_pin_reset(GPIOA, gpio_pin_4);
            break;
        default:
            // Shouldn't ever happen!
    }
    gpio_pin_reset(GPIOA, gpio_pin_4);  
    
}

int main(void) {

    // Qm.n inputs passed to NN_qpredict()
    int8_t qinputs[NN_INPUTS];
    // Qm.n result returned from NN_qpredict()
    int8_t qresult;

    // Initialize clocks/peripherals and configure I/O
    sys_init();

    // Start the systick timer (2 Hz) and call the
    // systick_callback_function on timer events:

    systick_init(systick_callback_function);


    // Configure usart2 for 115200 baud, 8 data, no parit, 1 stop
    // do not register a callback handler for any received data at this time
    // (any received data will be dropped)
    usart2_init(usart2_rx_callback_function);

    // Enable the ADC
 
    adc_init(adc_callback_function);

    // Enable exception/interrupt handling in the processor core:
    __asm("cpsie i");

    // Banner
    printf("Lab 4: Quantized NN - press any key:\n");

    while( 1 ) {
        if( run_main ) {
           
            // Sample the analog signal on Port A Pin 0, returns a "raw counts" value
            // in the range 0-4095 based on an input voltage in the range 0 - 3.3 V
            adc_convert(ADC_CH0);


            // We want to scale the 12-bit ADC inputs, range 0-4095, into 
            // an appropriately scaled int8_t inputs to the quantized NN
            // using the quantization scalar QNN_SCALE_FACTOR from the programming assignment
            //
            // Previously we normalized the ADC sample with: (float)ch0/4095.0 into the range 0.0-1.0
            // but we do not want to do floating point division - we want all float gone!
            //
            // So first we scale the ADC sample value _up_ with the quantization scale factor.
            // We have to be careful here because we are using 12 of 16 bits 
            // in ch0 and ch1 (since they are both uint16_t) - so there are only 4 free bits.
            // Our scale factor cannot be larger than 16 (2^4) or we could overflow a uint16_t.  
            // If your scale factor is > 16, change ch0 & ch1 types to uint32_t!
            //
            // My scale factor is 16, so I get away with leaving ch0 and ch1 as uint16_t, 
            // but if your scale factor is larger you need to switch to uint32_t for ch0 & ch1!
            //
            // Think of the following as (ch0/4095) * QNN_SCALE_FACTOR, but we have to multiply first.
            // If the maximum ch0 value is 4095, and we did (ch0/4095) as integer division
            // we would get 1 if ch0 = 4095, or 0 if ch0 < 4095 (because it is integer division)
            // Instead we scale _up_ first, then divide, leaving us with a Q3.4 result!


            volatile uint16_t ch01 = (ch0 * 8)/4095; // force multiplication first, then division!
            volatile uint16_t ch11 = (ch1 * 8)/4095;

            // Now cast these at int8_t - we can discard the extra bits because we've just 
            // normalized the Qm.n in int8_t to represent the value 0.0 -> 1.0
            // Range is 0 to 8, with 3 decimal bits.
            qinputs[0] = (int8_t)ch01;
            qinputs[1] = (int8_t)ch11;

            // Predict! (and toggle GPIO around the prediction call so we can time the
            // execution time of the quantized NN_qpredict() function)
            
            qresult = NN_qpredict(qinputs);
            

            // Instead of presenting the results in the dequantized range 0-1, which would
            // require floating point, we will present the results "* 100" so we can use
            // integer arithmetic to dequantize to the scaled up result and so we do not 
            // need the code bloat associated with supporting floating point numbers in printf()
            // We'll use an int16_t because we are multplying a Qm.n by a number that requires 6 bits
            // to encode, so the intermediate value is Qm+6.n which is larger than 8 bits!
            int16_t result = ((int16_t)qresult * 100) / 8;

            // Display the Qm.n inputs and result using signed integer format (printf() float support is not enabled!)
            printf("count: %d,in[0]: %d, in[1]: %d, result: %d\n",sys_tick, ch01, ch11, result);

            // Clear the 'keypressed' flag
            run_main = false;
        }
    }
}