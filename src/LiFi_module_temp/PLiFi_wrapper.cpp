#include "PLiFi_wrapper.h"
#include "PLiFi.hpp"
#include "periph_conf.h"
#include "periph/gpio.h"
#include "xtimer.h"
#include "mutex.h"
#include "periph/adc.h"

extern "C"
{
    PLiFiHandle create_PLiFi(gpio_t sPin, adc_t rPin,
          void (*rx_cb)(PLiFi *, const uint8_t *, uint8_t),
          void (*tx_cb)(PLiFi *lifi)) { return (PLiFi*) new PLiFi(sPin,rPin,rx_cb,tx_cb); }
    void    free_PLiFi(PLiFiHandle p) { delete (PLiFi*) p; }
    int send_PLiFi_data(PLiFiHandle p, const uint8_t *data, uint8_t data_len) {
    return ((LiFi*) p)->send_data(data,data_len);
    }
        void tx_loop_PLiFi(PLiFiHandle p) {((LiFi*) p)->tx_loop();}
    void rx_loop_PLiFi(PLiFiHandle p){((LiFi*) p)->rx_loop();}
    
}
