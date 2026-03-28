#include "gpio.h"
#include "rcc.h"

#include "adc.h"
#include "gpio.h"
#include "rcc.h"

#include <stdio.h>

#include "stm32f042k6.h"

// Configures SYSCLK/HCLK to use the internal 48 MHz oscillator.
static void clock_init(void) {
    // Enable HSI48 and wait until stable.
    RCC->CR2 |= RCC_CR2_HSI48ON;
    while(!(RCC->CR2 & RCC_CR2_HSI48RDY));

    // Select HSI48 as the system clock source.
    RCC->CFG.SW = RCC_CFG_SW_HSI48;
}

// Enables GPIO clocks and configures debug/indicator pins.
static void rcc_gpio_init(void) {
    // Enable AHB clocks for GPIOA and GPIOB.
    RCC->AHBENR |= RCC_AHBENR_IOPA_EN;
    RCC->AHBENR |= RCC_AHBENR_IOPB_EN;

    // Configure GPIOA pins used for timing instrumentation as outputs.
    GPIOA->MODER.moder3 = gpio_mode_output;
    GPIOA->MODER.moder4 = gpio_mode_output;

    // Configure user LED pin as output.
    GPIOB->MODER.moder3 = gpio_mode_output;
}

// Enables USART2 clocks and configures PA2/PA15 alternate functions.
static void rcc_usart2_init(void) {
    // Enable and reset USART2 to a known default state.
    RCC->APB1ENR |= RCC_APB1ENR_USART2_EN;
    RCC->APB1RSTR |= RCC_APB1RSTR_USART2_RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_USART2_RST;

    // Ensure GPIOA clock is enabled before pin mux programming.
    RCC->AHBENR |= RCC_AHBENR_IOPA_EN;

    // Map PA2 to USART2_TX (AF1) and PA15 to USART2_RX (AF1).
    GPIOA->MODER.moder2 = gpio_mode_alternate_function;
    GPIOA->AFRL.afsel2 = 1;
    GPIOA->MODER.moder15 = gpio_mode_alternate_function;
    GPIOA->AFRH.afsel15 = 1;
}

// Enables ADC clocks and configures analog-capable input pins.
static void rcc_adc_init(void) {
    // Enable and reset ADC peripheral.
    RCC->APB2ENR |= RCC_APB2ENR_ADC_EN;
    RCC->APB2RSTR |= RCC_APB2RSTR_ADC_RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_ADC_RST;

    // Enable the dedicated HSI14 ADC clock and wait for readiness.
    RCC->CR2 |= RCC_CR2_HSI14_ENABLE;
    while(!(RCC->CR2 & RCC_CR2_HSI14_RDY));

    // Configure ADC input pins as analog mode.
    RCC->AHBENR |= RCC_AHBENR_IOPA_EN;
    GPIOA->MODER.moder0 = gpio_mode_analog;
    GPIOA->MODER.moder1 = gpio_mode_analog;
}

// Performs system-level initialization after reset.
void sys_init(void) {
    clock_init();
    rcc_gpio_init();
    rcc_usart2_init();
    rcc_adc_init();
}
