#ifndef NETWORK_H
#define NETWORK_H

#include <netinet/in.h>

/* ============================================================
   Network Initialization
   ============================================================ */

bool Network_Init();

void Network_Close();


/* ============================================================
   Network Packet Processing
   ============================================================ */

void Network_ProcessPackets();


/* ============================================================
   Send Data To Client
   ============================================================ */

void Network_SendToClient(
    const char* message,
    const struct sockaddr_in& client_address);


/* ============================================================
   Send Data To Specific Player
   ============================================================ */

void Network_SendToPlayer(
    int player_id,
    const char* message);


/* ============================================================
   Broadcast Data To All Players
   ============================================================ */

void Network_Broadcast(
    const char* message);


/* ============================================================
   Network Status
   ============================================================ */

int Network_GetSocket();

#endif /* NETWORK_H */
