#pragma once
#include "mutex.h"
#include "periph/adc.h"
//#include "LiFi.hpp"
#include "LiFi_wrapper.h"
#include "periph_conf.h"
#include "periph/gpio.h"
#include "xtimer.h"

class PLiFi {
    // private members and functions
  private:
    LiFi lifi;
    mutex_t mutex;
    void (*rx_callback)(PLiFi *lifi, const uint8_t *msg, uint8_t msg_len);
    void (*tx_callback)(PLiFi *lifi);
    uint8_t obuf[64];
    uint8_t obuf_len;

    //public functions
  public:
    //transmitter fuctions
    PLiFi(gpio_t sPin, adc_t rPin,
          void (*rx_cb)(PLiFi *, const uint8_t *, uint8_t),
          void (*tx_cb)(PLiFi *lifi));
    int send_data(const uint8_t *data, uint8_t data_len);
    void tx_loop();
    void rx_loop();
};
