#pragma once
#include "mutex.h"
#include "periph/adc.h"
#include "PLiFi.hpp"
#include "LiFi_wrapper.h"
#include "periph_conf.h"
#include "periph/gpio.h"
#include "xtimer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void * PLiFiHandle;
PLiFiHandle create_PLiFi(gpio_t sPin, adc_t rPin,
          void (*rx_cb)(PLiFi *, const uint8_t *, uint8_t),
          void (*tx_cb)(PLiFi *lifi));
int send_PLiFi_data(PLiFiHandle p, const uint8_t *data, uint8_t data_len);
   void tx_loop_PLiFi(PLiFiHandle p);
    void rx_loop_PLiFi(PLiFiHandle p);
    void free_PLiFi(PLiFiHandle p);

#ifdef __cplusplus
}
#endif
