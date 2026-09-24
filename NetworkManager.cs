using System;
using System.Collections.Generic;
using System.Globalization;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using UnityEngine;

public class NetworkManager : MonoBehaviour
{
    [Header("QNX Server")]
    [SerializeField] private string serverIP = "10.11.41.38";
    [SerializeField] private int serverPort = 7777;

    [Header("Players")]
    [SerializeField] private Transform player1Transform;
    [SerializeField] private Transform player2Transform;
    [SerializeField] private GameObject remotePlayerPrefab;

    [Header("World Objects")]
    [SerializeField] private Transform mobTransform;
    [SerializeField] private GameObject foodObject;

    [Header("Networking")]
    [SerializeField] private float inputSendInterval = 0.05f;
    [SerializeField] private bool enableNetworkLogs = true;

    private UdpClient udpClient;

    private bool socketReady = false;
    private bool connected = false;

    private int playerID = 0;
    private int selectedPlayerID = 0;

    private float inputTimer = 0f;

    private Transform localPlayer;

    private readonly Dictionary<int, GameObject> remotePlayers =
        new Dictionary<int, GameObject>();

    private readonly Dictionary<int, Vector3> remoteTargetPositions =
        new Dictionary<int, Vector3>();

    private readonly Dictionary<int, float> remoteTargetRotations =
        new Dictionary<int, float>();

    private readonly Dictionary<int, int> remotePlayerHealth =
        new Dictionary<int, int>();

    // ============================================================
    // WORLD STATE
    // ============================================================

    private Vector3 mobTargetPosition;
    private int mobDirection = 1;

    public int LocalHealth { get; private set; } = 100;

    public int PlayerID => playerID;
    public bool IsConnected => connected;

    private void Awake()
    {
        Debug.Log("[Network] NetworkManager initialized.");
    }

    private void Update()
    {
        if (!socketReady)
            return;

        ReceivePackets();

        if (connected)
        {
            SendInput();
            InterpolateRemotePlayers();
            InterpolateMob();
        }
    }

    // ============================================================
    // NETWORK CONNECTION
    // ============================================================

    public void StartNetwork(int selectedPlayer)
    {
        if (socketReady)
        {
            Debug.LogWarning("[Network] Network already started.");
            return;
        }

        if (selectedPlayer != 1 && selectedPlayer != 2)
        {
            Debug.LogError("[Network] Invalid player selection.");
            return;
        }

        selectedPlayerID = selectedPlayer;

        try
        {
            udpClient = new UdpClient(0);

            // Windows-only fix: prevent ICMP Port-Unreachable responses from poisoning
            // this UDP socket and causing a permanent ConnectionReset on Receive().
            try
            {
                const int SIO_UDP_CONNRESET = -1744830452; // 0x9800000C
                udpClient.Client.IOControl(
                    SIO_UDP_CONNRESET,
                    new byte[] { 0, 0, 0, 0 },
                    null
                );
            }
            catch (Exception ex)
            {
                // Harmless on non-Windows platforms — IOControl with this code
                // isn't supported there and isn't needed there either.
                Debug.Log($"[Network] SIO_UDP_CONNRESET not applied (expected on non-Windows): {ex.Message}");
            }

            socketReady = true;
            connected = false;
            playerID = 0;

            Debug.Log(
                $"[Network] UDP socket ready -> {serverIP}:{serverPort}"
            );

            Debug.Log(
                $"[Network] Local UDP endpoint: " +
                $"{udpClient.Client.LocalEndPoint}"
            );

            SendRaw("HELLO");
            SendRaw($"SELECT {selectedPlayer}");

            Debug.Log(
                $"[Network] Requested Player {selectedPlayer}"
            );
        }
        catch (Exception ex)
        {
            Debug.LogError(
                $"[Network] Failed to initialize: {ex}"
            );

            socketReady = false;
            connected = false;

            udpClient?.Close();
            udpClient = null;
        }
    }

    // ============================================================
    // RECEIVE PACKETS
    // ============================================================

