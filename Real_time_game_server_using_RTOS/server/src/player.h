#ifndef PLAYER_H
#define PLAYER_H

#include <netinet/in.h>

#include "config.h"


/* ============================================================
   Player State
   ============================================================ */

struct Player
{
    bool active;

    int id;

    /* Server-authoritative state */
    float x;
    float y;
    float rotation;

    float velocity_y;
    bool  is_grounded;

    int health;

    /* Latest input received from the client */
    float input_x;
    bool  is_running;
    bool  jump_requested;
    bool  attack_requested;

    struct sockaddr_in address;

    long long last_packet_time_ns;
};



/* ============================================================
   Player Management
   ============================================================ */

void Player_Init();

int Player_Add(const struct sockaddr_in& client_address);

int Player_AddWithID(int player_id, const struct sockaddr_in& client_address);

void Player_Remove(int player_id);

Player* Player_Get(int player_id);

Player* Player_FindByAddress(
    const struct sockaddr_in& client_address);


/* ============================================================
   Server-Authoritative Input
   ============================================================ */

void Player_UpdateInput(
    int player_id,
    float move_x,
    bool  run,
    bool  jump,
    bool  attack);



/* ============================================================
   Health Management
   ============================================================ */

void Player_ApplyDamage(int player_id, int amount);

void Player_Heal(int player_id, int amount);


/* ============================================================
   Player Statistics
   ============================================================ */

int Player_GetActiveCount();

#endif /* PLAYER_H */
