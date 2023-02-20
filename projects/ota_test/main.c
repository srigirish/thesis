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

#include "nrf24l01p.h"
#include "nrf24l01p_settings.h"
#include "mutex.h"
#include "periph/spi.h"
#include "ztimer.h"
#include "xtimer.h"
#include "msg.h"

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
#include "PLiFi_wrapper.h"

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

#define SS_PIN PA6
#define RST_PIN PA5
#define ACK 0xff
#define HIGH 1
#define LOW 0
char* serverAddr = "2001:db8::1";
char* serverPort = "5683";
char* filePath = "status/conveyorA.txt";
// RF24 radio(7, 8);
static nrf24l01p_t dev;
spi_t spi;
gpio_t ce = GPIO_PIN(PORT_C, 7);
gpio_t cs = GPIO_PIN(PORT_A, 4);
gpio_t irq = GPIO_PIN(PORT_E, 10);
uint8_t addrLen = 4;
int firstSensor, secondSensor;
char rxMessage, txMessage;
static int state = 0;
uint8_t source;
bool sent = false;
void(* resetFunc) (void) = 0; //reset function declaration
void move(char dir);
void stop(void);
char* keys[10] = {"fS","sS","fSw","sSw","mD","tD","sD","iM","iT","iS"};
char* defaultValues[10] = {"0","0","0","0","f","ccl","f","0","0","0"};
static char* statusMessage[10][2];

uint8_t acc[10];
static const uint32_t DEVICE_ID_POS = 0;
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
char userRequest = 'a';

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

static char tx_stack1[THREAD_STACKSIZE_MAIN];
static char rx_stack1[THREAD_STACKSIZE_MAIN];

static void on_rx(PLiFi *lifi, const uint8_t *msg, uint8_t len);
static void control_message(const uint8_t *msg, uint8_t len);
static void request_message(const uint8_t *msg, uint8_t len);
static void on_message_for_me(const uint8_t *msg, uint8_t len);

PLiFiHandle lifi1 = create_PLiFi(GPIO_PIN(PORT_E, 7), GPIO_PIN(PORT_A, 0), on_rx, NULL); //22,0

void *tx_loop(void *arg)
{
    PLiFiHandle *lifi = (PLiFiHandle *)arg;

    lifi->tx_loop_PLiFi();
    return NULL;
}

void *rx_loop(void *arg)
{
    PLiFiHandle *lifi = (PLiFiHandle *)arg;
    lifi->rx_loop_PLiFi();
    return NULL;
}

static void on_rx(PLiFiHandle *lifi, const uint8_t *msg, uint8_t len)
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
                lifi1.send_PLiFi_data(msg, len);
            }
        }
    }
}

static void control_message(const uint8_t *msg, uint8_t len)
{
    if (len > 0)
    {
    //    Serial.print("Got: 0x");
        for (uint8_t i = 0; i < len - 1; i++)
        {
       //     Serial.print(msg[i], HEX);
       //     Serial.print(", 0x");
        }
        // Serial.println(msg[len - 1], HEX);
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
        // Serial.println("Got: Empty message");
    }
}

