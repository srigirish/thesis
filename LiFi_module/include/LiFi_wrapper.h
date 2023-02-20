#pragma once
#include "LiFi.hpp"
#include "periph/adc.h"
#include <stdint.h>
#include "periph_conf.h"
#include "periph/gpio.h"
#include "xtimer.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef void * LiFiHandle;
LiFiHandle create_LiFi(gpio_t sPin, adc_t rPin);
int send_LiFi_data(LiFiHandle p, const uint8_t *data, uint8_t data_len);
int receive_LiFi(LiFiHandle p, uint8_t *buf, uint8_t buf_size);
    void free_LiFi(LiFiHandle);

#ifdef __cplusplus
}
#endif
