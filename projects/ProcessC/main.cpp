#include <string.h>
// #include "SPI.h"
// #include "MFRC522.h"
#define SS_PIN 53
#define RST_PIN 49
#include "thread.h"
#include "PLiFi.hpp"
#include <stdlib.h>
// #include "periph/eeprom.h"
#include "shell.h"
#include "periph_conf.h"
#include "periph/gpio.h"
#include "xtimer.h"
//MFRC522 mfrc522(SS_PIN, RST_PIN); // Instance of the class
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
char userRequest = 'c';
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
        printf("Got: 0x");
        for (uint8_t i = 0; i < len - 1; i++)
        {
            printf("%x, 0x",msg[i]);
        }
        printf("%x", msg[len - 1]);
        control_data[0] = msg[len - 1];
        if (control_data[0] == sink3[0])
        {
            state = 1;
        }
        if (control_data[0] == sink3[1])
        {
            state = 2;
            state2 = 4;
        }
        if (control_data[0] == sink3[2])
        {
            state = 3;
            state2 = 0;
            
            userRequest = '0';
        }
    }
    else
    {
        printf("Got: Empty message \n");
    }
}

static void request_message(const uint8_t *msg, uint8_t len)
{
    if (len > 0)
    {
        printf("Got: 0x");
        for (uint8_t i = 0; i < len - 1; i++)
        {
            printf("%x, 0x",msg[i]);
        }
        printf("%x", msg[len - 1]);
        control_data[0] = msg[len - 1];
        
        if (control_data[0] == pallet_types[0])
        {
            userRequest = 'a';
            gpio_write(GPIO_PIN(PORT_D, 13), HIGH);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW);   //Orange
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);   //Green
        }
        else if (control_data[0] == pallet_types[1])
        {
            userRequest = 'b';
            gpio_write(GPIO_PIN(PORT_D, 13), HIGH);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW);   //Orange
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);   //Green
        }
        else if (control_data[0] == pallet_types[2])
        {
            userRequest = 'c';
            gpio_write(GPIO_PIN(PORT_D, 13), LOW);   //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW);   //Orange
            gpio_write(GPIO_PIN(PORT_D, 11), HIGH);  //Green
        }
        else if (control_data[0] == hiBay_data[2])
        {
            userRequest = '0';
            gpio_write(GPIO_PIN(PORT_D, 13), HIGH);   //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW);   //Orange
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Green
        }
    }
    else
    {
        printf("Got: Empty message \n");
    }
}

static void on_message_for_me(const uint8_t *msg, uint8_t len)
{
    if (len > 0)
    {
        printf("Got: 0x");
        for (uint8_t i = 0; i < len - 1; i++)
        {
            printf("%x, 0x",msg[i]);
        }
        printf("%x", msg[len - 1]);
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
        printf("Got: Empty message \n");
    }
}
static const uint8_t sink1_1[] = {0x3E, 0xF1, 0xBB, 0xDF};
static const uint8_t sink1_2[] = {0xC5, 0x1D, 0xF7, 0x58};
static const uint8_t sink2_1[] = {0x74, 0x31, 0xDC, 0xE9};
static const uint8_t sink2_2[] = {0x95, 0xFB, 0xF8, 0x58};
static const uint8_t sink3_1[] = {0x74, 0x8E, 0xE1, 0xE9};
static const uint8_t sink3_2[] = {0x54, 0xE6, 0xB6, 0xEB};

int main(void)
{
//    SPI.begin();        // Init SPI bus
//    mfrc522.PCD_Init(); // Init MFRC522
    printf("RFID reading UID \n");
//    device_id = eeprom_read_byte(DEVICE_ID_POS);
device_id = 0x04;
    printf("Device ID: 0x");
    printf("%x",device_id);

    thread_create(tx_stack1, sizeof(tx_stack1), THREAD_PRIORITY_MAIN - 1,
                  THREAD_CREATE_STACKTEST, tx_loop, &lifi1, "tx1");
    thread_create(rx_stack1, sizeof(rx_stack1), THREAD_PRIORITY_MAIN - 2,
                  THREAD_CREATE_STACKTEST, rx_loop, &lifi1, "rx1");
    xtimer_msleep(1);

    gpio_init(GPIO_PIN(PORT_D, 14), GPIO_IN_PD);	//2
    gpio_init(GPIO_PIN(PORT_D, 15), GPIO_IN_PD);	//3
    gpio_init(GPIO_PIN(PORT_A, 5), GPIO_OUT);	//6
    gpio_init(GPIO_PIN(PORT_A, 6), GPIO_OUT);	//9
    gpio_init(GPIO_PIN(PORT_A, 7), GPIO_OUT);	//4

    gpio_write(GPIO_PIN(PORT_A, 7), HIGH);

    gpio_write(GPIO_PIN(PORT_A, 5), LOW); //HIGH for forward
    gpio_write(GPIO_PIN(PORT_A, 6), LOW);

    gpio_init(GPIO_PIN(PORT_D, 13), GPIO_OUT);	//23
    gpio_init(GPIO_PIN(PORT_D, 12), GPIO_OUT);	//25
    gpio_init(GPIO_PIN(PORT_D, 11), GPIO_OUT);	//27

    gpio_write(GPIO_PIN(PORT_D, 13), LOW);  //Red
    gpio_write(GPIO_PIN(PORT_D, 12), HIGH);  //Orange
    gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Green

    printf("setup() done \n");


int startSensor = 0;
int endSensor = 0;
// bool messageSent = false;

while(1)
{
    uint8_t msg[2];

    startSensor = gpio_read(GPIO_PIN(PORT_D, 14));
    endSensor = gpio_read(GPIO_PIN(PORT_D, 15));
    
        
    // if(endSensor!=digitalRead(5)) {
    //     messageSent = false;
    // }

    if (startSensor && userRequest == 'c')
    {
        printf("Starting Sensor ");
        printf("%d \n", startSensor);
        state2 = 1;
    }
    else if (endSensor && state2 == 1)
    {
        state2 = 2;
    }
    printf("%c",state);

    switch (state2)
    {
    case 0:
        if(userRequest=='0') {
            gpio_write(GPIO_PIN(PORT_D, 13), HIGH);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW);  //Orange
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Green
        }

        gpio_write(GPIO_PIN(PORT_A, 5), LOW); //HIGH for forward
        gpio_write(GPIO_PIN(PORT_A, 6), LOW);
        break;
    case 1:

        gpio_write(GPIO_PIN(PORT_A, 5), HIGH); //HIGH for forward
        gpio_write(GPIO_PIN(PORT_A, 6), LOW);
        break;
    case 2:
        gpio_write(GPIO_PIN(PORT_A, 5), LOW); //HIGH for forward
        gpio_write(GPIO_PIN(PORT_A, 6), LOW);

        msg[0] = control_info[0];
        msg[1] = sink3[0];
        if (lifi1.send_data(msg, sizeof(msg)))
        {
            xtimer_msleep(10);
        }
//        messageSent = true;
        printf("Message 0x7e sent \n");
        state2 = 3;
        break;
    case 3:
        gpio_write(GPIO_PIN(PORT_A, 5), LOW); //HIGH for forward
        gpio_write(GPIO_PIN(PORT_A, 6), LOW);
        break;
    case 4:
        gpio_write(GPIO_PIN(PORT_A, 5), HIGH); //HIGH for forward
        gpio_write(GPIO_PIN(PORT_A, 6), LOW);
        break;
    }
}
}