static void request_message(const uint8_t *msg, uint8_t len)
{
    if (len > 0)
    {
    //   Serial.print("Got: 0x");
        for (uint8_t i = 0; i < len - 1; i++)
        {
      //      Serial.print(msg[i], HEX);
       //     Serial.print(", 0x");
        }
        // Serial.println(msg[len - 1], HEX);
        control_data[0] = msg[len - 1];
        
        if (control_data[0] == pallet_types[0])
        {
            userRequest = 'a';
            gpio_write(GPIO_PIN(PORT_D, 13), LOW);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW); //Orange
            gpio_write(GPIO_PIN(PORT_D, 11), HIGH);  //Green

        }
        else if (control_data[0] == pallet_types[1])
        {
            userRequest = 'b';
            gpio_write(GPIO_PIN(PORT_D, 13), HIGH);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW); //Orange
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Green
        }
        else if (control_data[0] == pallet_types[2])
        {
            userRequest = 'c';
            gpio_write(GPIO_PIN(PORT_D, 13), HIGH);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW); //Orange
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Green
        }
        else if (control_data[0] == pallet_types[3])
        {
            userRequest = '1';
            gpio_write(GPIO_PIN(PORT_D, 13), HIGH);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), LOW); //Orange
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Green
        }
        if (control_data[0] == pallet_types[5])
        {
            userRequest = '0';
            gpio_write(GPIO_PIN(PORT_D, 13), LOW);  //Red
            gpio_write(GPIO_PIN(PORT_D, 12), HIGH); //Orange
            gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Green
        }
        if (control_data[0] == hiBay_data[2])
        {
            if(userRequest != '1') {
                userRequest = '0';
                gpio_write(GPIO_PIN(PORT_D, 13), LOW);  //Red
                gpio_write(GPIO_PIN(PORT_D, 12), HIGH); //Orange
                gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Green
            }
        }
    }
    else
    {
        // Serial.println("Got: Empty message");
    }
}

static void on_message_for_me(const uint8_t *msg, uint8_t len)
{
    if (len > 0)
    {
       // Serial.print("Got: 0x");
        for (uint8_t i = 0; i < len - 1; i++)
        {
         //   Serial.print(msg[i], HEX);
         //   Serial.print(", 0x");
        }
        // Serial.println(msg[len - 1], HEX);
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
        // Serial.println("Got: Empty message");
    }
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

    gpio_init(GPIO_PIN(PORT_B, 10), GPIO_IN);	//2
    gpio_init(GPIO_PIN(PORT_B, 11), GPIO_IN);	//3
    gpio_init(GPIO_PIN(PORT_E, 14), GPIO_OUT);	//6
    gpio_init(GPIO_PIN(PORT_E, 15), GPIO_OUT);	//9
    gpio_init(GPIO_PIN(PORT_E, 12), GPIO_OUT);	//4

    gpio_write(GPIO_PIN(PORT_E, 12), HIGH);

    gpio_write(GPIO_PIN(PORT_E, 14), LOW); //HIGH for forward
    gpio_write(GPIO_PIN(PORT_E, 15), LOW);

    gpio_init(GPIO_PIN(PORT_D, 13), GPIO_OUT);	//23
    gpio_init(GPIO_PIN(PORT_D, 12), GPIO_OUT);	//25
    gpio_init(GPIO_PIN(PORT_D, 11), GPIO_OUT);	//27

    gpio_write(GPIO_PIN(PORT_D, 13), LOW);  //Red
    gpio_write(GPIO_PIN(PORT_D, 12), HIGH);  //Orange
    gpio_write(GPIO_PIN(PORT_D, 11), LOW);  //Green

    // Serial.println("setup() done");


bool startSensor = false;
bool endSensor = false;
// bool messageSent = false;

while(1)
{
    uint8_t msg[2];

    startSensor = gpio_read(GPIO_PIN(PORT_B, 10));
    endSensor = gpio_read(GPIO_PIN(PORT_B, 11));
  printf("%d\n",state2);

    // if(endSensor!=digitalRead(5)) {
    //     messageSent = false;
    // }

    if (startSensor && !endSensor && (userRequest == '0' || userRequest == 'a'))
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
        gpio_write(GPIO_PIN(PORT_E, 14), LOW);
        gpio_write(GPIO_PIN(PORT_E, 15), LOW); //HIGH for forward
        break;
    case 1:
        gpio_write(GPIO_PIN(PORT_D, 13), LOW);  //Red
        gpio_write(GPIO_PIN(PORT_D, 12), LOW); //Orange
        gpio_write(GPIO_PIN(PORT_D, 11), HIGH);  //Green

        gpio_write(GPIO_PIN(PORT_E, 14), LOW);
        gpio_write(GPIO_PIN(PORT_E, 15), HIGH); //HIGH for forward
        sendcoap("iM","1");
        sendcoap("mD","f");
        break;
    case 2:
        gpio_write(GPIO_PIN(PORT_E, 14), LOW);
        gpio_write(GPIO_PIN(PORT_E, 15), LOW); //HIGH for forward
        sendcoap("iM","0");

        msg[0] = control_info[0];
        msg[1] = sink1[0];
        if (lifi1.send_data(msg, sizeof(msg)))
        {
            xtimer_usleep(10);
        }
//        messageSent = true;
        // Serial.println("Message 0x7e sent");
        state2 = 3;
        break;
    case 3:
        gpio_write(GPIO_PIN(PORT_E, 14), LOW);
        gpio_write(GPIO_PIN(PORT_E, 15), LOW); //HIGH for forward
        break;
    case 4:
        gpio_write(GPIO_PIN(PORT_E, 14), LOW);
        gpio_write(GPIO_PIN(PORT_E, 15), HIGH); //HIGH for forward
        break;
    }
}
    return NULL;
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

