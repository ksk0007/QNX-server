#include "network.h"
#include "config.h"
#include "player.h"
#include "game_world.h"

#include <cstdio>
#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <sys/socket.h>


/* ============================================================
   Internal Network Variables
   ============================================================ */

static int server_socket_fd = -1;


/* ============================================================
   Initialize UDP Network
   ============================================================ */

bool Network_Init()
{
    server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (server_socket_fd < 0)
    {
        perror("[ERROR] socket()");
        return false;
    }

    int reuse = 1;

    if (setsockopt(server_socket_fd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &reuse,
                   sizeof(reuse)) < 0)
    {
        perror("[ERROR] setsockopt()");

        close(server_socket_fd);
        server_socket_fd = -1;

        return false;
    }

    int flags = fcntl(server_socket_fd, F_GETFL, 0);

    if (flags < 0)
    {
        perror("[ERROR] fcntl(F_GETFL)");

        close(server_socket_fd);
        server_socket_fd = -1;

        return false;
    }

    if (fcntl(server_socket_fd,
              F_SETFL,
              flags | O_NONBLOCK) < 0)
    {
        perror("[ERROR] fcntl(F_SETFL)");

        close(server_socket_fd);
        server_socket_fd = -1;

        return false;
    }

    struct sockaddr_in server_address;

    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SERVER_PORT);

    if (bind(server_socket_fd,
             (struct sockaddr*)&server_address,
             sizeof(server_address)) < 0)
    {
        perror("[ERROR] bind()");

        close(server_socket_fd);
        server_socket_fd = -1;

        return false;
    }

    printf("[NETWORK] UDP socket initialized\n");
    printf("[NETWORK] Listening on port %d\n", SERVER_PORT);

    return true;
}


/* ============================================================
   Close UDP Network
   ============================================================ */

void Network_Close()
{
    if (server_socket_fd >= 0)
    {
        close(server_socket_fd);
        server_socket_fd = -1;
    }

    printf("[NETWORK] Socket closed\n");
}


/* ============================================================
   Send Message To Client
   ============================================================ */

void Network_SendToClient(
    const char* message,
    const struct sockaddr_in& client_address)
{
    if (server_socket_fd < 0)
    {
        printf("[TX ERROR] Invalid server socket\n");
        return;
    }

    ssize_t sent = sendto(
        server_socket_fd,
        message,
        strlen(message),
        0,
        (const struct sockaddr*)&client_address,
        sizeof(client_address)
    );

    char client_ip[INET_ADDRSTRLEN];

    inet_ntop(
        AF_INET,
        &client_address.sin_addr,
        client_ip,
        sizeof(client_ip)
    );

    if (sent < 0)
    {
        perror("[TX ERROR] sendto()");
    }
    else
    {
        if (strncmp(message, "STATE", 5) != 0)
        {
            printf("[TX] %s:%d <- %s (%zd bytes)\n",
                    client_ip,
                    ntohs(client_address.sin_port),
                    message,
                    sent);
        }
    }
}

/* ============================================================
   Send Message To Specific Player
   ============================================================ */

void Network_SendToPlayer(
    int player_id,
    const char* message)
{
    Player* player = Player_Get(player_id);

    if (player == NULL)
        return;

    Network_SendToClient(message, player->address);
}


/* ============================================================
   Broadcast Message To All Active Players
   ============================================================ */

void Network_Broadcast(const char* message)
{
    for (int i = 1; i <= MAX_PLAYERS; i++)
    {
        Player* player = Player_Get(i);

        if (player == NULL)
            continue;

        Network_SendToClient(message, player->address);
    }
}


/* ============================================================
   Get Socket Descriptor
   ============================================================ */

int Network_GetSocket()
{
    return server_socket_fd;
}


/* ============================================================
   Parse INPUT Packet
   (matches Unity's NetworkManager.SendInput:
    "INPUT MOVE_X=<f> RUN=<0/1> JUMP=<0/1> ATTACK=<0/1>")
   ============================================================ */

static void ProcessInputPacket(
    const char* packet,
    Player* player)
{
    if (player == NULL)
        return;

    float move_x = 0.0f;
    int run = 0;
    int jump = 0;
    int attack = 0;

    int parsed = sscanf(
        packet,
        "INPUT MOVE_X=%f RUN=%d JUMP=%d ATTACK=%d",
        &move_x,
        &run,
        &jump,
        &attack
    );

    if (parsed != 4)
    {
        printf("[SERVER] Invalid INPUT packet: %s\n", packet);
        return;
    }

    Player_UpdateInput(
        player->id,
        move_x,
        run != 0,
        jump != 0,
        attack != 0
    );
}