    private void ReceivePackets()
    {
        if (udpClient == null || !socketReady)
            return;

        try
        {
            while (udpClient.Available > 0)
            {
                IPEndPoint remoteEndPoint =
                    new IPEndPoint(IPAddress.Any, 0);

                byte[] data =
                    udpClient.Receive(ref remoteEndPoint);

                Debug.Log(
                    $"[Network RAW RX] Received {data.Length} bytes"
                );

                string message =
                    Encoding.UTF8.GetString(data).Trim();

                if (enableNetworkLogs)
                {
                    Debug.Log(
                        $"[Network RX] {message} " +
                        $"FROM {remoteEndPoint.Address}:" +
                        $"{remoteEndPoint.Port}"
                    );
                }

                HandleServerMessage(message);
            }
        }
        catch (SocketException ex)
        {
            Debug.LogError(
                $"[Network] Receive SocketException: " +
                $"{ex.SocketErrorCode} - {ex.Message}"
            );
        }
        catch (Exception ex)
        {
            Debug.LogError(
                $"[Network] Receive error: {ex}"
            );
        }
    }

    // ============================================================
    // SERVER MESSAGE DISPATCHER
    // ============================================================

    private void HandleServerMessage(string message)
    {
        if (string.IsNullOrWhiteSpace(message))
            return;

        // WELCOME
        if (message.StartsWith("WELCOME"))
        {
            HandleWelcome(message);
            return;
        }

        // PLAYER STATE
        if (message.StartsWith("STATE"))
        {
            HandleState(message);
            return;
        }

        // MOB STATE
        if (message.StartsWith("MOB_STATE"))
        {
            HandleMobState(message);
            return;
        }

        // FOOD STATE
        if (message.StartsWith("FOOD_STATE"))
        {
            HandleFoodState(message);
            return;
        }

        // PLAYER JOINED
        if (message.StartsWith("PLAYER_JOINED"))
        {
            HandlePlayerJoined(message);
            return;
        }

        // PLAYER LEFT
        if (message.StartsWith("PLAYER_LEFT"))
        {
            HandlePlayerLeft(message);
            return;
        }

        // OLD UPDATE
        if (message.StartsWith("UPDATE"))
        {
            HandleUpdate(message);
            return;
        }

        // OCCUPIED
        if (message.StartsWith("OCCUPIED"))
        {
            HandleOccupied(message);
            return;
        }

        // SERVER FULL
        if (message == "SERVER_FULL")
        {
            HandleServerFull();
            return;
        }

        // PONG
        if (message == "PONG")
        {
            Debug.Log("[Network] PONG received.");
            return;
        }

        // ERROR
        if (message.StartsWith("ERROR"))
        {
            Debug.LogError(
                $"[Network] Server error: {message}"
            );
            return;
        }

        Debug.LogWarning(
            $"[Network] Unknown server message: {message}"
        );
    }

    // ============================================================
    // WELCOME
    // ============================================================

    private void HandleWelcome(string message)
    {
        string[] parts = message.Split(' ');

        int receivedID = -1;

        foreach (string part in parts)
        {
            if (part.StartsWith("PLAYER_ID="))
            {
                int.TryParse(
                    part.Substring("PLAYER_ID=".Length),
                    out receivedID
                );

                break;
            }
        }

        if (receivedID != 1 && receivedID != 2)
        {
            Debug.LogWarning(
                $"[Network] Invalid WELCOME packet: {message}"
            );

            return;
        }

        playerID = receivedID;
        connected = true;

        AssignLocalPlayer();

        Debug.Log(
            $"[Network] Connected! Player ID = {playerID}"
        );
    }

    // ============================================================
    // ASSIGN LOCAL PLAYER
    // ============================================================

    private void AssignLocalPlayer()
    {
        if (playerID == 1)
        {
            localPlayer = player1Transform;

            Debug.Log(
                "[Network] Local player assigned: Player 1"
            );
        }
        else if (playerID == 2)
        {
            localPlayer = player2Transform;

            Debug.Log(
                "[Network] Local player assigned: Player 2"
            );
        }

        if (localPlayer == null)
        {
            Debug.LogError(
                $"[Network] Local Player Transform for " +
                $"Player {playerID} is not assigned."
            );

            return;
        }

        Debug.Log(
            $"[Network] Local player assigned successfully: " +
            $"{localPlayer.name}"
        );
    }

