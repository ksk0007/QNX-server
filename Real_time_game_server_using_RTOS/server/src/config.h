#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
   Network Configuration
   ============================================================ */

#define SERVER_PORT 7777
#define BUFFER_SIZE 1024

/* ============================================================
   Game Server Configuration
   ============================================================ */

#define TICK_RATE_HZ 240

#define TICK_PERIOD_NS \
    (1000000000LL / TICK_RATE_HZ)

/* Maximum players currently supported */
#define MAX_PLAYERS 2

/* ============================================================
   Real-Time Configuration
   ============================================================ */

#define RT_PRIORITY 20

#define RT_SCHEDULER SCHED_FIFO

/* ============================================================
   Player Movement Configuration
   Match these with Unity PlayerController values
   ============================================================ */

#define PLAYER_WALK_SPEED       10.0f
#define PLAYER_RUN_SPEED        20.0f
#define PLAYER_AIR_SPEED         3.0f

#define PLAYER_JUMP_IMPULSE     10.0f
#define PLAYER_LONG_JUMP_IMPULSE 15.0f

#define GRAVITY                -30.0f
#define TERMINAL_VELOCITY      -25.0f

/* ============================================================
   World / Ground Configuration
   ============================================================ */

/*
   Unity ground level.

   Currently set to Y = 0.
   The server will prevent players from falling below this Y.
*/
#define GROUND_LEVEL_Y 0.0f

/* ============================================================
   Player Spawn Configuration
   ============================================================ */

/*
   Player 1 spawn position
*/
#define PLAYER1_SPAWN_X 0.0f
#define PLAYER1_SPAWN_Y 0.0f

/*
   Player 2 spawn position
*/
#define PLAYER2_SPAWN_X 2.0f
#define PLAYER2_SPAWN_Y 0.0f

/* ============================================================
   Player Health Configuration
   ============================================================ */

#define PLAYER_MAX_HEALTH 100

/* ============================================================
   Connection Configuration
   ============================================================ */

#define CLIENT_TIMEOUT_SECONDS 15

#endif /* CONFIG_H */
