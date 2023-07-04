#include <stdio.h>

#include "contiki.h"
#include "sys/etimer.h"
#include "buzzer.h"

PROCESS(process_mario, "Mario 1UP");
AUTOSTART_PROCESSES(&process_mario);

static int counter_etimer;
int buzzerMelody[18] = {0, 1975, 2637, 0, 0, 0, 0, 1318, 1568, 2637, 2093, 2349, 3136, 3136, 3136, 0, 0, 0}; // coin B6 E7 mario 1up music E6 G6 E7 C7 D7 G7, 0= REST
int buzzerRhythm[18] = {0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0};                               // Rhythm matching melody 1 = buzz, 0 = REST

void play_note()
{
    clock_time_t t;
    int beat, s, ms1, ms2, ms3;
    t = clock_time();
    s = t / CLOCK_SECOND;
    ms1 = (t % CLOCK_SECOND) * 10 / CLOCK_SECOND;
    ms2 = ((t % CLOCK_SECOND) * 100 / CLOCK_SECOND) % 10;
    ms3 = ((t % CLOCK_SECOND) * 1000 / CLOCK_SECOND) % 10;
    beat = counter_etimer % 18;

    counter_etimer++;
    printf("Time(E): %d (cnt) %ld (ticks) %d.%d%d%d (sec) \n", counter_etimer, t, s, ms1, ms2, ms3);
    // buzz note if Rhythm == 1
    if (buzzer_state() && buzzerRhythm[beat] == 0)
        buzzer_stop();
    else
        buzzer_start(buzzerMelody[beat]);
}

PROCESS_THREAD(process_mario, ev, data)
{
    static struct etimer timer_etimer;

    PROCESS_BEGIN();
    buzzer_init();
    printf(" The value of CLOCK_SECOND is %d \n", CLOCK_SECOND);

    while (1)
    {
        etimer_set(&timer_etimer, CLOCK_SECOND / 6); // 0.25s timer
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
        play_note();
    }

    PROCESS_END();
}
