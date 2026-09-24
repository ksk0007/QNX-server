#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include <sched.h>
#include <cerrno>


#include "player.h"
#include "game_world.h"
#include "config.h"
#include "network.h"
#include "player.h"


static bool server_running = true;
static unsigned long long total_ticks = 0;
static unsigned long long deadline_misses = 0;

static long long min_tick_interval_ns = 0;
static long long max_tick_interval_ns = 0;
static long long total_tick_interval_ns = 0;

static long long previous_tick_time_ns = 0;
static const long long DEADLINE_TOLERANCE_NS = 100000LL;




/* ============================================================
   Timing Helpers
   ============================================================ */

static long long TimespecToNanoseconds(const struct timespec& time)
{
    return ((long long)time.tv_sec * 1000000000LL)
           + time.tv_nsec;
}


static long long GetMonotonicTimeNanoseconds()
{
    struct timespec current_time;

    clock_gettime(CLOCK_MONOTONIC, &current_time);

    return TimespecToNanoseconds(current_time);
}




/* ============================================================
   Configure Real-Time Thread
   ============================================================ */

static void ConfigureRealtimeThread()
{
    struct sched_param scheduling_parameters;

    scheduling_parameters.sched_priority = RT_PRIORITY;

    if (pthread_setschedparam(pthread_self(),
                              RT_SCHEDULER,
                              &scheduling_parameters) != 0)
    {
        perror("[WARNING] pthread_setschedparam()");
    }
    else
    {
        printf("[RT] Scheduling policy: SCHED_FIFO\n");
        printf("[RT] Priority: %d\n", RT_PRIORITY);
    }
}


/* ============================================================
   Update Game World
   ============================================================ */


static void UpdateGameWorld()
{
    GameWorld_Update();
    GameWorld_BroadcastState();
}


/* ============================================================
   Main
   ============================================================ */

int main()
{
    printf("\n");
    printf("============================================\n");
    printf("        QNX AUTHORITATIVE GAME SERVER       \n");
    printf("============================================\n");
    printf("OS: QNX Neutrino\n");
    printf("Protocol: UDP\n");
    printf("Port: %d\n", SERVER_PORT);
    printf("Tick Rate: %d Hz\n", TICK_RATE_HZ);
    printf("============================================\n\n");


    if (!Network_Init())
    {
        printf("[FATAL] Network initialization failed\n");
        return EXIT_FAILURE;
    }



    Player_Init();

    GameWorld_Init();

    ConfigureRealtimeThread();

    printf("[SERVER] Server started successfully\n");
    printf("[SERVER] Waiting for clients...\n\n");


    struct timespec next_wakeup;

    clock_gettime(CLOCK_MONOTONIC, &next_wakeup);


    long long next_wakeup_ns =
        ((long long)next_wakeup.tv_sec * 1000000000LL)
        + next_wakeup.tv_nsec;





while (server_running)
{
    /*
       Schedule and wait for the next absolute tick.
    */

    next_wakeup_ns += TICK_PERIOD_NS;

    next_wakeup.tv_sec =
        next_wakeup_ns / 1000000000LL;

    next_wakeup.tv_nsec =
        next_wakeup_ns % 1000000000LL;

    clock_nanosleep(CLOCK_MONOTONIC,
                    TIMER_ABSTIME,
                    &next_wakeup,
                    NULL);


    /*
       Measure actual tick start time after waking.
    */

    long long current_tick_time_ns =
        GetMonotonicTimeNanoseconds();


    /*
       Measure wake-up lateness.
    */

    long long wakeup_error_ns =
        current_tick_time_ns - next_wakeup_ns;

    if (wakeup_error_ns > DEADLINE_TOLERANCE_NS)
    {
        deadline_misses++;
    }




    /*
       Measure time between consecutive ticks
    */

    if (previous_tick_time_ns != 0)
    {
        long long tick_interval_ns =
            current_tick_time_ns - previous_tick_time_ns;

        total_tick_interval_ns += tick_interval_ns;

        if (min_tick_interval_ns == 0 ||
            tick_interval_ns < min_tick_interval_ns)
        {
            min_tick_interval_ns = tick_interval_ns;
        }

        if (tick_interval_ns > max_tick_interval_ns)
        {
            max_tick_interval_ns = tick_interval_ns;
        }
    }

    previous_tick_time_ns = current_tick_time_ns;

    total_ticks++;



    /*
       Receive network packets
    */

    Network_ProcessPackets();


    /*
       Update authoritative game world
    */



    UpdateGameWorld();
}


/*
   Shutdown server after the main loop exits.
*/

Network_Close();

printf("[SERVER] Server stopped\n");

return EXIT_SUCCESS;
}
