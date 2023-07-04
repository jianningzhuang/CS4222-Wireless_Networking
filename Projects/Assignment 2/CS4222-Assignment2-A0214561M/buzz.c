#include <stdio.h>
#include <stdint.h>

#include "contiki.h"
#include "sys/etimer.h"
#include "buzzer.h"
#include "board-peripherals.h"

/*---------------------------------------------------------------------------*/

PROCESS(process_mario, "Mario coin 1UP");
AUTOSTART_PROCESSES(&process_mario);

/*---------------------------------------------------------------------------*/

static struct etimer timer_etimer;
static int counter_etimer = 0;

static int prev_lux = -1; // default/NULL value = -1

static int state = 1; // 1 => IDLE, 2 => Active, program starts in IDLE 1

/*---------------------------------------------------------------------------*/

static void init_opt_reading(void);
static bool process_light_reading(void);

static void init_mpu_reading(void);
static void print_mpu_reading(int);
static bool process_mpu_reading(void);

/*---------------------------------------------------------------------------*/

void do_etimer_timeout()
{

    counter_etimer++;
    // printf("Counter: %d \n", counter_etimer);

    if (state == 1) // check for significant motion
    {
        if (process_mpu_reading())
        {
            counter_etimer = 0;
            state = 2;
            buzzer_start(1046);
        }
    }
    else
    {
        if (counter_etimer % 24 >= 12)
        {
            if (buzzer_state())
            {
                buzzer_stop();
            }
        }
        else
        {
            if (!buzzer_state())
            {
                buzzer_start(1046);
            }
        }

        if (process_light_reading())
        {
            counter_etimer = 0;
            state = 1;
            if (buzzer_state())
            {
                buzzer_stop();
            }
        }
    }
}

static void
init_opt_reading(void)
{
    SENSORS_ACTIVATE(opt_3001_sensor);
}

static bool
process_light_reading()
{
    int value;
    int current_lux;

    value = opt_3001_sensor.value(0);
    if (value != CC26XX_SENSOR_READING_ERROR)
    {
        printf("OPT: Light=%d.%02d lux\n", value / 100, value % 100);
        current_lux = value / 100;
        if (prev_lux == -1)
        {
            prev_lux = current_lux;
        }
        int difference = current_lux - prev_lux;
        printf("OPT: Difference=%d lux\n", difference);
        if (difference > 300 || difference < -300)
        {
            prev_lux = -1;
            init_opt_reading();
            return true;
        }
        prev_lux = current_lux;
    }
    else
    {
        printf("OPT: Light Sensor's Warming Up\n\n");
    }
    init_opt_reading();

    return false;
}

static void
init_mpu_reading(void)
{
    mpu_9250_sensor.configure(SENSORS_ACTIVE, MPU_9250_SENSOR_TYPE_ALL);
}

static void
print_mpu_reading(int reading)
{
    if (reading < 0)
    {
        printf("-");
        reading = -reading;
    }

    printf("%d.%02d", reading / 100, reading % 100);
}

static bool
process_mpu_reading()
{
    int value;

    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_X);
    if (value < -175 || value > 175)
    {
        printf("significant LEFT-RIGHT movement \n");
        print_mpu_reading(value);
        printf(" G\n");
        return true;
    }

    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Y);
    if (value < -175 || value > 175)
    {
        printf("significant FRONT-BACK movement \n");
        print_mpu_reading(value);
        printf(" G\n");
        return true;
    }

    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Z);
    if (value < -225 || value > 150)
    {
        printf("significant UP-DOWN movement \n");
        print_mpu_reading(value);
        printf(" G\n");
        return true;
    }

    return false;
}

PROCESS_THREAD(process_mario, ev, data)
{

    PROCESS_BEGIN();
    buzzer_init();
    init_opt_reading();
    init_mpu_reading();
    printf(" The value of CLOCK_SECOND is %d \n", CLOCK_SECOND);

    while (1)
    {
        if (state == 1)
        {
            etimer_set(&timer_etimer, CLOCK_SECOND / 16); // 16 Hz MPU reading
        }
        else if (state == 2)
        {
            etimer_set(&timer_etimer, CLOCK_SECOND / 4); // 4Hz Buzz and Light reading => 12 counts => 3 seconds buzz
        }
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
        do_etimer_timeout();
    }

    PROCESS_END();
}