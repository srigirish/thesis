#include "Arduino.h"
#include "thread.h"
#include <stdlib.h>

static char tx_stack1[THREAD_STACKSIZE_MAIN];

int b=1;
void *tx_loop(void *arg)
{
    int *temp = (int *)arg;
    temp = &b;
    b = *temp;
    digitalWrite(13,HIGH);
    delay(500);
    digitalWrite(13,LOW);
    delay(500);
    return NULL;
}


void setup()
{
    int a=10;
    void *temp=&a;
    thread_create(tx_stack1, sizeof(tx_stack1), THREAD_PRIORITY_MAIN - 1,
                  THREAD_CREATE_STACKTEST, tx_loop, temp, "tx1");
    delay(1);

    pinMode(13,OUTPUT);

    Serial.println("setup() done");
}

void loop()
{

}