    // ============================================================
    // AUTHORITATIVE PLAYER STATE
    // ============================================================

    private void HandleState(string message)
    {
        string[] parts = message.Split(' ');

        int receivedPlayerID = -1;

        float x = 0f;
        float y = 0f;
        float rotation = 0f;

        int hp = 100;
        int grounded = 1;

        foreach (string part in parts)
        {
            if (part.StartsWith("PLAYER_ID="))
            {
                int.TryParse(
                    part.Substring("PLAYER_ID=".Length),
                    out receivedPlayerID
                );
            }
            else if (part.StartsWith("ID="))
            {
                int.TryParse(
                    part.Substring(3),
                    out receivedPlayerID
                );
            }
            else if (part.StartsWith("X="))
            {
                float.TryParse(
                    part.Substring(2),
                    NumberStyles.Float,
                    CultureInfo.InvariantCulture,
                    out x
                );
            }
            else if (part.StartsWith("Y="))
            {
                float.TryParse(
                    part.Substring(2),
                    NumberStyles.Float,
                    CultureInfo.InvariantCulture,
                    out y
                );
            }
            else if (part.StartsWith("ROT="))
            {
                float.TryParse(
                    part.Substring(4),
                    NumberStyles.Float,
                    CultureInfo.InvariantCulture,
                    out rotation
                );
            }
            else if (part.StartsWith("HP="))
            {
                int.TryParse(
                    part.Substring(3),
                    out hp
                );
            }
            else if (part.StartsWith("GROUNDED="))
            {
                int.TryParse(
                    part.Substring("GROUNDED=".Length),
                    out grounded
                );
            }
        }

        if (receivedPlayerID <= 0)
        {
            Debug.LogWarning(
                $"[Network] Invalid STATE packet: {message}"
            );

            return;
        }

        Vector3 serverPosition = new Vector3(x, y, 0f);

        if (enableNetworkLogs)
        {
            Debug.Log(
                $"[Network] Authoritative State: " +
                $"Player={receivedPlayerID} " +
                $"X={x:F2} " +
                $"Y={y:F2} " +
                $"ROT={rotation:F2} " +
                $"HP={hp} " +
                $"GROUNDED={grounded}"
            );
        }

        // --------------------------------------------------------
        // LOCAL PLAYER
        // --------------------------------------------------------

        if (receivedPlayerID == playerID)
        {
            // Store authoritative health.
            LocalHealth = hp;

            if (localPlayer != null)
            {
                // Synchronize server health with the local Damageable component
                Damageable localDamageable = localPlayer.GetComponent<Damageable>();
                if (localDamageable != null && localDamageable.Health != hp)
                {
                    localDamageable.Health = hp;
                }

                // Server owns player position.
                localPlayer.position = serverPosition;

                // Server owns player rotation.
                localPlayer.rotation = Quaternion.Euler(0f, 0f, rotation);

                // Disable Local Physics Gravity
                Rigidbody2D rb = localPlayer.GetComponent<Rigidbody2D>();
                if (rb != null)
                {
                    rb.gravityScale = 0f;
                    rb.linearVelocity = Vector2.zero;
                }

                // Update Player Animation
                Animator anim = localPlayer.GetComponent<Animator>();
                if (anim != null)
                {
                    anim.SetBool(
                        AnimationStrings.isGrounded,
                        grounded == 1
                    );
                }
            }

            return;
        }

        // --------------------------------------------------------
        // REMOTE PLAYER
        // --------------------------------------------------------

        if (!remotePlayers.ContainsKey(receivedPlayerID))
        {
            SpawnRemotePlayer(receivedPlayerID);
        }

        if (!remotePlayers.ContainsKey(receivedPlayerID))
            return;

        remoteTargetPositions[receivedPlayerID] = serverPosition;
        remoteTargetRotations[receivedPlayerID] = rotation;
        remotePlayerHealth[receivedPlayerID] = hp;
    }

