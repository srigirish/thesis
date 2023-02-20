#include "nrf24l01p.h"
#include "nrf24l01p_settings.h"
#include "periph/gpio.h"
#include "periph/spi.h"
#include "ztimer.h"
#include "xtimer.h"
#include "thread.h"
#include "msg.h"
#include "fmt.h"

#define ACK 0xff
#define HIGH 1
#define LOW 0

// RF24 radio(7, 8);
static nrf24l01p_t dev;
spi_t spi;
gpio_t ce = GPIO_PIN(PORT_C, 7);
gpio_t cs = GPIO_PIN(PORT_A, 4);
gpio_t irq = GPIO_PIN(PORT_E, 10);
uint8_t addrLen = 4;

#define NRF24L01P_SPI_DEVICE (0)

//const uint8_t* NRF24L01P_TX_ADDR = &tx_addr;

const uint64_t slider = 0xF0F0F0F0D2LL;
const uint64_t dispatchModule[5] = { 0x3A3A3A3AD2LL, 0x3A3A3A3AC3LL, 0x3A3A3A3AB4LL, 0x3A3A3A3AA5LL, 0x3A3A3A3A96LL };
    static const uint8_t CON_C_ADDRESS[] =  {0xc7, 0xc7, 0xc7, 0xc7, 0xc7,};
    static const uint8_t DIS_TT_ADDRESS[] =  {0xd7, 0xd7, 0xd7, 0xd7, 0xd7,};
    static const uint8_t SLIDER_ADDRESS[] =  {0xe7, 0xe7, 0xe7, 0xe7, 0xe7,};

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
bool sent = false;

void(* resetFunc) (void) = 0; //reset function declaration

void move(char direction) {
  if (direction == 'f') {
     gpio_write(GPIO_PIN(PORT_F, 13), HIGH);
     gpio_write(GPIO_PIN(PORT_F, 12), LOW);
  }
  if (direction == 'b') {
     gpio_write(GPIO_PIN(PORT_F, 13), LOW);
     gpio_write(GPIO_PIN(PORT_F, 12), HIGH);
  }
}

void stop() {
   gpio_write(GPIO_PIN(PORT_F, 13), LOW);
   gpio_write(GPIO_PIN(PORT_F, 12), LOW);
}


int main(void) {
int response = 0;

nrf24l01p_init(&dev, NRF24L01P_SPI_DEVICE, ce, cs, irq);
nrf24l01p_set_power(&dev, -10);
nrf24l01p_set_rx_address(&dev, NRF24L01P_PIPE1, CON_C_ADDRESS, INITIAL_ADDRESS_WIDTH);
nrf24l01p_set_tx_address(&dev, SLIDER_ADDRESS, INITIAL_ADDRESS_WIDTH);
nrf24l01p_set_rxmode(&dev);

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
  int firstSensor =  gpio_read(GPIO_PIN(PORT_D, 14));
  int secondSensor =  gpio_read(GPIO_PIN(PORT_D, 15));
          printf("%d \n",state);
 
    
  if (state == 0) {
      printf("Waiting for message \n");
    nrf24l01p_set_rxmode(&dev);
   // while (!radio.available(&address));
    nrf24l01p_read_payload(&dev, &rxMessage, sizeof(rxMessage));
    nrf24l01p_set_txmode(&dev);
    if(rxMessage==0xcf) {
    printf("Received ");
    printf("%x \n", rxMessage);
    state = 1;
    }
  }

  else if (state == 1) {
      move('f');
    if(firstSensor == 1)
      state = 2;
  }

  else if (state == 2) {
    txMessage = 0xff;
    sent = nrf24l01p_preload(&dev, &txMessage, sizeof(txMessage));
    if (sent) {
      nrf24l01p_transmit(&dev);
      printf("Sent Ack to Slider \n");
    }
    if(secondSensor == 1) {
    stop();
    state = 0;
    }
  }
}
}
