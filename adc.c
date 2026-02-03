#include "adc.h"
#include "nvic.h"

#include "stm32f042k6.h"

//Registering the callback function handling interupts
static void (*adc_cf)(ADC_CHANNEL_t channel, uint16_t data) = 0;


static ADC_CHANNEL_t active_channel;


// enable the ADC and any IO pins used by the ADC using the bus clock (8 Mhz)
// configure for 12-bit sampling mode
// Taking in a callback function helping with data manipulation
void adc_init(void (*adc_cb)(ADC_CHANNEL_t channel, uint16_t data)) {
    //Declaring the call back function 
    adc_cf = adc_cb;

    // Set the correct sampling time for the input signal's impedance
    ADC->SMPR = ADC_SMPR_13_5;

    // Calibrate the ADC
    ADC->CR |= ADC_CR_ADCAL;
    // hardware clears the bit when calibration completes
    // if your program hangs and you hit break & are sitting on this 
    // line something is wrong with your request to calibrate... 
    // check the requirements in the user manual!
    while( ADC->CR & ADC_CR_ADCAL);

    // Enable the ADC
    ADC->CR |= ADC_CR_ADEN;

    // Wait for the ADC to become ready
    while( !(ADC->ISR & ADC_ISR_ADRDY) );

    //Enabling end of conversion interupts
    ADC->IER |= ADC_IER_EOCIE;
    //Enabling the ADC interupt to reach the M0 core
    NVIC_ISER |= NVIC_ISER_SETENA_ADC_COMP;
} 

// Conversion from the selected channel called by the callback function 
void adc_convert(ADC_CHANNEL_t channel) {
    //declaring the active channerl
    active_channel = channel;
    
    // Configure the channel selection register 
    ADC->CHSELR = channel;

    // Start the conversion
    ADC->CR |= ADC_CR_ADSTART;

}

//interupt handler ADC
void ADC1_IRQHandler(void){
    
    if (ADC->ISR & ADC_ISR_EOC){ // Checking interupt source, Why can't I use == for checking?
        gpio_pin_set(GPIOA, gpio_pin_4); 
        uint16_t data =  (uint16_t)ADC->DR; // Readeing the converted data
        if (adc_cf !=0) {          //Checking if cb function is defined
            adc_cf(active_channel, data);
        
        }
        ADC->ISR = ADC_ISR_EOC; //The end of conversion flag should be automaticall cleared after data is read
        gpio_pin_reset(GPIOA, gpio_pin_4);
    }
    
    
}