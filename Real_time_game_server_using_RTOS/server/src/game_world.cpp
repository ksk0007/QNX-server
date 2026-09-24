#include "game_world.h"
#include "player.h"
#include "network.h"
#include "config.h"
#include "world_collision.h"

#include <cstdio>
#include <cstring>
#include <cmath>


/* ============================================================
   Tunable Mob / Food Constants
   (move these into config.h later if you want them shared)
   ============================================================ */

static const float MOB_PATROL_LEFT_X   = 2.0f;
static const float MOB_PATROL_RIGHT_X  = 8.0f;
static const float MOB_SPEED           = 1.5f;   /* units/sec */
static const int   MOB_MAX_HEALTH      = 100;
static const float MOB_ATTACK_RADIUS   = 0.8f;
static const int   MOB_ATTACK_DAMAGE   = 10;
static const float MOB_ATTACK_COOLDOWN = 1.0f;   /* sec between hits, per player */

static const float FOOD_SPAWN_X        = 6.0f;
static const float FOOD_Y_OFFSET       = 0.3f;   /* sits slightly above ground */
static const float FOOD_PICKUP_RADIUS  = 0.6f;
static const int   FOOD_HEAL_AMOUNT    = 20;
static const float FOOD_RESPAWN_TIME   = 8.0f;   /* sec */


/* ============================================================
   Mob (Knight) State
   ============================================================ */

struct Mob
{
    int   id;
    float x;
    float y;
    int   direction;   /* -1 = moving left, 1 = moving right */
    int   health;
};

static Mob mob;


/* ============================================================
   Food (Apple) State
   ============================================================ */

struct Food
{
    int   id;
    float x;
    float y;
    bool  active;
    float respawn_timer;
};

static Food food;


/* ============================================================
   Per-Player Mob Attack Cooldown
   (index 0 unused, players are 1-based)
   ============================================================ */

static float player_mob_hit_cooldown[MAX_PLAYERS + 1];


/* ============================================================
   Game World Initialization
   ============================================================ */

void GameWorld_Init()
{
    WorldCollision_Init();

    mob.id = 1;
    mob.x = MOB_PATROL_LEFT_X;
    mob.y = GROUND_LEVEL_Y;
    mob.direction = 1;
    mob.health = MOB_MAX_HEALTH;

    food.id = 1;
    food.x = FOOD_SPAWN_X;
    food.y = GROUND_LEVEL_Y + FOOD_Y_OFFSET;
    food.active = true;
    food.respawn_timer = 0.0f;

    for (int i = 0; i <= MAX_PLAYERS; i++)
    {
        player_mob_hit_cooldown[i] = 0.0f;
    }

    printf("[GAME] Game world initialized\n");
    /* Player_Init() already sets correct spawn positions,
       nothing else to reset here. */
}


/* ============================================================
   Process Player Movement (authoritative: horizontal input,
   gravity, jumping, and swept world box collisions happen here)
   ============================================================ */

void GameWorld_ProcessMovement()
{
    const float delta_time = 1.0f / (float)TICK_RATE_HZ;

    for (int i = 1; i <= MAX_PLAYERS; i++)
    {
        Player* player = Player_Get(i);

        if (player == NULL)
            continue;

        /* ---------------- Horizontal movement ---------------- */

        float speed;

        if (!player->is_grounded)
        {
            speed = PLAYER_AIR_SPEED;
        }
        else if (player->is_running)
        {
            speed = PLAYER_RUN_SPEED;
        }
        else
        {
            speed = PLAYER_WALK_SPEED;
        }

        /* ---------------- Calculate Movement ---------------- */

        float delta_x = player->input_x * speed * delta_time;

        /* ---------------- Jumping ---------------- */

        if (player->jump_requested && player->is_grounded)
        {
            player->velocity_y = player->is_running
                ? PLAYER_LONG_JUMP_IMPULSE
                : PLAYER_JUMP_IMPULSE;

            player->is_grounded = false;
        }

        player->jump_requested = false;

        /* ---------------- Gravity ---------------- */

        player->velocity_y += GRAVITY * delta_time;

        if (player->velocity_y < TERMINAL_VELOCITY)
        {
            player->velocity_y = TERMINAL_VELOCITY;
        }

        float delta_y = player->velocity_y * delta_time;

        /* ---------------- World Collision ---------------- */

        WorldCollision_MovePlayer(
            player,
            delta_x,
            delta_y
        );

        player->rotation = 0.0f;
    }
}


