#ifndef GAME_WORLD_H
#define GAME_WORLD_H

/* ============================================================
   Game World Initialization
   ============================================================ */

void GameWorld_Init();


/* ============================================================
   Update Authoritative Game State
   ============================================================ */

void GameWorld_Update();


/* ============================================================
   Process Player Movement
   ============================================================ */

void GameWorld_ProcessMovement();


/* ============================================================
   Send Current World State To Players
   ============================================================ */

void GameWorld_BroadcastState();

#endif /* GAME_WORLD_H */
