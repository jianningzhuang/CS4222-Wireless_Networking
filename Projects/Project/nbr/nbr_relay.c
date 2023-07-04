/*
 * CS4222/5422: Project
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

// Identification information of the node

// Configures the wake-up timer for neighbour discovery
#define TIME_SLOT RTIMER_SECOND / 40 // 10 HZ, 0.1s
#define N 20

#define THREE_METER_THRESHOLD -50

static int wait_for_light_sensor_readings = 0;

static int row;
static int col;

typedef struct
{
  unsigned long node_id;
  unsigned long time_detected;
  unsigned long time_absent;

} node;

static volatile node light_sensor_node; // should only detect 1 light sensor sender (from SLACK)

// For neighbour discovery, we would like to send message to everyone. We use Broadcast address:
linkaddr_t dest_addr;
linkaddr_t light_sensor_node_addr;

#define NUM_SEND 2
/*---------------------------------------------------------------------------*/
typedef struct
{
  unsigned long src_id;
  unsigned long timestamp;
  unsigned long seq;
  int light_sensor;

} light_sensor_packet_struct;

typedef struct
{
  unsigned long src_id;
  unsigned long timestamp;
  unsigned long seq;

} relay_packet_struct;

typedef struct
{
  unsigned long src_id;
  unsigned long timestamp;

} request_packet_struct;

typedef struct
{
  unsigned long src_id;
  unsigned long timestamp;
  int light_sensor_readings[10];

} light_sensor_readings_packet_struct;

/*---------------------------------------------------------------------------*/
// duty cycle = WAKE_TIME / (WAKE_TIME + SLEEP_SLOT * SLEEP_CYCLE)
/*---------------------------------------------------------------------------*/

// sender timer implemented using rtimer
static struct rtimer rt;

// Protothread variable
static struct pt pt;

// Structure holding the data to be transmitted/received
static light_sensor_packet_struct light_sensor_packet;

static relay_packet_struct relay_packet;

static request_packet_struct request_packet;

static light_sensor_readings_packet_struct light_sensor_readings_packet;

// Current time stamp of the node
unsigned long curr_timestamp;

// Starts the main contiki neighbour discovery process
PROCESS(nbr_discovery_process, "cc2650 neighbour discovery process");
AUTOSTART_PROCESSES(&nbr_discovery_process);