/* ============================================================
   Process Mob Patrol Movement
   ============================================================ */

void GameWorld_ProcessMob()
{
    const float delta_time =
        1.0f / (float)TICK_RATE_HZ;

    mob.x += mob.direction * MOB_SPEED * delta_time;

    if (mob.x >= MOB_PATROL_RIGHT_X)
    {
        mob.x = MOB_PATROL_RIGHT_X;
        mob.direction = -1;
    }
    else if (mob.x <= MOB_PATROL_LEFT_X)
    {
        mob.x = MOB_PATROL_LEFT_X;
        mob.direction = 1;
    }
}


/* ============================================================
   Process Food Respawn Timer
   ============================================================ */

void GameWorld_ProcessFood()
{
    const float delta_time =
        1.0f / (float)TICK_RATE_HZ;

    if (food.active)
        return;

    food.respawn_timer -= delta_time;

    if (food.respawn_timer <= 0.0f)
    {
        food.active = true;
        food.x = FOOD_SPAWN_X;
        food.y = GROUND_LEVEL_Y + FOOD_Y_OFFSET;

        printf("[GAME] Food respawned\n");
    }
}


/* ============================================================
   Check Mob/Food Collisions Against Players
   ============================================================ */

void GameWorld_CheckCollisions()
{
    const float delta_time =
        1.0f / (float)TICK_RATE_HZ;

    for (int i = 1; i <= MAX_PLAYERS; i++)
    {
        Player* player = Player_Get(i);

        if (player == NULL)
            continue;

        /* -------- Mob attack -------- */

        if (player_mob_hit_cooldown[i] > 0.0f)
        {
            player_mob_hit_cooldown[i] -= delta_time;
        }

        float mob_dx = player->x - mob.x;
        float mob_dy = player->y - mob.y;
        float mob_dist = sqrtf(mob_dx * mob_dx + mob_dy * mob_dy);

        if (mob_dist <= MOB_ATTACK_RADIUS &&
            player_mob_hit_cooldown[i] <= 0.0f)
        {
            Player_ApplyDamage(player->id, MOB_ATTACK_DAMAGE);
            player_mob_hit_cooldown[i] = MOB_ATTACK_COOLDOWN;

            printf("[GAME] Mob hit Player %d (HP now %d)\n",
                   player->id, player->health);
        }

        /* -------- Food pickup -------- */

        if (food.active)
        {
            float food_dx = player->x - food.x;
            float food_dy = player->y - food.y;
            float food_dist = sqrtf(food_dx * food_dx + food_dy * food_dy);

            if (food_dist <= FOOD_PICKUP_RADIUS)
            {
                food.active = false;
                food.respawn_timer = FOOD_RESPAWN_TIME;

                Player_Heal(player->id, FOOD_HEAL_AMOUNT);

                printf("[GAME] Player %d ate the food (HP now %d)\n",
                       player->id, player->health);
            }
        }
    }
}


/* ============================================================
   Update Authoritative Game State
   ============================================================ */

void GameWorld_Update()
{
    GameWorld_ProcessMovement();
    GameWorld_ProcessMob();
    GameWorld_ProcessFood();
    GameWorld_CheckCollisions();
}


/* ============================================================
   Broadcast Current World State
   ============================================================ */

void GameWorld_BroadcastState()
{
    char state_message[512];

    for (int i = 1; i <= MAX_PLAYERS; i++)
    {
        Player* player = Player_Get(i);

        if (player == NULL)
            continue;

        snprintf(state_message,
                 sizeof(state_message),
                 "STATE PLAYER_ID=%d X=%.2f Y=%.2f ROT=%.2f HP=%d GROUNDED=%d\n",
                 player->id,
                 player->x,
                 player->y,
                 player->rotation,
                 player->health,
                 player->is_grounded ? 1 : 0);

        Network_Broadcast(state_message);
    }

    snprintf(state_message,
             sizeof(state_message),
             "MOB_STATE ID=%d X=%.2f Y=%.2f DIR=%d HP=%d\n",
             mob.id,
             mob.x,
             mob.y,
             mob.direction,
             mob.health);

    Network_Broadcast(state_message);

    snprintf(state_message,
             sizeof(state_message),
             "FOOD_STATE ID=%d X=%.2f Y=%.2f ACTIVE=%d\n",
             food.id,
             food.x,
             food.y,
             food.active ? 1 : 0);

    Network_Broadcast(state_message);
}
