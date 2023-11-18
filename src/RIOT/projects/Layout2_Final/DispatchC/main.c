/*
 * Copyright (C) 2019 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       SUIT updates over CoAP example server application (using nanocoap)
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @}
 */


#include "mutex.h"
#include "ztimer.h"
#include "xtimer.h"
#include "msg.h"
#include "nrf24l01p.h"
#include "nrf24l01p_settings.h"
#include "periph/uart.h"
#include "periph/adc.h"
#include "periph/gpio.h"
#include "periph/spi.h"
#include "board.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "fmt.h"
#include "thread.h"

#include "irq.h"
#include "net/nanocoap_sock.h"
#include "pthread.h"
#include "pthread_threading.h"
#include "od.h"

#include "shell.h"
#include "periph_conf.h"

#include "suit/transport/coap.h"
#ifdef MODULE_SUIT_STORAGE_FLASHWRITE
#include "riotboot/slot.h"
#endif

#include "suit/storage.h"
#include "suit/storage/ram.h"
#ifdef BOARD_NATIVE
#include "suit/storage/vfs.h"
#include "xfa.h"
#include "vfs_default.h"
#endif

#ifdef MODULE_PERIPH_GPIO
#include "periph/gpio.h"
#endif

//extern "C" {
//#include "PLiFi.hpp"
//}

#define ACK 0xff
#define HIGH 1
#define LOW 0
#define NRF24L01P_SPI_DEVICE (0)
#define RES             ADC_RES_10BIT


void getBinary(unsigned int in, int count, int* out)
{
    /* assert: count <= sizeof(int)*CHAR_BIT */
    unsigned int mask = 1U << (count-1);
    int i;
    for (i = 0; i < count; i++) {
        out[i] = (in & mask) ? 1 : 0;
        in <<= 1;
    }
}

int led_read(int num) {
	int temp = 0;
	if(num == 1)
	temp = adc_sample(ADC_LINE(1), RES);
	return temp;
}

void led_write(unsigned int num) {
	int temp[8];
	getBinary(num,8,temp);
	for(int i = 0; i<8; i++) {
	if(temp[i]==1)
	gpio_write(GPIO_PIN(PORT_E, 7), HIGH);
	else
	gpio_write(GPIO_PIN(PORT_E, 7), LOW);
	xtimer_msleep(100);
	}
	printf("\n");
	gpio_write(GPIO_PIN(PORT_E, 7), LOW);
}

char* serverAddr = "2001:db8::1";
char* serverPort = "5683";
char* filePath = "status/disConveyor2.txt";
int firstSensor, secondSensor, ledValue;
char txMessage, rxMessage;
static int state = 0;
void move(char dir);
void stop(void);
char* keys[10] = {"fS","sS","fSw","sSw","mD","tD","sD","iM","iT","iS"};
char* defaultValues[10] = {"0","0","0","0","f","ccl","f","0","0","0"};
static char* statusMessage[10][2];

//static const uint8_t CON_A_ADDRESS[] =  {0xa7, 0xa7, 0xa7, 0xa7, 0xa7,};
static const uint8_t CON_C_ADDRESS[] =  {0xc7, 0xc7, 0xc7, 0xc7, 0xc7,};
//static const uint8_t DIS_TT_ADDRESS[] =  {0xd7, 0xd7, 0xd7, 0xd7, 0xd7,};
static const uint8_t SLIDER_ADDRESS[] =  {0xe7, 0xe7, 0xe7, 0xe7, 0xe7,};

char rx_handler_stack[THREAD_STACKSIZE_MAIN];

static msg_t _msg_q[1];

char module_motion_thread_stack[THREAD_STACKSIZE_MAIN];
#define COAP_INBUF_SIZE (256U)

/* Extend stacksize of nanocoap server thread */
static char _nanocoap_server_stack[THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF];
#define NANOCOAP_SERVER_QUEUE_SIZE     (8)
static msg_t _nanocoap_server_msg_queue[NANOCOAP_SERVER_QUEUE_SIZE];

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

/* add handled storages */
#if IS_USED(MODULE_SUIT_STORAGE_VFS)
XFA_USE(char*, suit_storage_files_reg);
#ifdef BOARD_NATIVE
XFA(suit_storage_files_reg, 0) char* _slot0 = VFS_DEFAULT_DATA "/SLOT0.txt";
XFA(suit_storage_files_reg, 1) char* _slot1 = VFS_DEFAULT_DATA "/SLOT1.txt";
#endif
#endif
int sendcoap(char* key, char* value);