// Function called after reception of a packet
void receive_packet_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest)
{

  // Check if the received packet size matches with what we expect it to be

  if (len == sizeof(light_sensor_packet))
  {

    static light_sensor_packet_struct received_packet_data;

    // Copy the content of packet into the data structure
    memcpy(&received_packet_data, data, len);

    // Print the details of the received packet
    printf("Received neighbour discovery packet %lu with rssi %d from light sensor %ld\n", received_packet_data.seq, (signed int)packetbuf_attr(PACKETBUF_ATTR_RSSI), received_packet_data.src_id);

    int rssi = (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI);

    unsigned long curr_time = clock_time();

    if (rssi < THREE_METER_THRESHOLD && light_sensor_node.node_id != 0)
    {
      printf("AWAY\n");
      light_sensor_node.time_detected = 0;
      printf("CURRENT TIME %lu TIME ABSENT %lu\n", curr_time / CLOCK_SECOND, light_sensor_node.time_absent / CLOCK_SECOND);
      if (light_sensor_node.time_absent == 0)
      {
        light_sensor_node.time_absent = curr_time;
      }
      else
      {
        if ((curr_time / CLOCK_SECOND - light_sensor_node.time_absent / CLOCK_SECOND) >= 30)
        {
          printf("%lu ABSENT %lu\n", light_sensor_node.time_absent / CLOCK_SECOND, light_sensor_node.node_id);
          light_sensor_node.node_id = 0;
          light_sensor_node.time_detected = 0;
          light_sensor_node.time_absent = 0;
        }
      }
    }
    else if (rssi >= THREE_METER_THRESHOLD)
    {
      printf("NEAR\n");
      if (light_sensor_node.node_id == 0)
      {
        printf("NEW LIGHT SENSOR NODE\n");
        light_sensor_node.node_id = received_packet_data.src_id;
        linkaddr_copy(&light_sensor_node_addr, src);
      }
      light_sensor_node.time_absent = 0;
      if (light_sensor_node.time_detected == 0)
      {
        light_sensor_node.time_detected = curr_time;
      }
      else
      {
        if ((curr_time / CLOCK_SECOND - light_sensor_node.time_detected / CLOCK_SECOND) >= 15)
        {
          printf("%lu DETECT %lu\n", light_sensor_node.time_detected / CLOCK_SECOND, light_sensor_node.node_id);
          light_sensor_node.time_detected = curr_time; // DETECT in another 15s if still in proximity
          wait_for_light_sensor_readings = 1;          // set flag
        }
      }
    }
  }

  else if (len == sizeof(light_sensor_readings_packet))
  {

    static light_sensor_readings_packet_struct received_packet_data;

    // Copy the content of packet into the data structure
    memcpy(&received_packet_data, data, len);

    // Print the details of the received packet
    printf("Received Light Sensor Data packet with rssi %d from %ld\n", (signed int)packetbuf_attr(PACKETBUF_ATTR_RSSI), received_packet_data.src_id);
    printf("Light: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,\n", received_packet_data.light_sensor_readings[0], received_packet_data.light_sensor_readings[1], received_packet_data.light_sensor_readings[2], received_packet_data.light_sensor_readings[3], received_packet_data.light_sensor_readings[4], received_packet_data.light_sensor_readings[5], received_packet_data.light_sensor_readings[6], received_packet_data.light_sensor_readings[7], received_packet_data.light_sensor_readings[8], received_packet_data.light_sensor_readings[9]);

    wait_for_light_sensor_readings = 0; // clear flag
  }
  else
  {
    printf("Unknown Data Packet Received\n");
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

    if (wait_for_light_sensor_readings)
    {
      // radio on
      NETSTACK_RADIO.on();

      nullnet_buf = (uint8_t *)&request_packet; // data transmitted
      nullnet_len = sizeof(request_packet);     // length of data transmitted
      request_packet.src_id = node_id;
      request_packet.timestamp = clock_time();

      while (wait_for_light_sensor_readings)
      {
        printf("Sending Request for Light Sensor Readings\n");
        NETSTACK_NETWORK.output(&light_sensor_node_addr); // Packet transmission
        rtimer_set(t, RTIMER_TIME(t) + TIME_SLOT, 1, (rtimer_callback_t)sender_scheduler, ptr);
        PT_YIELD(&pt);
      }

      // radio off
      NETSTACK_RADIO.off();
    }

    if (r == row || c == col)
    {
      // radio on
      NETSTACK_RADIO.on();

      // send NUM_SEND number of neighbour discovery beacon packets
      for (i = 0; i < NUM_SEND; i++)
      {

        // Initialize the nullnet module with information of packet to be trasnmitted
        nullnet_buf = (uint8_t *)&relay_packet; // data transmitted
        nullnet_len = sizeof(relay_packet);     // length of data transmitted

        relay_packet.seq++;

        curr_timestamp = clock_time();

        relay_packet.timestamp = curr_timestamp;

        slot_num = r * N + c;

        printf("Send seq# %lu  @ slot# %d %8lu ticks   %3lu.%03lu\n", relay_packet.seq, slot_num, curr_timestamp, curr_timestamp / CLOCK_SECOND, ((curr_timestamp % CLOCK_SECOND) * 1000) / CLOCK_SECOND);

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

    if (c == 0)
    { // check absent every 10/N seconds when discovering
      curr_timestamp = clock_time();

      if (light_sensor_node.time_absent != 0)
      {
        printf("CURRENT TIME %lu TIME ABSENT %lu\n", curr_timestamp / CLOCK_SECOND, light_sensor_node.time_absent / CLOCK_SECOND);
        if ((curr_timestamp / CLOCK_SECOND - light_sensor_node.time_absent / CLOCK_SECOND) >= 30)
        {
          printf("%lu ABSENT %lu\n", light_sensor_node.time_absent / CLOCK_SECOND, light_sensor_node.node_id);
          light_sensor_node.node_id = 0;
          light_sensor_node.time_detected = 0;
          light_sensor_node.time_absent = 0;
        }
      }
      if (light_sensor_node.time_detected != 0) // moved away after in proximity without sending packet to detect away
      {
        printf("CURRENT TIME %lu TIME LAST DETECTED %lu\n", curr_timestamp / CLOCK_SECOND, light_sensor_node.time_detected / CLOCK_SECOND);
        if ((curr_timestamp / CLOCK_SECOND - light_sensor_node.time_detected / CLOCK_SECOND) >= 30)
        {
          printf("%lu ABSENT %lu\n", light_sensor_node.time_detected / CLOCK_SECOND, light_sensor_node.node_id);
          light_sensor_node.node_id = 0;
          light_sensor_node.time_detected = 0;
          light_sensor_node.time_absent = 0;
        }
      }
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
  relay_packet.src_id = node_id; // Initialize the node ID
  relay_packet.seq = 0;          // Initialize the sequence number of the packet

  row = random_rand() % N;
  col = random_rand() % N;

  nullnet_set_input_callback(receive_packet_callback); // initialize receiver callback
  linkaddr_copy(&dest_addr, &linkaddr_null);

  printf("CC2650 neighbour discovery\n");
  printf("Node %d will be sending packet of size %d Bytes\n", node_id, (int)sizeof(relay_packet_struct));
  printf("N: %d row: %d col: %d\n", N, row, col);

  // Start sender in one millisecond.
  rtimer_set(&rt, RTIMER_NOW() + (RTIMER_SECOND / 1000), 1, (rtimer_callback_t)sender_scheduler, NULL);

  PROCESS_END();
}
