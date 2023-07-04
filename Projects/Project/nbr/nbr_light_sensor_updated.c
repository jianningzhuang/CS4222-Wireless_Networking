/*
 * CS4222/5422: Assignment 3b
 * Perform neighbour discovery
 */

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "net/linkaddr.h"
#include <string.h>
#include <stdio.h>
#include "node-id.h"
#include "board-peripherals.h"

// Identification information of the node

// Configures the wake-up timer for neighbour discovery
#define TIME_SLOT RTIMER_SECOND / 10 // 10 HZ, 0.1s
#define N 10

#define THREE_METER_THRESHOLD -50

static int row;
static int col;

static int light_counter = 0;

static volatile int light_sensor_readings[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

typedef struct
{
  unsigned long node_id;
  unsigned long time_detected;
  unsigned long time_absent;

} node;

static volatile node node_list[10];

// For neighbour discovery, we would like to send message to everyone. We use Broadcast address:
linkaddr_t dest_addr;

#define NUM_SEND 2
/*---------------------------------------------------------------------------*/
typedef struct
{
  unsigned long src_id;
  unsigned long timestamp;
  unsigned long seq;

} data_packet_struct;

typedef struct
{
  unsigned long src_id;
  unsigned long timestamp;
  int light_sensor_readings[10];

} light_sensor_packet_struct;

/*---------------------------------------------------------------------------*/
// duty cycle = WAKE_TIME / (WAKE_TIME + SLEEP_SLOT * SLEEP_CYCLE)
/*---------------------------------------------------------------------------*/

// sender timer implemented using rtimer
static struct rtimer rt;
static struct etimer timer_etimer;

// Protothread variable
static struct pt pt;

// Structure holding the data to be transmitted
static data_packet_struct data_packet;

static light_sensor_packet_struct light_sensor_packet;

// Current time stamp of the node
unsigned long curr_timestamp;

/*---------------------------------------------------------------------------*/

static void init_opt_reading(void);
static void get_light_reading(void);

/*---------------------------------------------------------------------------*/

// Starts the main contiki neighbour discovery process
PROCESS(nbr_discovery_process, "cc2650 neighbour discovery process");
PROCESS(light_sensor_process, "light sensor process");
AUTOSTART_PROCESSES(&nbr_discovery_process, &light_sensor_process);

// Function called after reception of a packet
void receive_packet_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest)
{

  // Check if the received packet size matches with what we expect it to be

  if (len == sizeof(data_packet))
  {

    static data_packet_struct received_packet_data;

    // Copy the content of packet into the data structure
    memcpy(&received_packet_data, data, len);

    // Print the details of the received packet
    printf("Received neighbour discovery packet %lu with rssi %d from %ld\n", received_packet_data.seq, (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI), received_packet_data.src_id);

    int rssi = (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI);

    unsigned long curr_time = clock_time();

    if (rssi < THREE_METER_THRESHOLD)
    {
      printf("AWAY\n");
      for (int i = 0; i < 10; i++)
      {
        if (node_list[i].node_id == received_packet_data.src_id)
        {
          node_list[i].time_detected = 0;
          printf("CURRENT TIME %lu TIME ABSENT %lu\n", curr_time / CLOCK_SECOND, node_list[i].time_absent / CLOCK_SECOND);
          if (node_list[i].time_absent == 0)
          {
            node_list[i].time_absent = curr_time;
          }
          else
          {
            if ((curr_time / CLOCK_SECOND - node_list[i].time_absent / CLOCK_SECOND) >= 30)
            {
              printf("%lu ABSENT %lu\n", node_list[i].time_absent / CLOCK_SECOND, node_list[i].node_id);
              node_list[i].node_id = 0;
              node_list[i].time_detected = 0;
              node_list[i].time_absent = 0;
            }
          }
          break;
        }
      }
    }
    else
    {
      printf("NEAR\n");
      bool new_node = true;
      for (int i = 0; i < 10; i++)
      {
        if (node_list[i].node_id == received_packet_data.src_id)
        {
          new_node = false;
          node_list[i].time_absent = 0;
          if (node_list[i].time_detected == 0)
          {
            node_list[i].time_detected = curr_time;
          }
          else
          {
            if ((curr_time / CLOCK_SECOND - node_list[i].time_detected / CLOCK_SECOND) >= 15)
            {
              printf("%lu DETECT %lu\n", node_list[i].time_detected / CLOCK_SECOND, node_list[i].node_id);

              // send array, radio should be on
              // NETSTACK_RADIO.on();

              // Initialize the nullnet module with information of packet to be trasnmitted
              nullnet_buf = (uint8_t *)&light_sensor_packet; // data transmitted
              nullnet_len = sizeof(light_sensor_packet);     // length of data transmitted

              light_sensor_packet.src_id = node_id;
              light_sensor_packet.timestamp = clock_time();
              for (int j = 0; j < 10; j++)
              {
                light_sensor_packet.light_sensor_readings[j] = light_sensor_readings[j];
              }

              printf("Send Light Sensor Data\n");

              NETSTACK_NETWORK.output(&dest_addr); // Packet transmission
            }
          }
          break;
        }
      }
      if (new_node)
      {
        printf("NEW NODE\n");
        for (int i = 0; i < 10; i++)
        {
          if (node_list[i].node_id == 0)
          {
            node_list[i].node_id = received_packet_data.src_id;
            node_list[i].time_absent = 0;
            node_list[i].time_detected = curr_time;
            break;
          }
        }
      }
    }
  }
}

