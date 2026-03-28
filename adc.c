#include "adc.h"
#include "nvic.h"

#include "stm32f042k6.h"

// Registered ADC end-of-conversion callback.
static void (*adc_cf)(ADC_CHANNEL_t channel, uint16_t data) = 0;

// Tracks the channel associated with the active conversion.
static ADC_CHANNEL_t active_channel;

// Initializes ADC sampling, calibration, readiness, and interrupt routing.
void adc_init(void (*adc_cb)(ADC_CHANNEL_t channel, uint16_t data)) {
    // Store application callback for conversion completion events.
    adc_cf = adc_cb;

    // Configure sampling time based on expected source impedance.
    ADC->SMPR = ADC_SMPR_13_5;

    // Start hardware calibration and block until complete.
    ADC->CR |= ADC_CR_ADCAL;
    while(ADC->CR & ADC_CR_ADCAL);

    // Enable ADC and wait until the peripheral reports ready.
    ADC->CR |= ADC_CR_ADEN;
    while(!(ADC->ISR & ADC_ISR_ADRDY));

    // Enable ADC end-of-conversion interrupts.
    ADC->IER |= ADC_IER_EOCIE;

    // Route ADC interrupt requests through NVIC to the core.
    NVIC_ISER |= NVIC_ISER_SETENA_ADC_COMP;
}

// Starts a single conversion on the requested channel.
void adc_convert(ADC_CHANNEL_t channel) {
    active_channel = channel;

    // Select active input channel and trigger conversion.
    ADC->CHSELR = channel;
    ADC->CR |= ADC_CR_ADSTART;
}

// ADC interrupt service routine for end-of-conversion events.
void ADC1_IRQHandler(void) {
    // Use bitmask testing because ISR contains multiple independent status bits.
    if (ADC->ISR & ADC_ISR_EOC) {
        gpio_pin_set(GPIOA, gpio_pin_4);

        // Reading DR returns the completed sample value.
        uint16_t data = (uint16_t)ADC->DR;

        // Dispatch sample to the registered callback if provided.
        if (adc_cf != 0) {
            adc_cf(active_channel, data);
        }

        // Clear EOC in software for deterministic ISR behavior.
        ADC->ISR = ADC_ISR_EOC;
        gpio_pin_reset(GPIOA, gpio_pin_4);
    }
}
