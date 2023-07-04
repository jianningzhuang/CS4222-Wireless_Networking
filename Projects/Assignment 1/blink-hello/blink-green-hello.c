#include "contiki.h"
#include "dev/leds.h"
#include "sys/etimer.h"

#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
static struct etimer et_hello;
static struct etimer et_blink;
static uint8_t hello_counter;
static uint8_t blink_counter;
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
PROCESS(blink_process, "LED blink process");
AUTOSTART_PROCESSES(&hello_world_process, &blink_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{

  PROCESS_BEGIN();

  /* Setup a periodic timer that expires after 5 seconds. */
  etimer_set(&et_hello, CLOCK_SECOND * 5);
  hello_counter = 0;

  while (1)
  {
    printf("Hello, Jianning! #%u\n", hello_counter);
    hello_counter++;
    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_hello));
    etimer_reset(&et_hello);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(blink_process, ev, data)
{
  PROCESS_BEGIN();

  etimer_set(&et_blink, CLOCK_SECOND);
  blink_counter = 0;

  leds_off(LEDS_ALL);

  while (1)
  {
    leds_toggle(LEDS_GREEN);

    printf("Blink... (state %d)\n", leds_get());
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_blink));
    etimer_reset(&et_blink);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/