// Scheduler function for the sender of neighbour discovery packets
char sender_scheduler(struct rtimer *t, void *ptr)
{

  static uint16_t i = 0;
  static int r = 0;
  static int c = 0;
  static int slot_num;

  // Begin the protothread
  PT_BEGIN(&pt);

  // Get the current time stamp
  curr_timestamp = clock_time();

  printf("Start clock %lu ticks, timestamp %3lu.%03lu\n", curr_timestamp, curr_timestamp / CLOCK_SECOND,
         ((curr_timestamp % CLOCK_SECOND) * 1000) / CLOCK_SECOND);

  while (1)
  {

    if (r == row || c == col)
    {
      // radio on
      NETSTACK_RADIO.on();

      // send NUM_SEND number of neighbour discovery beacon packets
      for (i = 0; i < NUM_SEND; i++)
      {

        // Initialize the nullnet module with information of packet to be trasnmitted
        nullnet_buf = (uint8_t *)&data_packet; // data transmitted
        nullnet_len = sizeof(data_packet);     // length of data transmitted

        data_packet.seq++;

        curr_timestamp = clock_time();

        data_packet.timestamp = curr_timestamp;

        slot_num = r * N + c;

        printf("Send seq# %lu  @ slot# %d %8lu ticks   %3lu.%03lu\n", data_packet.seq, slot_num, curr_timestamp, curr_timestamp / CLOCK_SECOND, ((curr_timestamp % CLOCK_SECOND) * 1000) / CLOCK_SECOND);

        NETSTACK_NETWORK.output(&dest_addr); // Packet transmission

        // wait for WAKE_TIME before sending the next packet
        if (i != (NUM_SEND - 1))
        {

          rtimer_set(t, RTIMER_TIME(t) + TIME_SLOT, 1, (rtimer_callback_t)sender_scheduler, ptr);
          PT_YIELD(&pt);
        }
      }

      // radio off
      NETSTACK_RADIO.off();
    }

    else
    {
      // radio off
      // NETSTACK_RADIO.off();

      rtimer_set(t, RTIMER_TIME(t) + TIME_SLOT, 1, (rtimer_callback_t)sender_scheduler, ptr);
      PT_YIELD(&pt);
    }

    c = (c + 1) % N;
    if (c == 0)
    {
      r = (r + 1) % N;
    }
  }

  PT_END(&pt);
}

// Main thread that handles the neighbour discovery process
PROCESS_THREAD(nbr_discovery_process, ev, data)
{

  // static struct etimer periodic_timer;

  PROCESS_BEGIN();

  // initialize data packet sent for neighbour discovery exchange
  data_packet.src_id = node_id; // Initialize the node ID
  data_packet.seq = 0;          // Initialize the sequence number of the packet

  light_sensor_packet.src_id = node_id;

  row = random_rand() % N;
  col = random_rand() % N;

  nullnet_set_input_callback(receive_packet_callback); // initialize receiver callback
  linkaddr_copy(&dest_addr, &linkaddr_null);

  printf("CC2650 neighbour discovery\n");
  printf("Node %d will be sending Discovery Packet of size %d Bytes\n", node_id, (int)sizeof(data_packet_struct));
  printf("N: %d row: %d col: %d\n", N, row, col);
  printf("Node %d will be sending Light Sensor Data Packet of size %d Bytes\n", node_id, (int)sizeof(light_sensor_packet_struct));

  // Start sender in one millisecond.
  rtimer_set(&rt, RTIMER_NOW() + (RTIMER_SECOND / 1000), 1, (rtimer_callback_t)sender_scheduler, NULL);

  PROCESS_END();
}

static void
get_light_reading()
{
  int value;
  int current_lux;

  light_counter++;

  value = opt_3001_sensor.value(0);
  if (value != CC26XX_SENSOR_READING_ERROR && light_counter == 40)
  {
    light_counter = 0;
    printf("OPT: Light=%d.%02d lux\n", value / 100, value % 100);
    current_lux = value / 100;
    for (int i = 0; i < 9; i++)
    {
      light_sensor_readings[i] = light_sensor_readings[i + 1];
    }
    light_sensor_readings[9] = current_lux;
    printf("Light: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,\n", light_sensor_readings[0], light_sensor_readings[1], light_sensor_readings[2], light_sensor_readings[3], light_sensor_readings[4], light_sensor_readings[5], light_sensor_readings[6], light_sensor_readings[7], light_sensor_readings[8], light_sensor_readings[9]);
  }

  init_opt_reading();

  unsigned long curr_time = clock_time();

  for (int i = 0; i < 10; i++)
  {
    if (node_list[i].node_id != 0)
    {

      if (node_list[i].time_absent != 0)
      {
        printf("CURRENT TIME %lu TIME ABSENT %lu\n", curr_time / CLOCK_SECOND, node_list[i].time_absent / CLOCK_SECOND);
        if ((curr_time / CLOCK_SECOND - node_list[i].time_absent / CLOCK_SECOND) >= 30)
        {
          printf("%lu ABSENT %lu\n", node_list[i].time_absent / CLOCK_SECOND, node_list[i].node_id);
          node_list[i].node_id = 0;
          node_list[i].time_detected = 0;
          node_list[i].time_absent = 0;
        }
      }
      break;
    }
  }
}

static void
init_opt_reading(void)
{
  SENSORS_ACTIVATE(opt_3001_sensor);
}

PROCESS_THREAD(light_sensor_process, ev, data)
{

  PROCESS_BEGIN();
  init_opt_reading();

  while (1)
  {

    etimer_set(&timer_etimer, CLOCK_SECOND); // sample every seconds
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
    get_light_reading();
  }

  PROCESS_END();
}