    // ============================================================
    // MOB STATE
    // ============================================================

    private void HandleMobState(string message)
    {
        string[] parts = message.Split(' ');

        float x = 0f;
        float y = 0f;
        int direction = 1;

        foreach (string part in parts)
        {
            if (part.StartsWith("X="))
            {
                float.TryParse(
                    part.Substring(2),
                    NumberStyles.Float,
                    CultureInfo.InvariantCulture,
                    out x
                );
            }
            else if (part.StartsWith("Y="))
            {
                float.TryParse(
                    part.Substring(2),
                    NumberStyles.Float,
                    CultureInfo.InvariantCulture,
                    out y
                );
            }
            else if (part.StartsWith("DIR="))
            {
                int.TryParse(
                    part.Substring(4),
                    out direction
                );
            }
        }

        mobTargetPosition = new Vector3(x, y, 0f);
        mobDirection = direction;
    }

    // ============================================================
    // FOOD STATE
    // ============================================================

    private void HandleFoodState(string message)
    {
        string[] parts = message.Split(' ');

        float x = 0f;
        float y = 0f;
        int active = 1;

        foreach (string part in parts)
        {
            if (part.StartsWith("X="))
            {
                float.TryParse(
                    part.Substring(2),
                    NumberStyles.Float,
                    CultureInfo.InvariantCulture,
                    out x
                );
            }
            else if (part.StartsWith("Y="))
            {
                float.TryParse(
                    part.Substring(2),
                    NumberStyles.Float,
                    CultureInfo.InvariantCulture,
                    out y
                );
            }
            else if (part.StartsWith("ACTIVE="))
            {
                int.TryParse(
                    part.Substring(7),
                    out active
                );
            }
        }

        if (foodObject != null)
        {
            foodObject.transform.position = new Vector3(x, y, 0f);
            foodObject.SetActive(active == 1);
        }
    }

    // ============================================================
    // PLAYER JOINED
    // ============================================================

    private void HandlePlayerJoined(string message)
    {
        string[] parts = message.Split(' ');

        int joinedPlayerID = 0;

        foreach (string part in parts)
        {
            if (part.StartsWith("PLAYER_ID="))
            {
                int.TryParse(
                    part.Substring("PLAYER_ID=".Length),
                    out joinedPlayerID
                );
            }
            else if (part.StartsWith("ID="))
            {
                int.TryParse(
                    part.Substring(3),
                    out joinedPlayerID
                );
            }
            else if (int.TryParse(part, out int parsedID))
            {
                joinedPlayerID = parsedID;
            }
        }

        if (joinedPlayerID <= 0)
        {
            Debug.LogWarning(
                $"[Network] Invalid PLAYER_JOINED packet: {message}"
            );
            return;
        }

        Debug.Log(
            $"[Network] Player {joinedPlayerID} joined the game."
        );

        if (joinedPlayerID == playerID)
            return;

        SpawnRemotePlayer(joinedPlayerID);
    }

    // ============================================================
    // PLAYER LEFT
    // ============================================================

    private void HandlePlayerLeft(string message)
    {
        string[] parts = message.Split(' ');

        int leavingPlayerID = 0;

        foreach (string part in parts)
        {
            if (part.StartsWith("PLAYER_ID="))
            {
                int.TryParse(
                    part.Substring("PLAYER_ID=".Length),
                    out leavingPlayerID
                );
            }
            else if (part.StartsWith("ID="))
            {
                int.TryParse(
                    part.Substring(3),
                    out leavingPlayerID
                );
            }
            else if (int.TryParse(part, out int parsedID))
            {
                leavingPlayerID = parsedID;
            }
        }

        if (leavingPlayerID <= 0)
            return;

        RemoveRemotePlayer(leavingPlayerID);
    }

    // ============================================================
    // SPAWN REMOTE PLAYER
    // ============================================================

