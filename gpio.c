
#include <stdlib.h>

#include "gpio.h"
#include "retval.h"

// Reads a GPIO pin state into the provided output pointer.
ret_val_t gpio_pin_get(GPIO_t *port, gpio_pin_t pin, bool *state) {
    // Validate required pointers before register access.
    if((port == NULL) || (state == NULL)) {
        return RET_GPIO_ERR_NULL_PTR;
    }

    *state = port->IDR & pin;
    return RET_SUCCESS;
}

// Sets a GPIO output pin.
ret_val_t gpio_pin_set(GPIO_t *port, gpio_pin_t pin) {
    if(port == NULL) {
        return RET_GPIO_ERR_NULL_PTR;
    }

    port->BSRR = pin;
    return RET_SUCCESS;
}

// Clears a GPIO output pin.
ret_val_t gpio_pin_reset(GPIO_t *port, gpio_pin_t pin) {
    if(port == NULL) {
        return RET_GPIO_ERR_NULL_PTR;
    }

    port->BRR = pin;
    return RET_SUCCESS;
}

// Toggles a GPIO pin based on its current input register state.
ret_val_t gpio_pin_toggle(GPIO_t *port, gpio_pin_t pin) {
    if(port == NULL) {
        return RET_GPIO_ERR_NULL_PTR;
    }

    if(port->IDR & pin) {
        gpio_pin_reset(port, pin);
    } else {
        gpio_pin_set(port, pin);
    }

    return RET_SUCCESS;
}