static const shell_command_t shell_commands[] = {
#ifdef MODULE_SUIT_STORAGE_FLASHWRITE
    { "current-slot", "Print current slot number", cmd_print_current_slot },
    { "riotboot-hdr", "Print current slot header", cmd_print_riotboot_hdr },
#endif
    { NULL, NULL, NULL }
};

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

void stop(void)
{
   gpio_write(GPIO_PIN(PORT_F, 13), LOW);
   gpio_write(GPIO_PIN(PORT_F, 12), LOW);
}

int main(void)
{
    puts("OTA Test Update Example");
    for(int i = 0; i<10;i++) {
    	statusMessage[i][0] = keys[i];
    	statusMessage[i][1] = defaultValues[i];
    }
  nrf24l01p_init(&dev, NRF24L01P_SPI_DEVICE, ce, cs, irq);
  nrf24l01p_set_rx_address_long(&dev, NRF24L01P_PIPE1, dispatchModule[0], addrLen);
  
   gpio_init(GPIO_PIN(PORT_B, 8), GPIO_OUT);	//4
   gpio_init(GPIO_PIN(PORT_B, 9), GPIO_OUT);	//5
   gpio_init(GPIO_PIN(PORT_F, 12), GPIO_OUT);	//6
   gpio_init(GPIO_PIN(PORT_F, 13), GPIO_OUT); 	//9	//high away from slider
   gpio_init(GPIO_PIN(PORT_D, 14), GPIO_IN_PD);	//2	 //towards slider
   gpio_init(GPIO_PIN(PORT_D, 15), GPIO_IN_PD);	//3

   gpio_write(GPIO_PIN(PORT_B, 8), HIGH);
   gpio_write(GPIO_PIN(PORT_B, 9), HIGH);
   state = 0;

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
    /* start suit updater thread */
    suit_worker_run();

    /* start nanocoap server thread */
    thread_create(_nanocoap_server_stack, sizeof(_nanocoap_server_stack),
                  THREAD_PRIORITY_MAIN - 1,
                  THREAD_CREATE_STACKTEST,
                  _nanocoap_server_thread, NULL, "nanocoap server");
        
        thread_create(module_motion_thread_stack, sizeof(module_motion_thread_stack),
                  THREAD_PRIORITY_MAIN - 2, THREAD_CREATE_STACKTEST,
                  _module_motion_thread, NULL, "module_thread");
                  
                   thread_create(tx_stack1, sizeof(tx_stack1), THREAD_PRIORITY_MAIN - 3,
                  THREAD_CREATE_STACKTEST, tx_loop, &lifi1, "tx1");
    thread_create(rx_stack1, sizeof(rx_stack1), THREAD_PRIORITY_MAIN - 4,
                  THREAD_CREATE_STACKTEST, rx_loop, &lifi1, "rx1");
    xtimer_usleep(1);

    /* the shell contains commands that receive packets via GNRC and thus
       needs a msg queue */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    puts("Starting the shell");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);


    return 0;
}