    private void SpawnRemotePlayer(int remotePlayerID)
    {
        if (remotePlayerID <= 0 || remotePlayerID == playerID)
            return;

        if (remotePlayers.ContainsKey(remotePlayerID))
            return;

        if (remotePlayerPrefab == null)
        {
            Debug.LogError(
                "[Network] Remote Player Prefab is not assigned."
            );
            return;
        }

        Vector3 spawnPosition = Vector3.zero;
        Quaternion spawnRotation = Quaternion.identity;

        if (remotePlayerID == 1 && player1Transform != null)
        {
            spawnPosition = player1Transform.position;
            spawnRotation = player1Transform.rotation;
        }
        else if (remotePlayerID == 2 && player2Transform != null)
        {
            spawnPosition = player2Transform.position;
            spawnRotation = player2Transform.rotation;
        }

        GameObject newRemotePlayer = Instantiate(
            remotePlayerPrefab,
            spawnPosition,
            spawnRotation
        );

        newRemotePlayer.name = $"RemotePlayer_{remotePlayerID}";

        // Remote players are kinematic puppets
        Rigidbody2D remoteRb = newRemotePlayer.GetComponent<Rigidbody2D>();
        if (remoteRb != null)
        {
            remoteRb.gravityScale = 0f;
            remoteRb.bodyType = RigidbodyType2D.Kinematic;
            remoteRb.linearVelocity = Vector2.zero;
        }

        NetworkPlayer networkPlayer = newRemotePlayer.GetComponent<NetworkPlayer>();
        if (networkPlayer != null)
        {
            networkPlayer.playerId = remotePlayerID;
        }
        else
        {
            Debug.LogWarning(
                "[Network] Remote Player Prefab does not have a NetworkPlayer component."
            );
        }

        remotePlayers.Add(remotePlayerID, newRemotePlayer);
        remoteTargetPositions.Add(remotePlayerID, spawnPosition);
        remoteTargetRotations.Add(remotePlayerID, spawnRotation.eulerAngles.z);
        remotePlayerHealth.Add(remotePlayerID, 100);

        Debug.Log(
            $"[Network] Remote Player {remotePlayerID} spawned."
        );
    }

    // ============================================================
    // REMOVE REMOTE PLAYER
    // ============================================================

    private void RemoveRemotePlayer(int remotePlayerID)
    {
        if (!remotePlayers.ContainsKey(remotePlayerID))
            return;

        GameObject remotePlayer = remotePlayers[remotePlayerID];

        if (remotePlayer != null)
        {
            Destroy(remotePlayer);
        }

        remotePlayers.Remove(remotePlayerID);
        remoteTargetPositions.Remove(remotePlayerID);
        remoteTargetRotations.Remove(remotePlayerID);
        remotePlayerHealth.Remove(remotePlayerID);

        Debug.Log(
            $"[Network] Remote Player {remotePlayerID} removed."
        );
    }

    // ============================================================
    // REMOTE PLAYER INTERPOLATION
    // ============================================================

    private void InterpolateRemotePlayers()
    {
        float interpolationSpeed = 12f;

        foreach (var pair in remotePlayers)
        {
            int id = pair.Key;
            GameObject remotePlayer = pair.Value;

            if (remotePlayer == null)
                continue;

            if (!remoteTargetPositions.ContainsKey(id))
                continue;

            Vector3 targetPosition = remoteTargetPositions[id];

            remotePlayer.transform.position = Vector3.Lerp(
                remotePlayer.transform.position,
                targetPosition,
                interpolationSpeed * Time.deltaTime
            );

            if (remoteTargetRotations.ContainsKey(id))
            {
                float targetRotation = remoteTargetRotations[id];

                Quaternion targetQuaternion = Quaternion.Euler(
                    0f,
                    0f,
                    targetRotation
                );

                remotePlayer.transform.rotation = Quaternion.Lerp(
                    remotePlayer.transform.rotation,
                    targetQuaternion,
                    interpolationSpeed * Time.deltaTime
                );
            }
        }
    }

    // ============================================================
    // MOB INTERPOLATION
    // ============================================================

