#include "crc16_ccitt.hpp"
#include "LiFi.hpp"
#include "LiFi_wrapper.h"
#include "periph_conf.h"
#include "periph/gpio.h"
#include "xtimer.h"

//#define ENABLE_DEBUG
#include "debug.hpp"

#define TICK 3//<-- number of milli seconds per tick (--> use for delay())
#define CLOCK_HALF 5 // <-- number of ticks per half clock
#define CLOCK (2 * CLOCK_HALF) // <-- number of ticks per clock (1 data bit)
#define GET_CLASSIFIER_TICKS (TICK * CLOCK * 3)
#define MINIMUM_HIGH_LOW_DIFFERENCE 50 // <-- used in get_classifier()

char debug_str_buf[256];
uint8_t debug_str_pos;
extern "C"
{

    LiFiHandle create_LiFi(gpio_t sPin, adc_t rPin) { return (LiFi*) new LiFi(sPin,rPin); }
    void free_LiFi(LiFiHandle p) { delete (LiFi*) p; }
    int send_LiFi_data(LiFiHandle p, const uint8_t *data, uint8_t data_len) {
    return ((LiFi*) p)->send_data(data,data_len);
    }
    int receive_LiFi(LiFiHandle p, uint8_t *buf, uint8_t buf_size) {return ((LiFi*) p)->receive(buf,buf_size);}
}
