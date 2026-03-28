#include "nn.h"

#include <stdint.h>
#include <stdio.h>

#include <math.h>


// The fixed point implementation used is Q4,3 format, which means 4 bits for the integer part and 3 bits for the fractional part.
// In theory we could use Q3,4 format to allow for a larger fractional range, but that would limit the integer range to -8 to 7,
// which is not sufficient for our model parameters and intermediate results.
// In order to determine the appropriate fixed-point format, we need to analyze the range of values that will be encountered in the neural network.
// The ADC outputs a number between 0 and 4095, which at most takes up 12 bits, since I decided to use 
//unsigned 16-bit integers to store the ADC values. 
// We can only scale the ADC values up by a factor of 16 (2^4) before we risk overflowing the uint16_t type, 
// which means we have at most 4 bits to represent the integer part of our fixed-point numbers.
// Casting the 16 bit ADC values to int8_t format implicitly descards the first 8 bits, which leaves us with a Q4.3 format.

//Within process neuron, we are multiplying two Q4.3 numbers together, which gives us a Q8.6 result,
// so we need to use at least 16 bits to store the intermediate result of the multiplication to avoid overflow and preserve precision.
// Quantize the floating-point based NN architecture to work with int8_t,
// which is an 8-bit signed value ranging from -128 to 127.  Characterize the 
// maximum/minium value through your NN.  
//
// Hidden Layer (input range 0 to 1.1):
// 
// Both scale factors for the first neuron are ~ -3 so my accumulated sum of scaled
// inputs, worst case, is about -6.  But the bias is ~ +1.5 so
// this neuron's value into the activation function will range from about -4.5 to about 1.5
// its activation function is elu, which clamps negative values, 
// so after elu() this neuron's value ranges ~-1 to 1.5
//
// Both scale factors for the second neuron are ~2 so my accumulated sum of scaled
// inputs, worst case, is about 4, but the bias is ~ -2 so
// this neuron's value into the activation function will range from about -2 to about 2
// its activation function is elu, so again the neuron's final value ranges from ~-1 to ~2
//
// So, back of napkin, the worst-case intermediate value is ~-6 and the final neuron 
// values, which become inputs to the next layer, are within ~-1 to ~2
// 
// Output Layer (input range approx -1 to 2)
// 
// The scale factors are both negative, around -4 and -3 respectively.
// My worst-case accumulated sum of scaled inputs is -6, and the bias is 
// approximately zero, so this neuron's largest value into the activation
// function is ~-6.  
// its activation function is sigmoid, which ranges from 0 to 1
//
// So the worst, case value in my system has a magnitude of 6, which I can encode in 3 bits.
//
// This suggests I can get away with Q3.4 fixed-point representation, but I don't want any 
// trouble while I'm debugging so I'm going to hedge my bet and start out with Q4.3

#define QNN_FRACTIONAL_BITS (3)
#define QNN_SCALE_FACTOR (8.0)   // 2.0 ^ QNN_FRACTIONAL_BITS - pre-computed

#define NN_INPUTS          (2)
#define NN_LAYER0_NEURONS  (2) //Why are these in brackets
#define NN_OUTPUT (1)

static const int architec[3]={2,2,1}; // Neural network structure by layers, modify for other structures

// Quantized NN Model Parameters
const int8_t l0_qweights[2][2] = {
        { -25, 24, },
        { 25, -26, },
};

const int8_t l0_qbiases[2] = { -6, -7};

const int8_t l1_qweights[1][2] = {
        { 30, 29} 
};

const int8_t l1_qbiases[1] = {2};

// process_neuron(input_values)
int8_t qout_vector0[2];
int8_t qout_vector1[1];

//Activation func Tables

// Quantized lookup table for qelu activation function
// Look up quantized result of input argument by casting the
// int8_t (qm.n) to uint8_t for the lookup table index, e.g.:
// int8_t qresult = qelu_lut[(uint8_t)qargument];

static const int8_t qelu_lut[256] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29, 30, 31,
        32, 33, 34, 35, 36, 37, 38, 39,
        40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55,
        56, 57, 58, 59, 60, 61, 62, 63,
        64, 65, 66, 67, 68, 69, 70, 71,
        72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 87,
        88, 89, 90, 91, 92, 93, 94, 95,
        96, 97, 98, 99, 100, 101, 102, 103,
        104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119,
        120, 121, 122, 123, 124, 125, 126, 127,
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, -7, -7, -7, -7, -7, -7,
        -7, -7, -7, -6, -6, -6, -6, -5,
        -5, -5, -4, -4, -3, -3, -2, -1,
        };

// Quantized lookup table for qsigmoid activation function
// Look up quantized result of input argument by casting the
// int8_t (qm.n) to uint8_t for the lookup table index, e.g.:
// int8_t qresult = qsigmoid_lut[(uint8_t)qargument];

static const int8_t qsigmoid_lut[256] = {
        4, 4, 4, 5, 5, 5, 5, 6,
        6, 6, 6, 6, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 2, 2, 2,
        2, 2, 3, 3, 3, 3, 4, 4,
        };

int8_t qsigmoid(uint8_t index){
    return qsigmoid_lut[index];
}

int8_t qelu(uint8_t index){
    return qelu_lut[index];
}



static void qlayer(const int8_t *inputs,const int *arch,const int8_t *weights, const int8_t *bias, int8_t (*af)(uint8_t),int depth, int8_t *out_vector){
    int num_inputs = arch[depth];
    int num_outputs = arch[depth+1];

    for (int j = 0; j < num_outputs; j++) { // Iterating over neurons
        //Bsed on the analysis above it's clear that we can use 8Bit number
        int16_t result = 0;
        int8_t i;
        for( i = 0; i < num_inputs; i++ ) {
            // weights stored contiguously as [neuron][input]
            result += (int16_t)weights[j * num_inputs + i] * (int16_t)inputs[i]; //Multiplying two Q4,3 values, giving us Q8,6
            //if a large number is reached it will be treated as negative? 
        }
        result += ((int16_t)bias[j]<< QNN_FRACTIONAL_BITS); // Align and extend
        // if (result > 127) result = 127;
        // if (result < -128) result = -128;
        int8_t z = (int8_t)(result >> QNN_FRACTIONAL_BITS);
        result = af((uint8_t)z);// Apply activation
        
        out_vector[j]= result; // Over riding the global output array
    }
}



int8_t NN_qpredict(const int8_t *input_qval) {
    qlayer(input_qval, architec, &l0_qweights[0][0], l0_qbiases, qelu, 0, qout_vector0);
    qlayer(qout_vector0, architec, &l1_qweights[0][0], l1_qbiases, qsigmoid, 1, qout_vector1);
   
    return qout_vector1[0];
}

// // Convert a floating point value to its fixed-point equivalent representation
// int8_t NN_quantize(const float value) {
//     return (int8_t)(roundf(value*QNN_SCALE_FACTOR));
// }

// // Convert a fixed-point value to its floating point equivalent representation
// float NN_dequantize(const int8_t value) {
//     return (float)(value) / QNN_SCALE_FACTOR;
// }
