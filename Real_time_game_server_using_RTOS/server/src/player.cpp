#include "player.h"

#include <cstdio>
#include <cstring>
#include <arpa/inet.h>


/* ============================================================
   Internal Player Storage
   ============================================================ */

static Player players[MAX_PLAYERS];


/* ============================================================
   Get Spawn Position For A Player ID
   ============================================================ */

static void GetSpawnPosition(int player_id, float* out_x, float* out_y)
{
    if (player_id == 1)
    {
        *out_x = PLAYER1_SPAWN_X;
        *out_y = PLAYER1_SPAWN_Y;
    }
    else
    {
        *out_x = PLAYER2_SPAWN_X;
        *out_y = PLAYER2_SPAWN_Y;
    }
}


/* ============================================================
   Initialize All Player Slots
   ============================================================ */

void Player_Init()
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        players[i].active = false;
        players[i].id = i + 1;

        float spawn_x, spawn_y;
        GetSpawnPosition(players[i].id, &spawn_x, &spawn_y);

        players[i].x = spawn_x;
        players[i].y = spawn_y;
        players[i].rotation = 0.0f;

        players[i].velocity_y = 0.0f;
        players[i].is_grounded = true;

        players[i].health = PLAYER_MAX_HEALTH;

        players[i].input_x = 0.0f;
        players[i].is_running = false;
        players[i].jump_requested = false;
        players[i].attack_requested = false;

        players[i].last_packet_time_ns = 0;

        memset(&players[i].address,
               0,
               sizeof(players[i].address));
    }

    printf("[PLAYER] Player system initialized\n");
}



/* ============================================================
   Apply Damage To Player (clamped at 0)
   ============================================================ */

void Player_ApplyDamage(int player_id, int amount)
{
    Player* player = Player_Get(player_id);

    if (player == NULL)
        return;

    player->health -= amount;

    if (player->health < 0)
        player->health = 0;
}


/* ============================================================
   Heal Player (clamped at PLAYER_MAX_HEALTH)
   ============================================================ */

void Player_Heal(int player_id, int amount)
{
    Player* player = Player_Get(player_id);

    if (player == NULL)
        return;

    player->health += amount;

    if (player->health > PLAYER_MAX_HEALTH)
        player->health = PLAYER_MAX_HEALTH;
}




/* ============================================================
   Add New Player (auto-assign first free slot)
   ============================================================ */

int Player_Add(const struct sockaddr_in& client_address)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!players[i].active)
        {
            players[i].active = true;

            float spawn_x, spawn_y;
            GetSpawnPosition(players[i].id, &spawn_x, &spawn_y);

            players[i].x = spawn_x;
            players[i].y = spawn_y;
            players[i].rotation = 0.0f;

            players[i].velocity_y = 0.0f;
            players[i].is_grounded = true;

            players[i].health = PLAYER_MAX_HEALTH;

            players[i].input_x = 0.0f;
            players[i].is_running = false;
            players[i].jump_requested = false;
            players[i].attack_requested = false;

            players[i].address = client_address;

            players[i].last_packet_time_ns = 0;

            printf("[PLAYER] Player %d joined (spawn %.2f, %.2f)\n",
                   players[i].id, spawn_x, spawn_y);

            return players[i].id;
        }
    }

    printf("[PLAYER] Server full\n");

    return -1;
}


/* ============================================================
   Add A Player Into A Specific Requested Slot
   ============================================================ */

int Player_AddWithID(int player_id, const struct sockaddr_in& client_address)
{
    if (player_id < 1 || player_id > MAX_PLAYERS)
        return -1;

    Player* p = &players[player_id - 1];

    if (p->active)
    {
        return -1;
    }

    p->active = true;

    float spawn_x, spawn_y;
    GetSpawnPosition(p->id, &spawn_x, &spawn_y);

    p->x = spawn_x;
    p->y = spawn_y;
    p->rotation = 0.0f;

    p->velocity_y = 0.0f;
    p->is_grounded = true;

    p->health = PLAYER_MAX_HEALTH;

    p->input_x = 0.0f;
    p->is_running = false;
    p->jump_requested = false;
    p->attack_requested = false;

    p->address = client_address;
    p->last_packet_time_ns = 0;

    printf("[PLAYER] Player %d joined (spawn %.2f, %.2f)\n",
           p->id, spawn_x, spawn_y);

    return p->id;
}


/* ============================================================
   Remove Player
   ============================================================ */

void Player_Remove(int player_id)
{
    Player* player = Player_Get(player_id);

    if (player == NULL)
        return;

    player->active = false;

    memset(&player->address,
           0,
           sizeof(player->address));

    printf("[PLAYER] Player %d removed\n",
           player_id);
}


/* ============================================================
   Get Player By ID
   ============================================================ */

Player* Player_Get(int player_id)
{
    if (player_id < 1 || player_id > MAX_PLAYERS)
        return NULL;

    Player* player = &players[player_id - 1];

    if (!player->active)
        return NULL;

    return player;
}


/* ============================================================
   Find Player By IP Address and Port
   ============================================================ */

Player* Player_FindByAddress(
    const struct sockaddr_in& client_address)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (!players[i].active)
            continue;

        if (players[i].address.sin_addr.s_addr ==
                client_address.sin_addr.s_addr &&
            players[i].address.sin_port ==
                client_address.sin_port)
        {
            return &players[i];
        }
    }

    return NULL;
}


/* ============================================================
   Store Latest Player Input
   ============================================================ */

void Player_UpdateInput(
    int player_id,
    float move_x,
    bool  run,
    bool  jump,
    bool  attack)
{
    Player* player = Player_Get(player_id);

    if (player == NULL)
        return;

    if (move_x > 1.0f)  move_x = 1.0f;
    if (move_x < -1.0f) move_x = -1.0f;

    player->input_x = move_x;
    player->is_running = run;

    if (jump)
        player->jump_requested = true;

    if (attack)
        player->attack_requested = true;
}


/* ============================================================
   Count Active Players
   ============================================================ */

int Player_GetActiveCount()
{
    int count = 0;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (players[i].active)
            count++;
    }

    return count;
}
