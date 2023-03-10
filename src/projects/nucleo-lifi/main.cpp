#include <string.h>
#include <stdlib.h>
// #include "SPI.h"
// #include "MFRC522.h"
#define SS_PIN PA6
#define RST_PIN PA5
#include "thread.h"
#include "PLiFi.hpp"
#include "shell.h"
#include "periph_conf.h"
#include "periph/gpio.h"
#include "xtimer.h"
// #include "periph/eeprom.h"
// MFRC522 mfrc522(SS_PIN, RST_PIN); // Instance of the class
uint8_t acc[10];
static const uint32_t DEVICE_ID_POS = 0;
static int state = 0;
int state2 = 0;
uint8_t device_id;
uint8_t control_info[] = {0x7e};
uint8_t request_data[] = {0x7d};
uint8_t pallet_types[] = {0xaa, 0xbb, 0xcc, 0xaf, 0xbf, 0xa0, 0xb0};
uint8_t sink1[] = {0xd6, 0xd7, 0xd8};
uint8_t sink2[] = {0xd0, 0xd1, 0xd2};
uint8_t sink3[] = {0xd3, 0xd4, 0xd5};
uint8_t hiBay_data[] = {0xa1, 0xa2, 0xa3};
uint8_t control_data[255];
char userRequest = '0';

//Definitions

#define HIGH 1
#define LOW 0

static char tx_stack1[THREAD_STACKSIZE_MAIN];
static char rx_stack1[THREAD_STACKSIZE_MAIN];

static void on_rx(PLiFi *lifi, const uint8_t *msg, uint8_t len);
static void control_message(const uint8_t *msg, uint8_t len);
static void request_message(const uint8_t *msg, uint8_t len);
static void on_message_for_me(const uint8_t *msg, uint8_t len);


PLiFi lifi1(22, 0, on_rx, NULL);

void *tx_loop(void *arg)
{
    PLiFi *lifi = (PLiFi *)arg;

    lifi->tx_loop();
    return NULL;
}

void *rx_loop(void *arg)
{
    PLiFi *lifi = (PLiFi *)arg;
    lifi->rx_loop();
    return NULL;
}

static void on_rx(PLiFi *lifi, const uint8_t *msg, uint8_t len)
{
    if (len >= 2)
    {
        if (msg[0] == device_id)
        {
            on_message_for_me(msg + 2, len - 2);
        }
        else if (msg[0] == control_info[0])
        {
            control_message(msg + 1, len - 1);
        }
        else if (msg[0] == request_data[0])
        {
            request_message(msg + 1, len - 1);
        }
        else
        {
            if (lifi != &lifi1)
            {
                lifi1.send_data(msg, len);
            }
        }
    }
}

static void control_message(const uint8_t *msg, uint8_t len)
{
    if (len > 0)
    {

        control_data[0] = msg[len - 1];
        if (control_data[0] == sink1[0])
        {
            state = 1;
        }
        if (control_data[0] == sink1[1])
        {
            state = 2;
            state2 = 4;
        }
        if (control_data[0] == sink1[2])
        {
            state = 3;
            state2 = 0;
        }
    }
    else
    {
        
    }
}

static void request_message(const uint8_t *msg, uint8_t len)
{
    if (len > 0)
    {
        control_data[0] = msg[len - 1];
        
        if (control_data[0] == pallet_types[0])
        {
            userRequest = 'a';
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW); //Orange
            gpio_write(GPIO_PIN(PORT_D, 13), HIGH);  //Green

        }
        else if (control_data[0] == pallet_types[1])
        {
            userRequest = 'b';
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW); //Orange
            gpio_write(GPIO_PIN(PORT_D, 13), HIGH);  //Green
        }
        else if (control_data[0] == pallet_types[2])
        {
            userRequest = 'c';
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW); //Orange
            gpio_write(GPIO_PIN(PORT_D, 13), HIGH);  //Green
        }
        else if (control_data[0] == pallet_types[3])
        {
            userRequest = '1';
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW); //Orange
            gpio_write(GPIO_PIN(PORT_D, 13), HIGH);  //Green
        }
        if (control_data[0] == pallet_types[5])
        {
            userRequest = '0';
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW); //Orange
            gpio_write(GPIO_PIN(PORT_D, 13), HIGH);  //Green
        }
        if (control_data[0] == hiBay_data[2])
        {
            if(userRequest != '1') {
                userRequest = '0';
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW); //Orange
            gpio_write(GPIO_PIN(PORT_D, 13), HIGH);  //Green
            }
        }
    }
    else
    {
        
    }
}

