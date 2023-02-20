#include "nrf24l01p.h"
#include "nrf24l01p_settings.h"
#include "periph/gpio.h"
#include "periph/spi.h"
#include "ztimer.h"
#include "thread.h"
#include "msg.h"

#define ACK 0xff
#define HIGH 1
#define LOW 0

// RF24 radio(7, 8);
nrf24l01p_t *dev;
spi_t spi;
gpio_t ce = GPIO_PIN(PORT_C, 7);
gpio_t cs = GPIO_PIN(PORT_A, 4);
gpio_t irq;
uint8_t addrLen = 2;


const uint64_t slider[5] = { 0xF0F0F0F0D2LL, 0xF0F0F0F0C3LL, 0xF0F0F0F0B4LL, 0xF0F0F0F0A5LL, 0xF0F0F0F096LL };
const uint64_t dispatchModule[5] = { 0x3A3A3A3AD2LL, 0x3A3A3A3AC3LL, 0x3A3A3A3AB4LL, 0x3A3A3A3AA5LL, 0x3A3A3A3A96LL };


/*
   NRF Connections with Arduino Mega:
   CE - 7
   CSN - 8
   M1/MISO - 50
   M0/MOSI - 51
   SCK - 52
*/

int state;
char txMessage;
char rxMessage;
uint8_t source;
uint8_t incomingData = 0xa1;
bool sent = false;
void(* resetFunc) (void) = 0; //reset function declaration

void move(char direction) {
  if (direction == 'f') {
     gpio_write(GPIO_PIN(PORT_F, 12), HIGH);
     gpio_write(GPIO_PIN(PORT_F, 13), LOW);
  }
  if (direction == 'b') {
     gpio_write(GPIO_PIN(PORT_F, 12), LOW);
     gpio_write(GPIO_PIN(PORT_F, 13), HIGH);
  }
}

void stop() {
   gpio_write(GPIO_PIN(PORT_F, 13), LOW);
   gpio_write(GPIO_PIN(PORT_F, 12), LOW);
}


int main(void) {
  nrf24l01p_init(dev, 1, ce, cs, irq);
  nrf24l01p_set_rx_address_long(dev, NRF24L01P_PIPE0, dispatchModule[1], addrLen);
  
   gpio_init(GPIO_PIN(PORT_B, 8), GPIO_OUT);	//4
   gpio_init(GPIO_PIN(PORT_B, 9), GPIO_OUT);	//5
   gpio_init(GPIO_PIN(PORT_F, 12), GPIO_OUT);	//6
   gpio_init(GPIO_PIN(PORT_F, 13), GPIO_OUT); 	//9	//high away from slider
   gpio_init(GPIO_PIN(PORT_D, 14), GPIO_IN_PD);	//2	 //towards slider
   gpio_init(GPIO_PIN(PORT_D, 15), GPIO_IN_PD);	//3

   gpio_write(GPIO_PIN(PORT_B, 8), HIGH);
   gpio_write(GPIO_PIN(PORT_B, 9), HIGH);
  state = 0;


while(1) {
  int firstSensor =  gpio_read(GPIO_PIN(PORT_D, 15));
  int secondSensor =  gpio_read(GPIO_PIN(PORT_D, 14));

  printf("%d \n", state);

  if (state == 0) {
  
  if(incomingData == 0xa1) {
      source = 0xaf;
      state = 1;
    } 
    
    if(incomingData == 0xb1) {
    source = 0xbf;
    state = 1;  
    }

    if(incomingData == 0xc1) {
    source = 0xcf;
    state = 1;  
    }
    
   else
   printf("Invalid Source \n");
    nrf24l01p_set_rxmode(dev);
  }

  else if (state == 1 && firstSensor != 0) {
  move('f');
    state = 2;
  }

  else if (state == 2 && secondSensor != 0) {
  stop();
    nrf24l01p_set_tx_address_long(dev, slider[0], addrLen);
    txMessage = 0xaa;
    sent = nrf24l01p_preload(dev, &txMessage, sizeof(txMessage));
    if (sent) {
      nrf24l01p_transmit(dev);
      printf("Sent Ack to slider \n");
      state = 3;
    }
  }

  else if (state == 3) {
    nrf24l01p_set_rxmode(dev);
  //  while (!radio.available());
    nrf24l01p_read_payload(dev, &rxMessage, sizeof(rxMessage));
    nrf24l01p_set_txmode(dev);
    if (rxMessage == ACK) {
      printf("Received ");
      printf("%x \n", rxMessage);
      state = 4;
    }
  }

  else if (state == 4) {
    nrf24l01p_set_tx_address_long(dev, slider[0], addrLen);
    txMessage = source;
    move('f');
    sent = nrf24l01p_preload(dev, &txMessage, sizeof(txMessage));
    if (sent) {
      nrf24l01p_transmit(dev);
      printf("Sent message to Slider \n");
      state = 5;
    }
  }
  
  else if (state == 5) {
    nrf24l01p_set_rxmode(dev);
  //  while (!radio.available(&address));
    nrf24l01p_read_payload(dev, &rxMessage, sizeof(rxMessage));
    nrf24l01p_set_txmode(dev);
    if (rxMessage == 0xff) {
    stop();
      printf("Received Ack from Slider \n");
      state = 0;
    }
  }
}
}
