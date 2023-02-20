#include "nrf24l01p.h"
#include "nrf24l01p_settings.h"
#include "periph/gpio.h"
#include "periph/spi.h"
#include "periph/uart.h"
#include "board.h"
#include "ringbuffer.h"
#include "ztimer.h"
#include "xtimer.h"
#include "thread.h"
#include "msg.h"

#ifdef MODULE_STDIO_UART
#include "stdio_uart.h"
#endif

#ifndef SHELL_BUFSIZE
#define SHELL_BUFSIZE       (128U)
#endif
#ifndef UART_BUFSIZE
#define UART_BUFSIZE        (128U)
#endif

#ifndef STDIO_UART_DEV
#define STDIO_UART_DEV      (UART_UNDEF)
#endif

#ifndef STX
#define STX 0x2
#endif

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

typedef struct {
    char rx_mem[UART_BUFSIZE];
    ringbuffer_t rx_buf;
} uart_ctx_t;

static uart_ctx_t ctx[UART_NUMOF];

#define NRF24L01P_SPI_DEVICE (0)


const uint64_t slider = 0xF0F0F0F0D2LL;
const uint64_t dispatchModule[5] = { 0x3A3A3A3AD2LL, 0x3A3A3A3AC3LL, 0x3A3A3A3AB4LL, 0x3A3A3A3AA5LL, 0x3A3A3A3A96LL };
    static const uint8_t CON_A_ADDRESS[] =  {0xa7, 0xa7, 0xa7, 0xa7, 0xa7,};
    static const uint8_t CON_B_ADDRESS[] =  {0xb7, 0xb7, 0xb7, 0xb7, 0xb7,};
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
char rxMessage = 0x0;
char source;
bool sent = false;
uint32_t baud = 115200;
void(* resetFunc) (void) = 0; //reset function declaration

static void rx_cb(void *arg, uint8_t data)
{
    uart_t dev = (uart_t)(uintptr_t)arg;

    ringbuffer_add_one(&ctx[dev].rx_buf, data);
    ringbuffer_get(&ctx[dev].rx_buf, &rxMessage, sizeof(rxMessage));
}

void moveConveyor(char direction) {
  if (direction == 'f') {
     gpio_write(GPIO_PIN(PORT_F, 12), HIGH);
     gpio_write(GPIO_PIN(PORT_F, 13), LOW);
  }
  if (direction == 'b') {
     gpio_write(GPIO_PIN(PORT_F, 12), LOW);
     gpio_write(GPIO_PIN(PORT_F, 13), HIGH);
  }
}

void stopConveyor() {
   gpio_write(GPIO_PIN(PORT_F, 13), LOW);
   gpio_write(GPIO_PIN(PORT_F, 12), LOW);
}

void moveSlider(char direction) {
  if (direction == 'f') {
  while(gpio_read(GPIO_PIN(PORT_F, 14)) == 1) {
     gpio_write(GPIO_PIN(PORT_D, 15), HIGH);
     gpio_write(GPIO_PIN(PORT_D, 14), LOW);
  }
  }
  else if (direction == 'b') {
  while(gpio_read(GPIO_PIN(PORT_E, 11)) == 1) {
     gpio_write(GPIO_PIN(PORT_D, 15), LOW);
     gpio_write(GPIO_PIN(PORT_D, 14), HIGH);
  }
  }
   gpio_write(GPIO_PIN(PORT_D, 15), LOW);
   gpio_write(GPIO_PIN(PORT_D, 14), LOW);
}


int main(void) {
 nrf24l01p_init(&dev, NRF24L01P_SPI_DEVICE, ce, cs, irq);
nrf24l01p_set_power(&dev, -10);
int response = uart_init(UART_DEV(0), baud, rx_cb, NULL);

 nrf24l01p_set_rx_address(&dev, NRF24L01P_PIPE1, SLIDER_ADDRESS, INITIAL_ADDRESS_WIDTH);
  
   gpio_init(GPIO_PIN(PORT_B, 8), GPIO_OUT);	//4
   gpio_init(GPIO_PIN(PORT_B, 9), GPIO_OUT);	//5
   gpio_init(GPIO_PIN(PORT_F, 12), GPIO_OUT);	//6	//high away from storage
   gpio_init(GPIO_PIN(PORT_F, 13), GPIO_OUT); 	//9	
   gpio_init(GPIO_PIN(PORT_D, 14), GPIO_IN_PD);	//10    //high towards conveyor A
   gpio_init(GPIO_PIN(PORT_D, 15), GPIO_IN_PD);	//11
   
   gpio_init(GPIO_PIN(PORT_E, 9), GPIO_IN); 	//2	//sensor	
   gpio_init(GPIO_PIN(PORT_E, 11), GPIO_IN_PU);	//30	//near conveyor A
   gpio_init(GPIO_PIN(PORT_F, 14), GPIO_IN_PU);	//32

   gpio_write(GPIO_PIN(PORT_B, 8), HIGH);
   gpio_write(GPIO_PIN(PORT_B, 9), HIGH);
   
  moveSlider('b');
  state = -1;


while(1) {
  int sensor =  gpio_read(GPIO_PIN(PORT_E, 9));
  printf("%d\n",response);      
  if (state == 0) {
  
//Serial communication with production
    switch(rxMessage) {
    case 0xaf:
        nrf24l01p_set_tx_address(&dev, DIS_TT_ADDRESS, INITIAL_ADDRESS_WIDTH);
        break;
    case 0xbf:
    	nrf24l01p_set_tx_address(&dev, DIS_TT_ADDRESS, INITIAL_ADDRESS_WIDTH);
    	break;
    case 0xcf:
        nrf24l01p_set_tx_address(&dev, CON_C_ADDRESS, INITIAL_ADDRESS_WIDTH);
        break;   
    }
  }

  else if (state == 1) {
    moveConveyor('f');
    nrf24l01p_set_txmode(&dev);
    txMessage = rxMessage;
    sent = nrf24l01p_preload(&dev, &txMessage, sizeof(txMessage));
    if (sent) {
      nrf24l01p_transmit(&dev);
      nrf24l01p_set_rxmode(&dev);
      printf("Sending...\n");
    }
    if(sensor == 1)
    state = 2;
  }

  else if (state == 2) {
  stopConveyor();
  if(rxMessage == 0xaf || rxMessage == 0xbf) {
  	moveConveyor('f');
  }
  else if(rxMessage == 0xcf) {
  moveSlider('f');
  moveConveyor('f');
  }
  state = 3;
  }
  
  else if (state == 3) {
    nrf24l01p_set_rxmode(&dev);
    nrf24l01p_read_payload(&dev, &rxMessage, sizeof(rxMessage));
    nrf24l01p_set_txmode(&dev);
    if(rxMessage == 0xff) {
    stopConveyor();
    moveSlider('b');
    state = 0;
    }
  }
}
}
