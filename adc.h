#pragma once

#include <stdint.h>

#include "retval.h"

typedef volatile struct ADC_CFGR1_s {
    uint32_t DMAEN      : 1;  // DMA Request Enable
    uint32_t DMACFG     : 1;  // DMA Configuration one_shot vs circular
    uint32_t SCANDIR    : 1;  // DMA scan direction
    uint32_t RES0       : 2;  // reserved
    uint32_t ALIGN      : 1;  // conversion data alignment
    uint32_t EXTSEL     : 3;  // external trigger selection
    uint32_t RES1       : 1;
    uint32_t EXTEN      : 2;  // external trigger enable
    uint32_t OVRMOD     : 1;  // ignore overruns
    uint32_t CONT       : 1;  // run in continuous mode
    uint32_t WAIT       : 1;  // 
    uint32_t AUTOFF     : 1;  // power down after conversion
    uint32_t DISCEN     : 1;
    uint32_t RES2       : 5;
    uint32_t AWDSGL     : 1;
    uint32_t AWDEN      : 1;
    uint32_t RES3       : 2;
    uint32_t AWDCH      : 5;
    uint32_t RES4       : 1;
} adc_cfgr1_t;

#define ADC_CFGR_EXTSEL_TRG0_TIM1_TRGO  (0)
#define ADC_CFGR_EXTSEL_TRG0_TIM1_CC4   (1)
#define ADC_CFGR_EXTSEL_TRG0_TIM2_TRGO  (2)
#define ADC_CFGR_EXTSEL_TRG0_TIM3_TRGO  (3)
#define ADC_CFGR_EXTSEL_TRG0_TIM15_TRGO (4)

#define ADC_CFGR_EXTEN_DISABLED         (0)
#define ADC_CFGR_EXTEN_RISING_EDGE      (1)
#define ADC_CFGR_EXTEN_FALLING_EDGE     (2)
#define ADC_CFGR_EXTEN_BOTH_EDGES       (3)

typedef volatile struct ADC_CFGR2_s {
    uint32_t res : 30;
    uint32_t ckmode : 2;
} adc_cfgr2_t;

#define ADC_CFGR2_CKMODE_ADCCLK   (0)
#define ADC_CFGR2_CKMODE_PCLKDIV2 (1)
#define ADC_CFGR2_CKMODE_PCLKDIV4 (2)

typedef volatile struct ADC_s {
    uint32_t ISR;           // 0x00 Interrupt Status
    uint32_t IER;           // 0x04 Interrupt Enable
    uint32_t CR;            // 0x08 Control
    adc_cfgr1_t CFGR1;      // 0x0C Configuration 1
    adc_cfgr2_t CFGR2;      // 0x10 Configuration 2
    uint32_t SMPR;          // 0x14 Sampling time 
    uint32_t RES0;          // 0x18 
    uint32_t RES1;          // 0x1C
    uint32_t TR;            // 0x20 Watchdog threshold 
    uint32_t RES2;          // 0x24
    uint32_t CHSELR;        // 0x28 Channel selection
    uint32_t RES3[5];       // 0x2C-0x3C
    uint32_t DR;            // 0x40 Data
    uint32_t RES4[177];     // 0x44-0x304
    uint32_t CCR;           // 0x308 Common configuration
} ADC_t;

#define ADC_ISR_ADRDY  (0x00000001) // ADC is ready to perform conversions
#define ADC_ISR_EOC    (0x00000004) // End of ADC conversion complete (data ready)
#define ADC_ISR_OVR    (0x00000010) // ADC Overrun occured
#define ADC_CR_ADEN    (0x00000001) // enable the ADC (after configuration)
#define ADC_CR_ADSTART (0x00000004) // start conversion
#define ADC_CR_ADSTP   (0x00000010) // stop converting (e.g. continuous mode)
#define ADC_CR_ADCAL   (0x80000000) // calibrate the ADC

#define ADC_IER_EOCIE  (0x00000004) // Enable end of conversion interrupt
#define ADC_IER_OVRIE  (0x00000010) // Enable overrun interrupt

// Sampling time - see datasheet table 55
#define ADC_SMPR_1_5  (0)
#define ADC_SMPR_13_5 (2)

// Channels
#define ADC_CHSELR_0 (0x00000001)
#define ADC_CHSELR_1 (0x00000002)
#define ADC_CHSELR_TEMP (0x0010000)
#define ADC_CHSELR_VREF (0x0020000)

// Common Configuration Register bits
#define ADC_CCR_TSEN   (0x00800000)
#define ADC_CCR_VREFEN (0x00400000)

typedef enum {ADC_CH0=ADC_CHSELR_0, ADC_CH1=ADC_CHSELR_1} ADC_CHANNEL_t;

// configure the ADC for the correct sampling time and perform
// its internal calibration procedure.  
void adc_init(void(*adc_callback_function)(ADC_CHANNEL_t channel, uint16_t output));

// start a single conversion on ADC Channel "channel" (ADC_CH0 or ADC_CH1)
// and waits for a result before returning
// returns RET_ERROR if the ADC is not ready to start a conversion
// otherwise returns RET_SUCCESS
ret_val_t adc_convert(ADC_CHANNEL_t channel); 