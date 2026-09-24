#ifndef WORLD_COLLISION_H
#define WORLD_COLLISION_H

#include "player.h"

/*
   Server-side copy of the Unity Ground CompositeCollider2D.

   Coordinates were exported from Unity in world space.
   Player collider:
       Capsule width  = 0.6600
       Capsule height = 1.62529
       Offset         = (-0.007, -0.234)

   The collision solver uses an axis-aligned bounding box approximation
   of the capsule for deterministic server-side movement.
*/

void WorldCollision_Init();

/* Move one player by delta X/Y and resolve against the exported world. */
void WorldCollision_MovePlayer(Player* player, float delta_x, float delta_y);

/* Return true if the player's collider overlaps solid world geometry. */
bool WorldCollision_PlayerOverlaps(const Player* player);

#endif /* WORLD_COLLISION_H */