void *nrf24l01p_rx_handler(void *arg)
{
    (void)arg;
    msg_init_queue(_msg_q, 1);
    unsigned int pid = thread_getpid();
    char rx_buf[NRF24L01P_MAX_DATA_LENGTH];

    puts("Registering nrf24l01p_rx_handler thread...");
    nrf24l01p_register(&dev, &pid);

    msg_t m;

    while (msg_receive(&m)) {
        printf("nrf24l01p_rx_handler got a message: ");

        switch (m.type) {
            case RCV_PKT_NRF24L01P:
                puts("Received packet.");

                /* CE low */
                nrf24l01p_stop(m.content.ptr);

                /* read payload */
                nrf24l01p_read_payload(m.content.ptr, rx_buf, NRF24L01P_MAX_DATA_LENGTH);

                /* flush rx fifo */
                nrf24l01p_flush_rx_fifo(m.content.ptr);

                /* CE high */
                nrf24l01p_start(m.content.ptr);

                /* print rx buffer */
                source++;
                    printf("%d ", source);
                

                puts("");

                break;

            default:
                puts("stray message.");
                break;
        }
    }

    puts("nrf24l01p_rx_handler: this should not have happened!");

    return NULL;
}

static void *_nanocoap_server_thread(void *arg)
{
    (void)arg;

    /* nanocoap_server uses gnrc sock which uses gnrc which needs a msg queue */
    msg_init_queue(_nanocoap_server_msg_queue, NANOCOAP_SERVER_QUEUE_SIZE);

    /* initialize nanocoap server instance */
    uint8_t buf[COAP_INBUF_SIZE];
    sock_udp_ep_t local = { .port=COAP_PORT, .family=AF_INET6 };
    nanocoap_server(&local, buf, sizeof(buf));

    return NULL;
}

static ssize_t _send(coap_pkt_t *pkt, size_t len, char *addr_str, char *port_str)
{
    ipv6_addr_t addr;
    sock_udp_ep_t remote;

    remote.family = AF_INET6;

    /* parse for interface */
    char *iface = ipv6_addr_split_iface(addr_str);
    if (!iface) {
        if (gnrc_netif_numof() == 1) {
            /* assign the single interface found in gnrc_netif_numof() */
            remote.netif = (uint16_t)gnrc_netif_iter(NULL)->pid;
        }
        else {
            remote.netif = SOCK_ADDR_ANY_NETIF;
        }
    }
    else {
        int pid = atoi(iface);
        if (gnrc_netif_get_by_pid(pid) == NULL) {
            puts("nanocli: interface not valid");
            return 0;
        }
        remote.netif = pid;
    }

    /* parse destination address */
    if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
        puts("nanocli: unable to parse destination address");
        return 0;
    }
    if ((remote.netif == SOCK_ADDR_ANY_NETIF) && ipv6_addr_is_link_local(&addr)) {
        puts("nanocli: must specify interface for link local target");
        return 0;
    }
    memcpy(&remote.addr.ipv6[0], &addr.u8[0], sizeof(addr.u8));

    /* parse port */
    remote.port = atoi(port_str);
    if (remote.port == 0) {
        puts("nanocli: unable to parse destination port");
        return 0;
    }

    return nanocoap_request(pkt, NULL, &remote, len);
}

char* makeJson(char* key, char* value)
{
	char* message = (char*) malloc(130);
	memset(message,0,20);
	strcat(message,"{");
	for(int i = 0;i<10;i++) {
	if(!strcmp(key,statusMessage[i][0])) {
		statusMessage[i][1] = value;	
	}
	}
	for(int i = 0; i<10; i++) {
		strcat(message,"\"");
		strcat(message,statusMessage[i][0]);
		strcat(message,"\"");
		strcat(message,":");
		strcat(message,"\"");
		strcat(message,statusMessage[i][1]);
		strcat(message,"\"");
		if(i==9)
		continue;
		strcat(message,",");
	}
	strcat(message,"}");
	return message;
}
 
