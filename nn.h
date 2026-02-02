#pragma once

#include <stdint.h>

// Hard-coded for the NN implementation in nn.c
#define NN_INPUTS (2)

// Quantize the floating-point based NN architecture to work with int8_t,
// which is an 8-bit signed value ranging from -128 to 127.  Observe the 
// maximum/minium value through your NN.  
//
// What is the magnitude of the largest/smallest scale factor?  Mine is 3.8
// What is the magnitude of the largest/smallest input?  Mine is 1.1 (test vectors)
// What is the magnitude of the largest/smallest bias?  Mine is 2
// 
// With two inputs, scaled and summed, the largest magnitude my NN needs to represent
// is 2 * 3.8 * 1.1 + 2.0 = 16.72
//
// This says I need 5 bits to encode the integer portion (2^4-1 = 15, not enough range),
// plus 1 bit for the sign, leaving 2 bits for the fractional portion.  My encoding is
// q5.2 (signed), so I am scaling floats by 2^2 = 4.0 before mapping into q5.2 and 
// then scaling from q5.2 back to floats by (floating point) dividing by 2^2 (or multiplying by 1/4)

// Run the neural network model to generate a prediction
// based on the inputs provided.
int8_t NN_qpredict(const int8_t *inputs);