/* ============================================================
   Process Incoming Packets
   ============================================================ */

void Network_ProcessPackets()
{
    char buffer[BUFFER_SIZE];

    struct sockaddr_in client_address;

    socklen_t client_address_length =
        sizeof(client_address);

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        memset(&client_address, 0, sizeof(client_address));

        ssize_t received = recvfrom(
            server_socket_fd,
            buffer,
            sizeof(buffer) - 1,
            0,
            (struct sockaddr*)&client_address,
            &client_address_length
        );

        if (received < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            perror("[ERROR] recvfrom()");
            break;
        }

        if (received == 0)
            break;

        buffer[received] = '\0';

        char client_ip[INET_ADDRSTRLEN];

        inet_ntop(AF_INET,
                  &client_address.sin_addr,
                  client_ip,
                  sizeof(client_ip));

        printf("[RX] %s:%d -> %s\n",
               client_ip,
               ntohs(client_address.sin_port),
               buffer);

        Player* player =
            Player_FindByAddress(client_address);




        /* ----------------------------------------------------
           HELLO
           ---------------------------------------------------- */

        if (strncmp(buffer, "HELLO", 5) == 0)
        {
            if (player != NULL)
            {
                char response[128];

                snprintf(response,
                         sizeof(response),
                         "WELCOME PLAYER_ID=%d\n",
                         player->id);

                Network_SendToClient(response, client_address);
            }
            else
            {
                /* Do NOT assign a slot yet — wait for SELECT
                   so the client controls which player it becomes. */
                Network_SendToClient("HELLO_ACK\n", client_address);
            }
        }




        /* ----------------------------------------------------
           PING
           ---------------------------------------------------- */

        else if (strncmp(buffer, "PING", 4) == 0)
        {
            Network_SendToClient("PONG\n",
                                 client_address);
        }



        /* ----------------------------------------------------
           SELECT PLAYER
           ---------------------------------------------------- */

        else if (strncmp(buffer, "SELECT", 6) == 0)
        {
            int requested_player_id;

            if (player != NULL)
            {
                /* Already assigned to a slot — just reconfirm it */
                char response[128];

                snprintf(response,
                         sizeof(response),
                         "WELCOME PLAYER_ID=%d\n",
                         player->id);

                Network_SendToClient(response, client_address);
            }
            else if (sscanf(buffer, "SELECT %d", &requested_player_id) == 1)
            {
                int assigned_id =
                    Player_AddWithID(requested_player_id, client_address);

                if (assigned_id > 0)
                {
                    char response[128];

                    snprintf(response,
                             sizeof(response),
                             "WELCOME PLAYER_ID=%d\n",
                             assigned_id);

                    Network_SendToClient(response, client_address);

                    printf("[PLAYER] Player %d confirmed\n", assigned_id);
                }
                else
                {
                    Network_SendToClient("OCCUPIED\n", client_address);

                    printf("[SERVER] Player slot %d already occupied\n",
                           requested_player_id);
                }
            }
            else
            {
                Network_SendToClient(
                    "ERROR INVALID_SELECT\n",
                    client_address
                );
            }
        }



        /* ----------------------------------------------------
           INPUT
           ---------------------------------------------------- */

        else if (strncmp(buffer, "INPUT", 5) == 0)
        {
            if (player == NULL)
            {
                Network_SendToClient("ERROR NOT_CONNECTED\n",
                             client_address);
            }
            else
            {
                ProcessInputPacket(buffer, player);
            }
        }


        /* ----------------------------------------------------
           DISCONNECT
           ---------------------------------------------------- */

        else if (strncmp(buffer, "DISCONNECT", 10) == 0)
        {
            if (player != NULL)
            {
                Network_SendToClient("GOODBYE\n",
                                     client_address);

                Player_Remove(player->id);
            }
        }


        /* ----------------------------------------------------
           Unknown Packet
           ---------------------------------------------------- */

        else
        {
            printf("[SERVER] Unknown packet ignored\n");
        }

        client_address_length = sizeof(client_address);
    }
}