void *_module_motion_thread(void *arg)
{
    (void) arg;
  int firstSensor = gpio_read(GPIO_PIN(PORT_B, 10));
  int secondSensor = gpio_read(GPIO_PIN(PORT_B, 11));
    state = 0;
    txMessage = 'a';
    while(1) {
     if (state == 0) {
      printf("Waiting for message \n");
   // while (!radio.available(&address));
    if(source > 0) {
    printf("Received \n");
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
    nrf24l01p_set_txmode(&dev);
    sent = nrf24l01p_preload(&dev, &txMessage, sizeof(txMessage));
    if (sent) {
      nrf24l01p_transmit(&dev);
      printf("Sent Ack to Slider \n");
    }
    nrf24l01p_set_rxmode(&dev);
    if(secondSensor == 1) {
    stop();
    state = 0;
    }
   }
  }
}

 

/* assuming that first button is always BTN0 */
#if defined(MODULE_PERIPH_GPIO_IRQ) && defined(BTN0_PIN)
static void cb(void *arg)
{
    (void) arg;
    printf("Button pressed! Triggering suit update! \n");
    suit_worker_trigger(SUIT_MANIFEST_RESOURCE, sizeof(SUIT_MANIFEST_RESOURCE));
}
#endif

#ifdef MODULE_SUIT_STORAGE_FLASHWRITE
static int cmd_print_riotboot_hdr(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    int current_slot = riotboot_slot_current();
    if (current_slot != -1) {
        /* Sometimes, udhcp output messes up the following printfs.  That
         * confuses the test script. As a workaround, just disable interrupts
         * for a while.
         */
        unsigned state = irq_disable();
        riotboot_slot_print_hdr(current_slot);
        irq_restore(state);
    }
    else {
        printf("[FAILED] You're not running riotboot\n");
    }
    return 0;
}

static int cmd_print_current_slot(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    /* Sometimes, udhcp output messes up the following printfs.  That
     * confuses the test script. As a workaround, just disable interrupts
     * for a while.
     */
    unsigned state = irq_disable();
    printf("Running from slot %d\n", riotboot_slot_current());
    irq_restore(state);
    return 0;
}
#endif



int sendcoap(char* key, char* value)
{
    unsigned buflen = 128;
    uint8_t buf[buflen];
    coap_pkt_t pkt;
    size_t len;
    uint8_t token[2] = {0xDA, 0xEC};
    char* payload = makeJson(key,value);
	

    pkt.hdr = (coap_hdr_t *)buf;

    /* parse options */
   
    
        ssize_t hdrlen = coap_build_hdr(pkt.hdr, COAP_TYPE_CON, &token[0], 2,
                                        3, 1);
        coap_pkt_init(&pkt, &buf[0], buflen, hdrlen);
        coap_opt_add_string(&pkt, COAP_OPT_URI_PATH, filePath, '/');
            coap_opt_add_uint(&pkt, COAP_OPT_CONTENT_FORMAT, COAP_FORMAT_TEXT);
            len = coap_opt_finish(&pkt, COAP_OPT_FINISH_PAYLOAD);

            pkt.payload_len = strlen(payload);
            memcpy(pkt.payload, payload, pkt.payload_len);
            len += pkt.payload_len;
        	free(payload);
        

        printf("nanocli: sending msg ID %u, %u bytes\n", coap_get_id(&pkt),
               (unsigned) len);

        ssize_t res = _send(&pkt, buflen, serverAddr, serverPort);
        if (res < 0) {
            printf("nanocli: msg send failed: %d\n", (int)res);
        }
        else {
            char *class_str = (coap_get_code_class(&pkt) == COAP_CLASS_SUCCESS)
                                    ? "Success" : "Error";
            printf("nanocli: response %s, code %1u.%02u", class_str,
                   coap_get_code_class(&pkt), coap_get_code_detail(&pkt));
            if (pkt.payload_len) {
                unsigned format = coap_get_content_type(&pkt);
                if (format == COAP_FORMAT_TEXT
                        || format == COAP_FORMAT_LINK
                        || coap_get_code_class(&pkt) == COAP_CLASS_CLIENT_FAILURE
                        || coap_get_code_class(&pkt) == COAP_CLASS_SERVER_FAILURE) {
                    /* Expecting diagnostic payload in failure cases */
                    printf(", %u bytes\n%.*s\n", pkt.payload_len, pkt.payload_len,
                                                                  (char *)pkt.payload);
                }
                else {
                    printf(", %u bytes\n", pkt.payload_len);
                    od_hex_dump(pkt.payload, pkt.payload_len, OD_WIDTH_DEFAULT);
                }
            }
            else {
                printf(", empty payload\n");
            }
        }
        return 0;
}


void move(char direction) {
  if (direction == 'f') {
     gpio_write(GPIO_PIN(PORT_E, 15), HIGH);
     gpio_write(GPIO_PIN(PORT_E, 14), LOW);
  }
  if (direction == 'b') {
     gpio_write(GPIO_PIN(PORT_E, 15), LOW);
     gpio_write(GPIO_PIN(PORT_E, 14), HIGH);
  }
}

void stop(void)
{
   gpio_write(GPIO_PIN(PORT_E, 15), LOW);
   gpio_write(GPIO_PIN(PORT_E, 14), LOW);
}

int main(void)
{
    puts("OTA Test Update Example");
    for(int i = 0; i<10;i++) {
    	statusMessage[i][0] = keys[i];
    	statusMessage[i][1] = defaultValues[i];
    }
    
    nrf24l01p_init(&dev, NRF24L01P_SPI_DEVICE, ce, cs, irq);
    nrf24l01p_set_power(&dev, -15);
    nrf24l01p_disable_all_auto_ack(&dev);
    nrf24l01p_disable_crc(&dev);
    nrf24l01p_set_rx_address(&dev, NRF24L01P_PIPE1, CON_C_ADDRESS, INITIAL_ADDRESS_WIDTH);
    nrf24l01p_set_tx_address(&dev, SLIDER_ADDRESS, INITIAL_ADDRESS_WIDTH);
    nrf24l01p_set_rxmode(&dev);
 // uart_init(UART_DEVICE, BAUDRATE, rx_cb, (void *)UART_DEVICE);
  
  
   gpio_init(GPIO_PIN(PORT_B, 10), GPIO_IN_PD);	//2
   gpio_init(GPIO_PIN(PORT_B, 11), GPIO_IN_PD);	//3
   gpio_init(GPIO_PIN(PORT_E, 14), GPIO_OUT);	//6
   gpio_init(GPIO_PIN(PORT_E, 15), GPIO_OUT); 	//9	//high away from slider
   gpio_init(GPIO_PIN(PORT_E, 12), GPIO_OUT);	//4	 //towards slider
  // gpio_init(GPIO_PIN(PORT_D, 15), GPIO_IN_PD);	//3
  gpio_init(GPIO_PIN(PORT_E, 7), GPIO_OUT);	//6
   gpio_init(GPIO_PIN(PORT_A, 0), GPIO_IN_PD);	//3
   gpio_write(GPIO_PIN(PORT_E, 12), HIGH);
   gpio_write(GPIO_PIN(PORT_E, 14), HIGH);
   gpio_write(GPIO_PIN(PORT_E, 15), HIGH);
   if(adc_init(ADC_LINE(1))<0)
   printf("Error initialising ADC!!!!\n");

#if defined(MODULE_PERIPH_GPIO_IRQ) && defined(BTN0_PIN)
    /* initialize a button to manually trigger an update */
    gpio_init_int(BTN0_PIN, BTN0_MODE, GPIO_FALLING, cb, NULL);
#endif

#ifdef MODULE_SUIT_STORAGE_FLASHWRITE
    cmd_print_current_slot(0, NULL);
    cmd_print_riotboot_hdr(0, NULL);
#endif
    /* initialize suit storage */
    suit_storage_init_all();

    /* start nanocoap server thread */
    thread_create(_nanocoap_server_stack, sizeof(_nanocoap_server_stack),
                  THREAD_PRIORITY_MAIN - 2,
                  THREAD_CREATE_STACKTEST,
                  _nanocoap_server_thread, NULL, "nanocoap server");
        
        thread_create(module_motion_thread_stack, sizeof(module_motion_thread_stack),
                  THREAD_PRIORITY_MIN, THREAD_CREATE_STACKTEST,
                  _module_motion_thread, NULL, "module_thread");
                  
	      if (thread_create(
	rx_handler_stack, sizeof(rx_handler_stack), THREAD_PRIORITY_MAIN - 1, 0,
	nrf24l01p_rx_handler, 0, "nrf24l01p_rx_handler") < 0) {
	puts("Error in thread_create");
	}

    xtimer_usleep(1);

    /* the shell contains commands that receive packets via GNRC and thus
       needs a msg queue */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    return 0;
}
