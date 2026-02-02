#include "nn.h"

#include <stdint.h>
#include <stdio.h>

#include <math.h>
 //
 //Add justigication for using Q4,3 structure

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
