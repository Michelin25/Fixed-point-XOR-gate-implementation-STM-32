#include "usart.h"
#include "stm32f042k6.h"

#include <stdint.h>

#include "nvic.h"

// Optional callback invoked for each received USART2 byte.
static void(*usart_rx_cbfn_ptr)(uint8_t data) = 0;

void USART2_IRQHandler(void) {
    // Service receive-not-empty interrupt events.
    if(USART2->ISR & USART_ISR_RXNE) {
        // Clear overrun status if present.
        if(USART2->ISR & USART_ISR_ORE) {
            USART2->ICR = USART_ICR_ORECF;
        }

        // Reading RDR acknowledges RXNE and returns received data.
        uint8_t rx_data = USART2->RDR;

        // Dispatch to the registered callback when available.
        if(usart_rx_cbfn_ptr) {
            usart_rx_cbfn_ptr(rx_data);
        }
    }
}

void usart2_init(void(*usart_rx_cbfn)(uint8_t data)) {
    // Configure 115200 baud for 48 MHz peripheral clock.
    USART2->BRR = 0x1A1;

    // Register optional RX callback.
    usart_rx_cbfn_ptr = usart_rx_cbfn;

    // Enable USART receive interrupt generation.
    USART2->CR1 |= USART_CR1_RXNEIE;

    // Enable USART2 interrupt routing through NVIC.
    NVIC_ISER |= NVIC_ISER_SETENA_USART2;

    // Enable USART2 peripheral, RX, and TX paths.
    USART2->CR1 |= USART_CR1_UE;
    USART2->CR1 |= USART_CR1_RE;
    USART2->CR1 |= USART_CR1_TE;
}

// Transmits one byte over USART2.
void usart2_tx(uint8_t tx_data) {
    // Wait until transmit data register is empty.
    while(!(USART2->ISR & USART_ISR_TXE));

    USART2->TDR = tx_data;
}