static void on_message_for_me(const uint8_t *msg, uint8_t len)
{
    if (len > 0)
    {

        //set state
        acc[0] = msg[len - 1];
        if (acc[0] == 0xf1)
            state = 1;
        if (acc[0] == 0xf2)
            state = 2;
        if (acc[0] == 0xf3)
            state = 3;
    }
    else
    {
        
    }
}
static const uint8_t sink1_1[] = {0x3E, 0xF1, 0xBB, 0xDF};
static const uint8_t sink1_2[] = {0xC5, 0x1D, 0xF7, 0x58};
static const uint8_t sink2_1[] = {0x74, 0x31, 0xDC, 0xE9};
static const uint8_t sink2_2[] = {0x95, 0xFB, 0xF8, 0x58};
static const uint8_t sink3_1[] = {0x74, 0x8E, 0xE1, 0xE9};
static const uint8_t sink3_2[] = {0x54, 0xE6, 0xB6, 0xEB};

int main(void) {

    // SPI.begin();        // Init SPI bus
    // mfrc522.PCD_Init(); // Init MFRC522

    device_id = 0x00;


    thread_create(tx_stack1, sizeof(tx_stack1), THREAD_PRIORITY_MAIN - 1,
                  THREAD_CREATE_STACKTEST, tx_loop, &lifi1, "tx1");
    thread_create(rx_stack1, sizeof(rx_stack1), THREAD_PRIORITY_MAIN - 2,
                  THREAD_CREATE_STACKTEST, rx_loop, &lifi1, "rx1");
    xtimer_usleep(1);

    gpio_init(GPIO_PIN(PORT_A, 0), GPIO_IN);
    gpio_init(GPIO_PIN(PORT_A, 1), GPIO_IN);
    gpio_init(GPIO_PIN(PORT_A, 2), GPIO_OUT);
    gpio_init(GPIO_PIN(PORT_A, 3), GPIO_OUT);
    gpio_init(GPIO_PIN(PORT_C, 0), GPIO_OUT);

    gpio_write(GPIO_PIN(PORT_A, 3), HIGH);

    gpio_write(GPIO_PIN(PORT_A, 1), LOW); //HIGH for forward
    gpio_write(GPIO_PIN(PORT_A, 2), LOW);

    gpio_init(GPIO_PIN(PORT_D, 11), GPIO_OUT);
    gpio_init(GPIO_PIN(PORT_D, 12), GPIO_OUT);
    gpio_init(GPIO_PIN(PORT_D, 13), GPIO_OUT);

    gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Red
    gpio_write(GPIO_PIN(PORT_D, 12), HIGH);  //Orange
    gpio_write(GPIO_PIN(PORT_D, 13), LOW);  //Green




bool startSensor = false;
bool endSensor = false;
// boolean messageSent = false;

while(1)
{
    uint8_t msg[2];

    startSensor = gpio_read(GPIO_PIN(PORT_A, 0));
    endSensor = gpio_read(GPIO_PIN(PORT_A, 1));

    // if(endSensor!=digitalRead(5)) {
    //     messageSent = false;
    // }

    if (startSensor && (userRequest == '0' || userRequest == 'a'))
    {
        state2 = 1;
    }
    else if (endSensor && state2 == 1)
    {
        state2 = 2;
    }

    switch (state2)
    {
    case 0:
        gpio_write(GPIO_PIN(PORT_A, 1), LOW);
        gpio_write(GPIO_PIN(PORT_A, 2), LOW); //HIGH for forward
        break;
    case 1:
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW); //Orange
            gpio_write(GPIO_PIN(PORT_D, 13), HIGH);  //Green

        gpio_write(GPIO_PIN(PORT_A, 1), LOW);
        gpio_write(GPIO_PIN(PORT_A, 2), HIGH); //HIGH for forward
        break;
    case 2:
        gpio_write(GPIO_PIN(PORT_A, 1), LOW);
        gpio_write(GPIO_PIN(PORT_A, 2), LOW); //HIGH for forward

        msg[0] = control_info[0];
        msg[1] = sink1[0];
        if (lifi1.send_data(msg, sizeof(msg)))
        {
            xtimer_usleep(10);
        }
//        messageSent = true;
        state2 = 3;
        break;
    case 3:
        gpio_write(GPIO_PIN(PORT_A, 1), LOW);
        gpio_write(GPIO_PIN(PORT_A, 2), LOW); //HIGH for forward
        break;
    case 4:
        gpio_write(GPIO_PIN(PORT_A, 1), LOW);
        gpio_write(GPIO_PIN(PORT_A, 2), HIGH); //HIGH for forward
        break;
    }
}
}