    private void InterpolateMob()
    {
        if (mobTransform == null)
            return;

        mobTransform.position = Vector3.Lerp(
            mobTransform.position,
            mobTargetPosition,
            12f * Time.deltaTime
        );

        Vector3 scale = mobTransform.localScale;
        scale.x = Mathf.Abs(scale.x) * (mobDirection >= 0 ? 1f : -1f);
        mobTransform.localScale = scale;
    }

    // ============================================================
    // OLD UPDATE COMPATIBILITY
    // ============================================================

    private void HandleUpdate(string message)
    {
        string convertedMessage = message.Replace("UPDATE", "STATE");
        convertedMessage = convertedMessage.Replace("ID=", "PLAYER_ID=");

        HandleState(convertedMessage);
    }

    // ============================================================
    // AUTHORITATIVE INPUT SENDING
    // ============================================================

    private void SendInput()
    {
        if (!connected || udpClient == null || localPlayer == null)
            return;

        inputTimer += Time.deltaTime;

        if (inputTimer < inputSendInterval)
            return;

        inputTimer = 0f;

        PlayerController controller = localPlayer.GetComponent<PlayerController>();

        if (controller == null)
            return;

        float moveX = controller.MoveInput.x;
        int run = controller.IsRunning ? 1 : 0;
        int jump = controller.ConsumeJumpRequest() ? 1 : 0;
        int attack = controller.ConsumeAttackRequest() ? 1 : 0;

        string packet =
            $"INPUT MOVE_X={moveX.ToString(CultureInfo.InvariantCulture)} " +
            $"RUN={run} " +
            $"JUMP={jump} " +
            $"ATTACK={attack}";

        SendRaw(packet);
    }

    // ============================================================
    // OCCUPIED
    // ============================================================

    private void HandleOccupied(string message)
    {
        Debug.LogWarning(
            $"[Network] Requested player is occupied: {message}"
        );

        connected = false;
        playerID = 0;
        localPlayer = null;
    }

    // ============================================================
    // SERVER FULL
    // ============================================================

    private void HandleServerFull()
    {
        Debug.LogWarning("[Network] Server is full.");

        connected = false;
        playerID = 0;
        localPlayer = null;
    }

    // ============================================================
    // SEND RAW UDP
    // ============================================================

    public void SendRaw(string message)
    {
        if (!socketReady || udpClient == null)
        {
            Debug.LogWarning(
                "[Network] Cannot send - socket not ready."
            );

            return;
        }

        try
        {
            byte[] data = Encoding.UTF8.GetBytes(message);

            IPEndPoint serverEndPoint = new IPEndPoint(
                IPAddress.Parse(serverIP),
                serverPort
            );

            int sent = udpClient.Send(data, data.Length, serverEndPoint);

            if (enableNetworkLogs)
            {
                Debug.Log(
                    $"[Network TX] {message} " +
                    $"({sent} bytes) -> " +
                    $"{serverIP}:{serverPort}"
                );
            }
        }
        catch (Exception ex)
        {
            Debug.LogError(
                $"[Network TX ERROR] {ex}"
            );
        }
    }

    // ============================================================
    // DISCONNECT
    // ============================================================

    public void Disconnect()
    {
        if (!socketReady)
            return;

        try
        {
            if (connected)
            {
                SendRaw("DISCONNECT");
            }

            connected = false;
            playerID = 0;
            selectedPlayerID = 0;
            localPlayer = null;
            LocalHealth = 100;

            foreach (var pair in remotePlayers)
            {
                if (pair.Value != null)
                {
                    Destroy(pair.Value);
                }
            }

            remotePlayers.Clear();
            remoteTargetPositions.Clear();
            remoteTargetRotations.Clear();
            remotePlayerHealth.Clear();

            udpClient?.Close();
            udpClient = null;
            socketReady = false;

            Debug.Log("[Network] Disconnected from server.");
        }
        catch (Exception ex)
        {
            Debug.LogError(
                $"[Network] Disconnect error: {ex.Message}"
            );
        }
    }

    // ============================================================
    // APPLICATION QUIT
    // ============================================================

    private void OnApplicationQuit()
    {
        Disconnect();
    }
}
