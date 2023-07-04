/*
 * Copyright (C) 2015, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>

#include "contiki.h"
#include "sys/rtimer.h"

#include "board-peripherals.h"

#include <stdint.h>

PROCESS(process_rtimer, "RTimer");
AUTOSTART_PROCESSES(&process_rtimer);

// static int counter_rtimer;
static struct rtimer timer_rtimer;
static rtimer_clock_t timeout_rtimer = RTIMER_SECOND / 16;
/*---------------------------------------------------------------------------*/
static void init_mpu_reading(void);
static void get_mpu_reading(void);

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

/*---------------------------------------------------------------------------*/
void do_rtimer_timeout(struct rtimer *timer, void *ptr)
{
    /* rtimer period 50ms = 20Hz*/
    /*
    clock_time_t t;

    rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);

    int s, ms1, ms2, ms3;
    s = clock_time() / CLOCK_SECOND;
    ms1 = (clock_time() % CLOCK_SECOND) * 10 / CLOCK_SECOND;
    ms2 = ((clock_time() % CLOCK_SECOND) * 100 / CLOCK_SECOND) % 10;
    ms3 = ((clock_time() % CLOCK_SECOND) * 1000 / CLOCK_SECOND) % 10;

    counter_rtimer++;
    printf(": %d (cnt) %d (ticks) %d.%d%d%d (sec) \n", counter_rtimer, clock_time(), s, ms1, ms2, ms3);
    */
    rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);
    get_mpu_reading();
}

static void
get_mpu_reading()
{
    int value;

    printf("MPU Gyro: X=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_X);
    print_mpu_reading(value);
    printf(" deg/sec\n");

    printf("MPU Gyro: Y=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Y);
    print_mpu_reading(value);
    printf(" deg/sec\n");

    printf("MPU Gyro: Z=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Z);
    print_mpu_reading(value);
    printf(" deg/sec\n");

    printf("MPU Acc: X=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_X);
    print_mpu_reading(value);
    printf(" G\n");

    printf("MPU Acc: Y=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Y);
    print_mpu_reading(value);
    printf(" G\n");

    printf("MPU Acc: Z=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Z);
    print_mpu_reading(value);
    printf(" G\n");
}

static void
init_mpu_reading(void)
{
    mpu_9250_sensor.configure(SENSORS_ACTIVE, MPU_9250_SENSOR_TYPE_ALL);
}

PROCESS_THREAD(process_rtimer, ev, data)
{
    PROCESS_BEGIN();
    init_mpu_reading();

    while (1)
    {
        rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);
        PROCESS_YIELD();
    }

    PROCESS_END();
}